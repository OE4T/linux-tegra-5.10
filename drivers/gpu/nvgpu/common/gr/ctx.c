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
#include <nvgpu/gr/global_ctx.h>
#include <nvgpu/gr/ctx.h>
#include <nvgpu/vm.h>
#include <nvgpu/io.h>
#include <nvgpu/gmmu.h>

static void nvgpu_gr_ctx_unmap_global_ctx_buffers(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx,
	struct nvgpu_gr_global_ctx_buffer_desc *global_ctx_buffer,
	struct vm_gk20a *vm);

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
	struct nvgpu_gr_ctx *gr_ctx,
	struct nvgpu_gr_global_ctx_buffer_desc *global_ctx_buffer,
	struct vm_gk20a *vm)
{
	nvgpu_log_fn(g, " ");

	if (gr_ctx != NULL) {
		nvgpu_gr_ctx_unmap_global_ctx_buffers(g, gr_ctx,
			global_ctx_buffer, vm);

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

	/* nothing to do if already initialized */
	if (nvgpu_mem_is_valid(&gr_ctx->preempt_ctxsw_buffer)) {
		return 0;
	}

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
			&gr_ctx->gfxp_rtvcb_ctxsw_buffer);
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

static void nvgpu_gr_ctx_unmap_global_ctx_buffers(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx,
	struct nvgpu_gr_global_ctx_buffer_desc *global_ctx_buffer,
	struct vm_gk20a *vm)
{
	u64 *g_bfr_va = gr_ctx->global_ctx_buffer_va;
	int *g_bfr_index = gr_ctx->global_ctx_buffer_index;
	u32 i;

	nvgpu_log_fn(g, " ");

	for (i = 0; i < NVGPU_GR_CTX_VA_COUNT; i++) {
		nvgpu_gr_global_ctx_buffer_unmap(global_ctx_buffer,
			g_bfr_index[i], vm, g_bfr_va[i]);
	}

	(void) memset(g_bfr_va, 0, sizeof(gr_ctx->global_ctx_buffer_va));
	(void) memset(g_bfr_index, 0, sizeof(gr_ctx->global_ctx_buffer_index));

	gr_ctx->global_ctx_buffer_mapped = false;
}

int nvgpu_gr_ctx_map_global_ctx_buffers(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx,
	struct nvgpu_gr_global_ctx_buffer_desc *global_ctx_buffer,
	struct vm_gk20a *vm, bool vpr)
{
	u64 *g_bfr_va;
	int *g_bfr_index;
	u64 gpu_va = 0ULL;

	nvgpu_log_fn(g, " ");

	g_bfr_va = gr_ctx->global_ctx_buffer_va;
	g_bfr_index = gr_ctx->global_ctx_buffer_index;

	/* Circular Buffer */
	if (vpr && nvgpu_gr_global_ctx_buffer_ready(global_ctx_buffer,
			NVGPU_GR_GLOBAL_CTX_CIRCULAR_VPR)) {
		gpu_va = nvgpu_gr_global_ctx_buffer_map(global_ctx_buffer,
				NVGPU_GR_GLOBAL_CTX_CIRCULAR_VPR,
				vm, NVGPU_VM_MAP_CACHEABLE, true);
		g_bfr_index[NVGPU_GR_CTX_CIRCULAR_VA] = NVGPU_GR_GLOBAL_CTX_CIRCULAR_VPR;
	} else {
		gpu_va = nvgpu_gr_global_ctx_buffer_map(global_ctx_buffer,
				NVGPU_GR_GLOBAL_CTX_CIRCULAR,
				vm, NVGPU_VM_MAP_CACHEABLE, true);
		g_bfr_index[NVGPU_GR_CTX_CIRCULAR_VA] = NVGPU_GR_GLOBAL_CTX_CIRCULAR;
	}
	if (gpu_va == 0ULL) {
		goto clean_up;
	}

	g_bfr_va[NVGPU_GR_CTX_CIRCULAR_VA] = gpu_va;

	/* Attribute Buffer */
	if (vpr && nvgpu_gr_global_ctx_buffer_ready(global_ctx_buffer,
			NVGPU_GR_GLOBAL_CTX_ATTRIBUTE_VPR)) {
		gpu_va = nvgpu_gr_global_ctx_buffer_map(global_ctx_buffer,
				NVGPU_GR_GLOBAL_CTX_ATTRIBUTE_VPR,
				vm, NVGPU_VM_MAP_CACHEABLE, false);
		g_bfr_index[NVGPU_GR_CTX_ATTRIBUTE_VA] = NVGPU_GR_GLOBAL_CTX_ATTRIBUTE_VPR;
	} else {
		gpu_va = nvgpu_gr_global_ctx_buffer_map(global_ctx_buffer,
				NVGPU_GR_GLOBAL_CTX_ATTRIBUTE,
				vm, NVGPU_VM_MAP_CACHEABLE, false);
		g_bfr_index[NVGPU_GR_CTX_ATTRIBUTE_VA] = NVGPU_GR_GLOBAL_CTX_ATTRIBUTE;
	}
	if (gpu_va == 0ULL) {
		goto clean_up;
	}

	g_bfr_va[NVGPU_GR_CTX_ATTRIBUTE_VA] = gpu_va;

	/* Page Pool */
	if (vpr && nvgpu_gr_global_ctx_buffer_ready(global_ctx_buffer,
			NVGPU_GR_GLOBAL_CTX_PAGEPOOL_VPR)) {
		gpu_va = nvgpu_gr_global_ctx_buffer_map(global_ctx_buffer,
				NVGPU_GR_GLOBAL_CTX_PAGEPOOL_VPR,
				vm, NVGPU_VM_MAP_CACHEABLE, true);
		g_bfr_index[NVGPU_GR_CTX_PAGEPOOL_VA] = NVGPU_GR_GLOBAL_CTX_PAGEPOOL_VPR;
	} else {
		gpu_va = nvgpu_gr_global_ctx_buffer_map(global_ctx_buffer,
				NVGPU_GR_GLOBAL_CTX_PAGEPOOL,
				vm, NVGPU_VM_MAP_CACHEABLE, true);
		g_bfr_index[NVGPU_GR_CTX_PAGEPOOL_VA] = NVGPU_GR_GLOBAL_CTX_PAGEPOOL;
	}
	if (gpu_va == 0ULL) {
		goto clean_up;
	}

	g_bfr_va[NVGPU_GR_CTX_PAGEPOOL_VA] = gpu_va;

	/* Priv register Access Map */
	gpu_va = nvgpu_gr_global_ctx_buffer_map(global_ctx_buffer,
			NVGPU_GR_GLOBAL_CTX_PRIV_ACCESS_MAP,
			vm, 0, true);
	if (gpu_va == 0ULL) {
		goto clean_up;
	}

	g_bfr_va[NVGPU_GR_CTX_PRIV_ACCESS_MAP_VA] = gpu_va;
	g_bfr_index[NVGPU_GR_CTX_PRIV_ACCESS_MAP_VA] = NVGPU_GR_GLOBAL_CTX_PRIV_ACCESS_MAP;

#ifdef CONFIG_GK20A_CTXSW_TRACE
	/* FECS trace buffer */
	if (nvgpu_is_enabled(g, NVGPU_FECS_TRACE_VA)) {
		gpu_va = nvgpu_gr_global_ctx_buffer_map(global_ctx_buffer,
				NVGPU_GR_GLOBAL_CTX_FECS_TRACE_BUFFER,
				vm, 0, true);
		if (gpu_va == 0ULL) {
			goto clean_up;
		}

		g_bfr_va[NVGPU_GR_CTX_FECS_TRACE_BUFFER_VA] = gpu_va;
		g_bfr_index[NVGPU_GR_CTX_FECS_TRACE_BUFFER_VA] =
			NVGPU_GR_GLOBAL_CTX_FECS_TRACE_BUFFER;
	}
#endif

	/* RTV circular buffer */
	if (nvgpu_gr_global_ctx_buffer_ready(global_ctx_buffer,
			NVGPU_GR_GLOBAL_CTX_RTV_CIRCULAR_BUFFER)) {
		gpu_va = nvgpu_gr_global_ctx_buffer_map(global_ctx_buffer,
				NVGPU_GR_GLOBAL_CTX_RTV_CIRCULAR_BUFFER,
				vm, 0, true);
		if (gpu_va == 0ULL) {
			goto clean_up;
		}

		g_bfr_va[NVGPU_GR_CTX_RTV_CIRCULAR_BUFFER_VA] = gpu_va;
		g_bfr_index[NVGPU_GR_CTX_RTV_CIRCULAR_BUFFER_VA] =
			NVGPU_GR_GLOBAL_CTX_RTV_CIRCULAR_BUFFER;
	}

	gr_ctx->global_ctx_buffer_mapped = true;

	return 0;

clean_up:
	nvgpu_gr_ctx_unmap_global_ctx_buffers(g, gr_ctx, global_ctx_buffer, vm);

	return -ENOMEM;
}

u64 nvgpu_gr_ctx_get_global_ctx_va(struct nvgpu_gr_ctx *gr_ctx,
	enum nvgpu_gr_ctx_global_ctx_va index)
{
	return gr_ctx->global_ctx_buffer_va[index];
}

/* load saved fresh copy of gloden image into channel gr_ctx */
int nvgpu_gr_ctx_load_golden_ctx_image(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx,
	struct nvgpu_gr_global_ctx_local_golden_image *local_golden_image,
	bool cde)
{
	u64 virt_addr = 0;
	struct nvgpu_mem *mem;

	nvgpu_log_fn(g, " ");

	mem = &gr_ctx->mem;

	nvgpu_gr_global_ctx_load_local_golden_image(g,
		local_golden_image, mem);

	if (g->ops.gr.ctxsw_prog.init_ctxsw_hdr_data != NULL) {
		g->ops.gr.ctxsw_prog.init_ctxsw_hdr_data(g, mem);
	}

	if ((g->ops.gr.ctxsw_prog.set_cde_enabled != NULL) && cde) {
		g->ops.gr.ctxsw_prog.set_cde_enabled(g, mem);
	}

	/* set priv access map */
	g->ops.gr.ctxsw_prog.set_priv_access_map_config_mode(g, mem,
		g->allow_all);
	g->ops.gr.ctxsw_prog.set_priv_access_map_addr(g, mem,
		nvgpu_gr_ctx_get_global_ctx_va(gr_ctx,
			NVGPU_GR_CTX_PRIV_ACCESS_MAP_VA));

	/* disable verif features */
	g->ops.gr.ctxsw_prog.disable_verif_features(g, mem);

	if (g->ops.gr.ctxsw_prog.set_pmu_options_boost_clock_frequencies !=
			NULL) {
		g->ops.gr.ctxsw_prog.set_pmu_options_boost_clock_frequencies(g,
			mem, gr_ctx->boosted_ctx);
	}

	nvgpu_log(g, gpu_dbg_info, "write patch count = %d",
			gr_ctx->patch_ctx.data_count);
	g->ops.gr.ctxsw_prog.set_patch_count(g, mem,
		gr_ctx->patch_ctx.data_count);
	g->ops.gr.ctxsw_prog.set_patch_addr(g, mem,
		gr_ctx->patch_ctx.mem.gpu_va);

	/* Update main header region of the context buffer with the info needed
	 * for PM context switching, including mode and possibly a pointer to
	 * the PM backing store.
	 */
	if (gr_ctx->pm_ctx.pm_mode !=
	    g->ops.gr.ctxsw_prog.hw_get_pm_mode_no_ctxsw()) {
		if (gr_ctx->pm_ctx.mem.gpu_va == 0ULL) {
			nvgpu_err(g,
				"context switched pm with no pm buffer!");
			return -EFAULT;
		}

		virt_addr = gr_ctx->pm_ctx.mem.gpu_va;
	} else {
		virt_addr = 0;
	}

	g->ops.gr.ctxsw_prog.set_pm_mode(g, mem, gr_ctx->pm_ctx.pm_mode);
	g->ops.gr.ctxsw_prog.set_pm_ptr(g, mem, virt_addr);

	return 0;
}

/*
 * Context state can be written directly, or "patched" at times. So that code
 * can be used in either situation it is written using a series of
 * _ctx_patch_write(..., patch) statements. However any necessary map overhead
 * should be minimized; thus, bundle the sequence of these writes together, and
 * set them up and close with _ctx_patch_write_begin/_ctx_patch_write_end.
 */
int nvgpu_gr_ctx_patch_write_begin(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx,
	bool update_patch_count)
{
	if (update_patch_count) {
		/* reset patch count if ucode has already processed it */
		gr_ctx->patch_ctx.data_count =
			g->ops.gr.ctxsw_prog.get_patch_count(g, &gr_ctx->mem);
		nvgpu_log(g, gpu_dbg_info, "patch count reset to %d",
					gr_ctx->patch_ctx.data_count);
	}
	return 0;
}

void nvgpu_gr_ctx_patch_write_end(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx,
	bool update_patch_count)
{
	/* Write context count to context image if it is mapped */
	if (update_patch_count) {
		g->ops.gr.ctxsw_prog.set_patch_count(g, &gr_ctx->mem,
			     gr_ctx->patch_ctx.data_count);
		nvgpu_log(g, gpu_dbg_info, "write patch count %d",
			gr_ctx->patch_ctx.data_count);
	}
}

void nvgpu_gr_ctx_patch_write(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx,
	u32 addr, u32 data, bool patch)
{
	if (patch) {
		u32 patch_slot = gr_ctx->patch_ctx.data_count *
				PATCH_CTX_SLOTS_REQUIRED_PER_ENTRY;

		if (patch_slot > (PATCH_CTX_ENTRIES_FROM_SIZE(
					gr_ctx->patch_ctx.mem.size) -
				PATCH_CTX_SLOTS_REQUIRED_PER_ENTRY)) {
			nvgpu_err(g, "failed to access patch_slot %d",
				patch_slot);
			return;
		}

		nvgpu_mem_wr32(g, &gr_ctx->patch_ctx.mem, patch_slot, addr);
		nvgpu_mem_wr32(g, &gr_ctx->patch_ctx.mem, patch_slot + 1U, data);
		gr_ctx->patch_ctx.data_count++;

		nvgpu_log(g, gpu_dbg_info,
			"patch addr = 0x%x data = 0x%x data_count %d",
			addr, data, gr_ctx->patch_ctx.data_count);
	} else {
		nvgpu_writel(g, addr, data);
	}
}
