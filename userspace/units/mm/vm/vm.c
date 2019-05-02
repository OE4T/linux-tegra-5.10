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

#include <stdio.h>

#include <unit/unit.h>
#include <unit/io.h>
#include <unit/unit-requirement-ids.h>

#include <nvgpu/posix/types.h>
#include <os/posix/os_posix.h>

#include <nvgpu/gk20a.h>
#include <nvgpu/nvgpu_sgt.h>
#include <nvgpu/vm_area.h>

#include <hal/mm/cache/flush_gk20a.h>
#include <hal/mm/cache/flush_gv11b.h>
#include <hal/mm/gmmu/gmmu_gp10b.h>
#include <hal/mm/gmmu/gmmu_gv11b.h>
#include <hal/fb/fb_gp10b.h>
#include <hal/fb/fb_gm20b.h>

#include <nvgpu/hw/gv11b/hw_gmmu_gv11b.h>

/* Random CPU physical address for the buffers we'll map */
#define BUF_CPU_PA		0xEFAD80000000ULL
#define PHYS_ADDR_BITS_HIGH	0x00FFFFFFU
#define PHYS_ADDR_BITS_LOW	0xFFFFFF00U
/* Check if address is aligned at the requested boundary */
#define IS_ALIGNED(addr, align)	((addr & (align - 1U)) == 0U)

/*
 * Helper function used to create custom SGTs from a list of SGLs.
 * The created SGT needs to be explicitly free'd.
 */
static struct nvgpu_sgt *custom_sgt_create(struct unit_module *m,
					   struct gk20a *g,
					   struct nvgpu_mem *mem,
					   struct nvgpu_mem_sgl *sgl_list,
					   u32 nr_sgls)
{
	int ret = 0;
	struct nvgpu_sgt *sgt = NULL;

	if (mem == NULL) {
		unit_err(m, "mem is NULL\n");
		goto fail;
	}
	if (sgl_list == NULL) {
		unit_err(m, "sgl_list is NULL\n");
		goto fail;
	}

	ret = nvgpu_mem_posix_create_from_list(g, mem, sgl_list, nr_sgls);
	if (ret != 0) {
		unit_err(m, "Failed to create mem from sgl list\n");
		goto fail;
	}

	sgt = nvgpu_sgt_create_from_mem(g, mem);
	if (sgt == NULL) {
		goto fail;
	}

	return sgt;

fail:
	unit_err(m, "Failed to create sgt\n");
	return NULL;
}

/*
 * TODO: This function is copied from the gmmu/page table unit test. Instead of
 * duplicating code, share a single implementation of the function.
 */
static inline bool pte_is_valid(u32 *pte)
{
	return ((pte[0] & gmmu_new_pte_valid_true_f()) != 0U);
}

/*
 * TODO: This function is copied from the gmmu/page table unit test. Instead of
 * duplicating code, share a single implementation of the function.
 */
static u64 pte_get_phys_addr(struct unit_module *m, u32 *pte)
{
	u64 addr_bits;

	if (pte == NULL) {
		unit_err(m, "pte is NULL\n");
		unit_err(m, "Failed to get phys addr\n");
		return 0;
	}

	addr_bits = ((u64)(pte[1] & PHYS_ADDR_BITS_HIGH)) << 32;
	addr_bits |= (u64)(pte[0] & PHYS_ADDR_BITS_LOW);
	addr_bits >>= 8;
	return (addr_bits << gmmu_new_pde_address_shift_v());
}

/* Initialize test environment */
static int init_test_env(struct unit_module *m, struct gk20a *g)
{
	struct nvgpu_os_posix *p = nvgpu_os_posix_from_gk20a(g);
	if (p == NULL) {
		unit_err(m, "posix is NULL\n");
		unit_err(m, "Failed to initialize test environment\n");
		return UNIT_FAIL;
	}
	p->mm_is_iommuable = true;

	nvgpu_set_enabled(g, NVGPU_MM_UNIFIED_MEMORY, true);
	nvgpu_set_enabled(g, NVGPU_HAS_SYNCPOINTS, true);

	g->ops.fb.compression_page_size = gp10b_fb_compression_page_size;
	g->ops.fb.tlb_invalidate = gm20b_fb_tlb_invalidate;

	g->ops.mm.gmmu.get_default_big_page_size =
					gp10b_mm_get_default_big_page_size;
	g->ops.mm.gmmu.get_mmu_levels = gp10b_mm_get_mmu_levels;
	g->ops.mm.gmmu.map = nvgpu_gmmu_map_locked;
	g->ops.mm.gmmu.unmap = nvgpu_gmmu_unmap_locked;
	g->ops.mm.gmmu.gpu_phys_addr = gv11b_gpu_phys_addr;
	g->ops.mm.cache.l2_flush = gv11b_mm_l2_flush;
	g->ops.mm.cache.fb_flush = gk20a_mm_fb_flush;

	return UNIT_SUCCESS;
}

/*
 * Try mapping a buffer into the GPU virtual address space:
 *    - Allocate a new CPU buffer
 *    - If a specific GPU VA was requested, allocate a VM area for a fixed GPU
 *      VA mapping
 *    - Map buffer into the GPU virtual address space
 *    - Verify that the buffer was mapped correctly
 *    - Unmap buffer
 */
static int map_buffer(struct unit_module *m,
	              struct gk20a *g,
	              struct vm_gk20a *vm,
	              u64 cpu_pa,
	              u64 gpu_va,
	              size_t buf_size,
	              size_t page_size,
	              size_t alignment)
{
	int ret = UNIT_SUCCESS;
	struct nvgpu_mapped_buf *mapped_buf = NULL;
	struct nvgpu_mapped_buf *mapped_buf_check = NULL;
	struct nvgpu_os_buffer os_buf = {0};
	struct nvgpu_mem_sgl sgl_list[1];
	struct nvgpu_mem mem = {0};
	struct nvgpu_sgt *sgt = NULL;
	bool fixed_gpu_va = (gpu_va != 0);
	u32 pte[2];

	if (vm == NULL) {
		unit_err(m, "vm is NULL\n");
		ret = UNIT_FAIL;
		goto exit;
	}

	/* Allocate a CPU buffer */
	os_buf.buf = nvgpu_kzalloc(g, buf_size);
	if (os_buf.buf == NULL) {
		unit_err(m, "Failed to allocate a CPU buffer\n");
		ret = UNIT_FAIL;
		goto free_sgt_os_buf;
	}
	os_buf.size = buf_size;

	memset(&sgl_list[0], 0, sizeof(sgl_list[0]));
	sgl_list[0].phys = cpu_pa;
	sgl_list[0].dma = 0;
	sgl_list[0].length = buf_size;

	mem.size = buf_size;
	mem.cpu_va = os_buf.buf;

	/* Create sgt */
	sgt = custom_sgt_create(m, g, &mem, sgl_list, 1);
	if (sgt == NULL) {
		ret = UNIT_FAIL;
		goto free_sgt_os_buf;
	}

	if (fixed_gpu_va) {
		size_t num_pages = DIV_ROUND_UP(buf_size, page_size);
		u64 gpu_va_copy = gpu_va;

		unit_info(m, "Allocating VM Area for fixed GPU VA mapping\n");
		ret = nvgpu_vm_area_alloc(vm,
					  num_pages,
					  page_size,
					  &gpu_va_copy,
					  NVGPU_VM_AREA_ALLOC_FIXED_OFFSET);
		if (ret != 0) {
			unit_err(m, "Failed to allocate a VM area\n");
			ret = UNIT_FAIL;
			goto free_sgt_os_buf;
		}
		if (gpu_va_copy != gpu_va) {
			unit_err(m, "VM area created at the wrong GPU VA\n");
			ret = UNIT_FAIL;
			goto free_vm_area;
		}
	}

	mapped_buf = nvgpu_vm_map(vm,
				  &os_buf,
				  sgt,
				  gpu_va,
				  buf_size,
				  0,
				  gk20a_mem_flag_none,
				  NVGPU_VM_MAP_CACHEABLE,
				  0,
				  0,
				  NULL,
				  APERTURE_SYSMEM);
	if (mapped_buf == NULL) {
		unit_err(m, "Failed to map buffer into the GPU virtual address"
			    " space\n");
		ret = UNIT_FAIL;
		goto free_vm_area;
	}

	/* Check if we can find the mapped buffer */
	mapped_buf_check = nvgpu_vm_find_mapped_buf(vm, mapped_buf->addr);
	if (mapped_buf_check == NULL) {
		unit_err(m, "Can't find mapped buffer\n");
		ret = UNIT_FAIL;
		goto free_mapped_buf;
	}
	if (mapped_buf_check->addr != mapped_buf->addr) {
		unit_err(m, "Invalid buffer GPU VA\n");
		ret = UNIT_FAIL;
		goto free_mapped_buf;
	}

	/*
	 * Based on the virtual address returned, lookup the corresponding PTE
	 */
	ret = nvgpu_get_pte(g, vm, mapped_buf->addr, pte);
	if (ret != 0) {
		unit_err(m, "PTE lookup failed\n");
		ret = UNIT_FAIL;
		goto free_mapped_buf;
	}

	/* Check if PTE is valid */
	if (!pte_is_valid(pte)) {
		unit_err(m, "Invalid PTE!\n");
		ret = UNIT_FAIL;
		goto free_mapped_buf;
	}

	/* Check if PTE corresponds to the physical address we requested */
	if (pte_get_phys_addr(m, pte) != cpu_pa) {
		unit_err(m, "Unexpected physical address in PTE\n");
		ret = UNIT_FAIL;
		goto free_mapped_buf;
	}

	/* Check if the buffer's GPU VA is aligned correctly */
	if (!IS_ALIGNED(mapped_buf->addr, alignment)) {
		unit_err(m, "Incorrect buffer GPU VA alignment\n");
		ret = UNIT_FAIL;
		goto free_mapped_buf;
	}

	/*
	 * If a specific GPU VA was requested, check that the buffer's GPU VA
	 * matches the requested GPU VA
	 */
	if (fixed_gpu_va && (mapped_buf->addr != gpu_va)) {
		unit_err(m, "Mapped buffer's GPU VA does not match requested"
			    " GPU VA\n");
		ret = UNIT_FAIL;
		goto free_mapped_buf;
	}

	ret = UNIT_SUCCESS;

free_mapped_buf:
	if (mapped_buf != NULL) {
		nvgpu_vm_unmap(vm, mapped_buf->addr, NULL);
	}
free_vm_area:
	if (fixed_gpu_va) {
		ret = nvgpu_vm_area_free(vm, gpu_va);
		if (ret != 0) {
			unit_err(m, "Failed to free vm area\n");
			ret = UNIT_FAIL;
		}
	}
free_sgt_os_buf:
	if (sgt != NULL) {
		nvgpu_sgt_free(g, sgt);
	}
	if (os_buf.buf != NULL) {
		nvgpu_kfree(g, os_buf.buf);
	}

exit:
	if (ret == UNIT_FAIL) {
		unit_err(m, "Buffer mapping failed\n");
	}
	return ret;
}

/*
 * This is the test for requirement NVGPU-RQCD-45.C1.
 * Requirement: The VM unit shall be able to map a buffer of memory such that
 * the GPU may access that memory.
 *
 * This test does the following:
 *    - Initialize a VM with the following characteristics:
 *         - 64KB large page support enabled
 *         - Low hole size = 64MB
 *         - Address space size = 128GB
 *         - Kernel reserved space size = 4GB
 *    - Map a 4KB buffer into the VM
 *         - Check that the resulting GPU virtual address is aligned to 4KB
 *         - Unmap the buffer
 *    - Map a 64KB buffer into the VM
 *         - Check that the resulting GPU virtual address is aligned to 64KB
 *         - Unmap the buffer
 *    - Uninitialize the VM
 */
static int test_map_buf(struct unit_module *m, struct gk20a *g, void *__args)
{
	int ret = UNIT_SUCCESS;
	struct vm_gk20a *vm = NULL;
	u64 low_hole = 0;
	u64 user_vma = 0;
	u64 kernel_reserved = 0;
	u64 aperture_size = 0;
	bool big_pages = true;
	size_t buf_size = 0;
	size_t page_size = 0;
	size_t alignment = 0;

	if (m == NULL) {
		ret = UNIT_FAIL;
		goto exit;
	}
	if (g == NULL) {
		unit_err(m, "gk20a is NULL\n");
		ret = UNIT_FAIL;
		goto exit;
	}

	/* Initialize test environment */
	ret = init_test_env(m, g);
	if (ret != UNIT_SUCCESS) {
		goto exit;
	}

	/* Initialize VM */
	big_pages = true;
	low_hole = SZ_1M * 64;
	aperture_size = 128 * SZ_1G;
	kernel_reserved = 4 * SZ_1G - low_hole;
	user_vma = aperture_size - low_hole - kernel_reserved;
	unit_info(m, "Initializing VM:\n");
	unit_info(m, "   - Low Hole Size = 0x%llx\n", low_hole);
	unit_info(m, "   - User Aperture Size = 0x%llx\n", user_vma);
	unit_info(m, "   - Kernel Reserved Size = 0x%llx\n", kernel_reserved);
	unit_info(m, "   - Total Aperture Size = 0x%llx\n", aperture_size);
	vm = nvgpu_vm_init(g,
			   g->ops.mm.gmmu.get_default_big_page_size(),
			   low_hole,
			   kernel_reserved,
			   aperture_size,
			   big_pages,
			   false,
			   true,
			   __func__);
	if (vm == NULL) {
		unit_err(m, "Failed to init VM\n");
		ret = UNIT_FAIL;
		goto exit;
	}

	/* Map 4KB buffer */
	buf_size = SZ_4K;
	page_size = SZ_4K;
	alignment = SZ_4K;
	unit_info(m, "Mapping Buffer:\n");
	unit_info(m, "   - CPU PA = 0x%llx\n", BUF_CPU_PA);
	unit_info(m, "   - Buffer Size = 0x%lx\n", buf_size);
	unit_info(m, "   - Page Size = 0x%lx\n", page_size);
	unit_info(m, "   - Alignment = 0x%lx\n", alignment);
	ret = map_buffer(m,
			 g,
			 vm,
			 BUF_CPU_PA,
			 0,
			 buf_size,
			 page_size,
			 alignment);
	if (ret != UNIT_SUCCESS) {
		unit_err(m, "4KB buffer mapping failed\n");
		goto exit;
	}

	/* Map 64KB buffer */
	buf_size = SZ_64K;
	page_size = SZ_64K;
	alignment = SZ_64K;
	unit_info(m, "Mapping Buffer:\n");
	unit_info(m, "   - CPU PA = 0x%llx\n", BUF_CPU_PA);
	unit_info(m, "   - Buffer Size = 0x%lx\n", buf_size);
	unit_info(m, "   - Page Size = 0x%lx\n", page_size);
	unit_info(m, "   - Alignment = 0x%lx\n", alignment);
	ret = map_buffer(m,
			 g,
			 vm,
			 BUF_CPU_PA,
			 0,
			 buf_size,
			 page_size,
			 alignment);
	if (ret != UNIT_SUCCESS) {
		unit_err(m, "64KB buffer mapping failed\n");
		goto exit;
	}

	ret = UNIT_SUCCESS;

exit:
	if (vm != NULL) {
		nvgpu_vm_put(vm);
	}

	return ret;
}

/*
 * This is the test for requirement NVGPU-RQCD-45.C2.
 * Requirement: When a GPU virtual address is passed into the nvgpu_vm_map()
 * function the resulting GPU virtual address of the map does/does not match
 * the requested GPU virtual address.
 *
 * This test does the following:
 *    - Initialize a VM with the following characteristics:
 *         - 64KB large page support enabled
 *         - Low hole size = 64MB
 *         - Address space size = 128GB
 *         - Kernel reserved space size = 4GB
 *    - Map a 4KB buffer into the VM at a specific GPU virtual address
 *         - Check that the resulting GPU virtual address is aligned to 4KB
 *         - Check that the resulting GPU VA is the same as the requested GPU VA
 *         - Unmap the buffer
 *    - Map a 64KB buffer into the VM at a specific GPU virtual address
 *         - Check that the resulting GPU virtual address is aligned to 64KB
 *         - Check that the resulting GPU VA is the same as the requested GPU VA
 *         - Unmap the buffer
 *    - Uninitialize the VM
 */
static int test_map_buf_gpu_va(struct unit_module *m,
			       struct gk20a *g,
			       void *__args)
{
	int ret = UNIT_SUCCESS;
	struct vm_gk20a *vm = NULL;
	u64 low_hole = 0;
	u64 user_vma = 0;
	u64 user_vma_limit = 0;
	u64 kernel_reserved = 0;
	u64 aperture_size = 0;
	u64 gpu_va = 0;
	bool big_pages = true;
	size_t buf_size = 0;
	size_t page_size = 0;
	size_t alignment = 0;

	if (m == NULL) {
		ret = UNIT_FAIL;
		goto exit;
	}
	if (g == NULL) {
		unit_err(m, "gk20a is NULL\n");
		ret = UNIT_FAIL;
		goto exit;
	}

	/* Initialize test environment */
	ret = init_test_env(m, g);
	if (ret != UNIT_SUCCESS) {
		goto exit;
	}

	/* Initialize VM */
	big_pages = true;
	low_hole = SZ_1M * 64;
	aperture_size = 128 * SZ_1G;
	kernel_reserved = 4 * SZ_1G - low_hole;
	user_vma = aperture_size - low_hole - kernel_reserved;
	user_vma_limit = aperture_size - kernel_reserved;
	unit_info(m, "Initializing VM:\n");
	unit_info(m, "   - Low Hole Size = 0x%llx\n", low_hole);
	unit_info(m, "   - User Aperture Size = 0x%llx\n", user_vma);
	unit_info(m, "   - Kernel Reserved Size = 0x%llx\n", kernel_reserved);
	unit_info(m, "   - Total Aperture Size = 0x%llx\n", aperture_size);
	vm = nvgpu_vm_init(g,
			   g->ops.mm.gmmu.get_default_big_page_size(),
			   low_hole,
			   kernel_reserved,
			   aperture_size,
			   big_pages,
			   false,
			   true,
			   __func__);
	if (vm == NULL) {
		unit_err(m, "Failed to init VM\n");
		ret = UNIT_FAIL;
		goto exit;
	}

	/* Map 4KB buffer */
	buf_size = SZ_4K;
	page_size = SZ_4K;
	alignment = SZ_4K;
	/*
	 * Calculate a valid base GPU VA for the buffer. We're multiplying
	 * buf_size by 10 just to be on the safe side.
	 */
	gpu_va = user_vma_limit - buf_size*10;
	unit_info(m, "Mapping Buffer:\n");
	unit_info(m, "   - CPU PA = 0x%llx\n", BUF_CPU_PA);
	unit_info(m, "   - GPU VA = 0x%llx\n", gpu_va);
	unit_info(m, "   - Buffer Size = 0x%lx\n", buf_size);
	unit_info(m, "   - Page Size = 0x%lx\n", page_size);
	unit_info(m, "   - Alignment = 0x%lx\n", alignment);
	ret = map_buffer(m,
			 g,
			 vm,
			 BUF_CPU_PA,
			 gpu_va,
			 buf_size,
			 page_size,
			 alignment);
	if (ret != UNIT_SUCCESS) {
		unit_err(m, "4KB buffer mapping failed\n");
		goto exit;
	}

	/* Map 64KB buffer */
	buf_size = SZ_64K;
	page_size = SZ_64K;
	alignment = SZ_64K;
	/*
	 * Calculate a valid base GPU VA for the buffer. We're multiplying
	 * buf_size by 10 just to be on the safe side.
	 */
	gpu_va = user_vma_limit - buf_size*10;
	unit_info(m, "Mapping Buffer:\n");
	unit_info(m, "   - CPU PA = 0x%llx\n", BUF_CPU_PA);
	unit_info(m, "   - GPU VA = 0x%llx\n", gpu_va);
	unit_info(m, "   - Buffer Size = 0x%lx\n", buf_size);
	unit_info(m, "   - Page Size = 0x%lx\n", page_size);
	unit_info(m, "   - Alignment = 0x%lx\n", alignment);
	ret = map_buffer(m,
			 g,
			 vm,
			 BUF_CPU_PA,
			 gpu_va,
			 buf_size,
			 page_size,
			 alignment);
	if (ret != UNIT_SUCCESS) {
		unit_err(m, "64KB buffer mapping failed\n");
		goto exit;
	}

	ret = UNIT_SUCCESS;

exit:
	if (vm != NULL) {
		nvgpu_vm_put(vm);
	}

	return ret;
}

struct unit_module_test vm_tests[] = {
	UNIT_TEST_REQ("NVGPU-RQCD-45.C1",
		      VM_REQ1_UID,
		      "V5",
		      map_buf,
		      test_map_buf,
		      NULL, 0),
	UNIT_TEST_REQ("NVGPU-RQCD-45.C2",
		      VM_REQ1_UID,
		      "V5",
		      map_buf_gpu_va,
		      test_map_buf_gpu_va,
		      NULL, 0),
};

UNIT_MODULE(vm, vm_tests, UNIT_PRIO_NVGPU_TEST);
