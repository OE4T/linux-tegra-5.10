/*
 * Copyright (c) 2016-2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/bug.h>
#include <nvgpu/pmu.h>
#include <nvgpu/clk_arb.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/pmu/perf.h>

#include "pmu_perf.h"

int nvgpu_perf_pmu_init_pmupstate(struct gk20a *g)
{
	/* If already allocated, do not re-allocate */
	if (g->perf_pmu != NULL) {
		return 0;
	}

	g->perf_pmu = nvgpu_kzalloc(g, sizeof(*g->perf_pmu));
	if (g->perf_pmu == NULL) {
		return -ENOMEM;
	}

	return 0;
}

static void vfe_thread_stop_cb(void *data)
{
	struct nvgpu_cond *cond = (struct nvgpu_cond *)data;

	nvgpu_cond_signal(cond);
}

void nvgpu_perf_pmu_free_pmupstate(struct gk20a *g)
{
	if (nvgpu_thread_is_running(&g->perf_pmu->vfe_init.state_task)) {
		nvgpu_thread_stop_graceful(&g->perf_pmu->vfe_init.state_task,
				vfe_thread_stop_cb, &g->perf_pmu->vfe_init.wq);
	}
	nvgpu_cond_destroy(&g->perf_pmu->vfe_init.wq);
	nvgpu_kfree(g, g->perf_pmu);
	g->perf_pmu = NULL;
}
