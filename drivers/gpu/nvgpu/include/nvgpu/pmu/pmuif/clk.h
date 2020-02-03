/*
 * Copyright (c) 2016-2020, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_PMUIF_CLK_H
#define NVGPU_PMUIF_CLK_H

#include <nvgpu/flcnif_cmn.h>
#include <nvgpu/pmu/volt.h>

#include "ctrlboardobj.h"
#include "ctrlclk.h"
#include "boardobj.h"

/*
 *  Try to get gpc2clk, mclk, sys2clk, xbar2clk work for Pascal
 *
 *  mclk is same for both
 *  gpc2clk is 17 for Pascal and 13 for Volta, making it 17
 *    as volta uses gpcclk
 *  sys2clk is 20 in Pascal and 15 in Volta.
 *    Changing for Pascal would break nvdclk of Volta
 *  xbar2clk is 19 in Pascal and 14 in Volta
 *    Changing for Pascal would break pwrclk of Volta
 */
#define CLKWHICH_GPCCLK		1U
#define CLKWHICH_XBARCLK	2U
#define CLKWHICH_SYSCLK		3U
#define CLKWHICH_HUBCLK		4U
#define CLKWHICH_MCLK		5U
#define CLKWHICH_HOSTCLK	6U
#define CLKWHICH_DISPCLK	7U
#define CLKWHICH_XCLK		12U
#define CLKWHICH_XBAR2CLK	14U
#define CLKWHICH_SYS2CLK	15U
#define CLKWHICH_HUB2CLK	16U
#define CLKWHICH_GPC2CLK	17U
#define CLKWHICH_PWRCLK		19U
#define CLKWHICH_NVDCLK		20U
#define CLKWHICH_PCIEGENCLK	26U

#define NV_PMU_RPC_ID_CLK_CNTR_SAMPLE_DOMAIN                               0x01U
#define NV_PMU_RPC_ID_CLK_CLK_DOMAIN_35_PROG_VOLT_TO_FREQ                  0x02U
#define NV_PMU_RPC_ID_CLK_CLK_DOMAIN_35_PROG_FREQ_TO_VOLT                  0x03U
#define NV_PMU_RPC_ID_CLK_CLK_DOMAIN_35_PROG_FREQ_QUANTIZE                 0x04U
#define NV_PMU_RPC_ID_CLK_CLK_DOMAIN_35_PROG_CLIENT_FREQ_DELTA_ADJ         0x05U
#define NV_PMU_RPC_ID_CLK_FREQ_EFFECTIVE_AVG                               0x06U
#define NV_PMU_RPC_ID_CLK_LOAD                                             0x07U
#define NV_PMU_RPC_ID_CLK_VF_CHANGE_INJECT                                 0x08U
#define NV_PMU_RPC_ID_CLK_MCLK_SWITCH                                      0x09U
#define NV_PMU_RPC_ID_CLK__COUNT                                           0x0AU

/*!
 * Macros for the @ref feature parameter in the @ref NV_PMU_CLK_LOAD structure
 */
#define NV_NV_PMU_CLK_LOAD_FEATURE_INVALID                         (0x00000000U)
#define NV_NV_PMU_CLK_LOAD_FEATURE_FLL                             (0x00000001U)
#define NV_NV_PMU_CLK_LOAD_FEATURE_VIN                             (0x00000002U)
#define NV_NV_PMU_CLK_LOAD_FEATURE_FREQ_CONTROLLER                 (0x00000003U)
#define NV_NV_PMU_CLK_LOAD_FEATURE_FREQ_EFFECTIVE_AVG              (0x00000004U)
#define NV_NV_PMU_CLK_LOAD_FEATURE_CLK_DOMAIN                      (0x00000005U)
#define NV_NV_PMU_CLK_LOAD_FEATURE_CLK_CONTROLLER                  (0x00000006U)

/*
 * CLK_DOMAIN BOARDOBJGRP Header structure.  Describes global state about the
 * CLK_DOMAIN feature.
 */
struct nv_pmu_clk_clk_domain_boardobjgrp_set_header {
	struct nv_pmu_boardobjgrp_e32 super;
	u32 vbios_domains;
	struct ctrl_boardobjgrp_mask_e32 prog_domains_mask;
	struct ctrl_boardobjgrp_mask_e32 master_domains_mask;
	struct ctrl_boardobjgrp_mask_e32 clkmon_domains_mask;
	u16 cntr_sampling_periodms;
	u16 clkmon_refwin_usec;
	u8 version;
	bool b_override_o_v_o_c;
	bool b_debug_mode;
	bool b_enforce_vf_monotonicity;
	bool b_enforce_vf_smoothening;
	u8 volt_rails_max;
	struct ctrl_clk_clk_delta deltas;
};

struct nv_pmu_clk_clk_domain_boardobj_set {
	struct nv_pmu_boardobj super;
	u32 domain;
	u32 api_domain;
	u8 perf_domain_grp_idx;
};

struct nv_pmu_clk_clk_domain_3x_boardobj_set {
	struct nv_pmu_clk_clk_domain_boardobj_set super;
	bool b_noise_aware_capable;
};

struct nv_pmu_clk_clk_domain_3x_fixed_boardobj_set {
	struct nv_pmu_clk_clk_domain_3x_boardobj_set super;
	u16 freq_mhz;
};

struct nv_pmu_clk_clk_domain_3x_prog_boardobj_set {
	struct nv_pmu_clk_clk_domain_3x_boardobj_set super;
	u8 clk_prog_idx_first;
	u8 clk_prog_idx_last;
	bool b_force_noise_unaware_ordering;
	struct ctrl_clk_freq_delta factory_delta;
	short freq_delta_min_mhz;
	short freq_delta_max_mhz;
	struct ctrl_clk_clk_delta deltas;
};

struct nv_pmu_clk_clk_domain_30_prog_boardobj_set {
	struct nv_pmu_clk_clk_domain_3x_prog_boardobj_set super;
	u8 noise_unaware_ordering_index;
	u8 noise_aware_ordering_index;
};

struct nv_pmu_clk_clk_domain_3x_master_boardobj_set {
	u8 rsvd;	/* Stubbing for RM_PMU_BOARDOBJ_INTERFACE */
	u32 slave_idxs_mask;
};

struct nv_pmu_clk_clk_domain_30_master_boardobj_set {
	struct nv_pmu_clk_clk_domain_30_prog_boardobj_set super;
	struct nv_pmu_clk_clk_domain_3x_master_boardobj_set master;
};

struct nv_pmu_clk_clk_domain_3x_slave_boardobj_set {
	u8 rsvd;	/* Stubbing for RM_PMU_BOARDOBJ_INTERFACE */
	u8 master_idx;
};

struct nv_pmu_clk_clk_domain_30_slave_boardobj_set {
	struct nv_pmu_clk_clk_domain_30_prog_boardobj_set super;
	struct nv_pmu_clk_clk_domain_3x_slave_boardobj_set slave;
};

struct nv_pmu_clk_clk_domain_35_prog_boardobj_set {
	struct nv_pmu_clk_clk_domain_3x_prog_boardobj_set super;
	u8 pre_volt_ordering_index;
	u8 post_volt_ordering_index;
	u8 clk_pos;
	u8 clk_vf_curve_count;
	struct ctrl_clk_domain_info_35_prog_clk_mon clkmon_info;
	struct ctrl_clk_domain_control_35_prog_clk_mon clkmon_ctrl;
	u32 por_volt_delta_uv[CTRL_VOLT_VOLT_RAIL_CLIENT_MAX_RAILS];
};

struct nv_pmu_clk_clk_domain_35_master_boardobj_set {
	struct nv_pmu_clk_clk_domain_35_prog_boardobj_set super;
	struct nv_pmu_clk_clk_domain_3x_master_boardobj_set master;
	struct ctrl_boardobjgrp_mask_e32 master_slave_domains_grp_mask;
};


struct nv_pmu_clk_clk_domain_35_slave_boardobj_set {
	struct nv_pmu_clk_clk_domain_35_prog_boardobj_set super;
	struct nv_pmu_clk_clk_domain_3x_slave_boardobj_set slave;
};

union nv_pmu_clk_clk_domain_boardobj_set_union {
	struct nv_pmu_boardobj board_obj;
	struct nv_pmu_clk_clk_domain_boardobj_set super;
	struct nv_pmu_clk_clk_domain_3x_boardobj_set v3x;
	struct nv_pmu_clk_clk_domain_3x_fixed_boardobj_set v3x_fixed;
	struct nv_pmu_clk_clk_domain_3x_prog_boardobj_set v3x_prog;
	struct nv_pmu_clk_clk_domain_30_prog_boardobj_set v30_prog;
	struct nv_pmu_clk_clk_domain_30_master_boardobj_set v30_master;
	struct nv_pmu_clk_clk_domain_30_slave_boardobj_set v30_slave;
	struct nv_pmu_clk_clk_domain_35_prog_boardobj_set v35_prog;
	struct nv_pmu_clk_clk_domain_35_master_boardobj_set v35_master;
	struct nv_pmu_clk_clk_domain_35_slave_boardobj_set v35_slave;
};

NV_PMU_BOARDOBJ_GRP_SET_MAKE_E32(clk, clk_domain);

struct nv_pmu_clk_clk_prog_boardobjgrp_set_header {
	struct nv_pmu_boardobjgrp_e255 super;
	u8 slave_entry_count;
	u8 vf_entry_count;
	u8 vf_sec_entry_count;
};

struct nv_pmu_clk_clk_prog_boardobj_set {
	struct nv_pmu_boardobj super;
};

struct nv_pmu_clk_clk_prog_1x_boardobj_set {
	struct nv_pmu_clk_clk_prog_boardobj_set super;
	u8 source;
	u16 freq_max_mhz;
	union ctrl_clk_clk_prog_1x_source_data source_data;
};

struct nv_pmu_clk_clk_prog_1x_master_boardobj_set {
	struct nv_pmu_clk_clk_prog_1x_boardobj_set super;
	u8 rsvd;	/* Stubbing for RM_PMU_BOARDOBJ_INTERFACE */
	bool b_o_c_o_v_enabled;
	struct ctrl_clk_clk_prog_1x_master_vf_entry vf_entries[
		CTRL_CLK_CLK_PROG_1X_MASTER_VF_ENTRY_MAX_ENTRIES];
	struct ctrl_clk_clk_delta deltas;
	union ctrl_clk_clk_prog_1x_master_source_data source_data;
};

struct nv_pmu_clk_clk_prog_1x_master_ratio_boardobj_set {
	struct nv_pmu_clk_clk_prog_1x_master_boardobj_set super;
	u8 rsvd;	/* Stubbing for RM_PMU_BOARDOBJ_INTERFACE */
	struct ctrl_clk_clk_prog_1x_master_ratio_slave_entry slave_entries[
		CTRL_CLK_PROG_1X_MASTER_MAX_SLAVE_ENTRIES];
};

struct nv_pmu_clk_clk_prog_1x_master_table_boardobj_set {
	struct nv_pmu_clk_clk_prog_1x_master_boardobj_set super;
	u8 rsvd;	/* Stubbing for RM_PMU_BOARDOBJ_INTERFACE */
	struct ctrl_clk_clk_prog_1x_master_table_slave_entry
	slave_entries[CTRL_CLK_PROG_1X_MASTER_MAX_SLAVE_ENTRIES];
};

struct nv_pmu_clk_clk_prog_3x_master_boardobj_set {
	u8 rsvd;	/* Stubbing for RM_PMU_BOARDOBJ_INTERFACE */
	bool b_o_c_o_v_enabled;
	struct ctrl_clk_clk_prog_1x_master_vf_entry vf_entries[
			CTRL_CLK_CLK_PROG_1X_MASTER_VF_ENTRY_MAX_ENTRIES];
	struct ctrl_clk_clk_delta deltas;
	union ctrl_clk_clk_prog_1x_master_source_data source_data;
};

struct nv_pmu_clk_clk_prog_3x_master_ratio_boardobj_set {
	u8 rsvd;	/* Stubbing for RM_PMU_BOARDOBJ_INTERFACE */
	struct ctrl_clk_clk_prog_1x_master_ratio_slave_entry slave_entries[
				CTRL_CLK_PROG_1X_MASTER_MAX_SLAVE_ENTRIES];
};

struct nv_pmu_clk_clk_prog_3x_master_table_boardobj_set {
	u8 rsvd;	/* Stubbing for RM_PMU_BOARDOBJ_INTERFACE */
	struct ctrl_clk_clk_prog_1x_master_table_slave_entry slave_entries[
				CTRL_CLK_PROG_1X_MASTER_MAX_SLAVE_ENTRIES];
};

struct nv_pmu_clk_clk_prog_35_master_boardobj_set {
	struct nv_pmu_clk_clk_prog_1x_boardobj_set super;
	struct nv_pmu_clk_clk_prog_3x_master_boardobj_set master;
	struct ctrl_clk_clk_prog_35_master_sec_vf_entry_voltrail
		voltrail_sec_vf_entries[
			CTRL_CLK_CLK_PROG_1X_MASTER_VF_ENTRY_MAX_ENTRIES];
};

struct nv_pmu_clk_clk_prog_35_master_ratio_boardobj_set {
	struct nv_pmu_clk_clk_prog_35_master_boardobj_set super;
	struct nv_pmu_clk_clk_prog_3x_master_ratio_boardobj_set ratio;
};

struct nv_pmu_clk_clk_prog_35_master_table_boardobj_set {
	struct nv_pmu_clk_clk_prog_35_master_boardobj_set super;
	struct nv_pmu_clk_clk_prog_3x_master_table_boardobj_set table;
};

union nv_pmu_clk_clk_prog_boardobj_set_union {
	struct nv_pmu_boardobj board_obj;
	struct nv_pmu_clk_clk_prog_boardobj_set super;
	struct nv_pmu_clk_clk_prog_1x_boardobj_set v1x;
	struct nv_pmu_clk_clk_prog_1x_master_boardobj_set v1x_master;
	struct nv_pmu_clk_clk_prog_1x_master_ratio_boardobj_set
							v1x_master_ratio;
	struct nv_pmu_clk_clk_prog_1x_master_table_boardobj_set
							v1x_master_table;
	struct nv_pmu_clk_clk_prog_35_master_boardobj_set v35_master;
	struct nv_pmu_clk_clk_prog_35_master_ratio_boardobj_set
							v35_master_ratio;
	struct nv_pmu_clk_clk_prog_35_master_table_boardobj_set
							v35_master_table;
};

NV_PMU_BOARDOBJ_GRP_SET_MAKE_E255(clk, clk_prog);

struct nv_pmu_clk_lut_device_desc {
	u8 vselect_mode;
	u16 hysteresis_threshold;
};

struct nv_pmu_clk_regime_desc {
	u8 regime_id;
	u8 target_regime_id_override;
	u16 fixed_freq_regime_limit_mhz;
};

struct nv_pmu_clk_clk_fll_device_boardobjgrp_set_header {
	struct nv_pmu_boardobjgrp_e32 super;
	struct ctrl_boardobjgrp_mask_e32 lut_prog_master_mask;
	u32 lut_step_size_uv;
	u32 lut_min_voltage_uv;
	u8 lut_num_entries;
	u16 max_min_freq_mhz;
};

struct nv_pmu_clk_clk_fll_device_boardobj_set {
	struct nv_pmu_boardobj super;
	u8 id;
	u8 mdiv;
	u8 vin_idx_logic;
	u8 vin_idx_sram;
	u8 rail_idx_for_lut;
	u16 input_freq_mhz;
	u32 clk_domain;
	struct nv_pmu_clk_lut_device_desc lut_device;
	struct nv_pmu_clk_regime_desc regime_desc;
	u8 min_freq_vfe_idx;
	u8 freq_ctrl_idx;
	bool b_skip_pldiv_below_dvco_min;
	bool b_dvco_1x;
	struct ctrl_boardobjgrp_mask_e32 lut_prog_broadcast_slave_mask;
};

union nv_pmu_clk_clk_fll_device_boardobj_set_union {
	struct nv_pmu_boardobj board_obj;
	struct nv_pmu_clk_clk_fll_device_boardobj_set super;
};

NV_PMU_BOARDOBJ_GRP_SET_MAKE_E32(clk, clk_fll_device);

struct nv_pmu_clk_clk_vin_device_boardobjgrp_set_header {
	struct nv_pmu_boardobjgrp_e32 super;
	u8 version;
	bool b_vin_is_disable_allowed;
	u8 reserved[13];
};

struct nv_pmu_clk_clk_vin_device_boardobj_set {
	struct nv_pmu_boardobj super;
	u8 id;
	u8 volt_rail_idx;
	u8 por_override_mode;
	u8 override_mode;
	u32 flls_shared_mask;
};

struct nv_pmu_clk_clk_vin_device_v20_boardobj_set {
	struct nv_pmu_clk_clk_vin_device_boardobj_set super;
	struct ctrl_clk_vin_device_info_data_v20 data;
};

union nv_pmu_clk_clk_vin_device_boardobj_set_union {
	struct nv_pmu_boardobj board_obj;
	struct nv_pmu_clk_clk_vin_device_boardobj_set super;
	struct nv_pmu_clk_clk_vin_device_v20_boardobj_set v20;
};

NV_PMU_BOARDOBJ_GRP_SET_MAKE_E32(clk, clk_vin_device);

struct nv_pmu_clk_clk_vf_point_boardobjgrp_set_header {
	struct nv_pmu_boardobjgrp_e255 super;
};

struct nv_pmu_clk_clk_vf_point_sec_boardobjgrp_set_header {
	struct nv_pmu_boardobjgrp_e255 super;
};
struct nv_pmu_clk_clk_vf_point_boardobj_set {
	struct nv_pmu_boardobj super;
	u8 vfe_equ_idx;
	u8 volt_rail_idx;
};

struct nv_pmu_clk_clk_vf_point_freq_boardobj_set {
	struct nv_pmu_clk_clk_vf_point_boardobj_set super;
	u16 freq_mhz;
	int volt_delta_uv;
};

struct nv_pmu_clk_clk_vf_point_volt_boardobj_set {
	struct nv_pmu_clk_clk_vf_point_boardobj_set super;
	u32 source_voltage_uv;
	struct ctrl_clk_freq_delta freq_delta;
};

struct nv_pmu_clk_clk_vf_point_volt_35_sec_boardobj_set {
	struct nv_pmu_clk_clk_vf_point_volt_boardobj_set super;
	u8 dvco_offset_code_override;
};

union nv_pmu_clk_clk_vf_point_boardobj_set_union {
	struct nv_pmu_boardobj board_obj;
	struct nv_pmu_clk_clk_vf_point_boardobj_set super;
	struct nv_pmu_clk_clk_vf_point_freq_boardobj_set freq;
	struct nv_pmu_clk_clk_vf_point_volt_boardobj_set volt;
};

union nv_pmu_clk_clk_vf_point_sec_boardobj_set_union {
	struct nv_pmu_boardobj board_obj;
	struct nv_pmu_clk_clk_vf_point_boardobj_set super;
	struct nv_pmu_clk_clk_vf_point_freq_boardobj_set freq;
	struct nv_pmu_clk_clk_vf_point_volt_boardobj_set volt;
	struct nv_pmu_clk_clk_vf_point_volt_35_sec_boardobj_set v35_volt_sec;
};

NV_PMU_BOARDOBJ_GRP_SET_MAKE_E255(clk, clk_vf_point);
NV_PMU_BOARDOBJ_GRP_SET_MAKE_E255(clk, clk_vf_point_sec);

struct nv_pmu_clk_clk_vf_point_boardobjgrp_get_status_header {
	struct nv_pmu_boardobjgrp_e255 super;
	u32 vf_points_cahce_counter;
};

struct nv_pmu_clk_clk_vf_point_35_freq_boardobj_get_status {
	struct nv_pmu_boardobj super;
	struct ctrl_clk_vf_point_base_vf_tuple base_vf_tuple;
	struct ctrl_clk_vf_point_vf_tuple
		offseted_vf_tuple[CTRL_CLK_CLK_VF_POINT_FREQ_TUPLE_MAX_SIZE];
};

struct nv_pmu_clk_clk_vf_point_35_volt_pri_boardobj_get_status {
	struct nv_pmu_boardobj super;
	struct ctrl_clk_vf_point_base_vf_tuple base_vf_tuple;
	struct ctrl_clk_vf_point_vf_tuple
		offseted_vf_tuple[CTRL_CLK_CLK_VF_POINT_FREQ_TUPLE_MAX_SIZE];
};

struct nv_pmu_clk_clk_vf_point_35_volt_sec_boardobj_get_status {
	struct nv_pmu_boardobj super;
	struct ctrl_clk_vf_point_base_vf_tuple_sec base_vf_tuple;
	struct ctrl_clk_vf_point_vf_tuple
		offseted_vf_tuple[CTRL_CLK_CLK_VF_POINT_FREQ_TUPLE_MAX_SIZE];
};

struct nv_pmu_clk_clk_vf_point_boardobj_get_status {
	struct nv_pmu_boardobj super;
	struct ctrl_clk_vf_pair pair;
	u8 dummy[38];
};

struct nv_pmu_clk_clk_vf_point_volt_boardobj_get_status {
	struct nv_pmu_clk_clk_vf_point_boardobj_get_status super;
	u16 vf_gain_value;
};

union nv_pmu_clk_clk_vf_point_boardobj_get_status_union {
	struct nv_pmu_boardobj board_obj;
	struct nv_pmu_clk_clk_vf_point_boardobj_get_status super;
	struct nv_pmu_clk_clk_vf_point_volt_boardobj_get_status volt;
	struct nv_pmu_clk_clk_vf_point_35_freq_boardobj_get_status v35_freq;
	struct nv_pmu_clk_clk_vf_point_35_volt_pri_boardobj_get_status
								v35_volt_pri;
	struct nv_pmu_clk_clk_vf_point_35_volt_sec_boardobj_get_status
								v35_volt_sec;
};

NV_PMU_BOARDOBJ_GRP_GET_STATUS_MAKE_E255(clk, clk_vf_point);

#define NV_PMU_VF_INJECT_MAX_CLOCK_DOMAINS                                 (12U)

struct nv_pmu_clk_clk_domain_list {
	u8 num_domains;
	struct ctrl_clk_clk_domain_list_item clk_domains[
		NV_PMU_VF_INJECT_MAX_CLOCK_DOMAINS];
};

struct nv_pmu_clk_clk_domain_list_v1 {
	u8 num_domains;
	struct ctrl_clk_clk_domain_list_item_v1 clk_domains[
		NV_PMU_VF_INJECT_MAX_CLOCK_DOMAINS];
};

struct nv_pmu_clk_vf_change_inject {
	u8 flags;
	struct nv_pmu_clk_clk_domain_list clk_list;
};

struct nv_pmu_clk_vf_change_inject_v1 {
	u8 flags;
	struct nv_pmu_clk_clk_domain_list_v1 clk_list;
};

#define NV_NV_PMU_CLK_LOAD_ACTION_MASK_VIN_HW_CAL_PROGRAM_YES      (0x00000001U)

struct nv_pmu_clk_load {
	u8 feature;
	u32 action_mask;
};

struct nv_pmu_clk_freq_effective_avg {
	u32 clkDomainMask;
	u32 freqkHz[CTRL_BOARDOBJ_MAX_BOARD_OBJECTS];
};

#define NV_NV_PMU_CLK_LOAD_ACTION_MASK_FREQ_EFFECTIVE_AVG_CALLBACK_NO \
								   (0x00000000U)
#define NV_NV_PMU_CLK_LOAD_ACTION_MASK_FREQ_EFFECTIVE_AVG_CALLBACK_YES \
								   (0x00000004U)

/* CLK CMD ID definitions.  */
#define NV_PMU_CLK_CMD_ID_BOARDOBJ_GRP_SET                         (0x00000001U)
#define NV_PMU_CLK_CMD_ID_RPC                                      (0x00000000U)
#define NV_PMU_CLK_CMD_ID_BOARDOBJ_GRP_GET_STATUS                  (0x00000002U)

struct nv_pmu_rpc_struct_clk_load {
	struct nv_pmu_rpc_header hdr;
	struct nv_pmu_clk_load clk_load;
	u32 scratch[1];
};

struct nv_pmu_clk_cmd_rpc {
	u8 cmd_type;
	u8 pad[3];
	struct nv_pmu_allocation request;
};

struct nv_pmu_clk_cmd_generic {
	u8 cmd_type;
	bool b_perf_daemon_cmd;
	u8 pad[2];
};

#define NV_PMU_CLK_CMD_RPC_ALLOC_OFFSET                                        \
	(offsetof(struct nv_pmu_clk_cmd_rpc, request))

struct nv_pmu_clk_cmd {
	union {
		u8 cmd_type;
		struct nv_pmu_boardobj_cmd_grp grp_set;
		struct nv_pmu_clk_cmd_generic generic;
		struct nv_pmu_clk_cmd_rpc rpc;
		struct nv_pmu_boardobj_cmd_grp grp_get_status;
	};
};

/* CLK MSG ID definitions */
#define NV_PMU_CLK_MSG_ID_BOARDOBJ_GRP_SET                         (0x00000001U)
#define NV_PMU_CLK_MSG_ID_RPC                                      (0x00000000U)
#define NV_PMU_CLK_MSG_ID_BOARDOBJ_GRP_GET_STATUS                  (0x00000002U)

struct nv_pmu_clk_msg_rpc {
	u8 msg_type;
	u8 rsvd[3];
	struct nv_pmu_allocation response;
};

#define NV_PMU_CLK_MSG_RPC_ALLOC_OFFSET       \
	((u32)offsetof(struct nv_pmu_clk_msg_rpc, response))

struct nv_pmu_clk_msg {
	union {
		u8 msg_type;
		struct nv_pmu_boardobj_msg_grp grp_set;
		struct nv_pmu_clk_msg_rpc rpc;
		struct nv_pmu_boardobj_msg_grp grp_get_status;
	};
};

struct nv_pmu_clk_clk_vin_device_boardobjgrp_get_status_header {
	struct nv_pmu_boardobjgrp_e32 super;
};

struct nv_pmu_clk_clk_vin_device_boardobj_get_status {
	struct nv_pmu_boardobj_query super;
	u32 actual_voltage_uv;
	u32 corrected_voltage_uv;
	u8 sampled_code;
	u8 override_code;
};

union nv_pmu_clk_clk_vin_device_boardobj_get_status_union {
	struct nv_pmu_boardobj_query board_obj;
	struct nv_pmu_clk_clk_vin_device_boardobj_get_status super;
};

NV_PMU_BOARDOBJ_GRP_GET_STATUS_MAKE_E32(clk, clk_vin_device);

struct nv_pmu_clk_lut_vf_entry {
	u32 entry;
};

struct nv_pmu_clk_clk_fll_device_boardobjgrp_get_status_header {
	struct nv_pmu_boardobjgrp_e32 super;
};

struct nv_pmu_clk_clk_fll_device_boardobj_get_status {
	struct nv_pmu_boardobj_query super;
	u8 current_regime_id;
	bool b_dvco_min_reached;
	u16 min_freq_mhz;
	struct nv_pmu_clk_lut_vf_entry
		lut_vf_curve[NV_UNSIGNED_ROUNDED_DIV(
					CTRL_CLK_LUT_NUM_ENTRIES_MAX, 2)];
};

union nv_pmu_clk_clk_fll_device_boardobj_get_status_union {
	struct nv_pmu_boardobj_query board_obj;
	struct nv_pmu_clk_clk_fll_device_boardobj_get_status super;
};

NV_PMU_BOARDOBJ_GRP_GET_STATUS_MAKE_E32(clk, clk_fll_device);

struct nv_pmu_rpc_clk_domain_35_prog_freq_to_volt {
	/*[IN/OUT] Must be first field in RPC structure */
	struct nv_pmu_rpc_header hdr;
	u8 clk_domain_idx;
	u8 volt_rail_idx;
	u8 voltage_type;
	struct ctrl_clk_vf_input input;
	struct ctrl_clk_vf_output output;
	u32 scratch[1];
};

#endif /* NVGPU_PMUIF_CLK_H */
