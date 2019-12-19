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
#ifndef UNIT_NVGPU_GR_OBJ_CTX_H
#define UNIT_NVGPU_GR_OBJ_CTX_H

#include <nvgpu/types.h>

struct gk20a;
struct unit_module;

/** @addtogroup SWUTS-gr-obj-ctx
 *  @{
 *
 * Software Unit Test Specification for common.gr.obj_ctx
 */

/**
 * Test specification for: test_gr_obj_ctx_error_injection.
 *
 * Description: Verify error handling in object context creation path.
 *
 * Test Type: Feature, Error guessing
 *
 * Targets: #nvgpu_gr_obj_ctx_init,
 *          #nvgpu_gr_obj_ctx_alloc,
 *          #nvgpu_gr_obj_ctx_is_golden_image_ready
 *          #nvgpu_gr_obj_ctx_deinit,
 *          #nvgpu_gr_obj_ctx_set_ctxsw_preemption_mode.
 *
 * Input: gr_obj_ctx_setup must have been executed successfully.
 *
 * Steps:
 * - Inject memory allocation failures and call #nvgpu_gr_obj_ctx_init,
 *   should fail.
 * - Disable error injection and call #nvgpu_gr_obj_ctx_init, should pass.
 * - Initialize VM, instance block, global context buffers, subcontext
 *   which are needed to allocate object context.
 * - Inject errors for gr_ctx and patch_ctx allocation,
 *   #nvgpu_gr_obj_ctx_alloc should fail.
 * - Inject errors to fail global context buffer mapping,
 *   #nvgpu_gr_obj_ctx_alloc should fail.
 * - Replace existing HALs with dummy ones to return errors,
 *   #nvgpu_gr_obj_ctx_alloc should fail in each case.
 * - Inject error to fail golden context verification,
 *   #nvgpu_gr_obj_ctx_alloc should fail.
 * - Disable all error injection and #nvgpu_gr_obj_ctx_alloc should pass.
 * - Check if golden image is ready with
 *   #nvgpu_gr_obj_ctx_is_golden_image_ready.
 * - Call #nvgpu_gr_obj_ctx_alloc again and ensure no error is return.
 * - Call #nvgpu_gr_obj_ctx_set_ctxsw_preemption_mode with incorrect
 *   compute class and ensure it returns error.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_gr_obj_ctx_error_injection(struct unit_module *m,
		struct gk20a *g, void *args);

#endif /* UNIT_NVGPU_GR_OBJ_CTX_H */

/**
 * @}
 */

