/*
 * GP10B master
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

#include <nvgpu/gk20a.h>
#include <nvgpu/io.h>
#include <nvgpu/mc.h>
#include <nvgpu/ltc.h>
#include <nvgpu/engines.h>
#include <nvgpu/power_features/pg.h>

#include "mc_gp10b.h"

#include <nvgpu/atomic.h>
#include <nvgpu/unit.h>

#include <nvgpu/hw/gp10b/hw_mc_gp10b.h>

void mc_gp10b_intr_enable(struct gk20a *g)
{
	u32 eng_intr_mask = nvgpu_engine_interrupt_mask(g);

	nvgpu_writel(g, mc_intr_en_clear_r(NVGPU_MC_INTR_STALLING),
				U32_MAX);
	g->mc_intr_mask_restore[NVGPU_MC_INTR_STALLING] =
				mc_intr_pfifo_pending_f() |
				 mc_intr_priv_ring_pending_f() |
				 mc_intr_pbus_pending_f() |
				 mc_intr_ltc_pending_f() |
				 mc_intr_replayable_fault_pending_f() |
				 eng_intr_mask;
	nvgpu_writel(g, mc_intr_en_set_r(NVGPU_MC_INTR_STALLING),
			g->mc_intr_mask_restore[NVGPU_MC_INTR_STALLING]);

	nvgpu_writel(g, mc_intr_en_clear_r(NVGPU_MC_INTR_NONSTALLING),
				U32_MAX);
	g->mc_intr_mask_restore[NVGPU_MC_INTR_NONSTALLING] =
				mc_intr_pfifo_pending_f() |
				 eng_intr_mask;
	nvgpu_writel(g, mc_intr_en_set_r(NVGPU_MC_INTR_NONSTALLING),
			g->mc_intr_mask_restore[NVGPU_MC_INTR_NONSTALLING]);
}
