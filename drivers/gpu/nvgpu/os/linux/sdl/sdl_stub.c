/*
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

#include <nvgpu/nvgpu_err.h>

struct gk20a;
struct mmu_fault_info;

int nvgpu_report_host_err(struct gk20a *g, u32 hw_unit,
	u32 inst, u32 err_id, u32 intr_info)
{
	return 0;
}

int nvgpu_report_ecc_err(struct gk20a *g, u32 hw_unit, u32 inst,
		u32 err_type, u64 err_addr, u64 err_count)
{
	return 0;
}

int nvgpu_report_gr_err(struct gk20a *g, u32 hw_unit, u32 inst,
		u32 err_type, struct gr_err_info *err_info, u32 sub_err_type)
{
	return 0;
}

int nvgpu_report_pmu_err(struct gk20a *g, u32 hw_unit, u32 err_id,
	u32 sub_err_type, u32 status)
{
	return 0;
}

int nvgpu_report_ce_err(struct gk20a *g, u32 hw_unit,
	u32 inst, u32 err_id, u32 intr_info)
{
	return 0;
}

int nvgpu_report_pri_err(struct gk20a *g, u32 hw_unit, u32 inst,
		u32 err_type, u32 err_addr, u32 err_code)
{
	return 0;
}

int nvgpu_report_ctxsw_err(struct gk20a *g, u32 hw_unit, u32 err_id,
		void *data)
{
	return 0;
}

int nvgpu_report_mmu_err(struct gk20a *g, u32 hw_unit,
		u32 err_type, struct mmu_fault_info *fault_info,
		u32 status, u32 sub_err_type)
{
	return 0;
}
