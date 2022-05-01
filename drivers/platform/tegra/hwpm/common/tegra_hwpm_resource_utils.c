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

int tegra_hwpm_func_single_aperture(struct tegra_soc_hwpm *hwpm,
	struct tegra_hwpm_func_args *func_args,
	enum tegra_hwpm_funcs iia_func, u32 ip_idx, struct hwpm_ip *chip_ip,
	u32 inst_idx, u32 aperture_idx, enum hwpm_aperture_type a_type)
{
	struct hwpm_ip_aperture *aperture = NULL;
	int err = 0, ret = 0;

	if (a_type == HWPM_APERTURE_PERFMUX) {
		aperture = chip_ip->ip_perfmux[aperture_idx];
	} else if (a_type == HWPM_APERTURE_PERFMON) {
		aperture = chip_ip->ip_perfmon[aperture_idx];
	} else {
		tegra_hwpm_err(hwpm, "INVALID a_type %d", a_type);
		return -EINVAL;
	}

	if (aperture == NULL) {
		return 0;
	}

	if (aperture->hw_inst_mask != BIT(inst_idx)) {
		return 0;
	}

	switch (iia_func) {
	case TEGRA_HWPM_GET_ALIST_SIZE:
		if (aperture->alist) {
			hwpm->full_alist_size =
				tegra_hwpm_safe_add_u64(
					hwpm->full_alist_size,
					aperture->alist_size);
		} else {
			tegra_hwpm_err(hwpm, "IP %d"
				" aperture type %d idx %d NULL alist",
				ip_idx, a_type, aperture_idx);
		}
		break;
	case TEGRA_HWPM_COMBINE_ALIST:
		err = hwpm->active_chip->copy_alist(hwpm,
			aperture, func_args->alist, &func_args->full_alist_idx);
		if (err != 0) {
			tegra_hwpm_err(hwpm, "IP %d"
				" aperture type %d idx %d alist copy failed",
				ip_idx, a_type, aperture_idx);
			return err;
		}
		break;
	case TEGRA_HWPM_RESERVE_GIVEN_RESOURCE:
		if (a_type == HWPM_APERTURE_PERFMUX) {
			err = tegra_hwpm_perfmux_reserve(hwpm, aperture);
			if (err != 0) {
				tegra_hwpm_err(hwpm, "IP %d aperture"
					" type %d idx %d reserve failed",
					ip_idx, aperture_idx);
				goto fail;
			}
		} else if (a_type == HWPM_APERTURE_PERFMON) {
			err = tegra_hwpm_perfmon_reserve(hwpm, aperture);
			if (err != 0) {
				tegra_hwpm_err(hwpm, "IP %d aperture"
					" type %d idx %d reserve failed",
					ip_idx, aperture_idx);
				goto fail;
			}
		}
		break;
	case TEGRA_HWPM_RELEASE_RESOURCES:
		if (a_type == HWPM_APERTURE_PERFMUX) {
			ret = hwpm->active_chip->perfmux_disable(hwpm, aperture);
			if (ret != 0) {
				tegra_hwpm_err(hwpm, "IP %d aperture"
					" type %d idx %d disable failed",
					ip_idx, aperture_idx);
			}

			ret = tegra_hwpm_perfmux_release(hwpm, aperture);
			if (ret != 0) {
				tegra_hwpm_err(hwpm, "IP %d aperture"
					" type %d idx %d release failed",
					ip_idx, aperture_idx);
			}
		} else if (a_type == HWPM_APERTURE_PERFMON) {
			ret = hwpm->active_chip->perfmon_disable(hwpm, aperture);
			if (ret != 0) {
				tegra_hwpm_err(hwpm, "IP %d aperture"
					" type %d idx %d disable failed",
					ip_idx, aperture_idx);
			}

			ret = tegra_hwpm_perfmon_release(hwpm, aperture);
			if (ret != 0) {
				tegra_hwpm_err(hwpm, "IP %d aperture"
					" type %d idx %d release failed",
					ip_idx, aperture_idx);
			}
		}
		break;
	case TEGRA_HWPM_BIND_RESOURCES:
		err = hwpm->active_chip->zero_alist_regs(hwpm, aperture);
		if (err != 0) {
			tegra_hwpm_err(hwpm, "IP %d aperture"
				" type %d idx %d zero regs failed",
				ip_idx, aperture_idx);
			goto fail;
		}
		if (a_type == HWPM_APERTURE_PERFMON) {
			err = hwpm->active_chip->perfmon_enable(hwpm, aperture);
			if (err != 0) {
				tegra_hwpm_err(hwpm, "IP %d aperture"
					" type %d idx %d enable failed",
					ip_idx, aperture_idx);
				goto fail;
			}
		}
		break;
	default:
		tegra_hwpm_err(hwpm, "func 0x%x unknown", iia_func);
		return -EINVAL;
		break;
	}

	return 0;
fail:
	return err;
}

int tegra_hwpm_func_all_perfmons(struct tegra_soc_hwpm *hwpm,
	struct tegra_hwpm_func_args *func_args,
	enum tegra_hwpm_funcs iia_func, u32 ip_idx, struct hwpm_ip *chip_ip,
	u32 inst_idx)
{
	u32 perfmon_idx;
	int err = 0;

	for (perfmon_idx = 0U; perfmon_idx < chip_ip->num_perfmon_slots;
		perfmon_idx++) {
		err = tegra_hwpm_func_single_aperture(
			hwpm, func_args, iia_func, ip_idx,
			chip_ip, inst_idx, perfmon_idx, HWPM_APERTURE_PERFMON);
		if (err < 0) {
			tegra_hwpm_err(hwpm,
				"IP %d inst %d  perfmon %d func 0x%x failed",
				ip_idx, inst_idx, perfmon_idx, iia_func);
			goto fail;
		}
	}

	return 0;
fail:
	return err;
}

int tegra_hwpm_func_all_perfmuxes(struct tegra_soc_hwpm *hwpm,
	struct tegra_hwpm_func_args *func_args,
	enum tegra_hwpm_funcs iia_func, u32 ip_idx, struct hwpm_ip *chip_ip,
	u32 inst_idx)
{
	u32 perfmux_idx;
	int err = 0;

	for (perfmux_idx = 0U; perfmux_idx < chip_ip->num_perfmux_slots;
		perfmux_idx++) {
		err = tegra_hwpm_func_single_aperture(
			hwpm, func_args, iia_func, ip_idx,
			chip_ip, inst_idx, perfmux_idx, HWPM_APERTURE_PERFMUX);
		if (err < 0) {
			tegra_hwpm_err(hwpm,
				"IP %d inst %d perfmux %d func 0x%x failed",
				ip_idx, inst_idx, perfmux_idx, iia_func);
			goto fail;
		}
	}

	return 0;
fail:
	return err;
}

int tegra_hwpm_func_all_inst(struct tegra_soc_hwpm *hwpm,
	struct tegra_hwpm_func_args *func_args,
	enum tegra_hwpm_funcs iia_func, u32 ip_idx, struct hwpm_ip *chip_ip)
{
	unsigned long inst_idx = 0UL;
	unsigned long floorsweep_info = 0UL;
	unsigned long reserved_insts = 0UL;
	int err = 0;

	/* Execute tasks for all instances if any */
	floorsweep_info = (unsigned long)chip_ip->fs_mask;

	for_each_set_bit(inst_idx, &floorsweep_info, 32U) {
		switch (iia_func) {
		case TEGRA_HWPM_GET_ALIST_SIZE:
			/* do nothing, continue to apertures */
			break;
		case TEGRA_HWPM_COMBINE_ALIST:
			func_args->full_alist_idx = 0ULL;
			break;
		case TEGRA_HWPM_RESERVE_GIVEN_RESOURCE:
			/* do nothing, continue to apertures */
			break;
		case TEGRA_HWPM_BIND_RESOURCES:
			/* do nothing, continue to instances */
			break;
		case TEGRA_HWPM_RELEASE_RESOURCES:
			/* do nothing, continue to instances */
			break;
		default:
			tegra_hwpm_err(hwpm, "func 0x%x unknown", iia_func);
			goto fail;
			break;
		}

		/* Continue functionality for all apertures */
		err = tegra_hwpm_func_all_perfmuxes(
			hwpm, func_args, iia_func, ip_idx, chip_ip, inst_idx);
		if (err < 0) {
			tegra_hwpm_err(hwpm, "IP %d inst %d func 0x%x failed",
				ip_idx, inst_idx, iia_func);
			goto fail;
		}

		err = tegra_hwpm_func_all_perfmons(
			hwpm, func_args, iia_func, ip_idx, chip_ip, inst_idx);
		if (err < 0) {
			tegra_hwpm_err(hwpm, "IP %d inst %d func 0x%x failed",
				ip_idx, inst_idx, iia_func);
			goto fail;
		}

		/* Post execute functionality */
		switch (iia_func) {
		case TEGRA_HWPM_GET_ALIST_SIZE:
			/* do nothing, continue to apertures */
			break;
		case TEGRA_HWPM_COMBINE_ALIST:
			/* do nothing, continue to apertures */
			break;
		case TEGRA_HWPM_RESERVE_GIVEN_RESOURCE:
			reserved_insts |= BIT(inst_idx);
			break;
		case TEGRA_HWPM_BIND_RESOURCES:
			/* do nothing, continue to instances */
			break;
		case TEGRA_HWPM_RELEASE_RESOURCES:
			/* do nothing, continue to instances */
			break;
		default:
			tegra_hwpm_err(hwpm, "func 0x%x unknown", iia_func);
			goto fail;
			break;
		}
	}

	return 0;

fail:
	if (iia_func == TEGRA_HWPM_RESERVE_GIVEN_RESOURCE) {
	/* Revert previously reserved instances of this IP */
		for_each_set_bit(inst_idx, &reserved_insts, 32U) {
			/* Release all apertures belonging to this instance */
			err = tegra_hwpm_func_all_perfmuxes(hwpm, func_args,
				 TEGRA_HWPM_RELEASE_RESOURCES, ip_idx,
				 chip_ip, inst_idx);
			if (err < 0) {
				tegra_hwpm_err(hwpm,
					"IP %d inst %d func 0x%x failed",
					ip_idx, inst_idx, iia_func);
				return err;
			}

			err = tegra_hwpm_func_all_perfmons(hwpm, func_args,
				 TEGRA_HWPM_RELEASE_RESOURCES, ip_idx,
				 chip_ip, inst_idx);
			if (err < 0) {
				tegra_hwpm_err(hwpm,
					"IP %d inst %d func 0x%x failed",
					ip_idx, inst_idx, iia_func);
				return err;
			}
		}
	}

	return err;
}

int tegra_hwpm_func_single_ip(struct tegra_soc_hwpm *hwpm,
	struct tegra_hwpm_func_args *func_args,
	enum tegra_hwpm_funcs iia_func, u32 ip_idx)
{
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	struct hwpm_ip *chip_ip = active_chip->chip_ips[ip_idx];
	int err = 0;

	tegra_hwpm_fn(hwpm, " ");

	if (chip_ip == NULL) {
		tegra_hwpm_err(hwpm, "IP %d not populated", ip_idx);
		return -ENODEV;
	}

	switch (iia_func) {
	case TEGRA_HWPM_GET_ALIST_SIZE:
	case TEGRA_HWPM_COMBINE_ALIST:
	case TEGRA_HWPM_BIND_RESOURCES:
		/* Skip unavailable IPs */
		if (!chip_ip->reserved) {
			return 0;
		}

		if (chip_ip->fs_mask == 0U) {
			/* No IP instance is available */
			return 0;
		}
		break;
	case TEGRA_HWPM_RESERVE_GIVEN_RESOURCE:
		/* PMA and RTR are already reserved */
		if ((ip_idx == active_chip->get_pma_int_idx(hwpm)) ||
			(ip_idx == active_chip->get_rtr_int_idx(hwpm))) {
			return 0;
		}
		/* Skip IPs which are already reserved */
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
		break;
	case TEGRA_HWPM_RELEASE_RESOURCES:
		/* PMA and RTR will be released later */
		if ((ip_idx == active_chip->get_pma_int_idx(hwpm)) ||
			(ip_idx == active_chip->get_rtr_int_idx(hwpm))) {
			return 0;
		}
		/* Skip unavailable IPs */
		if (!chip_ip->reserved) {
			return 0;
		}

		if (chip_ip->fs_mask == 0U) {
			/* No IP instance is available */
			return 0;
		}
		break;
	default:
		tegra_hwpm_err(hwpm, "func 0x%x unknown", iia_func);
		goto fail;
		break;
	}

	/* Continue functionality for all instances in this IP */
	err = tegra_hwpm_func_all_inst(hwpm, func_args, iia_func,
		ip_idx, chip_ip);
	if (err < 0) {
		tegra_hwpm_err(hwpm, "IP %d func 0x%x failed",
			ip_idx, iia_func);
		goto fail;
	}

	/* Post execute functionality */
	if (iia_func == TEGRA_HWPM_RESERVE_GIVEN_RESOURCE) {
		chip_ip->reserved = true;
	}
	if (iia_func == TEGRA_HWPM_RELEASE_RESOURCES) {
		chip_ip->reserved = false;
	}

	return 0;
fail:
	return err;
}

int tegra_hwpm_func_all_ip(struct tegra_soc_hwpm *hwpm,
	struct tegra_hwpm_func_args *func_args,
	enum tegra_hwpm_funcs iia_func)
{
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	u32 ip_idx;
	int err = 0;

	tegra_hwpm_fn(hwpm, " ");

	for (ip_idx = 0U; ip_idx < active_chip->get_ip_max_idx(hwpm);
		ip_idx++) {

		err = tegra_hwpm_func_single_ip(
			hwpm, func_args, iia_func, ip_idx);
		if (err < 0) {
			tegra_hwpm_err(hwpm, "IP %d func 0x%x failed",
				ip_idx, iia_func);
			goto fail;
		}
	}

	return 0;
fail:
	return err;
}

int tegra_hwpm_reserve_resource(struct tegra_soc_hwpm *hwpm, u32 resource)
{
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	u32 ip_idx = TEGRA_SOC_HWPM_IP_INACTIVE;
	int err = 0;

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

	err = tegra_hwpm_func_single_ip(hwpm, NULL,
		TEGRA_HWPM_RESERVE_GIVEN_RESOURCE, ip_idx);
	if (err != 0) {
		tegra_hwpm_err(hwpm, "failed to reserve IP %d", ip_idx);
		return err;
	}

	return 0;
}

int tegra_hwpm_bind_resources(struct tegra_soc_hwpm *hwpm)
{
	int err = 0;

	tegra_hwpm_fn(hwpm, " ");

	err = tegra_hwpm_func_all_ip(hwpm, NULL, TEGRA_HWPM_BIND_RESOURCES);
	if (err != 0) {
		tegra_hwpm_err(hwpm, "failed to bind resources");
		return err;
	}

	return 0;
}

int tegra_hwpm_release_resources(struct tegra_soc_hwpm *hwpm)
{
	int ret = 0;

	tegra_hwpm_fn(hwpm, " ");

	ret = tegra_hwpm_func_all_ip(hwpm, NULL, TEGRA_HWPM_RELEASE_RESOURCES);
	if (ret != 0) {
		tegra_hwpm_err(hwpm, "failed to release resources");
		return ret;
	}

	return 0;
}
