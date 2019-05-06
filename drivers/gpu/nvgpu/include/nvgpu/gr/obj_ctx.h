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

#ifndef NVGPU_GR_OBJ_CTX_H
#define NVGPU_GR_OBJ_CTX_H

#include <nvgpu/types.h>
#include <nvgpu/lock.h>

#define NVGPU_OBJ_CTX_FLAGS_SUPPORT_GFXP		BIT32(1)
#define NVGPU_OBJ_CTX_FLAGS_SUPPORT_CILP		BIT32(2)

struct gk20a;
struct nvgpu_gr_ctx;
struct nvgpu_gr_subctx;
struct nvgpu_gr_config;
struct nvgpu_gr_ctx_desc;
struct vm_gk20a;
struct nvgpu_gr_global_ctx_buffer_desc;
struct nvgpu_mem;
struct nvgpu_channel;
struct nvgpu_gr_obj_ctx_golden_image;

void nvgpu_gr_obj_ctx_commit_inst_gpu_va(struct gk20a *g,
	struct nvgpu_mem *inst_block, u64 gpu_va);
void nvgpu_gr_obj_ctx_commit_inst(struct gk20a *g, struct nvgpu_mem *inst_block,
	struct nvgpu_gr_ctx *gr_ctx, struct nvgpu_gr_subctx *subctx,
	u64 gpu_va);

int nvgpu_gr_obj_ctx_set_ctxsw_preemption_mode(struct gk20a *g,
	struct nvgpu_gr_config *config, struct nvgpu_gr_ctx_desc *gr_ctx_desc,
	struct nvgpu_gr_ctx *gr_ctx, struct vm_gk20a *vm, u32 class,
	u32 graphics_preempt_mode, u32 compute_preempt_mode);
void nvgpu_gr_obj_ctx_update_ctxsw_preemption_mode(struct gk20a *g,
	struct nvgpu_gr_config *config,
	struct nvgpu_gr_ctx *gr_ctx, struct nvgpu_gr_subctx *subctx);

int nvgpu_gr_obj_ctx_commit_global_ctx_buffers(struct gk20a *g,
	struct nvgpu_gr_global_ctx_buffer_desc *global_ctx_buffer,
	struct nvgpu_gr_config *config,	struct nvgpu_gr_ctx *gr_ctx, bool patch);

int nvgpu_gr_obj_ctx_alloc_golden_ctx_image(struct gk20a *g,
	struct nvgpu_gr_obj_ctx_golden_image *golden_image,
	struct nvgpu_gr_global_ctx_buffer_desc *global_ctx_buffer,
	struct nvgpu_gr_config *config,
	struct nvgpu_gr_ctx *gr_ctx,
	struct nvgpu_mem *inst_block);

int nvgpu_gr_obj_ctx_alloc(struct gk20a *g,
	struct nvgpu_gr_obj_ctx_golden_image *golden_image,
	struct nvgpu_gr_global_ctx_buffer_desc *global_ctx_buffer,
	struct nvgpu_gr_ctx_desc *gr_ctx_desc,
	struct nvgpu_gr_config *config,
	struct nvgpu_gr_ctx *gr_ctx,
	struct nvgpu_gr_subctx *subctx,
	struct vm_gk20a *vm,
	struct nvgpu_mem *inst_block,
	u32 class_num, u32 flags,
	bool cde, bool vpr);

void nvgpu_gr_obj_ctx_set_golden_image_size(
		struct nvgpu_gr_obj_ctx_golden_image *golden_image,
		size_t size);
size_t nvgpu_gr_obj_ctx_get_golden_image_size(
		struct nvgpu_gr_obj_ctx_golden_image *golden_image);

u32 *nvgpu_gr_obj_ctx_get_local_golden_image_ptr(
	struct nvgpu_gr_obj_ctx_golden_image *golden_image);

bool nvgpu_gr_obj_ctx_is_golden_image_ready(
	struct nvgpu_gr_obj_ctx_golden_image *golden_image);

int nvgpu_gr_obj_ctx_init(struct gk20a *g,
	struct nvgpu_gr_obj_ctx_golden_image **gr_golden_image, u32 size);
void nvgpu_gr_obj_ctx_deinit(struct gk20a *g,
	struct nvgpu_gr_obj_ctx_golden_image *golden_image);

#endif /* NVGPU_GR_OBJ_CTX_H */
