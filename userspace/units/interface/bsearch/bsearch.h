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

#ifndef UNIT_BSEARCH_H
#define UNIT_BSEARCH_H

struct gk20a;
struct unit_module;

/** @addtogroup SWUTS-interface-bsearch
 *  @{
 *
 * Software Unit Test Specification for interface.bsearch
 */

/**
 * The test will create a table of size TABLE_SIZE with ordered values from 0
 * to TABLE_SIZE-1.
 */
#define TABLE_SIZE 1000

/**
 * The test will bsearch the SEARCH_FOR value inside the test table created
 * with TABLE_SIZE.  So obviously, the following condition must be met:
 * 0 <= SEARCH_FOR < TABLE_SIZE-1
 */
#define SEARCH_FOR 727

/**
 * Test specification for: test_bsearch
 *
 * Description: Simple test for bsearch. The goal here is code coverage, the
 * underlying implementation of bsearch is provided by the stdlib.
 *
 * Test Type: Feature, Coverage
 *
 * Input: None
 *
 * Steps:
 * - Create a table of integers of size TABLE_SIZE.
 * - Fill the table with incrementing values starting at 0, so that the table
 *   is sorted.
 * - Run a binary search on the table by calling nvgpu_bsearch, looking for a
 *   known value (SEARCH_FOR) in the sorted table using integer comparisons.
 * - Ensure that the correct value was found in the table.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_bsearch(struct unit_module *m, struct gk20a *g, void *args);

/** }@ */
#endif /* UNIT_BSEARCH_H */
