/*
 * Copyright (c) 2015-2019, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/debugger.h>
#include <nvgpu/enabled.h>
#include <nvgpu/vgpu/vgpu.h>
#include <nvgpu/error_notifier.h>
#include <nvgpu/channel.h>
#include <nvgpu/gr/gr.h>

#include <nvgpu/vgpu/ce_vgpu.h>
#include <nvgpu/vgpu/vm_vgpu.h>

#include "hal/bus/bus_gk20a.h"
#include "hal/bus/bus_gm20b.h"
#include "hal/mm/gmmu/gmmu_gm20b.h"
#include "hal/mm/gmmu/gmmu_gp10b.h"
#include "hal/regops/regops_gp10b.h"
#include "hal/class/class_gp10b.h"
#include "hal/fifo/engines_gm20b.h"
#include "hal/fifo/pbdma_gm20b.h"
#include "hal/fifo/pbdma_gp10b.h"
#include "hal/fifo/ramin_gk20a.h"
#include "hal/fifo/ramin_gm20b.h"
#include "hal/fifo/ramin_gp10b.h"
#include "hal/fifo/runlist_gk20a.h"
#include "hal/fifo/userd_gk20a.h"
#include "hal/fifo/mmu_fault_gm20b.h"
#include "hal/fifo/mmu_fault_gp10b.h"
#include "hal/therm/therm_gm20b.h"
#include "hal/therm/therm_gp10b.h"
#include "hal/ltc/ltc_gm20b.h"
#include "hal/ltc/ltc_gp10b.h"
#include "hal/fb/fb_gm20b.h"
#include "hal/fb/fb_gp10b.h"
#include "hal/gr/fecs_trace/fecs_trace_gm20b.h"
#include "hal/gr/init/gr_init_gm20b.h"
#include "hal/gr/init/gr_init_gp10b.h"
#include "hal/gr/config/gr_config_gm20b.h"
#include "hal/gr/ctxsw_prog/ctxsw_prog_gm20b.h"
#include "hal/gr/ctxsw_prog/ctxsw_prog_gp10b.h"
#include "hal/netlist/netlist_gp10b.h"
#include "hal/perf/perf_gm20b.h"
#include "hal/sync/syncpt_cmdbuf_gk20a.h"
#include "hal/sync/sema_cmdbuf_gk20a.h"
#include "hal/init/hal_gp10b.h"

#include "common/fifo/channel_gm20b.h"
#include "common/clk_arb/clk_arb_gp10b.h"

#include "common/vgpu/fifo/fifo_vgpu.h"
#include "common/vgpu/fifo/runlist_vgpu.h"
#include "common/vgpu/fifo/ramfc_vgpu.h"
#include "common/vgpu/fifo/userd_vgpu.h"
#include "common/vgpu/gr/gr_vgpu.h"
#include "common/vgpu/gr/ctx_vgpu.h"
#include "common/vgpu/ltc/ltc_vgpu.h"
#include "common/vgpu/mm/mm_vgpu.h"
#include "common/vgpu/cbc/cbc_vgpu.h"
#include "common/vgpu/debugger_vgpu.h"
#include "common/vgpu/gr/fecs_trace_vgpu.h"
#include "common/vgpu/perf/perf_vgpu.h"
#include "common/vgpu/perf/cyclestats_snapshot_vgpu.h"
#include "common/vgpu/ptimer/ptimer_vgpu.h"
#include "common/vgpu/init/init_vgpu.h"
#include "vgpu_hal_gp10b.h"

#include "gk20a/gr_gk20a.h"

#include "gp10b/mm_gp10b.h"
#include "gp10b/gr_gp10b.h"

#include "gm20b/gr_gm20b.h"
#include "gm20b/mm_gm20b.h"

#include <nvgpu/hw/gp10b/hw_pram_gp10b.h>
#include <nvgpu/hw/gp10b/hw_pwr_gp10b.h>

static const struct gpu_ops vgpu_gp10b_ops = {
	.ltc = {
		.determine_L2_size_bytes = vgpu_determine_L2_size_bytes,
		.set_zbc_color_entry = NULL,
		.set_zbc_depth_entry = NULL,
		.init_fs_state = vgpu_ltc_init_fs_state,
		.flush = NULL,
		.set_enabled = NULL,
		.pri_is_ltc_addr = gm20b_ltc_pri_is_ltc_addr,
		.is_ltcs_ltss_addr = gm20b_ltc_is_ltcs_ltss_addr,
		.is_ltcn_ltss_addr = gm20b_ltc_is_ltcn_ltss_addr,
		.split_lts_broadcast_addr = gm20b_ltc_split_lts_broadcast_addr,
		.split_ltc_broadcast_addr = gm20b_ltc_split_ltc_broadcast_addr,
		.intr = {
			.configure = NULL,
			.isr = NULL,
			.en_illegal_compstat = NULL,
		},
	},
	.cbc = {
		.init = NULL,
		.alloc_comptags = vgpu_cbc_alloc_comptags,
		.ctrl = NULL,
		.fix_config = NULL,
	},
	.ce = {
		.isr_stall = NULL,
		.isr_nonstall = NULL,
		.get_num_pce = vgpu_ce_get_num_pce,
	},
	.gr = {
		.set_alpha_circular_buffer_size = NULL,
		.set_circular_buffer_size = NULL,
		.get_sm_dsm_perf_regs = gr_gm20b_get_sm_dsm_perf_regs,
		.get_sm_dsm_perf_ctrl_regs = gr_gm20b_get_sm_dsm_perf_ctrl_regs,
		.set_hww_esr_report_mask = NULL,
		.set_gpc_tpc_mask = NULL,
		.is_tpc_addr = gr_gm20b_is_tpc_addr,
		.get_tpc_num = gr_gm20b_get_tpc_num,
		.dump_gr_regs = NULL,
		.update_pc_sampling = vgpu_gr_update_pc_sampling,
		.get_rop_l2_en_mask = vgpu_gr_rop_l2_en_mask,
		.init_sm_dsm_reg_info = gr_gm20b_init_sm_dsm_reg_info,
		.init_cyclestats = vgpu_gr_init_cyclestats,
		.set_sm_debug_mode = vgpu_gr_set_sm_debug_mode,
		.bpt_reg_info = NULL,
		.get_lrf_tex_ltc_dram_override = NULL,
		.update_smpc_ctxsw_mode = vgpu_gr_update_smpc_ctxsw_mode,
		.update_hwpm_ctxsw_mode = vgpu_gr_update_hwpm_ctxsw_mode,
		.record_sm_error_state = gm20b_gr_record_sm_error_state,
		.clear_sm_error_state = vgpu_gr_clear_sm_error_state,
		.suspend_contexts = vgpu_gr_suspend_contexts,
		.resume_contexts = vgpu_gr_resume_contexts,
		.trigger_suspend = NULL,
		.wait_for_pause = gr_gk20a_wait_for_pause,
		.resume_from_pause = NULL,
		.clear_sm_errors = gr_gk20a_clear_sm_errors,
		.tpc_enabled_exceptions = NULL,
		.get_esr_sm_sel = gk20a_gr_get_esr_sm_sel,
		.sm_debugger_attached = NULL,
		.suspend_single_sm = NULL,
		.suspend_all_sms = NULL,
		.resume_single_sm = NULL,
		.resume_all_sms = NULL,
		.get_sm_hww_warp_esr = NULL,
		.get_sm_hww_global_esr = NULL,
		.get_sm_hww_warp_esr_pc = NULL,
		.get_sm_no_lock_down_hww_global_esr_mask =
			gk20a_gr_get_sm_no_lock_down_hww_global_esr_mask,
		.lock_down_sm = NULL,
		.wait_for_sm_lock_down = NULL,
		.clear_sm_hww = NULL,
		.init_ovr_sm_dsm_perf =  gk20a_gr_init_ovr_sm_dsm_perf,
		.get_ovr_perf_regs = gk20a_gr_get_ovr_perf_regs,
		.set_boosted_ctx = NULL,
		.pre_process_sm_exception = NULL,
		.set_bes_crop_debug3 = NULL,
		.set_bes_crop_debug4 = NULL,
		.decode_priv_addr = gr_gk20a_decode_priv_addr,
		.create_priv_addr_table = gr_gk20a_create_priv_addr_table,
		.split_fbpa_broadcast_addr = gr_gk20a_split_fbpa_broadcast_addr,
		.get_offset_in_gpccs_segment =
			gr_gk20a_get_offset_in_gpccs_segment,
		.set_debug_mode = gm20b_gr_set_debug_mode,
		.reset = NULL,
		.halt_pipe = NULL,
		.disable_ctxsw = nvgpu_gr_disable_ctxsw,
		.enable_ctxsw = nvgpu_gr_enable_ctxsw,
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
			.set_zcull_ptr = gm20b_ctxsw_prog_set_zcull_ptr,
			.set_zcull = gm20b_ctxsw_prog_set_zcull,
			.set_zcull_mode_no_ctxsw =
				gm20b_ctxsw_prog_set_zcull_mode_no_ctxsw,
			.is_zcull_mode_separate_buffer =
				gm20b_ctxsw_prog_is_zcull_mode_separate_buffer,
			.set_pm_ptr = gm20b_ctxsw_prog_set_pm_ptr,
			.set_pm_mode = gm20b_ctxsw_prog_set_pm_mode,
			.set_pm_smpc_mode = gm20b_ctxsw_prog_set_pm_smpc_mode,
			.hw_get_pm_mode_no_ctxsw =
				gm20b_ctxsw_prog_hw_get_pm_mode_no_ctxsw,
			.hw_get_pm_mode_ctxsw = gm20b_ctxsw_prog_hw_get_pm_mode_ctxsw,
			.init_ctxsw_hdr_data = gp10b_ctxsw_prog_init_ctxsw_hdr_data,
			.set_compute_preemption_mode_cta =
				gp10b_ctxsw_prog_set_compute_preemption_mode_cta,
			.set_compute_preemption_mode_cilp =
				gp10b_ctxsw_prog_set_compute_preemption_mode_cilp,
			.set_graphics_preemption_mode_gfxp =
				gp10b_ctxsw_prog_set_graphics_preemption_mode_gfxp,
			.set_cde_enabled = gm20b_ctxsw_prog_set_cde_enabled,
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
			.set_pmu_options_boost_clock_frequencies = NULL,
			.set_full_preemption_ptr =
				gp10b_ctxsw_prog_set_full_preemption_ptr,
			.dump_ctxsw_stats = gp10b_ctxsw_prog_dump_ctxsw_stats,
		},
		.config = {
			.get_gpc_tpc_mask = vgpu_gr_get_gpc_tpc_mask,
			.init_sm_id_table = vgpu_gr_init_sm_id_table,
		},
		.setup = {
			.bind_ctxsw_zcull = vgpu_gr_bind_ctxsw_zcull,
			.alloc_obj_ctx = vgpu_gr_alloc_obj_ctx,
			.free_gr_ctx = vgpu_gr_free_gr_ctx,
			.set_preemption_mode = vgpu_gr_set_preemption_mode,
		},
		.zbc = {
			.add_color = NULL,
			.add_depth = NULL,
			.set_table = vgpu_gr_add_zbc,
			.query_table = vgpu_gr_query_zbc,
			.add_stencil = NULL,
			.get_gpcs_swdx_dss_zbc_c_format_reg = NULL,
			.get_gpcs_swdx_dss_zbc_z_format_reg = NULL,
		},
		.zcull = {
			.get_zcull_info = vgpu_gr_get_zcull_info,
			.program_zcull_mapping = NULL,
		},
		.falcon = {
			.init_ctx_state = vgpu_gr_init_ctx_state,
			.load_ctxsw_ucode = NULL,
		},
#ifdef CONFIG_GK20A_CTXSW_TRACE
		.fecs_trace = {
			.alloc_user_buffer = vgpu_alloc_user_buffer,
			.free_user_buffer = vgpu_free_user_buffer,
			.get_mmap_user_buffer_info =
				vgpu_get_mmap_user_buffer_info,
			.init = vgpu_fecs_trace_init,
			.deinit = vgpu_fecs_trace_deinit,
			.enable = vgpu_fecs_trace_enable,
			.disable = vgpu_fecs_trace_disable,
			.is_enabled = vgpu_fecs_trace_is_enabled,
			.reset = NULL,
			.flush = NULL,
			.poll = vgpu_fecs_trace_poll,
			.bind_channel = NULL,
			.unbind_channel = NULL,
			.max_entries = vgpu_fecs_trace_max_entries,
			.set_filter = vgpu_fecs_trace_set_filter,
			.get_buffer_full_mailbox_val =
				gm20b_fecs_trace_get_buffer_full_mailbox_val,
		},
#endif /* CONFIG_GK20A_CTXSW_TRACE */
		.init = {
			.get_fbp_en_mask = vgpu_gr_get_fbp_en_mask,
			.fs_state = vgpu_gr_init_fs_state,
			.get_bundle_cb_default_size =
				gm20b_gr_init_get_bundle_cb_default_size,
			.get_min_gpm_fifo_depth =
				gm20b_gr_init_get_min_gpm_fifo_depth,
			.get_bundle_cb_token_limit =
				gm20b_gr_init_get_bundle_cb_token_limit,
			.get_attrib_cb_default_size =
				gp10b_gr_init_get_attrib_cb_default_size,
			.get_alpha_cb_default_size =
				gp10b_gr_init_get_alpha_cb_default_size,
			.get_attrib_cb_gfxp_default_size =
				gp10b_gr_init_get_attrib_cb_gfxp_default_size,
			.get_attrib_cb_gfxp_size =
				gp10b_gr_init_get_attrib_cb_gfxp_size,
			.get_attrib_cb_size =
				gp10b_gr_init_get_attrib_cb_size,
			.get_alpha_cb_size =
				gp10b_gr_init_get_alpha_cb_size,
			.get_global_attr_cb_size =
				gp10b_gr_init_get_global_attr_cb_size,
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
				gp10b_gr_init_commit_global_attrib_cb,
			.commit_global_cb_manager =
				gp10b_gr_init_commit_global_cb_manager,
			.get_ctx_spill_size = gp10b_gr_init_get_ctx_spill_size,
			.get_ctx_pagepool_size =
				gp10b_gr_init_get_ctx_pagepool_size,
			.get_ctx_betacb_size =
				gp10b_gr_init_get_ctx_betacb_size,
			.get_ctx_attrib_cb_size =
				gp10b_gr_init_get_ctx_attrib_cb_size,
			.commit_ctxsw_spill = gp10b_gr_init_commit_ctxsw_spill,
			.commit_cbes_reserve =
				gp10b_gr_init_commit_cbes_reserve,
			.detect_sm_arch = vgpu_gr_detect_sm_arch,
			.get_supported__preemption_modes =
				gp10b_gr_init_get_supported_preemption_modes,
			.get_default_preemption_modes =
				gp10b_gr_init_get_default_preemption_modes,
		},
	},
	.class = {
		.is_valid = gp10b_class_is_valid,
		.is_valid_gfx = gp10b_class_is_valid_gfx,
		.is_valid_compute = gp10b_class_is_valid_compute,
	},
	.perf = {
		.get_pmm_per_chiplet_offset =
			gm20b_perf_get_pmm_per_chiplet_offset,
	},
	.fb = {
		.init_hw = NULL,
		.init_fs_state = NULL,
		.set_mmu_page_size = NULL,
		.set_use_full_comp_tag_line = NULL,
		.compression_page_size = gp10b_fb_compression_page_size,
		.compressible_page_size = gp10b_fb_compressible_page_size,
		.compression_align_mask = gm20b_fb_compression_align_mask,
		.vpr_info_fetch = NULL,
		.dump_vpr_info = NULL,
		.dump_wpr_info = NULL,
		.read_wpr_info = NULL,
		.is_debug_mode_enabled = NULL,
		.set_debug_mode = vgpu_mm_mmu_set_debug_mode,
		.tlb_invalidate = vgpu_mm_tlb_invalidate,
	},
	.cg = {
		.slcg_bus_load_gating_prod = NULL,
		.slcg_ce2_load_gating_prod = NULL,
		.slcg_chiplet_load_gating_prod = NULL,
		.slcg_ctxsw_firmware_load_gating_prod = NULL,
		.slcg_fb_load_gating_prod = NULL,
		.slcg_fifo_load_gating_prod = NULL,
		.slcg_gr_load_gating_prod = NULL,
		.slcg_ltc_load_gating_prod = NULL,
		.slcg_perf_load_gating_prod = NULL,
		.slcg_priring_load_gating_prod = NULL,
		.slcg_pmu_load_gating_prod = NULL,
		.slcg_therm_load_gating_prod = NULL,
		.slcg_xbar_load_gating_prod = NULL,
		.blcg_bus_load_gating_prod = NULL,
		.blcg_ce_load_gating_prod = NULL,
		.blcg_ctxsw_firmware_load_gating_prod = NULL,
		.blcg_fb_load_gating_prod = NULL,
		.blcg_fifo_load_gating_prod = NULL,
		.blcg_gr_load_gating_prod = NULL,
		.blcg_ltc_load_gating_prod = NULL,
		.blcg_pwr_csb_load_gating_prod = NULL,
		.blcg_pmu_load_gating_prod = NULL,
		.blcg_xbar_load_gating_prod = NULL,
		.pg_gr_load_gating_prod = NULL,
	},
	.fifo = {
		.init_fifo_setup_hw = vgpu_init_fifo_setup_hw,
		.preempt_channel = vgpu_fifo_preempt_channel,
		.preempt_tsg = vgpu_fifo_preempt_tsg,
		.is_preempt_pending = NULL,
		.reset_enable_hw = NULL,
		.recover = NULL,
		.setup_sw = vgpu_fifo_setup_sw,
		.cleanup_sw = vgpu_fifo_cleanup_sw,
		.set_sm_exception_type_mask = vgpu_set_sm_exception_type_mask,
		.intr_0_enable = NULL,
		.intr_1_enable = NULL,
		.intr_0_isr = NULL,
		.intr_1_isr = NULL,
		.handle_sched_error = NULL,
		.handle_ctxsw_timeout = NULL,
		.ctxsw_timeout_enable = NULL,
		.trigger_mmu_fault = NULL,
		.get_mmu_fault_info = NULL,
		/* TODO : set to NULL? */
		.get_mmu_fault_desc = gp10b_fifo_get_mmu_fault_desc,
		.get_mmu_fault_client_desc =
			gp10b_fifo_get_mmu_fault_client_desc,
		.get_mmu_fault_gpc_desc = gm20b_fifo_get_mmu_fault_gpc_desc,
		.is_mmu_fault_pending = NULL,
	},
	.engine = {
		.is_fault_engine_subid_gpc = gm20b_is_fault_engine_subid_gpc,
		.get_mask_on_id = NULL,
		.init_info = vgpu_fifo_init_engine_info,
	},
	.pbdma = {
		.setup_sw = NULL,
		.cleanup_sw = NULL,
		.setup_hw = NULL,
		.intr_enable = NULL,
		.acquire_val = gm20b_pbdma_acquire_val,
		.get_signature = gp10b_pbdma_get_signature,
		.dump_status = NULL,
		.handle_intr_0 = NULL,
		.handle_intr_1 = gm20b_pbdma_handle_intr_1,
		.handle_intr = gm20b_pbdma_handle_intr,
		.read_data = NULL,
		.reset_header = NULL,
		.device_fatal_0_intr_descs = NULL,
		.channel_fatal_0_intr_descs = NULL,
		.restartable_0_intr_descs = NULL,
		.format_gpfifo_entry = gm20b_pbdma_format_gpfifo_entry,
	},
	.sync = {
#ifdef CONFIG_TEGRA_GK20A_NVHOST
		.syncpt = {
			.alloc_buf = gk20a_syncpt_alloc_buf,
			.free_buf = gk20a_syncpt_free_buf,
			.add_wait_cmd = gk20a_syncpt_add_wait_cmd,
			.get_wait_cmd_size =
					gk20a_syncpt_get_wait_cmd_size,
			.get_incr_per_release =
					gk20a_syncpt_get_incr_per_release,
			.add_incr_cmd = gk20a_syncpt_add_incr_cmd,
			.get_incr_cmd_size =
					gk20a_syncpt_get_incr_cmd_size,
			.get_sync_ro_map = NULL,
		},
#endif
		.sema = {
			.get_wait_cmd_size = gk20a_sema_get_wait_cmd_size,
			.get_incr_cmd_size = gk20a_sema_get_incr_cmd_size,
			.add_cmd = gk20a_sema_add_cmd,
		},
	},
	.engine_status = {
		.read_engine_status_info = NULL,
		.dump_engine_status = NULL,
	},
	.pbdma_status = {
		.read_pbdma_status_info = NULL,
	},
	.ramfc = {
		.setup = vgpu_ramfc_setup,
		.capture_ram_dump = NULL,
		.commit_userd = NULL,
		.get_syncpt = NULL,
		.set_syncpt = NULL,
	},
	.ramin = {
		.set_gr_ptr = NULL,
		.set_big_page_size = gm20b_ramin_set_big_page_size,
		.init_pdb = gp10b_ramin_init_pdb,
		.init_subctx_pdb = NULL,
		.set_adr_limit = gk20a_ramin_set_adr_limit,
		.base_shift = gk20a_ramin_base_shift,
		.alloc_size = gk20a_ramin_alloc_size,
		.set_eng_method_buffer = NULL,
	},
	.runlist = {
		.reschedule = NULL,
		.update_for_channel = vgpu_runlist_update_for_channel,
		.reload = vgpu_runlist_reload,
		.count_max = gk20a_runlist_count_max,
		.entry_size = vgpu_runlist_entry_size,
		.length_max = vgpu_runlist_length_max,
		.get_tsg_entry = gk20a_runlist_get_tsg_entry,
		.get_ch_entry = gk20a_runlist_get_ch_entry,
		.hw_submit = NULL,
		.wait_pending = NULL,
	},
	.userd = {
		.setup_sw = vgpu_userd_setup_sw,
		.cleanup_sw = vgpu_userd_cleanup_sw,
#ifdef NVGPU_USERD
		.init_mem = gk20a_userd_init_mem,
		.gp_get = gk20a_userd_gp_get,
		.gp_put = gk20a_userd_gp_put,
		.pb_get = gk20a_userd_pb_get,
		.entry_size = gk20a_userd_entry_size,
#endif
	},
	.channel = {
		.alloc_inst = vgpu_channel_alloc_inst,
		.free_inst = vgpu_channel_free_inst,
		.bind = vgpu_channel_bind,
		.unbind = vgpu_channel_unbind,
		.enable = vgpu_channel_enable,
		.disable = vgpu_channel_disable,
		.count = vgpu_channel_count,
		.abort_clean_up = nvgpu_channel_abort_clean_up,
		.suspend_all_serviceable_ch =
                        nvgpu_channel_suspend_all_serviceable_ch,
		.resume_all_serviceable_ch =
                        nvgpu_channel_resume_all_serviceable_ch,
		.set_error_notifier = nvgpu_set_error_notifier,
		.debug_dump = NULL,
	},
	.tsg = {
		.open = vgpu_tsg_open,
		.release = vgpu_tsg_release,
		.init_eng_method_buffers = NULL,
		.deinit_eng_method_buffers = NULL,
		.enable = vgpu_tsg_enable,
		.disable = nvgpu_tsg_disable,
		.bind_channel = vgpu_tsg_bind_channel,
		.bind_channel_eng_method_buffers = NULL,
		.unbind_channel = vgpu_tsg_unbind_channel,
		.unbind_channel_check_hw_state = NULL,
		.unbind_channel_check_ctx_reload = NULL,
		.unbind_channel_check_eng_faulted = NULL,
		.check_ctxsw_timeout = nvgpu_tsg_check_ctxsw_timeout,
		.force_reset = vgpu_tsg_force_reset_ch,
		.post_event_id = nvgpu_tsg_post_event_id,
		.set_timeslice = vgpu_tsg_set_timeslice,
		.default_timeslice_us = vgpu_tsg_default_timeslice_us,
		.set_interleave = vgpu_tsg_set_interleave,
	},
	.netlist = {
		.get_netlist_name = gp10b_netlist_get_name,
		.is_fw_defined = gp10b_netlist_is_firmware_defined,
	},
	.mm = {
		.vm_bind_channel = vgpu_vm_bind_channel,
		.setup_hw = NULL,
		.is_bar1_supported = gm20b_mm_is_bar1_supported,
		.init_inst_block = gk20a_init_inst_block,
		.init_bar2_vm = gp10b_init_bar2_vm,
		.remove_bar2_vm = gp10b_remove_bar2_vm,
		.bar1_map_userd = vgpu_mm_bar1_map_userd,
		.vm_as_alloc_share = vgpu_vm_as_alloc_share,
		.vm_as_free_share = vgpu_vm_as_free_share,
		.cache = {
			.fb_flush = vgpu_mm_fb_flush,
			.l2_invalidate = vgpu_mm_l2_invalidate,
			.l2_flush = vgpu_mm_l2_flush,
			.cbc_clean = NULL,
		},
		.gmmu = {
			.map = vgpu_locked_gmmu_map,
			.unmap = vgpu_locked_gmmu_unmap,
			.get_big_page_sizes = gm20b_mm_get_big_page_sizes,
			.get_default_big_page_size =
				gp10b_mm_get_default_big_page_size,
			.gpu_phys_addr = gm20b_gpu_phys_addr,
			.get_iommu_bit = gk20a_mm_get_iommu_bit,
			.get_mmu_levels = gp10b_mm_get_mmu_levels,
		},
	},
	.pramin = {
		.data032_r = NULL,
	},
	.therm = {
		.init_therm_setup_hw = NULL,
		.init_elcg_mode = NULL,
		.init_blcg_mode = NULL,
		.elcg_init_idle_filters = NULL,
	},
	.pmu = {
		.pmu_setup_elpg = NULL,
		.pmu_get_queue_head = NULL,
		.pmu_get_queue_head_size = NULL,
		.pmu_get_queue_tail = NULL,
		.pmu_get_queue_tail_size = NULL,
		.pmu_reset = NULL,
		.pmu_queue_head = NULL,
		.pmu_queue_tail = NULL,
		.pmu_msgq_tail = NULL,
		.pmu_mutex_size = NULL,
		.pmu_mutex_acquire = NULL,
		.pmu_mutex_release = NULL,
		.pmu_is_interrupted = NULL,
		.pmu_isr = NULL,
		.pmu_init_perfmon_counter = NULL,
		.pmu_pg_idle_counter_config = NULL,
		.pmu_read_idle_counter = NULL,
		.pmu_reset_idle_counter = NULL,
		.pmu_read_idle_intr_status = NULL,
		.pmu_clear_idle_intr_status = NULL,
		.pmu_dump_elpg_stats = NULL,
		.pmu_dump_falcon_stats = NULL,
		.pmu_enable_irq = NULL,
		.write_dmatrfbase = NULL,
		.pmu_elpg_statistics = NULL,
		.pmu_init_perfmon = NULL,
		.pmu_perfmon_start_sampling = NULL,
		.pmu_perfmon_stop_sampling = NULL,
		.pmu_pg_init_param = NULL,
		.pmu_pg_supported_engines_list = NULL,
		.pmu_pg_engines_feature_list = NULL,
		.dump_secure_fuses = NULL,
		.reset_engine = NULL,
		.is_engine_in_reset = NULL,
		.save_zbc = NULL,
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
		.exec_regops = vgpu_exec_regops,
		.get_global_whitelist_ranges =
			gp10b_get_global_whitelist_ranges,
		.get_global_whitelist_ranges_count =
			gp10b_get_global_whitelist_ranges_count,
		.get_context_whitelist_ranges =
			gp10b_get_context_whitelist_ranges,
		.get_context_whitelist_ranges_count =
			gp10b_get_context_whitelist_ranges_count,
		.get_runcontrol_whitelist = gp10b_get_runcontrol_whitelist,
		.get_runcontrol_whitelist_count =
			gp10b_get_runcontrol_whitelist_count,
		.get_qctl_whitelist = gp10b_get_qctl_whitelist,
		.get_qctl_whitelist_count = gp10b_get_qctl_whitelist_count,
	},
	.mc = {
		.intr_mask = NULL,
		.intr_enable = NULL,
		.intr_unit_config = NULL,
		.isr_stall = NULL,
		.intr_stall = NULL,
		.intr_stall_pause = NULL,
		.intr_stall_resume = NULL,
		.intr_nonstall = NULL,
		.intr_nonstall_pause = NULL,
		.intr_nonstall_resume = NULL,
		.isr_nonstall = NULL,
		.enable = NULL,
		.disable = NULL,
		.reset = NULL,
		.is_intr1_pending = NULL,
		.log_pending_intrs = NULL,
		.reset_mask = NULL,
		.is_enabled = NULL,
		.fb_reset = NULL,
		.is_mmu_fault_pending = NULL,
	},
	.debug = {
		.show_dump = NULL,
	},
#ifdef NVGPU_DEBUGGER
	.debugger = {
		.post_events = nvgpu_dbg_gpu_post_events,
		.dbg_set_powergate = vgpu_dbg_set_powergate,
		.check_and_set_global_reservation =
			vgpu_check_and_set_global_reservation,
		.check_and_set_context_reservation =
			vgpu_check_and_set_context_reservation,
		.release_profiler_reservation =
			vgpu_release_profiler_reservation,
	},
#endif
	.perfbuf = {
		.perfbuf_enable = vgpu_perfbuffer_enable,
		.perfbuf_disable = vgpu_perfbuffer_disable,
	},
	.bus = {
		.init_hw = NULL,
		.isr = NULL,
		.bar1_bind = NULL,
		.bar2_bind = NULL,
		.set_bar0_window = NULL,
	},
	.ptimer = {
		.isr = NULL,
		.read_ptimer = vgpu_read_ptimer,
		.get_timestamps_zipper = vgpu_get_timestamps_zipper,
	},
#if defined(CONFIG_GK20A_CYCLE_STATS)
	.css = {
		.enable_snapshot = vgpu_css_enable_snapshot_buffer,
		.disable_snapshot = vgpu_css_release_snapshot_buffer,
		.check_data_available = vgpu_css_flush_snapshots,
		.detach_snapshot = vgpu_css_detach,
		.set_handled_snapshots = NULL,
		.allocate_perfmon_ids = NULL,
		.release_perfmon_ids = NULL,
		.get_max_buffer_size = vgpu_css_get_buffer_size,
	},
#endif
	.priv_ring = {
		.enable_priv_ring = NULL,
		.isr = NULL,
		.set_ppriv_timeout_settings = NULL,
		.enum_ltc = NULL,
	},
	.fuse = {
		.check_priv_security = NULL,
		.is_opt_ecc_enable = NULL,
		.is_opt_feature_override_disable = NULL,
		.fuse_status_opt_fbio = NULL,
		.fuse_status_opt_fbp = NULL,
		.fuse_status_opt_rop_l2_fbp = NULL,
		.fuse_status_opt_tpc_gpc = NULL,
		.fuse_ctrl_opt_tpc_gpc = NULL,
		.fuse_opt_sec_debug_en = NULL,
		.fuse_opt_priv_sec_en = NULL,
		.read_vin_cal_fuse_rev = NULL,
		.read_vin_cal_slope_intercept_fuse = NULL,
		.read_vin_cal_gain_offset_fuse = NULL,
	},
	.top = {
		.get_max_fbps_count = vgpu_gr_get_max_fbps_count,
		.get_max_ltc_per_fbp = vgpu_gr_get_max_ltc_per_fbp,
		.get_max_lts_per_ltc = vgpu_gr_get_max_lts_per_ltc,
	},
	.chip_init_gpu_characteristics = vgpu_init_gpu_characteristics,
	.get_litter_value = gp10b_get_litter_value,
};

int vgpu_gp10b_init_hal(struct gk20a *g)
{
	struct gpu_ops *gops = &g->ops;
	struct vgpu_priv_data *priv = vgpu_get_priv_data(g);

	gops->ltc = vgpu_gp10b_ops.ltc;
	gops->cbc = vgpu_gp10b_ops.cbc;
	gops->ce = vgpu_gp10b_ops.ce;
	gops->gr = vgpu_gp10b_ops.gr;
	gops->class = vgpu_gp10b_ops.class;
	gops->gr.ctxsw_prog = vgpu_gp10b_ops.gr.ctxsw_prog;
	gops->gr.config = vgpu_gp10b_ops.gr.config;
	gops->fb = vgpu_gp10b_ops.fb;
	gops->perf = vgpu_gp10b_ops.perf;
	gops->cg = vgpu_gp10b_ops.cg;
	gops->fifo = vgpu_gp10b_ops.fifo;
	gops->engine = vgpu_gp10b_ops.engine;
	gops->pbdma = vgpu_gp10b_ops.pbdma;
	gops->ramfc = vgpu_gp10b_ops.ramfc;
	gops->ramin = vgpu_gp10b_ops.ramin;
	gops->runlist = vgpu_gp10b_ops.runlist;
	gops->userd = vgpu_gp10b_ops.userd;
	gops->channel = vgpu_gp10b_ops.channel;
	gops->tsg = vgpu_gp10b_ops.tsg;
	gops->sync = vgpu_gp10b_ops.sync;
	gops->engine_status = vgpu_gp10b_ops.engine_status;
	gops->pbdma_status = vgpu_gp10b_ops.pbdma_status;
	gops->netlist = vgpu_gp10b_ops.netlist;
	gops->mm = vgpu_gp10b_ops.mm;
	gops->pramin = vgpu_gp10b_ops.pramin;
	gops->therm = vgpu_gp10b_ops.therm;
	gops->pmu = vgpu_gp10b_ops.pmu;
	gops->clk_arb = vgpu_gp10b_ops.clk_arb;
	gops->regops = vgpu_gp10b_ops.regops;
	gops->mc = vgpu_gp10b_ops.mc;
	gops->debug = vgpu_gp10b_ops.debug;
#ifdef NVGPU_DEBUGGER
	gops->debugger = vgpu_gp10b_ops.debugger;
#endif
	gops->perfbuf = vgpu_gp10b_ops.perfbuf;
	gops->bus = vgpu_gp10b_ops.bus;
	gops->ptimer = vgpu_gp10b_ops.ptimer;
#if defined(CONFIG_GK20A_CYCLE_STATS)
	gops->css = vgpu_gp10b_ops.css;
#endif
	gops->falcon = vgpu_gp10b_ops.falcon;

	gops->priv_ring = vgpu_gp10b_ops.priv_ring;

	gops->fuse = vgpu_gp10b_ops.fuse;
	gops->top = vgpu_gp10b_ops.top;

	/* Lone Functions */
	gops->chip_init_gpu_characteristics =
		vgpu_gp10b_ops.chip_init_gpu_characteristics;
	gops->get_litter_value = vgpu_gp10b_ops.get_litter_value;
	gops->semaphore_wakeup = gk20a_channel_semaphore_wakeup;

	if (priv->constants.can_set_clkrate) {
		gops->clk.support_clk_freq_controller = true;
	} else {
		gops->clk.support_clk_freq_controller = false;
		gops->clk_arb.get_arbiter_clk_domains = NULL;
	}

	g->name = "gp10b";

	return 0;
}
