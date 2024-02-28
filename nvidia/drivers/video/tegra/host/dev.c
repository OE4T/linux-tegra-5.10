/*
 * drivers/video/tegra/host/dev.c
 *
 * Tegra Graphics Host Driver Entrypoint
 *
 * Copyright (c) 2010-2018, NVIDIA Corporation. All rights reserved.
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

#define CREATE_TRACE_POINTS
#include <trace/events/nvhost.h>

#include <linux/module.h>
#include <linux/io.h>
#include <linux/nvhost.h>
#include <linux/list.h>
#include <linux/slab.h>

#include "dev.h"

#if defined(NVHOST_DEBUG)
u32 nvhost_dbg_mask = NVHOST_DEFAULT_DBG_MASK;
u32 nvhost_dbg_ftrace;
#endif

/* host1x device list is used in 2 places:
 * 1. In ioctl(NVHOST_IOCTL_CTRL_MODULE_REGRDWR) of host1x device
 * 2. debug-fs dump of host1x and client device
 *    as well as channel state */
struct nvhost_device_list {
	struct list_head list;
	struct platform_device *pdev;
};

/* HEAD for the host1x device list */
static struct nvhost_device_list ndev_head;

/* Constructor for the host1x device list */
void nvhost_device_list_init(void)
{
	INIT_LIST_HEAD(&ndev_head.list);
}

/* Adds a device to tail of host1x device list */
int nvhost_device_list_add(struct platform_device *pdev)
{
	struct nvhost_device_list *list;

	list = kzalloc(sizeof(struct nvhost_device_list), GFP_KERNEL);
	if (!list) {
		nvhost_err(&pdev->dev,
			   "could not allocate device list");
		return -ENOMEM;
	}

	list->pdev = pdev;
	list_add_tail(&list->list, &ndev_head.list);

	return 0;
}

/* Iterator function for host1x device list
 * It takes a fptr as an argument and calls that function for each
 * device in the list */
void nvhost_device_list_for_all(void *data,
	int (*fptr)(struct platform_device *pdev, void *fdata, int locked_id),
	int locked_id)
{
	struct list_head *pos;
	struct nvhost_device_list *nlist;
	int ret;

	list_for_each(pos, &ndev_head.list) {
		nlist = list_entry(pos, struct nvhost_device_list, list);
		if (nlist && nlist->pdev && fptr) {
			ret = fptr(nlist->pdev, data, locked_id);
			if (ret) {
				pr_info("%s: iterator error\n", __func__);
				break;
			}
		}
	}
}

/* Simple search function for the host1x device list
 * It takes the moduleid as argument and returns a pointer to host1x
 * device that matches the moduleid otherwise returns NULL */
struct platform_device *nvhost_device_list_match_by_id(u32 id)
{
	struct list_head *pos;
	struct nvhost_device_list *nlist;
	struct nvhost_device_data *pdata;

	list_for_each(pos, &ndev_head.list) {
		nlist = list_entry(pos, struct nvhost_device_list, list);
		if (nlist && nlist->pdev) {
			pdata = platform_get_drvdata(nlist->pdev);
			if (pdata && (pdata->moduleid == id))
				return nlist->pdev;
		}
	}

	return NULL;
}

void nvhost_client_devfs_name_init(struct platform_device *pdev)
{
	struct nvhost_device_data *pdata = platform_get_drvdata(pdev);
	struct nvhost_device_data *other_pdata;
	struct list_head *pos;
	struct nvhost_device_list *nlist;
	char *name_family = pdata->devfs_name_family;
	int cur_max_id = -1;

	if (!name_family)
		return;

	list_for_each(pos, &ndev_head.list) {
		nlist = list_entry(pos, struct nvhost_device_list, list);

		other_pdata = platform_get_drvdata(nlist->pdev);
		if (other_pdata->devfs_name_family &&
		    strcmp(name_family, other_pdata->devfs_name_family) == 0 &&
		    other_pdata->id > cur_max_id) {
			cur_max_id = other_pdata->id;
		}
	}

	pdata->id = cur_max_id + 1;
	pdata->devfs_name = kasprintf(GFP_KERNEL, "%s%d",
				      name_family, pdata->id);
}

/* Removes a device from the host1x device list */
void nvhost_device_list_remove(struct platform_device *pdev)
{
	struct list_head *pos;
	struct nvhost_device_list *nlist;
	struct nvhost_device_data *pdata = platform_get_drvdata(pdev);

	if (pdata && pdata->devfs_name_family)
		kfree(pdata->devfs_name);

	list_for_each(pos, &ndev_head.list) {
		nlist = list_entry(pos, struct nvhost_device_list, list);
		if (nlist && nlist->pdev == pdev) {
			list_del(&nlist->list);
			kfree(nlist);
			break;
		}
	}
}

MODULE_AUTHOR("NVIDIA");
MODULE_DESCRIPTION("Graphics host driver for Tegra products");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform-nvhost");
