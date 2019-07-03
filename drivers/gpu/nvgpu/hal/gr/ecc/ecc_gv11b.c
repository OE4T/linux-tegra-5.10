/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/io.h>
#include <nvgpu/ecc.h>
#include <nvgpu/gk20a.h>

#include <nvgpu/hw/gv11b/hw_gr_gv11b.h>

#include "ecc_gv11b.h"

int gv11b_gr_intr_inject_fecs_ecc_error(struct gk20a *g,
		struct nvgpu_hw_err_inject_info *err, u32 error_info)
{
	nvgpu_info(g, "Injecting FECS fault %s", err->name);
	nvgpu_writel(g, err->get_reg_addr(), err->get_reg_val(1U));

	return 0;
}

int gv11b_gr_intr_inject_gpccs_ecc_error(struct gk20a *g,
		struct nvgpu_hw_err_inject_info *err, u32 error_info)
{
	unsigned int gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_GPC_STRIDE);
	unsigned int gpc = (error_info & 0xFFU);
	unsigned int reg_addr = nvgpu_safe_add_u32(err->get_reg_addr(),
					nvgpu_safe_mult_u32(gpc , gpc_stride));

	nvgpu_info(g, "Injecting GPCCS fault %s for gpc: %d", err->name, gpc);
	nvgpu_writel(g, reg_addr, err->get_reg_val(1U));

	return 0;
}

int gv11b_gr_intr_inject_sm_ecc_error(struct gk20a *g,
		struct nvgpu_hw_err_inject_info *err,
		u32 error_info)
{
	unsigned int gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_GPC_STRIDE);
	unsigned int tpc_stride =
		nvgpu_get_litter_value(g, GPU_LIT_TPC_IN_GPC_STRIDE);
	unsigned int gpc = (error_info & 0xFF00U) >> 8U;
	unsigned int tpc = (error_info & 0xFFU);
	unsigned int reg_addr = nvgpu_safe_add_u32(err->get_reg_addr(),
					nvgpu_safe_add_u32(
					nvgpu_safe_mult_u32(gpc , gpc_stride),
					nvgpu_safe_mult_u32(tpc , tpc_stride)));

	nvgpu_info(g, "Injecting SM fault %s for gpc: %d, tpc: %d",
			err->name, gpc, tpc);
	nvgpu_writel(g, reg_addr, err->get_reg_val(1U));

	return 0;
}

int gv11b_gr_intr_inject_mmu_ecc_error(struct gk20a *g,
		struct nvgpu_hw_err_inject_info *err, u32 error_info)
{
	unsigned int gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_GPC_STRIDE);
	unsigned int gpc = (error_info & 0xFFU);
	unsigned int reg_addr = nvgpu_safe_add_u32(err->get_reg_addr(),
					nvgpu_safe_mult_u32(gpc , gpc_stride));

	nvgpu_info(g, "Injecting MMU fault %s for gpc: %d", err->name, gpc);
	nvgpu_writel(g, reg_addr, err->get_reg_val(1U));

	return 0;
}

int gv11b_gr_intr_inject_gcc_ecc_error(struct gk20a *g,
		struct nvgpu_hw_err_inject_info *err, u32 error_info)
{
	unsigned int gpc_stride = nvgpu_get_litter_value(g,
			GPU_LIT_GPC_STRIDE);
	unsigned int gpc = (error_info & 0xFFU);
	unsigned int reg_addr = nvgpu_safe_add_u32(err->get_reg_addr(),
					nvgpu_safe_mult_u32(gpc , gpc_stride));

	nvgpu_info(g, "Injecting GCC fault %s for gpc: %d", err->name, gpc);
	nvgpu_writel(g, reg_addr, err->get_reg_val(1U));

	return 0;
}
