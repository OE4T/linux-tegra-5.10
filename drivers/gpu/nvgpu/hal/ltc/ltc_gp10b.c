/*
 * GP10B L2
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

#include <trace/events/gk20a.h>

#include <nvgpu/ltc.h>
#include <nvgpu/log.h>
#include <nvgpu/enabled.h>
#include <nvgpu/io.h>
#include <nvgpu/timers.h>
#include <nvgpu/gk20a.h>

#include <nvgpu/hw/gp10b/hw_ltc_gp10b.h>

#include "ltc_gm20b.h"
#include "ltc_gp10b.h"

int gp10b_determine_L2_size_bytes(struct gk20a *g)
{
	u32 tmp;
	int ret;

	nvgpu_log_fn(g, " ");

	tmp = gk20a_readl(g, ltc_ltc0_lts0_tstg_info_1_r());

	ret = g->ltc_count *
		ltc_ltc0_lts0_tstg_info_1_slice_size_in_kb_v(tmp) * 1024U *
		ltc_ltc0_lts0_tstg_info_1_slices_per_l2_v(tmp);

	nvgpu_log(g, gpu_dbg_info, "L2 size: %d\n", ret);

	nvgpu_log_fn(g, "done");

	return ret;
}

void gp10b_ltc_lts_isr(struct gk20a *g,	unsigned int ltc, unsigned int slice)
{
	u32 offset;
	u32 ltc_intr;
	u32 ltc_stride = nvgpu_get_litter_value(g, GPU_LIT_LTC_STRIDE);
	u32 lts_stride = nvgpu_get_litter_value(g, GPU_LIT_LTS_STRIDE);

	offset = ltc_stride * ltc + lts_stride * slice;
	ltc_intr = gk20a_readl(g, ltc_ltc0_lts0_intr_r() + offset);

	/* Detect and handle ECC errors */
	if ((ltc_intr &
	     ltc_ltcs_ltss_intr_ecc_sec_error_pending_f()) != 0U) {
		u32 ecc_stats_reg_val;

		nvgpu_err(g,
			"Single bit error detected in GPU L2!");

		ecc_stats_reg_val =
			gk20a_readl(g,
				ltc_ltc0_lts0_dstg_ecc_report_r() + offset);
		g->ecc.ltc.ecc_sec_count[ltc][slice].counter +=
			ltc_ltc0_lts0_dstg_ecc_report_sec_count_v(ecc_stats_reg_val);
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
			gk20a_readl(g,
				ltc_ltc0_lts0_dstg_ecc_report_r() + offset);
		g->ecc.ltc.ecc_ded_count[ltc][slice].counter +=
			ltc_ltc0_lts0_dstg_ecc_report_ded_count_v(ecc_stats_reg_val);
		ecc_stats_reg_val &=
			~(ltc_ltc0_lts0_dstg_ecc_report_ded_count_m());
		nvgpu_writel_check(g,
			ltc_ltc0_lts0_dstg_ecc_report_r() + offset,
			ecc_stats_reg_val);
	}

	nvgpu_err(g, "ltc%d, slice %d: %08x",
		  ltc, slice, ltc_intr);
	nvgpu_writel_check(g, ltc_ltc0_lts0_intr_r() +
		ltc_stride * ltc + lts_stride * slice,
		ltc_intr);
}

void gp10b_ltc_isr(struct gk20a *g, unsigned int ltc)
{
	unsigned int slice;

	for (slice = 0U; slice < g->slices_per_ltc; slice++) {
		gp10b_ltc_lts_isr(g, ltc, slice);
	}
}

void gp10b_ltc_init_fs_state(struct gk20a *g)
{
	u32 ltc_intr;

	gm20b_ltc_init_fs_state(g);

	gk20a_writel(g, ltc_ltca_g_axi_pctrl_r(),
			ltc_ltca_g_axi_pctrl_user_sid_f(g->ltc_streamid));

	/* Enable ECC interrupts */
	ltc_intr = gk20a_readl(g, ltc_ltcs_ltss_intr_r());
	ltc_intr |= ltc_ltcs_ltss_intr_en_ecc_sec_error_enabled_f() |
			ltc_ltcs_ltss_intr_en_ecc_ded_error_enabled_f();
	gk20a_writel(g, ltc_ltcs_ltss_intr_r(),
			ltc_intr);
}

void gp10b_ltc_set_enabled(struct gk20a *g, bool enabled)
{
	u32 reg_f = ltc_ltcs_ltss_tstg_set_mgmt_2_l2_bypass_mode_enabled_f();
	u32 reg = gk20a_readl(g, ltc_ltcs_ltss_tstg_set_mgmt_2_r());

	if (enabled) {
		/* bypass disabled (normal caching ops) */
		reg &= ~reg_f;
	} else {
		/* bypass enabled (no caching) */
		reg |= reg_f;
	}

	nvgpu_writel_check(g, ltc_ltcs_ltss_tstg_set_mgmt_2_r(), reg);
}
