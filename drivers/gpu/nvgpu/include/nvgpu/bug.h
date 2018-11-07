/*
 * Copyright (c) 2017-2018, NVIDIA CORPORATION.  All rights reserved.
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
#ifndef NVGPU_BUG_H
#define NVGPU_BUG_H

#ifdef __KERNEL__
#include <linux/bug.h>
#elif defined(__NVGPU_POSIX__)
#include <nvgpu/posix/bug.h>
#else
#include <nvgpu_rmos/include/bug.h>
#endif

/*
 * Define an assert macro that code within nvgpu can use.
 *
 * The goal of this macro is for debugging but what that means varies from OS
 * to OS. On Linux wee don't want to BUG() for general driver misbehaving. BUG()
 * is a very heavy handed tool - in fact there's probably no where within the
 * nvgpu core code where it makes sense to use a BUG() when running under Linux.
 *
 * However, on QNX (and POSIX) BUG() will just kill the current process. This
 * means we can use it for handling bugs in nvgpu.
 *
 * As a result this macro varies depending on platform.
 */
#if defined(__KERNEL__)
#define nvgpu_assert(cond)	WARN_ON(!(cond))
#elif defined(__NVGPU_POSIX__)
/*
 * A static inline for POSIX so that we can hide the branch in BUG_ON() from the
 * branch analysis for users of this function. When this assert fails the
 * function will not return.
 */
static inline void nvgpu_assert(bool cond)
{
	BUG_ON(!cond);
}
#else
#define nvgpu_assert(cond)	BUG_ON(!(cond))
#endif

#endif /* NVGPU_BUG_H */
