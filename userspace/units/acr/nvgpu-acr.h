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

/** @addtogroup SWUTS-acr
 *  @{
 *
 * Software Unit Test Specification for acr
 */

/**
 * Test specification for: test_acr_init
 *
 * Description: The test_acr_init shall test the initialisation of
 * the ACR unit
 *
 * Test Type: Feature based
 *
 * Input: None
 *
 * Steps:
 * - Initialize the falcon test environment
 * - Initialize the PMU
 * - Inject memory allocation fault to test the fail scenario 1
 * - Give incorrect chip version to test the fail scenario 2
 * - Give correct chip id and test the pass scenario
 * - Uninitialize the PMU support
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_acr_init(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: free_falcon_test_env
 *
 * Description: The free_falcon_test_env shall free up the falcon
 * test environment.
 *
 * Test Type: Feature based
 *
 * Input: None
 *
 * Steps:
 * - Free up the space allocated for utf_flcn (both imem and dmem)
 * - Free up the register space
 *
 * Output: Returns PASS if the steps above were executed successfully.
 *
 */
int free_falcon_test_env(struct unit_module *m, struct gk20a *g, void *__args);
