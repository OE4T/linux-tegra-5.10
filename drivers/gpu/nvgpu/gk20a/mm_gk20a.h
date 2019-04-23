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

#ifndef MM_GK20A_H
#define MM_GK20A_H

#include <nvgpu/nvgpu_mem.h>
#include <nvgpu/allocator.h>
#include <nvgpu/vm.h>
#include <nvgpu/list.h>
#include <nvgpu/rbtree.h>
#include <nvgpu/kref.h>

struct compbit_store_desc {
	struct nvgpu_mem mem;

	/* The value that is written to the hardware. This depends on
	 * on the number of ltcs and is not an address. */
	u64 base_hw;
};

struct gk20a_buffer_state {
	struct nvgpu_list_node list;

	/* The valid compbits and the fence must be changed atomically. */
	struct nvgpu_mutex lock;

	/* Offset of the surface within the dma-buf whose state is
	 * described by this struct (one dma-buf can contain multiple
	 * surfaces with different states). */
	size_t offset;

	/* A bitmask of valid sets of compbits (0 = uncompressed). */
	u32 valid_compbits;

	/* The ZBC color used on this buffer. */
	u32 zbc_color;

	/* This struct reflects the state of the buffer when this
	 * fence signals. */
	struct nvgpu_fence_type *fence;
};

static inline struct gk20a_buffer_state *
gk20a_buffer_state_from_list(struct nvgpu_list_node *node)
{
	return (struct gk20a_buffer_state *)
		((uintptr_t)node - offsetof(struct gk20a_buffer_state, list));
};

struct gk20a;
struct channel_gk20a;

#define dev_from_vm(vm) dev_from_gk20a(vm->mm->g)

void gk20a_mm_ltc_isr(struct gk20a *g);

bool gk20a_mm_mmu_debug_mode_enabled(struct gk20a *g);

int gk20a_alloc_inst_block(struct gk20a *g, struct nvgpu_mem *inst_block);
void gk20a_init_inst_block(struct nvgpu_mem *inst_block, struct vm_gk20a *vm,
		u32 big_page_size);
int gk20a_init_mm_setup_hw(struct gk20a *g);

/* vm-as interface */
struct nvgpu_as_alloc_space_args;
struct nvgpu_as_free_space_args;
int gk20a_vm_release_share(struct gk20a_as_share *as_share);

void pde_range_from_vaddr_range(struct vm_gk20a *vm,
					      u64 addr_lo, u64 addr_hi,
					      u32 *pde_lo, u32 *pde_hi);
u32 gk20a_mm_get_iommu_bit(struct gk20a *g);
u64 gk20a_mm_bar1_map_userd(struct gk20a *g, struct nvgpu_mem *mem, u32 offset);
#endif /* MM_GK20A_H */
