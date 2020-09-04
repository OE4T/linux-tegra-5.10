/*
 * Copyright (c) 2018-2020, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/gk20a.h>
#include <nvgpu/engines.h>
#include <nvgpu/device.h>
#include <nvgpu/ce.h>
#include <nvgpu/power_features/cg.h>
#include <nvgpu/mc.h>

int nvgpu_ce_init_support(struct gk20a *g)
{
	int err = 0;

	if (g->ops.ce.set_pce2lce_mapping != NULL) {
		g->ops.ce.set_pce2lce_mapping(g);
	}

	err = nvgpu_mc_reset_devtype(g, NVGPU_DEVTYPE_LCE);
	if (err != 0) {
		nvgpu_err(g, "NVGPU_DEVTYPE_LCE reset failed");
		return err;
	}

	nvgpu_cg_slcg_ce2_load_enable(g);

	nvgpu_cg_blcg_ce_load_enable(g);

	if (g->ops.ce.init_prod_values != NULL) {
		g->ops.ce.init_prod_values(g);
	}

	if (g->ops.ce.init_hw != NULL) {
		g->ops.ce.init_hw(g);
	}

	if (g->ops.ce.intr_enable != NULL) {
		g->ops.ce.intr_enable(g, true);
	}

	/** Enable interrupts at MC level */
	nvgpu_mc_intr_stall_unit_config(g, MC_INTR_UNIT_CE, MC_INTR_ENABLE);
	nvgpu_mc_intr_nonstall_unit_config(g, MC_INTR_UNIT_CE, MC_INTR_ENABLE);

	return 0;
}
