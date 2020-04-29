/*
* Copyright (c) 2020, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_CLK_H
#define NVGPU_CLK_H

#include <nvgpu/boardobjgrp_e255.h>
#include "ucode_clk_inf.h"

struct clk_vf_point {
	struct boardobj super;
	u8  vfe_equ_idx;
	u8  volt_rail_idx;
	struct ctrl_clk_vf_pair pair;
};

struct clk_vf_point_volt {
	struct clk_vf_point super;
	u32 source_voltage_uv;
	struct ctrl_clk_freq_delta freq_delta;
};

struct clk_vf_point_freq {
	struct clk_vf_point super;
	int volt_delta_uv;
};

struct nvgpu_clk_vf_points {
	struct boardobjgrp_e255 super;
};

struct clk_vf_point *nvgpu_construct_clk_vf_point(struct gk20a *g,
	void *pargs);
#endif /* NVGPU_CLK_H */
