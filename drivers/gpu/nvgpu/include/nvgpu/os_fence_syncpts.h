/*
 * nvgpu os fence syncpts
 *
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

#ifndef NVGPU_OS_FENCE_SYNCPT_H
#define NVGPU_OS_FENCE_SYNCPT_H

struct nvgpu_os_fence;

struct nvgpu_os_fence_syncpt {
	struct nvgpu_os_fence *fence;
};

#if defined(CONFIG_TEGRA_GK20A_NVHOST) && !defined(CONFIG_NVGPU_SYNCFD_NONE)
/*
 * Return a struct of nvgpu_os_fence_syncpt only if the underlying os_fence
 * object is backed by syncpoints, else return empty object.
 */
int nvgpu_os_fence_get_syncpts(struct nvgpu_os_fence_syncpt *fence_syncpt_out,
	struct nvgpu_os_fence *fence_in);

/*
 * This method returns the nth syncpt id and syncpt threshold as *syncpt_id
 * and *syncpt_threshold respectively and should be called on a valid
 * instance of type nvgpu_os_fence_syncpt.
 */
void nvgpu_os_fence_syncpt_extract_nth_syncpt(
	struct nvgpu_os_fence_syncpt *fence, u32 n,
		u32 *syncpt_id, u32 *syncpt_threshold);


/*
 * This method returns the number of underlying syncpoints
 * and should be called only on a valid instance of type
 * nvgpu_os_fence_syncpt.
 */
u32 nvgpu_os_fence_syncpt_get_num_syncpoints(
	struct nvgpu_os_fence_syncpt *fence);

#else

static inline int nvgpu_os_fence_get_syncpts(
	struct nvgpu_os_fence_syncpt *fence_syncpt_out,
	struct nvgpu_os_fence *fence_in)
{
	return -EINVAL;
}

static inline void nvgpu_os_fence_syncpt_extract_nth_syncpt(
	struct nvgpu_os_fence_syncpt *fence, u32 n,
		u32 *syncpt_id, u32 *syncpt_threshold)
{
}

static inline u32 nvgpu_os_fence_syncpt_get_num_syncpoints(
	struct nvgpu_os_fence_syncpt *fence)
{
	return 0;
}

#endif

#endif /* NVGPU_OS_FENCE_SYNPT_H */
