/*
 * GM20B L2
 *
 * Copyright (c) 2014-2019 NVIDIA CORPORATION.  All rights reserved.
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

#ifdef CONFIG_NVGPU_TRACE
#include <trace/events/gk20a.h>
#endif

#include <nvgpu/timers.h>
#include <nvgpu/enabled.h>
#include <nvgpu/bug.h>
#include <nvgpu/ltc.h>
#include <nvgpu/fbp.h>
#include <nvgpu/io.h>
#include <nvgpu/utils.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/safe_ops.h>

#include <nvgpu/hw/gm20b/hw_ltc_gm20b.h>
#include <nvgpu/hw/gm20b/hw_top_gm20b.h>
#include <nvgpu/hw/gm20b/hw_pri_ringmaster_gm20b.h>

#include "ltc_gm20b.h"

void gm20b_ltc_init_fs_state(struct gk20a *g)
{
	u32 reg;
	u32 line_size = 512U;

	nvgpu_log_info(g, "initialize gm20b l2");

	g->ltc->max_ltc_count = gk20a_readl(g, top_num_ltcs_r());
	g->ltc->ltc_count = gk20a_readl(g, pri_ringmaster_enum_ltc_r());
	nvgpu_log_info(g, "%d ltcs out of %d", g->ltc->ltc_count,
					g->ltc->max_ltc_count);

	reg = gk20a_readl(g, ltc_ltcs_ltss_cbc_param_r());
	g->ltc->slices_per_ltc = ltc_ltcs_ltss_cbc_param_slices_per_ltc_v(reg);
	g->ltc->cacheline_size =
		line_size << ltc_ltcs_ltss_cbc_param_cache_line_size_v(reg);

	gk20a_writel(g, ltc_ltcs_ltss_cbc_num_active_ltcs_r(),
	g->ltc->ltc_count);
	gk20a_writel(g, ltc_ltcs_misc_ltc_num_active_ltcs_r(),
	g->ltc->ltc_count);

	gk20a_writel(g, ltc_ltcs_ltss_dstg_cfg0_r(),
		     gk20a_readl(g, ltc_ltc0_lts0_dstg_cfg0_r()) |
		     ltc_ltcs_ltss_dstg_cfg0_vdc_4to2_disable_m());

	g->ops.ltc.intr.configure(g);
}

/*
 * Performs a full flush of the L2 cache.
 */

u64 gm20b_determine_L2_size_bytes(struct gk20a *g)
{
	u32 lts_per_ltc;
	u32 ways;
	u32 sets;
	u32 bytes_per_line;
	u32 active_ltcs;
	u64 cache_size;

	u32 tmp;
	u32 active_sets_value;

	tmp = gk20a_readl(g, ltc_ltc0_lts0_tstg_cfg1_r());
	ways = (u32)hweight32(ltc_ltc0_lts0_tstg_cfg1_active_ways_v(tmp));

	active_sets_value = ltc_ltc0_lts0_tstg_cfg1_active_sets_v(tmp);
	if (active_sets_value == ltc_ltc0_lts0_tstg_cfg1_active_sets_all_v()) {
		sets = 64U;
	} else if (active_sets_value ==
		 ltc_ltc0_lts0_tstg_cfg1_active_sets_half_v()) {
		sets = 32U;
	} else if (active_sets_value ==
		 ltc_ltc0_lts0_tstg_cfg1_active_sets_quarter_v()) {
		sets = 16U;
	} else {
		nvgpu_err(g, "Unknown constant %d for active sets",
		       active_sets_value);
		sets = 0U;
	}

	active_ltcs = nvgpu_fbp_get_num_fbps(g->fbp);

	/* chip-specific values */
	lts_per_ltc = 2U;
	bytes_per_line = 128U;
	cache_size = nvgpu_safe_mult_u64(nvgpu_safe_mult_u64(
			nvgpu_safe_mult_u64(active_ltcs, lts_per_ltc), ways),
			nvgpu_safe_mult_u64(sets, bytes_per_line));

	return cache_size;
}

#ifdef CONFIG_NVGPU_GRAPHICS
/*
 * Sets the ZBC color for the passed index.
 */
void gm20b_ltc_set_zbc_color_entry(struct gk20a *g,
					  u32 *color_l2,
					  u32 index)
{
	u32 i;

	nvgpu_writel_check(g, ltc_ltcs_ltss_dstg_zbc_index_r(),
		     ltc_ltcs_ltss_dstg_zbc_index_address_f(index));

	for (i = 0;
	     i < ltc_ltcs_ltss_dstg_zbc_color_clear_value__size_1_v(); i++) {
		nvgpu_writel_check(g,
			ltc_ltcs_ltss_dstg_zbc_color_clear_value_r(i),
			color_l2[i]);
	}
}

/*
 * Sets the ZBC depth for the passed index.
 */
void gm20b_ltc_set_zbc_depth_entry(struct gk20a *g,
					  u32 depth_val,
					  u32 index)
{
	nvgpu_writel_check(g, ltc_ltcs_ltss_dstg_zbc_index_r(),
		     ltc_ltcs_ltss_dstg_zbc_index_address_f(index));

	nvgpu_writel_check(g,
			ltc_ltcs_ltss_dstg_zbc_depth_clear_value_r(),
			depth_val);
}
#endif /* CONFIG_NVGPU_GRAPHICS */

void gm20b_ltc_set_enabled(struct gk20a *g, bool enabled)
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

	gk20a_writel(g, ltc_ltcs_ltss_tstg_set_mgmt_2_r(), reg);
}

#ifdef CONFIG_NVGPU_DEBUGGER
/*
 * LTC pri addressing
 */
bool gm20b_ltc_pri_is_ltc_addr(struct gk20a *g, u32 addr)
{
	return ((addr >= ltc_pltcg_base_v()) && (addr < ltc_pltcg_extent_v()));
}

bool gm20b_ltc_is_ltcs_ltss_addr(struct gk20a *g, u32 addr)
{
	u32 ltc_shared_base = ltc_ltcs_ltss_v();
	u32 lts_stride = nvgpu_get_litter_value(g, GPU_LIT_LTS_STRIDE);

	if (addr >= ltc_shared_base) {
		return (addr < nvgpu_safe_add_u32(ltc_shared_base, lts_stride));
	}
	return false;
}

bool gm20b_ltc_is_ltcn_ltss_addr(struct gk20a *g, u32 addr)
{
	u32 lts_shared_base = ltc_ltc0_ltss_v();
	u32 lts_stride = nvgpu_get_litter_value(g, GPU_LIT_LTS_STRIDE);
	u32 addr_mask = nvgpu_get_litter_value(g, GPU_LIT_LTC_STRIDE) - 1U;
	u32 base_offset = lts_shared_base & addr_mask;
	u32 end_offset = nvgpu_safe_add_u32(base_offset, lts_stride);

	return (!gm20b_ltc_is_ltcs_ltss_addr(g, addr)) &&
		((addr & addr_mask) >= base_offset) &&
		((addr & addr_mask) < end_offset);
}

static void gm20b_ltc_update_ltc_lts_addr(struct gk20a *g, u32 addr,
		u32 ltc_num, u32 *priv_addr_table, u32 *priv_addr_table_index)
{
	u32 num_ltc_slices = g->ops.top.get_max_lts_per_ltc(g);
	u32 index = *priv_addr_table_index;
	u32 lts_num;
	u32 ltc_stride = nvgpu_get_litter_value(g, GPU_LIT_LTC_STRIDE);
	u32 lts_stride = nvgpu_get_litter_value(g, GPU_LIT_LTS_STRIDE);

	for (lts_num = 0; lts_num < num_ltc_slices;
				lts_num = nvgpu_safe_add_u32(lts_num, 1U)) {
		priv_addr_table[index] = nvgpu_safe_add_u32(
			ltc_ltc0_lts0_v(),
			nvgpu_safe_add_u32(
				nvgpu_safe_add_u32(
				nvgpu_safe_mult_u32(ltc_num, ltc_stride),
				nvgpu_safe_mult_u32(lts_num, lts_stride)),
						(addr & nvgpu_safe_sub_u32(
							lts_stride, 1U))));
		index = nvgpu_safe_add_u32(index, 1U);
	}

	*priv_addr_table_index = index;
}

void gm20b_ltc_split_lts_broadcast_addr(struct gk20a *g, u32 addr,
					u32 *priv_addr_table,
					u32 *priv_addr_table_index)
{
	u32 num_ltc = g->ltc->ltc_count;
	u32 i, start, ltc_num = 0;
	u32 pltcg_base = ltc_pltcg_base_v();
	u32 ltc_stride = nvgpu_get_litter_value(g, GPU_LIT_LTC_STRIDE);

	for (i = 0; i < num_ltc; i++) {
		start = nvgpu_safe_add_u32(pltcg_base,
				nvgpu_safe_mult_u32(i, ltc_stride));
		if (addr >= start) {
			if (addr < nvgpu_safe_add_u32(start, ltc_stride)) {
				ltc_num = i;
				break;
			}
		}
	}
	gm20b_ltc_update_ltc_lts_addr(g, addr, ltc_num, priv_addr_table,
				priv_addr_table_index);
}

void gm20b_ltc_split_ltc_broadcast_addr(struct gk20a *g, u32 addr,
					u32 *priv_addr_table,
					u32 *priv_addr_table_index)
{
	u32 num_ltc = g->ltc->ltc_count;
	u32 ltc_num;

	for (ltc_num = 0; ltc_num < num_ltc; ltc_num =
					nvgpu_safe_add_u32(ltc_num, 1U)) {
		gm20b_ltc_update_ltc_lts_addr(g, addr, ltc_num,
					priv_addr_table, priv_addr_table_index);
	}
}
#endif /* CONFIG_NVGPU_DEBUGGER */
