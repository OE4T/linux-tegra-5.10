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

#ifndef UNIT_NVGPU_INIT_H
#define UNIT_NVGPU_INIT_H

struct gk20a;
struct unit_module;

/** @addtogroup SWUTS-init
 *  @{
 *
 * Software Unit Test Specification for nvgpu.common.init
 */

/**
 * Test specification for: test_setup_env
 *
 * Description: Do basic setup before starting other tests.
 *
 * Test Type: Other (setup)
 *
 * Input: None
 *
 * Steps:
 * - Initialize reg spaces used by init unit tests.
 *
 * Output:
 * - UNIT_FAIL if encounters an error creating reg space
 * - UNIT_SUCCESS otherwise
 */
int test_setup_env(struct unit_module *m,
			  struct gk20a *g, void *args);

/**
 * Test specification for: test_free_env
 *
 * Description: Cleanup resources allocated in test_setup_env()
 *
 * Test Type: Other (setup)
 *
 * Input: None
 *
 * Steps:
 * - Delete reg spaces
 *
 * Output:
 * - UNIT_SUCCESS always
 */
int test_free_env(struct unit_module *m,
			 struct gk20a *g, void *args);

/**
 * Test specification for: test_can_busy
 *
 * Description: Validate nvgpu_can_busy()
 *
 * Test Type: Feature based
 *
 * Input: None
 *
 * Steps:
 * - Vary NVGPU_KERNEL_IS_DYING & NVGPU_DRIVER_IS_DYING enable values and
 *   verify the result from nvgpu_can_busy()
 *
 *
 * Output:
 * - UNIT_FAIL if nvgpu_can_busy() returns the incorrect value.
 * - UNIT_SUCCESS otherwise
 */
int test_can_busy(struct unit_module *m,
			 struct gk20a *g, void *args);

/**
 * Test specification for: test_get_put
 *
 * Description: Validate nvgpu_get() and nvgpu_put() and the refcount.
 *
 * Test Type: Feature based
 *
 * Input:
 * - test_setup_env() must be called before.
 *
 * Steps:
 * - Initialize refcount.
 * - Get gpu and validate return and refcount.
 * - Put gpu and validate refcount.
 * - Put gpu again to initiate teardown and validate refcount.
 * - Get gpu again to verify failure return and validate refcount.
 * - Re-Initialize refcount.
 * - Set function pointers to NULL to test different paths/branches.
 * - Get gpu and validate return and refcount.
 * - Put gpu and validate refcount.
 * - Put gpu again to initiate teardown and validate refcount.
 *
 * Output:
 * - UNIT_FAIL if nvgpu_get() returns the incorrect value or refcount is
 *   incorrect
 * - UNIT_SUCCESS otherwise
 */
int test_get_put(struct unit_module *m,
			struct gk20a *g, void *args);

/**
 * Test specification for: test_check_gpu_state
 *
 * Description: Validate the nvgpu_check_gpu_state() API which will restart
 *
 * Test Type: Feature based
 *
 * Input:
 * - test_setup_env() must be called before.
 *
 * Steps:
 * - Test valid case.
 *   - Set the mc_boot_0 reg to a valid state.
 *   - Call nvgpu_check_gpu_state() and the call should return normally.
 * - Test invalid case.
 *   - Set the mc_boot_0 reg to the invalid state.
 *   - Call nvgpu_check_gpu_state() and trap the BUG() call.
 *
 * Output:
 * - UNIT_FAIL if nvgpu_check_gpu_state() does not cause a BUG() for the invalid
 *   case
 * - If the valid case fails, BUG() may occur and cause the framework to stop
 *   the test.
 * - UNIT_SUCCESS otherwise
 */
int test_check_gpu_state(struct unit_module *m,
				struct gk20a *g, void *args);

/**
 * Test specification for: test_hal_init
 *
 * Description: Test HAL initialization for GV11B
 *
 * Test Type: Feature based
 *
 * Input:
 * - test_setup_env() must be called before.
 *
 * Steps:
 * - Setup the mc_boot_0 reg for GV11B.
 * - Initialize the fuse regs.
 * - Init the HAL and verify return.
 *
 * Output:
 * - UNIT_FAIL if HAL initialization fails
 * - UNIT_SUCCESS otherwise
 */
int test_hal_init(struct unit_module *m,
			 struct gk20a *g, void *args);

/**
 * Test specification for: test_poweron
 *
 * Description: Test nvgpu_finalize_poweron
 *
 * Test Type: Feature based
 *
 * Input:
 * - test_setup_env() must be called before.
 *
 * Steps:
 * 1) Setup poweron init function pointers.
 * 2) Call nvgpu_finalize_poweron().
 * 3) Check return status.
 * - These 3 basic steps are repeated:
 *   a) For the case where all units return success.
 *   b) Once each for individual unit returning failure.
 * - Lastly, it verifies the case where the the deviceis already powered on.
 *
 * Output:
 * - UNIT_FAIL if nvgpu_finalize_poweron() ever returns the unexpected value.
 * - UNIT_SUCCESS otherwise
 */
int test_poweron(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_poweron_branches
 *
 * Description: Test branches in nvgpu_finalize_poweron not covered by the
 * basic path already covered in test_poweron.
 *
 * Test Type: Feature based
 *
 * Input:
 * - test_setup_env() must be called before.
 *
 * Steps:
 * 1) Setup poweron init function pointers to NULL and enable flags.
 * 2) Call nvgpu_finalize_poweron().
 * 3) Check return status.
 * 4) Test syncpt handling by enabling syncpts, altering syncpt flags, and
 *    manipluatin mem calls to cover other paths in the syncpt init.
 *
 * Output:
 * - UNIT_FAIL if nvgpu_finalize_poweron() ever returns the unexpected value.
 * - UNIT_SUCCESS otherwise
 */
int test_poweron_branches(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_poweroff
 *
 * Description: Test nvgpu_prepare_poweroff
 *
 * Test Type: Feature based
 *
 * Input:
 * - test_setup_env() must be called before.
 *
 * Steps:
 * 1) Setup poweroff init function pointers.
 * 2) Call nvgpu_finalize_poweron().
 * 3) Check return status.
 * - These 3 basic steps are repeated:
 *   a) For the case where all units return success.
 *   b) Once each for individual unit returning failure.
 *   b) To complete branch coverage, with appropriate function poiners set to
 *      NULL.
 *
 * Output:
 * - UNIT_FAIL if nvgpu_finalize_poweron() ever returns the unexpected value.
 * - UNIT_SUCCESS otherwise
 */
int test_poweroff(struct unit_module *m, struct gk20a *g, void *args);

#endif /* UNIT_NVGPU_INIT_H */
