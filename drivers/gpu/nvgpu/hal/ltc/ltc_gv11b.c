/*
 * GV11B LTC
 *
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

#include <nvgpu/io.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/nvgpu_err.h>
#include <nvgpu/static_analysis.h>
#include <nvgpu/gr/zbc.h>

#include "ltc_gv11b.h"

#include <nvgpu/hw/gv11b/hw_ltc_gv11b.h>

#include <nvgpu/utils.h>

#ifdef CONFIG_NVGPU_GRAPHICS
/*
 * Sets the ZBC stencil for the passed index.
 */
void gv11b_ltc_set_zbc_stencil_entry(struct gk20a *g,
					  u32 stencil_depth,
					  u32 index)
{
	nvgpu_writel_check(g, ltc_ltcs_ltss_dstg_zbc_index_r(),
		ltc_ltcs_ltss_dstg_zbc_index_address_f(
			nvgpu_safe_add_u32(index, NVGPU_GR_ZBC_STARTOF_TABLE)));

	nvgpu_writel_check(g,
			   ltc_ltcs_ltss_dstg_zbc_stencil_clear_value_r(),
			   stencil_depth);
}
#endif /* CONFIG_NVGPU_GRAPHICS */

#ifdef CONFIG_NVGPU_INJECT_HWERR
void gv11b_ltc_inject_ecc_error(struct gk20a *g,
		struct nvgpu_hw_err_inject_info *err, u32 error_info)
{
	u32 ltc_stride = nvgpu_get_litter_value(g, GPU_LIT_LTC_STRIDE);
	u32 lts_stride = nvgpu_get_litter_value(g, GPU_LIT_LTS_STRIDE);
	u32 ltc = (error_info & 0xFF00U) >> 8U;
	u32 lts = (error_info & 0xFFU);
	u32 reg_addr = nvgpu_safe_add_u32(err->get_reg_addr(),
			nvgpu_safe_add_u32(nvgpu_safe_mult_u32(ltc, ltc_stride),
					nvgpu_safe_mult_u32(lts, lts_stride)));

	nvgpu_info(g, "Injecting LTC fault %s for ltc: %d, lts: %d",
			err->name, ltc, lts);
	nvgpu_writel(g, reg_addr, err->get_reg_val(1U));
}

static inline u32 ltc0_lts0_l1_cache_ecc_control_r(void)
{
	return ltc_ltc0_lts0_l1_cache_ecc_control_r();
}

static inline u32 ltc0_lts0_l1_cache_ecc_control_inject_corrected_err_f(u32 v)
{
	return ltc_ltc0_lts0_l1_cache_ecc_control_inject_corrected_err_f(v);
}

static inline u32 ltc0_lts0_l1_cache_ecc_control_inject_uncorrected_err_f(u32 v)
{
	return ltc_ltc0_lts0_l1_cache_ecc_control_inject_uncorrected_err_f(v);
}

static struct nvgpu_hw_err_inject_info ltc_ecc_err_desc[] = {
	NVGPU_ECC_ERR("cache_rstg_ecc_corrected",
			gv11b_ltc_inject_ecc_error,
			ltc0_lts0_l1_cache_ecc_control_r,
			ltc0_lts0_l1_cache_ecc_control_inject_corrected_err_f),
	NVGPU_ECC_ERR("cache_rstg_ecc_uncorrected",
			gv11b_ltc_inject_ecc_error,
			ltc0_lts0_l1_cache_ecc_control_r,
			ltc0_lts0_l1_cache_ecc_control_inject_uncorrected_err_f),
};

static struct nvgpu_hw_err_inject_info_desc ltc_err_desc;

struct nvgpu_hw_err_inject_info_desc *gv11b_ltc_get_err_desc(struct gk20a *g)
{
	ltc_err_desc.info_ptr = ltc_ecc_err_desc;
	ltc_err_desc.info_size = nvgpu_safe_cast_u64_to_u32(
			sizeof(ltc_ecc_err_desc) /
			sizeof(struct nvgpu_hw_err_inject_info));

	return &ltc_err_desc;
}
#endif /* CONFIG_NVGPU_INJECT_HWERR */
