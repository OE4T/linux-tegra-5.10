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

#ifndef NVGPU_SAFE_OPS_H
#define NVGPU_SAFE_OPS_H

#include <nvgpu/types.h>
#include <nvgpu/bug.h>

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
#endif /* NVGPU_SAFE_OPS_H */
