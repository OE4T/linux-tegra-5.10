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
#include <nvgpu/log.h>
#include <nvgpu/bug.h>
#include <nvgpu/gr/ctx.h>
#include <nvgpu/ltc.h>
#include <nvgpu/netlist.h>

#include <nvgpu/gr/config.h>
#include "common/gr/gr_priv.h"

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

static int gr_gv11b_ecc_scrub_is_done(struct gk20a *g,
			struct nvgpu_gr_config *gr_config,
			u32 scrub_reg, u32 scrub_mask, u32 scrub_done)
{
	struct nvgpu_timeout timeout;
	u32 val;
	u32 gpc, tpc;
	u32 gpc_offset, tpc_offset;
	int err;
	u32 gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_GPC_STRIDE);
	u32 tpc_in_gpc_stride = nvgpu_get_litter_value(g,
					GPU_LIT_TPC_IN_GPC_STRIDE);

	err = nvgpu_timeout_init(g, &timeout,
		(GR_ECC_SCRUBBING_TIMEOUT_MAX_US /
		 GR_ECC_SCRUBBING_TIMEOUT_DEFAULT_US),
		NVGPU_TIMER_RETRY_TIMER);
	if (err != 0) {
		nvgpu_err(g, "timeout_init failed: %d", err);
		return err;
	}

	for (gpc = 0; gpc < nvgpu_gr_config_get_gpc_count(gr_config); gpc++) {
		gpc_offset = gpc_stride * gpc;

		for (tpc = 0;
		     tpc < nvgpu_gr_config_get_gpc_tpc_count(gr_config, gpc);
		     tpc++) {
			tpc_offset = tpc_in_gpc_stride * tpc;

			do {
				val = nvgpu_readl(g,
					gpc_offset + tpc_offset + scrub_reg);
				if ((val & scrub_mask) == scrub_done) {
					break;
				}

				if (nvgpu_timeout_expired(&timeout) != 0) {
					return -ETIMEDOUT;
				}

				nvgpu_udelay(
					GR_ECC_SCRUBBING_TIMEOUT_DEFAULT_US);
			} while (true);
		}
	}

	return 0;
}

static int gr_gv11b_ecc_scrub_sm_lrf(struct gk20a *g,
				     struct nvgpu_gr_config *gr_config)
{
	u32 scrub_mask, scrub_done;

	if (!nvgpu_is_enabled(g, NVGPU_ECC_ENABLED_SM_LRF)) {
		nvgpu_log_info(g, "ECC SM LRF is disabled");
		return 0;
	}

	nvgpu_log_info(g, "gr_gv11b_ecc_scrub_sm_lrf");
	scrub_mask =
		(gr_pri_gpcs_tpcs_sm_lrf_ecc_control_scrub_qrfdp0_task_f() |
		gr_pri_gpcs_tpcs_sm_lrf_ecc_control_scrub_qrfdp1_task_f() |
		gr_pri_gpcs_tpcs_sm_lrf_ecc_control_scrub_qrfdp2_task_f() |
		gr_pri_gpcs_tpcs_sm_lrf_ecc_control_scrub_qrfdp3_task_f() |
		gr_pri_gpcs_tpcs_sm_lrf_ecc_control_scrub_qrfdp4_task_f() |
		gr_pri_gpcs_tpcs_sm_lrf_ecc_control_scrub_qrfdp5_task_f() |
		gr_pri_gpcs_tpcs_sm_lrf_ecc_control_scrub_qrfdp6_task_f() |
		gr_pri_gpcs_tpcs_sm_lrf_ecc_control_scrub_qrfdp7_task_f());

	/* Issue scrub lrf regions with single write command */
	nvgpu_writel(g, gr_pri_gpcs_tpcs_sm_lrf_ecc_control_r(), scrub_mask);

	scrub_done =
		(gr_pri_gpc0_tpc0_sm_lrf_ecc_control_scrub_qrfdp0_init_f() |
		gr_pri_gpc0_tpc0_sm_lrf_ecc_control_scrub_qrfdp1_init_f() |
		gr_pri_gpc0_tpc0_sm_lrf_ecc_control_scrub_qrfdp2_init_f() |
		gr_pri_gpc0_tpc0_sm_lrf_ecc_control_scrub_qrfdp3_init_f() |
		gr_pri_gpc0_tpc0_sm_lrf_ecc_control_scrub_qrfdp4_init_f() |
		gr_pri_gpc0_tpc0_sm_lrf_ecc_control_scrub_qrfdp5_init_f() |
		gr_pri_gpc0_tpc0_sm_lrf_ecc_control_scrub_qrfdp6_init_f() |
		gr_pri_gpc0_tpc0_sm_lrf_ecc_control_scrub_qrfdp7_init_f());

	return gr_gv11b_ecc_scrub_is_done(g, gr_config,
				gr_pri_gpc0_tpc0_sm_lrf_ecc_control_r(),
				scrub_mask, scrub_done);
}

static int gr_gv11b_ecc_scrub_sm_l1_data(struct gk20a *g,
					 struct nvgpu_gr_config *gr_config)
{
	u32 scrub_mask, scrub_done;

	if (!nvgpu_is_enabled(g, NVGPU_ECC_ENABLED_SM_L1_DATA)) {
		nvgpu_log_info(g, "ECC L1DATA is disabled");
		return 0;
	}
	nvgpu_log_info(g, "gr_gv11b_ecc_scrub_sm_l1_data");
	scrub_mask =
		(gr_pri_gpcs_tpcs_sm_l1_data_ecc_control_scrub_el1_0_task_f() |
		gr_pri_gpcs_tpcs_sm_l1_data_ecc_control_scrub_el1_1_task_f());

	nvgpu_writel(g, gr_pri_gpcs_tpcs_sm_l1_data_ecc_control_r(),
				scrub_mask);

	scrub_done =
		(gr_pri_gpc0_tpc0_sm_l1_data_ecc_control_scrub_el1_0_init_f() |
		gr_pri_gpc0_tpc0_sm_l1_data_ecc_control_scrub_el1_1_init_f());
	return gr_gv11b_ecc_scrub_is_done(g, gr_config,
				gr_pri_gpc0_tpc0_sm_l1_data_ecc_control_r(),
				scrub_mask, scrub_done);
}

static int gr_gv11b_ecc_scrub_sm_l1_tag(struct gk20a *g,
					struct nvgpu_gr_config *gr_config)
{
	u32 scrub_mask, scrub_done;

	if (!nvgpu_is_enabled(g, NVGPU_ECC_ENABLED_SM_L1_TAG)) {
		nvgpu_log_info(g, "ECC L1TAG is disabled");
		return 0;
	}
	nvgpu_log_info(g, "gr_gv11b_ecc_scrub_sm_l1_tag");
	scrub_mask =
	 (gr_pri_gpcs_tpcs_sm_l1_tag_ecc_control_scrub_el1_0_task_f() |
	  gr_pri_gpcs_tpcs_sm_l1_tag_ecc_control_scrub_el1_1_task_f() |
	  gr_pri_gpcs_tpcs_sm_l1_tag_ecc_control_scrub_pixprf_task_f() |
	  gr_pri_gpcs_tpcs_sm_l1_tag_ecc_control_scrub_miss_fifo_task_f());
	nvgpu_writel(g, gr_pri_gpcs_tpcs_sm_l1_tag_ecc_control_r(),
		     scrub_mask);

	scrub_done =
	 (gr_pri_gpc0_tpc0_sm_l1_tag_ecc_control_scrub_el1_0_init_f() |
	  gr_pri_gpc0_tpc0_sm_l1_tag_ecc_control_scrub_el1_1_init_f() |
	  gr_pri_gpc0_tpc0_sm_l1_tag_ecc_control_scrub_pixprf_init_f() |
	  gr_pri_gpc0_tpc0_sm_l1_tag_ecc_control_scrub_miss_fifo_init_f());
	return gr_gv11b_ecc_scrub_is_done(g, gr_config,
				gr_pri_gpc0_tpc0_sm_l1_tag_ecc_control_r(),
				scrub_mask, scrub_done);
}

static int gr_gv11b_ecc_scrub_sm_cbu(struct gk20a *g,
				     struct nvgpu_gr_config *gr_config)
{
	u32 scrub_mask, scrub_done;

	if (!nvgpu_is_enabled(g, NVGPU_ECC_ENABLED_SM_CBU)) {
		nvgpu_log_info(g, "ECC CBU is disabled");
		return 0;
	}
	nvgpu_log_info(g, "gr_gv11b_ecc_scrub_sm_cbu");
	scrub_mask =
	 (gr_pri_gpcs_tpcs_sm_cbu_ecc_control_scrub_warp_sm0_task_f() |
	  gr_pri_gpcs_tpcs_sm_cbu_ecc_control_scrub_warp_sm1_task_f() |
	  gr_pri_gpcs_tpcs_sm_cbu_ecc_control_scrub_barrier_sm0_task_f() |
	  gr_pri_gpcs_tpcs_sm_cbu_ecc_control_scrub_barrier_sm1_task_f());
	nvgpu_writel(g, gr_pri_gpcs_tpcs_sm_cbu_ecc_control_r(), scrub_mask);

	scrub_done =
	 (gr_pri_gpc0_tpc0_sm_cbu_ecc_control_scrub_warp_sm0_init_f() |
	  gr_pri_gpc0_tpc0_sm_cbu_ecc_control_scrub_warp_sm1_init_f() |
	  gr_pri_gpc0_tpc0_sm_cbu_ecc_control_scrub_barrier_sm0_init_f() |
	  gr_pri_gpc0_tpc0_sm_cbu_ecc_control_scrub_barrier_sm1_init_f());
	return gr_gv11b_ecc_scrub_is_done(g, gr_config,
				gr_pri_gpc0_tpc0_sm_cbu_ecc_control_r(),
				scrub_mask, scrub_done);
}

static int gr_gv11b_ecc_scrub_sm_icahe(struct gk20a *g,
				       struct nvgpu_gr_config *gr_config)
{
	u32 scrub_mask, scrub_done;

	if (!nvgpu_is_enabled(g, NVGPU_ECC_ENABLED_SM_ICACHE)) {
		nvgpu_log_info(g, "ECC ICAHE is disabled");
		return 0;
	}
	nvgpu_log_info(g, "gr_gv11b_ecc_scrub_sm_icahe");
	scrub_mask =
	 (gr_pri_gpcs_tpcs_sm_icache_ecc_control_scrub_l0_data_task_f() |
	  gr_pri_gpcs_tpcs_sm_icache_ecc_control_scrub_l0_predecode_task_f() |
	  gr_pri_gpcs_tpcs_sm_icache_ecc_control_scrub_l1_data_task_f() |
	  gr_pri_gpcs_tpcs_sm_icache_ecc_control_scrub_l1_predecode_task_f());
	nvgpu_writel(g, gr_pri_gpcs_tpcs_sm_icache_ecc_control_r(),
		     scrub_mask);

	scrub_done =
	 (gr_pri_gpc0_tpc0_sm_icache_ecc_control_scrub_l0_data_init_f() |
	  gr_pri_gpc0_tpc0_sm_icache_ecc_control_scrub_l0_predecode_init_f() |
	  gr_pri_gpc0_tpc0_sm_icache_ecc_control_scrub_l1_data_init_f() |
	  gr_pri_gpc0_tpc0_sm_icache_ecc_control_scrub_l1_predecode_init_f());
	return gr_gv11b_ecc_scrub_is_done(g, gr_config,
				gr_pri_gpc0_tpc0_sm_icache_ecc_control_r(),
				scrub_mask, scrub_done);
}

void gv11b_gr_init_ecc_scrub_reg(struct gk20a *g,
				 struct nvgpu_gr_config *gr_config)
{
	nvgpu_log_fn(g, "ecc srub start ");

	if (gr_gv11b_ecc_scrub_sm_lrf(g, gr_config) != 0) {
		nvgpu_warn(g, "ECC SCRUB SM LRF Failed");
	}
	if (gr_gv11b_ecc_scrub_sm_l1_data(g, gr_config) != 0) {
		nvgpu_warn(g, "ECC SCRUB SM L1 DATA Failed");
	}
	if (gr_gv11b_ecc_scrub_sm_l1_tag(g, gr_config) != 0) {
		nvgpu_warn(g, "ECC SCRUB SM L1 TAG Failed");
	}
	if (gr_gv11b_ecc_scrub_sm_cbu(g, gr_config) != 0) {
		nvgpu_warn(g, "ECC SCRUB SM CBU Failed");
	}
	if (gr_gv11b_ecc_scrub_sm_icahe(g, gr_config) != 0) {
		nvgpu_warn(g, "ECC SCRUB SM ICACHE Failed");
	}

}

u32 gv11b_gr_init_get_nonpes_aware_tpc(struct gk20a *g, u32 gpc, u32 tpc,
				       struct nvgpu_gr_config *gr_config)
{
	u32 tpc_new = 0;
	u32 temp;
	u32 pes;

	for (pes = 0U;
	     pes < nvgpu_gr_config_get_gpc_ppc_count(gr_config, gpc);
	     pes++) {
		if ((nvgpu_gr_config_get_pes_tpc_mask(gr_config, gpc, pes) &
		    BIT32(tpc)) != 0U) {
			break;
		}
		tpc_new += nvgpu_gr_config_get_pes_tpc_count(gr_config,
				gpc, pes);
	}
	temp = (BIT32(tpc) - 1U) &
		nvgpu_gr_config_get_pes_tpc_mask(gr_config, gpc, pes);
	temp = (u32)hweight32(temp);
	tpc_new += temp;

	nvgpu_log_info(g, "tpc: %d -> new tpc: %d", tpc, tpc_new);
	return tpc_new;
}

void gv11b_gr_init_gpc_mmu(struct gk20a *g)
{
	u32 temp;

	nvgpu_log_info(g, "initialize gpc mmu");

	temp = g->ops.fb.mmu_ctrl(g);
	temp &= gr_gpcs_pri_mmu_ctrl_vm_pg_size_m() |
		gr_gpcs_pri_mmu_ctrl_use_pdb_big_page_size_m() |
		gr_gpcs_pri_mmu_ctrl_vol_fault_m() |
		gr_gpcs_pri_mmu_ctrl_comp_fault_m() |
		gr_gpcs_pri_mmu_ctrl_miss_gran_m() |
		gr_gpcs_pri_mmu_ctrl_cache_mode_m() |
		gr_gpcs_pri_mmu_ctrl_mmu_aperture_m() |
		gr_gpcs_pri_mmu_ctrl_mmu_vol_m() |
		gr_gpcs_pri_mmu_ctrl_mmu_disable_m()|
		gr_gpcs_pri_mmu_ctrl_atomic_capability_mode_m()|
		gr_gpcs_pri_mmu_ctrl_atomic_capability_sys_ncoh_mode_m();
	nvgpu_writel(g, gr_gpcs_pri_mmu_ctrl_r(), temp);
	nvgpu_writel(g, gr_gpcs_pri_mmu_pm_unit_mask_r(), 0);
	nvgpu_writel(g, gr_gpcs_pri_mmu_pm_req_mask_r(), 0);

	nvgpu_writel(g, gr_gpcs_pri_mmu_debug_ctrl_r(),
			g->ops.fb.mmu_debug_ctrl(g));
	nvgpu_writel(g, gr_gpcs_pri_mmu_debug_wr_r(),
			g->ops.fb.mmu_debug_wr(g));
	nvgpu_writel(g, gr_gpcs_pri_mmu_debug_rd_r(),
			g->ops.fb.mmu_debug_rd(g));
}

void gv11b_gr_init_get_access_map(struct gk20a *g,
				   u32 **whitelist, int *num_entries)
{
	static u32 wl_addr_gv11b[] = {
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
		0x419e84, /* gr_pri_gpcs_tpcs_sms_dbgr_control0 */
		0x419ba4, /* gr_pri_gpcs_tpcs_sm_disp_ctrl     */
	};
	size_t array_size;

	*whitelist = wl_addr_gv11b;
	array_size = ARRAY_SIZE(wl_addr_gv11b);
	*num_entries = (int)array_size;
}

void gv11b_gr_init_sm_id_numbering(struct gk20a *g, u32 gpc, u32 tpc, u32 smid,
				   struct nvgpu_gr_config *gr_config)
{
	u32 gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_GPC_STRIDE);
	u32 tpc_in_gpc_stride = nvgpu_get_litter_value(g,
					GPU_LIT_TPC_IN_GPC_STRIDE);
	u32 gpc_offset = gpc_stride * gpc;
	u32 global_tpc_index;
	u32 tpc_offset;
	struct sm_info *sm_info =
		nvgpu_gr_config_get_sm_info(gr_config, smid);

	global_tpc_index =
		nvgpu_gr_config_get_sm_info_global_tpc_index(sm_info);

	tpc = g->ops.gr.init.get_nonpes_aware_tpc(g, gpc, tpc, gr_config);
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
	u32 no_of_sm = g->ops.gr.init.get_no_of_sm(g);

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
			struct sm_info *sm_info;

			tpc_id = (i << 2) + j;
			sm_id = tpc_id * sm_per_tpc;

			if (sm_id >= no_of_sm) {
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

	if (nvgpu_gr_config_get_map_tiles(gr_config) == NULL) {
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

	if (g->gr->fecs_feature_override_ecc_val != 0U) {
		nvgpu_writel(g,
			gr_fecs_feature_override_ecc_r(),
			g->gr->fecs_feature_override_ecc_val);
	}

	data = nvgpu_readl(g, gr_debug_0_r());
	data = set_field(data,
		gr_debug_0_scg_force_slow_drain_tpc_m(),
		gr_debug_0_scg_force_slow_drain_tpc_enabled_f());
	nvgpu_writel(g, gr_debug_0_r(), data);

	nvgpu_writel(g, gr_bes_zrop_settings_r(),
		     gr_bes_zrop_settings_num_active_ltcs_f(
			nvgpu_ltc_get_ltc_count(g)));
	nvgpu_writel(g, gr_bes_crop_settings_r(),
		     gr_bes_crop_settings_num_active_ltcs_f(
			nvgpu_ltc_get_ltc_count(g)));

	return err;
}

int gv11b_gr_init_preemption_state(struct gk20a *g)
{
	u32 debug_2;

	nvgpu_log_fn(g, " ");

	debug_2 = nvgpu_readl(g, gr_debug_2_r());
	debug_2 = set_field(debug_2,
		gr_debug_2_gfxp_wfi_timeout_unit_m(),
		gr_debug_2_gfxp_wfi_timeout_unit_usec_f());
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

u32 gv11b_gr_init_get_bundle_cb_default_size(struct gk20a *g)
{
	return gr_scc_bundle_cb_size_div_256b__prod_v();
}

u32 gv11b_gr_init_get_min_gpm_fifo_depth(struct gk20a *g)
{
	return gr_pd_ab_dist_cfg2_state_limit_min_gpm_fifo_depths_v();
}

u32 gv11b_gr_init_get_bundle_cb_token_limit(struct gk20a *g)
{
	return gr_pd_ab_dist_cfg2_token_limit_init_v();
}

u32 gv11b_gr_init_get_attrib_cb_default_size(struct gk20a *g)
{
	return gr_gpc0_ppc0_cbm_beta_cb_size_v_default_v();
}

u32 gv11b_gr_init_get_alpha_cb_default_size(struct gk20a *g)
{
	return gr_gpc0_ppc0_cbm_alpha_cb_size_v_default_v();
}

u32 gv11b_gr_init_get_attrib_cb_gfxp_default_size(struct gk20a *g)
{
	return gr_gpc0_ppc0_cbm_beta_cb_size_v_gfxp_v();
}

u32 gv11b_gr_init_get_attrib_cb_gfxp_size(struct gk20a *g)
{
	return gr_gpc0_ppc0_cbm_beta_cb_size_v_gfxp_v();
}

u32 gv11b_gr_init_get_attrib_cb_size(struct gk20a *g, u32 tpc_count)
{
	return min(g->ops.gr.init.get_attrib_cb_default_size(g),
		 gr_gpc0_ppc0_cbm_beta_cb_size_v_f(~U32(0U)) / tpc_count);
}

u32 gv11b_gr_init_get_alpha_cb_size(struct gk20a *g, u32 tpc_count)
{
	return min(g->ops.gr.init.get_alpha_cb_default_size(g),
		 gr_gpc0_ppc0_cbm_alpha_cb_size_v_f(~U32(0U)) / tpc_count);
}

u32 gv11b_gr_init_get_global_attr_cb_size(struct gk20a *g, u32 tpc_count,
	u32 max_tpc)
{
	u32 size;

	size = g->ops.gr.init.get_attrib_cb_size(g, tpc_count) *
		gr_gpc0_ppc0_cbm_beta_cb_size_v_granularity_v() * max_tpc;

	size += g->ops.gr.init.get_alpha_cb_size(g, tpc_count) *
		gr_gpc0_ppc0_cbm_alpha_cb_size_v_granularity_v() * max_tpc;

	size = ALIGN(size, 128);

	return size;
}

void gv11b_gr_init_commit_global_attrib_cb(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx, u32 tpc_count, u32 max_tpc, u64 addr,
	bool patch)
{
	u32 attrBufferSize;

	gm20b_gr_init_commit_global_attrib_cb(g, gr_ctx, tpc_count, max_tpc,
		addr, patch);

	addr = (u64_lo32(addr) >>
			gr_gpcs_setup_attrib_cb_base_addr_39_12_align_bits_v()) |
		(u64_hi32(addr) <<
			(32U - gr_gpcs_setup_attrib_cb_base_addr_39_12_align_bits_v()));

	if (nvgpu_gr_ctx_get_preempt_ctxsw_buffer(gr_ctx)->gpu_va != 0ULL) {
		attrBufferSize =
			U32(nvgpu_gr_ctx_get_betacb_ctxsw_buffer(gr_ctx)->size);
	} else {
		attrBufferSize = g->ops.gr.init.get_global_attr_cb_size(g,
			tpc_count, max_tpc);
	}

	attrBufferSize /= gr_gpcs_tpcs_tex_rm_cb_1_size_div_128b_granularity_f();

	nvgpu_assert(u64_hi32(addr) == 0U);
	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_gpcs_tpcs_mpc_vtg_cb_global_base_addr_r(),
		gr_gpcs_tpcs_mpc_vtg_cb_global_base_addr_v_f((u32)addr) |
		gr_gpcs_tpcs_mpc_vtg_cb_global_base_addr_valid_true_f(), patch);

	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_gpcs_tpcs_tex_rm_cb_0_r(),
		gr_gpcs_tpcs_tex_rm_cb_0_base_addr_43_12_f((u32)addr), patch);

	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_gpcs_tpcs_tex_rm_cb_1_r(),
		gr_gpcs_tpcs_tex_rm_cb_1_size_div_128b_f(attrBufferSize) |
		gr_gpcs_tpcs_tex_rm_cb_1_valid_true_f(), patch);
}

static int gv11b_gr_init_write_bundle_veid_state(struct gk20a *g, u32 index,
	struct netlist_av_list *sw_veid_bundle_init)
{
	u32 j;
	u32  num_subctx;
	int err = 0;

	num_subctx = g->ops.gr.init.get_max_subctx_count();

	for (j = 0U; j < num_subctx; j++) {
		nvgpu_log_fn(g, "write bundle_address_r for subctx: %d", j);
		nvgpu_writel(g, gr_pipe_bundle_address_r(),
			sw_veid_bundle_init->l[index].addr |
			gr_pipe_bundle_address_veid_f(j));

		err = g->ops.gr.init.wait_idle(g);
	}

	return err;
}

int gv11b_gr_init_load_sw_veid_bundle(struct gk20a *g,
	struct netlist_av_list *sw_veid_bundle_init)
{
	u32 i;
	int err = 0;
	u32 last_bundle_data = 0;

	for (i = 0U; i < sw_veid_bundle_init->count; i++) {
		nvgpu_log_fn(g, "veid bundle count: %d", i);

		if (i == 0U || last_bundle_data !=
				sw_veid_bundle_init->l[i].value) {
			nvgpu_writel(g, gr_pipe_bundle_data_r(),
				sw_veid_bundle_init->l[i].value);
			last_bundle_data = sw_veid_bundle_init->l[i].value;
			nvgpu_log_fn(g, "last_bundle_data : 0x%08x",
						last_bundle_data);
		}

		if (gr_pipe_bundle_address_value_v(
			sw_veid_bundle_init->l[i].addr) == GR_GO_IDLE_BUNDLE) {
			nvgpu_log_fn(g, "go idle bundle");
				nvgpu_writel(g, gr_pipe_bundle_address_r(),
					sw_veid_bundle_init->l[i].addr);
				err = g->ops.gr.init.wait_idle(g);
		} else {
			err = gv11b_gr_init_write_bundle_veid_state(g, i,
					sw_veid_bundle_init);
		}

		if (err != 0) {
			nvgpu_err(g, "failed to init sw veid bundle");
			break;
		}
	}

	return err;
}

u32 gv11b_gr_init_get_ctx_spill_size(struct gk20a *g)
{
	return  gr_gpc0_swdx_rm_spill_buffer_size_256b_default_v() *
		gr_gpc0_swdx_rm_spill_buffer_size_256b_byte_granularity_v();
}

u32 gv11b_gr_init_get_ctx_betacb_size(struct gk20a *g)
{
	return g->ops.gr.init.get_attrib_cb_default_size(g) +
		(gr_gpc0_ppc0_cbm_beta_cb_size_v_gfxp_v() -
		 gr_gpc0_ppc0_cbm_beta_cb_size_v_default_v());
}

void gv11b_gr_init_commit_ctxsw_spill(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx, u64 addr, u32 size, bool patch)
{
	addr = (u64_lo32(addr) >>
			gr_gpc0_swdx_rm_spill_buffer_addr_39_8_align_bits_v()) |
		(u64_hi32(addr) <<
			(32U - gr_gpc0_swdx_rm_spill_buffer_addr_39_8_align_bits_v()));

	size /=	gr_gpc0_swdx_rm_spill_buffer_size_256b_byte_granularity_v();

	nvgpu_assert(u64_hi32(addr) == 0U);
	nvgpu_gr_ctx_patch_write(g, gr_ctx,
			gr_gpc0_swdx_rm_spill_buffer_addr_r(),
			gr_gpc0_swdx_rm_spill_buffer_addr_39_8_f(U32(addr)),
			patch);
	nvgpu_gr_ctx_patch_write(g, gr_ctx,
			gr_gpc0_swdx_rm_spill_buffer_size_r(),
			gr_gpc0_swdx_rm_spill_buffer_size_256b_f(size),
			patch);
}

void gv11b_gr_init_commit_cbes_reserve(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx, bool patch)
{
	u32 cbes_reserve = gr_gpcs_swdx_beta_cb_ctrl_cbes_reserve_gfxp_v();

	nvgpu_gr_ctx_patch_write(g, gr_ctx,
		gr_gpcs_swdx_beta_cb_ctrl_r(),
		gr_gpcs_swdx_beta_cb_ctrl_cbes_reserve_f(cbes_reserve),
		patch);
	nvgpu_gr_ctx_patch_write(g, gr_ctx,
		gr_gpcs_ppcs_cbm_beta_cb_ctrl_r(),
		gr_gpcs_ppcs_cbm_beta_cb_ctrl_cbes_reserve_f(cbes_reserve),
		patch);
}

void gv11b_gr_init_commit_gfxp_wfi_timeout(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx, bool patch)
{
	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_fe_gfxp_wfi_timeout_r(),
		GFXP_WFI_TIMEOUT_COUNT_IN_USEC_DEFAULT, patch);
}

u32 gv11b_gr_init_get_max_subctx_count(void)
{
	return gr_pri_fe_chip_def_info_max_veid_count_init_v();
}

u32 gv11b_gr_init_get_patch_slots(struct gk20a *g,
	struct nvgpu_gr_config *config)
{
	u32 size = 0;

	/*
	 * CMD to update PE table
	 */
	size++;

	/*
	 * Update PE table contents
	 * for PE table, each patch buffer update writes 32 TPCs
	 */
	size += DIV_ROUND_UP(nvgpu_gr_config_get_tpc_count(config), 32U);

	/*
	 * Update the PL table contents
	 * For PL table, each patch buffer update configures 4 TPCs
	 */
	size += DIV_ROUND_UP(nvgpu_gr_config_get_tpc_count(config), 4U);

	/*
	 * We need this for all subcontexts
	 */
	size *= g->ops.gr.init.get_max_subctx_count();

	/*
	 * Add space for a partition mode change as well
	 * reserve two slots since DYNAMIC -> STATIC requires
	 * DYNAMIC -> NONE -> STATIC
	 */
	size += 2U;

	/*
	 * Add current patch buffer size
	 */
	size += gm20b_gr_init_get_patch_slots(g, config);

	/*
	 * Align to 4K size
	 */
	size = ALIGN(size, PATCH_CTX_SLOTS_PER_PAGE);

	/*
	 * Increase the size to accommodate for additional TPC partition update
	 */
	size += 2U * PATCH_CTX_SLOTS_PER_PAGE;

	return size;
}

void gv11b_gr_init_detect_sm_arch(struct gk20a *g)
{
	u32 v = gk20a_readl(g, gr_gpc0_tpc0_sm_arch_r());

	g->params.sm_arch_spa_version =
		gr_gpc0_tpc0_sm_arch_spa_version_v(v);
	g->params.sm_arch_sm_version =
		gr_gpc0_tpc0_sm_arch_sm_version_v(v);
	g->params.sm_arch_warp_count =
		gr_gpc0_tpc0_sm_arch_warp_count_v(v);
}

