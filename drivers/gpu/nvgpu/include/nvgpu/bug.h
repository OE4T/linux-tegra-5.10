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
#ifndef NVGPU_BUG_H
#define NVGPU_BUG_H

#ifdef __KERNEL__
#include <linux/bug.h>
#else
#include <nvgpu/posix/bug.h>
#endif
#include <nvgpu/cov_whitelist.h>

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
#define nvgpu_assert(cond)	((void) WARN_ON(!(cond)))
#else
/*
 * A static inline for POSIX/QNX/etc so that we can hide the branch in BUG_ON()
 * from the branch analyzer. This prevents unit testing branch analysis from
 * marking this as an untested branch everywhere the nvgpu_assert() macro is
 * used. Similarly for MISRA this hides the messy BUG_ON() macro from users of
 * this function. This means the MISRA scanner will only trigger 1 issue for
 * this instead of 1 for every place it's used.
 *
 * When this assert fails the function will not return.
 */
static inline void nvgpu_assert(bool cond)
{
NVGPU_COV_WHITELIST_BLOCK_BEGIN(false_positive, 1, NVGPU_MISRA(Rule, 14_4), "Bug 2277532")
NVGPU_COV_WHITELIST_BLOCK_BEGIN(false_positive, 1, NVGPU_MISRA(Rule, 15_6), "Bug 2277532")
	BUG_ON(!cond);
NVGPU_COV_WHITELIST_BLOCK_END(NVGPU_MISRA(Rule, 14_4))
NVGPU_COV_WHITELIST_BLOCK_END(NVGPU_MISRA(Rule, 15_6))
}
#endif

/*
 * Define simple macros to force the consequences of a failed assert
 * (presumably done in a previous if statement).
 * The exact behavior will be OS dependent. See above.
 */
#define nvgpu_do_assert()	nvgpu_assert(false)

/*
 * Define compile-time assert check.
 */
#define ASSERT_CONCAT_(a, b) a##b
#define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)
#define nvgpu_static_assert(e)						\
	enum {								\
		ASSERT_CONCAT(assert_line_, __LINE__) = 1 / (!!(e))	\
	}

struct gk20a;

#define nvgpu_do_assert_print(g, fmt, arg...)				\
	do {								\
		nvgpu_err(g, fmt, ##arg);				\
		nvgpu_do_assert();					\
	} while (false)

#endif /* NVGPU_BUG_H */
