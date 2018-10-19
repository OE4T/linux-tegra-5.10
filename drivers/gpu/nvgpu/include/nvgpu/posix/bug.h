/*
 * Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef __NVGPU_POSIX_BUG_H__
#define __NVGPU_POSIX_BUG_H__

#include <nvgpu/types.h>
#include <setjmp.h>

/*
 * TODO: make these actually useful!
 */

#define BUG()					__bug("")
#define BUG_ON(cond)				\
	do {					\
		if (cond) {			\
			BUG();			\
		}				\
	} while (false)

#define WARN(cond, msg, arg...)			__warn(cond, msg, ##arg)
#define WARN_ON(cond)				__warn(cond, "")

#define WARN_ONCE(cond, msg, arg...)		\
	({static bool warn_once_warned = false;	\
	  if (!warn_once_warned) {		\
		  WARN(cond, msg, ##arg);	\
		  warn_once_warned = true;	\
	  }					\
	  cond; })


void dump_stack(void);

void __bug(const char *fmt, ...) __attribute__ ((noreturn));
bool __warn(bool cond, const char *fmt, ...);

/* Provide a simple API for BUG() handling */
void bug_handler_register(jmp_buf *handler);
void bug_handler_cancel(void);

/*
 * Macro to indicate that a BUG() call is expected when executing
 * the "code_to_run" block of code. The macro uses a statement expression
 * and the setjmp API to set a long jump point that gets called by the BUG()
 * function if enabled. This allows the macro to simply expand as true if
 * BUG() was called, and false otherwise.
 */

#define EXPECT_BUG(code_to_run)				\
	({						\
		jmp_buf handler;			\
		bool bug_result = true;			\
		if (!setjmp(handler)) {			\
			bug_handler_register(&handler);	\
			code_to_run;			\
			bug_handler_cancel();		\
			bug_result = false;		\
		}					\
		bug_result;				\
	})

#endif
