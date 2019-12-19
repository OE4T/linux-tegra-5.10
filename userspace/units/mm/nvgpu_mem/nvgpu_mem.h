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

#ifndef UNIT_NVGPU_MEM_H
#define UNIT_NVGPU_MEM_H

struct gk20a;
struct unit_module;

/** @addtogroup SWUTS-mm-nvgpu-mem
 *  @{
 *
 * Software Unit Test Specification for mm.nvgpu_mem
 */

/**
 * Test specification for: test_nvgpu_mem_create_from_phys
 *
 * Description: Initialize nvgpu_mem for given size and base address.
 *
 * Test Type: Feature
 *
 * Input: None
 *
 * Steps:
 * - Initialize nvgpu_mem
 *   - Allocate memory for nvgpu_mem sgt and sgl
 *   - Initialize nvgpu_mem structure members to appropriate value.
 * - Allocate cpu_va memory for later tests
 *
 * Output: Returns SUCCESS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_nvgpu_mem_create_from_phys(struct unit_module *m,
						struct gk20a *g, void *args);

/**
 * Test specification for: test_nvgpu_mem_phys_ops
 *
 * Description: Check all nvgpu_sgt_ops functions
 *
 * Test Type: Feature
 *
 * Input: test_nvgpu_mem_create_from_phys
 *
 * Steps:
 * - Execute nvgpu_sgt_ops functions
 *   - Check if each nvgpu_sgt_ops function executes and returns expected value.
 *
 * Output: Returns SUCCESS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_nvgpu_mem_phys_ops(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_nvgpu_memset_sysmem
 *
 * Description: Store pre-defined pattern at allocated nvgpu_mem address
 *
 * Test Type: Feature
 *
 * Input: test_nvgpu_mem_create_from_phys
 *
 * Steps:
 * - Store data pattern and check value for multiple cases
 *   - Execute below steps for APERTURE_SYSMEM and APERTURE_INVALID cases
 *   - Using nvgpu_memset() store pre-defined data pattern in part of allocated
 * memory
 *   - Check if set data pattern is correctly stored
 *
 * Output: Returns SUCCESS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_nvgpu_memset_sysmem(struct unit_module *m,
						struct gk20a *g, void *args);

/**
 * Test specification for: test_nvgpu_mem_wr_rd
 *
 * Description: Test read and write functions for sysmem
 *
 * Test Type: Feature
 *
 * Input: test_nvgpu_mem_create_from_phys
 *
 * Steps:
 * - Check if memory is of sysmem type
 * - Check if memory aperture is not invalid
 * - Execute below steps for APERTURE_SYSMEM and APERTURE_INVALID cases
 * - Execute all write functions and confirm data written
 *   - Write preset data pattern to allocated nvgpu_mem
 *   - Confirm data written at the memory location is correct
 * - Execute read functions and confirm data read
 *   - Read data from a segment of allocated memory
 *   - Confirm read data is correct
 *
 * Output: Returns SUCCESS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_nvgpu_mem_wr_rd(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_nvgpu_mem_iommu_translate
 *
 * Description: Test if given address is iommuable
 *
 * Test Type: Feature
 *
 * Input: test_nvgpu_mem_create_from_phys
 *
 * Steps:
 * - Check if nvgpu_mem is iommuable
 *   - Return value is equal to nvgpu_mem phys address value
 *
 * Output: Returns SUCCESS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_nvgpu_mem_iommu_translate(struct unit_module *m,
						struct gk20a *g, void *args);

/**
 * Test specification for: test_nvgpu_aperture_mask
 *
 * Description: Check if nvgpu_mem aperture is correct
 *
 * Test Type: Feature
 *
 * Input: test_nvgpu_mem_create_from_phys
 *
 * Steps:
 * - Execute these steps for all the aperture types
 * - Check if nvgpu_mem aperture mask values returned are as expected
 *
 * Output: Returns SUCCESS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_nvgpu_aperture_mask(struct unit_module *m,
						struct gk20a *g, void *args);

/**
 * Test specification for: test_nvgpu_mem_vidmem
 *
 * Description: Test read and write memory functions for vidmem
 *
 * Test Type: Feature
 *
 * Input: test_nvgpu_mem_create_from_phys
 *
 * Steps:
 * - Execute read and write calls for vidmem which are converted to pramin calls
 *   - pramin functions are tested in pramin module
 *
 * Output: Returns SUCCESS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_nvgpu_mem_vidmem(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_free_nvgpu_mem
 *
 * Description: Cleanup allocated memory for nvgpu_mem structure
 *
 * Test Type: Other (cleanup)
 *
 * Input: test_nvgpu_mem_create_from_phys
 *
 * Steps:
 * - Free allocated memory
 *
 * Output: Returns SUCCESS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_free_nvgpu_mem(struct unit_module *m, struct gk20a *g, void *args);

#endif /* UNIT_NVGPU_MEM_H */
