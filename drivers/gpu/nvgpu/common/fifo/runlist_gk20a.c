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

#include <gk20a/fifo_gk20a.h>

#include "runlist_gk20a.h"

#include <nvgpu/hw/gk20a/hw_fifo_gk20a.h>
#include <nvgpu/hw/gk20a/hw_ram_gk20a.h>
#include <nvgpu/hw/gk20a/hw_gr_gk20a.h>

#define FECS_METHOD_WFI_RESTORE 0x80000U
#define FECS_MAILBOX_0_ACK_RESTORE 0x4U

int gk20a_fifo_reschedule_runlist(struct channel_gk20a *ch, bool preempt_next)
{
	return nvgpu_fifo_reschedule_runlist(ch, preempt_next, true);
}

/* trigger host preempt of GR pending load ctx if that ctx is not for ch */
int gk20a_fifo_reschedule_preempt_next(struct channel_gk20a *ch,
		bool wait_preempt)
{
	struct gk20a *g = ch->g;
	struct fifo_runlist_info_gk20a *runlist =
		&g->fifo.runlist_info[ch->runlist_id];
	int ret = 0;
	u32 gr_eng_id = 0;
	u32 engstat = 0, ctxstat = 0, fecsstat0 = 0, fecsstat1 = 0;
	u32 preempt_id;
	u32 preempt_type = 0;

	if (1U != gk20a_fifo_get_engine_ids(
		g, &gr_eng_id, 1, ENGINE_GR_GK20A)) {
		return ret;
	}
	if ((runlist->eng_bitmask & BIT32(gr_eng_id)) == 0U) {
		return ret;
	}

	if (wait_preempt && ((gk20a_readl(g, fifo_preempt_r()) &
				fifo_preempt_pending_true_f()) != 0U)) {
		return ret;
	}

	fecsstat0 = gk20a_readl(g, gr_fecs_ctxsw_mailbox_r(0));
	engstat = gk20a_readl(g, fifo_engine_status_r(gr_eng_id));
	ctxstat = fifo_engine_status_ctx_status_v(engstat);
	if (ctxstat == fifo_engine_status_ctx_status_ctxsw_switch_v()) {
		/* host switching to next context, preempt that if needed */
		preempt_id = fifo_engine_status_next_id_v(engstat);
		preempt_type = fifo_engine_status_next_id_type_v(engstat);
	} else {
		return ret;
	}
	if ((preempt_id == ch->tsgid) && (preempt_type != 0U)) {
		return ret;
	}
	fecsstat1 = gk20a_readl(g, gr_fecs_ctxsw_mailbox_r(0));
	if (fecsstat0 != FECS_MAILBOX_0_ACK_RESTORE ||
		fecsstat1 != FECS_MAILBOX_0_ACK_RESTORE) {
		/* preempt useless if FECS acked save and started restore */
		return ret;
	}

	gk20a_fifo_issue_preempt(g, preempt_id, preempt_type != 0U);
#ifdef TRACEPOINTS_ENABLED
	trace_gk20a_reschedule_preempt_next(ch->chid, fecsstat0, engstat,
		fecsstat1, gk20a_readl(g, gr_fecs_ctxsw_mailbox_r(0)),
		gk20a_readl(g, fifo_preempt_r()));
#endif
	if (wait_preempt) {
		g->ops.fifo.is_preempt_pending(g, preempt_id, preempt_type);
	}
#ifdef TRACEPOINTS_ENABLED
	trace_gk20a_reschedule_preempted_next(ch->chid);
#endif
	return ret;
}

int gk20a_fifo_set_runlist_interleave(struct gk20a *g,
				u32 id,
				u32 runlist_id,
				u32 new_level)
{
	nvgpu_log_fn(g, " ");

	g->fifo.tsg[id].interleave_level = new_level;

	return 0;
}

void gk20a_get_tsg_runlist_entry(struct tsg_gk20a *tsg, u32 *runlist)
{

	u32 runlist_entry_0 = ram_rl_entry_id_f(tsg->tsgid) |
			ram_rl_entry_type_tsg_f() |
			ram_rl_entry_tsg_length_f(tsg->num_active_channels);

	if (tsg->timeslice_timeout != 0U) {
		runlist_entry_0 |=
			ram_rl_entry_timeslice_scale_f(tsg->timeslice_scale) |
			ram_rl_entry_timeslice_timeout_f(tsg->timeslice_timeout);
	} else {
		/* safety check before casting */
#if (NVGPU_FIFO_DEFAULT_TIMESLICE_SCALE & 0xffffffff00000000UL)
#error NVGPU_FIFO_DEFAULT_TIMESLICE_SCALE too large for u32 cast
#endif
#if (NVGPU_FIFO_DEFAULT_TIMESLICE_TIMEOUT & 0xffffffff00000000UL)
#error NVGPU_FIFO_DEFAULT_TIMESLICE_TIMEOUT too large for u32 cast
#endif
		runlist_entry_0 |=
			ram_rl_entry_timeslice_scale_f(
				(u32)NVGPU_FIFO_DEFAULT_TIMESLICE_SCALE) |
			ram_rl_entry_timeslice_timeout_f(
				(u32)NVGPU_FIFO_DEFAULT_TIMESLICE_TIMEOUT);
	}

	runlist[0] = runlist_entry_0;
	runlist[1] = 0;

}

void gk20a_get_ch_runlist_entry(struct channel_gk20a *ch, u32 *runlist)
{
	runlist[0] = ram_rl_entry_chid_f(ch->chid);
	runlist[1] = 0;
}

void gk20a_fifo_runlist_hw_submit(struct gk20a *g, u32 runlist_id,
	u32 count, u32 buffer_index)
{
	struct fifo_runlist_info_gk20a *runlist = NULL;
	u64 runlist_iova;

	runlist = &g->fifo.runlist_info[runlist_id];
	runlist_iova = nvgpu_mem_get_addr(g, &runlist->mem[buffer_index]);

	nvgpu_spinlock_acquire(&g->fifo.runlist_submit_lock);

	if (count != 0U) {
		gk20a_writel(g, fifo_runlist_base_r(),
			fifo_runlist_base_ptr_f(u64_lo32(runlist_iova >> 12)) |
			nvgpu_aperture_mask(g, &runlist->mem[buffer_index],
				fifo_runlist_base_target_sys_mem_ncoh_f(),
				fifo_runlist_base_target_sys_mem_coh_f(),
				fifo_runlist_base_target_vid_mem_f()));
	}

	gk20a_writel(g, fifo_runlist_r(),
		fifo_runlist_engine_f(runlist_id) |
		fifo_eng_runlist_length_f(count));

	nvgpu_spinlock_release(&g->fifo.runlist_submit_lock);
}

int gk20a_fifo_runlist_wait_pending(struct gk20a *g, u32 runlist_id)
{
	struct nvgpu_timeout timeout;
	u32 delay = GR_IDLE_CHECK_DEFAULT;
	int ret = -ETIMEDOUT;

	nvgpu_timeout_init(g, &timeout, gk20a_get_gr_idle_timeout(g),
			   NVGPU_TIMER_CPU_TIMER);

	do {
		if ((gk20a_readl(g, fifo_eng_runlist_r(runlist_id)) &
				fifo_eng_runlist_pending_true_f()) == 0U) {
			ret = 0;
			break;
		}

		nvgpu_usleep_range(delay, delay * 2U);
		delay = min_t(u32, delay << 1, GR_IDLE_CHECK_MAX);
	} while (nvgpu_timeout_expired(&timeout) == 0);

	if (ret != 0) {
		nvgpu_err(g, "runlist wait timeout: runlist id: %u",
			runlist_id);
	}

	return ret;
}

void gk20a_fifo_runlist_write_state(struct gk20a *g, u32 runlists_mask,
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

	reg_val = gk20a_readl(g, fifo_sched_disable_r());

	if (runlist_state == RUNLIST_DISABLED) {
		reg_val |= reg_mask;
	} else {
		reg_val &= ~reg_mask;
	}

	gk20a_writel(g, fifo_sched_disable_r(), reg_val);

}

