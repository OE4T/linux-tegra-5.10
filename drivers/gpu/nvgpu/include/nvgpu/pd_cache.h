/*
 * Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_PD_CACHE_H
#define NVGPU_PD_CACHE_H

#include <nvgpu/types.h>

struct gk20a;
struct vm_gk20a;
struct nvgpu_mem;
struct gk20a_mmu_level;

/*
 * GMMU page directory. This is the kernel's tracking of a list of PDEs or PTEs
 * in the GMMU.
 */
struct nvgpu_gmmu_pd {
	/*
	 * DMA memory describing the PTEs or PDEs. @mem_offs describes the
	 * offset of the PDE table in @mem. @cached specifies if this PD is
	 * using pd_cache memory.
	 */
	struct nvgpu_mem	*mem;
	u32			 mem_offs;
	bool			 cached;
	u32			 pd_size; /* In bytes. */

	/*
	 * List of pointers to the next level of page tables. Does not
	 * need to be populated when this PD is pointing to PTEs.
	 */
	struct nvgpu_gmmu_pd	*entries;
	int			 num_entries;
};

int  nvgpu_pd_alloc(struct vm_gk20a *vm, struct nvgpu_gmmu_pd *pd, u32 bytes);
void nvgpu_pd_free(struct vm_gk20a *vm, struct nvgpu_gmmu_pd *pd);
int  nvgpu_pd_cache_init(struct gk20a *g);
void nvgpu_pd_cache_fini(struct gk20a *g);
u32  nvgpu_pd_offset_from_index(const struct gk20a_mmu_level *l, u32 pd_idx);
void nvgpu_pd_write(struct gk20a *g, struct nvgpu_gmmu_pd *pd,
		    size_t w, u32 data);
u64  nvgpu_pd_gpu_addr(struct gk20a *g, struct nvgpu_gmmu_pd *pd);

#endif
