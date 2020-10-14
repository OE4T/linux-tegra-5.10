/*
 * NVGPU IOCTLs
 *
 * Copyright (c) 2011-2020, NVIDIA CORPORATION.  All rights reserved.
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

#include <linux/file.h>
#include <linux/slab.h>

#include <nvgpu/nvgpu_common.h>
#include <nvgpu/gk20a.h>

#include "ioctl_channel.h"
#include "ioctl_ctrl.h"
#include "ioctl_as.h"
#include "ioctl_tsg.h"
#include "ioctl_dbg.h"
#include "ioctl_prof.h"
#include "module.h"
#include "os_linux.h"
#include "fecs_trace_linux.h"
#include "platform_gk20a.h"

#define GK20A_NUM_CDEVS 10

const struct file_operations gk20a_channel_ops = {
	.owner = THIS_MODULE,
	.release = gk20a_channel_release,
	.open = gk20a_channel_open,
#ifdef CONFIG_COMPAT
	.compat_ioctl = gk20a_channel_ioctl,
#endif
	.unlocked_ioctl = gk20a_channel_ioctl,
};

static const struct file_operations gk20a_ctrl_ops = {
	.owner = THIS_MODULE,
	.release = gk20a_ctrl_dev_release,
	.open = gk20a_ctrl_dev_open,
	.unlocked_ioctl = gk20a_ctrl_dev_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = gk20a_ctrl_dev_ioctl,
#endif
	.mmap = gk20a_ctrl_dev_mmap,
};

static const struct file_operations gk20a_dbg_ops = {
	.owner = THIS_MODULE,
	.release = gk20a_dbg_gpu_dev_release,
	.open = gk20a_dbg_gpu_dev_open,
	.unlocked_ioctl = gk20a_dbg_gpu_dev_ioctl,
	.poll = gk20a_dbg_gpu_dev_poll,
#ifdef CONFIG_COMPAT
	.compat_ioctl = gk20a_dbg_gpu_dev_ioctl,
#endif
};

static const struct file_operations gk20a_as_ops = {
	.owner = THIS_MODULE,
	.release = gk20a_as_dev_release,
	.open = gk20a_as_dev_open,
#ifdef CONFIG_COMPAT
	.compat_ioctl = gk20a_as_dev_ioctl,
#endif
	.unlocked_ioctl = gk20a_as_dev_ioctl,
};

/*
 * Note: We use a different 'open' to trigger handling of the profiler session.
 * Most of the code is shared between them...  Though, at some point if the
 * code does get too tangled trying to handle each in the same path we can
 * separate them cleanly.
 */
static const struct file_operations gk20a_prof_ops = {
	.owner = THIS_MODULE,
	.release = gk20a_dbg_gpu_dev_release,
	.open = gk20a_prof_gpu_dev_open,
	.unlocked_ioctl = gk20a_dbg_gpu_dev_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = gk20a_dbg_gpu_dev_ioctl,
#endif
};

static const struct file_operations gk20a_prof_dev_ops = {
	.owner = THIS_MODULE,
	.release = nvgpu_prof_fops_release,
	.open = nvgpu_prof_dev_fops_open,
	.unlocked_ioctl = nvgpu_prof_fops_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = nvgpu_prof_fops_ioctl,
#endif
};

static const struct file_operations gk20a_prof_ctx_ops = {
	.owner = THIS_MODULE,
	.release = nvgpu_prof_fops_release,
	.open = nvgpu_prof_ctx_fops_open,
	.unlocked_ioctl = nvgpu_prof_fops_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = nvgpu_prof_fops_ioctl,
#endif
};

const struct file_operations gk20a_tsg_ops = {
	.owner = THIS_MODULE,
	.release = nvgpu_ioctl_tsg_dev_release,
	.open = nvgpu_ioctl_tsg_dev_open,
#ifdef CONFIG_COMPAT
	.compat_ioctl = nvgpu_ioctl_tsg_dev_ioctl,
#endif
	.unlocked_ioctl = nvgpu_ioctl_tsg_dev_ioctl,
};

#ifdef CONFIG_NVGPU_FECS_TRACE
static const struct file_operations gk20a_ctxsw_ops = {
	.owner = THIS_MODULE,
	.release = gk20a_ctxsw_dev_release,
	.open = gk20a_ctxsw_dev_open,
#ifdef CONFIG_COMPAT
	.compat_ioctl = gk20a_ctxsw_dev_ioctl,
#endif
	.unlocked_ioctl = gk20a_ctxsw_dev_ioctl,
	.poll = gk20a_ctxsw_dev_poll,
	.read = gk20a_ctxsw_dev_read,
	.mmap = gk20a_ctxsw_dev_mmap,
};
#endif

static const struct file_operations gk20a_sched_ops = {
	.owner = THIS_MODULE,
	.release = gk20a_sched_dev_release,
	.open = gk20a_sched_dev_open,
#ifdef CONFIG_COMPAT
	.compat_ioctl = gk20a_sched_dev_ioctl,
#endif
	.unlocked_ioctl = gk20a_sched_dev_ioctl,
	.poll = gk20a_sched_dev_poll,
	.read = gk20a_sched_dev_read,
};

static char *nvgpu_devnode(const char *cdev_name)
{
	/* Special case to maintain legacy names */
	if (strcmp(cdev_name, "channel") == 0) {
		return kasprintf(GFP_KERNEL, "nvhost-gpu");
	}

	return kasprintf(GFP_KERNEL, "nvhost-%s-gpu", cdev_name);
}

static char *nvgpu_pci_devnode(struct device *dev, umode_t *mode)
{
	if (mode) {
		*mode = S_IRUSR | S_IWUSR;
	}

	/* Special case to maintain legacy names */
	if (strcmp(dev_name(dev), "channel") == 0) {
		return kasprintf(GFP_KERNEL, "nvgpu-pci/card-%s",
			dev_name(dev->parent));
	}

	return kasprintf(GFP_KERNEL, "nvgpu-pci/card-%s-%s",
			dev_name(dev->parent), dev_name(dev));
}

static int gk20a_create_device(
	struct device *dev, int devno,
	const char *cdev_name,
	struct cdev *cdev, struct device **out,
	const struct file_operations *ops,
	struct class *class)
{
	struct device *subdev;
	int err;
	struct gk20a *g = gk20a_from_dev(dev);
	const char *device_name = NULL;

	nvgpu_log_fn(g, " ");

	cdev_init(cdev, ops);
	cdev->owner = THIS_MODULE;

	err = cdev_add(cdev, devno, 1);
	if (err) {
		dev_err(dev, "failed to add %s cdev\n", cdev_name);
		return err;
	}

	if (class->devnode == NULL) {
		device_name = nvgpu_devnode(cdev_name);
	}

	subdev = device_create(class, dev, devno, NULL,
			device_name ? device_name : cdev_name);
	if (IS_ERR(subdev)) {
		err = PTR_ERR(dev);
		cdev_del(cdev);
		dev_err(dev, "failed to create %s device for %s\n",
			cdev_name, dev_name(dev));
		return err;
	}

	if (device_name != NULL) {
		kfree(device_name);
	}

	*out = subdev;
	return 0;
}

void gk20a_user_deinit(struct device *dev)
{
	struct gk20a *g = gk20a_from_dev(dev);
	struct nvgpu_os_linux *l = nvgpu_os_linux_from_gk20a(g);
	struct class *class = l->devnode_class;

	if (class == NULL) {
		return;
	}

	if (l->channel.node) {
		device_destroy(class, l->channel.cdev.dev);
		cdev_del(&l->channel.cdev);
	}

	if (l->as_dev.node) {
		device_destroy(class, l->as_dev.cdev.dev);
		cdev_del(&l->as_dev.cdev);
	}

	if (l->ctrl.node) {
		device_destroy(class, l->ctrl.cdev.dev);
		cdev_del(&l->ctrl.cdev);
	}

	if (l->dbg.node) {
		device_destroy(class, l->dbg.cdev.dev);
		cdev_del(&l->dbg.cdev);
	}

	if (l->prof.node) {
		device_destroy(class, l->prof.cdev.dev);
		cdev_del(&l->prof.cdev);
	}

	if (l->prof_dev.node) {
		device_destroy(class, l->prof_dev.cdev.dev);
		cdev_del(&l->prof_dev.cdev);
	}

	if (l->prof_ctx.node) {
		device_destroy(class, l->prof_ctx.cdev.dev);
		cdev_del(&l->prof_ctx.cdev);
	}

	if (l->tsg.node) {
		device_destroy(class, l->tsg.cdev.dev);
		cdev_del(&l->tsg.cdev);
	}

	if (l->ctxsw.node) {
		device_destroy(class, l->ctxsw.cdev.dev);
		cdev_del(&l->ctxsw.cdev);
	}

	if (l->sched.node) {
		device_destroy(class, l->sched.cdev.dev);
		cdev_del(&l->sched.cdev);
	}

	if (l->cdev_region)
		unregister_chrdev_region(l->cdev_region, GK20A_NUM_CDEVS);

	class_destroy(class);
	l->devnode_class = NULL;
}

int gk20a_user_init(struct device *dev)
{
	int err;
	dev_t devno;
	struct gk20a *g = gk20a_from_dev(dev);
	struct nvgpu_os_linux *l = nvgpu_os_linux_from_gk20a(g);
	struct class *class;

	if (g->pci_class != 0U) {
		class = class_create(THIS_MODULE, "nvidia-pci-gpu");
		if (IS_ERR(class)) {
			dev_err(dev, "failed to create class");
			return PTR_ERR(class);
		}
		class->devnode = nvgpu_pci_devnode;
	} else {
		class = class_create(THIS_MODULE, "nvidia-gpu");
		if (IS_ERR(class)) {
			dev_err(dev, "failed to create class");
			return PTR_ERR(class);
		}
		class->devnode = NULL;
	}

	err = alloc_chrdev_region(&devno, 0, GK20A_NUM_CDEVS, dev_name(dev));
	if (err) {
		dev_err(dev, "failed to allocate devno\n");
		goto fail;
	}
	l->cdev_region = devno;

	err = gk20a_create_device(dev, devno++, "channel",
				  &l->channel.cdev, &l->channel.node,
				  &gk20a_channel_ops,
				  class);
	if (err)
		goto fail;

	err = gk20a_create_device(dev, devno++, "as",
				  &l->as_dev.cdev, &l->as_dev.node,
				  &gk20a_as_ops,
				  class);
	if (err)
		goto fail;

	err = gk20a_create_device(dev, devno++, "ctrl",
				  &l->ctrl.cdev, &l->ctrl.node,
				  &gk20a_ctrl_ops,
				  class);
	if (err)
		goto fail;

	err = gk20a_create_device(dev, devno++, "dbg",
				  &l->dbg.cdev, &l->dbg.node,
				  &gk20a_dbg_ops,
				  class);
	if (err)
		goto fail;

	err = gk20a_create_device(dev, devno++, "prof",
				  &l->prof.cdev, &l->prof.node,
				  &gk20a_prof_ops,
				  class);
	if (err)
		goto fail;

	err = gk20a_create_device(dev, devno++, "prof-dev",
				  &l->prof_dev.cdev, &l->prof_dev.node,
				  &gk20a_prof_dev_ops,
				  class);
	if (err)
		goto fail;

	err = gk20a_create_device(dev, devno++, "prof-ctx",
				  &l->prof_ctx.cdev, &l->prof_ctx.node,
				  &gk20a_prof_ctx_ops,
				  class);
	if (err)
		goto fail;

	err = gk20a_create_device(dev, devno++, "tsg",
				  &l->tsg.cdev, &l->tsg.node,
				  &gk20a_tsg_ops,
				  class);
	if (err)
		goto fail;

#if defined(CONFIG_NVGPU_FECS_TRACE)
	err = gk20a_create_device(dev, devno++, "ctxsw",
				  &l->ctxsw.cdev, &l->ctxsw.node,
				  &gk20a_ctxsw_ops,
				  class);
	if (err)
		goto fail;
#endif

	err = gk20a_create_device(dev, devno++, "sched",
				  &l->sched.cdev, &l->sched.node,
				  &gk20a_sched_ops,
				  class);
	if (err)
		goto fail;

	l->devnode_class = class;
	return 0;
fail:
	gk20a_user_deinit(dev);
	return err;
}
