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
 */

#include <linux/of_platform.h>
#include <linux/mailbox_client.h>
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include "tegra23x_psc.h"

#define MBOX_MSG_LEN 64

struct psc_debug_dev {
	struct platform_device *pdev;
	struct mbox_client cl;
	struct mbox_chan *chan;
	wait_queue_head_t read_wait;
	atomic_t open_cnt;
	atomic_t data_ready;
	u8 rx_msg[MBOX_MSG_LEN];
};

static struct psc_debug_dev psc_debug;
static struct dentry *debugfs_root;

static int psc_debug_open(struct inode *inode, struct file *file)
{
	struct mbox_chan *chan;
	struct psc_debug_dev *dbg = &psc_debug;
	struct platform_device *pdev = dbg->pdev;

	if (atomic_read(&dbg->open_cnt))
		return -EBUSY;

	file->private_data = dbg;

	chan = mbox_request_channel(&dbg->cl, 0);
	if (IS_ERR(chan) && (PTR_ERR(chan) != -EPROBE_DEFER)) {
		dev_err(&pdev->dev, "failed to get channel, err %lx\n",
			PTR_ERR(chan));
		return PTR_ERR(chan);
	}

	dev_info(&pdev->dev, "get mbox channel 0\n");
	dbg->chan = chan;

	atomic_set(&dbg->open_cnt, 0);
	atomic_set(&dbg->data_ready, 0);

	nonseekable_open(inode, file);

	return 0;
}

static int psc_debug_release(struct inode *inode, struct file *file)
{
	struct psc_debug_dev *dbg = file->private_data;

	mbox_free_channel(dbg->chan);

	atomic_set(&dbg->open_cnt, 0);
	atomic_set(&dbg->data_ready, 0);
	file->private_data = NULL;
	return 0;
}

static ssize_t psc_debug_read(struct file *file, char __user *buffer,
				size_t count, loff_t *ppos)
{
	struct psc_debug_dev *dbg = file->private_data;
	ssize_t ret;
	loff_t pos = 0;

	if (atomic_read(&dbg->data_ready) <= 0) {
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;

		if (wait_event_interruptible(dbg->read_wait,
					atomic_read(&dbg->data_ready) > 0))
			return -EINTR;
	}

	ret = simple_read_from_buffer(buffer, count, &pos,
			dbg->rx_msg, min_t(size_t, count, MBOX_MSG_LEN));
	*ppos += pos;
	atomic_set(&dbg->data_ready, 0);

	return ret;
}

static ssize_t psc_debug_write(struct file *file, const char __user *buffer,
				size_t count, loff_t *ppos)
{
	struct psc_debug_dev *dbg = file->private_data;
	struct platform_device *pdev = dbg->pdev;
	u8 tx_buf[MBOX_MSG_LEN] = { 0 };
	ssize_t ret;

	if (count > MBOX_MSG_LEN) {
		dev_err(&pdev->dev, "write size > MBOX_MSG_LEN\n");
		return -EINVAL;
	}

	if (copy_from_user(tx_buf, buffer, count)) {
		dev_err(&pdev->dev, "copy_from_user() error!\n");
		return -EFAULT;
	}
	ret = mbox_send_message(dbg->chan, tx_buf);
	return ret < 0 ? ret : count;
}

static const struct file_operations psc_debug_fops = {
	.open		= psc_debug_open,
	.read		= psc_debug_read,
	.write		= psc_debug_write,
	.llseek		= no_llseek,
	.release	= psc_debug_release,
};

static void psc_chan_rx_callback(struct mbox_client *c, void *msg)
{
	struct device *dev = c->dev;
	struct psc_debug_dev *dbg = container_of(c, struct psc_debug_dev, cl);
	uint32_t *data = msg;

	dev_dbg(dev, "data: %08x %08x %08x\n", data[0], data[1], data[2]);
	memcpy(dbg->rx_msg, msg, MBOX_MSG_LEN);

	atomic_set(&dbg->data_ready, 1);
	wake_up_interruptible(&dbg->read_wait);
}

int psc_debugfs_create(struct platform_device *pdev)
{
	struct psc_debug_dev *dbg = &psc_debug;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	int count;

	if (!debugfs_initialized())
		return -ENODEV;

	count = of_count_phandle_with_args(np, "mboxes", "#mbox-cells");
	if (count != 1) {
		dev_err(dev, "incorrect mboxes property in '%pOF'\n", np);
		return -EINVAL;
	}

	debugfs_root = debugfs_create_dir("psc", NULL);
	if (debugfs_root == NULL) {
		dev_err(dev, "failed to create psc debugfs\n");
		return -EINVAL;
	}

	debugfs_create_file("mbox_dbg", 0600, debugfs_root,
			dbg, &psc_debug_fops);

	dbg->cl.dev = dev;
	dbg->cl.rx_callback = psc_chan_rx_callback;
	dbg->cl.tx_block = true;
	dbg->cl.tx_tout = 2000;
	dbg->cl.knows_txdone = false;
	dbg->pdev = pdev;

	init_waitqueue_head(&dbg->read_wait);

	return 0;
}

void psc_debugfs_remove(struct platform_device *pdev)
{
	dev_dbg(&pdev->dev, "%s\n", __func__);

	debugfs_remove_recursive(debugfs_root);
}
