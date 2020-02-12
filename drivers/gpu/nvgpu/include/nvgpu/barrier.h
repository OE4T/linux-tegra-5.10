/*
 * Copyright (c) 2017-2020, NVIDIA CORPORATION.  All rights reserved.
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

/* This file contains NVGPU_* high-level abstractions for various
 * memor-barrier operations available in linux/kernel. Every OS
 * should provide their own OS specific calls under this common API
 */

#ifndef NVGPU_BARRIER_H
#define NVGPU_BARRIER_H

#ifdef __KERNEL__
#include <nvgpu/linux/barrier.h>
#elif defined(__NVGPU_POSIX__)
#include <nvgpu/posix/barrier.h>
#else
#include <nvgpu_rmos/include/barrier.h>
#endif

#define nvgpu_mb()	nvgpu_mb_impl()
#define nvgpu_rmb()	nvgpu_rmb_impl()
#define nvgpu_wmb()	nvgpu_wmb_impl()

#define nvgpu_smp_mb()	nvgpu_smp_mb_impl()
#define nvgpu_smp_rmb()	nvgpu_smp_rmb_impl()
#define nvgpu_smp_wmb()	nvgpu_smp_wmb_impl()

#define NV_READ_ONCE(x)		NV_READ_ONCE_IMPL((x))
#define NV_WRITE_ONCE(x, y)	NV_WRITE_ONCE_IMPL((x), (y))

/*
 * Sometimes we want to prevent speculation.
 */
#ifdef __NVGPU_PREVENT_UNTRUSTED_SPECULATION
#define nvgpu_speculation_barrier()	nvgpu_speculation_barrier_impl()
#else
#define nvgpu_speculation_barrier()
#endif

#endif /* NVGPU_BARRIER_H */
