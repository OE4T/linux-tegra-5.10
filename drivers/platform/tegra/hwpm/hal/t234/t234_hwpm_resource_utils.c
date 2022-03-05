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

#include <tegra_hwpm_static_analysis.h>
#include <tegra_hwpm_log.h>
#include <tegra_hwpm_io.h>
#include <tegra_hwpm.h>

#include <hal/t234/t234_hwpm_internal.h>

#include <hal/t234/hw/t234_pmasys_soc_hwpm.h>
#include <hal/t234/hw/t234_pmmsys_soc_hwpm.h>

static int t234_hwpm_perfmon_enable(struct tegra_soc_hwpm *hwpm,
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

static int t234_hwpm_perfmux_reserve(struct tegra_soc_hwpm *hwpm,
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
		u32 **fake_regs = &perfmux->fake_registers;

		*fake_regs = (u32 *)kzalloc(sizeof(u32) * num_regs, GFP_KERNEL);
		if (!(*fake_regs)) {
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

static int t234_hwpm_perfmux_disable(struct tegra_soc_hwpm *hwpm,
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

static int t234_hwpm_perfmux_release(struct tegra_soc_hwpm *hwpm,
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

int t234_hwpm_perfmon_reserve(struct tegra_soc_hwpm *hwpm,
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

static int t234_hwpm_perfmon_disable(struct tegra_soc_hwpm *hwpm,
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

int t234_hwpm_perfmon_release(struct tegra_soc_hwpm *hwpm,
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

int t234_hwpm_release_all_resources(struct tegra_soc_hwpm *hwpm)
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

	for (ip_idx = 0U; ip_idx < T234_HWPM_IP_MAX; ip_idx++) {
		chip_ip = active_chip->chip_ips[ip_idx];

		/* PMA and RTR will be released later */
		if ((ip_idx == T234_HWPM_IP_PMA) ||
			(ip_idx == T234_HWPM_IP_RTR)) {
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

				err = t234_hwpm_perfmon_disable(hwpm, perfmon);
				if (err != 0) {
					tegra_hwpm_err(hwpm, "IP %d"
						" perfmon %d disable failed",
						ip_idx, perfmon_idx);
				}

				err = t234_hwpm_perfmon_release(hwpm, perfmon);
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

				err = t234_hwpm_perfmux_disable(hwpm, perfmux);
				if (err != 0) {
					tegra_hwpm_err(hwpm, "IP %d"
						" perfmux %d disable failed",
						ip_idx, perfmux_idx);
				}

				err = t234_hwpm_perfmux_release(hwpm, perfmux);
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

/* ip_idx is wrt enum t234_hwpm_active_ips */
int t234_hwpm_reserve_given_resource(struct tegra_soc_hwpm *hwpm, u32 ip_idx)
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
	if ((ip_idx == T234_HWPM_IP_PMA) || (ip_idx == T234_HWPM_IP_RTR)) {
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

			err = t234_hwpm_perfmon_reserve(hwpm, perfmon);
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

			err = t234_hwpm_perfmux_reserve(hwpm, perfmux);
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

			ret = t234_hwpm_perfmon_disable(hwpm, perfmon);
			if (ret != 0) {
				tegra_hwpm_err(hwpm,
					"IP %d perfmon %d disable failed",
					ip_idx, perfmon_idx);
			}

			ret = t234_hwpm_perfmon_release(hwpm, perfmon);
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

			ret = t234_hwpm_perfmux_disable(hwpm, perfmux);
			if (ret != 0) {
				tegra_hwpm_err(hwpm,
					"IP %d perfmux %d disable failed",
					ip_idx, perfmux_idx);
			}

			ret = t234_hwpm_perfmux_release(hwpm, perfmux);
			if (ret != 0) {
				tegra_hwpm_err(hwpm,
					"IP %d perfmux %d release failed",
					ip_idx, perfmux_idx);
			}
		}
	}
	return err;
}

int t234_hwpm_bind_reserved_resources(struct tegra_soc_hwpm *hwpm)
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

				err = active_chip->zero_alist_regs(
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

				err = active_chip->zero_alist_regs(
					hwpm, perfmon);
				if (err != 0) {
					tegra_hwpm_err(hwpm, "IP %d"
						" perfmon %d zero regs failed",
						ip_idx, perfmon_idx);
				}

				err = t234_hwpm_perfmon_enable(hwpm, perfmon);
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
