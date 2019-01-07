/*
 * Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
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

#include <linux/kernel.h> /* printk() */
#include <linux/slab.h>   /* kmalloc() */
#include <linux/fs.h>   /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/fcntl.h> /* O_ACCMODE */
#include <asm/uaccess.h>
#include <asm-generic/bug.h>
#include <linux/mmc/ioctl.h>
#include "tegra_vblk.h"

int vblk_prep_mmc_multi_ioc(struct vblk_dev *vblkdev,
		void __user *user,
		uint32_t cmd)
{
	int err = 0;
	struct combo_info_t *combo_info;
	struct combo_cmd_t *combo_cmd;
	int i = 0;
	uint64_t num_cmd;
	struct mmc_ioc_cmd ic;
	struct mmc_ioc_multi_cmd __user *user_cmd;
	struct mmc_ioc_cmd __user *usr_ptr;
	uint32_t combo_cmd_size;
	uint32_t ioctl_bytes = VBLK_MAX_IOCTL_SIZE;
	uint8_t *tmpaddr;
	struct vblk_ioctl_req *ioctl_req = &vblkdev->ioctl_req;
	void *ioctl_buf = &ioctl_req->ioctl_buf;

	combo_info = (struct combo_info_t *)ioctl_buf;
	combo_cmd_size = sizeof(uint32_t);

	if (cmd == MMC_IOC_MULTI_CMD) {
		user_cmd = (struct mmc_ioc_multi_cmd __user *)user;
		if (copy_from_user(&num_cmd, &user_cmd->num_of_cmds,
				sizeof(num_cmd))) {
			err = -EFAULT;
			goto exit;
		}

		if (num_cmd > MMC_IOC_MAX_CMDS) {
			err = -EINVAL;
			goto exit;
		}

		usr_ptr = (void * __user)&user_cmd->cmds;
	} else {
		num_cmd = 1;
		usr_ptr = (void * __user)user;
	}
	combo_info->count = num_cmd;

	combo_cmd = (struct combo_cmd_t *)(ioctl_buf +
		sizeof(struct combo_info_t));

	combo_cmd_size = sizeof(struct combo_info_t) +
		sizeof(struct combo_cmd_t) * combo_info->count;
	if (combo_cmd_size < sizeof(struct combo_info_t)) {
		dev_err(vblkdev->device,
			"combo_cmd_size is overflowing!\n");
		err = -EINVAL;
		goto exit;
	}

	if (combo_cmd_size > ioctl_bytes) {
		dev_err(vblkdev->device,
			" buffer has no enough space to serve ioctl\n");
		err = -EFAULT;
		goto exit;
	}

	tmpaddr = (uint8_t *)&ic;
	for (i = 0; i < combo_info->count; i++) {
		if (copy_from_user((void *)tmpaddr, usr_ptr, sizeof(ic))) {
			err = -EFAULT;
			goto exit;
		}
		combo_cmd->cmd = ic.opcode;
		combo_cmd->arg = ic.arg;
		combo_cmd->write_flag = (uint32_t)ic.write_flag;
		combo_cmd->data_len = (uint32_t)(ic.blksz * ic.blocks);
		combo_cmd->buf_offset = combo_cmd_size;
		combo_cmd_size += combo_cmd->data_len;
		if ((combo_cmd_size < combo_cmd->data_len) ||
				(combo_cmd_size > ioctl_bytes)) {
			dev_err(vblkdev->device,
				" buffer has no enough space to serve ioctl\n");
			err = -EFAULT;
			goto exit;
		}

		if (ic.write_flag && combo_cmd->data_len) {
			if (copy_from_user((
				(void *)ioctl_buf +
				combo_cmd->buf_offset),
				(void __user *)(unsigned long)ic.data_ptr,
				(u64)combo_cmd->data_len))
			{
				dev_err(vblkdev->device,
					"copy from user failed for data!\n");
				err = -EFAULT;
				goto exit;
			}
		}
		combo_cmd++;
		usr_ptr++;
	}

	ioctl_req->ioctl_id = VBLK_MMC_MULTI_IOC_ID;
	ioctl_req->ioctl_len = ioctl_bytes;

exit:
	return err;
}

int vblk_complete_mmc_multi_ioc(struct vblk_dev *vblkdev,
		void __user *user,
		uint32_t cmd)
{
	uint64_t num_cmd;
	struct mmc_ioc_cmd ic;
	struct mmc_ioc_cmd *ic_ptr = &ic;
	struct mmc_ioc_multi_cmd __user *user_cmd;
	struct mmc_ioc_cmd __user *usr_ptr;
	struct combo_cmd_t *combo_cmd;
	uint32_t i;
	int err = 0;
	struct vblk_ioctl_req *ioctl_req = &vblkdev->ioctl_req;
	void *ioctl_buf = &ioctl_req->ioctl_buf;

	if (cmd == MMC_IOC_MULTI_CMD) {
		user_cmd = (struct mmc_ioc_multi_cmd __user *)user;
		if (copy_from_user(&num_cmd, &user_cmd->num_of_cmds,
				sizeof(num_cmd))) {
			err = -EFAULT;
			goto exit;
		}

		if (num_cmd > MMC_IOC_MAX_CMDS) {
			err = -EINVAL;
			goto exit;
		}

		usr_ptr = (void * __user)&user_cmd->cmds;
	} else {
		usr_ptr = (void * __user)user;
		num_cmd = 1;
	}

	combo_cmd = (struct combo_cmd_t *)(ioctl_buf +
			sizeof(struct combo_info_t));

	for (i = 0; i < num_cmd; i++) {
		if (copy_from_user((void *)ic_ptr, usr_ptr,
			sizeof(struct mmc_ioc_cmd))) {
			err = -EFAULT;
			goto exit;
		}

		if (copy_to_user(&(usr_ptr->response), combo_cmd->response,
			sizeof(combo_cmd->response))) {
			err = -EFAULT;
			goto exit;
		}

		if (!ic.write_flag && combo_cmd->data_len) {
			if (copy_to_user(
				(void __user *)(unsigned long)ic.data_ptr,
				(ioctl_buf + combo_cmd->buf_offset),
				(u64)combo_cmd->data_len))
			{
				dev_err(vblkdev->device,
					"copy to user of ioctl data failed!\n");
				err = -EFAULT;
				goto exit;
			}
		}
		combo_cmd++;
		usr_ptr++;
	}

exit:
	return err;
}
