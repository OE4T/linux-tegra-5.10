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
#ifndef UNIT_NVGPU_LTC_H
#define UNIT_NVGPU_LTC_H

#include <nvgpu/types.h>

/** @addtogroup SWUTS-ltc
 *  @{
 *
 * Software Unit Test Specification for ltc
 */

/**
 * Test specification for: test_ltc_init_support
 *
 * Description: The ltc unit get initilized
 *
 * Test Type: Feature based
 *
 * Input: None
 *
 * Steps:
 * - Initialize the test environment for ltc unit testing:
 *   - Setup gv11b register spaces for hals to read emulated values.
 *   - Register read/write IO callbacks.
 *   - Setup init parameters to setup gv11b arch.
 *   - Initialize hal to setup the hal functions.
 * - Call nvgpu_init_ltc_support to initilize ltc unit.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_ltc_init_support(struct unit_module *m,
		struct gk20a *g, void *args);

/**
 * Test specification for: test_ltc_functionality_tests
 *
 * Description: This test test ltc sync enabled and queries data
 * related to different ltc data.
 * Checks whether valid data is retured or not.
 *
 * Test Type: Feature based
 *
 * Input: None
 *
 * Steps:
 * - Set ltc_enabled_current to false and then call
 *   nvgpu_ltc_sync_enabled.
 * - Call nvgpu_ltc_get_ltc_count
 * - Call nvgpu_ltc_get_slices_per_ltc
 * - Call nvgpu_ltc_get_cacheline_size
 * Checked called functions returns correct data.
 *
 * Output: Returns PASS if returned data is valid. FAIL otherwise.
 */

int test_ltc_functionality_tests(struct unit_module *m,
		struct gk20a *g, void *args);

/**
 * Test specification for: test_ltc_negative_tests
 *
 * Description: This test covers negative paths in ltc unit.
 *
 * Test Type: Feature based
 *
 * Input: None
 *
 * Steps:
 * - Set ltc.set_enabled to NULL and then call nvgpu_ltc_sync_enabled
 * - Call nvgpu_ltc_remove_support twice
 * - Call nvgpu_init_ltc_support
 *
 * Output: Returns PASS if expected result is met, FAIL otherwise.
 */
int test_ltc_negative_tests(struct unit_module *m,
		struct gk20a *g, void *args);

/**
 * Test specification for: test_ltc_remove_support
 *
 * Description: The ltc unit removes all populated ltc info.
 *
 * Test Type: Feature based
 *
 * Input: None
 *
 * Steps:
 * - Call nvgpu_ltc_remove_support
 *
 * Output: Returns PASS
 */
int test_ltc_remove_support(struct unit_module *m,
		struct gk20a *g, void *args);

#endif /* UNIT_NVGPU_LTC_H */
