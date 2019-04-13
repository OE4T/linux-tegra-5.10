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

#include <nvgpu/io.h>
#include <nvgpu/channel.h>
#include <nvgpu/runlist.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/engine_status.h>
#include <nvgpu/engines.h>
#include <nvgpu/gr/gr_falcon.h>

#include <gk20a/fifo_gk20a.h>

#include "runlist_gk20a.h"

#include <nvgpu/hw/gk20a/hw_fifo_gk20a.h>
#include <nvgpu/hw/gk20a/hw_ram_gk20a.h>

#define FECS_MAILBOX_0_ACK_RESTORE 0x4U

#define RL_MAX_TIMESLICE_TIMEOUT ram_rl_entry_timeslice_timeout_v(U32_MAX)
#define RL_MAX_TIMESLICE_SCALE ram_rl_entry_timeslice_scale_v(U32_MAX)

int gk20a_runlist_reschedule(struct channel_gk20a *ch, bool preempt_next)
{
	return nvgpu_fifo_reschedule_runlist(ch, preempt_next, true);
}

/* trigger host preempt of GR pending load ctx if that ctx is not for ch */
int gk20a_fifo_reschedule_preempt_next(struct channel_gk20a *ch,
		bool wait_preempt)
{
	struct gk20a *g = ch->g;
	struct fifo_runlist_info_gk20a *runlist =
		g->fifo.runlist_info[ch->runlist_id];
	int ret = 0;
	u32 gr_eng_id = 0;
	u32 fecsstat0 = 0, fecsstat1 = 0;
	u32 preempt_id;
	u32 preempt_type = 0;
	struct nvgpu_engine_status_info engine_status;

	if (1U != nvgpu_engine_get_ids(
		g, &gr_eng_id, 1, NVGPU_ENGINE_GR_GK20A)) {
		return ret;
	}
	if ((runlist->eng_bitmask & BIT32(gr_eng_id)) == 0U) {
		return ret;
	}

	if (wait_preempt && ((nvgpu_readl(g, fifo_preempt_r()) &
				fifo_preempt_pending_true_f()) != 0U)) {
		return ret;
	}

	fecsstat0 = g->ops.gr.falcon.read_fecs_ctxsw_mailbox(g,
			NVGPU_GR_FALCON_FECS_CTXSW_MAILBOX0);
	g->ops.engine_status.read_engine_status_info(g, gr_eng_id, &engine_status);
	if (nvgpu_engine_status_is_ctxsw_switch(&engine_status)) {
		nvgpu_engine_status_get_next_ctx_id_type(&engine_status,
			&preempt_id, &preempt_type);
	} else {
		return ret;
	}
	if ((preempt_id == ch->tsgid) && (preempt_type != 0U)) {
		return ret;
	}
	fecsstat1 = g->ops.gr.falcon.read_fecs_ctxsw_mailbox(g,
			NVGPU_GR_FALCON_FECS_CTXSW_MAILBOX0);
	if (fecsstat0 != FECS_MAILBOX_0_ACK_RESTORE ||
		fecsstat1 != FECS_MAILBOX_0_ACK_RESTORE) {
		/* preempt useless if FECS acked save and started restore */
		return ret;
	}

	g->ops.fifo.preempt_trigger(g, preempt_id, preempt_type != 0U);
#ifdef TRACEPOINTS_ENABLED
	trace_gk20a_reschedule_preempt_next(ch->chid, fecsstat0,
		engine_status.reg_data, fecsstat1,
		g->ops.gr.falcon.read_fecs_ctxsw_mailbox(g,
			NVGPU_GR_FALCON_FECS_CTXSW_MAILBOX0),
		nvgpu_readl(g, fifo_preempt_r()));
#endif
	if (wait_preempt) {
		if (g->ops.fifo.is_preempt_pending(g, preempt_id,
			preempt_type) != 0) {
				nvgpu_err(g, "fifo preempt timed out");
				/*
				 * This function does not care if preempt
				 * times out since it is here only to improve
				 * latency. If a timeout happens, it will be
				 * handled by other fifo handling code.
				 */
		}
	}
#ifdef TRACEPOINTS_ENABLED
	trace_gk20a_reschedule_preempted_next(ch->chid);
#endif
	return ret;
}

int gk20a_runlist_set_interleave(struct gk20a *g,
				u32 id,
				u32 runlist_id,
				u32 new_level)
{
	nvgpu_log_fn(g, " ");

	g->fifo.tsg[id].interleave_level = new_level;

	return 0;
}

u32 gk20a_runlist_count_max(void)
{
	return fifo_eng_runlist_base__size_1_v();
}

u32 gk20a_runlist_entry_size(struct gk20a *g)
{
	return ram_rl_entry_size_v();
}

u32 gk20a_runlist_length_max(struct gk20a *g)
{
	return fifo_eng_runlist_length_max_v();
}

void gk20a_runlist_get_tsg_entry(struct tsg_gk20a *tsg,
		u32 *runlist, u32 timeslice)
{
	struct gk20a *g = tsg->g;
	u32 timeout = timeslice;
	u32 scale = 0U;

	WARN_ON(timeslice == 0U);

	while (timeout > RL_MAX_TIMESLICE_TIMEOUT) {
		timeout >>= 1U;
		scale++;
	}

	if (scale > RL_MAX_TIMESLICE_SCALE) {
		nvgpu_err(g, "requested timeslice value is clamped\n");
		timeout = RL_MAX_TIMESLICE_TIMEOUT;
		scale = RL_MAX_TIMESLICE_SCALE;
	}

	runlist[0] = ram_rl_entry_id_f(tsg->tsgid) |
			ram_rl_entry_type_tsg_f() |
			ram_rl_entry_tsg_length_f(tsg->num_active_channels) |
			ram_rl_entry_timeslice_scale_f(scale) |
			ram_rl_entry_timeslice_timeout_f(timeout);
	runlist[1] = 0;
}

void gk20a_runlist_get_ch_entry(struct channel_gk20a *ch, u32 *runlist)
{
	runlist[0] = ram_rl_entry_chid_f(ch->chid);
	runlist[1] = 0;
}

void gk20a_runlist_hw_submit(struct gk20a *g, u32 runlist_id,
	u32 count, u32 buffer_index)
{
	struct fifo_runlist_info_gk20a *runlist = NULL;
	u64 runlist_iova;

	runlist = g->fifo.runlist_info[runlist_id];
	runlist_iova = nvgpu_mem_get_addr(g, &runlist->mem[buffer_index]);

	nvgpu_spinlock_acquire(&g->fifo.runlist_submit_lock);

	if (count != 0U) {
		nvgpu_writel(g, fifo_runlist_base_r(),
			fifo_runlist_base_ptr_f(u64_lo32(runlist_iova >> 12)) |
			nvgpu_aperture_mask(g, &runlist->mem[buffer_index],
				fifo_runlist_base_target_sys_mem_ncoh_f(),
				fifo_runlist_base_target_sys_mem_coh_f(),
				fifo_runlist_base_target_vid_mem_f()));
	}

	nvgpu_writel(g, fifo_runlist_r(),
		fifo_runlist_engine_f(runlist_id) |
		fifo_eng_runlist_length_f(count));

	nvgpu_spinlock_release(&g->fifo.runlist_submit_lock);
}

int gk20a_runlist_wait_pending(struct gk20a *g, u32 runlist_id)
{
	struct nvgpu_timeout timeout;
	u32 delay = POLL_DELAY_MIN_US;
	int ret = 0;

	ret = nvgpu_timeout_init(g, &timeout, nvgpu_get_poll_timeout(g),
			   NVGPU_TIMER_CPU_TIMER);
	if (ret != 0) {
		nvgpu_err(g, "nvgpu_timeout_init failed err=%d", ret);
		return ret;
	}

	ret = -ETIMEDOUT;
	do {
		if ((nvgpu_readl(g, fifo_eng_runlist_r(runlist_id)) &
				fifo_eng_runlist_pending_true_f()) == 0U) {
			ret = 0;
			break;
		}

		nvgpu_usleep_range(delay, delay * 2U);
		delay = min_t(u32, delay << 1, POLL_DELAY_MAX_US);
	} while (nvgpu_timeout_expired(&timeout) == 0);

	if (ret != 0) {
		nvgpu_err(g, "runlist wait timeout: runlist id: %u",
			runlist_id);
	}

	return ret;
}

void gk20a_runlist_write_state(struct gk20a *g, u32 runlists_mask,
					 u32 runlist_state)
{
	u32 reg_val;
	u32 reg_mask = 0U;
	u32 i = 0U;

	while (runlists_mask != 0U) {
		if ((runlists_mask & BIT32(i)) != 0U) {
			reg_mask |= fifo_sched_disable_runlist_m(i);
		}
		runlists_mask &= ~BIT32(i);
		i++;
	}

	reg_val = nvgpu_readl(g, fifo_sched_disable_r());

	if (runlist_state == RUNLIST_DISABLED) {
		reg_val |= reg_mask;
	} else {
		reg_val &= ~reg_mask;
	}

	nvgpu_writel(g, fifo_sched_disable_r(), reg_val);

}
