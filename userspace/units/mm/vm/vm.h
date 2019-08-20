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

struct gk20a;
struct unit_module;

/** @addtogroup SWUTS-mm-vm
 *  @{
 *
 * Software Unit Test Specification for mm.vm
 */

/**
 * Test specification for: test_map_buf
 *
 * Description: The VM unit shall be able to map a buffer of memory such that
 * the GPU may access that memory.
 *
 * Test Type: Feature based
 *
 * Input: None
 *
 * Steps:
 * - Initialize a VM with the following characteristics:
 *   - 64KB large page support enabled
 *   - Low hole size = 64MB
 *   - Address space size = 128GB
 *   - Kernel reserved space size = 4GB
 * - Map a 4KB buffer into the VM
 *   - Check that the resulting GPU virtual address is aligned to 4KB
 *   - Unmap the buffer
 * - Map a 64KB buffer into the VM
 *   - Check that the resulting GPU virtual address is aligned to 64KB
 *   - Unmap the buffer
 * - Uninitialize the VM
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_map_buf(struct unit_module *m, struct gk20a *g, void *__args);

/**
 * Test specification for: test_map_buf_gpu_va
 *
 * Description: When a GPU virtual address is passed into the nvgpu_vm_map()
 * function the resulting GPU virtual address of the map does/does not match
 * the requested GPU virtual address.
 *
 * Test Type: Feature based
 *
 * Input: None
 *
 * Steps:
 * - Initialize a VM with the following characteristics:
 *   - 64KB large page support enabled
 *   - Low hole size = 64MB
 *   - Address space size = 128GB
 *   - Kernel reserved space size = 4GB
 * - Map a 4KB buffer into the VM at a specific GPU virtual address
 *   - Check that the resulting GPU virtual address is aligned to 4KB
 *   - Check that the resulting GPU VA is the same as the requested GPU VA
 *   - Unmap the buffer
 * - Map a 64KB buffer into the VM at a specific GPU virtual address
 *   - Check that the resulting GPU virtual address is aligned to 64KB
 *   - Check that the resulting GPU VA is the same as the requested GPU VA
 *   - Unmap the buffer
 * - Uninitialize the VM
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_map_buf_gpu_va(struct unit_module *m, struct gk20a *g, void *__args);

/**
 * Test specification for: test_batch
 *
 * Description: This test exercises the VM unit's batch mode. Batch mode is used
 * to optimize cache flushes.
 *
 * Test Type: Feature based
 *
 * Input: None
 *
 * Steps:
 * - Initialize a VM with the following characteristics:
 *   - 64KB large page support enabled
 *   - Low hole size = 64MB
 *   - Address space size = 128GB
 *   - Kernel reserved space size = 4GB
 * - Map/unmap 10 4KB buffers using batch mode
 * - Disable batch mode and verify cache flush counts
 * - Uninitialize the VM
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_batch(struct unit_module *m, struct gk20a *g, void *__args);
