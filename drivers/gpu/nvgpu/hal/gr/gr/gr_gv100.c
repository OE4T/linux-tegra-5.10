/*
 * GV100 GPU GR
 *
 * Copyright (c) 2017-2020, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/log.h>
#include <nvgpu/debug.h>
#include <nvgpu/enabled.h>
#include <nvgpu/io.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/gr/ctx.h>
#include <nvgpu/gr/config.h>
#include <nvgpu/fbp.h>

#include "gr_gk20a.h"
#include "gr_pri_gk20a.h"
#include "gr_gv100.h"

#include "common/gr/gr_priv.h"

#include <nvgpu/hw/gv100/hw_gr_gv100.h>
#include <nvgpu/hw/gv100/hw_perf_gv100.h>

#ifdef CONFIG_NVGPU_TEGRA_FUSE
void gr_gv100_set_gpc_tpc_mask(struct gk20a *g, u32 gpc_index)
{
}
#endif

static u32 gr_gv100_get_active_fbpa_mask(struct gk20a *g)
{
	u32 active_fbpa_mask;
	u32 num_fbpas;

	num_fbpas = g->ops.top.get_max_fbpas_count(g);

	/*
	 * Read active fbpa mask from fuse
	 * Note that 0:enable and 1:disable in value read from fuse so we've to
	 * flip the bits.
	 * Also set unused bits to zero
	 */
	active_fbpa_mask = g->ops.fuse.fuse_status_opt_fbio(g);
	active_fbpa_mask = ~active_fbpa_mask;
	active_fbpa_mask = active_fbpa_mask & (BIT32(num_fbpas) - 1U);

	return active_fbpa_mask;
}

void gr_gv100_split_fbpa_broadcast_addr(struct gk20a *g, u32 addr,
				      u32 num_fbpas,
				      u32 *priv_addr_table, u32 *t)
{
	u32 active_fbpa_mask;
	u32 fbpa_id;

	active_fbpa_mask = gr_gv100_get_active_fbpa_mask(g);

	for (fbpa_id = 0; fbpa_id < num_fbpas; fbpa_id++) {
		if ((active_fbpa_mask & BIT32(fbpa_id)) != 0U) {
			priv_addr_table[(*t)++] = pri_fbpa_addr(g,
					pri_fbpa_addr_mask(g, addr), fbpa_id);
		}
	}
}

void gr_gv100_set_pmm_register(struct gk20a *g, u32 offset, u32 val,
				u32 num_chiplets, u32 num_perfmons)
{
	u32 perfmon_index = 0;
	u32 chiplet_index = 0;
	u32 reg_offset = 0;
	u32 chiplet_stride = g->ops.perf.get_pmm_per_chiplet_offset();

	for (chiplet_index = 0; chiplet_index < num_chiplets; chiplet_index++) {
		for (perfmon_index = 0; perfmon_index < num_perfmons;
						perfmon_index++) {
			reg_offset = offset + perfmon_index *
				perf_pmmsys_perdomain_offset_v() +
				chiplet_index * chiplet_stride;
			nvgpu_writel(g, reg_offset, val);
		}
	}
}

void gr_gv100_get_num_hwpm_perfmon(struct gk20a *g, u32 *num_sys_perfmon,
				u32 *num_fbp_perfmon, u32 *num_gpc_perfmon)
{
	int err;
	u32 buf_offset_lo, buf_offset_addr, num_offsets;
	u32 perfmon_index = 0;

	for (perfmon_index = 0; perfmon_index <
			perf_pmmsys_engine_sel__size_1_v();
			perfmon_index++) {
		err = gr_gk20a_get_pm_ctx_buffer_offsets(g,
				perf_pmmsys_engine_sel_r(perfmon_index),
				1,
				&buf_offset_lo,
				&buf_offset_addr,
				&num_offsets);
		if (err != 0) {
			break;
		}
	}
	*num_sys_perfmon = perfmon_index;

	for (perfmon_index = 0; perfmon_index <
			perf_pmmfbp_engine_sel__size_1_v();
			perfmon_index++) {
		err = gr_gk20a_get_pm_ctx_buffer_offsets(g,
				perf_pmmfbp_engine_sel_r(perfmon_index),
				1,
				&buf_offset_lo,
				&buf_offset_addr,
				&num_offsets);
		if (err != 0) {
			break;
		}
	}
	*num_fbp_perfmon = perfmon_index;

	for (perfmon_index = 0; perfmon_index <
			perf_pmmgpc_engine_sel__size_1_v();
			perfmon_index++) {
		err = gr_gk20a_get_pm_ctx_buffer_offsets(g,
				perf_pmmgpc_engine_sel_r(perfmon_index),
				1,
				&buf_offset_lo,
				&buf_offset_addr,
				&num_offsets);
		if (err != 0) {
			break;
		}
	}
	*num_gpc_perfmon = perfmon_index;
}

void gr_gv100_init_hwpm_pmm_register(struct gk20a *g)
{
	u32 num_sys_perfmon = 0;
	u32 num_fbp_perfmon = 0;
	u32 num_gpc_perfmon = 0;

	g->ops.gr.get_num_hwpm_perfmon(g, &num_sys_perfmon,
				&num_fbp_perfmon, &num_gpc_perfmon);

	g->ops.gr.set_pmm_register(g, perf_pmmsys_engine_sel_r(0),
		0xFFFFFFFFU, 1U, num_sys_perfmon);
	g->ops.gr.set_pmm_register(g, perf_pmmfbp_engine_sel_r(0),
		0xFFFFFFFFU, nvgpu_fbp_get_num_fbps(g->fbp), num_fbp_perfmon);
	g->ops.gr.set_pmm_register(g, perf_pmmgpc_engine_sel_r(0),
		0xFFFFFFFFU, nvgpu_gr_config_get_gpc_count(g->gr->config),
		num_gpc_perfmon);
}
