/*
 * Copyright (c) 2017-2021, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/dma.h>
#include <nvgpu/vgpu/vgpu.h>
#include <nvgpu/nvhost.h>
#include <nvgpu/vgpu/tegra_vgpu.h>
#include <nvgpu/channel.h>
#include <nvgpu/channel_sync_syncpt.h>

#include "syncpt_cmdbuf_gv11b_vgpu.h"
#include "common/vgpu/ivc/comm_vgpu.h"

#ifdef CONFIG_TEGRA_GK20A_NVHOST

static int set_syncpt_ro_map_gpu_va_locked(struct vm_gk20a *vm)
{
	int err;
	struct gk20a *g = gk20a_from_vm(vm);
	struct tegra_vgpu_cmd_msg msg = {};
	struct tegra_vgpu_map_syncpt_params *p = &msg.params.map_syncpt;

	if (vm->syncpt_ro_map_gpu_va) {
		return 0;
	}

	vm->syncpt_ro_map_gpu_va = nvgpu_vm_alloc_va(vm,
			g->syncpt_unit_size,
			GMMU_PAGE_SIZE_KERNEL);
	if (!vm->syncpt_ro_map_gpu_va) {
		nvgpu_err(g, "allocating read-only va space failed");
		return -ENOMEM;
	}

	msg.cmd = TEGRA_VGPU_CMD_MAP_SYNCPT;
	msg.handle = vgpu_get_handle(g);
	p->as_handle = vm->handle;
	p->gpu_va = vm->syncpt_ro_map_gpu_va;
	p->len = g->syncpt_unit_size;
	p->offset = 0;
	p->prot = TEGRA_VGPU_MAP_PROT_READ_ONLY;
	err = vgpu_comm_sendrecv(&msg, sizeof(msg), sizeof(msg));
	err = err ? err : msg.ret;
	if (err) {
		nvgpu_err(g,
			"mapping read-only va space failed err %d",
			err);
		nvgpu_vm_free_va(vm, vm->syncpt_ro_map_gpu_va,
				 GMMU_PAGE_SIZE_KERNEL);
		vm->syncpt_ro_map_gpu_va = 0;
		return err;
	}

	return 0;
}

int vgpu_gv11b_syncpt_alloc_buf(struct nvgpu_channel *c,
				u32 syncpt_id, struct nvgpu_mem *syncpt_buf)
{
	int err;
	struct gk20a *g = c->g;
	struct tegra_vgpu_cmd_msg msg = {};
	struct tegra_vgpu_map_syncpt_params *p = &msg.params.map_syncpt;

	/*
	 * Add ro map for complete sync point shim range in vm.
	 * All channels sharing same vm will share same ro mapping.
	 * Create rw map for current channel sync point.
	 */
	nvgpu_mutex_acquire(&c->vm->syncpt_ro_map_lock);
	err = set_syncpt_ro_map_gpu_va_locked(c->vm);
	nvgpu_mutex_release(&c->vm->syncpt_ro_map_lock);
	if (err) {
		return err;
	}

	syncpt_buf->gpu_va = nvgpu_vm_alloc_va(c->vm, g->syncpt_size,
			GMMU_PAGE_SIZE_KERNEL);
	if (!syncpt_buf->gpu_va) {
		nvgpu_err(g, "allocating syncpt va space failed");
		return -ENOMEM;
	}

	msg.cmd = TEGRA_VGPU_CMD_MAP_SYNCPT;
	msg.handle = vgpu_get_handle(g);
	p->as_handle = c->vm->handle;
	p->gpu_va = syncpt_buf->gpu_va;
	p->len = g->syncpt_size;
	p->offset =
		nvgpu_nvhost_syncpt_unit_interface_get_byte_offset(g,
			syncpt_id);
	p->prot = TEGRA_VGPU_MAP_PROT_NONE;
	err = vgpu_comm_sendrecv(&msg, sizeof(msg), sizeof(msg));
	err = err ? err : msg.ret;
	if (err) {
		nvgpu_err(g, "mapping syncpt va space failed err %d", err);
		nvgpu_vm_free_va(c->vm, syncpt_buf->gpu_va,
				 GMMU_PAGE_SIZE_KERNEL);
		return err;
	}

	return 0;
}

void vgpu_gv11b_syncpt_free_buf(struct nvgpu_channel *c,
					struct nvgpu_mem *syncpt_buf)
{
	nvgpu_gmmu_unmap(c->vm, syncpt_buf);
	nvgpu_vm_free_va(c->vm, syncpt_buf->gpu_va, GMMU_PAGE_SIZE_KERNEL);
	nvgpu_dma_free(c->g, syncpt_buf);
}

int vgpu_gv11b_syncpt_get_sync_ro_map(struct vm_gk20a *vm,
	u64 *base_gpuva, u32 *sync_size, u32 *num_syncpoints)
{
	struct gk20a *g = gk20a_from_vm(vm);
	int err;
	size_t tmp;

	nvgpu_mutex_acquire(&vm->syncpt_ro_map_lock);
	err = set_syncpt_ro_map_gpu_va_locked(vm);
	nvgpu_mutex_release(&vm->syncpt_ro_map_lock);
	if (err) {
		return err;
	}

	*base_gpuva = vm->syncpt_ro_map_gpu_va;
	*sync_size = g->syncpt_size;

	tmp = g->syncpt_size ? (g->syncpt_unit_size / g->syncpt_size) : 0U;
	*num_syncpoints = (tmp <= U32_MAX) ? tmp : U32_MAX;

	return 0;
}
#endif /* CONFIG_TEGRA_GK20A_NVHOST */
