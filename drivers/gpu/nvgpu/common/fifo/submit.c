/*
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

#include <nvgpu/gk20a.h>
#include <nvgpu/nvgpu_init.h>
#include <nvgpu/channel.h>
#include <nvgpu/ltc.h>
#include <nvgpu/os_sched.h>
#include <nvgpu/utils.h>
#include <nvgpu/channel_sync.h>
#include <nvgpu/channel_sync_syncpt.h>
#include <nvgpu/bug.h>
#include <nvgpu/fence.h>
#include <nvgpu/profile.h>
#include <nvgpu/vpr.h>
#include <nvgpu/trace.h>
#include <nvgpu/nvhost.h>

/*
 * Handle the submit synchronization - pre-fences and post-fences.
 */
static int nvgpu_submit_prepare_syncs(struct nvgpu_channel *c,
				      struct nvgpu_channel_fence *fence,
				      struct nvgpu_channel_job *job,
				      struct priv_cmd_entry **wait_cmd,
				      struct priv_cmd_entry **incr_cmd,
				      struct nvgpu_fence_type **post_fence,
				      bool register_irq,
				      u32 flags)
{
	struct gk20a *g = c->g;
	bool need_sync_fence = false;
	bool new_sync_created = false;
	int wait_fence_fd = -1;
	int err = 0;
	bool need_wfi = (flags & NVGPU_SUBMIT_FLAGS_SUPPRESS_WFI) == 0U;
	bool pre_alloc_enabled = nvgpu_channel_is_prealloc_enabled(c);
	struct nvgpu_channel_sync_syncpt *sync_syncpt = NULL;
	bool flag_fence_get = (flags & NVGPU_SUBMIT_FLAGS_FENCE_GET) != 0U;
	bool flag_sync_fence = (flags & NVGPU_SUBMIT_FLAGS_SYNC_FENCE) != 0U;
	bool flag_fence_wait = (flags & NVGPU_SUBMIT_FLAGS_FENCE_WAIT) != 0U;

	if (g->aggressive_sync_destroy_thresh != 0U) {
		nvgpu_mutex_acquire(&c->sync_lock);
		if (c->sync == NULL) {
			c->sync = nvgpu_channel_sync_create(c, false);
			if (c->sync == NULL) {
				err = -ENOMEM;
				goto fail;
			}
			new_sync_created = true;
		}
		nvgpu_channel_sync_get_ref(c->sync);
	}

	if ((g->ops.channel.set_syncpt != NULL) && new_sync_created) {
		err = g->ops.channel.set_syncpt(c);
		if (err != 0) {
			goto fail;
		}
	}

	/*
	 * Optionally insert syncpt/semaphore wait in the beginning of gpfifo
	 * submission when user requested and the wait hasn't expired.
	 */
	if (flag_fence_wait) {
		u32 max_wait_cmds = nvgpu_channel_is_deterministic(c) ?
			1U : 0U;

		if (!pre_alloc_enabled) {
			job->wait_cmd = nvgpu_kzalloc(g,
				sizeof(struct priv_cmd_entry));
		}

		if (job->wait_cmd == NULL) {
			err = -ENOMEM;
			goto fail;
		}

		if (flag_sync_fence) {
			nvgpu_assert(fence->id <= (u32)INT_MAX);
			wait_fence_fd = (int)fence->id;
			err = nvgpu_channel_sync_wait_fence_fd(c->sync,
				wait_fence_fd, job->wait_cmd, max_wait_cmds);
		} else {
			sync_syncpt = nvgpu_channel_sync_to_syncpt(c->sync);
			if (sync_syncpt != NULL) {
				err = nvgpu_channel_sync_wait_syncpt(
					sync_syncpt, fence->id,
					fence->value, job->wait_cmd);
			} else {
				err = -EINVAL;
			}
		}

		if (err != 0) {
			goto clean_up_wait_cmd;
		}

		if (job->wait_cmd->valid) {
			/* not expired yet */
			*wait_cmd = job->wait_cmd;
		}
	}

	if (flag_fence_get && flag_sync_fence) {
		need_sync_fence = true;
	}

	/*
	 * Always generate an increment at the end of a GPFIFO submission. This
	 * is used to keep track of method completion for idle railgating. The
	 * sync_pt/semaphore PB is added to the GPFIFO later on in submit.
	 */
	job->post_fence = nvgpu_fence_alloc(c);
	if (job->post_fence == NULL) {
		err = -ENOMEM;
		goto clean_up_wait_cmd;
	}
	if (!pre_alloc_enabled) {
		job->incr_cmd = nvgpu_kzalloc(g, sizeof(struct priv_cmd_entry));
	}

	if (job->incr_cmd == NULL) {
		err = -ENOMEM;
		goto clean_up_post_fence;
	}

	if (flag_fence_get) {
		err = nvgpu_channel_sync_incr_user(c->sync,
			wait_fence_fd, job->incr_cmd,
			job->post_fence, need_wfi, need_sync_fence,
			register_irq);
	} else {
		err = nvgpu_channel_sync_incr(c->sync,
			job->incr_cmd, job->post_fence, need_sync_fence,
			register_irq);
	}
	if (err == 0) {
		*incr_cmd = job->incr_cmd;
		*post_fence = job->post_fence;
	} else {
		goto clean_up_incr_cmd;
	}

	if (g->aggressive_sync_destroy_thresh != 0U) {
		nvgpu_mutex_release(&c->sync_lock);
	}
	return 0;

clean_up_incr_cmd:
	nvgpu_channel_free_priv_cmd_entry(c, job->incr_cmd);
	if (!pre_alloc_enabled) {
		job->incr_cmd = NULL;
	}
clean_up_post_fence:
	nvgpu_fence_put(job->post_fence);
	job->post_fence = NULL;
clean_up_wait_cmd:
	if (job->wait_cmd != NULL) {
		nvgpu_channel_free_priv_cmd_entry(c, job->wait_cmd);
	}
	if (!pre_alloc_enabled) {
		job->wait_cmd = NULL;
	}
fail:
	if (g->aggressive_sync_destroy_thresh != 0U) {
		nvgpu_mutex_release(&c->sync_lock);
	}
	*wait_cmd = NULL;
	return err;
}

static void nvgpu_submit_append_priv_cmdbuf(struct nvgpu_channel *c,
		struct priv_cmd_entry *cmd)
{
	struct gk20a *g = c->g;
	struct nvgpu_mem *gpfifo_mem = &c->gpfifo.mem;
	struct nvgpu_gpfifo_entry gpfifo_entry;

	g->ops.pbdma.format_gpfifo_entry(g, &gpfifo_entry,
			cmd->gva, cmd->size);

	nvgpu_mem_wr_n(g, gpfifo_mem,
			c->gpfifo.put * (u32)sizeof(gpfifo_entry),
			&gpfifo_entry, (u32)sizeof(gpfifo_entry));

#ifdef CONFIG_NVGPU_TRACE
	if (cmd->mem->aperture == APERTURE_SYSMEM) {
		trace_gk20a_push_cmdbuf(g->name, 0, cmd->size, 0,
				(u32 *)cmd->mem->cpu_va + cmd->off);
	}
#endif

	c->gpfifo.put = (c->gpfifo.put + 1U) & (c->gpfifo.entry_num - 1U);
}

static int nvgpu_submit_append_gpfifo_user_direct(struct nvgpu_channel *c,
		struct nvgpu_gpfifo_userdata userdata,
		u32 num_entries)
{
	struct gk20a *g = c->g;
	struct nvgpu_gpfifo_entry *gpfifo_cpu = c->gpfifo.mem.cpu_va;
	u32 gpfifo_size = c->gpfifo.entry_num;
	u32 len = num_entries;
	u32 start = c->gpfifo.put;
	u32 end = start + len; /* exclusive */
	int err;

	nvgpu_speculation_barrier();
	if (end > gpfifo_size) {
		/* wrap-around */
		u32 length0 = gpfifo_size - start;
		u32 length1 = len - length0;

		err = g->os_channel.copy_user_gpfifo(
				&gpfifo_cpu[start], userdata,
				0, length0);
		if (err != 0) {
			return err;
		}

		err = g->os_channel.copy_user_gpfifo(
				gpfifo_cpu, userdata,
				length0, length1);
		if (err != 0) {
			return err;
		}
	} else {
		err = g->os_channel.copy_user_gpfifo(
				&gpfifo_cpu[start], userdata,
				0, len);
		if (err != 0) {
			return err;
		}
	}

	return 0;
}

static void nvgpu_submit_append_gpfifo_common(struct nvgpu_channel *c,
		struct nvgpu_gpfifo_entry *src, u32 num_entries)
{
	struct gk20a *g = c->g;
	struct nvgpu_mem *gpfifo_mem = &c->gpfifo.mem;
	/* in bytes */
	u32 gpfifo_size =
		c->gpfifo.entry_num * (u32)sizeof(struct nvgpu_gpfifo_entry);
	u32 len = num_entries * (u32)sizeof(struct nvgpu_gpfifo_entry);
	u32 start = c->gpfifo.put * (u32)sizeof(struct nvgpu_gpfifo_entry);
	u32 end = start + len; /* exclusive */

	if (end > gpfifo_size) {
		/* wrap-around */
		u32 length0 = gpfifo_size - start;
		u32 length1 = len - length0;
		struct nvgpu_gpfifo_entry *src2 = &src[length0];

		nvgpu_mem_wr_n(g, gpfifo_mem, start, src, length0);
		nvgpu_mem_wr_n(g, gpfifo_mem, 0, src2, length1);
	} else {
		nvgpu_mem_wr_n(g, gpfifo_mem, start, src, len);
	}
}

/*
 * Copy source gpfifo entries into the gpfifo ring buffer, potentially
 * splitting into two memcpys to handle wrap-around.
 */
static int nvgpu_submit_append_gpfifo(struct nvgpu_channel *c,
		struct nvgpu_gpfifo_entry *kern_gpfifo,
		struct nvgpu_gpfifo_userdata userdata,
		u32 num_entries)
{
	int err;

	if ((kern_gpfifo == NULL)
#ifdef CONFIG_NVGPU_DGPU
	    && (c->gpfifo.pipe == NULL)
#endif
	   ) {
		/*
		 * This path (from userspace to sysmem) is special in order to
		 * avoid two copies unnecessarily (from user to pipe, then from
		 * pipe to gpu sysmem buffer).
		 */
		err = nvgpu_submit_append_gpfifo_user_direct(c, userdata,
				num_entries);
		if (err != 0) {
			return err;
		}
	}
#ifdef CONFIG_NVGPU_DGPU
	else if (kern_gpfifo == NULL) {
		/* from userspace to vidmem, use the common path */
		err = c->g->os_channel.copy_user_gpfifo(c->gpfifo.pipe,
				userdata, 0, num_entries);
		if (err != 0) {
			return err;
		}

		nvgpu_submit_append_gpfifo_common(c, c->gpfifo.pipe,
				num_entries);
	}
#endif
	else {
		/* from kernel to either sysmem or vidmem, don't need
		 * copy_user_gpfifo so use the common path */
		nvgpu_submit_append_gpfifo_common(c, kern_gpfifo, num_entries);
	}

	trace_write_pushbuffers(c, num_entries);

	c->gpfifo.put = (c->gpfifo.put + num_entries) &
		(c->gpfifo.entry_num - 1U);

	return 0;
}

static int check_submit_allowed(struct nvgpu_channel *c)
{
	struct gk20a *g = c->g;

	if (nvgpu_is_enabled(g, NVGPU_DRIVER_IS_DYING)) {
		return -ENODEV;
	}

	if (nvgpu_channel_check_unserviceable(c)) {
		return -ETIMEDOUT;
	}

	if (c->usermode_submit_enabled) {
		return -EINVAL;
	}

	if (!nvgpu_mem_is_valid(&c->gpfifo.mem)) {
		return -ENOMEM;
	}

	/* an address space needs to have been bound at this point. */
	if (!nvgpu_channel_as_bound(c)) {
		nvgpu_err(g,
			    "not bound to an address space at time of gpfifo"
			    " submission.");
		return -EINVAL;
	}

	return 0;
}

static int nvgpu_submit_channel_gpfifo(struct nvgpu_channel *c,
				struct nvgpu_gpfifo_entry *gpfifo,
				struct nvgpu_gpfifo_userdata userdata,
				u32 num_entries,
				u32 flags,
				struct nvgpu_channel_fence *fence,
				struct nvgpu_fence_type **fence_out,
				struct nvgpu_profile *profile)
{
	struct gk20a *g = c->g;
	struct priv_cmd_entry *wait_cmd = NULL;
	struct priv_cmd_entry *incr_cmd = NULL;
	struct nvgpu_channel_job *job = NULL;
	/* we might need two extra gpfifo entries - one for pre fence
	 * and one for post fence. */
	const u32 extra_entries = 2U;
	bool skip_buffer_refcounting = (flags &
			NVGPU_SUBMIT_FLAGS_SKIP_BUFFER_REFCOUNTING) != 0U;
	int err = 0;
	bool need_job_tracking;
	bool need_deferred_cleanup = false;
	bool flag_fence_wait = (flags & NVGPU_SUBMIT_FLAGS_FENCE_WAIT) != 0U;
	bool flag_fence_get = (flags & NVGPU_SUBMIT_FLAGS_FENCE_GET) != 0U;
	bool flag_sync_fence = (flags & NVGPU_SUBMIT_FLAGS_SYNC_FENCE) != 0U;

	err = check_submit_allowed(c);
	if (err != 0) {
		return err;
	}

	/* fifo not large enough for request. Return error immediately.
	 * Kernel can insert gpfifo entries before and after user gpfifos.
	 * So, add extra_entries in user request. Also, HW with fifo size N
	 * can accept only N-1 entreis and so the below condition */
	if (c->gpfifo.entry_num - 1U < num_entries + extra_entries) {
		nvgpu_err(g, "not enough gpfifo space allocated");
		return -ENOMEM;
	}

	if ((flag_fence_wait || flag_fence_get) && (fence == NULL)) {
		return -EINVAL;
	}

	nvgpu_profile_snapshot(profile, PROFILE_ENTRY);

	/* update debug settings */
	nvgpu_ltc_sync_enabled(g);

	nvgpu_log_info(g, "channel %d", c->chid);

	/*
	 * Job tracking is necessary for any of the following conditions:
	 *  - pre- or post-fence functionality
	 *  - channel wdt
	 *  - GPU rail-gating with non-deterministic channels
	 *  - VPR resize enabled with non-deterministic channels
	 *  - buffer refcounting
	 *
	 * If none of the conditions are met, then job tracking is not
	 * required and a fast submit can be done (ie. only need to write
	 * out userspace GPFIFO entries and update GP_PUT).
	 */
	need_job_tracking = (flag_fence_wait ||
			flag_fence_get ||
			((nvgpu_is_enabled(g, NVGPU_CAN_RAILGATE) ||
				nvgpu_is_vpr_resize_enabled()) &&
				!nvgpu_channel_is_deterministic(c)) ||
			!skip_buffer_refcounting);

#ifdef CONFIG_NVGPU_CHANNEL_WDT
       need_job_tracking = need_job_tracking || c->wdt.enabled;
#endif

	if (need_job_tracking) {
		/*
		 * If the channel is to have deterministic latency and
		 * job tracking is required, the channel must have
		 * pre-allocated resources. Otherwise, we fail the submit here
		 */
		if (nvgpu_channel_is_deterministic(c) &&
				!nvgpu_channel_is_prealloc_enabled(c)) {
			return -EINVAL;
		}

		/*
		 * Deferred clean-up is necessary for any of the following
		 * conditions that could make clean-up behaviour
		 * non-deterministic and as such not suitable for the submit
		 * path:
		 * - channel's deterministic flag is not set (job tracking is
		 *   dynamically allocated)
		 * - no syncpt support (struct nvgpu_semaphore is dynamically
		 *   allocated, not pooled)
		 * - dependency on sync framework for post fences
		 * - buffer refcounting, which is O(n)
		 * - channel wdt, which needs periodic async cleanup
		 *
		 * If none of the conditions are met, then deferred clean-up
		 * is not required, and we clean-up one job-tracking
		 * resource in the submit path.
		 */
		need_deferred_cleanup = !nvgpu_channel_is_deterministic(c) ||
					!nvgpu_has_syncpoints(g) ||
					(flag_sync_fence && flag_fence_get) ||
					!skip_buffer_refcounting;

#ifdef CONFIG_NVGPU_CHANNEL_WDT
		need_deferred_cleanup = need_deferred_cleanup || c->wdt.enabled;
#endif

		/*
		 * For deterministic channels, we don't allow deferred clean_up
		 * processing to occur. In cases we hit this, we fail the submit
		 */
		if (nvgpu_channel_is_deterministic(c) && need_deferred_cleanup) {
			return -EINVAL;
		}

		if (!nvgpu_channel_is_deterministic(c)) {
			/*
			 * Get a power ref unless this is a deterministic
			 * channel that holds them during the channel lifetime.
			 * This one is released by nvgpu_channel_clean_up_jobs,
			 * via syncpt or sema interrupt, whichever is used.
			 */
			err = gk20a_busy(g);
			if (err != 0) {
				nvgpu_err(g,
					"failed to host gk20a to submit gpfifo");
				nvgpu_print_current(g, NULL, NVGPU_ERROR);
				return err;
			}
		}

		if (!need_deferred_cleanup) {
			/* clean up a single job */
			nvgpu_channel_clean_up_jobs(c, false);
		}
	}


#ifdef CONFIG_NVGPU_DETERMINISTIC_CHANNELS
	/* Grab access to HW to deal with do_idle */
	if (c->deterministic) {
		nvgpu_rwsem_down_read(&g->deterministic_busy);
	}

	if (c->deterministic && c->deterministic_railgate_allowed) {
		/*
		 * Nope - this channel has dropped its own power ref. As
		 * deterministic submits don't hold power on per each submitted
		 * job like normal ones do, the GPU might railgate any time now
		 * and thus submit is disallowed.
		 */
		err = -EINVAL;
		goto clean_up;
	}
#endif

#ifdef CONFIG_NVGPU_TRACE
	trace_gk20a_channel_submit_gpfifo(g->name,
					  c->chid,
					  num_entries,
					  flags,
					  fence ? fence->id : 0,
					  fence ? fence->value : 0);
#endif

	nvgpu_log_info(g, "pre-submit put %d, get %d, size %d",
		c->gpfifo.put, c->gpfifo.get, c->gpfifo.entry_num);

	/*
	 * Make sure we have enough space for gpfifo entries. Check cached
	 * values first and then read from HW. If no space, return EAGAIN
	 * and let userpace decide to re-try request or not.
	 */
	if (nvgpu_channel_get_gpfifo_free_count(c) <
			num_entries + extra_entries) {
		if (nvgpu_channel_update_gpfifo_get_and_get_free_count(c) <
				num_entries + extra_entries) {
			err = -EAGAIN;
			goto clean_up;
		}
	}

	if (need_job_tracking) {
		struct nvgpu_fence_type *post_fence = NULL;

		err = nvgpu_channel_alloc_job(c, &job);
		if (err != 0) {
			goto clean_up;
		}

		err = nvgpu_submit_prepare_syncs(c, fence, job,
						 &wait_cmd, &incr_cmd,
						 &post_fence,
						 need_deferred_cleanup,
						 flags);
		if (err != 0) {
			goto clean_up_job;
		}

		nvgpu_profile_snapshot(profile, PROFILE_JOB_TRACKING);

		/*
		 * wait_cmd can be unset even if flag_fence_wait exists. See
		 * the expiration check in channel_sync_syncpt_gen_wait_cmd.
		 */
		if (wait_cmd != NULL) {
			nvgpu_submit_append_priv_cmdbuf(c, wait_cmd);
		}

		err = nvgpu_submit_append_gpfifo(c, gpfifo, userdata,
				num_entries);
		if (err != 0) {
			nvgpu_fence_put(post_fence);
			goto clean_up_job;
		}

		nvgpu_submit_append_priv_cmdbuf(c, incr_cmd);

		err = nvgpu_channel_add_job(c, job, skip_buffer_refcounting);
		if (err != 0) {
			nvgpu_fence_put(post_fence);
			goto clean_up_job;
		}

		if (fence_out != NULL) {
			*fence_out = nvgpu_fence_get(post_fence);
		}
	} else {
		nvgpu_profile_snapshot(profile, PROFILE_JOB_TRACKING);

		err = nvgpu_submit_append_gpfifo(c, gpfifo, userdata,
				num_entries);
		if (err != 0) {
			goto clean_up;
		}
		if (fence_out != NULL) {
			*fence_out = NULL;
		}
	}

	nvgpu_profile_snapshot(profile, PROFILE_APPEND);

	g->ops.userd.gp_put(g, c);

#ifdef CONFIG_NVGPU_DETERMINISTIC_CHANNELS
	/* No hw access beyond this point */
	if (c->deterministic) {
		nvgpu_rwsem_up_read(&g->deterministic_busy);
	}
#endif

#ifdef CONFIG_NVGPU_TRACE
	if (fence_out != NULL && *fence_out != NULL) {
		trace_gk20a_channel_submitted_gpfifo(g->name,
					c->chid, num_entries, flags,
					(*fence_out)->syncpt_id,
					(*fence_out)->syncpt_value);
	} else {
		trace_gk20a_channel_submitted_gpfifo(g->name,
					c->chid, num_entries, flags,
					0, 0);
	}
#endif

	nvgpu_log_info(g, "post-submit put %d, get %d, size %d",
		c->gpfifo.put, c->gpfifo.get, c->gpfifo.entry_num);

	nvgpu_profile_snapshot(profile, PROFILE_END);

	nvgpu_log_fn(g, "done");
	return err;

clean_up_job:
	nvgpu_channel_free_job(c, job);
clean_up:
	nvgpu_log_fn(g, "fail");
#ifdef CONFIG_NVGPU_DETERMINISTIC_CHANNELS
	if (c->deterministic) {
		nvgpu_rwsem_up_read(&g->deterministic_busy);
		return err;
	}
#endif
	if (need_deferred_cleanup) {
		gk20a_idle(g);
	}

	return err;
}

int nvgpu_submit_channel_gpfifo_user(struct nvgpu_channel *c,
				struct nvgpu_gpfifo_userdata userdata,
				u32 num_entries,
				u32 flags,
				struct nvgpu_channel_fence *fence,
				struct nvgpu_fence_type **fence_out,
				struct nvgpu_profile *profile)
{
	return nvgpu_submit_channel_gpfifo(c, NULL, userdata, num_entries,
			flags, fence, fence_out, profile);
}

int nvgpu_submit_channel_gpfifo_kernel(struct nvgpu_channel *c,
				struct nvgpu_gpfifo_entry *gpfifo,
				u32 num_entries,
				u32 flags,
				struct nvgpu_channel_fence *fence,
				struct nvgpu_fence_type **fence_out)
{
	struct nvgpu_gpfifo_userdata userdata = { NULL, NULL };

	return nvgpu_submit_channel_gpfifo(c, gpfifo, userdata, num_entries,
			flags, fence, fence_out, NULL);
}
