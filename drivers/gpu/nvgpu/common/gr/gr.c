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

#include <nvgpu/gr/gr.h>
#include <nvgpu/gr/config.h>

u32 nvgpu_gr_gpc_offset(struct gk20a *g, u32 gpc)
{
	u32 gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_GPC_STRIDE);
	u32 gpc_offset = gpc_stride * gpc;

	return gpc_offset;
}

u32 nvgpu_gr_tpc_offset(struct gk20a *g, u32 tpc)
{
	u32 tpc_in_gpc_stride = nvgpu_get_litter_value(g,
					GPU_LIT_TPC_IN_GPC_STRIDE);
	u32 tpc_offset = tpc_in_gpc_stride * tpc;

	return tpc_offset;
}

int nvgpu_gr_suspend(struct gk20a *g)
{
	int ret = 0;

	nvgpu_log_fn(g, " ");

	ret = g->ops.gr.init.wait_empty(g);
	if (ret != 0) {
		return ret;
	}

	/* Disable fifo access */
	g->ops.gr.init.fifo_access(g, false);

	/* disable gr intr */
	g->ops.gr.intr.enable_interrupts(g, false);

	/* disable all exceptions */
	g->ops.gr.intr.enable_exceptions(g, g->gr.config, false);

	nvgpu_gr_flush_channel_tlb(g);

	g->gr.initialized = false;

	nvgpu_log_fn(g, "done");
	return ret;
}

/* invalidate channel lookup tlb */
void nvgpu_gr_flush_channel_tlb(struct gk20a *g)
{
	nvgpu_spinlock_acquire(&g->gr.ch_tlb_lock);
	(void) memset(g->gr.chid_tlb, 0,
		sizeof(struct gr_channel_map_tlb_entry) *
		GR_CHANNEL_MAP_TLB_SIZE);
	nvgpu_spinlock_release(&g->gr.ch_tlb_lock);
}

/* Wait until GR is initialized */
void nvgpu_gr_wait_initialized(struct gk20a *g)
{
	NVGPU_COND_WAIT(&g->gr.init_wq, g->gr.initialized, 0U);
}
