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

#include "gr_intr_gv11b.h"

#include <nvgpu/hw/gv11b/hw_gr_gv11b.h>

void gv11b_gr_intr_enable_exceptions(struct gk20a *g,
				     struct nvgpu_gr_config *gr_config,
				     bool enable)
{
	u32 reg_val = 0U;

	if (!enable) {
		nvgpu_writel(g, gr_exception_en_r(), reg_val);
		nvgpu_writel(g, gr_exception1_en_r(), reg_val);
		nvgpu_writel(g, gr_exception2_en_r(), reg_val);
		return;
	}

	/*
	 * clear exceptions :
	 * other than SM : hww_esr are reset in *enable_hww_excetpions*
	 * SM            : cleared in *set_hww_esr_report_mask*
	 */

	/* enable exceptions */
	nvgpu_writel(g, gr_exception2_en_r(), 0x0U); /* BE not enabled */

	reg_val = (u32)BIT32(nvgpu_gr_config_get_gpc_count(gr_config));
	nvgpu_writel(g, gr_exception1_en_r(), (reg_val - 1U));

	reg_val = gr_exception_en_fe_enabled_f() |
			gr_exception_en_memfmt_enabled_f() |
			gr_exception_en_pd_enabled_f() |
			gr_exception_en_scc_enabled_f() |
			gr_exception_en_ds_enabled_f() |
			gr_exception_en_ssync_enabled_f() |
			gr_exception_en_mme_enabled_f() |
			gr_exception_en_sked_enabled_f() |
			gr_exception_en_gpc_enabled_f();

	nvgpu_log(g, gpu_dbg_info, "gr_exception_en 0x%08x", reg_val);

	nvgpu_writel(g, gr_exception_en_r(), reg_val);
}

void gv11b_gr_intr_enable_gpc_exceptions(struct gk20a *g,
					 struct nvgpu_gr_config *gr_config)
{
	u32 tpc_mask, tpc_mask_calc;

	nvgpu_writel(g, gr_gpcs_tpcs_tpccs_tpc_exception_en_r(),
			gr_gpcs_tpcs_tpccs_tpc_exception_en_sm_enabled_f() |
			gr_gpcs_tpcs_tpccs_tpc_exception_en_mpc_enabled_f());

	tpc_mask_calc = (u32)BIT32(
			 nvgpu_gr_config_get_max_tpc_per_gpc_count(gr_config));
	tpc_mask =
		gr_gpcs_gpccs_gpc_exception_en_tpc_f(tpc_mask_calc - 1U);

	nvgpu_writel(g, gr_gpcs_gpccs_gpc_exception_en_r(),
		(tpc_mask | gr_gpcs_gpccs_gpc_exception_en_gcc_f(1U) |
			    gr_gpcs_gpccs_gpc_exception_en_gpccs_f(1U) |
			    gr_gpcs_gpccs_gpc_exception_en_gpcmmu_f(1U)));
}
