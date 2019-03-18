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

#include <nvgpu/gr/config.h>

#include "gr_init_gv11b.h"

#include <nvgpu/hw/gv11b/hw_gr_gv11b.h>

/*
 * Each gpc can have maximum 32 tpcs, so each tpc index need
 * 5 bits. Each map register(32bits) can hold 6 tpcs info.
 */
#define GR_TPCS_INFO_FOR_MAPREGISTER 6U

void gv11b_gr_init_sm_id_numbering(struct gk20a *g,
					u32 gpc, u32 tpc, u32 smid)
{
	u32 gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_GPC_STRIDE);
	u32 tpc_in_gpc_stride = nvgpu_get_litter_value(g,
					GPU_LIT_TPC_IN_GPC_STRIDE);
	u32 gpc_offset = gpc_stride * gpc;
	u32 global_tpc_index = g->gr.sm_to_cluster[smid].global_tpc_index;
	u32 tpc_offset;

	tpc = g->ops.gr.get_nonpes_aware_tpc(g, gpc, tpc);
	tpc_offset = tpc_in_gpc_stride * tpc;

	nvgpu_writel(g, gr_gpc0_tpc0_sm_cfg_r() + gpc_offset + tpc_offset,
			gr_gpc0_tpc0_sm_cfg_tpc_id_f(global_tpc_index));
	nvgpu_writel(g, gr_gpc0_gpm_pd_sm_id_r(tpc) + gpc_offset,
			gr_gpc0_gpm_pd_sm_id_id_f(global_tpc_index));
	nvgpu_writel(g, gr_gpc0_tpc0_pe_cfg_smid_r() + gpc_offset + tpc_offset,
			gr_gpc0_tpc0_pe_cfg_smid_value_f(global_tpc_index));
}

int gv11b_gr_init_sm_id_config(struct gk20a *g, u32 *tpc_sm_id,
			       struct nvgpu_gr_config *gr_config)
{
	u32 i, j;
	u32 tpc_index, gpc_index, tpc_id;
	u32 sm_per_tpc = nvgpu_get_litter_value(g, GPU_LIT_NUM_SM_PER_TPC);
	u32 num_gpcs = nvgpu_get_litter_value(g, GPU_LIT_NUM_GPCS);

	/* Each NV_PGRAPH_PRI_CWD_GPC_TPC_ID can store 4 TPCs.*/
	for (i = 0U;
	     i <= ((nvgpu_gr_config_get_tpc_count(gr_config) - 1U) / 4U);
	     i++) {
		u32 reg = 0;
		u32 bit_stride = gr_cwd_gpc_tpc_id_gpc0_s() +
				 gr_cwd_gpc_tpc_id_tpc0_s();

		for (j = 0U; j < 4U; j++) {
			u32 sm_id;
			u32 bits;

			tpc_id = (i << 2) + j;
			sm_id = tpc_id * sm_per_tpc;

			if (sm_id >= g->gr.no_of_sm) {
				break;
			}

			gpc_index = g->gr.sm_to_cluster[sm_id].gpc_index;
			tpc_index = g->gr.sm_to_cluster[sm_id].tpc_index;

			bits = gr_cwd_gpc_tpc_id_gpc0_f(gpc_index) |
				gr_cwd_gpc_tpc_id_tpc0_f(tpc_index);
			reg |= bits << (j * bit_stride);

			tpc_sm_id[gpc_index + (num_gpcs * ((tpc_index & 4U)
				 >> 2U))] |= tpc_id << tpc_index * bit_stride;
		}
		nvgpu_writel(g, gr_cwd_gpc_tpc_id_r(i), reg);
	}

	for (i = 0U; i < gr_cwd_sm_id__size_1_v(); i++) {
		nvgpu_writel(g, gr_cwd_sm_id_r(i), tpc_sm_id[i]);
	}

	return 0;
}

void gv11b_gr_init_tpc_mask(struct gk20a *g, u32 gpc_index, u32 pes_tpc_mask)
{
	nvgpu_writel(g, gr_fe_tpc_fs_r(gpc_index), pes_tpc_mask);
}

int gv11b_gr_init_rop_mapping(struct gk20a *g,
			      struct nvgpu_gr_config *gr_config)
{
	u32 map;
	u32 i, j;
	u32 mapreg_num, base, offset, mapregs, tile_cnt, tpc_cnt;
	u32 num_gpcs = nvgpu_get_litter_value(g, GPU_LIT_NUM_GPCS);
	u32 num_tpc_per_gpc = nvgpu_get_litter_value(g,
				GPU_LIT_NUM_TPC_PER_GPC);
	u32 num_tpcs = num_gpcs * num_tpc_per_gpc;

	nvgpu_log_fn(g, " ");

	if (gr_config->map_tiles == NULL) {
		return -1;
	}

	nvgpu_writel(g, gr_crstr_map_table_cfg_r(),
		gr_crstr_map_table_cfg_row_offset_f(
			nvgpu_gr_config_get_map_row_offset(gr_config)) |
		gr_crstr_map_table_cfg_num_entries_f(
			nvgpu_gr_config_get_tpc_count(gr_config)));
	/*
	 * 6 tpc can be stored in one map register.
	 * But number of tpcs are not always multiple of six,
	 * so adding additional check for valid number of
	 * tpcs before programming map register.
	 */
	mapregs = DIV_ROUND_UP(num_tpcs, GR_TPCS_INFO_FOR_MAPREGISTER);

	for (mapreg_num = 0U, base = 0U; mapreg_num < mapregs; mapreg_num++,
				base = base + GR_TPCS_INFO_FOR_MAPREGISTER) {
		map = 0U;
		for (offset = 0U;
		      (offset < GR_TPCS_INFO_FOR_MAPREGISTER && num_tpcs > 0U);
		      offset++, num_tpcs--) {
			tile_cnt = nvgpu_gr_config_get_map_tile_count(
						gr_config, base + offset);
			switch (offset) {
			case 0:
				map = map | gr_crstr_gpc_map_tile0_f(tile_cnt);
				break;
			case 1:
				map = map | gr_crstr_gpc_map_tile1_f(tile_cnt);
				break;
			case 2:
				map = map | gr_crstr_gpc_map_tile2_f(tile_cnt);
				break;
			case 3:
				map = map | gr_crstr_gpc_map_tile3_f(tile_cnt);
				break;
			case 4:
				map = map | gr_crstr_gpc_map_tile4_f(tile_cnt);
				break;
			case 5:
				map = map | gr_crstr_gpc_map_tile5_f(tile_cnt);
				break;
			default:
				nvgpu_err(g, "incorrect rop mapping %x",
					  offset);
				break;
			}
		}

		nvgpu_writel(g, gr_crstr_gpc_map_r(mapreg_num), map);
		nvgpu_writel(g, gr_ppcs_wwdx_map_gpc_map_r(mapreg_num), map);
		nvgpu_writel(g, gr_rstr2d_gpc_map_r(mapreg_num), map);
	}

	nvgpu_writel(g, gr_ppcs_wwdx_map_table_cfg_r(),
		gr_ppcs_wwdx_map_table_cfg_row_offset_f(
			nvgpu_gr_config_get_map_row_offset(gr_config)) |
		gr_ppcs_wwdx_map_table_cfg_num_entries_f(
			nvgpu_gr_config_get_tpc_count(gr_config)));

	for (i = 0U, j = 1U; i < gr_ppcs_wwdx_map_table_cfg_coeff__size_1_v();
					i++, j = j + 4U) {
		tpc_cnt = nvgpu_gr_config_get_tpc_count(gr_config);
		nvgpu_writel(g, gr_ppcs_wwdx_map_table_cfg_coeff_r(i),
			gr_ppcs_wwdx_map_table_cfg_coeff_0_mod_value_f(
				(BIT32(j) % tpc_cnt)) |
			gr_ppcs_wwdx_map_table_cfg_coeff_1_mod_value_f(
				(BIT32(j + 1U) % tpc_cnt)) |
			gr_ppcs_wwdx_map_table_cfg_coeff_2_mod_value_f(
				(BIT32(j + 2U) % tpc_cnt)) |
			gr_ppcs_wwdx_map_table_cfg_coeff_3_mod_value_f(
				(BIT32(j + 3U) % tpc_cnt)));
	}

	nvgpu_writel(g, gr_rstr2d_map_table_cfg_r(),
		gr_rstr2d_map_table_cfg_row_offset_f(
			nvgpu_gr_config_get_map_row_offset(gr_config)) |
		gr_rstr2d_map_table_cfg_num_entries_f(
			nvgpu_gr_config_get_tpc_count(gr_config)));

	return 0;
}

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
