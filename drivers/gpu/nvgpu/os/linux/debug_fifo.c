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
 */

#include "debug_fifo.h"
#include "os_linux.h"

#include <linux/debugfs.h>
#include <linux/seq_file.h>

#include <nvgpu/sort.h>
#include <nvgpu/timers.h>
#include <nvgpu/channel.h>
#include <nvgpu/gr/ctx.h>
#include <nvgpu/engines.h>
#include <nvgpu/runlist.h>
#include <nvgpu/swprofile.h>

#include <nvgpu/fifo/swprofile.h>

void __gk20a_fifo_profile_free(struct nvgpu_ref *ref);

static void *gk20a_fifo_sched_debugfs_seq_start(
		struct seq_file *s, loff_t *pos)
{
	struct gk20a *g = s->private;
	struct nvgpu_fifo *f = &g->fifo;

	if (*pos >= f->num_channels)
		return NULL;

	return &f->channel[*pos];
}

static void *gk20a_fifo_sched_debugfs_seq_next(
		struct seq_file *s, void *v, loff_t *pos)
{
	struct gk20a *g = s->private;
	struct nvgpu_fifo *f = &g->fifo;

	++(*pos);
	if (*pos >= f->num_channels)
		return NULL;

	return &f->channel[*pos];
}

static void gk20a_fifo_sched_debugfs_seq_stop(
		struct seq_file *s, void *v)
{
}

static int gk20a_fifo_sched_debugfs_seq_show(
		struct seq_file *s, void *v)
{
	struct gk20a *g = s->private;
	struct nvgpu_fifo *f = &g->fifo;
	struct nvgpu_channel *ch = v;
	struct nvgpu_tsg *tsg = NULL;

	struct nvgpu_engine_info *engine_info;
	struct nvgpu_runlist_info *runlist;
	u32 runlist_id;
	int ret = SEQ_SKIP;
	u32 engine_id;

	engine_id = nvgpu_engine_get_gr_id(g);
	engine_info = (f->engine_info + engine_id);
	runlist_id = engine_info->runlist_id;
	runlist = f->runlist_info[runlist_id];

	if (ch == f->channel) {
		seq_puts(s, "chid     tsgid    pid      timeslice  timeout  interleave graphics_preempt compute_preempt\n");
		seq_puts(s, "                            (usecs)   (msecs)\n");
		ret = 0;
	}

	if (!test_bit(ch->chid, runlist->active_channels))
		return ret;

	if (nvgpu_channel_get(ch)) {
		tsg = nvgpu_tsg_from_ch(ch);

		if (tsg)
			seq_printf(s, "%-8d %-8d %-8d %-9d %-8d %-10d %-8d %-8d\n",
				ch->chid,
				ch->tsgid,
				ch->tgid,
				tsg->timeslice_us,
				ch->ctxsw_timeout_max_ms,
				tsg->interleave_level,
				nvgpu_gr_ctx_get_graphics_preemption_mode(tsg->gr_ctx),
				nvgpu_gr_ctx_get_compute_preemption_mode(tsg->gr_ctx));
		nvgpu_channel_put(ch);
	}
	return 0;
}

static const struct seq_operations gk20a_fifo_sched_debugfs_seq_ops = {
	.start = gk20a_fifo_sched_debugfs_seq_start,
	.next = gk20a_fifo_sched_debugfs_seq_next,
	.stop = gk20a_fifo_sched_debugfs_seq_stop,
	.show = gk20a_fifo_sched_debugfs_seq_show
};

static int gk20a_fifo_sched_debugfs_open(struct inode *inode,
	struct file *file)
{
	struct gk20a *g = inode->i_private;
	int err;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	err = seq_open(file, &gk20a_fifo_sched_debugfs_seq_ops);
	if (err)
		return err;

	nvgpu_log(g, gpu_dbg_info, "i_private=%p", inode->i_private);

	((struct seq_file *)file->private_data)->private = inode->i_private;
	return 0;
};

/*
 * The file operations structure contains our open function along with
 * set of the canned seq_ ops.
 */
static const struct file_operations gk20a_fifo_sched_debugfs_fops = {
	.owner = THIS_MODULE,
	.open = gk20a_fifo_sched_debugfs_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release
};

static int gk20a_fifo_profile_enable(void *data, u64 val)
{
	struct gk20a *g = (struct gk20a *) data;
	struct nvgpu_fifo *f = &g->fifo;

	if (val == 0) {
		nvgpu_swprofile_close(&f->kickoff_profiler);
		return 0;
	} else {
		return nvgpu_swprofile_open(g, &f->kickoff_profiler);
	}
}

DEFINE_SIMPLE_ATTRIBUTE(
	gk20a_fifo_profile_enable_debugfs_fops,
	NULL,
	gk20a_fifo_profile_enable,
	"%llu\n"
);

static void gk20a_fifo_write_to_seqfile_no_nl(void *ctx, const char *str)
{
	seq_printf((struct seq_file *)ctx, str);
}

static int gk20a_fifo_profile_stats(struct seq_file *s, void *unused)
{
	struct gk20a *g = s->private;
	struct nvgpu_debug_context o = {
		.fn = gk20a_fifo_write_to_seqfile_no_nl,
		.ctx = s,
	};

	nvgpu_swprofile_print_ranges(g, &g->fifo.kickoff_profiler, &o);

	return 0;
}

static int gk20a_fifo_profile_stats_open(struct inode *inode, struct file *file)
{
	return single_open(file, gk20a_fifo_profile_stats, inode->i_private);
}

static const struct file_operations gk20a_fifo_profile_stats_debugfs_fops = {
	.open		= gk20a_fifo_profile_stats_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

void gk20a_fifo_debugfs_init(struct gk20a *g)
{
	struct nvgpu_os_linux *l = nvgpu_os_linux_from_gk20a(g);
	struct dentry *gpu_root = l->debugfs;
	struct dentry *fifo_root;
	struct dentry *profile_root;

	fifo_root = debugfs_create_dir("fifo", gpu_root);
	if (IS_ERR_OR_NULL(fifo_root))
		return;

	nvgpu_log(g, gpu_dbg_info, "g=%p", g);

	debugfs_create_file("sched", 0600, fifo_root, g,
		&gk20a_fifo_sched_debugfs_fops);

	profile_root = debugfs_create_dir("profile", fifo_root);
	if (IS_ERR_OR_NULL(profile_root))
		return;

	debugfs_create_file("enable", 0600, profile_root, g,
		&gk20a_fifo_profile_enable_debugfs_fops);

	debugfs_create_file("stats", 0600, profile_root, g,
		&gk20a_fifo_profile_stats_debugfs_fops);

}
