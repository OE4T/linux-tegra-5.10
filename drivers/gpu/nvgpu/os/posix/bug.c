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

#include <nvgpu/log.h>
#include <nvgpu/lock.h>
#include <nvgpu/list.h>
#include <nvgpu/posix/bug.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#ifndef _QNX_SOURCE
#include <execinfo.h>
#endif
#ifdef __NVGPU_UNIT_TEST__
#include <setjmp.h>
#include <signal.h>
#endif

#define BACKTRACE_MAXSIZE 1024

struct nvgpu_bug_desc {
	bool in_use;
	pthread_once_t once;
	struct nvgpu_spinlock lock;
	struct nvgpu_list_node head;
};

struct nvgpu_bug_desc bug = {
	.once = PTHREAD_ONCE_INIT
};

#ifdef __NVGPU_UNIT_TEST__
void nvgpu_bug_cb_longjmp(void *arg)
{
	nvgpu_info(NULL, "Expected BUG detected!");

	jmp_buf *jmp_handler = arg;
	longjmp(*jmp_handler, 1);
}
#endif

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

static void nvgpu_bug_init(void)
{
	nvgpu_err(NULL, "doing init for bug cb");
	nvgpu_spinlock_init(&bug.lock);
	nvgpu_init_list_node(&bug.head);
	bug.in_use = true;
}

void nvgpu_bug_exit(int status)
{
#ifndef __NVGPU_UNIT_TEST__
	exit(status);
#endif
}

void nvgpu_bug_register_cb(struct nvgpu_bug_cb *cb)
{
	(void) pthread_once(&bug.once, nvgpu_bug_init);
	nvgpu_spinlock_acquire(&bug.lock);
	nvgpu_list_add_tail(&cb->node, &bug.head);
	nvgpu_spinlock_release(&bug.lock);
}

void nvgpu_bug_unregister_cb(struct nvgpu_bug_cb *cb)
{
	(void) pthread_once(&bug.once, nvgpu_bug_init);
	nvgpu_spinlock_acquire(&bug.lock);
	nvgpu_list_del(&cb->node);
	nvgpu_spinlock_release(&bug.lock);
}

/*
 * Ahhh! A bug!
 */
void nvgpu_posix_bug(const char *fmt, ...)
{
	struct nvgpu_bug_cb *cb;

	/*
	 * If BUG was unexpected, raise a SIGSEGV signal, dump the stack and
	 * kill the thread.
	 */
	nvgpu_err(NULL, "BUG detected!");
	dump_stack();

	if (!bug.in_use) {
		goto done;
	}

	nvgpu_spinlock_acquire(&bug.lock);
	while (!nvgpu_list_empty(&bug.head)) {
		/*
		 * Always process first entry, in -unlikely- where a
		 * callback would unregister another one.
		 */
		cb = nvgpu_list_first_entry(&bug.head,
			nvgpu_bug_cb, node);
		/* Remove callback from list */
		nvgpu_list_del(&cb->node);
		/*
		 * Release spinlock before invoking callback.
		 * This allows callback to register/unregister other
		 * callbacks (unlikely).
		 * This allows using a longjmp in a callback
		 * for unit testing.
		 */
		nvgpu_spinlock_release(&bug.lock);
		cb->cb(cb->arg);
		nvgpu_spinlock_acquire(&bug.lock);
	}
	nvgpu_spinlock_release(&bug.lock);

done:
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
