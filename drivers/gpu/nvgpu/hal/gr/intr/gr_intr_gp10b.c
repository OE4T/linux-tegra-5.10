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

#include <nvgpu/hw/gp10b/hw_gr_gp10b.h>

void gp10b_gr_intr_set_go_idle_timeout(struct gk20a *g, u32 data)
{
	nvgpu_writel(g, gr_fe_go_idle_timeout_r(), data);
}

void gp10b_gr_intr_set_coalesce_buffer_size(struct gk20a *g, u32 data)
{
	u32 val;

	nvgpu_log_fn(g, " ");

	val = nvgpu_readl(g, gr_gpcs_tc_debug0_r());
	val = set_field(val, gr_gpcs_tc_debug0_limit_coalesce_buffer_size_m(),
			gr_gpcs_tc_debug0_limit_coalesce_buffer_size_f(data));
	nvgpu_writel(g, gr_gpcs_tc_debug0_r(), val);

	nvgpu_log_fn(g, "done");
}

int gp10b_gr_intr_handle_sw_method(struct gk20a *g, u32 addr,
				     u32 class_num, u32 offset, u32 data)
{
	nvgpu_log_fn(g, " ");

	if (class_num == PASCAL_COMPUTE_A) {
		switch (offset << 2) {
		case NVC0C0_SET_SHADER_EXCEPTIONS:
			g->ops.gr.intr.set_shader_exceptions(g, data);
			break;
		case NVC0C0_SET_RD_COALESCE:
			g->ops.gr.init.lg_coalesce(g, data);
			break;
		default:
			goto fail;
		}
	}

	if (class_num == PASCAL_A) {
		switch (offset << 2) {
		case NVC097_SET_SHADER_EXCEPTIONS:
			g->ops.gr.intr.set_shader_exceptions(g, data);
			break;
		case NVC097_SET_CIRCULAR_BUFFER_SIZE:
			g->ops.gr.set_circular_buffer_size(g, data);
			break;
		case NVC097_SET_ALPHA_CIRCULAR_BUFFER_SIZE:
			g->ops.gr.set_alpha_circular_buffer_size(g, data);
			break;
		case NVC097_SET_GO_IDLE_TIMEOUT:
			gp10b_gr_intr_set_go_idle_timeout(g, data);
			break;
		case NVC097_SET_COALESCE_BUFFER_SIZE:
			gp10b_gr_intr_set_coalesce_buffer_size(g, data);
			break;
		case NVC097_SET_RD_COALESCE:
			g->ops.gr.init.lg_coalesce(g, data);
			break;
		case NVC097_SET_BES_CROP_DEBUG3:
			g->ops.gr.set_bes_crop_debug3(g, data);
			break;
		case NVC097_SET_BES_CROP_DEBUG4:
			g->ops.gr.set_bes_crop_debug4(g, data);
			break;
		default:
			goto fail;
		}
	}
	return 0;

fail:
	return -EINVAL;
}

void gp10b_gr_intr_handle_tex_exception(struct gk20a *g, u32 gpc, u32 tpc)
{
	u32 gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_GPC_STRIDE);
	u32 tpc_in_gpc_stride = nvgpu_get_litter_value(g,
						GPU_LIT_TPC_IN_GPC_STRIDE);
	u32 offset = gpc_stride * gpc + tpc_in_gpc_stride * tpc;
	u32 esr;
	u32 ecc_stats_reg_val;

	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg, " ");

	esr = nvgpu_readl(g,
			 gr_gpc0_tpc0_tex_m_hww_esr_r() + offset);
	nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg, "0x%08x", esr);

	if ((esr & gr_gpc0_tpc0_tex_m_hww_esr_ecc_sec_pending_f()) != 0U) {
		nvgpu_log(g, gpu_dbg_fn | gpu_dbg_intr,
			"Single bit error detected in TEX!");

		/* Pipe 0 counters */
		nvgpu_writel(g,
			gr_pri_gpc0_tpc0_tex_m_routing_r() + offset,
			gr_pri_gpc0_tpc0_tex_m_routing_sel_pipe0_f());

		ecc_stats_reg_val = nvgpu_readl(g,
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_total_r() + offset);
		g->ecc.gr.tex_ecc_total_sec_pipe0_count[gpc][tpc].counter +=
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_total_sec_v(
							ecc_stats_reg_val);
		ecc_stats_reg_val &=
			~gr_pri_gpc0_tpc0_tex_m_ecc_cnt_total_sec_m();
		nvgpu_writel(g,
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_total_r() + offset,
			ecc_stats_reg_val);

		ecc_stats_reg_val = nvgpu_readl(g,
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_unique_r() + offset);
		g->ecc.gr.tex_unique_ecc_sec_pipe0_count[gpc][tpc].counter +=
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_unique_sec_v(
							ecc_stats_reg_val);
		ecc_stats_reg_val &=
			~gr_pri_gpc0_tpc0_tex_m_ecc_cnt_unique_sec_m();
		nvgpu_writel(g,
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_unique_r() + offset,
			ecc_stats_reg_val);


		/* Pipe 1 counters */
		nvgpu_writel(g,
			gr_pri_gpc0_tpc0_tex_m_routing_r() + offset,
			gr_pri_gpc0_tpc0_tex_m_routing_sel_pipe1_f());

		ecc_stats_reg_val = nvgpu_readl(g,
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_total_r() + offset);
		g->ecc.gr.tex_ecc_total_sec_pipe1_count[gpc][tpc].counter +=
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_total_sec_v(
							ecc_stats_reg_val);
		ecc_stats_reg_val &=
			~gr_pri_gpc0_tpc0_tex_m_ecc_cnt_total_sec_m();
		nvgpu_writel(g,
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_total_r() + offset,
			ecc_stats_reg_val);

		ecc_stats_reg_val = nvgpu_readl(g,
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_unique_r() + offset);
		g->ecc.gr.tex_unique_ecc_sec_pipe1_count[gpc][tpc].counter +=
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_unique_sec_v(
							ecc_stats_reg_val);
		ecc_stats_reg_val &=
			~gr_pri_gpc0_tpc0_tex_m_ecc_cnt_unique_sec_m();
		nvgpu_writel(g,
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_unique_r() + offset,
			ecc_stats_reg_val);


		nvgpu_writel(g,
			gr_pri_gpc0_tpc0_tex_m_routing_r() + offset,
			gr_pri_gpc0_tpc0_tex_m_routing_sel_default_f());
	}

	if ((esr & gr_gpc0_tpc0_tex_m_hww_esr_ecc_ded_pending_f()) != 0U) {
		nvgpu_log(g, gpu_dbg_fn | gpu_dbg_intr,
			"Double bit error detected in TEX!");

		/* Pipe 0 counters */
		nvgpu_writel(g,
			gr_pri_gpc0_tpc0_tex_m_routing_r() + offset,
			gr_pri_gpc0_tpc0_tex_m_routing_sel_pipe0_f());

		ecc_stats_reg_val = nvgpu_readl(g,
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_total_r() + offset);
		g->ecc.gr.tex_ecc_total_ded_pipe0_count[gpc][tpc].counter +=
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_total_ded_v(
							ecc_stats_reg_val);
		ecc_stats_reg_val &=
			~gr_pri_gpc0_tpc0_tex_m_ecc_cnt_total_ded_m();
		nvgpu_writel(g,
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_total_r() + offset,
			ecc_stats_reg_val);

		ecc_stats_reg_val = nvgpu_readl(g,
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_unique_r() + offset);
		g->ecc.gr.tex_unique_ecc_ded_pipe0_count[gpc][tpc].counter +=
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_unique_ded_v(
							ecc_stats_reg_val);
		ecc_stats_reg_val &=
			~gr_pri_gpc0_tpc0_tex_m_ecc_cnt_unique_ded_m();
		nvgpu_writel(g,
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_unique_r() + offset,
			ecc_stats_reg_val);


		/* Pipe 1 counters */
		nvgpu_writel(g,
			gr_pri_gpc0_tpc0_tex_m_routing_r() + offset,
			gr_pri_gpc0_tpc0_tex_m_routing_sel_pipe1_f());

		ecc_stats_reg_val = nvgpu_readl(g,
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_total_r() + offset);
		g->ecc.gr.tex_ecc_total_ded_pipe1_count[gpc][tpc].counter +=
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_total_ded_v(
							ecc_stats_reg_val);
		ecc_stats_reg_val &=
			~gr_pri_gpc0_tpc0_tex_m_ecc_cnt_total_ded_m();
		nvgpu_writel(g,
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_total_r() + offset,
			ecc_stats_reg_val);

		ecc_stats_reg_val = nvgpu_readl(g,
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_unique_r() + offset);
		g->ecc.gr.tex_unique_ecc_ded_pipe1_count[gpc][tpc].counter +=
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_unique_ded_v(
							ecc_stats_reg_val);
		ecc_stats_reg_val &=
			~gr_pri_gpc0_tpc0_tex_m_ecc_cnt_unique_ded_m();
		nvgpu_writel(g,
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_unique_r() + offset,
			ecc_stats_reg_val);


		nvgpu_writel(g,
			gr_pri_gpc0_tpc0_tex_m_routing_r() + offset,
			gr_pri_gpc0_tpc0_tex_m_routing_sel_default_f());
	}

	nvgpu_writel(g,
		     gr_gpc0_tpc0_tex_m_hww_esr_r() + offset,
		     esr | gr_gpc0_tpc0_tex_m_hww_esr_reset_active_f());

}
