/*
 * Copyright (c) 2018-2020, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/gr/gr_ecc.h>
#include <nvgpu/ltc.h>

void nvgpu_ecc_stat_add(struct gk20a *g, struct nvgpu_ecc_stat *stat)
{
	struct nvgpu_ecc *ecc = &g->ecc;

	nvgpu_init_list_node(&stat->node);

	nvgpu_list_add_tail(&stat->node, &ecc->stats_list);
	ecc->stats_count = nvgpu_safe_add_s32(ecc->stats_count, 1);
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

/* release all ecc_stat */
void nvgpu_ecc_free(struct gk20a *g)
{
	struct nvgpu_ecc *ecc = &g->ecc;

	nvgpu_gr_ecc_free(g);
	nvgpu_ltc_ecc_free(g);

	if (g->ops.fb.ecc.free != NULL) {
		g->ops.fb.ecc.free(g);
	}

#ifdef CONFIG_NVGPU_DGPU
	if (g->ops.fb.fbpa_ecc_free != NULL) {
		g->ops.fb.fbpa_ecc_free(g);
	}
#endif

	if (g->ops.pmu.ecc_free != NULL) {
		g->ops.pmu.ecc_free(g);
	}

	(void)memset(ecc, 0, sizeof(*ecc));
}

int nvgpu_ecc_init_support(struct gk20a *g)
{
	struct nvgpu_ecc *ecc = &g->ecc;

	if (ecc->initialized) {
		return 0;
	}

	nvgpu_init_list_node(&ecc->stats_list);

	return 0;
}

/**
 * Note that this function is to be called after all units requiring ecc stats
 * have added entries to ecc->stats_list.
 */
int nvgpu_ecc_finalize_support(struct gk20a *g)
{
#ifdef CONFIG_NVGPU_SYSFS
	int err;
#endif

	if (g->ecc.initialized) {
		return 0;
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
	if (!g->ecc.initialized) {
		return;
	}

#ifdef CONFIG_NVGPU_SYSFS
	nvgpu_ecc_sysfs_remove(g);
#endif
	nvgpu_ecc_free(g);
}
