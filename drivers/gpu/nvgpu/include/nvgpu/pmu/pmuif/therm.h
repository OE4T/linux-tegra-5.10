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

#ifndef NVGPU_PMUIF_THERM_H
#define NVGPU_PMUIF_THERM_H

#include <nvgpu/flcnif_cmn.h>

#define NV_PMU_THERM_CMD_ID_RPC                                      0x00000002U
#define NV_PMU_THERM_MSG_ID_RPC                                      0x00000002U
#define NV_PMU_THERM_EVENT_THERMAL_1                                 0x00000004U
#define NV_PMU_THERM_CMD_ID_HW_SLOWDOWN_NOTIFICATION                 0x00000001U
#define NV_PMU_THERM_MSG_ID_EVENT_HW_SLOWDOWN_NOTIFICATION           0x00000001U

struct nv_pmu_therm_msg_rpc {
	u8 msg_type;
	u8 rsvd[3];
	struct nv_pmu_allocation response;
};

struct nv_pmu_therm_msg_event_hw_slowdown_notification {
	u8 msg_type;
	u32 mask;
};

#define NV_PMU_THERM_MSG_RPC_ALLOC_OFFSET       \
	offsetof(struct nv_pmu_therm_msg_rpc, response)

struct nv_pmu_therm_msg {
	union {
		u8 msg_type;
		struct nv_pmu_therm_msg_rpc rpc;
		struct nv_pmu_therm_msg_event_hw_slowdown_notification
								hw_slct_msg;
	};
};

#endif /* NVGPU_PMUIF_THERM_H */
