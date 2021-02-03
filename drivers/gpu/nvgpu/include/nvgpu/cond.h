/*
 * Copyright (c) 2017-2021, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_COND_H
#define NVGPU_COND_H

#ifdef __KERNEL__
#include <nvgpu/linux/cond.h>
#else
#include <nvgpu/posix/cond.h>
#endif

/*
 * struct nvgpu_cond
 *
 * Should be implemented per-OS in a separate library
 */
struct nvgpu_cond;

/**
 * @brief Initialize a condition variable.
 *
 * @param cond [in]	The condition variable to initialize.
 *
 * Initialize a condition variable before using it.
 *
 * @return If successful, this function returns 0. Otherwise, an error number
 * is returned to indicate the error.
 *
 * @retval ENOMEM for insufficient memory.
 * @retval EINVAL for invalid value.
 * @retval EBUSY for a pthread condition variable pointer to a previously
 * initialized condition variable that hasn't been destroyed.
 * @retval EFAULT for any faults that occurred while trying to access the
 * condition variable or attribute.
 */
int nvgpu_cond_init(struct nvgpu_cond *cond);

/**
 * @brief Signal a condition variable.
 *
 * @param cond [in]	The condition variable to signal.
 * 			  - Should not to be equal to NULL.
 * 			  - Structure pointed by \a cond should be initialized
 * 			    before invoking this function.
 *
 * This function is used to unblock a thread blocked on a condition variable.
 * Wakes up at least one of the threads that are blocked on the specified
 * condition variable \a cond.
 */
void nvgpu_cond_signal(struct nvgpu_cond *cond);

/**
 * @brief Signal a condition variable.
 *
 * @param cond [in]	The condition variable to signal.
 * 			  - Should not to be equal to NULL.
 * 			  - Structure pointed by \a cond should be initialized
 * 			    before invoking this function.
 *
 * Wakes up at least one of the threads that are blocked on the specified
 * condition variable \a cond. In posix implementation, the function provides
 * the same functionality as #nvgpu_cond_signal, but this function is being
 * provided to be congruent with kernel implementations having interruptible
 * and uninterruptible waits.
 */
void nvgpu_cond_signal_interruptible(struct nvgpu_cond *cond);

/**
 * @brief Signal all waiters of a condition variable.
 *
 * @param cond [in]	The condition variable to broadcast.
 * 			  - Should not to be equal to NULL.
 * 			  - Structure pointed by \a cond should be initialized
 * 			    before invoking this function.
 *
 * This function is used to unblock threads blocked on a condition variable.
 * Wakes up all the threads that are blocked on the specified condition variable
 * \a cond.
 *
 * @return If successful, this function returns 0. Otherwise, an error number
 * is returned to indicate the error.
 *
 * @retval -EINVAL if \a cond is NULL or not initialized.
 * @retval EFAULT a fault occurred while trying to access the buffers provided.
 * @retval EINVAL for invalid condition variable. This error value is generated
 * by the OS API used inside this function, which is returned as it is.
 */
int nvgpu_cond_broadcast(struct nvgpu_cond *cond);

/**
 * @brief Signal all waiters of a condition variable.
 *
 * @param cond [in]	The condition variable to broadcast.
 * 			  - Should not to be equal to NULL.
 * 			  - Structure pointed by \a cond should be initialized
 * 			    before invoking this function.
 *
 * This function is used to unblock threads blocked on a condition variable.
 * Wakes up all the threads that are blocked on the specified condition variable
 * \a cond. In Posix implementation this API is same as #nvgpu_cond_broadcast in
 * functionality, but the API is provided to be congruent with implementations
 * having interruptible and uninterruptible waits.
 *
 * @return If successful, this function returns 0. Otherwise, an error number
 * is returned to indicate the error.
 *
 * @retval -EINVAL if \a cond is NULL or not initialized.
 * @retval EFAULT a fault occurred while trying to access the buffers provided.
 * @retval EINVAL for invalid condition variable. This error value is generated
 * by the OS API used inside this function, which is returned as it is.
 */
int nvgpu_cond_broadcast_interruptible(struct nvgpu_cond *cond);

/**
 * @brief Destroy a condition variable.
 *
 * @param cond [in]	The condition variable to destroy.
 * 			  - Should not to be equal to NULL.
 *
 * Destroys the condition variable along with the associated attributes and
 * mutex.
 */
void nvgpu_cond_destroy(struct nvgpu_cond *cond);

#endif /* NVGPU_COND_H */
