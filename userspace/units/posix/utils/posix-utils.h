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

/**
 * @addtogroup SWUTS-posix-utils
 * @{
 *
 * Software Unit Test Specification for posix-utils
 */

#ifndef __UNIT_POSIX_UTILS_H__
#define __UNIT_POSIX_UTILS_H__

/**
 * Test specification for test_hamming_weight
 *
 * Description: Test the hamming weight implementation.
 *
 * Test Type: Feature
 *
 * Targets: nvgpu_posix_hweight8, nvgpu_posix_hweight16,
 *          nvgpu_posix_hweight32, nvgpu_posix_hweight64
 *
 * Inputs: None
 *
 * Steps:
 * 1) Call nvgpu_posix_hweight8 in loop with only the loop index bit position
 *    set.
 * 2) Return FAIL if the return value from nvgpu_posix_hweight8 is not equal
 *    to 1 in any of the iterations.
 * 3) Repeat steps 1 and 2 for nvgpu_posix_hweight16, nvgpu_posix_hweight32
 *    and nvgpu_posix_hweight64.
 * 4) Call nvgpu_posix_hweight8 with input parameter set as 0.
 * 5) Return FAIL if the return value from nvgpu_posix_hweight8 is not equal
 *    to 0.
 * 6) Call nvgpu_posix_hweight8 with input parameter set to maximum value.
 * 7) Return FAIL if the return value from nvgpu_posix_hweight8 is not equal
 *    to the number of bits in the input parameter.
 * 8) Repeat steps 4,5,6 and 7 for nvgpu_posix_hweight16, nvgpu_posix_hweight32
 *    and nvgpu_posix_hweight64.
 *
 * Output:
 * The test returns PASS if all the hamming weight function invocations return
 * the expected value. Otherwise the test returns FAIL.
 *
 */
int test_hamming_weight(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for test_be32tocpu
 *
 * Description: Test the endian conversion implementation.
 *
 * Test Type: Feature
 *
 * Targets: be32_to_cpu
 *
 * Inputs: None
 *
 * Steps:
 * 1) Invoke function be32_to_cpu with a fixed pattern as input.
 * 2) Check if the machine is little endian.
 * 3) If the machine is little endian, confirm that the return value from
 *    be32_to_cpu is equal to the little endian order of the pattern, else
 *    return FAIL.
 * 3) EXPECT_BUG is also tested to make sure that BUG is not called where it is
 *    not expected.
 *
 * Output:
 * The test returns PASS if BUG is called as expected based on the parameters
 * passed and EXPECT_BUG handles it accordingly.
 * The test returns FAIL if either BUG is not called as expected or if
 * EXPECT_BUG indicates that a BUG call was made which was not requested by
 * the test.
 *
 */
int test_be32tocpu(struct unit_module *m, struct gk20a *g, void *args);

#endif /* __UNIT_POSIX_UTILS_H__ */
