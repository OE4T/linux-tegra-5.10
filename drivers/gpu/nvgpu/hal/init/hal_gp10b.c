/*
 * GP10B Tegra HAL interface
 *
 * Copyright (c) 2014-2020, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/ptimer.h>
#include <nvgpu/error_notifier.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/debugger.h>
#include <nvgpu/pm_reservation.h>
#include <nvgpu/pbdma.h>
#include <nvgpu/engines.h>
#include <nvgpu/runlist.h>
#include <nvgpu/fifo/userd.h>
#include <nvgpu/perfbuf.h>
#include <nvgpu/cyclestats_snapshot.h>
#include <nvgpu/fuse.h>
#include <nvgpu/regops.h>
#ifdef CONFIG_NVGPU_GRAPHICS
#include <nvgpu/gr/zbc.h>
#endif
#include <nvgpu/gr/gr_falcon.h>
#include <nvgpu/gr/setup.h>
#include <nvgpu/gr/fecs_trace.h>
#include <nvgpu/gr/gr.h>
#include <nvgpu/gr/gr_intr.h>
#ifdef CONFIG_NVGPU_LS_PMU
#include <nvgpu/pmu/pmu_perfmon.h>
#endif
#include <nvgpu/nvgpu_init.h>
#include <nvgpu/acr.h>
#include <nvgpu/ce.h>
#include <nvgpu/ce_app.h>
#include <nvgpu/pmu.h>
#ifdef CONFIG_NVGPU_LS_PMU
#include <nvgpu/pmu/pmu_pstate.h>
#endif
#include <nvgpu/fbp.h>
#include <nvgpu/therm.h>
#include <nvgpu/clk_arb.h>

#include "hal/mm/mm_gk20a.h"
#include "hal/mm/mm_gm20b.h"
#include "hal/mm/mm_gp10b.h"
#include "hal/mm/cache/flush_gk20a.h"
#include "hal/mm/gmmu/gmmu_gm20b.h"
#include "hal/mm/gmmu/gmmu_gp10b.h"
#include "hal/mc/mc_gm20b.h"
#include "hal/mc/mc_gp10b.h"
#include "hal/bus/bus_gk20a.h"
#include "hal/bus/bus_gm20b.h"
#include "hal/bus/bus_gp10b.h"
#include "hal/ce/ce_gp10b.h"
#include "hal/class/class_gp10b.h"
#include "hal/priv_ring/priv_ring_gm20b.h"
#include "hal/priv_ring/priv_ring_gp10b.h"
#include "hal/power_features/cg/gp10b_gating_reglist.h"
#include "hal/cbc/cbc_gm20b.h"
#include "hal/cbc/cbc_gp10b.h"
#include "hal/therm/therm_gm20b.h"
#include "hal/therm/therm_gp10b.h"
#include "hal/ltc/ltc_gm20b.h"
#include "hal/ltc/ltc_gp10b.h"
#include "hal/ltc/intr/ltc_intr_gp10b.h"
#include "hal/fb/fb_gm20b.h"
#include "hal/fb/fb_gp10b.h"
#include "hal/fuse/fuse_gm20b.h"
#include "hal/fuse/fuse_gp10b.h"
#include "hal/ptimer/ptimer_gk20a.h"
#include "hal/ptimer/ptimer_gp10b.h"
#include "hal/regops/regops_gp10b.h"
#include "hal/fifo/fifo_gk20a.h"
#include "hal/fifo/preempt_gk20a.h"
#include "hal/fifo/pbdma_gm20b.h"
#include "hal/fifo/pbdma_gp10b.h"
#include "hal/fifo/engines_gm20b.h"
#include "hal/fifo/engines_gp10b.h"
#include "hal/fifo/engine_status_gm20b.h"
#include "hal/fifo/pbdma_status_gm20b.h"
#include "hal/fifo/ramfc_gk20a.h"
#include "hal/fifo/ramfc_gp10b.h"
#include "hal/fifo/ramin_gk20a.h"
#include "hal/fifo/ramin_gm20b.h"
#include "hal/fifo/ramin_gp10b.h"
#include "hal/fifo/runlist_ram_gk20a.h"
#include "hal/fifo/runlist_fifo_gk20a.h"
#include "hal/fifo/tsg_gk20a.h"
#include "hal/fifo/userd_gk20a.h"
#include "hal/fifo/fifo_intr_gk20a.h"
#include "hal/fifo/mmu_fault_gm20b.h"
#include "hal/fifo/mmu_fault_gp10b.h"
#include "hal/fifo/ctxsw_timeout_gk20a.h"
#ifdef CONFIG_NVGPU_RECOVERY
#include "hal/rc/rc_gk20a.h"
#endif
#include "hal/gr/ecc/ecc_gp10b.h"
#include "hal/gr/fecs_trace/fecs_trace_gm20b.h"
#include "hal/gr/config/gr_config_gm20b.h"
#ifdef CONFIG_NVGPU_GRAPHICS
#include "hal/gr/zbc/zbc_gm20b.h"
#include "hal/gr/zbc/zbc_gp10b.h"
#endif
#include "hal/gr/zcull/zcull_gm20b.h"
#include "hal/gr/falcon/gr_falcon_gm20b.h"
#include "hal/gr/falcon/gr_falcon_gp10b.h"
#include "hal/gr/init/gr_init_gm20b.h"
#include "hal/gr/init/gr_init_gp10b.h"
#include "hal/gr/intr/gr_intr_gm20b.h"
#include "hal/gr/intr/gr_intr_gp10b.h"
#include "hal/gr/ctxsw_prog/ctxsw_prog_gm20b.h"
#include "hal/gr/ctxsw_prog/ctxsw_prog_gp10b.h"
#include "hal/gr/gr/gr_gk20a.h"
#include "hal/gr/gr/gr_gp10b.h"
#include "hal/gr/gr/gr_gm20b.h"
#ifdef CONFIG_NVGPU_LS_PMU
#include "hal/pmu/pmu_gk20a.h"
#include "hal/pmu/pmu_gm20b.h"
#include "hal/pmu/pmu_gp10b.h"
#endif
#include "hal/sync/syncpt_cmdbuf_gk20a.h"
#include "hal/sync/sema_cmdbuf_gk20a.h"
#include "hal/falcon/falcon_gk20a.h"
#include "hal/perf/perf_gm20b.h"
#include "hal/netlist/netlist_gp10b.h"
#include "hal/top/top_gm20b.h"
#include "hal/top/top_gp10b.h"
#include "hal/pramin/pramin_init.h"
#ifdef CONFIG_NVGPU_LS_PMU
#include "common/pmu/pg/pg_sw_gm20b.h"
#include "common/pmu/pg/pg_sw_gp10b.h"
#endif
#include "hal/fifo/channel_gk20a.h"
#include "hal/fifo/channel_gm20b.h"

#ifdef CONFIG_NVGPU_CLK_ARB
#include "common/clk_arb/clk_arb_gp10b.h"
#endif

#include "hal_gp10b.h"
#include "hal_gp10b_litter.h"

#include <nvgpu/hw/gp10b/hw_pwr_gp10b.h>

static int gp10b_init_gpu_characteristics(struct gk20a *g)
{
	int err;

	err = nvgpu_init_gpu_characteristics(g);
	if (err != 0) {
		nvgpu_err(g, "failed to init GPU characteristics");
		return err;
	}
	g->ops.gr.ecc.detect(g);
	nvgpu_set_enabled(g, NVGPU_SUPPORT_RESCHEDULE_RUNLIST, true);

	return 0;
}

static const struct gpu_ops gp10b_ops = {
	.acr = {
		.acr_init = nvgpu_acr_init,
		.acr_construct_execute = nvgpu_acr_construct_execute,
	},
#ifdef CONFIG_NVGPU_DGPU
	.bios = {
		.bios_sw_init = nvgpu_bios_sw_init,
	},
#endif /* CONFIG_NVGPU_DGPU */
	.ecc = {
		.ecc_init_support = nvgpu_ecc_init_support,
		.ecc_finalize_support = nvgpu_ecc_finalize_support,
		.ecc_remove_support = nvgpu_ecc_remove_support,
	},
	.ltc = {
		.ecc_init = gp10b_lts_ecc_init,
		.init_ltc_support = nvgpu_init_ltc_support,
		.ltc_remove_support = nvgpu_ltc_remove_support,
		.determine_L2_size_bytes = gp10b_determine_L2_size_bytes,
		.init_fs_state = gp10b_ltc_init_fs_state,
		.flush = gm20b_flush_ltc,
		.set_enabled = gp10b_ltc_set_enabled,
#ifdef CONFIG_NVGPU_GRAPHICS
		.set_zbc_color_entry = gm20b_ltc_set_zbc_color_entry,
		.set_zbc_depth_entry = gm20b_ltc_set_zbc_depth_entry,
		.zbc_table_size = gm20b_ltc_zbc_table_size,
#endif /* CONFIG_NVGPU_GRAPHICS */
#ifdef CONFIG_NVGPU_DEBUGGER
		.pri_is_ltc_addr = gm20b_ltc_pri_is_ltc_addr,
		.is_ltcs_ltss_addr = gm20b_ltc_is_ltcs_ltss_addr,
		.is_ltcn_ltss_addr = gm20b_ltc_is_ltcn_ltss_addr,
		.split_lts_broadcast_addr = gm20b_ltc_split_lts_broadcast_addr,
		.split_ltc_broadcast_addr = gm20b_ltc_split_ltc_broadcast_addr,
#endif /* CONFIG_NVGPU_DEBUGGER */
		.intr = {
#ifdef CONFIG_NVGPU_FALCON_NON_FUSA
			.configure = gp10b_ltc_intr_configure,
			.isr = gp10b_ltc_intr_isr,
#endif
			.en_illegal_compstat = NULL,
		},
	},
#ifdef CONFIG_NVGPU_COMPRESSION
	.cbc = {
		.cbc_init_support = nvgpu_cbc_init_support,
		.cbc_remove_support = nvgpu_cbc_remove_support,
		.init = gm20b_cbc_init,
		.alloc_comptags = gp10b_cbc_alloc_comptags,
		.ctrl = gp10b_cbc_ctrl,
		.fix_config = gm20b_cbc_fix_config,
	},
#endif
	.ce = {
		.ce_init_support = nvgpu_ce_init_support,
#ifdef CONFIG_NVGPU_DGPU
		.ce_app_init_support = nvgpu_ce_app_init_support,
		.ce_app_suspend = nvgpu_ce_app_suspend,
		.ce_app_destroy = nvgpu_ce_app_destroy,
#endif
		.isr_stall = gp10b_ce_stall_isr,
		.isr_nonstall = gp10b_ce_nonstall_isr,
	},
	.gr = {
		.gr_prepare_sw = nvgpu_gr_prepare_sw,
		.gr_enable_hw = nvgpu_gr_enable_hw,
		.gr_init_support = nvgpu_gr_init_support,
		.gr_suspend = nvgpu_gr_suspend,
#ifdef CONFIG_NVGPU_DEBUGGER
		.get_gr_status = gr_gm20b_get_gr_status,
		.set_alpha_circular_buffer_size =
			gr_gp10b_set_alpha_circular_buffer_size,
		.set_circular_buffer_size = gr_gp10b_set_circular_buffer_size,
		.get_sm_dsm_perf_regs = gr_gm20b_get_sm_dsm_perf_regs,
		.get_sm_dsm_perf_ctrl_regs = gr_gm20b_get_sm_dsm_perf_ctrl_regs,
#ifdef CONFIG_NVGPU_TEGRA_FUSE
		.set_gpc_tpc_mask = gr_gp10b_set_gpc_tpc_mask,
#endif
		.dump_gr_regs = gr_gp10b_dump_gr_status_regs,
		.update_pc_sampling = gr_gm20b_update_pc_sampling,
		.init_sm_dsm_reg_info = gr_gm20b_init_sm_dsm_reg_info,
		.init_cyclestats = gr_gm20b_init_cyclestats,
		.set_sm_debug_mode = gr_gk20a_set_sm_debug_mode,
		.bpt_reg_info = gr_gm20b_bpt_reg_info,
		.get_lrf_tex_ltc_dram_override = gp10b_gr_get_ecc_override_val,
		.update_smpc_ctxsw_mode = gr_gk20a_update_smpc_ctxsw_mode,
		.update_hwpm_ctxsw_mode = gr_gk20a_update_hwpm_ctxsw_mode,
		.set_mmu_debug_mode = NULL,
		.clear_sm_error_state = gm20b_gr_clear_sm_error_state,
		.suspend_contexts = gr_gp10b_suspend_contexts,
		.resume_contexts = gr_gk20a_resume_contexts,
		.trigger_suspend = gr_gk20a_trigger_suspend,
		.wait_for_pause = gr_gk20a_wait_for_pause,
		.resume_from_pause = gr_gk20a_resume_from_pause,
		.clear_sm_errors = gr_gk20a_clear_sm_errors,
		.sm_debugger_attached = gk20a_gr_sm_debugger_attached,
		.suspend_single_sm = gk20a_gr_suspend_single_sm,
		.suspend_all_sms = gk20a_gr_suspend_all_sms,
		.resume_single_sm = gk20a_gr_resume_single_sm,
		.resume_all_sms = gk20a_gr_resume_all_sms,
		.lock_down_sm = gk20a_gr_lock_down_sm,
		.wait_for_sm_lock_down = gk20a_gr_wait_for_sm_lock_down,
		.init_ovr_sm_dsm_perf =  gk20a_gr_init_ovr_sm_dsm_perf,
		.get_ovr_perf_regs = gk20a_gr_get_ovr_perf_regs,
#ifdef CONFIG_NVGPU_CHANNEL_TSG_SCHEDULING
		.set_boosted_ctx = gr_gp10b_set_boosted_ctx,
#endif
		.pre_process_sm_exception = gr_gp10b_pre_process_sm_exception,
		.set_bes_crop_debug3 = gr_gp10b_set_bes_crop_debug3,
		.decode_priv_addr = gr_gk20a_decode_priv_addr,
		.create_priv_addr_table = gr_gk20a_create_priv_addr_table,
		.split_fbpa_broadcast_addr = gr_gk20a_split_fbpa_broadcast_addr,
		.get_offset_in_gpccs_segment =
			gr_gk20a_get_offset_in_gpccs_segment,
		.process_context_buffer_priv_segment =
			gr_gk20a_process_context_buffer_priv_segment,
		.get_ctx_buffer_offsets = gr_gk20a_get_ctx_buffer_offsets,
		.set_debug_mode = gm20b_gr_set_debug_mode,
		.esr_bpt_pending_events = gm20b_gr_esr_bpt_pending_events,
#endif /* CONFIG_NVGPU_DEBUGGER */
		.ecc = {
			.detect = gp10b_ecc_detect_enabled_units,
			.gpc_tpc_ecc_init = gp10b_gr_ecc_init,
		},
		.ctxsw_prog = {
			.hw_get_fecs_header_size =
				gm20b_ctxsw_prog_hw_get_fecs_header_size,
			.get_patch_count = gm20b_ctxsw_prog_get_patch_count,
			.set_patch_count = gm20b_ctxsw_prog_set_patch_count,
			.set_patch_addr = gm20b_ctxsw_prog_set_patch_addr,
			.init_ctxsw_hdr_data = gp10b_ctxsw_prog_init_ctxsw_hdr_data,
			.set_compute_preemption_mode_cta =
				gp10b_ctxsw_prog_set_compute_preemption_mode_cta,
			.set_priv_access_map_config_mode =
				gm20b_ctxsw_prog_set_priv_access_map_config_mode,
			.set_priv_access_map_addr =
				gm20b_ctxsw_prog_set_priv_access_map_addr,
			.disable_verif_features =
				gm20b_ctxsw_prog_disable_verif_features,
#ifdef CONFIG_NVGPU_GRAPHICS
			.set_zcull_ptr = gm20b_ctxsw_prog_set_zcull_ptr,
			.set_zcull = gm20b_ctxsw_prog_set_zcull,
			.set_zcull_mode_no_ctxsw =
				gm20b_ctxsw_prog_set_zcull_mode_no_ctxsw,
			.is_zcull_mode_separate_buffer =
				gm20b_ctxsw_prog_is_zcull_mode_separate_buffer,
			.set_graphics_preemption_mode_gfxp =
				gp10b_ctxsw_prog_set_graphics_preemption_mode_gfxp,
			.set_full_preemption_ptr =
				gp10b_ctxsw_prog_set_full_preemption_ptr,
#endif /* CONFIG_NVGPU_GRAPHICS */
#ifdef CONFIG_NVGPU_CILP
			.set_compute_preemption_mode_cilp =
				gp10b_ctxsw_prog_set_compute_preemption_mode_cilp,
#endif
#ifdef CONFIG_NVGPU_DEBUGGER
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
			.set_pm_ptr = gm20b_ctxsw_prog_set_pm_ptr,
			.set_pm_mode = gm20b_ctxsw_prog_set_pm_mode,
			.set_pm_smpc_mode = gm20b_ctxsw_prog_set_pm_smpc_mode,
			.hw_get_pm_mode_no_ctxsw =
				gm20b_ctxsw_prog_hw_get_pm_mode_no_ctxsw,
			.hw_get_pm_mode_ctxsw = gm20b_ctxsw_prog_hw_get_pm_mode_ctxsw,
			.set_cde_enabled = gm20b_ctxsw_prog_set_cde_enabled,
			.set_pc_sampling = gm20b_ctxsw_prog_set_pc_sampling,
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
			.set_pmu_options_boost_clock_frequencies =
				gp10b_ctxsw_prog_set_pmu_options_boost_clock_frequencies,
#endif /* CONFIG_NVGPU_DEBUGGER */
#ifdef CONFIG_DEBUG_FS
			.dump_ctxsw_stats = gp10b_ctxsw_prog_dump_ctxsw_stats,
#endif
#ifdef CONFIG_NVGPU_FECS_TRACE
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
#endif
		},
		.config = {
			.get_gpc_tpc_mask = gm20b_gr_config_get_gpc_tpc_mask,
			.get_tpc_count_in_gpc =
				gm20b_gr_config_get_tpc_count_in_gpc,
			.get_pes_tpc_mask = gm20b_gr_config_get_pes_tpc_mask,
			.get_pd_dist_skip_table_size =
				gm20b_gr_config_get_pd_dist_skip_table_size,
			.init_sm_id_table = gm20b_gr_config_init_sm_id_table,
#ifdef CONFIG_NVGPU_GRAPHICS
			.get_zcull_count_in_gpc =
				gm20b_gr_config_get_zcull_count_in_gpc,
#endif /* CONFIG_NVGPU_GRAPHICS */
		},
#ifdef CONFIG_NVGPU_FECS_TRACE
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
			.flush = gm20b_fecs_trace_flush,
			.poll = nvgpu_gr_fecs_trace_poll,
			.bind_channel = nvgpu_gr_fecs_trace_bind_channel,
			.unbind_channel = nvgpu_gr_fecs_trace_unbind_channel,
			.max_entries = nvgpu_gr_fecs_trace_max_entries,
			.get_buffer_full_mailbox_val =
				gm20b_fecs_trace_get_buffer_full_mailbox_val,
			.get_read_index = gm20b_fecs_trace_get_read_index,
			.get_write_index = gm20b_fecs_trace_get_write_index,
			.set_read_index = gm20b_fecs_trace_set_read_index,
		},
#endif /* CONFIG_NVGPU_FECS_TRACE */
		.setup = {
			.alloc_obj_ctx = nvgpu_gr_setup_alloc_obj_ctx,
			.free_gr_ctx = nvgpu_gr_setup_free_gr_ctx,
#ifdef CONFIG_NVGPU_GRAPHICS
			.bind_ctxsw_zcull = nvgpu_gr_setup_bind_ctxsw_zcull,
#endif /* CONFIG_NVGPU_GRAPHICS */
			.set_preemption_mode = nvgpu_gr_setup_set_preemption_mode,
		},
#ifdef CONFIG_NVGPU_GRAPHICS
		.zbc = {
			.add_color = gp10b_gr_zbc_add_color,
			.add_depth = gp10b_gr_zbc_add_depth,
			.set_table = nvgpu_gr_zbc_set_table,
			.query_table = nvgpu_gr_zbc_query_table,
			.add_stencil = NULL,
			.get_gpcs_swdx_dss_zbc_c_format_reg =
				gp10b_gr_zbc_get_gpcs_swdx_dss_zbc_c_format_reg,
			.get_gpcs_swdx_dss_zbc_z_format_reg =
				gp10b_gr_zbc_get_gpcs_swdx_dss_zbc_z_format_reg,
		},
		.zcull = {
			.init_zcull_hw = gm20b_gr_init_zcull_hw,
			.get_zcull_info = gm20b_gr_get_zcull_info,
			.program_zcull_mapping = gm20b_gr_program_zcull_mapping,
		},
#endif /* CONFIG_NVGPU_GRAPHICS */
		.init = {
			.get_no_of_sm = nvgpu_gr_get_no_of_sm,
			.wait_initialized = nvgpu_gr_wait_initialized,
			.ecc_scrub_reg = NULL,
			.lg_coalesce = gm20b_gr_init_lg_coalesce,
			.su_coalesce = gm20b_gr_init_su_coalesce,
			.pes_vsc_stream = gm20b_gr_init_pes_vsc_stream,
			.gpc_mmu = gm20b_gr_init_gpc_mmu,
			.fifo_access = gm20b_gr_init_fifo_access,
#ifdef CONFIG_NVGPU_SET_FALCON_ACCESS_MAP
			.get_access_map = gp10b_gr_init_get_access_map,
#endif
			.get_sm_id_size = gp10b_gr_init_get_sm_id_size,
			.sm_id_config = gp10b_gr_init_sm_id_config,
			.sm_id_numbering = gm20b_gr_init_sm_id_numbering,
			.tpc_mask = gm20b_gr_init_tpc_mask,
			.fs_state = gp10b_gr_init_fs_state,
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
			.preemption_state = gp10b_gr_init_preemption_state,
			.fe_go_idle_timeout = gm20b_gr_init_fe_go_idle_timeout,
			.load_method_init = gm20b_gr_init_load_method_init,
			.commit_global_timeslice =
				gm20b_gr_init_commit_global_timeslice,
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
			.pipe_mode_override = gm20b_gr_init_pipe_mode_override,
			.load_sw_bundle_init =
				gm20b_gr_init_load_sw_bundle_init,
			.get_patch_slots = gm20b_gr_init_get_patch_slots,
			.detect_sm_arch = gm20b_gr_init_detect_sm_arch,
			.get_supported__preemption_modes =
				gp10b_gr_init_get_supported_preemption_modes,
			.get_default_preemption_modes =
				gp10b_gr_init_get_default_preemption_modes,
#ifdef CONFIG_NVGPU_GRAPHICS
			.get_ctx_attrib_cb_size =
				gp10b_gr_init_get_ctx_attrib_cb_size,
			.commit_cbes_reserve =
				gp10b_gr_init_commit_cbes_reserve,
			.rop_mapping = gm20b_gr_init_rop_mapping,
			.get_attrib_cb_gfxp_default_size =
				gp10b_gr_init_get_attrib_cb_gfxp_default_size,
			.get_attrib_cb_gfxp_size =
				gp10b_gr_init_get_attrib_cb_gfxp_size,
			.get_gfxp_rtv_cb_size = NULL,
			.get_ctx_spill_size = gp10b_gr_init_get_ctx_spill_size,
			.get_ctx_pagepool_size =
				gp10b_gr_init_get_ctx_pagepool_size,
			.get_ctx_betacb_size =
				gp10b_gr_init_get_ctx_betacb_size,
			.commit_ctxsw_spill = gp10b_gr_init_commit_ctxsw_spill,
#endif /* CONFIG_NVGPU_GRAPHICS */
		},
		.intr = {
			.handle_fecs_error = gp10b_gr_intr_handle_fecs_error,
			.handle_sw_method = gp10b_gr_intr_handle_sw_method,
			.set_shader_exceptions =
					gm20b_gr_intr_set_shader_exceptions,
			.handle_class_error =
					gp10b_gr_intr_handle_class_error,
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
			.get_tpc_exception = gm20b_gr_intr_get_tpc_exception,
			.handle_tex_exception =
					gp10b_gr_intr_handle_tex_exception,
			.enable_hww_exceptions =
					gm20b_gr_intr_enable_hww_exceptions,
			.enable_interrupts = gm20b_gr_intr_enable_interrupts,
			.enable_gpc_exceptions =
					gm20b_gr_intr_enable_gpc_exceptions,
			.enable_exceptions = gm20b_gr_intr_enable_exceptions,
			.nonstall_isr = gm20b_gr_intr_nonstall_isr,
			.tpc_exception_sm_enable =
				gm20b_gr_intr_tpc_exception_sm_enable,
			.handle_sm_exception =
				gp10b_gr_intr_handle_sm_exception,
			.stall_isr = nvgpu_gr_intr_stall_isr,
			.flush_channel_tlb = nvgpu_gr_intr_flush_channel_tlb,
			.set_hww_esr_report_mask =
				gm20b_gr_intr_set_hww_esr_report_mask,
			.get_esr_sm_sel = gm20b_gr_intr_get_esr_sm_sel,
			.clear_sm_hww = gm20b_gr_intr_clear_sm_hww,
			.record_sm_error_state =
				gm20b_gr_intr_record_sm_error_state,
			.get_sm_hww_warp_esr =
				gm20b_gr_intr_get_sm_hww_warp_esr,
			.get_sm_hww_global_esr =
				gm20b_gr_intr_get_sm_hww_global_esr,
			.get_sm_no_lock_down_hww_global_esr_mask =
				gm20b_gr_intr_get_sm_no_lock_down_hww_global_esr_mask,
#ifdef CONFIG_NVGPU_DEBUGGER
			.tpc_exception_sm_disable =
				gm20b_gr_intr_tpc_exception_sm_disable,
			.tpc_enabled_exceptions =
				gm20b_gr_intr_tpc_enabled_exceptions,
#endif
		},
		.falcon = {
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
			.dump_stats = gm20b_gr_falcon_dump_stats,
			.fecs_ctxsw_mailbox_size =
				gm20b_gr_falcon_get_fecs_ctxsw_mailbox_size,
			.fecs_ctxsw_clear_mailbox =
				gm20b_gr_falcon_fecs_ctxsw_clear_mailbox,
			.get_fecs_ctx_state_store_major_rev_id =
				gm20b_gr_falcon_get_fecs_ctx_state_store_major_rev_id,
			.start_gpccs = gm20b_gr_falcon_start_gpccs,
			.start_fecs = gm20b_gr_falcon_start_fecs,
			.get_gpccs_start_reg_offset =
				gm20b_gr_falcon_get_gpccs_start_reg_offset,
			.bind_instblk = gm20b_gr_falcon_bind_instblk,
			.wait_mem_scrubbing =
					gm20b_gr_falcon_wait_mem_scrubbing,
			.wait_ctxsw_ready = gm20b_gr_falcon_wait_ctxsw_ready,
			.ctrl_ctxsw = gp10b_gr_falcon_ctrl_ctxsw,
			.get_current_ctx = gm20b_gr_falcon_get_current_ctx,
			.get_ctx_ptr = gm20b_gr_falcon_get_ctx_ptr,
			.get_fecs_current_ctx_data =
				gm20b_gr_falcon_get_fecs_current_ctx_data,
			.init_ctx_state = gp10b_gr_falcon_init_ctx_state,
			.fecs_host_int_enable =
				gm20b_gr_falcon_fecs_host_int_enable,
			.read_fecs_ctxsw_status0 =
				gm20b_gr_falcon_read_fecs_ctxsw_status0,
			.read_fecs_ctxsw_status1 =
				gm20b_gr_falcon_read_fecs_ctxsw_status1,
#ifdef CONFIG_NVGPU_GR_FALCON_NON_SECURE_BOOT
			.load_ctxsw_ucode_header =
				gm20b_gr_falcon_load_ctxsw_ucode_header,
			.load_ctxsw_ucode_boot =
				gm20b_gr_falcon_load_ctxsw_ucode_boot,
			.load_gpccs_dmem = gm20b_gr_falcon_load_gpccs_dmem,
			.gpccs_dmemc_write = gm20b_gr_falcon_gpccs_dmemc_write,
			.load_fecs_dmem = gm20b_gr_falcon_load_fecs_dmem,
			.fecs_dmemc_write = gm20b_gr_falcon_fecs_dmemc_write,
			.load_gpccs_imem = gm20b_gr_falcon_load_gpccs_imem,
			.gpccs_imemc_write = gm20b_gr_falcon_gpccs_imemc_write,
			.load_fecs_imem = gm20b_gr_falcon_load_fecs_imem,
			.fecs_imemc_write = gm20b_gr_falcon_fecs_imemc_write,
			.start_ucode = gm20b_gr_falcon_start_ucode,
			.load_ctxsw_ucode =
					nvgpu_gr_falcon_load_ctxsw_ucode,
#endif
#ifdef CONFIG_NVGPU_SIM
			.configure_fmodel = gm20b_gr_falcon_configure_fmodel,
#endif
		},
	},
	.gpu_class = {
		.is_valid = gp10b_class_is_valid,
		.is_valid_gfx = gp10b_class_is_valid_gfx,
		.is_valid_compute = gp10b_class_is_valid_compute,
	},
	.fb = {
		.init_hw = gm20b_fb_init_hw,
		.init_fs_state = fb_gm20b_init_fs_state,
		.set_mmu_page_size = NULL,
		.mmu_ctrl = gm20b_fb_mmu_ctrl,
		.mmu_debug_ctrl = gm20b_fb_mmu_debug_ctrl,
		.mmu_debug_wr = gm20b_fb_mmu_debug_wr,
		.mmu_debug_rd = gm20b_fb_mmu_debug_rd,
#ifdef CONFIG_NVGPU_COMPRESSION
		.set_use_full_comp_tag_line =
			gm20b_fb_set_use_full_comp_tag_line,
		.compression_page_size = gp10b_fb_compression_page_size,
		.compressible_page_size = gp10b_fb_compressible_page_size,
		.compression_align_mask = gm20b_fb_compression_align_mask,
#endif
		.vpr_info_fetch = gm20b_fb_vpr_info_fetch,
		.dump_vpr_info = gm20b_fb_dump_vpr_info,
		.dump_wpr_info = gm20b_fb_dump_wpr_info,
		.read_wpr_info = gm20b_fb_read_wpr_info,
#ifdef CONFIG_NVGPU_DEBUGGER
		.is_debug_mode_enabled = gm20b_fb_debug_mode_enabled,
		.set_debug_mode = gm20b_fb_set_debug_mode,
		.set_mmu_debug_mode = gm20b_fb_set_mmu_debug_mode,
#endif
		.tlb_invalidate = gm20b_fb_tlb_invalidate,
#ifdef CONFIG_NVGPU_DGPU
		.mem_unlock = NULL,
#endif
	},
	.cg = {
		.slcg_bus_load_gating_prod =
			gp10b_slcg_bus_load_gating_prod,
		.slcg_ce2_load_gating_prod =
			gp10b_slcg_ce2_load_gating_prod,
		.slcg_chiplet_load_gating_prod =
			gp10b_slcg_chiplet_load_gating_prod,
		.slcg_fb_load_gating_prod =
			gp10b_slcg_fb_load_gating_prod,
		.slcg_fifo_load_gating_prod =
			gp10b_slcg_fifo_load_gating_prod,
		.slcg_gr_load_gating_prod =
			gp10b_slcg_gr_load_gating_prod,
		.slcg_ltc_load_gating_prod =
			gp10b_slcg_ltc_load_gating_prod,
		.slcg_perf_load_gating_prod =
			gp10b_slcg_perf_load_gating_prod,
		.slcg_priring_load_gating_prod =
			gp10b_slcg_priring_load_gating_prod,
		.slcg_pmu_load_gating_prod =
			gp10b_slcg_pmu_load_gating_prod,
		.slcg_therm_load_gating_prod =
			gp10b_slcg_therm_load_gating_prod,
		.slcg_xbar_load_gating_prod =
			gp10b_slcg_xbar_load_gating_prod,
		.blcg_bus_load_gating_prod =
			gp10b_blcg_bus_load_gating_prod,
		.blcg_ce_load_gating_prod =
			gp10b_blcg_ce_load_gating_prod,
		.blcg_fb_load_gating_prod =
			gp10b_blcg_fb_load_gating_prod,
		.blcg_fifo_load_gating_prod =
			gp10b_blcg_fifo_load_gating_prod,
		.blcg_gr_load_gating_prod =
			gp10b_blcg_gr_load_gating_prod,
		.blcg_ltc_load_gating_prod =
			gp10b_blcg_ltc_load_gating_prod,
		.blcg_pmu_load_gating_prod =
			gp10b_blcg_pmu_load_gating_prod,
		.blcg_xbar_load_gating_prod =
			gp10b_blcg_xbar_load_gating_prod,
	},
	.fifo = {
		.fifo_init_support = nvgpu_fifo_init_support,
		.fifo_suspend = nvgpu_fifo_suspend,
		.init_fifo_setup_hw = gk20a_init_fifo_setup_hw,
		.preempt_channel = gk20a_fifo_preempt_channel,
		.preempt_tsg = gk20a_fifo_preempt_tsg,
		.preempt_trigger = gk20a_fifo_preempt_trigger,
		.is_preempt_pending = gk20a_fifo_is_preempt_pending,
		.reset_enable_hw = gk20a_init_fifo_reset_enable_hw,
#ifdef CONFIG_NVGPU_RECOVERY
		.recover = gk20a_fifo_recover,
#endif
		.intr_set_recover_mask = gk20a_fifo_intr_set_recover_mask,
		.intr_unset_recover_mask = gk20a_fifo_intr_unset_recover_mask,
		.setup_sw = nvgpu_fifo_setup_sw,
		.cleanup_sw = nvgpu_fifo_cleanup_sw,
#ifdef CONFIG_NVGPU_DEBUGGER
		.set_sm_exception_type_mask = nvgpu_tsg_set_sm_exception_type_mask,
#endif
		.intr_0_enable = gk20a_fifo_intr_0_enable,
		.intr_1_enable = gk20a_fifo_intr_1_enable,
		.intr_0_isr = gk20a_fifo_intr_0_isr,
		.intr_1_isr = gk20a_fifo_intr_1_isr,
		.handle_sched_error = gk20a_fifo_handle_sched_error,
		.ctxsw_timeout_enable = gk20a_fifo_ctxsw_timeout_enable,
		.handle_ctxsw_timeout = gk20a_fifo_handle_ctxsw_timeout,
		.trigger_mmu_fault = gm20b_fifo_trigger_mmu_fault,
		.get_mmu_fault_info = gp10b_fifo_get_mmu_fault_info,
		.get_mmu_fault_desc = gp10b_fifo_get_mmu_fault_desc,
		.get_mmu_fault_client_desc =
			gp10b_fifo_get_mmu_fault_client_desc,
		.get_mmu_fault_gpc_desc = gm20b_fifo_get_mmu_fault_gpc_desc,
		.get_runlist_timeslice = gk20a_fifo_get_runlist_timeslice,
		.get_pb_timeslice = gk20a_fifo_get_pb_timeslice,
		.is_mmu_fault_pending = gk20a_fifo_is_mmu_fault_pending,
		.bar1_snooping_disable = gk20a_fifo_bar1_snooping_disable,
		.find_pbdma_for_runlist = gk20a_fifo_find_pbdma_for_runlist,
	},
	.engine = {
		.is_fault_engine_subid_gpc = gm20b_is_fault_engine_subid_gpc,
		.init_info = nvgpu_engine_init_info,
		.init_ce_info = gp10b_engine_init_ce_info,
	},
	.pbdma = {
		.setup_sw = nvgpu_pbdma_setup_sw,
		.cleanup_sw = nvgpu_pbdma_cleanup_sw,
		.setup_hw = gm20b_pbdma_setup_hw,
		.intr_enable = gm20b_pbdma_intr_enable,
		.acquire_val = gm20b_pbdma_acquire_val,
		.get_signature = gp10b_pbdma_get_signature,
		.dump_status = gm20b_pbdma_dump_status,
		.syncpt_debug_dump = gm20b_pbdma_syncpoint_debug_dump,
		.handle_intr_0 = gm20b_pbdma_handle_intr_0,
		.handle_intr_1 = gm20b_pbdma_handle_intr_1,
		.handle_intr = gm20b_pbdma_handle_intr,
		.read_data = gm20b_pbdma_read_data,
		.reset_header = gm20b_pbdma_reset_header,
		.device_fatal_0_intr_descs =
			gm20b_pbdma_device_fatal_0_intr_descs,
		.channel_fatal_0_intr_descs =
			gp10b_pbdma_channel_fatal_0_intr_descs,
		.restartable_0_intr_descs =
			gm20b_pbdma_restartable_0_intr_descs,
		.format_gpfifo_entry =
			gm20b_pbdma_format_gpfifo_entry,
		.get_gp_base = gm20b_pbdma_get_gp_base,
		.get_gp_base_hi = gm20b_pbdma_get_gp_base_hi,
		.get_fc_formats = gm20b_pbdma_get_fc_formats,
		.get_fc_pb_header = gm20b_pbdma_get_fc_pb_header,
		.get_fc_subdevice = gm20b_pbdma_get_fc_subdevice,
		.get_fc_target = gm20b_pbdma_get_fc_target,
		.get_ctrl_hce_priv_mode_yes =
			gm20b_pbdma_get_ctrl_hce_priv_mode_yes,
		.get_userd_aperture_mask = gm20b_pbdma_get_userd_aperture_mask,
		.get_userd_addr = gm20b_pbdma_get_userd_addr,
		.get_userd_hi_addr = gm20b_pbdma_get_userd_hi_addr,
		.get_fc_runlist_timeslice =
			gp10b_pbdma_get_fc_runlist_timeslice,
		.get_config_auth_level_privileged =
			gp10b_pbdma_get_config_auth_level_privileged,
		.allowed_syncpoints_0_index_f =
			gp10b_pbdma_allowed_syncpoints_0_index_f,
		.allowed_syncpoints_0_valid_f =
			gp10b_pbdma_allowed_syncpoints_0_valid_f,
		.allowed_syncpoints_0_index_v =
			gp10b_pbdma_allowed_syncpoints_0_index_v,
	},
	.sync = {
#ifdef CONFIG_TEGRA_GK20A_NVHOST
		.syncpt = {
			.get_sync_ro_map = NULL,
			.alloc_buf = gk20a_syncpt_alloc_buf,
			.free_buf = gk20a_syncpt_free_buf,
#ifdef CONFIG_NVGPU_KERNEL_MODE_SUBMIT
			.add_wait_cmd = gk20a_syncpt_add_wait_cmd,
			.get_wait_cmd_size =
					gk20a_syncpt_get_wait_cmd_size,
			.add_incr_cmd = gk20a_syncpt_add_incr_cmd,
			.get_incr_cmd_size =
					gk20a_syncpt_get_incr_cmd_size,
			.get_incr_per_release =
					gk20a_syncpt_get_incr_per_release,
#endif
		},
#endif /* CONFIG_TEGRA_GK20A_NVHOST */
#if defined(CONFIG_NVGPU_KERNEL_MODE_SUBMIT) && \
	defined(CONFIG_NVGPU_SW_SEMAPHORE)
		.sema = {
			.add_wait_cmd = gk20a_sema_add_wait_cmd,
			.get_wait_cmd_size = gk20a_sema_get_wait_cmd_size,
			.add_incr_cmd = gk20a_sema_add_incr_cmd,
			.get_incr_cmd_size = gk20a_sema_get_incr_cmd_size,
		},
#endif
	},
	.engine_status = {
		.read_engine_status_info =
			gm20b_read_engine_status_info,
		.dump_engine_status = gm20b_dump_engine_status,
	},
	.pbdma_status = {
		.read_pbdma_status_info =
			gm20b_read_pbdma_status_info,
	},
	.ramfc = {
		.setup = gp10b_ramfc_setup,
		.capture_ram_dump = gk20a_ramfc_capture_ram_dump,
		.commit_userd = gp10b_ramfc_commit_userd,
		.get_syncpt = gp10b_ramfc_get_syncpt,
		.set_syncpt = gp10b_ramfc_set_syncpt,
	},
	.ramin = {
		.set_gr_ptr = gk20a_ramin_set_gr_ptr,
		.set_big_page_size = gm20b_ramin_set_big_page_size,
		.init_pdb = gp10b_ramin_init_pdb,
		.init_subctx_pdb = NULL,
		.set_adr_limit = gk20a_ramin_set_adr_limit,
		.base_shift = gk20a_ramin_base_shift,
		.alloc_size = gk20a_ramin_alloc_size,
		.set_eng_method_buffer = NULL,
	},
	.runlist = {
#ifdef CONFIG_NVGPU_CHANNEL_TSG_SCHEDULING
		.reschedule = gk20a_runlist_reschedule,
		.reschedule_preempt_next_locked = gk20a_fifo_reschedule_preempt_next,
#endif
		.update_for_channel = nvgpu_runlist_update_for_channel,
		.reload = nvgpu_runlist_reload,
		.count_max = gk20a_runlist_count_max,
		.entry_size = gk20a_runlist_entry_size,
		.length_max = gk20a_runlist_length_max,
		.get_tsg_entry = gk20a_runlist_get_tsg_entry,
		.get_ch_entry = gk20a_runlist_get_ch_entry,
		.hw_submit = gk20a_runlist_hw_submit,
		.wait_pending = gk20a_runlist_wait_pending,
		.write_state = gk20a_runlist_write_state,
		.init_enginfo = nvgpu_runlist_init_enginfo,
	},
	.userd = {
#ifdef CONFIG_NVGPU_USERD
		.setup_sw = nvgpu_userd_setup_sw,
		.cleanup_sw = nvgpu_userd_cleanup_sw,
		.init_mem = gk20a_userd_init_mem,
#ifdef CONFIG_NVGPU_KERNEL_MODE_SUBMIT
		.gp_get = gk20a_userd_gp_get,
		.gp_put = gk20a_userd_gp_put,
		.pb_get = gk20a_userd_pb_get,
#endif
#endif /* CONFIG_NVGPU_USERD */
		.entry_size = gk20a_userd_entry_size,
	},
	.channel = {
		.alloc_inst = nvgpu_channel_alloc_inst,
		.free_inst = nvgpu_channel_free_inst,
		.bind = gm20b_channel_bind,
		.unbind = gk20a_channel_unbind,
		.enable = gk20a_channel_enable,
		.disable = gk20a_channel_disable,
		.count = gm20b_channel_count,
		.read_state = gk20a_channel_read_state,
		.force_ctx_reload = gm20b_channel_force_ctx_reload,
#if defined(CONFIG_NVGPU_KERNEL_MODE_SUBMIT) && defined(CONFIG_TEGRA_GK20A_NVHOST)
		.set_syncpt = nvgpu_channel_set_syncpt,
#endif
		.abort_clean_up = nvgpu_channel_abort_clean_up,
		.suspend_all_serviceable_ch =
                        nvgpu_channel_suspend_all_serviceable_ch,
		.resume_all_serviceable_ch =
                        nvgpu_channel_resume_all_serviceable_ch,
		.set_error_notifier = nvgpu_set_err_notifier,
	},
	.tsg = {
		.enable = gk20a_tsg_enable,
		.disable = nvgpu_tsg_disable,
		.bind_channel = NULL,
		.unbind_channel = NULL,
		.unbind_channel_check_hw_state =
				nvgpu_tsg_unbind_channel_check_hw_state,
		.unbind_channel_check_ctx_reload =
				nvgpu_tsg_unbind_channel_check_ctx_reload,
		.unbind_channel_check_eng_faulted = NULL,
#ifdef CONFIG_NVGPU_KERNEL_MODE_SUBMIT
		.check_ctxsw_timeout = nvgpu_tsg_check_ctxsw_timeout,
#endif
#ifdef CONFIG_NVGPU_CHANNEL_TSG_CONTROL
		.force_reset = nvgpu_tsg_force_reset_ch,
		.post_event_id = nvgpu_tsg_post_event_id,
#endif
#ifdef CONFIG_NVGPU_CHANNEL_TSG_SCHEDULING
		.set_timeslice = nvgpu_tsg_set_timeslice,
#endif
		.default_timeslice_us = nvgpu_tsg_default_timeslice_us,
	},
	.netlist = {
		.get_netlist_name = gp10b_netlist_get_name,
		.is_fw_defined = gp10b_netlist_is_firmware_defined,
	},
	.mm = {
		.init_mm_support = nvgpu_init_mm_support,
		.pd_cache_init = nvgpu_pd_cache_init,
		.mm_suspend = nvgpu_mm_suspend,
		.vm_bind_channel = nvgpu_vm_bind_channel,
		.setup_hw = nvgpu_mm_setup_hw,
		.is_bar1_supported = gm20b_mm_is_bar1_supported,
		.init_inst_block = gk20a_mm_init_inst_block,
		.init_bar2_vm = gp10b_mm_init_bar2_vm,
		.remove_bar2_vm = gp10b_mm_remove_bar2_vm,
#ifdef CONFIG_NVGPU_USERD
		.bar1_map_userd = gk20a_mm_bar1_map_userd,
#endif
		.cache = {
			.fb_flush = gk20a_mm_fb_flush,
			.l2_invalidate = gk20a_mm_l2_invalidate,
			.l2_flush = gk20a_mm_l2_flush,
#ifdef CONFIG_NVGPU_COMPRESSION
			.cbc_clean = gk20a_mm_cbc_clean,
#endif
		},
		.gmmu = {
			.get_mmu_levels = gp10b_mm_get_mmu_levels,
			.get_max_page_table_levels =
				gp10b_get_max_page_table_levels,
			.map = nvgpu_gmmu_map_locked,
			.unmap = nvgpu_gmmu_unmap_locked,
			.get_big_page_sizes = gm20b_mm_get_big_page_sizes,
			.get_default_big_page_size =
				nvgpu_gmmu_default_big_page_size,
			.get_iommu_bit = gp10b_mm_get_iommu_bit,
			.gpu_phys_addr = gm20b_gpu_phys_addr,
		}
	},
	.therm = {
		.init_therm_support = nvgpu_init_therm_support,
		.init_therm_setup_hw = gp10b_init_therm_setup_hw,
		.init_elcg_mode = gm20b_therm_init_elcg_mode,
		.init_blcg_mode = gm20b_therm_init_blcg_mode,
		.elcg_init_idle_filters = gp10b_elcg_init_idle_filters,
	},
#ifdef CONFIG_NVGPU_LS_PMU
	.pmu = {
		/* Init */
		.pmu_early_init = nvgpu_pmu_early_init,
		.pmu_rtos_init = nvgpu_pmu_rtos_init,
		.pmu_pstate_sw_setup = nvgpu_pmu_pstate_sw_setup,
		.pmu_pstate_pmu_setup = nvgpu_pmu_pstate_pmu_setup,
		.pmu_destroy = nvgpu_pmu_destroy,
		.is_pmu_supported = gp10b_is_pmu_supported,
		.falcon_base_addr = gk20a_pmu_falcon_base_addr,
		.setup_apertures = gm20b_pmu_setup_apertures,
		.secured_pmu_start = gm20b_secured_pmu_start,
		.pmu_setup_elpg = gp10b_pmu_setup_elpg,
		.pmu_get_queue_head = gp10b_pmu_queue_head_r,
		.pmu_get_queue_head_size = gp10b_pmu_queue_head__size_1_v,
		.pmu_get_queue_tail = gp10b_pmu_queue_tail_r,
		.pmu_get_queue_tail_size = gp10b_pmu_queue_tail__size_1_v,
		.pmu_reset = nvgpu_pmu_reset,
		.pmu_queue_head = gk20a_pmu_queue_head,
		.pmu_queue_tail = gk20a_pmu_queue_tail,
		.pmu_msgq_tail = gk20a_pmu_msgq_tail,
		.pmu_mutex_size = gp10b_pmu_mutex__size_1_v,
		.pmu_mutex_owner = gk20a_pmu_mutex_owner,
		.pmu_mutex_acquire = gk20a_pmu_mutex_acquire,
		.pmu_mutex_release = gk20a_pmu_mutex_release,
		.pmu_is_interrupted = gk20a_pmu_is_interrupted,
		.pmu_isr = gk20a_pmu_isr,
		.pmu_init_perfmon_counter = gk20a_pmu_init_perfmon_counter,
		.pmu_pg_idle_counter_config = gk20a_pmu_pg_idle_counter_config,
		.pmu_read_idle_counter = gk20a_pmu_read_idle_counter,
		.pmu_reset_idle_counter = gk20a_pmu_reset_idle_counter,
		.pmu_read_idle_intr_status = gk20a_pmu_read_idle_intr_status,
		.pmu_clear_idle_intr_status = gk20a_pmu_clear_idle_intr_status,
		.pmu_dump_elpg_stats = gk20a_pmu_dump_elpg_stats,
		.pmu_dump_falcon_stats = gk20a_pmu_dump_falcon_stats,
		.pmu_enable_irq = gk20a_pmu_enable_irq,
		.write_dmatrfbase = gp10b_write_dmatrfbase,
		.dump_secure_fuses = pmu_dump_security_fuses_gm20b,
		.reset_engine = gk20a_pmu_engine_reset,
		.is_engine_in_reset = gk20a_pmu_is_engine_in_reset,
		.get_irqdest = gk20a_pmu_get_irqdest,
		.is_debug_mode_enabled = gm20b_pmu_is_debug_mode_en,
		.pmu_clear_bar0_host_err_status =
			gm20b_clear_pmu_bar0_host_err_status,
		.bar0_error_status = gk20a_pmu_bar0_error_status,
		.flcn_setup_boot_config = gm20b_pmu_flcn_setup_boot_config,
		.pmu_ns_bootstrap = gk20a_pmu_ns_bootstrap,
	},
#endif
#ifdef CONFIG_NVGPU_CLK_ARB
	.clk_arb = {
		.clk_arb_init_arbiter = nvgpu_clk_arb_init_arbiter,
		.check_clk_arb_support = gp10b_check_clk_arb_support,
		.get_arbiter_clk_domains = gp10b_get_arbiter_clk_domains,
		.get_arbiter_f_points = gp10b_get_arbiter_f_points,
		.get_arbiter_clk_range = gp10b_get_arbiter_clk_range,
		.get_arbiter_clk_default = gp10b_get_arbiter_clk_default,
		.arbiter_clk_init = gp10b_init_clk_arbiter,
		.clk_arb_run_arbiter_cb = gp10b_clk_arb_run_arbiter_cb,
		.clk_arb_cleanup = gp10b_clk_arb_cleanup,
	},
#endif
#ifdef CONFIG_NVGPU_DEBUGGER
	.regops = {
		.exec_regops = exec_regops_gk20a,
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
	},
#endif
	.mc = {
		.get_chip_details = gm20b_get_chip_details,
		.intr_mask = mc_gp10b_intr_mask,
		.intr_enable = NULL,
		.intr_stall_unit_config = mc_gp10b_intr_stall_unit_config,
		.intr_nonstall_unit_config = mc_gp10b_intr_nonstall_unit_config,
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
		.reset_mask = gm20b_mc_reset_mask,
#ifdef CONFIG_NVGPU_LS_PMU
		.is_enabled = gm20b_mc_is_enabled,
#endif
		.fb_reset = gm20b_mc_fb_reset,
		.ltc_isr = mc_gp10b_ltc_isr,
		.is_mmu_fault_pending = gm20b_mc_is_mmu_fault_pending,
	},
	.debug = {
		.show_dump = gk20a_debug_show_dump,
	},
#ifdef CONFIG_NVGPU_DEBUGGER
	.debugger = {
		.post_events = nvgpu_dbg_gpu_post_events,
		.dbg_set_powergate = nvgpu_dbg_set_powergate,
	},
	.perf = {
		.enable_membuf = gm20b_perf_enable_membuf,
		.disable_membuf = gm20b_perf_disable_membuf,
		.membuf_reset_streaming = gm20b_perf_membuf_reset_streaming,
		.get_membuf_pending_bytes = gm20b_perf_get_membuf_pending_bytes,
		.set_membuf_handled_bytes = gm20b_perf_set_membuf_handled_bytes,
		.get_membuf_overflow_status =
			gm20b_perf_get_membuf_overflow_status,
		.get_pmm_per_chiplet_offset =
			gm20b_perf_get_pmm_per_chiplet_offset,
	},
	.perfbuf = {
		.perfbuf_enable = nvgpu_perfbuf_enable_locked,
		.perfbuf_disable = nvgpu_perfbuf_disable_locked,
	},
#endif
#ifdef CONFIG_NVGPU_PROFILER
	.pm_reservation = {
		.acquire = nvgpu_pm_reservation_acquire,
		.release = nvgpu_pm_reservation_release,
		.release_all_per_vmid =
			nvgpu_pm_reservation_release_all_per_vmid,
	},
#endif
	.bus = {
		.init_hw = gk20a_bus_init_hw,
		.isr = gk20a_bus_isr,
		.bar1_bind = gm20b_bus_bar1_bind,
		.bar2_bind = gp10b_bus_bar2_bind,
#ifdef CONFIG_NVGPU_DGPU
		.set_bar0_window = gk20a_bus_set_bar0_window,
#endif
	},
	.ptimer = {
		.isr = gk20a_ptimer_isr,
		.read_ptimer = gk20a_read_ptimer,
#ifdef CONFIG_NVGPU_IOCTL_NON_FUSA
		.get_timestamps_zipper = nvgpu_get_timestamps_zipper,
#endif
#ifdef CONFIG_NVGPU_DEBUGGER
		.config_gr_tick_freq = gp10b_ptimer_config_gr_tick_freq,
#endif
	},
#if defined(CONFIG_NVGPU_CYCLESTATS)
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
		.falcon_sw_init = nvgpu_falcon_sw_init,
		.falcon_sw_free = nvgpu_falcon_sw_free,
		.reset = gk20a_falcon_reset,
		.is_falcon_cpu_halted =  gk20a_is_falcon_cpu_halted,
		.is_falcon_idle =  gk20a_is_falcon_idle,
		.is_falcon_scrubbing_done =  gk20a_is_falcon_scrubbing_done,
		.get_mem_size = gk20a_falcon_get_mem_size,
		.get_ports_count = gk20a_falcon_get_ports_count,
		.copy_to_dmem = gk20a_falcon_copy_to_dmem,
		.copy_to_imem = gk20a_falcon_copy_to_imem,
		.dmemc_blk_mask = gk20a_falcon_dmemc_blk_mask,
		.imemc_blk_field = gk20a_falcon_imemc_blk_field,
		.bootstrap = gk20a_falcon_bootstrap,
		.mailbox_read = gk20a_falcon_mailbox_read,
		.mailbox_write = gk20a_falcon_mailbox_write,
#ifdef CONFIG_NVGPU_FALCON_DEBUG
		.dump_falcon_stats = gk20a_falcon_dump_stats,
#endif
#ifdef CONFIG_NVGPU_FALCON_NON_FUSA
		.clear_halt_interrupt_status =
			gk20a_falcon_clear_halt_interrupt_status,
		.set_irq = gk20a_falcon_set_irq,
		.copy_from_dmem = gk20a_falcon_copy_from_dmem,
		.copy_from_imem = gk20a_falcon_copy_from_imem,
		.get_falcon_ctls = gk20a_falcon_get_ctls,
#endif
	},
	.fbp = {
		.fbp_init_support = nvgpu_fbp_init_support,
	},
	.priv_ring = {
		.enable_priv_ring = gm20b_priv_ring_enable,
		.isr = gp10b_priv_ring_isr,
		.isr_handle_0 = gp10b_priv_ring_isr_handle_0,
		.isr_handle_1 = gp10b_priv_ring_isr_handle_1,
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
		.read_per_device_identifier =
			nvgpu_tegra_fuse_read_per_device_identifier,
	},
	.top = {
		.parse_next_device = gm20b_top_parse_next_dev,
		.device_info_parse_enum = gm20b_device_info_parse_enum,
		.device_info_parse_data = gp10b_device_info_parse_data,
		.get_ce_inst_id = NULL,
		.get_max_gpc_count = gm20b_top_get_max_gpc_count,
		.get_max_tpc_per_gpc_count =
			gm20b_top_get_max_tpc_per_gpc_count,
		.get_max_fbps_count = gm20b_top_get_max_fbps_count,
		.get_max_ltc_per_fbp = gm20b_top_get_max_ltc_per_fbp,
		.get_max_lts_per_ltc = gm20b_top_get_max_lts_per_ltc,
		.get_num_ltcs = gm20b_top_get_num_ltcs,
	},
#ifdef CONFIG_NVGPU_TPC_POWERGATE
	.tpc = {
		.init_tpc_powergate = NULL,
		.tpc_gr_pg = NULL,
	},
#endif
	.chip_init_gpu_characteristics = gp10b_init_gpu_characteristics,
	.get_litter_value = gp10b_get_litter_value,
};

int gp10b_init_hal(struct gk20a *g)
{
	struct gpu_ops *gops = &g->ops;

#ifndef CONFIG_NVGPU_USERD
	nvgpu_err(g, "CONFIG_NVGPU_USERD is needed for gp10b support");
	return -EINVAL;
#endif

	gops->acr = gp10b_ops.acr;
	gops->bios = gp10b_ops.bios;
	gops->ecc = gp10b_ops.ecc;
	gops->fbp = gp10b_ops.fbp;
	gops->ltc = gp10b_ops.ltc;
#ifdef CONFIG_NVGPU_COMPRESSION
	gops->cbc = gp10b_ops.cbc;
#endif
	gops->ce = gp10b_ops.ce;
	gops->gr = gp10b_ops.gr;
	gops->gpu_class = gp10b_ops.gpu_class;
	gops->gr.ctxsw_prog = gp10b_ops.gr.ctxsw_prog;
	gops->gr.config = gp10b_ops.gr.config;
	gops->fb = gp10b_ops.fb;
	gops->cg = gp10b_ops.cg;
	gops->fifo = gp10b_ops.fifo;
	gops->engine = gp10b_ops.engine;
	gops->pbdma = gp10b_ops.pbdma;
	gops->ramfc = gp10b_ops.ramfc;
	gops->ramin = gp10b_ops.ramin;
	gops->runlist = gp10b_ops.runlist;
	gops->userd = gp10b_ops.userd;
	gops->channel = gp10b_ops.channel;
	gops->tsg = gp10b_ops.tsg;
	gops->sync = gp10b_ops.sync;
	gops->engine_status = gp10b_ops.engine_status;
	gops->pbdma_status = gp10b_ops.pbdma_status;
	gops->netlist = gp10b_ops.netlist;
	gops->mm = gp10b_ops.mm;
	gops->therm = gp10b_ops.therm;
#ifdef CONFIG_NVGPU_LS_PMU
	gops->pmu = gp10b_ops.pmu;
#endif
	gops->clk_arb = gp10b_ops.clk_arb;
	gops->mc = gp10b_ops.mc;
	gops->debug = gp10b_ops.debug;
#ifdef CONFIG_NVGPU_DEBUGGER
	gops->debugger = gp10b_ops.debugger;
	gops->regops = gp10b_ops.regops;
	gops->perf = gp10b_ops.perf;
	gops->perfbuf = gp10b_ops.perfbuf;
#endif
#ifdef CONFIG_NVGPU_PROFILER
	gops->pm_reservation = gp10b_ops.pm_reservation;
#endif
	gops->bus = gp10b_ops.bus;
	gops->ptimer = gp10b_ops.ptimer;
#if defined(CONFIG_NVGPU_CYCLESTATS)
	gops->css = gp10b_ops.css;
#endif
	gops->falcon = gp10b_ops.falcon;

	gops->priv_ring = gp10b_ops.priv_ring;

	gops->fuse = gp10b_ops.fuse;
	gops->tpc = gp10b_ops.tpc;
	gops->top = gp10b_ops.top;

	/* Lone Functions */
	gops->chip_init_gpu_characteristics =
		gp10b_ops.chip_init_gpu_characteristics;
	gops->get_litter_value = gp10b_ops.get_litter_value;
	gops->semaphore_wakeup = nvgpu_channel_semaphore_wakeup;

	nvgpu_set_enabled(g, NVGPU_GR_USE_DMA_FOR_FW_BOOTSTRAP, true);
#ifdef CONFIG_NVGPU_FECS_TRACE
	nvgpu_set_enabled(g, NVGPU_FECS_TRACE_VA, false);
	nvgpu_set_enabled(g, NVGPU_FECS_TRACE_FEATURE_CONTROL, false);
#endif

#ifdef CONFIG_NVGPU_DGPU
	nvgpu_pramin_ops_init(g);
#endif

	/* Read fuses to check if gpu needs to boot in secure/non-secure mode */
	if (gops->fuse.check_priv_security(g) != 0) {
		return -EINVAL; /* Do not boot gpu */
	}

	/* priv security dependent ops */
	if (nvgpu_is_enabled(g, NVGPU_SEC_PRIVSECURITY)) {
		gops->gr.falcon.load_ctxsw_ucode =
			nvgpu_gr_falcon_load_secure_ctxsw_ucode;
	} else {
		/* Inherit from gk20a */
#ifdef CONFIG_NVGPU_LS_PMU
		gops->pmu.setup_apertures =
				gm20b_pmu_ns_setup_apertures;
#endif
	}

#ifdef CONFIG_NVGPU_COMPRESSION
	nvgpu_set_enabled(g, NVGPU_SUPPORT_COMPRESSION, true);

	if (!nvgpu_is_enabled(g, NVGPU_SUPPORT_COMPRESSION)) {
		gops->cbc.init = NULL;
		gops->cbc.ctrl = NULL;
		gops->cbc.alloc_comptags = NULL;
	}
#endif

#ifdef CONFIG_NVGPU_GRAPHICS
	nvgpu_set_enabled(g, NVGPU_SUPPORT_ZBC_STENCIL, false);
	nvgpu_set_enabled(g, NVGPU_SUPPORT_PREEMPTION_GFXP, true);
#endif
	nvgpu_set_enabled(g, NVGPU_PMU_FECS_BOOTSTRAP_DONE, false);
	nvgpu_set_enabled(g, NVGPU_SUPPORT_SET_CTX_MMU_DEBUG_MODE, false);

	g->max_sm_diversity_config_count =
		NVGPU_DEFAULT_SM_DIVERSITY_CONFIG_COUNT;

	g->name = "gp10b";

	return 0;
}
