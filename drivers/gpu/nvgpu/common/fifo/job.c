/*
 * Copyright (c) 2011-2020, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/log.h>
#include <nvgpu/lock.h>
#include <nvgpu/kmem.h>
#include <nvgpu/barrier.h>
#include <nvgpu/circ_buf.h>
#include <nvgpu/channel.h>
#include <nvgpu/job.h>
#include <nvgpu/priv_cmdbuf.h>
#include <nvgpu/fence.h>

static inline struct nvgpu_channel_job *
channel_gk20a_job_from_list(struct nvgpu_list_node *node)
{
	return (struct nvgpu_channel_job *)
	((uintptr_t)node - offsetof(struct nvgpu_channel_job, list));
};

int nvgpu_channel_alloc_job(struct nvgpu_channel *c,
		struct nvgpu_channel_job **job_out)
{
	unsigned int put = c->joblist.pre_alloc.put;
	unsigned int get = c->joblist.pre_alloc.get;
	unsigned int next = (put + 1) % c->joblist.pre_alloc.length;
	bool full = next == get;

	if (full) {
		return -EAGAIN;
	}

	*job_out = &c->joblist.pre_alloc.jobs[put];

	return 0;
}

void nvgpu_channel_free_job(struct nvgpu_channel *c,
		struct nvgpu_channel_job *job)
{
	(void) memset(job, 0, sizeof(*job));
}

void nvgpu_channel_joblist_lock(struct nvgpu_channel *c)
{
	nvgpu_mutex_acquire(&c->joblist.pre_alloc.read_lock);
}

void nvgpu_channel_joblist_unlock(struct nvgpu_channel *c)
{
	nvgpu_mutex_release(&c->joblist.pre_alloc.read_lock);
}

struct nvgpu_channel_job *channel_joblist_peek(struct nvgpu_channel *c)
{
	struct nvgpu_channel_job *job = NULL;

	if (!nvgpu_channel_joblist_is_empty(c)) {
		unsigned int get = c->joblist.pre_alloc.get;

		job = &c->joblist.pre_alloc.jobs[get];
	}

	return job;
}

void channel_joblist_add(struct nvgpu_channel *c,
		struct nvgpu_channel_job *job)
{
	c->joblist.pre_alloc.put = (c->joblist.pre_alloc.put + 1U) %
			(c->joblist.pre_alloc.length);
}

void channel_joblist_delete(struct nvgpu_channel *c,
		struct nvgpu_channel_job *job)
{
	c->joblist.pre_alloc.get = (c->joblist.pre_alloc.get + 1U) %
			(c->joblist.pre_alloc.length);
}

bool nvgpu_channel_joblist_is_empty(struct nvgpu_channel *c)
{
	return c->joblist.pre_alloc.get == c->joblist.pre_alloc.put;
}

int channel_prealloc_resources(struct nvgpu_channel *c, u32 num_jobs)
{
	int err;
	u32 size;

	size = (u32)sizeof(struct nvgpu_channel_job);
	if (num_jobs > nvgpu_safe_sub_u32(U32_MAX / size, 1U)) {
		err = -ERANGE;
		goto clean_up;
	}

	/*
	 * The max capacity of this ring buffer is the alloc size minus one (in
	 * units of item slot), so allocate a size of (num_jobs + 1) * size
	 * bytes.
	 */
	c->joblist.pre_alloc.jobs = nvgpu_vzalloc(c->g,
			nvgpu_safe_mult_u32(
				nvgpu_safe_add_u32(num_jobs, 1U),
				size));
	if (c->joblist.pre_alloc.jobs == NULL) {
		err = -ENOMEM;
		goto clean_up;
	}

	err = nvgpu_fence_pool_alloc(c, num_jobs);
	if (err != 0) {
		goto clean_up;
	}

	/*
	 * length is the allocation size of the ringbuffer; the number of jobs
	 * that fit is one less.
	 */
	c->joblist.pre_alloc.length = nvgpu_safe_add_u32(num_jobs, 1U);
	c->joblist.pre_alloc.put = 0;
	c->joblist.pre_alloc.get = 0;

	return 0;

clean_up:
	nvgpu_vfree(c->g, c->joblist.pre_alloc.jobs);
	(void) memset(&c->joblist.pre_alloc, 0, sizeof(c->joblist.pre_alloc));
	return err;
}

void channel_free_prealloc_resources(struct nvgpu_channel *c)
{
	if (c->joblist.pre_alloc.jobs != NULL) {
		nvgpu_vfree(c->g, c->joblist.pre_alloc.jobs);
		c->joblist.pre_alloc.jobs = NULL;
		nvgpu_fence_pool_free(c);
	}
}
