/*
 * Copyright (c) 2020-2021, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_NEXT_RUNLIST_H
#define NVGPU_NEXT_RUNLIST_H

/**
 * @file
 *
 * Declare runlist info specific struct and defines.
 */
#include <nvgpu/types.h>

struct nvgpu_next_pbdma_info;
struct nvgpu_device;
struct nvgpu_fifo;

#define RLENG_PER_RUNLIST_SIZE			3

struct nvgpu_next_runlist {
	/** Runlist pri base - offset into device's runlist space */
	u32 runlist_pri_base;
	/** Channel ram address in bar0 pri space */
	u32 chram_bar0_offset;
	/** Pointer to pbdma info stored in engine_info*/
	const struct nvgpu_next_pbdma_info *pbdma_info;
	/** Pointer to engine info for per runlist engine id */
	const struct nvgpu_device *rl_dev_list[RLENG_PER_RUNLIST_SIZE];
};

void nvgpu_next_runlist_init_enginfo(struct gk20a *g, struct nvgpu_fifo *f);

#endif /* NVGPU_NEXT_RUNLIST_H */
