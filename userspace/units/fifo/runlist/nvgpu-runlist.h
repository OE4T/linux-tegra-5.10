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
#ifndef UNIT_NVGPU_RUNLIST_H
#define UNIT_NVGPU_RUNLIST_H

#include <nvgpu/types.h>

struct unit_module;
struct gk20a;

/** @addtogroup SWUTS-fifo-runlist
 *  @{
 *
 * Software Unit Test Specification for fifo/runlist */

/**
 * Test specification for: test_tsg_format_gen
 *
 * Description: Test format of TSG runlist entry
 *
 * Test Type: Feature based
 *
 * Input: None
 *
 * Steps:
 * - Check that inserting a single TSG of any level with a number of channels
 *   works as expected:
 *   - priority 0, one channel.
 *   - priority 1, two channels.
 *   - priority 2, five channels.
 *   - priority 0, one channel, nondefault timeslice timeout.
 *   - priority 0, three channels with two inactives in the middle.
 * - Each entry in #tsg_fmt_tests describes one subtest above:
 *   - number of channels to be allocated.
 *   - runlist interleavel level.
 *   - runlist timeslice.
 *   - expected RL entry (header+channels).
 *
 * After allocating channels and binding them to TSG, the TSG is added to
 * runlist by calling nvgpu_runlist_construct_locked.
 * Then entries of the runlist are checked against expected values (as
 * specified by #tsg_fmt_tests).
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_tsg_format_gen(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_flat
 *
 * Description: Build runlist without interleaving (aka "flat")
 *
 * Test Type: Feature based
 *
 * Input: None
 *
 * Steps:
 * - Allocate TSGs, each bound to multiple channels.
 * - Add TSGs to runlist, using pseudo-random priorites for TSGs.
 * - Flat runlist is used: interleave is set to false when calling
 *   nvgpu_runlist_construct_locked.
 * - Check that resulting runlist is ordered according to priorities
 *   (higher priorities first).
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_flat(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_flat_oversize_tiny
 *
 * Description: Only one TSG header can fit the runlist
 *              (not even its channels)
 *
 * Test Type: Feature based
 *
 * Input: None
 *
 * Steps:
 * - Same as #test_flat, except that runlist size
 *   is set to 1 entry, and that failure is expected when
 *   calling nvgpu_runlist_construct_locked.
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_flat_oversize_tiny(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_flat_oversize_single
 *
 * Description: Only one TSG header with its channels fits the runlist
 *
 * Test Type: Feature based
 *
 * Input: None
 *
 * Steps:
 * - Same as #test_flat, except that runlist size
 *   is set to 2 entries, and that failure is expected when
 *   calling nvgpu_runlist_construct_locked.
 * - Check that whatever was inserted in the runlist matches expected TSGs.
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_flat_oversize_single(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_flat_oversize_onehalf
 *
 * Description: Second TSG's channels are chopped off.
 *
 * Test Type: Feature based
 *
 * Input: None
 *
 * Steps:
 * - Same as #test_flat, except that runlist size
 *   is set to 3 entries, and that failure is expected when
 *   calling nvgpu_runlist_construct_locked.
 * - Check that whatever was inserted in the runlist matches expected TSGs.
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_flat_oversize_onehalf(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_flat_oversize_two
 *
 * Description: Two full TSG entries fit exactly
 *
 * Test Type: Feature based
 *
 * Input: None
 *
 * Steps:
 * - Same as #test_flat, except that runlist size
 *   is set to 4 entries, and that failure is expected when
 *   calling nvgpu_runlist_construct_locked.
 * - Check that whatever was inserted in the runlist matches expected TSGs.
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_flat_oversize_two(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_flat_oversize_end
 *
 * Description: All but the last channel entry fit.
 *
 * Test Type: Feature based
 *
 * Input: None
 *
 * Steps:
 * - Same as #test_flat, except that runlist size
 *   is set to (n x TSG - 1) entries, and that failure is expected when
 *   calling nvgpu_runlist_construct_locked.
 * - Check that whatever was inserted in the runlist matches expected TSGs.
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_flat_oversize_end(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_interleaving_gen_all_run
 *
 * Description: Build runlist with interleaving, all levels
 *
 * Test Type: Feature based
 *
 * Input: None
 *
 * Steps:
 *  - Create TSGs and channels, such that the first two TSGs, IDs 0 and 1
 *    (with just one channel each) are at interleave level "low" ("l0"), the
 *    next IDs 2 and 3 are at level "med" ("l1"), and the last IDs 4 and 5
 *    are at level "hi" ("l2"). Runlist construction doesn't care, so we use
 *    an easy to understand order.
 *  - When debugging this test and/or the runlist code, the logs of any
 *    interleave test should follow the order in the "expected" array. We
 *    start at the highest level, so the first IDs added should be h1 and
 *    h2, i.e., 4 and 5, etc.
 *  - TSGs are added to runlists using nvgpu_runlist_construct_locked, with
 *    interleave enabled.
 *  - Expected order of TSGs is stored in an array, and actual runlist is
 *    compared with expected order.
 *  - If runlist is too small to accomodate all TSGs/channels, an error
 *    is expected for nvgpu_runlist_construct_locked, and actual truncated
 *    runlist is compared with first elements of expected array.
 *  - This test runs for levels: l0, l1, l2.
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_interleaving_gen_all_run(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_interleaving_l0
 *
 * Description: Build runlist with interleaving, l0 level only
 *
 * Test Type: Feature based
 *
 * Input: None
 *
 * Steps:
 *  - Same as #test_interleaving_gen_all_run but with only two TSGs
 *    which are at interleave level "low" (l0)
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_interleaving_l0(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_interleaving_l1
 *
 * Description: Build runlist with interleaving, l1 level only
 *
 * Test Type: Feature based
 *
 * Input: None
 *
 * Steps:
 *  - Same as #test_interleaving_gen_all_run but with only two TSGs
 *    which are at interleave level "med" (l1).
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_interleaving_l1(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_interleaving_l2
 *
 * Description: Build runlist with interleaving, l2 level only
 *
 * Test Type: Feature based
 *
 * Input: None
 *
 * Steps:
 *  - Same as #test_interleaving_gen_all_run but with only two TSGs
 *    which are at interleave level "high" (l2)
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_interleaving_l2(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_interleaving_l0_l1
 *
 * Description: Build runlist with interleaving, l0 and l1 levels
 *
 * Test Type: Feature based
 *
 * Input: None
 *
 * Steps:
 *  - Same as #test_interleaving_gen_all_run but with low and medium
 *    interleave level TSGs (l0 and l1)
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_interleaving_l0_l1(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_interleaving_l1_l2
 *
 * Description: Build runlist with interleaving, l1 and l2 levels
 *
 * Test Type: Feature based
 *
 * Input: None
 *
 * Steps:
 *  - Same as #test_interleaving_gen_all_run but with medium and high
 *    interleave level TSGs (l1 and l2)
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_interleaving_l1_l2(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_interleaving_l0_l2
 *
 * Description: Build runlist with interleaving, l0 and l2 levels
 *
 * Test Type: Feature based
 *
 * Input: None
 *
 * Steps:
 *  - Same as #test_interleaving_gen_all_run but with low and high
 *    interleave level TSGs (l0 and l2)
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_interleaving_l0_l2(struct unit_module *m, struct gk20a *g, void *args);

/**
 * @}
 */

#endif /* UNIT_NVGPU_RUNLIST_H */
