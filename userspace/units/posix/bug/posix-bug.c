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

#include <stdlib.h>
#include <stdint.h>

#include <unit/io.h>
#include <unit/unit.h>
#include <nvgpu/bug.h>

#include <nvgpu/types.h>

/*
 * Simple wrapper function to call BUG() or not. It was not strictly necessary
 * to wrap the call to BUG() in a function but it better ressembles the way
 * EXPECT_BUG is to be used in unit tests.
 */
static void bug_caller(struct unit_module *m, bool call)
{
	if (call == true) {
		unit_info(m, "Calling BUG()\n");
		BUG();
	} else {
		unit_info(m, "Not calling BUG()\n");
	}
}

/*
 * Test to ensure the EXPECT_BUG construct works as intended by making sure it
 * behaves properly when BUG is called or not.
 * In the event that EXPECT_BUG is completely broken, the call to BUG() would
 * cause the unit to crash and report a failure correctly.
 */
static int test_expect_bug(struct unit_module *m,
				  struct gk20a *g, void *args)
{

	/* Make sure calls to BUG() are caught as intended. */
	if (!EXPECT_BUG(bug_caller(m, true))) {
		unit_err(m, "BUG() was not called but it was expected.\n");
		return UNIT_FAIL;
	} else {
		unit_info(m, "BUG() was called as expected\n");
	}

	/* Make sure there are no false positives when BUG is not called. */
	if (!EXPECT_BUG(bug_caller(m, false))) {
		unit_info(m, "BUG() was not called as expected.\n");
	} else {
		unit_err(m, "BUG() was called but it was not expected.\n");
		return UNIT_FAIL;
	}

	return UNIT_SUCCESS;
}

struct unit_module_test posix_bug_tests[] = {
	UNIT_TEST(expect_bug, test_expect_bug, NULL, 0),
};

UNIT_MODULE(posix_bug, posix_bug_tests, UNIT_PRIO_POSIX_TEST);
