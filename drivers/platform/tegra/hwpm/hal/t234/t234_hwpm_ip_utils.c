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
#include <hal/t234/t234_hwpm_internal.h>
#include <hal/t234/hw/t234_addr_map_soc_hwpm.h>

/*
 * Currently, all IPs do not self register to the hwpm driver
 * This function is used to force set floorsweep mask for IPs which
 * contain perfmon only (eg. SCF)
 */
static int t234_hwpm_update_floorsweep_mask_using_perfmon(
	struct tegra_soc_hwpm *hwpm,
	u32 ip_idx, u32 ip_perfmon_idx, bool available)
{
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	struct hwpm_ip *chip_ip = active_chip->chip_ips[ip_idx];
	hwpm_ip_perfmon *perfmon = NULL;

	tegra_hwpm_fn(hwpm, " ");

	if (chip_ip->override_enable) {
		/* This IP shouldn't be configured, ignore this request */
		return 0;
	}

	perfmon = chip_ip->ip_perfmon[ip_perfmon_idx];
	if (perfmon == NULL) {
		tegra_hwpm_err(hwpm,
			"IP %d perfmon_idx %d not populated as expected",
			ip_idx, ip_perfmon_idx);
		return -EINVAL;
	}

	/* Update floorsweep info */
	if (available) {
		chip_ip->fs_mask |= perfmon->hw_inst_mask;
	} else {
		chip_ip->fs_mask &= ~(perfmon->hw_inst_mask);
	}

	return 0;
}

static int t234_hwpm_update_floorsweep_mask(struct tegra_soc_hwpm *hwpm,
	u32 ip_idx, u32 ip_perfmux_idx, bool available)
{
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	struct hwpm_ip *chip_ip = active_chip->chip_ips[ip_idx];
	hwpm_ip_perfmux *perfmux = NULL;

	tegra_hwpm_fn(hwpm, " ");

	if (chip_ip->override_enable) {
		/* This IP shouldn't be configured, ignore this request */
		return 0;
	}

	perfmux = chip_ip->ip_perfmux[ip_perfmux_idx];
	if (perfmux == NULL) {
		tegra_hwpm_err(hwpm,
			"IP %d perfmux_idx %d not populated as expected",
			ip_idx, ip_perfmux_idx);
		return -EINVAL;
	}

	/* Update floorsweep info */
	if (available) {
		chip_ip->fs_mask |= perfmux->hw_inst_mask;
	} else {
		chip_ip->fs_mask &= ~(perfmux->hw_inst_mask);
	}

	return 0;
}

static int t234_hwpm_update_ip_ops_info(struct tegra_soc_hwpm *hwpm,
	struct tegra_soc_hwpm_ip_ops *hwpm_ip_ops,
	u32 ip_idx, u32 ip_perfmux_idx, bool available)
{
	u32 perfmux_idx, max_num_perfmux = 0U;
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	struct hwpm_ip *chip_ip = active_chip->chip_ips[ip_idx];
	struct tegra_soc_hwpm_ip_ops *ip_ops;
	hwpm_ip_perfmux *given_perfmux = chip_ip->ip_perfmux[ip_perfmux_idx];
	hwpm_ip_perfmux *perfmux = NULL;

	tegra_hwpm_fn(hwpm, " ");

	if (chip_ip->override_enable) {
		/* This IP shouldn't be configured, ignore this request */
		return 0;
	}

	if (given_perfmux == NULL) {
		tegra_hwpm_err(hwpm,
			"IP %d given_perfmux idx %d not populated as expected",
			ip_idx, ip_perfmux_idx);
		return -EINVAL;
	}

	/* Update IP ops info for all perfmux in the instance */
	max_num_perfmux =
		chip_ip->num_instances * chip_ip->num_perfmux_per_inst;
	for (perfmux_idx = 0U; perfmux_idx < max_num_perfmux; perfmux_idx++) {
		perfmux = &chip_ip->perfmux_static_array[perfmux_idx];

		if (perfmux->hw_inst_mask != given_perfmux->hw_inst_mask) {
			continue;
		}

		ip_ops = &perfmux->ip_ops;

		if (available) {
			ip_ops->ip_base_address = hwpm_ip_ops->ip_base_address;
			ip_ops->ip_index = hwpm_ip_ops->ip_index;
			ip_ops->ip_dev = hwpm_ip_ops->ip_dev;
			ip_ops->hwpm_ip_pm = hwpm_ip_ops->hwpm_ip_pm;
			ip_ops->hwpm_ip_reg_op = hwpm_ip_ops->hwpm_ip_reg_op;
		} else {
			/* Do I need a check to see if the ip_ops are set ? */
			ip_ops->ip_base_address = 0ULL;
			ip_ops->ip_index = TEGRA_SOC_HWPM_IP_INACTIVE;
			ip_ops->ip_dev = NULL;
			ip_ops->hwpm_ip_pm = NULL;
			ip_ops->hwpm_ip_reg_op = NULL;
		}
	}

	return 0;
}

static int t234_hwpm_fs_and_ip_ops(struct tegra_soc_hwpm *hwpm,
	struct tegra_soc_hwpm_ip_ops *hwpm_ip_ops,
	u32 ip_idx, u32 perfmux_idx, bool available)
{
	int ret = -EINVAL;

	tegra_hwpm_fn(hwpm, " ");

	ret = t234_hwpm_update_floorsweep_mask(
		hwpm, ip_idx, perfmux_idx, available);
	if (ret != 0) {
		tegra_hwpm_err(hwpm,
			"IP %d perfmux %d: Failed to update FS mask",
			ip_idx, perfmux_idx);
		goto fail;
	}
	ret = t234_hwpm_update_ip_ops_info(hwpm, hwpm_ip_ops,
		ip_idx, perfmux_idx, available);
	if (ret != 0) {
		tegra_hwpm_err(hwpm,
			"IP %d perfmux %d: Failed to update ip_ops",
			ip_idx, perfmux_idx);
		goto fail;
	}
fail:
	return ret;
}

/*
 * This function finds the IP perfmux index corresponding to given base address.
 * Perfmux aperture belongs to IP domain and contains IP instance info
 * wrt base address.
 * Return instance index
 */
static int t234_hwpm_find_ip_perfmux_index(struct tegra_soc_hwpm *hwpm,
	u64 base_addr, u32 ip_index, u32 *ip_perfmux_idx)
{
	struct tegra_soc_hwpm_chip *active_chip = NULL;
	struct hwpm_ip *chip_ip = NULL;
	u32 perfmux_idx;
	u64 addr_offset = 0ULL;
	hwpm_ip_perfmux *perfmux = NULL;

	tegra_hwpm_fn(hwpm, " ");

	if (ip_perfmux_idx == NULL) {
		tegra_hwpm_err(hwpm, "pointer for ip_perfmux_idx is NULL");
		return -EINVAL;
	}

	if (hwpm->active_chip == NULL) {
		tegra_hwpm_err(hwpm, "chip struct not populated");
		return -ENODEV;
	}

	active_chip = hwpm->active_chip;

	if (ip_index == TEGRA_SOC_HWPM_IP_INACTIVE) {
		tegra_hwpm_err(hwpm, "invalid ip_index %d", ip_index);
		return -EINVAL;
	}

	chip_ip = active_chip->chip_ips[ip_index];

	if (chip_ip == NULL) {
		tegra_hwpm_err(hwpm, "IP %d not populated", ip_index);
		return -ENODEV;
	}

	if (chip_ip->override_enable) {
		/* This IP should not be configured for HWPM */
		tegra_hwpm_dbg(hwpm, hwpm_info,
			"IP %d enable override", ip_index);
		return 0; /* Should this be notified to caller or ignored */
	}

	/* Validate phys_addr falls in IP address range */
	if ((base_addr < chip_ip->perfmux_range_start) ||
		(base_addr > chip_ip->perfmux_range_end)) {
		tegra_hwpm_dbg(hwpm, hwpm_info,
			"phys address 0x%llx not in IP %d",
			base_addr, ip_index);
		return -ENODEV;
	}

	/* Find IP instance for given phys_address */
	/*
	 * Since all IP instances are configured to be in consecutive memory,
	 * instance index can be found using instance physical address stride.
	 */
	addr_offset = base_addr - chip_ip->perfmux_range_start;
	perfmux_idx = (u32)(addr_offset / chip_ip->inst_perfmux_stride);

	/* Make sure instance index is valid */
	if (perfmux_idx >= chip_ip->num_perfmux_slots) {
		tegra_hwpm_err(hwpm,
			"IP:%d -> base addr 0x%llx is out of bounds",
			ip_index, base_addr);
		return -EINVAL;
	}

	/* Validate IP instance perfmux start address = given phys addr */
	perfmux = chip_ip->ip_perfmux[perfmux_idx];

	if (perfmux == NULL) {
		/*
		 * This a valid case as not all MSS base addresses are shared
		 * between MSS IPs.
		 */
		tegra_hwpm_dbg(hwpm, hwpm_info,
			"For addr 0x%llx IP %d perfmux_idx %d not populated",
			base_addr, ip_index, perfmux_idx);
		return -ENODEV;
	}

	if (base_addr != perfmux->start_abs_pa) {
		tegra_hwpm_dbg(hwpm, hwpm_info,
			"base addr 0x%llx != perfmux abs addr", base_addr);
		return -EINVAL;
	}

	*ip_perfmux_idx = perfmux_idx;

	return 0;
}

int t234_hwpm_extract_ip_ops(struct tegra_soc_hwpm *hwpm,
	struct tegra_soc_hwpm_ip_ops *hwpm_ip_ops, bool available)
{
	int ret = 0;
	u32 perfmux_idx = 0U;
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
		/* Get IP info */
		ret = t234_hwpm_find_ip_perfmux_index(hwpm,
			hwpm_ip_ops->ip_base_address, ip_idx, &perfmux_idx);
		if (ret != 0) {
			tegra_hwpm_err(hwpm,
				"IP %d base 0x%llx no perfmux match",
				ip_idx, hwpm_ip_ops->ip_base_address);
			goto fail;
		}

		ret = t234_hwpm_fs_and_ip_ops(hwpm, hwpm_ip_ops,
			ip_idx, perfmux_idx, available);
		if (ret != 0) {
			tegra_hwpm_err(hwpm,
				"Failed to %s fs/ops for IP %d perfmux %d",
				available == true ? "set" : "reset",
				ip_idx, perfmux_idx);
			goto fail;
		}
		break;
	case T234_HWPM_IP_MSS_CHANNEL:
	case T234_HWPM_IP_MSS_ISO_NISO_HUBS:
	case T234_HWPM_IP_MSS_MCF:
		/* Check base address in T234_HWPM_IP_MSS_CHANNEL */
		ip_idx = T234_HWPM_IP_MSS_CHANNEL;
		ret = t234_hwpm_find_ip_perfmux_index(hwpm,
			hwpm_ip_ops->ip_base_address, ip_idx, &perfmux_idx);
		if (ret != 0) {
			/*
			 * Return value of ENODEV will indicate that the base
			 * address doesn't belong to this IP.
			 * This case is valid, as not all base addresses are
			 * shared between MSS IPs.
			 * Hence, reset return value to 0.
			 */
			if (ret != -ENODEV) {
				goto fail;
			}
			ret = 0;
		} else {
			ret = t234_hwpm_fs_and_ip_ops(hwpm, hwpm_ip_ops,
				ip_idx, perfmux_idx, available);
			if (ret != 0) {
				tegra_hwpm_err(hwpm,
					"IP %d perfmux %d: fs/ops %s failed",
					ip_idx, perfmux_idx,
					available == true ? "set" : "reset");
				goto fail;
			}
		}

		/* Check base address in T234_HWPM_IP_MSS_ISO_NISO_HUBS */
		ip_idx = T234_HWPM_IP_MSS_ISO_NISO_HUBS;
		ret = t234_hwpm_find_ip_perfmux_index(hwpm,
			hwpm_ip_ops->ip_base_address, ip_idx, &perfmux_idx);
		if (ret != 0) {
			/*
			 * Return value of ENODEV will indicate that the base
			 * address doesn't belong to this IP.
			 * This case is valid, as not all base addresses are
			 * shared between MSS IPs.
			 * Hence, reset return value to 0.
			 */
			if (ret != -ENODEV) {
				goto fail;
			}
			ret = 0;
		} else {
			ret = t234_hwpm_fs_and_ip_ops(hwpm, hwpm_ip_ops,
				ip_idx, perfmux_idx, available);
			if (ret != 0) {
				tegra_hwpm_err(hwpm,
					"IP %d perfmux %d: fs/ops %s failed",
					ip_idx, perfmux_idx,
					available == true ? "set" : "reset");
				goto fail;
			}
		}

		/* Check base address in T234_HWPM_IP_MSS_MCF */
		ip_idx = T234_HWPM_IP_MSS_MCF;
		ret = t234_hwpm_find_ip_perfmux_index(hwpm,
			hwpm_ip_ops->ip_base_address, ip_idx, &perfmux_idx);
		if (ret != 0) {
			/*
			 * Return value of ENODEV will indicate that the base
			 * address doesn't belong to this IP.
			 * This case is valid, as not all base addresses are
			 * shared between MSS IPs.
			 * Hence, reset return value to 0.
			 */
			if (ret != -ENODEV) {
				goto fail;
			}
			ret = 0;
		} else {
			ret = t234_hwpm_fs_and_ip_ops(hwpm, hwpm_ip_ops,
				ip_idx, perfmux_idx, available);
			if (ret != 0) {
				tegra_hwpm_err(hwpm,
					"IP %d perfmux %d: fs/ops %s failed",
					ip_idx, perfmux_idx,
					available == true ? "set" : "reset");
				goto fail;
			}
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

/*
 * Find IP perfmux index and set corresponding floorsweep info.
 */
int t234_hwpm_set_fs_info(struct tegra_soc_hwpm *hwpm, u64 base_address,
	u32 ip_idx, bool available)
{
	int ret = 0;
	u32 perfmux_idx = 0U;

	tegra_hwpm_fn(hwpm, " ");

	ret = t234_hwpm_find_ip_perfmux_index(hwpm,
		base_address, ip_idx, &perfmux_idx);
	if (ret != 0) {
		tegra_hwpm_err(hwpm, "IP %d base 0x%llx no perfmux match",
			ip_idx, base_address);
		goto fail;
	}

	ret = t234_hwpm_update_floorsweep_mask(
		hwpm, ip_idx, perfmux_idx, available);
	if (ret != 0) {
		tegra_hwpm_err(hwpm,
			"IP %d perfmux %d base 0x%llx: FS mask update failed",
			ip_idx, perfmux_idx, base_address);
		goto fail;
	}
fail:
	return ret;
}

/*
 * Some IPs don't register with HWPM driver at the moment. Force set available
 * instances of such IPs.
 */
int t234_hwpm_init_fs_info(struct tegra_soc_hwpm *hwpm)
{
	u32 i;
	int ret = 0;
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	struct hwpm_ip *chip_ip = NULL;

	tegra_hwpm_fn(hwpm, " ");

	if (tegra_platform_is_vsp()) {
		/* Static IP instances as per VSP netlist */
		/* MSS CHANNEL: vsp has single instance available */
		ret = t234_hwpm_set_fs_info(hwpm, addr_map_mc0_base_r(),
			T234_HWPM_IP_MSS_CHANNEL, true);
		if (ret != 0) {
			goto fail;
		}

		/* MSS GPU HUB */
		ret = t234_hwpm_set_fs_info(hwpm,
			addr_map_mss_nvlink_1_base_r(),
			T234_HWPM_IP_MSS_GPU_HUB, true);
		if (ret != 0) {
			goto fail;
		}
	}
	if (tegra_platform_is_silicon()) {
		/* Static IP instances corresponding to silicon */
		/* VI */
		/*ret = t234_hwpm_set_fs_info(hwpm, addr_map_vi_thi_base_r(),
			T234_HWPM_IP_VI, true);
		if (ret != 0) {
			goto fail;
		}
		ret = t234_hwpm_set_fs_info(hwpm, addr_map_vi2_thi_base_r(),
			T234_HWPM_IP_VI, true);
		if (ret != 0) {
			goto fail;
		}*/

		/* ISP */
		ret = t234_hwpm_set_fs_info(hwpm, addr_map_isp_thi_base_r(),
			T234_HWPM_IP_ISP, true);
		if (ret != 0) {
			goto fail;
		}

		/* PVA */
		ret = t234_hwpm_set_fs_info(hwpm, addr_map_pva0_pm_base_r(),
			T234_HWPM_IP_PVA, true);
		if (ret != 0) {
			goto fail;
		}

		/* NVDLA */
		ret = t234_hwpm_set_fs_info(hwpm,
			addr_map_nvdla0_base_r(),
			T234_HWPM_IP_NVDLA, true);
		if (ret != 0) {
			goto fail;
		}
		ret = t234_hwpm_set_fs_info(hwpm,
			addr_map_nvdla1_base_r(),
			T234_HWPM_IP_NVDLA, true);
		if (ret != 0) {
			goto fail;
		}

		/* MGBE */
		/*ret = t234_hwpm_set_fs_info(hwpm,
			addr_map_mgbe0_mac_rm_base_r(),
			T234_HWPM_IP_MGBE, true);
		if (ret != 0) {
			goto fail;
		}*/

		/* SCF */
		ret = t234_hwpm_update_floorsweep_mask_using_perfmon(hwpm,
			T234_HWPM_IP_SCF, 0U, true);
		if (ret != 0) {
			tegra_hwpm_err(hwpm,
				"T234_HWPM_IP_SCF: FS mask update failed");
			goto fail;
		}

		/* NVDEC */
		ret = t234_hwpm_set_fs_info(hwpm, addr_map_nvdec_base_r(),
			T234_HWPM_IP_NVDEC, true);
		if (ret != 0) {
			goto fail;
		}

		/* PCIE */
		/*ret = t234_hwpm_set_fs_info(hwpm,
			addr_map_pcie_c1_ctl_base_r(),
			T234_HWPM_IP_PCIE, true);
		if (ret != 0) {
			goto fail;
		}
		ret = t234_hwpm_set_fs_info(hwpm,
			addr_map_pcie_c4_ctl_base_r(),
			T234_HWPM_IP_PCIE, true);
		if (ret != 0) {
			goto fail;
		}
		ret = t234_hwpm_set_fs_info(hwpm,
			addr_map_pcie_c5_ctl_base_r(),
			T234_HWPM_IP_PCIE, true);
		if (ret != 0) {
			goto fail;
		}*/

		/* DISPLAY */
		/*ret = t234_hwpm_set_fs_info(hwpm, addr_map_disp_base_r(),
			T234_HWPM_IP_DISPLAY, true);
		if (ret != 0) {
			goto fail;
		}*/

		/* MSS CHANNEL */
		ret = t234_hwpm_set_fs_info(hwpm, addr_map_mc0_base_r(),
			T234_HWPM_IP_MSS_CHANNEL, true);
		if (ret != 0) {
			goto fail;
		}
		ret = t234_hwpm_set_fs_info(hwpm, addr_map_mc4_base_r(),
			T234_HWPM_IP_MSS_CHANNEL, true);
		if (ret != 0) {
			goto fail;
		}
		ret = t234_hwpm_set_fs_info(hwpm, addr_map_mc8_base_r(),
			T234_HWPM_IP_MSS_CHANNEL, true);
		if (ret != 0) {
			goto fail;
		}
		ret = t234_hwpm_set_fs_info(hwpm, addr_map_mc12_base_r(),
			T234_HWPM_IP_MSS_CHANNEL, true);
		if (ret != 0) {
			goto fail;
		}

		/* MSS ISO NISO HUBS */
		ret = t234_hwpm_set_fs_info(hwpm, addr_map_mc0_base_r(),
			TEGRA_SOC_HWPM_IP_MSS_ISO_NISO_HUBS, true);
		if (ret != 0) {
			goto fail;
		}

		/* MSS MCF */
		ret = t234_hwpm_set_fs_info(hwpm, addr_map_mc0_base_r(),
			TEGRA_SOC_HWPM_IP_MSS_MCF, true);
		if (ret != 0) {
			goto fail;
		}

		/* MSS GPU HUB */
		ret = t234_hwpm_set_fs_info(hwpm,
			addr_map_mss_nvlink_1_base_r(),
			T234_HWPM_IP_MSS_GPU_HUB, true);
		if (ret != 0) {
			goto fail;
		}
	}

	tegra_hwpm_dbg(hwpm, hwpm_verbose, "IP floorsweep info:");
	for (i = 0U; i < T234_HWPM_IP_MAX; i++) {
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
					mcc_fs_mask |= (0xFU << i);
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
