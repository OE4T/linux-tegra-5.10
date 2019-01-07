/*
 * Copyright (c) 2015-2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h> /* printk() */
#include <linux/fs.h>   /* everything... */
#include <linux/errno.h> /* error codes */
#include <asm-generic/bug.h>
#include <linux/slab.h>   /* kmalloc() */
#include <scsi/scsi.h>
#include <scsi/sg.h>
#include <linux/mmc/ioctl.h>
#include <linux/version.h>
#include "tegra_vblk.h"

void vblk_complete_ioctl_req(struct vblk_dev *vblkdev,
		struct vsc_request *vsc_req,
		int status)
{
	struct vblk_ioctl_req *ioctl_req = &vblkdev->ioctl_req;

	ioctl_req->status = status;
	memcpy(ioctl_req->ioctl_buf, vsc_req->mempool_virt,
			ioctl_req->ioctl_len);
}

int vblk_prep_ioctl_req(struct vblk_dev *vblkdev,
		struct vsc_request *vsc_req)
{
	int32_t ret = 0;
	struct vs_request *vs_req;
	struct vblk_ioctl_req *ioctl_req = &vblkdev->ioctl_req;

	if (ioctl_req->ioctl_len > vsc_req->mempool_len) {
		dev_err(vblkdev->device,
			"Ioctl length exceeding mempool length!\n");
		return -EINVAL;
	}

	vs_req = &vsc_req->vs_req;
	vs_req->blkdev_req.req_op = VS_BLK_IOCTL;
	memcpy(vsc_req->mempool_virt, &ioctl_req->ioctl_buf,
			ioctl_req->ioctl_len);
	vs_req->blkdev_req.ioctl_req.ioctl_id = ioctl_req->ioctl_id;
	vs_req->blkdev_req.ioctl_req.data_offset = vsc_req->mempool_offset;
	vs_req->blkdev_req.ioctl_req.ioctl_len = ioctl_req->ioctl_len;

	return ret;
}

int vblk_submit_ioctl_req(struct block_device *bdev,
		unsigned int cmd, void __user *user)
{
	struct vblk_dev *vblkdev = bdev->bd_disk->private_data;
	struct vblk_ioctl_req *ioctl_req = &vblkdev->ioctl_req;
	struct request *rq;
	int err;

	/*
	 * The caller must have CAP_SYS_RAWIO, and must be calling this on the
	 * whole block device, not on a partition.  This prevents overspray
	 * between sibling partitions.
	 */
	if ((!capable(CAP_SYS_RAWIO)) || (bdev != bdev->bd_contains))
		return -EPERM;

	switch (cmd) {
	case SG_IO:
		err = vblk_prep_sg_io(vblkdev, user);
		break;
	case MMC_IOC_MULTI_CMD:
	case MMC_IOC_CMD:
		err = vblk_prep_mmc_multi_ioc(vblkdev, user, cmd);
		break;
	default:
		dev_err(vblkdev->device, "unsupported command %x!\n", cmd);
		err = -EINVAL;
		goto exit;
	}

	if (err)
		goto exit;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
	rq = blk_get_request(vblkdev->queue, REQ_OP_DRV_IN, GFP_KERNEL);
#else
	rq = blk_get_request(vblkdev->queue, READ, GFP_KERNEL);
#endif
	if (IS_ERR_OR_NULL(rq)) {
		dev_err(vblkdev->device,
			"Failed to get handle to a request!\n");
		err = PTR_ERR(rq);
		goto exit;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
	blk_execute_rq(vblkdev->queue, vblkdev->gd, rq, 0);
	blk_put_request(rq);
#else
	rq->cmd_type = REQ_TYPE_DRV_PRIV;
	err = blk_execute_rq(vblkdev->queue, vblkdev->gd, rq, 0);
	blk_put_request(rq);

	if (err)
		goto exit;
#endif

	if (ioctl_req->status) {
		err = ioctl_req->status;
		goto exit;
	}

	switch (cmd) {
	case SG_IO:
		err = vblk_complete_sg_io(vblkdev, user);
		break;
	case MMC_IOC_MULTI_CMD:
	case MMC_IOC_CMD:
		err = vblk_complete_mmc_multi_ioc(vblkdev, user, cmd);
		break;
	default:
		dev_err(vblkdev->device, "unsupported command %x!\n", cmd);
		err = -EINVAL;
		goto exit;
	}

exit:
	return err;
}

/* The ioctl() implementation */
int vblk_ioctl(struct block_device *bdev, fmode_t mode,
	unsigned int cmd, unsigned long arg)
{
	int ret;
	struct vblk_dev *vblkdev = bdev->bd_disk->private_data;

	mutex_lock(&vblkdev->ioctl_lock);
	switch (cmd) {
	case MMC_IOC_MULTI_CMD:
	case MMC_IOC_CMD:
	case SG_IO:
		ret = vblk_submit_ioctl_req(bdev, cmd,
			(void __user *)arg);
		break;
	default:  /* unknown command */
		ret = -ENOTTY;
		break;
	}
	mutex_unlock(&vblkdev->ioctl_lock);

	return ret;
}
