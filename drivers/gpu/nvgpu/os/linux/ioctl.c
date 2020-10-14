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

const struct file_operations gk20a_as_ops = {
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

struct nvgpu_dev_node {
	char name[20];
	const struct file_operations *fops;
};

static const struct nvgpu_dev_node dev_node_list[] = {
	{"as",		&gk20a_as_ops},
	{"channel",	&gk20a_channel_ops},
	{"ctrl",	&gk20a_ctrl_ops},
#if defined(CONFIG_NVGPU_FECS_TRACE)
	{"ctxsw",	&gk20a_ctxsw_ops},
#endif
	{"dbg",		&gk20a_dbg_ops},
	{"prof",	&gk20a_prof_ops},
	{"prof-ctx",	&gk20a_prof_ctx_ops},
	{"prof-dev",	&gk20a_prof_dev_ops},
	{"sched",	&gk20a_sched_ops},
	{"tsg",		&gk20a_tsg_ops},
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
	struct nvgpu_cdev *cdev, *n;

	if (class == NULL) {
		return;
	}

	nvgpu_list_for_each_entry_safe(cdev, n, &l->cdev_list_head, nvgpu_cdev, list_entry) {
		nvgpu_list_del(&cdev->list_entry);

		device_destroy(class, cdev->cdev.dev);
		cdev_del(&cdev->cdev);

		nvgpu_kfree(g, cdev);
	}

	if (l->cdev_region) {
		unregister_chrdev_region(l->cdev_region, l->num_cdevs);
	}

	class_destroy(class);
	l->devnode_class = NULL;
	l->num_cdevs = 0;
}

int gk20a_user_init(struct device *dev)
{
	int err;
	dev_t devno;
	struct gk20a *g = gk20a_from_dev(dev);
	struct nvgpu_os_linux *l = nvgpu_os_linux_from_gk20a(g);
	struct class *class;
	u32 num_cdevs;
	struct nvgpu_cdev *cdev;
	u32 cdev_index;

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

	num_cdevs = sizeof(dev_node_list) / sizeof(dev_node_list[0]);
	err = alloc_chrdev_region(&devno, 0, num_cdevs, dev_name(dev));
	if (err) {
		dev_err(dev, "failed to allocate devno\n");
		goto fail;
	}
	l->cdev_region = devno;

	nvgpu_init_list_node(&l->cdev_list_head);

	for (cdev_index = 0; cdev_index < num_cdevs; cdev_index++) {
		cdev = nvgpu_kzalloc(g, sizeof(*cdev));
		if (cdev == NULL) {
			dev_err(dev, "failed to allocate cdev\n");
			goto fail;
		}

		err = gk20a_create_device(dev, devno++, dev_node_list[cdev_index].name,
					  &cdev->cdev, &cdev->node,
					  dev_node_list[cdev_index].fops,
					  class);
		if (err) {
			goto fail;
		}

		nvgpu_init_list_node(&cdev->list_entry);
		nvgpu_list_add(&cdev->list_entry, &l->cdev_list_head);
	}

	l->num_cdevs = num_cdevs;
	l->devnode_class = class;

	return 0;
fail:
	gk20a_user_deinit(dev);
	return err;
}
