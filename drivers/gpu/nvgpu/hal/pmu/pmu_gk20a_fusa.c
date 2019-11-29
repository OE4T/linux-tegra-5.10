/*
 * Copyright (c) 2011-2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/pmu/debug.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/io.h>

#include <nvgpu/hw/gk20a/hw_pwr_gk20a.h>

#include "pmu_gk20a.h"

void gk20a_pmu_isr(struct gk20a *g)
{
	struct nvgpu_pmu *pmu = g->pmu;
	u32 intr, mask;

	nvgpu_log_fn(g, " ");

	nvgpu_mutex_acquire(&pmu->isr_mutex);
	if (!pmu->isr_enabled) {
		nvgpu_mutex_release(&pmu->isr_mutex);
		return;
	}

	mask = nvgpu_readl(g, pwr_falcon_irqmask_r());
	mask &= nvgpu_readl(g, pwr_falcon_irqdest_r());

	intr = nvgpu_readl(g, pwr_falcon_irqstat_r());

	nvgpu_pmu_dbg(g, "received falcon interrupt: 0x%08x", intr);

	intr = nvgpu_readl(g, pwr_falcon_irqstat_r()) & mask;

	if (intr == 0U) {
		nvgpu_mutex_release(&pmu->isr_mutex);
		return;
	}

	if (g->ops.pmu.handle_ext_irq != NULL) {
		g->ops.pmu.handle_ext_irq(g, intr);
	}

	nvgpu_writel(g, pwr_falcon_irqsclr_r(), intr);

#ifdef CONFIG_NVGPU_LS_PMU
	if (nvgpu_pmu_get_fw_state(g, pmu) == PMU_FW_STATE_OFF) {
		nvgpu_mutex_release(&pmu->isr_mutex);
		return;
	}

	gk20a_pmu_handle_interrupts(g, intr);
#endif

	nvgpu_mutex_release(&pmu->isr_mutex);
}
