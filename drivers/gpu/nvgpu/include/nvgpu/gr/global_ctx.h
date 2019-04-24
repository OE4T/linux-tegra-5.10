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
#ifndef NVGPU_GR_GLOBAL_CTX_H
#define NVGPU_GR_GLOBAL_CTX_H

#include <nvgpu/nvgpu_mem.h>

#define NVGPU_GR_GLOBAL_CTX_PRIV_ACCESS_MAP_SIZE	(512U * 1024U)

struct gk20a;
struct nvgpu_mem;
struct vm_gk20a;
struct nvgpu_gr_global_ctx_buffer_desc;
struct nvgpu_gr_global_ctx_local_golden_image;

typedef void (*global_ctx_mem_destroy_fn)(struct gk20a *g,
				struct nvgpu_mem *mem);

#define NVGPU_GR_GLOBAL_CTX_CIRCULAR			0U
#define NVGPU_GR_GLOBAL_CTX_PAGEPOOL			1U
#define NVGPU_GR_GLOBAL_CTX_ATTRIBUTE			2U
#define NVGPU_GR_GLOBAL_CTX_CIRCULAR_VPR		3U
#define NVGPU_GR_GLOBAL_CTX_PAGEPOOL_VPR		4U
#define NVGPU_GR_GLOBAL_CTX_ATTRIBUTE_VPR		5U
#define NVGPU_GR_GLOBAL_CTX_PRIV_ACCESS_MAP		6U
#define NVGPU_GR_GLOBAL_CTX_RTV_CIRCULAR_BUFFER		7U
#define NVGPU_GR_GLOBAL_CTX_FECS_TRACE_BUFFER		8U
#define NVGPU_GR_GLOBAL_CTX_COUNT			9U

struct nvgpu_gr_global_ctx_buffer_desc *nvgpu_gr_global_ctx_desc_alloc(
	struct gk20a *g);
void nvgpu_gr_global_ctx_desc_free(struct gk20a *g,
	struct nvgpu_gr_global_ctx_buffer_desc *desc);

void nvgpu_gr_global_ctx_set_size(struct nvgpu_gr_global_ctx_buffer_desc *desc,
	u32 index, size_t size);
size_t nvgpu_gr_global_ctx_get_size(struct nvgpu_gr_global_ctx_buffer_desc *desc,
	u32 index);

int nvgpu_gr_global_ctx_buffer_alloc(struct gk20a *g,
	struct nvgpu_gr_global_ctx_buffer_desc *desc);
void nvgpu_gr_global_ctx_buffer_free(struct gk20a *g,
	struct nvgpu_gr_global_ctx_buffer_desc *desc);

u64 nvgpu_gr_global_ctx_buffer_map(struct nvgpu_gr_global_ctx_buffer_desc *desc,
	u32 index,
	struct vm_gk20a *vm, u32 flags, bool priv);
void nvgpu_gr_global_ctx_buffer_unmap(
	struct nvgpu_gr_global_ctx_buffer_desc *desc,
	u32 index,
	struct vm_gk20a *vm, u64 gpu_va);

struct nvgpu_mem *nvgpu_gr_global_ctx_buffer_get_mem(
	struct nvgpu_gr_global_ctx_buffer_desc *desc,
	u32 index);
bool nvgpu_gr_global_ctx_buffer_ready(
	struct nvgpu_gr_global_ctx_buffer_desc *desc,
	u32 index);

struct nvgpu_gr_global_ctx_local_golden_image *
nvgpu_gr_global_ctx_init_local_golden_image(struct gk20a *g,
	struct nvgpu_mem *source_mem, size_t size);
void nvgpu_gr_global_ctx_load_local_golden_image(struct gk20a *g,
	struct nvgpu_gr_global_ctx_local_golden_image *local_golden_image,
	struct nvgpu_mem *target_mem);
void nvgpu_gr_global_ctx_deinit_local_golden_image(struct gk20a *g,
	struct nvgpu_gr_global_ctx_local_golden_image *local_golden_image);
u32 *nvgpu_gr_global_ctx_get_local_golden_image_ptr(
	struct nvgpu_gr_global_ctx_local_golden_image *local_golden_image);

#endif /* NVGPU_GR_GLOBAL_CTX_H */
