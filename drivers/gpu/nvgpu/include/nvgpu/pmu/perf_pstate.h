/*
 * general p state infrastructure
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

#ifndef NVGPU_PMU_PERF_PSTATE_H_
#define NVGPU_PMU_PERF_PSTATE_H_

#include <nvgpu/types.h>

#define CTRL_PERF_PSTATE_P0		0U
#define CTRL_PERF_PSTATE_P5		5U
#define CTRL_PERF_PSTATE_P8		8U

#define CLK_SET_INFO_MAX_SIZE		(32U)

struct gk20a;
struct boardobj;
struct boardobjgrp_e32;

struct clk_set_info {
	u32 clkwhich;
	u32 nominal_mhz;
	u16 min_mhz;
	u16 max_mhz;
};

struct clk_set_info_list {
	u32 num_info;
	struct clk_set_info clksetinfo[CLK_SET_INFO_MAX_SIZE];
};

struct pstate {
	struct boardobj super;
	u32 num;
	u8 lpwr_entry_idx;
	u32 flags;
	u8 pcie_idx;
	u8 nvlink_idx;
	struct clk_set_info_list clklist;
};

struct pstates {
	struct boardobjgrp_e32 super;
	u8 num_clk_domains;
};

struct clk_set_info *nvgpu_pmu_perf_pstate_get_clk_set_info(struct gk20a *g,
			u32 pstate_num,
			u32 clkwhich);
struct pstate *nvgpu_pmu_perf_pstate_find(struct gk20a *g, u32 num);
int nvgpu_pmu_perf_pstate_sw_setup(struct gk20a *g);
int nvgpu_pmu_perf_pstate_pmu_setup(struct gk20a *g);

#endif /* NVGPU_PMU_PERF_PSTATE_H_ */
