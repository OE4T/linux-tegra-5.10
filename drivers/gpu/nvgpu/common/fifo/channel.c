/*
 * GK20A Graphics channel
 *
 * Copyright (c) 2011-2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <trace/events/gk20a.h>

#include <nvgpu/semaphore.h>
#include <nvgpu/timers.h>
#include <nvgpu/kmem.h>
#include <nvgpu/dma.h>
#include <nvgpu/log.h>
#include <nvgpu/atomic.h>
#include <nvgpu/bug.h>
#include <nvgpu/list.h>
#include <nvgpu/circ_buf.h>
#include <nvgpu/cond.h>
#include <nvgpu/enabled.h>
#include <nvgpu/debug.h>
#include <nvgpu/debugger.h>
#include <nvgpu/ltc.h>
#include <nvgpu/barrier.h>
#include <nvgpu/error_notifier.h>
#include <nvgpu/os_sched.h>
#include <nvgpu/log2.h>
#include <nvgpu/ptimer.h>
#include <nvgpu/worker.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/gr/gr.h>
#include <nvgpu/channel.h>
#include <nvgpu/channel_sync.h>
#include <nvgpu/channel_sync_syncpt.h>
#include <nvgpu/runlist.h>
#include <nvgpu/fifo/userd.h>
#include <nvgpu/fence.h>
#include <nvgpu/preempt.h>

static void free_channel(struct fifo_gk20a *f, struct channel_gk20a *ch);
static void gk20a_channel_dump_ref_actions(struct channel_gk20a *ch);

static void channel_gk20a_free_priv_cmdbuf(struct channel_gk20a *ch);

static void channel_gk20a_free_prealloc_resources(struct channel_gk20a *c);

static void channel_gk20a_joblist_add(struct channel_gk20a *c,
		struct channel_gk20a_job *job);
static void channel_gk20a_joblist_delete(struct channel_gk20a *c,
		struct channel_gk20a_job *job);
static struct channel_gk20a_job *channel_gk20a_joblist_peek(
		struct channel_gk20a *c);

/* allocate GPU channel */
static struct channel_gk20a *allocate_channel(struct fifo_gk20a *f)
{
	struct channel_gk20a *ch = NULL;
	struct gk20a *g = f->g;

	nvgpu_mutex_acquire(&f->free_chs_mutex);
	if (!nvgpu_list_empty(&f->free_chs)) {
		ch = nvgpu_list_first_entry(&f->free_chs, channel_gk20a,
							  free_chs);
		nvgpu_list_del(&ch->free_chs);
		WARN_ON(nvgpu_atomic_read(&ch->ref_count) != 0);
		WARN_ON(ch->referenceable);
		f->used_channels++;
	}
	nvgpu_mutex_release(&f->free_chs_mutex);

	if ((g->aggressive_sync_destroy_thresh != 0U) &&
			(f->used_channels >
			 g->aggressive_sync_destroy_thresh)) {
		g->aggressive_sync_destroy = true;
	}

	return ch;
}

static void free_channel(struct fifo_gk20a *f,
		struct channel_gk20a *ch)
{
	struct gk20a *g = f->g;

	trace_gk20a_release_used_channel(ch->chid);
	/* refcount is zero here and channel is in a freed/dead state */
	nvgpu_mutex_acquire(&f->free_chs_mutex);
	/* add to head to increase visibility of timing-related bugs */
	nvgpu_list_add(&ch->free_chs, &f->free_chs);
	f->used_channels--;
	nvgpu_mutex_release(&f->free_chs_mutex);

	/*
	 * On teardown it is not possible to dereference platform, but ignoring
	 * this is fine then because no new channels would be created.
	 */
	if (!nvgpu_is_enabled(g, NVGPU_DRIVER_IS_DYING)) {
		if ((g->aggressive_sync_destroy_thresh != 0U) &&
			(f->used_channels <
			 g->aggressive_sync_destroy_thresh)) {
			g->aggressive_sync_destroy = false;
		}
	}
}

int channel_gk20a_commit_va(struct channel_gk20a *c)
{
	struct gk20a *g = c->g;

	nvgpu_log_fn(g, " ");

	g->ops.mm.init_inst_block(&c->inst_block, c->vm,
			c->vm->gmmu_page_sizes[GMMU_PAGE_SIZE_BIG]);

	return 0;
}

int channel_gk20a_update_runlist(struct channel_gk20a *c, bool add)
{
	return c->g->ops.runlist.update_for_channel(c->g, c->runlist_id,
			c, add, true);
}

int gk20a_enable_channel_tsg(struct gk20a *g, struct channel_gk20a *ch)
{
	struct tsg_gk20a *tsg;

	tsg = tsg_gk20a_from_ch(ch);
	if (tsg != NULL) {
		g->ops.tsg.enable(tsg);
		return 0;
	} else {
		return -EINVAL;
	}
}

int gk20a_disable_channel_tsg(struct gk20a *g, struct channel_gk20a *ch)
{
	struct tsg_gk20a *tsg;

	tsg = tsg_gk20a_from_ch(ch);
	if (tsg != NULL) {
		g->ops.tsg.disable(tsg);
		return 0;
	} else {
		return -EINVAL;
	}
}

void nvgpu_channel_abort_clean_up(struct channel_gk20a *ch)
{
	/* synchronize with actual job cleanup */
	nvgpu_mutex_acquire(&ch->joblist.cleanup_lock);

	/* ensure no fences are pending */
	nvgpu_mutex_acquire(&ch->sync_lock);
	if (ch->sync != NULL) {
		nvgpu_channel_sync_set_min_eq_max(ch->sync);
	}
	if (ch->user_sync != NULL) {
		nvgpu_channel_sync_set_safe_state(ch->user_sync);
	}
	nvgpu_mutex_release(&ch->sync_lock);

	nvgpu_mutex_release(&ch->joblist.cleanup_lock);

	/*
	 * When closing the channel, this scheduled update holds one ref which
	 * is waited for before advancing with freeing.
	 */
	gk20a_channel_update(ch);
}

void gk20a_channel_set_unserviceable(struct channel_gk20a *ch)
{
	nvgpu_spinlock_acquire(&ch->unserviceable_lock);
	ch->unserviceable = true;
	nvgpu_spinlock_release(&ch->unserviceable_lock);
}

bool  gk20a_channel_check_unserviceable(struct channel_gk20a *ch)
{
	bool unserviceable_status;

	nvgpu_spinlock_acquire(&ch->unserviceable_lock);
	unserviceable_status = ch->unserviceable;
	nvgpu_spinlock_release(&ch->unserviceable_lock);

	return unserviceable_status;
}

void gk20a_channel_abort(struct channel_gk20a *ch, bool channel_preempt)
{
	struct tsg_gk20a *tsg = tsg_gk20a_from_ch(ch);

	nvgpu_log_fn(ch->g, " ");

	if (tsg != NULL) {
		return nvgpu_tsg_abort(ch->g, tsg, channel_preempt);
	} else {
		nvgpu_err(ch->g, "chid: %d is not bound to tsg", ch->chid);
	}
}

int gk20a_wait_channel_idle(struct channel_gk20a *ch)
{
	bool channel_idle = false;
	struct nvgpu_timeout timeout;

	nvgpu_timeout_init(ch->g, &timeout, nvgpu_get_poll_timeout(ch->g),
			   NVGPU_TIMER_CPU_TIMER);

	do {
		channel_gk20a_joblist_lock(ch);
		channel_idle = channel_gk20a_joblist_is_empty(ch);
		channel_gk20a_joblist_unlock(ch);
		if (channel_idle) {
			break;
		}

		nvgpu_usleep_range(1000, 3000);
	} while (nvgpu_timeout_expired(&timeout) == 0);

	if (!channel_idle) {
		nvgpu_err(ch->g, "jobs not freed for channel %d",
				ch->chid);
		return -EBUSY;
	}

	return 0;
}

void gk20a_wait_until_counter_is_N(
	struct channel_gk20a *ch, nvgpu_atomic_t *counter, int wait_value,
	struct nvgpu_cond *c, const char *caller, const char *counter_name)
{
	while (true) {
		if (NVGPU_COND_WAIT(
			    c,
			    nvgpu_atomic_read(counter) == wait_value,
			    5000U) == 0) {
			break;
		}

		nvgpu_warn(ch->g,
			   "%s: channel %d, still waiting, %s left: %d, waiting for: %d",
			   caller, ch->chid, counter_name,
			   nvgpu_atomic_read(counter), wait_value);

		gk20a_channel_dump_ref_actions(ch);
	}
}

/* call ONLY when no references to the channel exist: after the last put */
static void gk20a_free_channel(struct channel_gk20a *ch, bool force)
{
	struct gk20a *g = ch->g;
	struct tsg_gk20a *tsg;
	struct fifo_gk20a *f = &g->fifo;
	struct vm_gk20a *ch_vm = ch->vm;
	unsigned long timeout = nvgpu_get_poll_timeout(g);
	struct dbg_session_gk20a *dbg_s;
	struct dbg_session_data *session_data, *tmp_s;
	struct dbg_session_channel_data *ch_data, *tmp;
	int err;
	bool deferred_reset_pending;

	nvgpu_log_fn(g, " ");

	WARN_ON(ch->g == NULL);

	trace_gk20a_free_channel(ch->chid);

	/*
	 * Disable channel/TSG and unbind here. This should not be executed if
	 * HW access is not available during shutdown/removal path as it will
	 * trigger a timeout
	 */
	if (!nvgpu_is_enabled(g, NVGPU_DRIVER_IS_DYING)) {
		/* abort channel and remove from runlist */
		tsg = tsg_gk20a_from_ch(ch);
		if (tsg != NULL) {
			/* Between tsg is not null and unbind_channel call,
			 * ioctl cannot be called anymore because user doesn't
			 * have an open channel fd anymore to use for the unbind
			 * ioctl.
			 */
			err = nvgpu_tsg_unbind_channel(tsg, ch);
			if (err != 0) {
				nvgpu_err(g,
					"failed to unbind channel %d from TSG",
					ch->chid);
			}
		} else {
			/*
			 * Channel is already unbound from TSG by User with
			 * explicit call
			 * Nothing to do here in that case
			 */
		}
	}

	/*
	 * OS channel close may require that syncpoint should be set to some
	 * safe value before it is called. nvgpu_tsg_unbind_channel(above) is
	 * internally doing that by calling nvgpu_nvhost_syncpt_set_safe_state
	 * deep down in the stack. Otherwise os_channel close may block if the
	 * app is killed abruptly (which was going to do the syncpoint signal).
	 */
	if (g->os_channel.close != NULL) {
		g->os_channel.close(ch, force);
	}

	/* wait until there's only our ref to the channel */
	if (!force) {
		gk20a_wait_until_counter_is_N(
			ch, &ch->ref_count, 1, &ch->ref_count_dec_wq,
			__func__, "references");
	}

	/* wait until all pending interrupts for recently completed
	 * jobs are handled */
	nvgpu_wait_for_deferred_interrupts(g);

	/* prevent new refs */
	nvgpu_spinlock_acquire(&ch->ref_obtain_lock);
	if (!ch->referenceable) {
		nvgpu_spinlock_release(&ch->ref_obtain_lock);
		nvgpu_err(ch->g,
			  "Extra %s() called to channel %u",
			  __func__, ch->chid);
		return;
	}
	ch->referenceable = false;
	nvgpu_spinlock_release(&ch->ref_obtain_lock);

	/* matches with the initial reference in gk20a_open_new_channel() */
	nvgpu_atomic_dec(&ch->ref_count);

	/* wait until no more refs to the channel */
	if (!force) {
		gk20a_wait_until_counter_is_N(
			ch, &ch->ref_count, 0, &ch->ref_count_dec_wq,
			__func__, "references");
	}

	/* if engine reset was deferred, perform it now */
	nvgpu_mutex_acquire(&f->deferred_reset_mutex);
	deferred_reset_pending = g->fifo.deferred_reset_pending;
	nvgpu_mutex_release(&f->deferred_reset_mutex);

	if (deferred_reset_pending) {
		nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg, "engine reset was"
				" deferred, running now");
		nvgpu_mutex_acquire(&g->fifo.engines_reset_mutex);
		nvgpu_channel_deferred_reset_engines(g, ch);
		nvgpu_mutex_release(&g->fifo.engines_reset_mutex);
	}

	if (!gk20a_channel_as_bound(ch)) {
		goto unbind;
	}

	nvgpu_log_info(g, "freeing bound channel context, timeout=%ld",
			timeout);

#ifdef CONFIG_GK20A_CTXSW_TRACE
	if (g->ops.gr.fecs_trace.unbind_channel && !ch->vpr)
		g->ops.gr.fecs_trace.unbind_channel(g, &ch->inst_block);
#endif

	if (g->ops.gr.setup.free_subctx != NULL) {
		g->ops.gr.setup.free_subctx(ch);
		ch->subctx = NULL;
	}

	if (ch->usermode_submit_enabled) {
		gk20a_channel_free_usermode_buffers(ch);
		(void) nvgpu_userd_init_channel(g, ch);
		ch->usermode_submit_enabled = false;
	}

	nvgpu_gr_flush_channel_tlb(g);

	nvgpu_dma_unmap_free(ch_vm, &ch->gpfifo.mem);
	nvgpu_big_free(g, ch->gpfifo.pipe);
	(void) memset(&ch->gpfifo, 0, sizeof(struct gpfifo_desc));

	channel_gk20a_free_priv_cmdbuf(ch);

	/* sync must be destroyed before releasing channel vm */
	nvgpu_mutex_acquire(&ch->sync_lock);
	if (ch->sync != NULL) {
		nvgpu_channel_sync_destroy(ch->sync, false);
		ch->sync = NULL;
	}
	if (ch->user_sync != NULL) {
		/*
		 * Set user managed syncpoint to safe state
		 * But it's already done if channel is recovered
		 */
		if (gk20a_channel_check_unserviceable(ch)) {
			nvgpu_channel_sync_destroy(ch->user_sync, false);
		} else {
			nvgpu_channel_sync_destroy(ch->user_sync, true);
		}
		ch->user_sync = NULL;
	}
	nvgpu_mutex_release(&ch->sync_lock);

	/*
	 * free the channel used semaphore index.
	 * we need to do this before releasing the address space,
	 * as the semaphore pool might get freed after that point.
	 */
	if (ch->hw_sema != NULL) {
		nvgpu_hw_semaphore_free(ch);
	}

	/*
	 * When releasing the channel we unbind the VM - so release the ref.
	 */
	nvgpu_vm_put(ch_vm);

	/* make sure we don't have deferred interrupts pending that
	 * could still touch the channel */
	nvgpu_wait_for_deferred_interrupts(g);

unbind:
	g->ops.channel.unbind(ch);
	g->ops.channel.free_inst(g, ch);

	/* put back the channel-wide submit ref from init */
	if (ch->deterministic) {
		nvgpu_rwsem_down_read(&g->deterministic_busy);
		ch->deterministic = false;
		if (!ch->deterministic_railgate_allowed) {
			gk20a_idle(g);
		}
		ch->deterministic_railgate_allowed = false;

		nvgpu_rwsem_up_read(&g->deterministic_busy);
	}

	ch->vpr = false;
	ch->vm = NULL;

	WARN_ON(ch->sync != NULL);

	/* unlink all debug sessions */
	nvgpu_mutex_acquire(&g->dbg_sessions_lock);

	nvgpu_list_for_each_entry_safe(session_data, tmp_s,
			&ch->dbg_s_list, dbg_session_data, dbg_s_entry) {
		dbg_s = session_data->dbg_s;
		nvgpu_mutex_acquire(&dbg_s->ch_list_lock);
		nvgpu_list_for_each_entry_safe(ch_data, tmp, &dbg_s->ch_list,
				dbg_session_channel_data, ch_entry) {
			if (ch_data->chid == ch->chid) {
				ch_data->unbind_single_channel(dbg_s, ch_data);
			}
		}
		nvgpu_mutex_release(&dbg_s->ch_list_lock);
	}

	nvgpu_mutex_release(&g->dbg_sessions_lock);

	/* free pre-allocated resources, if applicable */
	if (channel_gk20a_is_prealloc_enabled(ch)) {
		channel_gk20a_free_prealloc_resources(ch);
	}

#if GK20A_CHANNEL_REFCOUNT_TRACKING
	(void) memset(ch->ref_actions, 0, sizeof(ch->ref_actions));
	ch->ref_actions_put = 0;
#endif

	/* make sure we catch accesses of unopened channels in case
	 * there's non-refcounted channel pointers hanging around */
	ch->g = NULL;
	nvgpu_smp_wmb();

	/* ALWAYS last */
	free_channel(f, ch);
}

static void gk20a_channel_dump_ref_actions(struct channel_gk20a *ch)
{
#if GK20A_CHANNEL_REFCOUNT_TRACKING
	size_t i, get;
	s64 now = nvgpu_current_time_ms();
	s64 prev = 0;
	struct gk20a *g = ch->g;

	nvgpu_spinlock_acquire(&ch->ref_actions_lock);

	nvgpu_info(g, "ch %d: refs %d. Actions, most recent last:",
			ch->chid, nvgpu_atomic_read(&ch->ref_count));

	/* start at the oldest possible entry. put is next insertion point */
	get = ch->ref_actions_put;

	/*
	 * If the buffer is not full, this will first loop to the oldest entry,
	 * skipping not-yet-initialized entries. There is no ref_actions_get.
	 */
	for (i = 0; i < GK20A_CHANNEL_REFCOUNT_TRACKING; i++) {
		struct channel_gk20a_ref_action *act = &ch->ref_actions[get];

		if (act->trace.nr_entries) {
			nvgpu_info(g,
				"%s ref %zu steps ago (age %lld ms, diff %lld ms)",
				act->type == channel_gk20a_ref_action_get
					? "GET" : "PUT",
				GK20A_CHANNEL_REFCOUNT_TRACKING - 1 - i,
				now - act->timestamp_ms,
				act->timestamp_ms - prev);

			print_stack_trace(&act->trace, 0);
			prev = act->timestamp_ms;
		}

		get = (get + 1) % GK20A_CHANNEL_REFCOUNT_TRACKING;
	}

	nvgpu_spinlock_release(&ch->ref_actions_lock);
#endif
}

static void gk20a_channel_save_ref_source(struct channel_gk20a *ch,
		enum channel_gk20a_ref_action_type type)
{
#if GK20A_CHANNEL_REFCOUNT_TRACKING
	struct channel_gk20a_ref_action *act;

	nvgpu_spinlock_acquire(&ch->ref_actions_lock);

	act = &ch->ref_actions[ch->ref_actions_put];
	act->type = type;
	act->trace.max_entries = GK20A_CHANNEL_REFCOUNT_TRACKING_STACKLEN;
	act->trace.nr_entries = 0;
	act->trace.skip = 3; /* onwards from the caller of this */
	act->trace.entries = act->trace_entries;
	save_stack_trace(&act->trace);
	act->timestamp_ms = nvgpu_current_time_ms();
	ch->ref_actions_put = (ch->ref_actions_put + 1) %
		GK20A_CHANNEL_REFCOUNT_TRACKING;

	nvgpu_spinlock_release(&ch->ref_actions_lock);
#endif
}

/* Try to get a reference to the channel. Return nonzero on success. If fails,
 * the channel is dead or being freed elsewhere and you must not touch it.
 *
 * Always when a channel_gk20a pointer is seen and about to be used, a
 * reference must be held to it - either by you or the caller, which should be
 * documented well or otherwise clearly seen. This usually boils down to the
 * file from ioctls directly, or an explicit get in exception handlers when the
 * channel is found by a chid.
 *
 * Most global functions in this file require a reference to be held by the
 * caller.
 */
struct channel_gk20a *_gk20a_channel_get(struct channel_gk20a *ch,
					 const char *caller) {
	struct channel_gk20a *ret;

	nvgpu_spinlock_acquire(&ch->ref_obtain_lock);

	if (likely(ch->referenceable)) {
		gk20a_channel_save_ref_source(ch, channel_gk20a_ref_action_get);
		nvgpu_atomic_inc(&ch->ref_count);
		ret = ch;
	} else {
		ret = NULL;
	}

	nvgpu_spinlock_release(&ch->ref_obtain_lock);

	if (ret != NULL) {
		trace_gk20a_channel_get(ch->chid, caller);
	}

	return ret;
}

void _gk20a_channel_put(struct channel_gk20a *ch, const char *caller)
{
	gk20a_channel_save_ref_source(ch, channel_gk20a_ref_action_put);
	trace_gk20a_channel_put(ch->chid, caller);
	nvgpu_atomic_dec(&ch->ref_count);
	nvgpu_cond_broadcast(&ch->ref_count_dec_wq);

	/* More puts than gets. Channel is probably going to get
	 * stuck. */
	WARN_ON(nvgpu_atomic_read(&ch->ref_count) < 0);

	/* Also, more puts than gets. ref_count can go to 0 only if
	 * the channel is closing. Channel is probably going to get
	 * stuck. */
	WARN_ON(nvgpu_atomic_read(&ch->ref_count) == 0 && ch->referenceable);
}

struct channel_gk20a *_gk20a_channel_from_id(struct gk20a *g, u32 chid,
					 const char *caller)
{
	if (chid == FIFO_INVAL_CHANNEL_ID) {
		return NULL;
	}

	return _gk20a_channel_get(&g->fifo.channel[chid], caller);
}

void gk20a_channel_close(struct channel_gk20a *ch)
{
	gk20a_free_channel(ch, false);
}

/*
 * Be careful with this - it is meant for terminating channels when we know the
 * driver is otherwise dying. Ref counts and the like are ignored by this
 * version of the cleanup.
 */
void __gk20a_channel_kill(struct channel_gk20a *ch)
{
	gk20a_free_channel(ch, true);
}

struct channel_gk20a *gk20a_open_new_channel(struct gk20a *g,
		u32 runlist_id,
		bool is_privileged_channel,
		pid_t pid, pid_t tid)
{
	struct fifo_gk20a *f = &g->fifo;
	struct channel_gk20a *ch;

	/* compatibility with existing code */
	if (!nvgpu_engine_is_valid_runlist_id(g, runlist_id)) {
		runlist_id = nvgpu_engine_get_gr_runlist_id(g);
	}

	nvgpu_log_fn(g, " ");

	ch = allocate_channel(f);
	if (ch == NULL) {
		/* TBD: we want to make this virtualizable */
		nvgpu_err(g, "out of hw chids");
		return NULL;
	}

	trace_gk20a_open_new_channel(ch->chid);

	BUG_ON(ch->g != NULL);
	ch->g = g;

	/* Runlist for the channel */
	ch->runlist_id = runlist_id;

	/* Channel privilege level */
	ch->is_privileged_channel = is_privileged_channel;

	ch->pid = tid;
	ch->tgid = pid;  /* process granularity for FECS traces */

	if (nvgpu_userd_init_channel(g, ch) != 0) {
		nvgpu_err(g, "userd init failed");
		goto clean_up;
	}

	if (g->ops.channel.alloc_inst(g, ch) != 0) {
		nvgpu_err(g, "inst allocation failed");
		goto clean_up;
	}

	/* now the channel is in a limbo out of the free list but not marked as
	 * alive and used (i.e. get-able) yet */

	/* By default, channel is regular (non-TSG) channel */
	ch->tsgid = NVGPU_INVALID_TSG_ID;

	/* clear ctxsw timeout counter and update timestamp */
	ch->ctxsw_timeout_accumulated_ms = 0;
	ch->ctxsw_timeout_gpfifo_get = 0;
	/* set gr host default timeout */
	ch->ctxsw_timeout_max_ms = nvgpu_get_poll_timeout(g);
	ch->ctxsw_timeout_debug_dump = true;
	ch->unserviceable = false;

	/* init kernel watchdog timeout */
	ch->wdt.enabled = true;
	ch->wdt.limit_ms = g->ch_wdt_init_limit_ms;
	ch->wdt.debug_dump = true;

	ch->obj_class = 0;
	ch->subctx_id = 0;
	ch->runqueue_sel = 0;

	ch->mmu_nack_handled = false;

	/* The channel is *not* runnable at this point. It still needs to have
	 * an address space bound and allocate a gpfifo and grctx. */

	nvgpu_cond_init(&ch->notifier_wq);
	nvgpu_cond_init(&ch->semaphore_wq);

	if (g->os_channel.open != NULL) {
		g->os_channel.open(ch);
	}

	/* Mark the channel alive, get-able, with 1 initial use
	 * references. The initial reference will be decreased in
	 * gk20a_free_channel().
	 *
	 * Use the lock, since an asynchronous thread could
	 * try to access this channel while it's not fully
	 * initialized.
	 */
	nvgpu_spinlock_acquire(&ch->ref_obtain_lock);
	ch->referenceable = true;
	nvgpu_atomic_set(&ch->ref_count, 1);
	nvgpu_spinlock_release(&ch->ref_obtain_lock);

	return ch;

clean_up:
	ch->g = NULL;
	free_channel(f, ch);
	return NULL;
}

/* allocate private cmd buffer.
   used for inserting commands before/after user submitted buffers. */
static int channel_gk20a_alloc_priv_cmdbuf(struct channel_gk20a *ch,
	u32 num_in_flight)
{
	struct gk20a *g = ch->g;
	struct vm_gk20a *ch_vm = ch->vm;
	struct priv_cmd_queue *q = &ch->priv_cmd_q;
	u64 size, tmp_size;
	int err = 0;
	bool gpfifo_based = false;

	if (num_in_flight == 0U) {
		num_in_flight = ch->gpfifo.entry_num;
		gpfifo_based = true;
	}

	/*
	 * Compute the amount of priv_cmdbuf space we need. In general the worst
	 * case is the kernel inserts both a semaphore pre-fence and post-fence.
	 * Any sync-pt fences will take less memory so we can ignore them for
	 * now.
	 *
	 * A semaphore ACQ (fence-wait) is 8 words: semaphore_a, semaphore_b,
	 * semaphore_c, and semaphore_d. A semaphore INCR (fence-get) will be 10
	 * words: all the same as an ACQ plus a non-stalling intr which is
	 * another 2 words.
	 *
	 * We have two cases to consider: the first is we base the size of the
	 * priv_cmd_buf on the gpfifo count. Here we multiply by a factor of
	 * 2/3rds because only at most 2/3rds of the GPFIFO can be used for
	 * sync commands:
	 *
	 *   nr_gpfifos * (2 / 3) * (8 + 10) * 4 bytes
	 *
	 * If instead num_in_flight is specified then we will use that to size
	 * the priv_cmd_buf. The worst case is two sync commands (one ACQ and
	 * one INCR) per submit so we have a priv_cmd_buf size of:
	 *
	 *   num_in_flight * (8 + 10) * 4 bytes
	 */
	size = num_in_flight * 18UL * sizeof(u32);
	if (gpfifo_based) {
		size = 2U * size / 3U;
	}

	tmp_size = PAGE_ALIGN(roundup_pow_of_two(size));
	nvgpu_assert(tmp_size <= U32_MAX);
	size = (u32)tmp_size;

	err = nvgpu_dma_alloc_map_sys(ch_vm, size, &q->mem);
	if (err != 0) {
		nvgpu_err(g, "%s: memory allocation failed", __func__);
		goto clean_up;
	}

	tmp_size = q->mem.size / sizeof(u32);
	nvgpu_assert(tmp_size <= U32_MAX);
	q->size = (u32)tmp_size;

	return 0;

clean_up:
	channel_gk20a_free_priv_cmdbuf(ch);
	return err;
}

static void channel_gk20a_free_priv_cmdbuf(struct channel_gk20a *ch)
{
	struct vm_gk20a *ch_vm = ch->vm;
	struct priv_cmd_queue *q = &ch->priv_cmd_q;

	if (q->size == 0U) {
		return;
	}

	nvgpu_dma_unmap_free(ch_vm, &q->mem);

	(void) memset(q, 0, sizeof(struct priv_cmd_queue));
}

/* allocate a cmd buffer with given size. size is number of u32 entries */
int gk20a_channel_alloc_priv_cmdbuf(struct channel_gk20a *c, u32 orig_size,
			     struct priv_cmd_entry *e)
{
	struct priv_cmd_queue *q = &c->priv_cmd_q;
	u32 free_count;
	u32 size = orig_size;

	nvgpu_log_fn(c->g, "size %d", orig_size);

	if (e == NULL) {
		nvgpu_err(c->g,
			"ch %d: priv cmd entry is null",
			c->chid);
		return -EINVAL;
	}

	/* if free space in the end is less than requested, increase the size
	 * to make the real allocated space start from beginning. */
	if (q->put + size > q->size) {
		size = orig_size + (q->size - q->put);
	}

	nvgpu_log_info(c->g, "ch %d: priv cmd queue get:put %d:%d",
			c->chid, q->get, q->put);

	free_count = (q->size - (q->put - q->get) - 1U) % q->size;

	if (size > free_count) {
		return -EAGAIN;
	}

	e->size = orig_size;
	e->mem = &q->mem;

	/* if we have increased size to skip free space in the end, set put
	   to beginning of cmd buffer (0) + size */
	if (size != orig_size) {
		e->off = 0;
		e->gva = q->mem.gpu_va;
		q->put = orig_size;
	} else {
		e->off = q->put;
		e->gva = q->mem.gpu_va + q->put * sizeof(u32);
		q->put = (q->put + orig_size) & (q->size - 1U);
	}

	/* we already handled q->put + size > q->size so BUG_ON this */
	BUG_ON(q->put > q->size);

	/*
	 * commit the previous writes before making the entry valid.
	 * see the corresponding nvgpu_smp_rmb() in gk20a_free_priv_cmdbuf().
	 */
	nvgpu_smp_wmb();

	e->valid = true;
	nvgpu_log_fn(c->g, "done");

	return 0;
}

/* Don't call this to free an explict cmd entry.
 * It doesn't update priv_cmd_queue get/put */
void free_priv_cmdbuf(struct channel_gk20a *c,
			     struct priv_cmd_entry *e)
{
	if (channel_gk20a_is_prealloc_enabled(c)) {
		(void) memset(e, 0, sizeof(struct priv_cmd_entry));
	} else {
		nvgpu_kfree(c->g, e);
	}
}

int channel_gk20a_alloc_job(struct channel_gk20a *c,
		struct channel_gk20a_job **job_out)
{
	int err = 0;

	if (channel_gk20a_is_prealloc_enabled(c)) {
		u32 put = c->joblist.pre_alloc.put;
		u32 get = c->joblist.pre_alloc.get;

		/*
		 * ensure all subsequent reads happen after reading get.
		 * see corresponding nvgpu_smp_wmb in
		 * gk20a_channel_clean_up_jobs()
		 */
		nvgpu_smp_rmb();

		if (CIRC_SPACE(put, get, c->joblist.pre_alloc.length) != 0) {
			*job_out = &c->joblist.pre_alloc.jobs[put];
		} else {
			nvgpu_warn(c->g,
					"out of job ringbuffer space");
			err = -EAGAIN;
		}
	} else {
		*job_out = nvgpu_kzalloc(c->g,
					 sizeof(struct channel_gk20a_job));
		if (*job_out == NULL) {
			err = -ENOMEM;
		}
	}

	return err;
}

void channel_gk20a_free_job(struct channel_gk20a *c,
		struct channel_gk20a_job *job)
{
	/*
	 * In case of pre_allocated jobs, we need to clean out
	 * the job but maintain the pointers to the priv_cmd_entry,
	 * since they're inherently tied to the job node.
	 */
	if (channel_gk20a_is_prealloc_enabled(c)) {
		struct priv_cmd_entry *wait_cmd = job->wait_cmd;
		struct priv_cmd_entry *incr_cmd = job->incr_cmd;
		(void) memset(job, 0, sizeof(*job));
		job->wait_cmd = wait_cmd;
		job->incr_cmd = incr_cmd;
	} else {
		nvgpu_kfree(c->g, job);
	}
}

void channel_gk20a_joblist_lock(struct channel_gk20a *c)
{
	if (channel_gk20a_is_prealloc_enabled(c)) {
		nvgpu_mutex_acquire(&c->joblist.pre_alloc.read_lock);
	} else {
		nvgpu_spinlock_acquire(&c->joblist.dynamic.lock);
	}
}

void channel_gk20a_joblist_unlock(struct channel_gk20a *c)
{
	if (channel_gk20a_is_prealloc_enabled(c)) {
		nvgpu_mutex_release(&c->joblist.pre_alloc.read_lock);
	} else {
		nvgpu_spinlock_release(&c->joblist.dynamic.lock);
	}
}

static struct channel_gk20a_job *channel_gk20a_joblist_peek(
		struct channel_gk20a *c)
{
	u32 get;
	struct channel_gk20a_job *job = NULL;

	if (channel_gk20a_is_prealloc_enabled(c)) {
		if (!channel_gk20a_joblist_is_empty(c)) {
			get = c->joblist.pre_alloc.get;
			job = &c->joblist.pre_alloc.jobs[get];
		}
	} else {
		if (!nvgpu_list_empty(&c->joblist.dynamic.jobs)) {
			job = nvgpu_list_first_entry(&c->joblist.dynamic.jobs,
				       channel_gk20a_job, list);
		}
	}

	return job;
}

static void channel_gk20a_joblist_add(struct channel_gk20a *c,
		struct channel_gk20a_job *job)
{
	if (channel_gk20a_is_prealloc_enabled(c)) {
		c->joblist.pre_alloc.put = (c->joblist.pre_alloc.put + 1U) %
				(c->joblist.pre_alloc.length);
	} else {
		nvgpu_list_add_tail(&job->list, &c->joblist.dynamic.jobs);
	}
}

static void channel_gk20a_joblist_delete(struct channel_gk20a *c,
		struct channel_gk20a_job *job)
{
	if (channel_gk20a_is_prealloc_enabled(c)) {
		c->joblist.pre_alloc.get = (c->joblist.pre_alloc.get + 1U) %
				(c->joblist.pre_alloc.length);
	} else {
		nvgpu_list_del(&job->list);
	}
}

bool channel_gk20a_joblist_is_empty(struct channel_gk20a *c)
{
	if (channel_gk20a_is_prealloc_enabled(c)) {
		u32 get = c->joblist.pre_alloc.get;
		u32 put = c->joblist.pre_alloc.put;
		return !(CIRC_CNT(put, get, c->joblist.pre_alloc.length));
	}

	return nvgpu_list_empty(&c->joblist.dynamic.jobs);
}

bool channel_gk20a_is_prealloc_enabled(struct channel_gk20a *c)
{
	bool pre_alloc_enabled = c->joblist.pre_alloc.enabled;

	nvgpu_smp_rmb();
	return pre_alloc_enabled;
}

static int channel_gk20a_prealloc_resources(struct channel_gk20a *ch,
	       unsigned int num_jobs)
{
	unsigned int i;
	int err;
	size_t size;
	struct priv_cmd_entry *entries = NULL;

	if ((channel_gk20a_is_prealloc_enabled(ch)) || (num_jobs == 0U)) {
		return -EINVAL;
	}

	/*
	 * pre-allocate the job list.
	 * since vmalloc take in an unsigned long, we need
	 * to make sure we don't hit an overflow condition
	 */
	size = sizeof(struct channel_gk20a_job);
	if (num_jobs <= ULONG_MAX / size) {
		ch->joblist.pre_alloc.jobs = nvgpu_vzalloc(ch->g,
							  num_jobs * size);
	}
	if (ch->joblist.pre_alloc.jobs == NULL) {
		err = -ENOMEM;
		goto clean_up;
	}

	/*
	 * pre-allocate 2x priv_cmd_entry for each job up front.
	 * since vmalloc take in an unsigned long, we need
	 * to make sure we don't hit an overflow condition
	 */
	size = sizeof(struct priv_cmd_entry);
	if (num_jobs <= ULONG_MAX / (size << 1)) {
		entries = nvgpu_vzalloc(ch->g,
					((unsigned long)num_jobs << 1UL) *
					(unsigned long)size);
	}
	if (entries == NULL) {
		err = -ENOMEM;
		goto clean_up_joblist;
	}

	for (i = 0; i < num_jobs; i++) {
		ch->joblist.pre_alloc.jobs[i].wait_cmd = &entries[i];
		ch->joblist.pre_alloc.jobs[i].incr_cmd =
			&entries[i + num_jobs];
	}

	/* pre-allocate a fence pool */
	err = nvgpu_fence_pool_alloc(ch, num_jobs);
	if (err != 0) {
		goto clean_up_priv_cmd;
	}

	ch->joblist.pre_alloc.length = num_jobs;
	ch->joblist.pre_alloc.put = 0;
	ch->joblist.pre_alloc.get = 0;

	/*
	 * commit the previous writes before setting the flag.
	 * see corresponding nvgpu_smp_rmb in
	 * channel_gk20a_is_prealloc_enabled()
	 */
	nvgpu_smp_wmb();
	ch->joblist.pre_alloc.enabled = true;

	return 0;

clean_up_priv_cmd:
	nvgpu_vfree(ch->g, entries);
clean_up_joblist:
	nvgpu_vfree(ch->g, ch->joblist.pre_alloc.jobs);
clean_up:
	(void) memset(&ch->joblist.pre_alloc, 0, sizeof(ch->joblist.pre_alloc));
	return err;
}

static void channel_gk20a_free_prealloc_resources(struct channel_gk20a *c)
{
	nvgpu_vfree(c->g, c->joblist.pre_alloc.jobs[0].wait_cmd);
	nvgpu_vfree(c->g, c->joblist.pre_alloc.jobs);
	nvgpu_fence_pool_free(c);

	/*
	 * commit the previous writes before disabling the flag.
	 * see corresponding nvgpu_smp_rmb in
	 * channel_gk20a_is_prealloc_enabled()
	 */
	nvgpu_smp_wmb();
	c->joblist.pre_alloc.enabled = false;
}

int nvgpu_channel_set_syncpt(struct channel_gk20a *ch)
{
	struct gk20a *g = ch->g;
	struct nvgpu_channel_sync_syncpt *sync_syncpt;
	u32 new_syncpt = 0U;
	u32 old_syncpt = g->ops.ramfc.get_syncpt(ch);

	if (ch->sync != NULL) {
		sync_syncpt = nvgpu_channel_sync_to_syncpt(ch->sync);
		if (sync_syncpt != NULL) {
			new_syncpt =
			    nvgpu_channel_sync_get_syncpt_id(sync_syncpt);
		} else {
			new_syncpt = FIFO_INVAL_SYNCPT_ID;
		}
	}

	if ((new_syncpt != 0U) && (new_syncpt != old_syncpt)) {
		/* disable channel */
		gk20a_disable_channel_tsg(g, ch);

		/* preempt the channel */
		WARN_ON(nvgpu_preempt_channel(g, ch) != 0);

		g->ops.ramfc.set_syncpt(ch, new_syncpt);
	}

	/* enable channel */
	gk20a_enable_channel_tsg(g, ch);

	nvgpu_log_fn(g, "done");

	return 0;
}

int nvgpu_channel_setup_bind(struct channel_gk20a *c,
		struct nvgpu_setup_bind_args *args)
{
	struct gk20a *g = c->g;
	struct vm_gk20a *ch_vm;
	u32 gpfifo_size, gpfifo_entry_size;
	u64 gpfifo_gpu_va;
	int err = 0;
	u64 pbdma_acquire_timeout;

	gpfifo_size = args->num_gpfifo_entries;
	gpfifo_entry_size = nvgpu_get_gpfifo_entry_size();

	if ((args->flags & NVGPU_SETUP_BIND_FLAGS_SUPPORT_VPR) != 0U) {
		c->vpr = true;
	}

	if ((args->flags & NVGPU_SETUP_BIND_FLAGS_SUPPORT_DETERMINISTIC) != 0U) {
		nvgpu_rwsem_down_read(&g->deterministic_busy);
		/*
		 * Railgating isn't deterministic; instead of disallowing
		 * railgating globally, take a power refcount for this
		 * channel's lifetime. The gk20a_idle() pair for this happens
		 * when the channel gets freed.
		 *
		 * Deterministic flag and this busy must be atomic within the
		 * busy lock.
		 */
		err = gk20a_busy(g);
		if (err != 0) {
			nvgpu_rwsem_up_read(&g->deterministic_busy);
			return err;
		}

		c->deterministic = true;
		nvgpu_rwsem_up_read(&g->deterministic_busy);
	}

	/* an address space needs to have been bound at this point. */
	if (!gk20a_channel_as_bound(c)) {
		nvgpu_err(g,
			"not bound to an address space at time of setup_bind");
		err = -EINVAL;
		goto clean_up_idle;
	}
	ch_vm = c->vm;

	if (nvgpu_mem_is_valid(&c->gpfifo.mem) ||
			c->usermode_submit_enabled) {
		nvgpu_err(g, "channel %d :"
			   "gpfifo already allocated", c->chid);
		err = -EEXIST;
		goto clean_up_idle;
	}

	if ((args->flags & NVGPU_SETUP_BIND_FLAGS_USERMODE_SUPPORT) != 0U) {
		if (g->os_channel.alloc_usermode_buffers != NULL) {
			err = g->os_channel.alloc_usermode_buffers(c, args);
			if (err != 0) {
				nvgpu_err(g, "Usermode buffer alloc failed");
				goto clean_up;
			}
			c->userd_iova = nvgpu_mem_get_addr(g,
				&c->usermode_userd);
			c->usermode_submit_enabled = true;
		} else {
			nvgpu_err(g, "Usermode submit not supported");
			err = -EINVAL;
			goto clean_up;
		}
		gpfifo_gpu_va = c->usermode_gpfifo.gpu_va;
	} else {
		err = nvgpu_dma_alloc_map_sys(ch_vm,
				(size_t)gpfifo_size * (size_t)gpfifo_entry_size,
				&c->gpfifo.mem);
		if (err != 0) {
			nvgpu_err(g, "memory allocation failed");
			goto clean_up;
		}

		if (c->gpfifo.mem.aperture == APERTURE_VIDMEM) {
			c->gpfifo.pipe = nvgpu_big_malloc(g,
						(size_t)gpfifo_size *
						(size_t)gpfifo_entry_size);
			if (c->gpfifo.pipe == NULL) {
				err = -ENOMEM;
				goto clean_up_unmap;
			}
		}
		gpfifo_gpu_va = c->gpfifo.mem.gpu_va;
	}

	c->gpfifo.entry_num = gpfifo_size;
	c->gpfifo.get = c->gpfifo.put = 0;

	nvgpu_log_info(g, "channel %d : gpfifo_base 0x%016llx, size %d",
		c->chid, gpfifo_gpu_va, c->gpfifo.entry_num);

	if (!c->usermode_submit_enabled) {
		g->ops.userd.init_mem(g, c);

		if (g->aggressive_sync_destroy_thresh == 0U) {
			nvgpu_mutex_acquire(&c->sync_lock);
			c->sync = nvgpu_channel_sync_create(c, false);
			if (c->sync == NULL) {
				err = -ENOMEM;
				nvgpu_mutex_release(&c->sync_lock);
				goto clean_up_unmap;
			}
			nvgpu_mutex_release(&c->sync_lock);

			if (g->ops.channel.set_syncpt != NULL) {
				err = g->ops.channel.set_syncpt(c);
				if (err != 0) {
					goto clean_up_sync;
				}
			}
		}
	}

	if (!nvgpu_is_timeouts_enabled(c->g) || !c->wdt.enabled) {
		pbdma_acquire_timeout = 0;
	} else {
		pbdma_acquire_timeout = c->wdt.limit_ms;
	}

	err = g->ops.ramfc.setup(c, gpfifo_gpu_va,
			c->gpfifo.entry_num, pbdma_acquire_timeout,
			args->flags);
	if (err != 0) {
		goto clean_up_sync;
	}

	/* TBD: setup engine contexts */

	if (c->deterministic && args->num_inflight_jobs != 0U) {
		err = channel_gk20a_prealloc_resources(c,
				args->num_inflight_jobs);
		if (err != 0) {
			goto clean_up_sync;
		}
	}

	err = channel_gk20a_alloc_priv_cmdbuf(c, args->num_inflight_jobs);
	if (err != 0) {
		goto clean_up_prealloc;
	}

	err = channel_gk20a_update_runlist(c, true);
	if (err != 0) {
		goto clean_up_priv_cmd;
	}

	g->ops.channel.bind(c);

	nvgpu_log_fn(g, "done");
	return 0;

clean_up_priv_cmd:
	channel_gk20a_free_priv_cmdbuf(c);
clean_up_prealloc:
	if (c->deterministic && args->num_inflight_jobs != 0U) {
		channel_gk20a_free_prealloc_resources(c);
	}
clean_up_sync:
	if (c->sync != NULL) {
		nvgpu_channel_sync_destroy(c->sync, false);
		c->sync = NULL;
	}
clean_up_unmap:
	nvgpu_big_free(g, c->gpfifo.pipe);
	nvgpu_dma_unmap_free(ch_vm, &c->gpfifo.mem);
	if (c->usermode_submit_enabled) {
		gk20a_channel_free_usermode_buffers(c);
		(void) nvgpu_userd_init_channel(g, c);
		c->usermode_submit_enabled = false;
	}
clean_up:
	(void) memset(&c->gpfifo, 0, sizeof(struct gpfifo_desc));
clean_up_idle:
	if (c->deterministic) {
		nvgpu_rwsem_down_read(&g->deterministic_busy);
		gk20a_idle(g);
		c->deterministic = false;
		nvgpu_rwsem_up_read(&g->deterministic_busy);
	}
	nvgpu_err(g, "fail");
	return err;
}

void gk20a_channel_free_usermode_buffers(struct channel_gk20a *c)
{
	if (nvgpu_mem_is_valid(&c->usermode_userd)) {
		nvgpu_dma_free(c->g, &c->usermode_userd);
	}
	if (nvgpu_mem_is_valid(&c->usermode_gpfifo)) {
		nvgpu_dma_unmap_free(c->vm, &c->usermode_gpfifo);
	}
	if (c->g->os_channel.free_usermode_buffers != NULL) {
		c->g->os_channel.free_usermode_buffers(c);
	}
}

/* Update with this periodically to determine how the gpfifo is draining. */
static inline u32 update_gp_get(struct gk20a *g,
				struct channel_gk20a *c)
{
	u32 new_get = g->ops.userd.gp_get(g, c);

	if (new_get < c->gpfifo.get) {
		c->gpfifo.wrap = !c->gpfifo.wrap;
	}
	c->gpfifo.get = new_get;
	return new_get;
}

u32 nvgpu_gp_free_count(struct channel_gk20a *c)
{
	return (c->gpfifo.entry_num - (c->gpfifo.put - c->gpfifo.get) - 1U) %
		c->gpfifo.entry_num;
}

static bool nvgpu_channel_ctxsw_timeout_debug_dump_state(struct gk20a *g,
		struct channel_gk20a *ch)
{
	bool verbose = false;
	if (nvgpu_is_error_notifier_set(ch,
			NVGPU_ERR_NOTIFIER_FIFO_ERROR_IDLE_TIMEOUT)) {
		verbose = ch->ctxsw_timeout_debug_dump;
	}

	return verbose;
}

static void nvgpu_channel_set_has_timedout_and_wakeup_wqs(struct gk20a *g,
		struct channel_gk20a *ch)
{
	/* mark channel as faulted */
	gk20a_channel_set_unserviceable(ch);

	/* unblock pending waits */
	nvgpu_cond_broadcast_interruptible(&ch->semaphore_wq);
	nvgpu_cond_broadcast_interruptible(&ch->notifier_wq);
}

bool nvgpu_channel_mark_error(struct gk20a *g, struct channel_gk20a *ch)
{
	bool verbose;

	verbose = nvgpu_channel_ctxsw_timeout_debug_dump_state(g, ch);
	nvgpu_channel_set_has_timedout_and_wakeup_wqs(g, ch);

	return verbose;
}

void nvgpu_channel_set_error_notifier(struct gk20a *g, struct channel_gk20a *ch,
				u32 error_notifier)
{
	g->ops.channel.set_error_notifier(ch, error_notifier);
}

void nvgpu_channel_set_ctx_mmu_error(struct gk20a *g,
		struct channel_gk20a *ch)
{
	nvgpu_err(g,
		"channel %d generated a mmu fault", ch->chid);
	nvgpu_channel_set_error_notifier(g, ch,
				NVGPU_ERR_NOTIFIER_FIFO_ERROR_MMU_ERR_FLT);
}

bool nvgpu_channel_update_and_check_ctxsw_timeout(struct channel_gk20a *ch,
		u32 timeout_delta_ms, bool *progress)
{
	u32 gpfifo_get = update_gp_get(ch->g, ch);

	if (gpfifo_get == ch->ctxsw_timeout_gpfifo_get) {
		/* didn't advance since previous ctxsw timeout check */
		ch->ctxsw_timeout_accumulated_ms += timeout_delta_ms;
		*progress = false;
	} else {
		/* first ctxsw timeout isr encountered */
		ch->ctxsw_timeout_accumulated_ms = timeout_delta_ms;
		*progress = true;
	}

	ch->ctxsw_timeout_gpfifo_get = gpfifo_get;

	return nvgpu_is_timeouts_enabled(ch->g) &&
		ch->ctxsw_timeout_accumulated_ms > ch->ctxsw_timeout_max_ms;
}

u32 nvgpu_get_gp_free_count(struct channel_gk20a *c)
{
	update_gp_get(c->g, c);
	return nvgpu_gp_free_count(c);
}

static void nvgpu_channel_wdt_init(struct channel_gk20a *ch)
{
	struct gk20a *g = ch->g;

	if (gk20a_channel_check_unserviceable(ch)) {
		ch->wdt.running = false;
		return;
	}

	ch->wdt.gp_get = g->ops.userd.gp_get(g, ch);
	ch->wdt.pb_get = g->ops.userd.pb_get(g, ch);
	ch->wdt.running = true;
	nvgpu_timeout_init(g, &ch->wdt.timer,
			   ch->wdt.limit_ms,
			   NVGPU_TIMER_CPU_TIMER);
}

/**
 * Start a timeout counter (watchdog) on this channel.
 *
 * Trigger a watchdog to recover the channel after the per-platform timeout
 * duration (but strictly no earlier) if the channel hasn't advanced within
 * that time.
 *
 * If the timeout is already running, do nothing. This should be called when
 * new jobs are submitted. The timeout will stop when the last tracked job
 * finishes, making the channel idle.
 *
 * The channel's gpfifo read pointer will be used to determine if the job has
 * actually stuck at that time. After the timeout duration has expired, a
 * worker thread will consider the channel stuck and recover it if stuck.
 */
static void nvgpu_channel_wdt_start(struct channel_gk20a *ch)
{
	if (!nvgpu_is_timeouts_enabled(ch->g)) {
		return;
	}

	if (!ch->wdt.enabled) {
		return;
	}

	nvgpu_spinlock_acquire(&ch->wdt.lock);

	if (ch->wdt.running) {
		nvgpu_spinlock_release(&ch->wdt.lock);
		return;
	}
	nvgpu_channel_wdt_init(ch);
	nvgpu_spinlock_release(&ch->wdt.lock);
}

/**
 * Stop a running timeout counter (watchdog) on this channel.
 *
 * Make the watchdog consider the channel not running, so that it won't get
 * recovered even if no progress is detected. Progress is not tracked if the
 * watchdog is turned off.
 *
 * No guarantees are made about concurrent execution of the timeout handler.
 * (This should be called from an update handler running in the same thread
 * with the watchdog.)
 */
static bool nvgpu_channel_wdt_stop(struct channel_gk20a *ch)
{
	bool was_running;

	nvgpu_spinlock_acquire(&ch->wdt.lock);
	was_running = ch->wdt.running;
	ch->wdt.running = false;
	nvgpu_spinlock_release(&ch->wdt.lock);
	return was_running;
}

/**
 * Continue a previously stopped timeout
 *
 * Enable the timeout again but don't reinitialize its timer.
 *
 * No guarantees are made about concurrent execution of the timeout handler.
 * (This should be called from an update handler running in the same thread
 * with the watchdog.)
 */
static void nvgpu_channel_wdt_continue(struct channel_gk20a *ch)
{
	nvgpu_spinlock_acquire(&ch->wdt.lock);
	ch->wdt.running = true;
	nvgpu_spinlock_release(&ch->wdt.lock);
}

/**
 * Reset the counter of a timeout that is in effect.
 *
 * If this channel has an active timeout, act as if something happened on the
 * channel right now.
 *
 * Rewinding a stopped counter is irrelevant; this is a no-op for non-running
 * timeouts. Stopped timeouts can only be started (which is technically a
 * rewind too) or continued (where the stop is actually pause).
 */
static void nvgpu_channel_wdt_rewind(struct channel_gk20a *ch)
{
	nvgpu_spinlock_acquire(&ch->wdt.lock);
	if (ch->wdt.running) {
		nvgpu_channel_wdt_init(ch);
	}
	nvgpu_spinlock_release(&ch->wdt.lock);
}

/**
 * Rewind the timeout on each non-dormant channel.
 *
 * Reschedule the timeout of each active channel for which timeouts are running
 * as if something was happened on each channel right now. This should be
 * called when a global hang is detected that could cause a false positive on
 * other innocent channels.
 */
void nvgpu_channel_wdt_restart_all_channels(struct gk20a *g)
{
	struct fifo_gk20a *f = &g->fifo;
	u32 chid;

	for (chid = 0; chid < f->num_channels; chid++) {
		struct channel_gk20a *ch = gk20a_channel_from_id(g, chid);

		if (ch != NULL) {
			if (!gk20a_channel_check_unserviceable(ch)) {
				nvgpu_channel_wdt_rewind(ch);
			}
			gk20a_channel_put(ch);
		}
	}
}

/**
 * Check if a timed out channel has hung and recover it if it has.
 *
 * Test if this channel has really got stuck at this point by checking if its
 * {gp,pb}_get has advanced or not. If no {gp,pb}_get action happened since
 * when the watchdog was started and it's timed out, force-reset the channel.
 *
 * The gpu is implicitly on at this point, because the watchdog can only run on
 * channels that have submitted jobs pending for cleanup.
 */
static void nvgpu_channel_wdt_handler(struct channel_gk20a *ch)
{
	struct gk20a *g = ch->g;
	u32 gp_get;
	u32 new_gp_get;
	u64 pb_get;
	u64 new_pb_get;

	nvgpu_log_fn(g, " ");

	if (gk20a_channel_check_unserviceable(ch)) {
		/* channel is already recovered */
		nvgpu_channel_wdt_stop(ch);
		return;
	}

	/* Get status but keep timer running */
	nvgpu_spinlock_acquire(&ch->wdt.lock);
	gp_get = ch->wdt.gp_get;
	pb_get = ch->wdt.pb_get;
	nvgpu_spinlock_release(&ch->wdt.lock);

	new_gp_get = g->ops.userd.gp_get(g, ch);
	new_pb_get = g->ops.userd.pb_get(g, ch);

	if (new_gp_get != gp_get || new_pb_get != pb_get) {
		/* Channel has advanced, timer keeps going but resets */
		nvgpu_channel_wdt_rewind(ch);
	} else if (nvgpu_timeout_peek_expired(&ch->wdt.timer) == 0) {
		/* Seems stuck but waiting to time out */
	} else {
		nvgpu_err(g, "Job on channel %d timed out",
			  ch->chid);

		/* force reset calls gk20a_debug_dump but not this */
		if (ch->wdt.debug_dump) {
			gk20a_gr_debug_dump(g);
		}

		g->ops.tsg.force_reset(ch,
			NVGPU_ERR_NOTIFIER_FIFO_ERROR_IDLE_TIMEOUT,
			ch->wdt.debug_dump);
	}
}

/**
 * Test if the per-channel watchdog is on; check the timeout in that case.
 *
 * Each channel has an expiration time based watchdog. The timer is
 * (re)initialized in two situations: when a new job is submitted on an idle
 * channel and when the timeout is checked but progress is detected. The
 * watchdog timeout limit is a coarse sliding window.
 *
 * The timeout is stopped (disabled) after the last job in a row finishes
 * and marks the channel idle.
 */
static void nvgpu_channel_wdt_check(struct channel_gk20a *ch)
{
	bool running;

	nvgpu_spinlock_acquire(&ch->wdt.lock);
	running = ch->wdt.running;
	nvgpu_spinlock_release(&ch->wdt.lock);

	if (running) {
		nvgpu_channel_wdt_handler(ch);
	}
}

/**
 * Loop every living channel, check timeouts and handle stuck channels.
 */
static void nvgpu_channel_poll_wdt(struct gk20a *g)
{
	unsigned int chid;


	for (chid = 0; chid < g->fifo.num_channels; chid++) {
		struct channel_gk20a *ch = gk20a_channel_from_id(g, chid);

		if (ch != NULL) {
			if (!gk20a_channel_check_unserviceable(ch)) {
				nvgpu_channel_wdt_check(ch);
			}
			gk20a_channel_put(ch);
		}
	}
}

static inline struct nvgpu_channel_worker *
nvgpu_channel_worker_from_worker(struct nvgpu_worker *worker)
{
	return (struct nvgpu_channel_worker *)
	   ((uintptr_t)worker - offsetof(struct nvgpu_channel_worker, worker));
};


static void nvgpu_channel_worker_poll_init(struct nvgpu_worker *worker)
{
	struct nvgpu_channel_worker *ch_worker =
		nvgpu_channel_worker_from_worker(worker);

	ch_worker->watchdog_interval = 100U;

	nvgpu_timeout_init(worker->g, &ch_worker->timeout,
			ch_worker->watchdog_interval, NVGPU_TIMER_CPU_TIMER);
}

static void nvgpu_channel_worker_poll_wakeup_post_process_item(
		struct nvgpu_worker *worker)
{
	struct gk20a *g = worker->g;

	struct nvgpu_channel_worker *ch_worker =
		nvgpu_channel_worker_from_worker(worker);

	if (nvgpu_timeout_peek_expired(&ch_worker->timeout) != 0) {
		nvgpu_channel_poll_wdt(g);
		nvgpu_timeout_init(g, &ch_worker->timeout,
				ch_worker->watchdog_interval,
				NVGPU_TIMER_CPU_TIMER);
	}
}
static void nvgpu_channel_worker_poll_wakeup_process_item(
		struct nvgpu_list_node *work_item)
{
	struct channel_gk20a *ch = channel_gk20a_from_worker_item(work_item);

	nvgpu_assert(ch != NULL);

	nvgpu_log_fn(ch->g, " ");

	gk20a_channel_clean_up_jobs(ch, true);

	/* ref taken when enqueued */
	gk20a_channel_put(ch);
}

static u32 nvgpu_channel_worker_poll_wakeup_condition_get_timeout(
		struct nvgpu_worker *worker)
{
	struct nvgpu_channel_worker *ch_worker =
		nvgpu_channel_worker_from_worker(worker);

	return ch_worker->watchdog_interval;
}

const struct nvgpu_worker_ops channel_worker_ops = {
	.pre_process = nvgpu_channel_worker_poll_init,
	.wakeup_early_exit = NULL,
	.wakeup_post_process =
		nvgpu_channel_worker_poll_wakeup_post_process_item,
	.wakeup_process_item =
		nvgpu_channel_worker_poll_wakeup_process_item,
	.wakeup_condition = NULL,
	.wakeup_timeout =
		nvgpu_channel_worker_poll_wakeup_condition_get_timeout,
};

/**
 * Initialize the channel worker's metadata and start the background thread.
 */
int nvgpu_channel_worker_init(struct gk20a *g)
{
	struct nvgpu_worker *worker = &g->channel_worker.worker;

	nvgpu_worker_init_name(worker, "nvgpu_channel_poll", g->name);

	return nvgpu_worker_init(g, worker, &channel_worker_ops);
}

void nvgpu_channel_worker_deinit(struct gk20a *g)
{
	struct nvgpu_worker *worker = &g->channel_worker.worker;

	nvgpu_worker_deinit(worker);
}

/**
 * Append a channel to the worker's list, if not there already.
 *
 * The worker thread processes work items (channels in its work list) and polls
 * for other things. This adds @ch to the end of the list and wakes the worker
 * up immediately. If the channel already existed in the list, it's not added,
 * because in that case it has been scheduled already but has not yet been
 * processed.
 */
static void gk20a_channel_worker_enqueue(struct channel_gk20a *ch)
{
	struct gk20a *g = ch->g;
	int ret;

	nvgpu_log_fn(g, " ");

	/*
	 * Ref released when this item gets processed. The caller should hold
	 * one ref already, so normally shouldn't fail, but the channel could
	 * end up being freed between the time the caller got its reference and
	 * the time we end up here (e.g., if the client got killed); if so, just
	 * return.
	 */
	if (gk20a_channel_get(ch) == NULL) {
		nvgpu_info(g, "cannot get ch ref for worker!");
		return;
	}

	ret = nvgpu_worker_enqueue(&g->channel_worker.worker,
			&ch->worker_item);
	if (ret != 0) {
		gk20a_channel_put(ch);
		return;
	}
}

int gk20a_free_priv_cmdbuf(struct channel_gk20a *c, struct priv_cmd_entry *e)
{
	struct priv_cmd_queue *q = &c->priv_cmd_q;
	struct gk20a *g = c->g;

	if (e == NULL) {
		return 0;
	}

	if (e->valid) {
		/* read the entry's valid flag before reading its contents */
		nvgpu_smp_rmb();
		if ((q->get != e->off) && e->off != 0U) {
			nvgpu_err(g, "requests out-of-order, ch=%d",
				  c->chid);
		}
		q->get = e->off + e->size;
	}

	free_priv_cmdbuf(c, e);

	return 0;
}

int gk20a_channel_add_job(struct channel_gk20a *c,
				 struct channel_gk20a_job *job,
				 bool skip_buffer_refcounting)
{
	struct vm_gk20a *vm = c->vm;
	struct nvgpu_mapped_buf **mapped_buffers = NULL;
	int err = 0, num_mapped_buffers = 0;
	bool pre_alloc_enabled = channel_gk20a_is_prealloc_enabled(c);

	if (!skip_buffer_refcounting) {
		err = nvgpu_vm_get_buffers(vm, &mapped_buffers,
					&num_mapped_buffers);
		if (err != 0) {
			return err;
		}
	}

	/*
	 * Ref to hold the channel open during the job lifetime. This is
	 * released by job cleanup launched via syncpt or sema interrupt.
	 */
	c = gk20a_channel_get(c);

	if (c != NULL) {
		job->num_mapped_buffers = num_mapped_buffers;
		job->mapped_buffers = mapped_buffers;

		nvgpu_channel_wdt_start(c);

		if (!pre_alloc_enabled) {
			channel_gk20a_joblist_lock(c);
		}

		/*
		 * ensure all pending write complete before adding to the list.
		 * see corresponding nvgpu_smp_rmb in
		 * gk20a_channel_clean_up_jobs()
		 */
		nvgpu_smp_wmb();
		channel_gk20a_joblist_add(c, job);

		if (!pre_alloc_enabled) {
			channel_gk20a_joblist_unlock(c);
		}
	} else {
		err = -ETIMEDOUT;
		goto err_put_buffers;
	}

	return 0;

err_put_buffers:
	nvgpu_vm_put_buffers(vm, mapped_buffers, num_mapped_buffers);

	return err;
}

/**
 * Clean up job resources for further jobs to use.
 * @clean_all: If true, process as many jobs as possible, otherwise just one.
 *
 * Loop all jobs from the joblist until a pending job is found, or just one if
 * clean_all is not set. Pending jobs are detected from the job's post fence,
 * so this is only done for jobs that have job tracking resources. Free all
 * per-job memory for completed jobs; in case of preallocated resources, this
 * opens up slots for new jobs to be submitted.
 */
void gk20a_channel_clean_up_jobs(struct channel_gk20a *c,
					bool clean_all)
{
	struct vm_gk20a *vm;
	struct channel_gk20a_job *job;
	struct gk20a *g;
	bool job_finished = false;
	bool watchdog_on = false;

	c = gk20a_channel_get(c);
	if (c == NULL) {
		return;
	}

	if (!c->g->power_on) { /* shutdown case */
		gk20a_channel_put(c);
		return;
	}

	vm = c->vm;
	g = c->g;

	/*
	 * If !clean_all, we're in a condition where watchdog isn't supported
	 * anyway (this would be a no-op).
	 */
	if (clean_all) {
		watchdog_on = nvgpu_channel_wdt_stop(c);
	}

	/* Synchronize with abort cleanup that needs the jobs. */
	nvgpu_mutex_acquire(&c->joblist.cleanup_lock);

	while (true) {
		bool completed;

		channel_gk20a_joblist_lock(c);
		if (channel_gk20a_joblist_is_empty(c)) {
			/*
			 * No jobs in flight, timeout will remain stopped until
			 * new jobs are submitted.
			 */
			channel_gk20a_joblist_unlock(c);
			break;
		}

		/*
		 * ensure that all subsequent reads occur after checking
		 * that we have a valid node. see corresponding nvgpu_smp_wmb in
		 * gk20a_channel_add_job().
		 */
		nvgpu_smp_rmb();
		job = channel_gk20a_joblist_peek(c);
		channel_gk20a_joblist_unlock(c);

		completed = nvgpu_fence_is_expired(job->post_fence);
		if (!completed) {
			/*
			 * The watchdog eventually sees an updated gp_get if
			 * something happened in this loop. A new job can have
			 * been submitted between the above call to stop and
			 * this - in that case, this is a no-op and the new
			 * later timeout is still used.
			 */
			if (clean_all && watchdog_on) {
				nvgpu_channel_wdt_continue(c);
			}
			break;
		}

		WARN_ON(c->sync == NULL);

		if (c->sync != NULL) {
			if (c->has_os_fence_framework_support &&
				g->os_channel.os_fence_framework_inst_exists(c)) {
					g->os_channel.signal_os_fence_framework(c);
			}

			if (g->aggressive_sync_destroy_thresh != 0U) {
				nvgpu_mutex_acquire(&c->sync_lock);
				if (nvgpu_channel_sync_put_ref_and_check(c->sync)
					&& g->aggressive_sync_destroy) {
					nvgpu_channel_sync_destroy(c->sync,
						false);
					c->sync = NULL;
				}
				nvgpu_mutex_release(&c->sync_lock);
			}
		}

		if (job->num_mapped_buffers != 0) {
			nvgpu_vm_put_buffers(vm, job->mapped_buffers,
				job->num_mapped_buffers);
		}

		/* Remove job from channel's job list before we close the
		 * fences, to prevent other callers (gk20a_channel_abort) from
		 * trying to dereference post_fence when it no longer exists.
		 */
		channel_gk20a_joblist_lock(c);
		channel_gk20a_joblist_delete(c, job);
		channel_gk20a_joblist_unlock(c);

		/* Close the fence (this will unref the semaphore and release
		 * it to the pool). */
		nvgpu_fence_put(job->post_fence);

		/* Free the private command buffers (wait_cmd first and
		 * then incr_cmd i.e. order of allocation) */
		gk20a_free_priv_cmdbuf(c, job->wait_cmd);
		gk20a_free_priv_cmdbuf(c, job->incr_cmd);

		/* another bookkeeping taken in add_job. caller must hold a ref
		 * so this wouldn't get freed here. */
		gk20a_channel_put(c);

		/*
		 * ensure all pending writes complete before freeing up the job.
		 * see corresponding nvgpu_smp_rmb in channel_gk20a_alloc_job().
		 */
		nvgpu_smp_wmb();

		channel_gk20a_free_job(c, job);
		job_finished = true;

		/*
		 * Deterministic channels have a channel-wide power reference;
		 * for others, there's one per submit.
		 */
		if (!c->deterministic) {
			gk20a_idle(g);
		}

		if (!clean_all) {
			/* Timeout isn't supported here so don't touch it. */
			break;
		}
	}

	nvgpu_mutex_release(&c->joblist.cleanup_lock);

	if ((job_finished) &&
			(g->os_channel.work_completion_signal != NULL)) {
		g->os_channel.work_completion_signal(c);
	}

	gk20a_channel_put(c);
}

/**
 * Schedule a job cleanup work on this channel to free resources and to signal
 * about completion.
 *
 * Call this when there has been an interrupt about finished jobs, or when job
 * cleanup needs to be performed, e.g., when closing a channel. This is always
 * safe to call even if there is nothing to clean up. Any visible actions on
 * jobs just before calling this are guaranteed to be processed.
 */
void gk20a_channel_update(struct channel_gk20a *c)
{
	if (!c->g->power_on) { /* shutdown case */
		return;
	}

	trace_gk20a_channel_update(c->chid);
	/* A queued channel is always checked for job cleanup. */
	gk20a_channel_worker_enqueue(c);
}

/*
 * Stop deterministic channel activity for do_idle() when power needs to go off
 * momentarily but deterministic channels keep power refs for potentially a
 * long time.
 *
 * Takes write access on g->deterministic_busy.
 *
 * Must be paired with gk20a_channel_deterministic_unidle().
 */
void gk20a_channel_deterministic_idle(struct gk20a *g)
{
	struct fifo_gk20a *f = &g->fifo;
	u32 chid;

	/* Grab exclusive access to the hw to block new submits */
	nvgpu_rwsem_down_write(&g->deterministic_busy);

	for (chid = 0; chid < f->num_channels; chid++) {
		struct channel_gk20a *ch = gk20a_channel_from_id(g, chid);

		if (ch == NULL) {
			continue;
		}

		if (ch->deterministic && !ch->deterministic_railgate_allowed) {
			/*
			 * Drop the power ref taken when setting deterministic
			 * flag. deterministic_unidle will put this and the
			 * channel ref back. If railgate is allowed separately
			 * for this channel, the power ref has already been put
			 * away.
			 *
			 * Hold the channel ref: it must not get freed in
			 * between. A race could otherwise result in lost
			 * gk20a_busy() via unidle, and in unbalanced
			 * gk20a_idle() via closing the channel.
			 */
			gk20a_idle(g);
		} else {
			/* Not interesting, carry on. */
			gk20a_channel_put(ch);
		}
	}
}

/*
 * Allow deterministic channel activity again for do_unidle().
 *
 * This releases write access on g->deterministic_busy.
 */
void gk20a_channel_deterministic_unidle(struct gk20a *g)
{
	struct fifo_gk20a *f = &g->fifo;
	u32 chid;

	for (chid = 0; chid < f->num_channels; chid++) {
		struct channel_gk20a *ch = gk20a_channel_from_id(g, chid);

		if (ch == NULL) {
			continue;
		}

		/*
		 * Deterministic state changes inside deterministic_busy lock,
		 * which we took in deterministic_idle.
		 */
		if (ch->deterministic && !ch->deterministic_railgate_allowed) {
			if (gk20a_busy(g) != 0) {
				nvgpu_err(g, "cannot busy() again!");
			}
			/* Took this in idle() */
			gk20a_channel_put(ch);
		}

		gk20a_channel_put(ch);
	}

	/* Release submits, new deterministic channels and frees */
	nvgpu_rwsem_up_write(&g->deterministic_busy);
}

static void nvgpu_channel_destroy(struct gk20a *g, struct channel_gk20a *c)
{
	nvgpu_mutex_destroy(&c->ioctl_lock);
	nvgpu_mutex_destroy(&c->joblist.cleanup_lock);
	nvgpu_mutex_destroy(&c->joblist.pre_alloc.read_lock);
	nvgpu_mutex_destroy(&c->sync_lock);
#if defined(CONFIG_GK20A_CYCLE_STATS)
	nvgpu_mutex_destroy(&c->cyclestate.cyclestate_buffer_mutex);
	nvgpu_mutex_destroy(&c->cs_client_mutex);
#endif
	nvgpu_mutex_destroy(&c->dbg_s_lock);
}

void nvgpu_channel_cleanup_sw(struct gk20a *g)
{
	struct fifo_gk20a *f = &g->fifo;
	u32 chid;

	/*
	 * Make sure all channels are closed before deleting them.
	 */
	for (chid = 0; chid < f->num_channels; chid++) {
		struct channel_gk20a *ch = &f->channel[chid];

		/*
		 * Could race but worst that happens is we get an error message
		 * from gk20a_free_channel() complaining about multiple closes.
		 */
		if (ch->referenceable) {
			__gk20a_channel_kill(ch);
		}

		nvgpu_channel_destroy(g, ch);
	}

	nvgpu_vfree(g, f->channel);
	f->channel = NULL;
	nvgpu_mutex_destroy(&f->free_chs_mutex);
}

int gk20a_init_channel_support(struct gk20a *g, u32 chid)
{
	struct channel_gk20a *c = g->fifo.channel+chid;
	int err;

	c->g = NULL;
	c->chid = chid;
	nvgpu_atomic_set(&c->bound, 0);
	nvgpu_spinlock_init(&c->ref_obtain_lock);
	nvgpu_atomic_set(&c->ref_count, 0);
	c->referenceable = false;
	nvgpu_cond_init(&c->ref_count_dec_wq);

	nvgpu_spinlock_init(&c->unserviceable_lock);

#if GK20A_CHANNEL_REFCOUNT_TRACKING
	nvgpu_spinlock_init(&c->ref_actions_lock);
#endif
	nvgpu_spinlock_init(&c->joblist.dynamic.lock);
	nvgpu_spinlock_init(&c->wdt.lock);

	nvgpu_init_list_node(&c->joblist.dynamic.jobs);
	nvgpu_init_list_node(&c->dbg_s_list);
	nvgpu_init_list_node(&c->worker_item);

	err = nvgpu_mutex_init(&c->ioctl_lock);
	if (err != 0) {
		return err;
	}
	err = nvgpu_mutex_init(&c->joblist.cleanup_lock);
	if (err != 0) {
		goto fail_1;
	}
	err = nvgpu_mutex_init(&c->joblist.pre_alloc.read_lock);
	if (err != 0) {
		goto fail_2;
	}
	err = nvgpu_mutex_init(&c->sync_lock);
	if (err != 0) {
		goto fail_3;
	}
#if defined(CONFIG_GK20A_CYCLE_STATS)
	err = nvgpu_mutex_init(&c->cyclestate.cyclestate_buffer_mutex);
	if (err != 0) {
		goto fail_4;
	}
	err = nvgpu_mutex_init(&c->cs_client_mutex);
	if (err != 0) {
		goto fail_5;
	}
#endif
	err = nvgpu_mutex_init(&c->dbg_s_lock);
	if (err != 0) {
		goto fail_6;
	}
	nvgpu_init_list_node(&c->ch_entry);
	nvgpu_list_add(&c->free_chs, &g->fifo.free_chs);

	return 0;

fail_6:
#if defined(CONFIG_GK20A_CYCLE_STATS)
	nvgpu_mutex_destroy(&c->cs_client_mutex);
fail_5:
	nvgpu_mutex_destroy(&c->cyclestate.cyclestate_buffer_mutex);
fail_4:
#endif
	nvgpu_mutex_destroy(&c->sync_lock);
fail_3:
	nvgpu_mutex_destroy(&c->joblist.pre_alloc.read_lock);
fail_2:
	nvgpu_mutex_destroy(&c->joblist.cleanup_lock);
fail_1:
	nvgpu_mutex_destroy(&c->ioctl_lock);

	return err;
}

int nvgpu_channel_setup_sw(struct gk20a *g)
{
	struct fifo_gk20a *f = &g->fifo;
	u32 chid, i;
	int err;

	f->num_channels = g->ops.channel.count(g);

	err = nvgpu_mutex_init(&f->free_chs_mutex);
	if (err != 0) {
		nvgpu_err(g, "mutex init failed");
		return err;
	}

	f->channel = nvgpu_vzalloc(g, f->num_channels * sizeof(*f->channel));
	if (f->channel == NULL) {
		nvgpu_err(g, "no mem for channels");
		err = -ENOMEM;
		goto clean_up_mutex;
	}

	nvgpu_init_list_node(&f->free_chs);

	for (chid = 0; chid < f->num_channels; chid++) {
		err = gk20a_init_channel_support(g, chid);
		if (err != 0) {
			nvgpu_err(g, "channel init failed, chid=%u", chid);
			goto clean_up;
		}
	}

	return 0;

clean_up:
	for (i = 0; i < chid; i++) {
		struct channel_gk20a *ch = &f->channel[i];

		nvgpu_channel_destroy(g, ch);
	}
	nvgpu_vfree(g, f->channel);
	f->channel = NULL;

clean_up_mutex:
	nvgpu_mutex_destroy(&f->free_chs_mutex);

	return err;
}

/* in this context the "channel" is the host1x channel which
 * maps to *all* gk20a channels */
int nvgpu_channel_suspend_all_serviceable_ch(struct gk20a *g)
{
	struct fifo_gk20a *f = &g->fifo;
	u32 chid;
	bool channels_in_use = false;
	u32 active_runlist_ids = 0;

	nvgpu_log_fn(g, " ");

	for (chid = 0; chid < f->num_channels; chid++) {
		struct channel_gk20a *ch = gk20a_channel_from_id(g, chid);

		if (ch == NULL) {
			continue;
		}
		if (gk20a_channel_check_unserviceable(ch)) {
			nvgpu_log_info(g, "do not suspend recovered "
						"channel %d", chid);
		} else {
			nvgpu_log_info(g, "suspend channel %d", chid);
			/* disable channel */
			gk20a_disable_channel_tsg(g, ch);
			/* preempt the channel */
			nvgpu_preempt_channel(g, ch);
			/* wait for channel update notifiers */
			if (g->os_channel.work_completion_cancel_sync != NULL) {
				g->os_channel.work_completion_cancel_sync(ch);
			}

			channels_in_use = true;

			active_runlist_ids |= (u32) BIT64(ch->runlist_id);
		}

		gk20a_channel_put(ch);
	}

	if (channels_in_use) {
		nvgpu_runlist_reload_ids(g, active_runlist_ids, false);

		for (chid = 0; chid < f->num_channels; chid++) {
			struct channel_gk20a *ch = gk20a_channel_from_id(g, chid);

			if (ch != NULL) {
				if (gk20a_channel_check_unserviceable(ch)) {
					nvgpu_log_info(g, "do not unbind "
							"recovered channel %d",
							chid);
				} else {
					g->ops.channel.unbind(ch);
				}
				gk20a_channel_put(ch);
			}
		}
	}

	nvgpu_log_fn(g, "done");
	return 0;
}

void nvgpu_channel_resume_all_serviceable_ch(struct gk20a *g)
{
	struct fifo_gk20a *f = &g->fifo;
	u32 chid;
	bool channels_in_use = false;
	u32 active_runlist_ids = 0;

	nvgpu_log_fn(g, " ");

	for (chid = 0; chid < f->num_channels; chid++) {
		struct channel_gk20a *ch = gk20a_channel_from_id(g, chid);

		if (ch == NULL) {
			continue;
		}
		if (gk20a_channel_check_unserviceable(ch)) {
			nvgpu_log_info(g, "do not resume recovered "
						"channel %d", chid);
		} else {
			nvgpu_log_info(g, "resume channel %d", chid);
			g->ops.channel.bind(ch);
			channels_in_use = true;
			active_runlist_ids |= (u32) BIT64(ch->runlist_id);
		}
		gk20a_channel_put(ch);
	}

	if (channels_in_use) {
		nvgpu_runlist_reload_ids(g, active_runlist_ids, true);
	}

	nvgpu_log_fn(g, "done");
}

void gk20a_channel_semaphore_wakeup(struct gk20a *g, bool post_events)
{
	struct fifo_gk20a *f = &g->fifo;
	u32 chid;

	nvgpu_log_fn(g, " ");

	/*
	 * Ensure that all pending writes are actually done  before trying to
	 * read semaphore values from DRAM.
	 */
	g->ops.mm.cache.fb_flush(g);

	for (chid = 0; chid < f->num_channels; chid++) {
		struct channel_gk20a *c = g->fifo.channel+chid;
		if (gk20a_channel_get(c) != NULL) {
			if (nvgpu_atomic_read(&c->bound) != 0) {
				nvgpu_cond_broadcast_interruptible(
						&c->semaphore_wq);
				if (post_events) {
					struct tsg_gk20a *tsg =
							tsg_gk20a_from_ch(c);
					if (tsg != NULL) {
						g->ops.tsg.post_event_id(tsg,
						    NVGPU_EVENT_ID_BLOCKING_SYNC);
					}
				}
				/*
				 * Only non-deterministic channels get the
				 * channel_update callback. We don't allow
				 * semaphore-backed syncs for these channels
				 * anyways, since they have a dependency on
				 * the sync framework.
				 * If deterministic channels are receiving a
				 * semaphore wakeup, it must be for a
				 * user-space managed
				 * semaphore.
				 */
				if (!c->deterministic) {
					gk20a_channel_update(c);
				}
			}
			gk20a_channel_put(c);
		}
	}
}

/* return with a reference to the channel, caller must put it back */
struct channel_gk20a *nvgpu_channel_refch_from_inst_ptr(struct gk20a *g,
			u64 inst_ptr)
{
	struct fifo_gk20a *f = &g->fifo;
	unsigned int ci;

	if (unlikely(f->channel == NULL)) {
		return NULL;
	}
	for (ci = 0; ci < f->num_channels; ci++) {
		struct channel_gk20a *ch;
		u64 ch_inst_ptr;

		ch = gk20a_channel_from_id(g, ci);
		/* only alive channels are searched */
		if (ch == NULL) {
			continue;
		}

		ch_inst_ptr = nvgpu_inst_block_addr(g, &ch->inst_block);
		if (inst_ptr == ch_inst_ptr) {
			return ch;
		}

		gk20a_channel_put(ch);
	}
	return NULL;
}

int nvgpu_channel_alloc_inst(struct gk20a *g, struct channel_gk20a *ch)
{
	int err;

	nvgpu_log_fn(g, " ");

	err = g->ops.mm.alloc_inst_block(g, &ch->inst_block);
	if (err != 0) {
		return err;
	}

	nvgpu_log_info(g, "channel %d inst block physical addr: 0x%16llx",
		ch->chid, nvgpu_inst_block_addr(g, &ch->inst_block));

	nvgpu_log_fn(g, "done");
	return 0;
}

void nvgpu_channel_free_inst(struct gk20a *g, struct channel_gk20a *ch)
{
	nvgpu_free_inst_block(g, &ch->inst_block);
}

void nvgpu_channel_debug_dump_all(struct gk20a *g,
		 struct gk20a_debug_output *o)
{
	struct fifo_gk20a *f = &g->fifo;
	u32 chid;
	struct nvgpu_channel_dump_info **infos;

	infos = nvgpu_kzalloc(g, sizeof(*infos) * f->num_channels);
	if (infos == NULL) {
		gk20a_debug_output(o, "cannot alloc memory for channels");
		return;
	}

	for (chid = 0U; chid < f->num_channels; chid++) {
		struct channel_gk20a *ch = gk20a_channel_from_id(g, chid);

		if (ch != NULL) {
			struct nvgpu_channel_dump_info *info;

			info = nvgpu_kzalloc(g, sizeof(*info));

			/*
			 * ref taken stays to below loop with
			 * successful allocs
			 */
			if (info == NULL) {
				gk20a_channel_put(ch);
			} else {
				infos[chid] = info;
			}
		}
	}

	for (chid = 0U; chid < f->num_channels; chid++) {
		struct channel_gk20a *ch = &f->channel[chid];
		struct nvgpu_channel_dump_info *info = infos[chid];
		struct nvgpu_hw_semaphore *hw_sema = ch->hw_sema;

		/* if this info exists, the above loop took a channel ref */
		if (info == NULL) {
			continue;
		}

		info->chid = ch->chid;
		info->tsgid = ch->tsgid;
		info->pid = ch->pid;
		info->refs = nvgpu_atomic_read(&ch->ref_count);
		info->deterministic = ch->deterministic;

		if (hw_sema != NULL) {
			info->sema.value = nvgpu_hw_semaphore_read(hw_sema);
			info->sema.next =
				(u32)nvgpu_hw_semaphore_read_next(hw_sema);
			info->sema.addr = nvgpu_hw_semaphore_addr(hw_sema);
		}

		g->ops.channel.read_state(g, ch, &info->hw_state);
		g->ops.ramfc.capture_ram_dump(g, ch, info);

		gk20a_channel_put(ch);
	}

	gk20a_debug_output(o, "Channel Status - chip %-5s", g->name);
	gk20a_debug_output(o, "---------------------------");
	for (chid = 0U; chid < f->num_channels; chid++) {
		struct nvgpu_channel_dump_info *info = infos[chid];

		if (info != NULL) {
			g->ops.channel.debug_dump(g, o, info);
			nvgpu_kfree(g, info);
		}
	}
	gk20a_debug_output(o, " ");

	nvgpu_kfree(g, infos);
}

int nvgpu_channel_deferred_reset_engines(struct gk20a *g,
		struct channel_gk20a *ch)
{
	unsigned long engine_id, engines = 0U;
	struct tsg_gk20a *tsg;
	bool deferred_reset_pending;
	struct fifo_gk20a *f = &g->fifo;
	int err = 0;

	nvgpu_mutex_acquire(&g->dbg_sessions_lock);

	nvgpu_mutex_acquire(&f->deferred_reset_mutex);
	deferred_reset_pending = g->fifo.deferred_reset_pending;
	nvgpu_mutex_release(&f->deferred_reset_mutex);

	if (!deferred_reset_pending) {
		nvgpu_mutex_release(&g->dbg_sessions_lock);
		return 0;
	}

	err = g->ops.gr.disable_ctxsw(g);
	if (err != 0) {
		nvgpu_err(g, "failed to disable ctxsw");
		goto fail;
	}

	tsg = tsg_gk20a_from_ch(ch);
	if (tsg != NULL) {
		engines = g->ops.engine.get_mask_on_id(g,
				tsg->tsgid, true);
	} else {
		nvgpu_err(g, "chid: %d is not bound to tsg", ch->chid);
	}

	if (engines == 0U) {
		goto clean_up;
	}

	/*
	 * If deferred reset is set for an engine, and channel is running
	 * on that engine, reset it
	 */

	for_each_set_bit(engine_id, &g->fifo.deferred_fault_engines, 32UL) {
		if ((BIT64(engine_id) & engines) != 0ULL) {
			nvgpu_engine_reset(g, (u32)engine_id);
		}
	}

	nvgpu_mutex_acquire(&f->deferred_reset_mutex);
	g->fifo.deferred_fault_engines = 0;
	g->fifo.deferred_reset_pending = false;
	nvgpu_mutex_release(&f->deferred_reset_mutex);

clean_up:
	err = g->ops.gr.enable_ctxsw(g);
	if (err != 0) {
		nvgpu_err(g, "failed to enable ctxsw");
	}
fail:
	nvgpu_mutex_release(&g->dbg_sessions_lock);

	return err;
}
