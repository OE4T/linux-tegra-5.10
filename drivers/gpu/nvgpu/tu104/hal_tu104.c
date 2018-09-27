/*
 * TU104 Tegra HAL interface
 *
 * Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
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

#include "common/bus/bus_gk20a.h"
#include "common/bus/bus_gp10b.h"
#include "common/bus/bus_gv100.h"
#include "common/bus/bus_tu104.h"
#include "common/priv_ring/priv_ring_gm20b.h"
#include "common/priv_ring/priv_ring_gp10b.h"
#include "common/clock_gating/tu104_gating_reglist.h"
#include "common/ptimer/ptimer_gk20a.h"
#include "common/fb/fb_gm20b.h"
#include "common/fb/fb_gp10b.h"
#include "common/fb/fb_gp106.h"
#include "common/fb/fb_gv11b.h"
#include "common/fb/fb_gv100.h"
#include "common/fb/fb_tu104.h"
#include "common/xve/xve_gp106.h"
#include "common/netlist/netlist_tu104.h"
#include "common/therm/therm_gm20b.h"
#include "common/therm/therm_gp10b.h"
#include "common/therm/therm_gp106.h"
#include "common/therm/therm_gv11b.h"
#include "common/ltc/ltc_gm20b.h"
#include "common/ltc/ltc_gp10b.h"
#include "common/ltc/ltc_gv11b.h"
#include "common/ltc/ltc_tu104.h"
#include "common/fuse/fuse_gm20b.h"
#include "common/fuse/fuse_gp10b.h"
#include "common/fuse/fuse_gp106.h"
#include "common/mc/mc_gm20b.h"
#include "common/mc/mc_gp10b.h"
#include "common/mc/mc_gv11b.h"
#include "common/mc/mc_gv100.h"
#include "common/mc/mc_tu104.h"
#include "common/perf/perf_gv11b.h"

#include "gk20a/fifo_gk20a.h"
#include "gk20a/fecs_trace_gk20a.h"
#include "gk20a/flcn_gk20a.h"
#include "gk20a/regops_gk20a.h"
#include "gk20a/mm_gk20a.h"
#include "gk20a/pmu_gk20a.h"
#include "gk20a/gr_gk20a.h"

#include "gm20b/gr_gm20b.h"
#include "gm20b/fifo_gm20b.h"
#include "gm20b/mm_gm20b.h"
#include "gm20b/pmu_gm20b.h"
#include "gm20b/acr_gm20b.h"

#include "gv100/clk_gv100.h"
#include "gp106/clk_arb_gp106.h"
#include "gp106/pmu_gp106.h"
#include "gp106/acr_gp106.h"
#include "gp106/sec2_gp106.h"
#include "gp106/bios_gp106.h"
#include "gp106/flcn_gp106.h"

#include "gp10b/gr_gp10b.h"
#include "gp10b/ce_gp10b.h"
#include "gp10b/fifo_gp10b.h"
#include "gp10b/fecs_trace_gp10b.h"
#include "gp10b/mm_gp10b.h"
#include "gp10b/pmu_gp10b.h"

#include "gv11b/hal_gv11b.h"
#include "gv11b/gr_gv11b.h"
#include "gv11b/gv11b.h"
#include "gv11b/ce_gv11b.h"
#include "gv11b/mm_gv11b.h"
#include "gv11b/pmu_gv11b.h"
#include "gv11b/pmu_gv11b.h"
#include "gv11b/fifo_gv11b.h"
#include "gv11b/regops_gv11b.h"
#include "gv11b/subctx_gv11b.h"

#include "gv100/bios_gv100.h"
#include "gv100/fifo_gv100.h"
#include "gv100/flcn_gv100.h"
#include "gv100/gr_gv100.h"
#include "gv100/mm_gv100.h"
#include "gv100/pmu_gv100.h"
#include "gv100/nvlink_gv100.h"
#include "gv100/regops_gv100.h"

#include "tu104/fifo_tu104.h"
#include "tu104/gr_tu104.h"
#include "tu104/bios_tu104.h"
#include "tu104/fbpa_tu104.h"
#include "tu104/sec2_tu104.h"
#include "tu104/flcn_tu104.h"
#include "tu104/nvlink_tu104.h"
#include "tu104/ecc_tu104.h"
#include "tu104/hal_tu104.h"
#include "tu104/regops_tu104.h"
#include "tu104/acr_tu104.h"

#include <nvgpu/ptimer.h>
#include <nvgpu/debug.h>
#include <nvgpu/enabled.h>
#include <nvgpu/ctxsw_trace.h>
#include <nvgpu/error_notifier.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/clk_arb.h>
#include <nvgpu/debugger.h>
#include <nvgpu/channel.h>
#include <nvgpu/perfbuf.h>
#include <nvgpu/cyclestats_snapshot.h>

#include <nvgpu/hw/tu104/hw_proj_tu104.h>
#include <nvgpu/hw/tu104/hw_fifo_tu104.h>
#include <nvgpu/hw/tu104/hw_ram_tu104.h>
#include <nvgpu/hw/tu104/hw_top_tu104.h>
#include <nvgpu/hw/tu104/hw_pram_tu104.h>
#include <nvgpu/hw/tu104/hw_pwr_tu104.h>
#include <nvgpu/hw/tu104/hw_gr_tu104.h>

static u32 tu104_get_litter_value(struct gk20a *g, int value)
{
	u32 ret = 0;
	switch (value) {
	case GPU_LIT_NUM_GPCS:
		ret = proj_scal_litter_num_gpcs_v();
		break;
	case GPU_LIT_NUM_PES_PER_GPC:
		ret = proj_scal_litter_num_pes_per_gpc_v();
		break;
	case GPU_LIT_NUM_ZCULL_BANKS:
		ret = proj_scal_litter_num_zcull_banks_v();
		break;
	case GPU_LIT_NUM_TPC_PER_GPC:
		ret = proj_scal_litter_num_tpc_per_gpc_v();
		break;
	case GPU_LIT_NUM_SM_PER_TPC:
		ret = proj_scal_litter_num_sm_per_tpc_v();
		break;
	case GPU_LIT_NUM_FBPS:
		ret = proj_scal_litter_num_fbps_v();
		break;
	case GPU_LIT_GPC_BASE:
		ret = proj_gpc_base_v();
		break;
	case GPU_LIT_GPC_STRIDE:
		ret = proj_gpc_stride_v();
		break;
	case GPU_LIT_GPC_SHARED_BASE:
		ret = proj_gpc_shared_base_v();
		break;
	case GPU_LIT_TPC_IN_GPC_BASE:
		ret = proj_tpc_in_gpc_base_v();
		break;
	case GPU_LIT_TPC_IN_GPC_STRIDE:
		ret = proj_tpc_in_gpc_stride_v();
		break;
	case GPU_LIT_TPC_IN_GPC_SHARED_BASE:
		ret = proj_tpc_in_gpc_shared_base_v();
		break;
	case GPU_LIT_PPC_IN_GPC_BASE:
		ret = proj_ppc_in_gpc_base_v();
		break;
	case GPU_LIT_PPC_IN_GPC_STRIDE:
		ret = proj_ppc_in_gpc_stride_v();
		break;
	case GPU_LIT_PPC_IN_GPC_SHARED_BASE:
		ret = proj_ppc_in_gpc_shared_base_v();
		break;
	case GPU_LIT_ROP_BASE:
		ret = proj_rop_base_v();
		break;
	case GPU_LIT_ROP_STRIDE:
		ret = proj_rop_stride_v();
		break;
	case GPU_LIT_ROP_SHARED_BASE:
		ret = proj_rop_shared_base_v();
		break;
	case GPU_LIT_HOST_NUM_ENGINES:
		ret = proj_host_num_engines_v();
		break;
	case GPU_LIT_HOST_NUM_PBDMA:
		ret = proj_host_num_pbdma_v();
		break;
	case GPU_LIT_LTC_STRIDE:
		ret = proj_ltc_stride_v();
		break;
	case GPU_LIT_LTS_STRIDE:
		ret = proj_lts_stride_v();
		break;
	case GPU_LIT_NUM_FBPAS:
		ret = proj_scal_litter_num_fbpas_v();
		break;
	case GPU_LIT_FBPA_SHARED_BASE:
		ret = proj_fbpa_shared_base_v();
		break;
	case GPU_LIT_FBPA_BASE:
		ret = proj_fbpa_base_v();
		break;
	case GPU_LIT_FBPA_STRIDE:
		ret = proj_fbpa_stride_v();
		break;
	case GPU_LIT_SM_PRI_STRIDE:
		ret = proj_sm_stride_v();
		break;
	case GPU_LIT_SMPC_PRI_BASE:
		ret = proj_smpc_base_v();
		break;
	case GPU_LIT_SMPC_PRI_SHARED_BASE:
		ret = proj_smpc_shared_base_v();
		break;
	case GPU_LIT_SMPC_PRI_UNIQUE_BASE:
		ret = proj_smpc_unique_base_v();
		break;
	case GPU_LIT_SMPC_PRI_STRIDE:
		ret = proj_smpc_stride_v();
		break;
	case GPU_LIT_TWOD_CLASS:
		ret = FERMI_TWOD_A;
		break;
	case GPU_LIT_THREED_CLASS:
		ret = TURING_A;
		break;
	case GPU_LIT_COMPUTE_CLASS:
		ret = TURING_COMPUTE_A;
		break;
	case GPU_LIT_GPFIFO_CLASS:
		ret = TURING_CHANNEL_GPFIFO_A;
		break;
	case GPU_LIT_I2M_CLASS:
		ret = KEPLER_INLINE_TO_MEMORY_B;
		break;
	case GPU_LIT_DMA_COPY_CLASS:
		ret = TURING_DMA_COPY_A;
		break;
	case GPU_LIT_GPC_PRIV_STRIDE:
		ret = proj_gpc_priv_stride_v();
		break;
	case GPU_LIT_PERFMON_PMMGPCTPCA_DOMAIN_START:
		ret = 2;
		break;
	case GPU_LIT_PERFMON_PMMGPCTPCB_DOMAIN_START:
		ret = 8;
		break;
	case GPU_LIT_PERFMON_PMMGPCTPC_DOMAIN_COUNT:
		ret = 6;
		break;
	case GPU_LIT_PERFMON_PMMFBP_LTC_DOMAIN_START:
		ret = 2;
		break;
	case GPU_LIT_PERFMON_PMMFBP_LTC_DOMAIN_COUNT:
		ret = 8;
		break;
	case GPU_LIT_PERFMON_PMMFBP_ROP_DOMAIN_START:
		ret = 10;
		break;
	case GPU_LIT_PERFMON_PMMFBP_ROP_DOMAIN_COUNT:
		ret = 2;
		break;
	default:
		nvgpu_err(g, "Missing definition %d", value);
		BUG();
		break;
	}

	return ret;
}

static int tu104_init_gpu_characteristics(struct gk20a *g)
{
	int err;

	err = gk20a_init_gpu_characteristics(g);
	if (err != 0) {
		return err;
	}

	__nvgpu_set_enabled(g, NVGPU_SUPPORT_TSG_SUBCONTEXTS, true);
	__nvgpu_set_enabled(g, NVGPU_SUPPORT_GET_TEMPERATURE, true);
	if (nvgpu_has_syncpoints(g)) {
		__nvgpu_set_enabled(g, NVGPU_SUPPORT_SYNCPOINT_ADDRESS, true);
		__nvgpu_set_enabled(g, NVGPU_SUPPORT_USER_SYNCPOINT, true);
	}
	__nvgpu_set_enabled(g, NVGPU_SUPPORT_USERMODE_SUBMIT, true);

	return 0;
}



static const struct gpu_ops tu104_ops = {
	.bios = {
		.init = gp106_bios_init,
		.preos_wait_for_halt = NULL,
		.preos_reload_check = NULL,
		.devinit = NULL,
		.preos = NULL,
		.verify_devinit = tu104_bios_verify_devinit,
	},
	.ltc = {
		.determine_L2_size_bytes = gp10b_determine_L2_size_bytes,
		.set_zbc_s_entry = gv11b_ltc_set_zbc_stencil_entry,
		.set_zbc_color_entry = gm20b_ltc_set_zbc_color_entry,
		.set_zbc_depth_entry = gm20b_ltc_set_zbc_depth_entry,
		.init_cbc = NULL,
		.get_cbc_base_divisor = ltc_tu104_get_cbc_base_divisor,
		.init_fs_state = ltc_tu104_init_fs_state,
		.init_comptags = ltc_tu104_init_comptags,
		.cbc_ctrl = ltc_tu104_cbc_ctrl,
		.isr = gv11b_ltc_isr,
		.cbc_fix_config = NULL,
		.flush = gm20b_flush_ltc,
		.set_enabled = gp10b_ltc_set_enabled,
		.intr_en_illegal_compstat = gv11b_ltc_intr_en_illegal_compstat,
		.pri_is_ltc_addr = gm20b_ltc_pri_is_ltc_addr,
		.is_ltcs_ltss_addr = gm20b_ltc_is_ltcs_ltss_addr,
		.is_ltcn_ltss_addr = gm20b_ltc_is_ltcn_ltss_addr,
		.split_lts_broadcast_addr = gm20b_ltc_split_lts_broadcast_addr,
		.split_ltc_broadcast_addr = gm20b_ltc_split_ltc_broadcast_addr,
	},
	.ce2 = {
		.isr_stall = gv11b_ce_isr,
		.isr_nonstall = NULL,
		.get_num_pce = gv11b_ce_get_num_pce,
	},
	.gr = {
		.get_patch_slots = gr_gv100_get_patch_slots,
		.init_gpc_mmu = gr_gv11b_init_gpc_mmu,
		.bundle_cb_defaults = gr_tu104_bundle_cb_defaults,
		.cb_size_default = gr_tu104_cb_size_default,
		.calc_global_ctx_buffer_size =
			gr_gv11b_calc_global_ctx_buffer_size,
		.commit_global_attrib_cb = gr_gv11b_commit_global_attrib_cb,
		.commit_global_bundle_cb = gr_gp10b_commit_global_bundle_cb,
		.commit_global_cb_manager = gr_gp10b_commit_global_cb_manager,
		.commit_global_pagepool = gr_gp10b_commit_global_pagepool,
		.handle_sw_method = gr_tu104_handle_sw_method,
		.set_alpha_circular_buffer_size =
			gr_gv11b_set_alpha_circular_buffer_size,
		.set_circular_buffer_size = gr_gv11b_set_circular_buffer_size,
		.enable_hww_exceptions = gr_gv11b_enable_hww_exceptions,
		.is_valid_class = gr_tu104_is_valid_class,
		.is_valid_gfx_class = gr_tu104_is_valid_gfx_class,
		.is_valid_compute_class = gr_tu104_is_valid_compute_class,
		.get_sm_dsm_perf_regs = gv11b_gr_get_sm_dsm_perf_regs,
		.get_sm_dsm_perf_ctrl_regs = gr_tu104_get_sm_dsm_perf_ctrl_regs,
		.init_fs_state = gr_gv11b_init_fs_state,
		.set_hww_esr_report_mask = gv11b_gr_set_hww_esr_report_mask,
		.falcon_load_ucode = gr_gm20b_load_ctxsw_ucode_segments,
		.load_ctxsw_ucode = gr_gm20b_load_ctxsw_ucode,
		.set_gpc_tpc_mask = gr_gv100_set_gpc_tpc_mask,
		.get_gpc_tpc_mask = gr_gm20b_get_gpc_tpc_mask,
		.get_gpc_mask = gr_gm20b_get_gpc_mask,
		.alloc_obj_ctx = gk20a_alloc_obj_ctx,
		.bind_ctxsw_zcull = gr_gk20a_bind_ctxsw_zcull,
		.get_zcull_info = gr_gk20a_get_zcull_info,
		.is_tpc_addr = gr_gm20b_is_tpc_addr,
		.get_tpc_num = gr_gm20b_get_tpc_num,
		.detect_sm_arch = gr_gv11b_detect_sm_arch,
		.add_zbc_color = gr_gp10b_add_zbc_color,
		.add_zbc_depth = gr_gp10b_add_zbc_depth,
		.get_gpcs_swdx_dss_zbc_c_format_reg =
			gr_gv11b_get_gpcs_swdx_dss_zbc_c_format_reg,
		.get_gpcs_swdx_dss_zbc_z_format_reg =
			gr_gv11b_get_gpcs_swdx_dss_zbc_z_format_reg,
		.zbc_set_table = gk20a_gr_zbc_set_table,
		.zbc_query_table = gr_gk20a_query_zbc,
		.pmu_save_zbc = gk20a_pmu_save_zbc,
		.add_zbc = gr_gk20a_add_zbc,
		.pagepool_default_size = gr_gv11b_pagepool_default_size,
		.init_ctx_state = gr_gp10b_init_ctx_state,
		.alloc_gr_ctx = gr_gp10b_alloc_gr_ctx,
		.free_gr_ctx = gr_gk20a_free_gr_ctx,
		.update_ctxsw_preemption_mode =
			gr_gp10b_update_ctxsw_preemption_mode,
		.dump_gr_regs = gr_gv11b_dump_gr_status_regs,
		.update_pc_sampling = gr_gm20b_update_pc_sampling,
		.get_fbp_en_mask = gr_gm20b_get_fbp_en_mask,
		.get_max_ltc_per_fbp = gr_gm20b_get_max_ltc_per_fbp,
		.get_max_lts_per_ltc = gr_gm20b_get_max_lts_per_ltc,
		.get_rop_l2_en_mask = gr_gm20b_rop_l2_en_mask,
		.get_max_fbps_count = gr_gm20b_get_max_fbps_count,
		.init_sm_dsm_reg_info = gr_tu104_init_sm_dsm_reg_info,
		.wait_empty = gr_gv11b_wait_empty,
		.init_cyclestats = gr_gm20b_init_cyclestats,
		.set_sm_debug_mode = gv11b_gr_set_sm_debug_mode,
		.enable_cde_in_fecs = gr_gm20b_enable_cde_in_fecs,
		.bpt_reg_info = gv11b_gr_bpt_reg_info,
		.get_access_map = gr_gv11b_get_access_map,
		.handle_fecs_error = gr_gv11b_handle_fecs_error,
		.handle_sm_exception = gr_gk20a_handle_sm_exception,
		.handle_tex_exception = NULL,
		.enable_gpc_exceptions = gr_tu104_enable_gpc_exceptions,
		.enable_exceptions = gr_gv11b_enable_exceptions,
		.get_lrf_tex_ltc_dram_override = get_ecc_override_val,
		.update_smpc_ctxsw_mode = gr_gk20a_update_smpc_ctxsw_mode,
		.get_hw_accessor_stream_out_mode =
			gr_gv100_get_hw_accessor_stream_out_mode,
		.get_num_hwpm_perfmon = gr_gv100_get_num_hwpm_perfmon,
		.set_pmm_register = gr_gv100_set_pmm_register,
		.update_hwpm_ctxsw_mode = gr_gk20a_update_hwpm_ctxsw_mode,
		.init_hwpm_pmm_register = gr_gv100_init_hwpm_pmm_register,
		.record_sm_error_state = gv11b_gr_record_sm_error_state,
		.clear_sm_error_state = gv11b_gr_clear_sm_error_state,
		.suspend_contexts = gr_gp10b_suspend_contexts,
		.resume_contexts = gr_gk20a_resume_contexts,
		.get_preemption_mode_flags = gr_tu104_get_preemption_mode_flags,
		.init_sm_id_table = gr_gv100_init_sm_id_table,
		.load_smid_config = gr_gv11b_load_smid_config,
		.program_sm_id_numbering = gr_gv11b_program_sm_id_numbering,
		.setup_rop_mapping = gr_gv11b_setup_rop_mapping,
		.program_zcull_mapping = gr_gv11b_program_zcull_mapping,
		.commit_global_timeslice = gr_gv11b_commit_global_timeslice,
		.commit_inst = gr_gv11b_commit_inst,
		.write_zcull_ptr = gr_gv11b_write_zcull_ptr,
		.write_pm_ptr = gr_gv11b_write_pm_ptr,
		.load_tpc_mask = gr_gv11b_load_tpc_mask,
		.trigger_suspend = gv11b_gr_sm_trigger_suspend,
		.wait_for_pause = gr_gk20a_wait_for_pause,
		.resume_from_pause = gv11b_gr_resume_from_pause,
		.clear_sm_errors = gr_gk20a_clear_sm_errors,
		.tpc_enabled_exceptions = gr_gk20a_tpc_enabled_exceptions,
		.get_esr_sm_sel = gv11b_gr_get_esr_sm_sel,
		.sm_debugger_attached = gv11b_gr_sm_debugger_attached,
		.suspend_single_sm = gv11b_gr_suspend_single_sm,
		.suspend_all_sms = gv11b_gr_suspend_all_sms,
		.resume_single_sm = gv11b_gr_resume_single_sm,
		.resume_all_sms = gv11b_gr_resume_all_sms,
		.get_sm_hww_warp_esr = gv11b_gr_get_sm_hww_warp_esr,
		.get_sm_hww_global_esr = gv11b_gr_get_sm_hww_global_esr,
		.get_sm_no_lock_down_hww_global_esr_mask =
			gv11b_gr_get_sm_no_lock_down_hww_global_esr_mask,
		.lock_down_sm = gv11b_gr_lock_down_sm,
		.wait_for_sm_lock_down = gv11b_gr_wait_for_sm_lock_down,
		.clear_sm_hww = gv11b_gr_clear_sm_hww,
		.init_ovr_sm_dsm_perf =  gv11b_gr_init_ovr_sm_dsm_perf,
		.get_ovr_perf_regs = gv11b_gr_get_ovr_perf_regs,
		.disable_rd_coalesce = gm20a_gr_disable_rd_coalesce,
		.set_boosted_ctx = gr_gp10b_set_boosted_ctx,
		.set_preemption_mode = gr_gp10b_set_preemption_mode,
		.set_czf_bypass = NULL,
		.pre_process_sm_exception = gr_gv11b_pre_process_sm_exception,
		.set_preemption_buffer_va = gr_gv11b_set_preemption_buffer_va,
		.init_preemption_state = NULL,
		.update_boosted_ctx = gr_gp10b_update_boosted_ctx,
		.set_bes_crop_debug3 = gr_gp10b_set_bes_crop_debug3,
		.set_bes_crop_debug4 = gr_gp10b_set_bes_crop_debug4,
		.init_ecc = tu104_ecc_init,
		.set_ctxsw_preemption_mode = gr_gp10b_set_ctxsw_preemption_mode,
		.is_etpc_addr = gv11b_gr_pri_is_etpc_addr,
		.egpc_etpc_priv_addr_table = gv11b_gr_egpc_etpc_priv_addr_table,
		.handle_tpc_mpc_exception = gr_gv11b_handle_tpc_mpc_exception,
		.zbc_s_query_table = gr_gv11b_zbc_s_query_table,
		.load_zbc_s_default_tbl = gr_gv11b_load_stencil_default_tbl,
		.handle_gpc_gpcmmu_exception =
			gr_gv11b_handle_gpc_gpcmmu_exception,
		.add_zbc_type_s = gr_gv11b_add_zbc_type_s,
		.get_egpc_base = gv11b_gr_get_egpc_base,
		.get_egpc_etpc_num = gv11b_gr_get_egpc_etpc_num,
		.handle_gpc_gpccs_exception =
			gr_gv11b_handle_gpc_gpccs_exception,
		.load_zbc_s_tbl = gr_gv11b_load_stencil_tbl,
		.access_smpc_reg = gv11b_gr_access_smpc_reg,
		.is_egpc_addr = gv11b_gr_pri_is_egpc_addr,
		.add_zbc_s = gr_gv11b_add_zbc_stencil,
		.handle_gcc_exception = gr_gv11b_handle_gcc_exception,
		.init_sw_veid_bundle = gr_gv11b_init_sw_veid_bundle,
		.handle_tpc_sm_ecc_exception =
			gr_gv11b_handle_tpc_sm_ecc_exception,
		.decode_egpc_addr = gv11b_gr_decode_egpc_addr,
		.fecs_host_int_enable = gr_gv11b_fecs_host_int_enable,
		.handle_ssync_hww = gr_gv11b_handle_ssync_hww,
		.handle_notify_pending = gk20a_gr_handle_notify_pending,
		.handle_semaphore_pending = gk20a_gr_handle_semaphore_pending,
		.add_ctxsw_reg_pm_fbpa = gr_gv100_add_ctxsw_reg_pm_fbpa,
		.add_ctxsw_reg_perf_pma = gr_gv100_add_ctxsw_reg_perf_pma,
		.decode_priv_addr = gr_gv11b_decode_priv_addr,
		.create_priv_addr_table = gr_gv11b_create_priv_addr_table,
		.get_pmm_per_chiplet_offset =
			gr_gv11b_get_pmm_per_chiplet_offset,
		.split_fbpa_broadcast_addr = gr_gv100_split_fbpa_broadcast_addr,
		.init_sw_bundle64 = gr_tu104_init_sw_bundle64,
		.fecs_ctxsw_mailbox_size = gr_fecs_ctxsw_mailbox__size_1_v,
		.alloc_global_ctx_buffers = gr_tu104_alloc_global_ctx_buffers,
		.map_global_ctx_buffers = gr_tu104_map_global_ctx_buffers,
		.commit_global_ctx_buffers = gr_tu104_commit_global_ctx_buffers,
		.get_nonpes_aware_tpc = gr_gv11b_get_nonpes_aware_tpc,
		.get_offset_in_gpccs_segment =
			gr_tu104_get_offset_in_gpccs_segment,
		.set_debug_mode = gm20b_gr_set_debug_mode,
		.dump_gr_falcon_stats = gk20a_fecs_dump_falcon_stats,
		.get_fecs_ctx_state_store_major_rev_id =
			gk20a_gr_get_fecs_ctx_state_store_major_rev_id,
	},
	.fb = {
		.init_hw = gv11b_fb_init_hw,
		.init_fs_state = gp106_fb_init_fs_state,
		.init_cbc = fb_tu104_init_cbc,
		.set_mmu_page_size = NULL,
		.set_use_full_comp_tag_line =
			gm20b_fb_set_use_full_comp_tag_line,
		.mmu_ctrl = gm20b_fb_mmu_ctrl,
		.mmu_debug_ctrl = gm20b_fb_mmu_debug_ctrl,
		.mmu_debug_wr = gm20b_fb_mmu_debug_wr,
		.mmu_debug_rd = gm20b_fb_mmu_debug_rd,
		.compression_page_size = gp10b_fb_compression_page_size,
		.compressible_page_size = gp10b_fb_compressible_page_size,
		.compression_align_mask = gm20b_fb_compression_align_mask,
		.vpr_info_fetch = NULL,
		.dump_vpr_info = NULL,
		.dump_wpr_info = gm20b_fb_dump_wpr_info,
		.read_wpr_info = gm20b_fb_read_wpr_info,
		.is_debug_mode_enabled = gm20b_fb_debug_mode_enabled,
		.set_debug_mode = gm20b_fb_set_debug_mode,
		.tlb_invalidate = fb_tu104_tlb_invalidate,
		.hub_isr = tu104_fb_hub_isr,
		.handle_replayable_fault = gv11b_fb_handle_replayable_mmu_fault,
		.mem_unlock = gv100_fb_memory_unlock,
		.init_nvlink = gv100_fb_init_nvlink,
		.enable_nvlink = gv100_fb_enable_nvlink,
		.enable_hub_intr = tu104_fb_enable_hub_intr,
		.disable_hub_intr = tu104_fb_disable_hub_intr,
		.init_fbpa = tu104_fbpa_init,
		.handle_fbpa_intr = tu104_fbpa_handle_intr,
		.write_mmu_fault_buffer_lo_hi =
				fb_tu104_write_mmu_fault_buffer_lo_hi,
		.write_mmu_fault_buffer_get =
				fb_tu104_write_mmu_fault_buffer_get,
		.write_mmu_fault_buffer_size =
				fb_tu104_write_mmu_fault_buffer_size,
		.write_mmu_fault_status = fb_tu104_write_mmu_fault_status,
		.read_mmu_fault_buffer_get =
				fb_tu104_read_mmu_fault_buffer_get,
		.read_mmu_fault_buffer_put =
				fb_tu104_read_mmu_fault_buffer_put,
		.read_mmu_fault_buffer_size =
				fb_tu104_read_mmu_fault_buffer_size,
		.read_mmu_fault_addr_lo_hi = fb_tu104_read_mmu_fault_addr_lo_hi,
		.read_mmu_fault_inst_lo_hi = fb_tu104_read_mmu_fault_inst_lo_hi,
		.read_mmu_fault_info = fb_tu104_read_mmu_fault_info,
		.read_mmu_fault_status = fb_tu104_read_mmu_fault_status,
		.mmu_invalidate_replay = fb_tu104_mmu_invalidate_replay,
		.mmu_fault_pending = tu104_fb_mmu_fault_pending,
		.is_fault_buf_enabled = gv11b_fb_is_fault_buf_enabled,
		.fault_buf_set_state_hw = gv11b_fb_fault_buf_set_state_hw,
		.fault_buf_configure_hw = gv11b_fb_fault_buf_configure_hw,
		.get_vidmem_size = gv100_fb_get_vidmem_size,
		.apply_pdb_cache_war = tu104_fb_apply_pdb_cache_war,
	},
	.clock_gating = {
		.slcg_bus_load_gating_prod =
			tu104_slcg_bus_load_gating_prod,
		.slcg_ce2_load_gating_prod =
			tu104_slcg_ce2_load_gating_prod,
		.slcg_chiplet_load_gating_prod =
			tu104_slcg_chiplet_load_gating_prod,
		.slcg_ctxsw_firmware_load_gating_prod =
			tu104_slcg_ctxsw_firmware_load_gating_prod,
		.slcg_fb_load_gating_prod =
			tu104_slcg_fb_load_gating_prod,
		.slcg_fifo_load_gating_prod =
			tu104_slcg_fifo_load_gating_prod,
		.slcg_gr_load_gating_prod =
			gr_tu104_slcg_gr_load_gating_prod,
		.slcg_ltc_load_gating_prod =
			ltc_tu104_slcg_ltc_load_gating_prod,
		.slcg_perf_load_gating_prod =
			tu104_slcg_perf_load_gating_prod,
		.slcg_priring_load_gating_prod =
			tu104_slcg_priring_load_gating_prod,
		.slcg_pmu_load_gating_prod =
			tu104_slcg_pmu_load_gating_prod,
		.slcg_therm_load_gating_prod =
			tu104_slcg_therm_load_gating_prod,
		.slcg_xbar_load_gating_prod =
			tu104_slcg_xbar_load_gating_prod,
		.blcg_bus_load_gating_prod =
			tu104_blcg_bus_load_gating_prod,
		.blcg_ce_load_gating_prod =
			tu104_blcg_ce_load_gating_prod,
		.blcg_ctxsw_firmware_load_gating_prod =
			tu104_blcg_ctxsw_firmware_load_gating_prod,
		.blcg_fb_load_gating_prod =
			tu104_blcg_fb_load_gating_prod,
		.blcg_fifo_load_gating_prod =
			tu104_blcg_fifo_load_gating_prod,
		.blcg_gr_load_gating_prod =
			tu104_blcg_gr_load_gating_prod,
		.blcg_ltc_load_gating_prod =
			tu104_blcg_ltc_load_gating_prod,
		.blcg_pwr_csb_load_gating_prod =
			tu104_blcg_pwr_csb_load_gating_prod,
		.blcg_pmu_load_gating_prod =
			tu104_blcg_pmu_load_gating_prod,
		.blcg_xbar_load_gating_prod =
			tu104_blcg_xbar_load_gating_prod,
	},
	.fifo = {
		.get_preempt_timeout = gv100_fifo_get_preempt_timeout,
		.init_fifo_setup_hw = tu104_init_fifo_setup_hw,
		.bind_channel = channel_gm20b_bind,
		.unbind_channel = channel_gv11b_unbind,
		.disable_channel = gk20a_fifo_disable_channel,
		.enable_channel = gk20a_fifo_enable_channel,
		.alloc_inst = gk20a_fifo_alloc_inst,
		.free_inst = gk20a_fifo_free_inst,
		.setup_ramfc = channel_tu104_setup_ramfc,
		.default_timeslice_us = gk20a_fifo_default_timeslice_us,
		.setup_userd = gk20a_fifo_setup_userd,
		.userd_gp_get = gv11b_userd_gp_get,
		.userd_gp_put = gv11b_userd_gp_put,
		.userd_pb_get = gv11b_userd_pb_get,
		.pbdma_acquire_val = gk20a_fifo_pbdma_acquire_val,
		.preempt_channel = gv11b_fifo_preempt_channel,
		.preempt_tsg = gv11b_fifo_preempt_tsg,
		.enable_tsg = gv11b_fifo_enable_tsg,
		.disable_tsg = gk20a_disable_tsg,
		.tsg_verify_channel_status = gk20a_fifo_tsg_unbind_channel_verify_status,
		.tsg_verify_status_ctx_reload = gm20b_fifo_tsg_verify_status_ctx_reload,
		.tsg_verify_status_faulted = gv11b_fifo_tsg_verify_status_faulted,
		.update_runlist = gk20a_fifo_update_runlist,
		.trigger_mmu_fault = NULL,
		.get_mmu_fault_info = NULL,
		.get_mmu_fault_desc = NULL,
		.get_mmu_fault_client_desc = NULL,
		.get_mmu_fault_gpc_desc = NULL,
		.wait_engine_idle = gk20a_fifo_wait_engine_idle,
		.get_num_fifos = gv100_fifo_get_num_fifos,
		.get_pbdma_signature = gp10b_fifo_get_pbdma_signature,
		.set_runlist_interleave = gk20a_fifo_set_runlist_interleave,
		.tsg_set_timeslice = gk20a_fifo_tsg_set_timeslice,
		.force_reset_ch = gk20a_fifo_force_reset_ch,
		.engine_enum_from_type = gp10b_fifo_engine_enum_from_type,
		.device_info_data_parse = gp10b_device_info_data_parse,
		.eng_runlist_base_size = fifo_runlist_base_lo__size_1_v,
		.init_engine_info = gk20a_fifo_init_engine_info,
		.runlist_entry_size = ram_rl_entry_size_v,
		.get_tsg_runlist_entry = gv11b_get_tsg_runlist_entry,
		.get_ch_runlist_entry = gv11b_get_ch_runlist_entry,
		.is_fault_engine_subid_gpc = gv11b_is_fault_engine_subid_gpc,
		.dump_pbdma_status = gk20a_dump_pbdma_status,
		.dump_eng_status = gv11b_dump_eng_status,
		.dump_channel_status_ramfc = gv11b_dump_channel_status_ramfc,
		.capture_channel_ram_dump = gv11b_capture_channel_ram_dump,
		.intr_0_error_mask = gv11b_fifo_intr_0_error_mask,
		.is_preempt_pending = gv11b_fifo_is_preempt_pending,
		.init_pbdma_intr_descs = gv11b_fifo_init_pbdma_intr_descs,
		.reset_enable_hw = gv11b_init_fifo_reset_enable_hw,
		.teardown_ch_tsg = gv11b_fifo_teardown_ch_tsg,
		.handle_sched_error = gv11b_fifo_handle_sched_error,
		.handle_pbdma_intr_0 = gv11b_fifo_handle_pbdma_intr_0,
		.handle_pbdma_intr_1 = gv11b_fifo_handle_pbdma_intr_1,
		.init_eng_method_buffers = gv11b_fifo_init_eng_method_buffers,
		.deinit_eng_method_buffers =
			gv11b_fifo_deinit_eng_method_buffers,
		.tsg_bind_channel = gk20a_tsg_bind_channel,
		.tsg_unbind_channel = gk20a_fifo_tsg_unbind_channel,
		.post_event_id = gk20a_tsg_event_id_post_event,
		.ch_abort_clean_up = gk20a_channel_abort_clean_up,
		.check_tsg_ctxsw_timeout = gk20a_fifo_check_tsg_ctxsw_timeout,
		.check_ch_ctxsw_timeout = gk20a_fifo_check_ch_ctxsw_timeout,
		.channel_suspend = gk20a_channel_suspend,
		.channel_resume = gk20a_channel_resume,
		.set_error_notifier = nvgpu_set_error_notifier_if_empty,
		.setup_sw = gk20a_init_fifo_setup_sw,
#ifdef CONFIG_TEGRA_GK20A_NVHOST
		.alloc_syncpt_buf = gv11b_fifo_alloc_syncpt_buf,
		.free_syncpt_buf = gv11b_fifo_free_syncpt_buf,
		.add_syncpt_wait_cmd = gv11b_fifo_add_syncpt_wait_cmd,
		.get_syncpt_wait_cmd_size = gv11b_fifo_get_syncpt_wait_cmd_size,
		.add_syncpt_incr_cmd = gv11b_fifo_add_syncpt_incr_cmd,
		.get_syncpt_incr_cmd_size = gv11b_fifo_get_syncpt_incr_cmd_size,
		.get_syncpt_incr_per_release =
                                gv11b_fifo_get_syncpt_incr_per_release,
		.get_sync_ro_map = gv11b_fifo_get_sync_ro_map,
#endif
		.resetup_ramfc = NULL,
		.device_info_fault_id = top_device_info_data_fault_id_enum_v,
		.free_channel_ctx_header = gv11b_free_subctx_header,
		.handle_ctxsw_timeout = gv11b_fifo_handle_ctxsw_timeout,
		.runlist_hw_submit = tu104_fifo_runlist_hw_submit,
		.runlist_wait_pending = tu104_fifo_runlist_wait_pending,
		.ring_channel_doorbell = tu104_ring_channel_doorbell,
		.get_sema_wait_cmd_size = gv11b_fifo_get_sema_wait_cmd_size,
		.get_sema_incr_cmd_size = gv11b_fifo_get_sema_incr_cmd_size,
		.add_sema_cmd = gv11b_fifo_add_sema_cmd,
		.usermode_base = tu104_fifo_usermode_base,
		.doorbell_token = tu104_fifo_doorbell_token,
		.init_pdb_cache_war = tu104_init_pdb_cache_war,
		.deinit_pdb_cache_war = tu104_deinit_pdb_cache_war,
		.set_sm_exception_type_mask = gk20a_tsg_set_sm_exception_type_mask,
	},
	.netlist = {
		.get_netlist_name = tu104_netlist_get_name,
		.is_fw_defined = tu104_netlist_is_firmware_defined,
	},
#ifdef CONFIG_GK20A_CTXSW_TRACE
	.fecs_trace = {
		.alloc_user_buffer = gk20a_ctxsw_dev_ring_alloc,
		.free_user_buffer = gk20a_ctxsw_dev_ring_free,
		.mmap_user_buffer = gk20a_ctxsw_dev_mmap_buffer,
		.init = gk20a_fecs_trace_init,
		.deinit = gk20a_fecs_trace_deinit,
		.enable = gk20a_fecs_trace_enable,
		.disable = gk20a_fecs_trace_disable,
		.is_enabled = gk20a_fecs_trace_is_enabled,
		.reset = gk20a_fecs_trace_reset,
		.flush = NULL,
		.poll = gk20a_fecs_trace_poll,
		.bind_channel = gk20a_fecs_trace_bind_channel,
		.unbind_channel = gk20a_fecs_trace_unbind_channel,
		.max_entries = gk20a_gr_max_entries,
	},
#endif /* CONFIG_GK20A_CTXSW_TRACE */
	.mm = {
		.support_sparse = gm20b_mm_support_sparse,
		.gmmu_map = gk20a_locked_gmmu_map,
		.gmmu_unmap = gk20a_locked_gmmu_unmap,
		.vm_bind_channel = gk20a_vm_bind_channel,
		.fb_flush = gk20a_mm_fb_flush,
		.l2_invalidate = gk20a_mm_l2_invalidate,
		.l2_flush = gv11b_mm_l2_flush,
		.cbc_clean = gk20a_mm_cbc_clean,
		.set_big_page_size = gm20b_mm_set_big_page_size,
		.get_big_page_sizes = gm20b_mm_get_big_page_sizes,
		.get_default_big_page_size = gp10b_mm_get_default_big_page_size,
		.gpu_phys_addr = gv11b_gpu_phys_addr,
		.get_mmu_levels = gp10b_mm_get_mmu_levels,
		.init_pdb = gp10b_mm_init_pdb,
		.init_mm_setup_hw = gv11b_init_mm_setup_hw,
		.is_bar1_supported = gv11b_mm_is_bar1_supported,
		.alloc_inst_block = gk20a_alloc_inst_block,
		.init_inst_block = gv11b_init_inst_block,
		.mmu_fault_pending = gv11b_mm_mmu_fault_pending,
		.get_kind_invalid = gm20b_get_kind_invalid,
		.get_kind_pitch = gm20b_get_kind_pitch,
		.init_bar2_vm = gp10b_init_bar2_vm,
		.remove_bar2_vm = gp10b_remove_bar2_vm,
		.fault_info_mem_destroy = gv11b_mm_fault_info_mem_destroy,
		.mmu_fault_disable_hw = gv11b_mm_mmu_fault_disable_hw,
		.get_flush_retries = gv100_mm_get_flush_retries,
	},
	.pramin = {
		.data032_r = pram_data032_r,
	},
	.therm = {
		/* PROD values match with H/W INIT values */
		.init_elcg_mode = gv11b_therm_init_elcg_mode,
		.init_blcg_mode = gm20b_therm_init_blcg_mode,
		.elcg_init_idle_filters = NULL,
		.get_internal_sensor_curr_temp =
			gp106_get_internal_sensor_curr_temp,
	},
	.pmu = {
		.init_wpr_region = gv100_pmu_init_acr,
		.load_lsfalcon_ucode = gv100_load_falcon_ucode,
		.is_lazy_bootstrap = gp106_is_lazy_bootstrap,
		.is_priv_load = gp106_is_priv_load,
		.prepare_ucode = gp106_prepare_ucode_blob,
		.pmu_populate_loader_cfg = gp106_pmu_populate_loader_cfg,
		.flcn_populate_bl_dmem_desc = gp106_flcn_populate_bl_dmem_desc,
		.pmu_queue_tail = gk20a_pmu_queue_tail,
		.pmu_get_queue_head = pwr_pmu_queue_head_r,
		.pmu_mutex_release = gk20a_pmu_mutex_release,
		.pmu_is_interrupted = gk20a_pmu_is_interrupted,
		.pmu_isr = gk20a_pmu_isr,
		.pmu_init_perfmon_counter = gk20a_pmu_init_perfmon_counter,
		.pmu_pg_idle_counter_config = gk20a_pmu_pg_idle_counter_config,
		.pmu_read_idle_counter = gk20a_pmu_read_idle_counter,
		.pmu_reset_idle_counter = gk20a_pmu_reset_idle_counter,
		.pmu_dump_elpg_stats = gk20a_pmu_dump_elpg_stats,
		.pmu_dump_falcon_stats = gk20a_pmu_dump_falcon_stats,
		.pmu_enable_irq = gk20a_pmu_enable_irq,
		.is_pmu_supported = gp106_is_pmu_supported,
		.pmu_pg_supported_engines_list = gp106_pmu_pg_engines_list,
		.pmu_elpg_statistics = gp106_pmu_elpg_statistics,
		.pmu_init_perfmon = nvgpu_pmu_init_perfmon,
		.pmu_perfmon_start_sampling = nvgpu_pmu_perfmon_start_sampling,
		.pmu_perfmon_stop_sampling = nvgpu_pmu_perfmon_stop_sampling,
		.pmu_mutex_acquire = gk20a_pmu_mutex_acquire,
		.pmu_is_lpwr_feature_supported =
			gp106_pmu_is_lpwr_feature_supported,
		.pmu_msgq_tail = gk20a_pmu_msgq_tail,
		.pmu_pg_engines_feature_list = gp106_pmu_pg_feature_list,
		.pmu_get_queue_head_size = pwr_pmu_queue_head__size_1_v,
		.pmu_queue_head = gk20a_pmu_queue_head,
		.pmu_pg_param_post_init = nvgpu_lpwr_post_init,
		.pmu_get_queue_tail_size = pwr_pmu_queue_tail__size_1_v,
		.pmu_pg_init_param = gp106_pg_param_init,
		.reset_engine = gp106_pmu_engine_reset,
		.write_dmatrfbase = gp10b_write_dmatrfbase,
		.pmu_mutex_size = pwr_pmu_mutex__size_1_v,
		.is_engine_in_reset = gp106_pmu_is_engine_in_reset,
		.pmu_get_queue_tail = pwr_pmu_queue_tail_r,
		.get_irqdest = gk20a_pmu_get_irqdest,
		.alloc_super_surface = nvgpu_pmu_super_surface_alloc,
		.handle_ext_irq = gv11b_pmu_handle_ext_irq,
		.is_debug_mode_enabled = gm20b_pmu_is_debug_mode_en,
		.update_lspmu_cmdline_args =
			gp106_update_lspmu_cmdline_args,
		.setup_apertures = gp106_pmu_setup_apertures,
		.secured_pmu_start = gm20b_secured_pmu_start,
	},
	.clk = {
		.init_clk_support = gv100_init_clk_support,
		.get_crystal_clk_hz = gv100_crystal_clk_hz,
		.get_rate_cntr = gv100_get_rate_cntr,
		.measure_freq = gv100_clk_measure_freq,
		.suspend_clk_support = gv100_suspend_clk_support,
	},
	.clk_arb = {
		.get_arbiter_clk_domains = gp106_get_arbiter_clk_domains,
		.get_arbiter_clk_range = gp106_get_arbiter_clk_range,
		.get_arbiter_clk_default = gp106_get_arbiter_clk_default,
		.get_current_pstate = nvgpu_clk_arb_get_current_pstate,
	},
	.regops = {
		.exec_regops = exec_regops_gk20a,
		.get_global_whitelist_ranges =
			tu104_get_global_whitelist_ranges,
		.get_global_whitelist_ranges_count =
			tu104_get_global_whitelist_ranges_count,
		.get_context_whitelist_ranges =
			tu104_get_context_whitelist_ranges,
		.get_context_whitelist_ranges_count =
			tu104_get_context_whitelist_ranges_count,
		.get_runcontrol_whitelist = tu104_get_runcontrol_whitelist,
		.get_runcontrol_whitelist_count =
			tu104_get_runcontrol_whitelist_count,
		.get_qctl_whitelist = tu104_get_qctl_whitelist,
		.get_qctl_whitelist_count = tu104_get_qctl_whitelist_count,
	},
	.mc = {
		.intr_enable = intr_tu104_enable,
		.intr_mask = intr_tu104_mask,
		.intr_unit_config = mc_gp10b_intr_unit_config,
		.isr_stall = mc_gp10b_isr_stall,
		.intr_stall = intr_tu104_stall,
		.intr_stall_pause = intr_tu104_stall_pause,
		.intr_stall_resume = intr_tu104_stall_resume,
		.intr_nonstall = intr_tu104_nonstall,
		.intr_nonstall_pause = intr_tu104_nonstall_pause,
		.intr_nonstall_resume = intr_tu104_nonstall_resume,
		.isr_nonstall = intr_tu104_isr_nonstall,
		.enable = gm20b_mc_enable,
		.disable = gm20b_mc_disable,
		.reset = gm20b_mc_reset,
		.is_intr1_pending = NULL,
		.log_pending_intrs = intr_tu104_log_pending_intrs,
		.is_intr_hub_pending = intr_tu104_is_intr_hub_pending,
		.is_intr_nvlink_pending = gv100_mc_is_intr_nvlink_pending,
		.is_stall_and_eng_intr_pending =
					gv100_mc_is_stall_and_eng_intr_pending,
		.fbpa_isr = mc_tu104_fbpa_isr,
		.reset_mask = gv100_mc_reset_mask,
		.is_enabled = gm20b_mc_is_enabled,
		.fb_reset = NULL,
		.ltc_isr = mc_tu104_ltc_isr,
	},
	.debug = {
		.show_dump = gk20a_debug_show_dump,
	},
	.debugger = {
		.post_events = nvgpu_dbg_gpu_post_events,
		.dbg_set_powergate = nvgpu_dbg_set_powergate,
		.check_and_set_global_reservation =
			nvgpu_check_and_set_global_reservation,
		.check_and_set_context_reservation =
			nvgpu_check_and_set_context_reservation,
		.release_profiler_reservation =
			nvgpu_release_profiler_reservation,
	},
	.perf = {
		.enable_membuf = gv11b_perf_enable_membuf,
		.disable_membuf = gv11b_perf_disable_membuf,
		.membuf_reset_streaming = gv11b_perf_membuf_reset_streaming,
		.get_membuf_pending_bytes = gv11b_perf_get_membuf_pending_bytes,
		.set_membuf_handled_bytes = gv11b_perf_set_membuf_handled_bytes,
		.get_membuf_overflow_status =
			gv11b_perf_get_membuf_overflow_status,
	},
	.perfbuf = {
		.perfbuf_enable = nvgpu_perfbuf_enable_locked,
		.perfbuf_disable = nvgpu_perfbuf_disable_locked,
	},
	.bus = {
		.init_hw = gk20a_bus_init_hw,
		.isr = gk20a_bus_isr,
		.bar1_bind = NULL,
		.bar2_bind = bus_tu104_bar2_bind,
		.set_bar0_window = gk20a_bus_set_bar0_window,
		.read_sw_scratch = gv100_bus_read_sw_scratch,
		.write_sw_scratch = gv100_bus_write_sw_scratch,
	},
	.ptimer = {
		.isr = gk20a_ptimer_isr,
		.read_ptimer = gk20a_read_ptimer,
		.get_timestamps_zipper = nvgpu_get_timestamps_zipper,
	},
#if defined(CONFIG_GK20A_CYCLE_STATS)
	.css = {
		.enable_snapshot = nvgpu_css_enable_snapshot,
		.disable_snapshot = nvgpu_css_disable_snapshot,
		.check_data_available = nvgpu_css_check_data_available,
		.set_handled_snapshots = nvgpu_css_set_handled_snapshots,
		.allocate_perfmon_ids = nvgpu_css_allocate_perfmon_ids,
		.release_perfmon_ids = nvgpu_css_release_perfmon_ids,
		.get_overflow_status = nvgpu_css_get_overflow_status,
		.get_pending_snapshots = nvgpu_css_get_pending_snapshots,
	},
#endif
	.xve = {
		.get_speed        = xve_get_speed_gp106,
		.xve_readl        = xve_xve_readl_gp106,
		.xve_writel       = xve_xve_writel_gp106,
		.disable_aspm     = xve_disable_aspm_gp106,
		.reset_gpu        = xve_reset_gpu_gp106,
#if defined(CONFIG_PCI_MSI)
		.rearm_msi        = xve_rearm_msi_gp106,
#endif
		.enable_shadow_rom = NULL,
		.disable_shadow_rom = NULL,
	},
	.falcon = {
		.falcon_hal_sw_init = tu104_falcon_hal_sw_init,
	},
	.priv_ring = {
		.enable_priv_ring = gm20b_priv_ring_enable,
		.isr = gp10b_priv_ring_isr,
		.decode_error_code = gp10b_priv_ring_decode_error_code,
		.set_ppriv_timeout_settings = NULL,
		.enum_ltc = gm20b_priv_ring_enum_ltc,
	},
	.fuse = {
		.is_opt_ecc_enable = gp10b_fuse_is_opt_ecc_enable,
		.is_opt_feature_override_disable =
			gp10b_fuse_is_opt_feature_override_disable,
		.fuse_status_opt_fbio = gm20b_fuse_status_opt_fbio,
		.fuse_status_opt_fbp = gm20b_fuse_status_opt_fbp,
		.fuse_status_opt_rop_l2_fbp = gm20b_fuse_status_opt_rop_l2_fbp,
		.fuse_status_opt_gpc = gm20b_fuse_status_opt_gpc,
		.fuse_status_opt_tpc_gpc = gm20b_fuse_status_opt_tpc_gpc,
		.fuse_ctrl_opt_tpc_gpc = gm20b_fuse_ctrl_opt_tpc_gpc,
		.fuse_opt_sec_debug_en = gm20b_fuse_opt_sec_debug_en,
		.fuse_opt_priv_sec_en = gm20b_fuse_opt_priv_sec_en,
		.read_vin_cal_fuse_rev = gp106_fuse_read_vin_cal_fuse_rev,
		.read_vin_cal_slope_intercept_fuse =
			gp106_fuse_read_vin_cal_slope_intercept_fuse,
		.read_vin_cal_gain_offset_fuse =
			gp106_fuse_read_vin_cal_gain_offset_fuse,
	},
#if defined(CONFIG_TEGRA_NVLINK)
	.nvlink = {
		.discover_ioctrl = gv100_nvlink_discover_ioctrl,
		.discover_link = gv100_nvlink_discover_link,
		.init = gv100_nvlink_init,
		.isr = gv100_nvlink_isr,
		.rxdet = tu104_nvlink_rxdet,
		.setup_pll = tu104_nvlink_setup_pll,
		.minion_data_ready_en = tu104_nvlink_minion_data_ready_en,
		.get_connected_link_mask = tu104_nvlink_get_connected_link_mask,
		.set_sw_war = NULL,
		/* API */
		.link_early_init = gv100_nvlink_link_early_init,
		.link_get_state = gv100_nvlink_link_get_state,
		.link_set_mode = gv100_nvlink_link_set_mode,
		.link_get_mode = gv100_nvlink_link_get_mode,
		.get_sublink_mode = gv100_nvlink_link_get_sublink_mode,
		.get_tx_sublink_state = tu104_nvlink_link_get_tx_sublink_state,
		.get_rx_sublink_state = tu104_nvlink_link_get_rx_sublink_state,
		.set_sublink_mode = gv100_nvlink_link_set_sublink_mode,
		.interface_init = gv100_nvlink_interface_init,
		.reg_init = gv100_nvlink_reg_init,
		.shutdown = gv100_nvlink_shutdown,
		.early_init = gv100_nvlink_early_init,
		.speed_config = tu104_nvlink_speed_config,
	},
#endif
	.acr = {
		.acr_sw_init = nvgpu_tu104_acr_sw_init,
	},
	.sec2 = {
		.secured_sec2_start = tu104_start_sec2_secure,
		.enable_irq = tu104_sec2_enable_irq,
		.is_interrupted = tu104_sec2_is_interrupted,
		.isr = tu104_sec2_isr,
		.msgq_tail = tu104_sec2_msgq_tail,
	},
	.chip_init_gpu_characteristics = tu104_init_gpu_characteristics,
	.get_litter_value = tu104_get_litter_value,
};

int tu104_init_hal(struct gk20a *g)
{
	struct gpu_ops *gops = &g->ops;

	gops->bios = tu104_ops.bios;
	gops->ltc = tu104_ops.ltc;
	gops->ce2 = tu104_ops.ce2;
	gops->gr = tu104_ops.gr;
	gops->fb = tu104_ops.fb;
	gops->clock_gating = tu104_ops.clock_gating;
	gops->fifo = tu104_ops.fifo;
	gops->netlist = tu104_ops.netlist;
	gops->mm = tu104_ops.mm;
#ifdef CONFIG_GK20A_CTXSW_TRACE
	gops->fecs_trace = tu104_ops.fecs_trace;
#endif
	gops->pramin = tu104_ops.pramin;
	gops->therm = tu104_ops.therm;
	gops->pmu = tu104_ops.pmu;
	gops->regops = tu104_ops.regops;
	gops->mc = tu104_ops.mc;
	gops->debug = tu104_ops.debug;
	gops->debugger = tu104_ops.debugger;
	gops->perf = tu104_ops.perf;
	gops->perfbuf = tu104_ops.perfbuf;
	gops->bus = tu104_ops.bus;
	gops->ptimer = tu104_ops.ptimer;
#if defined(CONFIG_GK20A_CYCLE_STATS)
	gops->css = tu104_ops.css;
#endif
	gops->xve = tu104_ops.xve;
	gops->falcon = tu104_ops.falcon;
	gops->priv_ring = tu104_ops.priv_ring;
	gops->fuse = tu104_ops.fuse;
	gops->nvlink = tu104_ops.nvlink;
	gops->acr = tu104_ops.acr;
	gops->sec2 = tu104_ops.sec2;

	/* clocks */
	gops->clk.init_clk_support = tu104_ops.clk.init_clk_support;
	gops->clk.get_rate_cntr = tu104_ops.clk.get_rate_cntr;
	gops->clk.get_crystal_clk_hz = tu104_ops.clk.get_crystal_clk_hz;
	gops->clk.measure_freq = tu104_ops.clk.measure_freq;
	gops->clk.suspend_clk_support = tu104_ops.clk.suspend_clk_support;

	/* Lone functions */
	gops->chip_init_gpu_characteristics =
		tu104_ops.chip_init_gpu_characteristics;
	gops->get_litter_value = tu104_ops.get_litter_value;
	gops->semaphore_wakeup = gk20a_channel_semaphore_wakeup;

	__nvgpu_set_enabled(g, NVGPU_GR_USE_DMA_FOR_FW_BOOTSTRAP, true);
	__nvgpu_set_enabled(g, NVGPU_SEC_PRIVSECURITY, true);
	__nvgpu_set_enabled(g, NVGPU_SEC_SECUREGPCCS, true);
	__nvgpu_set_enabled(g, NVGPU_PMU_FECS_BOOTSTRAP_DONE, false);
	__nvgpu_set_enabled(g, NVGPU_SUPPORT_MULTIPLE_WPR, true);
	__nvgpu_set_enabled(g, NVGPU_FECS_TRACE_VA, true);
	__nvgpu_set_enabled(g, NVGPU_SUPPORT_SEC2_RTOS, true);

	/* for now */
	__nvgpu_set_enabled(g, NVGPU_PMU_PSTATE, false);

	g->pmu_lsf_pmu_wpr_init_done = 0;

	g->name = "tu10x";

	return 0;
}
