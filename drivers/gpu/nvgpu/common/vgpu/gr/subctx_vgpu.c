/*
 * Copyright (c) 2017-2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/vgpu/vgpu.h>
#include <nvgpu/vgpu/tegra_vgpu.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/gr/subctx.h>

#include "common/gr/subctx_priv.h"

#include "subctx_vgpu.h"
#include "common/vgpu/ivc/comm_vgpu.h"

int vgpu_alloc_subctx_header(struct gk20a *g,
			struct nvgpu_gr_subctx **gr_subctx,
			struct vm_gk20a *vm, u64 virt_ctx)
{
	struct nvgpu_gr_subctx *subctx;
	struct nvgpu_mem *ctxheader;
	struct tegra_vgpu_cmd_msg msg = {};
	struct tegra_vgpu_alloc_ctx_header_params *p =
				&msg.params.alloc_ctx_header;
	int err;

	subctx = nvgpu_kzalloc(g, sizeof(*subctx));
	if (subctx == NULL) {
		return -ENOMEM;
	}

	ctxheader = &subctx->ctx_header;

	msg.cmd = TEGRA_VGPU_CMD_ALLOC_CTX_HEADER;
	msg.handle = vgpu_get_handle(g);
	p->ch_handle = virt_ctx;
	p->ctx_header_va = nvgpu_vm_alloc_va(vm,
			g->ops.gr.ctxsw_prog.hw_get_fecs_header_size(),
			GMMU_PAGE_SIZE_KERNEL);
	if (p->ctx_header_va == 0U) {
		nvgpu_err(g, "alloc va failed for ctx_header");
		err = -ENOMEM;
		goto fail;
	}
	err = vgpu_comm_sendrecv(&msg, sizeof(msg), sizeof(msg));
	err = err ? err : msg.ret;
	if (unlikely(err != 0)) {
		nvgpu_err(g, "alloc ctx_header failed err %d", err);
		nvgpu_vm_free_va(vm, p->ctx_header_va,
			GMMU_PAGE_SIZE_KERNEL);
		goto fail;
	}
	ctxheader->gpu_va = p->ctx_header_va;

	*gr_subctx = subctx;
	return 0;

fail:
	nvgpu_kfree(g, subctx);
	return err;
}

void vgpu_free_subctx_header(struct gk20a *g, struct nvgpu_gr_subctx *subctx,
			struct vm_gk20a *vm, u64 virt_ctx)
{
	struct nvgpu_mem *ctxheader;
	struct tegra_vgpu_cmd_msg msg = {};
	struct tegra_vgpu_free_ctx_header_params *p =
				&msg.params.free_ctx_header;
	int err;

	if (subctx != NULL) {
		ctxheader = &subctx->ctx_header;

		msg.cmd = TEGRA_VGPU_CMD_FREE_CTX_HEADER;
		msg.handle = vgpu_get_handle(g);
		p->ch_handle = virt_ctx;
		err = vgpu_comm_sendrecv(&msg, sizeof(msg), sizeof(msg));
		err = err ? err : msg.ret;
		if (unlikely(err != 0)) {
			nvgpu_err(g, "free ctx_header failed err %d", err);
		}
		nvgpu_vm_free_va(vm, ctxheader->gpu_va,
				 GMMU_PAGE_SIZE_KERNEL);
		ctxheader->gpu_va = 0;
		nvgpu_kfree(g, subctx);
	}
}

void vgpu_gr_setup_free_subctx(struct nvgpu_channel *c)
{
	vgpu_free_subctx_header(c->g, c->subctx, c->vm, c->virt_ctx);
}
