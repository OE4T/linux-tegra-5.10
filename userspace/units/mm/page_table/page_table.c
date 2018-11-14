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

#include <unit/io.h>
#include <unit/unit.h>

#include <nvgpu/gk20a.h>
#include <nvgpu/types.h>
#include <nvgpu/sizes.h>
#include <nvgpu/gmmu.h>
#include <nvgpu/mm.h>
#include <nvgpu/vm.h>
#include <os/posix/os_posix.h>

#include <gk20a/mm_gk20a.h>
#include <gm20b/mm_gm20b.h>
#include <gp10b/mm_gp10b.h>
#include <gv11b/mm_gv11b.h>
#include <common/fb/fb_gp10b.h>
#include <common/fb/fb_gm20b.h>
#include <nvgpu/hw/gv11b/hw_gmmu_gv11b.h>

#define TEST_PA_ADDRESS 0xEFAD80000000
#define TEST_COMP_TAG 0xEF
#define TEST_INVALID_ADDRESS 0xAAC0000000

/* Size of the buffer to map. It must be a multiple of 4KB */
#define TEST_SIZE (1 * SZ_1M)
#define TEST_SIZE_64KB_PAGES 16

struct test_parameters {
	enum nvgpu_aperture aperture;
	bool is_iommuable;
	enum gk20a_mem_rw_flag rw_flag;
	u32 flags;
	bool priv;
	u32 page_size;
	u32 offset_pages;
	bool sparse;
	u32 ctag_offset;
	/* Below are flags for special cases, default to disabled */
	bool special_2_sgl;
	bool special_null_phys;
};

static struct test_parameters test_iommu_sysmem = {
	.aperture = APERTURE_SYSMEM,
	.is_iommuable = true,
	.rw_flag = gk20a_mem_flag_none,
	.flags = NVGPU_VM_MAP_CACHEABLE,
	.priv = true,
};

static struct test_parameters test_iommu_sysmem_ro = {
	.aperture = APERTURE_SYSMEM,
	.is_iommuable = true,
	.rw_flag = gk20a_mem_flag_read_only,
	.flags = NVGPU_VM_MAP_CACHEABLE,
	.priv = true,
};

static struct test_parameters test_iommu_sysmem_coh = {
	.aperture = APERTURE_SYSMEM,
	.is_iommuable = true,
	.rw_flag = gk20a_mem_flag_none,
	.flags = NVGPU_VM_MAP_CACHEABLE | NVGPU_VM_MAP_IO_COHERENT,
	.priv = false,
};

static struct test_parameters test_no_iommu_sysmem = {
	.aperture = APERTURE_SYSMEM,
	.is_iommuable = false,
	.rw_flag = gk20a_mem_flag_none,
	.flags = NVGPU_VM_MAP_CACHEABLE,
	.priv = true,
};

static struct test_parameters test_iommu_sysmem_adv = {
	.aperture = APERTURE_SYSMEM,
	.is_iommuable = true,
	.rw_flag = gk20a_mem_flag_none,
	.flags = NVGPU_VM_MAP_CACHEABLE,
	.priv = true,
	.page_size = GMMU_PAGE_SIZE_KERNEL,
	.offset_pages = 0,
	.sparse = false,
};

static struct test_parameters test_iommu_sysmem_adv_ctag = {
	.aperture = APERTURE_SYSMEM,
	.is_iommuable = true,
	.rw_flag = gk20a_mem_flag_none,
	.flags = NVGPU_VM_MAP_CACHEABLE,
	.priv = true,
	.page_size = GMMU_PAGE_SIZE_KERNEL,
	.offset_pages = 10,
	.sparse = false,
	.ctag_offset = TEST_COMP_TAG,
};

static struct test_parameters test_iommu_sysmem_adv_big = {
	.aperture = APERTURE_SYSMEM,
	.is_iommuable = true,
	.rw_flag = gk20a_mem_flag_none,
	.flags = NVGPU_VM_MAP_CACHEABLE,
	.priv = true,
	.page_size = GMMU_PAGE_SIZE_BIG,
	.offset_pages = 0,
	.sparse = false,
};

static struct test_parameters test_iommu_sysmem_adv_big_offset = {
	.aperture = APERTURE_SYSMEM,
	.is_iommuable = true,
	.rw_flag = gk20a_mem_flag_none,
	.flags = NVGPU_VM_MAP_CACHEABLE,
	.priv = true,
	.page_size = GMMU_PAGE_SIZE_BIG,
	.offset_pages = 10,
	.sparse = false,
};

static struct test_parameters test_no_iommu_sysmem_adv_big_offset_large = {
	.aperture = APERTURE_SYSMEM,
	.is_iommuable = false,
	.rw_flag = gk20a_mem_flag_none,
	.flags = NVGPU_VM_MAP_CACHEABLE,
	.priv = true,
	.page_size = GMMU_PAGE_SIZE_BIG,
	.offset_pages = TEST_SIZE_64KB_PAGES+1,
	.sparse = false,
};

static struct test_parameters test_iommu_sysmem_adv_small_sparse = {
	.aperture = APERTURE_SYSMEM,
	.is_iommuable = true,
	.rw_flag = gk20a_mem_flag_none,
	.flags = NVGPU_VM_MAP_CACHEABLE,
	.priv = true,
	.page_size = GMMU_PAGE_SIZE_SMALL,
	.offset_pages = 0,
	.sparse = true,
	.special_null_phys = true,
};

static struct test_parameters test_no_iommu_vidmem = {
	.aperture = APERTURE_VIDMEM,
	.is_iommuable = false,
	.rw_flag = gk20a_mem_flag_none,
	.flags = NVGPU_VM_MAP_CACHEABLE,
	.priv = false,
};

static struct test_parameters test_no_iommu_sysmem_noncacheable = {
	.aperture = APERTURE_SYSMEM,
	.is_iommuable = false,
	.rw_flag = gk20a_mem_flag_none,
	.flags = 0,
	.priv = false,
};

static struct test_parameters test_no_iommu_unmapped = {
	.aperture = APERTURE_SYSMEM,
	.is_iommuable = false,
	.rw_flag = gk20a_mem_flag_none,
	.flags = NVGPU_VM_MAP_UNMAPPED_PTE,
	.priv = false,
};

static void init_platform(struct unit_module *m, struct gk20a *g, bool is_iGPU)
{
	if (is_iGPU) {
		__nvgpu_set_enabled(g, NVGPU_MM_UNIFIED_MEMORY, true);
	} else {
		__nvgpu_set_enabled(g, NVGPU_MM_UNIFIED_MEMORY, false);
	}
}

/*
 * Init the minimum set of HALs to run GMMU tests, then call the init_mm
 * base function.
 */
static int init_mm(struct unit_module *m, struct gk20a *g)
{
	u64 low_hole, aperture_size;
	struct nvgpu_os_posix *p = nvgpu_os_posix_from_gk20a(g);
	struct mm_gk20a *mm = &g->mm;

	p->mm_is_iommuable = true;

	g->ops.mm.get_default_big_page_size =
		gp10b_mm_get_default_big_page_size;
	g->ops.mm.get_mmu_levels = gp10b_mm_get_mmu_levels;
	g->ops.mm.alloc_inst_block = gk20a_alloc_inst_block;
	g->ops.mm.init_inst_block = gv11b_init_inst_block;
	g->ops.mm.init_pdb = gp10b_mm_init_pdb;
	g->ops.mm.gmmu_map = gk20a_locked_gmmu_map;
	g->ops.mm.gmmu_unmap = gk20a_locked_gmmu_unmap;
	g->ops.mm.gpu_phys_addr = gv11b_gpu_phys_addr;
	g->ops.mm.is_bar1_supported = gv11b_mm_is_bar1_supported;
	g->ops.fb.compression_page_size = gp10b_fb_compression_page_size;
	g->ops.fb.tlb_invalidate = gm20b_fb_tlb_invalidate;

	if (g->ops.mm.is_bar1_supported(g)) {
		unit_return_fail(m, "BAR1 is not supported on Volta+\n");
	}

	/*
	 * Initialize one VM space for system memory to be used throughout this
	 * unit module.
	 * Values below are similar to those used in nvgpu_init_system_vm()
	 */
	low_hole = SZ_4K * 16UL;
	aperture_size = GK20A_PMU_VA_SIZE;
	mm->pmu.aperture_size = GK20A_PMU_VA_SIZE;

	mm->pmu.vm = nvgpu_vm_init(g, g->ops.mm.get_default_big_page_size(),
				   low_hole,
				   aperture_size - low_hole,
				   aperture_size,
				   true,
				   false,
				   "system");
	if (mm->pmu.vm == NULL) {
		unit_return_fail(m, "nvgpu_vm_init failed\n");
	}

	return UNIT_SUCCESS;
}

/*
 * Test: test_nvgpu_gmmu_init
 * This test must be run once and be the first oneas it initializes the MM
 * subsystem.
 */
static int test_nvgpu_gmmu_init(struct unit_module *m,
					struct gk20a *g, void *args)
{
	u64 debug_level = (u64) args;

	g->log_mask = 0;
	if (debug_level >= 1) {
		g->log_mask = gpu_dbg_map;
	}
	if (debug_level >= 2) {
		g->log_mask |= gpu_dbg_map_v;
	}
	if (debug_level >= 3) {
		g->log_mask |= gpu_dbg_pte;
	}

	init_platform(m, g, true);

	if (init_mm(m, g) != 0) {
		unit_return_fail(m, "nvgpu_init_mm_support failed\n");
	}

	return UNIT_SUCCESS;
}

/*
 * Test: test_nvgpu_gmmu_clean
 * This test should be the last one to run as it de-initializes components.
 */
static int test_nvgpu_gmmu_clean(struct unit_module *m,
					struct gk20a *g, void *args)
{
	g->log_mask = 0;
	nvgpu_vm_put(g->mm.pmu.vm);

	return UNIT_SUCCESS;
}


/*
 * Define a few helper functions to decode PTEs.
 * These function rely on functions imported from hw_gmmu_* header files. As a
 * result, when updating this unit test, you must ensure that the HAL functions
 * used to write PTEs are for the same chip as the gmmu_new_pte* functions used
 * below.
 */
static bool pte_is_valid(u32 *pte)
{
	return (pte[0] & gmmu_new_pte_valid_true_f());
}

static bool pte_is_read_only(u32 *pte)
{
	return (pte[0] & gmmu_new_pte_read_only_true_f());
}

static bool pte_is_rw(u32 *pte)
{
	return !(pte[0] & gmmu_new_pte_read_only_true_f());
}

static bool pte_is_priv(u32 *pte)
{
	return (pte[0] & gmmu_new_pte_privilege_true_f());
}

static bool pte_is_volatile(u32 *pte)
{
	return (pte[0] & gmmu_new_pte_vol_true_f());
}

static u64 pte_get_phys_addr(u32 *pte)
{
	u64 addr_bits = ((u64) (pte[1] & 0x00FFFFFF)) << 32;

	addr_bits |= (u64) (pte[0] & ~0xFF);
	addr_bits >>= 8;
	return (addr_bits << gmmu_new_pde_address_shift_v());
}

/*
 * Test: test_nvgpu_gmmu_map_unmap
 * This test does a simple map and unmap of a buffer. Several parameters can
 * be changed and provided in the args.
 * This test will also attempt to compare the data in PTEs to the parameters
 * provided.
 */
static int test_nvgpu_gmmu_map_unmap(struct unit_module *m,
					struct gk20a *g, void *args)
{
	struct nvgpu_mem mem;
	u32 pte[2];
	int result;
	struct nvgpu_os_posix *p = nvgpu_os_posix_from_gk20a(g);
	struct test_parameters *params = (struct test_parameters *) args;

	p->mm_is_iommuable = params->is_iommuable;
	mem.size = TEST_SIZE;
	mem.cpu_va = (void *) TEST_PA_ADDRESS;
	mem.gpu_va = nvgpu_gmmu_map(g->mm.pmu.vm, &mem, mem.size,
			params->flags, params->rw_flag, params->priv,
			params->aperture);

	if (mem.gpu_va == 0) {
		unit_return_fail(m, "Failed to map GMMU page");
	}

	nvgpu_log(g, gpu_dbg_map, "Mapped VA=%p", (void *) mem.gpu_va);

	/*
	 * Based on the VA returned from gmmu_map, lookup the corresponding
	 * PTE
	 */
	result = __nvgpu_get_pte(g, g->mm.pmu.vm, mem.gpu_va, &pte[0]);
	if (result != 0) {
		unit_return_fail(m, "PTE lookup failed with code=%d\n", result);
	}
	nvgpu_log(g, gpu_dbg_map, "Found PTE=%08x %08x", pte[1], pte[0]);

	/* Make sure PTE is valid */
	if (!pte_is_valid(pte) &&
		(!(params->flags & NVGPU_VM_MAP_UNMAPPED_PTE))) {
		unit_return_fail(m, "Unexpected invalid PTE\n");
	}

	/* Make sure PTE corresponds to the PA we wanted to map */
	if (pte_get_phys_addr(pte) != TEST_PA_ADDRESS) {
		unit_return_fail(m, "Unexpected physical address in PTE\n");
	}

	/* Check RO, WO, RW */
	switch (params->rw_flag) {
	case gk20a_mem_flag_none:
		if (!pte_is_rw(pte) &&
			(!(params->flags & NVGPU_VM_MAP_UNMAPPED_PTE))) {
			unit_return_fail(m, "PTE is not RW as expected.\n");
		}
		break;
	case gk20a_mem_flag_write_only:
		/* WO is not supported anymore in Pascal+ */
		break;
	case gk20a_mem_flag_read_only:
		if (!pte_is_read_only(pte)) {
			unit_return_fail(m, "PTE is not RO as expected.\n");
		}
		break;
	default:
		unit_return_fail(m, "Unexpected params->rw_flag value.\n");
		break;
	}

	/* Check privileged bit */
	if ((params->priv) && (!pte_is_priv(pte))) {
		unit_return_fail(m, "PTE is not PRIV as expected.\n");
	} else if (!(params->priv) && (pte_is_priv(pte))) {
		unit_return_fail(m, "PTE is PRIV when it should not.\n");
	}

	/* Check if cached */
	if ((params->flags & NVGPU_VM_MAP_CACHEABLE) && pte_is_volatile(pte)) {
		unit_return_fail(m, "PTE is not cacheable as expected.\n");
	} else if ((params->flags & NVGPU_VM_MAP_CACHEABLE) &&
			pte_is_volatile(pte)) {
		unit_return_fail(m, "PTE is not volatile as expected.\n");
	}

	/* Now unmap the buffer and make sure the PTE is now invalid */
	nvgpu_gmmu_unmap(g->mm.pmu.vm, &mem, mem.gpu_va);

	result = __nvgpu_get_pte(g, g->mm.pmu.vm, mem.gpu_va, &pte[0]);
	if (result != 0) {
		unit_return_fail(m, "PTE lookup failed with code=%d\n", result);
	}

	if (pte_is_valid(pte)) {
		unit_return_fail(m, "PTE still valid for unmapped memory\n");
	}

	return UNIT_SUCCESS;
}

/*
 * Test: test_nvgpu_gmmu_set_pte
 * This test targets the __nvgpu_set_pte() function by mapping a buffer, and
 * then trying to alter the validity bit of the corresponding PTE.
 */
static int test_nvgpu_gmmu_set_pte(struct unit_module *m,
					struct gk20a *g, void *args)
{
	struct nvgpu_mem mem;
	u32 pte[2];
	int result;
	struct nvgpu_os_posix *p = nvgpu_os_posix_from_gk20a(g);
	struct test_parameters *params = (struct test_parameters *) args;

	p->mm_is_iommuable = params->is_iommuable;
	mem.size = TEST_SIZE;
	mem.cpu_va = (void *) TEST_PA_ADDRESS;
	mem.gpu_va = nvgpu_gmmu_map(g->mm.pmu.vm, &mem, mem.size,
			params->flags, params->rw_flag, params->priv,
			params->aperture);

	if (mem.gpu_va == 0) {
		unit_return_fail(m, "Failed to map GMMU page");
	}

	result = __nvgpu_get_pte(g, g->mm.pmu.vm, mem.gpu_va, &pte[0]);
	if (result != 0) {
		unit_return_fail(m, "PTE lookup failed with code=%d\n", result);
	}

	/* Flip the valid bit of the PTE */
	pte[0] &= ~(gmmu_new_pte_valid_true_f());

	/* Test error case where the VA is not mapped */
	result = __nvgpu_set_pte(g, g->mm.pmu.vm, TEST_INVALID_ADDRESS,
				&pte[0]);
	if (result == 0) {
		unit_return_fail(m, "Set PTE succeeded with invalid VA\n");
	}

	/* Now rewrite PTE of the already mapped page */
	result = __nvgpu_set_pte(g, g->mm.pmu.vm, mem.gpu_va, &pte[0]);
	if (result != 0) {
		unit_return_fail(m, "Set PTE failed with code=%d\n", result);
	}

	result = __nvgpu_get_pte(g, g->mm.pmu.vm, mem.gpu_va, &pte[0]);
	if (result != 0) {
		unit_return_fail(m, "PTE lookup failed with code=%d\n", result);
	}

	if (pte_is_valid(pte)) {
		unit_return_fail(m, "Unexpected valid PTE\n");
	}

	return UNIT_SUCCESS;
}

/*
 * Helper function to wrap calls to g->ops.mm.gmmu_map and thus giving
 * access to more parameters
 */
static u64 gmmu_map_advanced(struct unit_module *m, struct gk20a *g,
	struct nvgpu_mem *mem, struct test_parameters *params,
	struct vm_gk20a_mapping_batch *batch)
{
	struct nvgpu_sgt *sgt;
	u64 vaddr;

	struct nvgpu_os_posix *p = nvgpu_os_posix_from_gk20a(g);
	struct vm_gk20a *vm = g->mm.pmu.vm;
	size_t offset = params->offset_pages *
		vm->gmmu_page_sizes[params->page_size];

	p->mm_is_iommuable = params->is_iommuable;

	if (params->sparse && params->special_null_phys) {
		mem->cpu_va = NULL;
	}

	sgt = nvgpu_sgt_create_from_mem(g, mem);

	if (sgt == NULL) {
		unit_err(m, "Failed to create SGT\n");
		return 0;
	}

	if (nvgpu_is_enabled(g, NVGPU_USE_COHERENT_SYSMEM)) {
		params->flags |= NVGPU_VM_MAP_IO_COHERENT;
	}

	nvgpu_mutex_acquire(&vm->update_gmmu_lock);

	vaddr = g->ops.mm.gmmu_map(vm, (u64) mem->cpu_va,
				   sgt,
				   offset,
				   mem->size,
				   params->page_size,
				   0,      /* kind */
				   params->ctag_offset,
				   params->flags,
				   params->rw_flag,
				   false,  /* clear_ctags (unused) */
				   params->sparse,
				   params->priv,
				   batch,
				   params->aperture);
	nvgpu_mutex_release(&vm->update_gmmu_lock);

	nvgpu_sgt_free(g, sgt);

	return vaddr;
}

/*
 * Helper function to wrap calls to g->ops.mm.gmmu_unmap and thus giving
 * access to more parameters
 */
static void gmmu_unmap_advanced(struct vm_gk20a *vm, struct nvgpu_mem *mem,
	u64 gpu_va, struct test_parameters *params,
	struct vm_gk20a_mapping_batch *batch)
{
	struct gk20a *g = gk20a_from_vm(vm);

	nvgpu_mutex_acquire(&vm->update_gmmu_lock);

	g->ops.mm.gmmu_unmap(vm,
			     gpu_va,
			     mem->size,
			     params->page_size,
			     mem->free_gpu_va,
			     gk20a_mem_flag_none,
			     false,
			     batch);

	nvgpu_mutex_release(&vm->update_gmmu_lock);
}

/*
 * Test: test_nvgpu_gmmu_map_unmap_adv
 * Similar to test_nvgpu_gmmu_map_unmap but using the advanced helper functions
 * defined above. This test function is used to test advanced features defined
 * in the parameters.
 */
static int test_nvgpu_gmmu_map_unmap_adv(struct unit_module *m,
					struct gk20a *g, void *args)
{
	struct nvgpu_mem mem;
	u64 vaddr;

	struct test_parameters *params = (struct test_parameters *) args;

	mem.size = TEST_SIZE;
	mem.cpu_va = (void *) TEST_PA_ADDRESS;

	vaddr = gmmu_map_advanced(m, g, &mem, params, NULL);

	if (vaddr == 0ULL) {
		unit_return_fail(m, "Failed to map buffer\n");
	}

	nvgpu_gmmu_unmap(g->mm.pmu.vm, &mem, vaddr);

	return UNIT_SUCCESS;
}

/*
 * Test: test_nvgpu_gmmu_map_unmap_batched
 * This tests uses the batch mode and maps 2 buffers. Then it checks that
 * the flags in the batch structure were set correctly.
 */
static int test_nvgpu_gmmu_map_unmap_batched(struct unit_module *m,
					struct gk20a *g, void *args)
{
	struct nvgpu_mem mem, mem2;
	u64 vaddr, vaddr2;
	struct vm_gk20a_mapping_batch batch;

	struct test_parameters *params = (struct test_parameters *) args;

	mem.size = TEST_SIZE;
	mem.cpu_va = (void *) TEST_PA_ADDRESS;
	mem2.size = TEST_SIZE;
	mem2.cpu_va = (void *) (TEST_PA_ADDRESS + TEST_SIZE);

	vaddr = gmmu_map_advanced(m, g, &mem, params, &batch);
	if (vaddr == 0ULL) {
		unit_return_fail(m, "Failed to map buffer\n");
	}

	vaddr2 = gmmu_map_advanced(m, g, &mem2, params, &batch);
	if (vaddr2 == 0ULL) {
		unit_return_fail(m, "Failed to map buffer 2\n");
	}

	if (!batch.need_tlb_invalidate) {
		unit_return_fail(m, "TLB invalidate flag not set.\n");
	}

	batch.need_tlb_invalidate = false;
	gmmu_unmap_advanced(g->mm.pmu.vm, &mem, vaddr, params, &batch);
	gmmu_unmap_advanced(g->mm.pmu.vm, &mem, vaddr2, params, &batch);

	if (!batch.need_tlb_invalidate) {
		unit_return_fail(m, "TLB invalidate flag not set.\n");
	}

	if (!batch.gpu_l2_flushed) {
		unit_return_fail(m, "GPU L2 not flushed.\n");
	}

	return UNIT_SUCCESS;
}

struct unit_module_test nvgpu_gmmu_tests[] = {
	UNIT_TEST(gmmu_init, test_nvgpu_gmmu_init, (void *) 1),

	UNIT_TEST(gmmu_map_unmap_iommu_sysmem, test_nvgpu_gmmu_map_unmap,
		(void *) &test_iommu_sysmem),
	UNIT_TEST(gmmu_map_unmap_iommu_sysmem_ro, test_nvgpu_gmmu_map_unmap,
		(void *) &test_iommu_sysmem_ro),
	UNIT_TEST(gmmu_map_unmap_no_iommu_sysmem, test_nvgpu_gmmu_map_unmap,
		(void *) &test_no_iommu_sysmem),
	UNIT_TEST(gmmu_map_unmap_vidmem, test_nvgpu_gmmu_map_unmap,
		(void *) &test_no_iommu_vidmem),
	UNIT_TEST(gmmu_map_unmap_iommu_sysmem_coh, test_nvgpu_gmmu_map_unmap,
		(void *) &test_iommu_sysmem_coh),
	UNIT_TEST(gmmu_set_pte, test_nvgpu_gmmu_set_pte,
		(void *) &test_iommu_sysmem),
	UNIT_TEST(gmmu_map_unmap_iommu_sysmem_adv_kernel_pages,
		test_nvgpu_gmmu_map_unmap_adv,
		(void *) &test_iommu_sysmem_adv),
	UNIT_TEST(gmmu_map_unmap_iommu_sysmem_adv_big_pages,
		test_nvgpu_gmmu_map_unmap_adv,
		(void *) &test_iommu_sysmem_adv_big),
	UNIT_TEST(gmmu_map_unmap_iommu_sysmem_adv_big_pages_offset,
		test_nvgpu_gmmu_map_unmap_adv,
		(void *) &test_iommu_sysmem_adv_big_offset),
	UNIT_TEST(gmmu_map_unmap_no_iommu_sysmem_adv_big_pages_offset_large,
		test_nvgpu_gmmu_map_unmap_adv,
		(void *) &test_no_iommu_sysmem_adv_big_offset_large),
	UNIT_TEST(gmmu_map_unmap_iommu_sysmem_adv_small_pages_sparse,
		test_nvgpu_gmmu_map_unmap_adv,
		(void *) &test_iommu_sysmem_adv_small_sparse),
	UNIT_TEST(gmmu_map_unmap_no_iommu_sysmem_noncacheable,
		test_nvgpu_gmmu_map_unmap,
		(void *) &test_no_iommu_sysmem_noncacheable),
	UNIT_TEST(gmmu_map_unmap_iommu_sysmem_adv_small_pages_sparse,
		test_nvgpu_gmmu_map_unmap_adv,
		(void *) &test_iommu_sysmem_adv_ctag),
	UNIT_TEST(gmmu_map_unmap_iommu_sysmem_adv_big_pages_batched,
		test_nvgpu_gmmu_map_unmap_batched,
		(void *) &test_iommu_sysmem_adv_big),
	UNIT_TEST(gmmu_map_unmap_unmapped, test_nvgpu_gmmu_map_unmap,
		(void *) &test_no_iommu_unmapped),

	UNIT_TEST(gmmu_clean, test_nvgpu_gmmu_clean, NULL),
};

UNIT_MODULE(nvgpu_gmmu, nvgpu_gmmu_tests, UNIT_PRIO_NVGPU_TEST);
