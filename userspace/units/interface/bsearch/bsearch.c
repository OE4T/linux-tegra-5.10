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

#include <unit/io.h>
#include <unit/unit.h>

#include <nvgpu/bsearch.h>
#include <stdlib.h>

/* The test will create a table of size TABLE_SIZE with ordered values from 0
 * to TABLE_SIZE-1. Then the test will bsearch the SEARCH_FOR value.
 * So obviously, this condition must be met: 0 <= SEARCH_FOR < TABLE_SIZE-1
 */
#define TABLE_SIZE 1000
#define SEARCH_FOR 727

u32 num_iterations = 0;

/*
 * Simple comparator function for ints.
 * Returns 0 if values are equal, or the delta if not.
 */
static int int_compare(const void *a, const void *b)
{
	num_iterations++;
	return (*((const int *)a) - *((const int *)b));
}

/*
 * Simple test for bsearch. The goal here is code coverage, the underlying
 * implementation of bsearch is provided by the stdlib.
 */
static int test_bsearch(struct unit_module *m, struct gk20a *g,
					void *args)
{
	int sorted_table[TABLE_SIZE];
	int i;
	int key = SEARCH_FOR;

	num_iterations = 0;

	/* Create a sorted table by having consecutive, incrementing values */
	for (i = 0; i < TABLE_SIZE; i++) {
		sorted_table[i] = i;
	}

	/* Run the binary search */
	int *node_ptr = (int *) nvgpu_bsearch((void *)(&key),
		(void *)&sorted_table, TABLE_SIZE, sizeof(int), int_compare);

	if (node_ptr != NULL) {
		if (*node_ptr == SEARCH_FOR) {
			unit_info(m, "Found correct key after %d iterations\n",
				num_iterations);
			return UNIT_SUCCESS;
		}
		unit_return_fail(m, "Found incorrect value %d\n", *node_ptr);
	}
	unit_return_fail(m, "Key not found in array\n");
}

struct unit_module_test interface_bsearch_tests[] = {
	UNIT_TEST(test_bsearch, test_bsearch, NULL),
};

UNIT_MODULE(interface_bsearch, interface_bsearch_tests, UNIT_PRIO_NVGPU_TEST);
