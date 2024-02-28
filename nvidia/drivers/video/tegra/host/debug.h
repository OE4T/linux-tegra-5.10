/*
 * drivers/video/tegra/host/debug.h
 *
 * Tegra Graphics Host Debug
 *
 * Copyright (C) 2011-2018, NVIDIA Corporation. All rights reserved.
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
#ifndef __NVHOST_DEBUG_H
#define __NVHOST_DEBUG_H

#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include "nvhost_syncpt.h"

struct output {
	void (*fn)(void *ctx, const char* str, size_t len);
	void *ctx;
	char buf[256];
};

static inline void write_to_seqfile(void *ctx, const char* str, size_t len)
{
	seq_write((struct seq_file *)ctx, str, len);
}

static inline void write_to_printk(void *ctx, const char* str, size_t len)
{
	printk("%s", str);
}

void nvhost_debug_output(struct output *o, const char* fmt, ...);
#ifdef CONFIG_DEBUG_FS
void nvhost_debug_dump_locked(struct nvhost_master *master, int locked_id);
#else
static inline void nvhost_debug_dump_locked(struct nvhost_master *master, int locked_id)
{
}
#endif

void nvhost_syncpt_debug(struct nvhost_syncpt *sp);

extern u32 nvhost_debug_force_timeout_dump;
extern unsigned int nvhost_debug_trace_cmdbuf;
extern unsigned int nvhost_debug_trace_actmon;

#endif /*__NVHOST_DEBUG_H */
