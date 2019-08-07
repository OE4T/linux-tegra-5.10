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

#ifndef NVGPU_POSIX_UTILS_H
#define NVGPU_POSIX_UTILS_H

#include <nvgpu/safe_ops.h>

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

#define DIV_ROUND_UP_U64(n, d)					\
({								\
	u64 divr = (u64)(d);					\
	u64 divm = nvgpu_safe_sub_u64((divr), U64(1));		\
	u64 rup = nvgpu_safe_add_u64((u64)(n), (divm));		\
	u64 ret_val = ((rup) / (divr));				\
	ret_val ;						\
})

#define DIV_ROUND_UP(n, d)						\
({									\
	typeof(n) val =  ((sizeof(typeof(n))) == (sizeof(u64))) ?	\
		(typeof(n))(DIV_ROUND_UP_U64(n, d)) :			\
		nvgpu_safe_cast_u64_to_u32(DIV_ROUND_UP_U64(n, d));	\
	val;								\
})

#define DIV_ROUND_UP_ULL	DIV_ROUND_UP

#define DIV_ROUND_CLOSEST(a, divisor)(                  \
{                                                       \
	typeof(a) val = (a);                            \
	typeof(divisor) div = (divisor);                \
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

#define IS_UNSIGNED_TYPE(x)						\
	(__builtin_types_compatible_p(typeof(x), unsigned int) ||	\
		__builtin_types_compatible_p(typeof(x), unsigned long) || \
		__builtin_types_compatible_p(typeof(x), unsigned long long))

#define IS_UNSIGNED_LONG_TYPE(x)					\
	(__builtin_types_compatible_p(typeof(x), unsigned long) ||	\
		__builtin_types_compatible_p(typeof(x), unsigned long long))

#define IS_SIGNED_LONG_TYPE(x)						\
	(__builtin_types_compatible_p(typeof(x), signed long) ||	\
		__builtin_types_compatible_p(typeof(x), signed long long))

#define ALIGN_MASK(x, mask)						\
	__builtin_choose_expr(						\
		(IS_UNSIGNED_TYPE(x) && IS_UNSIGNED_TYPE(mask)),	\
		__builtin_choose_expr(					\
			IS_UNSIGNED_LONG_TYPE(x),			\
			(nvgpu_safe_add_u64((x), (mask)) & ~(mask)),	\
			(nvgpu_safe_add_u32((x), (mask)) & ~(mask))),	\
		/* Results in build error. Make x/mask type unsigned */ \
		(void)0)

#define ALIGN(x, a)							\
	__builtin_choose_expr(						\
		(IS_UNSIGNED_TYPE(x) && IS_UNSIGNED_TYPE(a)),		\
		__builtin_choose_expr(					\
			IS_UNSIGNED_LONG_TYPE(x),			\
				ALIGN_MASK((x),				\
				(nvgpu_safe_sub_u64((typeof(x))(a), 1))), \
				ALIGN_MASK((x),				\
				(nvgpu_safe_sub_u32((typeof(x))(a), 1)))), \
			/* Results in build error. Make x/a type unsigned */ \
			(void)0)

#define PAGE_ALIGN(x)		ALIGN(x, PAGE_SIZE)

#define HZ_TO_KHZ(x) ((x) / KHZ)
#define HZ_TO_MHZ(a) (u16)(a/MHZ)
#define HZ_TO_MHZ_ULL(a) (((a) > 0xF414F9CD7ULL) ? (u16) 0xffffU :\
		(((a) >> 32) > 0U) ? (u16) (((a) * 0x10C8ULL) >> 32) :\
		(u16) ((u32) (a)/MHZ))
#define KHZ_TO_HZ(x) ((x) * KHZ)
#define MHZ_TO_KHZ(x) ((x) * KHZ)
#define KHZ_TO_MHZ(a) (u16)(a/KHZ)
#define MHZ_TO_HZ_ULL(a) ((u64)(a) * MHZ)

/*
 * Caps return at the size of the buffer not what would have been written if buf
 * were arbitrarily sized.
 */
#ifdef CONFIG_NVGPU_LOGGING
static inline int scnprintf(char *buf, size_t size, const char *format, ...)
{
	size_t ret;
	va_list args;

	va_start(args, format);
	ret = (size_t)vsnprintf(buf, size, format, args);
	va_end(args);

	return ret <= size ? (int)ret : (int)size;
}
#else
static inline int scnprintf(char *buf, size_t size, const char *format, ...)
{
	return 0;
}
#endif

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
static inline unsigned int nvgpu_posix_hweight8(uint8_t x)
{
	unsigned int ret;
	uint8_t result = ((U8(x) >> U8(1)) & U8(0x55));

	result = nvgpu_safe_sub_u8(x, result);

	result = (result & U8(0x33)) + ((result >> U8(2)) & U8(0x33));
	result = (result + (result >> U8(4))) & U8(0x0f);
	ret = (unsigned int)result;

	return ret;
}
static inline unsigned int nvgpu_posix_hweight16(uint16_t x)
{
	unsigned int ret;

	ret = nvgpu_posix_hweight8((uint8_t)(x & U8(0xff)));
	ret += nvgpu_posix_hweight8((uint8_t)((x >> U8(8)) & U8(0xff)));

	return ret;
}

static inline unsigned int nvgpu_posix_hweight32(uint32_t x)
{
	unsigned int ret;

	ret = nvgpu_posix_hweight16((uint16_t)(x & U16(0xffff)));
	ret += nvgpu_posix_hweight16((uint16_t)((x >> U16(16)) & U16(0xffff)));

	return ret;
}

static inline unsigned int nvgpu_posix_hweight64(uint64_t x)
{
	unsigned int ret;
	u32 lo, hi;

	lo = nvgpu_safe_cast_u64_to_u32(x & ~(u32)0);
	hi = nvgpu_safe_cast_u64_to_u32(x >> 32) & ~(u32)0;

	ret =  nvgpu_posix_hweight32(lo);
	ret += nvgpu_posix_hweight32(hi);

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

#define container_of(ptr, type, member) ({                    \
	const typeof(((type *)0)->member) *__mptr = (ptr);    \
	(type *)((char *)__mptr - offsetof(type, member)); })

#define __packed __attribute__((packed))

#define MAX_ERRNO	4095

#define ERESTARTSYS ERESTART

#endif /* NVGPU_POSIX_UTILS_H */
