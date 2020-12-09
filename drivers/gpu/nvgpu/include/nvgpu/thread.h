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

#ifndef NVGPU_THREAD_H
#define NVGPU_THREAD_H

#ifdef __KERNEL__
#include <nvgpu/linux/thread.h>
#else
#include <nvgpu/posix/thread.h>
#endif

#include <nvgpu/types.h>
#include <nvgpu/utils.h>

/**
 * @brief Create and run a new thread.
 *
 * @param thread [in]	Thread structure to use.
 * @param data [in]	Data to pass to threadfn.
 * @param threadfn [in]	Thread function.
 * @param name [in]	Name of the thread.
 *
 * Create a thread and run threadfn in it. The thread stays alive as long as
 * threadfn is running. As soon as threadfn returns the thread is destroyed.
 * The threadfn needs to continuously poll #nvgpu_thread_should_stop() to
 * determine if it should exit.
 *
 * @return Return 0 on success, otherwise returns error number to indicate the
 * error.
 *
 * @retval EINVAL invalid thread attribute object.
 * @retval EAGAIN insufficient system resources to create thread.
 * @retval EFAULT error occurred trying to access the buffers or the
 * start routine provided for thread creation.
 */
int nvgpu_thread_create(struct nvgpu_thread *thread,
		void *data,
		int (*threadfn)(void *data), const char *name);

/**
 * @brief Stop a thread.
 *
 * @param thread [in]	Thread to stop.
 *
 * A thread can be requested to stop by calling this function. This function
 * places a request to cancel the corresponding thread by setting a status and
 * waits until that thread exits.
 */
void nvgpu_thread_stop(struct nvgpu_thread *thread);

/**
 * @brief Request a thread to be stopped gracefully.
 *
 * @param thread [in]		Thread to stop.
 * @param thread_stop_fn [in]	Callback function to trigger graceful exit.
 * @param data [in]		Thread data.
 *
 * Request a thread to stop by setting the status of the thread appropriately.
 * In posix implementation, the callback function \a thread_stop_fn is invoked
 * and waits till the thread exits.
 */
void nvgpu_thread_stop_graceful(struct nvgpu_thread *thread,
		void (*thread_stop_fn)(void *data), void *data);

/**
 * @brief Query if thread should stop.
 *
 * @param thread [in]	Thread to be queried for stop status.
 *
 * Return true if thread should exit. Can be run only in the thread's own
 * context.
 *
 * @return Boolean value which indicates if the thread has to stop or not.
 *
 * @retval TRUE if the thread should stop.
 * @retval FALSE if the thread need not stop.
 */
bool nvgpu_thread_should_stop(struct nvgpu_thread *thread);

/**
 * @brief Query if thread is running.
 *
 * @param thread [in]	Thread to be queried for running status.
 *
 * Returns a boolean value based on the current running status of the thread.
 *
 * @return TRUE if the current running status of the thread is 1, else FALSE.
 *
 * @retval TRUE if the thread is running.
 * @retval FALSE if the thread is not running.
 */
bool nvgpu_thread_is_running(struct nvgpu_thread *thread);

/**
 * @brief Join a thread to reclaim resources after it has exited.
 *
 * @param thread [in] - Thread to join.
 *
 * The calling thread waits till the thread referenced by \a thread exits.
 */
void nvgpu_thread_join(struct nvgpu_thread *thread);

#endif /* NVGPU_THREAD_H */
