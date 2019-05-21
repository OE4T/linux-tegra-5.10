/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <unit/io.h>
#include <unit/unit.h>

#include <nvgpu/channel.h>
#include <nvgpu/tsg.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/fifo/userd.h>
#include <nvgpu/runlist.h>
#include <nvgpu/fuse.h>
#include <nvgpu/dma.h>
#include <nvgpu/gr/ctx.h>

#include "common/gr/ctx_priv.h"

#include "hal/fifo/tsg_gk20a.h"

#include "hal/init/hal_gv11b.h"

#include "../nvgpu-fifo-gv11b.h"

#ifdef TSG_UNIT_DEBUG
#define unit_verbose	unit_info
#else
#define unit_verbose(unit, msg, ...) \
	do { \
		if (0) { \
			unit_info(unit, msg, ##__VA_ARGS__); \
		} \
	} while (0)
#endif

struct test_tsg_args {
	bool init_done;
	struct nvgpu_tsg *tsg;
	struct nvgpu_channel *ch;
};

struct test_tsg_args test_args = {
	.init_done = false,
};

/* structure set by stubs */
struct test_stub_rc {
	u32 branches;
	u32 chid;
	u32 tsgid;
	u32 count;
};

struct test_stub_rc stub_rc = {
	.branches = 0,
};

static inline void reset_stub_rc(void)
{
	memset(&stub_rc, 0, sizeof(stub_rc));
}

static int branches_strn(char *dst, size_t size,
		const char *labels[], u32 branches)
{
	int i;
	char *p = dst;
	int len;

	for (i = 0; i < 32; i++) {
		if (branches & BIT(i)) {
			len = snprintf(p, size, "%s ", labels[i]);
			size -= len;
			p += len;
		}
	}

	return (p - dst);
}

/* not MT-safe */
static char *branches_str(u32 branches, const char *labels[])
{
	static char buf[256];

	memset(buf, 0, sizeof(buf));
	branches_strn(buf, sizeof(buf), labels, branches);
	return buf;
}

/*
 * If taken, some branches are final, e.g. the function exits.
 * There is no need to test subsequent branches combinations,
 * if one final branch is taken.
 *
 * We want to skip the subtest if:
 * - it has at least one final branch
 * - it is supposed to test some branches after this final branch
 *
 * Parameters:
 * branches		bitmask of branches to be taken for one subtest
 * final_branches	bitmask of final branches
 *
 * Note: the assumption is that branches are numbered in their
 * order of appearance in the function to be tested.
 */
static bool pruned(u32 branches, u32 final_branches)
{
	u32 match = branches & final_branches;
	int bit;

	/* Does the subtest have one final branch ? */
	if (match == 0U) {
		return false;
	}
	bit = ffs(match) - 1;

	/*
	 * Skip the test if it attempts to test some branches
	 * after this final branch.
	 */
	return (branches > BIT(bit));
}

/* test implementations of some hals */
static u32 stub_gv11b_gr_init_get_no_of_sm(struct gk20a *g)
{
	return 8;
}

#ifdef NVGPU_USERD
static int stub_userd_setup_sw(struct gk20a *g)
{
	struct nvgpu_fifo *f = &g->fifo;
	int err;

	f->userd_entry_size = g->ops.userd.entry_size(g);

	err = nvgpu_userd_init_slabs(g);
	if (err != 0) {
		nvgpu_err(g, "failed to init userd support");
		return err;
	}

	return 0;
}
#endif

static int test_fifo_init_support(struct unit_module *m,
		struct gk20a *g, void *args)
{
	struct test_tsg_args *t = args;
	int err;

	if (t->init_done) {
		unit_return_fail(m, "init already done");
	}

	err = test_fifo_setup_gv11b_reg_space(m, g);
	if (err != 0) {
		goto fail;
	}

	gv11b_init_hal(g);
	g->ops.fifo.init_fifo_setup_hw = NULL;
	g->ops.gr.init.get_no_of_sm = stub_gv11b_gr_init_get_no_of_sm;
	g->ops.tsg.init_eng_method_buffers = NULL;

#ifdef NVGPU_USERD
	/*
	 * Regular USERD init requires bar1.vm to be initialized
	 * Use a stub in unit tests, since it will be disabled in
	 * safety build anyway.
	 */
	g->ops.userd.setup_sw = stub_userd_setup_sw;
#endif

	err = nvgpu_fifo_init_support(g);
	if (err != 0) {
		test_fifo_cleanup_gv11b_reg_space(m, g);
		goto fail;
	}

	/* Do not allocate from vidmem */
	nvgpu_set_enabled(g, NVGPU_MM_UNIFIED_MEMORY, true);

	t->init_done = true;

	return UNIT_SUCCESS;

fail:
	return UNIT_FAIL;
}

#define F_TSG_OPEN_ACQUIRE_CH_FAIL	BIT(0)
#define F_TSG_OPEN_SM_FAIL		BIT(1)
#define F_TSG_OPEN_LAST			BIT(2)

static const char const *f_tsg_open[] = {
	"acquire_ch_fail",
	"sm_fail",
};

static u32 stub_gr_init_get_no_of_sm_0(struct gk20a *g)
{
	return 0;
}

static int test_tsg_open(struct unit_module *m,
		struct gk20a *g, void *args)
{
	struct nvgpu_fifo *f = &g->fifo;
	struct gpu_ops gops = g->ops;
	u32 num_channels = f->num_channels;
	struct nvgpu_tsg *tsg;
	u32 branches;
	int rc = UNIT_FAIL;
	u32 fail = F_TSG_OPEN_ACQUIRE_CH_FAIL |
			F_TSG_OPEN_SM_FAIL;

	for (branches = 0U; branches < F_TSG_OPEN_LAST; branches++) {

		reset_stub_rc();
		unit_verbose(m, "%s branches=%s\n", __func__,
			branches_str(branches, f_tsg_open));

		f->num_channels =
			branches & F_TSG_OPEN_ACQUIRE_CH_FAIL ?
			0U : num_channels;

		g->ops.gr.init.get_no_of_sm =
			branches & F_TSG_OPEN_SM_FAIL ?
			stub_gr_init_get_no_of_sm_0 : gops.gr.init.get_no_of_sm;

		tsg = nvgpu_tsg_open(g, getpid());

		if (branches & fail) {
			if (tsg != NULL) {
				goto done;
			}
		} else {
			if (tsg == NULL) {
				goto done;
			}
			nvgpu_ref_put(&tsg->refcount, nvgpu_tsg_release);
			tsg = NULL;
		}
	}
	rc = UNIT_SUCCESS;

done:
	if (rc != UNIT_SUCCESS) {
		unit_err(m, "%s branches=%s\n", __func__,
			branches_str(branches, f_tsg_open));
	}

	if (tsg != NULL) {
		nvgpu_ref_put(&tsg->refcount, nvgpu_tsg_release);
	}
	g->ops = gops;
	f->num_channels = num_channels;
	return rc;
}

#define F_TSG_BIND_CHANNEL_CH_BOUND		BIT(0)
#define F_TSG_BIND_CHANNEL_RL_MISMATCH		BIT(1)
#define F_TSG_BIND_CHANNEL_ACTIVE		BIT(2)
#define F_TSG_BIND_CHANNEL_BIND_HAL		BIT(3)
#define F_TSG_BIND_CHANNEL_ENG_METHOD_BUFFER	BIT(3)
#define F_TSG_BIND_CHANNEL_LAST			BIT(4)

static const char const *f_tsg_bind[] = {
	"ch_bound",
	"rl_mismatch",
	"active",
	"bind_hal",
	"eng_method_buffer",
};

static int test_tsg_bind_channel(struct unit_module *m,
		struct gk20a *g, void *args)
{
	struct nvgpu_fifo *f = &g->fifo;
	struct gpu_ops gops = g->ops;
	struct nvgpu_tsg *tsg, tsg_save;
	struct nvgpu_channel *chA, *chB, *ch;
	struct nvgpu_runlist_info *runlist;
	u32 branches;
	int rc = UNIT_FAIL;
	int err;
	u32 prune = F_TSG_BIND_CHANNEL_CH_BOUND |
		    F_TSG_BIND_CHANNEL_RL_MISMATCH |
		    F_TSG_BIND_CHANNEL_ACTIVE;

	tsg = nvgpu_tsg_open(g, getpid());
	chA = gk20a_open_new_channel(g, ~0U, false, getpid(), getpid());
	chB = gk20a_open_new_channel(g, ~0U, false, getpid(), getpid());

	if ((tsg == NULL) || (chA == NULL) || (chB == NULL)) {
		goto done;
	}

	if (nvgpu_tsg_bind_channel(tsg, chA) != 0) {
		unit_err(m, "%s failed to bind chA", __func__);
		goto done;
	}

	tsg_save = *tsg;

	for (branches = 0U; branches < F_TSG_BIND_CHANNEL_LAST; branches++) {

		if (pruned(branches, prune)) {
			unit_verbose(m, "%s branches=%s (pruned)\n", __func__,
				branches_str(branches, f_tsg_bind));
			continue;
		}
		reset_stub_rc();
		ch = chB;

		/* ch already bound */
		if (branches & F_TSG_BIND_CHANNEL_CH_BOUND) {
			ch = chA;
		}

		/* runlist id mismatch */
		tsg->runlist_id =
			branches & F_TSG_BIND_CHANNEL_RL_MISMATCH ?
			ch->runlist_id + 1 : tsg_save.runlist_id;

		/* ch already already active */
		runlist = &f->active_runlist_info[tsg->runlist_id];
		if (branches & F_TSG_BIND_CHANNEL_ACTIVE) {
			set_bit((int)ch->chid, runlist->active_channels);
		} else {
			clear_bit((int)ch->chid, runlist->active_channels);
		}

		g->ops.tsg.bind_channel =
			branches & F_TSG_BIND_CHANNEL_BIND_HAL ?
			gops.tsg.bind_channel : NULL;

		g->ops.tsg.bind_channel_eng_method_buffers =
			branches & F_TSG_BIND_CHANNEL_ENG_METHOD_BUFFER ?
			gops.tsg.bind_channel_eng_method_buffers : NULL;

		unit_verbose(m, "%s branches=%s\n", __func__,
			branches_str(branches, f_tsg_bind));

		err = nvgpu_tsg_bind_channel(tsg, ch);

		if (branches & (F_TSG_BIND_CHANNEL_CH_BOUND|
				F_TSG_BIND_CHANNEL_RL_MISMATCH|
				F_TSG_BIND_CHANNEL_ACTIVE)) {
			if (err == 0) {
				goto done;
			}
		} else {
			if (err != 0) {
				goto done;
			}

			if (nvgpu_list_empty(&tsg->ch_list)) {
				goto done;
			}

			err = nvgpu_tsg_unbind_channel(tsg, ch);
			if (err != 0 || ch->tsgid != NVGPU_INVALID_TSG_ID) {
				unit_err(m, "%s failed to unbind", __func__);
				goto done;
			}
		}
	}

	rc = UNIT_SUCCESS;

done:
	if (rc != UNIT_SUCCESS) {
		unit_err(m, "%s branches=%s\n", __func__,
			branches_str(branches, f_tsg_bind));
	}

	if (chA != NULL) {
		nvgpu_channel_close(chA);
	}
	if (chB != NULL) {
		nvgpu_channel_close(chB);
	}
	if (tsg != NULL) {
		nvgpu_ref_put(&tsg->refcount, nvgpu_tsg_release);
	}
	g->ops = gops;
	return rc;
}

#define F_TSG_UNBIND_CHANNEL_UNSERVICEABLE		BIT(0)
#define F_TSG_UNBIND_CHANNEL_PREEMPT_TSG_FAIL		BIT(1)
#define F_TSG_UNBIND_CHANNEL_CHECK_HW_STATE_FAIL	BIT(2)
#define F_TSG_UNBIND_CHANNEL_RUNLIST_UPDATE_FAIL	BIT(3)
#define F_TSG_UNBIND_CHANNEL_UNBIND_HAL			BIT(4)
#define F_TSG_UNBIND_CHANNEL_LAST			BIT(5)

static const char const *f_tsg_unbind[] = {
	"ch_timedout",
	"preempt_tsg_fail",
	"check_hw_state_fail",
	"runlist_update_fail"
};

static int stub_fifo_preempt_tsg_EINVAL(
		struct gk20a *g, struct nvgpu_tsg *tsg)
{
	return -EINVAL;
}

static int stub_tsg_unbind_channel_check_hw_state_EINVAL(
		struct nvgpu_tsg *tsg, struct nvgpu_channel *ch)
{
	return -EINVAL;
}

static int stub_tsg_unbind_channel(struct nvgpu_tsg *tsg,
		struct nvgpu_channel *ch)
{
	if (ch->tsgid != tsg->tsgid) {
		return -EINVAL;
	}

	return 0;
}

static int stub_runlist_update_for_channel_EINVAL(
		struct gk20a *g, u32 runlist_id,
		struct nvgpu_channel *ch, bool add, bool wait_for_finish)
{
	return -EINVAL;
}

static int test_tsg_unbind_channel(struct unit_module *m,
		struct gk20a *g, void *args)
{
	struct gpu_ops gops = g->ops;
	struct nvgpu_tsg *tsg;
	struct nvgpu_channel *chA, *chB;
	u32 f, branches;
	int rc = UNIT_FAIL;
	u32 prune = F_TSG_UNBIND_CHANNEL_PREEMPT_TSG_FAIL;

	for (f = 0U; f < F_TSG_BIND_CHANNEL_LAST; f++) {

		reset_stub_rc();

		branches = f;

		if (pruned(branches, prune) ||
			/* hw_state is not checked if ch is unserviceable */
			(branches & F_TSG_UNBIND_CHANNEL_UNSERVICEABLE &&
			 branches & F_TSG_UNBIND_CHANNEL_CHECK_HW_STATE_FAIL)) {
			unit_verbose(m, "%s branches=%s (pruned)\n", __func__,
				branches_str(branches, f_tsg_unbind));
			continue;
		}

		/*
		 * tsg unbind tears down TSG in case of failure:
		 * we need to create tsg + bind channel for each test
		 */
		tsg = nvgpu_tsg_open(g, getpid());
		chA = gk20a_open_new_channel(g, ~0U, false, getpid(), getpid());
		chB = gk20a_open_new_channel(g, ~0U, false, getpid(), getpid());
		if (tsg == NULL || chA == NULL || chB == NULL) {
			goto done;
		}

		if (nvgpu_tsg_bind_channel(tsg, chA) != 0 ||
			nvgpu_tsg_bind_channel(tsg, chB) != 0) {
			goto done;
		}

		chA->unserviceable =
			branches & F_TSG_UNBIND_CHANNEL_UNSERVICEABLE ?
			true : false;

		g->ops.fifo.preempt_tsg =
			branches & F_TSG_UNBIND_CHANNEL_PREEMPT_TSG_FAIL ?
			stub_fifo_preempt_tsg_EINVAL :
			gops.fifo.preempt_tsg;

		g->ops.tsg.unbind_channel_check_hw_state =
			branches & F_TSG_UNBIND_CHANNEL_CHECK_HW_STATE_FAIL ?
			stub_tsg_unbind_channel_check_hw_state_EINVAL :
			gops.tsg.unbind_channel_check_hw_state;

		g->ops.runlist.update_for_channel =
			branches & F_TSG_UNBIND_CHANNEL_RUNLIST_UPDATE_FAIL ?
			stub_runlist_update_for_channel_EINVAL :
			gops.runlist.update_for_channel;

		g->ops.tsg.unbind_channel =
			branches & F_TSG_UNBIND_CHANNEL_UNBIND_HAL ?
			stub_tsg_unbind_channel : NULL;

		unit_verbose(m, "%s branches=%s\n", __func__,
			branches_str(branches, f_tsg_unbind));

		(void) nvgpu_tsg_unbind_channel(tsg, chA);

		if (branches & (F_TSG_UNBIND_CHANNEL_PREEMPT_TSG_FAIL|
				F_TSG_UNBIND_CHANNEL_CHECK_HW_STATE_FAIL|
				F_TSG_UNBIND_CHANNEL_RUNLIST_UPDATE_FAIL)) {
			/* check that TSG has been torn down */
			if (!chA->unserviceable || !chB->unserviceable ||
					chA->tsgid != NVGPU_INVALID_TSG_ID) {
				goto done;
			}
		} else {
			/* check that TSG has not been torn down */
			if (chB->unserviceable ||
				nvgpu_list_empty(&tsg->ch_list)) {
				goto done;
			}
		}

		nvgpu_channel_close(chA);
		nvgpu_channel_close(chB);
		nvgpu_ref_put(&tsg->refcount, nvgpu_tsg_release);
		chA = NULL;
		chB = NULL;
		tsg = NULL;
	}

	rc = UNIT_SUCCESS;

done:
	if (rc == UNIT_FAIL) {
		unit_err(m, "%s branches=%s\n", __func__,
			branches_str(branches, f_tsg_unbind));
	}
	if (chA != NULL) {
		nvgpu_channel_close(chA);
	}
	if (chB != NULL) {
		nvgpu_channel_close(chB);
	}
	if (tsg != NULL) {
		nvgpu_ref_put(&tsg->refcount, nvgpu_tsg_release);
	}
	g->ops = gops;
	return rc;
}

#define F_TSG_RELEASE_GR_CTX		BIT(0)
#define F_TSG_RELEASE_MEM		BIT(1)
#define F_TSG_RELEASE_VM		BIT(2)
#define F_TSG_RELEASE_UNHOOK_EVENTS	BIT(3)
#define F_TSG_RELEASE_ENG_BUFS		BIT(4)
#define F_TSG_RELEASE_SM_ERR_STATES	BIT(5)
#define F_TSG_RELEASE_LAST		BIT(6)

static const char const *f_tsg_release[] = {
	"gr_ctx",
	"mem",
	"vm",
	"unhook_events",
	"eng_bufs",
	"sm_err_states"
};

static void stub_tsg_deinit_eng_method_buffers(struct gk20a *g,
		struct nvgpu_tsg *tsg)
{
	stub_rc.branches |= F_TSG_RELEASE_ENG_BUFS;
	stub_rc.tsgid = tsg->tsgid;
}

static void stub_gr_setup_free_gr_ctx(struct gk20a *g,
		struct vm_gk20a *vm, struct nvgpu_gr_ctx *gr_ctx)
{
	stub_rc.count++;
}


static int test_tsg_release(struct unit_module *m,
		struct gk20a *g, void *args)
{
	struct nvgpu_fifo *f = &g->fifo;
	struct gpu_ops gops = g->ops;
	struct nvgpu_tsg *tsg;
	struct nvgpu_list_node ev1, ev2;
	struct vm_gk20a vm;
	u32 branches;
	int rc = UNIT_FAIL;
	struct nvgpu_mem mem;
	u32 free_gr_ctx_mask =
		F_TSG_RELEASE_GR_CTX|F_TSG_RELEASE_MEM|F_TSG_RELEASE_VM;

	for (branches = 0U; branches < F_TSG_RELEASE_LAST; branches++) {

		if (!(branches & F_TSG_RELEASE_GR_CTX) &&
				(branches & F_TSG_RELEASE_MEM)) {
			unit_verbose(m, "%s branches=%s (pruned)\n", __func__,
				branches_str(branches, f_tsg_release));
			continue;
		}
		reset_stub_rc();
		unit_verbose(m, "%s branches=%s\n", __func__,
			branches_str(branches, f_tsg_release));

		tsg = nvgpu_tsg_open(g, getpid());
		if (tsg == NULL || tsg->gr_ctx == NULL ||
			tsg->gr_ctx->mem.aperture != APERTURE_INVALID) {
			goto done;
		}

		if (!(branches & F_TSG_RELEASE_GR_CTX)) {
			nvgpu_free_gr_ctx_struct(g, tsg->gr_ctx);
			tsg->gr_ctx = NULL;
		}

		if (branches & F_TSG_RELEASE_MEM) {
			nvgpu_dma_alloc(g, PAGE_SIZE, &mem);
			tsg->gr_ctx->mem = mem;
		}

		if (branches & F_TSG_RELEASE_VM) {
			tsg->vm = &vm;
			/* prevent nvgpu_vm_remove */
			nvgpu_ref_init(&vm.ref);
			nvgpu_ref_get(&vm.ref);
		} else {
			tsg->vm = NULL;
		}

		if ((branches & free_gr_ctx_mask) == free_gr_ctx_mask) {
			g->ops.gr.setup.free_gr_ctx =
				stub_gr_setup_free_gr_ctx;
		}

		if (branches & F_TSG_RELEASE_UNHOOK_EVENTS) {
			nvgpu_list_add(&ev1, &tsg->event_id_list);
			nvgpu_list_add(&ev2, &tsg->event_id_list);
		}

		g->ops.tsg.deinit_eng_method_buffers =
			branches & F_TSG_RELEASE_ENG_BUFS ?
			stub_tsg_deinit_eng_method_buffers : NULL;

		if (branches & F_TSG_RELEASE_SM_ERR_STATES) {
			if (tsg->sm_error_states == NULL) {
				goto done;
			}
		} else {
			nvgpu_kfree(g, tsg->sm_error_states);
			tsg->sm_error_states = NULL;
		}

		nvgpu_ref_put(&tsg->refcount, nvgpu_tsg_release);

		if ((branches & free_gr_ctx_mask) == free_gr_ctx_mask) {
			if (tsg->gr_ctx != NULL) {
				goto done;
			}
		} else {
			g->ops.gr.setup.free_gr_ctx =
				gops.gr.setup.free_gr_ctx;

			if (branches & F_TSG_RELEASE_MEM) {
				nvgpu_dma_free(g, &mem);
			}

			if (tsg->gr_ctx != NULL) {
				nvgpu_free_gr_ctx_struct(g, tsg->gr_ctx);
				tsg->gr_ctx = NULL;
			}

			if (stub_rc.count > 0) {
				goto done;
			}
		}

		if (branches & F_TSG_RELEASE_UNHOOK_EVENTS) {
			if (!nvgpu_list_empty(&tsg->event_id_list)) {
				goto done;
			}
		}

		if (branches & F_TSG_RELEASE_ENG_BUFS) {
			if (((stub_rc.branches &
					F_TSG_RELEASE_ENG_BUFS) == 0) ||
				stub_rc.tsgid != tsg->tsgid) {
				goto done;
			}
		}

		if (f->tsg[tsg->tsgid].in_use || tsg->gr_ctx != NULL ||
			tsg->vm != NULL || tsg->sm_error_states != NULL) {
			goto done;
		}
	}
	rc = UNIT_SUCCESS;

done:
	if (rc != UNIT_SUCCESS) {
		unit_err(m, "%s branches=%s\n", __func__,
			branches_str(branches, f_tsg_release));
	}
	g->ops = gops;
	return rc;
}

#define F_TSG_UNBIND_CHANNEL_CHECK_HW_NEXT		BIT(0)
#define F_TSG_UNBIND_CHANNEL_CHECK_HW_CTX_RELOAD	BIT(1)
#define F_TSG_UNBIND_CHANNEL_CHECK_HW_ENG_FAULTED	BIT(2)
#define F_TSG_UNBIND_CHANNEL_CHECK_HW_LAST		BIT(3)

static const char const *f_tsg_unbind_channel_check_hw[] = {
	"next",
	"ctx_reload",
	"eng_faulted",
};

static void stub_channel_read_state_NEXT(struct gk20a *g,
		struct nvgpu_channel *ch, struct nvgpu_channel_hw_state *state)
{
	state->next = true;
}

static int test_tsg_unbind_channel_check_hw_state(struct unit_module *m,
		struct gk20a *g, void *args)
{
	struct gpu_ops gops = g->ops;
	struct nvgpu_channel *ch;
	struct nvgpu_tsg *tsg;
	u32 branches;
	int rc = UNIT_FAIL;
	int err;
	u32 prune = F_TSG_UNBIND_CHANNEL_CHECK_HW_NEXT;

	tsg = nvgpu_tsg_open(g, getpid());
	ch = gk20a_open_new_channel(g, ~0U, false, getpid(), getpid());
	if (tsg == NULL || ch == NULL ||
		nvgpu_tsg_bind_channel(tsg, ch) != 0) {
		goto done;
	}

	for (branches = 0; branches < F_TSG_UNBIND_CHANNEL_CHECK_HW_LAST;
			branches++) {

		if (pruned(branches, prune)) {
			unit_verbose(m, "%s branches=%s (pruned)\n", __func__,
				branches_str(branches,
					f_tsg_unbind_channel_check_hw));
			continue;
		}
		reset_stub_rc();

		g->ops.channel.read_state =
			branches & F_TSG_UNBIND_CHANNEL_CHECK_HW_NEXT ?
			stub_channel_read_state_NEXT : gops.channel.read_state;

		g->ops.tsg.unbind_channel_check_ctx_reload =
			branches & F_TSG_UNBIND_CHANNEL_CHECK_HW_CTX_RELOAD ?
			gops.tsg.unbind_channel_check_ctx_reload : NULL;

		g->ops.tsg.unbind_channel_check_eng_faulted =
			branches & F_TSG_UNBIND_CHANNEL_CHECK_HW_ENG_FAULTED ?
			gops.tsg.unbind_channel_check_eng_faulted : NULL;

		unit_verbose(m, "%s branches=%s\n", __func__,
			branches_str(branches, f_tsg_unbind_channel_check_hw));

		err = nvgpu_tsg_unbind_channel_check_hw_state(tsg, ch);

		if (branches & F_TSG_UNBIND_CHANNEL_CHECK_HW_NEXT) {
			if (err == 0) {
				goto done;
			}
		} else {
			if (err != 0) {
				goto done;
			}
		}
	}
	rc = UNIT_SUCCESS;

done:
	if (rc == UNIT_FAIL) {
		unit_err(m, "%s branches=%s\n", __func__,
			branches_str(branches, f_tsg_unbind_channel_check_hw));
	}
	if (ch != NULL) {
		nvgpu_channel_close(ch);
	}
	if (tsg != NULL) {
		nvgpu_ref_put(&tsg->refcount, nvgpu_tsg_release);
	}
	g->ops = gops;
	return rc;
}

static int test_fifo_remove_support(struct unit_module *m,
		struct gk20a *g, void *args)
{
	struct test_tsg_args *t = args;

	if (!t->init_done) {
		unit_return_fail(m, "missing init support");
	}

	if (g->fifo.remove_support) {
		g->fifo.remove_support(&g->fifo);
	}

	return UNIT_SUCCESS;
}

static int test_tsg_check_and_get_from_id(struct unit_module *m,
		struct gk20a *g, void *args)
{
	struct nvgpu_tsg *tsg;
	int rc = UNIT_FAIL;

	tsg = nvgpu_tsg_check_and_get_from_id(g, NVGPU_INVALID_TSG_ID);
	if (tsg != NULL) {
		goto done;
	}

	tsg = nvgpu_tsg_open(g, getpid());
	if (tsg == NULL) {
		goto done;
	}

	if (nvgpu_tsg_check_and_get_from_id(g, tsg->tsgid) != tsg) {
		goto done;
	}
	nvgpu_ref_put(&tsg->refcount, nvgpu_tsg_release);

	rc = UNIT_SUCCESS;
done:
	return rc;
}

struct unit_module_test nvgpu_tsg_tests[] = {
	UNIT_TEST(init_support, test_fifo_init_support, &test_args, 0),
	UNIT_TEST(open, test_tsg_open, &test_args, 0),
	UNIT_TEST(release, test_tsg_release, &test_args, 0),
	UNIT_TEST(get_from_id, test_tsg_check_and_get_from_id, &test_args, 0),
	UNIT_TEST(bind_channel, test_tsg_bind_channel, &test_args, 0),
	UNIT_TEST(unbind_channel, test_tsg_unbind_channel, &test_args, 0),
	UNIT_TEST(unbind_channel_check_hw_state,
			test_tsg_unbind_channel_check_hw_state, &test_args, 0),
	UNIT_TEST(remove_support, test_fifo_remove_support, &test_args, 0),
};

UNIT_MODULE(nvgpu_tsg, nvgpu_tsg_tests, UNIT_PRIO_NVGPU_TEST);
