/*
 * GA10B Tegra Platform Interface
 *
 * Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
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

#include <linux/uaccess.h>
#include <uapi/linux/nvgpu.h>

#include <nvgpu/profiler.h>
#include <nvgpu/log.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/kmem.h>
#include <nvgpu/fb.h>

#include "nvgpu_next_ioctl_prof.h"

static u32 nvgpu_prof_vab_reserve_translate_vab_mode(struct gk20a *g, u32 mode)
{
	u32 vab_mode = 0U;

	if (mode == NVGPU_PROFILER_VAB_RANGE_CHECKER_MODE_ACCESS) {
		vab_mode = NVGPU_VAB_MODE_ACCESS;
	} else if (mode == NVGPU_PROFILER_VAB_RANGE_CHECKER_MODE_DIRTY) {
		vab_mode = NVGPU_VAB_MODE_DIRTY;
	} else {
		nvgpu_err(g, "Unknown vab mode: 0x%x", mode);
	}

	return vab_mode;
}

static int nvgpu_prof_ioctl_vab_reserve(struct nvgpu_profiler_object *prof,
		struct nvgpu_profiler_vab_reserve_args *arg)
{
	struct gk20a *g = prof->g;
	int err;
	u32 vab_mode = nvgpu_prof_vab_reserve_translate_vab_mode(g,
		(u32)arg->vab_mode);
	struct nvgpu_profiler_vab_range_checker *user_ckr =
		(struct nvgpu_profiler_vab_range_checker *)(uintptr_t)
		arg->range_checkers_ptr;
	struct nvgpu_vab_range_checker *ckr;

	if (arg->num_range_checkers == 0) {
		nvgpu_err(g, "Range checkers cannot be zero");
		return -EINVAL;
	}

	ckr = nvgpu_kzalloc(g, sizeof(struct nvgpu_vab_range_checker) *
		arg->num_range_checkers);
	if (copy_from_user(ckr, user_ckr,
		sizeof(struct nvgpu_vab_range_checker) *
		arg->num_range_checkers)) {
		return -EFAULT;
	}

	err = g->ops.fb.vab.reserve(g, vab_mode, arg->num_range_checkers, ckr);

	nvgpu_kfree(g, ckr);

	return err;
}

static int nvgpu_prof_ioctl_vab_flush(struct nvgpu_profiler_object *prof,
		struct nvgpu_profiler_vab_flush_state_args *arg)
{
	int err;
	struct gk20a *g = prof->g;
	u64 *user_data = nvgpu_kzalloc(g, arg->buffer_size);

	err = g->ops.fb.vab.dump_and_clear(g, user_data, arg->buffer_size);
	if (err < 0) {
		goto fail;
	}

	if (copy_to_user((void __user *)(uintptr_t)arg->buffer_ptr,
			user_data, arg->buffer_size)) {
		nvgpu_err(g, "copy_to_user failed!");
		err = -EFAULT;
	}

fail:
	nvgpu_kfree(g, user_data);
	return err;
}

int nvgpu_next_prof_fops_ioctl(struct nvgpu_profiler_object *prof,
	unsigned int cmd, void *buf)
{
	int err = -ENOTTY;
	struct gk20a *g = prof->g;

	switch (cmd) {
	case NVGPU_PROFILER_IOCTL_VAB_RESERVE:
		if (!nvgpu_is_enabled(g, NVGPU_SUPPORT_VAB_ENABLED)) {
			break;
		}

		err = nvgpu_prof_ioctl_vab_reserve(prof,
			(struct nvgpu_profiler_vab_reserve_args *)buf);
		break;

	case NVGPU_PROFILER_IOCTL_VAB_FLUSH_STATE:
		if (!nvgpu_is_enabled(g, NVGPU_SUPPORT_VAB_ENABLED)) {
			break;
		}

		err = nvgpu_prof_ioctl_vab_flush(prof,
			(struct nvgpu_profiler_vab_flush_state_args *)buf);
		break;

	case NVGPU_PROFILER_IOCTL_VAB_RELEASE:
		if (nvgpu_is_enabled(g, NVGPU_SUPPORT_VAB_ENABLED)) {
			err = g->ops.fb.vab.release(g);
		}
		break;

	default:
		nvgpu_err(g, "unrecognized profiler ioctl cmd: 0x%x", cmd);
		err = -ENOTTY;
		break;
	}
	return err;
}
