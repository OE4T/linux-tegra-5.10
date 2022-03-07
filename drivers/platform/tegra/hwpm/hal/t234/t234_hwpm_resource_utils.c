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
#include <linux/bitops.h>
#include <linux/of_address.h>
#include <uapi/linux/tegra-soc-hwpm-uapi.h>

#include <tegra_hwpm_log.h>
#include <tegra_hwpm_io.h>
#include <tegra_hwpm.h>

#include <hal/t234/t234_hwpm_internal.h>

#include <hal/t234/hw/t234_pmasys_soc_hwpm.h>
#include <hal/t234/hw/t234_pmmsys_soc_hwpm.h>

int t234_hwpm_perfmon_enable(struct tegra_soc_hwpm *hwpm,
	hwpm_ip_perfmon *perfmon)
{
	u32 reg_val;

	tegra_hwpm_fn(hwpm, " ");

	/* Enable */
	tegra_hwpm_dbg(hwpm, hwpm_verbose, "Enabling PERFMON(0x%llx - 0x%llx)",
		perfmon->start_abs_pa, perfmon->end_abs_pa);

	reg_val = tegra_hwpm_readl(hwpm, perfmon,
		pmmsys_sys0_enginestatus_r(0));
	reg_val = set_field(reg_val, pmmsys_sys0_enginestatus_enable_m(),
		pmmsys_sys0_enginestatus_enable_out_f());
	tegra_hwpm_writel(hwpm, perfmon,
		pmmsys_sys0_enginestatus_r(0), reg_val);

	return 0;
}

int t234_hwpm_perfmux_disable(struct tegra_soc_hwpm *hwpm,
	hwpm_ip_perfmux *perfmux)
{
	int err = 0;

	tegra_hwpm_fn(hwpm, " ");

	/*
	 * Indicate that HWPM monitoring is disabled/closed.
	 * Since perfmux is controlled by IP, indicate monitoring disabled
	 * by enabling IP power management.
	 */
	/* Make sure that ip_ops are initialized */
	if ((perfmux->ip_ops.ip_dev != NULL) &&
		(perfmux->ip_ops.hwpm_ip_pm != NULL)) {
		err = (*perfmux->ip_ops.hwpm_ip_pm)(
			perfmux->ip_ops.ip_dev, false);
		if (err != 0) {
			tegra_hwpm_err(hwpm, "Runtime PM enable failed");
		}
	} else {
		tegra_hwpm_dbg(hwpm, hwpm_verbose, "Runtime PM not configured");
	}

	return 0;
}

int t234_hwpm_perfmon_disable(struct tegra_soc_hwpm *hwpm,
	hwpm_ip_perfmon *perfmon)
{
	u32 reg_val;

	tegra_hwpm_fn(hwpm, " ");

	/* Disable */
	tegra_hwpm_dbg(hwpm, hwpm_verbose, "Disabling PERFMON(0x%llx - 0x%llx)",
		perfmon->start_abs_pa, perfmon->end_abs_pa);

	reg_val = tegra_hwpm_readl(hwpm, perfmon, pmmsys_control_r(0));
	reg_val = set_field(reg_val, pmmsys_control_mode_m(),
		pmmsys_control_mode_disable_f());
	tegra_hwpm_writel(hwpm, perfmon, pmmsys_control_r(0), reg_val);

	return 0;
}
