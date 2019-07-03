/*
 * GV11B LTC
 *
 * Copyright (c) 2016-2019, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/safe_ops.h>

#include "ltc_gv11b.h"

#include <nvgpu/hw/gv11b/hw_ltc_gv11b.h>
#include <nvgpu/hw/gv11b/hw_top_gv11b.h>

#include <nvgpu/utils.h>

static struct nvgpu_hw_err_inject_info ltc_ecc_err_desc[] = {
	NVGPU_ECC_ERR("cache_rstg_ecc_corrected",
			gv11b_ltc_inject_ecc_error,
			ltc_ltc0_lts0_l1_cache_ecc_control_r,
			ltc_ltc0_lts0_l1_cache_ecc_control_inject_corrected_err_f),
	NVGPU_ECC_ERR("cache_rstg_ecc_uncorrected",
			gv11b_ltc_inject_ecc_error,
			ltc_ltc0_lts0_l1_cache_ecc_control_r,
			ltc_ltc0_lts0_l1_cache_ecc_control_inject_uncorrected_err_f),
};

static struct nvgpu_hw_err_inject_info_desc ltc_err_desc;

struct nvgpu_hw_err_inject_info_desc * gv11b_ltc_get_err_desc(struct gk20a *g)
{
	ltc_err_desc.info_ptr = ltc_ecc_err_desc;
	ltc_err_desc.info_size = nvgpu_safe_cast_u64_to_u32(
			sizeof(ltc_ecc_err_desc) /
			sizeof(struct nvgpu_hw_err_inject_info));

	return &ltc_err_desc;
}

/*
 * Sets the ZBC stencil for the passed index.
 */

void gv11b_ltc_init_fs_state(struct gk20a *g)
{
	u32 reg;
	u32 line_size = 512U;

	nvgpu_log_info(g, "initialize gv11b l2");

	g->ltc->max_ltc_count = gk20a_readl(g, top_num_ltcs_r());
	g->ltc->ltc_count = g->ops.priv_ring.enum_ltc(g);
	nvgpu_log_info(g, "%u ltcs out of %u", g->ltc->ltc_count,
					g->ltc->max_ltc_count);

	reg = gk20a_readl(g, ltc_ltcs_ltss_cbc_param_r());
	g->ltc->slices_per_ltc = ltc_ltcs_ltss_cbc_param_slices_per_ltc_v(reg);;
	g->ltc->cacheline_size =
		line_size << ltc_ltcs_ltss_cbc_param_cache_line_size_v(reg);

	g->ops.ltc.intr.configure(g);

}
