/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <nvgpu/cbc.h>
#include <nvgpu/dma.h>
#include <nvgpu/nvgpu_mem.h>
#include <nvgpu/gk20a.h>

int nvgpu_cbc_alloc(struct gk20a *g, size_t compbit_backing_size,
			bool vidmem_alloc)
{
	struct nvgpu_cbc *cbc = g->cbc;

	if (nvgpu_mem_is_valid(&cbc->compbit_store.mem))
		return 0;

	if (vidmem_alloc) {
		/*
		 * Backing store MUST be physically contiguous and allocated in
		 * one chunk
		 * Vidmem allocation API does not support FORCE_CONTIGUOUS like
		 * flag to allocate contiguous memory
		 * But this allocation will happen in vidmem bootstrap allocator
		 * which always allocates contiguous memory
		 */
		return nvgpu_dma_alloc_vid(g,
					 compbit_backing_size,
					 &cbc->compbit_store.mem);
	} else {
		return nvgpu_dma_alloc_flags_sys(g,
					 NVGPU_DMA_PHYSICALLY_ADDRESSED,
					 compbit_backing_size,
					 &cbc->compbit_store.mem);
	}
}
