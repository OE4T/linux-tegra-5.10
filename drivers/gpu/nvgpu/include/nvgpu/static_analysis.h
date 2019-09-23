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

#ifndef NVGPU_STATIC_ANALYSIS_H
#define NVGPU_STATIC_ANALYSIS_H

/**
 * @file
 *
 * Macros/functions/etc for static analysis of nvgpu code.
 */

#include <nvgpu/types.h>
#include <nvgpu/bug.h>

/** @name Coverity Whitelisting
 *  These macros are used for whitelisting coverity violations. The macros are
 *  only enabled when a coverity scan is being run.
 */
/**@{*/
#ifdef NV_IS_COVERITY
/**
 * NVGPU_MISRA - Define a MISRA rule for NVGPU_COV_WHITELIST.
 *
 * @param type - This should be Rule or Directive depending on if you're dealing
 *               with a MISRA rule or directive.
 * @param num  - This is the MISRA rule/directive number. Replace hyphens and
 *               periods in the rule/directive number with underscores. Example:
 *               14.2 should be 14_2.
 *
 * This is a convenience macro for defining a MISRA rule for the
 * NVGPU_COV_WHITELIST macro.
 *
 * Example 1: For defining MISRA rule 14.2, use NVGPU_MISRA(Rule, 14_2).\n
 * Example 2: For defining MISRA directive 4.7, use NVGPU_MISRA(Directive, 4_7).
 */
#define NVGPU_MISRA(type, num)	MISRA_C_2012_##type##_##num

/**
 * NVGPU_CERT - Define a CERT C rule for NVGPU_COV_WHITELIST.
 *
 * @param num - This is the CERT C rule number. Replace hyphens and periods in
 *              the rule number with underscores. Example: INT30-C should be
 *              INT30_C.
 *
 * This is a convenience macro for defining a CERT C rule for the
 * NVGPU_COV_WHITELIST macro.
 *
 * Example: For defining CERT C rule INT30-C, use NVGPU_CERT(INT30_C).
 */
#define NVGPU_CERT(num)		CERT_##num

/**
 * Helper macro for stringifying the _Pragma() string
 */
#define NVGPU_COV_STRING(x)	#x

/**
 * NVGPU_COV_WHITELIST - Whitelist a coverity violation on the next line.
 *
 * @param type        - This is the whitelisting category. Valid values are
 *                      deviate or false_positive.\n
 *                      deviate is for an approved rule deviation.\n
 *                      false_positive is normally used for a bug in coverity
 *                      which causes a false violation to appear in the scan.
 * @param checker     - This is the MISRA or CERT C rule causing the violation.
 *                      Use the NVGPU_MISRA() or NVGPU_CERT() macro to define
 *                      this field.
 * @param comment_str - This is the comment that you want associated with this
 *                      whitelisting. This should normally be a bug number
 *                      (ex: coverity bug) or JIRA task ID (ex: RFD). Unlike the
 *                      other arguments, this argument must be a quoted string.
 *
 * Use this macro to whitelist a coverity violation in the next line of code.
 *
 * Example 1: Whitelist a MISRA rule 14.2 violation due to a deviation
 * documented in the JIRA TID-123 RFD:\n
 * NVGPU_COV_WHITELIST(deviate, NVGPU_MISRA(Rule, 14_2), "JIRA TID-123")\n
 * // Next line of code with a rule 14.2 violation
 *
 * Example 2: Whitelist violations for CERT C rules INT30-C and STR30-C caused
 * by coverity bugs:\n
 * NVGPU_COV_WHITELIST(false_positive, NVGPU_CERT(INT30_C), "Bug 123456")\n
 * NVGPU_COV_WHITELIST(false_positive, NVGPU_CERT(STR30_C), "Bug 123457")\n
 * // Next line of code with INT30-C and STR30-C violations
 */
#define NVGPU_COV_WHITELIST(type, checker, comment_str) \
	_Pragma(NVGPU_COV_STRING(coverity compliance type checker comment_str))

/**
 * NVGPU_COV_WHITELIST_BLOCK_BEGIN - Whitelist a coverity violation for a block
 *                                   of code.
 *
 * @param type        - This is the whitelisting category. Valid values are
 *                      deviate or false_positive.\n
 *                      deviate is for an approved rule deviation.\n
 *                      false_positive is normally used for a bug in coverity
 *                      which causes a false violation to appear in the scan.
 * @param num         - This is number of violations expected within the block.
 * @param checker     - This is the MISRA or CERT C rule causing the violation.
 *                      Use the NVGPU_MISRA() or NVGPU_CERT() macro to define
 *                      this field.
 * @param comment_str - This is the comment that you want associated with this
 *                      whitelisting. This should normally be a bug number
 *                      (ex: coverity bug) or JIRA task ID (ex: RFD). Unlike the
 *                      other arguments, this argument must be a quoted string.
 *
 * Use this macro to whitelist a coverity violation for a block of code. It
 * must be terminated by an NVGPU_COV_WHITELIST_BLOCK_END()
 *
 * Example: Whitelist 10 MISRA rule 14.2 violation due to a deviation
 * documented in the JIRA TID-123 RFD:\n
 * NVGPU_COV_WHITELIST_BLOCK_BEGIN(deviate, 10, NVGPU_MISRA(Rule, 14_2), "JIRA TID-123")\n
 *  > Next block of code with 10 rule 14.2 violations
 * NVGPU_COV_WHITELIST_BLOCK_END(NVGPU_MISRA(Rule, 14_2))\n
 *
 */
#define NVGPU_COV_WHITELIST_BLOCK_BEGIN(type, num, checker, comment_str) \
	_Pragma(NVGPU_COV_STRING(coverity compliance block type:num checker comment_str))

/**
 * NVGPU_COV_WHITELIST_BLOCK_END - End whitelist a block of code.that is
 *                                 whitelisted with a
 *                                 NVGPU_COV_WHITELIST_BLOCK_BEGIN
 *
 * @param checker     - This is the MISRA or CERT C rule causing the violation.
 *                      Use the NVGPU_MISRA() or NVGPU_CERT() macro to define
 *                      this field.
 *
 * Use this macro to mark the end of the block whitelisted by
 * NVGPU_COV_WHITELIST_BLOCK_END()
 *
 */
#define NVGPU_COV_WHITELIST_BLOCK_END(checker) \
	_Pragma(NVGPU_COV_STRING(coverity compliance end_block checker))
#else
/**
 * no-op macros for normal compilation - whitelisting is disabled when a
 * coverity scan is NOT being run
 */
#define NVGPU_MISRA(type, num)
#define NVGPU_CERT(num)
#define NVGPU_COV_WHITELIST(type, checker, comment_str)
#define NVGPU_COV_WHITELIST_BLOCK_BEGIN(type, num, checker, comment_str)
#define NVGPU_COV_WHITELIST_BLOCK_END(checker)
#endif
/**@}*/ /* "Coverity Whitelisting" doxygen group */

static inline u32 nvgpu_safe_add_u32(u32 ui_a, u32 ui_b)
{
	if (UINT_MAX - ui_a < ui_b) {
		BUG();
	} else {
		return ui_a + ui_b;
	}
}

static inline s32 nvgpu_safe_add_s32(s32 si_a, s32 si_b)
{
	if (((si_b > 0) && (si_a > (INT_MAX - si_b))) ||
		((si_b < 0) && (si_a < (INT_MIN - si_b)))) {
		BUG();
	} else {
		return si_a + si_b;
	}
}

static inline u64 nvgpu_safe_add_u64(u64 ul_a, u64 ul_b)
{
NVGPU_COV_WHITELIST(false_positive, NVGPU_CERT(INT30_C), "Bug 2643092")
	if (ULONG_MAX - ul_a < ul_b) {
		BUG();
	} else {
		return ul_a + ul_b;
	}
}

static inline s64 nvgpu_safe_add_s64(s64 sl_a, s64 sl_b)
{
	if (((sl_b > 0) && (sl_a > (LONG_MAX - sl_b))) ||
		((sl_b < 0) && (sl_a < (LONG_MIN - sl_b)))) {
		BUG();
	} else {
		return sl_a + sl_b;
	}
}

static inline u8 nvgpu_safe_sub_u8(u8 uc_a, u8 uc_b)
{
	if (uc_a < uc_b) {
		BUG();
	} else {
		return uc_a - uc_b;
	}
}

static inline u32 nvgpu_safe_sub_u32(u32 ui_a, u32 ui_b)
{
	if (ui_a < ui_b) {
		BUG();
	} else {
		return ui_a - ui_b;
	}
}

static inline s32 nvgpu_safe_sub_s32(s32 si_a, s32 si_b)
{
	if ((si_b > 0 && si_a < INT_MIN + si_b) ||
		(si_b < 0 && si_a > INT_MAX + si_b)) {
		BUG();
	} else {
		return si_a - si_b;
	}
}

static inline u64 nvgpu_safe_sub_u64(u64 ul_a, u64 ul_b)
{
	if (ul_a < ul_b) {
		BUG();
	} else {
		return ul_a - ul_b;
	}
}

static inline s64 nvgpu_safe_sub_s64(s64 si_a, s64 si_b)
{
	if ((si_b > 0 && si_a < LONG_MIN + si_b) ||
		(si_b < 0 && si_a > LONG_MAX + si_b)) {
		BUG();
	} else {
		return si_a - si_b;
	}
}

static inline u32 nvgpu_safe_mult_u32(u32 ui_a, u32 ui_b)
{
	if (ui_a == 0U || ui_b == 0U) {
		return 0U;
	} else if (ui_a > UINT_MAX / ui_b) {
		BUG();
	} else {
		return ui_a * ui_b;
	}
}

static inline u64 nvgpu_safe_mult_u64(u64 ul_a, u64 ul_b)
{
	if (ul_a == 0UL || ul_b == 0UL) {
		return 0UL;
	} else if (ul_a > ULONG_MAX / ul_b) {
		BUG();
	} else {
		return ul_a * ul_b;
	}
}

static inline s64 nvgpu_safe_mult_s64(s64 sl_a, s64 sl_b)
{
	if (sl_a > 0) {
		if (sl_b > 0) {
			if (sl_a > (LONG_MAX / sl_b)) {
				BUG();
			}
		} else {
			if (sl_b < (LONG_MIN / sl_a)) {
				BUG();
			}
		}
	} else {
		if (sl_b > 0) {
			if (sl_a < (LONG_MIN / sl_b)) {
				BUG();
			}
		} else {
			if ((sl_a != 0) && (sl_b < (LONG_MAX / sl_a))) {
				BUG();
			}
		}
	}

	return sl_a * sl_b;
}

static inline u16 nvgpu_safe_cast_u64_to_u16(u64 ul_a)
{
	if (ul_a > USHRT_MAX) {
		BUG();
	} else {
		return (u16)ul_a;
	}
}

static inline u32 nvgpu_safe_cast_u64_to_u32(u64 ul_a)
{
	if (ul_a > UINT_MAX) {
		BUG();
	} else {
		return (u32)ul_a;
	}
}

static inline u8 nvgpu_safe_cast_u64_to_u8(u64 ul_a)
{
	if (ul_a > UCHAR_MAX) {
		BUG();
	} else {
		return (u8)ul_a;
	}
}

static inline u32 nvgpu_safe_cast_s64_to_u32(s64 l_a)
{
	if ((l_a < 0) || (l_a > UINT_MAX)) {
		BUG();
	} else {
		return (u32)l_a;
	}
}

static inline u64 nvgpu_safe_cast_s64_to_u64(s64 l_a)
{
	if (l_a < 0) {
		BUG();
	} else {
		return (u64)l_a;
	}
}

static inline u32 nvgpu_safe_cast_bool_to_u32(bool bl_a)
{
	return bl_a == true ? 1U : 0U;
}

static inline u8 nvgpu_safe_cast_s8_to_u8(s8 sc_a)
{
NVGPU_COV_WHITELIST(false_positive, NVGPU_CERT(STR34_C), "Bug 2673832")
	if (sc_a < 0) {
		BUG();
	} else {
		return (u8)sc_a;
	}
}

static inline u32 nvgpu_safe_cast_s32_to_u32(s32 si_a)
{
	if (si_a < 0) {
		BUG();
	} else {
		return (u32)si_a;
	}
}

static inline u64 nvgpu_safe_cast_s32_to_u64(s32 si_a)
{
	if (si_a < 0) {
		BUG();
	} else {
		return (u64)si_a;
	}
}

static inline u16 nvgpu_safe_cast_u32_to_u16(u32 ui_a)
{
	if (ui_a > USHRT_MAX) {
		BUG();
	} else {
		return (u16)ui_a;
	}
}

static inline u8 nvgpu_safe_cast_u32_to_u8(u32 ui_a)
{
	if (ui_a > UCHAR_MAX) {
		BUG();
	} else {
		return (u8)ui_a;
	}
}

static inline s8 nvgpu_safe_cast_u32_to_s8(u32 ui_a)
{
	if (ui_a > SCHAR_MAX) {
		BUG();
	} else {
		return (s8)ui_a;
	}
}

static inline s32 nvgpu_safe_cast_u32_to_s32(u32 ui_a)
{
	if (ui_a > INT_MAX) {
		BUG();
	} else {
		return (s32)ui_a;
	}
}

static inline s32 nvgpu_safe_cast_u64_to_s32(u64 ul_a)
{
	if (ul_a > INT_MAX) {
		BUG();
	} else {
		return (s32)ul_a;
	}
}

static inline s64 nvgpu_safe_cast_u64_to_s64(u64 ul_a)
{
NVGPU_COV_WHITELIST(false_positive, NVGPU_MISRA(Rule, 14_3), "Bug 2615925")
	if (ul_a > LONG_MAX) {
		BUG();
	} else {
		return (s64)ul_a;
	}
}

static inline s32 nvgpu_safe_cast_s64_to_s32(s64 sl_a)
{
	if (sl_a > INT_MAX || sl_a < INT_MIN) {
		BUG();
	} else {
		return (s32)sl_a;
	}
}

#define NVGPU_PRECISION(v) _Generic(v, \
		unsigned int : __builtin_popcount, \
		unsigned long : __builtin_popcountl, \
		unsigned long long : __builtin_popcountll, \
		default : __builtin_popcount)(v)

static inline void nvgpu_safety_checks(void)
{
	/*
	 * For CERT-C INT35-C rule
	 * Check compatibility between size (in bytes) and precision
	 * (in bits) of unsigned int. BUG() if two are not same.
	 */
	if (sizeof(unsigned int) * 8U != NVGPU_PRECISION(UINT_MAX)) {
		BUG();
	}

	/*
	 * For CERT-C INT34-C rule
	 * Check precision of unsigned types. Shift operands have been
	 * checked to be less than these values.
	 */
	if (NVGPU_PRECISION(UCHAR_MAX) != 8 ||
		NVGPU_PRECISION(USHRT_MAX) != 16 ||
		NVGPU_PRECISION(UINT_MAX) != 32 ||
		NVGPU_PRECISION(ULONG_MAX) != 64 ||
		NVGPU_PRECISION(ULLONG_MAX) != 64) {
		BUG();
	}
}

#endif /* NVGPU_STATIC_ANALYSIS_H */
