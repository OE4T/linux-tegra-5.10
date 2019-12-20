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

#endif /* NVGPU_PMUIF_PERF_H */
