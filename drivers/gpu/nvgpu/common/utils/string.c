/*
 * Copyright (c) 2018-2021, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/string.h>
#include <nvgpu/log.h>
#include <nvgpu/static_analysis.h>

void
nvgpu_memcpy(u8 *destb, const u8 *srcb, size_t n)
{
	(void) memcpy(destb, srcb, n);
}

int
nvgpu_memcmp(const u8 *b1, const u8 *b2, size_t n)
{
	return memcmp(b1, b2, n);
}

int nvgpu_strnadd_u32(char *dst, const u32 value, size_t size, u32 radix)
{
	int n;
	u32 v;
	char *p;
	u32 digit;

	if ((radix < 2U) || (radix > 16U)) {
		return 0;
	}

	if (size > ((u64)(INT_MAX))) {
		return 0;
	}

	/* how many digits do we need ? */
	n = 0;
	v = value;
	do {
		n = nvgpu_safe_add_s32(n, 1);
		v = v / radix;
	} while (v > 0U);

	/* bail out if there is not room for '\0' */
	if (n >= (s32)size) {
		return 0;
	}

	/* number of digits (not including '\0') */
	p = dst + n;

	/* terminate with '\0' */
	*p = '\0';
	p--;

	v = value;
	do {
		digit = v % radix;
		*p = "0123456789abcdef"[digit];
		v = v / radix;
		p--;
	}
	while (v > 0U);

	return n;
}

bool nvgpu_mem_is_word_aligned(struct gk20a *g, u8 *addr)
{
	if (((unsigned long)addr % 4UL) != 0UL) {
		nvgpu_log_info(g, "addr not 4-byte aligned");
		return false;
	}

	return true;
}
