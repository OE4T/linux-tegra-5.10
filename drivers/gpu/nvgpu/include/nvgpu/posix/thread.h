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

#ifndef NVGPU_POSIX_THREAD_H
#define NVGPU_POSIX_THREAD_H

#include <pthread.h>

#include <nvgpu/types.h>
#include <nvgpu/atomic.h>

/*
 * For some reason POSIX only allows 16 bytes of name length.
 */
#define NVGPU_THREAD_POSIX_MAX_NAMELEN		16

#define nvgpu_thread_cleanup_push(handler, data) \
			pthread_cleanup_push(handler, data)

#define nvgpu_thread_cleanup_pop() pthread_cleanup_pop(1)

#define nvgpu_getpid getpid

#define nvgpu_gettid pthread_self
/*
 * Handles passing an nvgpu thread function into a posix thread.
 */
struct nvgpu_posix_thread_data {
	int (*fn)(void *data);
	void *data;
};

struct nvgpu_thread {
	nvgpu_atomic_t running;
	bool should_stop;
	pthread_t thread;
	struct nvgpu_posix_thread_data nvgpu;
	char tname[NVGPU_THREAD_POSIX_MAX_NAMELEN];
};

int nvgpu_thread_create_priority(struct nvgpu_thread *thread,
			void *data, int (*threadfn)(void *data),
			int priority, const char *name);

#endif /* NVGPU_POSIX_THREAD_H */
