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
#include <nvgpu/mig.h>
#include <nvgpu/nvgpu_init.h>

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
	/* Device node name */
	char name[20];
	/* file operations for device */
	const struct file_operations *fops;
	/* If node should be created for physical instance in MIG mode */
	bool mig_physical_node;
};

static const struct nvgpu_dev_node dev_node_list[] = {
	{"as",		&gk20a_as_ops,		false	},
	{"channel",	&gk20a_channel_ops,	false	},
	{"ctrl",	&gk20a_ctrl_ops,	true	},
#if defined(CONFIG_NVGPU_FECS_TRACE)
	{"ctxsw",	&gk20a_ctxsw_ops,	false	},
#endif
	{"dbg",		&gk20a_dbg_ops,		false	},
	{"prof",	&gk20a_prof_ops,	false	},
	{"prof-ctx",	&gk20a_prof_ctx_ops,	false	},
	{"prof-dev",	&gk20a_prof_dev_ops,	false	},
	{"sched",	&gk20a_sched_ops,	false	},
	{"tsg",		&gk20a_tsg_ops,		false	},
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

static char *nvgpu_mig_phys_devnode(struct device *dev, umode_t *mode)
{
	struct nvgpu_cdev_class_priv_data *priv_data;

	if (mode) {
		*mode = S_IRUSR | S_IWUSR;
	}

	priv_data = dev_get_drvdata(dev);

	if (priv_data->pci) {
		return kasprintf(GFP_KERNEL, "nvgpu/dgpu-%s/%s",
				dev_name(dev->parent), dev_name(dev));
	}

	return kasprintf(GFP_KERNEL, "nvgpu/igpu0/%s", dev_name(dev));
}

static char *nvgpu_mig_fgpu_devnode(struct device *dev, umode_t *mode)
{
	struct nvgpu_cdev_class_priv_data *priv_data;

	if (mode) {
		*mode = S_IRUSR | S_IWUSR;
	}

	priv_data = dev_get_drvdata(dev);

	if (priv_data->pci) {
		return kasprintf(GFP_KERNEL, "nvgpu/dgpu-%s/fgpu-%u-%u/%s",
				dev_name(dev->parent), priv_data->major_instance_id,
				priv_data->minor_instance_id, dev_name(dev));
	}

	return kasprintf(GFP_KERNEL, "nvgpu/igpu0/fgpu-%u-%u/%s",
				priv_data->major_instance_id,
				priv_data->minor_instance_id, dev_name(dev));
}

static int gk20a_create_device(
	struct device *dev, int devno,
	const char *cdev_name,
	struct cdev *cdev, struct device **out,
	const struct file_operations *ops,
	struct nvgpu_class *class)
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

	if (class->class->devnode == NULL) {
		device_name = nvgpu_devnode(cdev_name);
	}

	subdev = device_create(class->class, dev, devno,
			class->priv_data ? class->priv_data : NULL,
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
	struct nvgpu_cdev *cdev, *n;
	struct nvgpu_class *class, *p;

	nvgpu_list_for_each_entry_safe(cdev, n, &l->cdev_list_head, nvgpu_cdev, list_entry) {
		nvgpu_list_del(&cdev->list_entry);

		device_destroy(cdev->class, cdev->cdev.dev);
		cdev_del(&cdev->cdev);

		nvgpu_kfree(g, cdev);
	}

	if (l->cdev_region) {
		unregister_chrdev_region(l->cdev_region, l->num_cdevs);
		l->num_cdevs = 0;
	}

	nvgpu_list_for_each_entry_safe(class, p, &l->class_list_head, nvgpu_class, list_entry) {
		nvgpu_list_del(&class->list_entry);

		class_destroy(class->class);
		nvgpu_kfree(g, class);
	}
}

static struct nvgpu_class *nvgpu_create_class(struct gk20a *g, const char *class_name)
{
	struct nvgpu_os_linux *l = nvgpu_os_linux_from_gk20a(g);
	struct nvgpu_class *class;

	class = nvgpu_kzalloc(g, sizeof(*class));
	if (class == NULL) {
		return NULL;
	}

	class->class = class_create(THIS_MODULE, class_name);
	if (IS_ERR(class->class)) {
		nvgpu_err(g, "failed to create class");
		nvgpu_kfree(g, class);
		return NULL;
	}

	nvgpu_init_list_node(&class->list_entry);
	nvgpu_list_add_tail(&class->list_entry, &l->class_list_head);

	return class;
}

/*
 * GPU instance information in MIG mode should be fetched from
 * common.grmgr unit. But instance information is populated during GPU
 * poweron and device nodes are enumerated during probe.
 *
 * Handle this temporarily by adding static information of instances
 * where GPU is partitioned into two instances. In long term, this will
 * need to be handled with design changes.
 *
 * This static information should be removed once instance information
 * is fetched from common.grmgr unit.
 */
struct nvgpu_mig_static_info {
	enum nvgpu_mig_gpu_instance_type instance_type;
	u32 major_instance_id;
	u32 minor_instance_id;
};

static const struct nvgpu_mig_static_info nvgpu_default_mig_static_info[] =
{
	{
		.instance_type = NVGPU_MIG_TYPE_PHYSICAL,
	},
	{
		.instance_type = NVGPU_MIG_TYPE_MIG,
		.major_instance_id = 0,
		.minor_instance_id = 0,
	},
	{
		.instance_type = NVGPU_MIG_TYPE_MIG,
		.major_instance_id = 0,
		.minor_instance_id = 1,
	},
};

static const struct nvgpu_mig_static_info nvgpu_default_pci_mig_static_info[] =
{
	{
		.instance_type = NVGPU_MIG_TYPE_PHYSICAL,
	},
	{
		.instance_type = NVGPU_MIG_TYPE_MIG,
		.major_instance_id = 1,
		.minor_instance_id = 0,
	},
	{
		.instance_type = NVGPU_MIG_TYPE_MIG,
		.major_instance_id = 2,
		.minor_instance_id = 0,
	},
};

static int nvgpu_prepare_mig_dev_node_class_list(struct gk20a *g, u32 *num_classes)
{
	u32 class_count = 0U;
	const struct nvgpu_mig_static_info *info;
	struct nvgpu_class *class;
	u32 i;
	u32 num_instances;
	struct nvgpu_cdev_class_priv_data *priv_data;

	if (g->pci_class != 0U) {
		info = &nvgpu_default_pci_mig_static_info[0];
		num_instances = sizeof(nvgpu_default_pci_mig_static_info) /
				sizeof(nvgpu_default_pci_mig_static_info[0]);
	} else {
		info = &nvgpu_default_mig_static_info[0];
		num_instances = sizeof(nvgpu_default_mig_static_info) /
				sizeof(nvgpu_default_mig_static_info[0]);
	}

	for (i = 0U; i < num_instances; i++) {
		priv_data = nvgpu_kzalloc(g, sizeof(*priv_data));
		if (priv_data == NULL) {
			return -ENOMEM;
		}

		snprintf(priv_data->class_name, sizeof(priv_data->class_name),
			"nvidia%s-gpu-fgpu%u",
			(g->pci_class != 0U) ? "-pci" : "", i);

		class = nvgpu_create_class(g, priv_data->class_name);
		if (class == NULL) {
			kfree(priv_data);
			return -ENOMEM;
		}
		class_count++;

		if (info[i].instance_type == NVGPU_MIG_TYPE_PHYSICAL) {
			class->class->devnode = nvgpu_mig_phys_devnode;
		} else {
			class->class->devnode = nvgpu_mig_fgpu_devnode;
		}

		priv_data->local_instance_id = i;
		priv_data->major_instance_id = info[i].major_instance_id;
		priv_data->minor_instance_id = info[i].minor_instance_id;
		priv_data->pci = (g->pci_class != 0U);
		class->priv_data = priv_data;
		class->instance_type = info[i].instance_type;
	}

	*num_classes = class_count;
	return 0;
}

static int nvgpu_prepare_default_dev_node_class_list(struct gk20a *g, u32 *num_classes)
{
	struct nvgpu_class *class;
	u32 count = 0U;

	if (g->pci_class != 0U) {
		class = nvgpu_create_class(g, "nvidia-pci-gpu");
		if (class == NULL) {
			return -ENOMEM;
		}
		class->class->devnode = nvgpu_pci_devnode;
		count++;
	} else {
		class = nvgpu_create_class(g, "nvidia-gpu");
		if (class == NULL) {
			return -ENOMEM;
		}
		class->class->devnode = NULL;
		count++;
	}

	*num_classes = count;
	return 0;
}

static int nvgpu_prepare_dev_node_class_list(struct gk20a *g, u32 *num_classes)
{
	int err;

	if (nvgpu_is_enabled(g, NVGPU_SUPPORT_MIG)) {
		err = nvgpu_prepare_mig_dev_node_class_list(g, num_classes);
	} else {
		err = nvgpu_prepare_default_dev_node_class_list(g, num_classes);
	}

	return err;
}

static bool check_valid_dev_node(struct gk20a *g, struct nvgpu_class *class,
		const struct nvgpu_dev_node *node)
{
	if (nvgpu_is_enabled(g, NVGPU_SUPPORT_MIG)) {
		if ((class->instance_type == NVGPU_MIG_TYPE_PHYSICAL) &&
		    !node->mig_physical_node) {
			return false;
		}
	}

	return true;
}

int gk20a_user_init(struct device *dev)
{
	int err;
	dev_t devno;
	struct gk20a *g = gk20a_from_dev(dev);
	struct nvgpu_os_linux *l = nvgpu_os_linux_from_gk20a(g);
	struct nvgpu_class *class;
	u32 num_cdevs, total_cdevs;
	u32 num_classes;
	struct nvgpu_cdev *cdev;
	u32 cdev_index;

	nvgpu_init_list_node(&l->cdev_list_head);
	nvgpu_init_list_node(&l->class_list_head);

	err = nvgpu_prepare_dev_node_class_list(g, &num_classes);
	if (err != 0) {
		return err;
	}

	num_cdevs = sizeof(dev_node_list) / sizeof(dev_node_list[0]);
	total_cdevs = num_cdevs * num_classes;

	err = alloc_chrdev_region(&devno, 0, total_cdevs, dev_name(dev));
	if (err) {
		dev_err(dev, "failed to allocate devno\n");
		goto fail;
	}
	l->cdev_region = devno;

	nvgpu_list_for_each_entry(class, &l->class_list_head, nvgpu_class, list_entry) {
		for (cdev_index = 0; cdev_index < num_cdevs; cdev_index++) {
			if (!check_valid_dev_node(g, class, &dev_node_list[cdev_index])) {
				continue;
			}

			cdev = nvgpu_kzalloc(g, sizeof(*cdev));
			if (cdev == NULL) {
				dev_err(dev, "failed to allocate cdev\n");
				goto fail;
			}

			err = gk20a_create_device(dev, devno++,
					dev_node_list[cdev_index].name,
					&cdev->cdev, &cdev->node,
					dev_node_list[cdev_index].fops,
					class);
			if (err) {
				goto fail;
			}

			cdev->class = class->class;
			nvgpu_init_list_node(&cdev->list_entry);
			nvgpu_list_add(&cdev->list_entry, &l->cdev_list_head);
		}
	}

	l->num_cdevs = total_cdevs;

	return 0;
fail:
	gk20a_user_deinit(dev);
	return err;
}
