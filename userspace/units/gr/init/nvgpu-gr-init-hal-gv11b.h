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
#ifndef UNIT_NVGPU_GR_INIT_HAL_GV11B_H
#define UNIT_NVGPU_GR_INIT_HAL_GV11B_H

#include <nvgpu/types.h>

struct gk20a;
struct unit_module;

/** @addtogroup SWUTS-gr-init-hal-gv11b
 *  @{
 *
 * Software Unit Test Specification for common.gr.init HAL
 */

/**
 * Test specification for: test_gr_init_hal_wait_empty.
 *
 * Description: Verify error handling in g->ops.gr.init.wait_empty.
 *
 * Test Type: Feature, Error guessing.
 *
 * Targets: g->ops.gr.init.wait_empty.
 *
 * Input: gr_init_setup, gr_init_prepare, gr_init_support must have
 *        been executed successfully.
 *
 * Steps:
 * - Inject timeout error and call g->ops.gr.init.wait_empty.
 *   Should fail since timeout initialization fails.
 * - Set various pass/fail values of gr_status and gr_activity registers
 *   and verify the pass/fail output of g->ops.gr.init.wait_empty as
 *   appropriate.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_gr_init_hal_wait_empty(struct unit_module *m,
		struct gk20a *g, void *args);

/**
 * Test specification for: test_gr_init_hal_ecc_scrub_reg.
 *
 * Description: Verify error handling in gops.gr.init.ecc_scrub_reg function.
 *
 * Test Type: Feature, Error guessing.
 *
 * Targets: g->ops.gr.init.ecc_scrub_reg.
 *
 * Input: gr_init_setup, gr_init_prepare, gr_init_support must have
 *        been executed successfully.
 *
 * Steps:
 * - Disable feature flags for common.gr ECC handling for code coverage
 *   and call g->ops.gr.init.ecc_scrub_reg.
 * - Re-enable all the feature flags.
 * - Inject timeout initialization failures and call
 *   g->ops.gr.init.ecc_scrub_reg.
 * - Set incorrect values of scrub_done for each error type so that scrub
 *   wait times out.
 * - Ensure that g->ops.gr.init.ecc_scrub_reg returns error.
 * - Set correct values of scrub_done for each error so that scrub wait
 *   is successful again.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_gr_init_hal_ecc_scrub_reg(struct unit_module *m,
		struct gk20a *g, void *args);

/**
 * Test specification for: test_gr_init_hal_error_injection.
 *
 * Description: Code coverage test for g->ops.gr.init.commit_global_pagepool.
 *
 * Test Type: Feature, Error guessing.
 *
 * Targets: g->ops.gr.init.commit_global_pagepool.
 *
 * Input: gr_init_setup, gr_init_prepare, gr_init_support must have
 *        been executed successfully.
 *
 * Steps:
 * - Setup VM for testing.
 * - Setup gr_ctx and patch_ctx for testing.
 * - Call g->ops.gr.init.commit_global_pagepool with global_ctx flag set
 *   to false and with arbitrary size.
 * - Read back size from register and ensure correct size is set.
 * - Cleanup temporary resources.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_gr_init_hal_error_injection(struct unit_module *m,
		struct gk20a *g, void *args);

#endif /* UNIT_NVGPU_GR_INIT_HAL_GV11B_H */

/**
 * @}
 */

