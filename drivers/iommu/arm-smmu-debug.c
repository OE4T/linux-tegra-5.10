/*
 * Copyright (C) 2020 NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#include <linux/arm-smmu-debug.h>

static ssize_t smmu_context_filter_write(struct file *file,
					 const char __user *user_buf,
					 size_t count, loff_t *ppos)
{
	s16 cbndx;
	char *pbuf, *temp, *val;
	bool first_times = 1;
	ssize_t ret = count;
	struct seq_file *seqf = file->private_data;
	struct smmu_debugfs_info *info = seqf->private;
	unsigned long *bitmap = info->context_filter;

	/* Clear bitmap in case of user buf empty */
	if (count == 1 && *user_buf == '\n') {
		bitmap_zero(bitmap, info->num_context_banks);
		return ret;
	}

	pbuf = vmalloc(count + 1);
	if (!pbuf)
		return -ENOMEM;

	if (copy_from_user(pbuf, user_buf, count)) {
		ret = -EFAULT;
		goto end;
	}

	if (pbuf[count - 1] == '\n')
		pbuf[count - 1] = '\0';
	else
		pbuf[count] = '\0';

	temp = pbuf;

	do {
		val = strsep(&temp, ",");
		if (*val) {
			if (kstrtos16(val, 10, &cbndx))
				continue;

			/* Reset bitmap in case of negative index */
			if (cbndx < 0) {
				bitmap_fill(bitmap, info->num_context_banks);
				goto end;
			}

			if (cbndx >= info->num_context_banks) {
				dev_err(info->dev,
					"context filter index out of range\n");
				ret = -EINVAL;
				goto end;
			}

			if (first_times) {
				bitmap_zero(bitmap, info->num_context_banks);
				first_times = 0;
			}

			set_bit(cbndx, bitmap);
		}
	} while (temp);

end:
	vfree(pbuf);
	return ret;
}

static int smmu_context_filter_show(struct seq_file *s, void *unused)
{
	struct smmu_debugfs_info *info = s->private;
	unsigned long *bitmap = info->context_filter;
	int idx = 0;

	while (1) {
		idx = find_next_bit(bitmap, info->max_cbs, idx);
		if (idx >= info->num_context_banks)
			break;
		seq_printf(s, "%d,", idx);
		idx++;
	}
	seq_putc(s, '\n');
	return 0;
}

static int smmu_context_filter_open(struct inode *inode, struct file *file)
{
	return single_open(file, smmu_context_filter_show, inode->i_private);
}

static const struct file_operations smmu_context_filter_fops = {
	.open		= smmu_context_filter_open,
	.read		= seq_read,
	.write		= smmu_context_filter_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static void arm_smmu_debugfs_create(struct smmu_debugfs_info *info)
{
	struct dentry *d;

	d = debugfs_create_dir(dev_name(info->dev), NULL);
	if (!d)
		return;
	d = debugfs_create_file("context_filter", S_IRUGO | S_IWUSR,
			    d, info,
			    &smmu_context_filter_fops);
	if (!d) {
		pr_warn("Making context filter failed\n");
		return;
	}

	info->debugfs_root = d;
}

void arm_smmu_debugfs_setup(struct arm_smmu_device *smmu)
{
	struct smmu_debugfs_info *info;

	info = devm_kmalloc(smmu->dev, sizeof(struct smmu_debugfs_info),
			    GFP_KERNEL);
	if (info == NULL) {
		dev_err(smmu->dev, "Out of memoryn\n");
		return;
	}
	info->dev 		= smmu->dev;
	info->num_context_banks = smmu->num_context_banks;
	info->max_cbs		= ARM_SMMU_MAX_CBS;

	arm_smmu_debugfs_create(info);

	smmu->debug_info = info;
}

