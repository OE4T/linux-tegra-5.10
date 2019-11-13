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
 * Test specification for: test_acr_prepare_ucode_blob
 *
 * Description: The test_acr_prepare_ucode_blob shall test the blob creation of
 * the ACR unit
 *
 * Test Type: Feature based
 *
 * Input: None
 * Steps:
 * - Initialize the falcon test environment
 * - Set the flag NVGPU_SEC_SECUREGPCCS
 * - Allocate memory for GR
 * - Initialize the PMU
 * - Initialize the ACR unit
 * - Initialize the MMU
 * - Prepare SW and HW for GR
 * - Prepare ucode BLOB
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */

int test_acr_prepare_ucode_blob(struct unit_module *m, struct gk20a *g,
					void *__args);
/**
 * Test specification for: test_acr_is_lsf_lazy_bootstrap
 *
 * Description: The test_acr_is_lsf_lazy_bootstrap shall test the
 * lazy bootstrap of the ACR unit
 *
 * Test Type: Feature based
 *
 * Input: None
 *
 * Steps:
 * - Initialize the falcon test environment
 * - Set the flag NVGPU_SEC_SECUREGPCCS
 * - Allocate memory for GR
 * - Initialize the PMU
 * - Initialize the ACR unit
 * - Initialize the MMU
 * - Prepare SW and HW for GR
 * - lsf lazy bootstrap
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */

int test_acr_is_lsf_lazy_bootstrap(struct unit_module *m, struct gk20a *g,
					void *__args);
/**
 * Test specification for: test_acr_construct_execute
 *
 * Description: The test_acr_construct_execute shall test the two main tasks of
 * the ACR unit:
 * 1. Blob construct of LS ucode in non-wpr memory
 * 2. ACR HS ucode load & bootstrap
 *
 * Test Type: Feature based
 *
 * Input: None
 *
 * Steps:
 * - Initialize the falcon test environment
 * - Set the flag NVGPU_SEC_SECUREGPCCS
 * - Allocate memory for GR
 * - Initialize the PMU
 * - Initialize the ACR unit
 * - Initialize the MMU
 * - Prepare SW and HW for GR
 * - Set the falcon_falcon_cpuctl_halt_intr_m bit for the
 *   register falcon_falcon_cpuctl_r
 * - Call nvgpu_acr_construct_execute() via ACR sw ops.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */

int test_acr_construct_execute(struct unit_module *m,
					struct gk20a *g, void *args);
/**
 * Test specification for: test_acr_bootstrap_hs_acr
 *
 * Description: The test_acr_bootstrap_hs_acr shall test the ACR HS ucode load
 * & bootstrap functionality of the ACR unit
 *
 * Test Type: Feature based
 *
 * Input: None
 *
 * Steps:
 * - Initialize the falcon test environment
 * - Set the flag NVGPU_SEC_SECUREGPCCS
 * - Allocate memory for GR
 * - Initialize the PMU
 * - Initialize the ACR unit
 * - Initialize the MMU
 * - Prepare SW and HW for GR
 * - Set the falcon_falcon_cpuctl_halt_intr_m bit for the
 *   register falcon_falcon_cpuctl_r
 * - Prepare the ucode blob
 * - Call nvgpu_acr_bootstrap_hs_acr() twice to cover recovery branch.
 * - Create fail/negative scenario of nvgpu_acr_bootstrap_hs_acr()
     by passing g->acr = NULL. nvgpu_acr_bootstrap_hs_acr() should fail.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */

int test_acr_bootstrap_hs_acr(struct unit_module *m,
					struct gk20a *g, void *args);

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
