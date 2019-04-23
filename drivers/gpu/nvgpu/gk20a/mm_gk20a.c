/*
 * Copyright (c) 2011-2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <trace/events/gk20a.h>

#include <nvgpu/mm.h>
#include <nvgpu/vm.h>
#include <nvgpu/vm_area.h>
#include <nvgpu/dma.h>
#include <nvgpu/kmem.h>
#include <nvgpu/timers.h>
#include <nvgpu/pramin.h>
#include <nvgpu/list.h>
#include <nvgpu/nvgpu_mem.h>
#include <nvgpu/allocator.h>
#include <nvgpu/semaphore.h>
#include <nvgpu/page_allocator.h>
#include <nvgpu/log.h>
#include <nvgpu/bug.h>
#include <nvgpu/log2.h>
#include <nvgpu/enabled.h>
#include <nvgpu/vidmem.h>
#include <nvgpu/sizes.h>
#include <nvgpu/io.h>
#include <nvgpu/utils.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/channel.h>
#include <nvgpu/pd_cache.h>
#include <nvgpu/fence.h>

#include "mm_gk20a.h"

#include <nvgpu/hw/gk20a/hw_pram_gk20a.h>

/*
 * GPU mapping life cycle
 * ======================
 *
 * Kernel mappings
 * ---------------
 *
 * Kernel mappings are created through vm.map(..., false):
 *
 *  - Mappings to the same allocations are reused and refcounted.
 *  - This path does not support deferred unmapping (i.e. kernel must wait for
 *    all hw operations on the buffer to complete before unmapping).
 *  - References to dmabuf are owned and managed by the (kernel) clients of
 *    the gk20a_vm layer.
 *
 *
 * User space mappings
 * -------------------
 *
 * User space mappings are created through as.map_buffer -> vm.map(..., true):
 *
 *  - Mappings to the same allocations are reused and refcounted.
 *  - This path supports deferred unmapping (i.e. we delay the actual unmapping
 *    until all hw operations have completed).
 *  - References to dmabuf are owned and managed by the vm_gk20a
 *    layer itself. vm.map acquires these refs, and sets
 *    mapped_buffer->own_mem_ref to record that we must release the refs when we
 *    actually unmap.
 *
 */

/* make sure gk20a_init_mm_support is called before */
int gk20a_init_mm_setup_hw(struct gk20a *g)
{
	struct mm_gk20a *mm = &g->mm;
	int err;

	nvgpu_log_fn(g, " ");

	if (g->ops.fb.set_mmu_page_size != NULL) {
		g->ops.fb.set_mmu_page_size(g);
	}

	if (g->ops.fb.set_use_full_comp_tag_line != NULL) {
		mm->use_full_comp_tag_line =
			g->ops.fb.set_use_full_comp_tag_line(g);
	}

	g->ops.fb.init_hw(g);

	if (g->ops.bus.bar1_bind != NULL) {
		g->ops.bus.bar1_bind(g, &mm->bar1.inst_block);
	}

	if (g->ops.bus.bar2_bind != NULL) {
		err = g->ops.bus.bar2_bind(g, &mm->bar2.inst_block);
		if (err != 0) {
			return err;
		}
	}

	if (g->ops.mm.cache.fb_flush(g) != 0 ||
	    g->ops.mm.cache.fb_flush(g) != 0) {
		return -EBUSY;
	}

	nvgpu_log_fn(g, "done");
	return 0;
}

void gk20a_init_inst_block(struct nvgpu_mem *inst_block, struct vm_gk20a *vm,
		u32 big_page_size)
{
	struct gk20a *g = gk20a_from_vm(vm);
	u64 pdb_addr = nvgpu_pd_gpu_addr(g, &vm->pdb);

	nvgpu_log_info(g, "inst block phys = 0x%llx, kv = 0x%p",
		nvgpu_inst_block_addr(g, inst_block), inst_block->cpu_va);

	g->ops.ramin.init_pdb(g, inst_block, pdb_addr, vm->pdb.mem);

	g->ops.ramin.set_adr_limit(g, inst_block, vm->va_limit - 1U);

	if ((big_page_size != 0U) && (g->ops.ramin.set_big_page_size != NULL)) {
		g->ops.ramin.set_big_page_size(g, inst_block, big_page_size);
	}
}

int gk20a_alloc_inst_block(struct gk20a *g, struct nvgpu_mem *inst_block)
{
	int err;

	nvgpu_log_fn(g, " ");

	err = nvgpu_dma_alloc(g, g->ops.ramin.alloc_size(), inst_block);
	if (err != 0) {
		nvgpu_err(g, "%s: memory allocation failed", __func__);
		return err;
	}

	nvgpu_log_fn(g, "done");
	return 0;
}

u32 gk20a_mm_get_iommu_bit(struct gk20a *g)
{
	return 34;
}

u64 gk20a_mm_bar1_map_userd(struct gk20a *g, struct nvgpu_mem *mem, u32 offset)
{
	struct fifo_gk20a *f = &g->fifo;
	u64 gpu_va = f->userd_gpu_va + offset;

	return nvgpu_gmmu_map_fixed(g->mm.bar1.vm, mem, gpu_va,
				    PAGE_SIZE, 0,
				    gk20a_mem_flag_none, false,
				    mem->aperture);
}
