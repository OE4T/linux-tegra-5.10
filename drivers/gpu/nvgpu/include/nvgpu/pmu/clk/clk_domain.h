/*
 * general clock structures & definitions
 *
 * Copyright (c) 2019, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_PMU_CLK_DOMAIN_H
#define NVGPU_PMU_CLK_DOMAIN_H

#include <nvgpu/types.h>

struct gk20a;
struct nvgpu_clk_domain;
struct nvgpu_clk_slave_freq;
struct ctrl_perf_change_seq_change_input;
struct nvgpu_clk_pmupstate;

typedef int nvgpu_clkproglink(struct gk20a *g, struct nvgpu_clk_pmupstate *pclk,
	struct nvgpu_clk_domain *pdomain);

typedef int nvgpu_clkvfsearch(struct gk20a *g, struct nvgpu_clk_pmupstate *pclk,
	struct nvgpu_clk_domain *pdomain, u16 *clkmhz,
	u32 *voltuv, u8 rail);

typedef int nvgpu_clkgetfpoints(struct gk20a *g,
	struct nvgpu_clk_pmupstate *pclk, struct nvgpu_clk_domain *pdomain,
	u32 *pfpointscount, u16 *pfreqpointsinmhz, u8 rail);

struct nvgpu_clk_domain {
	struct boardobj super;
	u32 api_domain;
	u32 part_mask;
	u32 domain;
	u8 perf_domain_index;
	u8 perf_domain_grp_idx;
	u8 ratio_domain;
	u8 usage;
	nvgpu_clkproglink *clkdomainclkproglink;
	nvgpu_clkvfsearch *clkdomainclkvfsearch;
	nvgpu_clkgetfpoints *clkdomainclkgetfpoints;
};

struct nvgpu_clk_domains {
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

	struct nvgpu_clk_domain
		*ordered_noise_aware_list[CTRL_BOARDOBJ_MAX_BOARD_OBJECTS];

	struct nvgpu_clk_domain
		*ordered_noise_unaware_list[CTRL_BOARDOBJ_MAX_BOARD_OBJECTS];
};

int nvgpu_clk_domain_init_pmupstate(struct gk20a *g);
void nvgpu_clk_domain_free_pmupstate(struct gk20a *g);
int nvgpu_clk_pmu_clk_domains_load(struct gk20a *g);
int nvgpu_clk_domain_sw_setup(struct gk20a *g);
int nvgpu_clk_domain_pmu_setup(struct gk20a *g);

#endif /* NVGPU_PMU_CLK_DOMAIN_H */
