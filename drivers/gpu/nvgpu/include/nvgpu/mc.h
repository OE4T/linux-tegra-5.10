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

#ifndef NVGPU_MC_H
#define NVGPU_MC_H

#include <nvgpu/types.h>

struct gk20a;

/**
 * Enumeration of all units intended to be used by any HAL that requires
 * unit as parameter.
 *
 * Units are added to the enumeration as needed, so it is not complete.
 */
enum nvgpu_unit {
	NVGPU_UNIT_FIFO,
	NVGPU_UNIT_PERFMON,
	NVGPU_UNIT_GRAPH,
	NVGPU_UNIT_BLG,
	NVGPU_UNIT_PWR,
	NVGPU_UNIT_NVDEC,
};

#define NVGPU_MC_INTR_STALLING		0U
#define NVGPU_MC_INTR_NONSTALLING	1U

/** Operations that will need to be executed on non stall workqueue. */
#define NVGPU_NONSTALL_OPS_WAKEUP_SEMAPHORE	BIT32(0)
#define NVGPU_NONSTALL_OPS_POST_EVENTS		BIT32(1)

u32 nvgpu_mc_boot_0(struct gk20a *g, u32 *arch, u32 *impl, u32 *rev);
void nvgpu_wait_for_deferred_interrupts(struct gk20a *g);

#endif
