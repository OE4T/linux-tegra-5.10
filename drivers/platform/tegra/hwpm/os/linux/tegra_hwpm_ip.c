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
#include <tegra_hwpm_io.h>
#include <tegra_hwpm.h>

struct platform_device *tegra_soc_hwpm_pdev;

#define REGISTER_IP	true
#define UNREGISTER_IP	false

void tegra_soc_hwpm_ip_register(struct tegra_soc_hwpm_ip_ops *hwpm_ip_ops)
{
	struct tegra_soc_hwpm *hwpm = NULL;
	int ret = 0;

	if (tegra_soc_hwpm_pdev == NULL) {
		tegra_hwpm_dbg(hwpm, hwpm_info,
			"IP %d trying to register. HWPM device not available",
			hwpm_ip_ops->ip_index);
	} else {
		if (hwpm_ip_ops->ip_dev == NULL) {
			tegra_hwpm_err(hwpm, "IP dev to register is NULL");
			return;
		}
		hwpm = platform_get_drvdata(tegra_soc_hwpm_pdev);

		tegra_hwpm_dbg(hwpm, hwpm_info,
		"Register IP 0x%llx", hwpm_ip_ops->ip_base_address);

		if (hwpm->active_chip->extract_ip_ops == NULL) {
			tegra_hwpm_err(hwpm, "extract_ip_ops uninitialized");
			return;
		}
		ret = hwpm->active_chip->extract_ip_ops(hwpm,
			hwpm_ip_ops, REGISTER_IP);
		if (ret < 0) {
			tegra_hwpm_err(hwpm, "Failed to set IP ops for IP %d",
				hwpm_ip_ops->ip_index);
		}
	}
}

void tegra_soc_hwpm_ip_unregister(struct tegra_soc_hwpm_ip_ops *hwpm_ip_ops)
{
	struct tegra_soc_hwpm *hwpm = NULL;
	int ret = 0;

	if (tegra_soc_hwpm_pdev == NULL) {
		tegra_hwpm_dbg(hwpm, hwpm_info, "HWPM device not available");
	} else {
		if (hwpm_ip_ops->ip_dev == NULL) {
			tegra_hwpm_err(hwpm, "IP dev to unregister is NULL");
			return;
		}
		hwpm = platform_get_drvdata(tegra_soc_hwpm_pdev);

		tegra_hwpm_dbg(hwpm, hwpm_info,
		"Unregister IP 0x%llx", hwpm_ip_ops->ip_base_address);

		if (hwpm->active_chip->extract_ip_ops == NULL) {
			tegra_hwpm_err(hwpm, "extract_ip_ops uninitialized");
			return;
		}
		ret = hwpm->active_chip->extract_ip_ops(hwpm,
			hwpm_ip_ops, UNREGISTER_IP);
		if (ret < 0) {
			tegra_hwpm_err(hwpm, "Failed to reset IP ops for IP %d",
				hwpm_ip_ops->ip_index);
		}
	}
}

int tegra_hwpm_get_floorsweep_info(struct tegra_soc_hwpm *hwpm,
	struct tegra_soc_hwpm_ip_floorsweep_info *fs_info)
{
	int ret = 0;
	u32 i = 0U;

	tegra_hwpm_fn(hwpm, " ");

	if (hwpm->active_chip->get_fs_info == NULL) {
		tegra_hwpm_err(hwpm, "get_fs_info uninitialized");
		return -ENODEV;
	}

	for (i = 0U; i < fs_info->num_queries; i++) {
		ret = hwpm->active_chip->get_fs_info(
			hwpm, (u32)fs_info->ip_fsinfo[i].ip_type,
			&fs_info->ip_fsinfo[i].ip_inst_mask,
			&fs_info->ip_fsinfo[i].status);
		if (ret < 0) {
			/* Print error for debug purpose. */
			tegra_hwpm_err(hwpm, "Failed to get fs_info");
		}

		tegra_hwpm_dbg(hwpm, hwpm_verbose,
			"Query %d: ip_type %d: ip_status: %d inst_mask 0x%llx",
			i, fs_info->ip_fsinfo[i].ip_type,
			fs_info->ip_fsinfo[i].status,
			fs_info->ip_fsinfo[i].ip_inst_mask);
	}
	return ret;
}
