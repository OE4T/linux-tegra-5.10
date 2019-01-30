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

#include <nvgpu/cond.h>

#include <nvgpu/posix/cond.h>

int nvgpu_cond_init(struct nvgpu_cond *cond)
{
	int ret;
	ret = nvgpu_mutex_init(&cond->mutex);
	if (ret != 0) {
		return ret;
	}
	ret = pthread_cond_init(&cond->cond, NULL);
	if (ret != 0) {
		nvgpu_mutex_destroy(&cond->mutex);
		return ret;
	}
	cond->initialized = true;
	return ret;
}

void nvgpu_cond_signal(struct nvgpu_cond *cond)
{
	if (cond == NULL || !cond->initialized) {
		BUG();
	}
	nvgpu_mutex_acquire(&cond->mutex);
	(void) pthread_cond_signal(&cond->cond);
	nvgpu_mutex_release(&cond->mutex);
}

void nvgpu_cond_signal_interruptible(struct nvgpu_cond *cond)
{
	if (cond == NULL || !cond->initialized) {
		BUG();
	}
	nvgpu_mutex_acquire(&cond->mutex);
	(void) pthread_cond_signal(&cond->cond);
	nvgpu_mutex_release(&cond->mutex);
}

int nvgpu_cond_broadcast(struct nvgpu_cond *cond)
{
	int ret;
	if (cond == NULL || !cond->initialized) {
		BUG();
	}
	nvgpu_mutex_acquire(&cond->mutex);
	ret = pthread_cond_broadcast(&cond->cond);
	nvgpu_mutex_release(&cond->mutex);
	return ret;
}

int nvgpu_cond_broadcast_interruptible(struct nvgpu_cond *cond)
{
	int ret;
	if (cond == NULL || !cond->initialized) {
		BUG();
	}
	nvgpu_mutex_acquire(&cond->mutex);
	ret = pthread_cond_broadcast(&cond->cond);
	nvgpu_mutex_release(&cond->mutex);
	return ret;
}

void nvgpu_cond_destroy(struct nvgpu_cond *cond)
{
	if (cond == NULL || !cond->initialized) {
		BUG();
	}
	nvgpu_mutex_destroy(&cond->mutex);
	pthread_cond_destroy(&cond->cond);
	cond->initialized = false;
	return;
}
