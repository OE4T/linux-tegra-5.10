/*
 * TU104 FBPA
 *
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

#include <nvgpu/types.h>
#include <nvgpu/io.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/hw/tu104/hw_fbpa_tu104.h>

#include "hal/fbpa/fbpa_tu104.h"

int tu104_fbpa_init(struct gk20a *g)
{
	u32 val;

	val = gk20a_readl(g, fbpa_ecc_intr_ctrl_r());
	val |= fbpa_ecc_intr_ctrl_sec_intr_en_enabled_f() |
		fbpa_ecc_intr_ctrl_ded_intr_en_enabled_f();
	gk20a_writel(g, fbpa_ecc_intr_ctrl_r(), val);
	/* read back broadcast register */
	(void) gk20a_readl(g, fbpa_ecc_intr_ctrl_r());

	return 0;
}

static void tu104_fbpa_handle_ecc_intr(struct gk20a *g,
					u32 fbpa_id, u32 subp_id)
{
	u32 status, sec_cnt, ded_cnt;
	u32 offset = nvgpu_get_litter_value(g, GPU_LIT_FBPA_STRIDE) * fbpa_id;
	u32 cnt_idx = fbpa_id * 2U + subp_id;

	status = gk20a_readl(g, offset + fbpa_0_ecc_status_r(subp_id));

	if ((status & fbpa_0_ecc_status_sec_counter_overflow_pending_f()) != 0U) {
		nvgpu_err(g, "fbpa %u subp %u ecc sec counter overflow",
				fbpa_id, subp_id);
	}

	if ((status & fbpa_0_ecc_status_ded_counter_overflow_pending_f()) != 0U) {
		nvgpu_err(g, "fbpa %u subp %u ecc ded counter overflow",
				fbpa_id, subp_id);
	}

	if ((status & fbpa_0_ecc_status_sec_intr_pending_f()) != 0U) {
		sec_cnt = gk20a_readl(g,
				offset + fbpa_0_ecc_sec_count_r(subp_id));
		gk20a_writel(g, offset + fbpa_0_ecc_sec_count_r(subp_id), 0u);
		g->ecc.fbpa.fbpa_ecc_sec_err_count[cnt_idx].counter += sec_cnt;
	}

	if ((status & fbpa_0_ecc_status_ded_intr_pending_f()) != 0U) {
		ded_cnt = gk20a_readl(g,
				offset + fbpa_0_ecc_ded_count_r(subp_id));
		gk20a_writel(g, offset + fbpa_0_ecc_ded_count_r(subp_id), 0u);
		g->ecc.fbpa.fbpa_ecc_ded_err_count[cnt_idx].counter += ded_cnt;
	}

	gk20a_writel(g, offset + fbpa_0_ecc_status_r(subp_id), status);
}

void tu104_fbpa_handle_intr(struct gk20a *g, u32 fbpa_id)
{
	u32 offset, status;
	u32 ecc_subp0_mask = fbpa_0_intr_status_sec_subp0_pending_f() |
				fbpa_0_intr_status_ded_subp0_pending_f();
	u32 ecc_subp1_mask = fbpa_0_intr_status_sec_subp1_pending_f() |
				fbpa_0_intr_status_ded_subp1_pending_f();

	offset = nvgpu_get_litter_value(g, GPU_LIT_FBPA_STRIDE) * fbpa_id;

	status = gk20a_readl(g, offset + fbpa_0_intr_status_r());
	if ((status & (ecc_subp0_mask | ecc_subp1_mask)) == 0U) {
		nvgpu_err(g, "unknown interrupt fbpa %u status %08x",
				fbpa_id, status);
		return;
	}

	if ((status & ecc_subp0_mask) != 0U) {
		tu104_fbpa_handle_ecc_intr(g, fbpa_id, 0u);
	}
	if ((status & ecc_subp1_mask) != 0U) {
		tu104_fbpa_handle_ecc_intr(g, fbpa_id, 1u);
	}
}
