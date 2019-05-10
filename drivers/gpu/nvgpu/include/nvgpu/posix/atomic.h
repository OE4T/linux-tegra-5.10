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

#ifndef NVGPU_POSIX_ATOMIC_H
#define NVGPU_POSIX_ATOMIC_H

#include <stdatomic.h>
#include <nvgpu/types.h>

/*
 * Note: this code uses the GCC builtins to implement atomics.
 */

typedef struct __nvgpu_posix_atomic {
	atomic_int v;
} nvgpu_atomic_t;

typedef struct __nvgpu_posix_atomic64 {
	atomic_long v;
} nvgpu_atomic64_t;

#define nvgpu_atomic_init_impl(i)		{ i }

#define nvgpu_atomic64_init_impl(i)		{ i }

/*
 * These macros define the common cases to maximize code reuse, especially
 * between the 32bit and 64bit cases.
 * The static inline functions are maintained to provide type checking.
 */
#define NVGPU_POSIX_ATOMIC_SET(v, i) atomic_store(&((v)->v), (i))

#define NVGPU_POSIX_ATOMIC_READ(v) atomic_load(&((v)->v))

#define NVGPU_POSIX_ATOMIC_ADD_RETURN(v, i)				\
	({								\
		typeof((v)->v) tmp;					\
									\
		tmp = (typeof((v)->v))atomic_fetch_add(&((v)->v), (i));	\
		tmp += (i);						\
		tmp;							\
	})

#define NVGPU_POSIX_ATOMIC_SUB_RETURN(v, i)				\
	({								\
		typeof((v)->v) tmp;					\
									\
		tmp = (typeof((v)->v))atomic_fetch_sub(&((v)->v), (i));	\
		tmp -= (i);						\
		tmp;							\
	})

#define NVGPU_POSIX_ATOMIC_CMPXCHG(v, old, new)				\
	({								\
		typeof((v)->v) tmp = (old);				\
									\
		atomic_compare_exchange_strong(&((v)->v), &tmp, (new));	\
		tmp;							\
	})

#define NVGPU_POSIX_ATOMIC_XCHG(v, new) atomic_exchange(&((v)->v), (new))

#define NVGPU_POSIX_ATOMIC_ADD_UNLESS(v, a, u)				\
	({								\
		typeof((v)->v) old;					\
									\
		do {							\
			old = atomic_load(&((v)->v));			\
			if (old == (u)) {				\
				break;					\
			}						\
		} while (!atomic_compare_exchange_strong(&((v)->v),	\
				&old, old + (a)));			\
		old;							\
	})

static inline void nvgpu_atomic_set_impl(nvgpu_atomic_t *v, int i)
{
	NVGPU_POSIX_ATOMIC_SET(v, i);
}

static inline int nvgpu_atomic_read_impl(nvgpu_atomic_t *v)
{
	return NVGPU_POSIX_ATOMIC_READ(v);
}

static inline void nvgpu_atomic_inc_impl(nvgpu_atomic_t *v)
{
	(void)NVGPU_POSIX_ATOMIC_ADD_RETURN(v, 1);
}

static inline int nvgpu_atomic_inc_return_impl(nvgpu_atomic_t *v)
{
	return NVGPU_POSIX_ATOMIC_ADD_RETURN(v, 1);
}

static inline void nvgpu_atomic_dec_impl(nvgpu_atomic_t *v)
{
	(void)NVGPU_POSIX_ATOMIC_SUB_RETURN(v, 1);
}

static inline int nvgpu_atomic_dec_return_impl(nvgpu_atomic_t *v)
{
	return NVGPU_POSIX_ATOMIC_SUB_RETURN(v, 1);
}

static inline int nvgpu_atomic_cmpxchg_impl(nvgpu_atomic_t *v, int old, int new)
{
	return NVGPU_POSIX_ATOMIC_CMPXCHG(v, old, new);
}

static inline int nvgpu_atomic_xchg_impl(nvgpu_atomic_t *v, int new)
{
	return NVGPU_POSIX_ATOMIC_XCHG(v, new);
}

static inline bool nvgpu_atomic_inc_and_test_impl(nvgpu_atomic_t *v)
{
	return NVGPU_POSIX_ATOMIC_ADD_RETURN(v, 1) == 0;
}

static inline bool nvgpu_atomic_dec_and_test_impl(nvgpu_atomic_t *v)
{
	return NVGPU_POSIX_ATOMIC_SUB_RETURN(v, 1) == 0;
}

static inline void nvgpu_atomic_sub_impl(int i, nvgpu_atomic_t *v)
{
	(void)NVGPU_POSIX_ATOMIC_SUB_RETURN(v, i);
}

static inline int nvgpu_atomic_sub_return_impl(int i, nvgpu_atomic_t *v)
{
	return NVGPU_POSIX_ATOMIC_SUB_RETURN(v, i);
}

static inline bool nvgpu_atomic_sub_and_test_impl(int i, nvgpu_atomic_t *v)
{
	return NVGPU_POSIX_ATOMIC_SUB_RETURN(v, i) == 0;
}

static inline void nvgpu_atomic_add_impl(int i, nvgpu_atomic_t *v)
{
	(void)NVGPU_POSIX_ATOMIC_ADD_RETURN(v, i);
}

static inline int nvgpu_atomic_add_return_impl(int i, nvgpu_atomic_t *v)
{
	return NVGPU_POSIX_ATOMIC_ADD_RETURN(v, i);
}

static inline int nvgpu_atomic_add_unless_impl(nvgpu_atomic_t *v, int a, int u)
{
	return NVGPU_POSIX_ATOMIC_ADD_UNLESS(v, a, u);
}

static inline void nvgpu_atomic64_set_impl(nvgpu_atomic64_t *v, long i)
{
	NVGPU_POSIX_ATOMIC_SET(v, i);
}

static inline long nvgpu_atomic64_read_impl(nvgpu_atomic64_t *v)
{
	return NVGPU_POSIX_ATOMIC_READ(v);
}

static inline void nvgpu_atomic64_add_impl(long x, nvgpu_atomic64_t *v)
{
	(void)NVGPU_POSIX_ATOMIC_ADD_RETURN(v, x);
}

static inline long nvgpu_atomic64_add_return_impl(long x, nvgpu_atomic64_t *v)
{
	return NVGPU_POSIX_ATOMIC_ADD_RETURN(v, x);
}

static inline long nvgpu_atomic64_add_unless_impl(nvgpu_atomic64_t *v, long a,
						long u)
{
	return NVGPU_POSIX_ATOMIC_ADD_UNLESS(v, a, u);
}

static inline void nvgpu_atomic64_inc_impl(nvgpu_atomic64_t *v)
{
	(void)NVGPU_POSIX_ATOMIC_ADD_RETURN(v, 1);
}

static inline long nvgpu_atomic64_inc_return_impl(nvgpu_atomic64_t *v)
{
	return NVGPU_POSIX_ATOMIC_ADD_RETURN(v, 1);
}

static inline bool nvgpu_atomic64_inc_and_test_impl(nvgpu_atomic64_t *v)
{
	return NVGPU_POSIX_ATOMIC_ADD_RETURN(v, 1) == 0;
}

static inline void nvgpu_atomic64_dec_impl(nvgpu_atomic64_t *v)
{
	(void)NVGPU_POSIX_ATOMIC_SUB_RETURN(v, 1);
}

static inline long nvgpu_atomic64_dec_return_impl(nvgpu_atomic64_t *v)
{
	return NVGPU_POSIX_ATOMIC_SUB_RETURN(v, 1);
}

static inline bool nvgpu_atomic64_dec_and_test_impl(nvgpu_atomic64_t *v)
{
	return NVGPU_POSIX_ATOMIC_SUB_RETURN(v, 1) == 0;
}

static inline long nvgpu_atomic64_xchg_impl(nvgpu_atomic64_t *v, long new)
{
	return NVGPU_POSIX_ATOMIC_XCHG(v, new);
}

static inline long nvgpu_atomic64_cmpxchg_impl(nvgpu_atomic64_t *v,
					long old, long new)
{
	return NVGPU_POSIX_ATOMIC_CMPXCHG(v, old, new);
}

static inline void nvgpu_atomic64_sub_impl(long x, nvgpu_atomic64_t *v)
{
	(void)NVGPU_POSIX_ATOMIC_SUB_RETURN(v, x);
}

static inline long nvgpu_atomic64_sub_return_impl(long x, nvgpu_atomic64_t *v)
{
	return NVGPU_POSIX_ATOMIC_SUB_RETURN(v, x);
}

static inline bool nvgpu_atomic64_sub_and_test_impl(long x, nvgpu_atomic64_t *v)
{
	return NVGPU_POSIX_ATOMIC_SUB_RETURN(v, x) == 0;
}

/*
 * The following is only used by the lockless allocator and makes direct use
 * of the cmpxchg function. For POSIX, this is translated to a call to
 * nvgpu_atomic_cmpxchg.
 */
#define cmpxchg(p, old, new) 						\
	({								\
		typeof(*(p)) tmp = (old);				\
									\
		(void) nvgpu_atomic_cmpxchg((nvgpu_atomic_t *) (p), tmp,\
			(new));						\
		tmp;							\
	})


#endif /* NVGPU_POSIX_ATOMIC_H */
