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
#include <nvgpu/gr/ctx.h>
#include <nvgpu/gr/config.h>

#include <nvgpu/gr/config.h>
#include <nvgpu/gr/gr.h>

#include "common/gr/gr_priv.h"

#include "gr_init_gm20b.h"
#include "gr_init_gp10b.h"

#include <nvgpu/hw/gp10b/hw_gr_gp10b.h>

#define GFXP_WFI_TIMEOUT_COUNT_DEFAULT 100000U

void gp10b_gr_init_get_access_map(struct gk20a *g,
				   u32 **whitelist, int *num_entries)
{
	static u32 wl_addr_gp10b[] = {
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

	*whitelist = wl_addr_gp10b;
	array_size = ARRAY_SIZE(wl_addr_gp10b);
	*num_entries = (int)array_size;
}

u32 gp10b_gr_init_get_sm_id_size(void)
{
	return gr_cwd_sm_id__size_1_v();
}

int gp10b_gr_init_sm_id_config(struct gk20a *g, u32 *tpc_sm_id,
			       struct nvgpu_gr_config *gr_config)
{
	u32 i, j;
	u32 tpc_index, gpc_index;
	u32 max_gpcs = nvgpu_get_litter_value(g, GPU_LIT_NUM_GPCS);

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
			struct nvgpu_sm_info *sm_info;

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

			tpc_sm_id[gpc_index + max_gpcs * ((tpc_index & 4U) >> 2U)]
				|= sm_id << (bit_stride * (tpc_index & 3U));
		}
		nvgpu_writel(g, gr_cwd_gpc_tpc_id_r(i), reg);
	}

	for (i = 0; i < gr_cwd_sm_id__size_1_v(); i++) {
		nvgpu_writel(g, gr_cwd_sm_id_r(i), tpc_sm_id[i]);
	}

	return 0;
}

static bool gr_activity_empty_or_preempted(u32 val)
{
	while (val != 0U) {
		u32 v = val & 7U;

		if (v != gr_activity_4_gpc0_empty_v() &&
		    v != gr_activity_4_gpc0_preempted_v()) {
			return false;
		}
		val >>= 3;
	}

	return true;
}

int gp10b_gr_init_wait_empty(struct gk20a *g)
{
	u32 delay = POLL_DELAY_MIN_US;
	bool ctxsw_active;
	bool gr_busy;
	u32 gr_status;
	u32 activity0, activity1, activity2, activity4;
	struct nvgpu_timeout timeout;
	int err;

	nvgpu_log_fn(g, " ");

	err = nvgpu_timeout_init(g, &timeout, nvgpu_get_poll_timeout(g),
				 NVGPU_TIMER_CPU_TIMER);
	if (err != 0) {
		nvgpu_err(g, "timeout_init failed: %d", err);
		return err;
	}

	do {
		/* fmodel: host gets fifo_engine_status(gr) from gr
		   only when gr_status is read */
		gr_status = nvgpu_readl(g, gr_status_r());

		ctxsw_active = (gr_status & BIT32(7)) != 0U;

		activity0 = nvgpu_readl(g, gr_activity_0_r());
		activity1 = nvgpu_readl(g, gr_activity_1_r());
		activity2 = nvgpu_readl(g, gr_activity_2_r());
		activity4 = nvgpu_readl(g, gr_activity_4_r());

		gr_busy = !(gr_activity_empty_or_preempted(activity0) &&
			    gr_activity_empty_or_preempted(activity1) &&
			    activity2 == 0U &&
			    gr_activity_empty_or_preempted(activity4));

		if (!gr_busy && !ctxsw_active) {
			nvgpu_log_fn(g, "done");
			return 0;
		}

		nvgpu_usleep_range(delay, delay * 2U);
		delay = min_t(u32, delay << 1, POLL_DELAY_MAX_US);
	} while (nvgpu_timeout_expired(&timeout) == 0);

	nvgpu_err(g,
	    "timeout, ctxsw busy : %d, gr busy : %d, %08x, %08x, %08x, %08x",
	    ctxsw_active, gr_busy, activity0, activity1, activity2, activity4);

	return -EAGAIN;
}

int gp10b_gr_init_fs_state(struct gk20a *g)
{
	u32 data;

	nvgpu_log_fn(g, " ");

	data = nvgpu_readl(g, gr_gpcs_tpcs_sm_texio_control_r());
	data = set_field(data,
		gr_gpcs_tpcs_sm_texio_control_oor_addr_check_mode_m(),
		gr_gpcs_tpcs_sm_texio_control_oor_addr_check_mode_arm_63_48_match_f());
	nvgpu_writel(g, gr_gpcs_tpcs_sm_texio_control_r(), data);

	data = nvgpu_readl(g, gr_gpcs_tpcs_sm_disp_ctrl_r());
	data = set_field(data, gr_gpcs_tpcs_sm_disp_ctrl_re_suppress_m(),
			 gr_gpcs_tpcs_sm_disp_ctrl_re_suppress_disable_f());
	nvgpu_writel(g, gr_gpcs_tpcs_sm_disp_ctrl_r(), data);

	if (g->gr->fecs_feature_override_ecc_val != 0U) {
		nvgpu_writel(g,
			gr_fecs_feature_override_ecc_r(),
			g->gr->fecs_feature_override_ecc_val);
	}

	return gm20b_gr_init_fs_state(g);
}

int gp10b_gr_init_preemption_state(struct gk20a *g)
{
	u32 debug_2;
	u32 gfxp_wfi_timeout_count = GFXP_WFI_TIMEOUT_COUNT_DEFAULT;

	nvgpu_writel(g, gr_fe_gfxp_wfi_timeout_r(),
			gr_fe_gfxp_wfi_timeout_count_f(gfxp_wfi_timeout_count));

	debug_2 = nvgpu_readl(g, gr_debug_2_r());
	debug_2 = set_field(debug_2,
			gr_debug_2_gfxp_wfi_always_injects_wfi_m(),
			gr_debug_2_gfxp_wfi_always_injects_wfi_enabled_f());
	nvgpu_writel(g, gr_debug_2_r(), debug_2);

	return 0;
}

u32 gp10b_gr_init_get_attrib_cb_default_size(struct gk20a *g)
{
	return 0x800;
}

u32 gp10b_gr_init_get_alpha_cb_default_size(struct gk20a *g)
{
	return gr_gpc0_ppc0_cbm_alpha_cb_size_v_default_v();
}

u32 gp10b_gr_init_get_attrib_cb_gfxp_default_size(struct gk20a *g)
{
	return g->ops.gr.init.get_attrib_cb_default_size(g) +
			  (gr_gpc0_ppc0_cbm_beta_cb_size_v_gfxp_v() -
			   gr_gpc0_ppc0_cbm_beta_cb_size_v_default_v());
}

u32 gp10b_gr_init_get_attrib_cb_gfxp_size(struct gk20a *g)
{
	return g->ops.gr.init.get_attrib_cb_default_size(g) +
			  (gr_gpc0_ppc0_cbm_beta_cb_size_v_gfxp_v() -
			   gr_gpc0_ppc0_cbm_beta_cb_size_v_default_v());
}

u32 gp10b_gr_init_get_attrib_cb_size(struct gk20a *g, u32 tpc_count)
{
	return min(g->ops.gr.init.get_attrib_cb_default_size(g),
		 gr_gpc0_ppc0_cbm_beta_cb_size_v_f(~U32(0U)) / tpc_count);
}

u32 gp10b_gr_init_get_alpha_cb_size(struct gk20a *g, u32 tpc_count)
{
	return min(g->ops.gr.init.get_alpha_cb_default_size(g),
		 gr_gpc0_ppc0_cbm_alpha_cb_size_v_f(~U32(0U)) / tpc_count);
}

u32 gp10b_gr_init_get_global_attr_cb_size(struct gk20a *g, u32 tpc_count,
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

void gp10b_gr_init_commit_global_bundle_cb(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx, u64 addr, u32 size, bool patch)
{
	u32 data;
	u32 cb_addr;
	u32 bundle_cb_token_limit = g->ops.gr.init.get_bundle_cb_token_limit(g);

	addr = addr >> gr_scc_bundle_cb_base_addr_39_8_align_bits_v();

	nvgpu_log_info(g, "bundle cb addr : 0x%016llx, size : %u",
		addr, size);

	nvgpu_assert(u64_hi32(addr) == 0U);

	cb_addr = (u32)addr;
	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_scc_bundle_cb_base_r(),
		gr_scc_bundle_cb_base_addr_39_8_f(cb_addr), patch);

	nvgpu_assert(size <= U32_MAX);
	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_scc_bundle_cb_size_r(),
		gr_scc_bundle_cb_size_div_256b_f(size) |
		gr_scc_bundle_cb_size_valid_true_f(), patch);

	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_gpcs_swdx_bundle_cb_base_r(),
		gr_gpcs_swdx_bundle_cb_base_addr_39_8_f(cb_addr), patch);

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

u32 gp10b_gr_init_pagepool_default_size(struct gk20a *g)
{
	return gr_scc_pagepool_total_pages_hwmax_value_v();
}

void gp10b_gr_init_commit_global_pagepool(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx, u64 addr, size_t size, bool patch,
	bool global_ctx)
{
	u32 pp_addr;
	u32 pp_size;

	addr = addr >> gr_scc_pagepool_base_addr_39_8_align_bits_v();

	if (global_ctx) {
		size = size / gr_scc_pagepool_total_pages_byte_granularity_v();
	}

	if (size == g->ops.gr.init.pagepool_default_size(g)) {
		size = gr_scc_pagepool_total_pages_hwmax_v();
	}

	nvgpu_assert(u64_hi32(addr) == 0U);
	nvgpu_log_info(g, "pagepool buffer addr : 0x%016llx, size : %lu",
		addr, size);

	pp_addr = (u32)addr;
	pp_size = (u32)size;
	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_scc_pagepool_base_r(),
		gr_scc_pagepool_base_addr_39_8_f(pp_addr), patch);

	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_scc_pagepool_r(),
		gr_scc_pagepool_total_pages_f(pp_size) |
		gr_scc_pagepool_valid_true_f(), patch);

	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_gpcs_gcc_pagepool_base_r(),
		gr_gpcs_gcc_pagepool_base_addr_39_8_f(pp_addr), patch);

	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_gpcs_gcc_pagepool_r(),
		gr_gpcs_gcc_pagepool_total_pages_f(pp_size), patch);
}

void gp10b_gr_init_commit_global_attrib_cb(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx, u32 tpc_count, u32 max_tpc, u64 addr,
	bool patch)
{
	u32 attrBufferSize;
	u32 cb_addr;

	gm20b_gr_init_commit_global_attrib_cb(g, gr_ctx, tpc_count, max_tpc,
		addr, patch);

	addr = addr >> gr_gpcs_setup_attrib_cb_base_addr_39_12_align_bits_v();

	if (nvgpu_gr_ctx_get_preempt_ctxsw_buffer(gr_ctx)->gpu_va != 0ULL) {
		attrBufferSize =
			U32(nvgpu_gr_ctx_get_betacb_ctxsw_buffer(gr_ctx)->size);
	} else {
		attrBufferSize = g->ops.gr.init.get_global_attr_cb_size(g,
			tpc_count, max_tpc);
	}

	attrBufferSize /= gr_gpcs_tpcs_tex_rm_cb_1_size_div_128b_granularity_f();

	nvgpu_assert(u64_hi32(addr) == 0U);

	cb_addr = (u32)addr;
	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_gpcs_tpcs_mpc_vtg_cb_global_base_addr_r(),
		gr_gpcs_tpcs_mpc_vtg_cb_global_base_addr_v_f(cb_addr) |
		gr_gpcs_tpcs_mpc_vtg_cb_global_base_addr_valid_true_f(), patch);

	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_gpcs_tpcs_tex_rm_cb_0_r(),
		gr_gpcs_tpcs_tex_rm_cb_0_base_addr_43_12_f(cb_addr), patch);

	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_gpcs_tpcs_tex_rm_cb_1_r(),
		gr_gpcs_tpcs_tex_rm_cb_1_size_div_128b_f(attrBufferSize) |
		gr_gpcs_tpcs_tex_rm_cb_1_valid_true_f(), patch);
}

void gp10b_gr_init_commit_global_cb_manager(struct gk20a *g,
	struct nvgpu_gr_config *config, struct nvgpu_gr_ctx *gr_ctx, bool patch)
{
	u32 attrib_offset_in_chunk = 0;
	u32 alpha_offset_in_chunk = 0;
	u32 pd_ab_max_output;
	u32 gpc_index, ppc_index;
	u32 temp, temp2;
	u32 cbm_cfg_size_beta, cbm_cfg_size_alpha, cbm_cfg_size_steadystate;
	u32 attrib_size_in_chunk, cb_attrib_cache_size_init;
	u32 attrib_cb_default_size = g->ops.gr.init.get_attrib_cb_default_size(g);
	u32 alpha_cb_default_size = g->ops.gr.init.get_alpha_cb_default_size(g);
	u32 attrib_cb_size = g->ops.gr.init.get_attrib_cb_size(g,
		nvgpu_gr_config_get_tpc_count(config));
	u32 alpha_cb_size = g->ops.gr.init.get_alpha_cb_size(g,
		nvgpu_gr_config_get_tpc_count(config));
	u32 gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_GPC_STRIDE);
	u32 ppc_in_gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_PPC_IN_GPC_STRIDE);
	u32 num_pes_per_gpc = nvgpu_get_litter_value(g, GPU_LIT_NUM_PES_PER_GPC);

	nvgpu_log_fn(g, " ");

	if (nvgpu_gr_ctx_get_graphics_preemption_mode(gr_ctx)
			== NVGPU_PREEMPTION_MODE_GRAPHICS_GFXP) {
		attrib_size_in_chunk =
			g->ops.gr.init.get_attrib_cb_gfxp_size(g);
		cb_attrib_cache_size_init =
			g->ops.gr.init.get_attrib_cb_gfxp_default_size(g);
	} else {
		attrib_size_in_chunk = attrib_cb_size;
		cb_attrib_cache_size_init = attrib_cb_default_size;
	}

	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_ds_tga_constraintlogic_beta_r(),
		attrib_cb_default_size, patch);
	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_ds_tga_constraintlogic_alpha_r(),
		alpha_cb_default_size, patch);

	pd_ab_max_output = (alpha_cb_default_size *
		gr_gpc0_ppc0_cbm_beta_cb_size_v_granularity_v()) /
		gr_pd_ab_dist_cfg1_max_output_granularity_v();

	nvgpu_gr_ctx_patch_write(g, gr_ctx, gr_pd_ab_dist_cfg1_r(),
		gr_pd_ab_dist_cfg1_max_output_f(pd_ab_max_output) |
		gr_pd_ab_dist_cfg1_max_batches_init_f(), patch);

	attrib_offset_in_chunk = alpha_offset_in_chunk +
		nvgpu_gr_config_get_tpc_count(config) * alpha_cb_size;

	for (gpc_index = 0;
	     gpc_index < nvgpu_gr_config_get_gpc_count(config);
	     gpc_index++) {
		temp = gpc_stride * gpc_index;
		temp2 = num_pes_per_gpc * gpc_index;
		for (ppc_index = 0;
		     ppc_index < nvgpu_gr_config_get_gpc_ppc_count(config, gpc_index);
		     ppc_index++) {
			cbm_cfg_size_beta = cb_attrib_cache_size_init *
				nvgpu_gr_config_get_pes_tpc_count(config,
					gpc_index, ppc_index);
			cbm_cfg_size_alpha = alpha_cb_default_size *
				nvgpu_gr_config_get_pes_tpc_count(config,
					gpc_index, ppc_index);
			cbm_cfg_size_steadystate = attrib_cb_default_size *
				nvgpu_gr_config_get_pes_tpc_count(config,
					gpc_index, ppc_index);

			nvgpu_gr_ctx_patch_write(g, gr_ctx,
				gr_gpc0_ppc0_cbm_beta_cb_size_r() + temp +
				ppc_in_gpc_stride * ppc_index,
				cbm_cfg_size_beta, patch);

			nvgpu_gr_ctx_patch_write(g, gr_ctx,
				gr_gpc0_ppc0_cbm_beta_cb_offset_r() + temp +
				ppc_in_gpc_stride * ppc_index,
				attrib_offset_in_chunk, patch);

			nvgpu_gr_ctx_patch_write(g, gr_ctx,
				gr_gpc0_ppc0_cbm_beta_steady_state_cb_size_r() + temp +
				ppc_in_gpc_stride * ppc_index,
				cbm_cfg_size_steadystate,
				patch);

			attrib_offset_in_chunk += attrib_size_in_chunk *
				nvgpu_gr_config_get_pes_tpc_count(config,
					gpc_index, ppc_index);

			nvgpu_gr_ctx_patch_write(g, gr_ctx,
				gr_gpc0_ppc0_cbm_alpha_cb_size_r() + temp +
				ppc_in_gpc_stride * ppc_index,
				cbm_cfg_size_alpha, patch);

			nvgpu_gr_ctx_patch_write(g, gr_ctx,
				gr_gpc0_ppc0_cbm_alpha_cb_offset_r() + temp +
				ppc_in_gpc_stride * ppc_index,
				alpha_offset_in_chunk, patch);

			alpha_offset_in_chunk += alpha_cb_size *
				nvgpu_gr_config_get_pes_tpc_count(config,
					gpc_index, ppc_index);

			nvgpu_gr_ctx_patch_write(g, gr_ctx,
				gr_gpcs_swdx_tc_beta_cb_size_r(ppc_index + temp2),
				gr_gpcs_swdx_tc_beta_cb_size_v_f(cbm_cfg_size_steadystate),
				patch);
		}
	}
}

u32 gp10b_gr_init_get_ctx_spill_size(struct gk20a *g)
{
	return  gr_gpc0_swdx_rm_spill_buffer_size_256b_default_v() *
		gr_gpc0_swdx_rm_spill_buffer_size_256b_byte_granularity_v();
}

u32 gp10b_gr_init_get_ctx_pagepool_size(struct gk20a *g)
{
	return g->ops.gr.init.pagepool_default_size(g) *
		gr_scc_pagepool_total_pages_byte_granularity_v();
}

u32 gp10b_gr_init_get_ctx_betacb_size(struct gk20a *g)
{
	return g->ops.gr.init.get_attrib_cb_default_size(g) +
		(gr_gpc0_ppc0_cbm_beta_cb_size_v_gfxp_v() -
		 gr_gpc0_ppc0_cbm_beta_cb_size_v_default_v());
}

u32 gp10b_gr_init_get_ctx_attrib_cb_size(struct gk20a *g, u32 betacb_size,
	u32 tpc_count, u32 max_tpc)
{
	u32 alpha_cb_size = g->ops.gr.init.get_alpha_cb_size(g, tpc_count);
	u32 size;

	size = (betacb_size + alpha_cb_size) *
		gr_gpc0_ppc0_cbm_beta_cb_size_v_granularity_v() * max_tpc;

	return ALIGN(size, 128);
}

void gp10b_gr_init_commit_ctxsw_spill(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx, u64 addr, u32 size, bool patch)
{
	addr = addr >> gr_gpc0_swdx_rm_spill_buffer_addr_39_8_align_bits_v();

	size /= gr_gpc0_swdx_rm_spill_buffer_size_256b_byte_granularity_v();

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

void gp10b_gr_init_commit_cbes_reserve(struct gk20a *g,
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

void gp10b_gr_init_get_supported_preemption_modes(
	u32 *graphics_preemption_mode_flags, u32 *compute_preemption_mode_flags)
{
	*graphics_preemption_mode_flags = (NVGPU_PREEMPTION_MODE_GRAPHICS_WFI |
					  NVGPU_PREEMPTION_MODE_GRAPHICS_GFXP);
	*compute_preemption_mode_flags = (NVGPU_PREEMPTION_MODE_COMPUTE_WFI |
					 NVGPU_PREEMPTION_MODE_COMPUTE_CTA |
					 NVGPU_PREEMPTION_MODE_COMPUTE_CILP);
}

void gp10b_gr_init_get_default_preemption_modes(
	u32 *default_graphics_preempt_mode, u32 *default_compute_preempt_mode)
{
	*default_graphics_preempt_mode = NVGPU_PREEMPTION_MODE_GRAPHICS_WFI;
	*default_compute_preempt_mode = NVGPU_PREEMPTION_MODE_COMPUTE_WFI;
}
