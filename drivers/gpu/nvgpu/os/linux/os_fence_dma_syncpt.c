/*
 * Copyright (c) 2020, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/errno.h>
#include <nvgpu/types.h>
#include <nvgpu/os_fence.h>
#include <nvgpu/os_fence_syncpts.h>
#include <nvgpu/linux/os_fence_dma.h>
#include <nvgpu/nvhost.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/channel.h>

static const struct nvgpu_os_fence_ops syncpt_ops = {
	.drop_ref = nvgpu_os_fence_dma_drop_ref,
	.install_fence = nvgpu_os_fence_dma_install_fd,
};

int nvgpu_os_fence_syncpt_create(struct nvgpu_os_fence *fence_out,
		struct nvgpu_channel *c, struct nvgpu_nvhost_dev *nvhost_dev,
		u32 id, u32 thresh)
{
	return -ENOSYS;
}

int nvgpu_os_fence_syncpt_fdget(struct nvgpu_os_fence *fence_out,
		struct nvgpu_channel *c, int fd)
{
	return -ENOSYS;
}

int nvgpu_os_fence_get_syncpts(struct nvgpu_os_fence_syncpt *fence_syncpt_out,
		struct nvgpu_os_fence *fence_in)
{
	if (fence_in->ops != &syncpt_ops) {
		return -EINVAL;
	}

	fence_syncpt_out->fence = fence_in;

	return 0;
}

u32 nvgpu_os_fence_syncpt_get_num_syncpoints(
		struct nvgpu_os_fence_syncpt *fence)
{
	WARN(1, "can't get here until nvhost support exists");
	return -EINVAL;
}

void nvgpu_os_fence_syncpt_extract_nth_syncpt(
		struct nvgpu_os_fence_syncpt *fence, u32 n,
		u32 *syncpt_id, u32 *syncpt_threshold)
{
	WARN(1, "can't get here until nvhost support exists");
	*syncpt_id = 0;
	*syncpt_threshold = 0;
}
