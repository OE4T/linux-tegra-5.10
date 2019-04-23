/*
 * GV11B MMU
 *
 * Copyright (c) 2016-2019, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/dma.h>
#include <nvgpu/log.h>
#include <nvgpu/mm.h>
#include <nvgpu/enabled.h>
#include <nvgpu/gk20a.h>

#include "gk20a/mm_gk20a.h"

#include "gp10b/mm_gp10b.h"

#include "mm_gv11b.h"

#include <nvgpu/hw/gv11b/hw_gmmu_gv11b.h>

bool gv11b_mm_is_bar1_supported(struct gk20a *g)
{
	return false;
}

void gv11b_init_inst_block(struct nvgpu_mem *inst_block,
		struct vm_gk20a *vm, u32 big_page_size)
{
	struct gk20a *g = gk20a_from_vm(vm);
	u64 pdb_addr = nvgpu_pd_gpu_addr(g, &vm->pdb);

	nvgpu_log_info(g, "inst block phys = 0x%llx, kv = 0x%p",
		nvgpu_inst_block_addr(g, inst_block), inst_block->cpu_va);

	g->ops.ramin.init_pdb(g, inst_block, pdb_addr, vm->pdb.mem);

	if ((big_page_size != 0U) && (g->ops.ramin.set_big_page_size != NULL)) {
		g->ops.ramin.set_big_page_size(g, inst_block, big_page_size);
	}

	if (g->ops.ramin.init_subctx_pdb != NULL) {
		g->ops.ramin.init_subctx_pdb(g, inst_block, vm->pdb.mem, false);
	}
}

void gv11b_mm_mmu_fault_disable_hw(struct gk20a *g)
{
	nvgpu_mutex_acquire(&g->mm.hub_isr_mutex);

	if ((g->ops.fb.is_fault_buf_enabled(g,
			NVGPU_MMU_FAULT_NONREPLAY_REG_INDX))) {
		g->ops.fb.fault_buf_set_state_hw(g,
				NVGPU_MMU_FAULT_NONREPLAY_REG_INDX,
				NVGPU_MMU_FAULT_BUF_DISABLED);
	}

	if ((g->ops.fb.is_fault_buf_enabled(g,
			NVGPU_MMU_FAULT_REPLAY_REG_INDX))) {
		g->ops.fb.fault_buf_set_state_hw(g,
				NVGPU_MMU_FAULT_REPLAY_REG_INDX,
				NVGPU_MMU_FAULT_BUF_DISABLED);
	}

	nvgpu_mutex_release(&g->mm.hub_isr_mutex);
}

void gv11b_mm_fault_info_mem_destroy(struct gk20a *g)
{
	struct vm_gk20a *vm = g->mm.bar2.vm;

	nvgpu_log_fn(g, " ");

	nvgpu_mutex_acquire(&g->mm.hub_isr_mutex);

	if (nvgpu_mem_is_valid(
		    &g->mm.hw_fault_buf[NVGPU_MMU_FAULT_NONREPLAY_INDX])) {
		nvgpu_dma_unmap_free(vm,
			 &g->mm.hw_fault_buf[NVGPU_MMU_FAULT_NONREPLAY_INDX]);
	}
	if (nvgpu_mem_is_valid(
			&g->mm.hw_fault_buf[NVGPU_MMU_FAULT_REPLAY_INDX])) {
		nvgpu_dma_unmap_free(vm,
			 &g->mm.hw_fault_buf[NVGPU_MMU_FAULT_REPLAY_INDX]);
	}

	nvgpu_mutex_release(&g->mm.hub_isr_mutex);
	nvgpu_mutex_destroy(&g->mm.hub_isr_mutex);
}

static int gv11b_mm_mmu_fault_info_buf_init(struct gk20a *g)
{
	return 0;
}

static void gv11b_mm_mmu_hw_fault_buf_init(struct gk20a *g)
{
	struct vm_gk20a *vm = g->mm.bar2.vm;
	int err = 0;
	size_t fb_size;

	/* Max entries take care of 1 entry used for full detection */
	fb_size = ((size_t)g->ops.channel.count(g) + (size_t)1) *
				 (size_t)gmmu_fault_buf_size_v();

	if (!nvgpu_mem_is_valid(
		&g->mm.hw_fault_buf[NVGPU_MMU_FAULT_NONREPLAY_INDX])) {

		err = nvgpu_dma_alloc_map_sys(vm, fb_size,
			&g->mm.hw_fault_buf[NVGPU_MMU_FAULT_NONREPLAY_INDX]);
		if (err != 0) {
			nvgpu_err(g,
			"Error in hw mmu fault buf [0] alloc in bar2 vm ");
			/* Fault will be snapped in pri reg but not in buffer */
			return;
		}
	}

	if (!nvgpu_mem_is_valid(
		&g->mm.hw_fault_buf[NVGPU_MMU_FAULT_REPLAY_INDX])) {
		err = nvgpu_dma_alloc_map_sys(vm, fb_size,
				&g->mm.hw_fault_buf[NVGPU_MMU_FAULT_REPLAY_INDX]);
		if (err != 0) {
			nvgpu_err(g,
			"Error in hw mmu fault buf [1] alloc in bar2 vm ");
			/* Fault will be snapped in pri reg but not in buffer */
			return;
		}
	}
}

void gv11b_mm_mmu_fault_setup_hw(struct gk20a *g)
{
	if (nvgpu_mem_is_valid(
			&g->mm.hw_fault_buf[NVGPU_MMU_FAULT_NONREPLAY_INDX])) {
		g->ops.fb.fault_buf_configure_hw(g,
				NVGPU_MMU_FAULT_NONREPLAY_REG_INDX);
	}
	if (nvgpu_mem_is_valid(
			&g->mm.hw_fault_buf[NVGPU_MMU_FAULT_REPLAY_INDX])) {
		g->ops.fb.fault_buf_configure_hw(g,
				NVGPU_MMU_FAULT_REPLAY_REG_INDX);
	}
}

int gv11b_mm_mmu_fault_setup_sw(struct gk20a *g)
{
	int err = 0;

	nvgpu_log_fn(g, " ");

	err = nvgpu_mutex_init(&g->mm.hub_isr_mutex);
	if (err != 0) {
		nvgpu_err(g, "Error in hub_isr_mutex initialization");
		return err;
	}

	err = gv11b_mm_mmu_fault_info_buf_init(g);

	if (err == 0) {
		gv11b_mm_mmu_hw_fault_buf_init(g);
	}

	return err;
}
