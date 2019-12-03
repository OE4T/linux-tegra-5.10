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

/** @addtogroup SWUTS-falcon
 *  @{
 *
 * Software Unit Test Specification for falcon
 */

/**
 * Test specification for: test_falcon_sw_init_free
 *
 * Description: The falcon unit shall be able to initialize the falcon's
 * base register address, required software setup for valid falcon ID.
 *
 * Test Type: Feature based
 *
 * Input: None.
 *
 * Steps:
 * - Invoke nvgpu_falcon_sw_init with valid falcon ID before initializing HAL.
 *   - Verify that falcon initialization fails since valid gpu_arch|impl
 *     are not initialized.
 * - Initialize the test environment:
 *   - Register read/write IO callbacks that handle falcon IO as well.
 *   - Add relevant fuse registers to the register space.
 *   - Initialize hal to setup the hal functions.
 *   - Initialize UTF (Unit Test Framework) falcon structures for PMU and
 *     GPCCS falcons.
 *   - Create and initialize test buffer with random data.
 * - Invoke nvgpu_falcon_sw_init with invalid falcon ID.
 *   - Verify that falcon initialization fails.
 * - Invoke nvgpu_falcon_sw_init with valid falcon ID.
 *   - Verify that falcon initialization succeeds.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_falcon_sw_init_free(struct unit_module *m, struct gk20a *g,
			     void *__args);

/**
 * Test specification for: test_falcon_reset
 *
 * Description: The falcon unit shall be able to reset the falcon CPU or trigger
 * engine specific reset for valid falcon ID.
 *
 * Test Type: Feature based
 *
 * Input: None.
 *
 * Steps:
 * - Invoke nvgpu_falcon_reset with NULL falcon pointer.
 *   - Verify that reset fails with -EINVAL return value.
 * - Invoke nvgpu_falcon_reset with uninitialized falcon struct.
 *   - Verify that reset fails with -EINVAL return value.
 * - Invoke nvgpu_falcon_reset with valid falcon ID.
 *   - Verify that falcon initialization succeeds and check that bit
 *     falcon_cpuctl_hreset_f is set in falcon_cpuctl register.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_falcon_reset(struct unit_module *m, struct gk20a *g, void *__args);

/**
 * Test specification for: test_falcon_mem_scrub
 *
 * Description: The falcon unit shall be able to check and return the falcon
 * memory scrub status.
 *
 * Test Type: Feature based
 *
 * Input: None.
 *
 * Steps:
 * - Invoke nvgpu_falcon_mem_scrub_wait with uninitialized falcon struct.
 *   - Verify that wait fails with -EINVAL return value.
 * - Invoke nvgpu_falcon_mem_scrub_wait with initialized falcon struct where
 *   underlying falcon's memory scrub is completed.
 *   - Verify that wait succeeds with 0 return value.
 * - Invoke nvgpu_falcon_mem_scrub_wait with initialized falcon struct where
 *   underlying falcon's memory scrub is yet to complete.
 *   - Verify that wait fails with -ETIMEDOUT return value.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_falcon_mem_scrub(struct unit_module *m, struct gk20a *g, void *__args);

/**
 * Test specification for: test_falcon_idle
 *
 * Description: The falcon unit shall be able to check and return the falcon
 * idle status.
 *
 * Test Type: Feature based
 *
 * Input: None.
 *
 * Steps:
 * - Invoke nvgpu_falcon_wait_idle with uninitialized falcon struct.
 *   - Verify that wait fails with -EINVAL return value.
 * - Invoke nvgpu_falcon_wait_idle with initialized falcon struct where
 *   underlying falcon is idle.
 *   - Verify that wait succeeds with 0 return value.
 * - Invoke nvgpu_falcon_wait_idle with initialized falcon struct where
 *   underlying falcon is not idle.
 *   - Verify that wait fails with -ETIMEDOUT return value.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_falcon_idle(struct unit_module *m, struct gk20a *g, void *__args);

/**
 * Test specification for: test_falcon_halt
 *
 * Description: The falcon unit shall be able to check and return the falcon
 * halt status.
 *
 * Test Type: Feature based
 *
 * Input: None.
 *
 * Steps:
 * - Invoke nvgpu_falcon_wait_for_halt with uninitialized falcon struct.
 *   - Verify that wait fails with -EINVAL return value.
 * - Invoke nvgpu_falcon_wait_for_halt with initialized falcon struct where
 *   underlying falcon is halted.
 *   - Verify that wait succeeds with 0 return value.
 * - Invoke nvgpu_falcon_wait_for_halt with initialized falcon struct where
 *   underlying falcon is not halted.
 *   - Verify that wait fails with -ETIMEDOUT return value.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_falcon_halt(struct unit_module *m, struct gk20a *g, void *__args);

/**
 * Test specification for: test_falcon_mem_rw_init
 *
 * Description: The falcon unit shall be able to write to falcon's IMEM and
 * DMEM.
 *
 * Test Type: Feature based
 *
 * Input: None.
 *
 * Steps:
 * - Invoke nvgpu_falcon_copy_to_imem and nvgpu_falcon_copy_to_dmem with
 *   uninitialized falcon struct with sample random data.
 *   - Verify that writes fail with -EINVAL return value in both cases.
 * - Invoke nvgpu_falcon_copy_to_imem and nvgpu_falcon_copy_to_dmem with
 *   initialized falcon struct with sample random data.
 *   - Verify that writes succeed with 0 return value in both cases.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_falcon_mem_rw_init(struct unit_module *m, struct gk20a *g,
			    void *__args);

/**
 * Test specification for: test_falcon_mem_rw_range
 *
 * Description: The falcon unit shall be able to write to falcon's IMEM and
 * DMEM in accessible range.
 *
 * Test Type: Feature based
 *
 * Input: None.
 *
 * Steps:
 * - Invoke nvgpu_falcon_copy_to_imem and nvgpu_falcon_copy_to_dmem with
 *   initialized falcon struct with sample random data and valid range.
 *   - Verify that writes succeed with 0 return value in both cases.
 * - Invoke nvgpu_falcon_copy_to_imem and nvgpu_falcon_copy_to_dmem with
 *   initialized falcon struct with sample random data and invalid range.
 *   - Verify that writes fail with -EINVAL return value in both cases.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_falcon_mem_rw_range(struct unit_module *m, struct gk20a *g,
			     void *__args);

/**
 * Test specification for: test_falcon_mem_rw_aligned
 *
 * Description: The falcon unit shall be able to write to falcon's IMEM and
 * DMEM only at aligned offsets.
 *
 * Test Type: Feature based
 *
 * Input: None.
 *
 * Steps:
 * - Invoke nvgpu_falcon_copy_to_imem and nvgpu_falcon_copy_to_dmem with
 *   initialized falcon struct with sample random data and 4-byte aligned
 *   offset.
 *   - Verify that writes succeed with 0 return value in both cases.
 * - Invoke nvgpu_falcon_copy_to_imem and nvgpu_falcon_copy_to_dmem with
 *   initialized falcon struct with sample random data and non 4-byte
 *   aligned offset.
 *   - Verify that writes fail with -EINVAL return value in both cases.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_falcon_mem_rw_aligned(struct unit_module *m, struct gk20a *g,
			       void *__args);

/**
 * Test specification for: test_falcon_mem_rw_zero
 *
 * Description: The falcon unit shall fail the API call to write zero
 * bytes to falcon memory.
 *
 * Test Type: Feature based
 *
 * Input: None.
 *
 * Steps:
 * - Invoke nvgpu_falcon_copy_to_imem and nvgpu_falcon_copy_to_dmem with
 *   initialized falcon struct with sample random data and zero bytes.
 *   - Verify that writes fail with -EINVAL return value in both cases.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_falcon_mem_rw_zero(struct unit_module *m, struct gk20a *g,
			    void *__args);

/**
 * Test specification for: test_falcon_mailbox
 *
 * Description: The falcon unit shall read and write value of falcon's mailbox
 * registers.
 *
 * Test Type: Feature based
 *
 * Input: None.
 *
 * Steps:
 * - Invoke nvgpu_falcon_mailbox_read and nvgpu_falcon_mailbox_write with
 *   uninitialized falcon struct.
 *   - Verify that read returns zero.
 * - Write a sample value to mailbox registers and read using the nvgpu APIs.
 *   - Verify the value by reading the registers through IO accessor.
 * - Read/Write value from invalid mailbox register of initialized falcon.
 *   - Verify that read returns zero.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_falcon_mailbox(struct unit_module *m, struct gk20a *g, void *__args);

/**
 * Test specification for: test_falcon_bootstrap
 *
 * Description: The falcon unit shall configure the bootstrap parameters into
 * falcon memory and registers.
 *
 * Test Type: Feature based
 *
 * Input: None.
 *
 * Steps:
 * - Invoke nvgpu_falcon_bootstrap with uninitialized falcon struct.
 *   - Verify that call fails with -EINVAL return value.
 * - Invoke nvgpu_falcon_bootstrap with initialized falcon struct.
 *   - Verify that call succeeds.
 * - Invoke nvgpu_falcon_hs_ucode_load_bootstrap with uninitialized
 *   falcon struct.
 *   - Verify that call fails with -EINVAL return value.
 * - Fetch the ACR firmware from filesystem.
 * - Invoke nvgpu_falcon_hs_ucode_load_bootstrap with initialized falcon struct.
 *   Fail the falcon reset by failing mem scrub wait.
 *   - Verify that bootstrap fails.
 * - Invoke nvgpu_falcon_hs_ucode_load_bootstrap with initialized falcon struct.
 *   Fail the imem copy for non-secure code by setting invalid size in ucode
 *   header.
 *   - Verify that bootstrap fails.
 * - Invoke nvgpu_falcon_hs_ucode_load_bootstrap with initialized falcon struct.
 *   Fail the imem copy for secure code by setting invalid size in ucode header.
 *   - Verify that bootstrap fails.
 * - Invoke nvgpu_falcon_hs_ucode_load_bootstrap with initialized falcon struct.
 *   Fail the imem copy for secure code by setting invalid size in ucode header.
 *   - Verify that bootstrap fails.
 * - Invoke nvgpu_falcon_hs_ucode_load_bootstrap with initialized falcon struct.
 *   Fail the dmem copy setting invalid dmem size in ucode header.
 *   - Verify that bootstrap fails.
 * - Invoke nvgpu_falcon_hs_ucode_load_bootstrap with initialized falcon struct.
 *   - Verify that bootstrap succeeds and verify the expected state of registers
 *     falcon_dmactl_r, falcon_falcon_bootvec_r, falcon_falcon_cpuctl_r.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_falcon_bootstrap(struct unit_module *m, struct gk20a *g, void *__args);

/**
 * Test specification for: test_falcon_mem_rw_unaligned_cpu_buffer
 *
 * Description: The falcon unit shall be able to read/write from/to falcon's
 * IMEM and DMEM from memory buffer that is unaligned.
 *
 * Test Type: Feature based
 *
 * Input: None.
 *
 * Steps:
 * - Initialize unaligned random data memory buffer and set size.
 * - Invoke nvgpu_falcon_copy_to_imem and nvgpu_falcon_copy_to_dmem with
 *   initialized falcon struct with above initialized sample random data
 *   and valid range.
 *   - Verify that writes succeed with 0 return value in both cases.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_falcon_mem_rw_unaligned_cpu_buffer(struct unit_module *m,
					    struct gk20a *g, void *__args);
