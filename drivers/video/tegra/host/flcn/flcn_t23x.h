/*
 * Copyright (c) 2020-2021, NVIDIA Corporation.  All rights reserved.
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
 * struct of_device_id initialization for T234
 */

//static struct of_device_id tegra_flcn_of_match[] = {

#if defined(CONFIG_TEGRA_GRHOST_VIC)
	{ .compatible = "nvidia,tegra234-vic",
		.data = (struct nvhost_device_data *)&t23x_vic_info },
#endif
#if defined(CONFIG_TEGRA_GRHOST_NVJPG)
	{ .compatible = "nvidia,tegra234-nvjpg",
		.data = (struct nvhost_device_data *)&t23x_nvjpg_info,
		.name = "nvjpg" },
	{ .compatible = "nvidia,tegra234-nvjpg",
		.data = (struct nvhost_device_data *)&t23x_nvjpg1_info,
		.name = "nvjpg1" },
#endif
#if defined(CONFIG_TEGRA_GRHOST_NVENC)
	{ .compatible = "nvidia,tegra234-nvenc",
		.data = (struct nvhost_device_data *)&t23x_msenc_info,
		.name = "nvenc" },
#endif
#if defined(CONFIG_TEGRA_GRHOST_OFA)
	{ .compatible = "nvidia,tegra234-ofa",
		.data = (struct nvhost_device_data *)&t23x_ofa_info },
#endif

//};
