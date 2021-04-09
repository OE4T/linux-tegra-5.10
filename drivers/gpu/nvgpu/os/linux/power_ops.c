/*
 * Copyright (c) 2021, NVIDIA Corporation.  All rights reserved.
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
#include <linux/cdev.h>
#include <linux/file.h>
#include <linux/slab.h>
#include <linux/anon_inodes.h>
#include <linux/fs.h>
#include <uapi/linux/nvgpu.h>

#include <nvgpu/kmem.h>
#include <nvgpu/log.h>
#include <nvgpu/enabled.h>
#include <nvgpu/sizes.h>
#include <nvgpu/list.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/nvgpu_init.h>

#include "ioctl.h"

#include "platform_gk20a.h"
#include "os_linux.h"

#define NVGPU_DRIVER_POWER_ON_NEEDED	1

int gk20a_power_open(struct inode *inode, struct file *filp)
{
	struct gk20a *g;
	struct nvgpu_cdev *cdev;

	cdev = container_of(inode->i_cdev, struct nvgpu_cdev, cdev);
	g = nvgpu_get_gk20a_from_cdev(cdev);
	filp->private_data = g;

	g = nvgpu_get(g);
	if (!g) {
		return -ENODEV;
	}

	return 0;
}

int gk20a_power_read(struct file *filp, char __user *buf,
		size_t size, loff_t *off)
{
	struct gk20a *g = filp->private_data;
	u32 power_status = 0U;
	char power_out[2] = "";

	if (!g) {
		return -ENODEV;
	}

	power_status = g->power_on_state;
	power_out[0] = power_status + '0';

	if (size < sizeof(power_out)) {
		return -EINVAL;
	}

	if (*off >= (loff_t)sizeof(power_out)) {
		return 0;
	}

	if (*off + (loff_t)size > (loff_t)sizeof(power_out)) {
		size = sizeof(u32) - *off;
	}

	if (copy_to_user(buf, (char *)power_out + *off, size)) {
		return -EINVAL;
	}

	*off += size;
	return size;
}


int gk20a_power_write(struct file *filp, const char __user *buf,
		size_t size, loff_t *off)
{
	struct gk20a *g = filp->private_data;
	u32 power_status = 0U;
	int err = 0;
	unsigned char *userinput = NULL;

	if (!g) {
		return -ENODEV;
	}

	userinput = (unsigned char *)kzalloc(size, GFP_KERNEL);
	if (!userinput) {
		return -ENOMEM;
	}

	if (copy_from_user(userinput, buf, size)) {
		kfree(userinput);
		return -EFAULT;
	}

	if (kstrtouint(userinput, 10, &power_status)) {
		kfree(userinput);
		return -EINVAL;
	}

	if (power_status == NVGPU_DRIVER_POWER_ON_NEEDED) {
		if ((g->power_on_state == NVGPU_STATE_POWERING_ON) ||
				(g->power_on_state == NVGPU_STATE_POWERED_ON)) {
					goto free_input;
		}

		err = gk20a_busy(g);
		if (err) {
			nvgpu_err(g, "power_node_write failed at busy");
			kfree(userinput);
			return err;
		}

		gk20a_idle(g);
	} else {
		nvgpu_err(g, "1 is the valid value to power-on the GPU");
		kfree(userinput);
		return -EINVAL;
	}

free_input:
	*off += size;
	kfree(userinput);
	return size;
}

int gk20a_power_release(struct inode *inode, struct file *filp)
{
	struct gk20a *g;
	struct nvgpu_cdev *cdev;

	cdev = container_of(inode->i_cdev, struct nvgpu_cdev, cdev);
	g = nvgpu_get_gk20a_from_cdev(cdev);
	nvgpu_put(g);
	return 0;
}
