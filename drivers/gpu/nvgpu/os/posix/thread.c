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

#include <nvgpu/bug.h>
#include <nvgpu/thread.h>

/**
 * Use pthreads to mostly emulate the Linux kernel APIs. There are some things
 * that are quite different - especially the stop/should_stop notions. In user
 * space threads can send signals to one another but of course within the kernel
 * that is not as simple.
 *
 * This could use some nice debugging some day as well.
 */

/*
 * nvgpu thread functions return int. POSIX threads return void *. This little
 * wrapper takes the int returning nvgpu thread and instead passes that int back
 * through the void * pointer.
 */
static void *nvgpu_posix_thread_wrapper(void *data)
{
	struct nvgpu_posix_thread_data *nvgpu =
				(struct nvgpu_posix_thread_data *)data;

	return ERR_PTR(nvgpu->fn(nvgpu->data));
}

static void nvgpu_thread_cancel_sync(struct nvgpu_thread *thread)
{
	(void) pthread_cancel(thread->thread);
}

int nvgpu_thread_create(struct nvgpu_thread *thread,
			void *data,
			int (*threadfn)(void *data), const char *name)
{
	pthread_attr_t attr;
	int ret;

	(void) memset(thread, 0, sizeof(*thread));

	/*
	 * By subtracting 1 the above memset ensures that we have a zero
	 * terminated string.
	 */
	if (name != NULL) {
		(void) strncpy(thread->tname, name,
			NVGPU_THREAD_POSIX_MAX_NAMELEN - 1);
	}

	thread->nvgpu.data = data;
	thread->nvgpu.fn = threadfn;

	nvgpu_atomic_set(&thread->running, 1);
	ret = pthread_attr_init(&attr);
	if (ret != 0) {
		return ret;
	}

	ret = pthread_create(&thread->thread, &attr,
			     nvgpu_posix_thread_wrapper,
			     &thread->nvgpu);
	if (ret != 0) {
		(void) pthread_attr_destroy(&attr);
		return ret;
	}

#ifdef _GNU_SOURCE
	pthread_setname_np(thread->thread, thread->tname);
#endif

	ret = pthread_attr_destroy(&attr);
	if (ret != 0) {
		(void) pthread_cancel(thread->thread);
		return ret;
	}

	return 0;
}

int nvgpu_thread_create_priority(struct nvgpu_thread *thread,
			void *data, int (*threadfn)(void *data),
			int priority, const char *name)
{
	pthread_attr_t attr;
	struct sched_param param;
	int ret;

	(void) memset(thread, 0, sizeof(*thread));
	(void) memset(&param, 0, sizeof(struct sched_param));

	/*
	 * By subtracting 1 the above memset ensures that we have a zero
	 * terminated string.
	 */
	if (name != NULL) {
		(void) strncpy(thread->tname, name,
			NVGPU_THREAD_POSIX_MAX_NAMELEN - 1);
	}

	thread->nvgpu.data = data;
	thread->nvgpu.fn = threadfn;

	nvgpu_atomic_set(&thread->running, 1);

	ret = pthread_attr_init(&attr);
	if (ret != 0) {
		return ret;
	}

	ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	if (ret != 0) {
		(void) pthread_attr_destroy(&attr);
		return ret;
	}

	ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
	if (ret != 0) {
		(void) pthread_attr_destroy(&attr);
		return ret;
	}

	param.sched_priority = priority;
	ret = pthread_attr_setschedparam(&attr, &param);
	if (ret != 0) {
		(void) pthread_attr_destroy(&attr);
		return ret;
	}

	ret = pthread_create(&thread->thread, &attr,
			nvgpu_posix_thread_wrapper, &thread->nvgpu);
	if (ret != 0) {
		(void) pthread_attr_destroy(&attr);
		return ret;
	}

#ifdef _GNU_SOURCE
	pthread_setname_np(thread->thread, thread->tname);
#endif

	ret = pthread_attr_destroy(&attr);
	if (ret != 0) {
		(void) pthread_cancel(thread->thread);
		return ret;
	}

	return 0;
}

void nvgpu_thread_stop(struct nvgpu_thread *thread)
{
	int old = nvgpu_atomic_cmpxchg(&thread->running, 1, 0);

	if (old != 0) {
		nvgpu_thread_cancel_sync(thread);
		nvgpu_thread_join(thread);
	}
}

void nvgpu_thread_stop_graceful(struct nvgpu_thread *thread,
		void (*thread_stop_fn)(void *data), void *data)
{
	int old = nvgpu_atomic_cmpxchg(&thread->running, 1, 0);

	if (old != 0) {
		if (thread_stop_fn != NULL) {
			thread_stop_fn(data);
		}
		nvgpu_thread_join(thread);
	}
}

bool nvgpu_thread_should_stop(struct nvgpu_thread *thread)
{
	return (nvgpu_atomic_read(&thread->running) == 0);
}

bool nvgpu_thread_is_running(struct nvgpu_thread *thread)
{
	return (nvgpu_atomic_read(&thread->running) == 1);
}

void nvgpu_thread_join(struct nvgpu_thread *thread)
{
	(void) pthread_join(thread->thread, NULL);
}
