/*
 * Tegra Graphics Virtualization Host functions for HOST1X
 *
 * Copyright (c) 2020, NVIDIA Corporation. All rights reserved.
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

#include <uapi/linux/nvhost_ioctl.h>
#include <linux/tegra_vhost.h>

int vhost_virt_moduleid_t23x(int moduleid)
{
	switch (moduleid) {
	case NVHOST_MODULE_NVJPG1:
		return TEGRA_VHOST_MODULE_NVJPG1;
	case NVHOST_MODULE_OFA:
		return TEGRA_VHOST_MODULE_OFA;
	default:
		return -1;
	}
}

int vhost_moduleid_virt_to_hw_t23x(int moduleid)
{
	switch (moduleid) {
	case TEGRA_VHOST_MODULE_NVJPG1:
		return NVHOST_MODULE_NVJPG1;
	case TEGRA_VHOST_MODULE_OFA:
		return NVHOST_MODULE_OFA;
	default:
		return -1;

	}
}
