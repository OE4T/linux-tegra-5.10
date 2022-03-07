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

#include <linux/slab.h>
#include <uapi/linux/tegra-soc-hwpm-uapi.h>

#include <tegra_hwpm.h>
#include <tegra_hwpm_log.h>
#include <tegra_hwpm_common.h>
#include <tegra_hwpm_static_analysis.h>

static int tegra_hwpm_init_ip_perfmux_apertures(struct tegra_soc_hwpm *hwpm,
	struct hwpm_ip *chip_ip)
{
	u32 idx = 0U, perfmux_idx = 0U, max_perfmux = 0U;
	u64 perfmux_address_range = 0ULL, perfmux_offset = 0ULL;
	hwpm_ip_perfmux *perfmux = NULL;

	/* Initialize perfmux array */
	if (chip_ip->num_perfmux_per_inst == 0U) {
		/* no perfmux in this IP */
		return 0;
	}

	perfmux_address_range = tegra_hwpm_safe_add_u64(
		tegra_hwpm_safe_sub_u64(chip_ip->perfmux_range_end,
		chip_ip->perfmux_range_start), 1ULL);
	chip_ip->num_perfmux_slots = tegra_hwpm_safe_cast_u64_to_u32(
		perfmux_address_range / chip_ip->inst_perfmux_stride);

	chip_ip->ip_perfmux = kzalloc(
		sizeof(hwpm_ip_perfmux *) * chip_ip->num_perfmux_slots,
		GFP_KERNEL);
	if (chip_ip->ip_perfmux == NULL) {
		tegra_hwpm_err(hwpm, "Perfmux pointer array allocation failed");
		return -ENOMEM;
	}

	/* Set all perfmux slot pointers to NULL */
	for (idx = 0U; idx < chip_ip->num_perfmux_slots; idx++) {
		chip_ip->ip_perfmux[idx] = NULL;
	}

	/* Assign valid perfmuxes to corresponding slot pointers */
	max_perfmux = chip_ip->num_instances * chip_ip->num_perfmux_per_inst;
	for (perfmux_idx = 0U; perfmux_idx < max_perfmux; perfmux_idx++) {
		perfmux = &chip_ip->perfmux_static_array[perfmux_idx];

		/* Compute perfmux offset from perfmux range start */
		perfmux_offset = tegra_hwpm_safe_sub_u64(
			perfmux->start_abs_pa, chip_ip->perfmux_range_start);

		/* Compute perfmux slot index */
		idx = tegra_hwpm_safe_cast_u64_to_u32(
			perfmux_offset / chip_ip->inst_perfmux_stride);

		/* Set perfmux slot pointer */
		chip_ip->ip_perfmux[idx] = perfmux;
	}

	return 0;
}

static int tegra_hwpm_init_ip_perfmon_apertures(struct tegra_soc_hwpm *hwpm,
	struct hwpm_ip *chip_ip)
{
	u32 idx = 0U, perfmon_idx = 0U, max_perfmon = 0U;
	u64 perfmon_address_range = 0ULL, perfmon_offset = 0ULL;
	hwpm_ip_perfmon *perfmon = NULL;

	/* Initialize perfmon array */
	if (chip_ip->num_perfmon_per_inst == 0U) {
		/* no perfmons in this IP */
		return 0;
	}

	perfmon_address_range = tegra_hwpm_safe_add_u64(
		tegra_hwpm_safe_sub_u64(chip_ip->perfmon_range_end,
			chip_ip->perfmon_range_start), 1ULL);
	chip_ip->num_perfmon_slots = tegra_hwpm_safe_cast_u64_to_u32(
		perfmon_address_range / chip_ip->inst_perfmon_stride);

	chip_ip->ip_perfmon = kzalloc(
		sizeof(hwpm_ip_perfmon *) * chip_ip->num_perfmon_slots,
		GFP_KERNEL);
	if (chip_ip->ip_perfmon == NULL) {
		tegra_hwpm_err(hwpm, "Perfmon pointer array allocation failed");
		return -ENOMEM;
	}

	/* Set all perfmon slot pointers to NULL */
	for (idx = 0U; idx < chip_ip->num_perfmon_slots; idx++) {
		chip_ip->ip_perfmon[idx] = NULL;
	}

	/* Assign valid perfmuxes to corresponding slot pointers */
	max_perfmon = chip_ip->num_instances * chip_ip->num_perfmon_per_inst;
	for (perfmon_idx = 0U; perfmon_idx < max_perfmon; perfmon_idx++) {
		perfmon = &chip_ip->perfmon_static_array[perfmon_idx];

		/* Compute perfmon offset from perfmon range start */
		perfmon_offset = tegra_hwpm_safe_sub_u64(
			perfmon->start_abs_pa, chip_ip->perfmon_range_start);

		/* Compute perfmon slot index */
		idx = tegra_hwpm_safe_cast_u64_to_u32(
			perfmon_offset / chip_ip->inst_perfmon_stride);

		/* Set perfmon slot pointer */
		chip_ip->ip_perfmon[idx] = perfmon;
	}

	return 0;
}

int tegra_hwpm_init_chip_ip_structures(struct tegra_soc_hwpm *hwpm)
{
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	struct hwpm_ip *chip_ip = NULL;
	u32 ip_idx;
	int ret = 0;

	for (ip_idx = 0U; ip_idx < active_chip->get_ip_max_idx(hwpm);
		ip_idx++) {
		chip_ip = active_chip->chip_ips[ip_idx];

		ret = tegra_hwpm_init_ip_perfmon_apertures(hwpm, chip_ip);
		if (ret != 0) {
			tegra_hwpm_err(hwpm, "IP %d perfmon alloc failed",
				ip_idx);
			return ret;
		}

		ret = tegra_hwpm_init_ip_perfmux_apertures(hwpm, chip_ip);
		if (ret != 0) {
			tegra_hwpm_err(hwpm, "IP %d perfmux alloc failed",
				ip_idx);
			return ret;
		}
	}

	return 0;
}

/*
 * This function finds the IP perfmon index corresponding to given base address.
 * Perfmon aperture belongs to IP domain and contains IP instance info
 * wrt base address.
 * Return instance index
 */
static int tegra_hwpm_find_ip_perfmon_index(struct tegra_soc_hwpm *hwpm,
	u64 base_addr, u32 ip_index, u32 *ip_perfmon_idx)
{
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	struct hwpm_ip *chip_ip = active_chip->chip_ips[ip_index];
	u32 perfmon_idx;
	u64 addr_offset = 0ULL;
	hwpm_ip_perfmon *perfmon = NULL;

	tegra_hwpm_fn(hwpm, " ");

	if (ip_perfmon_idx == NULL) {
		tegra_hwpm_err(hwpm, "pointer for ip_perfmon_idx is NULL");
		return -EINVAL;
	}

	/* Validate phys_addr falls in IP address range */
	if ((base_addr < chip_ip->perfmon_range_start) ||
		(base_addr > chip_ip->perfmon_range_end)) {
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
	addr_offset = tegra_hwpm_safe_sub_u64(
		base_addr, chip_ip->perfmon_range_start);
	perfmon_idx = tegra_hwpm_safe_cast_u64_to_u32(
		addr_offset / chip_ip->inst_perfmon_stride);

	/* Make sure instance index is valid */
	if (perfmon_idx >= chip_ip->num_perfmon_slots) {
		tegra_hwpm_err(hwpm,
			"IP:%d -> base addr 0x%llx is out of bounds",
			ip_index, base_addr);
		return -EINVAL;
	}

	/* Validate IP instance perfmon start address = given phys addr */
	perfmon = chip_ip->ip_perfmon[perfmon_idx];

	if (perfmon == NULL) {
		/*
		 * This a valid case. For example, not all MSS base addresses
		 * are shared between MSS IPs.
		 */
		tegra_hwpm_dbg(hwpm, hwpm_info,
			"For addr 0x%llx IP %d perfmon_idx %d not populated",
			base_addr, ip_index, perfmon_idx);
		return -ENODEV;
	}

	if (base_addr != perfmon->start_abs_pa) {
		tegra_hwpm_dbg(hwpm, hwpm_info,
			"base addr 0x%llx != perfmon abs addr", base_addr);
		return -EINVAL;
	}

	*ip_perfmon_idx = perfmon_idx;

	return 0;
}

/*
 * This function finds the IP perfmux index corresponding to given base address.
 * Perfmux aperture belongs to IP domain and contains IP instance info
 * wrt base address.
 * Return instance index
 */
static int tegra_hwpm_find_ip_perfmux_index(struct tegra_soc_hwpm *hwpm,
	u64 base_addr, u32 ip_index, u32 *ip_perfmux_idx)
{
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	struct hwpm_ip *chip_ip = active_chip->chip_ips[ip_index];
	u32 perfmux_idx;
	u64 addr_offset = 0ULL;
	hwpm_ip_perfmux *perfmux = NULL;

	tegra_hwpm_fn(hwpm, " ");

	if (ip_perfmux_idx == NULL) {
		tegra_hwpm_err(hwpm, "pointer for ip_perfmux_idx is NULL");
		return -EINVAL;
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
	addr_offset = tegra_hwpm_safe_sub_u64(
		base_addr, chip_ip->perfmux_range_start);
	perfmux_idx = tegra_hwpm_safe_cast_u64_to_u32(
		addr_offset / chip_ip->inst_perfmux_stride);

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
		 * This a valid case. For example, not all MSS base addresses
		 * are shared between MSS IPs.
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

static int tegra_hwpm_update_ip_floorsweep_mask(struct tegra_soc_hwpm *hwpm,
	u32 ip_idx, u32 hw_inst_mask, bool available)
{
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	struct hwpm_ip *chip_ip = active_chip->chip_ips[ip_idx];

	tegra_hwpm_fn(hwpm, " ");

	/* Update floorsweep info */
	if (available) {
		chip_ip->fs_mask |= hw_inst_mask;
	} else {
		chip_ip->fs_mask &= ~(hw_inst_mask);
	}

	return 0;
}

static int tegra_hwpm_update_ip_ops_info(struct tegra_soc_hwpm *hwpm,
	struct tegra_soc_hwpm_ip_ops *hwpm_ip_ops,
	u32 ip_idx, u32 ip_perfmux_idx, bool available)
{
	u32 perfmux_idx, max_num_perfmux = 0U;
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	struct hwpm_ip *chip_ip = active_chip->chip_ips[ip_idx];
	struct tegra_hwpm_ip_ops *ip_ops;
	hwpm_ip_perfmux *given_perfmux = chip_ip->ip_perfmux[ip_perfmux_idx];
	hwpm_ip_perfmux *perfmux = NULL;

	tegra_hwpm_fn(hwpm, " ");

	/* Update IP ops info for all perfmuxes in the instance */
	max_num_perfmux = tegra_hwpm_safe_mult_u32(
		chip_ip->num_instances, chip_ip->num_perfmux_per_inst);
	for (perfmux_idx = 0U; perfmux_idx < max_num_perfmux; perfmux_idx++) {
		perfmux = &chip_ip->perfmux_static_array[perfmux_idx];

		if (perfmux->hw_inst_mask != given_perfmux->hw_inst_mask) {
			continue;
		}

		ip_ops = &perfmux->ip_ops;

		if (available) {
			ip_ops->ip_dev = hwpm_ip_ops->ip_dev;
			ip_ops->hwpm_ip_pm = hwpm_ip_ops->hwpm_ip_pm;
			ip_ops->hwpm_ip_reg_op = hwpm_ip_ops->hwpm_ip_reg_op;
		} else {
			ip_ops->ip_dev = NULL;
			ip_ops->hwpm_ip_pm = NULL;
			ip_ops->hwpm_ip_reg_op = NULL;
		}
	}

	return 0;
}

/*
 * Find IP hw instance mask and update IP floorsweep info and IP ops.
 */
int tegra_hwpm_set_fs_info_ip_ops(struct tegra_soc_hwpm *hwpm,
	struct tegra_soc_hwpm_ip_ops *hwpm_ip_ops,
	u64 base_address, u32 ip_idx, bool available)
{
	int ret = 0;
	u32 perfmux_idx = 0U, perfmon_idx = 0U;
	struct tegra_soc_hwpm_chip *active_chip = NULL;
	struct hwpm_ip *chip_ip = NULL;
	hwpm_ip_perfmux *perfmux = NULL;
	hwpm_ip_perfmon *perfmon = NULL;

	tegra_hwpm_fn(hwpm, " ");

	if (hwpm->active_chip == NULL) {
		tegra_hwpm_err(hwpm, "chip struct not populated");
		return -ENODEV;
	}

	active_chip = hwpm->active_chip;

	if (ip_idx == TEGRA_SOC_HWPM_IP_INACTIVE) {
		tegra_hwpm_err(hwpm, "invalid ip_idx %d", ip_idx);
		return -EINVAL;
	}

	chip_ip = active_chip->chip_ips[ip_idx];

	if (chip_ip == NULL) {
		tegra_hwpm_err(hwpm, "IP %d not populated", ip_idx);
		return -ENODEV;
	}

	if (chip_ip->override_enable) {
		/* This IP should not be configured for HWPM */
		tegra_hwpm_dbg(hwpm, hwpm_info,
			"IP %d enable override", ip_idx);
		return 0; /* Should this be notified to caller or ignored */
	}

	if (chip_ip->num_perfmux_per_inst != 0U) {
		/* Step 1: find IP hw instance mask using perfmux */
		ret = tegra_hwpm_find_ip_perfmux_index(hwpm,
			base_address, ip_idx, &perfmux_idx);
		if (ret != 0) {
			/* Error will be printed handled by parent function */
			goto fail;
		}

		perfmux = chip_ip->ip_perfmux[perfmux_idx];

		/* Step 2: Update IP floorsweep info */
		ret = tegra_hwpm_update_ip_floorsweep_mask(
			hwpm, ip_idx, perfmux->hw_inst_mask, available);
		if (ret != 0) {
			tegra_hwpm_err(hwpm, "IP %d perfmux %d base 0x%llx: "
				"FS mask update failed",
				ip_idx, perfmux_idx, base_address);
			goto fail;
		}

		if (hwpm_ip_ops != NULL) {
			/* Update IP ops */
			ret = tegra_hwpm_update_ip_ops_info(hwpm, hwpm_ip_ops,
				ip_idx, perfmux_idx, available);
			if (ret != 0) {
				tegra_hwpm_err(hwpm,
					"IP %d perfmux %d: Failed to update ip_ops",
					ip_idx, perfmux_idx);
				goto fail;
			}
		}
	} else {
		/* Step 1: find IP hw instance mask using perfmon */
		ret = tegra_hwpm_find_ip_perfmon_index(hwpm,
			base_address, ip_idx, &perfmon_idx);
		if (ret != 0) {
			/* Error will be printed handled by parent function */
			goto fail;
		}

		perfmon = chip_ip->ip_perfmon[perfmon_idx];

		/* Step 2: Update IP floorsweep info */
		ret = tegra_hwpm_update_ip_floorsweep_mask(
			hwpm, ip_idx, perfmon->hw_inst_mask, available);
		if (ret != 0) {
			tegra_hwpm_err(hwpm, "IP %d perfmon %d base 0x%llx: "
				"FS mask update failed",
				ip_idx, perfmon_idx, base_address);
			goto fail;
		}
	}

fail:
	return ret;
}

static int tegra_hwpm_complete_ip_register(struct tegra_soc_hwpm *hwpm)
{
	int ret = 0;
	struct hwpm_ip_register_list *node = ip_register_list_head;

	tegra_hwpm_fn(hwpm, " ");

	if (hwpm->active_chip->extract_ip_ops == NULL) {
		tegra_hwpm_err(hwpm, "extract_ip_ops uninitialized");
		return -ENODEV;
	}

	while (node != NULL) {
		tegra_hwpm_dbg(hwpm, hwpm_info, "IP ext idx %d info",
			node->ip_ops.ip_index);
		ret = hwpm->active_chip->extract_ip_ops(
			hwpm, &node->ip_ops, true);
		if (ret != 0) {
			tegra_hwpm_err(hwpm, "Failed to extract IP ops");
			return ret;
		}
		node = node->next;
	}
	return ret;
}

/*
 * There are 3 ways to get info about available IPs
 * 1. IP register to HWPM driver
 * 2. IP register to HWPM before HWPM driver is probed
 * 3. Force enabled IPs
 *
 * This function will handle case 2 and 3
 */
int tegra_hwpm_finalize_chip_info(struct tegra_soc_hwpm *hwpm)
{
	int ret = 0;

	tegra_hwpm_fn(hwpm, " ");

	/*
	 * Go through IP registration requests received before HWPM
	 * driver was probed.
	 */
	ret = tegra_hwpm_complete_ip_register(hwpm);
	if (ret != 0) {
		tegra_hwpm_err(hwpm, "Failed register IPs");
		return ret;
	}

	if (hwpm->active_chip->force_enable_ips == NULL) {
		tegra_hwpm_err(hwpm, "force_enable_ips uninitialized");
		return -ENODEV;
	}
	ret = hwpm->active_chip->force_enable_ips(hwpm);
	if (ret != 0) {
		tegra_hwpm_err(hwpm, "Failed to force enable IPs");
		return ret;
	}

	return ret;
}
