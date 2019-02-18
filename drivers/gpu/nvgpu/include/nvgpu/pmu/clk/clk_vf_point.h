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

#ifndef NVGPU_PMU_CLK_VF_POINT_H
#define NVGPU_PMU_CLK_VF_POINT_H

#include <nvgpu/boardobjgrp_e255.h>
#include <nvgpu/pmuif/gpmuifclk.h>
#include <nvgpu/types.h>
#include <nvgpu/pmu/clk/clk_fll.h>

struct gk20a;

struct nvgpu_clk_vf_points {
	struct boardobjgrp_e255 super;
};

u32 nvgpu_clk_vf_change_inject_data_fill_gv10x(struct gk20a *g,
	struct nv_pmu_clk_rpc *rpccall,
	struct nvgpu_set_fll_clk *setfllclk);
u32 nvgpu_clk_vf_change_inject_data_fill_gp10x(struct gk20a *g,
	struct nv_pmu_clk_rpc *rpccall,
	struct nvgpu_set_fll_clk *setfllclk);
int nvgpu_clk_vf_point_sw_setup(struct gk20a *g);
int nvgpu_clk_vf_point_pmu_setup(struct gk20a *g);

#endif /* NVGPU_PMU_CLK_VF_POINT_H */
