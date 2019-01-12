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
#include <nvgpu/gk20a.h>
#include <nvgpu/error_notifier.h>
#include <nvgpu/gr/ctx.h>

#include "gk20a/gr_gk20a.h"

bool gk20a_is_channel_marked_as_tsg(struct channel_gk20a *ch)
{
	return !(ch->tsgid == NVGPU_INVALID_TSG_ID);
}

int gk20a_enable_tsg(struct tsg_gk20a *tsg)
{
	struct gk20a *g = tsg->g;
	struct channel_gk20a *ch;
	bool is_next, is_ctx_reload;

	gk20a_tsg_disable_sched(g, tsg);

	/*
	 * Due to h/w bug that exists in Maxwell and Pascal,
	 * we first need to enable all channels with NEXT and CTX_RELOAD set,
	 * and then rest of the channels should be enabled
	 */
	nvgpu_rwsem_down_read(&tsg->ch_list_lock);
	nvgpu_list_for_each_entry(ch, &tsg->ch_list, channel_gk20a, ch_entry) {
		is_next = gk20a_fifo_channel_status_is_next(g, ch->chid);
		is_ctx_reload = gk20a_fifo_channel_status_is_ctx_reload(g, ch->chid);

		if (is_next || is_ctx_reload) {
			g->ops.fifo.enable_channel(ch);
		}
	}

	nvgpu_list_for_each_entry(ch, &tsg->ch_list, channel_gk20a, ch_entry) {
		is_next = gk20a_fifo_channel_status_is_next(g, ch->chid);
		is_ctx_reload = gk20a_fifo_channel_status_is_ctx_reload(g, ch->chid);

		if (is_next || is_ctx_reload) {
			continue;
		}

		g->ops.fifo.enable_channel(ch);
	}
	nvgpu_rwsem_up_read(&tsg->ch_list_lock);

	gk20a_tsg_enable_sched(g, tsg);

	return 0;
}

int gk20a_disable_tsg(struct tsg_gk20a *tsg)
{
	struct gk20a *g = tsg->g;
	struct channel_gk20a *ch;

	nvgpu_rwsem_down_read(&tsg->ch_list_lock);
	nvgpu_list_for_each_entry(ch, &tsg->ch_list, channel_gk20a, ch_entry) {
		g->ops.fifo.disable_channel(ch);
	}
	nvgpu_rwsem_up_read(&tsg->ch_list_lock);

	return 0;
}

static bool gk20a_is_channel_active(struct gk20a *g, struct channel_gk20a *ch)
{
	struct fifo_gk20a *f = &g->fifo;
	struct fifo_runlist_info_gk20a *runlist;
	unsigned int i;

	for (i = 0; i < f->max_runlists; ++i) {
		runlist = &f->runlist_info[i];
		if (test_bit(ch->chid, runlist->active_channels)) {
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
int gk20a_tsg_bind_channel(struct tsg_gk20a *tsg,
			struct channel_gk20a *ch)
{
	struct gk20a *g = ch->g;

	nvgpu_log_fn(g, " ");

	/* check if channel is already bound to some TSG */
	if (gk20a_is_channel_marked_as_tsg(ch)) {
		return -EINVAL;
	}

	/* channel cannot be bound to TSG if it is already active */
	if (gk20a_is_channel_active(tsg->g, ch)) {
		return -EINVAL;
	}

	ch->tsgid = tsg->tsgid;

	/* all the channel part of TSG should need to be same runlist_id */
	if (tsg->runlist_id == FIFO_INVAL_TSG_ID) {
		tsg->runlist_id = ch->runlist_id;
	} else if (tsg->runlist_id != ch->runlist_id) {
		nvgpu_err(tsg->g,
			"Error: TSG channel should be share same runlist ch[%d] tsg[%d]",
			ch->runlist_id, tsg->runlist_id);
		return -EINVAL;
	}

	nvgpu_rwsem_down_write(&tsg->ch_list_lock);
	nvgpu_list_add_tail(&ch->ch_entry, &tsg->ch_list);
	nvgpu_rwsem_up_write(&tsg->ch_list_lock);

	nvgpu_ref_get(&tsg->refcount);

	nvgpu_log(g, gpu_dbg_fn, "BIND tsg:%d channel:%d\n",
					tsg->tsgid, ch->chid);

	nvgpu_log_fn(g, "done");
	return 0;
}

/* The caller must ensure that channel belongs to a tsg */
int gk20a_tsg_unbind_channel(struct channel_gk20a *ch)
{
	struct gk20a *g = ch->g;
	struct tsg_gk20a *tsg = tsg_gk20a_from_ch(ch);
	int err;

	nvgpu_assert(tsg != NULL);

	err = g->ops.fifo.tsg_unbind_channel(ch);
	if (err != 0) {
		nvgpu_err(g, "Channel %d unbind failed, tearing down TSG %d",
			ch->chid, tsg->tsgid);

		gk20a_fifo_abort_tsg(ch->g, tsg, true);
		/* If channel unbind fails, channel is still part of runlist */
		channel_gk20a_update_runlist(ch, false);

		nvgpu_rwsem_down_write(&tsg->ch_list_lock);
		nvgpu_list_del(&ch->ch_entry);
		nvgpu_rwsem_up_write(&tsg->ch_list_lock);
	}

	nvgpu_ref_put(&tsg->refcount, gk20a_tsg_release);
	ch->tsgid = NVGPU_INVALID_TSG_ID;

	nvgpu_log(g, gpu_dbg_fn, "UNBIND tsg:%d channel:%d\n",
					tsg->tsgid, ch->chid);

	return 0;
}


void nvgpu_tsg_recover(struct gk20a *g, struct tsg_gk20a *tsg,
			 bool verbose, u32 rc_type)
{
	u32 engines;

	/* stop context switching to prevent engine assignments from
	   changing until TSG is recovered */
	nvgpu_mutex_acquire(&g->dbg_sessions_lock);
	gr_gk20a_disable_ctxsw(g);

	engines = g->ops.fifo.get_engines_mask_on_id(g, tsg->tsgid, true);

	if (engines != 0U) {
		gk20a_fifo_recover(g, engines, tsg->tsgid, true, true, verbose,
					rc_type);
	} else {
		if (nvgpu_tsg_mark_error(g, tsg) && verbose) {
			gk20a_debug_dump(g);
		}

		gk20a_fifo_abort_tsg(g, tsg, false);
	}

	gr_gk20a_enable_ctxsw(g);
	nvgpu_mutex_release(&g->dbg_sessions_lock);
}


int gk20a_init_tsg_support(struct gk20a *g, u32 tsgid)
{
	struct tsg_gk20a *tsg = NULL;
	int err;

	if (tsgid >= g->fifo.num_channels) {
		return -EINVAL;
	}

	tsg = &g->fifo.tsg[tsgid];

	tsg->in_use = false;
	tsg->tsgid = tsgid;

	nvgpu_init_list_node(&tsg->ch_list);
	nvgpu_rwsem_init(&tsg->ch_list_lock);

	nvgpu_init_list_node(&tsg->event_id_list);
	err = nvgpu_mutex_init(&tsg->event_id_list_lock);
	if (err != 0) {
		tsg->in_use = true; /* make this TSG unusable */
		return err;
	}

	return 0;
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

void nvgpu_tsg_set_ctx_mmu_error(struct gk20a *g,
		struct tsg_gk20a *tsg)
{
	struct channel_gk20a *ch = NULL;

	nvgpu_err(g, "TSG %d generated a mmu fault", tsg->tsgid);

	nvgpu_rwsem_down_read(&tsg->ch_list_lock);
	nvgpu_list_for_each_entry(ch, &tsg->ch_list, channel_gk20a, ch_entry) {
		if (gk20a_channel_get(ch) != NULL) {
			nvgpu_channel_set_ctx_mmu_error(g, ch);
			gk20a_channel_put(ch);
		}
	}
	nvgpu_rwsem_up_read(&tsg->ch_list_lock);

}

bool nvgpu_tsg_check_ctxsw_timeout(struct tsg_gk20a *tsg,
		bool *verbose, u32 *ms)
{
	struct channel_gk20a *ch;
	bool recover = false;
	bool progress = false;
	struct gk20a *g = tsg->g;

	*verbose = false;
	*ms = g->fifo_eng_timeout_us / 1000U;

	nvgpu_rwsem_down_read(&tsg->ch_list_lock);

	/* check if there was some progress on any of the TSG channels.
	 * fifo recovery is needed if at least one channel reached the
	 * maximum timeout without progress (update in gpfifo pointers).
	 */
	nvgpu_list_for_each_entry(ch, &tsg->ch_list, channel_gk20a, ch_entry) {
		if (gk20a_channel_get(ch) != NULL) {
			recover = gk20a_channel_update_and_check_timeout(ch,
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
		 * which channel caused the problem, so set timeout error
		 * notifier for all channels.
		 */
		nvgpu_log_info(g, "timeout on tsg=%d ch=%d",
				tsg->tsgid, ch->chid);
		*ms = ch->timeout_accumulated_ms;
		gk20a_channel_put(ch);
		nvgpu_list_for_each_entry(ch, &tsg->ch_list,
				channel_gk20a, ch_entry) {
			if (gk20a_channel_get(ch) != NULL) {
				ch->g->ops.fifo.set_error_notifier(ch,
					NVGPU_ERR_NOTIFIER_FIFO_ERROR_IDLE_TIMEOUT);
				if (ch->timeout_debug_dump) {
					*verbose = true;
				}
				gk20a_channel_put(ch);
			}
		}
	} else if (progress) {
		/*
		 * if at least one channel in the TSG made some progress, reset
		 * accumulated timeout for all channels in the TSG. In
		 * particular, this resets timeout for channels that already
		 * completed their work
		 */
		nvgpu_log_info(g, "progress on tsg=%d ch=%d",
				tsg->tsgid, ch->chid);
		gk20a_channel_put(ch);
		*ms = g->fifo_eng_timeout_us / 1000U;
		nvgpu_list_for_each_entry(ch, &tsg->ch_list,
				channel_gk20a, ch_entry) {
			if (gk20a_channel_get(ch) != NULL) {
				ch->timeout_accumulated_ms = *ms;
				gk20a_channel_put(ch);
			}
		}
	}

	/* if we could not detect progress on any of the channel, but none
	 * of them has reached the timeout, there is nothing more to do:
	 * timeout_accumulated_ms has been updated for all of them.
	 */
	nvgpu_rwsem_up_read(&tsg->ch_list_lock);
	return recover;
}

int gk20a_tsg_set_runlist_interleave(struct tsg_gk20a *tsg, u32 level)
{
	struct gk20a *g = tsg->g;
	int ret;

	nvgpu_log(g, gpu_dbg_sched, "tsgid=%u interleave=%u", tsg->tsgid, level);

	nvgpu_speculation_barrier();
	switch (level) {
	case NVGPU_FIFO_RUNLIST_INTERLEAVE_LEVEL_LOW:
	case NVGPU_FIFO_RUNLIST_INTERLEAVE_LEVEL_MEDIUM:
	case NVGPU_FIFO_RUNLIST_INTERLEAVE_LEVEL_HIGH:
		ret = g->ops.fifo.set_runlist_interleave(g, tsg->tsgid,
							0, level);
		if (ret == 0) {
			tsg->interleave_level = level;
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return (ret != 0) ? ret : g->ops.fifo.update_runlist(g,
							tsg->runlist_id,
							FIFO_INVAL_CHANNEL_ID,
							true,
							true);
}

int gk20a_tsg_set_timeslice(struct tsg_gk20a *tsg, u32 timeslice)
{
	struct gk20a *g = tsg->g;

	nvgpu_log(g, gpu_dbg_sched, "tsgid=%u timeslice=%u us", tsg->tsgid, timeslice);

	return g->ops.fifo.tsg_set_timeslice(tsg, timeslice);
}

u32 gk20a_tsg_get_timeslice(struct tsg_gk20a *tsg)
{
	struct gk20a *g = tsg->g;

	if (tsg->timeslice_us == 0U) {
		return g->ops.fifo.default_timeslice_us(g);
	}

	return tsg->timeslice_us;
}

void gk20a_tsg_enable_sched(struct gk20a *g, struct tsg_gk20a *tsg)
{
	gk20a_fifo_set_runlist_state(g, BIT32(tsg->runlist_id),
			RUNLIST_ENABLED);

}

void gk20a_tsg_disable_sched(struct gk20a *g, struct tsg_gk20a *tsg)
{
	gk20a_fifo_set_runlist_state(g, BIT32(tsg->runlist_id),
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

int gk20a_tsg_open_common(struct gk20a *g, struct tsg_gk20a *tsg)
{
	int err;

	/* we need to allocate this after g->ops.gr.init_fs_state() since
	 * we initialize gr->no_of_sm in this function
	 */
	if (g->gr.no_of_sm == 0U) {
		nvgpu_err(g, "no_of_sm %d not set, failed allocation",
				  g->gr.no_of_sm);
		return -EINVAL;
	}

	err = gk20a_tsg_alloc_sm_error_states_mem(g, tsg, g->gr.no_of_sm);
	if (err != 0) {
		return err;
	}

	tsg->g = g;
	tsg->num_active_channels = 0U;
	nvgpu_ref_init(&tsg->refcount);

	tsg->vm = NULL;
	tsg->interleave_level = NVGPU_FIFO_RUNLIST_INTERLEAVE_LEVEL_LOW;
	tsg->timeslice_us = 0U;
	tsg->timeslice_timeout = 0U;
	tsg->timeslice_scale = 0U;
	tsg->runlist_id = FIFO_INVAL_TSG_ID;
	tsg->sm_exception_mask_type = NVGPU_SM_EXCEPTION_TYPE_MASK_NONE;
	tsg->gr_ctx = nvgpu_kzalloc(g, sizeof(*tsg->gr_ctx));
	if (tsg->gr_ctx == NULL) {
		err = -ENOMEM;
		goto clean_up;
	}

	if (g->ops.fifo.init_eng_method_buffers != NULL) {
		g->ops.fifo.init_eng_method_buffers(g, tsg);
	}

	if (g->ops.fifo.tsg_open != NULL) {
		err = g->ops.fifo.tsg_open(tsg);
		if (err != 0) {
			nvgpu_err(g, "tsg %d fifo open failed %d",
				  tsg->tsgid, err);
			goto clean_up;
		}
	}

	return 0;

clean_up:
	gk20a_tsg_release_common(g, tsg);
	nvgpu_ref_put(&tsg->refcount, NULL);

	return err;
}

struct tsg_gk20a *gk20a_tsg_open(struct gk20a *g, pid_t pid)
{
	struct tsg_gk20a *tsg;
	int err;

	tsg = gk20a_tsg_acquire_unused_tsg(&g->fifo);
	if (tsg == NULL) {
		return NULL;
	}

	err = gk20a_tsg_open_common(g, tsg);
	if (err != 0) {
		release_used_tsg(&g->fifo, tsg);
		nvgpu_err(g, "tsg %d open failed %d", tsg->tsgid, err);
		return NULL;
	}

	tsg->tgid = pid;

	nvgpu_log(g, gpu_dbg_fn, "tsg opened %d\n", tsg->tsgid);

	return tsg;
}

void gk20a_tsg_release_common(struct gk20a *g, struct tsg_gk20a *tsg)
{
	if (g->ops.fifo.tsg_release != NULL) {
		g->ops.fifo.tsg_release(tsg);
	}

	nvgpu_kfree(g, tsg->gr_ctx);
	tsg->gr_ctx = NULL;

	if (g->ops.fifo.deinit_eng_method_buffers != NULL) {
		g->ops.fifo.deinit_eng_method_buffers(g, tsg);
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

void gk20a_tsg_release(struct nvgpu_ref *ref)
{
	struct tsg_gk20a *tsg = tsg_gk20a_from_ref(ref);
	struct gk20a *g = tsg->g;
	struct gk20a_event_id_data *event_id_data, *event_id_data_temp;

	if (tsg->gr_ctx != NULL && nvgpu_mem_is_valid(&tsg->gr_ctx->mem)) {
		gr_gk20a_free_tsg_gr_ctx(tsg);
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

	gk20a_tsg_release_common(g, tsg);
	release_used_tsg(&g->fifo, tsg);

	nvgpu_log(g, gpu_dbg_fn, "tsg released %d\n", tsg->tsgid);
}

struct tsg_gk20a *tsg_gk20a_from_ch(struct channel_gk20a *ch)
{
	struct tsg_gk20a *tsg = NULL;

	if (gk20a_is_channel_marked_as_tsg(ch)) {
		struct gk20a *g = ch->g;
		struct fifo_gk20a *f = &g->fifo;
		tsg = &f->tsg[ch->tsgid];
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
