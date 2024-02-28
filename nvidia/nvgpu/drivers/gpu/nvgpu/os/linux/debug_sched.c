/*
 * Copyright (c) 2017-2022, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "debug_sched.h"
#include "os_linux.h"
#include <nvgpu/nvgpu_init.h>

#include <linux/debugfs.h>
#include <linux/seq_file.h>

static int gk20a_sched_debugfs_show(struct seq_file *s, void *unused)
{
	struct gk20a *g = s->private;
	struct nvgpu_sched_ctrl *sched = &g->sched_ctrl;
	bool sched_busy = true;

	int n = sched->bitmap_size / sizeof(u64);
	int i;
	int err;

	err = gk20a_busy(g);
	if (err)
		return err;

	if (nvgpu_mutex_tryacquire(&sched->busy_lock)) {
		sched_busy = false;
		nvgpu_mutex_release(&sched->busy_lock);
	}

	seq_printf(s, "control_locked=%d\n", sched->control_locked);
	seq_printf(s, "busy=%d\n", sched_busy);
	seq_printf(s, "bitmap_size=%zu\n", sched->bitmap_size);

	nvgpu_mutex_acquire(&sched->status_lock);

	seq_puts(s, "active_tsg_bitmap\n");
	for (i = 0; i < n; i++)
		seq_printf(s, "\t0x%016llx\n", sched->active_tsg_bitmap[i]);

	seq_puts(s, "recent_tsg_bitmap\n");
	for (i = 0; i < n; i++)
		seq_printf(s, "\t0x%016llx\n", sched->recent_tsg_bitmap[i]);

	nvgpu_mutex_release(&sched->status_lock);

	gk20a_idle(g);

	return 0;
}

static int gk20a_sched_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, gk20a_sched_debugfs_show, inode->i_private);
}

static const struct file_operations gk20a_sched_debugfs_fops = {
	.open		= gk20a_sched_debugfs_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

void gk20a_sched_debugfs_init(struct gk20a *g)
{
	struct nvgpu_os_linux *l = nvgpu_os_linux_from_gk20a(g);

	debugfs_create_file("sched_ctrl", S_IRUGO, l->debugfs,
			g, &gk20a_sched_debugfs_fops);
}
