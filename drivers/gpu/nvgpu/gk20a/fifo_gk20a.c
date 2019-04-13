/*
 * GK20A Graphics FIFO (gr host)
 *
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
#include <nvgpu/log.h>
#include <nvgpu/soc.h>
#include <nvgpu/io.h>
#include <nvgpu/fifo.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/unit.h>
#include <nvgpu/ptimer.h>
#include <nvgpu/power_features/cg.h>

#include "fifo_gk20a.h"

#include <nvgpu/hw/gk20a/hw_fifo_gk20a.h>

int gk20a_init_fifo_reset_enable_hw(struct gk20a *g)
{
	u32 timeout;

	nvgpu_log_fn(g, " ");

	/* enable pmc pfifo */
	g->ops.mc.reset(g, g->ops.mc.reset_mask(g, NVGPU_UNIT_FIFO));

	nvgpu_cg_slcg_fifo_load_enable(g);

	nvgpu_cg_blcg_fifo_load_enable(g);

	timeout = gk20a_readl(g, fifo_fb_timeout_r());
	timeout = set_field(timeout, fifo_fb_timeout_period_m(),
			fifo_fb_timeout_period_max_f());
	nvgpu_log_info(g, "fifo_fb_timeout reg val = 0x%08x", timeout);
	gk20a_writel(g, fifo_fb_timeout_r(), timeout);

	g->ops.pbdma.setup_hw(g);

	g->ops.fifo.intr_0_enable(g, true);
	g->ops.fifo.intr_1_enable(g, true);

	nvgpu_log_fn(g, "done");

	return 0;
}

int gk20a_init_fifo_setup_hw(struct gk20a *g)
{
	struct fifo_gk20a *f = &g->fifo;
	u64 shifted_addr;

	nvgpu_log_fn(g, " ");

	/* set the base for the userd region now */
	shifted_addr = f->userd_gpu_va >> 12;
	if ((shifted_addr >> 32) != 0U) {
		nvgpu_err(g, "GPU VA > 32 bits %016llx\n", f->userd_gpu_va);
		return -EFAULT;
	}
	gk20a_writel(g, fifo_bar1_base_r(),
			fifo_bar1_base_ptr_f(u64_lo32(shifted_addr)) |
			fifo_bar1_base_valid_true_f());

	nvgpu_log_fn(g, "done");

	return 0;
}

int gk20a_fifo_suspend(struct gk20a *g)
{
	nvgpu_log_fn(g, " ");

	/* stop bar1 snooping */
	if (g->ops.mm.is_bar1_supported(g)) {
		gk20a_writel(g, fifo_bar1_base_r(),
			fifo_bar1_base_valid_false_f());
	}

	/* disable fifo intr */
	g->ops.fifo.intr_0_enable(g, false);
	g->ops.fifo.intr_1_enable(g, false);

	nvgpu_log_fn(g, "done");
	return 0;
}

int gk20a_fifo_init_pbdma_map(struct gk20a *g, u32 *pbdma_map, u32 num_pbdma)
{
	u32 id;

	for (id = 0; id < num_pbdma; ++id) {
		pbdma_map[id] = gk20a_readl(g, fifo_pbdma_map_r(id));
	}

	return 0;
}

u32 gk20a_fifo_get_runlist_timeslice(struct gk20a *g)
{
	return fifo_runlist_timeslice_timeout_128_f() |
			fifo_runlist_timeslice_timescale_3_f() |
			fifo_runlist_timeslice_enable_true_f();
}

u32 gk20a_fifo_get_pb_timeslice(struct gk20a *g) {
	return fifo_pb_timeslice_timeout_16_f() |
			fifo_pb_timeslice_timescale_0_f() |
			fifo_pb_timeslice_enable_true_f();
}
