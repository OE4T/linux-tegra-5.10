/*
 * GK20A Master Control
 *
 * Copyright (c) 2014-2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/io.h>
#include <nvgpu/mc.h>
#include <nvgpu/gk20a.h>

#include <nvgpu/hw/gm20b/hw_mc_gm20b.h>

u32 nvgpu_mc_boot_0(struct gk20a *g, u32 *arch, u32 *impl, u32 *rev)
{
	u32 val = __nvgpu_readl(g, mc_boot_0_r());

	if (val != 0xffffffffU) {

		if (arch != NULL) {
			*arch = mc_boot_0_architecture_v(val) <<
				NVGPU_GPU_ARCHITECTURE_SHIFT;
		}

		if (impl != NULL) {
			*impl = mc_boot_0_implementation_v(val);
		}

		if (rev != NULL) {
			*rev = (mc_boot_0_major_revision_v(val) << 4) |
				mc_boot_0_minor_revision_v(val);
		}
	}

	return val;
}

/**
 * cyclic_delta - Returns delta of cyclic integers a and b.
 *
 * @a - First integer
 * @b - Second integer
 *
 * Note: if a is ahead of b, delta is positive.
 */
static int cyclic_delta(int a, int b)
{
	return a - b;
}

/**
 * nvgpu_wait_for_deferred_interrupts - Wait for interrupts to complete
 *
 * @g - The GPU to wait on.
 *
 * Waits until all interrupt handlers that have been scheduled to run have
 * completed.
 */
void nvgpu_wait_for_deferred_interrupts(struct gk20a *g)
{
	int stall_irq_threshold = nvgpu_atomic_read(&g->hw_irq_stall_count);
	int nonstall_irq_threshold = nvgpu_atomic_read(&g->hw_irq_nonstall_count);

	/* wait until all stalling irqs are handled */
	NVGPU_COND_WAIT(&g->sw_irq_stall_last_handled_cond,
		cyclic_delta(stall_irq_threshold,
			nvgpu_atomic_read(&g->sw_irq_stall_last_handled))
		<= 0, 0);

	/* wait until all non-stalling irqs are handled */
	NVGPU_COND_WAIT(&g->sw_irq_nonstall_last_handled_cond,
		cyclic_delta(nonstall_irq_threshold,
			nvgpu_atomic_read(&g->sw_irq_nonstall_last_handled))
		<= 0, 0);
}
