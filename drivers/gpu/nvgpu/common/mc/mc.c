/*
 * GK20A Master Control
 *
 * Copyright (c) 2014-2020, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/mc.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/bug.h>

void nvgpu_wait_for_deferred_interrupts(struct gk20a *g)
{
	/* wait until all stalling irqs are handled */
	NVGPU_COND_WAIT(&g->mc.sw_irq_stall_last_handled_cond,
			nvgpu_atomic_read(&g->mc.sw_irq_stall_pending) == 0,
			0U);

	/* wait until all non-stalling irqs are handled */
	NVGPU_COND_WAIT(&g->mc.sw_irq_nonstall_last_handled_cond,
			nvgpu_atomic_read(&g->mc.sw_irq_nonstall_pending) == 0,
			0U);
}

void nvgpu_mc_intr_mask(struct gk20a *g)
{
	unsigned long flags = 0;

	if (g->ops.mc.intr_mask != NULL) {
		nvgpu_spinlock_irqsave(&g->mc.intr_lock, flags);
		g->ops.mc.intr_mask(g);
		nvgpu_spinunlock_irqrestore(&g->mc.intr_lock, flags);
	}
}

#ifdef CONFIG_NVGPU_NON_FUSA
void nvgpu_mc_log_pending_intrs(struct gk20a *g)
{
	if (g->ops.mc.log_pending_intrs != NULL) {
		g->ops.mc.log_pending_intrs(g);
	}
}

void nvgpu_mc_intr_enable(struct gk20a *g)
{
	unsigned long flags = 0;

	if (g->ops.mc.intr_enable != NULL) {
		nvgpu_spinlock_irqsave(&g->mc.intr_lock, flags);
		g->ops.mc.intr_enable(g);
		nvgpu_spinunlock_irqrestore(&g->mc.intr_lock, flags);
	}
}
#endif

void nvgpu_mc_intr_stall_unit_config(struct gk20a *g, u32 unit, bool enable)
{
	unsigned long flags = 0;

	nvgpu_spinlock_irqsave(&g->mc.intr_lock, flags);
	g->ops.mc.intr_stall_unit_config(g, unit, enable);
	nvgpu_spinunlock_irqrestore(&g->mc.intr_lock, flags);
}

void nvgpu_mc_intr_nonstall_unit_config(struct gk20a *g, u32 unit, bool enable)
{
	unsigned long flags = 0;

	nvgpu_spinlock_irqsave(&g->mc.intr_lock, flags);
	g->ops.mc.intr_nonstall_unit_config(g, unit, enable);
	nvgpu_spinunlock_irqrestore(&g->mc.intr_lock, flags);
}

void nvgpu_mc_intr_stall_pause(struct gk20a *g)
{
	unsigned long flags = 0;

	nvgpu_spinlock_irqsave(&g->mc.intr_lock, flags);
	g->ops.mc.intr_stall_pause(g);
	nvgpu_spinunlock_irqrestore(&g->mc.intr_lock, flags);
}

void nvgpu_mc_intr_stall_resume(struct gk20a *g)
{
	unsigned long flags = 0;

	nvgpu_spinlock_irqsave(&g->mc.intr_lock, flags);
	g->ops.mc.intr_stall_resume(g);
	nvgpu_spinunlock_irqrestore(&g->mc.intr_lock, flags);
}

void nvgpu_mc_intr_nonstall_pause(struct gk20a *g)
{
	unsigned long flags = 0;

	nvgpu_spinlock_irqsave(&g->mc.intr_lock, flags);
	g->ops.mc.intr_nonstall_pause(g);
	nvgpu_spinunlock_irqrestore(&g->mc.intr_lock, flags);
}

void nvgpu_mc_intr_nonstall_resume(struct gk20a *g)
{
	unsigned long flags = 0;

	nvgpu_spinlock_irqsave(&g->mc.intr_lock, flags);
	g->ops.mc.intr_nonstall_resume(g);
	nvgpu_spinunlock_irqrestore(&g->mc.intr_lock, flags);
}

#if defined(CONFIG_NVGPU_NON_FUSA) && defined(CONFIG_NVGPU_NEXT)
void nvgpu_mc_intr_unit_vectorid_init(struct gk20a *g, u32 unit,
		u32 *vectorid, u32 num_entries)
{
	unsigned long flags = 0;
	u32 i = 0U;
	struct nvgpu_intr_unit_info *intr_unit_info;

	nvgpu_assert(num_entries <= INTR_VECTORID_SIZE_MAX);

	intr_unit_info = g->mc.nvgpu_next.intr_unit_info;

	nvgpu_spinlock_irqsave(&g->mc.intr_lock, flags);

	if (intr_unit_info[unit].valid == false) {
		for (i = 0U; i < num_entries; i++) {
			intr_unit_info[unit].vectorid[i] = *(vectorid + i);
		}
		intr_unit_info[unit].vectorid_size = num_entries;
	}
	nvgpu_spinunlock_irqrestore(&g->mc.intr_lock, flags);
}

bool nvgpu_mc_intr_is_unit_info_valid(struct gk20a *g, u32 unit)
{
	unsigned long flags = 0;
	struct nvgpu_intr_unit_info *intr_unit_info;
	bool info_valid = false;

	intr_unit_info = g->mc.nvgpu_next.intr_unit_info;

	nvgpu_spinlock_irqsave(&g->mc.intr_lock, flags);
	if (intr_unit_info[unit].valid == true) {
		info_valid = true;
	}
	nvgpu_spinunlock_irqrestore(&g->mc.intr_lock, flags);

	return info_valid;
}
#endif
