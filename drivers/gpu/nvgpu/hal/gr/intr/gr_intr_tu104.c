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
#include <nvgpu/class.h>

#include <nvgpu/gr/config.h>

#include "gr_intr_gp10b.h"
#include "gr_intr_gv11b.h"
#include "gr_intr_tu104.h"

#include <nvgpu/hw/tu104/hw_gr_tu104.h>

static void gr_tu104_set_sm_disp_ctrl(struct gk20a *g, u32 data)
{
	u32 reg_val;

	nvgpu_log_fn(g, " ");

	reg_val = nvgpu_readl(g, gr_gpcs_tpcs_sm_disp_ctrl_r());

	if ((data & NVC5C0_SET_SM_DISP_CTRL_COMPUTE_SHADER_QUAD_MASK)
	       == NVC5C0_SET_SM_DISP_CTRL_COMPUTE_SHADER_QUAD_DISABLE) {
		reg_val = set_field(reg_val,
		     gr_gpcs_tpcs_sm_disp_ctrl_compute_shader_quad_m(),
		     gr_gpcs_tpcs_sm_disp_ctrl_compute_shader_quad_disable_f()
		     );
	} else {
		if ((data & NVC5C0_SET_SM_DISP_CTRL_COMPUTE_SHADER_QUAD_MASK)
		     == NVC5C0_SET_SM_DISP_CTRL_COMPUTE_SHADER_QUAD_ENABLE) {
			reg_val = set_field(reg_val,
			     gr_gpcs_tpcs_sm_disp_ctrl_compute_shader_quad_m(),
			     gr_gpcs_tpcs_sm_disp_ctrl_compute_shader_quad_enable_f()
			     );
		}
	}

	nvgpu_writel(g, gr_gpcs_tpcs_sm_disp_ctrl_r(), reg_val);
}

int tu104_gr_intr_handle_sw_method(struct gk20a *g, u32 addr,
			      u32 class_num, u32 offset, u32 data)
{
	int ret = 0;

	nvgpu_log_fn(g, " ");

	if (class_num == TURING_COMPUTE_A) {
		switch (offset << 2) {
		case NVC5C0_SET_SHADER_EXCEPTIONS:
			g->ops.gr.intr.set_shader_exceptions(g, data);
			break;
		case NVC5C0_SET_SKEDCHECK:
			gv11b_gr_intr_set_skedcheck(g, data);
			break;
		case NVC5C0_SET_SM_DISP_CTRL:
			gr_tu104_set_sm_disp_ctrl(g, data);
			break;
		case NVC5C0_SET_SHADER_CUT_COLLECTOR:
			gv11b_gr_intr_set_shader_cut_collector(g, data);
			break;
		default:
			ret = -EINVAL;
			break;
		}
	}

	if (ret != 0) {
		goto fail;
	}

	if (class_num == TURING_A) {
		switch (offset << 2) {
		case NVC597_SET_SHADER_EXCEPTIONS:
			g->ops.gr.intr.set_shader_exceptions(g, data);
			break;
		case NVC597_SET_CIRCULAR_BUFFER_SIZE:
			g->ops.gr.set_circular_buffer_size(g, data);
			break;
		case NVC597_SET_ALPHA_CIRCULAR_BUFFER_SIZE:
			g->ops.gr.set_alpha_circular_buffer_size(g, data);
			break;
		case NVC597_SET_GO_IDLE_TIMEOUT:
			gp10b_gr_intr_set_go_idle_timeout(g, data);
			break;
		case NVC097_SET_COALESCE_BUFFER_SIZE:
			gp10b_gr_intr_set_coalesce_buffer_size(g, data);
			break;
		case NVC597_SET_TEX_IN_DBG:
			gv11b_gr_intr_set_tex_in_dbg(g, data);
			break;
		case NVC597_SET_SKEDCHECK:
			gv11b_gr_intr_set_skedcheck(g, data);
			break;
		case NVC597_SET_BES_CROP_DEBUG3:
			g->ops.gr.set_bes_crop_debug3(g, data);
			break;
		case NVC597_SET_BES_CROP_DEBUG4:
			g->ops.gr.set_bes_crop_debug4(g, data);
			break;
		case NVC597_SET_SM_DISP_CTRL:
			gr_tu104_set_sm_disp_ctrl(g, data);
			break;
		case NVC597_SET_SHADER_CUT_COLLECTOR:
			gv11b_gr_intr_set_shader_cut_collector(g, data);
			break;
		default:
			ret = -EINVAL;
			break;
		}
	}

fail:
	return ret;
}

void tu104_gr_intr_enable_gpc_exceptions(struct gk20a *g,
					 struct nvgpu_gr_config *gr_config)
{
	u32 tpc_mask, tpc_mask_calc;

	nvgpu_writel(g, gr_gpcs_tpcs_tpccs_tpc_exception_en_r(),
			gr_gpcs_tpcs_tpccs_tpc_exception_en_sm_enabled_f());

	tpc_mask_calc = (u32)BIT32(
			 nvgpu_gr_config_get_max_tpc_per_gpc_count(gr_config));
	tpc_mask =
		gr_gpcs_gpccs_gpc_exception_en_tpc_f(tpc_mask_calc - 1U);

	nvgpu_writel(g, gr_gpcs_gpccs_gpc_exception_en_r(),
		(tpc_mask | gr_gpcs_gpccs_gpc_exception_en_gcc_f(1U) |
			    gr_gpcs_gpccs_gpc_exception_en_gpccs_f(1U) |
			    gr_gpcs_gpccs_gpc_exception_en_gpcmmu_f(1U)));
}
