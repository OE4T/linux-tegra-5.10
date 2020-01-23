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
#ifndef NVGPU_PMU_PERF_H
#define NVGPU_PMU_PERF_H

#include <nvgpu/pmu/volt.h>
#include <nvgpu/boardobjgrp_e32.h>
#include <nvgpu/boardobjgrp_e255.h>
#include <nvgpu/boardobjgrpmask.h>

/* Dependency of this include will be removed in further CL */
#include "../../../common/pmu/perf/ucode_perf_change_seq_inf.h"

struct nvgpu_clk_slave_freq;

#define CTRL_PERF_PSTATE_P0		0U
#define CTRL_PERF_PSTATE_P5		5U
#define CTRL_PERF_PSTATE_P8		8U
#define CLK_SET_INFO_MAX_SIZE		(32U)

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

/* PERF Message-type Definitions */
#define NV_PMU_PERF_MSG_ID_RPC                                   (0x00000003U)
#define NV_PMU_PERF_MSG_ID_BOARDOBJ_GRP_SET                      (0x00000004U)
#define NV_PMU_PERF_MSG_ID_BOARDOBJ_GRP_GET_STATUS               (0x00000006U)

struct nvgpu_vfe_invalidate {
	bool state_change;
	struct nvgpu_cond wq;
	struct nvgpu_thread state_task;
};

struct vfe_vars {
	struct boardobjgrp_e32 super;
	u8 polling_periodms;
};

struct vfe_equs {
	struct boardobjgrp_e255 super;
};

struct change_seq_pmu_script {
	struct perf_change_seq_pmu_script buf;
	u32 super_surface_offset;
};

struct clk_set_info {
	u32 clkwhich;
	u32 nominal_mhz;
	u16 min_mhz;
	u16 max_mhz;
};

struct pstates {
	struct boardobjgrp_e32 super;
	u8 num_clk_domains;
};

struct change_seq {
	u8 version;
	bool b_enabled_pmu_support;
	u32 thread_seq_id_last;
	u64 thread_carry_over_timens;
	struct ctrl_perf_change_seq_change last_pstate_values;
	struct boardobjgrpmask_e32 clk_domains_exclusion_mask;
	struct boardobjgrpmask_e32 clk_domains_inclusion_mask;
	u32 client_lock_mask;
};

struct change_seq_pmu {
	struct change_seq super;
	bool b_lock;
	bool b_vf_point_check_ignore;
	u32 cpu_adverised_step_id_mask;
	u32 cpu_step_id_mask;
	u32 event_mask_pending;
	u32 event_mask_received;
	u32 last_completed_change_Seq_id;
	struct change_seq_pmu_script script_curr;
	struct change_seq_pmu_script script_last;
	struct change_seq_pmu_script script_query;
	u32 change_state;
	s64 start_time;
	s64 stop_time;
};

struct perf_pmupstate {
	struct vfe_vars vfe_varobjs;
	struct vfe_equs vfe_equobjs;
	struct pstates pstatesobjs;
	struct nvgpu_pmu_volt volt;
	struct nvgpu_vfe_invalidate vfe_init;
	struct change_seq_pmu changeseq_pmu;
};

int nvgpu_pmu_perf_init(struct gk20a *g);
void nvgpu_pmu_perf_deinit(struct gk20a *g);
int nvgpu_pmu_perf_sw_setup(struct gk20a *g);
int nvgpu_pmu_perf_pmu_setup(struct gk20a *g);

int nvgpu_pmu_perf_load(struct gk20a *g);

int nvgpu_pmu_perf_vfe_get_s_param(struct gk20a *g, u64 *s_param);

int nvgpu_pmu_perf_vfe_get_volt_margin(struct gk20a *g, u32 *vmargin_uv);
int nvgpu_pmu_perf_vfe_get_freq_margin(struct gk20a *g, u32 *fmargin_mhz);

int nvgpu_pmu_perf_changeseq_set_clks(struct gk20a *g,
	struct nvgpu_clk_slave_freq *vf_point);

struct clk_set_info *nvgpu_pmu_perf_pstate_get_clk_set_info(struct gk20a *g,
			u32 pstate_num,
			u32 clkwhich);

#endif /* NVGPU_PMU_PERF_H */
