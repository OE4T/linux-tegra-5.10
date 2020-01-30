/*
 * GV100 master
 *
 * Copyright (c) 2016-2020, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/types.h>
#include <nvgpu/io.h>
#include <nvgpu/mc.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/bug.h>
#include <nvgpu/engines.h>

#include "mc_gp10b.h"
#include "mc_gv100.h"

#include <nvgpu/hw/gv100/hw_mc_gv100.h>

bool gv100_mc_is_intr_nvlink_pending(struct gk20a *g, u32 mc_intr_0)
{
	return ((mc_intr_0 & mc_intr_nvlink_pending_f()) != 0U);
}

bool gv100_mc_is_stall_and_eng_intr_pending(struct gk20a *g, u32 engine_id,
			u32 *eng_intr_pending)
{
	u32 mc_intr_0 = nvgpu_readl(g, mc_intr_r(NVGPU_MC_INTR_STALLING));
	u32 stall_intr, eng_intr_mask;

	eng_intr_mask = nvgpu_engine_act_interrupt_mask(g, engine_id);
	*eng_intr_pending = mc_intr_0 & eng_intr_mask;

	stall_intr = mc_intr_pfifo_pending_f() |
			mc_intr_hub_pending_f() |
			mc_intr_priv_ring_pending_f() |
			mc_intr_pbus_pending_f() |
			mc_intr_ltc_pending_f() |
			mc_intr_nvlink_pending_f();

	nvgpu_log(g, gpu_dbg_info | gpu_dbg_intr,
		"mc_intr_0 = 0x%08x, eng_intr = 0x%08x",
		mc_intr_0 & stall_intr, *eng_intr_pending);

	return (mc_intr_0 & (eng_intr_mask | stall_intr)) != 0U;
}

u32 gv100_mc_reset_mask(struct gk20a *g, enum nvgpu_unit unit)
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
	case NVGPU_UNIT_NVDEC:
		mask = mc_enable_nvdec_enabled_f();
		break;
	default:
		WARN(1, "unknown reset unit %d", unit);
		break;
	}

	return mask;
}
