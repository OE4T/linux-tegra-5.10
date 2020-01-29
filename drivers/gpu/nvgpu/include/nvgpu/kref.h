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

/*
 * The following structure is used for reference counting of objects in nvgpu.
 */
#ifndef NVGPU_KREF_H
#define NVGPU_KREF_H

#include <nvgpu/atomic.h>
#include <nvgpu/types.h>
#include <nvgpu/utils.h>

struct nvgpu_ref {
	/**
	 * Atomic reference count.
	 */
	nvgpu_atomic_t refcount;
};

/**
 * @brief Initialize the reference object.
 *
 * @param ref [in]	The nvgpu_ref object to initialize.
 *
 * Initializes the reference object pointed by \a ref.
 */
static inline void nvgpu_ref_init(struct nvgpu_ref *ref)
{
	nvgpu_atomic_set(&ref->refcount, 1);
}

/**
 * @brief Increment the reference count.
 *
 * @param ref [in]	The nvgpu_ref object.
 *
 * Increment reference count for the object
 */
static inline void nvgpu_ref_get(struct nvgpu_ref *ref)
{
	nvgpu_atomic_inc(&ref->refcount);
}

/**
 * @brief Decrement the reference count.
 *
 * @param ref[in]	The nvgpu_ref object.
 * @param release [in]	Pointer to the function that would be invoked to
 * 			clean up the object when the reference count becomes
 * 			zero.
 *
 * Decrement reference count for the object and call release() if it becomes
 * zero.
 */
static inline void nvgpu_ref_put(struct nvgpu_ref *ref,
		void (*release)(struct nvgpu_ref *r))
{
	if (nvgpu_atomic_sub_and_test(1, &ref->refcount)) {
		if (release != NULL) {
			release(ref);
		}
	}
}

/**
 * @brief Decrement reference count for the object, call release() if it
 * becomes zero and return the status of the removal.
 *
 * @param ref [in]	The nvgpu_ref object.
 * @param release [in]	Pointer to the function that would be invoked to
 * 			clean up the object when the reference count becomes
 * 			zero, i.e. the last reference corresponding to this
 * 			object is removed.
 *
 * Decrement the reference count for the object pointed by \a ref, invokes the
 * callback function \a release if it becomes zero and returns the status of
 * the removal.
 *
 * @return Return 1 if object was removed, otherwise return 0. The user should
 * not make any assumptions about the status of the object in the memory when
 * the function returns 0 and should only use it to know that there are no
 * further references to this object.
 */
static inline int nvgpu_ref_put_return(struct nvgpu_ref *ref,
		void (*release)(struct nvgpu_ref *r))
{
	if (nvgpu_atomic_sub_and_test(1, &ref->refcount)) {
		if (release != NULL) {
			release(ref);
		}
		return 1;
	}
	return 0;
}

/**
 * @brief Increment reference count of the object unless it is zero.
 *
 * @param ref [in]	The nvgpu_ref object.
 *
 * Increment the reference count of the object unless it is zero.
 *
 * @return Return non-zero if the increment succeeds, Otherwise return 0.
 */
static inline int nvgpu_ref_get_unless_zero(struct nvgpu_ref *ref)
{
	return nvgpu_atomic_add_unless(&ref->refcount, 1, 0);
}

#endif /* NVGPU_KREF_H */
