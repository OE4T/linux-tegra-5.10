/*
 * Copyright (c) 2018-2020, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_POSIX_BUG_H
#define NVGPU_POSIX_BUG_H

#include <nvgpu/types.h>
#ifdef __NVGPU_UNIT_TEST__
#include <setjmp.h>
#endif

/**
 * Macro to be used upon identifying a buggy behaviour in the code.
 * Implementation is specific to OS.  For QNX and Posix implementation
 * refer to nvgpu_posix_bug.
 */
#define BUG()		nvgpu_posix_bug(__func__, __LINE__)

/** Macro to handle a buggy code only upon meeting the condition. */
#define BUG_ON(cond)				\
	do {					\
		if (cond) {			\
			BUG();			\
		}				\
	} while (false)

/** Define for issuing warning on condition with message. */
#define WARN(cond, msg, arg...)			\
			((void) nvgpu_posix_warn(cond, msg, ##arg))
/** Define for issuing warning on condition. */
#define WARN_ON(cond)				\
			((void) nvgpu_posix_warn(cond, ""))

/** Define for issuing warning once on condition with message. */
#define WARN_ONCE(cond, msg, arg...)		\
	({static bool warn_once_warned = false;	\
	  if (!warn_once_warned) {		\
		  WARN(cond, msg, ##arg);	\
		  warn_once_warned = true;	\
	  }					\
	  cond; })


/**
 * @brief Dumps the stack.
 *
 * @param void.
 *
 * Dumps the call stack in user space build.
 */
void dump_stack(void);

/**
 * @brief Bug.
 *
 * @param msg [in]	Message to be printed in log.
 * @param line_no [in]	Line number.
 *
 * Function to be invoked upon identifying bug in the code.
 */
void nvgpu_posix_bug(const char *msg, int line_no) __attribute__ ((noreturn));

/**
 * @brief Issues Warning.
 *
 * @param cond [in]	Condition to check to issue warning.
 * @param fmt [in]	Format of variable argument list.
 * @param ... [in]	Variable length arguments.
 *
 * Used to report significant issues that needs prompt attention.
 * Warning is issued if the condition is met.
 *
 * @return Value of \a cond is returned.
 */
bool nvgpu_posix_warn(bool cond, const char *fmt, ...);

#ifdef __NVGPU_UNIT_TEST__
void nvgpu_bug_cb_longjmp(void *arg);

/**
 * Macro to indicate that a BUG() call is expected when executing
 * the "code_to_run" block of code. The macro uses a statement expression
 * and the setjmp API to set a long jump point that gets called by the BUG()
 * function if enabled. This allows the macro to simply expand as true if
 * BUG() was called, and false otherwise.
 * Note: it is safe to call nvgpu_bug_unregister_cb for a callback
 * that was already invoked/unregistered.
 */
#define EXPECT_BUG(code_to_run)					\
	({							\
		jmp_buf handler;				\
		volatile bool bug_result = true;		\
		struct nvgpu_bug_cb callback;			\
		callback.cb = nvgpu_bug_cb_longjmp;		\
		callback.arg = &handler;			\
		nvgpu_bug_register_cb(&callback);		\
		if (setjmp(handler) == 0) {			\
			code_to_run;				\
			bug_result = false;			\
		}						\
		nvgpu_bug_unregister_cb(&callback);		\
		bug_result;					\
	})
#endif

struct nvgpu_bug_cb;

/**
 * @brief Exit current process
 *
 * @param status [in]	Status to return
 *
 * This function is used during BUG() handling to exit
 * current process.
 */
void nvgpu_bug_exit(int status);

/**
 * @brief Register callback to be invoked on BUG()
 *
 * @param cb [in]	Pointer to callback structure
 *
 * Register a callback to be invoked on BUG().
 * The nvgpu_bug_cb structure contains a function pointer
 * and an argument to be passed to this function.
 * This mechanism can be used to perform some emergency
 * operations on a GPU before exiting the process.
 *
 * Note: callback is automatically unregistered before
 * being invoked.
 */
void nvgpu_bug_register_cb(struct nvgpu_bug_cb *cb);

/**
 * @brief Unregister a callback for BUG()
 *
 * @param cb [in]	Pointer to callback structure
 *
 * Remove a callback from the list of callbacks to be
 * invoked on BUG().
 */
void nvgpu_bug_unregister_cb(struct nvgpu_bug_cb *cb);

#endif /* NVGPU_POSIX_BUG_H */
