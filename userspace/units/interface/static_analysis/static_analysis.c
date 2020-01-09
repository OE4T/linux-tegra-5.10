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

#include <unit/unit.h>
#include <unit/io.h>
#include <nvgpu/posix/io.h>

#include <nvgpu/gk20a.h>
#include <nvgpu/static_analysis.h>

#include "static_analysis.h"

int test_arithmetic(struct unit_module *m, struct gk20a *g, void *args)
{
	int err;

	/* U8 sub */
	unit_assert(nvgpu_safe_sub_u8(1, 1) == 0, return UNIT_FAIL);
	unit_assert(nvgpu_safe_sub_u8(U8_MAX, U8_MAX) == 0,
							return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_sub_u8(0, 1));
	unit_assert(err != 0, return UNIT_FAIL);

	/* U32 add */
	unit_assert(nvgpu_safe_add_u32(U32_MAX-1, 1) == U32_MAX,
							return UNIT_FAIL);
	unit_assert(nvgpu_safe_add_u32(U32_MAX, 0) == U32_MAX,
							return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_add_u32(U32_MAX, 1));
	unit_assert(err != 0, return UNIT_FAIL);

	/* S32 add */
	unit_assert(nvgpu_safe_add_s32(INT_MAX-1, 1) == INT_MAX,
							return UNIT_FAIL);
	unit_assert(nvgpu_safe_add_s32(INT_MAX, 0) == INT_MAX,
							return UNIT_FAIL);
	unit_assert(nvgpu_safe_add_s32(INT_MIN+1, -1) == INT_MIN,
							return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_add_s32(INT_MAX, 1));
	unit_assert(err != 0, return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_add_s32(INT_MIN, -1));
	unit_assert(err != 0, return UNIT_FAIL);

	/* U32 sub */
	unit_assert(nvgpu_safe_sub_u32(1, 1) == 0, return UNIT_FAIL);
	unit_assert(nvgpu_safe_sub_u32(U32_MAX, U32_MAX) == 0,
							return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_sub_u32(0, 1));
	unit_assert(err != 0, return UNIT_FAIL);

	/* S32 sub */
	unit_assert(nvgpu_safe_sub_s32(INT_MIN+1, 1) == INT_MIN,
							return UNIT_FAIL);
	unit_assert(nvgpu_safe_sub_s32(INT_MAX-1, -1) == INT_MAX,
							return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_sub_s32(INT_MIN, 1));
	unit_assert(err != 0, return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_sub_s32(INT_MAX, -1));
	unit_assert(err != 0, return UNIT_FAIL);

	/* U32 Mult */
	unit_assert(nvgpu_safe_mult_u32(0, 0) == 0,
							return UNIT_FAIL);
	unit_assert(nvgpu_safe_mult_u32(U32_MAX, 0) == 0,
							return UNIT_FAIL);
	unit_assert(nvgpu_safe_mult_u32(U32_MAX, 1) == U32_MAX,
							return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_mult_u32(U32_MAX, 2));
	unit_assert(err != 0, return UNIT_FAIL);

	/* U64 add */
	unit_assert(nvgpu_safe_add_u64(U64_MAX-1, 1) == U64_MAX,
							return UNIT_FAIL);
	unit_assert(nvgpu_safe_add_u64(U64_MAX, 0) == U64_MAX,
							return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_add_u64(U64_MAX, 1));
	unit_assert(err != 0, return UNIT_FAIL);

	/* S64 add */
	unit_assert(nvgpu_safe_add_s64(LONG_MAX-1, 1) == LONG_MAX,
							return UNIT_FAIL);
	unit_assert(nvgpu_safe_add_s64(LONG_MAX, 0) == LONG_MAX,
							return UNIT_FAIL);
	unit_assert(nvgpu_safe_add_s64(LONG_MIN+1, -1) == LONG_MIN,
							return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_add_s64(LONG_MAX, 1));
	unit_assert(err != 0, return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_add_s64(LONG_MIN, -1));
	unit_assert(err != 0, return UNIT_FAIL);

	/* U64 sub */
	unit_assert(nvgpu_safe_sub_u64(1, 1) == 0, return UNIT_FAIL);
	unit_assert(nvgpu_safe_sub_u64(U64_MAX, U64_MAX) == 0,
							return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_sub_u64(0, 1));
	unit_assert(err != 0, return UNIT_FAIL);

	/* S64 sub */
	unit_assert(nvgpu_safe_sub_s64(LONG_MIN+1, 1) == LONG_MIN,
							return UNIT_FAIL);
	unit_assert(nvgpu_safe_sub_s64(LONG_MAX-1, -1) == LONG_MAX,
							return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_sub_s64(LONG_MIN, 1));
	unit_assert(err != 0, return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_sub_s64(LONG_MAX, -1));
	unit_assert(err != 0, return UNIT_FAIL);

	/* U64 Mult */
	unit_assert(nvgpu_safe_mult_u64(0, 0) == 0,
							return UNIT_FAIL);
	unit_assert(nvgpu_safe_mult_u64(U64_MAX, 0) == 0,
							return UNIT_FAIL);
	unit_assert(nvgpu_safe_mult_u64(U64_MAX, 1) == U64_MAX,
							return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_mult_u64(U64_MAX, 2));
	unit_assert(err != 0, return UNIT_FAIL);

	/* S64 Mult */
	unit_assert(nvgpu_safe_mult_s64(LONG_MAX, 1) == LONG_MAX,
							return UNIT_FAIL);
	unit_assert(nvgpu_safe_mult_s64(LONG_MAX, -1) == (-1 * LONG_MAX),
							return UNIT_FAIL);
	unit_assert(nvgpu_safe_mult_s64(-1, LONG_MAX) == (-1 * LONG_MAX),
							return UNIT_FAIL);
	unit_assert(nvgpu_safe_mult_s64(0, LONG_MAX) == 0,
							return UNIT_FAIL);
	unit_assert(nvgpu_safe_mult_s64(-1, -1) == 1,
							return UNIT_FAIL);
	unit_assert(nvgpu_safe_mult_s64(0, -1) == 0,
							return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_mult_s64(LONG_MAX, 2));
	unit_assert(err != 0, return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_mult_s64(LONG_MAX, -2));
	unit_assert(err != 0, return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_mult_s64(-1*LONG_MAX, 2));
	unit_assert(err != 0, return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_mult_s64(-1*LONG_MAX, -2));
	unit_assert(err != 0, return UNIT_FAIL);

	return UNIT_SUCCESS;
}

int test_cast(struct unit_module *m, struct gk20a *g, void *args)
{
	int err;

	/* U64 to U32 */
	unit_assert(nvgpu_safe_cast_u64_to_u32(U32_MAX) == U32_MAX,
							return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_cast_u64_to_u32(U64(U32_MAX)+1));
	unit_assert(err != 0, return UNIT_FAIL);

	/* U64 to U16 */
	unit_assert(nvgpu_safe_cast_u64_to_u16(U16_MAX) == U16_MAX,
							return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_cast_u64_to_u16(U64(U16_MAX)+1));
	unit_assert(err != 0, return UNIT_FAIL);

	/* U64 to U8 */
	unit_assert(nvgpu_safe_cast_u64_to_u8(U8_MAX) == U8_MAX,
							return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_cast_u64_to_u8(U64(U8_MAX)+1));
	unit_assert(err != 0, return UNIT_FAIL);

	/* U64 to S64 */
	unit_assert(nvgpu_safe_cast_u64_to_s64(LONG_MAX) == LONG_MAX,
							return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_cast_u64_to_s64(U64(LONG_MAX)+1));
	unit_assert(err != 0, return UNIT_FAIL);

	/* U64 to S32 */
	unit_assert(nvgpu_safe_cast_u64_to_s32(INT_MAX) == INT_MAX,
							return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_cast_u64_to_s32(U64(INT_MAX)+1));
	unit_assert(err != 0, return UNIT_FAIL);

	/* S64 to U64 */
	unit_assert(nvgpu_safe_cast_s64_to_u64(LONG_MAX) == LONG_MAX,
							return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_cast_s64_to_u64(-1));
	unit_assert(err != 0, return UNIT_FAIL);

	/* S64 to U32 */
	unit_assert(nvgpu_safe_cast_s64_to_u32(U32_MAX) == U32_MAX,
							return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_cast_s64_to_u32(-1));
	unit_assert(err != 0, return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_cast_s64_to_u32(U64(U32_MAX)+1));
	unit_assert(err != 0, return UNIT_FAIL);

	/* S64 to S32 */
	unit_assert(nvgpu_safe_cast_s64_to_s32(INT_MAX) == INT_MAX,
							return UNIT_FAIL);
	unit_assert(nvgpu_safe_cast_s64_to_s32(INT_MIN) == INT_MIN,
							return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_cast_s64_to_s32(S64(INT_MIN)-1));
	unit_assert(err != 0, return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_cast_s64_to_s32(S64(INT_MAX)+1));
	unit_assert(err != 0, return UNIT_FAIL);

	/* U32 to U16 */
	unit_assert(nvgpu_safe_cast_u32_to_u16(U16_MAX) == U16_MAX,
							return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_cast_u32_to_u16(U64(U16_MAX)+1));
	unit_assert(err != 0, return UNIT_FAIL);

	/* U32 to U8 */
	unit_assert(nvgpu_safe_cast_u32_to_u8(U8_MAX) == U8_MAX,
							return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_cast_u32_to_u8(U64(U8_MAX)+1));
	unit_assert(err != 0, return UNIT_FAIL);

	/* U32 to S32 */
	unit_assert(nvgpu_safe_cast_u32_to_s32(INT_MAX) == INT_MAX,
							return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_cast_u32_to_s32(U32(INT_MAX)+1));
	unit_assert(err != 0, return UNIT_FAIL);

	/* U32 to S8 */
	unit_assert(nvgpu_safe_cast_u32_to_s8(SCHAR_MAX) == SCHAR_MAX,
							return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_cast_u32_to_s8(U32(SCHAR_MAX)+1));
	unit_assert(err != 0, return UNIT_FAIL);

	/* S32 to U64 */
	unit_assert(nvgpu_safe_cast_s32_to_u64(INT_MAX) == INT_MAX,
							return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_cast_s32_to_u64(-1));
	unit_assert(err != 0, return UNIT_FAIL);

	/* S32 to U32 */
	unit_assert(nvgpu_safe_cast_s32_to_u32(INT_MAX) == INT_MAX,
							return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_cast_s32_to_u32(-1));
	unit_assert(err != 0, return UNIT_FAIL);

	/* S8 to U8 */
	unit_assert(nvgpu_safe_cast_s8_to_u8(SCHAR_MAX) == SCHAR_MAX,
							return UNIT_FAIL);
	err = EXPECT_BUG((void)nvgpu_safe_cast_s8_to_u8(-1));
	unit_assert(err != 0, return UNIT_FAIL);

	/* bool to U32 */
	unit_assert(nvgpu_safe_cast_bool_to_u32(false) == 0, return UNIT_FAIL);
	unit_assert(nvgpu_safe_cast_bool_to_u32(true) == 1, return UNIT_FAIL);

	return  UNIT_SUCCESS;
}

struct unit_module_test static_analysis_tests[] = {
	UNIT_TEST(arithmetic,		test_arithmetic,	NULL, 0),
	UNIT_TEST(cast,			test_cast,		NULL, 0),
};

UNIT_MODULE(static_analysis, static_analysis_tests, UNIT_PRIO_NVGPU_TEST);