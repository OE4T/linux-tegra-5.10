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

#ifndef NVGPU_POSIX_LOCK_H
#define NVGPU_POSIX_LOCK_H

#include <stdlib.h>

#include <pthread.h>
#include <nvgpu/log.h>
#include <nvgpu/bug.h>

/*
 * All locks for posix nvgpu are just pthread locks. There's not a lot of reason
 * to have real spinlocks in userspace since we aren't using real HW or running
 * perf critical code where a sleep could be devestating.
 *
 * This could be revisited later, though.
 */
struct __nvgpu_posix_lock {
	/** Pthread mutex structure used internally to implement lock */
	pthread_mutex_t mutex;
};

/**
 * @brief Acquire the lock.
 *
 * @param lock [in]	Lock to acquire.
 *
 * Internal implementation of lock acquire used by public APIs of mutex,
 * spinlock and raw spinlock. Uses pthread_mutex_lock to acquire the lock.
 */
static inline void nvgpu_posix_lock_acquire(struct __nvgpu_posix_lock *lock)
{
	int err = pthread_mutex_lock(&lock->mutex);
	nvgpu_assert(err == 0);
}

/**
 * @brief Attempt to acquire the lock.
 *
 * @param lock [in]	Lock to acquire.
 *
 * Internal implementation of lock try and acquire used by public mutex APIs.
 * Uses pthread_mutex_trylock to try and acquire the lock.
 *
 * @return Returns 0 on success; otherwise, returns error number.
 */
static inline int nvgpu_posix_lock_try_acquire(
	struct __nvgpu_posix_lock *lock)
{
	return pthread_mutex_trylock(&lock->mutex);
}

/**
 * @brief Release the lock.
 *
 * @param lock [in]	Lock to release.
 *
 * Internal implementation of lock release used by public APIs of mutex,
 * spinlock and raw spinlock. Uses pthread_mutex_unlock to release the lock.
 */
static inline void nvgpu_posix_lock_release(struct __nvgpu_posix_lock *lock)
{
	int err = pthread_mutex_unlock(&lock->mutex);

	if (err != 0) {
		nvgpu_err(NULL, "OS API pthread_mutex_unlock error = %d", err);
	}
}

struct nvgpu_mutex {
	/**
	 * nvgpu lock structure used to implement mutex APIs.
	 */
	struct __nvgpu_posix_lock lock;
};

struct nvgpu_spinlock {
	/**
	 * nvgpu lock structure used to implement spinlock APIs.
	 */
	struct __nvgpu_posix_lock lock;
};

struct nvgpu_raw_spinlock {
	/**
	 * nvgpu lock structure used to implement raw spinlock APIs.
	 */
	struct __nvgpu_posix_lock lock;
};

static inline void nvgpu_spinlock_irqsave(struct nvgpu_spinlock *mutex,
					  unsigned long flags)
{
	nvgpu_posix_lock_acquire(&mutex->lock);
}

static inline void nvgpu_spinunlock_irqrestore(struct nvgpu_spinlock *mutex,
					       unsigned long flags)
{
	nvgpu_posix_lock_release(&mutex->lock);
}

#endif /* NVGPU_POSIX_LOCK_H */
