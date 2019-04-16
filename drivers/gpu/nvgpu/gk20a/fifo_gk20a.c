/*
 * GK20A Graphics FIFO (gr host)
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

#include <nvgpu/mm.h>
#include <nvgpu/dma.h>
#include <nvgpu/timers.h>
#include <nvgpu/enabled.h>
#include <nvgpu/semaphore.h>
#include <nvgpu/kmem.h>
#include <nvgpu/log.h>
#include <nvgpu/soc.h>
#include <nvgpu/atomic.h>
#include <nvgpu/bug.h>
#include <nvgpu/log2.h>
#include <nvgpu/debug.h>
#include <nvgpu/nvhost.h>
#include <nvgpu/barrier.h>
#include <nvgpu/error_notifier.h>
#include <nvgpu/ptimer.h>
#include <nvgpu/io.h>
#include <nvgpu/utils.h>
#include <nvgpu/fifo.h>
#include <nvgpu/rc.h>
#include <nvgpu/runlist.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/channel.h>
#include <nvgpu/unit.h>
#include <nvgpu/types.h>
#include <nvgpu/vm_area.h>
#include <nvgpu/top.h>
#include <nvgpu/nvgpu_err.h>
#include <nvgpu/pbdma_status.h>
#include <nvgpu/engine_status.h>
#include <nvgpu/engines.h>
#include <nvgpu/power_features/cg.h>
#include <nvgpu/power_features/pg.h>
#include <nvgpu/power_features/power_features.h>
#include <nvgpu/gr/fecs_trace.h>

#include "mm_gk20a.h"

#include <hal/fifo/mmu_fault_gk20a.h>

#include <nvgpu/hw/gk20a/hw_fifo_gk20a.h>
#include <nvgpu/hw/gk20a/hw_pbdma_gk20a.h>
#include <nvgpu/hw/gk20a/hw_gr_gk20a.h>

#define FECS_METHOD_WFI_RESTORE 0x80000U

int gk20a_init_fifo_reset_enable_hw(struct gk20a *g)
{
	u32 timeout;

	nvgpu_log_fn(g, " ");

	/* enable pmc pfifo */
	g->ops.mc.reset(g, g->ops.mc.reset_mask(g, NVGPU_UNIT_FIFO));

	nvgpu_cg_slcg_fifo_load_enable(g);

	nvgpu_cg_blcg_fifo_load_enable(g);

	timeout = gk20a_readl(g, fifo_fb_timeout_r());
	timeout = set_field(timeout, fifo_fb_timeout_period_m(),
			fifo_fb_timeout_period_max_f());
	nvgpu_log_info(g, "fifo_fb_timeout reg val = 0x%08x", timeout);
	gk20a_writel(g, fifo_fb_timeout_r(), timeout);

	g->ops.pbdma.setup_hw(g);

	g->ops.fifo.intr_0_enable(g, true);
	g->ops.fifo.intr_1_enable(g, true);

	nvgpu_log_fn(g, "done");

	return 0;
}

int gk20a_init_fifo_setup_hw(struct gk20a *g)
{
	struct fifo_gk20a *f = &g->fifo;
	u64 shifted_addr;

	nvgpu_log_fn(g, " ");

	/* set the base for the userd region now */
	shifted_addr = f->userd_gpu_va >> 12;
	if ((shifted_addr >> 32) != 0U) {
		nvgpu_err(g, "GPU VA > 32 bits %016llx\n", f->userd_gpu_va);
		return -EFAULT;
	}
	gk20a_writel(g, fifo_bar1_base_r(),
			fifo_bar1_base_ptr_f(u64_lo32(shifted_addr)) |
			fifo_bar1_base_valid_true_f());

	nvgpu_log_fn(g, "done");

	return 0;
}

void gk20a_fifo_issue_preempt(struct gk20a *g, u32 id, bool is_tsg)
{
	if (is_tsg) {
		gk20a_writel(g, fifo_preempt_r(),
			fifo_preempt_id_f(id) |
			fifo_preempt_type_tsg_f());
	} else {
		gk20a_writel(g, fifo_preempt_r(),
			fifo_preempt_chid_f(id) |
			fifo_preempt_type_channel_f());
	}
}

static u32 gk20a_fifo_get_preempt_timeout(struct gk20a *g)
{
	/* Use fifo_eng_timeout converted to ms for preempt
	 * polling. gr_idle_timeout i.e 3000 ms is and not appropriate
	 * for polling preempt done as context switch timeout gets
	 * triggered every ctxsw_timeout_period_ms.
	 */

	return g->ctxsw_timeout_period_ms;
}

int gk20a_fifo_is_preempt_pending(struct gk20a *g, u32 id,
		unsigned int id_type)
{
	struct nvgpu_timeout timeout;
	u32 delay = POLL_DELAY_MIN_US;
	int ret = 0;

	ret = nvgpu_timeout_init(g, &timeout, gk20a_fifo_get_preempt_timeout(g),
			   NVGPU_TIMER_CPU_TIMER);
	if (ret != 0) {
		nvgpu_err(g, "nvgpu_timeout_init failed err=%d ", ret);
		return ret;
	}

	ret = -EBUSY;
	do {
		if ((gk20a_readl(g, fifo_preempt_r()) &
				fifo_preempt_pending_true_f()) == 0U) {
			ret = 0;
			break;
		}

		nvgpu_usleep_range(delay, delay * 2U);
		delay = min_t(u32, delay << 1, POLL_DELAY_MAX_US);
	} while (nvgpu_timeout_expired(&timeout) == 0);

	if (ret != 0) {
		nvgpu_err(g, "preempt timeout: id: %u id_type: %d ",
			id, id_type);
	}
	return ret;
}

int __locked_fifo_preempt(struct gk20a *g, u32 id, bool is_tsg)
{
	int ret;
	unsigned int id_type;

	nvgpu_log_fn(g, "id: %d is_tsg: %d", id, is_tsg);

	/* issue preempt */
	gk20a_fifo_issue_preempt(g, id, is_tsg);

	id_type = is_tsg ? ID_TYPE_TSG : ID_TYPE_CHANNEL;

	/* wait for preempt */
	ret = g->ops.fifo.is_preempt_pending(g, id, id_type);

	return ret;
}

int gk20a_fifo_preempt_channel(struct gk20a *g, struct channel_gk20a *ch)
{
	int ret = 0;
	u32 token = PMU_INVALID_MUTEX_OWNER_ID;
	int mutex_ret = 0;
	int err = 0;

	nvgpu_log_fn(g, "chid: %d", ch->chid);

	/* we have no idea which runlist we are using. lock all */
	nvgpu_fifo_lock_active_runlists(g);

	mutex_ret = nvgpu_pmu_lock_acquire(g, &g->pmu,
		PMU_MUTEX_ID_FIFO, &token);

	ret = __locked_fifo_preempt(g, ch->chid, false);

	if (mutex_ret == 0) {
		err = nvgpu_pmu_lock_release(g, &g->pmu, PMU_MUTEX_ID_FIFO,
			&token);
		if (err != 0) {
			nvgpu_err(g, "nvgpu_pmu_lock_release failed err=%d",
				err);
		}
	}

	nvgpu_fifo_unlock_active_runlists(g);

	if (ret != 0) {
		if (nvgpu_platform_is_silicon(g)) {
			nvgpu_err(g, "preempt timed out for chid: %u, "
			"ctxsw timeout will trigger recovery if needed",
			ch->chid);
		} else {
			struct tsg_gk20a *tsg;

			nvgpu_err(g, "preempt channel %d timeout", ch->chid);
			tsg = tsg_gk20a_from_ch(ch);
			if (tsg != NULL) {
				nvgpu_rc_preempt_timeout(g, tsg);
			} else {
				nvgpu_err(g, "chid: %d is not bound to tsg",
					ch->chid);
			}

		}
	}

	return ret;
}

int gk20a_fifo_preempt_tsg(struct gk20a *g, struct tsg_gk20a *tsg)
{
	int ret = 0;
	u32 token = PMU_INVALID_MUTEX_OWNER_ID;
	int mutex_ret = 0;
	int err = 0;

	nvgpu_log_fn(g, "tsgid: %d", tsg->tsgid);

	/* we have no idea which runlist we are using. lock all */
	nvgpu_fifo_lock_active_runlists(g);

	mutex_ret = nvgpu_pmu_lock_acquire(g, &g->pmu,
			PMU_MUTEX_ID_FIFO, &token);

	ret = __locked_fifo_preempt(g, tsg->tsgid, true);

	if (mutex_ret == 0) {
		err = nvgpu_pmu_lock_release(g, &g->pmu, PMU_MUTEX_ID_FIFO,
			&token);
		if (err != 0) {
			nvgpu_err(g, "nvgpu_pmu_lock_release failed err=%d",
				err);
		}
	}

	nvgpu_fifo_unlock_active_runlists(g);

	if (ret != 0) {
		if (nvgpu_platform_is_silicon(g)) {
			nvgpu_err(g, "preempt timed out for tsgid: %u, "
			"ctxsw timeout will trigger recovery if needed",
			tsg->tsgid);
		} else {
			nvgpu_err(g, "preempt TSG %d timeout", tsg->tsgid);
			nvgpu_rc_preempt_timeout(g, tsg);
		}
	}

	return ret;
}

int gk20a_fifo_preempt(struct gk20a *g, struct channel_gk20a *ch)
{
	int err;
	struct tsg_gk20a *tsg = tsg_gk20a_from_ch(ch);

	if (tsg != NULL) {
		err = g->ops.fifo.preempt_tsg(ch->g, tsg);
	} else {
		err = g->ops.fifo.preempt_channel(ch->g, ch);
	}

	return err;
}

u32 gk20a_fifo_default_timeslice_us(struct gk20a *g)
{
	u64 slice = (((u64)(NVGPU_FIFO_DEFAULT_TIMESLICE_TIMEOUT <<
				NVGPU_FIFO_DEFAULT_TIMESLICE_SCALE) *
			(u64)g->ptimer_src_freq) /
			(u64)PTIMER_REF_FREQ_HZ);

	BUG_ON(slice > U64(U32_MAX));

	return (u32)slice;
}

int gk20a_fifo_tsg_set_timeslice(struct tsg_gk20a *tsg, u32 timeslice)
{
	struct gk20a *g = tsg->g;

	if (timeslice < g->min_timeslice_us ||
		timeslice > g->max_timeslice_us) {
		return -EINVAL;
	}

	gk20a_channel_get_timescale_from_timeslice(g, timeslice,
			&tsg->timeslice_timeout, &tsg->timeslice_scale);

	tsg->timeslice_us = timeslice;

	return g->ops.runlist.reload(g, tsg->runlist_id, true, true);
}

int gk20a_fifo_suspend(struct gk20a *g)
{
	nvgpu_log_fn(g, " ");

	/* stop bar1 snooping */
	if (g->ops.mm.is_bar1_supported(g)) {
		gk20a_writel(g, fifo_bar1_base_r(),
			fifo_bar1_base_valid_false_f());
	}

	/* disable fifo intr */
	g->ops.fifo.intr_0_enable(g, false);
	g->ops.fifo.intr_1_enable(g, false);

	nvgpu_log_fn(g, "done");
	return 0;
}

int gk20a_fifo_init_pbdma_map(struct gk20a *g, u32 *pbdma_map, u32 num_pbdma)
{
	u32 id;

	for (id = 0; id < num_pbdma; ++id) {
		pbdma_map[id] = gk20a_readl(g, fifo_pbdma_map_r(id));
	}

	return 0;
}
