/*
 * Virtualized GPU Graphics
 *
 * Copyright (c) 2019-2020, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/kmem.h>
#include <nvgpu/bug.h>
#include <nvgpu/dma.h>
#include <nvgpu/dma.h>
#include <nvgpu/vgpu/vgpu_ivc.h>
#include <nvgpu/vgpu/vgpu.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/gr/global_ctx.h>
#include <nvgpu/gr/ctx.h>
#include <nvgpu/gr/obj_ctx.h>
#include <nvgpu/gr/hwpm_map.h>
#include <nvgpu/gr/gr_utils.h>

#include "common/gr/ctx_priv.h"

#include "ctx_vgpu.h"
#include "common/vgpu/ivc/comm_vgpu.h"

int vgpu_gr_alloc_gr_ctx(struct gk20a *g,
			struct nvgpu_gr_ctx *gr_ctx,
			struct vm_gk20a *vm)
{
	struct tegra_vgpu_cmd_msg msg = {0};
	struct tegra_vgpu_gr_ctx_params *p = &msg.params.gr_ctx;
	struct nvgpu_gr_obj_ctx_golden_image *gr_golden_image =
					nvgpu_gr_get_golden_image_ptr(g);
	u32 golden_image_size;
	int err;

	nvgpu_log_fn(g, " ");

	golden_image_size =
		nvgpu_gr_obj_ctx_get_golden_image_size(gr_golden_image);
	if (golden_image_size == 0) {
		return -EINVAL;
	}

	gr_ctx->mem.gpu_va = nvgpu_vm_alloc_va(vm,
				       golden_image_size,
				       GMMU_PAGE_SIZE_KERNEL);

	if (!gr_ctx->mem.gpu_va) {
		return -ENOMEM;
	}
	gr_ctx->mem.size = golden_image_size;
	gr_ctx->mem.aperture = APERTURE_SYSMEM;

	msg.cmd = TEGRA_VGPU_CMD_GR_CTX_ALLOC;
	msg.handle = vgpu_get_handle(g);
	p->as_handle = vm->handle;
	p->gr_ctx_va = gr_ctx->mem.gpu_va;
	p->tsg_id = gr_ctx->tsgid;
#ifdef CONFIG_NVGPU_SM_DIVERSITY
	p->sm_diversity_config = gr_ctx->sm_diversity_config;
#else
	p->sm_diversity_config = NVGPU_DEFAULT_SM_DIVERSITY_CONFIG;
#endif
	err = vgpu_comm_sendrecv(&msg, sizeof(msg), sizeof(msg));
	err = err ? err : msg.ret;

	if (unlikely(err)) {
		nvgpu_err(g, "fail to alloc gr_ctx");
		nvgpu_vm_free_va(vm, gr_ctx->mem.gpu_va,
				 GMMU_PAGE_SIZE_KERNEL);
		gr_ctx->mem.aperture = APERTURE_INVALID;
	}

	return err;
}

void vgpu_gr_free_gr_ctx(struct gk20a *g,
			 struct vm_gk20a *vm, struct nvgpu_gr_ctx *gr_ctx)
{
	nvgpu_log_fn(g, " ");

	if (gr_ctx->mem.gpu_va) {
		struct tegra_vgpu_cmd_msg msg;
		struct tegra_vgpu_gr_ctx_params *p = &msg.params.gr_ctx;
		int err;

		msg.cmd = TEGRA_VGPU_CMD_GR_CTX_FREE;
		msg.handle = vgpu_get_handle(g);
		p->tsg_id = gr_ctx->tsgid;
		err = vgpu_comm_sendrecv(&msg, sizeof(msg), sizeof(msg));
		WARN_ON(err || msg.ret);

		nvgpu_vm_free_va(vm, gr_ctx->mem.gpu_va,
				 GMMU_PAGE_SIZE_KERNEL);

		vgpu_gr_unmap_global_ctx_buffers(g, gr_ctx, vm);
		vgpu_gr_free_patch_ctx(g, vm, gr_ctx);
		vgpu_gr_free_pm_ctx(g, vm, gr_ctx);

#ifdef CONFIG_NVGPU_GRAPHICS
		nvgpu_dma_unmap_free(vm, &gr_ctx->pagepool_ctxsw_buffer);
		nvgpu_dma_unmap_free(vm, &gr_ctx->betacb_ctxsw_buffer);
		nvgpu_dma_unmap_free(vm, &gr_ctx->spill_ctxsw_buffer);
		nvgpu_dma_unmap_free(vm, &gr_ctx->preempt_ctxsw_buffer);
#endif

		(void) memset(gr_ctx, 0, sizeof(*gr_ctx));
	}
}

int vgpu_gr_alloc_patch_ctx(struct gk20a *g, struct nvgpu_gr_ctx *gr_ctx,
			struct vm_gk20a *ch_vm, u64 virt_ctx)
{
	struct patch_desc *patch_ctx;
	struct tegra_vgpu_cmd_msg msg;
	struct tegra_vgpu_ch_ctx_params *p = &msg.params.ch_ctx;
	int err;

	nvgpu_log_fn(g, " ");

	patch_ctx = &gr_ctx->patch_ctx;
	patch_ctx->mem.size = 1024 * sizeof(u32);
	patch_ctx->mem.gpu_va = nvgpu_vm_alloc_va(ch_vm,
						  patch_ctx->mem.size,
						  GMMU_PAGE_SIZE_KERNEL);
	if (!patch_ctx->mem.gpu_va) {
		return -ENOMEM;
	}

	msg.cmd = TEGRA_VGPU_CMD_CHANNEL_ALLOC_GR_PATCH_CTX;
	msg.handle = vgpu_get_handle(g);
	p->handle = virt_ctx;
	p->patch_ctx_va = patch_ctx->mem.gpu_va;
	err = vgpu_comm_sendrecv(&msg, sizeof(msg), sizeof(msg));
	if (err || msg.ret) {
		nvgpu_vm_free_va(ch_vm, patch_ctx->mem.gpu_va,
				 GMMU_PAGE_SIZE_KERNEL);
		err = -ENOMEM;
	}

	return err;
}

void vgpu_gr_free_patch_ctx(struct gk20a *g, struct vm_gk20a *vm,
			struct nvgpu_gr_ctx *gr_ctx)
{
	struct patch_desc *patch_ctx = &gr_ctx->patch_ctx;

	nvgpu_log_fn(g, " ");

	if (patch_ctx->mem.gpu_va) {
		/* server will free on channel close */

		nvgpu_vm_free_va(vm, patch_ctx->mem.gpu_va,
				 GMMU_PAGE_SIZE_KERNEL);
		patch_ctx->mem.gpu_va = 0;
	}
}

int vgpu_gr_alloc_pm_ctx(struct gk20a *g, struct nvgpu_gr_ctx *gr_ctx,
			struct vm_gk20a *vm)
{
	struct pm_ctx_desc *pm_ctx = &gr_ctx->pm_ctx;
	struct nvgpu_gr_hwpm_map *gr_hwpm_map = nvgpu_gr_get_hwpm_map_ptr(g);

	nvgpu_log_fn(g, " ");

	if (pm_ctx->mem.gpu_va != 0ULL) {
		return 0;
	}

	pm_ctx->mem.gpu_va = nvgpu_vm_alloc_va(vm,
			nvgpu_gr_hwpm_map_get_size(gr_hwpm_map),
			GMMU_PAGE_SIZE_KERNEL);

	if (!pm_ctx->mem.gpu_va) {
		nvgpu_err(g, "failed to map pm ctxt buffer");
		return -ENOMEM;
	}

	pm_ctx->mem.size = nvgpu_gr_hwpm_map_get_size(gr_hwpm_map);
	return 0;
}

void vgpu_gr_free_pm_ctx(struct gk20a *g, struct vm_gk20a *vm,
			struct nvgpu_gr_ctx *gr_ctx)
{
	struct pm_ctx_desc *pm_ctx = &gr_ctx->pm_ctx;

	nvgpu_log_fn(g, " ");

	/* check if hwpm was ever initialized. If not, nothing to do */
	if (pm_ctx->mem.gpu_va == 0) {
		return;
	}

	/* server will free on channel close */

	nvgpu_vm_free_va(vm, pm_ctx->mem.gpu_va,
			 GMMU_PAGE_SIZE_KERNEL);
	pm_ctx->mem.gpu_va = 0;
}

void vgpu_gr_unmap_global_ctx_buffers(struct gk20a *g,
			struct nvgpu_gr_ctx *gr_ctx, struct vm_gk20a *ch_vm)
{
	u64 *g_bfr_va = gr_ctx->global_ctx_buffer_va;
	u32 i;

	nvgpu_log_fn(g, " ");

	if (gr_ctx->global_ctx_buffer_mapped) {
		/* server will unmap on channel close */

		for (i = 0; i < NVGPU_GR_CTX_VA_COUNT; i++) {
			if (g_bfr_va[i]) {
				nvgpu_vm_free_va(ch_vm, g_bfr_va[i],
						 GMMU_PAGE_SIZE_KERNEL);
				g_bfr_va[i] = 0;
			}
		}

		gr_ctx->global_ctx_buffer_mapped = false;
	}
}

int vgpu_gr_map_global_ctx_buffers(struct gk20a *g, struct nvgpu_gr_ctx *gr_ctx,
		struct nvgpu_gr_global_ctx_buffer_desc *global_ctx_buffer,
		struct vm_gk20a *ch_vm, u64 virt_ctx)
{
	struct tegra_vgpu_cmd_msg msg;
	struct tegra_vgpu_ch_ctx_params *p = &msg.params.ch_ctx;
	u64 *g_bfr_va;
	u64 gpu_va;
	u32 i;
	int err;

	nvgpu_log_fn(g, " ");

	g_bfr_va = gr_ctx->global_ctx_buffer_va;

	/*
	 * MIG supports only compute class.
	 * Allocate BUNDLE_CB, PAGEPOOL, ATTRIBUTE_CB and RTV_CB
	 * if 2D/3D/I2M classes(graphics) are supported.
	 */
	if (!nvgpu_is_enabled(g, NVGPU_SUPPORT_MIG)) {
		/* Circular Buffer */
		gpu_va = nvgpu_vm_alloc_va(ch_vm,
				nvgpu_gr_global_ctx_get_size(global_ctx_buffer,
					NVGPU_GR_GLOBAL_CTX_CIRCULAR),
				GMMU_PAGE_SIZE_KERNEL);

		if (!gpu_va) {
			goto clean_up;
		}
		g_bfr_va[NVGPU_GR_CTX_CIRCULAR_VA] = gpu_va;

		/* Attribute Buffer */
		gpu_va = nvgpu_vm_alloc_va(ch_vm,
				nvgpu_gr_global_ctx_get_size(global_ctx_buffer,
					NVGPU_GR_GLOBAL_CTX_ATTRIBUTE),
				GMMU_PAGE_SIZE_KERNEL);

		if (!gpu_va) {
			goto clean_up;
		}
		g_bfr_va[NVGPU_GR_CTX_ATTRIBUTE_VA] = gpu_va;

		/* Page Pool */
		gpu_va = nvgpu_vm_alloc_va(ch_vm,
				nvgpu_gr_global_ctx_get_size(global_ctx_buffer,
					NVGPU_GR_GLOBAL_CTX_PAGEPOOL),
				GMMU_PAGE_SIZE_KERNEL);
		if (!gpu_va) {
			goto clean_up;
		}
		g_bfr_va[NVGPU_GR_CTX_PAGEPOOL_VA] = gpu_va;
	}

	/* Priv register Access Map */
	gpu_va = nvgpu_vm_alloc_va(ch_vm,
			nvgpu_gr_global_ctx_get_size(global_ctx_buffer,
				NVGPU_GR_GLOBAL_CTX_PRIV_ACCESS_MAP),
			GMMU_PAGE_SIZE_KERNEL);
	if (!gpu_va) {
		goto clean_up;
	}
	g_bfr_va[NVGPU_GR_CTX_PRIV_ACCESS_MAP_VA] = gpu_va;

	/* FECS trace Buffer */
#ifdef CONFIG_NVGPU_FECS_TRACE
	gpu_va = nvgpu_vm_alloc_va(ch_vm,
		nvgpu_gr_global_ctx_get_size(global_ctx_buffer,
			NVGPU_GR_GLOBAL_CTX_FECS_TRACE_BUFFER),
		GMMU_PAGE_SIZE_KERNEL);

	if (!gpu_va)
		goto clean_up;

	g_bfr_va[NVGPU_GR_CTX_FECS_TRACE_BUFFER_VA] = gpu_va;
#endif
	msg.cmd = TEGRA_VGPU_CMD_CHANNEL_MAP_GR_GLOBAL_CTX;
	msg.handle = vgpu_get_handle(g);
	p->handle = virt_ctx;
	p->cb_va = g_bfr_va[NVGPU_GR_CTX_CIRCULAR_VA];
	p->attr_va = g_bfr_va[NVGPU_GR_CTX_ATTRIBUTE_VA];
	p->page_pool_va = g_bfr_va[NVGPU_GR_CTX_PAGEPOOL_VA];
	p->priv_access_map_va = g_bfr_va[NVGPU_GR_CTX_PRIV_ACCESS_MAP_VA];
#ifdef CONFIG_NVGPU_FECS_TRACE
	p->fecs_trace_va = g_bfr_va[NVGPU_GR_CTX_FECS_TRACE_BUFFER_VA];
#endif
	err = vgpu_comm_sendrecv(&msg, sizeof(msg), sizeof(msg));
	if (err || msg.ret) {
		goto clean_up;
	}

	gr_ctx->global_ctx_buffer_mapped = true;
	return 0;

 clean_up:
	for (i = 0; i < NVGPU_GR_CTX_VA_COUNT; i++) {
		if (g_bfr_va[i]) {
			nvgpu_vm_free_va(ch_vm, g_bfr_va[i],
					 GMMU_PAGE_SIZE_KERNEL);
			g_bfr_va[i] = 0;
		}
	}
	return -ENOMEM;
}

/* load saved fresh copy of gloden image into channel gr_ctx */
int vgpu_gr_load_golden_ctx_image(struct gk20a *g, u64 virt_ctx)
{
	struct tegra_vgpu_cmd_msg msg;
	struct tegra_vgpu_ch_ctx_params *p = &msg.params.ch_ctx;
	int err;

	nvgpu_log_fn(g, " ");

	msg.cmd = TEGRA_VGPU_CMD_CHANNEL_LOAD_GR_GOLDEN_CTX;
	msg.handle = vgpu_get_handle(g);
	p->handle = virt_ctx;
	err = vgpu_comm_sendrecv(&msg, sizeof(msg), sizeof(msg));

	return (err || msg.ret) ? -1 : 0;
}
