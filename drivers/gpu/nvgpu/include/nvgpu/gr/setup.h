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
#ifndef NVGPU_GR_SETUP_H
#define NVGPU_GR_SETUP_H

#include <nvgpu/types.h>

struct gk20a;
struct nvgpu_channel;
struct vm_gk20a;
struct nvgpu_gr_ctx;

int nvgpu_gr_setup_bind_ctxsw_zcull(struct gk20a *g, struct nvgpu_channel *c,
			u64 zcull_va, u32 mode);

int nvgpu_gr_setup_alloc_obj_ctx(struct nvgpu_channel *c, u32 class_num,
		u32 flags);
void nvgpu_gr_setup_free_gr_ctx(struct gk20a *g,
		struct vm_gk20a *vm, struct nvgpu_gr_ctx *gr_ctx);
void nvgpu_gr_setup_free_subctx(struct nvgpu_channel *c);

int nvgpu_gr_setup_set_preemption_mode(struct nvgpu_channel *ch,
					u32 graphics_preempt_mode,
					u32 compute_preempt_mode);

#endif /* NVGPU_GR_SETUP_H */
