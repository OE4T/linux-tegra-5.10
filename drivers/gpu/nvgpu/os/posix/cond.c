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

#include <nvgpu/cond.h>

int nvgpu_cond_init(struct nvgpu_cond *cond)
{
	int ret;

	ret = pthread_condattr_init(&cond->attr);
	if (ret != 0) {
		return ret;
	}
	ret = pthread_condattr_setclock(&cond->attr, CLOCK_MONOTONIC);
	if (ret != 0) {
		(void) pthread_condattr_destroy(&cond->attr);
		return ret;
	}

	nvgpu_mutex_init(&cond->mutex);

	ret = pthread_cond_init(&cond->cond, &cond->attr);
	if (ret != 0) {
		(void) pthread_condattr_destroy(&cond->attr);
		(void) nvgpu_mutex_destroy(&cond->mutex);
		return ret;
	}

	cond->initialized = true;
	return ret;
}

void nvgpu_cond_signal(struct nvgpu_cond *cond)
{
	if ((cond == NULL) || !(cond->initialized)) {
		BUG();
	}
	nvgpu_mutex_acquire(&cond->mutex);
	(void) pthread_cond_signal(&cond->cond);
	nvgpu_mutex_release(&cond->mutex);
}

void nvgpu_cond_signal_interruptible(struct nvgpu_cond *cond)
{
	if ((cond == NULL) || !(cond->initialized)) {
		BUG();
	}
	nvgpu_mutex_acquire(&cond->mutex);
	(void) pthread_cond_signal(&cond->cond);
	nvgpu_mutex_release(&cond->mutex);
}

int nvgpu_cond_broadcast(struct nvgpu_cond *cond)
{
	int ret;

	if ((cond == NULL) || !(cond->initialized)) {
		return -EINVAL;
	}
	nvgpu_mutex_acquire(&cond->mutex);
	ret = pthread_cond_broadcast(&cond->cond);
	nvgpu_mutex_release(&cond->mutex);
	return ret;
}

int nvgpu_cond_broadcast_interruptible(struct nvgpu_cond *cond)
{
	int ret;

	if ((cond == NULL) || !(cond->initialized)) {
		return -EINVAL;
	}
	nvgpu_mutex_acquire(&cond->mutex);
	ret = pthread_cond_broadcast(&cond->cond);
	nvgpu_mutex_release(&cond->mutex);
	return ret;
}

void nvgpu_cond_destroy(struct nvgpu_cond *cond)
{
	if (cond == NULL) {
		BUG();
	}
	(void) pthread_cond_destroy(&cond->cond);
	nvgpu_mutex_destroy(&cond->mutex);
	(void) pthread_condattr_destroy(&cond->attr);
	cond->initialized = false;
}

void nvgpu_cond_signal_locked(struct nvgpu_cond *cond)
{
	if ((cond == NULL) || !(cond->initialized)) {
		BUG();
	}
	(void) pthread_cond_signal(&cond->cond);
}

int nvgpu_cond_broadcast_locked(struct nvgpu_cond *cond)
{
	if (!cond->initialized) {
		return -EINVAL;
	}

	return pthread_cond_broadcast(&cond->cond);
}

void nvgpu_cond_lock(struct nvgpu_cond *cond)
{
	nvgpu_mutex_acquire(&cond->mutex);
}

void nvgpu_cond_unlock(struct nvgpu_cond *cond)
{
	nvgpu_mutex_release(&cond->mutex);
}

int nvgpu_cond_timedwait(struct nvgpu_cond *c, unsigned int *ms)
{
	int ret;
	s64 t_start_ns, t_ns;
	struct timespec ts;

	if (*ms == NVGPU_COND_WAIT_TIMEOUT_MAX_MS) {
		return pthread_cond_wait(&c->cond, &c->mutex.lock.mutex);
	}

	if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
		return -EFAULT;
	}

	t_start_ns = nvgpu_safe_mult_s64(ts.tv_sec, 1000000000);
	t_start_ns = nvgpu_safe_add_s64(t_start_ns, ts.tv_nsec);
	t_ns = (s64)(*ms);
	t_ns *= 1000000;
	t_ns =  nvgpu_safe_add_s64(t_ns, t_start_ns);
	ts.tv_sec = t_ns / 1000000000;
	ts.tv_nsec = t_ns % 1000000000;

	ret = pthread_cond_timedwait(&c->cond, &c->mutex.lock.mutex, &ts);
	if (ret == 0) {
		if (clock_gettime(CLOCK_MONOTONIC, &ts) != -1) {
			t_ns = nvgpu_safe_mult_s64(ts.tv_sec, 1000000000);
			t_ns = nvgpu_safe_add_s64(t_ns, ts.tv_nsec);
			t_ns = nvgpu_safe_sub_s64(t_ns, t_start_ns);
			t_ns /= 1000000;
			if ((s64)*ms <= t_ns) {
				*ms = 0;
			} else {
				*ms -= (unsigned int)t_ns;
			}
		}
	}
	return ret;
}
