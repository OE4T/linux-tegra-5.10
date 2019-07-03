/*
 * GM20B THERMAL
 *
 * Copyright (c) 2015-2019, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/utils.h>
#include <nvgpu/enabled.h>
#include <nvgpu/fifo.h>
#include <nvgpu/power_features/cg.h>
#include <nvgpu/gk20a.h>

#include "therm_gm20b.h"

#include <nvgpu/hw/gm20b/hw_therm_gm20b.h>

void gm20b_therm_init_blcg_mode(struct gk20a *g, u32 mode, u32 engine)
{
	u32 gate_ctrl;
	bool error_status = false;

	if (!nvgpu_is_enabled(g, NVGPU_GPU_CAN_BLCG)) {
		return;
	}

	gate_ctrl = nvgpu_readl(g, therm_gate_ctrl_r(engine));

	switch (mode) {
	case BLCG_RUN:
		gate_ctrl = set_field(gate_ctrl,
				therm_gate_ctrl_blk_clk_m(),
				therm_gate_ctrl_blk_clk_run_f());
		break;
	case BLCG_AUTO:
		gate_ctrl = set_field(gate_ctrl,
				therm_gate_ctrl_blk_clk_m(),
				therm_gate_ctrl_blk_clk_auto_f());
		break;
	default:
		nvgpu_err(g,
			"invalid blcg mode %d", mode);
		error_status = true;
		break;
	}

	if (error_status == true) {
		return;
	}

	nvgpu_writel(g, therm_gate_ctrl_r(engine), gate_ctrl);
}
