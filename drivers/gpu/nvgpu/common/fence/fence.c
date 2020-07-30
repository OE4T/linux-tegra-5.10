/*
 * Copyright (c) 2014-2020, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/kmem.h>
#include <nvgpu/soc.h>
#include <nvgpu/nvhost.h>
#include <nvgpu/barrier.h>
#include <nvgpu/os_fence.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/channel.h>
#include <nvgpu/semaphore.h>
#include <nvgpu/fence.h>
#include <nvgpu/channel_sync_syncpt.h>
#include <nvgpu/user_fence.h>

static struct nvgpu_fence_type *nvgpu_fence_from_ref(struct nvgpu_ref *ref)
{
	return (struct nvgpu_fence_type *)((uintptr_t)ref -
				offsetof(struct nvgpu_fence_type, ref));
}

static void nvgpu_fence_free(struct nvgpu_ref *ref)
{
	struct nvgpu_fence_type *f = nvgpu_fence_from_ref(ref);

	if (nvgpu_os_fence_is_initialized(&f->os_fence)) {
		f->os_fence.ops->drop_ref(&f->os_fence);
	}

#ifdef CONFIG_NVGPU_SW_SEMAPHORE
	if (f->semaphore != NULL) {
		nvgpu_semaphore_put(f->semaphore);
	}
#endif
}

void nvgpu_fence_put(struct nvgpu_fence_type *f)
{
	nvgpu_ref_put(&f->ref, nvgpu_fence_free);
}

struct nvgpu_fence_type *nvgpu_fence_get(struct nvgpu_fence_type *f)
{
	nvgpu_ref_get(&f->ref);
	return f;
}

/*
 * Extract an object to be passed to the userspace as a result of a submitted
 * job. This must be balanced with a call to nvgpu_user_fence_release().
 */
struct nvgpu_user_fence nvgpu_fence_extract_user(struct nvgpu_fence_type *f)
{
	struct nvgpu_user_fence uf = (struct nvgpu_user_fence) {
		.syncpt_id = f->syncpt_id,
		.syncpt_value = f->syncpt_value,
		.os_fence = f->os_fence,
	};

	/*
	 * The os fence member has to live so it can be signaled when the job
	 * completes. The returned user fence may live longer than that before
	 * being safely attached to an fd if the job completes before a
	 * submission ioctl finishes, or if it's stored for cde job state
	 * tracking.
	 */
	if (nvgpu_os_fence_is_initialized(&f->os_fence)) {
		f->os_fence.ops->dup(&f->os_fence);
	}

	return uf;
}

int nvgpu_fence_wait(struct gk20a *g, struct nvgpu_fence_type *f,
							u32 timeout)
{
	if (!nvgpu_platform_is_silicon(g)) {
		timeout = U32_MAX;
	}
	return f->ops->wait(f, timeout);
}

bool nvgpu_fence_is_expired(struct nvgpu_fence_type *f)
{
	return f->ops->is_expired(f);
}

void nvgpu_fence_init(struct nvgpu_fence_type *f,
		const struct nvgpu_fence_ops *ops,
		struct nvgpu_os_fence os_fence)
{
	nvgpu_ref_init(&f->ref);
	f->ops = ops;
	f->syncpt_id = NVGPU_INVALID_SYNCPT_ID;
#ifdef CONFIG_NVGPU_SW_SEMAPHORE
	f->semaphore = NULL;
#endif
	f->os_fence = os_fence;
}

#ifdef CONFIG_NVGPU_SW_SEMAPHORE
/* Fences that are backed by GPU semaphores: */

static int nvgpu_semaphore_fence_wait(struct nvgpu_fence_type *f, u32 timeout)
{
	if (!nvgpu_semaphore_is_acquired(f->semaphore)) {
		return 0;
	}

	return NVGPU_COND_WAIT_INTERRUPTIBLE(
		f->semaphore_wq,
		!nvgpu_semaphore_is_acquired(f->semaphore),
		timeout);
}

static bool nvgpu_semaphore_fence_is_expired(struct nvgpu_fence_type *f)
{
	return !nvgpu_semaphore_is_acquired(f->semaphore);
}

static const struct nvgpu_fence_ops nvgpu_semaphore_fence_ops = {
	.wait = &nvgpu_semaphore_fence_wait,
	.is_expired = &nvgpu_semaphore_fence_is_expired,
};

/* This function takes ownership of the semaphore as well as the os_fence */
void nvgpu_fence_from_semaphore(
		struct nvgpu_fence_type *f,
		struct nvgpu_semaphore *semaphore,
		struct nvgpu_cond *semaphore_wq,
		struct nvgpu_os_fence os_fence)
{
	nvgpu_fence_init(f, &nvgpu_semaphore_fence_ops, os_fence);

	f->semaphore = semaphore;
	f->semaphore_wq = semaphore_wq;
}

#endif

#ifdef CONFIG_TEGRA_GK20A_NVHOST
/* Fences that are backed by host1x syncpoints: */

static int nvgpu_fence_syncpt_wait(struct nvgpu_fence_type *f, u32 timeout)
{
	return nvgpu_nvhost_syncpt_wait_timeout_ext(
			f->nvhost_dev, f->syncpt_id, f->syncpt_value,
			timeout, NVGPU_NVHOST_DEFAULT_WAITER);
}

static bool nvgpu_fence_syncpt_is_expired(struct nvgpu_fence_type *f)
{

	/*
	 * In cases we don't register a notifier, we can't expect the
	 * syncpt value to be updated. For this case, we force a read
	 * of the value from HW, and then check for expiration.
	 */
	if (!nvgpu_nvhost_syncpt_is_expired_ext(f->nvhost_dev, f->syncpt_id,
				f->syncpt_value)) {
		u32 val;

		if (!nvgpu_nvhost_syncpt_read_ext_check(f->nvhost_dev,
				f->syncpt_id, &val)) {
			return nvgpu_nvhost_syncpt_is_expired_ext(
					f->nvhost_dev,
					f->syncpt_id, f->syncpt_value);
		}
	}

	return true;
}

static const struct nvgpu_fence_ops nvgpu_fence_syncpt_ops = {
	.wait = &nvgpu_fence_syncpt_wait,
	.is_expired = &nvgpu_fence_syncpt_is_expired,
};

/* This function takes the ownership of the os_fence */
void nvgpu_fence_from_syncpt(
		struct nvgpu_fence_type *f,
		struct nvgpu_nvhost_dev *nvhost_dev,
		u32 id, u32 value, struct nvgpu_os_fence os_fence)
{
	nvgpu_fence_init(f, &nvgpu_fence_syncpt_ops, os_fence);

	f->nvhost_dev = nvhost_dev;
	f->syncpt_id = id;
	f->syncpt_value = value;
}
#endif
