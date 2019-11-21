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
#ifndef UNIT_NVGPU_GR_SETUP_H
#define UNIT_NVGPU_GR_SETUP_H

#include <nvgpu/types.h>

struct gk20a;
struct unit_module;

/** @addtogroup SWUTS-common-gr-setup
 *  @{
 *
 * Software Unit Test Specification for common.gr.setup
 */

/**
 * Test specification for: test_gr_setup_alloc_obj_ctx.
 *
 * Description: This test helps to verify common.gr object context creation.
 *
 * Test Type: Feature based.
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
 * Test Type: Feature based.
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
 * Test Type: Feature based.
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

#endif /* UNIT_NVGPU_GR_SETUP_H */

/**
 * @}
 */

