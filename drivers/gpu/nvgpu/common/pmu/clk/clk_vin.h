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

#ifndef NVGPU_CLK_VIN_H
#define NVGPU_CLK_VIN_H

#include <nvgpu/boardobj.h>
#include <nvgpu/boardobjgrp.h>
#include <nvgpu/boardobjgrp_e32.h>
#include <nvgpu/pmu/clk/clk_vin.h>

struct vin_device_v20 {
	struct nvgpu_vin_device super;
	struct ctrl_clk_vin_device_info_data_v20 data;
};

int clk_vin_init_pmupstate(struct gk20a *g);
void clk_vin_free_pmupstate(struct gk20a *g);
int clk_pmu_vin_load(struct gk20a *g);
int clk_vin_sw_setup(struct gk20a *g);
int clk_vin_pmu_setup(struct gk20a *g);
#endif /* NVGPU_CLK_VIN_H */
