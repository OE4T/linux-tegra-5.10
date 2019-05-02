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

#ifndef NVGPU_POSIX_COND_H
#define NVGPU_POSIX_COND_H

#include <nvgpu/bug.h>
#include <nvgpu/lock.h>

struct nvgpu_cond {
	bool initialized;
	struct nvgpu_mutex mutex;
	pthread_cond_t cond;
	pthread_condattr_t attr;
};

int nvgpu_cond_timedwait(struct nvgpu_cond *c, unsigned int *ms);
void nvgpu_cond_signal_locked(struct nvgpu_cond *cond);
int nvgpu_cond_broadcast_locked(struct nvgpu_cond *cond);
void nvgpu_cond_lock(struct nvgpu_cond *cond);
void nvgpu_cond_unlock(struct nvgpu_cond *cond);

/**
 * NVGPU_COND_WAIT_LOCKED - Wait for a condition to be true
 *
 * @cond - The condition variable to sleep on
 * @condition - The condition that needs to be true
 * @timeout_ms - Timeout in milliseconds, or 0 for infinite wait.
 *               This parameter must be a u32.
 *
 * Wait for a condition to become true. Returns -ETIMEOUT if
 * the wait timed out with condition false.
 */
#define NVGPU_COND_WAIT_LOCKED(cond, condition, timeout_ms)	\
({								\
	int ret = 0;						\
	NVGPU_COND_WAIT_TIMEOUT_LOCKED(cond, condition, ret,	\
		timeout_ms ? timeout_ms : (unsigned int)-1);	\
	ret;							\
})

/**
 * NVGPU_COND_WAIT - Wait for a condition to be true
 *
 * @cond - The condition variable to sleep on
 * @condition - The condition that needs to be true
 * @timeout_ms - Timeout in milliseconds, or 0 for infinite wait.
 *               This parameter must be a u32.
 *
 * Wait for a condition to become true. Returns -ETIMEOUT if
 * the wait timed out with condition false.
 */
#define NVGPU_COND_WAIT(cond, condition, timeout_ms)			\
({									\
	int ret = 0;							\
	nvgpu_mutex_acquire(&(cond)->mutex);				\
	NVGPU_COND_WAIT_TIMEOUT_LOCKED((cond), (condition), (ret),	\
		(timeout_ms) ? (timeout_ms) : ((unsigned int)-1));	\
	nvgpu_mutex_release(&(cond)->mutex);				\
	ret;								\
})

/**
 * NVGPU_COND_WAIT_INTERRUPTIBLE - Wait for a condition to be true
 *
 * @cond - The condition variable to sleep on
 * @condition - The condition that needs to be true
 * @timeout_ms - Timeout in milliseconds, or 0 for infinite wait
 *
 * Wait for a condition to become true. Returns -ETIMEOUT if
 * the wait timed out with condition false or -ERESTARTSYS on
 * signal.
 */
#define NVGPU_COND_WAIT_INTERRUPTIBLE(cond, condition, timeout_ms) \
			NVGPU_COND_WAIT((cond), (condition), (timeout_ms))


#define NVGPU_COND_WAIT_TIMEOUT_LOCKED(cond, condition, ret, timeout_ms)\
do {									\
	unsigned int __timeout = timeout_ms;				\
	ret = 0;							\
	while (!(condition) && ret == 0) {				\
		ret = nvgpu_cond_timedwait(cond, &__timeout);		\
	}								\
} while (0)

#endif /* NVGPU_POSIX_COND_H */
