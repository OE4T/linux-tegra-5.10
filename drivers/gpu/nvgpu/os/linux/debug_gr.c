/*
 * Copyright (C) 2017-2020 NVIDIA Corporation.  All rights reserved.
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

#include <nvgpu/gk20a.h>
#include <nvgpu/gr/ctx.h>

#include "common/gr/ctx_priv.h"
#include "common/gr/gr_priv.h"

#include "debug_gr.h"
#include "os_linux.h"

#include <linux/uaccess.h>
#include <linux/debugfs.h>

static int gr_default_attrib_cb_size_show(struct seq_file *s, void *data)
{
	struct gk20a *g = s->private;

	/* HAL might not be initialized yet */
	if (g->ops.gr.init.get_attrib_cb_default_size == NULL)
		return -EFAULT;

	seq_printf(s, "%u\n", g->ops.gr.init.get_attrib_cb_default_size(g));

	return 0;
}

static int gr_default_attrib_cb_size_open(struct inode *inode,
	struct file *file)
{
	return single_open(file, gr_default_attrib_cb_size_show,
		inode->i_private);
}

static const struct file_operations gr_default_attrib_cb_size_fops= {
	.open		= gr_default_attrib_cb_size_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static ssize_t force_preemption_gfxp_read(struct file *file,
		char __user *user_buf, size_t count, loff_t *ppos)
{
	char buf[3];
	struct gk20a *g = file->private_data;

	if (g->gr->gr_ctx_desc == NULL) {
		return -EFAULT;
	}

	if (g->gr->gr_ctx_desc->force_preemption_gfxp) {
		buf[0] = 'Y';
	} else {
		buf[0] = 'N';
	}

	buf[1] = '\n';
	buf[2] = 0x00;

	return simple_read_from_buffer(user_buf, count, ppos, buf, 2);
}

static ssize_t force_preemption_gfxp_write(struct file *file,
		const char __user *user_buf, size_t count, loff_t *ppos)
{
	char buf[32];
	int buf_size;
	bool val;
	struct gk20a *g = file->private_data;

	if (g->gr->gr_ctx_desc == NULL) {
		return -EFAULT;
	}

	buf_size = min(count, (sizeof(buf)-1));
	if (copy_from_user(buf, user_buf, buf_size)) {
		return -EFAULT;
	}

	if (strtobool(buf, &val) == 0) {
		g->gr->gr_ctx_desc->force_preemption_gfxp = val;
	}

	return count;
}

static struct file_operations force_preemption_gfxp_fops = {
	.open =		simple_open,
	.read =		force_preemption_gfxp_read,
	.write =	force_preemption_gfxp_write,
};

static ssize_t force_preemption_cilp_read(struct file *file,
		 char __user *user_buf, size_t count, loff_t *ppos)
{
	char buf[3];
	struct gk20a *g = file->private_data;

	if (g->gr->gr_ctx_desc == NULL) {
		return -EFAULT;
	}

	if (g->gr->gr_ctx_desc->force_preemption_cilp) {
		buf[0] = 'Y';
	} else {
		buf[0] = 'N';
	}

	buf[1] = '\n';
	buf[2] = 0x00;

	return simple_read_from_buffer(user_buf, count, ppos, buf, 2);
}

static ssize_t force_preemption_cilp_write(struct file *file,
		const char __user *user_buf, size_t count, loff_t *ppos)
{
	char buf[32];
	int buf_size;
	bool val;
	struct gk20a *g = file->private_data;

	if (g->gr->gr_ctx_desc == NULL) {
		return -EFAULT;
	}

	buf_size = min(count, (sizeof(buf)-1));
	if (copy_from_user(buf, user_buf, buf_size)) {
		return -EFAULT;
	}

	if (strtobool(buf, &val) == 0) {
		g->gr->gr_ctx_desc->force_preemption_cilp = val;
	}

	return count;
}

static struct file_operations force_preemption_cilp_fops = {
	.open =		simple_open,
	.read =		force_preemption_cilp_read,
	.write =	force_preemption_cilp_write,
};

static ssize_t dump_ctxsw_stats_on_channel_close_read(struct file *file,
		char __user *user_buf, size_t count, loff_t *ppos)
{
	char buf[3];
	struct gk20a *g = file->private_data;

	if (g->gr->gr_ctx_desc == NULL) {
		return -EFAULT;
	}

	if (g->gr->gr_ctx_desc->dump_ctxsw_stats_on_channel_close) {
		buf[0] = 'Y';
	} else {
		buf[0] = 'N';
	}

	buf[1] = '\n';
	buf[2] = 0x00;

	return simple_read_from_buffer(user_buf, count, ppos, buf, 2);
}

static ssize_t dump_ctxsw_stats_on_channel_close_write(struct file *file,
		const char __user *user_buf, size_t count, loff_t *ppos)
{
	char buf[32];
	int buf_size;
	bool val;
	struct gk20a *g = file->private_data;

	if (g->gr->gr_ctx_desc == NULL) {
		return -EFAULT;
	}

	buf_size = min(count, (sizeof(buf)-1));
	if (copy_from_user(buf, user_buf, buf_size)) {
		return -EFAULT;
	}

	if (strtobool(buf, &val) == 0) {
		g->gr->gr_ctx_desc->dump_ctxsw_stats_on_channel_close = val;
	}

	return count;
}

static struct file_operations dump_ctxsw_stats_on_channel_close_fops = {
	.open =		simple_open,
	.read =		dump_ctxsw_stats_on_channel_close_read,
	.write =	dump_ctxsw_stats_on_channel_close_write,
};

int gr_gk20a_debugfs_init(struct gk20a *g)
{
	struct nvgpu_os_linux *l = nvgpu_os_linux_from_gk20a(g);
	struct dentry *d;

	d = debugfs_create_file(
		"gr_default_attrib_cb_size", S_IRUGO, l->debugfs, g,
				&gr_default_attrib_cb_size_fops);
	if (!d)
		return -ENOMEM;

	d = debugfs_create_file(
		"force_preemption_gfxp", S_IRUGO|S_IWUSR, l->debugfs, g,
				&force_preemption_gfxp_fops);
	if (!d)
		return -ENOMEM;

	d = debugfs_create_file(
		"force_preemption_cilp", S_IRUGO|S_IWUSR, l->debugfs, g,
				&force_preemption_cilp_fops);
	if (!d)
		return -ENOMEM;

	if (!g->is_virtual) {
		d = debugfs_create_file(
			"dump_ctxsw_stats_on_channel_close", S_IRUGO|S_IWUSR,
				l->debugfs, g,
				&dump_ctxsw_stats_on_channel_close_fops);
		if (!d)
			return -ENOMEM;
	}

	return 0;
}

