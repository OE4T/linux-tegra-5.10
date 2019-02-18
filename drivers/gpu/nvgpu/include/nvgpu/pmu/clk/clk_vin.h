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

#ifndef NVGPU_PMU_CLK_VIN_H
#define NVGPU_PMU_CLK_VIN_H

#include <nvgpu/boardobjgrp_e32.h>
#include <nvgpu/types.h>

struct gk20a;
struct vin_device_v20;

struct nvgpu_avfsvinobjs {
	struct boardobjgrp_e32 super;
	u8 calibration_rev_vbios;
	u8 calibration_rev_fused;
	bool vin_is_disable_allowed;
};

int nvgpu_clk_pmu_vin_load(struct gk20a *g);
int nvgpu_clk_vin_sw_setup(struct gk20a *g);
int nvgpu_clk_vin_pmu_setup(struct gk20a *g);
int nvgpu_clk_avfs_get_vin_cal_fuse_v10(struct gk20a *g,
	struct nvgpu_avfsvinobjs *pvinobjs,
	struct vin_device_v20 *pvindev);
int nvgpu_clk_avfs_get_vin_cal_fuse_v20(struct gk20a *g,
	struct nvgpu_avfsvinobjs *pvinobjs,
	struct vin_device_v20 *pvindev);

#endif /* NVGPU_PMU_CLK_VIN_H */
