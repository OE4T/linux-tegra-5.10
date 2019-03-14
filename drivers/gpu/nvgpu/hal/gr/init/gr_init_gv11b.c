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
#include <nvgpu/soc.h>
#include <nvgpu/gr/ctx.h>

#include "gr_init_gv11b.h"

#include <nvgpu/hw/gv11b/hw_gr_gv11b.h>

int gv11b_gr_init_fs_state(struct gk20a *g)
{
	u32 data;
	int err = 0;
	u32 ver = g->params.gpu_arch + g->params.gpu_impl;

	nvgpu_log_fn(g, " ");

	data = nvgpu_readl(g, gr_gpcs_tpcs_sm_texio_control_r());
	data = set_field(data,
		gr_gpcs_tpcs_sm_texio_control_oor_addr_check_mode_m(),
		gr_gpcs_tpcs_sm_texio_control_oor_addr_check_mode_arm_63_48_match_f());
	nvgpu_writel(g, gr_gpcs_tpcs_sm_texio_control_r(), data);

	if (ver == NVGPU_GPUID_GV11B && nvgpu_is_soc_t194_a01(g)) {
		/*
		 * For t194 A01, Disable CBM alpha and beta invalidations
		 * Disable SCC pagepool invalidates
		 * Disable SWDX spill buffer invalidates
		 */
		data = nvgpu_readl(g, gr_gpcs_ppcs_cbm_debug_r());
		data = set_field(data,
			gr_gpcs_ppcs_cbm_debug_invalidate_alpha_m(),
			gr_gpcs_ppcs_cbm_debug_invalidate_alpha_disable_f());
		data = set_field(data,
			gr_gpcs_ppcs_cbm_debug_invalidate_beta_m(),
			gr_gpcs_ppcs_cbm_debug_invalidate_beta_disable_f());
		nvgpu_writel(g, gr_gpcs_ppcs_cbm_debug_r(), data);

		data = nvgpu_readl(g, gr_scc_debug_r());
		data = set_field(data,
			gr_scc_debug_pagepool_invalidates_m(),
			gr_scc_debug_pagepool_invalidates_disable_f());
		nvgpu_writel(g, gr_scc_debug_r(), data);

		data = nvgpu_readl(g, gr_gpcs_swdx_spill_unit_r());
		data = set_field(data,
			gr_gpcs_swdx_spill_unit_spill_buffer_cache_mgmt_mode_m(),
			gr_gpcs_swdx_spill_unit_spill_buffer_cache_mgmt_mode_disabled_f());
		nvgpu_writel(g, gr_gpcs_swdx_spill_unit_r(), data);
	}

	data = nvgpu_readl(g, gr_gpcs_tpcs_sm_disp_ctrl_r());
	data = set_field(data, gr_gpcs_tpcs_sm_disp_ctrl_re_suppress_m(),
			 gr_gpcs_tpcs_sm_disp_ctrl_re_suppress_disable_f());
	nvgpu_writel(g, gr_gpcs_tpcs_sm_disp_ctrl_r(), data);

	if (g->gr.fecs_feature_override_ecc_val != 0U) {
		nvgpu_writel(g,
			gr_fecs_feature_override_ecc_r(),
			g->gr.fecs_feature_override_ecc_val);
	}

	data = nvgpu_readl(g, gr_debug_0_r());
	data = set_field(data,
		gr_debug_0_scg_force_slow_drain_tpc_m(),
		gr_debug_0_scg_force_slow_drain_tpc_enabled_f());
	nvgpu_writel(g, gr_debug_0_r(), data);

	nvgpu_writel(g, gr_bes_zrop_settings_r(),
		     gr_bes_zrop_settings_num_active_ltcs_f(g->ltc_count));
	nvgpu_writel(g, gr_bes_crop_settings_r(),
		     gr_bes_crop_settings_num_active_ltcs_f(g->ltc_count));

	return err;
}

int gv11b_gr_init_preemption_state(struct gk20a *g, u32 gfxp_wfi_timeout_count,
	bool gfxp_wfi_timeout_unit_usec)
{
	u32 debug_2;
	u32 unit;

	nvgpu_log_fn(g, " ");

	if (gfxp_wfi_timeout_unit_usec) {
		unit = gr_debug_2_gfxp_wfi_timeout_unit_usec_f();
	} else {
		unit = gr_debug_2_gfxp_wfi_timeout_unit_sysclk_f();
	}

	debug_2 = nvgpu_readl(g, gr_debug_2_r());
	debug_2 = set_field(debug_2,
		gr_debug_2_gfxp_wfi_timeout_unit_m(),
		unit);
	nvgpu_writel(g, gr_debug_2_r(), debug_2);

	return 0;
}

void gv11b_gr_init_commit_global_timeslice(struct gk20a *g)
{
	u32 pd_ab_dist_cfg0;
	u32 ds_debug;
	u32 mpc_vtg_debug;
	u32 pe_vaf;
	u32 pe_vsc_vpc;

	nvgpu_log_fn(g, " ");

	pd_ab_dist_cfg0 = nvgpu_readl(g, gr_pd_ab_dist_cfg0_r());
	ds_debug = nvgpu_readl(g, gr_ds_debug_r());
	mpc_vtg_debug = nvgpu_readl(g, gr_gpcs_tpcs_mpc_vtg_debug_r());

	pe_vaf = nvgpu_readl(g, gr_gpcs_tpcs_pe_vaf_r());
	pe_vsc_vpc = nvgpu_readl(g, gr_gpcs_tpcs_pes_vsc_vpc_r());

	pe_vaf = gr_gpcs_tpcs_pe_vaf_fast_mode_switch_true_f() | pe_vaf;
	pe_vsc_vpc = gr_gpcs_tpcs_pes_vsc_vpc_fast_mode_switch_true_f() |
		     pe_vsc_vpc;
	pd_ab_dist_cfg0 = gr_pd_ab_dist_cfg0_timeslice_enable_en_f() |
			  pd_ab_dist_cfg0;
	ds_debug = gr_ds_debug_timeslice_mode_enable_f() | ds_debug;
	mpc_vtg_debug = gr_gpcs_tpcs_mpc_vtg_debug_timeslice_mode_enabled_f() |
			mpc_vtg_debug;

	nvgpu_gr_ctx_patch_write(g, NULL, gr_gpcs_tpcs_pe_vaf_r(), pe_vaf,
		false);
	nvgpu_gr_ctx_patch_write(g, NULL, gr_gpcs_tpcs_pes_vsc_vpc_r(),
		pe_vsc_vpc, false);
	nvgpu_gr_ctx_patch_write(g, NULL, gr_pd_ab_dist_cfg0_r(),
		pd_ab_dist_cfg0, false);
	nvgpu_gr_ctx_patch_write(g, NULL, gr_gpcs_tpcs_mpc_vtg_debug_r(),
		mpc_vtg_debug, false);
	nvgpu_gr_ctx_patch_write(g, NULL, gr_ds_debug_r(), ds_debug, false);
}
