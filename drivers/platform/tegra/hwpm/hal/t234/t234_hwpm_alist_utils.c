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

#include <linux/bitops.h>
#include <uapi/linux/tegra-soc-hwpm-uapi.h>

#include <tegra_hwpm_log.h>
#include <tegra_hwpm_io.h>
#include <tegra_hwpm.h>
#include <hal/t234/t234_hwpm_internal.h>
#include <hal/t234/t234_hwpm_regops_allowlist.h>

size_t t234_hwpm_get_alist_buf_size(struct tegra_soc_hwpm *hwpm)
{
	return sizeof(struct allowlist);
}

int t234_hwpm_zero_alist_regs(struct tegra_soc_hwpm *hwpm,
	struct hwpm_ip_aperture *aperture)
{
	u32 alist_idx = 0U;

	tegra_hwpm_fn(hwpm, " ");

	for (alist_idx = 0; alist_idx < aperture->alist_size; alist_idx++) {
		if (aperture->alist[alist_idx].zero_at_init) {
			regops_writel(hwpm, aperture,
				aperture->start_abs_pa +
				aperture->alist[alist_idx].reg_offset, 0U);
		}
	}
	return 0;
}

int t234_hwpm_get_alist_size(struct tegra_soc_hwpm *hwpm)
{
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	u32 ip_idx;
	u32 perfmux_idx, perfmon_idx;
	unsigned long inst_idx = 0UL;
	unsigned long floorsweep_info = 0UL;
	struct hwpm_ip *chip_ip = NULL;
	hwpm_ip_perfmux *perfmux = NULL;
	hwpm_ip_perfmon *perfmon = NULL;

	tegra_hwpm_fn(hwpm, " ");

	for (ip_idx = 0U; ip_idx < T234_HWPM_IP_MAX; ip_idx++) {
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
			/* Add perfmux alist size to full alist size */
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

				if (perfmux->alist) {
					hwpm->full_alist_size +=
						perfmux->alist_size;
				} else {
					tegra_hwpm_err(hwpm, "IP %d"
						" perfmux %d NULL alist",
						ip_idx, perfmux_idx);
				}
			}

			/* Add perfmon alist size to full alist size */
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

				if (perfmon->alist) {
					hwpm->full_alist_size +=
						perfmon->alist_size;
				} else {
					tegra_hwpm_err(hwpm, "IP %d"
						" perfmon %d NULL alist",
						ip_idx, perfmon_idx);
				}
			}
		}
	}

	return 0;
}

static int t234_hwpm_copy_alist(struct tegra_soc_hwpm *hwpm,
	struct hwpm_ip_aperture *aperture, u64 *full_alist,
	u64 *full_alist_idx)
{
	u64 f_alist_idx = *full_alist_idx;
	u64 alist_idx = 0ULL;

	tegra_hwpm_fn(hwpm, " ");

	if (aperture->alist == NULL) {
		tegra_hwpm_err(hwpm, "NULL allowlist in aperture");
		return -EINVAL;
	}

	for (alist_idx = 0ULL; alist_idx < aperture->alist_size; alist_idx++) {
		if (f_alist_idx >= hwpm->full_alist_size) {
			tegra_hwpm_err(hwpm, "No space in full_alist");
			return -ENOMEM;
		}

		full_alist[f_alist_idx++] = (aperture->start_abs_pa +
			aperture->alist[alist_idx].reg_offset);
	}

	/* Store next available index */
	*full_alist_idx = f_alist_idx;

	return 0;
}

int t234_hwpm_combine_alist(struct tegra_soc_hwpm *hwpm, u64 *alist)
{
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	u32 ip_idx;
	u32 perfmux_idx, perfmon_idx;
	unsigned long inst_idx = 0UL;
	unsigned long floorsweep_info = 0UL;
	struct hwpm_ip *chip_ip = NULL;
	hwpm_ip_perfmux *perfmux = NULL;
	hwpm_ip_perfmon *perfmon = NULL;
	u64 full_alist_idx = 0;
	int err = 0;

	tegra_hwpm_fn(hwpm, " ");

	for (ip_idx = 0U; ip_idx < T234_HWPM_IP_MAX; ip_idx++) {
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
			/* Copy perfmux alist to full alist array */
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

				err = t234_hwpm_copy_alist(hwpm, perfmux,
					alist, &full_alist_idx);
				if (err != 0) {
					tegra_hwpm_err(hwpm, "IP %d"
						" perfmux %d alist copy failed",
						ip_idx, perfmux_idx);
					goto fail;
				}
			}

			/* Copy perfmon alist to full alist array */
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

				err = t234_hwpm_copy_alist(hwpm, perfmon,
					alist, &full_alist_idx);
				if (err != 0) {
					tegra_hwpm_err(hwpm, "IP %d"
						" perfmon %d alist copy failed",
						ip_idx, perfmon_idx);
					goto fail;
				}
			}
		}
	}

	/* Check size of full alist with hwpm->full_alist_size*/
	if (full_alist_idx != hwpm->full_alist_size) {
		tegra_hwpm_err(hwpm, "full_alist_size 0x%llx doesn't match "
			"max full_alist_idx 0x%llx",
			hwpm->full_alist_size, full_alist_idx);
		err = -EINVAL;
	}

fail:
	return err;
}

bool t234_hwpm_check_alist(struct tegra_soc_hwpm *hwpm,
	struct hwpm_ip_aperture *aperture, u64 phys_addr)
{
	u32 alist_idx;
	u64 reg_offset;

	tegra_hwpm_fn(hwpm, " ");

	if (!aperture) {
		tegra_hwpm_err(hwpm, "Aperture is NULL");
		return false;
	}
	if (!aperture->alist) {
		tegra_hwpm_err(hwpm, "NULL allowlist in aperture");
		return false;
	}

	reg_offset = phys_addr - aperture->start_abs_pa;

	for (alist_idx = 0; alist_idx < aperture->alist_size; alist_idx++) {
		if (reg_offset == aperture->alist[alist_idx].reg_offset) {
			return true;
		}
	}
	return false;
}
