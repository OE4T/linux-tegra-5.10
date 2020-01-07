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
#ifndef UNIT_NVGPU_GR_H
#define UNIT_NVGPU_GR_H

#include <nvgpu/types.h>

struct gk20a;
struct unit_module;

/** @addtogroup SWUTS-gr
 *  @{
 *
 * Software Unit Test Specification for common.gr
 */

/**
 * Test specification for: test_gr_init_setup.
 *
 * Description: Setup common.gr unit.
 *
 * Test Type: Feature
 *
 * Input: None.
 *
 * Targets: #nvgpu_gr_alloc.
 *
 * Steps:
 * -  Initialize the test environment for common.gr unit testing:
 * -  Setup gv11b register spaces for hals to read emulated values.
 * -  Register read/write IO callbacks.
 * -  Setup init parameters to setup gv11b arch.
 * -  Initialize hal to setup the hal functions.
 * -  Call nvgpu_gr_alloc to allocate common.gr unit struct.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 *         otherwise.
 */
int test_gr_init_setup(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_gr_remove_setup.
 *
 * Description: Remove common.gr unit setup.
 *
 * Test Type: Feature
 *
 * Targets: #nvgpu_gr_free.
 *
 * Input: test_gr_init_setup must have been executed successfully.
 *
 * Steps:
 * -  Delete and remove the gv11b register spaces.
 * -  Delete the memory for common.gr unit.
 *
 * Output: Returns PASS.
 */
int test_gr_remove_setup(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_gr_init_prepare.
 *
 * Description: Prepare common.gr unit.
 *
 * Test Type: Feature
 *
 * Targets: #nvgpu_gr_prepare_sw, #nvgpu_gr_prepare_hw.
 *
 * Input: test_gr_init_setup must have been executed successfully.
 *
 * Steps:
 * -  Call nvgpu_gr_prepare_sw and nvgpu_gr_enable_hw which helps
 *    to initialize the s/w and enable h/w for GR engine.
 *
 * Output: Returns PASS.
 */
int test_gr_init_prepare(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_gr_init_support.
 *
 * Description: Initialize common.gr unit.
 *
 * Test Type: Feature
 *
 * Targets: #nvgpu_gr_init_support.
 *
 * Input: test_gr_init_setup and test_gr_init_prepare
 *        must have been executed successfully.
 *
 * Steps:
 * -  Call nvgpu_gr_init.
 * -  Call g->ops.gr.ecc.ecc_init_support.
 * -  Call g->ops.ltc.init_ltc_support & g->ops.mm.init_mm_support.
 * -  Override g->ops.gr.falcon.load_ctxsw_ucode function.
 * -  Call g->ops.chip_init_gpu_characteristics.
 * -  Call nvgpu_gr_init_support.
 * -  Call g->ops.gr.ecc.ecc_finalize_support.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 *         otherwise.
 */
int test_gr_init_support(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_gr_suspend.
 *
 * Description: Suspend common.gr unit.
 *
 * Test Type: Feature
 *
 * Targets: #nvgpu_gr_suspend.
 *
 * Input: #test_gr_init_setup, #test_gr_init_prepare and #test_gr_init_support
 *        must have been executed successfully.
 *
 * Steps:
 * -  Call nvgpu_gr_suspend.
 *
 * Output: Returns PASS.
 */
int test_gr_suspend(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_gr_remove_support.
 *
 * Description: Remove common.gr unit support.
 *
 * Test Type: Feature
 *
 * Targets: #nvgpu_gr_remove_support.
 *
 * Input: #test_gr_init_setup, #test_gr_init_prepare and #test_gr_init_support
 *        must have been executed successfully.
 *
 * Steps:
 * -  Call g->ops.ecc.ecc_remove_support.
 * -  Call nvgpu_gr_remove_support.
 *
 * Output: Returns PASS.
 */
int test_gr_remove_support(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_gr_init_ecc_features.
 *
 * Description: Set the ECC feature based on fuse and fecs override registers.
 *
 * Test Type: Feature, Error Injection
 *
 * Input: #test_gr_init_setup, #test_gr_init_prepare and #test_gr_init_support
 *        must have been executed successfully.
 *
 * Targets: gv11b_gr_gpc_tpc_ecc_init, gv11b_gr_fecs_ecc_init and
 *	    gv11b_ecc_detect_enabled_units.
 *
 * Steps:
 * -  Array with various combinations setting register bits for
 *    FUSES_OVERRIDE_DISABLE, OPT_ECC_ENABLE, fecs register for ecc
 *    and ecc1 overrides.
 * -  Call g->ops.gr.ecc.detect.
 * -  Error injection for allocation and other conditional checking
 *    in g->ops.gr.ecc.init call.
 *
 * Output: Returns PASS.
 */
int test_gr_init_ecc_features(struct unit_module *m,
			      struct gk20a *g, void *args);

/**
 * Test specification for: test_gr_init_setup_ready.
 *
 * Description: Setup for common.gr unit.
 *
 * Test Type: Feature
 *
 * Input: None
 *
 * Targets: #nvgpu_gr_prepare_sw, #nvgpu_gr_prepare_hw,
 *          and #nvgpu_gr_init_support.
 *
 * Steps:
 * -  Call #test_gr_init_setup.
 *    -  Setup gv11b arch and allocate struct for common.gr.
 * -  Call #test_gr_init_prepare.
 *    -  To initialize the s/w and enable h/w for GR engine.
 * -  Call #test_gr_init_support.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_gr_init_setup_ready(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_gr_init_error_injections.
 *
 * Description: Negative test for common.gr init unit.
 *
 * Test Type: Feature, Error Injection
 *
 * Input: #test_gr_setup_ready must have been executed successfully.
 *
 * Targets: #nvgpu_gr_init_support, #nvgpu_gr_prepare_sw, gr_remove_support.
 *
 * Steps:
 * -  Add various condition to cause failure in #nvgpu_gr_init_support.
 *    This includes failing of #nvgpu_gr_falcon_init_ctxsw, #nvgpu_gr_init_ctx_state,
 *    gr_init_setup_sw and gr_init_setup_hw functions.
 * -  Add various condition to cause failure in #nvgpu_gr_prepare_sw.
 *    This includes failing of #nvgpu_netlist_init_ctx_vars, #nvgpu_gr_falcon_init_support,
 *    #nvgpu_gr_intr_init_support and g->ops.gr.ecc.fecs_ecc_init functions.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_gr_init_error_injections(struct unit_module *m,
				  struct gk20a *g, void *args);

/**
 * Test specification for: test_gr_setup_cleanup.
 *
 * Description: Cleanup common.gr unit.
 *
 * Test Type: Feature
 *
 * Input: #test_gr_setup_ready must have been executed successfully.
 *
 * Targets: #nvgpu_gr_free, #nvgpu_gr_remove_support.
 *
 * Steps:
 * -  Call #test_gr_remove_support.
 * -  Call #test_gr_remove_setup.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_gr_init_setup_cleanup(struct unit_module *m, struct gk20a *g, void *args);

#endif /* UNIT_NVGPU_GR_H */

/**
 * @}
 */
