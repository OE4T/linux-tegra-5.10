/*
 * Copyright (c) 2021-2022, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <soc/tegra/fuse.h>
#include <uapi/linux/tegra-soc-hwpm-uapi.h>

#include <tegra_hwpm_log.h>
#include <tegra_hwpm.h>
#include <tegra_hwpm_common.h>
#include <tegra_hwpm_static_analysis.h>

int tegra_soc_hwpm_reserve_resource(struct tegra_soc_hwpm *hwpm, u32 resource)
{
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	struct hwpm_ip *chip_ip = NULL;
	u32 ip_idx = TEGRA_SOC_HWPM_IP_INACTIVE;
	int ret = 0;

	tegra_hwpm_fn(hwpm, " ");

	tegra_hwpm_dbg(hwpm, hwpm_info,
		"User requesting to reserve resource %d", resource);

	/* Translate resource to ip_idx */
	if (!active_chip->is_resource_active(hwpm, resource, &ip_idx)) {
		tegra_hwpm_err(hwpm, "Requested resource %d is unavailable",
			resource);
		/* Remove after uapi update */
		if (resource == TEGRA_SOC_HWPM_RESOURCE_MSS_NVLINK) {
			tegra_hwpm_dbg(hwpm, hwpm_verbose,
				"ignoring resource %d", resource);
			return 0;
		}
		return -EINVAL;
	}

	/* Get IP structure from ip_idx */
	chip_ip = active_chip->chip_ips[ip_idx];

	/* Skip IPs which are already reserved (covers PMA and RTR case) */
	if (chip_ip->reserved) {
		tegra_hwpm_dbg(hwpm, hwpm_info,
			"Chip IP %d already reserved", ip_idx);
		return 0;
	}

	/* Make sure IP override is not enabled */
	if (chip_ip->override_enable) {
		tegra_hwpm_dbg(hwpm, hwpm_info,
			"Chip IP %d not available", ip_idx);
		return 0;
	}

	if (active_chip->reserve_given_resource == NULL) {
		tegra_hwpm_err(hwpm,
			"reserve_given_resource HAL uninitialized");
		return -ENODEV;
	}
	ret = active_chip->reserve_given_resource(hwpm, ip_idx);
	if (ret != 0) {
		tegra_hwpm_err(hwpm, "Failed to reserve resource %d", resource);
		return ret;
	}

	return 0;
}

int tegra_soc_hwpm_release_resources(struct tegra_soc_hwpm *hwpm)
{
	int ret = 0;

	tegra_hwpm_fn(hwpm, " ");

	if (hwpm->active_chip->release_all_resources == NULL) {
		tegra_hwpm_err(hwpm, "release_resources HAL uninitialized");
		return -ENODEV;
	}
	ret = hwpm->active_chip->release_all_resources(hwpm);
	if (ret != 0) {
		tegra_hwpm_err(hwpm, "failed to release resources");
		return ret;
	}

	return 0;
}

int tegra_soc_hwpm_bind_resources(struct tegra_soc_hwpm *hwpm)
{
	int ret = 0;

	tegra_hwpm_fn(hwpm, " ");

	if (hwpm->active_chip->bind_reserved_resources == NULL) {
		tegra_hwpm_err(hwpm,
			"bind_reserved_resources HAL uninitialized");
		return -ENODEV;
	}
	ret = hwpm->active_chip->bind_reserved_resources(hwpm);
	if (ret != 0) {
		tegra_hwpm_err(hwpm, "failed to bind resources");
		return ret;
	}

	return 0;
}
