/*
 * GP10B L2 INTR
 *
 * Copyright (c) 2014-2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/ltc.h>
#include <nvgpu/log.h>
#include <nvgpu/io.h>
#include <nvgpu/gk20a.h>

#include <nvgpu/hw/gp10b/hw_ltc_gp10b.h>

#include "ltc_intr_gp10b.h"
#include "ltc_intr_gm20b.h"

void gp10b_ltc_intr_handle_lts_interrupts(struct gk20a *g, u32 ltc, u32 slice)
{
	u32 offset;
	u32 ltc_intr;
	u32 ltc_stride = nvgpu_get_litter_value(g, GPU_LIT_LTC_STRIDE);
	u32 lts_stride = nvgpu_get_litter_value(g, GPU_LIT_LTS_STRIDE);

	offset = ltc_stride * ltc + lts_stride * slice;
	ltc_intr = nvgpu_readl(g, ltc_ltc0_lts0_intr_r() + offset);

	/* Detect and handle ECC errors */
	if ((ltc_intr &
	     ltc_ltcs_ltss_intr_ecc_sec_error_pending_f()) != 0U) {
		u32 ecc_stats_reg_val;

		nvgpu_err(g,
			"Single bit error detected in GPU L2!");

		ecc_stats_reg_val =
			nvgpu_readl(g,
				ltc_ltc0_lts0_dstg_ecc_report_r() + offset);
		g->ecc.ltc.ecc_sec_count[ltc][slice].counter +=
			ltc_ltc0_lts0_dstg_ecc_report_sec_count_v(
							ecc_stats_reg_val);
		ecc_stats_reg_val &=
			~(ltc_ltc0_lts0_dstg_ecc_report_sec_count_m());
		nvgpu_writel_check(g,
			ltc_ltc0_lts0_dstg_ecc_report_r() + offset,
			ecc_stats_reg_val);
		if (g->ops.mm.l2_flush(g, true) != 0) {
			nvgpu_err(g, "l2_flush failed");
		}
	}
	if ((ltc_intr &
	     ltc_ltcs_ltss_intr_ecc_ded_error_pending_f()) != 0U) {
		u32 ecc_stats_reg_val;

		nvgpu_err(g,
			"Double bit error detected in GPU L2!");

		ecc_stats_reg_val =
			nvgpu_readl(g,
				ltc_ltc0_lts0_dstg_ecc_report_r() + offset);
		g->ecc.ltc.ecc_ded_count[ltc][slice].counter +=
			ltc_ltc0_lts0_dstg_ecc_report_ded_count_v(
							ecc_stats_reg_val);
		ecc_stats_reg_val &=
			~(ltc_ltc0_lts0_dstg_ecc_report_ded_count_m());
		nvgpu_writel_check(g,
			ltc_ltc0_lts0_dstg_ecc_report_r() + offset,
			ecc_stats_reg_val);
	}

	nvgpu_err(g, "ltc%d, slice %d: %08x", ltc, slice, ltc_intr);
	nvgpu_writel_check(g, ltc_ltc0_lts0_intr_r() +
		ltc_stride * ltc + lts_stride * slice, ltc_intr);
}

void gp10b_ltc_intr_isr(struct gk20a *g, u32 ltc)
{
	u32 slice;

	for (slice = 0U; slice < g->ltc->slices_per_ltc; slice++) {
		gp10b_ltc_intr_handle_lts_interrupts(g, ltc, slice);
	}
}

void gp10b_ltc_intr_configure(struct gk20a *g)
{
	u32 ltc_intr;

	gm20b_ltc_intr_configure(g);

	/* Enable ECC interrupts */
	ltc_intr = nvgpu_readl(g, ltc_ltcs_ltss_intr_r());
	ltc_intr |= ltc_ltcs_ltss_intr_en_ecc_sec_error_enabled_f() |
			ltc_ltcs_ltss_intr_en_ecc_ded_error_enabled_f();
	nvgpu_writel(g, ltc_ltcs_ltss_intr_r(), ltc_intr);
}
