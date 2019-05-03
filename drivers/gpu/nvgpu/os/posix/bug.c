/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/log.h>
#include <nvgpu/posix/bug.h>
#include <signal.h>
#include <pthread.h>
#include <stdbool.h>
#include <setjmp.h>
#include <stdlib.h>
#ifndef _QNX_SOURCE
#include <execinfo.h>
#endif

#define BACKTRACE_MAXSIZE 1024

static _Thread_local bool expect_bug;
static _Thread_local jmp_buf *jmp_handler;

void bug_handler_register(jmp_buf *handler)
{
	expect_bug = true;
	jmp_handler = handler;
}

void bug_handler_cancel(void)
{
	expect_bug = false;
	jmp_handler = NULL;
}

static void nvgpu_posix_dump_stack(int skip_frames)
{
#ifndef _QNX_SOURCE
	void *trace[BACKTRACE_MAXSIZE];
	int i;
	int trace_size = backtrace(trace, BACKTRACE_MAXSIZE);
	char **trace_syms = backtrace_symbols(trace, trace_size);

	for (i = skip_frames; i < trace_size; i++) {
		nvgpu_err(NULL, "[%d] %s", i - skip_frames, trace_syms[i]);
	}

	free(trace_syms);
#endif
	return;
}

void dump_stack(void)
{
	/* Skip this function and nvgpu_posix_dump_stack() */
	nvgpu_posix_dump_stack(2);
}

/*
 * Ahhh! A bug!
 */
void nvgpu_posix_bug(const char *fmt, ...)
{
	if (expect_bug) {
		nvgpu_info(NULL, "Expected BUG detected!");
		expect_bug = false;
		/* Perform a long jump to where "setjmp()" was called. */
		longjmp(*jmp_handler, 1);
	}
	/*
	 * If BUG was unexpected, raise a SIGSEGV signal, dump the stack and
	 * kill the thread.
	 */
	nvgpu_err(NULL, "BUG detected!");
	dump_stack();
	(void) raise(SIGSEGV);
	pthread_exit(NULL);
}

bool nvgpu_posix_warn(bool cond, const char *fmt, ...)
{
	if (!cond) {
		goto done;
	}

	nvgpu_warn(NULL, "WARNING detected!");

	dump_stack();

done:
	return cond;
}
