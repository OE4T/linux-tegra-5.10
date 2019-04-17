/*
 * GV11B Tegra HAL interface
 *
 * Copyright (c) 2016-2019, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/class.h>
#include <nvgpu/fuse.h>
#include <nvgpu/pbdma.h>
#include <nvgpu/regops.h>
#include <nvgpu/gr/gr_falcon.h>
#include <nvgpu/gr/gr.h>
#include <nvgpu/pmu/pmu_perfmon.h>

#include "hal/mm/cache/flush_gk20a.h"
#include "hal/mm/cache/flush_gv11b.h"
#include "hal/mm/gmmu/gmmu_gm20b.h"
#include "hal/mm/gmmu/gmmu_gp10b.h"
#include "hal/mm/gmmu/gmmu_gv11b.h"
#include "hal/mc/mc_gm20b.h"
#include "hal/mc/mc_gp10b.h"
#include "hal/mc/mc_gv11b.h"
#include "hal/bus/bus_gk20a.h"
#include "hal/bus/bus_gp10b.h"
#include "hal/bus/bus_gm20b.h"
#include "hal/ce/ce_gv11b.h"
#include "hal/class/class_gv11b.h"
#include "hal/priv_ring/priv_ring_gm20b.h"
#include "hal/priv_ring/priv_ring_gp10b.h"
#include "hal/gr/config/gr_config_gv100.h"
#include "hal/power_features/cg/gv11b_gating_reglist.h"
#include "hal/cbc/cbc_gp10b.h"
#include "hal/cbc/cbc_gv11b.h"
#include "hal/ce/ce_gp10b.h"
#include "hal/therm/therm_gm20b.h"
#include "hal/therm/therm_gv11b.h"
#include "hal/ltc/ltc_gm20b.h"
#include "hal/ltc/ltc_gp10b.h"
#include "hal/ltc/ltc_gv11b.h"
#include "hal/ltc/intr/ltc_intr_gv11b.h"
#include "hal/fb/fb_gm20b.h"
#include "hal/fb/fb_gp10b.h"
#include "hal/fb/fb_gv11b.h"
#include "hal/fb/fb_mmu_fault_gv11b.h"
#include "hal/fb/intr/fb_intr_gv11b.h"
#include "hal/fuse/fuse_gm20b.h"
#include "hal/fuse/fuse_gp10b.h"
#include "hal/ptimer/ptimer_gk20a.h"
#include "hal/regops/regops_gv11b.h"
#include "hal/rc/rc_gv11b.h"
#include "hal/fifo/fifo_gv11b.h"
#include "hal/fifo/pbdma_gm20b.h"
#include "hal/fifo/preempt_gv11b.h"
#include "hal/fifo/pbdma_gp10b.h"
#include "hal/fifo/pbdma_gv11b.h"
#include "hal/fifo/engine_status_gv100.h"
#include "hal/fifo/pbdma_status_gm20b.h"
#include "hal/fifo/engines_gp10b.h"
#include "hal/fifo/engines_gv11b.h"
#include "hal/fifo/ramfc_gp10b.h"
#include "hal/fifo/ramfc_gv11b.h"
#include "hal/fifo/ramin_gk20a.h"
#include "hal/fifo/ramin_gm20b.h"
#include "hal/fifo/ramin_gp10b.h"
#include "hal/fifo/ramin_gv11b.h"
#include "hal/fifo/runlist_ram_gk20a.h"
#include "hal/fifo/runlist_ram_gv11b.h"
#include "hal/fifo/runlist_fifo_gk20a.h"
#include "hal/fifo/runlist_fifo_gv11b.h"
#include "hal/fifo/tsg_gv11b.h"
#include "hal/fifo/userd_gk20a.h"
#include "hal/fifo/userd_gv11b.h"
#include "hal/fifo/usermode_gv11b.h"
#include "hal/fifo/fifo_intr_gk20a.h"
#include "hal/fifo/fifo_intr_gv11b.h"
#include "hal/fifo/ctxsw_timeout_gv11b.h"
#include "hal/gr/ecc/ecc_gv11b.h"
#include "hal/gr/fecs_trace/fecs_trace_gm20b.h"
#include "hal/gr/fecs_trace/fecs_trace_gv11b.h"
#include "hal/gr/falcon/gr_falcon_gm20b.h"
#include "hal/gr/falcon/gr_falcon_gp10b.h"
#include "hal/gr/falcon/gr_falcon_gv11b.h"
#include "hal/gr/config/gr_config_gm20b.h"
#include "hal/gr/zbc/zbc_gp10b.h"
#include "hal/gr/zbc/zbc_gv11b.h"
#include "hal/gr/zcull/zcull_gm20b.h"
#include "hal/gr/zcull/zcull_gv11b.h"
#include "hal/gr/init/gr_init_gm20b.h"
#include "hal/gr/init/gr_init_gp10b.h"
#include "hal/gr/init/gr_init_gv11b.h"
#include "hal/gr/intr/gr_intr_gm20b.h"
#include "hal/gr/intr/gr_intr_gv11b.h"
#include "hal/gr/hwpm_map/hwpm_map_gv100.h"
#include "hal/gr/ctxsw_prog/ctxsw_prog_gm20b.h"
#include "hal/gr/ctxsw_prog/ctxsw_prog_gp10b.h"
#include "hal/gr/ctxsw_prog/ctxsw_prog_gv11b.h"
#include "hal/gr/gr/gr_gk20a.h"
#include "hal/gr/gr/gr_gm20b.h"
#include "hal/gr/gr/gr_gp10b.h"
#include "hal/gr/gr/gr_gv100.h"
#include "hal/gr/gr/gr_gv11b.h"
#include "hal/pmu/pmu_gk20a.h"
#include "hal/pmu/pmu_gm20b.h"
#include "hal/pmu/pmu_gp106.h"
#include "hal/pmu/pmu_gp10b.h"
#include "hal/pmu/pmu_gv11b.h"
#include "hal/sync/syncpt_cmdbuf_gv11b.h"
#include "hal/sync/sema_cmdbuf_gv11b.h"
#include "hal/falcon/falcon_gk20a.h"
#include "hal/perf/perf_gv11b.h"
#include "hal/netlist/netlist_gv11b.h"
#include "hal/top/top_gm20b.h"
#include "hal/top/top_gp10b.h"
#include "hal/top/top_gv11b.h"


#include "common/pmu/pg/pg_sw_gm20b.h"
#include "common/pmu/pg/pg_sw_gp106.h"
#include "common/pmu/pg/pg_sw_gv11b.h"
#include "common/fifo/channel_gk20a.h"
#include "common/fifo/channel_gm20b.h"
#include "common/fifo/channel_gv11b.h"
#include "common/clk_arb/clk_arb_gp10b.h"

#include "gm20b/mm_gm20b.h"
#include "gp10b/mm_gp10b.h"

#include "hal_gv11b.h"
#include "gv11b/mm_gv11b.h"

#include <nvgpu/ptimer.h>
#include <nvgpu/error_notifier.h>
#include <nvgpu/debugger.h>
#include <nvgpu/runlist.h>
#include <nvgpu/fifo/userd.h>
#include <nvgpu/perfbuf.h>
#include <nvgpu/cyclestats_snapshot.h>
#include <nvgpu/gr/zbc.h>
#include <nvgpu/gr/setup.h>
#include <nvgpu/gr/fecs_trace.h>
#include <nvgpu/gr/gr_intr.h>

#include <nvgpu/hw/gv11b/hw_proj_gv11b.h>
#include <nvgpu/hw/gv11b/hw_pwr_gv11b.h>

static void gv11b_init_gpu_characteristics(struct gk20a *g)
{
	gk20a_init_gpu_characteristics(g);
	g->ops.gr.ecc.detect(g);
	nvgpu_set_enabled(g, NVGPU_SUPPORT_TSG_SUBCONTEXTS, true);
	nvgpu_set_enabled(g, NVGPU_SUPPORT_SCG, true);
	nvgpu_set_enabled(g, NVGPU_SUPPORT_RESCHEDULE_RUNLIST, true);
	nvgpu_set_enabled(g, NVGPU_SUPPORT_SYNCPOINT_ADDRESS, true);
	nvgpu_set_enabled(g, NVGPU_SUPPORT_USER_SYNCPOINT, true);
	nvgpu_set_enabled(g, NVGPU_SUPPORT_USERMODE_SUBMIT, true);
}

u32 gv11b_get_litter_value(struct gk20a *g, int value)
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
	case GPU_LIT_PPC_IN_GPC_SHARED_BASE:
		ret = proj_ppc_in_gpc_shared_base_v();
		break;
	case GPU_LIT_PPC_IN_GPC_STRIDE:
		ret = proj_ppc_in_gpc_stride_v();
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
	/* Even though GV11B doesn't have an FBPA unit, the HW reports one,
	 * and the microcode as a result leaves space in the context buffer
	 * for one, so make sure SW accounts for this also.
	 */
	case GPU_LIT_NUM_FBPAS:
		ret = proj_scal_litter_num_fbpas_v();
		break;
	/* Hardcode FBPA values other than NUM_FBPAS to 0. */
	case GPU_LIT_FBPA_STRIDE:
	case GPU_LIT_FBPA_BASE:
	case GPU_LIT_FBPA_SHARED_BASE:
		ret = 0;
		break;
	case GPU_LIT_TWOD_CLASS:
		ret = FERMI_TWOD_A;
		break;
	case GPU_LIT_THREED_CLASS:
		ret = VOLTA_A;
		break;
	case GPU_LIT_COMPUTE_CLASS:
		ret = VOLTA_COMPUTE_A;
		break;
	case GPU_LIT_GPFIFO_CLASS:
		ret = VOLTA_CHANNEL_GPFIFO_A;
		break;
	case GPU_LIT_I2M_CLASS:
		ret = KEPLER_INLINE_TO_MEMORY_B;
		break;
	case GPU_LIT_DMA_COPY_CLASS:
		ret = VOLTA_DMA_COPY_A;
		break;
	case GPU_LIT_GPC_PRIV_STRIDE:
		ret = proj_gpc_priv_stride_v();
		break;
	case GPU_LIT_PERFMON_PMMGPCTPCA_DOMAIN_START:
		ret = 2;
		break;
	case GPU_LIT_PERFMON_PMMGPCTPCB_DOMAIN_START:
		ret = 6;
		break;
	case GPU_LIT_PERFMON_PMMGPCTPC_DOMAIN_COUNT:
		ret = 4;
		break;
	case GPU_LIT_PERFMON_PMMFBP_LTC_DOMAIN_START:
		ret = 1;
		break;
	case GPU_LIT_PERFMON_PMMFBP_LTC_DOMAIN_COUNT:
		ret = 2;
		break;
	case GPU_LIT_PERFMON_PMMFBP_ROP_DOMAIN_START:
		ret = 3;
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

static const struct gpu_ops gv11b_ops = {
	.ltc = {
		.determine_L2_size_bytes = gp10b_determine_L2_size_bytes,
		.set_zbc_s_entry = gv11b_ltc_set_zbc_stencil_entry,
		.set_zbc_color_entry = gm20b_ltc_set_zbc_color_entry,
		.set_zbc_depth_entry = gm20b_ltc_set_zbc_depth_entry,
		.init_fs_state = gv11b_ltc_init_fs_state,
		.flush = gm20b_flush_ltc,
		.set_enabled = gp10b_ltc_set_enabled,
		.pri_is_ltc_addr = gm20b_ltc_pri_is_ltc_addr,
		.is_ltcs_ltss_addr = gm20b_ltc_is_ltcs_ltss_addr,
		.is_ltcn_ltss_addr = gm20b_ltc_is_ltcn_ltss_addr,
		.split_lts_broadcast_addr = gm20b_ltc_split_lts_broadcast_addr,
		.split_ltc_broadcast_addr = gm20b_ltc_split_ltc_broadcast_addr,
		.intr = {
			.configure = gv11b_ltc_intr_configure,
			.isr = gv11b_ltc_intr_isr,
			.en_illegal_compstat =
				gv11b_ltc_intr_en_illegal_compstat,
		}
	},
	.cbc = {
		.init = gv11b_cbc_init,
		.alloc_comptags = gp10b_cbc_alloc_comptags,
		.ctrl = gp10b_cbc_ctrl,
	},
	.ce = {
		.isr_stall = gv11b_ce_stall_isr,
		.isr_nonstall = gp10b_ce_nonstall_isr,
		.get_num_pce = gv11b_ce_get_num_pce,
		.mthd_buffer_fault_in_bar2_fault =
				gv11b_ce_mthd_buffer_fault_in_bar2_fault,
	},
	.gr = {
		.set_alpha_circular_buffer_size =
			gr_gv11b_set_alpha_circular_buffer_size,
		.set_circular_buffer_size = gr_gv11b_set_circular_buffer_size,
		.get_sm_dsm_perf_regs = gv11b_gr_get_sm_dsm_perf_regs,
		.get_sm_dsm_perf_ctrl_regs = gv11b_gr_get_sm_dsm_perf_ctrl_regs,
		.set_hww_esr_report_mask = gv11b_gr_set_hww_esr_report_mask,
		.set_gpc_tpc_mask = gr_gv11b_set_gpc_tpc_mask,
		.is_tpc_addr = gr_gm20b_is_tpc_addr,
		.get_tpc_num = gr_gm20b_get_tpc_num,
		.powergate_tpc = gr_gv11b_powergate_tpc,
		.dump_gr_regs = gr_gv11b_dump_gr_status_regs,
		.update_pc_sampling = gr_gm20b_update_pc_sampling,
		.get_rop_l2_en_mask = gr_gm20b_rop_l2_en_mask,
		.init_sm_dsm_reg_info = gv11b_gr_init_sm_dsm_reg_info,
		.init_cyclestats = gr_gm20b_init_cyclestats,
		.set_sm_debug_mode = gv11b_gr_set_sm_debug_mode,
		.bpt_reg_info = gv11b_gr_bpt_reg_info,
		.get_lrf_tex_ltc_dram_override = get_ecc_override_val,
		.update_smpc_ctxsw_mode = gr_gk20a_update_smpc_ctxsw_mode,
		.get_num_hwpm_perfmon = gr_gv100_get_num_hwpm_perfmon,
		.set_pmm_register = gr_gv100_set_pmm_register,
		.update_hwpm_ctxsw_mode = gr_gk20a_update_hwpm_ctxsw_mode,
		.init_hwpm_pmm_register = gr_gv100_init_hwpm_pmm_register,
		.record_sm_error_state = gv11b_gr_record_sm_error_state,
		.clear_sm_error_state = gv11b_gr_clear_sm_error_state,
		.suspend_contexts = gr_gp10b_suspend_contexts,
		.resume_contexts = gr_gk20a_resume_contexts,
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
		.get_sm_hww_warp_esr_pc = gv11b_gr_get_sm_hww_warp_esr_pc,
		.get_sm_no_lock_down_hww_global_esr_mask =
			gv11b_gr_get_sm_no_lock_down_hww_global_esr_mask,
		.lock_down_sm = gv11b_gr_lock_down_sm,
		.wait_for_sm_lock_down = gv11b_gr_wait_for_sm_lock_down,
		.clear_sm_hww = gv11b_gr_clear_sm_hww,
		.init_ovr_sm_dsm_perf =  gv11b_gr_init_ovr_sm_dsm_perf,
		.get_ovr_perf_regs = gv11b_gr_get_ovr_perf_regs,
		.set_boosted_ctx = gr_gp10b_set_boosted_ctx,
		.pre_process_sm_exception = gr_gv11b_pre_process_sm_exception,
		.set_bes_crop_debug3 = gr_gp10b_set_bes_crop_debug3,
		.set_bes_crop_debug4 = gr_gp10b_set_bes_crop_debug4,
		.is_etpc_addr = gv11b_gr_pri_is_etpc_addr,
		.egpc_etpc_priv_addr_table = gv11b_gr_egpc_etpc_priv_addr_table,
		.get_egpc_base = gv11b_gr_get_egpc_base,
		.get_egpc_etpc_num = gv11b_gr_get_egpc_etpc_num,
		.access_smpc_reg = gv11b_gr_access_smpc_reg,
		.is_egpc_addr = gv11b_gr_pri_is_egpc_addr,
		.handle_tpc_sm_ecc_exception =
			gr_gv11b_handle_tpc_sm_ecc_exception,
		.decode_egpc_addr = gv11b_gr_decode_egpc_addr,
		.handle_ssync_hww = gr_gv11b_handle_ssync_hww,
		.decode_priv_addr = gr_gv11b_decode_priv_addr,
		.create_priv_addr_table = gr_gv11b_create_priv_addr_table,
		.split_fbpa_broadcast_addr = gr_gk20a_split_fbpa_broadcast_addr,
		.get_offset_in_gpccs_segment =
			gr_gk20a_get_offset_in_gpccs_segment,
		.set_debug_mode = gm20b_gr_set_debug_mode,
		.log_mme_exception = NULL,
		.get_ctxsw_checksum_mismatch_mailbox_val =
				gr_gv11b_ctxsw_checksum_mismatch_mailbox_val,
		.reset = nvgpu_gr_reset,
		.esr_bpt_pending_events = gv11b_gr_esr_bpt_pending_events,
		.halt_pipe = nvgpu_gr_halt_pipe,
		.disable_ctxsw = nvgpu_gr_disable_ctxsw,
		.enable_ctxsw = nvgpu_gr_enable_ctxsw,
		.ecc = {
			.detect = gv11b_ecc_detect_enabled_units,
			.init = gv11b_ecc_init,
		},
		.ctxsw_prog = {
			.hw_get_fecs_header_size =
				gm20b_ctxsw_prog_hw_get_fecs_header_size,
			.hw_get_gpccs_header_size =
				gm20b_ctxsw_prog_hw_get_gpccs_header_size,
			.hw_get_extended_buffer_segments_size_in_bytes =
				gm20b_ctxsw_prog_hw_get_extended_buffer_segments_size_in_bytes,
			.hw_extended_marker_size_in_bytes =
				gm20b_ctxsw_prog_hw_extended_marker_size_in_bytes,
			.hw_get_perf_counter_control_register_stride =
				gm20b_ctxsw_prog_hw_get_perf_counter_control_register_stride,
			.get_main_image_ctx_id =
				gm20b_ctxsw_prog_get_main_image_ctx_id,
			.get_patch_count = gm20b_ctxsw_prog_get_patch_count,
			.set_patch_count = gm20b_ctxsw_prog_set_patch_count,
			.set_patch_addr = gm20b_ctxsw_prog_set_patch_addr,
			.set_zcull_ptr = gv11b_ctxsw_prog_set_zcull_ptr,
			.set_zcull = gm20b_ctxsw_prog_set_zcull,
			.set_zcull_mode_no_ctxsw =
				gm20b_ctxsw_prog_set_zcull_mode_no_ctxsw,
			.is_zcull_mode_separate_buffer =
				gm20b_ctxsw_prog_is_zcull_mode_separate_buffer,
			.set_pm_ptr = gv11b_ctxsw_prog_set_pm_ptr,
			.set_pm_mode = gm20b_ctxsw_prog_set_pm_mode,
			.set_pm_smpc_mode = gm20b_ctxsw_prog_set_pm_smpc_mode,
			.hw_get_pm_mode_no_ctxsw =
				gm20b_ctxsw_prog_hw_get_pm_mode_no_ctxsw,
			.hw_get_pm_mode_ctxsw = gm20b_ctxsw_prog_hw_get_pm_mode_ctxsw,
			.hw_get_pm_mode_stream_out_ctxsw =
				gv11b_ctxsw_prog_hw_get_pm_mode_stream_out_ctxsw,
			.init_ctxsw_hdr_data = gp10b_ctxsw_prog_init_ctxsw_hdr_data,
			.set_compute_preemption_mode_cta =
				gp10b_ctxsw_prog_set_compute_preemption_mode_cta,
			.set_compute_preemption_mode_cilp =
				gp10b_ctxsw_prog_set_compute_preemption_mode_cilp,
			.set_graphics_preemption_mode_gfxp =
				gp10b_ctxsw_prog_set_graphics_preemption_mode_gfxp,
			.set_cde_enabled = NULL,
			.set_pc_sampling = gm20b_ctxsw_prog_set_pc_sampling,
			.set_priv_access_map_config_mode =
				gm20b_ctxsw_prog_set_priv_access_map_config_mode,
			.set_priv_access_map_addr =
				gm20b_ctxsw_prog_set_priv_access_map_addr,
			.disable_verif_features =
				gm20b_ctxsw_prog_disable_verif_features,
			.check_main_image_header_magic =
				gm20b_ctxsw_prog_check_main_image_header_magic,
			.check_local_header_magic =
				gm20b_ctxsw_prog_check_local_header_magic,
			.get_num_gpcs = gm20b_ctxsw_prog_get_num_gpcs,
			.get_num_tpcs = gm20b_ctxsw_prog_get_num_tpcs,
			.get_extended_buffer_size_offset =
				gm20b_ctxsw_prog_get_extended_buffer_size_offset,
			.get_ppc_info = gm20b_ctxsw_prog_get_ppc_info,
			.get_local_priv_register_ctl_offset =
				gm20b_ctxsw_prog_get_local_priv_register_ctl_offset,
			.hw_get_ts_tag_invalid_timestamp =
				gm20b_ctxsw_prog_hw_get_ts_tag_invalid_timestamp,
			.hw_get_ts_tag = gm20b_ctxsw_prog_hw_get_ts_tag,
			.hw_record_ts_timestamp =
				gm20b_ctxsw_prog_hw_record_ts_timestamp,
			.hw_get_ts_record_size_in_bytes =
				gm20b_ctxsw_prog_hw_get_ts_record_size_in_bytes,
			.is_ts_valid_record = gm20b_ctxsw_prog_is_ts_valid_record,
			.get_ts_buffer_aperture_mask =
				gm20b_ctxsw_prog_get_ts_buffer_aperture_mask,
			.set_ts_num_records = gm20b_ctxsw_prog_set_ts_num_records,
			.set_ts_buffer_ptr = gm20b_ctxsw_prog_set_ts_buffer_ptr,
			.set_pmu_options_boost_clock_frequencies =
				gp10b_ctxsw_prog_set_pmu_options_boost_clock_frequencies,
			.set_full_preemption_ptr =
				gv11b_ctxsw_prog_set_full_preemption_ptr,
			.set_full_preemption_ptr_veid0 =
				gv11b_ctxsw_prog_set_full_preemption_ptr_veid0,
			.hw_get_perf_counter_register_stride =
				gv11b_ctxsw_prog_hw_get_perf_counter_register_stride,
			.set_context_buffer_ptr =
				gv11b_ctxsw_prog_set_context_buffer_ptr,
			.set_type_per_veid_header =
				gv11b_ctxsw_prog_set_type_per_veid_header,
			.dump_ctxsw_stats = gp10b_ctxsw_prog_dump_ctxsw_stats,
		},
		.config = {
			.get_gpc_tpc_mask = gm20b_gr_config_get_gpc_tpc_mask,
			.get_tpc_count_in_gpc =
				gm20b_gr_config_get_tpc_count_in_gpc,
			.get_zcull_count_in_gpc =
				gm20b_gr_config_get_zcull_count_in_gpc,
			.get_pes_tpc_mask = gm20b_gr_config_get_pes_tpc_mask,
			.get_pd_dist_skip_table_size =
				gm20b_gr_config_get_pd_dist_skip_table_size,
			.init_sm_id_table = gv100_gr_config_init_sm_id_table,
		},
#ifdef CONFIG_GK20A_CTXSW_TRACE
		.fecs_trace = {
			.alloc_user_buffer = nvgpu_gr_fecs_trace_ring_alloc,
			.free_user_buffer = nvgpu_gr_fecs_trace_ring_free,
			.get_mmap_user_buffer_info =
				nvgpu_gr_fecs_trace_get_mmap_buffer_info,
			.init = nvgpu_gr_fecs_trace_init,
			.deinit = nvgpu_gr_fecs_trace_deinit,
			.enable = nvgpu_gr_fecs_trace_enable,
			.disable = nvgpu_gr_fecs_trace_disable,
			.is_enabled = nvgpu_gr_fecs_trace_is_enabled,
			.reset = nvgpu_gr_fecs_trace_reset,
			.flush = NULL,
			.poll = nvgpu_gr_fecs_trace_poll,
			.bind_channel = nvgpu_gr_fecs_trace_bind_channel,
			.unbind_channel = nvgpu_gr_fecs_trace_unbind_channel,
			.max_entries = nvgpu_gr_fecs_trace_max_entries,
			.get_buffer_full_mailbox_val =
				gv11b_fecs_trace_get_buffer_full_mailbox_val,
			.get_read_index = gm20b_fecs_trace_get_read_index,
			.get_write_index = gm20b_fecs_trace_get_write_index,
			.set_read_index = gm20b_fecs_trace_set_read_index,
		},
#endif /* CONFIG_GK20A_CTXSW_TRACE */
		.setup = {
			.bind_ctxsw_zcull = nvgpu_gr_setup_bind_ctxsw_zcull,
			.alloc_obj_ctx = nvgpu_gr_setup_alloc_obj_ctx,
			.free_gr_ctx = nvgpu_gr_setup_free_gr_ctx,
			.free_subctx = nvgpu_gr_setup_free_subctx,
			.set_preemption_mode = nvgpu_gr_setup_set_preemption_mode,
		},
		.zbc = {
			.add_color = gp10b_gr_zbc_add_color,
			.add_depth = gp10b_gr_zbc_add_depth,
			.set_table = nvgpu_gr_zbc_set_table,
			.query_table = nvgpu_gr_zbc_query_table,
			.add_stencil = gv11b_gr_zbc_add_stencil,
			.get_gpcs_swdx_dss_zbc_c_format_reg =
				gv11b_gr_zbc_get_gpcs_swdx_dss_zbc_c_format_reg,
			.get_gpcs_swdx_dss_zbc_z_format_reg =
				gv11b_gr_zbc_get_gpcs_swdx_dss_zbc_z_format_reg,
		},
		.zcull = {
			.init_zcull_hw = gm20b_gr_init_zcull_hw,
			.get_zcull_info = gm20b_gr_get_zcull_info,
			.program_zcull_mapping = gv11b_gr_program_zcull_mapping,
		},
		.hwpm_map = {
			.align_regs_perf_pma =
				gv100_gr_hwpm_map_align_regs_perf_pma,
		},
		.init = {
			.get_nonpes_aware_tpc =
					gv11b_gr_init_get_nonpes_aware_tpc,
			.wait_initialized = nvgpu_gr_wait_initialized,
			.ecc_scrub_reg = gv11b_gr_init_ecc_scrub_reg,
			.get_fbp_en_mask = gm20b_gr_init_get_fbp_en_mask,
			.lg_coalesce = gm20b_gr_init_lg_coalesce,
			.su_coalesce = gm20b_gr_init_su_coalesce,
			.pes_vsc_stream = gm20b_gr_init_pes_vsc_stream,
			.gpc_mmu = gv11b_gr_init_gpc_mmu,
			.fifo_access = gm20b_gr_init_fifo_access,
			.get_access_map = gv11b_gr_init_get_access_map,
			.get_sm_id_size = gp10b_gr_init_get_sm_id_size,
			.sm_id_config = gv11b_gr_init_sm_id_config,
			.sm_id_numbering = gv11b_gr_init_sm_id_numbering,
			.tpc_mask = gv11b_gr_init_tpc_mask,
			.rop_mapping = gv11b_gr_init_rop_mapping,
			.fs_state = gv11b_gr_init_fs_state,
			.pd_tpc_per_gpc = gm20b_gr_init_pd_tpc_per_gpc,
			.pd_skip_table_gpc = gm20b_gr_init_pd_skip_table_gpc,
			.cwd_gpcs_tpcs_num = gm20b_gr_init_cwd_gpcs_tpcs_num,
			.wait_empty = gp10b_gr_init_wait_empty,
			.wait_idle = gm20b_gr_init_wait_idle,
			.wait_fe_idle = gm20b_gr_init_wait_fe_idle,
			.fe_pwr_mode_force_on =
				gm20b_gr_init_fe_pwr_mode_force_on,
			.override_context_reset =
				gm20b_gr_init_override_context_reset,
			.preemption_state = gv11b_gr_init_preemption_state,
			.fe_go_idle_timeout = gm20b_gr_init_fe_go_idle_timeout,
			.load_method_init = gm20b_gr_init_load_method_init,
			.commit_global_timeslice =
				gv11b_gr_init_commit_global_timeslice,
			.get_bundle_cb_default_size =
				gv11b_gr_init_get_bundle_cb_default_size,
			.get_min_gpm_fifo_depth =
				gv11b_gr_init_get_min_gpm_fifo_depth,
			.get_bundle_cb_token_limit =
				gv11b_gr_init_get_bundle_cb_token_limit,
			.get_attrib_cb_default_size =
				gv11b_gr_init_get_attrib_cb_default_size,
			.get_alpha_cb_default_size =
				gv11b_gr_init_get_alpha_cb_default_size,
			.get_attrib_cb_gfxp_default_size =
				gv11b_gr_init_get_attrib_cb_gfxp_default_size,
			.get_attrib_cb_gfxp_size =
				gv11b_gr_init_get_attrib_cb_gfxp_size,
			.get_attrib_cb_size =
				gv11b_gr_init_get_attrib_cb_size,
			.get_alpha_cb_size =
				gv11b_gr_init_get_alpha_cb_size,
			.get_global_attr_cb_size =
				gv11b_gr_init_get_global_attr_cb_size,
			.get_global_ctx_cb_buffer_size =
				gm20b_gr_init_get_global_ctx_cb_buffer_size,
			.get_global_ctx_pagepool_buffer_size =
				gm20b_gr_init_get_global_ctx_pagepool_buffer_size,
			.commit_global_bundle_cb =
				gp10b_gr_init_commit_global_bundle_cb,
			.pagepool_default_size =
				gp10b_gr_init_pagepool_default_size,
			.commit_global_pagepool =
				gp10b_gr_init_commit_global_pagepool,
			.commit_global_attrib_cb =
				gv11b_gr_init_commit_global_attrib_cb,
			.commit_global_cb_manager =
				gp10b_gr_init_commit_global_cb_manager,
			.pipe_mode_override = gm20b_gr_init_pipe_mode_override,
			.load_sw_bundle_init =
				gm20b_gr_init_load_sw_bundle_init,
			.load_sw_veid_bundle =
				gv11b_gr_init_load_sw_veid_bundle,
			.get_ctx_spill_size = gv11b_gr_init_get_ctx_spill_size,
			.get_ctx_pagepool_size =
				gp10b_gr_init_get_ctx_pagepool_size,
			.get_ctx_betacb_size =
				gv11b_gr_init_get_ctx_betacb_size,
			.get_ctx_attrib_cb_size =
				gp10b_gr_init_get_ctx_attrib_cb_size,
			.get_gfxp_rtv_cb_size = NULL,
			.commit_ctxsw_spill = gv11b_gr_init_commit_ctxsw_spill,
			.commit_cbes_reserve =
				gv11b_gr_init_commit_cbes_reserve,
			.gfxp_wfi_timeout =
				gv11b_gr_init_commit_gfxp_wfi_timeout,
			.get_max_subctx_count =
				gv11b_gr_init_get_max_subctx_count,
			.get_patch_slots = gv11b_gr_init_get_patch_slots,
			.detect_sm_arch = gv11b_gr_init_detect_sm_arch,
			.get_supported__preemption_modes =
				gp10b_gr_init_get_supported_preemption_modes,
			.get_default_preemption_modes =
				gp10b_gr_init_get_default_preemption_modes,
		},
		.intr = {
			.handle_fecs_error = gv11b_gr_intr_handle_fecs_error,
			.handle_sw_method = gv11b_gr_intr_handle_sw_method,
			.set_shader_exceptions =
					gv11b_gr_intr_set_shader_exceptions,
			.handle_class_error =
					gm20b_gr_intr_handle_class_error,
			.clear_pending_interrupts =
					gm20b_gr_intr_clear_pending_interrupts,
			.read_pending_interrupts =
					gm20b_gr_intr_read_pending_interrupts,
			.handle_exceptions =
					gm20b_gr_intr_handle_exceptions,
			.read_gpc_tpc_exception =
					gm20b_gr_intr_read_gpc_tpc_exception,
			.read_gpc_exception =
					gm20b_gr_intr_read_gpc_exception,
			.read_exception1 =
					gm20b_gr_intr_read_exception1,
			.trapped_method_info =
					gm20b_gr_intr_get_trapped_method_info,
			.handle_semaphore_pending =
					nvgpu_gr_intr_handle_semaphore_pending,
			.handle_notify_pending =
					nvgpu_gr_intr_handle_notify_pending,
			.handle_gcc_exception =
				gv11b_gr_intr_handle_gcc_exception,
			.handle_gpc_gpcmmu_exception =
				gv11b_gr_intr_handle_gpc_gpcmmu_exception,
			.handle_gpc_gpccs_exception =
				gv11b_gr_intr_handle_gpc_gpccs_exception,
			.get_tpc_exception = gm20b_gr_intr_get_tpc_exception,
			.handle_tpc_mpc_exception =
					gv11b_gr_intr_handle_tpc_mpc_exception,
			.handle_tex_exception = NULL,
			.enable_hww_exceptions =
					gv11b_gr_intr_enable_hww_exceptions,
			.enable_interrupts = gm20b_gr_intr_enable_interrupts,
			.enable_gpc_exceptions =
					gv11b_gr_intr_enable_gpc_exceptions,
			.enable_exceptions = gv11b_gr_intr_enable_exceptions,
			.nonstall_isr = gm20b_gr_intr_nonstall_isr,
			.tpc_exception_sm_enable =
				gm20ab_gr_intr_tpc_exception_sm_enable,
			.tpc_exception_sm_disable =
				gm20ab_gr_intr_tpc_exception_sm_disable,
			.handle_sm_exception =
				nvgpu_gr_intr_handle_sm_exception,
			.stall_isr = nvgpu_gr_intr_stall_isr,
		},
		.falcon = {
			.handle_fecs_ecc_error =
				gv11b_gr_falcon_handle_fecs_ecc_error,
			.read_fecs_ctxsw_mailbox =
				gm20b_gr_falcon_read_fecs_ctxsw_mailbox,
			.fecs_host_clear_intr =
				gm20b_gr_falcon_fecs_host_clear_intr,
			.fecs_host_intr_status =
				gm20b_gr_falcon_fecs_host_intr_status,
			.fecs_base_addr = gm20b_gr_falcon_fecs_base_addr,
			.gpccs_base_addr = gm20b_gr_falcon_gpccs_base_addr,
			.set_current_ctx_invalid =
				gm20b_gr_falcon_set_current_ctx_invalid,
			.dump_stats = gm20b_gr_falcon_fecs_dump_stats,
			.fecs_ctxsw_mailbox_size =
				gm20b_gr_falcon_get_fecs_ctxsw_mailbox_size,
			.get_fecs_ctx_state_store_major_rev_id =
				gm20b_gr_falcon_get_fecs_ctx_state_store_major_rev_id,
			.load_gpccs_dmem = gm20b_gr_falcon_load_gpccs_dmem,
			.load_fecs_dmem = gm20b_gr_falcon_load_fecs_dmem,
			.load_gpccs_imem = gm20b_gr_falcon_load_gpccs_imem,
			.load_fecs_imem = gm20b_gr_falcon_load_fecs_imem,
			.configure_fmodel = gm20b_gr_falcon_configure_fmodel,
			.start_ucode = gm20b_gr_falcon_start_ucode,
			.start_gpccs = gm20b_gr_falcon_start_gpccs,
			.start_fecs = gm20b_gr_falcon_start_fecs,
			.get_gpccs_start_reg_offset =
				gm20b_gr_falcon_get_gpccs_start_reg_offset,
			.bind_instblk = gm20b_gr_falcon_bind_instblk,
			.load_ctxsw_ucode_header =
				gm20b_gr_falcon_load_ctxsw_ucode_header,
			.load_ctxsw_ucode_boot =
				gm20b_gr_falcon_load_ctxsw_ucode_boot,
			.load_ctxsw_ucode =
					nvgpu_gr_falcon_load_ctxsw_ucode,
			.wait_mem_scrubbing =
					gm20b_gr_falcon_wait_mem_scrubbing,
			.wait_ctxsw_ready = gm20b_gr_falcon_wait_ctxsw_ready,
			.submit_fecs_method_op =
					gm20b_gr_falcon_submit_fecs_method_op,
			.submit_fecs_sideband_method_op =
				gm20b_gr_falcon_submit_fecs_sideband_method_op,
			.ctrl_ctxsw = gp10b_gr_falcon_ctrl_ctxsw,
			.get_current_ctx = gm20b_gr_falcon_get_current_ctx,
			.get_ctx_ptr = gm20b_gr_falcon_get_ctx_ptr,
			.get_fecs_current_ctx_data =
				gm20b_gr_falcon_get_fecs_current_ctx_data,
			.init_ctx_state = gp10b_gr_falcon_init_ctx_state,
			.fecs_host_int_enable =
					gv11b_gr_falcon_fecs_host_int_enable,
			.read_fecs_ctxsw_status0 =
				gm20b_gr_falcon_read_fecs_ctxsw_status0,
			.read_fecs_ctxsw_status1 =
				gm20b_gr_falcon_read_fecs_ctxsw_status1,
		},
	},
	.class = {
		.is_valid = gv11b_class_is_valid,
		.is_valid_gfx = gv11b_class_is_valid_gfx,
		.is_valid_compute = gv11b_class_is_valid_compute,
	},
	.fb = {
		.init_hw = gv11b_fb_init_hw,
		.init_fs_state = gv11b_fb_init_fs_state,
		.cbc_configure = gv11b_fb_cbc_configure,
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
		.vpr_info_fetch = gm20b_fb_vpr_info_fetch,
		.dump_vpr_info = gm20b_fb_dump_vpr_info,
		.dump_wpr_info = gm20b_fb_dump_wpr_info,
		.read_wpr_info = gm20b_fb_read_wpr_info,
		.is_debug_mode_enabled = gm20b_fb_debug_mode_enabled,
		.set_debug_mode = gm20b_fb_set_debug_mode,
		.tlb_invalidate = gm20b_fb_tlb_invalidate,
		.handle_replayable_fault = gv11b_fb_handle_replayable_mmu_fault,
		.mem_unlock = NULL,
		.write_mmu_fault_buffer_lo_hi =
				gv11b_fb_write_mmu_fault_buffer_lo_hi,
		.write_mmu_fault_buffer_get =
				fb_gv11b_write_mmu_fault_buffer_get,
		.write_mmu_fault_buffer_size =
				gv11b_fb_write_mmu_fault_buffer_size,
		.write_mmu_fault_status = gv11b_fb_write_mmu_fault_status,
		.read_mmu_fault_buffer_get =
				gv11b_fb_read_mmu_fault_buffer_get,
		.read_mmu_fault_buffer_put =
				gv11b_fb_read_mmu_fault_buffer_put,
		.read_mmu_fault_buffer_size =
				gv11b_fb_read_mmu_fault_buffer_size,
		.read_mmu_fault_addr_lo_hi = gv11b_fb_read_mmu_fault_addr_lo_hi,
		.read_mmu_fault_inst_lo_hi = gv11b_fb_read_mmu_fault_inst_lo_hi,
		.read_mmu_fault_info = gv11b_fb_read_mmu_fault_info,
		.read_mmu_fault_status = gv11b_fb_read_mmu_fault_status,
		.mmu_invalidate_replay = gv11b_fb_mmu_invalidate_replay,
		.is_fault_buf_enabled = gv11b_fb_is_fault_buf_enabled,
		.fault_buf_set_state_hw = gv11b_fb_fault_buf_set_state_hw,
		.fault_buf_configure_hw = gv11b_fb_fault_buf_configure_hw,
		.intr = {
			.enable = gv11b_fb_intr_enable,
			.disable = gv11b_fb_intr_disable,
			.isr = gv11b_fb_intr_isr,
			.is_mmu_fault_pending =
				gv11b_fb_intr_is_mmu_fault_pending,
		},
	},
	.cg = {
		.slcg_bus_load_gating_prod =
			gv11b_slcg_bus_load_gating_prod,
		.slcg_ce2_load_gating_prod =
			gv11b_slcg_ce2_load_gating_prod,
		.slcg_chiplet_load_gating_prod =
			gv11b_slcg_chiplet_load_gating_prod,
		.slcg_ctxsw_firmware_load_gating_prod =
			gv11b_slcg_ctxsw_firmware_load_gating_prod,
		.slcg_fb_load_gating_prod =
			gv11b_slcg_fb_load_gating_prod,
		.slcg_fifo_load_gating_prod =
			gv11b_slcg_fifo_load_gating_prod,
		.slcg_gr_load_gating_prod =
			gr_gv11b_slcg_gr_load_gating_prod,
		.slcg_ltc_load_gating_prod =
			ltc_gv11b_slcg_ltc_load_gating_prod,
		.slcg_perf_load_gating_prod =
			gv11b_slcg_perf_load_gating_prod,
		.slcg_priring_load_gating_prod =
			gv11b_slcg_priring_load_gating_prod,
		.slcg_pmu_load_gating_prod =
			gv11b_slcg_pmu_load_gating_prod,
		.slcg_therm_load_gating_prod =
			gv11b_slcg_therm_load_gating_prod,
		.slcg_xbar_load_gating_prod =
			gv11b_slcg_xbar_load_gating_prod,
		.blcg_bus_load_gating_prod =
			gv11b_blcg_bus_load_gating_prod,
		.blcg_ce_load_gating_prod =
			gv11b_blcg_ce_load_gating_prod,
		.blcg_ctxsw_firmware_load_gating_prod =
			gv11b_blcg_ctxsw_firmware_load_gating_prod,
		.blcg_fb_load_gating_prod =
			gv11b_blcg_fb_load_gating_prod,
		.blcg_fifo_load_gating_prod =
			gv11b_blcg_fifo_load_gating_prod,
		.blcg_gr_load_gating_prod =
			gv11b_blcg_gr_load_gating_prod,
		.blcg_ltc_load_gating_prod =
			gv11b_blcg_ltc_load_gating_prod,
		.blcg_pwr_csb_load_gating_prod =
			gv11b_blcg_pwr_csb_load_gating_prod,
		.blcg_pmu_load_gating_prod =
			gv11b_blcg_pmu_load_gating_prod,
		.blcg_xbar_load_gating_prod =
			gv11b_blcg_xbar_load_gating_prod,
		.pg_gr_load_gating_prod =
			gr_gv11b_pg_gr_load_gating_prod,
	},
	.fifo = {
		.init_fifo_setup_hw = gv11b_init_fifo_setup_hw,
		.preempt_channel = gv11b_fifo_preempt_channel,
		.preempt_tsg = gv11b_fifo_preempt_tsg,
		.preempt_trigger = gv11b_fifo_preempt_trigger,
		.preempt_runlists_for_rc = gv11b_fifo_preempt_runlists_for_rc,
		.preempt_poll_pbdma = gv11b_fifo_preempt_poll_pbdma,
		.init_pbdma_map = gk20a_fifo_init_pbdma_map,
		.is_preempt_pending = gv11b_fifo_is_preempt_pending,
		.reset_enable_hw = gv11b_init_fifo_reset_enable_hw,
		.recover = gv11b_fifo_recover,
		.intr_set_recover_mask = gv11b_fifo_intr_set_recover_mask,
		.intr_unset_recover_mask = gv11b_fifo_intr_unset_recover_mask,
		.setup_sw = nvgpu_fifo_setup_sw,
		.cleanup_sw = nvgpu_fifo_cleanup_sw,
		.set_sm_exception_type_mask = gk20a_tsg_set_sm_exception_type_mask,
		.intr_0_enable = gv11b_fifo_intr_0_enable,
		.intr_1_enable = gk20a_fifo_intr_1_enable,
		.intr_0_isr = gv11b_fifo_intr_0_isr,
		.intr_1_isr = gk20a_fifo_intr_1_isr,
		.handle_sched_error = gv11b_fifo_handle_sched_error,
		.ctxsw_timeout_enable = gv11b_fifo_ctxsw_timeout_enable,
		.handle_ctxsw_timeout = gv11b_fifo_handle_ctxsw_timeout,
		.trigger_mmu_fault = NULL,
		.get_mmu_fault_info = NULL,
		.get_mmu_fault_desc = NULL,
		.get_mmu_fault_client_desc = NULL,
		.get_mmu_fault_gpc_desc = NULL,
		.get_runlist_timeslice = gk20a_fifo_get_runlist_timeslice,
		.get_pb_timeslice = gk20a_fifo_get_pb_timeslice,
		.mmu_fault_id_to_pbdma_id = gv11b_fifo_mmu_fault_id_to_pbdma_id,
	},
	.engine = {
		.is_fault_engine_subid_gpc = gv11b_is_fault_engine_subid_gpc,
		.get_mask_on_id = nvgpu_engine_get_mask_on_id,
		.init_info = nvgpu_engine_init_info,
		.init_ce_info = gp10b_engine_init_ce_info,
	},
	.pbdma = {
		.setup_sw = nvgpu_pbdma_setup_sw,
		.cleanup_sw = nvgpu_pbdma_cleanup_sw,
		.setup_hw = gv11b_pbdma_setup_hw,
		.intr_enable = gv11b_pbdma_intr_enable,
		.acquire_val = gm20b_pbdma_acquire_val,
		.get_signature = gp10b_pbdma_get_signature,
		.dump_status = gm20b_pbdma_dump_status,
		.handle_intr_0 = gv11b_pbdma_handle_intr_0,
		.handle_intr_1 = gv11b_pbdma_handle_intr_1,
		.handle_intr = gm20b_pbdma_handle_intr,
		.read_data = gm20b_pbdma_read_data,
		.reset_header = gm20b_pbdma_reset_header,
		.device_fatal_0_intr_descs =
			gm20b_pbdma_device_fatal_0_intr_descs,
		.channel_fatal_0_intr_descs =
			gv11b_pbdma_channel_fatal_0_intr_descs,
		.restartable_0_intr_descs =
			gm20b_pbdma_restartable_0_intr_descs,
		.find_for_runlist = nvgpu_pbdma_find_for_runlist,
		.format_gpfifo_entry =
			gm20b_pbdma_format_gpfifo_entry,
		.get_gp_base = gm20b_pbdma_get_gp_base,
		.get_gp_base_hi = gm20b_pbdma_get_gp_base_hi,
		.get_fc_formats = NULL,
		.get_fc_pb_header = gv11b_pbdma_get_fc_pb_header,
		.get_fc_subdevice = gm20b_pbdma_get_fc_subdevice,
		.get_fc_target = gv11b_pbdma_get_fc_target,
		.get_ctrl_hce_priv_mode_yes =
			gm20b_pbdma_get_ctrl_hce_priv_mode_yes,
		.get_userd_aperture_mask = gm20b_pbdma_get_userd_aperture_mask,
		.get_userd_addr = gm20b_pbdma_get_userd_addr,
		.get_userd_hi_addr = gm20b_pbdma_get_userd_hi_addr,
		.get_fc_runlist_timeslice =
			gp10b_pbdma_get_fc_runlist_timeslice,
		.get_config_auth_level_privileged =
			gp10b_pbdma_get_config_auth_level_privileged,
		.set_channel_info_veid = gv11b_pbdma_set_channel_info_veid,
		.config_userd_writeback_enable =
			gv11b_pbdma_config_userd_writeback_enable,
	},
	.sync = {
#ifdef CONFIG_TEGRA_GK20A_NVHOST
		.syncpt = {
			.alloc_buf = gv11b_syncpt_alloc_buf,
			.free_buf = gv11b_syncpt_free_buf,
			.add_wait_cmd = gv11b_syncpt_add_wait_cmd,
			.get_wait_cmd_size =
					gv11b_syncpt_get_wait_cmd_size,
			.add_incr_cmd = gv11b_syncpt_add_incr_cmd,
			.get_incr_cmd_size =
					gv11b_syncpt_get_incr_cmd_size,
			.get_incr_per_release =
					gv11b_syncpt_get_incr_per_release,
			.get_sync_ro_map = gv11b_syncpt_get_sync_ro_map,
		},
#endif
		.sema = {
			.get_wait_cmd_size = gv11b_sema_get_wait_cmd_size,
			.get_incr_cmd_size = gv11b_sema_get_incr_cmd_size,
			.add_cmd = gv11b_sema_add_cmd,
		},
	},
	.engine_status = {
		.read_engine_status_info =
			gv100_read_engine_status_info,
		.dump_engine_status = gv100_dump_engine_status,
	},
	.pbdma_status = {
		.read_pbdma_status_info =
			gm20b_read_pbdma_status_info,
	},
	.ramfc = {
		.setup = gv11b_ramfc_setup,
		.capture_ram_dump = gv11b_ramfc_capture_ram_dump,
		.commit_userd = gp10b_ramfc_commit_userd,
		.get_syncpt = NULL,
		.set_syncpt = NULL,
	},
	.ramin = {
		.set_gr_ptr = gv11b_ramin_set_gr_ptr,
		.set_big_page_size = gm20b_ramin_set_big_page_size,
		.init_pdb = gp10b_ramin_init_pdb,
		.init_subctx_pdb = gv11b_ramin_init_subctx_pdb,
		.set_adr_limit = NULL,
		.base_shift = gk20a_ramin_base_shift,
		.alloc_size = gk20a_ramin_alloc_size,
		.set_eng_method_buffer = gv11b_ramin_set_eng_method_buffer,
	},
	.runlist = {
		.reschedule = gv11b_runlist_reschedule,
		.reschedule_preempt_next_locked = gk20a_fifo_reschedule_preempt_next,
		.update_for_channel = gk20a_runlist_update_for_channel,
		.reload = gk20a_runlist_reload,
		.count_max = gv11b_runlist_count_max,
		.entry_size = gv11b_runlist_entry_size,
		.length_max = gk20a_runlist_length_max,
		.get_tsg_entry = gv11b_runlist_get_tsg_entry,
		.get_ch_entry = gv11b_runlist_get_ch_entry,
		.hw_submit = gk20a_runlist_hw_submit,
		.wait_pending = gk20a_runlist_wait_pending,
		.write_state = gk20a_runlist_write_state,
	},
	.userd = {
		.setup_sw = nvgpu_userd_setup_sw,
		.cleanup_sw = nvgpu_userd_cleanup_sw,
#ifdef NVGPU_USERD
		.init_mem = gk20a_userd_init_mem,
		.gp_get = gv11b_userd_gp_get,
		.gp_put = gv11b_userd_gp_put,
		.pb_get = gv11b_userd_pb_get,
		.entry_size = gk20a_userd_entry_size,
#endif
	},
	.channel = {
		.alloc_inst = nvgpu_channel_alloc_inst,
		.free_inst = nvgpu_channel_free_inst,
		.bind = gm20b_channel_bind,
		.unbind = gv11b_channel_unbind,
		.enable = gk20a_channel_enable,
		.disable = gk20a_channel_disable,
		.count = gv11b_channel_count,
		.read_state = gv11b_channel_read_state,
		.force_ctx_reload = gm20b_channel_force_ctx_reload,
		.abort_clean_up = nvgpu_channel_abort_clean_up,
		.suspend_all_serviceable_ch =
                        nvgpu_channel_suspend_all_serviceable_ch,
		.resume_all_serviceable_ch =
                        nvgpu_channel_resume_all_serviceable_ch,
		.set_error_notifier = nvgpu_set_error_notifier_if_empty,
		.reset_faulted = gv11b_channel_reset_faulted,
		.debug_dump = gv11b_channel_debug_dump,
	},
	.tsg = {
		.enable = gv11b_tsg_enable,
		.disable = nvgpu_tsg_disable,
		.init_eng_method_buffers = gv11b_tsg_init_eng_method_buffers,
		.deinit_eng_method_buffers =
			gv11b_tsg_deinit_eng_method_buffers,
		.bind_channel = NULL,
		.bind_channel_eng_method_buffers =
			gv11b_tsg_bind_channel_eng_method_buffers,
		.unbind_channel = NULL,
		.unbind_channel_check_hw_state =
				nvgpu_tsg_unbind_channel_check_hw_state,
		.unbind_channel_check_ctx_reload =
				nvgpu_tsg_unbind_channel_check_ctx_reload,
		.unbind_channel_check_eng_faulted =
				gv11b_tsg_unbind_channel_check_eng_faulted,
		.check_ctxsw_timeout = nvgpu_tsg_check_ctxsw_timeout,
		.force_reset = nvgpu_tsg_force_reset_ch,
		.post_event_id = nvgpu_tsg_post_event_id,
		.set_timeslice = nvgpu_tsg_set_timeslice,
		.default_timeslice_us = nvgpu_tsg_default_timeslice_us,
	},
	.usermode = {
		.setup_hw = NULL,
		.base = gv11b_usermode_base,
		.bus_base = gv11b_usermode_bus_base,
		.ring_doorbell = gv11b_usermode_ring_doorbell,
		.doorbell_token = gv11b_usermode_doorbell_token,
	},
	.netlist = {
		.get_netlist_name = gv11b_netlist_get_name,
		.is_fw_defined = gv11b_netlist_is_firmware_defined,
	},
	.mm = {
		.vm_bind_channel = nvgpu_vm_bind_channel,
		.setup_hw = nvgpu_mm_setup_hw,
		.is_bar1_supported = gv11b_mm_is_bar1_supported,
		.alloc_inst_block = gk20a_alloc_inst_block,
		.init_inst_block = gv11b_init_inst_block,
		.init_bar2_vm = gp10b_init_bar2_vm,
		.remove_bar2_vm = gp10b_remove_bar2_vm,
		.fault_info_mem_destroy = gv11b_mm_fault_info_mem_destroy,
		.mmu_fault_disable_hw = gv11b_mm_mmu_fault_disable_hw,
		.bar1_map_userd = NULL,
		.mmu_fault = {
			.setup_sw = gv11b_mm_mmu_fault_setup_sw,
			.setup_hw = gv11b_mm_mmu_fault_setup_hw,
		},
		.cache = {
			.fb_flush = gk20a_mm_fb_flush,
			.l2_invalidate = gk20a_mm_l2_invalidate,
			.l2_flush = gv11b_mm_l2_flush,
			.cbc_clean = gk20a_mm_cbc_clean,
		},
		.gmmu = {
			.get_mmu_levels = gp10b_mm_get_mmu_levels,
			.map = nvgpu_gmmu_map_locked,
			.unmap = nvgpu_gmmu_unmap_locked,
			.get_big_page_sizes = gm20b_mm_get_big_page_sizes,
			.get_default_big_page_size =
				gp10b_mm_get_default_big_page_size,
			.get_iommu_bit = gp10b_mm_get_iommu_bit,
			.gpu_phys_addr = gv11b_gpu_phys_addr,
		}
	},
	.therm = {
		.init_therm_setup_hw = gv11b_init_therm_setup_hw,
		.init_elcg_mode = gv11b_therm_init_elcg_mode,
		.init_blcg_mode = gm20b_therm_init_blcg_mode,
		.elcg_init_idle_filters = gv11b_elcg_init_idle_filters,
	},
	.pmu = {
		/*
		 * Basic init ops are must, as PMU engine used by ACR to
		 * load & bootstrap GR LS falcons without LS PMU, remaining
		 * ops can be assigned/ignored as per build flag request
		 */
		/* Basic init ops */
		.is_pmu_supported = gv11b_is_pmu_supported,
		.falcon_base_addr = gk20a_pmu_falcon_base_addr,
		.pmu_reset = nvgpu_pmu_reset,
		.reset_engine = gp106_pmu_engine_reset,
		.is_engine_in_reset = gp106_pmu_is_engine_in_reset,
		.is_debug_mode_enabled = gm20b_pmu_is_debug_mode_en,
		.setup_apertures = gv11b_setup_apertures,
		.secured_pmu_start = gm20b_secured_pmu_start,
		.write_dmatrfbase = gp10b_write_dmatrfbase,
		/* ISR */
		.pmu_enable_irq = gk20a_pmu_enable_irq,
#ifdef NVGPU_LS_PMU
		.get_irqdest = gv11b_pmu_get_irqdest,
		.handle_ext_irq = gv11b_pmu_handle_ext_irq,
		.pmu_is_interrupted = gk20a_pmu_is_interrupted,
		.pmu_isr = gk20a_pmu_isr,
		/* queue */
		.pmu_get_queue_head = pwr_pmu_queue_head_r,
		.pmu_get_queue_head_size = pwr_pmu_queue_head__size_1_v,
		.pmu_get_queue_tail = pwr_pmu_queue_tail_r,
		.pmu_get_queue_tail_size = pwr_pmu_queue_tail__size_1_v,
		.pmu_queue_head = gk20a_pmu_queue_head,
		.pmu_queue_tail = gk20a_pmu_queue_tail,
		.pmu_msgq_tail = gk20a_pmu_msgq_tail,
		/* mutex */
		.pmu_mutex_size = pwr_pmu_mutex__size_1_v,
		.pmu_mutex_owner = gk20a_pmu_mutex_owner,
		.pmu_mutex_acquire = gk20a_pmu_mutex_acquire,
		.pmu_mutex_release = gk20a_pmu_mutex_release,
		/* power-gating */
		.pmu_pg_init_param = gv11b_pg_gr_init,
		.pmu_setup_elpg = gv11b_pmu_setup_elpg,
		.pmu_pg_idle_counter_config = gk20a_pmu_pg_idle_counter_config,
		.pmu_pg_supported_engines_list = gm20b_pmu_pg_engines_list,
		.pmu_pg_engines_feature_list = gm20b_pmu_pg_feature_list,
		.pmu_pg_set_sub_feature_mask = gv11b_pg_set_subfeature_mask,
		.pmu_elpg_statistics = gp106_pmu_elpg_statistics,
		.pmu_dump_elpg_stats = gk20a_pmu_dump_elpg_stats,
		/* perfmon */
		.pmu_init_perfmon_counter = gk20a_pmu_init_perfmon_counter,
		.pmu_read_idle_counter = gk20a_pmu_read_idle_counter,
		.pmu_reset_idle_counter = gk20a_pmu_reset_idle_counter,
		.pmu_read_idle_intr_status = gk20a_pmu_read_idle_intr_status,
		.pmu_clear_idle_intr_status = gk20a_pmu_clear_idle_intr_status,
		.pmu_init_perfmon = nvgpu_pmu_init_perfmon_rpc,
		.pmu_perfmon_start_sampling = nvgpu_pmu_perfmon_start_sampling_rpc,
		.pmu_perfmon_stop_sampling = nvgpu_pmu_perfmon_stop_sampling_rpc,
		.pmu_perfmon_get_samples_rpc = nvgpu_pmu_perfmon_get_samples_rpc,
		/* debug */
		.dump_secure_fuses = pmu_dump_security_fuses_gm20b,
		.pmu_dump_falcon_stats = gk20a_pmu_dump_falcon_stats,
		/* PMU uocde */
		.save_zbc = gm20b_pmu_save_zbc,
		.pmu_clear_bar0_host_err_status =
			gm20b_clear_pmu_bar0_host_err_status,
		.bar0_error_status = gk20a_pmu_bar0_error_status,
		.flcn_setup_boot_config = gm20b_pmu_flcn_setup_boot_config,
#endif
	},
	.clk_arb = {
		.check_clk_arb_support = gp10b_check_clk_arb_support,
		.get_arbiter_clk_domains = gp10b_get_arbiter_clk_domains,
		.get_arbiter_f_points = gp10b_get_arbiter_f_points,
		.get_arbiter_clk_range = gp10b_get_arbiter_clk_range,
		.get_arbiter_clk_default = gp10b_get_arbiter_clk_default,
		.arbiter_clk_init = gp10b_init_clk_arbiter,
		.clk_arb_run_arbiter_cb = gp10b_clk_arb_run_arbiter_cb,
		.clk_arb_cleanup = gp10b_clk_arb_cleanup,
	},
	.regops = {
		.exec_regops = exec_regops_gk20a,
		.get_global_whitelist_ranges =
			gv11b_get_global_whitelist_ranges,
		.get_global_whitelist_ranges_count =
			gv11b_get_global_whitelist_ranges_count,
		.get_context_whitelist_ranges =
			gv11b_get_context_whitelist_ranges,
		.get_context_whitelist_ranges_count =
			gv11b_get_context_whitelist_ranges_count,
		.get_runcontrol_whitelist = gv11b_get_runcontrol_whitelist,
		.get_runcontrol_whitelist_count =
			gv11b_get_runcontrol_whitelist_count,
		.get_qctl_whitelist = gv11b_get_qctl_whitelist,
		.get_qctl_whitelist_count = gv11b_get_qctl_whitelist_count,
	},
	.mc = {
		.intr_mask = mc_gp10b_intr_mask,
		.intr_enable = mc_gv11b_intr_enable,
		.intr_unit_config = mc_gp10b_intr_unit_config,
		.isr_stall = mc_gp10b_isr_stall,
		.intr_stall = mc_gp10b_intr_stall,
		.intr_stall_pause = mc_gp10b_intr_stall_pause,
		.intr_stall_resume = mc_gp10b_intr_stall_resume,
		.intr_nonstall = mc_gp10b_intr_nonstall,
		.intr_nonstall_pause = mc_gp10b_intr_nonstall_pause,
		.intr_nonstall_resume = mc_gp10b_intr_nonstall_resume,
		.isr_nonstall = gm20b_mc_isr_nonstall,
		.enable = gm20b_mc_enable,
		.disable = gm20b_mc_disable,
		.reset = gm20b_mc_reset,
		.is_intr1_pending = mc_gp10b_is_intr1_pending,
		.log_pending_intrs = mc_gp10b_log_pending_intrs,
		.is_intr_hub_pending = gv11b_mc_is_intr_hub_pending,
		.is_stall_and_eng_intr_pending =
					gv11b_mc_is_stall_and_eng_intr_pending,
		.reset_mask = gm20b_mc_reset_mask,
		.is_enabled = gm20b_mc_is_enabled,
		.fb_reset = NULL,
		.ltc_isr = mc_gp10b_ltc_isr,
		.is_mmu_fault_pending = gv11b_mc_is_mmu_fault_pending,
	},
	.debug = {
		.show_dump = gk20a_debug_show_dump,
	},
#ifdef NVGPU_DEBUGGER
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
#endif
	.perf = {
		.enable_membuf = gv11b_perf_enable_membuf,
		.disable_membuf = gv11b_perf_disable_membuf,
		.membuf_reset_streaming = gv11b_perf_membuf_reset_streaming,
		.get_membuf_pending_bytes = gv11b_perf_get_membuf_pending_bytes,
		.set_membuf_handled_bytes = gv11b_perf_set_membuf_handled_bytes,
		.get_membuf_overflow_status =
			gv11b_perf_get_membuf_overflow_status,
		.get_pmm_per_chiplet_offset =
			gv11b_perf_get_pmm_per_chiplet_offset,
	},
	.perfbuf = {
		.perfbuf_enable = nvgpu_perfbuf_enable_locked,
		.perfbuf_disable = nvgpu_perfbuf_disable_locked,
	},
	.bus = {
		.init_hw = gk20a_bus_init_hw,
		.isr = gk20a_bus_isr,
		.bar1_bind = gm20b_bus_bar1_bind,
		.bar2_bind = gp10b_bus_bar2_bind,
		.set_bar0_window = gk20a_bus_set_bar0_window,
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
		.get_max_buffer_size = nvgpu_css_get_max_buffer_size,
	},
#endif
	.falcon = {
		.reset = gk20a_falcon_reset,
		.set_irq = gk20a_falcon_set_irq,
		.clear_halt_interrupt_status =
			gk20a_falcon_clear_halt_interrupt_status,
		.is_falcon_cpu_halted =  gk20a_is_falcon_cpu_halted,
		.is_falcon_idle =  gk20a_is_falcon_idle,
		.is_falcon_scrubbing_done =  gk20a_is_falcon_scrubbing_done,
		.copy_from_dmem = gk20a_falcon_copy_from_dmem,
		.copy_to_dmem = gk20a_falcon_copy_to_dmem,
		.copy_to_imem = gk20a_falcon_copy_to_imem,
		.copy_from_imem = gk20a_falcon_copy_from_imem,
		.bootstrap = gk20a_falcon_bootstrap,
		.dump_falcon_stats = gk20a_falcon_dump_stats,
		.mailbox_read = gk20a_falcon_mailbox_read,
		.mailbox_write = gk20a_falcon_mailbox_write,
		.get_falcon_ctls = gk20a_falcon_get_ctls,
		.get_mem_size = gk20a_falcon_get_mem_size,
		.get_ports_count = gk20a_falcon_get_ports_count
	},
	.priv_ring = {
		.enable_priv_ring = gm20b_priv_ring_enable,
		.isr = gp10b_priv_ring_isr,
		.decode_error_code = gp10b_priv_ring_decode_error_code,
		.set_ppriv_timeout_settings =
			gm20b_priv_set_timeout_settings,
		.enum_ltc = gm20b_priv_ring_enum_ltc,
		.get_gpc_count = gm20b_priv_ring_get_gpc_count,
		.get_fbp_count = gm20b_priv_ring_get_fbp_count,
	},
	.fuse = {
		.check_priv_security = gp10b_fuse_check_priv_security,
		.is_opt_ecc_enable = gp10b_fuse_is_opt_ecc_enable,
		.is_opt_feature_override_disable =
			gp10b_fuse_is_opt_feature_override_disable,
		.fuse_status_opt_fbio = gm20b_fuse_status_opt_fbio,
		.fuse_status_opt_fbp = gm20b_fuse_status_opt_fbp,
		.fuse_status_opt_rop_l2_fbp = gm20b_fuse_status_opt_rop_l2_fbp,
		.fuse_status_opt_tpc_gpc = gm20b_fuse_status_opt_tpc_gpc,
		.fuse_ctrl_opt_tpc_gpc = gm20b_fuse_ctrl_opt_tpc_gpc,
		.fuse_opt_sec_debug_en = gm20b_fuse_opt_sec_debug_en,
		.fuse_opt_priv_sec_en = gm20b_fuse_opt_priv_sec_en,
		.read_vin_cal_fuse_rev = NULL,
		.read_vin_cal_slope_intercept_fuse = NULL,
		.read_vin_cal_gain_offset_fuse = NULL,
		.read_gcplex_config_fuse =
				nvgpu_tegra_fuse_read_gcplex_config_fuse,
	},
	.top = {
		.device_info_parse_enum = gm20b_device_info_parse_enum,
		.device_info_parse_data = gp10b_device_info_parse_data,
		.get_num_engine_type_entries =
				gp10b_get_num_engine_type_entries,
		.get_device_info = gp10b_get_device_info,
		.is_engine_gr = gm20b_is_engine_gr,
		.is_engine_ce = gp10b_is_engine_ce,
		.get_ce_inst_id = NULL,
		.get_max_gpc_count = gm20b_top_get_max_gpc_count,
		.get_max_tpc_per_gpc_count =
			gm20b_top_get_max_tpc_per_gpc_count,
		.get_max_fbps_count = gm20b_top_get_max_fbps_count,
		.get_max_ltc_per_fbp = gm20b_top_get_max_ltc_per_fbp,
		.get_max_lts_per_ltc = gm20b_top_get_max_lts_per_ltc,
		.get_num_lce = gv11b_top_get_num_lce,
	},
	.chip_init_gpu_characteristics = gv11b_init_gpu_characteristics,
	.get_litter_value = gv11b_get_litter_value,
};

int gv11b_init_hal(struct gk20a *g)
{
	struct gpu_ops *gops = &g->ops;

	gops->ltc = gv11b_ops.ltc;
	gops->cbc = gv11b_ops.cbc;
	gops->ce = gv11b_ops.ce;
	gops->gr = gv11b_ops.gr;
	gops->class = gv11b_ops.class;
	gops->gr.ctxsw_prog = gv11b_ops.gr.ctxsw_prog;
	gops->gr.config = gv11b_ops.gr.config;
	gops->fb = gv11b_ops.fb;
	gops->cg = gv11b_ops.cg;
	gops->fifo = gv11b_ops.fifo;
	gops->engine = gv11b_ops.engine;
	gops->pbdma = gv11b_ops.pbdma;
	gops->ramfc = gv11b_ops.ramfc;
	gops->ramin = gv11b_ops.ramin;
	gops->runlist = gv11b_ops.runlist;
	gops->userd = gv11b_ops.userd;
	gops->channel = gv11b_ops.channel;
	gops->tsg = gv11b_ops.tsg;
	gops->usermode = gv11b_ops.usermode;
	gops->sync = gv11b_ops.sync;
	gops->engine_status = gv11b_ops.engine_status;
	gops->pbdma_status = gv11b_ops.pbdma_status;
	gops->netlist = gv11b_ops.netlist;
	gops->mm = gv11b_ops.mm;
	gops->therm = gv11b_ops.therm;
	gops->pmu = gv11b_ops.pmu;
	gops->regops = gv11b_ops.regops;
	gops->mc = gv11b_ops.mc;
	gops->debug = gv11b_ops.debug;
#ifdef NVGPU_DEBUGGER
	gops->debugger = gv11b_ops.debugger;
#endif
	gops->perf = gv11b_ops.perf;
	gops->perfbuf = gv11b_ops.perfbuf;
	gops->bus = gv11b_ops.bus;
	gops->ptimer = gv11b_ops.ptimer;
#if defined(CONFIG_GK20A_CYCLE_STATS)
	gops->css = gv11b_ops.css;
#endif
	gops->falcon = gv11b_ops.falcon;
	gops->priv_ring = gv11b_ops.priv_ring;
	gops->fuse = gv11b_ops.fuse;
	gops->clk_arb = gv11b_ops.clk_arb;
	gops->top = gv11b_ops.top;

	/* Lone functions */
	gops->chip_init_gpu_characteristics =
		gv11b_ops.chip_init_gpu_characteristics;
	gops->get_litter_value = gv11b_ops.get_litter_value;
	gops->semaphore_wakeup = gk20a_channel_semaphore_wakeup;

	nvgpu_set_enabled(g, NVGPU_GR_USE_DMA_FOR_FW_BOOTSTRAP, false);

	/* Read fuses to check if gpu needs to boot in secure/non-secure mode */
	if (gops->fuse.check_priv_security(g) != 0) {
		/* Do not boot gpu */
		return -EINVAL;
	}

	/* priv security dependent ops */
	if (nvgpu_is_enabled(g, NVGPU_SEC_PRIVSECURITY)) {
		gops->gr.falcon.load_ctxsw_ucode =
			nvgpu_gr_falcon_load_secure_ctxsw_ucode;
	} else {
		/* non-secure boot */
		gops->pmu.pmu_ns_bootstrap = gv11b_pmu_bootstrap;
		gops->pmu.setup_apertures =
			gm20b_pmu_ns_setup_apertures;
	}

	nvgpu_set_enabled(g, NVGPU_PMU_FECS_BOOTSTRAP_DONE, false);
	nvgpu_set_enabled(g, NVGPU_FECS_TRACE_VA, true);

	nvgpu_set_enabled(g, NVGPU_SUPPORT_MULTIPLE_WPR, false);
	nvgpu_set_enabled(g, NVGPU_SUPPORT_ZBC_STENCIL, true);
	nvgpu_set_enabled(g, NVGPU_SUPPORT_PREEMPTION_GFXP, true);
	nvgpu_set_enabled(g, NVGPU_SUPPORT_PLATFORM_ATOMIC, true);

	/*
	 * gv11b bypasses the IOMMU since it uses a special nvlink path to
	 * memory.
	 */
	nvgpu_set_enabled(g, NVGPU_MM_BYPASSES_IOMMU, true);

	g->name = "gv11b";

	return 0;
}
