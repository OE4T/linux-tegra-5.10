/*
 * Copyright (c) 2011-2020, NVIDIA CORPORATION.  All rights reserved.
 *
 * GK20A Graphics
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
#ifndef GK20A_H
#define GK20A_H

/**
 * @mainpage
 *
 * NVGPU Design Documentation
 * ==========================
 *
 * Welcome to the nvgpu unit design documentation. The following pages document
 * the major top level units within nvgpu-common:
 *
 *   - @ref unit-ce
 *   - @ref unit-mm
 *   - @ref unit-common-bus
 *   - @ref unit-fifo
 *   - @ref unit-gr
 *   - @ref unit-fb
 *   - @ref unit-devctl
 *   - @ref unit-sdl
 *   - @ref unit-init
 *   - @ref unit-qnx_init
 *   - @ref unit-falcon
 *   - @ref unit-os_utils
 *   - @ref unit-acr
 *   - @ref unit-cg
 *   - @ref unit-pmu
 *   - @ref unit-common-priv-ring
 *   - @ref unit-common-nvgpu
 *   - @ref unit-common-ltc
 *   - @ref unit-common-utils
 *   - @ref unit-common-netlist
 *   - @ref unit-mc
 *   - Etc, etc.
 *
 * NVGPU Software Unit Design Documentation
 * ========================================
 *
 * For each top level unit, a corresponding Unit Test Specification is
 * available in the @ref NVGPU-SWUTS
 *
 * nvgpu-driver Level Requirements Table
 * =====================================
 *
 * ...
 */

struct gk20a;
struct nvgpu_acr;
struct nvgpu_fifo;
struct nvgpu_channel;
struct nvgpu_gr;
struct nvgpu_fbp;
struct sim_nvgpu;
struct nvgpu_ce_app;
struct gk20a_ctxsw_trace;
struct nvgpu_mem_alloc_tracker;
struct dbg_profiler_object_data;
struct nvgpu_debug_context;
struct nvgpu_clk_pll_debug_data;
struct nvgpu_nvhost_dev;
struct nvgpu_netlist_vars;
struct netlist_av64_list;
#ifdef CONFIG_NVGPU_FECS_TRACE
struct nvgpu_gr_fecs_trace;
#endif
struct nvgpu_cpu_time_correlation_sample;
#ifdef CONFIG_NVGPU_CLK_ARB
struct nvgpu_clk_arb;
#endif
struct nvgpu_setup_bind_args;
struct boardobjgrp;
struct boardobjgrp_pmu_cmd;
struct boardobjgrpmask;
struct nvgpu_sgt;
struct nvgpu_channel_hw_state;
struct nvgpu_mem;
struct gk20a_cs_snapshot_client;
struct dbg_session_gk20a;
struct nvgpu_dbg_reg_op;
struct gk20a_cs_snapshot;
struct _resmgr_context;
struct nvgpu_gpfifo_entry;
struct vm_gk20a_mapping_batch;
struct pmu_pg_stats_data;
struct clk_domains_mon_status_params;

enum nvgpu_flush_op;
enum gk20a_mem_rw_flag;
enum nvgpu_nvlink_minion_dlcmd;
enum nvgpu_unit;

#include <nvgpu/lock.h>
#include <nvgpu/thread.h>
#include <nvgpu/utils.h>

#include <nvgpu/mm.h>
#include <nvgpu/as.h>
#include <nvgpu/log.h>
#include <nvgpu/kref.h>
#include <nvgpu/pmu.h>
#include <nvgpu/atomic.h>
#include <nvgpu/barrier.h>
#include <nvgpu/rwsem.h>
#include <nvgpu/nvlink.h>
#include <nvgpu/nvlink_link_mode_transitions.h>
#include <nvgpu/ecc.h>
#include <nvgpu/channel.h>
#include <nvgpu/tsg.h>
#include <nvgpu/sec2/sec2.h>
#include <nvgpu/cbc.h>
#include <nvgpu/ltc.h>
#include <nvgpu/worker.h>
#include <nvgpu/bios.h>
#include <nvgpu/semaphore.h>
#include <nvgpu/fifo.h>
#include <nvgpu/sched.h>

#include <nvgpu/gops_class.h>
#include <nvgpu/gops_ce.h>
#include <nvgpu/gops_ptimer.h>
#include <nvgpu/gops_top.h>
#include <nvgpu/gops_bus.h>
#include <nvgpu/gops_gr.h>
#include <nvgpu/gops_falcon.h>
#include <nvgpu/gops_fifo.h>
#include <nvgpu/gops_fuse.h>
#include <nvgpu/gops_ltc.h>
#include <nvgpu/gops_ramfc.h>
#include <nvgpu/gops_ramin.h>
#include <nvgpu/gops_runlist.h>
#include <nvgpu/gops_userd.h>
#include <nvgpu/gops_engine.h>
#include <nvgpu/gops_pbdma.h>
#include <nvgpu/gops_sync.h>
#include <nvgpu/gops_channel.h>
#include <nvgpu/gops_tsg.h>
#include <nvgpu/gops_usermode.h>
#include <nvgpu/gops_mm.h>
#include <nvgpu/gops_netlist.h>
#include <nvgpu/gops_priv_ring.h>
#include <nvgpu/gops_therm.h>
#include <nvgpu/gops_fb.h>
#include <nvgpu/gops_mc.h>
#include <nvgpu/gops_cg.h>
#include <nvgpu/gops_pmu.h>
#include <nvgpu/gops_ecc.h>

#include "hal/clk/clk_gk20a.h"

#ifdef CONFIG_DEBUG_FS
struct railgate_stats {
	unsigned long last_rail_gate_start;
	unsigned long last_rail_gate_complete;
	unsigned long last_rail_ungate_start;
	unsigned long last_rail_ungate_complete;
	unsigned long total_rail_gate_time_ms;
	unsigned long total_rail_ungate_time_ms;
	unsigned long railgating_cycle_count;
};
#endif

#define GPU_LIT_NUM_GPCS	0
#define GPU_LIT_NUM_PES_PER_GPC 1
#define GPU_LIT_NUM_ZCULL_BANKS 2
#define GPU_LIT_NUM_TPC_PER_GPC 3
#define GPU_LIT_NUM_SM_PER_TPC  4
#define GPU_LIT_NUM_FBPS	5
#define GPU_LIT_GPC_BASE	6
#define GPU_LIT_GPC_STRIDE	7
#define GPU_LIT_GPC_SHARED_BASE 8
#define GPU_LIT_TPC_IN_GPC_BASE 9
#define GPU_LIT_TPC_IN_GPC_STRIDE 10
#define GPU_LIT_TPC_IN_GPC_SHARED_BASE 11
#define GPU_LIT_PPC_IN_GPC_BASE	12
#define GPU_LIT_PPC_IN_GPC_STRIDE 13
#define GPU_LIT_PPC_IN_GPC_SHARED_BASE 14
#define GPU_LIT_ROP_BASE	15
#define GPU_LIT_ROP_STRIDE	16
#define GPU_LIT_ROP_SHARED_BASE 17
#define GPU_LIT_HOST_NUM_ENGINES 18
#define GPU_LIT_HOST_NUM_PBDMA	19
#define GPU_LIT_LTC_STRIDE	20
#define GPU_LIT_LTS_STRIDE	21
#define GPU_LIT_NUM_FBPAS	22
#define GPU_LIT_FBPA_STRIDE	23
#define GPU_LIT_FBPA_BASE	24
#define GPU_LIT_FBPA_SHARED_BASE 25
#define GPU_LIT_SM_PRI_STRIDE	26
#define GPU_LIT_SMPC_PRI_BASE		27
#define GPU_LIT_SMPC_PRI_SHARED_BASE	28
#define GPU_LIT_SMPC_PRI_UNIQUE_BASE	29
#define GPU_LIT_SMPC_PRI_STRIDE		30
#define GPU_LIT_TWOD_CLASS	31
#define GPU_LIT_THREED_CLASS	32
#define GPU_LIT_COMPUTE_CLASS	33
#define GPU_LIT_GPFIFO_CLASS	34
#define GPU_LIT_I2M_CLASS	35
#define GPU_LIT_DMA_COPY_CLASS	36
#define GPU_LIT_GPC_PRIV_STRIDE	37
#define GPU_LIT_PERFMON_PMMGPCTPCA_DOMAIN_START 38
#define GPU_LIT_PERFMON_PMMGPCTPCB_DOMAIN_START 39
#define GPU_LIT_PERFMON_PMMGPCTPC_DOMAIN_COUNT  40
#define GPU_LIT_PERFMON_PMMFBP_LTC_DOMAIN_START 41
#define GPU_LIT_PERFMON_PMMFBP_LTC_DOMAIN_COUNT 42
#define GPU_LIT_PERFMON_PMMFBP_ROP_DOMAIN_START 43
#define GPU_LIT_PERFMON_PMMFBP_ROP_DOMAIN_COUNT 44

#define nvgpu_get_litter_value(g, v) ((g)->ops.get_litter_value((g), v))

#define MAX_TPC_PG_CONFIGS      9

struct nvgpu_gpfifo_userdata {
	struct nvgpu_gpfifo_entry nvgpu_user *entries;
	struct _resmgr_context *context;
};

enum nvgpu_event_id_type {
	NVGPU_EVENT_ID_BPT_INT = 0,
	NVGPU_EVENT_ID_BPT_PAUSE = 1,
	NVGPU_EVENT_ID_BLOCKING_SYNC = 2,
	NVGPU_EVENT_ID_CILP_PREEMPTION_STARTED = 3,
	NVGPU_EVENT_ID_CILP_PREEMPTION_COMPLETE = 4,
	NVGPU_EVENT_ID_GR_SEMAPHORE_WRITE_AWAKEN = 5,
	NVGPU_EVENT_ID_MAX = 6,
};

/**
 * @addtogroup unit-common-nvgpu
 * @{
 */

/**
 * @brief HAL methods
 *
 * gpu_ops contains function pointers for the unit HAL interfaces. gpu_ops
 * should only contain function pointers! Non-function pointer members should go
 * in struct gk20a or be implemented with the boolean flag API defined in
 * nvgpu/enabled.h. Each unit should have its own sub-struct in the gpu_ops
 * struct.
 */

struct gpu_ops {
	struct {
		int (*acr_init)(struct gk20a *g);
		int (*acr_construct_execute)(struct gk20a *g);
	} acr;
	struct {
		int (*sbr_pub_load_and_execute)(struct gk20a *g);
	} sbr;

	struct gops_ecc ecc;
	struct gops_ltc ltc;
#ifdef CONFIG_NVGPU_COMPRESSION
	struct {
		int (*cbc_init_support)(struct gk20a *g);
		void (*cbc_remove_support)(struct gk20a *g);
		void (*init)(struct gk20a *g, struct nvgpu_cbc *cbc);
		u64 (*get_base_divisor)(struct gk20a *g);
		int (*alloc_comptags)(struct gk20a *g,
					struct nvgpu_cbc *cbc);
		int (*ctrl)(struct gk20a *g, enum nvgpu_cbc_op op,
				u32 min, u32 max);
		u32 (*fix_config)(struct gk20a *g, int base);
	} cbc;
#endif
	struct gops_ce ce;
	struct gops_gr gr;
	struct gops_class gpu_class;
	struct gops_fb fb;

	struct {
		u32 (*falcon_base_addr)(void);
	} nvdec;
	struct gops_cg cg;
	struct gops_fifo fifo;
	struct gops_fuse fuse;
	struct gops_ramfc ramfc;
	struct gops_ramin ramin;
	struct gops_runlist runlist;
	struct gops_userd userd;
	struct gops_engine engine;
	struct gops_pbdma pbdma;
	struct gops_sync sync;
	struct gops_channel channel;
	struct gops_tsg tsg;
	struct gops_usermode usermode;
	struct gops_engine_status engine_status;
	struct gops_pbdma_status pbdma_status;
	struct gops_netlist netlist;
	struct gops_mm mm;
	/*
	 * This function is called to allocate secure memory (memory
	 * that the CPU cannot see). The function should fill the
	 * context buffer descriptor (especially fields destroy, sgt,
	 * size).
	 */
	int (*secure_alloc)(struct gk20a *g, struct nvgpu_mem *desc_mem,
			size_t size,
			void (**fn)(struct gk20a *g, struct nvgpu_mem *mem));

#ifdef CONFIG_NVGPU_DGPU
	struct {
		u32 (*data032_r)(u32 i);
	} pramin;
#endif
	struct gops_therm therm;
	struct gops_pmu pmu;
	struct {
		int (*init_debugfs)(struct gk20a *g);
		int (*init_clk_support)(struct gk20a *g);
		void (*suspend_clk_support)(struct gk20a *g);
		u32 (*get_crystal_clk_hz)(struct gk20a *g);
		int (*clk_domain_get_f_points)(struct gk20a *g,
			u32 clkapidomain, u32 *pfpointscount,
			u16 *pfreqpointsinmhz);
		int (*clk_get_round_rate)(struct gk20a *g, u32 api_domain,
			unsigned long rate_target, unsigned long *rounded_rate);
		int (*get_clk_range)(struct gk20a *g, u32 api_domain,
			u16 *min_mhz, u16 *max_mhz);
		unsigned long (*measure_freq)(struct gk20a *g, u32 api_domain);
		u32 (*get_rate_cntr)(struct gk20a *g, struct namemap_cfg *c);
		unsigned long (*get_rate)(struct gk20a *g, u32 api_domain);
		int (*set_rate)(struct gk20a *g, u32 api_domain, unsigned long rate);
		unsigned long (*get_fmax_at_vmin_safe)(struct gk20a *g);
		u32 (*get_ref_clock_rate)(struct gk20a *g);
		int (*predict_mv_at_hz_cur_tfloor)(struct clk_gk20a *clk,
			unsigned long rate);
		unsigned long (*get_maxrate)(struct gk20a *g, u32 api_domain);
		int (*prepare_enable)(struct clk_gk20a *clk);
		void (*disable_unprepare)(struct clk_gk20a *clk);
		int (*get_voltage)(struct clk_gk20a *clk, u64 *val);
		int (*get_gpcclk_clock_counter)(struct clk_gk20a *clk, u64 *val);
		int (*pll_reg_write)(struct gk20a *g, u32 reg, u32 val);
		int (*get_pll_debug_data)(struct gk20a *g,
				struct nvgpu_clk_pll_debug_data *d);
		int (*mclk_init)(struct gk20a *g);
		void (*mclk_deinit)(struct gk20a *g);
		int (*mclk_change)(struct gk20a *g, u16 val);
		void (*get_change_seq_time)(struct gk20a *g, s64 *change_time);
		void (*change_host_clk_source)(struct gk20a *g);
		bool split_rail_support;
		bool support_pmgr_domain;
		bool support_lpwr_pg;
		int (*perf_pmu_vfe_load)(struct gk20a *g);
		bool support_vf_point;
		u8 lut_num_entries;
		bool (*clk_mon_check_master_fault_status)(struct gk20a *g);
		int (*clk_mon_check_status)(struct gk20a *g,
			struct clk_domains_mon_status_params *clk_mon_status,
			u32 domain_mask);
		u32 (*clk_mon_init_domains)(struct gk20a *g);
#ifdef CONFIG_NVGPU_DGPU
		bool (*clk_mon_check_clk_good)(struct gk20a *g);
		bool (*clk_mon_check_pll_lock)(struct gk20a *g);
#endif
	} clk;
#ifdef CONFIG_NVGPU_CLK_ARB
	struct {
		int (*clk_arb_init_arbiter)(struct gk20a *g);
		int (*arbiter_clk_init)(struct gk20a *g);
		bool (*check_clk_arb_support)(struct gk20a *g);
		u32 (*get_arbiter_clk_domains)(struct gk20a *g);
		int (*get_arbiter_f_points)(struct gk20a *g, u32 api_domain,
				u32 *num_points, u16 *freqs_in_mhz);
		int (*get_arbiter_clk_range)(struct gk20a *g, u32 api_domain,
				u16 *min_mhz, u16 *max_mhz);
		int (*get_arbiter_clk_default)(struct gk20a *g, u32 api_domain,
				u16 *default_mhz);
		void (*clk_arb_run_arbiter_cb)(struct nvgpu_clk_arb *arb);
		/* This function is inherently unsafe to call while
		 *  arbiter is running arbiter must be blocked
		 *  before calling this function */
		u32 (*get_current_pstate)(struct gk20a *g);
		void (*clk_arb_cleanup)(struct nvgpu_clk_arb *arb);
		void (*stop_clk_arb_threads)(struct gk20a *g);
	} clk_arb;
#endif
	struct {
		int (*handle_pmu_perf_event)(struct gk20a *g, void *pmu_msg);
	} pmu_perf;
#ifdef CONFIG_NVGPU_DEBUGGER
	struct {
		int (*exec_regops)(struct gk20a *g,
			    struct nvgpu_channel *ch,
			    struct nvgpu_dbg_reg_op *ops,
			    u32 num_ops,
			    bool is_profiler,
			    bool *is_current_ctx);
		const struct regop_offset_range* (
				*get_global_whitelist_ranges)(void);
		u64 (*get_global_whitelist_ranges_count)(void);
		const struct regop_offset_range* (
				*get_context_whitelist_ranges)(void);
		u64 (*get_context_whitelist_ranges_count)(void);
		const u32* (*get_runcontrol_whitelist)(void);
		u64 (*get_runcontrol_whitelist_count)(void);
		const u32* (*get_qctl_whitelist)(void);
		u64 (*get_qctl_whitelist_count)(void);
	} regops;
#endif
	struct gops_mc mc;

	struct {
		void (*show_dump)(struct gk20a *g,
				struct nvgpu_debug_context *o);
	} debug;
#ifdef CONFIG_NVGPU_DEBUGGER
	struct {
		void (*post_events)(struct nvgpu_channel *ch);
		int (*dbg_set_powergate)(struct dbg_session_gk20a *dbg_s,
					bool disable_powergate);
		bool (*check_and_set_global_reservation)(
				struct dbg_session_gk20a *dbg_s,
				struct dbg_profiler_object_data *prof_obj);
		bool (*check_and_set_context_reservation)(
				struct dbg_session_gk20a *dbg_s,
				struct dbg_profiler_object_data *prof_obj);
		void (*release_profiler_reservation)(
				struct dbg_session_gk20a *dbg_s,
				struct dbg_profiler_object_data *prof_obj);
	} debugger;
	struct {
		void (*enable_membuf)(struct gk20a *g, u32 size,
			u64 buf_addr, struct nvgpu_mem *inst_block);
		void (*disable_membuf)(struct gk20a *g);
		void (*membuf_reset_streaming)(struct gk20a *g);
		u32 (*get_membuf_pending_bytes)(struct gk20a *g);
		void (*set_membuf_handled_bytes)(struct gk20a *g,
			u32 entries, u32 entry_size);
		bool (*get_membuf_overflow_status)(struct gk20a *g);
		u32 (*get_pmm_per_chiplet_offset)(void);
	} perf;
	struct {
		int (*perfbuf_enable)(struct gk20a *g, u64 offset, u32 size);
		int (*perfbuf_disable)(struct gk20a *g);
	} perfbuf;
#endif

	u32 (*get_litter_value)(struct gk20a *g, int value);
	int (*chip_init_gpu_characteristics)(struct gk20a *g);

	struct gops_bus bus;

	struct gops_ptimer ptimer;

	struct {
		int (*bios_sw_init)(struct gk20a *g);
		void (*bios_sw_deinit)(struct gk20a *g,
					struct nvgpu_bios *bios);
		u32 (*get_aon_secure_scratch_reg)(struct gk20a *g, u32 i);
		bool (*wait_for_bios_init_done)(struct gk20a *g);
	} bios;

#if defined(CONFIG_NVGPU_CYCLESTATS)
	struct {
		int (*enable_snapshot)(struct nvgpu_channel *ch,
				struct gk20a_cs_snapshot_client *client);
		void (*disable_snapshot)(struct gk20a *g);
		int (*check_data_available)(struct nvgpu_channel *ch,
						u32 *pending,
						bool *hw_overflow);
		void (*set_handled_snapshots)(struct gk20a *g, u32 num);
		u32 (*allocate_perfmon_ids)(struct gk20a_cs_snapshot *data,
				       u32 count);
		u32 (*release_perfmon_ids)(struct gk20a_cs_snapshot *data,
				      u32 start,
				      u32 count);
		int (*detach_snapshot)(struct nvgpu_channel *ch,
				struct gk20a_cs_snapshot_client *client);
		bool (*get_overflow_status)(struct gk20a *g);
		u32 (*get_pending_snapshots)(struct gk20a *g);
		u32 (*get_max_buffer_size)(struct gk20a *g);
	} css;
#endif
#ifdef CONFIG_NVGPU_DGPU
	struct {
		int (*get_speed)(struct gk20a *g, u32 *xve_link_speed);
		int (*set_speed)(struct gk20a *g, u32 xve_link_speed);
		void (*available_speeds)(struct gk20a *g, u32 *speed_mask);
		u32 (*xve_readl)(struct gk20a *g, u32 reg);
		void (*xve_writel)(struct gk20a *g, u32 reg, u32 val);
		void (*disable_aspm)(struct gk20a *g);
		void (*reset_gpu)(struct gk20a *g);
#if defined(CONFIG_PCI_MSI)
		void (*rearm_msi)(struct gk20a *g);
#endif
		void (*enable_shadow_rom)(struct gk20a *g);
		void (*disable_shadow_rom)(struct gk20a *g);
		u32 (*get_link_control_status)(struct gk20a *g);
		void (*devinit_deferred_settings)(struct gk20a *g);
	} xve;
#endif
	struct gops_falcon falcon;
	struct {
		int (*fbp_init_support)(struct gk20a *g);
	} fbp;
	struct gops_priv_ring priv_ring;
	struct {
		int (*init)(struct gk20a *g);
		u32 (*get_link_reset_mask)(struct gk20a *g);
		int (*discover_link)(struct gk20a *g);
		int (*rxdet)(struct gk20a *g, u32 link_id);
		void (*get_connected_link_mask)(u32 *link_mask);
		void (*set_sw_war)(struct gk20a *g, u32 link_id);
		int (*configure_ac_coupling)(struct gk20a *g,
				unsigned long mask, bool sync);
		void (*prog_alt_clk)(struct gk20a *g);
		void (*clear_link_reset)(struct gk20a *g, u32 link_id);
		void (*enable_link_an0)(struct gk20a *g, u32 link_id);
		/* API */
		struct {
			int (*setup_pll)(struct gk20a *g,
					unsigned long link_mask);
			int (*data_ready_en)(struct gk20a *g,
					unsigned long link_mask, bool sync);
			u32 (*get_link_state)(struct gk20a *g, u32 link_id);
			enum nvgpu_nvlink_link_mode (*get_link_mode)(
					struct gk20a *g,
					u32 link_id);
			int (*set_link_mode)(struct gk20a *g, u32 link_id,
					enum nvgpu_nvlink_link_mode mode);
			u32 (*get_rx_sublink_state)(struct gk20a *g,
					u32 link_id);
			u32 (*get_tx_sublink_state)(struct gk20a *g,
					u32 link_id);
			enum nvgpu_nvlink_sublink_mode (*get_sublink_mode)(
					struct gk20a *g, u32 link_id,
					bool is_rx_sublink);
			int (*set_sublink_mode)(struct gk20a *g, u32 link_id,
					bool is_rx_sublink,
					enum nvgpu_nvlink_sublink_mode mode);
		} link_mode_transitions;
		int (*reg_init)(struct gk20a *g);
		struct {
			u32 (*base_addr)(struct gk20a *g);
			bool (*is_running)(struct gk20a *g);
			int (*is_boot_complete)(struct gk20a *g,
						bool *boot_cmplte);
			u32 (*get_dlcmd_ordinal)(struct gk20a *g,
					enum nvgpu_nvlink_minion_dlcmd dlcmd);
			int (*send_dlcmd)(struct gk20a *g, u32 link_id,
					enum nvgpu_nvlink_minion_dlcmd dlcmd,
					bool sync);
			void (*clear_intr)(struct gk20a *g);
			void (*init_intr)(struct gk20a *g);
			void (*enable_link_intr)(struct gk20a *g, u32 link_id,
								bool enable);
			void (*falcon_isr)(struct gk20a *g);
			void (*isr)(struct gk20a *g);
			bool (*is_debug_mode)(struct gk20a *g);
		} minion;
		struct {
			void (*init_link_err_intr)(struct gk20a *g, u32 link_id);
			void (*enable_link_err_intr)(struct gk20a *g,
						u32 link_id, bool enable);
			void (*isr)(struct gk20a *g);
		} intr;
	} nvlink;
	struct gops_top top;
	struct {
		int (*init_sec2_setup_sw)(struct gk20a *g);
		int (*init_sec2_support)(struct gk20a *g);
		int (*sec2_destroy)(struct gk20a *g);
		void (*secured_sec2_start)(struct gk20a *g);
		void (*enable_irq)(struct nvgpu_sec2 *sec2, bool enable);
		bool (*is_interrupted)(struct nvgpu_sec2 *sec2);
		u32 (*get_intr)(struct gk20a *g);
		bool (*msg_intr_received)(struct gk20a *g);
		void (*set_msg_intr)(struct gk20a *g);
		void (*clr_intr)(struct gk20a *g, u32 intr);
		void (*process_intr)(struct gk20a *g, struct nvgpu_sec2 *sec2);
		void (*msgq_tail)(struct gk20a *g, struct nvgpu_sec2 *sec2,
			u32 *tail, bool set);
		u32 (*falcon_base_addr)(void);
		int (*sec2_reset)(struct gk20a *g);
		int (*sec2_copy_to_emem)(struct gk20a *g, u32 dst,
					 u8 *src, u32 size, u8 port);
		int (*sec2_copy_from_emem)(struct gk20a *g,
					   u32 src, u8 *dst, u32 size, u8 port);
		int (*sec2_queue_head)(struct gk20a *g,
				       u32 queue_id, u32 queue_index,
				       u32 *head, bool set);
		int (*sec2_queue_tail)(struct gk20a *g,
				       u32 queue_id, u32 queue_index,
				       u32 *tail, bool set);
		void (*flcn_setup_boot_config)(struct gk20a *g);
	} sec2;
	struct {
		u32 (*falcon_base_addr)(void);
		void (*falcon_setup_boot_config)(struct gk20a *g);
		int (*gsp_reset)(struct gk20a *g);
	} gsp;
#ifdef CONFIG_NVGPU_TPC_POWERGATE
	struct {
		int (*init_tpc_powergate)(struct gk20a *g, u32 fuse_status);
		void (*tpc_gr_pg)(struct gk20a *g);
	} tpc;
#endif
	void (*semaphore_wakeup)(struct gk20a *g, bool post_events);
};

/**
 * @brief HW version info read from the HW.
 */
struct nvgpu_gpu_params {
	/** GPU architecture ID */
	u32 gpu_arch;
	/** GPU implementation ID */
	u32 gpu_impl;
	/** GPU revision ID */
	u32 gpu_rev;
	/** sm version */
	u32 sm_arch_sm_version;
	/** sm instruction set */
	u32 sm_arch_spa_version;
	u32 sm_arch_warp_count;
};

/**
 * @brief The GPU superstructure.
 *
 * This structure describes the GPU. There is a unique \a gk20a struct for each
 * GPU in the system. This structure includes many state variables used
 * throughout the driver. It also contains the #gpu_ops HALs.
 *
 * Whenever possible, units should keep their data within their own sub-struct
 * and not in the main gk20a struct.
 */
struct gk20a {
	/**
	 * @brief Free data in the struct allocated during its creation.
	 *
	 * @param g [in]	The GPU superstructure
	 *
	 * This does not free all of the memory in the structure as many of the
	 * units allocate private data, and those units are responsible for
	 * freeing that data. \a gfree should be called after all of the units
	 * have had the opportunity to free their private data.
	 */
	void (*gfree)(struct gk20a *g);
	struct nvgpu_nvhost_dev *nvhost;

	/**
	 * Used by <nvgpu/enabled.h>. Do not access directly!
	 */
	unsigned long *enabled_flags;

	/** Used by Linux module to keep track of driver usage */
	nvgpu_atomic_t usage_count;

	/** Used by common.init unit to track users of the driver */
	struct nvgpu_ref refcount;

	/** Name of the gpu. */
	const char *name;

	/** Is the GPU ready to be used? */
	u32 power_on_state;

#ifdef CONFIG_NVGPU_DGPU
	bool gpu_reset_done;
#endif
#ifdef CONFIG_PM
	bool suspended;
#endif
	bool sw_ready;

#ifndef CONFIG_NVGPU_RECOVERY
	bool sw_quiesce_init_done;
	bool sw_quiesce_pending;
	struct nvgpu_cond sw_quiesce_cond;
	struct nvgpu_thread sw_quiesce_thread;
	struct nvgpu_bug_cb sw_quiesce_bug_cb;
#endif
	struct nvgpu_list_node bug_node;

	/** Controls which messages are logged */
	u64 log_mask;
	u32 log_trace;

	struct nvgpu_mutex tpc_pg_lock;

	/** Stored HW version info */
	struct nvgpu_gpu_params params;

#ifdef CONFIG_NVGPU_DETERMINISTIC_CHANNELS
	/**
	 * Guards access to hardware when usual gk20a_{busy,idle} are skipped
	 * for submits and held for channel lifetime but dropped for an ongoing
	 * gk20a_do_idle().
	 */
	struct nvgpu_rwsem deterministic_busy;
#endif

	struct nvgpu_netlist_vars *netlist_vars;
	bool netlist_valid;

	struct nvgpu_falcon pmu_flcn;
	struct nvgpu_falcon fecs_flcn;
	struct nvgpu_falcon gpccs_flcn;
	struct nvgpu_falcon nvdec_flcn;
	struct nvgpu_falcon minion_flcn;
	struct nvgpu_falcon gsp_flcn;
	struct clk_gk20a clk;
	struct nvgpu_fifo fifo;
	struct nvgpu_nvlink_dev nvlink;
	struct nvgpu_gr *gr;
	struct nvgpu_fbp *fbp;
#ifdef CONFIG_NVGPU_SIM
	struct sim_nvgpu *sim;
#endif
	struct mm_gk20a mm;
	struct nvgpu_pmu *pmu;
	struct nvgpu_acr *acr;
	struct nvgpu_ecc ecc;
	struct pmgr_pmupstate *pmgr_pmu;
	struct nvgpu_sec2 sec2;
#ifdef CONFIG_NVGPU_CHANNEL_TSG_SCHEDULING
	struct nvgpu_sched_ctrl sched_ctrl;
#endif

#ifdef CONFIG_DEBUG_FS
	struct railgate_stats pstats;
#endif
	/** Global default timeout for use throughout driver */
	u32 poll_timeout_default;
	/** User disabled timeouts */
	bool timeouts_disabled_by_user;

	unsigned int ch_wdt_init_limit_ms;
	u32 ctxsw_timeout_period_ms;
	u32 ctxsw_wdt_period_us;

	struct nvgpu_mutex power_lock;

	struct nvgpu_spinlock power_spinlock;

	/** Channel priorities */
	u32 tsg_timeslice_low_priority_us;
	u32 tsg_timeslice_medium_priority_us;
	u32 tsg_timeslice_high_priority_us;
	u32 tsg_timeslice_min_us;
	u32 tsg_timeslice_max_us;
	bool runlist_interleave;

	/** Lock serializing CG an PG programming for various units */
	struct nvgpu_mutex cg_pg_lock;
	/** SLCG setting read from the platform data */
	bool slcg_enabled;
	/** BLCG setting read from the platform data */
	bool blcg_enabled;
	/** ELCG setting read from the platform data */
	bool elcg_enabled;
	bool elpg_enabled;
	bool aelpg_enabled;
	bool can_elpg;
	bool mscg_enabled;
	bool forced_idle;
	bool forced_reset;
	bool allow_all;

	u32 ptimer_src_freq;

	/** @cond DOXYGEN_SHOULD_SKIP_THIS */
	int railgate_delay;
	/** @endcond */
	u8 ldiv_slowdown_factor;
	unsigned int aggressive_sync_destroy_thresh;
	bool aggressive_sync_destroy;

	/** Is LS PMU supported? */
	bool support_ls_pmu;

	/** Is this a virtual GPU? */
	bool is_virtual;

	bool has_cde;

	/** @cond DOXYGEN_SHOULD_SKIP_THIS */
	u32 emc3d_ratio;
	/** @endcond */

	/**
	 * A group of semaphore pools. One for each channel.
	 */
	struct nvgpu_semaphore_sea *sema_sea;

#ifdef CONFIG_NVGPU_DEBUGGER
	/* held while manipulating # of debug/profiler sessions present */
	/* also prevents debug sessions from attaching until released */
	struct nvgpu_mutex dbg_sessions_lock;
	int dbg_powergating_disabled_refcount; /*refcount for pg disable */
	/*refcount for timeout disable */
	nvgpu_atomic_t timeouts_disabled_refcount;

	/* must have dbg_sessions_lock before use */
	struct nvgpu_dbg_reg_op *dbg_regops_tmp_buf;
	u32 dbg_regops_tmp_buf_ops;

	/* For perfbuf mapping */
	struct {
		struct dbg_session_gk20a *owner;
		u64 offset;
	} perfbuf;

	/* For profiler reservations */
	struct nvgpu_list_node profiler_objects;
	bool global_profiler_reservation_held;
	int profiler_reservation_count;

	bool mmu_debug_ctrl;
	u32 mmu_debug_mode_refcnt;
#endif /* CONFIG_NVGPU_DEBUGGER */

#ifdef CONFIG_NVGPU_FECS_TRACE
	struct gk20a_ctxsw_trace *ctxsw_trace;
	struct nvgpu_gr_fecs_trace *fecs_trace;
#endif

#ifdef CONFIG_NVGPU_CYCLESTATS
	struct nvgpu_mutex		cs_lock;
	struct gk20a_cs_snapshot	*cs_data;
#endif

	/** @cond DOXYGEN_SHOULD_SKIP_THIS */
	/* Called after all references to driver are gone. Unused in safety */
	void (*remove_support)(struct gk20a *g);
	/** @endcond */

	u64 pg_ingating_time_us;
	u64 pg_ungating_time_us;
	u32 pg_gating_cnt;

	struct gk20a_as as;

	struct nvgpu_mutex client_lock;
	int client_refcount; /* open channels and ctrl nodes */

	/** The HAL function pointers */
	struct gpu_ops ops;

	/*used for change of enum zbc update cmd id from ver 0 to ver1*/
	u8 pmu_ver_cmd_id_zbc_table_update;

	struct nvgpu_mc mc;

	/*
	 * The deductible memory size for max_comptag_mem (in MBytes)
	 * Usually close to memory size that running system is taking
	*/
	u32 comptag_mem_deduct;

	u32 max_comptag_mem; /* max memory size (MB) for comptag */

	u32 ltc_streamid;

	struct nvgpu_cbc *cbc;
	struct nvgpu_ltc *ltc;

	struct nvgpu_channel_worker {
		struct nvgpu_worker worker;

#ifdef CONFIG_NVGPU_CHANNEL_WDT
		u32 watchdog_interval;
		struct nvgpu_timeout timeout;
#endif
	} channel_worker;

	/** @cond DOXYGEN_SHOULD_SKIP_THIS */
	struct nvgpu_clk_arb_worker {
		struct nvgpu_worker worker;
	} clk_arb_worker;
	/** @endcond */

	struct {
		void (*open)(struct nvgpu_channel *ch);
		void (*close)(struct nvgpu_channel *ch, bool force);
		void (*work_completion_signal)(struct nvgpu_channel *ch);
		void (*work_completion_cancel_sync)(struct nvgpu_channel *ch);
		bool (*os_fence_framework_inst_exists)(struct nvgpu_channel *ch);
		int (*init_os_fence_framework)(
			struct nvgpu_channel *ch, const char *fmt, ...);
		void (*signal_os_fence_framework)(struct nvgpu_channel *ch);
		void (*destroy_os_fence_framework)(struct nvgpu_channel *ch);
		int (*copy_user_gpfifo)(struct nvgpu_gpfifo_entry *dest,
				struct nvgpu_gpfifo_userdata userdata,
				u32 start, u32 length);
		int (*alloc_usermode_buffers)(struct nvgpu_channel *c,
			struct nvgpu_setup_bind_args *args);
		void (*free_usermode_buffers)(struct nvgpu_channel *c);
	} os_channel;

	/** @cond DOXYGEN_SHOULD_SKIP_THIS */
	/* Used by Linux OS Layer */
	struct gk20a_scale_profile *scale_profile;
	unsigned long last_freq;
	/** @endcond */

#ifdef CONFIG_NVGPU_NON_FUSA
	u32 tpc_fs_mask_user;
#endif

	u32 tpc_pg_mask;
	bool can_tpc_powergate;

	/** @cond DOXYGEN_SHOULD_SKIP_THIS */
	u32 valid_tpc_mask[MAX_TPC_PG_CONFIGS];

	struct nvgpu_bios *bios;
	bool bios_is_init;

	struct nvgpu_clk_arb *clk_arb;

	struct nvgpu_mutex clk_arb_enable_lock;

	nvgpu_atomic_t clk_arb_global_nr;

	struct nvgpu_ce_app *ce_app;
	/** @endcond */

	bool ltc_intr_en_illegal_compstat;

	/** Are we currently running on a FUSA device configuration? */
	bool is_fusa_sku;

	/** @cond DOXYGEN_SHOULD_SKIP_THIS */
	/* PCI device identifier */
	u16 pci_vendor_id, pci_device_id;
	u16 pci_subsystem_vendor_id, pci_subsystem_device_id;
	u16 pci_class;
	u8 pci_revision;

	/*
	 * PCI power management: i2c device index, port and address for
	 * INA3221.
	 */
	u32 ina3221_dcb_index;
	u32 ina3221_i2c_address;
	u32 ina3221_i2c_port;
	bool hardcode_sw_threshold;

	/* PCIe power states. */
	bool xve_l0s;
	bool xve_l1;

	/* Current warning temp in sfxp24.8 */
	s32 curr_warn_temp;

#if defined(CONFIG_PCI_MSI)
	/* Check if msi is enabled */
	bool msi_enabled;
#endif
#ifdef CONFIG_NVGPU_TRACK_MEM_USAGE
	struct nvgpu_mem_alloc_tracker *vmallocs;
	struct nvgpu_mem_alloc_tracker *kmallocs;
#endif

	/* memory training sequence and mclk switch scripts */
	u32 mem_config_idx;

	u64 dma_memory_used;
	/** @endcond */

#if defined(CONFIG_TEGRA_GK20A_NVHOST)
	u64		syncpt_unit_base;
	size_t		syncpt_unit_size;
	u32		syncpt_size;
#endif
	struct nvgpu_mem syncpt_mem;

	/** @cond DOXYGEN_SHOULD_SKIP_THIS */
	struct nvgpu_list_node boardobj_head;
	struct nvgpu_list_node boardobjgrp_head;

	struct nvgpu_mem pdb_cache_war_mem;
	/** @endcond */

#ifdef CONFIG_NVGPU_DGPU
	u16 dgpu_max_clk;
#endif

	/** Max SM diversity configuration count. */
	u32 max_sm_diversity_config_count;

};

/**
 * @brief Check if watchdog and context switch timeouts are enabled.
 *
 * @param g [in]	The GPU superstucture.
 *
 * @return True if these timeouts are enabled, false otherwise.
 */
static inline bool nvgpu_is_timeouts_enabled(struct gk20a *g)
{
#ifdef CONFIG_NVGPU_DEBUGGER
	return nvgpu_atomic_read(&g->timeouts_disabled_refcount) == 0;
#else
	return true;
#endif
}

/** Minimum poll delay value in us */
#define POLL_DELAY_MIN_US	10U
/** Maximum poll delay value in us */
#define POLL_DELAY_MAX_US	200U

/**
 * @brief Get the global poll timeout value
 *
 * @param g [in]	The GPU superstucture.
 *
 * @return The value of the global poll timeout value in us.
 */
static inline u32 nvgpu_get_poll_timeout(struct gk20a *g)
{
	return nvgpu_is_timeouts_enabled(g) ?
		g->poll_timeout_default : U32_MAX;
}

/** IO Resource in the device tree for BAR0 */
#define GK20A_BAR0_IORESOURCE_MEM	0U
/** IO Resource in the device tree for BAR1 */
#define GK20A_BAR1_IORESOURCE_MEM	1U
/** IO Resource in the device tree for SIM mem */
#define GK20A_SIM_IORESOURCE_MEM	2U

#ifdef CONFIG_PM
int gk20a_do_idle_impl(struct gk20a *g, bool force_reset);
int gk20a_do_unidle_impl(struct gk20a *g);
#endif

/**
 * Constructs unique and compact GPUID from nvgpu_gpu_characteristics
 * arch/impl fields.
 */
#define GK20A_GPUID(arch, impl) ((u32) ((arch) | (impl)))

/** gk20a HW version */
#define GK20A_GPUID_GK20A   0x000000EAU
/** gm20b HW version */
#define GK20A_GPUID_GM20B   0x0000012BU
/** gm20b.b HW version */
#define GK20A_GPUID_GM20B_B 0x0000012EU
/** gp10b HW version */
#define NVGPU_GPUID_GP10B   0x0000013BU
/** gv11b HW version */
#define NVGPU_GPUID_GV11B   0x0000015BU
/** gv100 HW version */
#define NVGPU_GPUID_GV100   0x00000140U
/** tu104 HW version */
#define NVGPU_GPUID_TU104   0x00000164U

/**
 * @}
 */

#endif /* GK20A_H */
