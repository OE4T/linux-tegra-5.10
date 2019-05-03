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
#ifndef NVGPU_ATOMIC_H
#define NVGPU_ATOMIC_H

#ifdef __KERNEL__
#include <nvgpu/linux/atomic.h>
#else
#include <nvgpu/posix/atomic.h>
#endif

#define NVGPU_ATOMIC_INIT(i)	nvgpu_atomic_init_impl(i)
#define NVGPU_ATOMIC64_INIT(i)	nvgpu_atomic64_init_impl(i)

static inline void nvgpu_atomic_set(nvgpu_atomic_t *v, int i)
{
	nvgpu_atomic_set_impl(v, i);
}
static inline int nvgpu_atomic_read(nvgpu_atomic_t *v)
{
	return nvgpu_atomic_read_impl(v);
}
static inline void nvgpu_atomic_inc(nvgpu_atomic_t *v)
{
	nvgpu_atomic_inc_impl(v);
}
static inline int nvgpu_atomic_inc_return(nvgpu_atomic_t *v)
{
	return nvgpu_atomic_inc_return_impl(v);
}
static inline void nvgpu_atomic_dec(nvgpu_atomic_t *v)
{
	 nvgpu_atomic_dec_impl(v);
}
static inline int nvgpu_atomic_dec_return(nvgpu_atomic_t *v)
{
	return nvgpu_atomic_dec_return_impl(v);
}
static inline int nvgpu_atomic_cmpxchg(nvgpu_atomic_t *v, int old, int new)
{
	return nvgpu_atomic_cmpxchg_impl(v, old, new);
}
static inline int nvgpu_atomic_xchg(nvgpu_atomic_t *v, int new)
{
	return nvgpu_atomic_xchg_impl(v, new);
}
static inline bool nvgpu_atomic_inc_and_test(nvgpu_atomic_t *v)
{
	return nvgpu_atomic_inc_and_test_impl(v);
}
static inline bool nvgpu_atomic_dec_and_test(nvgpu_atomic_t *v)
{
	return nvgpu_atomic_dec_and_test_impl(v);
}
static inline bool nvgpu_atomic_sub_and_test(int i, nvgpu_atomic_t *v)
{
	return nvgpu_atomic_sub_and_test_impl(i, v);
}
static inline void nvgpu_atomic_add(int i, nvgpu_atomic_t *v)
{
	nvgpu_atomic_add_impl(i, v);
}
static inline int nvgpu_atomic_sub_return(int i, nvgpu_atomic_t *v)
{
	return nvgpu_atomic_sub_return_impl(i, v);
}
static inline void nvgpu_atomic_sub(int i, nvgpu_atomic_t *v)
{
	nvgpu_atomic_sub_impl(i, v);
}
static inline int nvgpu_atomic_add_return(int i, nvgpu_atomic_t *v)
{
	return nvgpu_atomic_add_return_impl(i, v);
}
static inline int nvgpu_atomic_add_unless(nvgpu_atomic_t *v, int a, int u)
{
	return nvgpu_atomic_add_unless_impl(v, a, u);
}
static inline void nvgpu_atomic64_set(nvgpu_atomic64_t *v, long x)
{
	return  nvgpu_atomic64_set_impl(v, x);
}
static inline long nvgpu_atomic64_read(nvgpu_atomic64_t *v)
{
	return  nvgpu_atomic64_read_impl(v);
}
static inline void nvgpu_atomic64_add(long x, nvgpu_atomic64_t *v)
{
	nvgpu_atomic64_add_impl(x, v);
}
static inline void nvgpu_atomic64_inc(nvgpu_atomic64_t *v)
{
	nvgpu_atomic64_inc_impl(v);
}
static inline long nvgpu_atomic64_inc_return(nvgpu_atomic64_t *v)
{
	return nvgpu_atomic64_inc_return_impl(v);
}
static inline void nvgpu_atomic64_dec(nvgpu_atomic64_t *v)
{
	nvgpu_atomic64_dec_impl(v);
}
static inline long nvgpu_atomic64_dec_return(nvgpu_atomic64_t *v)
{
	return nvgpu_atomic64_dec_return_impl(v);
}
static inline long nvgpu_atomic64_xchg(nvgpu_atomic64_t *v, long new)
{
	return nvgpu_atomic64_xchg_impl(v, new);
}
static inline long nvgpu_atomic64_cmpxchg(nvgpu_atomic64_t *v, long old,
					long new)
{
	return nvgpu_atomic64_cmpxchg_impl(v, old, new);
}
static inline long nvgpu_atomic64_add_return(long x, nvgpu_atomic64_t *v)
{
	return nvgpu_atomic64_add_return_impl(x, v);
}
static inline long nvgpu_atomic64_add_unless(nvgpu_atomic64_t *v, long a,
						long u)
{
	return nvgpu_atomic64_add_unless_impl(v, a, u);
}
static inline void nvgpu_atomic64_sub(long x, nvgpu_atomic64_t *v)
{
	nvgpu_atomic64_sub_impl(x, v);
}
static inline bool nvgpu_atomic64_inc_and_test(nvgpu_atomic64_t *v)
{
	return nvgpu_atomic64_inc_and_test_impl(v);
}
static inline bool nvgpu_atomic64_dec_and_test(nvgpu_atomic64_t *v)
{
	return nvgpu_atomic64_dec_and_test_impl(v);
}
static inline bool nvgpu_atomic64_sub_and_test(long x, nvgpu_atomic64_t *v)
{
	return nvgpu_atomic64_sub_and_test_impl(x, v);
}
static inline long nvgpu_atomic64_sub_return(long x, nvgpu_atomic64_t *v)
{
	return nvgpu_atomic64_sub_return_impl(x, v);
}

#endif /* NVGPU_ATOMIC_H */
