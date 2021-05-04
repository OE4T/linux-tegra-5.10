/*
 * Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
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
#ifndef LINUX_NVGPU_NEXT_IOCTL_PROF_H
#define LINUX_NVGPU_NEXT_IOCTL_PROF_H

struct nvgpu_profiler_object;

int nvgpu_next_prof_fops_ioctl(struct nvgpu_profiler_object *prof,
	unsigned int cmd, void *buf);

#endif /* LINUX_NVGPU_NEXT_IOCTL_PROF_H */
