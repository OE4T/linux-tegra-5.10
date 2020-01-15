/*
 * Copyright (c) 2020, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef UNIT_STATIC_ANALYSIS_H
#define UNIT_STATIC_ANALYSIS_H

struct gk20a;
struct unit_module;

/** @addtogroup SWUTS-interface-static-analysis
 *  @{
 *
 * Software Unit Test Specification for static analysis unit
 */

/**
 * Test specification for: test_arithmetic
 *
 * Description: Verify functionality of static analysis safe arithmetic APIs.
 *
 * Test Type: Feature, Error guessing
 *
 * Targets: nvgpu_safe_sub_u8, nvgpu_safe_add_u32, nvgpu_safe_add_s32,
 *          nvgpu_safe_sub_u32, nvgpu_safe_sub_s32, nvgpu_safe_mult_u32,
 *          nvgpu_safe_add_u64, nvgpu_safe_add_s64, nvgpu_safe_sub_u64,
 *          nvgpu_safe_sub_s64, nvgpu_safe_mult_u64, nvgpu_safe_mult_s64
 *
 * Input: None
 *
 * Steps:
 * - Call the static analysis arithmetic APIs. Pass in valid values and verify
 *   correct return.
 * - Call the static analysis arithmetic APIs. Pass in values beyond type range
 *   and use EXPECT_BUG() to verify BUG() is called.
 *
 * Output: Returns PASS if expected result is met, FAIL otherwise.
 */
int test_arithmetic(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_cast
 *
 * Description: Verify functionality of static analysis safe cast APIs.
 *
 * Test Type: Feature, Error guessing
 *
 * Targets: nvgpu_safe_cast_u64_to_u32, nvgpu_safe_cast_u64_to_u16,
 *          nvgpu_safe_cast_u64_to_u8, nvgpu_safe_cast_u64_to_s64,
 *          nvgpu_safe_cast_u64_to_s32, nvgpu_safe_cast_s64_to_u64,
 *          nvgpu_safe_cast_s64_to_u32, nvgpu_safe_cast_s64_to_s32,
 *          nvgpu_safe_cast_u32_to_u16, nvgpu_safe_cast_u32_to_u8,
 *          nvgpu_safe_cast_u32_to_s32, nvgpu_safe_cast_u32_to_s8,
 *          nvgpu_safe_cast_s32_to_u64, nvgpu_safe_cast_s32_to_u32,
 *          nvgpu_safe_cast_s8_to_u8, nvgpu_safe_cast_bool_to_u32
 *
 * Input: None
 *
 * Steps:
 * - Call the static analysis arithmetic APIs. Pass in valid values and verify
 *   correct return.
 * - Call the static analysis arithmetic APIs. Pass in values beyond type range
 *   and use EXPECT_BUG() to verify BUG() is called.
 *
 * Output: Returns PASS if expected result is met, FAIL otherwise.
 */
int test_cast(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_safety_checks
 *
 * Description: Verify functionality of static analysis safety_check() API.
 *
 * Test Type: Feature
 *
 * Targets: nvgpu_safety_checks
 *
 * Input: None
 *
 * Steps:
 * - Call the API nvgpu_safety_checks(). No error should occur.
 *
 * Output: Returns PASS if expected result is met, FAIL otherwise.
 */
int test_safety_checks(struct unit_module *m, struct gk20a *g, void *args);

/**
 * @}
 */

#endif /* UNIT_STATIC_ANALYSIS_H */
