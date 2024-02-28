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

#ifndef NVGPU_RUNLIST_GA10B_H
#define NVGPU_RUNLIST_GA10B_H

#include <nvgpu/types.h>

struct gk20a;
struct nvgpu_pbdma_info;

u32 ga10b_runlist_get_runlist_id(struct gk20a *g, u32 runlist_pri_base);
u32 ga10b_runlist_get_engine_id_from_rleng_id(struct gk20a *g,
			u32 rleng_id, u32 runlist_pri_base);
u32 ga10b_runlist_get_chram_bar0_offset(struct gk20a *g, u32 runlist_pri_base);
void ga10b_runlist_get_pbdma_info(struct gk20a *g, u32 runlist_pri_base,
		struct nvgpu_pbdma_info *pbdma_info);
u32 ga10b_runlist_get_engine_intr_id(struct gk20a *g, u32 runlist_pri_base,
		u32 rleng_id);
u32 ga10b_runlist_get_esched_fb_thread_id(struct gk20a *g, u32 runlist_pri_base);

#endif /* NVGPU_RUNLIST_GA10B_H */
