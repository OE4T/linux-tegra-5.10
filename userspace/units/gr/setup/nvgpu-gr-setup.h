/*
 * Copyright (c) 2019-2020, NVIDIA CORPORATION.  All rights reserved.
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
#ifndef UNIT_NVGPU_GR_SETUP_H
#define UNIT_NVGPU_GR_SETUP_H

#include <nvgpu/types.h>

struct gk20a;
struct unit_module;

/** @addtogroup SWUTS-gr-setup
 *  @{
 *
 * Software Unit Test Specification for common.gr.setup
 */

/**
 * Test specification for: test_gr_setup_alloc_obj_ctx.
 *
 * Description: This test helps to verify common.gr object context creation.
 *
 * Test Type: Feature
 *
 * Targets: #nvgpu_gr_setup_alloc_obj_ctx,
 *          #nvgpu_gr_obj_ctx_alloc,
 *          #nvgpu_gr_ctx_set_tsgid,
 *          #nvgpu_gr_ctx_get_tsgid.
 *
 * Input: #test_gr_init_setup_ready must have been executed successfully.
 *
 * Steps:
 * -  Use stub functions for hals that use timeout and requires register update
 *    within timeout loop.
 *    - g->ops.mm.cache.l2_flush.
 *    - g->ops.gr.init.fe_pwr_mode_force_on.
 *    - g->ops.gr.init.wait_idle.
 *    - g->ops.gr.falcon.ctrl_ctxsw.
 * -  Set default golden image size.
 * -  Allocate and bind channel and tsg.
 * -  Call g->ops.gr.setup.alloc_obj_ctx.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_gr_setup_alloc_obj_ctx(struct unit_module *m,
				 struct gk20a *g, void *args);

/**
 * Test specification for: test_gr_setup_set_preemption_mode.
 *
 * Description: This test helps to verify set_preemption_mode.
 *
 * Test Type: Feature, Safety
 *
 * Targets: #nvgpu_gr_setup_set_preemption_mode,
 *          #nvgpu_gr_obj_ctx_set_ctxsw_preemption_mode,
 *          #nvgpu_gr_obj_ctx_update_ctxsw_preemption_mode,
 *          #nvgpu_gr_ctx_patch_write_begin,
 *          #nvgpu_gr_ctx_patch_write_end,
 *          gp10b_gr_init_commit_global_cb_manager,
 *          #nvgpu_gr_ctx_patch_write.
 *
 * Input: #test_gr_init_setup_ready and #test_gr_setup_alloc_obj_ctx
 *        must have been executed successfully.
 *
 * Steps:
 * -  Call g->ops.gr.setup.set_preemption_mode
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_gr_setup_set_preemption_mode(struct unit_module *m,
			       struct gk20a *g, void *args);

/**
 * Test specification for: test_gr_setup_free_obj_ctx.
 *
 * Description: Helps to verify common.gr object context cleanup.
 *
 * Test Type: Feature
 *
 * Targets: #nvgpu_gr_setup_free_subctx,
 *          #nvgpu_gr_setup_free_gr_ctx.
 *
 * Input: #test_gr_init_setup_ready and #test_gr_setup_alloc_obj_ctx
 *        must have been executed successfully.
 *
 * Steps:
 * -  Call nvgpu_tsg_unbind_channel.
 * -  Call nvgpu_channel_close.
 * -  Call nvgpu_tsg_release.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_gr_setup_free_obj_ctx(struct unit_module *m,
			       struct gk20a *g, void *args);

/**
 * Test specification for: test_gr_setup_preemption_mode_errors.
 *
 * Description: Helps to verify error paths in
 *              g->ops.gr.setup.set_preemption_mode call.
 *
 * Test Type: Error injection, Boundary values
 *
 * Targets: #nvgpu_gr_setup_set_preemption_mode,
 *          #nvgpu_gr_obj_ctx_set_ctxsw_preemption_mode.
 *
 * Input: #test_gr_init_setup_ready and #test_gr_setup_alloc_obj_ctx
 *        must have been executed successfully.
 *
 * Steps:
 * - Verify various combinations of compute and graphics modes.
 * - Verify the error path by failing #nvgpu_preempt_channel.
 * - Verify the error path for NVGPU_INVALID_TSG_ID as ch->tsgid.
 * - Verify the error path for invalid ch->obj_class.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_gr_setup_preemption_mode_errors(struct unit_module *m,
				      struct gk20a *g, void *args);

/**
 * Test specification for: test_gr_setup_alloc_obj_ctx_error_injections.
 *
 * Description: Helps to verify error paths in
 *              g->ops.gr.setup.alloc_obj_ctx call.
 *
 * Test Type: Error injection, Boundary values
 *
 * Targets: #nvgpu_gr_setup_alloc_obj_ctx,
 *          #nvgpu_gr_subctx_alloc, #nvgpu_gr_obj_ctx_alloc,
 *          #nvgpu_gr_obj_ctx_alloc_golden_ctx_image,
 *          #nvgpu_gr_setup_free_subctx,
 *          #nvgpu_gr_setup_free_gr_ctx.
 *
 * Input: #test_gr_init_setup_ready must have been executed successfully.
 *
 * Steps:
 * - Negative Tests for Setup alloc failures
 *   - Test-1 using invalid tsg, classobj and classnum.
 *   - Test-2 error injection in subctx allocation call.
 *   - Test-3 fail nvgpu_gr_obj_ctx_alloc by setting zero image size.
 *   - Test-4  and Test-8 fail nvgpu_gr_obj_ctx_alloc_golden_ctx_image
 *     by failing ctrl_ctsw.
 *   - Test-5 Fail L2 flush for branch coverage.
 *   - Test-6 Fake setup_free call for NULL checking.
 *
 * - Positive Tests
 *   - Test-7 nvgpu_gr_setup_alloc_obj_ctx pass without TSG subcontexts.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_gr_setup_alloc_obj_ctx_error_injections(struct unit_module *m,
						 struct gk20a *g, void *args);
#endif /* UNIT_NVGPU_GR_SETUP_H */

/**
 * @}
 */

