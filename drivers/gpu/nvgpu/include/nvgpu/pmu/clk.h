/*
 * general clock structures & definitions
 *
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
#ifndef NVGPU_PMU_CLK_H
#define NVGPU_PMU_CLK_H

#include <nvgpu/boardobjgrp_e32.h>
#include <nvgpu/boardobjgrp_e255.h>
#include <nvgpu/boardobjgrpmask.h>
#include <nvgpu/types.h>
#include <nvgpu/pmuif/ctrlclk.h>

struct clk_domain;
struct gk20a;

/* clock related defines for GPUs supporting clock control from pmu*/
struct avfsvinobjs {
	struct boardobjgrp_e32 super;
	u8 calibration_rev_vbios;
	u8 calibration_rev_fused;
	bool vin_is_disable_allowed;
};

struct avfsfllobjs {
	struct boardobjgrp_e32 super;
	struct boardobjgrpmask_e32 lut_prog_master_mask;
	u32 lut_step_size_uv;
	u32 lut_min_voltage_uv;
	u8 lut_num_entries;
	u16 max_min_freq_mhz;
};

typedef int clkproglink(struct gk20a *g, struct clk_pmupstate *pclk,
			struct clk_domain *pdomain);

typedef int clkvfsearch(struct gk20a *g, struct clk_pmupstate *pclk,
			struct clk_domain *pdomain, u16 *clkmhz,
			u32 *voltuv, u8 rail);

typedef int clkgetfpoints(struct gk20a *g, struct clk_pmupstate *pclk,
			struct clk_domain *pdomain, u32 *pfpointscount,
			  u16 *pfreqpointsinmhz, u8 rail);

struct clk_domain {
	struct boardobj super;
	u32 api_domain;
	u32 part_mask;
	u32 domain;
	u8 perf_domain_index;
	u8 perf_domain_grp_idx;
	u8 ratio_domain;
	u8 usage;
	clkproglink *clkdomainclkproglink;
	clkvfsearch *clkdomainclkvfsearch;
	clkgetfpoints *clkdomainclkgetfpoints;
};

struct clk_domains {
	struct boardobjgrp_e32 super;
	u8 n_num_entries;
	u8 version;
	bool b_enforce_vf_monotonicity;
	bool b_enforce_vf_smoothening;
	bool b_override_o_v_o_c;
	bool b_debug_mode;
	u32 vbios_domains;
	u16 cntr_sampling_periodms;
	struct boardobjgrpmask_e32 prog_domains_mask;
	struct boardobjgrpmask_e32 master_domains_mask;
	struct ctrl_clk_clk_delta  deltas;

	struct clk_domain *ordered_noise_aware_list[CTRL_BOARDOBJ_MAX_BOARD_OBJECTS];

	struct clk_domain *ordered_noise_unaware_list[CTRL_BOARDOBJ_MAX_BOARD_OBJECTS];
};

struct clk_progs {
	struct boardobjgrp_e255 super;
	u8 slave_entry_count;
	u8 vf_entry_count;
	u8 vf_sec_entry_count;
};

struct clk_vf_points {
	struct boardobjgrp_e255 super;
};

struct clk_mclk_state {
	u32 speed;
	struct nvgpu_mutex mclk_lock;
	struct nvgpu_mutex data_lock;

	u16 p5_min;
	u16 p0_min;

	void *vreg_buf;
	bool init;

	s64 switch_max;
	s64 switch_min;
	u64 switch_num;
	s64 switch_avg;
	s64 switch_std;
	bool debugfs_set;
};

struct clk_freq_controllers {
	struct boardobjgrp_e32 super;
	u32 sampling_period_ms;
	struct boardobjgrpmask_e32 freq_ctrl_load_mask;
	u8 volt_policy_idx;
	void *pprereq_load;
};

struct nvgpu_clk_freq_domain_grp {
	struct boardobjgrp_e32 super;
	u32 init_flags;
};

struct clk_pmupstate {
	struct avfsvinobjs avfs_vinobjs;
	struct avfsfllobjs avfs_fllobjs;
	struct clk_domains clk_domainobjs;
	struct clk_progs clk_progobjs;
	struct clk_vf_points clk_vf_pointobjs;
	struct clk_mclk_state clk_mclk;
	struct clk_freq_controllers clk_freq_controllers;
	struct nvgpu_clk_freq_domain_grp freq_domain_grp_objs;
};

struct set_fll_clk {
		u32 voltuv;
		u16 gpc2clkmhz;
		u8 current_regime_id_gpc;
		u8 target_regime_id_gpc;
		u16 sys2clkmhz;
		u8 current_regime_id_sys;
		u8 target_regime_id_sys;
		u16 xbar2clkmhz;
		u8 current_regime_id_xbar;
		u8 target_regime_id_xbar;
		u16 nvdclkmhz;
		u8 current_regime_id_nvd;
		u8 target_regime_id_nvd;
		u16 hostclkmhz;
		u8 current_regime_id_host;
		u8 target_regime_id_host;
};

int clk_init_pmupstate(struct gk20a *g);
void clk_free_pmupstate(struct gk20a *g);
int nvgpu_clk_set_fll_clk_gv10x(struct gk20a *g);
int clk_pmu_vin_load(struct gk20a *g);
int clk_pmu_clk_domains_load(struct gk20a *g);
u32 nvgpu_clk_vf_change_inject_data_fill_gv10x(struct gk20a *g,
	struct nv_pmu_clk_rpc *rpccall,
	struct set_fll_clk *setfllclk);
u32 nvgpu_clk_vf_change_inject_data_fill_gp10x(struct gk20a *g,
	struct nv_pmu_clk_rpc *rpccall,
	struct set_fll_clk *setfllclk);
int nvgpu_clk_set_boot_fll_clk_gv10x(struct gk20a *g);
int nvgpu_clk_set_boot_fll_clk_tu10x(struct gk20a *g);

int clk_vin_sw_setup(struct gk20a *g);
int clk_vin_pmu_setup(struct gk20a *g);
int clk_avfs_get_vin_cal_fuse_v10(struct gk20a *g,
					struct avfsvinobjs *pvinobjs,
					struct vin_device_v20 *pvindev);
int clk_avfs_get_vin_cal_fuse_v20(struct gk20a *g,
					struct avfsvinobjs *pvinobjs,
					struct vin_device_v20 *pvindev);

/*data and function definition to talk to driver*/
int clk_fll_sw_setup(struct gk20a *g);
int clk_fll_pmu_setup(struct gk20a *g);
u32 nvgpu_clk_get_vbios_clk_domain_gv10x( u32 vbios_domain);
u32 nvgpu_clk_get_vbios_clk_domain_gp10x( u32 vbios_domain);

/*data and function definition to talk to driver*/
int clk_domain_sw_setup(struct gk20a *g);
int clk_domain_pmu_setup(struct gk20a *g);

int clk_vf_point_sw_setup(struct gk20a *g);
int clk_vf_point_pmu_setup(struct gk20a *g);

int clk_prog_sw_setup(struct gk20a *g);
int clk_prog_pmu_setup(struct gk20a *g);

int nvgpu_clk_freq_domain_sw_setup(struct gk20a *g);
int nvgpu_clk_freq_domain_pmu_setup(struct gk20a *g);

int clk_freq_controller_sw_setup(struct gk20a *g);
int clk_freq_controller_pmu_setup(struct gk20a *g);

#endif /* NVGPU_PMU_CLK_H */
