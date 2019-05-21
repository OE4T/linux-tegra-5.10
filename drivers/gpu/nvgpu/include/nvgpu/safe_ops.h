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

static inline u32 nvgpu_safe_add_u32(u32 ui_a, u32 ui_b)
{
	if (UINT_MAX - ui_a < ui_b) {
		BUG();
	} else {
		return ui_a + ui_b;
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

static inline u32 nvgpu_safe_sub_u32(u32 ui_a, u32 ui_b)
{
	if (ui_a < ui_b) {
		BUG();
	} else {
		return ui_a - ui_b;
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

static inline u32 nvgpu_safe_mult_u32(u32 ui_a, u32 ui_b)
{
	if (ui_a == 0 || ui_b == 0) {
		return 0U;
	} else if (ui_a > UINT_MAX / ui_b) {
		BUG();
	} else {
		return ui_a * ui_b;
	}
}

static inline u64 nvgpu_safe_mult_u64(u64 ul_a, u64 ul_b)
{
	if (ul_a == 0 || ul_b == 0) {
		return 0UL;
	} else if (ul_a > UINT_MAX / ul_b) {
		BUG();
	} else {
		return ul_a * ul_b;
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

static inline u32 nvgpu_safe_cast_bool_to_u32(bool bl_a)
{
	return bl_a == true ? 1U : 0U;
}

#endif /* NVGPU_SAFE_OPS_H */
