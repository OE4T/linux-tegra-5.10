/*
 * Copyright (c) 2017-2019, NVIDIA CORPORATION.  All rights reserved.
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
#ifndef NVGPU_BITOPS_H
#define NVGPU_BITOPS_H

#include <nvgpu/types.h>
#include <nvgpu/bug.h>

/*
 * Explicit sizes for bit definitions. Please use these instead of BIT().
 */
#define BIT8(i)		(U8(1) << U8(i))
#define BIT16(i)	(U16(1) << U16(i))
#define BIT32(i)	(U32(1) << U32(i))
#define BIT64(i)	(U64(1) << U64(i))

#ifdef __KERNEL__
#include <linux/bitops.h>
#include <linux/bitmap.h>
#else
#include <nvgpu/posix/bitops.h>
#endif

static inline void nvgpu_bitmap_set(unsigned long *map, unsigned int start,
				    unsigned int len)
{
	BUG_ON(len > U32(INT_MAX));
	bitmap_set(map, start, (int)len);
}

static inline void nvgpu_bitmap_clear(unsigned long *map, unsigned int start,
				      unsigned int len)
{
	BUG_ON(len > U32(INT_MAX));
	bitmap_clear(map, start, (int)len);
}

#endif /* NVGPU_BITOPS_H */
