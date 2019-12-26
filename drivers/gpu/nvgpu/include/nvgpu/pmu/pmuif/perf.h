/*
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
#ifndef NVGPU_PMUIF_PERF_H
#define NVGPU_PMUIF_PERF_H

#include "volt.h"

#define NV_PMU_PERF_CMD_ID_RPC                                   (0x00000002U)
#define NV_PMU_PERF_CMD_ID_BOARDOBJ_GRP_SET                      (0x00000003U)
#define NV_PMU_PERF_CMD_ID_BOARDOBJ_GRP_GET_STATUS               (0x00000004U)

/*!
 * RPC calls serviced by PERF unit.
 */
#define NV_PMU_RPC_ID_PERF_BOARD_OBJ_GRP_CMD                     0x00U
#define NV_PMU_RPC_ID_PERF_LOAD                                  0x01U
#define NV_PMU_RPC_ID_PERF_CHANGE_SEQ_INFO_GET                   0x02U
#define NV_PMU_RPC_ID_PERF_CHANGE_SEQ_INFO_SET                   0x03U
#define NV_PMU_RPC_ID_PERF_CHANGE_SEQ_SET_CONTROL                0x04U
#define NV_PMU_RPC_ID_PERF_CHANGE_SEQ_QUEUE_CHANGE               0x05U
#define NV_PMU_RPC_ID_PERF_CHANGE_SEQ_LOCK                       0x06U
#define NV_PMU_RPC_ID_PERF_CHANGE_SEQ_QUERY                      0x07U
#define NV_PMU_RPC_ID_PERF_PERF_LIMITS_INVALIDATE                0x08U
#define NV_PMU_RPC_ID_PERF_PERF_PSTATE_STATUS_UPDATE             0x09U
#define NV_PMU_RPC_ID_PERF_VFE_EQU_EVAL                          0x0AU
#define NV_PMU_RPC_ID_PERF_VFE_INVALIDATE                        0x0BU
#define NV_PMU_RPC_ID_PERF_VFE_EQU_MONITOR_SET                   0x0CU
#define NV_PMU_RPC_ID_PERF_VFE_EQU_MONITOR_GET                   0x0DU
#define NV_PMU_RPC_ID_PERF__COUNT                                0x0EU
/*
 * Defines the structure that holds data
 * used to execute LOAD RPC.
 */
struct nv_pmu_rpc_struct_perf_load {
	/* [IN/OUT] Must be first field in RPC structure */
	struct nv_pmu_rpc_header hdr;
	bool b_load;
	u32 scratch[1];
};

/* PERF Message-type Definitions */
#define NV_PMU_PERF_MSG_ID_RPC                                   (0x00000003U)
#define NV_PMU_PERF_MSG_ID_BOARDOBJ_GRP_SET                      (0x00000004U)
#define NV_PMU_PERF_MSG_ID_BOARDOBJ_GRP_GET_STATUS               (0x00000006U)

/* PERF RPC ID Definitions */
#define NV_PMU_RPC_ID_PERF_VFE_CALLBACK                          0x01U
#define NV_PMU_RPC_ID_PERF_SEQ_COMPLETION                        0x02U
#define NV_PMU_RPC_ID_PERF_PSTATES_INVALIDATE                    0x03U

/*
 * Simply a union of all specific PERF messages. Forms the general packet
 * exchanged between the Kernel and PMU when sending and receiving PERF messages
 * (respectively).
 */

struct pmu_nvgpu_rpc_perf_event {
	struct pmu_hdr msg_hdr;
	struct pmu_nvgpu_rpc_header rpc_hdr;
};

struct nv_pmu_rpc_perf_change_seq_queue_change {
	/*[IN/OUT] Must be first field in RPC structure */
	struct nv_pmu_rpc_header hdr;
	struct ctrl_perf_change_seq_change_input change;
	u32 seq_id;
	u32 scratch[1];
};

struct nv_pmu_perf_change_seq_super_info_get {
	u8 version;
};

struct nv_pmu_perf_change_seq_pmu_info_get {
	struct nv_pmu_perf_change_seq_super_info_get  super;
	u32 cpu_advertised_step_id_mask;
};

struct nv_pmu_perf_change_seq_super_info_set {
	u8 version;
	struct ctrl_boardobjgrp_mask_e32 clk_domains_exclusion_mask;
	struct ctrl_boardobjgrp_mask_e32 clk_domains_inclusion_mask;
	u32 strp_id_exclusive_mask;
};

struct nv_pmu_perf_change_seq_pmu_info_set {
	struct nv_pmu_perf_change_seq_super_info_set super;
	bool b_lock;
	bool b_vf_point_check_ignore;
	u32 cpu_step_id_mask;
};

struct nv_pmu_rpc_perf_change_seq_info_get {
	/*[IN/OUT] Must be first field in RPC structure */
	struct nv_pmu_rpc_header hdr;
	struct nv_pmu_perf_change_seq_pmu_info_get info_get;
	u32 scratch[1];
};

struct nv_pmu_rpc_perf_change_seq_info_set {
	/*[IN/OUT] Must be first field in RPC structure */
	struct nv_pmu_rpc_header hdr;
	struct nv_pmu_perf_change_seq_pmu_info_set info_set;
	u32 scratch[1];
};

NV_PMU_MAKE_ALIGNED_STRUCT(ctrl_perf_change_seq_change,
		sizeof(struct ctrl_perf_change_seq_change));

NV_PMU_MAKE_ALIGNED_STRUCT(ctrl_perf_change_seq_pmu_script_header,
		sizeof(struct ctrl_perf_change_seq_pmu_script_header));

NV_PMU_MAKE_ALIGNED_UNION(ctrl_perf_change_seq_pmu_script_step_data,
		sizeof(union ctrl_perf_change_seq_pmu_script_step_data));

struct perf_change_seq_pmu_script {
	union ctrl_perf_change_seq_pmu_script_header_aligned hdr;
	union ctrl_perf_change_seq_change_aligned change;
	/* below should be an aligned structure */
	union ctrl_perf_change_seq_pmu_script_step_data_aligned
		steps[CTRL_PERF_CHANGE_SEQ_SCRIPT_VF_SWITCH_MAX_STEPS];
};

#endif /* NVGPU_PMUIF_PERF_H */
