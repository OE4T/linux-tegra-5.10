/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020, NVIDIA Corporation. All rights reserved.
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
#include <linux/debugfs.h>
#include <uapi/linux/ldpc_ioctl.h>

#include "ldpc_dev.h"

int ldpc_open(struct inode *inode, struct file *filp)
{
	return 0;
}

int ldpc_release(struct inode *inode, struct file *filp)
{
	return 0;
}

int ldpc_ioctl_get_kmd_version(void __user *arg)
{
	int ret = 0;
	struct ldpc_kmd_buf op;

	if (copy_from_user(&op, arg, sizeof(op))) {
		pr_warn("ldpc KO: failed to copy data from user\n");
		ret = -EFAULT;
		goto fail;
	}
	//TODO: Later on change this to the actual KMD version
	strcpy(op.kmd_version,"1.0");
	if (copy_to_user(arg, &op, sizeof(op))) {
		pr_warn("ldpc KO: failed to copy KMD version to user\n");
		ret = -EFAULT;
	}
fail:
	return ret;
}

static long ldpc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	if (_IOC_TYPE(cmd) != LDPC_IOCTL_MAGIC) {
		return -ENOTTY;
	}

	if (_IOC_NR(cmd) > LDPC_IOC_MAXNR) {
		return -ENOTTY;
	}

	if (_IOC_DIR(cmd) & _IOC_READ) {
		ret = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	}
	else if (_IOC_DIR(cmd) & _IOC_WRITE) {
		ret  =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	}
	if (ret) {
		return -EFAULT;
	}

	switch (cmd) {
	case LDPC_IOCTL_KMD_VER:
		ret = ldpc_ioctl_get_kmd_version((void __user*)arg);
		break;
	default:
		pr_warn("ldpc KO: Invalid IOCTL cmd\n");
		ret = -EINVAL;
		break;
	}

	return ret;
}

static const struct file_operations ldpc_fops = {
	.owner = THIS_MODULE,
	.open = ldpc_open,
	.release = ldpc_release,
        .unlocked_ioctl = ldpc_ioctl,
#ifdef CONFIG_COMPAT
        .compat_ioctl = ldpc_ioctl,
#endif
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

static int show_fw_version(struct seq_file *s, void *data)
{
	/*
	* TODO: For now dummy FW version is added as 1.0
	* This should be changed later on.
	*/
	seq_printf(s, "version=\"%s\"\n", "1.0");
	return 0;
}

static int fw_version_open(struct inode *inode, struct file *file)
{
	return single_open(file, show_fw_version, inode->i_private);
}

static const struct file_operations version_fops = {
	.open = fw_version_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/*
* Description:
* Create debugfs directory for the corresponding device (i.e. encoder/decoder)
* and the required debugfs files inside it.
*
* @devname: device name i.e. ldpc-enc or ldpc-dec
*/
void create_debugfs(struct ldpc_devdata *ldpc_data, const char *devname)
{

	ldpc_data -> debugfs_dir = debugfs_create_dir(devname, NULL);
	if (IS_ERR_OR_NULL(ldpc_data -> debugfs_dir)) {
		pr_err("ldpc KO: Not able to create the debugfs directory %s\n",devname);
		return;
	}
	/*
	* Create debugfs file for the firmware version
	*/
	ldpc_data -> fv = debugfs_create_file("firmware_version", S_IRUSR, ldpc_data -> debugfs_dir,
			NULL, &version_fops);
	if (!(ldpc_data->fv)) {
		pr_err("ldpc KO: Not able to create the firmware_version debugfs for %s\n",devname);
		return;
	}
}

/* Create encoder/decoder device mapping */
static int ldpc_device_get_resources(struct ldpc_devdata *pdata)
{
	int i;
	void __iomem *regs = NULL;
	struct platform_device *dev = pdata->pdev;
	int ret;

	for (i = 0; i < dev->num_resources; i++) {
		struct resource *r = NULL;

		r = platform_get_resource(dev, IORESOURCE_MEM, i);
		/* We've run out of mem resources */
		if (!r)
			break;

		regs = devm_ioremap_resource(&dev->dev, r);
		if (IS_ERR(regs)) {
			ret = PTR_ERR(regs);
			goto fail;
		}

		pdata->aperture[i] = regs;
	}

	return 0;

fail:
	dev_err(&dev->dev, "failed to get register memory\n");

	return ret;
}

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
	create_debugfs(ldpc_data, devname);

	ldpc_data->pdev = pdev;
	ret = ldpc_device_get_resources(ldpc_data);
	if (ret != 0) {
		pr_err("ldpc KO: failed to create device mapping:[%d]\n", ret);
		goto fail_get_res;
	}
	return ret;

fail_get_res:
	debugfs_remove_recursive(ldpc_data->debugfs_dir);
	device_destroy(ldpc_data->class, ldpc_data->dev_nr);

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
		debugfs_remove_recursive(ldpc_data->debugfs_dir);
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

MODULE_DESCRIPTION("LDPC KMD");
MODULE_AUTHOR("Ketan Patil <ketanp@nvidia.com>");
MODULE_LICENSE("GPL v2");
