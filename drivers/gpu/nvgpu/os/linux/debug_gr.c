/*
 * Copyright (C) 2017 NVIDIA Corporation.  All rights reserved.
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

#include "debug_gr.h"
#include "os_linux.h"

#include <linux/debugfs.h>

static int gr_default_attrib_cb_size_show(struct seq_file *s, void *data)
{
	struct gk20a *g = s->private;

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

int gr_gk20a_debugfs_init(struct gk20a *g)
{
	struct nvgpu_os_linux *l = nvgpu_os_linux_from_gk20a(g);
	struct dentry *d;

	d = debugfs_create_file(
		"gr_default_attrib_cb_size", S_IRUGO, l->debugfs, g,
						&gr_default_attrib_cb_size_fops);
	if (!d)
		return -ENOMEM;

	return 0;
}

