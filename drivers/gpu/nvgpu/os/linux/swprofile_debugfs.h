/*
 * Copyright (C) 2020 NVIDIA Corporation.  All rights reserved.
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

#ifndef __NVGPU_PROFILE_DEBUGFS_H__
#define __NVGPU_PROFILE_DEBUGFS_H__

struct dentry;

struct gk20a;
struct nvgpu_swprofiler;

void nvgpu_debugfs_swprofile_init(struct gk20a *g,
				  struct dentry *root,
				  struct nvgpu_swprofiler *p,
				  const char *name);

#endif /* __NVGPU_PROFILE_DEBUGFS_H__ */
