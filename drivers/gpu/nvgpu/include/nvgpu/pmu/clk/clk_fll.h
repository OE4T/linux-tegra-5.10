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

#ifndef NVGPU_PMU_CLK_FLL_H
#define NVGPU_PMU_CLK_FLL_H

#include <nvgpu/boardobjgrp_e32.h>
#include <nvgpu/boardobjgrpmask.h>
#include <nvgpu/types.h>

struct gk20a;

struct nvgpu_avfsfllobjs {
	struct boardobjgrp_e32 super;
	struct boardobjgrpmask_e32 lut_prog_master_mask;
	u32 lut_step_size_uv;
	u32 lut_min_voltage_uv;
	u8 lut_num_entries;
	u16 max_min_freq_mhz;
};

struct nvgpu_set_fll_clk {
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

int nvgpu_clk_fll_sw_setup(struct gk20a *g);
int nvgpu_clk_fll_pmu_setup(struct gk20a *g);

#endif /* NVGPU_PMU_CLK_FLL_H */
