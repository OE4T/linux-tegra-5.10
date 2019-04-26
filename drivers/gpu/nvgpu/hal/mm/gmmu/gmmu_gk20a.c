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
#include <nvgpu/gmmu.h>
#include <nvgpu/log2.h>

#include <nvgpu/hw/gk20a/hw_gmmu_gk20a.h>

#include "gmmu_gk20a.h"

/* for gk20a the "video memory" apertures here are misnomers. */
static inline u32 big_valid_pde0_bits(struct gk20a *g,
				      struct nvgpu_gmmu_pd *pd, u64 addr)
{
	u32 pde0_bits =
		nvgpu_aperture_mask(g, pd->mem,
				    gmmu_pde_aperture_big_sys_mem_ncoh_f(),
				    gmmu_pde_aperture_big_sys_mem_coh_f(),
				    gmmu_pde_aperture_big_video_memory_f()) |
		gmmu_pde_address_big_sys_f(
			   (u32)(addr >> gmmu_pde_address_shift_v()));

	return pde0_bits;
}

static inline u32 small_valid_pde1_bits(struct gk20a *g,
					struct nvgpu_gmmu_pd *pd, u64 addr)
{
	u32 pde1_bits =
		nvgpu_aperture_mask(g, pd->mem,
				    gmmu_pde_aperture_small_sys_mem_ncoh_f(),
				    gmmu_pde_aperture_small_sys_mem_coh_f(),
				    gmmu_pde_aperture_small_video_memory_f()) |
		gmmu_pde_vol_small_true_f() | /* tbd: why? */
		gmmu_pde_address_small_sys_f(
			   (u32)(addr >> gmmu_pde_address_shift_v()));

	return pde1_bits;
}

static void update_gmmu_pde_locked(struct vm_gk20a *vm,
				   const struct gk20a_mmu_level *l,
				   struct nvgpu_gmmu_pd *pd,
				   u32 pd_idx,
				   u64 virt_addr,
				   u64 phys_addr,
				   struct nvgpu_gmmu_attrs *attrs)
{
	struct gk20a *g = gk20a_from_vm(vm);
	bool small_valid, big_valid;
	u32 pd_offset = nvgpu_pd_offset_from_index(l, pd_idx);
	u32 pde_v[2] = {0, 0};

	small_valid = attrs->pgsz == GMMU_PAGE_SIZE_SMALL;
	big_valid   = attrs->pgsz == GMMU_PAGE_SIZE_BIG;

	pde_v[0] = gmmu_pde_size_full_f();
	pde_v[0] |= big_valid ?
		big_valid_pde0_bits(g, pd, phys_addr) :
		gmmu_pde_aperture_big_invalid_f();

	pde_v[1] |= (small_valid ? small_valid_pde1_bits(g, pd, phys_addr) :
		     (gmmu_pde_aperture_small_invalid_f() |
		      gmmu_pde_vol_small_false_f()))
		|
		(big_valid ? (gmmu_pde_vol_big_true_f()) :
		 gmmu_pde_vol_big_false_f());

	pte_dbg(g, attrs,
		"PDE: i=%-4u size=%-2u offs=%-4u pgsz: %c%c | "
		"GPU %#-12llx  phys %#-12llx "
		"[0x%08x, 0x%08x]",
		pd_idx, l->entry_size, pd_offset,
		small_valid ? 'S' : '-',
		big_valid ?   'B' : '-',
		virt_addr, phys_addr,
		pde_v[1], pde_v[0]);

	nvgpu_pd_write(g, &vm->pdb, (size_t)pd_offset + (size_t)0, pde_v[0]);
	nvgpu_pd_write(g, &vm->pdb, (size_t)pd_offset + (size_t)1, pde_v[1]);
}

static void __update_pte_sparse(u32 *pte_w)
{
	pte_w[0]  = gmmu_pte_valid_false_f();
	pte_w[1] |= gmmu_pte_vol_true_f();
}

static void __update_pte(struct vm_gk20a *vm,
			 u32 *pte_w,
			 u64 phys_addr,
			 struct nvgpu_gmmu_attrs *attrs)
{
	struct gk20a *g = gk20a_from_vm(vm);
	u32 page_size = vm->gmmu_page_sizes[attrs->pgsz];
	u32 pte_valid = attrs->valid ?
		gmmu_pte_valid_true_f() :
		gmmu_pte_valid_false_f();
	u32 phys_shifted = phys_addr >> gmmu_pte_address_shift_v();
	u32 addr = attrs->aperture == APERTURE_SYSMEM ?
		gmmu_pte_address_sys_f(phys_shifted) :
		gmmu_pte_address_vid_f(phys_shifted);
	int ctag_shift = 0;
	int shamt = ilog2(g->ops.fb.compression_page_size(g));
	if (shamt < 0) {
		nvgpu_err(g, "shift amount 'shamt' is negative");
	} else {
		ctag_shift = shamt;
	}

	pte_w[0] = pte_valid | addr;

	if (attrs->priv) {
		pte_w[0] |= gmmu_pte_privilege_true_f();
	}

	pte_w[1] = nvgpu_aperture_mask_raw(g, attrs->aperture,
					 gmmu_pte_aperture_sys_mem_ncoh_f(),
					 gmmu_pte_aperture_sys_mem_coh_f(),
					 gmmu_pte_aperture_video_memory_f()) |
		gmmu_pte_kind_f(attrs->kind_v) |
		gmmu_pte_comptagline_f((U32(attrs->ctag) >> U32(ctag_shift)));

	if ((attrs->ctag != 0ULL) &&
	     vm->mm->use_full_comp_tag_line &&
	    ((phys_addr & 0x10000ULL) != 0ULL)) {
		pte_w[1] |= gmmu_pte_comptagline_f(
			BIT32(gmmu_pte_comptagline_s() - 1U));
	}

	if (attrs->rw_flag == gk20a_mem_flag_read_only) {
		pte_w[0] |= gmmu_pte_read_only_true_f();
		pte_w[1] |= gmmu_pte_write_disable_true_f();
	} else if (attrs->rw_flag == gk20a_mem_flag_write_only) {
		pte_w[1] |= gmmu_pte_read_disable_true_f();
	}

	if (!attrs->cacheable) {
		pte_w[1] |= gmmu_pte_vol_true_f();
	}

	if (attrs->ctag != 0ULL) {
		attrs->ctag += page_size;
	}
}

static void update_gmmu_pte_locked(struct vm_gk20a *vm,
				   const struct gk20a_mmu_level *l,
				   struct nvgpu_gmmu_pd *pd,
				   u32 pd_idx,
				   u64 virt_addr,
				   u64 phys_addr,
				   struct nvgpu_gmmu_attrs *attrs)
{
	struct gk20a *g = gk20a_from_vm(vm);
	u32 page_size  = vm->gmmu_page_sizes[attrs->pgsz];
	u32 pd_offset = nvgpu_pd_offset_from_index(l, pd_idx);
	u32 pte_w[2] = {0, 0};
	int ctag_shift = 0;
	int shamt = ilog2(g->ops.fb.compression_page_size(g));
	if (shamt < 0) {
		nvgpu_err(g, "shift amount 'shamt' is negative");
	} else {
		ctag_shift = shamt;
	}

	if (phys_addr != 0ULL) {
		__update_pte(vm, pte_w, phys_addr, attrs);
	} else if (attrs->sparse) {
		__update_pte_sparse(pte_w);
	}

	pte_dbg(g, attrs,
		"PTE: i=%-4u size=%-2u offs=%-4u | "
		"GPU %#-12llx  phys %#-12llx "
		"pgsz: %3dkb perm=%-2s kind=%#02x APT=%-6s %c%c%c%c "
		"ctag=0x%08x "
		"[0x%08x, 0x%08x]",
		pd_idx, l->entry_size, pd_offset,
		virt_addr, phys_addr,
		page_size >> 10,
		nvgpu_gmmu_perm_str(attrs->rw_flag),
		attrs->kind_v,
		nvgpu_aperture_str(g, attrs->aperture),
		attrs->cacheable ? 'C' : '-',
		attrs->sparse    ? 'S' : '-',
		attrs->priv      ? 'P' : '-',
		attrs->valid     ? 'V' : '-',
		U32(attrs->ctag) >> U32(ctag_shift),
		pte_w[1], pte_w[0]);

	nvgpu_pd_write(g, pd, (size_t)pd_offset + (size_t)0, pte_w[0]);
	nvgpu_pd_write(g, pd, (size_t)pd_offset + (size_t)1, pte_w[1]);
}

u32 gk20a_get_pde_pgsz(struct gk20a *g, const struct gk20a_mmu_level *l,
				struct nvgpu_gmmu_pd *pd, u32 pd_idx)
{
	/*
	 * big and small page sizes are the same
	 */
	return GMMU_PAGE_SIZE_SMALL;
}

u32 gk20a_get_pte_pgsz(struct gk20a *g, const struct gk20a_mmu_level *l,
				struct nvgpu_gmmu_pd *pd, u32 pd_idx)
{
	/*
	 * return invalid
	 */
	return GMMU_NR_PAGE_SIZES;
}

const struct gk20a_mmu_level gk20a_mm_levels_64k[] = {
	{.hi_bit = {NV_GMMU_VA_RANGE-1, NV_GMMU_VA_RANGE-1},
	 .lo_bit = {26, 26},
	 .update_entry = update_gmmu_pde_locked,
	 .entry_size = 8,
	 .get_pgsz = gk20a_get_pde_pgsz},
	{.hi_bit = {25, 25},
	 .lo_bit = {12, 16},
	 .update_entry = update_gmmu_pte_locked,
	 .entry_size = 8,
	 .get_pgsz = gk20a_get_pte_pgsz},
	{.update_entry = NULL}
};

const struct gk20a_mmu_level gk20a_mm_levels_128k[] = {
	{.hi_bit = {NV_GMMU_VA_RANGE-1, NV_GMMU_VA_RANGE-1},
	 .lo_bit = {27, 27},
	 .update_entry = update_gmmu_pde_locked,
	 .entry_size = 8,
	 .get_pgsz = gk20a_get_pde_pgsz},
	{.hi_bit = {26, 26},
	 .lo_bit = {12, 17},
	 .update_entry = update_gmmu_pte_locked,
	 .entry_size = 8,
	 .get_pgsz = gk20a_get_pte_pgsz},
	{.update_entry = NULL}
};

const struct gk20a_mmu_level *gk20a_mm_get_mmu_levels(struct gk20a *g,
						      u32 big_page_size)
{
	return (big_page_size == SZ_64K) ?
		 gk20a_mm_levels_64k : gk20a_mm_levels_128k;
}

u32 gk20a_mm_get_iommu_bit(struct gk20a *g)
{
	return 34;
}
