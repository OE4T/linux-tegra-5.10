/*
 * Copyright (c) 2014-2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/bug.h>
#include <nvgpu/debug.h>
#include <nvgpu/kmem.h>
#include <nvgpu/log.h>
#include <nvgpu/os_sched.h>
#include <nvgpu/channel.h>
#include <nvgpu/tsg.h>
#include <nvgpu/rc.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/error_notifier.h>
#include <nvgpu/gr/config.h>
#include <nvgpu/gr/ctx.h>
#include <nvgpu/runlist.h>

#include "common/gr/gr_priv.h"

void nvgpu_tsg_disable(struct tsg_gk20a *tsg)
{
	struct gk20a *g = tsg->g;
	struct channel_gk20a *ch;

	nvgpu_rwsem_down_read(&tsg->ch_list_lock);
	nvgpu_list_for_each_entry(ch, &tsg->ch_list, channel_gk20a, ch_entry) {
		g->ops.channel.disable(ch);
	}
	nvgpu_rwsem_up_read(&tsg->ch_list_lock);
}

static bool gk20a_is_channel_active(struct gk20a *g, struct channel_gk20a *ch)
{
	struct fifo_gk20a *f = &g->fifo;
	struct fifo_runlist_info_gk20a *runlist;
	unsigned int i;

	for (i = 0; i < f->num_runlists; ++i) {
		runlist = &f->active_runlist_info[i];
		if (test_bit((int)ch->chid, runlist->active_channels)) {
			return true;
		}
	}

	return false;
}

/*
 * API to mark channel as part of TSG
 *
 * Note that channel is not runnable when we bind it to TSG
 */
int nvgpu_tsg_bind_channel(struct tsg_gk20a *tsg, struct channel_gk20a *ch)
{
	struct gk20a *g = ch->g;
	int err = 0;

	nvgpu_log_fn(g, "bind tsg:%u ch:%u\n", tsg->tsgid, ch->chid);

	/* check if channel is already bound to some TSG */
	if (tsg_gk20a_from_ch(ch) != NULL) {
		return -EINVAL;
	}

	/* channel cannot be bound to TSG if it is already active */
	if (gk20a_is_channel_active(tsg->g, ch)) {
		return -EINVAL;
	}

	/* all the channel part of TSG should need to be same runlist_id */
	if (tsg->runlist_id == FIFO_INVAL_TSG_ID) {
		tsg->runlist_id = ch->runlist_id;
	} else if (tsg->runlist_id != ch->runlist_id) {
		nvgpu_err(tsg->g,
			"runlist_id mismatch ch[%d] tsg[%d]",
			ch->runlist_id, tsg->runlist_id);
		return -EINVAL;
	}

	if (g->ops.tsg.bind_channel != NULL) {
		err = g->ops.tsg.bind_channel(tsg, ch);
	}

	nvgpu_rwsem_down_write(&tsg->ch_list_lock);
	nvgpu_list_add_tail(&ch->ch_entry, &tsg->ch_list);
	ch->tsgid = tsg->tsgid;
	nvgpu_rwsem_up_write(&tsg->ch_list_lock);

	if (g->ops.tsg.bind_channel_eng_method_buffers != NULL) {
		g->ops.tsg.bind_channel_eng_method_buffers(tsg, ch);
	}

	nvgpu_ref_get(&tsg->refcount);

	return err;
}

/* The caller must ensure that channel belongs to a tsg */
int nvgpu_tsg_unbind_channel(struct tsg_gk20a *tsg, struct channel_gk20a *ch)
{
	struct gk20a *g = ch->g;
	int err;

	nvgpu_log_fn(g, "unbind tsg:%u ch:%u\n", tsg->tsgid, ch->chid);

	err = nvgpu_tsg_unbind_channel_common(tsg, ch);
	if (err != 0) {
		nvgpu_err(g, "Channel %d unbind failed, tearing down TSG %d",
			ch->chid, tsg->tsgid);

		nvgpu_tsg_abort(g, tsg, true);
		/* If channel unbind fails, channel is still part of runlist */
		channel_gk20a_update_runlist(ch, false);

		nvgpu_rwsem_down_write(&tsg->ch_list_lock);
		nvgpu_list_del(&ch->ch_entry);
		ch->tsgid = NVGPU_INVALID_TSG_ID;
		nvgpu_rwsem_up_write(&tsg->ch_list_lock);
	}

	if (g->ops.tsg.unbind_channel != NULL) {
		err = g->ops.tsg.unbind_channel(tsg, ch);
	}

	nvgpu_ref_put(&tsg->refcount, nvgpu_tsg_release);

	return 0;
}

int nvgpu_tsg_unbind_channel_common(struct tsg_gk20a *tsg,
		struct channel_gk20a *ch)
{
	struct gk20a *g = ch->g;
	int err;
	bool tsg_timedout;

	/* If one channel in TSG times out, we disable all channels */
	nvgpu_rwsem_down_write(&tsg->ch_list_lock);
	tsg_timedout = gk20a_channel_check_unserviceable(ch);
	nvgpu_rwsem_up_write(&tsg->ch_list_lock);

	/* Disable TSG and examine status before unbinding channel */
	g->ops.tsg.disable(tsg);

	err = g->ops.fifo.preempt_tsg(g, tsg);
	if (err != 0) {
		goto fail_enable_tsg;
	}

	if (!tsg_timedout &&
	    (g->ops.tsg.unbind_channel_check_hw_state != NULL)) {
		err = g->ops.tsg.unbind_channel_check_hw_state(tsg, ch);
		if (err != 0) {
			nvgpu_err(g, "invalid hw_state for ch %u", ch->chid);
			goto fail_enable_tsg;
		}
	}

	/* Channel should be seen as TSG channel while updating runlist */
	err = channel_gk20a_update_runlist(ch, false);
	if (err != 0) {
		nvgpu_err(g, "update runlist failed ch:%u tsg:%u",
				ch->chid, tsg->tsgid);
		goto fail_enable_tsg;
	}

	/* Remove channel from TSG and re-enable rest of the channels */
	nvgpu_rwsem_down_write(&tsg->ch_list_lock);
	nvgpu_list_del(&ch->ch_entry);
	ch->tsgid = NVGPU_INVALID_TSG_ID;

	/* another thread could have re-enabled the channel because it was
	 * still on the list at that time, so make sure it's truly disabled
	 */
	g->ops.channel.disable(ch);
	nvgpu_rwsem_up_write(&tsg->ch_list_lock);

	/*
	 * Don't re-enable all channels if TSG has timed out already
	 *
	 * Note that we can skip disabling and preempting TSG too in case of
	 * time out, but we keep that to ensure TSG is kicked out
	 */
	if (!tsg_timedout) {
		g->ops.tsg.enable(tsg);
	}

	if (g->ops.channel.abort_clean_up != NULL) {
		g->ops.channel.abort_clean_up(ch);
	}

	return 0;

fail_enable_tsg:
	if (!tsg_timedout) {
		g->ops.tsg.enable(tsg);
	}
	return err;
}

int nvgpu_tsg_unbind_channel_check_hw_state(struct tsg_gk20a *tsg,
		struct channel_gk20a *ch)
{
	struct gk20a *g = ch->g;
	struct nvgpu_channel_hw_state hw_state;

	g->ops.channel.read_state(g, ch, &hw_state);

	if (hw_state.next) {
		nvgpu_err(g, "Channel %d to be removed from TSG %d has NEXT set!",
				ch->chid, ch->tsgid);
		return -EINVAL;
	}

	if (g->ops.tsg.unbind_channel_check_ctx_reload != NULL) {
		g->ops.tsg.unbind_channel_check_ctx_reload(tsg, ch, &hw_state);
	}

	if (g->ops.tsg.unbind_channel_check_eng_faulted != NULL) {
		g->ops.tsg.unbind_channel_check_eng_faulted(tsg, ch,
				&hw_state);
	}

	return 0;
}

void nvgpu_tsg_unbind_channel_check_ctx_reload(struct tsg_gk20a *tsg,
	struct channel_gk20a *ch,
	struct nvgpu_channel_hw_state *hw_state)
{
	struct gk20a *g = ch->g;
	struct channel_gk20a *temp_ch;

	/* If CTX_RELOAD is set on a channel, move it to some other channel */
	if (hw_state->ctx_reload) {
		nvgpu_rwsem_down_read(&tsg->ch_list_lock);
		nvgpu_list_for_each_entry(temp_ch, &tsg->ch_list,
				channel_gk20a, ch_entry) {
			if (temp_ch->chid != ch->chid) {
				g->ops.channel.force_ctx_reload(temp_ch);
				break;
			}
		}
		nvgpu_rwsem_up_read(&tsg->ch_list_lock);
	}
}

static void nvgpu_tsg_destroy(struct gk20a *g, struct tsg_gk20a *tsg)
{
	nvgpu_mutex_destroy(&tsg->event_id_list_lock);
}

/* force reset tsg that the channel is bound to */
int nvgpu_tsg_force_reset_ch(struct channel_gk20a *ch,
				u32 err_code, bool verbose)
{
	struct gk20a *g = ch->g;

	struct tsg_gk20a *tsg = tsg_gk20a_from_ch(ch);

	if (tsg != NULL) {
		nvgpu_tsg_set_error_notifier(g, tsg, err_code);
		nvgpu_rc_tsg_and_related_engines(g, tsg, verbose,
			RC_TYPE_FORCE_RESET);
	} else {
		nvgpu_err(g, "chid: %d is not bound to tsg", ch->chid);
	}

	return 0;
}

void nvgpu_tsg_cleanup_sw(struct gk20a *g)
{
	struct fifo_gk20a *f = &g->fifo;
	u32 tsgid;

	for (tsgid = 0; tsgid < f->num_channels; tsgid++) {
		struct tsg_gk20a *tsg = &f->tsg[tsgid];

		nvgpu_tsg_destroy(g, tsg);
	}

	nvgpu_vfree(g, f->tsg);
	f->tsg = NULL;
	nvgpu_mutex_destroy(&f->tsg_inuse_mutex);
}

int gk20a_init_tsg_support(struct gk20a *g, u32 tsgid)
{
	struct tsg_gk20a *tsg = NULL;

	if (tsgid >= g->fifo.num_channels) {
		return -EINVAL;
	}

	tsg = &g->fifo.tsg[tsgid];

	tsg->in_use = false;
	tsg->tsgid = tsgid;
	tsg->abortable = true;

	nvgpu_init_list_node(&tsg->ch_list);
	nvgpu_rwsem_init(&tsg->ch_list_lock);

	nvgpu_init_list_node(&tsg->event_id_list);

	return nvgpu_mutex_init(&tsg->event_id_list_lock);
}

int nvgpu_tsg_setup_sw(struct gk20a *g)
{
	struct fifo_gk20a *f = &g->fifo;
	u32 tsgid, i;
	int err;

	err = nvgpu_mutex_init(&f->tsg_inuse_mutex);
	if (err != 0) {
		nvgpu_err(g, "mutex init failed");
		return err;
	}

	f->tsg = nvgpu_vzalloc(g, f->num_channels * sizeof(*f->tsg));
	if (f->tsg == NULL) {
		nvgpu_err(g, "no mem for tsgs");
		err = -ENOMEM;
		goto clean_up_mutex;
	}

	for (tsgid = 0; tsgid < f->num_channels; tsgid++) {
		err = gk20a_init_tsg_support(g, tsgid);
		if (err != 0) {
			nvgpu_err(g, "tsg init failed, tsgid=%u", tsgid);
			goto clean_up;
		}
	}

	return 0;

clean_up:
	for (i = 0; i < tsgid; i++) {
		struct tsg_gk20a *tsg = &g->fifo.tsg[i];

		nvgpu_tsg_destroy(g, tsg);
	}
	nvgpu_vfree(g, f->tsg);
	f->tsg = NULL;

clean_up_mutex:
	nvgpu_mutex_destroy(&f->tsg_inuse_mutex);
	return err;
}

bool nvgpu_tsg_mark_error(struct gk20a *g,
		struct tsg_gk20a *tsg)
{
	struct channel_gk20a *ch = NULL;
	bool verbose = false;

	nvgpu_rwsem_down_read(&tsg->ch_list_lock);
	nvgpu_list_for_each_entry(ch, &tsg->ch_list, channel_gk20a, ch_entry) {
		if (gk20a_channel_get(ch) != NULL) {
			if (nvgpu_channel_mark_error(g, ch)) {
				verbose = true;
			}
			gk20a_channel_put(ch);
		}
	}
	nvgpu_rwsem_up_read(&tsg->ch_list_lock);

	return verbose;

}

void nvgpu_tsg_set_ctxsw_timeout_accumulated_ms(struct tsg_gk20a *tsg, u32 ms)
{
	struct channel_gk20a *ch = NULL;

	nvgpu_rwsem_down_read(&tsg->ch_list_lock);
	nvgpu_list_for_each_entry(ch, &tsg->ch_list, channel_gk20a, ch_entry) {
		if (gk20a_channel_get(ch) != NULL) {
			ch->ctxsw_timeout_accumulated_ms = ms;
			gk20a_channel_put(ch);
		}
	}
	nvgpu_rwsem_up_read(&tsg->ch_list_lock);
}

bool nvgpu_tsg_ctxsw_timeout_debug_dump_state(struct tsg_gk20a *tsg)
{
	struct channel_gk20a *ch = NULL;
	bool verbose = false;

	nvgpu_rwsem_down_read(&tsg->ch_list_lock);
	nvgpu_list_for_each_entry(ch, &tsg->ch_list, channel_gk20a, ch_entry) {
		if (gk20a_channel_get(ch) != NULL) {
			if (ch->ctxsw_timeout_debug_dump) {
				verbose = true;
			}
			gk20a_channel_put(ch);
		}
	}
	nvgpu_rwsem_up_read(&tsg->ch_list_lock);

	return verbose;
}

void nvgpu_tsg_set_error_notifier(struct gk20a *g, struct tsg_gk20a *tsg,
		u32 error_notifier)
{
	struct channel_gk20a *ch = NULL;

	nvgpu_rwsem_down_read(&tsg->ch_list_lock);
	nvgpu_list_for_each_entry(ch, &tsg->ch_list, channel_gk20a, ch_entry) {
		if (gk20a_channel_get(ch) != NULL) {
			nvgpu_channel_set_error_notifier(g, ch, error_notifier);
			gk20a_channel_put(ch);
		}
	}
	nvgpu_rwsem_up_read(&tsg->ch_list_lock);
}

void nvgpu_tsg_set_ctx_mmu_error(struct gk20a *g, struct tsg_gk20a *tsg)
{
	nvgpu_err(g, "TSG %d generated a mmu fault", tsg->tsgid);

	nvgpu_tsg_set_error_notifier(g, tsg,
		NVGPU_ERR_NOTIFIER_FIFO_ERROR_MMU_ERR_FLT);
}

bool nvgpu_tsg_check_ctxsw_timeout(struct tsg_gk20a *tsg,
		bool *debug_dump, u32 *ms)
{
	struct channel_gk20a *ch;
	bool recover = false;
	bool progress = false;
	struct gk20a *g = tsg->g;

	*debug_dump = false;
	*ms = g->ctxsw_timeout_period_ms;

	nvgpu_rwsem_down_read(&tsg->ch_list_lock);

	/* check if there was some progress on any of the TSG channels.
	 * fifo recovery is needed if at least one channel reached the
	 * maximum timeout without progress (update in gpfifo pointers).
	 */
	nvgpu_list_for_each_entry(ch, &tsg->ch_list, channel_gk20a, ch_entry) {
		if (gk20a_channel_get(ch) != NULL) {
			recover = nvgpu_channel_update_and_check_ctxsw_timeout(ch,
					*ms, &progress);
			if (progress || recover) {
				break;
			}
			gk20a_channel_put(ch);
		}
	}

	if (recover) {
		/*
		 * if one channel is presumed dead (no progress for too long),
		 * then fifo recovery is needed. we can't really figure out
		 * which channel caused the problem, so set ctxsw timeout error
		 * notifier for all channels.
		 */
		*ms = ch->ctxsw_timeout_accumulated_ms;
		gk20a_channel_put(ch);
		*debug_dump = nvgpu_tsg_ctxsw_timeout_debug_dump_state(tsg);

	} else if (progress) {
		/*
		 * if at least one channel in the TSG made some progress, reset
		 * ctxsw_timeout_accumulated_ms for all channels in the TSG. In
		 * particular, this resets ctxsw_timeout_accumulated_ms timeout
		 * for channels that already completed their work.
		 */
		nvgpu_log_info(g, "progress on tsg=%d ch=%d",
				tsg->tsgid, ch->chid);
		gk20a_channel_put(ch);
		*ms = g->ctxsw_timeout_period_ms;
		nvgpu_tsg_set_ctxsw_timeout_accumulated_ms(tsg, *ms);
	}

	/* if we could not detect progress on any of the channel, but none
	 * of them has reached the timeout, there is nothing more to do:
	 * ctxsw_timeout_accumulated_ms has been updated for all of them.
	 */
	nvgpu_rwsem_up_read(&tsg->ch_list_lock);
	return recover;
}

int nvgpu_tsg_set_interleave(struct tsg_gk20a *tsg, u32 level)
{
	struct gk20a *g = tsg->g;
	int ret;

	nvgpu_log(g, gpu_dbg_sched,
			"tsgid=%u interleave=%u", tsg->tsgid, level);

	nvgpu_speculation_barrier();

	if ((level != NVGPU_FIFO_RUNLIST_INTERLEAVE_LEVEL_LOW) &&
	    (level != NVGPU_FIFO_RUNLIST_INTERLEAVE_LEVEL_MEDIUM) &&
	    (level != NVGPU_FIFO_RUNLIST_INTERLEAVE_LEVEL_HIGH)) {
		return -EINVAL;
	}

	if (g->ops.tsg.set_interleave != NULL) {
		ret = g->ops.tsg.set_interleave(tsg, level);
		if (ret != 0) {
			nvgpu_err(g,
				"set interleave failed tsgid=%u", tsg->tsgid);
			return ret;
		}
	}

	tsg->interleave_level = level;

	return g->ops.runlist.reload(g, tsg->runlist_id, true, true);
}

int nvgpu_tsg_set_timeslice(struct tsg_gk20a *tsg, u32 timeslice_us)
{
	struct gk20a *g = tsg->g;

	nvgpu_log(g, gpu_dbg_sched, "tsgid=%u timeslice=%u us",
			tsg->tsgid, timeslice_us);

	if (timeslice_us < g->tsg_timeslice_min_us ||
		timeslice_us > g->tsg_timeslice_max_us) {
		return -EINVAL;
	}

	tsg->timeslice_us = timeslice_us;

	return g->ops.runlist.reload(g, tsg->runlist_id, true, true);
}

u32 nvgpu_tsg_get_timeslice(struct tsg_gk20a *tsg)
{
	return tsg->timeslice_us;
}

u32 nvgpu_tsg_default_timeslice_us(struct gk20a *g)
{
	return NVGPU_TSG_TIMESLICE_DEFAULT_US;
}

void nvgpu_tsg_enable_sched(struct gk20a *g, struct tsg_gk20a *tsg)
{
	nvgpu_fifo_runlist_set_state(g, BIT32(tsg->runlist_id),
			RUNLIST_ENABLED);

}

void nvgpu_tsg_disable_sched(struct gk20a *g, struct tsg_gk20a *tsg)
{
	nvgpu_fifo_runlist_set_state(g, BIT32(tsg->runlist_id),
			RUNLIST_DISABLED);
}

static void release_used_tsg(struct fifo_gk20a *f, struct tsg_gk20a *tsg)
{
	nvgpu_mutex_acquire(&f->tsg_inuse_mutex);
	f->tsg[tsg->tsgid].in_use = false;
	nvgpu_mutex_release(&f->tsg_inuse_mutex);
}

static struct tsg_gk20a *gk20a_tsg_acquire_unused_tsg(struct fifo_gk20a *f)
{
	struct tsg_gk20a *tsg = NULL;
	unsigned int tsgid;

	nvgpu_mutex_acquire(&f->tsg_inuse_mutex);
	for (tsgid = 0; tsgid < f->num_channels; tsgid++) {
		if (!f->tsg[tsgid].in_use) {
			f->tsg[tsgid].in_use = true;
			tsg = &f->tsg[tsgid];
			break;
		}
	}
	nvgpu_mutex_release(&f->tsg_inuse_mutex);

	return tsg;
}

int nvgpu_tsg_open_common(struct gk20a *g, struct tsg_gk20a *tsg, pid_t pid)
{
	u32 no_of_sm = nvgpu_gr_config_get_no_of_sm(g->gr->config);
	int err;

	/* we need to allocate this after g->ops.gr.init_fs_state() since
	 * we initialize gr.config->no_of_sm in this function
	 */
	if (no_of_sm == 0U) {
		nvgpu_err(g, "no_of_sm %d not set, failed allocation", no_of_sm);
		return -EINVAL;
	}

	err = gk20a_tsg_alloc_sm_error_states_mem(g, tsg, no_of_sm);
	if (err != 0) {
		return err;
	}

	tsg->tgid = pid;
	tsg->g = g;
	tsg->num_active_channels = 0U;
	nvgpu_ref_init(&tsg->refcount);

	tsg->vm = NULL;
	tsg->interleave_level = NVGPU_FIFO_RUNLIST_INTERLEAVE_LEVEL_LOW;
	tsg->timeslice_us = g->ops.tsg.default_timeslice_us(g);
	tsg->runlist_id = FIFO_INVAL_TSG_ID;
	tsg->sm_exception_mask_type = NVGPU_SM_EXCEPTION_TYPE_MASK_NONE;
	tsg->gr_ctx = nvgpu_alloc_gr_ctx_struct(g);
	if (tsg->gr_ctx == NULL) {
		err = -ENOMEM;
		goto clean_up;
	}

	if (g->ops.tsg.init_eng_method_buffers != NULL) {
		g->ops.tsg.init_eng_method_buffers(g, tsg);
	}

	if (g->ops.tsg.open != NULL) {
		err = g->ops.tsg.open(tsg);
		if (err != 0) {
			nvgpu_err(g, "tsg %d fifo open failed %d",
				  tsg->tsgid, err);
			goto clean_up;
		}
	}

	return 0;

clean_up:
	nvgpu_tsg_release_common(g, tsg);
	nvgpu_ref_put(&tsg->refcount, NULL);

	return err;
}

struct tsg_gk20a *nvgpu_tsg_open(struct gk20a *g, pid_t pid)
{
	struct tsg_gk20a *tsg;
	int err;

	tsg = gk20a_tsg_acquire_unused_tsg(&g->fifo);
	if (tsg == NULL) {
		return NULL;
	}

	err = nvgpu_tsg_open_common(g, tsg, pid);
	if (err != 0) {
		release_used_tsg(&g->fifo, tsg);
		nvgpu_err(g, "tsg %d open failed %d", tsg->tsgid, err);
		return NULL;
	}

	nvgpu_log(g, gpu_dbg_fn, "tsg opened %d\n", tsg->tsgid);

	return tsg;
}

void nvgpu_tsg_release_common(struct gk20a *g, struct tsg_gk20a *tsg)
{
	if (g->ops.tsg.release != NULL) {
		g->ops.tsg.release(tsg);
	}

	nvgpu_free_gr_ctx_struct(g, tsg->gr_ctx);
	tsg->gr_ctx = NULL;

	if (g->ops.tsg.deinit_eng_method_buffers != NULL) {
		g->ops.tsg.deinit_eng_method_buffers(g, tsg);
	}

	if (tsg->vm != NULL) {
		nvgpu_vm_put(tsg->vm);
		tsg->vm = NULL;
	}

	if(tsg->sm_error_states != NULL) {
		nvgpu_kfree(g, tsg->sm_error_states);
		tsg->sm_error_states = NULL;
		nvgpu_mutex_destroy(&tsg->sm_exception_mask_lock);
	}
}

static struct tsg_gk20a *tsg_gk20a_from_ref(struct nvgpu_ref *ref)
{
	return (struct tsg_gk20a *)
		((uintptr_t)ref - offsetof(struct tsg_gk20a, refcount));
}

void nvgpu_tsg_release(struct nvgpu_ref *ref)
{
	struct tsg_gk20a *tsg = tsg_gk20a_from_ref(ref);
	struct gk20a *g = tsg->g;
	struct gk20a_event_id_data *event_id_data, *event_id_data_temp;

	if (tsg->gr_ctx != NULL && nvgpu_mem_is_valid(
			nvgpu_gr_ctx_get_ctx_mem(tsg->gr_ctx)) &&
			tsg->vm != NULL) {
		g->ops.gr.setup.free_gr_ctx(g, tsg->vm, tsg->gr_ctx);
	}

	/* unhook all events created on this TSG */
	nvgpu_mutex_acquire(&tsg->event_id_list_lock);
	nvgpu_list_for_each_entry_safe(event_id_data, event_id_data_temp,
				&tsg->event_id_list,
				gk20a_event_id_data,
				event_id_node) {
		nvgpu_list_del(&event_id_data->event_id_node);
	}
	nvgpu_mutex_release(&tsg->event_id_list_lock);

	nvgpu_tsg_release_common(g, tsg);
	release_used_tsg(&g->fifo, tsg);

	nvgpu_log(g, gpu_dbg_fn, "tsg released %d\n", tsg->tsgid);
}

struct tsg_gk20a *tsg_gk20a_from_ch(struct channel_gk20a *ch)
{
	struct tsg_gk20a *tsg = NULL;
	u32 tsgid = ch->tsgid;

	if (tsgid != NVGPU_INVALID_TSG_ID) {
		struct gk20a *g = ch->g;
		struct fifo_gk20a *f = &g->fifo;

		tsg = &f->tsg[tsgid];
	} else {
		nvgpu_log(ch->g, gpu_dbg_fn, "tsgid is invalid for chid: %d",
			ch->chid);
	}
	return tsg;
}

int gk20a_tsg_alloc_sm_error_states_mem(struct gk20a *g,
					struct tsg_gk20a *tsg,
					u32 num_sm)
{
	int err = 0;

	if (tsg->sm_error_states != NULL) {
		return -EINVAL;
	}

	err = nvgpu_mutex_init(&tsg->sm_exception_mask_lock);
	if (err != 0) {
		return err;
	}

	tsg->sm_error_states = nvgpu_kzalloc(g,
			sizeof(struct nvgpu_tsg_sm_error_state)
			* num_sm);
	if (tsg->sm_error_states == NULL) {
		nvgpu_err(g, "sm_error_states mem allocation failed");
		nvgpu_mutex_destroy(&tsg->sm_exception_mask_lock);
		err = -ENOMEM;
	}

	return err;
}

void gk20a_tsg_update_sm_error_state_locked(struct tsg_gk20a *tsg,
				u32 sm_id,
				struct nvgpu_tsg_sm_error_state *sm_error_state)
{
	struct nvgpu_tsg_sm_error_state *tsg_sm_error_states;

	tsg_sm_error_states = tsg->sm_error_states + sm_id;

	tsg_sm_error_states->hww_global_esr =
			sm_error_state->hww_global_esr;
	tsg_sm_error_states->hww_warp_esr =
			sm_error_state->hww_warp_esr;
	tsg_sm_error_states->hww_warp_esr_pc =
			sm_error_state->hww_warp_esr_pc;
	tsg_sm_error_states->hww_global_esr_report_mask =
			sm_error_state->hww_global_esr_report_mask;
	tsg_sm_error_states->hww_warp_esr_report_mask =
			sm_error_state->hww_warp_esr_report_mask;
}

int gk20a_tsg_set_sm_exception_type_mask(struct channel_gk20a *ch,
		u32 exception_mask)
{
	struct tsg_gk20a *tsg;

	tsg = tsg_gk20a_from_ch(ch);
	if (tsg == NULL) {
		return -EINVAL;
	}

	nvgpu_mutex_acquire(&tsg->sm_exception_mask_lock);
	tsg->sm_exception_mask_type = exception_mask;
	nvgpu_mutex_release(&tsg->sm_exception_mask_lock);

	return 0;
}

void nvgpu_tsg_abort(struct gk20a *g, struct tsg_gk20a *tsg, bool preempt)
{
	struct channel_gk20a *ch = NULL;

	nvgpu_log_fn(g, " ");

	WARN_ON(tsg->abortable == false);

	g->ops.tsg.disable(tsg);

	if (preempt) {
		/*
		 * Ignore the return value below. If preempt fails, preempt_tsg
		 * operation will print the error and ctxsw timeout may trigger
		 * a recovery if needed.
		 */
		(void)g->ops.fifo.preempt_tsg(g, tsg);
	}

	nvgpu_rwsem_down_read(&tsg->ch_list_lock);
	nvgpu_list_for_each_entry(ch, &tsg->ch_list, channel_gk20a, ch_entry) {
		if (gk20a_channel_get(ch) != NULL) {
			gk20a_channel_set_unserviceable(ch);
			if (g->ops.channel.abort_clean_up != NULL) {
				g->ops.channel.abort_clean_up(ch);
			}
			gk20a_channel_put(ch);
		}
	}
	nvgpu_rwsem_up_read(&tsg->ch_list_lock);
}

void nvgpu_tsg_reset_faulted_eng_pbdma(struct gk20a *g, struct tsg_gk20a *tsg,
		bool eng, bool pbdma)
{
	struct channel_gk20a *ch;

	if (g->ops.channel.reset_faulted == NULL) {
		return;
	}

	if (tsg == NULL) {
		return;
	}

	nvgpu_log(g, gpu_dbg_info, "reset faulted eng and pbdma bits in ccsr");

	nvgpu_rwsem_down_read(&tsg->ch_list_lock);
	nvgpu_list_for_each_entry(ch, &tsg->ch_list, channel_gk20a, ch_entry) {
		g->ops.channel.reset_faulted(g, ch, eng, pbdma);
	}
	nvgpu_rwsem_up_read(&tsg->ch_list_lock);
}
