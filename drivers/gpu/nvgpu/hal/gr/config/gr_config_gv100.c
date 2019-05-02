/*
 * Copyright (c) 2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/gk20a.h>
#include <nvgpu/types.h>
#include <nvgpu/gr/config.h>

#include "gr_config_gv100.h"

/*
 * Estimate performance if the given logical TPC in the given logical GPC were
 * removed.
 */
static int gr_gv100_scg_estimate_perf(struct gk20a *g,
					struct nvgpu_gr_config *gr_config,
					unsigned long *gpc_tpc_mask,
					u32 disable_gpc_id, u32 disable_tpc_id,
					int *perf)
{
	int err = 0;
	u32 scale_factor = 512U; /* Use fx23.9 */
	u32 pix_scale = 1024U*1024U;	/* Pix perf in [29:20] */
	u32 world_scale = 1024U;	/* World performance in [19:10] */
	u32 tpc_scale = 1U;		/* TPC balancing in [9:0] */
	u32 scg_num_pes = 0U;
	u32 min_scg_gpc_pix_perf = scale_factor; /* Init perf as maximum */
	u32 average_tpcs = 0U;		/* Average of # of TPCs per GPC */
	u32 deviation;			/* absolute diff between TPC# and
					 * average_tpcs, averaged across GPCs
					 */
	u32 norm_tpc_deviation;		/* deviation/max_tpc_per_gpc */
	u32 tpc_balance;
	u32 scg_gpc_pix_perf;
	u32 scg_world_perf;
	u32 gpc_id;
	u32 pes_id;
	int diff;
	bool is_tpc_removed_gpc = false;
	bool is_tpc_removed_pes = false;
	u32 max_tpc_gpc = 0U;
	u32 num_tpc_mask;
	u32 *num_tpc_gpc = nvgpu_kzalloc(g, sizeof(u32) *
				nvgpu_get_litter_value(g, GPU_LIT_NUM_GPCS));

	if (num_tpc_gpc == NULL) {
		return -ENOMEM;
	}

	/* Calculate pix-perf-reduction-rate per GPC and find bottleneck TPC */
	for (gpc_id = 0;
	     gpc_id < nvgpu_gr_config_get_gpc_count(gr_config);
	     gpc_id++) {
		num_tpc_mask = gpc_tpc_mask[gpc_id];

		if ((gpc_id == disable_gpc_id) &&
		    ((num_tpc_mask & BIT32(disable_tpc_id)) != 0U)) {
			/* Safety check if a TPC is removed twice */
			if (is_tpc_removed_gpc) {
				err = -EINVAL;
				goto free_resources;
			}
			/* Remove logical TPC from set */
			num_tpc_mask &= ~(BIT32(disable_tpc_id));
			is_tpc_removed_gpc = true;
		}

		/* track balancing of tpcs across gpcs */
		num_tpc_gpc[gpc_id] = hweight32(num_tpc_mask);
		average_tpcs += num_tpc_gpc[gpc_id];

		/* save the maximum numer of gpcs */
		max_tpc_gpc = num_tpc_gpc[gpc_id] > max_tpc_gpc ?
				num_tpc_gpc[gpc_id] : max_tpc_gpc;

		/*
		 * Calculate ratio between TPC count and post-FS and post-SCG
		 *
		 * ratio represents relative throughput of the GPC
		 */
		scg_gpc_pix_perf = scale_factor * num_tpc_gpc[gpc_id] /
				nvgpu_gr_config_get_gpc_tpc_count(gr_config, gpc_id);

		if (min_scg_gpc_pix_perf > scg_gpc_pix_perf) {
			min_scg_gpc_pix_perf = scg_gpc_pix_perf;
		}

		/* Calculate # of surviving PES */
		for (pes_id = 0;
		     pes_id < nvgpu_gr_config_get_gpc_ppc_count(gr_config, gpc_id);
		     pes_id++) {
			/* Count the number of TPC on the set */
			num_tpc_mask = nvgpu_gr_config_get_pes_tpc_mask(
						gr_config, gpc_id, pes_id) &
					gpc_tpc_mask[gpc_id];

			if ((gpc_id == disable_gpc_id) &&
			    ((num_tpc_mask & BIT32(disable_tpc_id)) != 0U)) {

				if (is_tpc_removed_pes) {
					err = -EINVAL;
					goto free_resources;
				}
				num_tpc_mask &= ~(BIT32(disable_tpc_id));
				is_tpc_removed_pes = true;
			}
			if (hweight32(num_tpc_mask) != 0UL) {
				scg_num_pes++;
			}
		}
	}

	if (!is_tpc_removed_gpc || !is_tpc_removed_pes) {
		err = -EINVAL;
		goto free_resources;
	}

	if (max_tpc_gpc == 0U) {
		*perf = 0;
		goto free_resources;
	}

	/* Now calculate perf */
	scg_world_perf = (scale_factor * scg_num_pes) /
		nvgpu_gr_config_get_ppc_count(gr_config);
	deviation = 0;
	average_tpcs = scale_factor * average_tpcs /
			nvgpu_gr_config_get_gpc_count(gr_config);
	for (gpc_id =0;
	     gpc_id < nvgpu_gr_config_get_gpc_count(gr_config);
	     gpc_id++) {
		diff = average_tpcs - scale_factor * num_tpc_gpc[gpc_id];
		if (diff < 0) {
			diff = -diff;
		}
		deviation += U32(diff);
	}

	deviation /= nvgpu_gr_config_get_gpc_count(gr_config);

	norm_tpc_deviation = deviation / max_tpc_gpc;

	tpc_balance = scale_factor - norm_tpc_deviation;

	if ((tpc_balance > scale_factor)          ||
	    (scg_world_perf > scale_factor)       ||
	    (min_scg_gpc_pix_perf > scale_factor) ||
	    (norm_tpc_deviation > scale_factor)) {
		err = -EINVAL;
		goto free_resources;
	}

	*perf = (pix_scale * min_scg_gpc_pix_perf) +
		(world_scale * scg_world_perf) +
		(tpc_scale * tpc_balance);
free_resources:
	nvgpu_kfree(g, num_tpc_gpc);
	return err;
}

int gv100_gr_config_init_sm_id_table(struct gk20a *g,
		struct nvgpu_gr_config *gr_config)
{
	unsigned long tpc;
	u32 gpc, sm, pes, gtpc;
	u32 sm_id = 0;
	u32 sm_per_tpc = nvgpu_gr_config_get_sm_count_per_tpc(gr_config);
	u32 num_sm = sm_per_tpc * nvgpu_gr_config_get_tpc_count(gr_config);
	int perf, maxperf;
	int err = 0;
	unsigned long *gpc_tpc_mask;
	u32 *tpc_table, *gpc_table;

	if (gr_config == NULL) {
		return -ENOMEM;
	}

	gpc_table = nvgpu_kzalloc(g,
				nvgpu_gr_config_get_tpc_count(gr_config) *
				sizeof(u32));
	tpc_table = nvgpu_kzalloc(g,
				nvgpu_gr_config_get_tpc_count(gr_config) *
				sizeof(u32));
	gpc_tpc_mask = nvgpu_kzalloc(g,
				sizeof(unsigned long) *
				nvgpu_get_litter_value(g, GPU_LIT_NUM_GPCS));

	if ((gpc_table == NULL) ||
	    (tpc_table == NULL) ||
	    (gpc_tpc_mask == NULL)) {
		nvgpu_err(g, "Error allocating memory for sm tables");
		err = -ENOMEM;
		goto exit_build_table;
	}

	for (gpc = 0; gpc < nvgpu_gr_config_get_gpc_count(gr_config); gpc++) {
		for (pes = 0;
		     pes < nvgpu_gr_config_get_gpc_ppc_count(gr_config, gpc);
		     pes++) {
			gpc_tpc_mask[gpc] |= nvgpu_gr_config_get_pes_tpc_mask(
						gr_config, gpc, pes);
		}
	}

	for (gtpc = 0; gtpc < nvgpu_gr_config_get_tpc_count(gr_config); gtpc++) {
		maxperf = -1;
		for (gpc = 0; gpc < nvgpu_gr_config_get_gpc_count(gr_config); gpc++) {
			for_each_set_bit(tpc, &gpc_tpc_mask[gpc],
					nvgpu_gr_config_get_gpc_tpc_count(gr_config, gpc)) {
				perf = -1;
				err = gr_gv100_scg_estimate_perf(g, gr_config,
						gpc_tpc_mask, gpc, tpc, &perf);

				if (err != 0) {
					nvgpu_err(g,
						"Error while estimating perf");
					goto exit_build_table;
				}

				if (perf >= maxperf) {
					maxperf = perf;
					gpc_table[gtpc] = gpc;
					tpc_table[gtpc] = tpc;
				}
			}
		}
		gpc_tpc_mask[gpc_table[gtpc]] &= ~(BIT64(tpc_table[gtpc]));
	}

	for (tpc = 0, sm_id = 0;  sm_id < num_sm; tpc++, sm_id += sm_per_tpc) {
		for (sm = 0; sm < sm_per_tpc; sm++) {
			u32 index = sm_id + sm;
			struct nvgpu_sm_info *sm_info =
				nvgpu_gr_config_get_sm_info(gr_config, index);
			nvgpu_gr_config_set_sm_info_gpc_index(sm_info,
							gpc_table[tpc]);
			nvgpu_gr_config_set_sm_info_tpc_index(sm_info,
							tpc_table[tpc]);
			nvgpu_gr_config_set_sm_info_sm_index(sm_info, sm);
			nvgpu_gr_config_set_sm_info_global_tpc_index(sm_info, tpc);

			nvgpu_log_info(g,
				"gpc : %d tpc %d sm_index %d global_index: %d",
				nvgpu_gr_config_get_sm_info_gpc_index(sm_info),
				nvgpu_gr_config_get_sm_info_tpc_index(sm_info),
				nvgpu_gr_config_get_sm_info_sm_index(sm_info),
				nvgpu_gr_config_get_sm_info_global_tpc_index(sm_info));

		}
	}

	nvgpu_gr_config_set_no_of_sm(gr_config, num_sm);
	nvgpu_log_info(g, " total number of sm = %d", num_sm);

exit_build_table:
	nvgpu_kfree(g, gpc_table);
	nvgpu_kfree(g, tpc_table);
	nvgpu_kfree(g, gpc_tpc_mask);
	return err;
}
