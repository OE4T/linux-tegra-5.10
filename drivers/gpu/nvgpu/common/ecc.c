/*
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

#include <nvgpu/gk20a.h>
#include <nvgpu/gr/config.h>
#include <nvgpu/gr/gr.h>
#include <nvgpu/gr/gr_utils.h>
#include <nvgpu/ltc.h>
#include <nvgpu/static_analysis.h>
#include <nvgpu/string.h>

static void nvgpu_ecc_stat_add(struct gk20a *g, struct nvgpu_ecc_stat *stat)
{
	struct nvgpu_ecc *ecc = &g->ecc;

	nvgpu_init_list_node(&stat->node);

	nvgpu_list_add_tail(&stat->node, &ecc->stats_list);
	ecc->stats_count = nvgpu_safe_add_s32(ecc->stats_count, 1);
}

static void nvgpu_ecc_init(struct gk20a *g)
{
	struct nvgpu_ecc *ecc = &g->ecc;

	nvgpu_init_list_node(&ecc->stats_list);
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
		while (gpc-- != 0u) {
			nvgpu_kfree(g, stats[gpc]);
		}

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

int nvgpu_ecc_counter_init(struct gk20a *g,
		struct nvgpu_ecc_stat **stat, const char *name)
{
	struct nvgpu_ecc_stat *stats;

	stats = nvgpu_kzalloc(g, sizeof(*stats));
	if (stats == NULL) {
		return -ENOMEM;
	}

	(void)strncpy(stats->name, name, NVGPU_ECC_STAT_NAME_MAX_SIZE - 1U);
	nvgpu_ecc_stat_add(g, stats);
	*stat = stats;
	return 0;
}

int nvgpu_ecc_counter_init_per_lts(struct gk20a *g,
		struct nvgpu_ecc_stat ***stat, const char *name)
{
	struct nvgpu_ecc_stat **stats;
	u32 ltc, lts;
	char ltc_str[10] = {0}, lts_str[10] = {0};
	int err = 0;
	u32 ltc_count = nvgpu_ltc_get_ltc_count(g);
	u32 slices_per_ltc = nvgpu_ltc_get_slices_per_ltc(g);

	stats = nvgpu_kzalloc(g, nvgpu_safe_mult_u64(sizeof(*stats),
						     ltc_count));
	if (stats == NULL) {
		return -ENOMEM;
	}
	for (ltc = 0; ltc < ltc_count; ltc++) {
		stats[ltc] = nvgpu_kzalloc(g,
			nvgpu_safe_mult_u64(sizeof(*stats[ltc]),
					    slices_per_ltc));
		if (stats[ltc] == NULL) {
			err = -ENOMEM;
			goto fail;
		}
	}

	for (ltc = 0; ltc < ltc_count; ltc++) {
		for (lts = 0; lts < slices_per_ltc; lts++) {
			/**
			 * Store stats name as below:
			 * ltc<ltc_value>_lts<lts_value>_<name_string>
			 */
			(void)strcpy(stats[ltc][lts].name, "ltc");
			(void)nvgpu_strnadd_u32(ltc_str, ltc,
							sizeof(ltc_str), 10U);
			(void)strncat(stats[ltc][lts].name, ltc_str,
						NVGPU_ECC_STAT_NAME_MAX_SIZE -
						strlen(stats[ltc][lts].name));
			(void)strncat(stats[ltc][lts].name, "_lts",
						NVGPU_ECC_STAT_NAME_MAX_SIZE -
						strlen(stats[ltc][lts].name));
			(void)nvgpu_strnadd_u32(lts_str, lts,
							sizeof(lts_str), 10U);
			(void)strncat(stats[ltc][lts].name, lts_str,
						NVGPU_ECC_STAT_NAME_MAX_SIZE -
						strlen(stats[ltc][lts].name));
			(void)strncat(stats[ltc][lts].name, "_",
						NVGPU_ECC_STAT_NAME_MAX_SIZE -
						strlen(stats[ltc][lts].name));
			(void)strncat(stats[ltc][lts].name, name,
						NVGPU_ECC_STAT_NAME_MAX_SIZE -
						strlen(stats[ltc][lts].name));

			nvgpu_ecc_stat_add(g, &stats[ltc][lts]);
		}
	}

	*stat = stats;

fail:
	if (err != 0) {
		while (ltc-- > 0u) {
			nvgpu_kfree(g, stats[ltc]);
		}

		nvgpu_kfree(g, stats);
	}

	return err;
}

int nvgpu_ecc_counter_init_per_fbpa(struct gk20a *g,
		struct nvgpu_ecc_stat **stat, const char *name)
{
	u32 i;
	u32 num_fbpa = nvgpu_get_litter_value(g, GPU_LIT_NUM_FBPAS);
	struct nvgpu_ecc_stat *stats;
	char fbpa_str[10] = {0};

	stats = nvgpu_kzalloc(g, nvgpu_safe_mult_u64(sizeof(*stats),
						     (size_t)num_fbpa));
	if (stats == NULL) {
		return -ENOMEM;
	}

	for (i = 0; i < num_fbpa; i++) {
		/**
		 * Store stats name as below:
		 * fbpa<fbpa_value>_<name_string>
		 */
		(void)strcpy(stats[i].name, "fbpa");
		(void)nvgpu_strnadd_u32(fbpa_str, i, sizeof(fbpa_str), 10U);
		(void)strncat(stats[i].name, fbpa_str,
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

/* release all ecc_stat */
void nvgpu_ecc_free(struct gk20a *g)
{
	struct nvgpu_ecc *ecc = &g->ecc;
	struct nvgpu_gr_config *gr_config = nvgpu_gr_get_config_ptr(g);
	u32 gpc_count;
	u32 i;

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

	for (i = 0; i < nvgpu_ltc_get_ltc_count(g); i++) {
		if (ecc->ltc.ecc_sec_count != NULL) {
			nvgpu_kfree(g, ecc->ltc.ecc_sec_count[i]);
		}

		if (ecc->ltc.ecc_ded_count != NULL) {
			nvgpu_kfree(g, ecc->ltc.ecc_ded_count[i]);
		}
	}
	nvgpu_kfree(g, ecc->ltc.ecc_sec_count);
	nvgpu_kfree(g, ecc->ltc.ecc_ded_count);

	nvgpu_kfree(g, ecc->fb.mmu_l2tlb_ecc_corrected_err_count);
	nvgpu_kfree(g, ecc->fb.mmu_l2tlb_ecc_uncorrected_err_count);
	nvgpu_kfree(g, ecc->fb.mmu_hubtlb_ecc_corrected_err_count);
	nvgpu_kfree(g, ecc->fb.mmu_hubtlb_ecc_uncorrected_err_count);
	nvgpu_kfree(g, ecc->fb.mmu_fillunit_ecc_corrected_err_count);
	nvgpu_kfree(g, ecc->fb.mmu_fillunit_ecc_uncorrected_err_count);

	nvgpu_kfree(g, ecc->pmu.pmu_ecc_corrected_err_count);
	nvgpu_kfree(g, ecc->pmu.pmu_ecc_uncorrected_err_count);

	nvgpu_kfree(g, ecc->fbpa.fbpa_ecc_sec_err_count);
	nvgpu_kfree(g, ecc->fbpa.fbpa_ecc_ded_err_count);

	(void)memset(ecc, 0, sizeof(*ecc));

	ecc->initialized = false;
}

int nvgpu_ecc_init_support(struct gk20a *g)
{
	int err;

	if (g->ecc.initialized) {
		return 0;
	}

	if (g->ops.gr.ecc.init == NULL) {
		return 0;
	}

	nvgpu_ecc_init(g);
	err = g->ops.gr.ecc.init(g);
	if (err != 0) {
		return err;
	}

#ifdef CONFIG_NVGPU_SYSFS
	err = nvgpu_ecc_sysfs_init(g);
	if (err != 0) {
		nvgpu_ecc_free(g);
		return err;
	}
#endif

	g->ecc.initialized = true;

	return 0;
}

void nvgpu_ecc_remove_support(struct gk20a *g)
{
	if (g->ops.gr.ecc.init == NULL) {
		return;
	}

#ifdef CONFIG_NVGPU_SYSFS
	nvgpu_ecc_sysfs_remove(g);
#endif
	nvgpu_ecc_free(g);
}
