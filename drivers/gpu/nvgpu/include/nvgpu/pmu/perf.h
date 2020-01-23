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

#include <nvgpu/types.h>
#include <nvgpu/cond.h>
#include <nvgpu/thread.h>
#include <nvgpu/pmu/volt.h>
#include <nvgpu/pmu/lpwr.h>
#include <nvgpu/boardobjgrp_e32.h>
#include <nvgpu/boardobjgrp_e255.h>
#include <nvgpu/boardobjgrpmask.h>
#include <nvgpu/pmu/pmuif/perf.h>

/* Dependency of this include will be removed in further CL */
#include "../../../common/pmu/perf/ucode_perf_change_seq_inf.h"

struct nvgpu_clk_slave_freq;

#define CTRL_PERF_PSTATE_P0		0U
#define CTRL_PERF_PSTATE_P5		5U
#define CTRL_PERF_PSTATE_P8		8U
#define CLK_SET_INFO_MAX_SIZE		(32U)

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
	struct obj_lwpr lpwr;
	struct nvgpu_vfe_invalidate vfe_init;
	struct change_seq_pmu changeseq_pmu;
};

int nvgpu_perf_pmu_init_pmupstate(struct gk20a *g);
void nvgpu_perf_pmu_free_pmupstate(struct gk20a *g);
int nvgpu_perf_pmu_vfe_load_ps35(struct gk20a *g);

int nvgpu_vfe_equ_sw_setup(struct gk20a *g);
int nvgpu_vfe_equ_pmu_setup(struct gk20a *g);

int nvgpu_vfe_var_sw_setup(struct gk20a *g);
int nvgpu_vfe_var_pmu_setup(struct gk20a *g);
int nvgpu_vfe_var_get_s_param(struct gk20a *g, u64 *s_param);

int nvgpu_vfe_get_volt_margin_limit(struct gk20a *g, u32 *vmargin_uv);
int nvgpu_vfe_get_freq_margin_limit(struct gk20a *g, u32 *fmargin_mhz);

int nvgpu_perf_change_seq_sw_setup(struct gk20a *g);
int nvgpu_perf_change_seq_pmu_setup(struct gk20a *g);
int nvgpu_clk_set_req_fll_clk_ps35(struct gk20a *g,
	struct nvgpu_clk_slave_freq *vf_point);

int nvgpu_perf_pstate_get_lpwr_index(struct gk20a *g, u32 num, u8 *lpwr_idx);
int nvgpu_get_pstate_entry_idx(struct gk20a *g, u32 num);
struct clk_set_info *nvgpu_pmu_perf_pstate_get_clk_set_info(struct gk20a *g,
			u32 pstate_num,
			u32 clkwhich);
int nvgpu_pmu_perf_pstate_sw_setup(struct gk20a *g);
int nvgpu_pmu_perf_pstate_pmu_setup(struct gk20a *g);

#endif /* NVGPU_PMU_PERF_H */
