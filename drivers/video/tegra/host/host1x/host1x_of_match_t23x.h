/*
 * Copyright (c) 2021, NVIDIA Corporation.  All rights reserved.
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
 *
 * struct of_device_id initialization for T234 and T239
 */

//static struct of_device_id tegra_host1x_of_match[] = {

	{ .name = "host1x",
		.compatible = "nvidia,tegra234-host1x",
		.data = (struct nvhost_device_data *)&t23x_host1x_info },
	{ .name = "host1x",
		.compatible = "nvidia,tegra234-host1x-hv",
		.data = (struct nvhost_device_data *)&t23x_host1x_hv_info },
	{ .name = "host1x",
		.compatible = "nvidia,tegra239-host1x",
		.data = (struct nvhost_device_data *)&t239_host1x_info },

//};
