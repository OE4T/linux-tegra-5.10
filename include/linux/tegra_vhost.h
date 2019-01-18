/*
 * Tegra Host Virtualization Interfaces to Server
 *
 * Copyright (c) 2014-2019, NVIDIA Corporation. All rights reserved.
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

#ifndef __TEGRA_VHOST_H
#define __TEGRA_VHOST_H

enum {
	/* Must start at 1, 0 is used for VGPU */
	TEGRA_VHOST_MODULE_HOST = 1,
	TEGRA_VHOST_MODULE_VIC,
	TEGRA_VHOST_MODULE_VI,
	TEGRA_VHOST_MODULE_ISP,
	TEGRA_VHOST_MODULE_MSENC,
	TEGRA_VHOST_MODULE_NVDEC,
	TEGRA_VHOST_MODULE_NVJPG,
	TEGRA_VHOST_MODULE_NVENC1,
	TEGRA_VHOST_MODULE_NVDEC1,
	TEGRA_VHOST_MODULE_VI_THI,
	TEGRA_VHOST_MODULE_ISP_THI,
	TEGRA_VHOST_MODULE_NVCSI,
};

enum {
	TEGRA_VHOST_QUEUE_CMD = 0,
	TEGRA_VHOST_QUEUE_PB,
	TEGRA_VHOST_QUEUE_INTR,
	/* See also TEGRA_VGPU_QUEUE_* in tegra_vgpu.h */
};

enum {
	TEGRA_VHOST_CMD_CONNECT = 0,
	TEGRA_VHOST_CMD_DISCONNECT,
	TEGRA_VHOST_CMD_ABORT,
	TEGRA_VHOST_CMD_HOST1X_REGRDWR,
	TEGRA_VHOST_CMD_SUSPEND,
	TEGRA_VHOST_CMD_RESUME,
};

struct tegra_vhost_connect_params {
	u32 module;
	u64 handle;
};

#define REGRDWR_ARRAY_SIZE (u32)4
struct tegra_vhost_channel_regrdwr_params {
	u32 moduleid;
	u32 count;
	u32 write;
	u32 regs[REGRDWR_ARRAY_SIZE];
};

struct tegra_vhost_cmd_msg {
	u32 cmd;
	int ret;
	u64 handle;
	union {
		struct tegra_vhost_connect_params connect;
		struct tegra_vhost_channel_regrdwr_params regrdwr;
	} params;
};

#define TEGRA_VHOST_QUEUE_SIZES			\
	sizeof(struct tegra_vhost_cmd_msg)

#endif
