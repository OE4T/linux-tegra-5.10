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

#include <nvgpu/gr/gr_ecc.h>
#include <nvgpu/gr/gr_utils.h>
#include <nvgpu/gr/config.h>
#include <nvgpu/string.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/kmem.h>
#include <nvgpu/ecc.h>

int nvgpu_ecc_counter_init_per_gr(struct gk20a *g,
		struct nvgpu_ecc_stat **stat, const char *name)
{
	struct nvgpu_ecc_stat *stats;
	u32 i;
	char gr_str[10] = {0};

	stats = nvgpu_kzalloc(g, nvgpu_safe_mult_u64(sizeof(*stats),
			g->num_gr_instances));
	if (stats == NULL) {
		return -ENOMEM;
	}

	for (i = 0; i < g->num_gr_instances; i++) {
		/**
		 * Store stats name as below:
		 * gr<gr_index>_<name_string>
		 */
		(void)strcpy(stats[i].name, "gr");
		(void)nvgpu_strnadd_u32(gr_str, i, sizeof(gr_str), 10U);
		(void)strncat(stats[i].name, gr_str,
					NVGPU_ECC_STAT_NAME_MAX_SIZE -
					strlen(stats[i].name));
		(void)strncat(stats[i].name, "_",
					NVGPU_ECC_STAT_NAME_MAX_SIZE -
					strlen(stats[i].name));
		(void)strncat(stats[i].name, name,
					NVGPU_ECC_STAT_NAME_MAX_SIZE -
					strlen(stats[i].name));

		nvgpu_ecc_stat_add(g, &stats[i]);
	}

	*stat = stats;
	return 0;
}

int nvgpu_ecc_counter_init_per_tpc(struct gk20a *g,
		struct nvgpu_ecc_stat ***stat, const char *name)
{
	struct nvgpu_ecc_stat **stats;
	struct nvgpu_gr_config *gr_config = nvgpu_gr_get_config_ptr(g);
	u32 gpc_count = nvgpu_gr_config_get_gpc_count(gr_config);
	u32 gpc, tpc;
	char gpc_str[10] = {0}, tpc_str[10] = {0};
	int err = 0;

	stats = nvgpu_kzalloc(g, nvgpu_safe_mult_u64(sizeof(*stats),
			      gpc_count));
	if (stats == NULL) {
		return -ENOMEM;
	}
	for (gpc = 0; gpc < gpc_count; gpc++) {
		stats[gpc] = nvgpu_kzalloc(g,
			nvgpu_safe_mult_u64(sizeof(*stats[gpc]),
				nvgpu_gr_config_get_gpc_tpc_count(gr_config,
								  gpc)));
		if (stats[gpc] == NULL) {
			err = -ENOMEM;
			goto fail;
		}
	}

	for (gpc = 0; gpc < gpc_count; gpc++) {
		for (tpc = 0;
		     tpc < nvgpu_gr_config_get_gpc_tpc_count(gr_config, gpc);
		     tpc++) {
			/**
			 * Store stats name as below:
			 * gpc<gpc_value>_tpc<tpc_value>_<name_string>
			 */
			(void)strcpy(stats[gpc][tpc].name, "gpc");
			(void)nvgpu_strnadd_u32(gpc_str, gpc,
							sizeof(gpc_str), 10U);
			(void)strncat(stats[gpc][tpc].name, gpc_str,
						NVGPU_ECC_STAT_NAME_MAX_SIZE -
						strlen(stats[gpc][tpc].name));
			(void)strncat(stats[gpc][tpc].name, "_tpc",
						NVGPU_ECC_STAT_NAME_MAX_SIZE -
						strlen(stats[gpc][tpc].name));
			(void)nvgpu_strnadd_u32(tpc_str, tpc,
							sizeof(tpc_str), 10U);
			(void)strncat(stats[gpc][tpc].name, tpc_str,
						NVGPU_ECC_STAT_NAME_MAX_SIZE -
						strlen(stats[gpc][tpc].name));
			(void)strncat(stats[gpc][tpc].name, "_",
						NVGPU_ECC_STAT_NAME_MAX_SIZE -
						strlen(stats[gpc][tpc].name));
			(void)strncat(stats[gpc][tpc].name, name,
						NVGPU_ECC_STAT_NAME_MAX_SIZE -
						strlen(stats[gpc][tpc].name));

			nvgpu_ecc_stat_add(g, &stats[gpc][tpc]);
		}
	}

	*stat = stats;

fail:
	if (err != 0) {
#ifdef CONFIG_NVGPU_DGPU
		while (gpc-- != 0u) {
			nvgpu_kfree(g, stats[gpc]);
		}
#endif
		nvgpu_kfree(g, stats);
	}

	return err;
}

int nvgpu_ecc_counter_init_per_gpc(struct gk20a *g,
		struct nvgpu_ecc_stat **stat, const char *name)
{
	struct nvgpu_ecc_stat *stats;
	struct nvgpu_gr_config *gr_config = nvgpu_gr_get_config_ptr(g);
	u32 gpc_count = nvgpu_gr_config_get_gpc_count(gr_config);
	u32 gpc;
	char gpc_str[10] = {0};

	stats = nvgpu_kzalloc(g, nvgpu_safe_mult_u64(sizeof(*stats),
						     gpc_count));
	if (stats == NULL) {
		return -ENOMEM;
	}

	for (gpc = 0; gpc < gpc_count; gpc++) {
		/**
		 * Store stats name as below:
		 * gpc<gpc_value>_<name_string>
		 */
		(void)strcpy(stats[gpc].name, "gpc");
		(void)nvgpu_strnadd_u32(gpc_str, gpc, sizeof(gpc_str), 10U);
		(void)strncat(stats[gpc].name, gpc_str,
					NVGPU_ECC_STAT_NAME_MAX_SIZE -
					strlen(stats[gpc].name));
		(void)strncat(stats[gpc].name, "_",
					NVGPU_ECC_STAT_NAME_MAX_SIZE -
					strlen(stats[gpc].name));
		(void)strncat(stats[gpc].name, name,
					NVGPU_ECC_STAT_NAME_MAX_SIZE -
					strlen(stats[gpc].name));

		nvgpu_ecc_stat_add(g, &stats[gpc]);
	}

	*stat = stats;
	return 0;
}

/* helper function that frees the count array if non-NULL. */
static void free_ecc_stat_count_array(struct gk20a *g,
				      struct nvgpu_ecc_stat **stat,
				      u32 gpc_count)
{
	u32 i;

	if (stat != NULL) {
		for (i = 0; i < gpc_count; i++) {
			nvgpu_kfree(g, stat[i]);
		}
		nvgpu_kfree(g, stat);
	}
}

void nvgpu_gr_ecc_free(struct gk20a *g)
{
	struct nvgpu_ecc *ecc = &g->ecc;
	struct nvgpu_gr_config *gr_config = nvgpu_gr_get_config_ptr(g);
	u32 gpc_count;

	if (gr_config == NULL) {
		return;
	}

	gpc_count = nvgpu_gr_config_get_gpc_count(gr_config);

	free_ecc_stat_count_array(g, ecc->gr.sm_lrf_ecc_single_err_count,
				  gpc_count);
	free_ecc_stat_count_array(g, ecc->gr.sm_lrf_ecc_double_err_count,
				  gpc_count);
	free_ecc_stat_count_array(g, ecc->gr.sm_shm_ecc_sec_count,
				  gpc_count);
	free_ecc_stat_count_array(g, ecc->gr.sm_shm_ecc_sed_count,
				  gpc_count);
	free_ecc_stat_count_array(g, ecc->gr.sm_shm_ecc_ded_count,
				  gpc_count);
	free_ecc_stat_count_array(g, ecc->gr.tex_ecc_total_sec_pipe0_count,
				  gpc_count);
	free_ecc_stat_count_array(g, ecc->gr.tex_ecc_total_ded_pipe0_count,
				  gpc_count);
	free_ecc_stat_count_array(g, ecc->gr.tex_unique_ecc_sec_pipe0_count,
				  gpc_count);
	free_ecc_stat_count_array(g, ecc->gr.tex_unique_ecc_ded_pipe0_count,
				  gpc_count);
	free_ecc_stat_count_array(g, ecc->gr.tex_ecc_total_sec_pipe1_count,
				  gpc_count);
	free_ecc_stat_count_array(g, ecc->gr.tex_ecc_total_ded_pipe1_count,
				  gpc_count);
	free_ecc_stat_count_array(g, ecc->gr.tex_unique_ecc_sec_pipe1_count,
				  gpc_count);
	free_ecc_stat_count_array(g, ecc->gr.tex_unique_ecc_ded_pipe1_count,
				  gpc_count);
	free_ecc_stat_count_array(g, ecc->gr.sm_l1_tag_ecc_corrected_err_count,
				  gpc_count);
	free_ecc_stat_count_array(g,
				  ecc->gr.sm_l1_tag_ecc_uncorrected_err_count,
				  gpc_count);
	free_ecc_stat_count_array(g, ecc->gr.sm_cbu_ecc_corrected_err_count,
				  gpc_count);
	free_ecc_stat_count_array(g, ecc->gr.sm_cbu_ecc_uncorrected_err_count,
				  gpc_count);
	free_ecc_stat_count_array(g, ecc->gr.sm_l1_data_ecc_corrected_err_count,
				  gpc_count);
	free_ecc_stat_count_array(g,
				  ecc->gr.sm_l1_data_ecc_uncorrected_err_count,
				  gpc_count);
	free_ecc_stat_count_array(g, ecc->gr.sm_icache_ecc_corrected_err_count,
				  gpc_count);
	free_ecc_stat_count_array(g,
				  ecc->gr.sm_icache_ecc_uncorrected_err_count,
				  gpc_count);

	nvgpu_kfree(g, ecc->gr.gcc_l15_ecc_corrected_err_count);
	nvgpu_kfree(g, ecc->gr.gcc_l15_ecc_uncorrected_err_count);
	nvgpu_kfree(g, ecc->gr.gpccs_ecc_corrected_err_count);
	nvgpu_kfree(g, ecc->gr.gpccs_ecc_uncorrected_err_count);
	nvgpu_kfree(g, ecc->gr.mmu_l1tlb_ecc_corrected_err_count);
	nvgpu_kfree(g, ecc->gr.mmu_l1tlb_ecc_uncorrected_err_count);
	nvgpu_kfree(g, ecc->gr.fecs_ecc_corrected_err_count);
	nvgpu_kfree(g, ecc->gr.fecs_ecc_uncorrected_err_count);
}
