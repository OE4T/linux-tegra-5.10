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

#include <trace/events/gk20a.h>

#include <nvgpu/timers.h>
#include <nvgpu/enabled.h>
#include <nvgpu/bug.h>
#include <nvgpu/ltc.h>
#include <nvgpu/fbp.h>
#include <nvgpu/io.h>
#include <nvgpu/utils.h>
#include <nvgpu/gk20a.h>

#include <nvgpu/hw/gm20b/hw_ltc_gm20b.h>
#include <nvgpu/hw/gm20b/hw_top_gm20b.h>
#include <nvgpu/hw/gm20b/hw_pri_ringmaster_gm20b.h>

#include "ltc_gm20b.h"
#include "common/gr/gr_priv.h"

void gm20b_ltc_init_fs_state(struct gk20a *g)
{
	u32 reg;

	nvgpu_log_info(g, "initialize gm20b l2");

	g->ltc->max_ltc_count = gk20a_readl(g, top_num_ltcs_r());
	g->ltc->ltc_count = gk20a_readl(g, pri_ringmaster_enum_ltc_r());
	nvgpu_log_info(g, "%d ltcs out of %d", g->ltc->ltc_count,
					g->ltc->max_ltc_count);

	reg = gk20a_readl(g, ltc_ltcs_ltss_cbc_param_r());
	g->ltc->slices_per_ltc = ltc_ltcs_ltss_cbc_param_slices_per_ltc_v(reg);
	g->ltc->cacheline_size =
		U32(512) << ltc_ltcs_ltss_cbc_param_cache_line_size_v(reg);

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
void gm20b_flush_ltc(struct gk20a *g)
{
	struct nvgpu_timeout timeout;
	u32 ltc;
	u32 ltc_stride = nvgpu_get_litter_value(g, GPU_LIT_LTC_STRIDE);
	bool is_clean_pending_set = false;
	bool is_invalidate_pending_set = false;

	/* Clean... */
	nvgpu_writel_check(g, ltc_ltcs_ltss_tstg_cmgmt1_r(),
		ltc_ltcs_ltss_tstg_cmgmt1_clean_pending_f() |
		ltc_ltcs_ltss_tstg_cmgmt1_max_cycles_between_cleans_3_f() |
		ltc_ltcs_ltss_tstg_cmgmt1_clean_wait_for_fb_to_pull_true_f() |
		ltc_ltcs_ltss_tstg_cmgmt1_clean_evict_last_class_true_f() |
		ltc_ltcs_ltss_tstg_cmgmt1_clean_evict_normal_class_true_f() |
		ltc_ltcs_ltss_tstg_cmgmt1_clean_evict_first_class_true_f());

	/* Wait on each LTC individually. */
	for (ltc = 0; ltc < g->ltc->ltc_count; ltc++) {
		u32 op_pending;

		/*
		 * Use 5ms - this should be sufficient time to flush the cache.
		 * On tegra, rough EMC BW available for old tegra chips (newer
		 * chips are strictly faster) can be estimated as follows:
		 *
		 * Lowest reasonable EMC clock speed will be around 102MHz on
		 * t124 for display enabled boards and generally fixed to max
		 * for non-display boards (since they are generally plugged in).
		 *
		 * Thus, the available BW is 64b * 2 * 102MHz = 1.3GB/s. Of that
		 * BW the GPU will likely get about half (display and overhead/
		 * utilization inefficiency eating the rest) so 650MB/s at
		 * worst. Assuming at most 1MB of GPU L2 cache (less for most
		 * chips) worst case is we take 1MB/650MB/s = 1.5ms.
		 *
		 * So 5ms timeout here should be more than sufficient.
		 */
		nvgpu_timeout_init(g, &timeout, 5, NVGPU_TIMER_CPU_TIMER);

		do {
			int cmgmt1 = ltc_ltc0_ltss_tstg_cmgmt1_r() +
				     ltc * ltc_stride;
			op_pending = gk20a_readl(g, cmgmt1);
			is_clean_pending_set = (op_pending &
				ltc_ltc0_ltss_tstg_cmgmt1_clean_pending_f()) != 0U;
		} while (is_clean_pending_set &&
			(nvgpu_timeout_expired_msg(&timeout,
					"L2 flush timeout!") == 0));
	}

	/* And invalidate. */
	nvgpu_writel_check(g, ltc_ltcs_ltss_tstg_cmgmt0_r(),
	     ltc_ltcs_ltss_tstg_cmgmt0_invalidate_pending_f() |
	     ltc_ltcs_ltss_tstg_cmgmt0_max_cycles_between_invalidates_3_f() |
	     ltc_ltcs_ltss_tstg_cmgmt0_invalidate_evict_last_class_true_f() |
	     ltc_ltcs_ltss_tstg_cmgmt0_invalidate_evict_normal_class_true_f() |
	     ltc_ltcs_ltss_tstg_cmgmt0_invalidate_evict_first_class_true_f());

	/* Wait on each LTC individually. */
	for (ltc = 0; ltc < g->ltc->ltc_count; ltc++) {
		u32 op_pending;

		/* Again, 5ms. */
		nvgpu_timeout_init(g, &timeout, 5, NVGPU_TIMER_CPU_TIMER);

		do {
			int cmgmt0 = ltc_ltc0_ltss_tstg_cmgmt0_r() +
				     ltc * ltc_stride;
			op_pending = gk20a_readl(g, cmgmt0);
			is_invalidate_pending_set = (op_pending &
				ltc_ltc0_ltss_tstg_cmgmt0_invalidate_pending_f()) != 0U;
		} while (is_invalidate_pending_set &&
			(nvgpu_timeout_expired_msg(&timeout,
					"L2 flush timeout!") == 0));
	}
}

int gm20b_determine_L2_size_bytes(struct gk20a *g)
{
	u32 lts_per_ltc;
	u32 ways;
	u32 sets;
	u32 bytes_per_line;
	u32 active_ltcs;
	u32 cache_size;

	u32 tmp;
	u32 active_sets_value;

	tmp = gk20a_readl(g, ltc_ltc0_lts0_tstg_cfg1_r());
	ways = hweight32(ltc_ltc0_lts0_tstg_cfg1_active_ways_v(tmp));

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
	cache_size = active_ltcs * lts_per_ltc * ways * sets * bytes_per_line;

	return cache_size;
}

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

	return (addr >= ltc_shared_base) &&
		(addr < (ltc_shared_base + lts_stride));
}

bool gm20b_ltc_is_ltcn_ltss_addr(struct gk20a *g, u32 addr)
{
	u32 lts_shared_base = ltc_ltc0_ltss_v();
	u32 lts_stride = nvgpu_get_litter_value(g, GPU_LIT_LTS_STRIDE);
	u32 addr_mask = nvgpu_get_litter_value(g, GPU_LIT_LTC_STRIDE) - 1U;
	u32 base_offset = lts_shared_base & addr_mask;
	u32 end_offset = base_offset + lts_stride;

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

	for (lts_num = 0; lts_num < num_ltc_slices; lts_num++) {
		priv_addr_table[index++] = ltc_ltc0_lts0_v() +
						ltc_num * ltc_stride +
						lts_num * lts_stride +
						(addr & (lts_stride - 1U));
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
		start = pltcg_base + i * ltc_stride;
		if ((addr >= start) && (addr < (start + ltc_stride))) {
			ltc_num = i;
			break;
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

	for (ltc_num = 0; ltc_num < num_ltc; ltc_num++) {
		gm20b_ltc_update_ltc_lts_addr(g, addr, ltc_num,
					priv_addr_table, priv_addr_table_index);
	}
}
