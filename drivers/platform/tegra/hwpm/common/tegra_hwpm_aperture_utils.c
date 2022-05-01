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
#include <linux/of_address.h>

#include <tegra_hwpm_static_analysis.h>
#include <tegra_hwpm_log.h>
#include <tegra_hwpm_io.h>
#include <tegra_hwpm.h>

int tegra_hwpm_perfmon_reserve(struct tegra_soc_hwpm *hwpm,
	hwpm_ip_perfmon *perfmon)
{
	struct resource *res = NULL;

	tegra_hwpm_fn(hwpm, " ");

	/* Reserve */
	res = platform_get_resource_byname(hwpm->pdev,
					IORESOURCE_MEM, perfmon->name);
	if ((!res) || (res->start == 0) || (res->end == 0)) {
		tegra_hwpm_err(hwpm, "Failed to get perfmon %s", perfmon->name);
		return -ENOMEM;
	}

	perfmon->dt_mmio = devm_ioremap(hwpm->dev, res->start,
					resource_size(res));
	if (IS_ERR(perfmon->dt_mmio)) {
		tegra_hwpm_err(hwpm, "Couldn't map perfmon %s", perfmon->name);
		return PTR_ERR(perfmon->dt_mmio);
	}

	perfmon->start_pa = res->start;
	perfmon->end_pa = res->end;

	if (hwpm->fake_registers_enabled) {
		u64 address_range = tegra_hwpm_safe_add_u64(
			tegra_hwpm_safe_sub_u64(res->end, res->start), 1ULL);
		u64 num_regs = address_range / sizeof(u32);
		perfmon->fake_registers = (u32 *)kzalloc(sizeof(u32) * num_regs,
						GFP_KERNEL);
		if (perfmon->fake_registers == NULL) {
			tegra_hwpm_err(hwpm, "Perfmon (0x%llx - 0x%llx) "
				"Couldn't allocate memory for fake regs",
				perfmon->start_abs_pa, perfmon->end_abs_pa);
			return -ENOMEM;
		}
	}
	return 0;
}

int tegra_hwpm_perfmon_release(struct tegra_soc_hwpm *hwpm,
	hwpm_ip_perfmon *perfmon)
{
	tegra_hwpm_fn(hwpm, " ");

	if (perfmon->dt_mmio == NULL) {
		tegra_hwpm_err(hwpm, "Perfmon was not mapped");
		return -EINVAL;
	}
	devm_iounmap(hwpm->dev, perfmon->dt_mmio);
	perfmon->dt_mmio = NULL;
	perfmon->start_pa = 0ULL;
	perfmon->end_pa = 0ULL;

	if (perfmon->fake_registers) {
		kfree(perfmon->fake_registers);
		perfmon->fake_registers = NULL;
	}
	return 0;
}

int tegra_hwpm_perfmux_reserve(struct tegra_soc_hwpm *hwpm,
	hwpm_ip_perfmux *perfmux)
{
	int err = 0;
	int ret = 0;

	tegra_hwpm_fn(hwpm, " ");

	/*
	 * Indicate that HWPM driver is initializing monitoring.
	 * Since perfmux is controlled by IP, indicate monitoring enabled
	 * by disabling IP power management.
	 */
	/* Make sure that ip_ops are initialized */
	if ((perfmux->ip_ops.ip_dev != NULL) &&
		(perfmux->ip_ops.hwpm_ip_pm != NULL)) {
		err = (*perfmux->ip_ops.hwpm_ip_pm)(
			perfmux->ip_ops.ip_dev, true);
		if (err != 0) {
			tegra_hwpm_err(hwpm, "Runtime PM disable failed");
		}
	} else {
		tegra_hwpm_dbg(hwpm, hwpm_verbose, "Runtime PM not configured");
	}

	perfmux->start_pa = perfmux->start_abs_pa;
	perfmux->end_pa = perfmux->end_abs_pa;

	/* Allocate fake registers */
	if (hwpm->fake_registers_enabled) {
		u64 address_range = tegra_hwpm_safe_add_u64(
			tegra_hwpm_safe_sub_u64(
				perfmux->end_pa, perfmux->start_pa), 1ULL);
		u64 num_regs = address_range / sizeof(u32);
		perfmux->fake_registers = (u32 *)kzalloc(
			sizeof(u32) * num_regs, GFP_KERNEL);
		if (perfmux->fake_registers == NULL) {
			tegra_hwpm_err(hwpm, "Aperture(0x%llx - 0x%llx):"
				" Couldn't allocate memory for fake registers",
				perfmux->start_pa, perfmux->end_pa);
			ret = -ENOMEM;
			goto fail;
		}
	}

fail:
	return ret;
}

int tegra_hwpm_perfmux_release(struct tegra_soc_hwpm *hwpm,
	hwpm_ip_perfmux *perfmux)
{
	tegra_hwpm_fn(hwpm, " ");

	/*
	 * Release
	 * This is only required for for fake registers
	 */
	if (perfmux->fake_registers) {
		kfree(perfmux->fake_registers);
		perfmux->fake_registers = NULL;
	}

	return 0;
}

int tegra_hwpm_reserve_rtr(struct tegra_soc_hwpm *hwpm)
{
	int ret = 0;
	u32 perfmux_idx = 0U;
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	struct hwpm_ip *chip_ip_rtr = NULL;
	hwpm_ip_perfmux *rtr_perfmux = NULL;

	tegra_hwpm_fn(hwpm, " ");

	chip_ip_rtr = active_chip->chip_ips[active_chip->get_rtr_int_idx(hwpm)];

	/* Make sure that RTR is not already reserved */
	if (chip_ip_rtr->reserved == true) {
		tegra_hwpm_err(hwpm, "RTR already reserved, ignoring");
		return 0;
	}

	/* Reserve RTR perfmuxes */
	for (perfmux_idx = 0U; perfmux_idx < chip_ip_rtr->num_perfmux_slots;
		perfmux_idx++) {
		rtr_perfmux = chip_ip_rtr->ip_perfmux[perfmux_idx];

		if (rtr_perfmux == NULL) {
			continue;
		}

		/* Since RTR is hwpm component, use perfmon reserve function */
		ret = tegra_hwpm_perfmon_reserve(hwpm, rtr_perfmux);
		if (ret != 0) {
			tegra_hwpm_err(hwpm,
				"RTR perfmux %d reserve failed",
				perfmux_idx);
			return ret;
		}

		chip_ip_rtr->fs_mask |= rtr_perfmux->hw_inst_mask;
	}

	/* No perfmons in RTR */

	chip_ip_rtr->reserved = true;

	return ret;
}

int tegra_hwpm_release_rtr(struct tegra_soc_hwpm *hwpm)
{
	int ret = 0;
	u32 perfmux_idx;
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	struct hwpm_ip *chip_ip_rtr = NULL;
	hwpm_ip_perfmux *rtr_perfmux = NULL;

	tegra_hwpm_fn(hwpm, " ");

	chip_ip_rtr = active_chip->chip_ips[active_chip->get_rtr_int_idx(hwpm)];

	if (!chip_ip_rtr->reserved) {
		tegra_hwpm_dbg(hwpm, hwpm_info, "RTR wasn't mapped, ignoring.");
		return 0;
	}

	/* Release RTR perfmux */
	for (perfmux_idx = 0U; perfmux_idx < chip_ip_rtr->num_perfmux_slots;
		perfmux_idx++) {
		rtr_perfmux = chip_ip_rtr->ip_perfmux[perfmux_idx];

		if (rtr_perfmux == NULL) {
			continue;
		}

		/* RTR is hwpm component, use perfmon release func */
		ret = tegra_hwpm_perfmon_release(hwpm, rtr_perfmux);
		if (ret != 0) {
			tegra_hwpm_err(hwpm, "RTR perfmux %d release failed",
				perfmux_idx);
			return ret;
		}

		chip_ip_rtr->fs_mask &= ~(rtr_perfmux->hw_inst_mask);
	}

	/* No perfmons in RTR */

	chip_ip_rtr->reserved = false;
	return 0;
}