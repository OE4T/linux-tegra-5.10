/*
 * Copyright (c) 2020, NVIDIA Corporation.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NVGPU_USER_FENCE_H
#define NVGPU_USER_FENCE_H

#include <nvgpu/nvhost.h>
#include <nvgpu/os_fence.h>

/*
 * A post-submit fence to be given to userspace. Either the syncpt id and value
 * pair is valid or the os fence is valid; this depends on the flags that were
 * used: NVGPU_SUBMIT_GPFIFO_FLAGS_SYNC_FENCE implies os fence.
 */
struct nvgpu_user_fence {
	u32 syncpt_id, syncpt_value;
	struct nvgpu_os_fence os_fence;
};

/*
 * Initialize an empty fence that acts like a null pointer.
 */
static inline struct nvgpu_user_fence nvgpu_user_fence_init(void)
{
	return (struct nvgpu_user_fence) {
		.syncpt_id = NVGPU_INVALID_SYNCPT_ID,
	};
}

/*
 * Copy a fence, incrementing a refcount (if any) of the underlying object.
 * This needs to be balanced with a nvgpu_user_fence_release().
 *
 * It's OK to call this for an empty fence.
 */
static inline struct nvgpu_user_fence nvgpu_user_fence_clone(
		struct nvgpu_user_fence *f)
{
	if (nvgpu_os_fence_is_initialized(&f->os_fence)) {
		f->os_fence.ops->dup(&f->os_fence);
	}
	return *f;
}

/*
 * Decrement the refcount of the underlying fence, if any, and make this fence
 * behave like a null pointer.
 */
static inline void nvgpu_user_fence_release(struct nvgpu_user_fence *fence)
{
	if (nvgpu_os_fence_is_initialized(&fence->os_fence)) {
		fence->os_fence.ops->drop_ref(&fence->os_fence);
	}
	fence->syncpt_id = NVGPU_INVALID_SYNCPT_ID;
	fence->syncpt_value = 0;
}

#endif /* NVGPU_USER_FENCE_H */
