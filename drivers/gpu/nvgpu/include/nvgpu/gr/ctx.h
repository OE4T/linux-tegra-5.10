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

#ifndef NVGPU_INCLUDE_GR_CTX_H
#define NVGPU_INCLUDE_GR_CTX_H

#include <nvgpu/types.h>
#include <nvgpu/nvgpu_mem.h>

struct gk20a;
struct vm_gk20a;

enum nvgpu_gr_ctx_index {
	NVGPU_GR_CTX_CTX		= 0,
	NVGPU_GR_CTX_PM_CTX		,
	NVGPU_GR_CTX_PATCH_CTX		,
	NVGPU_GR_CTX_PREEMPT_CTXSW	,
	NVGPU_GR_CTX_SPILL_CTXSW	,
	NVGPU_GR_CTX_BETACB_CTXSW	,
	NVGPU_GR_CTX_PAGEPOOL_CTXSW	,
	NVGPU_GR_CTX_GFXP_RTVCB_CTXSW	,
	NVGPU_GR_CTX_COUNT
};

/*
 * either ATTRIBUTE or ATTRIBUTE_VPR maps to NVGPU_GR_CTX_ATTRIBUTE_VA
*/
enum nvgpu_gr_ctx_global_ctx_va {
	NVGPU_GR_CTX_CIRCULAR_VA		= 0,
	NVGPU_GR_CTX_PAGEPOOL_VA		= 1,
	NVGPU_GR_CTX_ATTRIBUTE_VA		= 2,
	NVGPU_GR_CTX_PRIV_ACCESS_MAP_VA		= 3,
	NVGPU_GR_CTX_RTV_CIRCULAR_BUFFER_VA	= 4,
	NVGPU_GR_CTX_FECS_TRACE_BUFFER_VA	= 5,
	NVGPU_GR_CTX_VA_COUNT			= 6
};

struct patch_desc {
	struct nvgpu_mem mem;
	u32 data_count;
};

struct zcull_ctx_desc {
	u64 gpu_va;
	u32 ctx_sw_mode;
};

struct pm_ctx_desc {
	struct nvgpu_mem mem;
	u32 pm_mode;
};

struct nvgpu_gr_ctx_desc {
	u32 size[NVGPU_GR_CTX_COUNT];
};

struct nvgpu_gr_ctx {
	u32 ctx_id;
	bool ctx_id_valid;
	struct nvgpu_mem mem;

	struct nvgpu_mem preempt_ctxsw_buffer;
	struct nvgpu_mem spill_ctxsw_buffer;
	struct nvgpu_mem betacb_ctxsw_buffer;
	struct nvgpu_mem pagepool_ctxsw_buffer;
	struct nvgpu_mem gfxp_rtvcb_ctxsw_buffer;

	struct patch_desc	patch_ctx;
	struct zcull_ctx_desc	zcull_ctx;
	struct pm_ctx_desc	pm_ctx;

	u32 graphics_preempt_mode;
	u32 compute_preempt_mode;

	bool golden_img_loaded;
	bool cilp_preempt_pending;
	bool boosted_ctx;

#ifdef CONFIG_TEGRA_GR_VIRTUALIZATION
	u64 virt_ctx;
#endif

	u64	global_ctx_buffer_va[NVGPU_GR_CTX_VA_COUNT];
	int	global_ctx_buffer_index[NVGPU_GR_CTX_VA_COUNT];
	bool	global_ctx_buffer_mapped;

	u32 tsgid;
};

struct nvgpu_gr_ctx_desc *
nvgpu_gr_ctx_desc_alloc(struct gk20a *g);
void nvgpu_gr_ctx_desc_free(struct gk20a *g,
	struct nvgpu_gr_ctx_desc *desc);

void nvgpu_gr_ctx_set_size(struct nvgpu_gr_ctx_desc *gr_ctx_desc,
	enum nvgpu_gr_ctx_index index, u32 size);

int nvgpu_gr_ctx_alloc(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx,
	struct nvgpu_gr_ctx_desc *gr_ctx_desc,
	struct vm_gk20a *vm);
void nvgpu_gr_ctx_free(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx,
	struct nvgpu_gr_global_ctx_buffer_desc *global_ctx_buffer,
	struct vm_gk20a *vm);

int nvgpu_gr_ctx_alloc_pm_ctx(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx,
	struct nvgpu_gr_ctx_desc *gr_ctx_desc,
	struct vm_gk20a *vm,
	u64 gpu_va);
void nvgpu_gr_ctx_free_pm_ctx(struct gk20a *g, struct vm_gk20a *vm,
	struct nvgpu_gr_ctx *gr_ctx);

int nvgpu_gr_ctx_alloc_patch_ctx(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx,
	struct nvgpu_gr_ctx_desc *gr_ctx_desc,
	struct vm_gk20a *vm);
void nvgpu_gr_ctx_free_patch_ctx(struct gk20a *g, struct vm_gk20a *vm,
	struct nvgpu_gr_ctx *gr_ctx);

void nvgpu_gr_ctx_set_zcull_ctx(struct gk20a *g, struct nvgpu_gr_ctx *gr_ctx,
	u32 mode, u64 gpu_va);

int nvgpu_gr_ctx_alloc_ctxsw_buffers(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx,
	struct nvgpu_gr_ctx_desc *gr_ctx_desc,
	struct vm_gk20a *vm);

int nvgpu_gr_ctx_map_global_ctx_buffers(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx,
	struct nvgpu_gr_global_ctx_buffer_desc *global_ctx_buffer,
	struct vm_gk20a *vm, bool vpr);
u64 nvgpu_gr_ctx_get_global_ctx_va(struct nvgpu_gr_ctx *gr_ctx,
	enum nvgpu_gr_ctx_global_ctx_va index);

int nvgpu_gr_ctx_load_golden_ctx_image(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx,
	struct nvgpu_gr_global_ctx_local_golden_image *local_golden_image,
	bool cde);

#endif /* NVGPU_INCLUDE_GR_CTX_H */
