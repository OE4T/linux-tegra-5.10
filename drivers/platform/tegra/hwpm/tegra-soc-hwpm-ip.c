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
 *
 * tegra-soc-hwpm-ip.c:
 * This file contains functions for SOC HWPM <-> IPC communication.
 */


#include <uapi/linux/tegra-soc-hwpm-uapi.h>
#include "tegra-soc-hwpm.h"

struct platform_device *tegra_soc_hwpm_pdev;

static enum tegra_soc_hwpm_dt_aperture tegra_soc_hwpm_get_apeture(
			struct tegra_soc_hwpm *hwpm, u64 ip_base_address)
{
	enum tegra_soc_hwpm_dt_aperture aperture =
					TEGRA_SOC_HWPM_NUM_DT_APERTURES;

	/* TODO chip speciifc implementation for finding aperture */
	if (ip_base_address == addr_map_vic_base_r()) {
		aperture = TEGRA_SOC_HWPM_VICA0_PERFMON_DT;
	} else if (ip_base_address == addr_map_nvenc_base_r()) {
		aperture = TEGRA_SOC_HWPM_NVENCA0_PERFMON_DT;
	} else if (ip_base_address == addr_map_ofa_base_r()) {
		aperture = TEGRA_SOC_HWPM_OFAA0_PERFMON_DT;
	}

	return aperture;
}

void tegra_soc_hwpm_ip_register(struct tegra_soc_hwpm_ip_ops *hwpm_ip_ops)
{
	struct tegra_soc_hwpm *hwpm = NULL;
	enum tegra_soc_hwpm_dt_aperture dt_aperture;

	tegra_soc_hwpm_dbg("HWPM Registered IP 0x%llx",
					hwpm_ip_ops->ip_base_address);

	if (tegra_soc_hwpm_pdev == NULL) {
		tegra_soc_hwpm_err(
				"IP trying to register before SOC HWPM 0x%llx",
				hwpm_ip_ops->ip_base_address);
	} else {
		hwpm = platform_get_drvdata(tegra_soc_hwpm_pdev);
		dt_aperture = tegra_soc_hwpm_get_apeture(hwpm,
						hwpm_ip_ops->ip_base_address);
		if (dt_aperture != TEGRA_SOC_HWPM_NUM_DT_APERTURES) {
			memcpy(&hwpm->ip_info[dt_aperture], hwpm_ip_ops,
					sizeof(struct tegra_soc_hwpm_ip_ops));
		} else {
			tegra_soc_hwpm_err(
					"SOC HWPM has no support for 0x%llx",
					hwpm_ip_ops->ip_base_address);
		}
	}

}

void tegra_soc_hwpm_ip_unregister(struct tegra_soc_hwpm_ip_ops *hwpm_ip_ops)
{
	struct tegra_soc_hwpm *hwpm = NULL;
	enum tegra_soc_hwpm_dt_aperture dt_aperture;

	if (tegra_soc_hwpm_pdev == NULL) {
		tegra_soc_hwpm_err("IP unregister before SOC HWPM 0x%llx",
						hwpm_ip_ops->ip_base_address);
	} else {
		hwpm = platform_get_drvdata(tegra_soc_hwpm_pdev);
		dt_aperture = tegra_soc_hwpm_get_apeture(hwpm,
						hwpm_ip_ops->ip_base_address);
		if (dt_aperture != TEGRA_SOC_HWPM_NUM_DT_APERTURES) {
			memset(&hwpm->ip_info[dt_aperture], 0,
				sizeof(struct tegra_soc_hwpm_ip_ops));
		}
	}
}
