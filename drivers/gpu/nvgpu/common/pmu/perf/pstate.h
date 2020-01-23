/*
 * general p state infrastructure
 *
 * Copyright (c) 2019-2020, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_PERF_PSTATE_H
#define NVGPU_PERF_PSTATE_H

#define CTRL_PERF_PSTATE_TYPE_35	0x05U

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

int perf_pstate_sw_setup(struct gk20a *g);
int perf_pstate_pmu_setup(struct gk20a *g);

#endif /* NVGPU_PERF_PSTATE_H */
