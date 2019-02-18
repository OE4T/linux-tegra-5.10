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

#ifndef NVGPU_PMU_CLK_H
#define NVGPU_PMU_CLK_H

#include <nvgpu/pmu/clk/clk_vin.h>
#include <nvgpu/pmu/clk/clk_fll.h>
#include <nvgpu/pmu/clk/clk_domain.h>
#include <nvgpu/pmu/clk/clk_prog.h>
#include <nvgpu/pmu/clk/clk_vf_point.h>
#include <nvgpu/pmu/clk/clk_mclk.h>
#include <nvgpu/pmu/clk/clk_freq_controller.h>
#include <nvgpu/pmu/clk/clk_freq_domain.h>

struct gk20a;

struct nvgpu_clk_pmupstate {
	struct nvgpu_avfsvinobjs avfs_vinobjs;
	struct nvgpu_avfsfllobjs avfs_fllobjs;
	struct nvgpu_clk_domains clk_domainobjs;
	struct nvgpu_clk_progs clk_progobjs;
	struct nvgpu_clk_vf_points clk_vf_pointobjs;
	struct nvgpu_clk_mclk_state clk_mclk;
	struct nvgpu_clk_freq_controllers clk_freq_controllers;
	struct nvgpu_clk_freq_domain_grp freq_domain_grp_objs;
};

int nvgpu_clk_init_pmupstate(struct gk20a *g);
void nvgpu_clk_free_pmupstate(struct gk20a *g);
int nvgpu_clk_set_fll_clk_gv10x(struct gk20a *g);
int nvgpu_clk_set_boot_fll_clk_gv10x(struct gk20a *g);
int nvgpu_clk_set_boot_fll_clk_tu10x(struct gk20a *g);

#endif /* NVGPU_PMU_CLK_H */
