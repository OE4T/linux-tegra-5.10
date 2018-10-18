/*
 * general p state infrastructure
 *
 * Copyright (c) 2016-2018, NVIDIA CORPORATION.  All rights reserved.
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
#ifndef NVGPU_CTRLPERF_H
#define NVGPU_CTRLPERF_H

#include "ctrlvolt.h"
#include "ctrlclk.h"

struct ctrl_perf_volt_rail_list_item {
	u8 volt_domain;
	u32 voltage_uv;
	u32 voltage_min_noise_unaware_uv;
};

struct ctrl_perf_volt_rail_list {
	u8    num_rails;
	struct ctrl_perf_volt_rail_list_item
		rails[CTRL_VOLT_VOLT_RAIL_MAX_RAILS];
};

union ctrl_perf_vfe_var_single_sensed_fuse_value_data {
	int signed_value;
	u32 unsigned_value;
};

struct ctrl_perf_vfe_var_single_sensed_fuse_value {
	bool b_signed;
	union ctrl_perf_vfe_var_single_sensed_fuse_value_data data;
};

struct ctrl_bios_vfield_register_segment_super {
	u8 low_bit;
	u8 high_bit;
};

struct ctrl_bios_vfield_register_segment_reg {
	struct ctrl_bios_vfield_register_segment_super super;
	u32 addr;
};

struct ctrl_bios_vfield_register_segment_index_reg {
	struct ctrl_bios_vfield_register_segment_super super;
	u32 addr;
	u32 reg_index;
	u32 index;
};

union ctrl_bios_vfield_register_segment_data {
	struct ctrl_bios_vfield_register_segment_reg reg;
	struct ctrl_bios_vfield_register_segment_index_reg index_reg;
};

struct ctrl_bios_vfield_register_segment {
	u8 type;
	union ctrl_bios_vfield_register_segment_data data;
};

#define NV_PMU_VFE_VAR_SINGLE_SENSED_FUSE_SEGMENTS_MAX                        1U

struct ctrl_perf_vfe_var_single_sensed_fuse_info {
	u8 segment_count;
	struct ctrl_bios_vfield_register_segment segments[NV_PMU_VFE_VAR_SINGLE_SENSED_FUSE_SEGMENTS_MAX];
};

struct ctrl_perf_vfe_var_single_sensed_fuse_override_info {
	u32 fuse_val_override;
        u8 b_fuse_regkey_override;
};

struct ctrl_perf_vfe_var_single_sensed_fuse_vfield_info {
	struct ctrl_perf_vfe_var_single_sensed_fuse_info fuse;
	u32 fuse_val_default;
	u32 hw_correction_scale;
	int hw_correction_offset;
	u8 v_field_id;
};

struct ctrl_perf_vfe_var_single_sensed_fuse_ver_vfield_info {
	struct ctrl_perf_vfe_var_single_sensed_fuse_info fuse;
	u8 ver_expected;
	bool b_ver_check;
	bool b_use_default_on_ver_check_fail;
	u8 v_field_id_ver;
};

/*-----------------------------  CHANGES_SEQ --------------------------------*/

/*!
 * Enumeration of the PERF CHANGE_SEQ feature version.
 *
 * _2X  - Legacy implementation of CHANGE_SEQ used in pstates 3.0 and earlier.
 * _PMU - Represents PMU based perf change sequence class and its sub-classes.
 * _31  - CHANGE_SEQ implementation used with pstates 3.1 and later.
 * _35  - CHANGE_SEQ implementation used with pstates 3.5 and later.
 */
#define CTRL_PERF_CHANGE_SEQ_VERSION_UNKNOWN	0xFF
#define CTRL_PERF_CHANGE_SEQ_VERSION_2X			0x01
#define CTRL_PERF_CHANGE_SEQ_VERSION_PMU		0x02
#define CTRL_PERF_CHANGE_SEQ_VERSION_31			0x03
#define CTRL_PERF_CHANGE_SEQ_VERSION_35			0x04

/*!
 * Flags to provide information about the input perf change request.
 * This flags will be used to understand the type of perf change req.
 */
#define CTRL_PERF_CHANGE_SEQ_CHANGE_NONE				0x00
#define CTRL_PERF_CHANGE_SEQ_CHANGE_FORCE				BIT(0)
#define CTRL_PERF_CHANGE_SEQ_CHANGE_FORCE_CLOCKS		BIT(1)
#define CTRL_PERF_CHANGE_SEQ_CHANGE_ASYNC				BIT(2)
#define CTRL_PERF_CHANGE_SEQ_CHANGE_SKIP_VBLANK_WAIT	BIT(3)

#define CTRL_PERF_CHANGE_SEQ_SYNC_CHANGE_QUEUE_SIZE 0x04
#define CTRL_PERF_CHANGE_SEQ_SCRIPT_MAX_PROFILING_THREADS 8

enum ctrl_perf_change_seq_sync_change_client {
	CTRL_PERF_CHANGE_SEQ_SYNC_CHANGE_CLIENT_INVALID = 0,
	CTRL_PERF_CHANGE_SEQ_SYNC_CHANGE_CLIENT_RM_NVGPU = 1,
	CTRL_PERF_CHANGE_SEQ_SYNC_CHANGE_CLIENT_PMU = 2,
};

struct ctrl_perf_chage_seq_change_pmu {
	u32 seq_id;
};

struct ctrl_perf_change_seq_change {
	struct ctrl_clk_clk_domain_list clk_list;
	struct ctrl_volt_volt_rail_list_v1 volt_list;
	u32 pstate_index;
	u32 flags;
	u32 vf_points_cache_counter;
	u8 version;
	struct ctrl_perf_chage_seq_change_pmu data;
};

struct ctrl_perf_chage_seq_input_clk {
	u32 clk_freq_khz;
};

struct ctrl_perf_chage_seq_input_volt {
	u32 voltage_uv;
	u32 voltage_min_noise_unaware_uv;
};

struct ctrl_perf_change_seq_change_input {
	u32 pstate_index;
	u32 flags;
	u32 vf_points_cache_counter;
	struct ctrl_boardobjgrp_mask_e32 clk_domains_mask;
	struct ctrl_perf_chage_seq_input_clk clk[CTRL_CLK_CLK_DOMAIN_CLIENT_MAX_DOMAINS];
	struct ctrl_boardobjgrp_mask_e32 volt_rails_mask;
	struct ctrl_perf_chage_seq_input_volt volt[CTRL_VOLT_VOLT_RAIL_CLIENT_MAX_RAILS];
};

struct u64_align32 {
	u32 lo;
	u32 hi;
};
struct ctrl_perf_change_seq_script_profiling_thread {
	u32 step_mask;
	struct u64_align32 timens;
};

struct ctrl_perf_change_seq_script_profiling {
	struct u64_align32 total_timens; /*align 32 */
	struct u64_align32 total_build_timens;
	struct u64_align32 total_execution_timens;
	u8 num_threads; /*number of threads required to process this script*/
	struct ctrl_perf_change_seq_script_profiling_thread
		nvgpu_threads[CTRL_PERF_CHANGE_SEQ_SCRIPT_MAX_PROFILING_THREADS];
};

struct ctrl_perf_change_seq_pmu_script_header {
	bool b_increase;
	u8 num_steps;
	u8 cur_step_index;
	struct ctrl_perf_change_seq_script_profiling profiling;
};

enum ctrl_perf_change_seq_pmu_step_id {
	CTRL_PERF_CHANGE_SEQ_PMU_STEP_ID_NONE,
	CTRL_PERF_CHANGE_SEQ_PMU_STEP_ID_PRE_CHANGE_RM,
	CTRL_PERF_CHANGE_SEQ_PMU_STEP_ID_PRE_CHANGE_PMU,
	CTRL_PERF_CHANGE_SEQ_PMU_STEP_ID_POST_CHANGE_RM,
	CTRL_PERF_CHANGE_SEQ_PMU_STEP_ID_POST_CHANGE_PMU,
	CTRL_PERF_CHANGE_SEQ_PMU_STEP_ID_PRE_PSTATE_RM,
	CTRL_PERF_CHANGE_SEQ_PMU_STEP_ID_PRE_PSTATE_PMU,
	CTRL_PERF_CHANGE_SEQ_PMU_STEP_ID_POST_PSTATE_RM,
	CTRL_PERF_CHANGE_SEQ_PMU_STEP_ID_POST_PSTATE_PMU,
	CTRL_PERF_CHANGE_SEQ_PMU_STEP_ID_VOLT,
	CTRL_PERF_CHANGE_SEQ_PMU_STEP_ID_LPWR,
	CTRL_PERF_CHANGE_SEQ_PMU_STEP_ID_BIF,
	CTRL_PERF_CHANGE_SEQ_31_STEP_ID_NOISE_UNAWARE_CLKS,
	CTRL_PERF_CHANGE_SEQ_31_STEP_ID_NOISE_AWARE_CLKS,
	CTRL_PERF_CHANGE_SEQ_35_STEP_ID_PRE_VOLT_CLKS,
	CTRL_PERF_CHANGE_SEQ_35_STEP_ID_POST_VOLT_CLKS,
	CTRL_PERF_CHANGE_SEQ_PMU_STEP_ID_MAX_STEPS,
};

struct ctrl_perf_change_seq_step_profiling {
	/*all aligned to 32 */
	u64 total_timens;
	u64 nv_thread_timens;
	u64 pmu_thread_timens;
};

struct ctrl_perf_change_seq_pmu_script_step_super {
	enum ctrl_perf_change_seq_pmu_step_id step_id;
	struct ctrl_perf_change_seq_step_profiling profiling;
};

struct ctrl_perf_change_seq_pmu_script_step_change {
	struct ctrl_perf_change_seq_pmu_script_step_super super;
	u32 pstate_index;
};

struct ctrl_perf_change_seq_pmu_script_step_pstate {
	struct ctrl_perf_change_seq_pmu_script_step_super super;
	u32 pstate_index;
};

struct ctrl_perf_change_seq_pmu_script_step_lpwr {
	struct ctrl_perf_change_seq_pmu_script_step_super super;
	u32 pstate_index;
};

struct ctrl_perf_change_seq_pmu_script_step_bif {
	struct ctrl_perf_change_seq_pmu_script_step_super super;
	u32 pstate_index;
	u8 pcie_idx;
	u8 nvlink_idx;
};

struct ctrl_perf_change_seq_pmu_script_step_clks {
	struct ctrl_perf_change_seq_pmu_script_step_super super;
	struct ctrl_volt_volt_rail_list_v1 volt_list;
	struct ctrl_clk_clk_domain_list clk_list;
};

struct ctrl_perf_change_seq_pmu_script_step_volt {
	struct ctrl_perf_change_seq_pmu_script_step_super super;
	struct ctrl_volt_volt_rail_list_v1 volt_list;
};

union ctrl_perf_change_seq_pmu_script_step_data {
	struct ctrl_perf_change_seq_pmu_script_step_super super;
	struct ctrl_perf_change_seq_pmu_script_step_change change;
	struct ctrl_perf_change_seq_pmu_script_step_pstate pstate;
	struct ctrl_perf_change_seq_pmu_script_step_lpwr lpwr;
	struct ctrl_perf_change_seq_pmu_script_step_bif bif;
	struct ctrl_perf_change_seq_pmu_script_step_clks clk;
	struct ctrl_perf_change_seq_pmu_script_step_volt volt;
};

#endif /* NVGPU_CTRLPERF_H */

