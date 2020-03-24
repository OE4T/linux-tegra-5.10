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
#ifndef CLK_MON_TU104_H
#define CLK_MON_TU104_H

/**
 * FMON register types
 */
#define FMON_THRESHOLD_HIGH				0x0U
#define FMON_THRESHOLD_LOW				0x1U
#define FMON_FAULT_STATUS				0x2U
#define FMON_FAULT_STATUS_PRIV_MASK			0x3U
#define CLK_CLOCK_MON_REG_TYPE_COUNT			0x4U
#define CLK_MON_BITS_PER_BYTE				0x8U

bool nvgpu_clk_mon_check_master_fault_status(struct gk20a *g);
int nvgpu_clk_mon_check_status(struct gk20a *g, struct
		clk_domains_mon_status_params *clk_mon_status,
		u32 domain_mask);
bool nvgpu_clk_mon_check_clk_good(struct gk20a *g);
bool nvgpu_clk_mon_check_pll_lock(struct gk20a *g);
#endif /* CLK_MON_TU104_H */
