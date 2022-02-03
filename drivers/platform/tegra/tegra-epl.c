/*
 * Copyright (c) 2021-2022, NVIDIA CORPORATION. All rights reserved.
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

/**
 * @file tegra-epl.c
 * @brief <b> epl driver to communicate with eplcom daemon</b>
 *
 * This file will register as client driver so that EPL client can
 * report SW error to FSI using HSP mailbox from user space
 */

/* ==================[Includes]============================================= */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/dma-buf.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/mailbox_client.h>
#include <linux/sched/signal.h>
#include "linux/tegra-epl.h"


/*Timeout in millisec*/
#define TIMEOUT		1000


/*32bit data Length*/
#define MAX_LEN	4

/* =================[Data types]======================================== */

/*Data type for mailbox client and channel details*/
struct epl_hsp_sm {
	struct mbox_client client;
	struct mbox_chan *chan;
};

/*Data type for accessing TOP2 HSP */
struct epl_hsp {
	struct epl_hsp_sm tx;
	struct device dev;
};

/* =================[GLOBAL variables]================================== */
static ssize_t device_file_ioctl(
		struct file *, unsigned int cmd, unsigned long arg);

static int device_file_major_number;
static const char device_name[] = "epdaemon";

static struct platform_device *pdev_local;

static struct epl_hsp *epl_hsp_v;

/*File operations*/
const static struct file_operations epl_driver_fops = {
	.owner   = THIS_MODULE,
	.unlocked_ioctl   = device_file_ioctl,
};

static int epl_register_device(void)
{
	int result = 0;
	struct class *dev_class;

	result = register_chrdev(0, device_name, &epl_driver_fops);
	if (result < 0) {
		pr_err("%s> register_chrdev code = %i\n", device_name, result);
		return result;
	}
	device_file_major_number = result;
	dev_class = class_create(THIS_MODULE, device_name);
	if (dev_class == NULL) {
		pr_err("%s> Could not create class for device\n", device_name);
		goto class_fail;
	}

	if ((device_create(dev_class, NULL,
		MKDEV(device_file_major_number, 0),
			 NULL, device_name)) == NULL) {
		pr_err("%s> Could not create device node\n", device_name);
		goto device_failure;
	}
	return 0;

device_failure:
	class_destroy(dev_class);
class_fail:
	unregister_chrdev(device_file_major_number, device_name);
	return -1;
}

static void tegra_hsp_tx_empty_notify(struct mbox_client *cl,
					 void *data, int empty_value)
{
	pr_debug("EPL: TX empty callback came\n");
}

static int tegra_hsp_mb_init(struct device *dev)
{
	int err;

	epl_hsp_v = devm_kzalloc(dev, sizeof(*epl_hsp_v), GFP_KERNEL);
	if (!epl_hsp_v)
		return -ENOMEM;

	epl_hsp_v->tx.client.dev = dev;
	epl_hsp_v->tx.client.tx_block = true;
	epl_hsp_v->tx.client.tx_tout = TIMEOUT;
	epl_hsp_v->tx.client.tx_done = tegra_hsp_tx_empty_notify;

	epl_hsp_v->tx.chan = mbox_request_channel_byname(&epl_hsp_v->tx.client,
							"epl-tx");
	if (IS_ERR(epl_hsp_v->tx.chan)) {
		err = PTR_ERR(epl_hsp_v->tx.chan);
		dev_err(dev, "failed to get tx mailbox: %d\n", err);
		devm_kfree(dev, epl_hsp_v);
		return err;
	}

	return 0;
}

static void epl_unregister_device(void)
{
	if (device_file_major_number != 0)
		unregister_chrdev(device_file_major_number, device_name);
}

static ssize_t device_file_ioctl(
			struct file *fp, unsigned int cmd, unsigned long arg)
{
	uint32_t lData[MAX_LEN];
	int ret;

	if (copy_from_user(lData, (uint8_t *)arg,
				 MAX_LEN * sizeof(uint32_t)))
		return -EACCES;

	switch (cmd) {

	case EPL_REPORT_ERROR_CMD:
		ret = mbox_send_message(epl_hsp_v->tx.chan,
						(void *) lData);
		break;

	default:
		return -EINVAL;
	}

	return ret;
}

static const struct of_device_id epl_client_dt_match[] = {
	{ .compatible = "nvidia,tegra234-epl-client"},
	{}
};

MODULE_DEVICE_TABLE(of, epl_client_dt_match);

static int epl_client_probe(struct platform_device *pdev)
{
	int ret = 0;

	epl_register_device();
	ret = tegra_hsp_mb_init(&pdev->dev);
	pdev_local = pdev;

	return ret;
}

static int epl_client_remove(struct platform_device *pdev)
{
	epl_unregister_device();
	devm_kfree(&pdev->dev, epl_hsp_v);
	return 0;
}

static struct platform_driver epl_client = {
	.driver         = {
	.name   = "epl_client",
		.probe_type = PROBE_PREFER_ASYNCHRONOUS,
		.of_match_table = of_match_ptr(epl_client_dt_match),
	},
	.probe          = epl_client_probe,
	.remove         = epl_client_remove,
};

module_platform_driver(epl_client);

MODULE_DESCRIPTION("tegra: Error Propagation Library driver");
MODULE_AUTHOR("Prashant Shaw <pshaw@nvidia.com>");
MODULE_LICENSE("GPL v2");
