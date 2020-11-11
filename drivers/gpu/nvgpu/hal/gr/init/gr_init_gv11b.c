/*
 * Copyright (c) 2019-2020, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/log.h>
#include <nvgpu/bug.h>
#include <nvgpu/static_analysis.h>
#include <nvgpu/gr/ctx.h>
#include <nvgpu/ltc.h>
#include <nvgpu/netlist.h>

#include <nvgpu/gr/config.h>
#include <nvgpu/gr/gr_utils.h>

#include "gr_init_gm20b.h"
#include "gr_init_gv11b.h"

#include <nvgpu/hw/gv11b/hw_gr_gv11b.h>

/* ecc scrubbing will done in 1 pri read cycle,but for safety used 10 retries */
#define GR_ECC_SCRUBBING_TIMEOUT_MAX_US 1000U
#define GR_ECC_SCRUBBING_TIMEOUT_DEFAULT_US 10U

/*
 * Each gpc can have maximum 32 tpcs, so each tpc index need
 * 5 bits. Each map register(32bits) can hold 6 tpcs info.
 */
#define GR_TPCS_INFO_FOR_MAPREGISTER 6U

#define GFXP_WFI_TIMEOUT_COUNT_IN_USEC_DEFAULT 100U

#ifdef CONFIG_NVGPU_GRAPHICS
void gv11b_gr_init_rop_mapping(struct gk20a *g,
			      struct nvgpu_gr_config *gr_config)
{
	u32 map;
	u32 i, j = 1U;
	u32 base = 0U;
	u32 mapreg_num, offset, mapregs, tile_cnt, tpc_cnt;
	u32 num_gpcs = nvgpu_get_litter_value(g, GPU_LIT_NUM_GPCS);
	u32 num_tpc_per_gpc = nvgpu_get_litter_value(g,
				GPU_LIT_NUM_TPC_PER_GPC);
	u32 num_tpcs = nvgpu_safe_mult_u32(num_gpcs, num_tpc_per_gpc);

	nvgpu_log_fn(g, " ");

	if (nvgpu_is_enabled(g, NVGPU_SUPPORT_MIG)) {
		nvgpu_log_fn(g, " MIG is enabled, skipped rop mapping");
		return;
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

	for (mapreg_num = 0U; mapreg_num < mapregs; mapreg_num++) {
		map = 0U;
		offset = 0U;
		while ((offset < GR_TPCS_INFO_FOR_MAPREGISTER)
			&& (num_tpcs > 0U)) {
			tile_cnt = nvgpu_gr_config_get_map_tile_count(
						gr_config, base + offset);
			if (offset == 0U) {
				map = map | gr_crstr_gpc_map_tile0_f(tile_cnt);
			} else if (offset == 1U) {
				map = map | gr_crstr_gpc_map_tile1_f(tile_cnt);
			} else if (offset == 2U) {
				map = map | gr_crstr_gpc_map_tile2_f(tile_cnt);
			} else if (offset == 3U) {
				map = map | gr_crstr_gpc_map_tile3_f(tile_cnt);
			} else if (offset == 4U) {
				map = map | gr_crstr_gpc_map_tile4_f(tile_cnt);
			} else if (offset == 5U) {
				map = map | gr_crstr_gpc_map_tile5_f(tile_cnt);
			} else {
				nvgpu_err(g, "incorrect rop mapping %x",
					  offset);
			}
			num_tpcs--;
			offset++;
		}

		nvgpu_writel(g, gr_crstr_gpc_map_r(mapreg_num), map);
		nvgpu_writel(g, gr_ppcs_wwdx_map_gpc_map_r(mapreg_num), map);
		nvgpu_writel(g, gr_rstr2d_gpc_map_r(mapreg_num), map);

		base = nvgpu_safe_add_u32(base, GR_TPCS_INFO_FOR_MAPREGISTER);
	}

	nvgpu_writel(g, gr_ppcs_wwdx_map_table_cfg_r(),
		gr_ppcs_wwdx_map_table_cfg_row_offset_f(
			nvgpu_gr_config_get_map_row_offset(gr_config)) |
		gr_ppcs_wwdx_map_table_cfg_num_entries_f(
			nvgpu_gr_config_get_tpc_count(gr_config)));

	for (i = 0U; i < gr_ppcs_wwdx_map_table_cfg_coeff__size_1_v(); i++) {
		tpc_cnt = nvgpu_gr_config_get_tpc_count(gr_config);
		nvgpu_writel(g, gr_ppcs_wwdx_map_table_cfg_coeff_r(i),
			gr_ppcs_wwdx_map_table_cfg_coeff_0_mod_value_f(
			   (BIT32(j) % tpc_cnt)) |
			gr_ppcs_wwdx_map_table_cfg_coeff_1_mod_value_f(
			   (BIT32(nvgpu_safe_add_u32(j, 1U)) % tpc_cnt)) |
			gr_ppcs_wwdx_map_table_cfg_coeff_2_mod_value_f(
			   (BIT32(nvgpu_safe_add_u32(j, 2U)) % tpc_cnt)) |
			gr_ppcs_wwdx_map_table_cfg_coeff_3_mod_value_f(
			   (BIT32(nvgpu_safe_add_u32(j, 3U)) % tpc_cnt)));
			j = nvgpu_safe_add_u32(j, 4U);
	}

	nvgpu_writel(g, gr_rstr2d_map_table_cfg_r(),
		gr_rstr2d_map_table_cfg_row_offset_f(
			nvgpu_gr_config_get_map_row_offset(gr_config)) |
		gr_rstr2d_map_table_cfg_num_entries_f(
			nvgpu_gr_config_get_tpc_count(gr_config)));
}

u32 gv11b_gr_init_get_attrib_cb_gfxp_default_size(struct gk20a *g)
{
	return gr_gpc0_ppc0_cbm_beta_cb_size_v_gfxp_v();
}

u32 gv11b_gr_init_get_attrib_cb_gfxp_size(struct gk20a *g)
{
	return gr_gpc0_ppc0_cbm_beta_cb_size_v_gfxp_v();
}

u32 gv11b_gr_init_get_ctx_spill_size(struct gk20a *g)
{
	return  nvgpu_safe_mult_u32(
		  gr_gpc0_swdx_rm_spill_buffer_size_256b_default_v(),
		  gr_gpc0_swdx_rm_spill_buffer_size_256b_byte_granularity_v());
}

u32 gv11b_gr_init_get_ctx_betacb_size(struct gk20a *g)
{
	return nvgpu_safe_add_u32(
		g->ops.gr.init.get_attrib_cb_default_size(g),
		nvgpu_safe_sub_u32(
			gr_gpc0_ppc0_cbm_beta_cb_size_v_gfxp_v(),
			gr_gpc0_ppc0_cbm_beta_cb_size_v_default_v()));
}

void gv11b_gr_init_commit_ctxsw_spill(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx, u64 addr, u32 size, bool patch)
{

	if (nvgpu_is_enabled(g, NVGPU_SUPPORT_MIG)) {
		nvgpu_log_fn(g, " MIG is enabled, skipped commit ctxsw spill");
		return;
	}

	addr = addr >> gr_gpc0_swdx_rm_spill_buffer_addr_39_8_align_bits_v();

	size /=	gr_gpc0_swdx_rm_spill_buffer_size_256b_byte_granularity_v();

	nvgpu_assert(u64_hi32(addr) == 0U);
	nvgpu_gr_ctx_patch_write(g, gr_ctx,
			gr_gpc0_swdx_rm_spill_buffer_addr_r(),
			gr_gpc0_swdx_rm_spill_buffer_addr_39_8_f((u32)addr),
			patch);
	nvgpu_gr_ctx_patch_write(g, gr_ctx,
			gr_gpc0_swdx_rm_spill_buffer_size_r(),
			gr_gpc0_swdx_rm_spill_buffer_size_256b_f(size),
			patch);
}

void gv11b_gr_init_commit_gfxp_wfi_timeout(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx, bool patch)
{

	if (nvgpu_is_enabled(g, NVGPU_SUPPORT_MIG)) {
		nvgpu_log_fn(g, " MIG is enabled, skipped gfxp wfi timeout");
		return;
	}

	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_fe_gfxp_wfi_timeout_r(),
		GFXP_WFI_TIMEOUT_COUNT_IN_USEC_DEFAULT, patch);
}

int gv11b_gr_init_preemption_state(struct gk20a *g)
{
	u32 debug_2;

	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gr, " ");

	if (nvgpu_is_enabled(g, NVGPU_SUPPORT_MIG)) {
		nvgpu_log_fn(g,
			" MIG is enabled, skipped init gfxp wfi timeout");
		return 0;
	}

	debug_2 = nvgpu_readl(g, gr_debug_2_r());
	debug_2 = set_field(debug_2,
		gr_debug_2_gfxp_wfi_timeout_unit_m(),
		gr_debug_2_gfxp_wfi_timeout_unit_usec_f());
	nvgpu_writel(g, gr_debug_2_r(), debug_2);

	return 0;
}
#endif /* CONFIG_NVGPU_GRAPHICS */

#ifdef CONFIG_NVGPU_SET_FALCON_ACCESS_MAP
void gv11b_gr_init_get_access_map(struct gk20a *g,
				   u32 **whitelist, u32 *num_entries)
{
	static u32 wl_addr_gv11b[] = {
		/* this list must be sorted (low to high) */
		0x404468, /* gr_pri_mme_max_instructions       */
		0x418380, /* gr_pri_gpcs_rasterarb_line_class  */
		0x418800, /* gr_pri_gpcs_setup_debug           */
		0x418830, /* gr_pri_gpcs_setup_debug_z_gamut_offset */
		0x4188fc, /* gr_pri_gpcs_zcull_ctx_debug       */
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
		0x419e84, /* gr_pri_gpcs_tpcs_sms_dbgr_control0 */
		0x419ba4, /* gr_pri_gpcs_tpcs_sm_disp_ctrl     */
	};
	size_t array_size;

	*whitelist = wl_addr_gv11b;
	array_size = ARRAY_SIZE(wl_addr_gv11b);
	*num_entries = nvgpu_safe_cast_u64_to_u32(array_size);
}
#endif
