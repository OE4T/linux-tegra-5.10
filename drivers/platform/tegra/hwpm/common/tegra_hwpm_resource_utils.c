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

/* ip_idx indicates internal active ip index */
static int tegra_hwpm_reserve_given_resource(struct tegra_soc_hwpm *hwpm,
	u32 ip_idx)
{
	int err = 0, ret = 0;
	u32 perfmux_idx, perfmon_idx;
	unsigned long inst_idx = 0UL;
	unsigned long floorsweep_info = 0UL, reserved_insts = 0UL;
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	struct hwpm_ip *chip_ip = active_chip->chip_ips[ip_idx];
	hwpm_ip_perfmon *perfmon = NULL;
	hwpm_ip_perfmux *perfmux = NULL;

	floorsweep_info = (unsigned long)chip_ip->fs_mask;

	tegra_hwpm_fn(hwpm, " ");

	tegra_hwpm_dbg(hwpm, hwpm_info, "Reserve IP %d, fs_mask 0x%x",
		ip_idx, chip_ip->fs_mask);

	/* PMA and RTR are already reserved */
	if ((ip_idx == active_chip->get_pma_int_idx(hwpm)) ||
		(ip_idx == active_chip->get_rtr_int_idx(hwpm))) {
		return 0;
	}

	for_each_set_bit(inst_idx, &floorsweep_info, 32U) {
		/* Reserve all perfmon belonging to this instance */
		for (perfmon_idx = 0U; perfmon_idx < chip_ip->num_perfmon_slots;
			perfmon_idx++) {
			perfmon = chip_ip->ip_perfmon[perfmon_idx];

			if (perfmon == NULL) {
				continue;
			}

			if (perfmon->hw_inst_mask != BIT(inst_idx)) {
				continue;
			}

			err = tegra_hwpm_perfmon_reserve(hwpm, perfmon);
			if (err != 0) {
				tegra_hwpm_err(hwpm,
					"IP %d perfmon %d reserve failed",
					ip_idx, perfmon_idx);
				goto fail;
			}
		}

		/* Reserve all perfmux belonging to this instance */
		for (perfmux_idx = 0U; perfmux_idx < chip_ip->num_perfmux_slots;
			perfmux_idx++) {
			perfmux = chip_ip->ip_perfmux[perfmux_idx];

			if (perfmux == NULL) {
				continue;
			}

			if (perfmux->hw_inst_mask != BIT(inst_idx)) {
				continue;
			}

			err = tegra_hwpm_perfmux_reserve(hwpm, perfmux);
			if (err != 0) {
				tegra_hwpm_err(hwpm,
					"IP %d perfmux %d reserve failed",
					ip_idx, perfmux_idx);
				goto fail;
			}
		}

		reserved_insts |= BIT(inst_idx);
	}
	chip_ip->reserved = true;

	return 0;
fail:
	if (hwpm->active_chip->perfmon_disable == NULL) {
		tegra_hwpm_err(hwpm, "perfmon_disable HAL uninitialized");
		return -ENODEV;
	}

	if (hwpm->active_chip->perfmux_disable == NULL) {
		tegra_hwpm_err(hwpm, "perfmux_disable HAL uninitialized");
		return -ENODEV;
	}

	/* release reserved instances */
	for_each_set_bit(inst_idx, &reserved_insts, 32U) {
		/* Release all perfmon belonging to this instance */
		for (perfmon_idx = 0U; perfmon_idx < chip_ip->num_perfmon_slots;
			perfmon_idx++) {
			perfmon = chip_ip->ip_perfmon[perfmon_idx];

			if (perfmon == NULL) {
				continue;
			}

			if (perfmon->hw_inst_mask != BIT(inst_idx)) {
				continue;
			}

			ret = hwpm->active_chip->perfmon_disable(hwpm, perfmon);
			if (ret != 0) {
				tegra_hwpm_err(hwpm,
					"IP %d perfmon %d disable failed",
					ip_idx, perfmon_idx);
			}

			ret = tegra_hwpm_perfmon_release(hwpm, perfmon);
			if (ret != 0) {
				tegra_hwpm_err(hwpm,
					"IP %d perfmon %d release failed",
					ip_idx, perfmon_idx);
			}
		}

		/* Release all perfmux belonging to this instance */
		for (perfmux_idx = 0U; perfmux_idx < chip_ip->num_perfmux_slots;
			perfmux_idx++) {
			perfmux = chip_ip->ip_perfmux[perfmux_idx];

			if (perfmux == NULL) {
				continue;
			}

			if (perfmux->hw_inst_mask != BIT(inst_idx)) {
				continue;
			}

			ret = hwpm->active_chip->perfmux_disable(hwpm, perfmux);
			if (ret != 0) {
				tegra_hwpm_err(hwpm,
					"IP %d perfmux %d disable failed",
					ip_idx, perfmux_idx);
			}

			ret = tegra_hwpm_perfmux_release(hwpm, perfmux);
			if (ret != 0) {
				tegra_hwpm_err(hwpm,
					"IP %d perfmux %d release failed",
					ip_idx, perfmux_idx);
			}
		}
	}
	return err;
}

int tegra_hwpm_reserve_resource(struct tegra_soc_hwpm *hwpm, u32 resource)
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

	ret = tegra_hwpm_reserve_given_resource(hwpm, ip_idx);
	if (ret != 0) {
		tegra_hwpm_err(hwpm, "Failed to reserve resource %d", resource);
		return ret;
	}

	return 0;
}

static int tegra_hwpm_bind_reserved_resources(struct tegra_soc_hwpm *hwpm)
{
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	struct hwpm_ip *chip_ip = NULL;
	u32 ip_idx;
	u32 perfmux_idx, perfmon_idx;
	unsigned long inst_idx = 0UL;
	unsigned long floorsweep_info = 0UL;
	int err = 0;
	hwpm_ip_perfmon *perfmon = NULL;
	hwpm_ip_perfmux *perfmux = NULL;

	tegra_hwpm_fn(hwpm, " ");

	if (hwpm->active_chip->zero_alist_regs == NULL) {
		tegra_hwpm_err(hwpm,
			"zero_alist_regs HAL uninitialized");
		return -ENODEV;
	}

	if (hwpm->active_chip->perfmon_enable == NULL) {
		tegra_hwpm_err(hwpm,
			"perfmon_enable HAL uninitialized");
		return -ENODEV;
	}

	for (ip_idx = 0U; ip_idx < active_chip->get_ip_max_idx(hwpm);
		ip_idx++) {
		chip_ip = active_chip->chip_ips[ip_idx];

		/* Skip unavailable IPs */
		if (!chip_ip->reserved) {
			continue;
		}

		if (chip_ip->fs_mask == 0U) {
			/* No IP instance is available */
			continue;
		}

		floorsweep_info = (unsigned long)chip_ip->fs_mask;

		for_each_set_bit(inst_idx, &floorsweep_info, 32U) {
			/* Zero out necessary perfmux registers */
			for (perfmux_idx = 0U;
				perfmux_idx < chip_ip->num_perfmux_slots;
				perfmux_idx++) {
				perfmux = chip_ip->ip_perfmux[perfmux_idx];

				if (perfmux == NULL) {
					continue;
				}

				if (perfmux->hw_inst_mask != BIT(inst_idx)) {
					continue;
				}

				err = hwpm->active_chip->zero_alist_regs(
					hwpm, perfmux);
				if (err != 0) {
					tegra_hwpm_err(hwpm, "IP %d"
						" perfmux %d zero regs failed",
						ip_idx, perfmux_idx);
				}
			}

			/* Zero out necessary perfmon registers */
			/* And enable reporting of PERFMON status */
			for (perfmon_idx = 0U;
				perfmon_idx < chip_ip->num_perfmon_slots;
				perfmon_idx++) {
				perfmon = chip_ip->ip_perfmon[perfmon_idx];

				if (perfmon == NULL) {
					continue;
				}

				if (perfmon->hw_inst_mask != BIT(inst_idx)) {
					continue;
				}

				err = hwpm->active_chip->zero_alist_regs(
					hwpm, perfmon);
				if (err != 0) {
					tegra_hwpm_err(hwpm, "IP %d"
						" perfmon %d zero regs failed",
						ip_idx, perfmon_idx);
				}

				err = hwpm->active_chip->perfmon_enable(
					hwpm, perfmon);
				if (err != 0) {
					tegra_hwpm_err(hwpm, "IP %d"
						" perfmon %d enable failed",
						ip_idx, perfmon_idx);
				}
			}
		}
	}
	return err;
}

int tegra_hwpm_bind_resources(struct tegra_soc_hwpm *hwpm)
{
	int ret = 0;

	tegra_hwpm_fn(hwpm, " ");

	ret = tegra_hwpm_bind_reserved_resources(hwpm);
	if (ret != 0) {
		tegra_hwpm_err(hwpm, "failed to bind resources");
		return ret;
	}

	return 0;
}

int tegra_hwpm_release_all_resources(struct tegra_soc_hwpm *hwpm)
{
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	struct hwpm_ip *chip_ip = NULL;
	hwpm_ip_perfmon *perfmon = NULL;
	hwpm_ip_perfmux *perfmux = NULL;
	u32 ip_idx;
	u32 perfmux_idx, perfmon_idx;
	unsigned long floorsweep_info = 0UL;
	unsigned long inst_idx = 0UL;
	int err = 0;

	tegra_hwpm_fn(hwpm, " ");

	for (ip_idx = 0U; ip_idx < active_chip->get_ip_max_idx(hwpm);
		ip_idx++) {
		chip_ip = active_chip->chip_ips[ip_idx];

		/* PMA and RTR will be released later */
		if ((ip_idx == active_chip->get_pma_int_idx(hwpm)) ||
			(ip_idx == active_chip->get_rtr_int_idx(hwpm))) {
			continue;
		}

		/* Disable only available IPs */
		if (chip_ip->override_enable) {
			/* IP not available */
			continue;
		}

		/* Disable and release only reserved IPs */
		if (!chip_ip->reserved) {
			continue;
		}

		if (chip_ip->fs_mask == 0U) {
			/* No IP instance is available */
			continue;
		}

		if (hwpm->active_chip->perfmon_disable == NULL) {
			tegra_hwpm_err(hwpm,
				"perfmon_disable HAL uninitialized");
			return -ENODEV;
		}

		if (hwpm->active_chip->perfmux_disable == NULL) {
			tegra_hwpm_err(hwpm,
				"perfmux_disable HAL uninitialized");
			return -ENODEV;
		}

		floorsweep_info = (unsigned long)chip_ip->fs_mask;

		for_each_set_bit(inst_idx, &floorsweep_info, 32U) {
			/* Release all perfmon associated with inst_idx */
			for (perfmon_idx = 0U;
				perfmon_idx < chip_ip->num_perfmon_slots;
				perfmon_idx++) {
				perfmon = chip_ip->ip_perfmon[perfmon_idx];

				if (perfmon == NULL) {
					continue;
				}

				if (perfmon->hw_inst_mask != BIT(inst_idx)) {
					continue;
				}

				err = hwpm->active_chip->perfmon_disable(
					hwpm, perfmon);
				if (err != 0) {
					tegra_hwpm_err(hwpm, "IP %d"
						" perfmon %d disable failed",
						ip_idx, perfmon_idx);
				}

				err = tegra_hwpm_perfmon_release(hwpm, perfmon);
				if (err != 0) {
					tegra_hwpm_err(hwpm, "IP %d"
						" perfmon %d release failed",
						ip_idx, perfmon_idx);
				}
			}

			/* Release all perfmux associated with inst_idx */
			for (perfmux_idx = 0U;
				perfmux_idx < chip_ip->num_perfmux_slots;
				perfmux_idx++) {
				perfmux = chip_ip->ip_perfmux[perfmux_idx];

				if (perfmux == NULL) {
					continue;
				}

				if (perfmux->hw_inst_mask != BIT(inst_idx)) {
					continue;
				}

				err = hwpm->active_chip->perfmux_disable(
					hwpm, perfmux);
				if (err != 0) {
					tegra_hwpm_err(hwpm, "IP %d"
						" perfmux %d disable failed",
						ip_idx, perfmux_idx);
				}

				err = tegra_hwpm_perfmux_release(hwpm, perfmux);
				if (err != 0) {
					tegra_hwpm_err(hwpm, "IP %d"
						" perfmux %d release failed",
						ip_idx, perfmux_idx);
				}
			}
		}
		chip_ip->reserved = false;
	}
	return 0;
}

int tegra_hwpm_release_resources(struct tegra_soc_hwpm *hwpm)
{
	int ret = 0;

	tegra_hwpm_fn(hwpm, " ");

	ret = tegra_hwpm_release_all_resources(hwpm);
	if (ret != 0) {
		tegra_hwpm_err(hwpm, "failed to release resources");
		return ret;
	}

	return 0;
}
