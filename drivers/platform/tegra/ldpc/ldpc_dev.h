/*
 * Copyright (c) 2020 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef __LDPC_DEV_H
#define __LDPC_DEV_H

#include <linux/types.h>
#include <linux/cdev.h>

#define LDPC_MAX_IORESOURCE_MEM 5
#define FW_NAME_MAX_SZ 32U

struct ldpc_devdata {
	struct class *class;
	struct cdev cdev;
	struct device *dev;
	dev_t dev_nr;
	struct platform_device *pdev;
	struct dentry *debugfs_dir;
	struct dentry *fv;
	int major;
	int minor;
	char fw_name[FW_NAME_MAX_SZ];
	void __iomem *aperture[LDPC_MAX_IORESOURCE_MEM];
};

#endif
