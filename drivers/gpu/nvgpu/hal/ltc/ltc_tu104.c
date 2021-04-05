/*
 * Copyright (c) 2018-2021, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/ltc.h>
#include <nvgpu/io.h>
#include <nvgpu/timers.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/trace.h>
#include "ltc_tu104.h"

#include "ltc_gv11b.h"

#include <nvgpu/hw/tu104/hw_ltc_tu104.h>

void ltc_tu104_init_fs_state(struct gk20a *g)
{
	u32 reg;
	u32 line_size = 512U;

	gv11b_ltc_init_fs_state(g);

	reg = nvgpu_readl(g, ltc_ltcs_ltss_cbc_param2_r());
	g->ltc->slices_per_ltc =
		ltc_ltcs_ltss_cbc_param2_slices_per_ltc_v(reg);
	g->ltc->cacheline_size =
		line_size << ltc_ltcs_ltss_cbc_param2_cache_line_size_v(reg);

	/* disable PLC compression */
	reg = nvgpu_readl(g, ltc_ltcs_ltss_tstg_set_mgmt_1_r());
	reg = set_field(reg,
		ltc_ltcs_ltss_tstg_set_mgmt_1_plc_recompress_plc_m(),
		ltc_ltcs_ltss_tstg_set_mgmt_1_plc_recompress_plc_disabled_f());
	reg = set_field(reg,
		ltc_ltcs_ltss_tstg_set_mgmt_1_plc_recompress_rmw_m(),
		ltc_ltcs_ltss_tstg_set_mgmt_1_plc_recompress_rmw_disabled_f());
	nvgpu_writel(g, ltc_ltcs_ltss_tstg_set_mgmt_1_r(), reg);
}

#ifdef CONFIG_NVGPU_DEBUGGER
u32 tu104_ltc_pri_is_lts_tstg_addr(struct gk20a *g, u32 addr)
{
	u32 ltc_stride = nvgpu_get_litter_value(g, GPU_LIT_LTC_STRIDE);
	u32 lts_stride = nvgpu_get_litter_value(g, GPU_LIT_LTS_STRIDE);
	u32 ltc_addr_mask = nvgpu_safe_sub_u32(ltc_stride, 1);
	u32 lts_addr_mask = nvgpu_safe_sub_u32(lts_stride, 1);
	u32 ltc_addr = addr & ltc_addr_mask;
	u32 lts_addr = ltc_addr & lts_addr_mask;

	return (lts_addr >= LTS_TSTG_BASE && lts_addr <= LTS_TSTG_EXTENT) ?
			    true : false;
}
#endif
