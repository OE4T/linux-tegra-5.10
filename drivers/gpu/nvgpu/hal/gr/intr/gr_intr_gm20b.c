/*
 * Copyright (c) 2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/gr/config.h>
#include <nvgpu/gr/gr.h>
#include <nvgpu/gr/gr_intr.h>

#include "gr_intr_gm20b.h"

#include <nvgpu/hw/gm20b/hw_gr_gm20b.h>

u32 gm20b_gr_intr_get_tpc_exception(struct gk20a *g, u32 offset,
				    struct nvgpu_gr_tpc_exception *pending_tpc)
{
	u32 tpc_exception = nvgpu_readl(g,
				gr_gpc0_tpc0_tpccs_tpc_exception_r()
				+ offset);

	(void) memset(pending_tpc, 0, sizeof(struct nvgpu_gr_tpc_exception));

	if (gr_gpc0_tpc0_tpccs_tpc_exception_tex_v(tpc_exception) ==
		gr_gpc0_tpc0_tpccs_tpc_exception_tex_pending_v()) {
			pending_tpc->tex_exception = true;
	}

	if (gr_gpc0_tpc0_tpccs_tpc_exception_sm_v(tpc_exception) ==
		gr_gpc0_tpc0_tpccs_tpc_exception_sm_pending_v()) {
			pending_tpc->sm_exception = true;
	}

	if ((tpc_exception & gr_gpc0_tpc0_tpccs_tpc_exception_mpc_m()) != 0U) {
		pending_tpc->mpc_exception = true;
	}

	return tpc_exception;
}

void gm20b_gr_intr_handle_tex_exception(struct gk20a *g, u32 gpc, u32 tpc)
{
	u32 gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_GPC_STRIDE);
	u32 tpc_in_gpc_stride = nvgpu_get_litter_value(g,
						GPU_LIT_TPC_IN_GPC_STRIDE);
	u32 offset = gpc_stride * gpc + tpc_in_gpc_stride * tpc;
	u32 esr;

	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg, " ");

	esr = nvgpu_readl(g,
			  gr_gpc0_tpc0_tex_m_hww_esr_r() + offset);
	nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg, "0x%08x", esr);

	nvgpu_writel(g,
		     gr_gpc0_tpc0_tex_m_hww_esr_r() + offset,
		     esr);
}

void gm20b_gr_intr_enable_hww_exceptions(struct gk20a *g)
{
	/* enable exceptions */
	nvgpu_writel(g, gr_fe_hww_esr_r(),
		     gr_fe_hww_esr_en_enable_f() |
		     gr_fe_hww_esr_reset_active_f());
	nvgpu_writel(g, gr_memfmt_hww_esr_r(),
		     gr_memfmt_hww_esr_en_enable_f() |
		     gr_memfmt_hww_esr_reset_active_f());
}

void gm20b_gr_intr_enable_interrupts(struct gk20a *g, bool enable)
{
	if (enable) {
		nvgpu_writel(g, gr_intr_r(), 0xFFFFFFFFU);
		nvgpu_writel(g, gr_intr_en_r(), 0xFFFFFFFFU);
	} else {
		nvgpu_writel(g, gr_intr_r(), 0);
		nvgpu_writel(g, gr_intr_en_r(), 0);
	}

}
void gm20b_gr_intr_enable_exceptions(struct gk20a *g,
				     struct nvgpu_gr_config *gr_config,
				     bool enable)
{
	u32 reg_value = (enable) ? 0xFFFFFFFFU : 0U;

	nvgpu_writel(g, gr_exception_en_r(), reg_value);
	nvgpu_writel(g, gr_exception1_en_r(), reg_value);
	nvgpu_writel(g, gr_exception2_en_r(), reg_value);
}

void gm20b_gr_intr_enable_gpc_exceptions(struct gk20a *g,
					 struct nvgpu_gr_config *gr_config)
{
	u32 tpc_mask, tpc_mask_calc;

	nvgpu_writel(g, gr_gpcs_tpcs_tpccs_tpc_exception_en_r(),
			gr_gpcs_tpcs_tpccs_tpc_exception_en_tex_enabled_f() |
			gr_gpcs_tpcs_tpccs_tpc_exception_en_sm_enabled_f());

	tpc_mask_calc = (u32)BIT32(
			 nvgpu_gr_config_get_max_tpc_per_gpc_count(gr_config));
	tpc_mask = gr_gpcs_gpccs_gpc_exception_en_tpc_f(tpc_mask_calc - 1U);

	nvgpu_writel(g, gr_gpcs_gpccs_gpc_exception_en_r(), tpc_mask);
}
