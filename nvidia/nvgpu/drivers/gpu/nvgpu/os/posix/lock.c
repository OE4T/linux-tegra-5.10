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

#include <nvgpu/bug.h>
#include <nvgpu/log.h>
#include <nvgpu/lock.h>

void nvgpu_mutex_init(struct nvgpu_mutex *mutex)
{
	int err = pthread_mutex_init(&mutex->lock.mutex, NULL);
	nvgpu_assert(err == 0);
}

void nvgpu_mutex_acquire(struct nvgpu_mutex *mutex)
{
	nvgpu_posix_lock_acquire(&mutex->lock);
}

void nvgpu_mutex_release(struct nvgpu_mutex *mutex)
{
	nvgpu_posix_lock_release(&mutex->lock);
}

int nvgpu_mutex_tryacquire(struct nvgpu_mutex *mutex)
{
	return ((nvgpu_posix_lock_try_acquire(&mutex->lock) == 0) ? 1 : 0);
}

void nvgpu_mutex_destroy(struct nvgpu_mutex *mutex)
{
	int err = pthread_mutex_destroy(&mutex->lock.mutex);
	nvgpu_assert(err == 0);
}

void nvgpu_spinlock_init(struct nvgpu_spinlock *spinlock)
{
	int err = pthread_mutex_init(&spinlock->lock.mutex, NULL);
	nvgpu_assert(err == 0);
}

void nvgpu_spinlock_acquire(struct nvgpu_spinlock *spinlock)
{
	nvgpu_posix_lock_acquire(&spinlock->lock);
}

void nvgpu_spinlock_release(struct nvgpu_spinlock *spinlock)
{
	nvgpu_posix_lock_release(&spinlock->lock);
}

void nvgpu_raw_spinlock_init(struct nvgpu_raw_spinlock *spinlock)
{
	int err = pthread_mutex_init(&spinlock->lock.mutex, NULL);
	nvgpu_assert(err == 0);
}

void nvgpu_raw_spinlock_acquire(struct nvgpu_raw_spinlock *spinlock)
{
	nvgpu_posix_lock_acquire(&spinlock->lock);
}

void nvgpu_raw_spinlock_release(struct nvgpu_raw_spinlock *spinlock)
{
	nvgpu_posix_lock_release(&spinlock->lock);
}
