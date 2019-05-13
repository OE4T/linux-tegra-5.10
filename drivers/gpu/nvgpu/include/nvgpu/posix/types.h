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

#ifndef NVGPU_POSIX_TYPES_H
#define NVGPU_POSIX_TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdarg.h>

/*
 * For endianness functions.
 */
#include <netinet/in.h>

#ifdef false
#undef false
#endif
#define false (0 != 0)

#ifdef true
#undef true
#endif
#define true (0 == 0)

typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;
typedef unsigned long long	u64;

typedef signed char		s8;
typedef signed short		s16;
typedef signed int		s32;
typedef signed long long	s64;

#define min_t(type, a, b)		\
	({				\
		type __a = (a);		\
		type __b = (b);		\
		__a < __b ? __a : __b;	\
	})

#if defined(min)
#undef min
#endif
#if defined(max)
#undef max
#endif

#define min(a, b)			\
	({				\
		(a) < (b) ? (a) : (b);	\
	})
#define max(a, b)			\
	({				\
		(a) > (b) ? (a) : (b);	\
	})
#define min3(a, b, c)			min(min(a, b), c)

#define PAGE_SIZE	4096U

#define ARRAY_SIZE(array)			\
	(sizeof(array) / sizeof((array)[0]))

#define MAX_SCHEDULE_TIMEOUT	LONG_MAX

#define DIV_ROUND_UP(n, d)	(((n) + (d) - 1U) / (d))
#define DIV_ROUND_UP_ULL	DIV_ROUND_UP

#define DIV_ROUND_CLOSEST(a, divisor)(                  \
{                                                       \
	typeof(a) val = a;                              \
	typeof(divisor) div = divisor;                  \
	(((typeof(a))-1) > 0 ||                         \
	((typeof(divisor))-1) > 0 || (val) > 0) ?       \
		(((val) + ((div) / 2)) / (div)) :       \
		(((val) - ((div) / 2)) / (div));        \
}                                                       \
)
/*
 * Joys of userspace: usually division just works since the compiler can link
 * against external division functions implicitly.
 */
#define do_div(a, b)		((a) /= (b))

#define div64_u64(a, b)		((a) / (b))

#define round_mask(x, y)	((__typeof__(x))((y) - 1U))
#define round_up(x, y)		((((x) - 1U) | round_mask(x, y)) + 1U)
#define roundup(x, y)		round_up(x, y)
#define round_down(x, y)	((x) & ~round_mask(x, y))

#define ALIGN_MASK(x, mask)	(((x) + (mask)) & ~(mask))
#define ALIGN(x, a)		ALIGN_MASK(x, (typeof(x))(a) - 1U)
#define PAGE_ALIGN(x)		ALIGN(x, PAGE_SIZE)

#define HZ_TO_KHZ(x) ((x) / KHZ)
#define HZ_TO_MHZ(a) (u16)(a/MHZ)
#define HZ_TO_MHZ_ULL(a) ((a > 0xF414F9CD7ULL) ? 0xffff : (a >> 32) ? \
	(u32) ((a * 0x10C8ULL) >> 32) : (u16) ((u32) a/MHZ))
#define KHZ_TO_HZ(x) ((x) * KHZ)
#define MHZ_TO_KHZ(x) ((x) * KHZ)
#define KHZ_TO_MHZ(a) (u16)(a/KHZ)
#define MHZ_TO_HZ_ULL(a) ((u64)a * MHZ)

/*
 * Caps return at the size of the buffer not what would have been written if buf
 * were arbitrarily sized.
 */
static inline int scnprintf(char *buf, size_t size, const char *format, ...)
{
	size_t ret;
	va_list args;

	va_start(args, format);
	ret = (size_t)vsnprintf(buf, size, format, args);
	va_end(args);

	return ret <= size ? (int)ret : (int)size;
}

static inline u32 be32_to_cpu(u32 x)
{
	/*
	 * Conveniently big-endian happens to be network byte order as well so
	 * we can use ntohl() for this.
	 */
	return ntohl(x);
}

/*
 * Hamming weights.
 */
static inline unsigned long nvgpu_posix_hweight8(uint8_t x)
{
	unsigned long ret;
	uint8_t result = x - ((x >> 1) & 0x55U);

	result = (result & 0x33U) + ((result >> 2) & 0x33U);
	result = (result + (result >> 4)) & 0x0FU;
	ret = (unsigned long)result;

	return ret;
}
static inline unsigned long nvgpu_posix_hweight16(uint16_t x)
{
	unsigned long ret;

	ret = nvgpu_posix_hweight8((uint8_t)x);
	ret += nvgpu_posix_hweight8((uint8_t)((x & 0xff00U) >> 8));

	return ret;
}

static inline unsigned long nvgpu_posix_hweight32(uint32_t x)
{
	unsigned long ret;

	ret = nvgpu_posix_hweight16((uint16_t)x);
	ret += nvgpu_posix_hweight16((uint16_t)((x & 0xffff0000U) >> 16));

	return ret;
}

static inline unsigned long nvgpu_posix_hweight64(uint64_t x)
{
	unsigned long ret;

	ret =  nvgpu_posix_hweight32((uint32_t)x);
	ret += nvgpu_posix_hweight32((uint32_t)((x &
					0xffffffff00000000U) >> 32));

	return ret;
}

#define hweight32		nvgpu_posix_hweight32
#define hweight_long		nvgpu_posix_hweight64

/*
 * Better suited under a compiler.h type header file, but for now these can live
 * here.
 */
#define __must_check		__attribute__((warn_unused_result))
#define __maybe_unused		__attribute__((unused))
#define __iomem
#define __user
#define unlikely(x)	(x)
#define likely(x)	(x)

#define __stringify(x)		#x

/*
 * Prevent compiler optimizations from mangling writes. But likely most uses of
 * this in nvgpu are incorrect (i.e unnecessary).
 */
#define WRITE_ONCE(p, v)				\
	({						\
		volatile typeof(p) *__p__ = &(p);	\
		*__p__ = (v);				\
	})

#define container_of(ptr, type, member) ({                     \
	const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
	(type *)( (char *)__mptr - offsetof(type,member) );})

#define __packed __attribute__((packed))

#define MAX_ERRNO	4095

#define IS_ERR_VALUE(x) ((x) >= (unsigned long)-MAX_ERRNO)

static inline void *ERR_PTR(long error)
{
	return (void *) error;
}

static inline long PTR_ERR(void *error)
{
	return (long)(uintptr_t)error;
}

static inline bool IS_ERR(const void *ptr)
{
	return IS_ERR_VALUE((unsigned long)ptr);
}

static inline bool IS_ERR_OR_NULL(const void *ptr)
{
	return (ptr == NULL) || IS_ERR_VALUE((unsigned long)ptr);
}

#define ERESTARTSYS ERESTART

#endif /* NVGPU_POSIX_TYPES_H */
