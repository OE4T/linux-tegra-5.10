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
#include <tegra_hwpm_common.h>
#include <tegra_hwpm.h>
#include <tegra_hwpm_static_analysis.h>
#include <hal/t234/t234_hwpm_internal.h>
#include <hal/t234/hw/t234_addr_map_soc_hwpm.h>

int t234_hwpm_extract_ip_ops(struct tegra_soc_hwpm *hwpm,
	struct tegra_soc_hwpm_ip_ops *hwpm_ip_ops, bool available)
{
	int ret = 0;
	u32 ip_idx = 0U;

	tegra_hwpm_fn(hwpm, " ");

	/* Convert tegra_soc_hwpm_ip to internal enum */
	if (!(t234_hwpm_is_ip_active(hwpm,
			hwpm_ip_ops->ip_index, &ip_idx))) {
		tegra_hwpm_err(hwpm,
			"SOC hwpm IP %d (base 0x%llx) is unconfigured",
			hwpm_ip_ops->ip_index, hwpm_ip_ops->ip_base_address);
		goto fail;
	}

	switch (ip_idx) {
	case T234_HWPM_IP_VI:
	case T234_HWPM_IP_ISP:
	case T234_HWPM_IP_VIC:
	case T234_HWPM_IP_OFA:
	case T234_HWPM_IP_PVA:
	case T234_HWPM_IP_NVDLA:
	case T234_HWPM_IP_MGBE:
	case T234_HWPM_IP_SCF:
	case T234_HWPM_IP_NVDEC:
	case T234_HWPM_IP_NVENC:
	case T234_HWPM_IP_PCIE:
	case T234_HWPM_IP_DISPLAY:
	case T234_HWPM_IP_MSS_GPU_HUB:
		ret = tegra_hwpm_set_fs_info_ip_ops(hwpm, hwpm_ip_ops,
			hwpm_ip_ops->ip_base_address, ip_idx, available);
		if (ret != 0) {
			tegra_hwpm_err(hwpm,
				"Failed to %s fs/ops for IP %d (base 0x%llx)",
				available == true ? "set" : "reset",
				ip_idx, hwpm_ip_ops->ip_base_address);
			goto fail;
		}
		break;
	case T234_HWPM_IP_MSS_CHANNEL:
	case T234_HWPM_IP_MSS_ISO_NISO_HUBS:
	case T234_HWPM_IP_MSS_MCF:
		/* MSS channel, ISO NISO hubs and MCF share MC channels */

		/* Check base address in T234_HWPM_IP_MSS_CHANNEL */
		ip_idx = T234_HWPM_IP_MSS_CHANNEL;
		ret = tegra_hwpm_set_fs_info_ip_ops(hwpm, hwpm_ip_ops,
			hwpm_ip_ops->ip_base_address, ip_idx, available);
		if (ret != 0) {
			/*
			 * Return value of ENODEV will indicate that the base
			 * address doesn't belong to this IP.
			 * This case is valid, as not all base addresses are
			 * shared between MSS IPs.
			 * In this case, reset return value to 0.
			 */
			if (ret != -ENODEV) {
				tegra_hwpm_err(hwpm,
					"IP %d base 0x%llx:Failed to %s fs/ops",
					ip_idx, hwpm_ip_ops->ip_base_address,
					available == true ? "set" : "reset");
				goto fail;
			}
			ret = 0;
		}

		/* Check base address in T234_HWPM_IP_MSS_ISO_NISO_HUBS */
		ip_idx = T234_HWPM_IP_MSS_ISO_NISO_HUBS;
		ret = tegra_hwpm_set_fs_info_ip_ops(hwpm, hwpm_ip_ops,
			hwpm_ip_ops->ip_base_address, ip_idx, available);
		if (ret != 0) {
			/*
			 * Return value of ENODEV will indicate that the base
			 * address doesn't belong to this IP.
			 * This case is valid, as not all base addresses are
			 * shared between MSS IPs.
			 * In this case, reset return value to 0.
			 */
			if (ret != -ENODEV) {
				tegra_hwpm_err(hwpm,
					"IP %d base 0x%llx:Failed to %s fs/ops",
					ip_idx, hwpm_ip_ops->ip_base_address,
					available == true ? "set" : "reset");
				goto fail;
			}
			ret = 0;
		}

		/* Check base address in T234_HWPM_IP_MSS_MCF */
		ip_idx = T234_HWPM_IP_MSS_MCF;
		ret = tegra_hwpm_set_fs_info_ip_ops(hwpm, hwpm_ip_ops,
			hwpm_ip_ops->ip_base_address, ip_idx, available);
		if (ret != 0) {
			/*
			 * Return value of ENODEV will indicate that the base
			 * address doesn't belong to this IP.
			 * This case is valid, as not all base addresses are
			 * shared between MSS IPs.
			 * In this case, reset return value to 0.
			 */
			if (ret != -ENODEV) {
				tegra_hwpm_err(hwpm,
					"IP %d base 0x%llx:Failed to %s fs/ops",
					ip_idx, hwpm_ip_ops->ip_base_address,
					available == true ? "set" : "reset");
				goto fail;
			}
			ret = 0;
		}
		break;
	case T234_HWPM_IP_PMA:
	case T234_HWPM_IP_RTR:
	default:
		tegra_hwpm_err(hwpm, "Invalid IP %d for ip_ops", ip_idx);
		break;
	}

fail:
	return ret;
}

int t234_hwpm_force_enable_ips(struct tegra_soc_hwpm *hwpm)
{
	u32 i = 0U;
	int ret = 0;
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	struct hwpm_ip *chip_ip = NULL;

	tegra_hwpm_fn(hwpm, " ");

	if (tegra_platform_is_vsp()) {
		/* Static IP instances as per VSP netlist */
		/* MSS CHANNEL: vsp has single instance available */
		ret = tegra_hwpm_set_fs_info_ip_ops(hwpm, NULL,
			addr_map_mc0_base_r(),
			T234_HWPM_IP_MSS_CHANNEL, true);
		if (ret != 0) {
			goto fail;
		}

		/* MSS GPU HUB */
		ret = tegra_hwpm_set_fs_info_ip_ops(hwpm, NULL,
			addr_map_mss_nvlink_1_base_r(),
			T234_HWPM_IP_MSS_GPU_HUB, true);
		if (ret != 0) {
			goto fail;
		}
	}
	if (tegra_platform_is_silicon()) {
		/* Static IP instances corresponding to silicon */
		/* VI */
		/*ret = tegra_hwpm_set_fs_info_ip_ops(hwpm, NULL,
			addr_map_vi_thi_base_r(),
			T234_HWPM_IP_VI, true);
		if (ret != 0) {
			goto fail;
		}
		ret = tegra_hwpm_set_fs_info_ip_ops(hwpm, NULL,
			addr_map_vi2_thi_base_r(),
			T234_HWPM_IP_VI, true);
		if (ret != 0) {
			goto fail;
		}*/

		/* ISP */
		ret = tegra_hwpm_set_fs_info_ip_ops(hwpm, NULL,
			addr_map_isp_thi_base_r(),
			T234_HWPM_IP_ISP, true);
		if (ret != 0) {
			goto fail;
		}

		/* NVDLA */
		ret = tegra_hwpm_set_fs_info_ip_ops(hwpm, NULL,
			addr_map_nvdla0_base_r(),
			T234_HWPM_IP_NVDLA, true);
		if (ret != 0) {
			goto fail;
		}
		ret = tegra_hwpm_set_fs_info_ip_ops(hwpm, NULL,
			addr_map_nvdla1_base_r(),
			T234_HWPM_IP_NVDLA, true);
		if (ret != 0) {
			goto fail;
		}

		/* MGBE */
		/*ret = tegra_hwpm_set_fs_info_ip_ops(hwpm, NULL,
			addr_map_mgbe0_mac_rm_base_r(),
			T234_HWPM_IP_MGBE, true);
		if (ret != 0) {
			goto fail;
		}*/

		/* SCF */
		ret = tegra_hwpm_set_fs_info_ip_ops(hwpm, NULL,
			addr_map_rpg_pm_scf_base_r(),
			T234_HWPM_IP_SCF, true);
		if (ret != 0) {
			goto fail;
		}

		/* NVDEC */
		ret = tegra_hwpm_set_fs_info_ip_ops(hwpm, NULL,
			addr_map_nvdec_base_r(),
			T234_HWPM_IP_NVDEC, true);
		if (ret != 0) {
			goto fail;
		}

		/* PCIE */
		/*ret = tegra_hwpm_set_fs_info_ip_ops(hwpm, NULL,
			addr_map_pcie_c1_ctl_base_r(),
			T234_HWPM_IP_PCIE, true);
		if (ret != 0) {
			goto fail;
		}
		ret = tegra_hwpm_set_fs_info_ip_ops(hwpm, NULL,
			addr_map_pcie_c4_ctl_base_r(),
			T234_HWPM_IP_PCIE, true);
		if (ret != 0) {
			goto fail;
		}
		ret = tegra_hwpm_set_fs_info_ip_ops(hwpm, NULL,
			addr_map_pcie_c5_ctl_base_r(),
			T234_HWPM_IP_PCIE, true);
		if (ret != 0) {
			goto fail;
		}*/

		/* DISPLAY */
		/*ret = tegra_hwpm_set_fs_info_ip_ops(hwpm, NULL,
			addr_map_disp_base_r(),
			T234_HWPM_IP_DISPLAY, true);
		if (ret != 0) {
			goto fail;
		}*/

		/* MSS CHANNEL */
		ret = tegra_hwpm_set_fs_info_ip_ops(hwpm, NULL,
			addr_map_mc0_base_r(),
			T234_HWPM_IP_MSS_CHANNEL, true);
		if (ret != 0) {
			goto fail;
		}
		ret = tegra_hwpm_set_fs_info_ip_ops(hwpm, NULL,
			addr_map_mc4_base_r(),
			T234_HWPM_IP_MSS_CHANNEL, true);
		if (ret != 0) {
			goto fail;
		}
		ret = tegra_hwpm_set_fs_info_ip_ops(hwpm, NULL,
			addr_map_mc8_base_r(),
			T234_HWPM_IP_MSS_CHANNEL, true);
		if (ret != 0) {
			goto fail;
		}
		ret = tegra_hwpm_set_fs_info_ip_ops(hwpm, NULL,
			addr_map_mc12_base_r(),
			T234_HWPM_IP_MSS_CHANNEL, true);
		if (ret != 0) {
			goto fail;
		}

		/* MSS ISO NISO HUBS */
		ret = tegra_hwpm_set_fs_info_ip_ops(hwpm, NULL,
			addr_map_mc0_base_r(),
			T234_HWPM_IP_MSS_ISO_NISO_HUBS, true);
		if (ret != 0) {
			goto fail;
		}

		/* MSS MCF */
		ret = tegra_hwpm_set_fs_info_ip_ops(hwpm, NULL,
			addr_map_mc0_base_r(),
			T234_HWPM_IP_MSS_MCF, true);
		if (ret != 0) {
			goto fail;
		}

		/* MSS GPU HUB */
		ret = tegra_hwpm_set_fs_info_ip_ops(hwpm, NULL,
			addr_map_mss_nvlink_1_base_r(),
			T234_HWPM_IP_MSS_GPU_HUB, true);
		if (ret != 0) {
			goto fail;
		}
	}

	tegra_hwpm_dbg(hwpm, hwpm_verbose, "IP floorsweep info:");
	for (i = 0U; i < active_chip->get_ip_max_idx(hwpm); i++) {
		chip_ip = active_chip->chip_ips[i];
		tegra_hwpm_dbg(hwpm, hwpm_verbose, "IP:%d fs_mask:0x%x",
			i, chip_ip->fs_mask);
	}

fail:
	return ret;
}

int t234_hwpm_get_fs_info(struct tegra_soc_hwpm *hwpm,
	u32 ip_index, u64 *fs_mask, u8 *ip_status)
{
	u32 ip_idx = 0U;
	u32 i = 0U;
	u32 mcc_fs_mask = 0U;
	struct tegra_soc_hwpm_chip *active_chip = NULL;
	struct hwpm_ip *chip_ip = NULL;

	tegra_hwpm_fn(hwpm, " ");

	/* Convert tegra_soc_hwpm_ip to internal enum */
	if (!(t234_hwpm_is_ip_active(hwpm, ip_index, &ip_idx))) {
		tegra_hwpm_dbg(hwpm, hwpm_info,
			"SOC hwpm IP %d is not configured", ip_index);

		*ip_status = TEGRA_SOC_HWPM_IP_STATUS_INVALID;
		*fs_mask = 0ULL;
		/* Remove after uapi update */
		if (ip_index == TEGRA_SOC_HWPM_IP_MSS_NVLINK) {
			tegra_hwpm_dbg(hwpm, hwpm_verbose,
				"For hwpm IP %d setting status as valid",
				ip_index);
			*ip_status = TEGRA_SOC_HWPM_IP_STATUS_VALID;
		}
	} else {
		active_chip = hwpm->active_chip;
		chip_ip = active_chip->chip_ips[ip_idx];
		/* TODO: Update after fS IOCTL discussion */
		if (ip_idx == T234_HWPM_IP_MSS_CHANNEL) {
			for (i = 0U; i < 4U; i++) {
				if (((0x1U << i) & chip_ip->fs_mask) != 0U) {
					mcc_fs_mask |= (0xFU << (i*4U));
				}
			}
			*fs_mask = mcc_fs_mask;
		} else {
			*fs_mask = chip_ip->fs_mask;
		}
		*ip_status = TEGRA_SOC_HWPM_IP_STATUS_VALID;
	}

	return 0;
}
