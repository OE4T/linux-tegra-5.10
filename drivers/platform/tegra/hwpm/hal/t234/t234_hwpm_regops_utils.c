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

#include <uapi/linux/tegra-soc-hwpm-uapi.h>

#include <tegra_hwpm_static_analysis.h>
#include <tegra_hwpm_log.h>
#include <tegra_hwpm_io.h>
#include <tegra_hwpm.h>
#include <hal/t234/t234_hwpm_internal.h>

static bool t234_hwpm_is_addr_in_ip_perfmon(struct tegra_soc_hwpm *hwpm,
	u64 phys_addr, u32 ip_idx, struct hwpm_ip_aperture **aperture)
{
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	struct hwpm_ip *chip_ip = active_chip->chip_ips[ip_idx];
	hwpm_ip_perfmon *perfmon = NULL;
	u64 address_offset = 0ULL;
	u32 perfmon_idx = 0U;

	tegra_hwpm_fn(hwpm, " ");

	/* Check if phys addr doesn't belong to IP perfmon range */
	if ((phys_addr < chip_ip->perfmon_range_start) ||
		(phys_addr > chip_ip->perfmon_range_end)) {
		return false;
	}

	/* Find perfmon idx corresponding phys addr */
	address_offset = tegra_hwpm_safe_sub_u64(
		phys_addr, chip_ip->perfmon_range_start);
	perfmon_idx = tegra_hwpm_safe_cast_u64_to_u32(
		address_offset / chip_ip->inst_perfmon_stride);

	perfmon = chip_ip->ip_perfmon[perfmon_idx];

	/* Check if perfmon is populated */
	if (perfmon == NULL) {
		/*
		 * NOTE: MSS channel and ISO NISO hub IPs have same perfmon
		 * range but differ in populated perfmons. In this case,
		 * NULL perfmon may not be a failure indication.
		 * Log this result for debug and return false.
		 */
		tegra_hwpm_dbg(hwpm, hwpm_info,
			"Accessing IP %d unpopulated perfmon_idx %d",
			ip_idx, perfmon_idx);
		return false;
	}

	/* Make sure that perfmon belongs to available IP instances */
	if ((perfmon->hw_inst_mask & chip_ip->fs_mask) == 0U) {
		/*
		 * NOTE: User accessing this address indicates that
		 * case 1: perfmon (corresponding IP HW instance) is available
		 * case 2: computed allowlist is incorrect
		 * For case 1,
		 * Perfmon information is added statically.
		 * It is possible that perfmon (or IP) HW instance is not
		 * available in a configuration.
		 * This is a valid case, return false to indicate.
		 */
		tegra_hwpm_err(hwpm,
			"accessed IP %d perfmon %d marked unavailable",
			ip_idx, perfmon_idx);
		return false;
	}

	/* Make sure phys addr belongs to the perfmon */
	if ((phys_addr >= perfmon->start_abs_pa) &&
		(phys_addr <= perfmon->end_abs_pa)) {
		if (t234_hwpm_check_alist(hwpm, perfmon, phys_addr)) {
			*aperture = perfmon;
			return true;
		}
		tegra_hwpm_dbg(hwpm, hwpm_verbose,
			"phys_addr 0x%llx not in IP %d perfmon_idx %d alist",
			phys_addr, ip_idx, perfmon_idx);
		return false;
	}

	tegra_hwpm_err(hwpm, "Execution shouldn't reach here");
	return false;
}

static bool t234_hwpm_is_addr_in_ip_perfmux(struct tegra_soc_hwpm *hwpm,
	u64 phys_addr, u32 ip_idx, struct hwpm_ip_aperture **aperture)
{
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	struct hwpm_ip *chip_ip = active_chip->chip_ips[ip_idx];
	hwpm_ip_perfmux *perfmux = NULL;
	u64 address_offset = 0ULL;
	u32 perfmux_idx = 0U;

	tegra_hwpm_fn(hwpm, " ");

	/* Check if phys addr doesn't belong to IP perfmux range */
	if ((phys_addr < chip_ip->perfmux_range_start) ||
		(phys_addr > chip_ip->perfmux_range_end)) {
		return false;
	}

	/* Find perfmux idx corresponding phys addr */
	address_offset = tegra_hwpm_safe_sub_u64(
		phys_addr, chip_ip->perfmux_range_start);
	perfmux_idx = tegra_hwpm_safe_cast_u64_to_u32(
		address_offset / chip_ip->inst_perfmux_stride);

	perfmux = chip_ip->ip_perfmux[perfmux_idx];

	/* Check if perfmux is populated */
	if (perfmux == NULL) {
		/*
		 * NOTE: MSS channel and ISO NISO hub IPs have same perfmux
		 * range but differ in populated perfmuxes. In this case,
		 * NULL perfmux may not be a failure indication.
		 * Log this result for debug and return false.
		 */
		tegra_hwpm_dbg(hwpm, hwpm_info,
			"Accessing IP %d unpopulated perfmux_idx %d",
			ip_idx, perfmux_idx);
		return false;
	}

	/* Make sure that perfmux belongs to available IP instances */
	if ((perfmux->hw_inst_mask & chip_ip->fs_mask) == 0U) {
		/*
		 * NOTE: User accessing this address indicates that
		 * case 1: perfmux (corresponding IP HW instance) is available
		 * case 2: computed allowlist is incorrect
		 * For case 1,
		 * Perfmux information is added statically.
		 * It is possible that perfmux (or IP) HW instance is not
		 * available in a configuration.
		 * This is a valid case, return false to indicate.
		 */
		tegra_hwpm_err(hwpm,
			"accessed IP %d perfmux %d marked unavailable",
			ip_idx, perfmux_idx);
		return false;
	}

	/* Make sure phys addr belongs to the perfmux */
	if ((phys_addr >= perfmux->start_abs_pa) &&
		(phys_addr <= perfmux->end_abs_pa)) {
		if (t234_hwpm_check_alist(hwpm, perfmux, phys_addr)) {
			*aperture = perfmux;
			return true;
		}
		tegra_hwpm_dbg(hwpm, hwpm_verbose,
			"phys_addr 0x%llx not in IP %d perfmux_idx %d alist",
			phys_addr, ip_idx, perfmux_idx);
		return false;
	}

	tegra_hwpm_err(hwpm, "Execution shouldn't reach here");
	return false;
}

/*
 * Find aperture corresponding to phys addr
 */
static int t234_hwpm_find_aperture(struct tegra_soc_hwpm *hwpm,
	u64 phys_addr, struct hwpm_ip_aperture **aperture)
{
	struct tegra_soc_hwpm_chip *active_chip = NULL;
	struct hwpm_ip *chip_ip = NULL;
	u32 ip_idx;

	tegra_hwpm_fn(hwpm, " ");

	if (hwpm->active_chip == NULL) {
		tegra_hwpm_err(hwpm, "chip struct not populated");
		return -ENODEV;
	}

	active_chip = hwpm->active_chip;

	/* Find IP index */
	for (ip_idx = 0U; ip_idx < T234_HWPM_IP_MAX; ip_idx++) {
		chip_ip = active_chip->chip_ips[ip_idx];
		if (chip_ip == NULL) {
			tegra_hwpm_err(hwpm, "IP %d not populated as expected",
				ip_idx);
			return -ENODEV;
		}

		if (!chip_ip->reserved) {
			continue;
		}

		if (t234_hwpm_is_addr_in_ip_perfmux(
			hwpm, phys_addr, ip_idx, aperture)) {
			return 0;
		}

		if (t234_hwpm_is_addr_in_ip_perfmon(
			hwpm, phys_addr, ip_idx, aperture)) {
			return 0;
		}
	}
	tegra_hwpm_err(hwpm, "addr 0x%llx not found in any IP", phys_addr);
	return -EINVAL;
}

int t234_hwpm_exec_reg_ops(struct tegra_soc_hwpm *hwpm,
	struct tegra_soc_hwpm_reg_op *reg_op)
{
	int ret = 0;
	u32 reg_val = 0U;
	u32 ip_idx = TEGRA_SOC_HWPM_IP_INACTIVE; /* ip_idx is unknown */
	u64 addr_hi = 0ULL;
	struct hwpm_ip_aperture *aperture = NULL;

	tegra_hwpm_fn(hwpm, " ");

	/* Find IP aperture containing phys_addr in allowlist */
	ret = t234_hwpm_find_aperture(hwpm, reg_op->phys_addr, &aperture);
	if (ret < 0) {
		if (ret == -ENODEV) {
			tegra_hwpm_err(hwpm, "HWPM structures not populated");
			reg_op->status =
				TEGRA_SOC_HWPM_REG_OP_STATUS_INSUFFICIENT_PERMISSIONS;
		} else {
			/* Phys addr not available in IP allowlist */
			tegra_hwpm_err(hwpm,
				"Phys addr 0x%llx not available in IP %d",
				reg_op->phys_addr, ip_idx);
			reg_op->status =
				TEGRA_SOC_HWPM_REG_OP_STATUS_INVALID_ADDR;
		}
		goto fail;
	}

	tegra_hwpm_dbg(hwpm, hwpm_verbose,
		"Found phys addr (0x%llx): aperture (0x%llx-0x%llx)",
		reg_op->phys_addr,
		aperture->start_abs_pa, aperture->end_abs_pa);

	switch (reg_op->cmd) {
	case TEGRA_SOC_HWPM_REG_OP_CMD_RD32:
		reg_op->reg_val_lo = tegra_hwpm_regops_readl(hwpm,
					aperture, reg_op->phys_addr);
		reg_op->status = TEGRA_SOC_HWPM_REG_OP_STATUS_SUCCESS;
		break;

	case TEGRA_SOC_HWPM_REG_OP_CMD_RD64:
		addr_hi = tegra_hwpm_safe_add_u64(reg_op->phys_addr, 4ULL);
		reg_op->reg_val_lo = tegra_hwpm_regops_readl(hwpm,
					aperture, reg_op->phys_addr);
		reg_op->reg_val_hi = tegra_hwpm_regops_readl(hwpm,
					aperture, addr_hi);
		reg_op->status = TEGRA_SOC_HWPM_REG_OP_STATUS_SUCCESS;
		break;

	/* Read Modify Write operation */
	case TEGRA_SOC_HWPM_REG_OP_CMD_WR32:
		reg_val = tegra_hwpm_regops_readl(hwpm,
				aperture, reg_op->phys_addr);
		reg_val = set_field(reg_val, reg_op->mask_lo,
				reg_op->reg_val_lo);
		tegra_hwpm_regops_writel(hwpm,
			aperture, reg_op->phys_addr, reg_val);
		reg_op->status = TEGRA_SOC_HWPM_REG_OP_STATUS_SUCCESS;
		break;

	/* Read Modify Write operation */
	case TEGRA_SOC_HWPM_REG_OP_CMD_WR64:
		addr_hi = tegra_hwpm_safe_add_u64(reg_op->phys_addr, 4ULL);

		/* Lower 32 bits */
		reg_val = tegra_hwpm_regops_readl(hwpm,
				aperture, reg_op->phys_addr);
		reg_val = set_field(reg_val, reg_op->mask_lo,
				reg_op->reg_val_lo);
		tegra_hwpm_regops_writel(hwpm,
			aperture, reg_op->phys_addr, reg_val);

		/* Upper 32 bits */
		reg_val = tegra_hwpm_regops_readl(hwpm, aperture, addr_hi);
		reg_val = set_field(reg_val, reg_op->mask_hi,
				reg_op->reg_val_hi);
		tegra_hwpm_regops_writel(hwpm,
			aperture, reg_op->phys_addr, reg_val);
		reg_op->status = TEGRA_SOC_HWPM_REG_OP_STATUS_SUCCESS;
		break;

	default:
		tegra_hwpm_err(hwpm, "Invalid reg op command(%u)", reg_op->cmd);
		reg_op->status = TEGRA_SOC_HWPM_REG_OP_STATUS_INVALID_CMD;
		ret = -EINVAL;
		break;
	}
fail:
	return ret;
}
