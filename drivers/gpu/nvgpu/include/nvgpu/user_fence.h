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

static inline struct nvgpu_user_fence nvgpu_user_fence_init(void)
{
	return (struct nvgpu_user_fence) {
		.syncpt_id = NVGPU_INVALID_SYNCPT_ID,
	};
}

static inline void nvgpu_user_fence_release(struct nvgpu_user_fence *fence)
{
	if (nvgpu_os_fence_is_initialized(&fence->os_fence)) {
		fence->os_fence.ops->drop_ref(&fence->os_fence);
	}
}

#endif /* NVGPU_USER_FENCE_H */
