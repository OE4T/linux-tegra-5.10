/*
 * Copyright (c) 2020 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/errno.h>

struct ldpc_devdata {
	struct class *class;
	struct cdev cdev;
	struct device *dev;
	dev_t dev_nr;
	struct platform_device *pdev;
	int major;
	int minor;
};

int ldpc_open(struct inode *inode, struct file *filp)
{
	return 0;
}

int ldpc_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static const struct file_operations ldpc_fops = {
	.owner = THIS_MODULE,
	.open = ldpc_open,
	.release = ldpc_release,
};

static const struct of_device_id ldpc_of_match[] = {
	{
		.compatible = "nvidia,tegra-ldpc-enc",
	},
	{
		.compatible = "nvidia,tegra-ldpc-dec",
	},
	{}
};
MODULE_DEVICE_TABLE(of, ldpc_of_match);

static int ldpc_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct ldpc_devdata *ldpc_data = NULL;
	const char* devname;
	char * class_name;
	char * node_name;
	int ret = 0;

	if (np == NULL) {
		pr_err("ldpc KO: corresponding device not found\n");
		ret = -ENXIO;
		goto fail;
	}

	of_property_read_string(np,"nvidia,devname",&devname);
	if (strcmp(devname, "ldpc-enc") == 0) {
		class_name = "ldpc_enc_class";
		node_name = "ldpc-enc";
	}
	else if (strcmp(devname, "ldpc-dec") == 0) {
		class_name = "ldpc_dec_class";
		node_name = "ldpc-dec";
	}
	else {
		pr_err("ldpc KO: DT node does not have correct devname value\n");
		ret = -EINVAL;
		goto fail;
	}

	ldpc_data = devm_kzalloc(&pdev->dev, sizeof(struct ldpc_devdata), GFP_KERNEL);
	if (ldpc_data == NULL) {
		pr_err("ldpc KO: failed to allocate memory\n");
		ret = -ENOMEM;
		goto fail;
	}
	ldpc_data->dev = &pdev->dev;
	platform_set_drvdata(pdev, ldpc_data);
	ldpc_data->class = class_create(THIS_MODULE, class_name);
	if (IS_ERR(ldpc_data->class)) {
		ret = PTR_ERR(ldpc_data->class);
		pr_err("ldpc KO: failed to create class\n");
		goto fail;
	}

	ret = alloc_chrdev_region(&ldpc_data->dev_nr, 0, 1, node_name);
	if (ret < 0) {
		pr_err("ldpc KO: failed to reserve chrdev region\n");
		goto fail_chrdev;
	}
	ldpc_data->major = MAJOR(ldpc_data->dev_nr);
	ldpc_data->minor = MINOR(ldpc_data->dev_nr);
	ldpc_data->cdev.owner = THIS_MODULE;
	cdev_init(&ldpc_data->cdev, &ldpc_fops);
	ret = cdev_add(&ldpc_data->cdev, ldpc_data->dev_nr, 1);
	if (ret < 0) {
		pr_err("ldpc KO: failed to add char dev\n");
		goto fail_add;
	}
	ldpc_data->dev = device_create(ldpc_data->class, &pdev->dev,
				ldpc_data->dev_nr, NULL, "%s", node_name);
	if (IS_ERR(ldpc_data->dev)) {
		ret = PTR_ERR(ldpc_data->dev);
		pr_err("ldpc KO: failed to create device node\n");
		goto fail_device;
	}
	return ret;

fail_device:
	cdev_del(&ldpc_data->cdev);

fail_add:
	unregister_chrdev_region(ldpc_data->dev_nr, 1);

fail_chrdev:
	class_destroy(ldpc_data->class);

fail:
	return ret;
}

static int ldpc_remove(struct platform_device *pdev)
{
	struct ldpc_devdata *ldpc_data = NULL;

	ldpc_data = platform_get_drvdata(pdev);
	if (ldpc_data) {
		device_destroy(ldpc_data->class, ldpc_data->dev_nr);
		cdev_del(&ldpc_data->cdev);
		unregister_chrdev_region(ldpc_data->dev_nr, 1);
		class_destroy(ldpc_data->class);
	}
	return 0;
}

static struct platform_driver ldpc_driver = {
	.probe = ldpc_probe,
	.remove = ldpc_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = "ldpc",
		.of_match_table = ldpc_of_match,
	},
};

static int __init ldpc_init(void)
{
	int ret;

	pr_info("ldpc KO: LDPC init\n");
	ret = platform_driver_register(&ldpc_driver);
	if (ret < 0) {
		pr_err("ldpc KO: Failed to register driver\n");
		return ret;
	}
	return 0;
}

static void __exit ldpc_exit(void)
{
	platform_driver_unregister(&ldpc_driver);
	pr_info("ldpc KO: LDPC exit\n");
	return;
}

module_init(ldpc_init);
module_exit(ldpc_exit);
MODULE_LICENSE("GPL v2");
