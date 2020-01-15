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

#ifndef UNIT_NVGPU_PTIMER_H
#define UNIT_NVGPU_PTIMER_H

struct gk20a;
struct unit_module;

/** @addtogroup SWUTS-ptimer
 *  @{
 *
 * Software Unit Test Specification for nvgpu.common.ptimer
 */

/**
 * Test specification for: test_setup_env
 *
 * Description: Setup prerequisites for tests.
 *
 * Test Type: Other (setup)
 *
 * Input: None
 *
 * Steps:
 * - Setup ptimer HAL function pointers.
 * - Setup timer reg space in mockio.
 *
 * Output:
 * - UNIT_FAIL if encounters an error creating reg space
 * - UNIT_SUCCESS otherwise
 */
int test_setup_env(struct unit_module *m,
			struct gk20a *g, void *args);

/**
 * Test specification for: test_free_env
 *
 * Description: Release resources from test_setup_env()
 *
 * Test Type: Other (setup)
 *
 * Input: test_setup_env() has been executed.
 *
 * Steps:
 * - Delete ptimer register space from mockio.
 *
 * Output:
 * - UNIT_FAIL if encounters an error creating reg space
 * - UNIT_SUCCESS otherwise
 */
int test_free_env(struct unit_module *m,
			struct gk20a *g, void *args);

/**
 * Test specification for: test_read_ptimer
 *
 * Description: Verify the read_ptimer API.
 *
 * Test Type: Feature Based
 *
 * Targets: gops_ptimer.read_ptimer, gk20a_read_ptimer
 *
 * Input: None
 *
 * Steps:
 * - Test case where the ptimer time values do not wrap.
 *   - Write values to ptimer regs timer_time_0 and timer_time_1 in mockio
 *     register space.
 *   - Call read_timer API.
 *   - Verify the expected value is returned.
 * - Test case where ptimer time values wrap once.
 *   - Configure mockio so that the timer_time_1 register reads a different
 *     value after the 1st read, but is consistent after 2nd read.
 *   - Call read_timer API.
 *   - Verify the expected value is returned.
  * - Test case where ptimer time values wrap once.
 *   - Configure mockio so that the timer_time_1 register reads a different
 *     value for up to 4 reads.
 *   - Call read_timer API.
 *   - Verify API returns an error.
 * - Test parameter checking of the API
 *   - Call read_timer API with a NULL pointer for the time parameter.
 *   - Verify API returns an error.
 *
 * Output:
 * - UNIT_FAIL if encounters an error creating reg space
 * - UNIT_SUCCESS otherwise
 */
int test_read_ptimer(struct unit_module *m,
			struct gk20a *g, void *args);

/**
 * Test specification for: test_ptimer_isr
 *
 * Description: Verify the ptimer isr API. The ISR only logs the errors and
 *              clears the ISR regs. This test verifies the code paths do not
 *              cause errors.
 *
 * Test Type: Feature Based
 *
 * Targets: gops_ptimer.isr, gk20a_ptimer_isr
 *
 * Input: None
 *
 * Steps:
 * - Test isr with 0 register values.
 *   - Initialize registers to 0: pri_timeout_save_0, pri_timeout_save_1,
 *     pri_timeout_fecs_errcode.
 *   - Call isr API.
 *   - Verify the save_* regs were all set to 0.
 * - Test with FECS bits set.
 *   - Set the fecs bit in the pri_timeout_save_0 reg and an error code in the
 *     pri_timeout_fecs_errcode reg.
 *   - Call isr API.
 *   - Verify the save_* regs were all set to 0.
 * - Test with FECS bits set and verify priv_ring decode error HAL is invoked.
 *   - Set the fecs bit in the pri_timeout_save_0 reg and an error code in the
 *     pri_timeout_fecs_errcode reg.
 *   - Set the HAL priv_ring.decode_error_code to a mock function.
 *   - Call isr API.
 *   - Verify the fecs error code was passed to the decode_error_code mock
 *     function.
 *   - Verify the save_* regs were all set to 0.
 * - Test branch for save0 timeout bit being set.
 *   - Set the timeout bit in the pri_timeout_save_0 reg.
 *   - Call isr API.
 *   - Verify the save_* regs were all set to 0.
 *
 * Output:
 * - UNIT_FAIL if encounters an error creating reg space
 * - UNIT_SUCCESS otherwise
 */
int test_ptimer_isr(struct unit_module *m,
			struct gk20a *g, void *args);

/**
 * Test specification for: test_ptimer_scaling
 *
 * Description: Verify the scale_ptimer() and ptimer_scalingfactor10x() APIs.
 *
 * Test Type: Feature Based, Boundary Values
 *
 * Targets: scale_ptimer, ptimer_scalingfactor10x
 *
 * Input: None
 *
 * Steps:
 * - Call the scale_ptimer() API with various input values and verify the
 *   returned value.
 * - Call the ptimer_scalingfactor10x() API with various input values and verify
 *   the returned value.
 *
 * Output:
 * - UNIT_FAIL if encounters an error creating reg space
 * - UNIT_SUCCESS otherwise
 */
int test_ptimer_scaling(struct unit_module *m,
			struct gk20a *g, void *args);

#endif /* UNIT_NVGPU_PTIMER_H */
