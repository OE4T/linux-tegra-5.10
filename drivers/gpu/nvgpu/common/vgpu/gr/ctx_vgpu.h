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

#ifndef CTX_VGPU_H
#define CTX_VGPU_H

struct gk20a;
struct nvgpu_gr_ctx;
struct vm_gk20a;
struct nvgpu_gr_global_ctx_buffer_desc;

int vgpu_gr_alloc_gr_ctx(struct gk20a *g,
			struct nvgpu_gr_ctx *gr_ctx,
			struct vm_gk20a *vm);
void vgpu_gr_free_gr_ctx(struct gk20a *g,
			 struct vm_gk20a *vm, struct nvgpu_gr_ctx *gr_ctx);
int vgpu_gr_alloc_patch_ctx(struct gk20a *g, struct nvgpu_gr_ctx *gr_ctx,
			struct vm_gk20a *ch_vm, u64 virt_ctx);
void vgpu_gr_free_patch_ctx(struct gk20a *g, struct vm_gk20a *vm,
			struct nvgpu_gr_ctx *gr_ctx);
int vgpu_gr_alloc_pm_ctx(struct gk20a *g, struct nvgpu_gr_ctx *gr_ctx,
			struct vm_gk20a *vm);
void vgpu_gr_free_pm_ctx(struct gk20a *g, struct vm_gk20a *vm,
			struct nvgpu_gr_ctx *gr_ctx);
void vgpu_gr_unmap_global_ctx_buffers(struct gk20a *g,
			struct nvgpu_gr_ctx *gr_ctx, struct vm_gk20a *ch_vm);
int vgpu_gr_map_global_ctx_buffers(struct gk20a *g, struct nvgpu_gr_ctx *gr_ctx,
		struct nvgpu_gr_global_ctx_buffer_desc *global_ctx_buffer,
		struct vm_gk20a *ch_vm, u64 virt_ctx);
int vgpu_gr_load_golden_ctx_image(struct gk20a *g, u64 virt_ctx);

#endif
