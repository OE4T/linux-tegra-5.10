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
#include <nvgpu/power_features/cg.h>

#include "hal/fifo/fifo_gk20a.h"

#include <nvgpu/hw/gk20a/hw_fifo_gk20a.h>

void gk20a_fifo_init_pbdma_map(struct gk20a *g, u32 *pbdma_map, u32 num_pbdma)
{
	u32 id;

	for (id = 0U; id < num_pbdma; ++id) {
		pbdma_map[id] = nvgpu_readl(g, fifo_pbdma_map_r(id));
	}
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
