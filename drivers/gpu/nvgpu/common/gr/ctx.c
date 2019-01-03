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

#include <nvgpu/gk20a.h>
#include <nvgpu/gr/ctx.h>
#include <nvgpu/vm.h>
#include <nvgpu/gmmu.h>

struct nvgpu_gr_ctx_desc *
nvgpu_gr_ctx_desc_alloc(struct gk20a *g)
{
	struct nvgpu_gr_ctx_desc *desc = nvgpu_kzalloc(g, sizeof(*desc));
	return desc;
}

void nvgpu_gr_ctx_desc_free(struct gk20a *g,
	struct nvgpu_gr_ctx_desc *desc)
{
	nvgpu_kfree(g, desc);
}

void nvgpu_gr_ctx_set_size(struct nvgpu_gr_ctx_desc *gr_ctx_desc,
	enum nvgpu_gr_ctx_index index, u32 size)
{
	gr_ctx_desc->size[index] = size;
}

int nvgpu_gr_ctx_alloc(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx,
	struct nvgpu_gr_ctx_desc *gr_ctx_desc,
	struct vm_gk20a *vm)
{
	int err = 0;

	nvgpu_log_fn(g, " ");

	if (gr_ctx_desc->size[NVGPU_GR_CTX_CTX] == 0U) {
		return -EINVAL;
	}

	err = nvgpu_dma_alloc(g, gr_ctx_desc->size[NVGPU_GR_CTX_CTX],
			&gr_ctx->mem);
	if (err != 0) {
		return err;
	}

	gr_ctx->mem.gpu_va = nvgpu_gmmu_map(vm,
					&gr_ctx->mem,
					gr_ctx->mem.size,
					0, /* not GPU-cacheable */
					gk20a_mem_flag_none, true,
					gr_ctx->mem.aperture);
	if (gr_ctx->mem.gpu_va == 0ULL) {
		goto err_free_mem;
	}

	return 0;

err_free_mem:
	nvgpu_dma_free(g, &gr_ctx->mem);

	return err;
}

void nvgpu_gr_ctx_free(struct gk20a *g,
	struct vm_gk20a *vm, struct nvgpu_gr_ctx *gr_ctx)
{
	nvgpu_log_fn(g, " ");

	if (gr_ctx != NULL) {
		nvgpu_gr_ctx_free_pm_ctx(g, vm, gr_ctx);
		nvgpu_gr_ctx_free_patch_ctx(g, vm, gr_ctx);

		if (nvgpu_mem_is_valid(&gr_ctx->gfxp_rtvcb_ctxsw_buffer)) {
			nvgpu_dma_unmap_free(vm,
				&gr_ctx->gfxp_rtvcb_ctxsw_buffer);
		}
		nvgpu_dma_unmap_free(vm, &gr_ctx->pagepool_ctxsw_buffer);
		nvgpu_dma_unmap_free(vm, &gr_ctx->betacb_ctxsw_buffer);
		nvgpu_dma_unmap_free(vm, &gr_ctx->spill_ctxsw_buffer);
		nvgpu_dma_unmap_free(vm, &gr_ctx->preempt_ctxsw_buffer);

		nvgpu_dma_unmap_free(vm, &gr_ctx->mem);
		(void) memset(gr_ctx, 0, sizeof(*gr_ctx));
	}
}

int nvgpu_gr_ctx_alloc_pm_ctx(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx,
	struct nvgpu_gr_ctx_desc *gr_ctx_desc,
	struct vm_gk20a *vm,
	u64 gpu_va)
{
	struct pm_ctx_desc *pm_ctx = &gr_ctx->pm_ctx;
	int err;

	if (pm_ctx->mem.gpu_va != 0ULL) {
		return 0;
	}

	err = nvgpu_dma_alloc_sys(g, gr_ctx_desc->size[NVGPU_GR_CTX_PM_CTX],
			&pm_ctx->mem);
	if (err != 0) {
		nvgpu_err(g,
			"failed to allocate pm ctx buffer");
		return err;
	}

	pm_ctx->mem.gpu_va = nvgpu_gmmu_map_fixed(vm,
					&pm_ctx->mem,
					gpu_va,
					pm_ctx->mem.size,
					NVGPU_VM_MAP_CACHEABLE,
					gk20a_mem_flag_none, true,
					pm_ctx->mem.aperture);
	if (pm_ctx->mem.gpu_va == 0ULL) {
		nvgpu_err(g,
			"failed to map pm ctxt buffer");
		nvgpu_dma_free(g, &pm_ctx->mem);
		return -ENOMEM;
	}

	return 0;
}

void nvgpu_gr_ctx_free_pm_ctx(struct gk20a *g, struct vm_gk20a *vm,
	struct nvgpu_gr_ctx *gr_ctx)
{
	struct pm_ctx_desc *pm_ctx = &gr_ctx->pm_ctx;

	if (pm_ctx->mem.gpu_va != 0ULL) {
		nvgpu_gmmu_unmap(vm, &pm_ctx->mem, pm_ctx->mem.gpu_va);

		nvgpu_dma_free(g, &pm_ctx->mem);
	}
}

int nvgpu_gr_ctx_alloc_patch_ctx(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx,
	struct nvgpu_gr_ctx_desc *gr_ctx_desc,
	struct vm_gk20a *vm)
{
	struct patch_desc *patch_ctx = &gr_ctx->patch_ctx;
	int err = 0;

	nvgpu_log(g, gpu_dbg_info, "patch buffer size in entries: %d",
		gr_ctx_desc->size[NVGPU_GR_CTX_PATCH_CTX]);

	err = nvgpu_dma_alloc_map_sys(vm, gr_ctx_desc->size[NVGPU_GR_CTX_PATCH_CTX],
			&patch_ctx->mem);
	if (err != 0) {
		return err;
	}

	return 0;
}

void nvgpu_gr_ctx_free_patch_ctx(struct gk20a *g, struct vm_gk20a *vm,
	struct nvgpu_gr_ctx *gr_ctx)
{
	struct patch_desc *patch_ctx = &gr_ctx->patch_ctx;

	if (patch_ctx->mem.gpu_va != 0ULL) {
		nvgpu_gmmu_unmap(vm, &patch_ctx->mem,
				 patch_ctx->mem.gpu_va);
	}

	nvgpu_dma_free(g, &patch_ctx->mem);
	patch_ctx->data_count = 0;
}

void nvgpu_gr_ctx_set_zcull_ctx(struct gk20a *g, struct nvgpu_gr_ctx *gr_ctx,
	u32 mode, u64 gpu_va)
{
	struct zcull_ctx_desc *zcull_ctx = &gr_ctx->zcull_ctx;

	zcull_ctx->ctx_sw_mode = mode;
	zcull_ctx->gpu_va = gpu_va;
}

static int nvgpu_gr_ctx_alloc_ctxsw_buffer(struct vm_gk20a *vm, size_t size,
	struct nvgpu_mem *mem)
{
	int err;

	err = nvgpu_dma_alloc_sys(vm->mm->g, size, mem);
	if (err != 0) {
		return err;
	}

	mem->gpu_va = nvgpu_gmmu_map(vm,
				mem,
				mem->aligned_size,
				NVGPU_VM_MAP_CACHEABLE,
				gk20a_mem_flag_none,
				false,
				mem->aperture);
	if (mem->gpu_va == 0ULL) {
		nvgpu_dma_free(vm->mm->g, mem);
		return -ENOMEM;
	}

	return 0;
}

int nvgpu_gr_ctx_alloc_ctxsw_buffers(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx,
	struct nvgpu_gr_ctx_desc *gr_ctx_desc,
	struct vm_gk20a *vm)
{
	int err;

	if (gr_ctx_desc->size[NVGPU_GR_CTX_PREEMPT_CTXSW] == 0U ||
	    gr_ctx_desc->size[NVGPU_GR_CTX_SPILL_CTXSW] == 0U ||
	    gr_ctx_desc->size[NVGPU_GR_CTX_BETACB_CTXSW] == 0U ||
	    gr_ctx_desc->size[NVGPU_GR_CTX_PAGEPOOL_CTXSW] == 0U) {
		return -EINVAL;
	}

	err = nvgpu_gr_ctx_alloc_ctxsw_buffer(vm,
			gr_ctx_desc->size[NVGPU_GR_CTX_PREEMPT_CTXSW],
			&gr_ctx->preempt_ctxsw_buffer);
	if (err != 0) {
		nvgpu_err(g, "cannot allocate preempt buffer");
		goto fail;
	}

	err = nvgpu_gr_ctx_alloc_ctxsw_buffer(vm,
			gr_ctx_desc->size[NVGPU_GR_CTX_SPILL_CTXSW],
			&gr_ctx->spill_ctxsw_buffer);
	if (err != 0) {
		nvgpu_err(g, "cannot allocate spill buffer");
		goto fail_free_preempt;
	}

	err = nvgpu_gr_ctx_alloc_ctxsw_buffer(vm,
			gr_ctx_desc->size[NVGPU_GR_CTX_BETACB_CTXSW],
			&gr_ctx->betacb_ctxsw_buffer);
	if (err != 0) {
		nvgpu_err(g, "cannot allocate beta buffer");
		goto fail_free_spill;
	}

	err = nvgpu_gr_ctx_alloc_ctxsw_buffer(vm,
			gr_ctx_desc->size[NVGPU_GR_CTX_PAGEPOOL_CTXSW],
			&gr_ctx->pagepool_ctxsw_buffer);
	if (err != 0) {
		nvgpu_err(g, "cannot allocate page pool");
		goto fail_free_betacb;
	}

	if (gr_ctx_desc->size[NVGPU_GR_CTX_GFXP_RTVCB_CTXSW] != 0U) {
		err = nvgpu_gr_ctx_alloc_ctxsw_buffer(vm,
			gr_ctx_desc->size[NVGPU_GR_CTX_GFXP_RTVCB_CTXSW],
			&gr_ctx->pagepool_ctxsw_buffer);
		if (err != 0) {
			nvgpu_err(g, "cannot allocate gfxp rtvcb");
			goto fail_free_pagepool;
		}
	}

	return 0;

fail_free_pagepool:
	nvgpu_dma_unmap_free(vm, &gr_ctx->pagepool_ctxsw_buffer);
fail_free_betacb:
	nvgpu_dma_unmap_free(vm, &gr_ctx->betacb_ctxsw_buffer);
fail_free_spill:
	nvgpu_dma_unmap_free(vm, &gr_ctx->spill_ctxsw_buffer);
fail_free_preempt:
	nvgpu_dma_unmap_free(vm, &gr_ctx->preempt_ctxsw_buffer);
fail:
	return err;
}
