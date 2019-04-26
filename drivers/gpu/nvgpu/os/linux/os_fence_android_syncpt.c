/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <linux/err.h>
#include <nvgpu/errno.h>

#include <nvgpu/types.h>
#include <nvgpu/os_fence.h>
#include <nvgpu/os_fence_syncpts.h>
#include <nvgpu/linux/os_fence_android.h>
#include <nvgpu/nvhost.h>
#include <nvgpu/atomic.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/channel.h>
#include <nvgpu/channel_sync.h>

#include "../drivers/staging/android/sync.h"

static const struct nvgpu_os_fence_ops syncpt_ops = {
	.drop_ref = nvgpu_os_fence_android_drop_ref,
	.install_fence = nvgpu_os_fence_android_install_fd,
};

int nvgpu_os_fence_syncpt_create(
	struct nvgpu_os_fence *fence_out, struct channel_gk20a *c,
	struct nvgpu_nvhost_dev *nvhost_dev, u32 id, u32 thresh)
{
	struct sync_fence *fence = nvgpu_nvhost_sync_create_fence(
		nvhost_dev, id, thresh, "fence");

	if (IS_ERR(fence)) {
		nvgpu_err(c->g, "error %d during construction of fence.",
			(int)PTR_ERR(fence));
		return PTR_ERR(fence);
	}

	nvgpu_os_fence_init(fence_out, c->g, &syncpt_ops, fence);

	return 0;
}

int nvgpu_os_fence_syncpt_fdget(struct nvgpu_os_fence *fence_out,
	struct channel_gk20a *c, int fd)
{
	struct sync_fence *fence = nvgpu_nvhost_sync_fdget(fd);

	if (fence == NULL) {
		return -ENOMEM;
	}

	nvgpu_os_fence_init(fence_out, c->g, &syncpt_ops, fence);

	return 0;
}

int nvgpu_os_fence_get_syncpts(
	struct nvgpu_os_fence_syncpt *fence_syncpt_out,
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
	struct sync_fence *f = nvgpu_get_sync_fence(fence->fence);

	return (u32)f->num_fences;
}

void nvgpu_os_fence_syncpt_extract_nth_syncpt(
	struct nvgpu_os_fence_syncpt *fence, u32 n,
		u32 *syncpt_id, u32 *syncpt_threshold)
{

	struct sync_fence *f = nvgpu_get_sync_fence(fence->fence);
	struct sync_pt *pt = sync_pt_from_fence(f->cbs[n].sync_pt);

	*syncpt_id = nvgpu_nvhost_sync_pt_id(pt);
	*syncpt_threshold = nvgpu_nvhost_sync_pt_thresh(pt);
}
