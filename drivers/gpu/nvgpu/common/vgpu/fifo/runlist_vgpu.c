/*
 * Virtualized GPU Runlist
 *
 * Copyright (c) 2019-2021, NVIDIA CORPORATION.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <nvgpu/kmem.h>
#include <nvgpu/string.h>
#include <nvgpu/bug.h>
#include <nvgpu/vgpu/vgpu_ivc.h>
#include <nvgpu/vgpu/vgpu.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/channel.h>
#include <nvgpu/runlist.h>

#include "runlist_vgpu.h"
#include "common/vgpu/ivc/comm_vgpu.h"

static int vgpu_submit_runlist(struct gk20a *g, u64 handle, u8 runlist_id,
			       u16 *runlist, u32 num_entries)
{
	struct tegra_vgpu_cmd_msg msg;
	struct tegra_vgpu_runlist_params *p;
	int err;
	void *oob_handle;
	void *oob;
	size_t size, oob_size;

	oob_handle = vgpu_ivc_oob_get_ptr(vgpu_ivc_get_server_vmid(),
			TEGRA_VGPU_QUEUE_CMD,
			&oob, &oob_size);
	if (!oob_handle) {
		return -EINVAL;
	}

	size = sizeof(*runlist) * num_entries;
	if (oob_size < size) {
		err = -ENOMEM;
		goto done;
	}

	msg.cmd = TEGRA_VGPU_CMD_SUBMIT_RUNLIST;
	msg.handle = handle;
	p = &msg.params.runlist;
	p->runlist_id = runlist_id;
	p->num_entries = num_entries;

	nvgpu_memcpy((u8 *)oob, (u8 *)runlist, size);
	err = vgpu_comm_sendrecv(&msg, sizeof(msg), sizeof(msg));

	err = (err || msg.ret) ? -1 : 0;

done:
	vgpu_ivc_oob_put_ptr(oob_handle);
	return err;
}

static bool vgpu_runlist_modify_active_locked(struct gk20a *g, u32 runlist_id,
					    struct nvgpu_channel *ch, bool add)
{
	struct nvgpu_fifo *f = &g->fifo;
	struct nvgpu_runlist *runlist;

	runlist = f->runlists[runlist_id];

	if (add) {
		if (nvgpu_test_and_set_bit(ch->chid,
				runlist->active_channels)) {
			return false;
			/* was already there */
		}
	} else {
		if (!nvgpu_test_and_clear_bit(ch->chid,
				runlist->active_channels)) {
			/* wasn't there */
			return false;
		}
	}

	return true;
}

static void vgpu_runlist_reconstruct_locked(struct gk20a *g, u32 runlist_id,
				     bool add_entries)
{
	struct nvgpu_fifo *f = &g->fifo;
	struct nvgpu_runlist *runlist;

	runlist = f->runlists[runlist_id];

	if (add_entries) {
		u16 *runlist_entry;
		u32 count = 0;
		unsigned long chid;

		runlist_entry = runlist->mem[0].cpu_va;

		nvgpu_assert(f->num_channels <= (unsigned int)U16_MAX);
		for_each_set_bit(chid,
				runlist->active_channels, f->num_channels) {
			nvgpu_log_info(g, "add channel %lu to runlist", chid);
			*runlist_entry++ = (u16)chid;
			count++;
		}

		runlist->count = count;
	} else {
		runlist->count = 0;
	}
}

static int vgpu_runlist_update_locked(struct gk20a *g, u32 runlist_id,
					struct nvgpu_channel *ch, bool add,
					bool wait_for_finish)
{
	struct nvgpu_fifo *f = &g->fifo;
	struct nvgpu_runlist *runlist;
	bool add_entries;

	nvgpu_log_fn(g, " ");

	if (ch != NULL) {
		bool update = vgpu_runlist_modify_active_locked(g, runlist_id,
				ch, add);
		if (!update) {
			/* no change in runlist contents */
			return 0;
		}
		/* had a channel to update, so reconstruct */
		add_entries = true;
	} else {
		/* no channel; add means update all, !add means clear all */
		add_entries = add;
	}

	runlist = f->runlists[runlist_id];

	vgpu_runlist_reconstruct_locked(g, runlist_id, add_entries);

	return vgpu_submit_runlist(g, vgpu_get_handle(g), runlist_id,
				runlist->mem[0].cpu_va, runlist->count);
}

/* add/remove a channel from runlist
   special cases below: runlist->active_channels will NOT be changed.
   (ch == NULL && !add) means remove all active channels from runlist.
   (ch == NULL &&  add) means restore all active channels on runlist. */
static int vgpu_runlist_do_update(struct gk20a *g, struct nvgpu_runlist *rl,
				struct nvgpu_channel *ch,
				bool add, bool wait_for_finish)
{
	u32 ret = 0;

	nvgpu_log_fn(g, " ");

	nvgpu_mutex_acquire(&rl->runlist_lock);

	ret = vgpu_runlist_update_locked(g, rl->id, ch, add,
					wait_for_finish);

	nvgpu_mutex_release(&rl->runlist_lock);
	return ret;
}

int vgpu_runlist_update(struct gk20a *g, struct nvgpu_runlist *rl,
			struct nvgpu_channel *ch,
			bool add, bool wait_for_finish)
{
	nvgpu_assert(ch != NULL);

	return vgpu_runlist_do_update(g, rl, ch, add, wait_for_finish);
}

int vgpu_runlist_reload(struct gk20a *g, struct nvgpu_runlist *rl,
			bool add, bool wait_for_finish)
{
	return vgpu_runlist_do_update(g, rl, NULL, add, wait_for_finish);
}

u32 vgpu_runlist_length_max(struct gk20a *g)
{
	struct vgpu_priv_data *priv = vgpu_get_priv_data(g);

	return priv->constants.num_channels;
}

u32 vgpu_runlist_entry_size(struct gk20a *g)
{
	/*
	 * This is not the HW format you're looking for (see
	 * vgpu_fifo_update_runlist_locked(), vgpu_submit_runlist())
	 */
	return (u32)sizeof(u16);
}
