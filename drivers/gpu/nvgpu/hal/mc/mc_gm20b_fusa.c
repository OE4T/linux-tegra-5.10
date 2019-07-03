/*
 * GM20B Master Control
 *
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

#include <nvgpu/timers.h>
#include <nvgpu/atomic.h>
#include <nvgpu/unit.h>
#include <nvgpu/io.h>
#include <nvgpu/mc.h>
#include <nvgpu/ltc.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/bug.h>
#include <nvgpu/engines.h>
#include <nvgpu/power_features/pg.h>

#include "mc_gm20b.h"

#include <nvgpu/hw/gm20b/hw_mc_gm20b.h>

u32 gm20b_mc_isr_nonstall(struct gk20a *g)
{
	u32 ops = 0U;
	u32 mc_intr_1;
	u32 eng_id;
	u32 act_eng_id = 0U;
	enum nvgpu_fifo_engine engine_enum;

	mc_intr_1 = g->ops.mc.intr_nonstall(g);

	if (g->ops.mc.is_intr1_pending(g, NVGPU_UNIT_FIFO, mc_intr_1)) {
		ops |= g->ops.fifo.intr_1_isr(g);
	}

	for (eng_id = 0U; eng_id < g->fifo.num_engines; eng_id++) {
		struct nvgpu_engine_info *engine_info;

		act_eng_id = g->fifo.active_engines_list[eng_id];
		engine_info = &g->fifo.engine_info[act_eng_id];

		if ((mc_intr_1 & engine_info->intr_mask) != 0U) {
			engine_enum = engine_info->engine_enum;
			/* GR Engine */
			if (engine_enum == NVGPU_ENGINE_GR) {
				ops |= g->ops.gr.intr.nonstall_isr(g);
			}
			/* CE Engine */
			if (((engine_enum == NVGPU_ENGINE_GRCE) ||
			     (engine_enum == NVGPU_ENGINE_ASYNC_CE)) &&
			      (g->ops.ce.isr_nonstall != NULL)) {
				ops |= g->ops.ce.isr_nonstall(g,
					engine_info->inst_id,
					engine_info->pri_base);
			}
		}
	}

	return ops;
}

void gm20b_mc_disable(struct gk20a *g, u32 units)
{
	u32 pmc;

	nvgpu_log(g, gpu_dbg_info, "pmc disable: %08x", units);

	nvgpu_spinlock_acquire(&g->mc_enable_lock);
	pmc = nvgpu_readl(g, mc_enable_r());
	pmc &= ~units;
	nvgpu_writel(g, mc_enable_r(), pmc);
	nvgpu_spinlock_release(&g->mc_enable_lock);
}

void gm20b_mc_enable(struct gk20a *g, u32 units)
{
	u32 pmc;

	nvgpu_log(g, gpu_dbg_info, "pmc enable: %08x", units);

	nvgpu_spinlock_acquire(&g->mc_enable_lock);
	pmc = nvgpu_readl(g, mc_enable_r());
	pmc |= units;
	nvgpu_writel(g, mc_enable_r(), pmc);
	pmc = nvgpu_readl(g, mc_enable_r());
	nvgpu_spinlock_release(&g->mc_enable_lock);

	nvgpu_udelay(MC_ENABLE_DELAY_US);
}

void gm20b_mc_reset(struct gk20a *g, u32 units)
{
	g->ops.mc.disable(g, units);
	if ((units & nvgpu_engine_get_all_ce_reset_mask(g)) != 0U) {
		nvgpu_udelay(MC_RESET_CE_DELAY_US);
	} else {
		nvgpu_udelay(MC_RESET_DELAY_US);
	}
	g->ops.mc.enable(g, units);
}

u32 gm20b_mc_reset_mask(struct gk20a *g, enum nvgpu_unit unit)
{
	u32 mask = 0U;

	switch (unit) {
	case NVGPU_UNIT_FIFO:
		mask = mc_enable_pfifo_enabled_f();
		break;
	case NVGPU_UNIT_PERFMON:
		mask = mc_enable_perfmon_enabled_f();
		break;
	case NVGPU_UNIT_GRAPH:
		mask = mc_enable_pgraph_enabled_f();
		break;
	case NVGPU_UNIT_BLG:
		mask = mc_enable_blg_enabled_f();
		break;
	case NVGPU_UNIT_PWR:
		mask = mc_enable_pwr_enabled_f();
		break;
	default:
		WARN(1, "unknown reset unit %d", unit);
		break;
	}

	return mask;
}

bool gm20b_mc_is_enabled(struct gk20a *g, enum nvgpu_unit unit)
{
	u32 mask = g->ops.mc.reset_mask(g, unit);

	return (nvgpu_readl(g, mc_enable_r()) & mask) != 0U;
}
