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

#include <pthread.h>
#include <nvgpu/rwsem.h>
#include <nvgpu/log.h>
#ifndef CONFIG_NVGPU_NON_FUSA
#include <nvgpu/bug.h>
#endif

void nvgpu_rwsem_init(struct nvgpu_rwsem *rwsem)
{
	int err = pthread_rwlock_init(&rwsem->rw_sem, NULL);
	if (err != 0) {
		nvgpu_err(NULL, "OS API pthread_rwlock_init error = %d", err);
	}
#ifndef CONFIG_NVGPU_NON_FUSA
	nvgpu_assert(err == 0);
#endif
}

/*
 * Acquire.
 */
void nvgpu_rwsem_down_read(struct nvgpu_rwsem *rwsem)
{
	int err = pthread_rwlock_rdlock(&rwsem->rw_sem);
	if (err != 0) {
		nvgpu_err(NULL, "OS API pthread_rwlock_rdlock err = %d", err);
	}
#ifndef CONFIG_NVGPU_NON_FUSA
	nvgpu_assert(err == 0);
#endif
}

/*
 * Release.
 */
void nvgpu_rwsem_up_read(struct nvgpu_rwsem *rwsem)
{
	int err = pthread_rwlock_unlock(&rwsem->rw_sem);
	if (err != 0) {
		nvgpu_err(NULL, "OS API pthread_rwlock_unlock err = %d", err);
	}
#ifndef CONFIG_NVGPU_NON_FUSA
	nvgpu_assert(err == 0);
#endif
}

void nvgpu_rwsem_down_write(struct nvgpu_rwsem *rwsem)
{
	int err = pthread_rwlock_wrlock(&rwsem->rw_sem);
	if (err != 0) {
		nvgpu_err(NULL, "OS API pthread_rwlock_wrlock err = %d", err);
	}
#ifndef CONFIG_NVGPU_NON_FUSA
	nvgpu_assert(err == 0);
#endif
}

void nvgpu_rwsem_up_write(struct nvgpu_rwsem *rwsem)
{
	int err = pthread_rwlock_unlock(&rwsem->rw_sem);
	if (err != 0) {
		nvgpu_err(NULL, "OS API pthread_rwlock_unlock err = %d", err);
	}
#ifndef CONFIG_NVGPU_NON_FUSA
	nvgpu_assert(err == 0);
#endif
}
