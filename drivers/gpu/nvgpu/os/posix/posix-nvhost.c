/*
 * Copyright (c) 2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/gk20a.h>
#include <nvgpu/bug.h>
#include <nvgpu/nvhost.h>

int nvgpu_get_nvhost_dev(struct gk20a *g)
{
	BUG();
	return 0;
}

void nvgpu_free_nvhost_dev(struct gk20a *g)
{
	BUG();
}

int nvgpu_nvhost_module_busy_ext(
	struct nvgpu_nvhost_dev *nvhost_dev)
{
	BUG();
	return 0;
}

void nvgpu_nvhost_module_idle_ext(
	struct nvgpu_nvhost_dev *nvhost_dev)
{
	BUG();
}

void nvgpu_nvhost_debug_dump_device(
	struct nvgpu_nvhost_dev *nvhost_dev)
{
	BUG();
}

const char *nvgpu_nvhost_syncpt_get_name(
	struct nvgpu_nvhost_dev *nvhost_dev, int id)
{
	BUG();
	return NULL;
}

bool nvgpu_nvhost_syncpt_is_valid_pt_ext(
	struct nvgpu_nvhost_dev *nvhost_dev, u32 id)
{
	BUG();
	return false;
}

int nvgpu_nvhost_syncpt_is_expired_ext(
	struct nvgpu_nvhost_dev *nvhost_dev, u32 id, u32 thresh)
{
	BUG();
	return 0;
}

u32 nvgpu_nvhost_syncpt_incr_max_ext(
	struct nvgpu_nvhost_dev *nvhost_dev, u32 id, u32 incrs)
{
	BUG();
	return 0U;
}

int nvgpu_nvhost_intr_register_notifier(
	struct nvgpu_nvhost_dev *nvhost_dev, u32 id, u32 thresh,
	void (*callback)(void *, int), void *private_data)
{
	BUG();
	return 0;
}

void nvgpu_nvhost_syncpt_set_min_eq_max_ext(
	struct nvgpu_nvhost_dev *nvhost_dev, u32 id)
{
	BUG();
}

void nvgpu_nvhost_syncpt_put_ref_ext(
	struct nvgpu_nvhost_dev *nvhost_dev, u32 id)
{
	BUG();
}

u32 nvgpu_nvhost_get_syncpt_host_managed(
	struct nvgpu_nvhost_dev *nvhost_dev,
	u32 param, const char *syncpt_name)
{
	BUG();
	return 0U;
}

u32 nvgpu_nvhost_get_syncpt_client_managed(
	struct nvgpu_nvhost_dev *nvhost_dev,
	const char *syncpt_name)
{
	BUG();
	return 0U;
}

int nvgpu_nvhost_syncpt_wait_timeout_ext(
	struct nvgpu_nvhost_dev *nvhost_dev, u32 id,
	u32 thresh, u32 timeout, u32 *value, struct timespec *ts)
{
	BUG();
	return 0;
}

int nvgpu_nvhost_syncpt_read_ext_check(
	struct nvgpu_nvhost_dev *nvhost_dev, u32 id, u32 *val)
{
	BUG();
	return 0;
}

u32 nvgpu_nvhost_syncpt_read_maxval(
	struct nvgpu_nvhost_dev *nvhost_dev, u32 id)
{
	BUG();
	return 0U;
}

void nvgpu_nvhost_syncpt_set_safe_state(
	struct nvgpu_nvhost_dev *nvhost_dev, u32 id)
{
	BUG();
}

int nvgpu_nvhost_create_symlink(struct gk20a *g)
{
	BUG();
	return 0;
}

void nvgpu_nvhost_remove_symlink(struct gk20a *g)
{
	BUG();
}

#ifdef CONFIG_SYNC
u32 nvgpu_nvhost_sync_pt_id(struct sync_pt *pt)
{
	BUG();
	return 0U;
}

u32 nvgpu_nvhost_sync_pt_thresh(struct sync_pt *pt)
{
	BUG();
	return 0U;
}

struct sync_fence *nvgpu_nvhost_sync_fdget(int fd)
{
	BUG();
	return NULL;
}

int nvgpu_nvhost_sync_num_pts(struct sync_fence *fence)
{
	BUG();
	return 0;
}

struct sync_fence *nvgpu_nvhost_sync_create_fence(
	struct nvgpu_nvhost_dev *nvhost_dev,
	u32 id, u32 thresh, const char *name)
{
	BUG();
	return NULL;
}
#endif /* CONFIG_SYNC */

#ifdef CONFIG_TEGRA_T19X_GRHOST
int nvgpu_nvhost_syncpt_unit_interface_get_aperture(
		struct nvgpu_nvhost_dev *nvhost_dev,
		u64 *base, size_t *size)
{
	BUG();
	return 0;
}

u32 nvgpu_nvhost_syncpt_unit_interface_get_byte_offset(u32 syncpt_id)
{
	BUG();
	return 0U;
}

int nvgpu_nvhost_syncpt_init(struct gk20a *g)
{
	return -ENOSYS;
}
#endif
