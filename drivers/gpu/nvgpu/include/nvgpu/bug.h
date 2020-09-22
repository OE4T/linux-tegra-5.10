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
#ifndef NVGPU_BUG_H
#define NVGPU_BUG_H

#ifdef __KERNEL__
#include <linux/bug.h>
#else
#include <nvgpu/posix/bug.h>
#endif
#include <nvgpu/cov_whitelist.h>
#include <nvgpu/list.h>

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
 * When this assert fails, the function will not return.
 */
#define nvgpu_assert(cond)											\
	({													\
		NVGPU_COV_WHITELIST_BLOCK_BEGIN(false_positive, 1, NVGPU_MISRA(Rule, 14_4), "Bug 2277532")	\
		NVGPU_COV_WHITELIST_BLOCK_BEGIN(false_positive, 1, NVGPU_MISRA(Rule, 15_6), "Bug 2277532")	\
		BUG_ON((cond) == ((bool)(0 != 0)));										\
		NVGPU_COV_WHITELIST_BLOCK_END(NVGPU_MISRA(Rule, 14_4))						\
		NVGPU_COV_WHITELIST_BLOCK_END(NVGPU_MISRA(Rule, 15_6))						\
	})
#endif

/*
 * Define simple macros to force the consequences of a failed assert
 * (presumably done in a previous if statement).
 * The exact behavior will be OS dependent. See above.
 */
#define nvgpu_do_assert()						\
NVGPU_COV_WHITELIST(false_positive, NVGPU_MISRA(Rule, 10_3), "Bug 2623654") \
	nvgpu_assert((bool)(0 != 0))

/*
 * Define compile-time assert check.
 */
#define ASSERT_CONCAT(a, b) a##b
#define ASSERT_ADD_INFO(a, b) ASSERT_CONCAT(a, b)
#define nvgpu_static_assert(e)						\
	enum {								\
		ASSERT_ADD_INFO(assert_line_, __LINE__) = 1 / (!!(e))	\
	}

struct gk20a;

#define nvgpu_do_assert_print(g, fmt, arg...)				\
	do {								\
		nvgpu_err(g, fmt, ##arg);				\
		nvgpu_do_assert();					\
	} while (false)


struct nvgpu_bug_cb
{
	void (*cb)(void *arg);
	void *arg;
	struct nvgpu_list_node node;
};

static inline struct nvgpu_bug_cb *
nvgpu_bug_cb_from_node(struct nvgpu_list_node *node)
{
	return (struct nvgpu_bug_cb *)
		((uintptr_t)node - offsetof(struct nvgpu_bug_cb, node));
};

#ifdef __KERNEL__
static inline void nvgpu_bug_exit(int status) { }
static inline void nvgpu_bug_register_cb(struct nvgpu_bug_cb *cb) { }
static inline void nvgpu_bug_unregister_cb(struct nvgpu_bug_cb *cb) { }
#endif

#endif /* NVGPU_BUG_H */
