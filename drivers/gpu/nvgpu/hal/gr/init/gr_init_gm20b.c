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
#include <nvgpu/log.h>
#include <nvgpu/bug.h>
#include <nvgpu/timers.h>
#include <nvgpu/enabled.h>
#include <nvgpu/engines.h>
#include <nvgpu/engine_status.h>
#include <nvgpu/netlist.h>
#include <nvgpu/gr/ctx.h>
#include <nvgpu/gr/config.h>
#include <nvgpu/ltc.h>

#include <nvgpu/gr/gr.h>
#include <nvgpu/gr/config.h>

#include "gr_init_gm20b.h"

#include <nvgpu/hw/gm20b/hw_gr_gm20b.h>

#define FE_PWR_MODE_TIMEOUT_MAX_US 2000U
#define FE_PWR_MODE_TIMEOUT_DEFAULT_US 10U
#define FECS_CTXSW_RESET_DELAY_US 10U

void gm20b_gr_init_lg_coalesce(struct gk20a *g, u32 data)
{
	u32 val;

	nvgpu_log_fn(g, " ");

	val = nvgpu_readl(g, gr_gpcs_tpcs_tex_m_dbg2_r());
	val = set_field(val,
			gr_gpcs_tpcs_tex_m_dbg2_lg_rd_coalesce_en_m(),
			gr_gpcs_tpcs_tex_m_dbg2_lg_rd_coalesce_en_f(data));
	nvgpu_writel(g, gr_gpcs_tpcs_tex_m_dbg2_r(), val);
}

void gm20b_gr_init_su_coalesce(struct gk20a *g, u32 data)
{
	u32 reg;

	reg = nvgpu_readl(g, gr_gpcs_tpcs_tex_m_dbg2_r());
	reg = set_field(reg,
			gr_gpcs_tpcs_tex_m_dbg2_su_rd_coalesce_en_m(),
			gr_gpcs_tpcs_tex_m_dbg2_su_rd_coalesce_en_f(data));

	nvgpu_writel(g, gr_gpcs_tpcs_tex_m_dbg2_r(), reg);
}

void gm20b_gr_init_pes_vsc_stream(struct gk20a *g)
{
	u32 data = nvgpu_readl(g, gr_gpc0_ppc0_pes_vsc_strem_r());

	data = set_field(data, gr_gpc0_ppc0_pes_vsc_strem_master_pe_m(),
			gr_gpc0_ppc0_pes_vsc_strem_master_pe_true_f());
	nvgpu_writel(g, gr_gpc0_ppc0_pes_vsc_strem_r(), data);
}

void gm20b_gr_init_gpc_mmu(struct gk20a *g)
{
	u32 temp;

	nvgpu_log_info(g, "initialize gpc mmu");

	temp = g->ops.fb.mmu_ctrl(g);
	temp &= gr_gpcs_pri_mmu_ctrl_vm_pg_size_m() |
		gr_gpcs_pri_mmu_ctrl_use_pdb_big_page_size_m() |
		gr_gpcs_pri_mmu_ctrl_use_full_comp_tag_line_m() |
		gr_gpcs_pri_mmu_ctrl_vol_fault_m() |
		gr_gpcs_pri_mmu_ctrl_comp_fault_m() |
		gr_gpcs_pri_mmu_ctrl_miss_gran_m() |
		gr_gpcs_pri_mmu_ctrl_cache_mode_m() |
		gr_gpcs_pri_mmu_ctrl_mmu_aperture_m() |
		gr_gpcs_pri_mmu_ctrl_mmu_vol_m() |
		gr_gpcs_pri_mmu_ctrl_mmu_disable_m();
	nvgpu_writel(g, gr_gpcs_pri_mmu_ctrl_r(), temp);
	nvgpu_writel(g, gr_gpcs_pri_mmu_pm_unit_mask_r(), 0);
	nvgpu_writel(g, gr_gpcs_pri_mmu_pm_req_mask_r(), 0);

	nvgpu_writel(g, gr_gpcs_pri_mmu_debug_ctrl_r(),
			g->ops.fb.mmu_debug_ctrl(g));
	nvgpu_writel(g, gr_gpcs_pri_mmu_debug_wr_r(),
			g->ops.fb.mmu_debug_wr(g));
	nvgpu_writel(g, gr_gpcs_pri_mmu_debug_rd_r(),
			g->ops.fb.mmu_debug_rd(g));

	nvgpu_writel(g, gr_gpcs_mmu_num_active_ltcs_r(),
			nvgpu_ltc_get_ltc_count(g));
}

void gm20b_gr_init_fifo_access(struct gk20a *g, bool enable)
{
	u32 fifo_val;

	fifo_val = nvgpu_readl(g, gr_gpfifo_ctl_r());
	fifo_val &= ~gr_gpfifo_ctl_semaphore_access_f(1);
	fifo_val &= ~gr_gpfifo_ctl_access_f(1);

	if (enable) {
		fifo_val |= (gr_gpfifo_ctl_access_enabled_f() |
			gr_gpfifo_ctl_semaphore_access_enabled_f());
	} else {
		fifo_val |= (gr_gpfifo_ctl_access_f(0) |
			gr_gpfifo_ctl_semaphore_access_f(0));
	}

	nvgpu_writel(g, gr_gpfifo_ctl_r(), fifo_val);
}

void gm20b_gr_init_get_access_map(struct gk20a *g,
				   u32 **whitelist, int *num_entries)
{
	static u32 wl_addr_gm20b[] = {
		/* this list must be sorted (low to high) */
		0x404468, /* gr_pri_mme_max_instructions       */
		0x418300, /* gr_pri_gpcs_rasterarb_line_class  */
		0x418800, /* gr_pri_gpcs_setup_debug           */
		0x418e00, /* gr_pri_gpcs_swdx_config           */
		0x418e40, /* gr_pri_gpcs_swdx_tc_bundle_ctrl   */
		0x418e44, /* gr_pri_gpcs_swdx_tc_bundle_ctrl   */
		0x418e48, /* gr_pri_gpcs_swdx_tc_bundle_ctrl   */
		0x418e4c, /* gr_pri_gpcs_swdx_tc_bundle_ctrl   */
		0x418e50, /* gr_pri_gpcs_swdx_tc_bundle_ctrl   */
		0x418e58, /* gr_pri_gpcs_swdx_tc_bundle_addr   */
		0x418e5c, /* gr_pri_gpcs_swdx_tc_bundle_addr   */
		0x418e60, /* gr_pri_gpcs_swdx_tc_bundle_addr   */
		0x418e64, /* gr_pri_gpcs_swdx_tc_bundle_addr   */
		0x418e68, /* gr_pri_gpcs_swdx_tc_bundle_addr   */
		0x418e6c, /* gr_pri_gpcs_swdx_tc_bundle_addr   */
		0x418e70, /* gr_pri_gpcs_swdx_tc_bundle_addr   */
		0x418e74, /* gr_pri_gpcs_swdx_tc_bundle_addr   */
		0x418e78, /* gr_pri_gpcs_swdx_tc_bundle_addr   */
		0x418e7c, /* gr_pri_gpcs_swdx_tc_bundle_addr   */
		0x418e80, /* gr_pri_gpcs_swdx_tc_bundle_addr   */
		0x418e84, /* gr_pri_gpcs_swdx_tc_bundle_addr   */
		0x418e88, /* gr_pri_gpcs_swdx_tc_bundle_addr   */
		0x418e8c, /* gr_pri_gpcs_swdx_tc_bundle_addr   */
		0x418e90, /* gr_pri_gpcs_swdx_tc_bundle_addr   */
		0x418e94, /* gr_pri_gpcs_swdx_tc_bundle_addr   */
		0x419864, /* gr_pri_gpcs_tpcs_pe_l2_evict_policy */
		0x419a04, /* gr_pri_gpcs_tpcs_tex_lod_dbg      */
		0x419a08, /* gr_pri_gpcs_tpcs_tex_samp_dbg     */
		0x419e10, /* gr_pri_gpcs_tpcs_sm_dbgr_control0 */
		0x419f78, /* gr_pri_gpcs_tpcs_sm_disp_ctrl     */
	};
	size_t array_size;

	*whitelist = wl_addr_gm20b;
	array_size = ARRAY_SIZE(wl_addr_gm20b);
	*num_entries = (int)array_size;
}

void gm20b_gr_init_sm_id_numbering(struct gk20a *g, u32 gpc, u32 tpc, u32 smid,
				   struct nvgpu_gr_config *gr_config)
{
	u32 gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_GPC_STRIDE);
	u32 tpc_in_gpc_stride = nvgpu_get_litter_value(g,
						GPU_LIT_TPC_IN_GPC_STRIDE);
	u32 gpc_offset = gpc_stride * gpc;
	u32 tpc_offset = tpc_in_gpc_stride * tpc;

	nvgpu_writel(g, gr_gpc0_tpc0_sm_cfg_r() + gpc_offset + tpc_offset,
			gr_gpc0_tpc0_sm_cfg_sm_id_f(smid));
	nvgpu_writel(g, gr_gpc0_gpm_pd_sm_id_r(tpc) + gpc_offset,
			gr_gpc0_gpm_pd_sm_id_id_f(smid));
	nvgpu_writel(g, gr_gpc0_tpc0_pe_cfg_smid_r() + gpc_offset + tpc_offset,
			gr_gpc0_tpc0_pe_cfg_smid_value_f(smid));
}

u32 gm20b_gr_init_get_sm_id_size(void)
{
	return gr_cwd_sm_id__size_1_v();
}

int gm20b_gr_init_sm_id_config(struct gk20a *g, u32 *tpc_sm_id,
			       struct nvgpu_gr_config *gr_config)
{
	u32 i, j;
	u32 tpc_index, gpc_index;

	/* Each NV_PGRAPH_PRI_CWD_GPC_TPC_ID can store 4 TPCs.*/
	for (i = 0U;
	     i <= ((nvgpu_gr_config_get_tpc_count(gr_config) - 1U) / 4U);
	     i++) {
		u32 reg = 0;
		u32 bit_stride = gr_cwd_gpc_tpc_id_gpc0_s() +
				 gr_cwd_gpc_tpc_id_tpc0_s();

		for (j = 0U; j < 4U; j++) {
			u32 sm_id = (i * 4U) + j;
			u32 bits;
			struct sm_info *sm_info;

			if (sm_id >=
				nvgpu_gr_config_get_tpc_count(gr_config)) {
				break;
			}
			sm_info =
				nvgpu_gr_config_get_sm_info(gr_config, sm_id);
			gpc_index =
				nvgpu_gr_config_get_sm_info_gpc_index(sm_info);
			tpc_index =
				nvgpu_gr_config_get_sm_info_tpc_index(sm_info);

			bits = gr_cwd_gpc_tpc_id_gpc0_f(gpc_index) |
			       gr_cwd_gpc_tpc_id_tpc0_f(tpc_index);
			reg |= bits << (j * bit_stride);

			tpc_sm_id[gpc_index] |=
					(sm_id << tpc_index * bit_stride);
		}
		nvgpu_writel(g, gr_cwd_gpc_tpc_id_r(i), reg);
	}

	for (i = 0; i < gr_cwd_sm_id__size_1_v(); i++) {
		nvgpu_writel(g, gr_cwd_sm_id_r(i), tpc_sm_id[i]);
	}

	return 0;
}

void gm20b_gr_init_tpc_mask(struct gk20a *g, u32 gpc_index, u32 pes_tpc_mask)
{
	nvgpu_writel(g, gr_fe_tpc_fs_r(), pes_tpc_mask);
}

int gm20b_gr_init_rop_mapping(struct gk20a *g,
			      struct nvgpu_gr_config *gr_config)
{
	u32 norm_entries, norm_shift;
	u32 coeff5_mod, coeff6_mod, coeff7_mod, coeff8_mod;
	u32 coeff9_mod, coeff10_mod, coeff11_mod;
	u32 map0, map1, map2, map3, map4, map5;
	u32 tpc_cnt;

	if (nvgpu_gr_config_get_map_tiles(gr_config) == NULL) {
		return -1;
	}

	nvgpu_log_fn(g, " ");

	tpc_cnt = nvgpu_gr_config_get_tpc_count(gr_config);

	nvgpu_writel(g, gr_crstr_map_table_cfg_r(),
		     gr_crstr_map_table_cfg_row_offset_f(
			nvgpu_gr_config_get_map_row_offset(gr_config)) |
		     gr_crstr_map_table_cfg_num_entries_f(tpc_cnt));

	map0 =  gr_crstr_gpc_map0_tile0_f(
			nvgpu_gr_config_get_map_tile_count(gr_config, 0)) |
		gr_crstr_gpc_map0_tile1_f(
			nvgpu_gr_config_get_map_tile_count(gr_config, 1)) |
		gr_crstr_gpc_map0_tile2_f(
			nvgpu_gr_config_get_map_tile_count(gr_config, 2)) |
		gr_crstr_gpc_map0_tile3_f(
			nvgpu_gr_config_get_map_tile_count(gr_config, 3)) |
		gr_crstr_gpc_map0_tile4_f(
			nvgpu_gr_config_get_map_tile_count(gr_config, 4)) |
		gr_crstr_gpc_map0_tile5_f(
			nvgpu_gr_config_get_map_tile_count(gr_config, 5));

	map1 =  gr_crstr_gpc_map1_tile6_f(
			nvgpu_gr_config_get_map_tile_count(gr_config, 6)) |
		gr_crstr_gpc_map1_tile7_f(
			nvgpu_gr_config_get_map_tile_count(gr_config, 7)) |
		gr_crstr_gpc_map1_tile8_f(
			nvgpu_gr_config_get_map_tile_count(gr_config, 8)) |
		gr_crstr_gpc_map1_tile9_f(
			nvgpu_gr_config_get_map_tile_count(gr_config, 9)) |
		gr_crstr_gpc_map1_tile10_f(
			nvgpu_gr_config_get_map_tile_count(gr_config, 10)) |
		gr_crstr_gpc_map1_tile11_f(
			nvgpu_gr_config_get_map_tile_count(gr_config, 11));

	map2 =  gr_crstr_gpc_map2_tile12_f(
			nvgpu_gr_config_get_map_tile_count(gr_config, 12)) |
		gr_crstr_gpc_map2_tile13_f(
			nvgpu_gr_config_get_map_tile_count(gr_config, 13)) |
		gr_crstr_gpc_map2_tile14_f(
			nvgpu_gr_config_get_map_tile_count(gr_config, 14)) |
		gr_crstr_gpc_map2_tile15_f(
			nvgpu_gr_config_get_map_tile_count(gr_config, 15)) |
		gr_crstr_gpc_map2_tile16_f(
			nvgpu_gr_config_get_map_tile_count(gr_config, 16)) |
		gr_crstr_gpc_map2_tile17_f(
			nvgpu_gr_config_get_map_tile_count(gr_config, 17));

	map3 =  gr_crstr_gpc_map3_tile18_f(
			nvgpu_gr_config_get_map_tile_count(gr_config, 18)) |
		gr_crstr_gpc_map3_tile19_f(
			nvgpu_gr_config_get_map_tile_count(gr_config, 19)) |
		gr_crstr_gpc_map3_tile20_f(
			nvgpu_gr_config_get_map_tile_count(gr_config, 20)) |
		gr_crstr_gpc_map3_tile21_f(
			nvgpu_gr_config_get_map_tile_count(gr_config, 21)) |
		gr_crstr_gpc_map3_tile22_f(
			nvgpu_gr_config_get_map_tile_count(gr_config, 22)) |
		gr_crstr_gpc_map3_tile23_f(
			nvgpu_gr_config_get_map_tile_count(gr_config, 23));

	map4 =  gr_crstr_gpc_map4_tile24_f(
			nvgpu_gr_config_get_map_tile_count(gr_config, 24)) |
		gr_crstr_gpc_map4_tile25_f(
			nvgpu_gr_config_get_map_tile_count(gr_config, 25)) |
		gr_crstr_gpc_map4_tile26_f(
			nvgpu_gr_config_get_map_tile_count(gr_config, 26)) |
		gr_crstr_gpc_map4_tile27_f(
			nvgpu_gr_config_get_map_tile_count(gr_config, 27)) |
		gr_crstr_gpc_map4_tile28_f(
			nvgpu_gr_config_get_map_tile_count(gr_config, 28)) |
		gr_crstr_gpc_map4_tile29_f(
			nvgpu_gr_config_get_map_tile_count(gr_config, 29));

	map5 =  gr_crstr_gpc_map5_tile30_f(
			nvgpu_gr_config_get_map_tile_count(gr_config, 30)) |
		gr_crstr_gpc_map5_tile31_f(
			nvgpu_gr_config_get_map_tile_count(gr_config, 31)) |
		gr_crstr_gpc_map5_tile32_f(0) |
		gr_crstr_gpc_map5_tile33_f(0) |
		gr_crstr_gpc_map5_tile34_f(0) |
		gr_crstr_gpc_map5_tile35_f(0);

	nvgpu_writel(g, gr_crstr_gpc_map0_r(), map0);
	nvgpu_writel(g, gr_crstr_gpc_map1_r(), map1);
	nvgpu_writel(g, gr_crstr_gpc_map2_r(), map2);
	nvgpu_writel(g, gr_crstr_gpc_map3_r(), map3);
	nvgpu_writel(g, gr_crstr_gpc_map4_r(), map4);
	nvgpu_writel(g, gr_crstr_gpc_map5_r(), map5);

	switch (tpc_cnt) {
	case 1:
		norm_shift = 4;
		break;
	case 2:
	case 3:
		norm_shift = 3;
		break;
	case 4:
	case 5:
	case 6:
	case 7:
		norm_shift = 2;
		break;
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		norm_shift = 1;
		break;
	default:
		norm_shift = 0;
		break;
	}

	norm_entries = tpc_cnt << norm_shift;
	coeff5_mod = BIT32(5) % norm_entries;
	coeff6_mod = BIT32(6) % norm_entries;
	coeff7_mod = BIT32(7) % norm_entries;
	coeff8_mod = BIT32(8) % norm_entries;
	coeff9_mod = BIT32(9) % norm_entries;
	coeff10_mod = BIT32(10) % norm_entries;
	coeff11_mod = BIT32(11) % norm_entries;

	nvgpu_writel(g, gr_ppcs_wwdx_map_table_cfg_r(),
	 gr_ppcs_wwdx_map_table_cfg_row_offset_f(
	  nvgpu_gr_config_get_map_row_offset(gr_config)) |
	 gr_ppcs_wwdx_map_table_cfg_normalized_num_entries_f(norm_entries) |
	 gr_ppcs_wwdx_map_table_cfg_normalized_shift_value_f(norm_shift) |
	 gr_ppcs_wwdx_map_table_cfg_coeff5_mod_value_f(coeff5_mod) |
	 gr_ppcs_wwdx_map_table_cfg_num_entries_f(tpc_cnt));

	nvgpu_writel(g, gr_ppcs_wwdx_map_table_cfg2_r(),
	 gr_ppcs_wwdx_map_table_cfg2_coeff6_mod_value_f(coeff6_mod) |
	 gr_ppcs_wwdx_map_table_cfg2_coeff7_mod_value_f(coeff7_mod) |
	 gr_ppcs_wwdx_map_table_cfg2_coeff8_mod_value_f(coeff8_mod) |
	 gr_ppcs_wwdx_map_table_cfg2_coeff9_mod_value_f(coeff9_mod) |
	 gr_ppcs_wwdx_map_table_cfg2_coeff10_mod_value_f(coeff10_mod) |
	 gr_ppcs_wwdx_map_table_cfg2_coeff11_mod_value_f(coeff11_mod));

	nvgpu_writel(g, gr_ppcs_wwdx_map_gpc_map0_r(), map0);
	nvgpu_writel(g, gr_ppcs_wwdx_map_gpc_map1_r(), map1);
	nvgpu_writel(g, gr_ppcs_wwdx_map_gpc_map2_r(), map2);
	nvgpu_writel(g, gr_ppcs_wwdx_map_gpc_map3_r(), map3);
	nvgpu_writel(g, gr_ppcs_wwdx_map_gpc_map4_r(), map4);
	nvgpu_writel(g, gr_ppcs_wwdx_map_gpc_map5_r(), map5);

	nvgpu_writel(g, gr_rstr2d_map_table_cfg_r(),
		gr_rstr2d_map_table_cfg_row_offset_f(
		 nvgpu_gr_config_get_map_row_offset(gr_config)) |
		 gr_rstr2d_map_table_cfg_num_entries_f(tpc_cnt));

	nvgpu_writel(g, gr_rstr2d_gpc_map0_r(), map0);
	nvgpu_writel(g, gr_rstr2d_gpc_map1_r(), map1);
	nvgpu_writel(g, gr_rstr2d_gpc_map2_r(), map2);
	nvgpu_writel(g, gr_rstr2d_gpc_map3_r(), map3);
	nvgpu_writel(g, gr_rstr2d_gpc_map4_r(), map4);
	nvgpu_writel(g, gr_rstr2d_gpc_map5_r(), map5);

	return 0;
}

int gm20b_gr_init_fs_state(struct gk20a *g)
{
	int err = 0;

	nvgpu_log_fn(g, " ");

	nvgpu_writel(g, gr_bes_zrop_settings_r(),
		     gr_bes_zrop_settings_num_active_ltcs_f(
			nvgpu_ltc_get_ltc_count(g)));
	nvgpu_writel(g, gr_bes_crop_settings_r(),
		     gr_bes_crop_settings_num_active_ltcs_f(
			nvgpu_ltc_get_ltc_count(g)));

	nvgpu_writel(g, gr_bes_crop_debug3_r(),
		     gk20a_readl(g, gr_be0_crop_debug3_r()) |
		     gr_bes_crop_debug3_comp_vdc_4to2_disable_m());

	return err;
}

void gm20b_gr_init_pd_tpc_per_gpc(struct gk20a *g,
				  struct nvgpu_gr_config *gr_config)
{
	u32 reg_index;
	u32 tpc_per_gpc;
	u32 gpc_id = 0;

	for (reg_index = 0U, gpc_id = 0U;
	     reg_index < gr_pd_num_tpc_per_gpc__size_1_v();
	     reg_index++, gpc_id += 8U) {

		tpc_per_gpc =
		 gr_pd_num_tpc_per_gpc_count0_f(
		  nvgpu_gr_config_get_gpc_tpc_count(gr_config, gpc_id + 0U)) |
		 gr_pd_num_tpc_per_gpc_count1_f(
		  nvgpu_gr_config_get_gpc_tpc_count(gr_config, gpc_id + 1U)) |
		 gr_pd_num_tpc_per_gpc_count2_f(
		  nvgpu_gr_config_get_gpc_tpc_count(gr_config, gpc_id + 2U)) |
		 gr_pd_num_tpc_per_gpc_count3_f(
		  nvgpu_gr_config_get_gpc_tpc_count(gr_config, gpc_id + 3U)) |
		 gr_pd_num_tpc_per_gpc_count4_f(
		  nvgpu_gr_config_get_gpc_tpc_count(gr_config, gpc_id + 4U)) |
		 gr_pd_num_tpc_per_gpc_count5_f(
		  nvgpu_gr_config_get_gpc_tpc_count(gr_config, gpc_id + 5U)) |
		 gr_pd_num_tpc_per_gpc_count6_f(
		  nvgpu_gr_config_get_gpc_tpc_count(gr_config, gpc_id + 6U)) |
		 gr_pd_num_tpc_per_gpc_count7_f(
		  nvgpu_gr_config_get_gpc_tpc_count(gr_config, gpc_id + 7U));

		nvgpu_writel(g, gr_pd_num_tpc_per_gpc_r(reg_index), tpc_per_gpc);
		nvgpu_writel(g, gr_ds_num_tpc_per_gpc_r(reg_index), tpc_per_gpc);
	}
}

void gm20b_gr_init_pd_skip_table_gpc(struct gk20a *g,
				     struct nvgpu_gr_config *gr_config)
{
	u32 gpc_index;
	bool skip_mask;

	for (gpc_index = 0;
	     gpc_index < gr_pd_dist_skip_table__size_1_v() * 4U;
	     gpc_index += 4U) {
		skip_mask =
		 (gr_pd_dist_skip_table_gpc_4n0_mask_f(
		   nvgpu_gr_config_get_gpc_skip_mask(gr_config,
						     gpc_index)) != 0U) ||
		 (gr_pd_dist_skip_table_gpc_4n1_mask_f(
		   nvgpu_gr_config_get_gpc_skip_mask(gr_config,
						     gpc_index + 1U)) != 0U) ||
		 (gr_pd_dist_skip_table_gpc_4n2_mask_f(
		   nvgpu_gr_config_get_gpc_skip_mask(gr_config,
						     gpc_index + 2U)) != 0U) ||
		 (gr_pd_dist_skip_table_gpc_4n3_mask_f(
		   nvgpu_gr_config_get_gpc_skip_mask(gr_config,
						     gpc_index + 3U)) != 0U);

		nvgpu_writel(g, gr_pd_dist_skip_table_r(gpc_index/4U),
			     (u32)skip_mask);
	}
}

void gm20b_gr_init_cwd_gpcs_tpcs_num(struct gk20a *g,
				     u32 gpc_count, u32 tpc_count)
{
	nvgpu_writel(g, gr_cwd_fs_r(),
		     gr_cwd_fs_num_gpcs_f(gpc_count) |
		     gr_cwd_fs_num_tpcs_f(tpc_count));
}

int gm20b_gr_init_wait_idle(struct gk20a *g)
{
	u32 delay = POLL_DELAY_MIN_US;
	u32 gr_engine_id;
	int err = 0;
	bool ctxsw_active;
	bool gr_busy;
	bool ctx_status_invalid;
	struct nvgpu_engine_status_info engine_status;
	struct nvgpu_timeout timeout;

	nvgpu_log_fn(g, " ");

	gr_engine_id = nvgpu_engine_get_gr_id(g);

	err = nvgpu_timeout_init(g, &timeout, nvgpu_get_poll_timeout(g),
				 NVGPU_TIMER_CPU_TIMER);
	if (err != 0) {
		return err;
	}

	do {
		/*
		 * fmodel: host gets fifo_engine_status(gr) from gr
		 * only when gr_status is read
		 */
		(void) nvgpu_readl(g, gr_status_r());

		g->ops.engine_status.read_engine_status_info(g, gr_engine_id,
							     &engine_status);

		ctxsw_active = engine_status.ctxsw_in_progress;

		ctx_status_invalid = nvgpu_engine_status_is_ctxsw_invalid(
							&engine_status);

		gr_busy = (nvgpu_readl(g, gr_engine_status_r()) &
				gr_engine_status_value_busy_f()) != 0U;

		if (ctx_status_invalid || (!gr_busy && !ctxsw_active)) {
			nvgpu_log_fn(g, "done");
			return 0;
		}

		nvgpu_usleep_range(delay, delay * 2U);
		delay = min_t(u32, delay << 1, POLL_DELAY_MAX_US);

	} while (nvgpu_timeout_expired(&timeout) == 0);

	nvgpu_err(g, "timeout, ctxsw busy : %d, gr busy : %d",
		  ctxsw_active, gr_busy);

	return -EAGAIN;
}

int gm20b_gr_init_wait_fe_idle(struct gk20a *g)
{
	u32 val;
	u32 delay = POLL_DELAY_MIN_US;
	struct nvgpu_timeout timeout;
	int err = 0;

	if (nvgpu_is_enabled(g, NVGPU_IS_FMODEL)) {
		return 0;
	}

	nvgpu_log_fn(g, " ");

	err = nvgpu_timeout_init(g, &timeout, nvgpu_get_poll_timeout(g),
				 NVGPU_TIMER_CPU_TIMER);
	if (err != 0) {
		return err;
	}

	do {
		val = nvgpu_readl(g, gr_status_r());

		if (gr_status_fe_method_lower_v(val) == 0U) {
			nvgpu_log_fn(g, "done");
			return 0;
		}

		nvgpu_usleep_range(delay, delay * 2U);
		delay = min_t(u32, delay << 1, POLL_DELAY_MAX_US);
	} while (nvgpu_timeout_expired(&timeout) == 0);

	nvgpu_err(g, "timeout, fe busy : %x", val);

	return -EAGAIN;
}

int gm20b_gr_init_fe_pwr_mode_force_on(struct gk20a *g, bool force_on)
{
	struct nvgpu_timeout timeout;
	int ret = 0;
	u32 reg_val;

	if (nvgpu_is_enabled(g, NVGPU_IS_FMODEL)) {
		return 0;
	}

	if (force_on) {
		reg_val = gr_fe_pwr_mode_req_send_f() |
			  gr_fe_pwr_mode_mode_force_on_f();
	} else {
		reg_val = gr_fe_pwr_mode_req_send_f() |
			  gr_fe_pwr_mode_mode_auto_f();
	}

	ret = nvgpu_timeout_init(g, &timeout,
			   FE_PWR_MODE_TIMEOUT_MAX_US /
				FE_PWR_MODE_TIMEOUT_DEFAULT_US,
			   NVGPU_TIMER_RETRY_TIMER);
	if (ret != 0) {
		return ret;
	}

	nvgpu_writel(g, gr_fe_pwr_mode_r(), reg_val);

	ret = -ETIMEDOUT;

	do {
		u32 req = gr_fe_pwr_mode_req_v(
				nvgpu_readl(g, gr_fe_pwr_mode_r()));
		if (req == gr_fe_pwr_mode_req_done_v()) {
			ret = 0;
			break;
		}

		nvgpu_udelay(FE_PWR_MODE_TIMEOUT_DEFAULT_US);
	} while (nvgpu_timeout_expired_msg(&timeout,
				"timeout setting FE mode %u", force_on) == 0);

	return ret;
}

void gm20b_gr_init_override_context_reset(struct gk20a *g)
{
	nvgpu_writel(g, gr_fecs_ctxsw_reset_ctl_r(),
			gr_fecs_ctxsw_reset_ctl_sys_halt_disabled_f() |
			gr_fecs_ctxsw_reset_ctl_gpc_halt_disabled_f() |
			gr_fecs_ctxsw_reset_ctl_be_halt_disabled_f() |
			gr_fecs_ctxsw_reset_ctl_sys_engine_reset_disabled_f() |
			gr_fecs_ctxsw_reset_ctl_gpc_engine_reset_disabled_f() |
			gr_fecs_ctxsw_reset_ctl_be_engine_reset_disabled_f() |
			gr_fecs_ctxsw_reset_ctl_sys_context_reset_enabled_f() |
			gr_fecs_ctxsw_reset_ctl_gpc_context_reset_enabled_f() |
			gr_fecs_ctxsw_reset_ctl_be_context_reset_enabled_f());

	nvgpu_udelay(FECS_CTXSW_RESET_DELAY_US);
	(void) nvgpu_readl(g, gr_fecs_ctxsw_reset_ctl_r());

	/* Deassert reset */
	nvgpu_writel(g, gr_fecs_ctxsw_reset_ctl_r(),
			gr_fecs_ctxsw_reset_ctl_sys_halt_disabled_f() |
			gr_fecs_ctxsw_reset_ctl_gpc_halt_disabled_f() |
			gr_fecs_ctxsw_reset_ctl_be_halt_disabled_f() |
			gr_fecs_ctxsw_reset_ctl_sys_engine_reset_disabled_f() |
			gr_fecs_ctxsw_reset_ctl_gpc_engine_reset_disabled_f() |
			gr_fecs_ctxsw_reset_ctl_be_engine_reset_disabled_f() |
			gr_fecs_ctxsw_reset_ctl_sys_context_reset_disabled_f() |
			gr_fecs_ctxsw_reset_ctl_gpc_context_reset_disabled_f() |
			gr_fecs_ctxsw_reset_ctl_be_context_reset_disabled_f());

	nvgpu_udelay(FECS_CTXSW_RESET_DELAY_US);
	(void) nvgpu_readl(g, gr_fecs_ctxsw_reset_ctl_r());
}

void gm20b_gr_init_fe_go_idle_timeout(struct gk20a *g, bool enable)
{
	if (enable) {
		nvgpu_writel(g, gr_fe_go_idle_timeout_r(),
			gr_fe_go_idle_timeout_count_prod_f());
	} else {
		nvgpu_writel(g, gr_fe_go_idle_timeout_r(),
			gr_fe_go_idle_timeout_count_disabled_f());
	}
}

void gm20b_gr_init_pipe_mode_override(struct gk20a *g, bool enable)
{
	if (enable) {
		nvgpu_writel(g, gr_pipe_bundle_config_r(),
			gr_pipe_bundle_config_override_pipe_mode_enabled_f());
	} else {
		nvgpu_writel(g, gr_pipe_bundle_config_r(),
			gr_pipe_bundle_config_override_pipe_mode_disabled_f());
	}
}

void gm20b_gr_init_load_method_init(struct gk20a *g,
		struct netlist_av_list *sw_method_init)
{
	u32 i;
	u32 last_method_data = 0U;

	if (sw_method_init->count != 0U) {
		nvgpu_writel(g, gr_pri_mme_shadow_raw_data_r(),
			     sw_method_init->l[0U].value);
		nvgpu_writel(g, gr_pri_mme_shadow_raw_index_r(),
			     gr_pri_mme_shadow_raw_index_write_trigger_f() |
			     sw_method_init->l[0U].addr);
		last_method_data = sw_method_init->l[0U].value;
	}

	for (i = 1U; i < sw_method_init->count; i++) {
		if (sw_method_init->l[i].value != last_method_data) {
			nvgpu_writel(g, gr_pri_mme_shadow_raw_data_r(),
				sw_method_init->l[i].value);
			last_method_data = sw_method_init->l[i].value;
		}
		nvgpu_writel(g, gr_pri_mme_shadow_raw_index_r(),
			gr_pri_mme_shadow_raw_index_write_trigger_f() |
			sw_method_init->l[i].addr);
	}
}

int gm20b_gr_init_load_sw_bundle_init(struct gk20a *g,
		struct netlist_av_list *sw_bundle_init)
{
	u32 i;
	int err = 0;
	u32 last_bundle_data = 0U;

	for (i = 0U; i < sw_bundle_init->count; i++) {
		if (i == 0U || last_bundle_data != sw_bundle_init->l[i].value) {
			nvgpu_writel(g, gr_pipe_bundle_data_r(),
				sw_bundle_init->l[i].value);
			last_bundle_data = sw_bundle_init->l[i].value;
		}

		nvgpu_writel(g, gr_pipe_bundle_address_r(),
			     sw_bundle_init->l[i].addr);

		if (gr_pipe_bundle_address_value_v(sw_bundle_init->l[i].addr) ==
		    GR_GO_IDLE_BUNDLE) {
			err = g->ops.gr.init.wait_idle(g);
			if (err != 0) {
				return err;
			}
		}

		err = g->ops.gr.init.wait_fe_idle(g);
		if (err != 0) {
			return err;
		}
	}

	return err;
}

void gm20b_gr_init_commit_global_timeslice(struct gk20a *g)
{
	u32 gpm_pd_cfg;
	u32 pd_ab_dist_cfg0;
	u32 ds_debug;
	u32 mpc_vtg_debug;
	u32 pe_vaf;
	u32 pe_vsc_vpc;

	nvgpu_log_fn(g, " ");

	gpm_pd_cfg = nvgpu_readl(g, gr_gpcs_gpm_pd_cfg_r());
	pd_ab_dist_cfg0 = nvgpu_readl(g, gr_pd_ab_dist_cfg0_r());
	ds_debug = nvgpu_readl(g, gr_ds_debug_r());
	mpc_vtg_debug = nvgpu_readl(g, gr_gpcs_tpcs_mpc_vtg_debug_r());

	pe_vaf = nvgpu_readl(g, gr_gpcs_tpcs_pe_vaf_r());
	pe_vsc_vpc = nvgpu_readl(g, gr_gpcs_tpcs_pes_vsc_vpc_r());

	gpm_pd_cfg = gr_gpcs_gpm_pd_cfg_timeslice_mode_enable_f() | gpm_pd_cfg;
	pe_vaf = gr_gpcs_tpcs_pe_vaf_fast_mode_switch_true_f() | pe_vaf;
	pe_vsc_vpc = gr_gpcs_tpcs_pes_vsc_vpc_fast_mode_switch_true_f() |
		     pe_vsc_vpc;
	pd_ab_dist_cfg0 = gr_pd_ab_dist_cfg0_timeslice_enable_en_f() |
			  pd_ab_dist_cfg0;
	ds_debug = gr_ds_debug_timeslice_mode_enable_f() | ds_debug;
	mpc_vtg_debug = gr_gpcs_tpcs_mpc_vtg_debug_timeslice_mode_enabled_f() |
			mpc_vtg_debug;

	nvgpu_gr_ctx_patch_write(g, NULL, gr_gpcs_gpm_pd_cfg_r(), gpm_pd_cfg,
		false);
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

u32 gm20b_gr_init_get_bundle_cb_default_size(struct gk20a *g)
{
	return gr_scc_bundle_cb_size_div_256b__prod_v();
}

u32 gm20b_gr_init_get_min_gpm_fifo_depth(struct gk20a *g)
{
	return gr_pd_ab_dist_cfg2_state_limit_min_gpm_fifo_depths_v();
}

u32 gm20b_gr_init_get_bundle_cb_token_limit(struct gk20a *g)
{
	return gr_pd_ab_dist_cfg2_token_limit_init_v();
}

u32 gm20b_gr_init_get_attrib_cb_default_size(struct gk20a *g)
{
	return gr_gpc0_ppc0_cbm_beta_cb_size_v_default_v();
}

u32 gm20b_gr_init_get_alpha_cb_default_size(struct gk20a *g)
{
	return gr_gpc0_ppc0_cbm_alpha_cb_size_v_default_v();
}

u32 gm20b_gr_init_get_attrib_cb_size(struct gk20a *g, u32 tpc_count)
{
	return g->ops.gr.init.get_attrib_cb_default_size(g)
		+ (g->ops.gr.init.get_attrib_cb_default_size(g) >> 1);
}

u32 gm20b_gr_init_get_alpha_cb_size(struct gk20a *g, u32 tpc_count)
{
	return g->ops.gr.init.get_alpha_cb_default_size(g)
		+ (g->ops.gr.init.get_alpha_cb_default_size(g) >> 1);
}

u32 gm20b_gr_init_get_global_attr_cb_size(struct gk20a *g, u32 tpc_count,
	u32 max_tpc)
{
	u32 size;

	size = g->ops.gr.init.get_attrib_cb_size(g, tpc_count) *
		gr_gpc0_ppc0_cbm_beta_cb_size_v_granularity_v() * max_tpc;

	size += g->ops.gr.init.get_alpha_cb_size(g, tpc_count) *
		gr_gpc0_ppc0_cbm_alpha_cb_size_v_granularity_v() * max_tpc;

	return size;
}

u32 gm20b_gr_init_get_global_ctx_cb_buffer_size(struct gk20a *g)
{
	return g->ops.gr.init.get_bundle_cb_default_size(g) *
		gr_scc_bundle_cb_size_div_256b_byte_granularity_v();
}

u32 gm20b_gr_init_get_global_ctx_pagepool_buffer_size(struct gk20a *g)
{
	return g->ops.gr.init.pagepool_default_size(g) *
		gr_scc_pagepool_total_pages_byte_granularity_v();
}

void gm20b_gr_init_commit_global_bundle_cb(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx, u64 addr, u64 size, bool patch)
{
	u32 data;
	u32 bundle_cb_token_limit = g->ops.gr.init.get_bundle_cb_token_limit(g);

	addr = addr >> U64(gr_scc_bundle_cb_base_addr_39_8_align_bits_v());

	nvgpu_log_info(g, "bundle cb addr : 0x%016llx, size : %llu",
		addr, size);

	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_scc_bundle_cb_base_r(),
		gr_scc_bundle_cb_base_addr_39_8_f(addr), patch);

	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_scc_bundle_cb_size_r(),
		gr_scc_bundle_cb_size_div_256b_f(size) |
		gr_scc_bundle_cb_size_valid_true_f(), patch);

	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_gpcs_swdx_bundle_cb_base_r(),
		gr_gpcs_swdx_bundle_cb_base_addr_39_8_f(addr), patch);

	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_gpcs_swdx_bundle_cb_size_r(),
		gr_gpcs_swdx_bundle_cb_size_div_256b_f(size) |
		gr_gpcs_swdx_bundle_cb_size_valid_true_f(), patch);

	/* data for state_limit */
	data = (g->ops.gr.init.get_bundle_cb_default_size(g) *
		gr_scc_bundle_cb_size_div_256b_byte_granularity_v()) /
		gr_pd_ab_dist_cfg2_state_limit_scc_bundle_granularity_v();

	data = min_t(u32, data, g->ops.gr.init.get_min_gpm_fifo_depth(g));

	nvgpu_log_info(g, "bundle cb token limit : %d, state limit : %d",
		bundle_cb_token_limit, data);

	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_pd_ab_dist_cfg2_r(),
		gr_pd_ab_dist_cfg2_token_limit_f(bundle_cb_token_limit) |
		gr_pd_ab_dist_cfg2_state_limit_f(data), patch);
}

u32 gm20b_gr_init_pagepool_default_size(struct gk20a *g)
{
	return gr_scc_pagepool_total_pages_hwmax_value_v();
}

void gm20b_gr_init_commit_global_pagepool(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx, u64 addr, u32 size, bool patch,
	bool global_ctx)
{
	addr = (u64_lo32(addr) >>
			U64(gr_scc_pagepool_base_addr_39_8_align_bits_v()) |
		(u64_hi32(addr) <<
			 U64(32U - gr_scc_pagepool_base_addr_39_8_align_bits_v())));

	if (global_ctx) {
		size = size / gr_scc_pagepool_total_pages_byte_granularity_v();
	}

	if (size == g->ops.gr.init.pagepool_default_size(g)) {
		size = gr_scc_pagepool_total_pages_hwmax_v();
	}

	nvgpu_assert(u64_hi32(addr) == 0U);
	nvgpu_log_info(g, "pagepool buffer addr : 0x%016llx, size : %d",
		addr, size);

	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_scc_pagepool_base_r(),
		gr_scc_pagepool_base_addr_39_8_f((u32)addr), patch);

	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_scc_pagepool_r(),
		gr_scc_pagepool_total_pages_f(size) |
		gr_scc_pagepool_valid_true_f(), patch);

	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_gpcs_gcc_pagepool_base_r(),
		gr_gpcs_gcc_pagepool_base_addr_39_8_f((u32)addr), patch);

	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_gpcs_gcc_pagepool_r(),
		gr_gpcs_gcc_pagepool_total_pages_f(size), patch);

	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_pd_pagepool_r(),
		gr_pd_pagepool_total_pages_f(size) |
		gr_pd_pagepool_valid_true_f(), patch);

	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_gpcs_swdx_rm_pagepool_r(),
		gr_gpcs_swdx_rm_pagepool_total_pages_f(size) |
		gr_gpcs_swdx_rm_pagepool_valid_true_f(), patch);
}

void gm20b_gr_init_commit_global_attrib_cb(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx, u32 tpc_count, u32 max_tpc, u64 addr,
	bool patch)
{
	addr = (u64_lo32(addr) >>
			gr_gpcs_setup_attrib_cb_base_addr_39_12_align_bits_v()) |
		(u64_hi32(addr) <<
			(32U - gr_gpcs_setup_attrib_cb_base_addr_39_12_align_bits_v()));

	nvgpu_log_info(g, "attrib cb addr : 0x%016llx", addr);

	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_gpcs_setup_attrib_cb_base_r(),
		gr_gpcs_setup_attrib_cb_base_addr_39_12_f(addr) |
		gr_gpcs_setup_attrib_cb_base_valid_true_f(), patch);

	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_gpcs_tpcs_pe_pin_cb_global_base_addr_r(),
		gr_gpcs_tpcs_pe_pin_cb_global_base_addr_v_f(addr) |
		gr_gpcs_tpcs_pe_pin_cb_global_base_addr_valid_true_f(), patch);

	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_gpcs_tpcs_mpc_vtg_cb_global_base_addr_r(),
		gr_gpcs_tpcs_mpc_vtg_cb_global_base_addr_v_f(addr) |
		gr_gpcs_tpcs_mpc_vtg_cb_global_base_addr_valid_true_f(), patch);
}

void gm20b_gr_init_commit_global_cb_manager(struct gk20a *g,
	struct nvgpu_gr_config *config, struct nvgpu_gr_ctx *gr_ctx, bool patch)
{
	u32 attrib_offset_in_chunk = 0;
	u32 alpha_offset_in_chunk = 0;
	u32 pd_ab_max_output;
	u32 gpc_index, ppc_index;
	u32 cbm_cfg_size1, cbm_cfg_size2;
	u32 attrib_cb_default_size = g->ops.gr.init.get_attrib_cb_default_size(g);
	u32 alpha_cb_default_size = g->ops.gr.init.get_alpha_cb_default_size(g);
	u32 attrib_cb_size = g->ops.gr.init.get_attrib_cb_size(g,
		nvgpu_gr_config_get_tpc_count(config));
	u32 alpha_cb_size = g->ops.gr.init.get_alpha_cb_size(g,
		nvgpu_gr_config_get_tpc_count(config));
	u32 gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_GPC_STRIDE);
	u32 ppc_in_gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_PPC_IN_GPC_STRIDE);
	u32 num_pes_per_gpc = nvgpu_get_litter_value(g,
			GPU_LIT_NUM_PES_PER_GPC);

	nvgpu_log_fn(g, " ");

	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_ds_tga_constraintlogic_r(),
		gr_ds_tga_constraintlogic_beta_cbsize_f(attrib_cb_default_size) |
		gr_ds_tga_constraintlogic_alpha_cbsize_f(alpha_cb_default_size),
		patch);

	pd_ab_max_output = (alpha_cb_default_size *
		gr_gpc0_ppc0_cbm_beta_cb_size_v_granularity_v()) /
		gr_pd_ab_dist_cfg1_max_output_granularity_v();

	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_pd_ab_dist_cfg1_r(),
		gr_pd_ab_dist_cfg1_max_output_f(pd_ab_max_output) |
		gr_pd_ab_dist_cfg1_max_batches_init_f(), patch);

	alpha_offset_in_chunk = attrib_offset_in_chunk +
		nvgpu_gr_config_get_tpc_count(config) * attrib_cb_size;

	for (gpc_index = 0;
	     gpc_index < nvgpu_gr_config_get_gpc_count(config);
	     gpc_index++) {
		u32 temp = gpc_stride * gpc_index;
		u32 temp2 = num_pes_per_gpc * gpc_index;
		for (ppc_index = 0;
		     ppc_index < nvgpu_gr_config_get_gpc_ppc_count(config,
		     gpc_index);
		     ppc_index++) {
			cbm_cfg_size1 = attrib_cb_default_size *
				nvgpu_gr_config_get_pes_tpc_count(config,
					gpc_index, ppc_index);
			cbm_cfg_size2 = alpha_cb_default_size *
				nvgpu_gr_config_get_pes_tpc_count(config,
					gpc_index, ppc_index);

			nvgpu_gr_ctx_patch_write(g, gr_ctx,
				gr_gpc0_ppc0_cbm_beta_cb_size_r() + temp +
				ppc_in_gpc_stride * ppc_index,
				cbm_cfg_size1, patch);

			nvgpu_gr_ctx_patch_write(g, gr_ctx,
				gr_gpc0_ppc0_cbm_beta_cb_offset_r() + temp +
				ppc_in_gpc_stride * ppc_index,
				attrib_offset_in_chunk, patch);

			attrib_offset_in_chunk += attrib_cb_size *
				nvgpu_gr_config_get_pes_tpc_count(config,
					gpc_index, ppc_index);

			nvgpu_gr_ctx_patch_write(g, gr_ctx,
				gr_gpc0_ppc0_cbm_alpha_cb_size_r() + temp +
				ppc_in_gpc_stride * ppc_index,
				cbm_cfg_size2, patch);

			nvgpu_gr_ctx_patch_write(g, gr_ctx,
				gr_gpc0_ppc0_cbm_alpha_cb_offset_r() + temp +
				ppc_in_gpc_stride * ppc_index,
				alpha_offset_in_chunk, patch);

			alpha_offset_in_chunk += alpha_cb_size *
				nvgpu_gr_config_get_pes_tpc_count(config,
					gpc_index, ppc_index);

			nvgpu_gr_ctx_patch_write(g, gr_ctx,
				gr_gpcs_swdx_tc_beta_cb_size_r(ppc_index + temp2),
				gr_gpcs_swdx_tc_beta_cb_size_v_f(cbm_cfg_size1) |
				gr_gpcs_swdx_tc_beta_cb_size_div3_f(cbm_cfg_size1/3U),
				patch);
		}
	}
}

u32 gm20b_gr_init_get_patch_slots(struct gk20a *g,
	struct nvgpu_gr_config *config)
{
	return PATCH_CTX_SLOTS_PER_PAGE;
}

void gm20b_gr_init_detect_sm_arch(struct gk20a *g)
{
	u32 v = gk20a_readl(g, gr_gpc0_tpc0_sm_arch_r());

	g->params.sm_arch_spa_version =
		gr_gpc0_tpc0_sm_arch_spa_version_v(v);
	g->params.sm_arch_sm_version =
		gr_gpc0_tpc0_sm_arch_sm_version_v(v);
	g->params.sm_arch_warp_count =
		gr_gpc0_tpc0_sm_arch_warp_count_v(v);
}

void gm20b_gr_init_get_supported_preemption_modes(
	u32 *graphics_preemption_mode_flags, u32 *compute_preemption_mode_flags)
{
	*graphics_preemption_mode_flags = NVGPU_PREEMPTION_MODE_GRAPHICS_WFI;
	*compute_preemption_mode_flags = (NVGPU_PREEMPTION_MODE_COMPUTE_WFI |
					 NVGPU_PREEMPTION_MODE_COMPUTE_CTA);
}

void gm20b_gr_init_get_default_preemption_modes(
	u32 *default_graphics_preempt_mode, u32 *default_compute_preempt_mode)
{
	*default_graphics_preempt_mode = NVGPU_PREEMPTION_MODE_GRAPHICS_WFI;
	*default_compute_preempt_mode = NVGPU_PREEMPTION_MODE_COMPUTE_CTA;
}

