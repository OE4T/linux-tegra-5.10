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

#include <linux/fs.h>
#include <linux/file.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <uapi/linux/nvgpu.h>

#include <nvgpu/kmem.h>
#include <nvgpu/log.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/nvgpu_init.h>
#include <nvgpu/profiler.h>
#include <nvgpu/pm_reservation.h>
#include <nvgpu/tsg.h>

#include "os_linux.h"
#include "ioctl_prof.h"
#include "ioctl_tsg.h"

struct nvgpu_profiler_object_priv {
	struct nvgpu_profiler_object *prof;
	struct gk20a *g;
};

static int nvgpu_prof_fops_open(struct gk20a *g, struct file *filp,
		enum nvgpu_profiler_pm_reservation_scope scope)
{
	struct nvgpu_profiler_object_priv *prof_priv;
	struct nvgpu_profiler_object *prof;
	int err;

	nvgpu_log(g, gpu_dbg_prof, "Request to open profiler session with scope %u",
		scope);

	prof_priv = nvgpu_kzalloc(g, sizeof(*prof_priv));
	if (prof_priv == NULL) {
		return -ENOMEM;
	}

	err = nvgpu_profiler_alloc(g, &prof, scope);
	if (err != 0) {
		nvgpu_kfree(g, prof_priv);
		return -ENOMEM;
	}

	prof_priv->g = g;
	prof_priv->prof = prof;
	filp->private_data = prof_priv;

	nvgpu_log(g, gpu_dbg_prof,
		"Profiler session with scope %u created successfully with profiler handle %u",
		scope, prof->prof_handle);

	return 0;
}

int nvgpu_prof_dev_fops_open(struct inode *inode, struct file *filp)
{
	struct nvgpu_os_linux *l = container_of(inode->i_cdev,
			 struct nvgpu_os_linux, prof_dev.cdev);
	struct gk20a *g;
	int err;

	g = nvgpu_get(&l->g);
	if (!g) {
		return -ENODEV;
	}

	if (!nvgpu_is_enabled(g, NVGPU_SUPPORT_PROFILER_V2_DEVICE)) {
		nvgpu_put(g);
		return -EINVAL;
	}

	err = nvgpu_prof_fops_open(g, filp,
			NVGPU_PROFILER_PM_RESERVATION_SCOPE_DEVICE);
	if (err != 0) {
		nvgpu_put(g);
	}

	return err;
}

int nvgpu_prof_ctx_fops_open(struct inode *inode, struct file *filp)
{
	struct nvgpu_os_linux *l = container_of(inode->i_cdev,
			 struct nvgpu_os_linux, prof_ctx.cdev);
	struct gk20a *g;
	int err;

	g = nvgpu_get(&l->g);
	if (!g) {
		return -ENODEV;
	}

	if (!nvgpu_is_enabled(g, NVGPU_SUPPORT_PROFILER_V2_CONTEXT)) {
		nvgpu_put(g);
		return -EINVAL;
	}

	err = nvgpu_prof_fops_open(g, filp,
			NVGPU_PROFILER_PM_RESERVATION_SCOPE_CONTEXT);
	if (err != 0) {
		nvgpu_put(g);
	}

	return err;
}

int nvgpu_prof_fops_release(struct inode *inode, struct file *filp)
{
	struct nvgpu_profiler_object_priv *prof_priv = filp->private_data;
	struct nvgpu_profiler_object *prof = prof_priv->prof;
	struct gk20a *g = prof_priv->g;

	nvgpu_log(g, gpu_dbg_prof,
		"Request to close profiler session with scope %u and profiler handle %u",
		prof->scope, prof->prof_handle);

	nvgpu_profiler_free(prof);
	nvgpu_kfree(g, prof_priv);
	nvgpu_put(g);

	nvgpu_log(g, gpu_dbg_prof, "Profiler session closed successfully");

	return 0;
}

static int nvgpu_prof_ioctl_bind_context(struct nvgpu_profiler_object *prof,
		struct nvgpu_profiler_bind_context_args *args)
{
	int tsg_fd = args->tsg_fd;
	struct nvgpu_tsg *tsg;
	struct gk20a *g = prof->g;

	if (prof->context_init) {
		nvgpu_err(g, "Context info is already initialized");
		return -EINVAL;
	}

	if (tsg_fd < 0) {
		if (prof->scope == NVGPU_PROFILER_PM_RESERVATION_SCOPE_DEVICE) {
			prof->context_init = true;
			return 0;
		}
		return -EINVAL;
	}

	tsg = nvgpu_tsg_get_from_file(tsg_fd);
	if (tsg == NULL) {
		nvgpu_err(g, "invalid TSG fd %d", tsg_fd);
		return -EINVAL;
	}

	return nvgpu_profiler_bind_context(prof, tsg);
}

static int nvgpu_prof_ioctl_unbind_context(struct nvgpu_profiler_object *prof)
{
	return nvgpu_profiler_unbind_context(prof);
}

static int nvgpu_prof_ioctl_get_pm_resource_type(u32 resource,
		enum nvgpu_profiler_pm_resource_type *pm_resource)
{
	switch (resource) {
	case NVGPU_PROFILER_PM_RESOURCE_ARG_HWPM_LEGACY:
		*pm_resource = NVGPU_PROFILER_PM_RESOURCE_TYPE_HWPM_LEGACY;
		return 0;
	case NVGPU_PROFILER_PM_RESOURCE_ARG_SMPC:
		*pm_resource = NVGPU_PROFILER_PM_RESOURCE_TYPE_SMPC;
		return 0;
	default:
		break;
	}

	return -EINVAL;
}

static int nvgpu_prof_ioctl_reserve_pm_resource(struct nvgpu_profiler_object *prof,
		struct nvgpu_profiler_reserve_pm_resource_args *args)
{
	enum nvgpu_profiler_pm_resource_type pm_resource;
	struct gk20a *g = prof->g;
	bool flag_ctxsw;
	int err;

	if (!prof->context_init) {
		nvgpu_err(g, "Context info not initialized");
		return -EINVAL;
	}

	err = nvgpu_prof_ioctl_get_pm_resource_type(args->resource,
			&pm_resource);
	if (err) {
		nvgpu_err(prof->g, "invalid resource %u", args->resource);
		return err;
	}

	flag_ctxsw = ((args->flags & NVGPU_PROFILER_RESERVE_PM_RESOURCE_ARG_FLAG_CTXSW) != 0);

	switch (prof->scope) {
	case NVGPU_PROFILER_PM_RESERVATION_SCOPE_DEVICE:
		if (flag_ctxsw && (prof->tsg == NULL)) {
			nvgpu_err(g, "Context must be bound to enable context switch");
			return -EINVAL;
		}
		if (!flag_ctxsw	&& (pm_resource == NVGPU_PROFILER_PM_RESOURCE_TYPE_SMPC)
				&& !nvgpu_is_enabled(g, NVGPU_SUPPORT_SMPC_GLOBAL_MODE)) {
			nvgpu_err(g, "SMPC global mode not supported");
			return -EINVAL;
		}
		if (flag_ctxsw) {
			prof->ctxsw[pm_resource] = true;
		} else {
			prof->ctxsw[pm_resource] = false;
		}
		break;

	case NVGPU_PROFILER_PM_RESERVATION_SCOPE_CONTEXT:
		if (prof->tsg == NULL) {
			nvgpu_err(g, "Context must be bound for context session");
			return -EINVAL;
		}
		prof->ctxsw[pm_resource] = true;
		break;

	default:
		return -EINVAL;
	}

	err = nvgpu_profiler_pm_resource_reserve(prof, pm_resource);
	if (err) {
		return err;
	}

	return 0;
}

static int nvgpu_prof_ioctl_release_pm_resource(struct nvgpu_profiler_object *prof,
		struct nvgpu_profiler_release_pm_resource_args *args)
{
	enum nvgpu_profiler_pm_resource_type pm_resource;
	int err;

	err = nvgpu_prof_ioctl_get_pm_resource_type(args->resource,
			&pm_resource);
	if (err) {
		return err;
	}

	err = nvgpu_profiler_pm_resource_release(prof, pm_resource);
	if (err) {
		return err;
	}

	prof->ctxsw[pm_resource] = false;

	return 0;
}

static int nvgpu_prof_ioctl_bind_pm_resources(struct nvgpu_profiler_object *prof)
{
	return nvgpu_profiler_bind_pm_resources(prof);
}

static int nvgpu_prof_ioctl_unbind_pm_resources(struct nvgpu_profiler_object *prof)
{
	return nvgpu_profiler_unbind_pm_resources(prof);
}

long nvgpu_prof_fops_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg)
{
	struct nvgpu_profiler_object_priv *prof_priv = filp->private_data;
	struct nvgpu_profiler_object *prof = prof_priv->prof;
	struct gk20a *g = prof_priv->g;
	u8 __maybe_unused buf[NVGPU_PROFILER_IOCTL_MAX_ARG_SIZE];
	int err = 0;

	if ((_IOC_TYPE(cmd) != NVGPU_PROFILER_IOCTL_MAGIC) ||
	    (_IOC_NR(cmd) == 0) ||
	    (_IOC_NR(cmd) > NVGPU_PROFILER_IOCTL_LAST) ||
	    (_IOC_SIZE(cmd) > NVGPU_PROFILER_IOCTL_MAX_ARG_SIZE)) {
		return -EINVAL;
	}

	(void) memset(buf, 0, sizeof(buf));
	if (_IOC_DIR(cmd) & _IOC_WRITE) {
		if (copy_from_user(buf, (void __user *)arg, _IOC_SIZE(cmd))) {
			return -EFAULT;
		}
	}

	nvgpu_log(g, gpu_dbg_prof, "Profiler handle %u received IOCTL cmd %u",
		prof->prof_handle, cmd);

	nvgpu_mutex_acquire(&prof->ioctl_lock);

	nvgpu_speculation_barrier();

	switch (cmd) {
	case NVGPU_PROFILER_IOCTL_BIND_CONTEXT:
		err = nvgpu_prof_ioctl_bind_context(prof,
			(struct nvgpu_profiler_bind_context_args *)buf);
		break;

	case NVGPU_PROFILER_IOCTL_UNBIND_CONTEXT:
		err = nvgpu_prof_ioctl_unbind_context(prof);
		break;

	case NVGPU_PROFILER_IOCTL_RESERVE_PM_RESOURCE:
		err = nvgpu_prof_ioctl_reserve_pm_resource(prof,
			(struct nvgpu_profiler_reserve_pm_resource_args *)buf);
		break;

	case NVGPU_PROFILER_IOCTL_RELEASE_PM_RESOURCE:
		err = nvgpu_prof_ioctl_release_pm_resource(prof,
			(struct nvgpu_profiler_release_pm_resource_args *)buf);
		break;

	case NVGPU_PROFILER_IOCTL_BIND_PM_RESOURCES:
		err = nvgpu_prof_ioctl_bind_pm_resources(prof);
		break;

	case NVGPU_PROFILER_IOCTL_UNBIND_PM_RESOURCES:
		err = nvgpu_prof_ioctl_unbind_pm_resources(prof);
		break;

	default:
		nvgpu_err(g, "unrecognized profiler ioctl cmd: 0x%x", cmd);
		err = -ENOTTY;
		break;
	}

	nvgpu_mutex_release(&prof->ioctl_lock);

	if ((err == 0) && (_IOC_DIR(cmd) & _IOC_READ))
		err = copy_to_user((void __user *)arg,
				   buf, _IOC_SIZE(cmd));

	nvgpu_log(g, gpu_dbg_prof, "Profiler handle %u IOCTL err =  %d",
		prof->prof_handle, err);

	return err;
}
