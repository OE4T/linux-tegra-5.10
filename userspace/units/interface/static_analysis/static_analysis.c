/*
 * Copyright (c) 2020-2021, NVIDIA CORPORATION.  All rights reserved.
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

/**
 * unsigned addition tests
 *
 * parameters:
 *   type_name: type (u8, u32 etc.)
 *   type_max : Maximum value of the type.
 *   tmp_operand: random value in the set (1, Maximum value)
 *
 * Boundary values: (0, 1, max-1, max)
 *
 * Valid tests: Addition result within range for each boundary value and
 *              random value.
 * Invalid tests: Addition result out of range if possible for each
 *                boundary and random value.
 */
#define GENERATE_ARITHMETIC_ADD_TESTS(type_name, type_max, tmp_operand) do {\
	unit_assert(nvgpu_safe_add_##type_name(type_max, 0) == type_max, return UNIT_FAIL); \
	unit_assert(nvgpu_safe_add_##type_name(type_max - 1, 1) == type_max, return UNIT_FAIL); \
	unit_assert(nvgpu_safe_add_##type_name(type_max - tmp_operand, tmp_operand) == type_max, \
							return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_add_##type_name(1, type_max)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_add_##type_name(tmp_operand, type_max - tmp_operand + 1)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_add_##type_name(type_max - 1, tmp_operand > 1 ? tmp_operand : 2)); \
	unit_assert(err != 0, return UNIT_FAIL); \
} while (0)

/**
 * wrapping unsigned addition tests
 *
 * parameters:
 *   type_name: type (u8, u32 etc.)
 *   type_max : Maximum value of the type.
 *   tmp_operand: random value in the set (1, Maximum value)
 *
 * Boundary values: (0, 1, max-1, max)
 *
 * Valid tests: Addition result within range for each boundary value and
 *              random value. Addition result wrapping for each boundary
 *              and random value.
 */
#define GENERATE_ARITHMETIC_WRAPPING_ADD_TESTS(type_name, type_max, tmp_operand) do {\
	unit_assert(nvgpu_wrapping_add_##type_name(type_max, 0) == type_max, return UNIT_FAIL); \
	unit_assert(nvgpu_wrapping_add_##type_name(type_max - 1, 1) == type_max, return UNIT_FAIL); \
	unit_assert(nvgpu_wrapping_add_##type_name(type_max - tmp_operand, tmp_operand) == type_max, \
							return UNIT_FAIL); \
	unit_assert(nvgpu_wrapping_add_##type_name(1, type_max) == 0, return UNIT_FAIL); \
	unit_assert(nvgpu_wrapping_add_##type_name(tmp_operand, type_max - tmp_operand + 1) == 0, return UNIT_FAIL); \
	unit_assert(nvgpu_wrapping_add_##type_name(type_max - 1, 2) == 0, return UNIT_FAIL); \
	unit_assert(nvgpu_wrapping_add_##type_name(type_max, type_max) == (type_max - 1), return UNIT_FAIL); \
} while (0)

/**
 * signed addition tests
 *
 * parameters:
 *   type_name: type (s32, s64 etc.)
 *   type_min:  Minimum value of the type.
 *   type_max : Maximum value of the type.
 *   tmp_operand1: random positive value in the set (1, Maximum value / 2)
 *   tmp_operand2: random negative value in the set (-1, Minimum value / 2)
 *
 * Boundary values: (min, min+1, -1, 0, 1, max-1, max)
 *
 * Valid tests: Addition result within range for each boundary value and
 *              random value.
 * Invalid tests: Addition result out of range if possible for each
 *                boundary and random value.
 */
#define GENERATE_ARITHMETIC_SIGNED_ADD_TESTS(type_name, type_min, type_max, tmp_operand1, tmp_operand2) do {\
	unit_assert(nvgpu_safe_add_##type_name(type_min, type_max) == -1, \
							return UNIT_FAIL); \
	unit_assert(nvgpu_safe_add_##type_name(0, type_max) == type_max, \
							return UNIT_FAIL); \
	unit_assert(nvgpu_safe_add_##type_name(-1, -1) == -2, \
							return UNIT_FAIL); \
	unit_assert(nvgpu_safe_add_##type_name(-1, 1) == 0, \
							return UNIT_FAIL); \
	unit_assert(nvgpu_safe_add_##type_name(1, 1) == 2, \
							return UNIT_FAIL); \
	unit_assert(nvgpu_safe_add_##type_name(type_max - tmp_operand1, tmp_operand1) == type_max, \
							return UNIT_FAIL); \
	unit_assert(nvgpu_safe_add_##type_name(type_min - tmp_operand2, tmp_operand2) == type_min, \
							return UNIT_FAIL); \
	unit_assert(nvgpu_safe_add_##type_name(tmp_operand1, tmp_operand2) == tmp_operand1 + tmp_operand2, \
							return UNIT_FAIL); \
	unit_assert(nvgpu_safe_add_##type_name(tmp_operand1, type_min + 1) == tmp_operand1 + type_min + 1, \
							return UNIT_FAIL); \
	unit_assert(nvgpu_safe_add_##type_name(type_max - 1, tmp_operand2) == type_max - 1 + tmp_operand2, \
							return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_add_##type_name(type_max - tmp_operand1 + 1, tmp_operand1)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_add_##type_name(type_max, tmp_operand1)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_add_##type_name(type_max, type_max)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_add_##type_name(type_min - tmp_operand2 - 1, tmp_operand2)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_add_##type_name(type_min, tmp_operand2)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_add_##type_name(type_min, type_min)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_add_##type_name(type_min + 1, type_min + 1)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_add_##type_name(type_max - 1, type_max - 1)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_add_##type_name(type_max - 1, tmp_operand1 > 1 ? tmp_operand1 : 2)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_add_##type_name(type_min + 1, tmp_operand2 < -1 ? tmp_operand2 : -2)); \
	unit_assert(err != 0, return UNIT_FAIL); \
} while (0)

/**
 * unsigned subtraction tests
 *
 * parameters:
 *   type_name: type (u8, u32 etc.)
 *   type_max : Maximum value of the type.
 *   tmp_operand: random value in the set (1, Maximum value)
 *
 * Boundary values: (0, 1, max-1, max)
 *
 * Valid tests: Subtraction result within range for each boundary value and
 *              random value.
 * Invalid tests: Subtraction result out of range if possible for each
 *                boundary and random value.
 */
#define GENERATE_ARITHMETIC_SUBTRACT_TESTS(type_name, type_max, tmp_operand) do {\
	unit_assert(nvgpu_safe_sub_##type_name(0, 0) == 0, return UNIT_FAIL); \
	unit_assert(nvgpu_safe_sub_##type_name(1, 0) == 1, return UNIT_FAIL); \
	unit_assert(nvgpu_safe_sub_##type_name(type_max, tmp_operand) == type_max - tmp_operand, \
							return UNIT_FAIL); \
	unit_assert(nvgpu_safe_sub_##type_name(tmp_operand, 0) == tmp_operand, \
							return UNIT_FAIL); \
	unit_assert(nvgpu_safe_sub_##type_name(type_max, type_max - 1) == 1, \
							return UNIT_FAIL); \
	unit_assert(nvgpu_safe_sub_##type_name(type_max - 1, 1) == type_max - 2, \
							return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_sub_##type_name(0, 1)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_sub_##type_name(0, tmp_operand)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_sub_##type_name(0, type_max)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_sub_##type_name(tmp_operand, type_max)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_sub_##type_name(type_max - 1, type_max)); \
	unit_assert(err != 0, return UNIT_FAIL); \
} while (0)

/**
 * signed subtraction tests
 *
 * parameters:
 *   type_name: type (s32, s64 etc.)
 *   type_min:  Minimum value of the type.
 *   type_max : Maximum value of the type.
 *   tmp_operand1: random positive value in the set (1, Maximum value / 2)
 *   tmp_operand2: random negative value in the set (-1, Minimum value / 2)
 *
 * Boundary values: (min, min+1, -1, 0, 1, max-1, max)
 *
 * Valid tests: Subtraction result within range for each boundary value and
 *              random value.
 * Invalid tests: Subtraction result out of range if possible for each
 *                boundary and random value.
 */
#define GENERATE_ARITHMETIC_SIGNED_SUBTRACT_TESTS(type_name, type_min, type_max, tmp_operand1, tmp_operand2) do {\
	unit_assert(nvgpu_safe_sub_##type_name(tmp_operand2, tmp_operand2) == 0, return UNIT_FAIL); \
	unit_assert(nvgpu_safe_sub_##type_name(tmp_operand2, tmp_operand1) == tmp_operand2 - tmp_operand1, return UNIT_FAIL); \
	unit_assert(nvgpu_safe_sub_##type_name(tmp_operand1, tmp_operand2) == tmp_operand1 - tmp_operand2, return UNIT_FAIL); \
	unit_assert(nvgpu_safe_sub_##type_name(0, 0) == 0, return UNIT_FAIL); \
	unit_assert(nvgpu_safe_sub_##type_name(0, type_max) == 0 - type_max, return UNIT_FAIL); \
	unit_assert(nvgpu_safe_sub_##type_name(type_max, 0) == type_max, return UNIT_FAIL); \
	unit_assert(nvgpu_safe_sub_##type_name(-1, -1) == 0, return UNIT_FAIL); \
	unit_assert(nvgpu_safe_sub_##type_name(-1, 1) == -2, return UNIT_FAIL); \
	unit_assert(nvgpu_safe_sub_##type_name(1, -1) == 2, return UNIT_FAIL); \
	unit_assert(nvgpu_safe_sub_##type_name(1, 1) == 0, return UNIT_FAIL); \
	unit_assert(nvgpu_safe_sub_##type_name(type_min + 1, type_min + 1) == 0, return UNIT_FAIL); \
	unit_assert(nvgpu_safe_sub_##type_name(type_min, type_min) == 0, return UNIT_FAIL); \
	unit_assert(nvgpu_safe_sub_##type_name(type_max - 1, type_max - 1) == 0, return UNIT_FAIL); \
	unit_assert(nvgpu_safe_sub_##type_name(type_max, type_max) == 0, return UNIT_FAIL); \
	unit_assert(nvgpu_safe_sub_##type_name(type_min + 1, tmp_operand2) == type_min + 1 - tmp_operand2, return UNIT_FAIL); \
	unit_assert(nvgpu_safe_sub_##type_name(type_max - 1, tmp_operand1) == type_max - 1 - tmp_operand1, return UNIT_FAIL); \
	unit_assert(nvgpu_safe_sub_##type_name(tmp_operand2, tmp_operand2 - type_min) == type_min, return UNIT_FAIL); \
	unit_assert(nvgpu_safe_sub_##type_name(type_max, type_max - tmp_operand1) == tmp_operand1, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_sub_##type_name(type_min, tmp_operand1)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_sub_##type_name(type_max, tmp_operand2)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_sub_##type_name(type_max - 1, type_min + 1)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_sub_##type_name(type_min + 1, type_max - 1)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_sub_##type_name(type_max - 1, tmp_operand2 < -1 ? tmp_operand2 : -2)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_sub_##type_name(type_min + 1, tmp_operand1 > 1 ? tmp_operand1 : 2)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_sub_##type_name(0, type_min)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_sub_##type_name(type_min, type_max)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_sub_##type_name(type_max, type_min)); \
	unit_assert(err != 0, return UNIT_FAIL); \
} while (0)

/**
 * unsigned multiplication tests
 *
 * parameters:
 *   type_name: type (u8, u32 etc.)
 *   type_max : Maximum value of the type.
 *   tmp_operand: random value in the set (1, Maximum value / 2)
 *
 * Boundary values: (0, 1, max-1, max)
 *
 * Valid tests: Multiplication result within range for each boundary value and
 *              random value.
 * Invalid tests: Multiplication result out of range if possible for each
 *                boundary and random value.
 */
#define GENERATE_ARITHMETIC_MULT_TESTS(type_name, type_max, tmp_operand) do {\
	unit_assert(nvgpu_safe_mult_##type_name(0, type_max) == 0, return UNIT_FAIL); \
	unit_assert(nvgpu_safe_mult_##type_name(type_max - 1, 1) == type_max - 1, return UNIT_FAIL); \
	unit_assert(nvgpu_safe_mult_##type_name(tmp_operand, 2) == tmp_operand * 2, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_mult_##type_name(type_max - 1, 2)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_mult_##type_name(type_max - 1, tmp_operand > 1 ? tmp_operand : 2)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_mult_##type_name(type_max, type_max)); \
	unit_assert(err != 0, return UNIT_FAIL); \
} while (0)

/**
 * signed multiplication tests
 *
 * parameters:
 *   type_name: type (s32, s64 etc.)
 *   type_min:  Minimum value of the type.
 *   type_max : Maximum value of the type.
 *   tmp_operand1: random positive value in the set (1, Maximum value / 2)
 *   tmp_operand2: random negative value in the set (-1, Minimum value / 2)
 *
 * Boundary values: (min, min+1, -1, 0, 1, max-1, max)
 *
 * Valid tests: Multiplication result within range for each boundary value and
 *              random value.
 * Invalid tests: Multiplication result out of range if possible for each
 *                boundary and random value.
 */
#define GENERATE_ARITHMETIC_SIGNED_MULT_TESTS(type_name, type_min, type_max, tmp_operand1, tmp_operand2) do {\
	unit_assert(nvgpu_safe_mult_##type_name(0, type_max) == 0, return UNIT_FAIL); \
	unit_assert(nvgpu_safe_mult_##type_name(1, type_min) == type_min, return UNIT_FAIL); \
	unit_assert(nvgpu_safe_mult_##type_name(-1, -1) == 1, return UNIT_FAIL); \
	unit_assert(nvgpu_safe_mult_##type_name(-1, 1) == -1, return UNIT_FAIL); \
	unit_assert(nvgpu_safe_mult_##type_name(1, 1) == 1, return UNIT_FAIL); \
	unit_assert(nvgpu_safe_mult_##type_name(tmp_operand1, 2) == tmp_operand1 * 2, return UNIT_FAIL); \
	unit_assert(nvgpu_safe_mult_##type_name(tmp_operand2, 2) == tmp_operand2 * 2, return UNIT_FAIL); \
	unit_assert(nvgpu_safe_mult_##type_name(type_max, -1) == type_max * -1, return UNIT_FAIL); \
	unit_assert(nvgpu_safe_mult_##type_name(type_max - 1, -1) == (type_max - 1) * -1, return UNIT_FAIL); \
	unit_assert(nvgpu_safe_mult_##type_name(type_min + 1, -1) == (type_min + 1) * -1, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_mult_##type_name(type_max, 2)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_mult_##type_name(type_max, -2)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_mult_##type_name(type_min, 2)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_mult_##type_name(type_min, -1)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_mult_##type_name(type_min, type_min)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_mult_##type_name(type_max, type_max)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_mult_##type_name(type_min, type_max)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_mult_##type_name(type_min + 1, type_min + 1)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_mult_##type_name(type_max - 1, type_max - 1)); \
	unit_assert(err != 0, return UNIT_FAIL); \
	err = EXPECT_BUG((void)nvgpu_safe_mult_##type_name(type_min + 1, type_max - 1)); \
	unit_assert(err != 0, return UNIT_FAIL); \
} while (0)

int test_arithmetic(struct unit_module *m, struct gk20a *g, void *args)
{
	s64 tmp_s64, tmp_s64_neg;
	s32 tmp_s32, tmp_s32_neg;
	u64 tmp_u64;
	u32 tmp_u32;
	u8 tmp_u8;
	int err;

	srand(time(0));

	/* random value in the set (1, U8_MAX - 1) */
	tmp_u8 = rand() % U8_MAX ?: 1;

	/* random value in the set ((u32)1, (u32)(U32_MAX - 1)) */
	tmp_u32 = (u32)((rand() % U32_MAX) ?: 1);

	/* random value in the set ((s32)1, (s32)(U32_MAX - 1) / 2) */
	tmp_s32 = (s32)((rand() % U32_MAX) / 2 ?: 1);

	/* random value in the set ((s32)-1, (s32)-((U32_MAX - 1) / 2)) */
	tmp_s32_neg = (s32)(0 - (rand() % U32_MAX) / 2) ?: -1;

	/* random value in the set ((u64)1, (u64)(U64_MAX - 1)) */
	tmp_u64 = (u64)((rand() % U64_MAX) ?: 1);

	/* random value in the set ((s64)1, (s64)(U64_MAX - 1) / 2) */
	tmp_s64 = (s64)((rand() % U64_MAX) / 2 ?: 1);

	/* random value in the set ((s64)-1, (s64)-((U64_MAX - 1) / 2)) */
	tmp_s64_neg = (s64)(0 - (rand() % U64_MAX) / 2) ?: -1;

	unit_info(m, "random operands\nu8: %u, u32: %u, s32: %d, s32_neg: %d\nu64: %llu, s64: %lld, s64_neg: %lld\n",
		  tmp_u8, tmp_u32, tmp_s32, tmp_s32_neg, tmp_u64, tmp_s64, tmp_s64_neg);

	/* U8 sub */
	GENERATE_ARITHMETIC_SUBTRACT_TESTS(u8, U8_MAX, tmp_u8);

	/* U32 add */
	GENERATE_ARITHMETIC_ADD_TESTS(u32, U32_MAX, tmp_u32);

	/* wrapping U32 add */
	GENERATE_ARITHMETIC_WRAPPING_ADD_TESTS(u32, U32_MAX, tmp_u32);

	/* S32 add */
	GENERATE_ARITHMETIC_SIGNED_ADD_TESTS(s32, INT_MIN, INT_MAX, tmp_s32, tmp_s32_neg);

	/* U32 sub */
	GENERATE_ARITHMETIC_SUBTRACT_TESTS(u32, U32_MAX, tmp_u32);

	/* S32 sub */
	GENERATE_ARITHMETIC_SIGNED_SUBTRACT_TESTS(s32, INT_MIN, INT_MAX, tmp_s32, tmp_s32_neg);

	/* U32 Mult */
	GENERATE_ARITHMETIC_MULT_TESTS(u32, U32_MAX, tmp_u32);

	/* U64 add */
	GENERATE_ARITHMETIC_ADD_TESTS(u64, U64_MAX, tmp_u64);

	/* S64 add */
	GENERATE_ARITHMETIC_SIGNED_ADD_TESTS(s64, LONG_MIN, LONG_MAX, tmp_s64, tmp_s64_neg);

	/* U64 sub */
	GENERATE_ARITHMETIC_SUBTRACT_TESTS(u64, U64_MAX, tmp_u64);

	/* S64 sub */
	GENERATE_ARITHMETIC_SIGNED_SUBTRACT_TESTS(s64, LONG_MIN, LONG_MAX, tmp_s64, tmp_s64_neg);

	/* U64 Mult */
	GENERATE_ARITHMETIC_MULT_TESTS(u64, U64_MAX, tmp_u64);

	/* S64 Mult */
	GENERATE_ARITHMETIC_SIGNED_MULT_TESTS(s64, LONG_MIN, LONG_MAX, tmp_s64, tmp_s64_neg);

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

int test_safety_checks(struct unit_module *m, struct gk20a *g, void *args)
{
	nvgpu_safety_checks();

	return  UNIT_SUCCESS;
}

struct unit_module_test static_analysis_tests[] = {
	UNIT_TEST(arithmetic,		test_arithmetic,	NULL, 0),
	UNIT_TEST(cast,			test_cast,		NULL, 0),
	UNIT_TEST(safety_checks,	test_safety_checks,	NULL, 0),
};

UNIT_MODULE(static_analysis, static_analysis_tests, UNIT_PRIO_NVGPU_TEST);
