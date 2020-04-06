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
	int err = 0;

	if (nvgpu_channel_is_prealloc_enabled(c)) {
		unsigned int put = c->joblist.pre_alloc.put;
		unsigned int get = c->joblist.pre_alloc.get;
		unsigned int next = (put + 1) % c->joblist.pre_alloc.length;
		bool full = next == get;

		/*
		 * ensure all subsequent reads happen after reading get.
		 * see corresponding nvgpu_smp_wmb in
		 * nvgpu_channel_clean_up_jobs()
		 */
		nvgpu_smp_rmb();

		if (!full) {
			*job_out = &c->joblist.pre_alloc.jobs[put];
		} else {
			nvgpu_warn(c->g,
					"out of job ringbuffer space");
			err = -EAGAIN;
		}
	} else {
		*job_out = nvgpu_kzalloc(c->g,
					 sizeof(struct nvgpu_channel_job));
		if (*job_out == NULL) {
			err = -ENOMEM;
		}
	}

	return err;
}

void nvgpu_channel_free_job(struct nvgpu_channel *c,
		struct nvgpu_channel_job *job)
{
	if (nvgpu_channel_is_prealloc_enabled(c)) {
		(void) memset(job, 0, sizeof(*job));
	} else {
		nvgpu_kfree(c->g, job);
	}
}

void nvgpu_channel_joblist_lock(struct nvgpu_channel *c)
{
	if (nvgpu_channel_is_prealloc_enabled(c)) {
		nvgpu_mutex_acquire(&c->joblist.pre_alloc.read_lock);
	} else {
		nvgpu_spinlock_acquire(&c->joblist.dynamic.lock);
	}
}

void nvgpu_channel_joblist_unlock(struct nvgpu_channel *c)
{
	if (nvgpu_channel_is_prealloc_enabled(c)) {
		nvgpu_mutex_release(&c->joblist.pre_alloc.read_lock);
	} else {
		nvgpu_spinlock_release(&c->joblist.dynamic.lock);
	}
}

struct nvgpu_channel_job *channel_joblist_peek(struct nvgpu_channel *c)
{
	u32 get;
	struct nvgpu_channel_job *job = NULL;

	if (nvgpu_channel_is_prealloc_enabled(c)) {
		if (!nvgpu_channel_joblist_is_empty(c)) {
			get = c->joblist.pre_alloc.get;
			job = &c->joblist.pre_alloc.jobs[get];
		}
	} else {
		if (!nvgpu_list_empty(&c->joblist.dynamic.jobs)) {
			job = nvgpu_list_first_entry(&c->joblist.dynamic.jobs,
				       channel_gk20a_job, list);
		}
	}

	return job;
}

void channel_joblist_add(struct nvgpu_channel *c,
		struct nvgpu_channel_job *job)
{
	if (nvgpu_channel_is_prealloc_enabled(c)) {
		c->joblist.pre_alloc.put = (c->joblist.pre_alloc.put + 1U) %
				(c->joblist.pre_alloc.length);
	} else {
		nvgpu_list_add_tail(&job->list, &c->joblist.dynamic.jobs);
	}
}

void channel_joblist_delete(struct nvgpu_channel *c,
		struct nvgpu_channel_job *job)
{
	if (nvgpu_channel_is_prealloc_enabled(c)) {
		c->joblist.pre_alloc.get = (c->joblist.pre_alloc.get + 1U) %
				(c->joblist.pre_alloc.length);
	} else {
		nvgpu_list_del(&job->list);
	}
}

bool nvgpu_channel_joblist_is_empty(struct nvgpu_channel *c)
{
	if (nvgpu_channel_is_prealloc_enabled(c)) {

		unsigned int get = c->joblist.pre_alloc.get;
		unsigned int put = c->joblist.pre_alloc.put;

		return get == put;
	}

	return nvgpu_list_empty(&c->joblist.dynamic.jobs);
}

int channel_prealloc_resources(struct nvgpu_channel *ch, u32 num_jobs)
{
#ifdef CONFIG_NVGPU_DETERMINISTIC_CHANNELS
	int err;
	size_t size;

	if ((nvgpu_channel_is_prealloc_enabled(ch)) || (num_jobs == 0U)) {
		return -EINVAL;
	}

	/*
	 * pre-allocate the job list.
	 * since vmalloc take in an unsigned long, we need
	 * to make sure we don't hit an overflow condition
	 */
	size = sizeof(struct nvgpu_channel_job);
	if (num_jobs <= U32_MAX / size) {
		ch->joblist.pre_alloc.jobs = nvgpu_vzalloc(ch->g,
							  num_jobs * size);
	}
	if (ch->joblist.pre_alloc.jobs == NULL) {
		err = -ENOMEM;
		goto clean_up;
	}

	/* pre-allocate a fence pool */
	err = nvgpu_fence_pool_alloc(ch, num_jobs);
	if (err != 0) {
		goto clean_up;
	}

	ch->joblist.pre_alloc.length = num_jobs;
	ch->joblist.pre_alloc.put = 0;
	ch->joblist.pre_alloc.get = 0;

	/*
	 * commit the previous writes before setting the flag.
	 * see corresponding nvgpu_smp_rmb in
	 * nvgpu_channel_is_prealloc_enabled()
	 */
	nvgpu_smp_wmb();
	ch->joblist.pre_alloc.enabled = true;

	return 0;

clean_up:
	nvgpu_vfree(ch->g, ch->joblist.pre_alloc.jobs);
	(void) memset(&ch->joblist.pre_alloc, 0, sizeof(ch->joblist.pre_alloc));
	return err;
#else
	return -ENOSYS;
#endif
}

void channel_free_prealloc_resources(struct nvgpu_channel *c)
{
#ifdef CONFIG_NVGPU_DETERMINISTIC_CHANNELS
	nvgpu_vfree(c->g, c->joblist.pre_alloc.jobs[0].wait_cmd);
	nvgpu_vfree(c->g, c->joblist.pre_alloc.jobs);
	nvgpu_fence_pool_free(c);

	/*
	 * commit the previous writes before disabling the flag.
	 * see corresponding nvgpu_smp_rmb in
	 * nvgpu_channel_is_prealloc_enabled()
	 */
	nvgpu_smp_wmb();
	c->joblist.pre_alloc.enabled = false;
#endif
}
