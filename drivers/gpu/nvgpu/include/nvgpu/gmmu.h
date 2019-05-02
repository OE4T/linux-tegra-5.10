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

#ifndef NVGPU_GMMU_H
#define NVGPU_GMMU_H

#include <nvgpu/types.h>
#include <nvgpu/nvgpu_mem.h>
#include <nvgpu/list.h>
#include <nvgpu/rbtree.h>
#include <nvgpu/lock.h>

/*
 * This is the GMMU API visible to blocks outside of the GMMU. Basically this
 * API supports all the different types of mappings that might be done in the
 * GMMU.
 */

struct vm_gk20a;
struct nvgpu_mem;
struct nvgpu_gmmu_pd;
struct vm_gk20a_mapping_batch;

#define GMMU_PAGE_SIZE_SMALL	0U
#define GMMU_PAGE_SIZE_BIG	1U
#define GMMU_PAGE_SIZE_KERNEL	2U
#define GMMU_NR_PAGE_SIZES	3U

enum gk20a_mem_rw_flag {
	gk20a_mem_flag_none = 0,	/* RW */
	gk20a_mem_flag_read_only = 1,	/* RO */
	gk20a_mem_flag_write_only = 2,	/* WO */
};

/*
 * Reduce the number of arguments getting passed through the various levels of
 * GMMU mapping functions.
 *
 * The following fields are set statically and do not change throughout the
 * mapping call:
 *
 *   pgsz:            Index into the page size table.
 *   kind_v:          Kind attributes for mapping.
 *   cacheable:       Cacheability of the mapping.
 *   rw_flag:         Flag from enum gk20a_mem_rw_flag
 *   sparse:          True if the mapping should be sparse.
 *   priv:            Privilidged mapping.
 *   valid:           True if the PTE should be marked valid.
 *   aperture:        VIDMEM or SYSMEM.
 *   debug:           When set print debugging info.
 *   l3_alloc:        True if l3_alloc flag is valid.
 *   platform_atomic: True if platform_atomic flag is valid.
 *
 *   These fields are dynamically updated as necessary during the map:
 *
 *   ctag:        Comptag line in the comptag cache;
 *                updated every time we write a PTE.
 */
struct nvgpu_gmmu_attrs {
	u32			 pgsz;
	u32			 kind_v;
	u64			 ctag;
	bool			 cacheable;
	enum gk20a_mem_rw_flag	 rw_flag;
	bool			 sparse;
	bool			 priv;
	bool			 valid;
	enum nvgpu_aperture	 aperture;
	bool			 debug;
	bool			 l3_alloc;
	bool			 platform_atomic;
};

struct gk20a_mmu_level {
	u32 hi_bit[2];
	u32 lo_bit[2];

	/*
	 * Build map from virt_addr -> phys_addr.
	 */
	void (*update_entry)(struct vm_gk20a *vm,
			     const struct gk20a_mmu_level *l,
			     struct nvgpu_gmmu_pd *pd,
			     u32 pd_idx,
			     u64 phys_addr,
			     u64 virt_addr,
			     struct nvgpu_gmmu_attrs *attrs);
	u32 entry_size;
	/*
	 * Get pde page size
	 */
	u32 (*get_pgsz)(struct gk20a *g, const struct gk20a_mmu_level *l,
				struct nvgpu_gmmu_pd *pd, u32 pd_idx);
};

static inline const char *nvgpu_gmmu_perm_str(enum gk20a_mem_rw_flag p)
{
	const char *str;

	switch (p) {
	case gk20a_mem_flag_none:
		str = "RW";
		break;
	case gk20a_mem_flag_write_only:
		str = "WO";
		break;
	case gk20a_mem_flag_read_only:
		str = "RO";
		break;
	default:
		str = "??";
		break;
	}
	return str;
}

int nvgpu_gmmu_init_page_table(struct vm_gk20a *vm);

/**
 * nvgpu_gmmu_map - Map memory into the GMMU.
 *
 * Kernel space.
 */
u64 nvgpu_gmmu_map(struct vm_gk20a *vm,
		   struct nvgpu_mem *mem,
		   u64 size,
		   u32 flags,
		   enum gk20a_mem_rw_flag rw_flag,
		   bool priv,
		   enum nvgpu_aperture aperture);

/**
 * nvgpu_gmmu_map_fixed - Map memory into the GMMU.
 *
 * Kernel space.
 */
u64 nvgpu_gmmu_map_fixed(struct vm_gk20a *vm,
			 struct nvgpu_mem *mem,
			 u64 addr,
			 u64 size,
			 u32 flags,
			 enum gk20a_mem_rw_flag rw_flag,
			 bool priv,
			 enum nvgpu_aperture aperture);

/**
 * nvgpu_gmmu_unmap - Unmap a buffer.
 *
 * Kernel space.
 */
void nvgpu_gmmu_unmap(struct vm_gk20a *vm,
		      struct nvgpu_mem *mem,
		      u64 gpu_va);

/**
 * nvgpu_pte_words - Compute number of words in a PTE.
 *
 * @g  - The GPU.
 *
 * This computes and returns the size of a PTE for the passed chip.
 */
u32 nvgpu_pte_words(struct gk20a *g);

/**
 * nvgpu_get_pte - Get the contents of a PTE by virtual address
 *
 * @g     - The GPU.
 * @vm    - VM to look in.
 * @vaddr - GPU virtual address.
 * @pte   - [out] Set to the contents of the PTE.
 *
 * Find a PTE in the passed VM based on the passed GPU virtual address. This
 * will @pte with a copy of the contents of the PTE. @pte must be an array of
 * u32s large enough to contain the PTE. This can be computed using
 * nvgpu_pte_words().
 *
 * If you wish to write to this PTE then you may modify @pte and then use the
 * nvgpu_set_pte().
 *
 * This function returns 0 if the PTE is found and -EINVAL otherwise.
 */
int nvgpu_get_pte(struct gk20a *g, struct vm_gk20a *vm, u64 vaddr, u32 *pte);

/**
 * nvgpu_set_pte - Set a PTE based on virtual address
 *
 * @g     - The GPU.
 * @vm    - VM to look in.
 * @vaddr - GPU virtual address.
 * @pte   - The contents of the PTE to write.
 *
 * Find a PTE and overwrite the contents of that PTE with the passed in data
 * located in @pte. If the PTE does not exist then no writing will happen. That
 * is this function will not fill out the page tables for you. The expectation
 * is that the passed @vaddr has already been mapped and this is just modifying
 * the mapping (for instance changing invalid to valid).
 *
 * @pte must contain at least the required words for the PTE. See
 * nvgpu_pte_words().
 *
 * This function returns 0 on success and -EINVAL otherwise.
 */
int nvgpu_set_pte(struct gk20a *g, struct vm_gk20a *vm, u64 vaddr, u32 *pte);

/*
 * Native GPU "HAL" functions.
 */
u64 nvgpu_gmmu_map_locked(struct vm_gk20a *vm,
			  u64 vaddr,
			  struct nvgpu_sgt *sgt,
			  u64 buffer_offset,
			  u64 size,
			  u32 pgsz_idx,
			  u8 kind_v,
			  u32 ctag_offset,
			  u32 flags,
			  enum gk20a_mem_rw_flag rw_flag,
			  bool clear_ctags,
			  bool sparse,
			  bool priv,
			  struct vm_gk20a_mapping_batch *batch,
			  enum nvgpu_aperture aperture);
void nvgpu_gmmu_unmap_locked(struct vm_gk20a *vm,
			     u64 vaddr,
			     u64 size,
			     u32 pgsz_idx,
			     bool va_allocated,
			     enum gk20a_mem_rw_flag rw_flag,
			     bool sparse,
			     struct vm_gk20a_mapping_batch *batch);

/*
 * Internal debugging routines. Probably not something you want to use.
 */
#define pte_dbg(g, attrs, fmt, args...)					\
	do {								\
		if (((attrs) != NULL) && ((attrs)->debug)) {		\
			nvgpu_info(g, fmt, ##args);			\
		} else {						\
			nvgpu_log(g, gpu_dbg_pte, fmt, ##args);		\
		}							\
	} while (false)

#endif /* NVGPU_GMMU_H */
