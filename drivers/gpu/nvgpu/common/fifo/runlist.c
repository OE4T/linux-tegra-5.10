/*
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

#include <nvgpu/gk20a.h>
#include <nvgpu/channel.h>
#include <nvgpu/fifo.h>
#include <nvgpu/runlist.h>
#include <nvgpu/bug.h>

static u32 nvgpu_runlist_append_tsg(struct gk20a *g,
		struct fifo_runlist_info_gk20a *runlist,
		u32 **runlist_entry,
		u32 *entries_left,
		struct tsg_gk20a *tsg)
{
	struct fifo_gk20a *f = &g->fifo;
	u32 runlist_entry_words = f->runlist_entry_size / (u32)sizeof(u32);
	struct channel_gk20a *ch;
	u32 count = 0;

	nvgpu_log_fn(f->g, " ");

	if (*entries_left == 0U) {
		return RUNLIST_APPEND_FAILURE;
	}

	/* add TSG entry */
	nvgpu_log_info(g, "add TSG %d to runlist", tsg->tsgid);
	g->ops.fifo.get_tsg_runlist_entry(tsg, *runlist_entry);
	nvgpu_log_info(g, "tsg rl entries left %d runlist [0] %x [1] %x",
			*entries_left,
			(*runlist_entry)[0], (*runlist_entry)[1]);
	*runlist_entry += runlist_entry_words;
	count++;
	(*entries_left)--;

	nvgpu_rwsem_down_read(&tsg->ch_list_lock);
	/* add runnable channels bound to this TSG */
	nvgpu_list_for_each_entry(ch, &tsg->ch_list,
			channel_gk20a, ch_entry) {
		if (!test_bit((int)ch->chid,
			      runlist->active_channels)) {
			continue;
		}

		if (*entries_left == 0U) {
			nvgpu_rwsem_up_read(&tsg->ch_list_lock);
			return RUNLIST_APPEND_FAILURE;
		}

		nvgpu_log_info(g, "add channel %d to runlist",
			ch->chid);
		g->ops.fifo.get_ch_runlist_entry(ch, *runlist_entry);
		nvgpu_log_info(g, "rl entries left %d runlist [0] %x [1] %x",
			*entries_left,
			(*runlist_entry)[0], (*runlist_entry)[1]);
		count++;
		*runlist_entry += runlist_entry_words;
		(*entries_left)--;
	}
	nvgpu_rwsem_up_read(&tsg->ch_list_lock);

	return count;
}


static u32 nvgpu_runlist_append_prio(struct fifo_gk20a *f,
				struct fifo_runlist_info_gk20a *runlist,
				u32 **runlist_entry,
				u32 *entries_left,
				u32 interleave_level)
{
	u32 count = 0;
	unsigned long tsgid;

	nvgpu_log_fn(f->g, " ");

	for_each_set_bit(tsgid, runlist->active_tsgs, f->num_channels) {
		struct tsg_gk20a *tsg = &f->tsg[tsgid];
		u32 entries;

		if (tsg->interleave_level == interleave_level) {
			entries = nvgpu_runlist_append_tsg(f->g, runlist,
					runlist_entry, entries_left, tsg);
			if (entries == RUNLIST_APPEND_FAILURE) {
				return RUNLIST_APPEND_FAILURE;
			}
			count += entries;
		}
	}

	return count;
}

static u32 nvgpu_runlist_append_hi(struct fifo_gk20a *f,
				struct fifo_runlist_info_gk20a *runlist,
				u32 **runlist_entry,
				u32 *entries_left)
{
	nvgpu_log_fn(f->g, " ");

	/*
	 * No higher levels - this is where the "recursion" ends; just add all
	 * active TSGs at this level.
	 */
	return nvgpu_runlist_append_prio(f, runlist, runlist_entry,
			entries_left,
			NVGPU_FIFO_RUNLIST_INTERLEAVE_LEVEL_HIGH);
}

static u32 nvgpu_runlist_append_med(struct fifo_gk20a *f,
				struct fifo_runlist_info_gk20a *runlist,
				u32 **runlist_entry,
				u32 *entries_left)
{
	u32 count = 0;
	unsigned long tsgid;

	nvgpu_log_fn(f->g, " ");

	for_each_set_bit(tsgid, runlist->active_tsgs, f->num_channels) {
		struct tsg_gk20a *tsg = &f->tsg[tsgid];
		u32 entries;

		if (tsg->interleave_level !=
				NVGPU_FIFO_RUNLIST_INTERLEAVE_LEVEL_MEDIUM) {
			continue;
		}

		/* LEVEL_MEDIUM list starts with a LEVEL_HIGH, if any */

		entries = nvgpu_runlist_append_hi(f, runlist,
				runlist_entry, entries_left);
		if (entries == RUNLIST_APPEND_FAILURE) {
			return RUNLIST_APPEND_FAILURE;
		}
		count += entries;

		entries = nvgpu_runlist_append_tsg(f->g, runlist,
				runlist_entry, entries_left, tsg);
		if (entries == RUNLIST_APPEND_FAILURE) {
			return RUNLIST_APPEND_FAILURE;
		}
		count += entries;
	}

	return count;
}

static u32 nvgpu_runlist_append_low(struct fifo_gk20a *f,
				struct fifo_runlist_info_gk20a *runlist,
				u32 **runlist_entry,
				u32 *entries_left)
{
	u32 count = 0;
	unsigned long tsgid;

	nvgpu_log_fn(f->g, " ");

	for_each_set_bit(tsgid, runlist->active_tsgs, f->num_channels) {
		struct tsg_gk20a *tsg = &f->tsg[tsgid];
		u32 entries;

		if (tsg->interleave_level !=
				NVGPU_FIFO_RUNLIST_INTERLEAVE_LEVEL_LOW) {
			continue;
		}

		/* The medium level starts with the highs, if any. */

		entries = nvgpu_runlist_append_med(f, runlist,
				runlist_entry, entries_left);
		if (entries == RUNLIST_APPEND_FAILURE) {
			return RUNLIST_APPEND_FAILURE;
		}
		count += entries;

		entries = nvgpu_runlist_append_hi(f, runlist,
				runlist_entry, entries_left);
		if (entries == RUNLIST_APPEND_FAILURE) {
			return RUNLIST_APPEND_FAILURE;
		}
		count += entries;

		entries = nvgpu_runlist_append_tsg(f->g, runlist,
				runlist_entry, entries_left, tsg);
		if (entries == RUNLIST_APPEND_FAILURE) {
			return RUNLIST_APPEND_FAILURE;
		}
		count += entries;
	}

	if (count == 0U) {
		/*
		 * No transitions to fill with higher levels, so add
		 * the next level once. If that's empty too, we have only
		 * LEVEL_HIGH jobs.
		 */
		count = nvgpu_runlist_append_med(f, runlist,
				runlist_entry, entries_left);
		if (count == 0U) {
			count = nvgpu_runlist_append_hi(f, runlist,
					runlist_entry, entries_left);
		}
	}

	return count;
}

static u32 nvgpu_runlist_append_flat(struct fifo_gk20a *f,
				struct fifo_runlist_info_gk20a *runlist,
				u32 **runlist_entry,
				u32 *entries_left)
{
	u32 count = 0, entries, i;

	nvgpu_log_fn(f->g, " ");

	/* Group by priority but don't interleave. High comes first. */

	for (i = 0; i < NVGPU_FIFO_RUNLIST_INTERLEAVE_NUM_LEVELS; i++) {
		u32 level = NVGPU_FIFO_RUNLIST_INTERLEAVE_LEVEL_HIGH - i;

		entries = nvgpu_runlist_append_prio(f, runlist, runlist_entry,
				entries_left, level);
		if (entries == RUNLIST_APPEND_FAILURE) {
			return RUNLIST_APPEND_FAILURE;
		}
		count += entries;
	}

	return count;
}

u32 nvgpu_runlist_construct_locked(struct fifo_gk20a *f,
				struct fifo_runlist_info_gk20a *runlist,
				u32 buf_id,
				u32 max_entries)
{
	u32 *runlist_entry_base = runlist->mem[buf_id].cpu_va;

	nvgpu_log_fn(f->g, " ");

	/*
	 * The entry pointer and capacity counter that live on the stack here
	 * keep track of the current position and the remaining space when tsg
	 * and channel entries are ultimately appended.
	 */
	if (f->g->runlist_interleave) {
		return nvgpu_runlist_append_low(f, runlist,
				&runlist_entry_base, &max_entries);
	} else {
		return nvgpu_runlist_append_flat(f, runlist,
				&runlist_entry_base, &max_entries);
	}
}

int gk20a_fifo_update_runlist_locked(struct gk20a *g, u32 runlist_id,
					    u32 chid, bool add,
					    bool wait_for_finish)
{
	int ret = 0;
	struct fifo_gk20a *f = &g->fifo;
	struct fifo_runlist_info_gk20a *runlist = NULL;
	u64 runlist_iova;
	u32 new_buf;
	struct channel_gk20a *ch = NULL;
	struct tsg_gk20a *tsg = NULL;

	runlist = &f->runlist_info[runlist_id];

	/* valid channel, add/remove it from active list.
	   Otherwise, keep active list untouched for suspend/resume. */
	if (chid != FIFO_INVAL_CHANNEL_ID) {
		ch = &f->channel[chid];
		if (gk20a_is_channel_marked_as_tsg(ch)) {
			tsg = &f->tsg[ch->tsgid];
		}

		if (add) {
			if (test_and_set_bit(chid,
				runlist->active_channels)) {
				return 0;
			}
			if ((tsg != NULL) && (++tsg->num_active_channels != 0U)) {
				set_bit((int)f->channel[chid].tsgid,
					runlist->active_tsgs);
			}
		} else {
			if (!test_and_clear_bit(chid,
				runlist->active_channels)) {
				return 0;
			}
			if ((tsg != NULL) &&
			    (--tsg->num_active_channels == 0U)) {
				clear_bit((int)f->channel[chid].tsgid,
					runlist->active_tsgs);
			}
		}
	}

	/* There just 2 buffers */
	new_buf = runlist->cur_buffer == 0U ? 1U : 0U;

	runlist_iova = nvgpu_mem_get_addr(g, &runlist->mem[new_buf]);

	nvgpu_log_info(g, "runlist_id : %d, switch to new buffer 0x%16llx",
		runlist_id, (u64)runlist_iova);

	if (runlist_iova == 0ULL) {
		ret = -EINVAL;
		goto clean_up;
	}

	if (chid != FIFO_INVAL_CHANNEL_ID || /* add/remove a valid channel */
	    add /* resume to add all channels back */) {
		u32 num_entries;

		num_entries = nvgpu_runlist_construct_locked(f,
						runlist,
						new_buf,
						f->num_runlist_entries);
		if (num_entries == RUNLIST_APPEND_FAILURE) {
			ret = -E2BIG;
			goto clean_up;
		}
		runlist->count = num_entries;
		WARN_ON(runlist->count > f->num_runlist_entries);
	} else {
		/* suspend to remove all channels */
		runlist->count = 0;
	}

	g->ops.fifo.runlist_hw_submit(g, runlist_id, runlist->count, new_buf);

	if (wait_for_finish) {
		ret = g->ops.fifo.runlist_wait_pending(g, runlist_id);

		if (ret == -ETIMEDOUT) {
			nvgpu_err(g, "runlist %d update timeout", runlist_id);
			/* trigger runlist update timeout recovery */
			return ret;

		} else if (ret == -EINTR) {
			nvgpu_err(g, "runlist update interrupted");
		}
	}

	runlist->cur_buffer = new_buf;

clean_up:
	return ret;
}

/* trigger host to expire current timeslice and reschedule runlist from front */
int nvgpu_fifo_reschedule_runlist(struct channel_gk20a *ch, bool preempt_next,
		bool wait_preempt)
{
	struct gk20a *g = ch->g;
	struct fifo_runlist_info_gk20a *runlist;
	u32 token = PMU_INVALID_MUTEX_OWNER_ID;
	int mutex_ret = -EINVAL;
	int ret = 0;

	runlist = &g->fifo.runlist_info[ch->runlist_id];
	if (nvgpu_mutex_tryacquire(&runlist->runlist_lock) == 0) {
		return -EBUSY;
	}

	if (g->ops.pmu.is_pmu_supported(g)) {
		mutex_ret = nvgpu_pmu_mutex_acquire(
			&g->pmu, PMU_MUTEX_ID_FIFO, &token);
	}

	g->ops.fifo.runlist_hw_submit(
		g, ch->runlist_id, runlist->count, runlist->cur_buffer);

	if (preempt_next) {
		g->ops.fifo.reschedule_preempt_next_locked(ch, wait_preempt);
	}

	g->ops.fifo.runlist_wait_pending(g, ch->runlist_id);

	if (mutex_ret == 0) {
		nvgpu_pmu_mutex_release(
			&g->pmu, PMU_MUTEX_ID_FIFO, &token);
	}
	nvgpu_mutex_release(&runlist->runlist_lock);

	return ret;
}

static void gk20a_fifo_runlist_reset_engines(struct gk20a *g, u32 runlist_id)
{
	u32 engines = g->ops.fifo.runlist_busy_engines(g, runlist_id);

	if (engines != 0U) {
		gk20a_fifo_recover(g, engines, ~(u32)0, false, false, true,
				RC_TYPE_RUNLIST_UPDATE_TIMEOUT);
	}
}

/* add/remove a channel from runlist
   special cases below: runlist->active_channels will NOT be changed.
   (chid == ~0 && !add) means remove all active channels from runlist.
   (chid == ~0 &&  add) means restore all active channels on runlist. */
int gk20a_fifo_update_runlist(struct gk20a *g, u32 runlist_id, u32 chid,
			      bool add, bool wait_for_finish)
{
	struct fifo_runlist_info_gk20a *runlist = NULL;
	struct fifo_gk20a *f = &g->fifo;
	u32 token = PMU_INVALID_MUTEX_OWNER_ID;
	int mutex_ret = -EINVAL;
	int ret = 0;

	nvgpu_log_fn(g, " ");

	runlist = &f->runlist_info[runlist_id];

	nvgpu_mutex_acquire(&runlist->runlist_lock);

	if (g->ops.pmu.is_pmu_supported(g)) {
		mutex_ret = nvgpu_pmu_mutex_acquire(&g->pmu,
						PMU_MUTEX_ID_FIFO, &token);
	}

	ret = gk20a_fifo_update_runlist_locked(g, runlist_id, chid, add,
					       wait_for_finish);

	if (mutex_ret == 0) {
		nvgpu_pmu_mutex_release(&g->pmu, PMU_MUTEX_ID_FIFO, &token);
	}

	nvgpu_mutex_release(&runlist->runlist_lock);

	if (ret == -ETIMEDOUT) {
		gk20a_fifo_runlist_reset_engines(g, runlist_id);
	}

	return ret;
}

int gk20a_fifo_update_runlist_ids(struct gk20a *g, u32 runlist_ids, u32 chid,
				bool add, bool wait_for_finish)
{
	int ret = -EINVAL;
	unsigned long runlist_id = 0;
	int errcode;
	unsigned long ulong_runlist_ids = (unsigned long)runlist_ids;

	if (g == NULL) {
		goto end;
	}

	ret = 0;
	for_each_set_bit(runlist_id, &ulong_runlist_ids, 32U) {
		/* Capture the last failure error code */
		errcode = g->ops.fifo.update_runlist(g, (u32)runlist_id, chid,
						add, wait_for_finish);
		if (errcode != 0) {
			nvgpu_err(g,
				"failed to update_runlist %lu %d",
				runlist_id, errcode);
			ret = errcode;
		}
	}
end:
	return ret;
}

const char *gk20a_fifo_interleave_level_name(u32 interleave_level)
{
	const char *ret_string = NULL;

	switch (interleave_level) {
	case NVGPU_FIFO_RUNLIST_INTERLEAVE_LEVEL_LOW:
		ret_string = "LOW";
		break;

	case NVGPU_FIFO_RUNLIST_INTERLEAVE_LEVEL_MEDIUM:
		ret_string = "MEDIUM";
		break;

	case NVGPU_FIFO_RUNLIST_INTERLEAVE_LEVEL_HIGH:
		ret_string = "HIGH";
		break;

	default:
		ret_string = "?";
		break;
	}

	return ret_string;
}

void gk20a_fifo_set_runlist_state(struct gk20a *g, u32 runlists_mask,
		u32 runlist_state)
{
	u32 token = PMU_INVALID_MUTEX_OWNER_ID;
	int mutex_ret = -EINVAL;

	nvgpu_log(g, gpu_dbg_info, "runlist mask = 0x%08x state = 0x%08x",
			runlists_mask, runlist_state);

	if (g->ops.pmu.is_pmu_supported(g)) {
		mutex_ret = nvgpu_pmu_mutex_acquire(&g->pmu,
						PMU_MUTEX_ID_FIFO, &token);
	}

	g->ops.fifo.runlist_write_state(g, runlists_mask, runlist_state);

	if (mutex_ret == 0) {
		nvgpu_pmu_mutex_release(&g->pmu, PMU_MUTEX_ID_FIFO, &token);
	}
}

void gk20a_fifo_delete_runlist(struct fifo_gk20a *f)
{
	u32 i;
	u32 runlist_id;
	struct fifo_runlist_info_gk20a *runlist;
	struct gk20a *g = NULL;

	if ((f == NULL) || (f->runlist_info == NULL)) {
		return;
	}

	g = f->g;

	for (runlist_id = 0; runlist_id < f->max_runlists; runlist_id++) {
		runlist = &f->runlist_info[runlist_id];
		for (i = 0; i < MAX_RUNLIST_BUFFERS; i++) {
			nvgpu_dma_free(g, &runlist->mem[i]);
		}

		nvgpu_kfree(g, runlist->active_channels);
		runlist->active_channels = NULL;

		nvgpu_kfree(g, runlist->active_tsgs);
		runlist->active_tsgs = NULL;

		nvgpu_mutex_destroy(&runlist->runlist_lock);

	}
	(void) memset(f->runlist_info, 0,
		(sizeof(struct fifo_runlist_info_gk20a) * f->max_runlists));

	nvgpu_kfree(g, f->runlist_info);
	f->runlist_info = NULL;
	f->max_runlists = 0;
}

int nvgpu_init_runlist(struct gk20a *g, struct fifo_gk20a *f)
{
	struct fifo_runlist_info_gk20a *runlist;
	unsigned int runlist_id;
	u32 i;
	size_t runlist_size;
	int err = 0;

	nvgpu_log_fn(g, " ");

	f->max_runlists = g->ops.fifo.eng_runlist_base_size();
	f->runlist_info = nvgpu_kzalloc(g,
					sizeof(struct fifo_runlist_info_gk20a) *
					f->max_runlists);
	if (f->runlist_info == NULL) {
		goto clean_up_runlist;
	}

	for (runlist_id = 0; runlist_id < f->max_runlists; runlist_id++) {
		runlist = &f->runlist_info[runlist_id];

		runlist->active_channels =
			nvgpu_kzalloc(g, DIV_ROUND_UP(f->num_channels,
						      BITS_PER_BYTE));
		if (runlist->active_channels == NULL) {
			goto clean_up_runlist;
		}

		runlist->active_tsgs =
			nvgpu_kzalloc(g, DIV_ROUND_UP(f->num_channels,
						      BITS_PER_BYTE));
		if (runlist->active_tsgs == NULL) {
			goto clean_up_runlist;
		}

		runlist_size = (size_t)f->runlist_entry_size *
				(size_t)f->num_runlist_entries;
		nvgpu_log(g, gpu_dbg_info,
				"runlist_entries %d runlist size %zu",
				f->num_runlist_entries, runlist_size);

		for (i = 0; i < MAX_RUNLIST_BUFFERS; i++) {
			err = nvgpu_dma_alloc_flags_sys(g,
					g->is_virtual ?
					  0 : NVGPU_DMA_PHYSICALLY_ADDRESSED,
					runlist_size,
					&runlist->mem[i]);
			if (err != 0) {
				nvgpu_err(g, "memory allocation failed");
				goto clean_up_runlist;
			}
		}

		err = nvgpu_mutex_init(&runlist->runlist_lock);
		if (err != 0) {
			nvgpu_err(g,
				"Error in runlist_lock mutex initialization");
			goto clean_up_runlist;
		}

		/* None of buffers is pinned if this value doesn't change.
		    Otherwise, one of them (cur_buffer) must have been pinned. */
		runlist->cur_buffer = MAX_RUNLIST_BUFFERS;
	}

	nvgpu_log_fn(g, "done");
	return 0;

clean_up_runlist:
	gk20a_fifo_delete_runlist(f);
	nvgpu_log_fn(g, "fail");
	return err;
}
