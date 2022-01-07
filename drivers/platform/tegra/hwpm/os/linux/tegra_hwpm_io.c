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
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/io.h>

#include <uapi/linux/tegra-soc-hwpm-uapi.h>

#include <tegra_hwpm_log.h>
#include <tegra_hwpm_io.h>
#include <tegra_hwpm.h>

static u32 fake_readl(struct tegra_soc_hwpm *hwpm,
	struct hwpm_ip_aperture *aperture, u64 offset)
{
	u32 reg_val = 0;

	if (!hwpm->fake_registers_enabled) {
		tegra_hwpm_err(hwpm, "Fake registers are disabled!");
		return 0;
	}

	reg_val = aperture->fake_registers[offset];
	return reg_val;
}

static void fake_writel(struct tegra_soc_hwpm *hwpm,
	struct hwpm_ip_aperture *aperture, u64 offset, u32 val)
{
	if (!hwpm->fake_registers_enabled) {
		tegra_hwpm_err(hwpm, "Fake registers are disabled!");
		return;
	}

	aperture->fake_registers[offset] = val;
}

/*
 * Read IP domain registers
 * IP(except PMA and RTR) perfmux fall in this category
 */
static u32 ip_readl(struct tegra_soc_hwpm *hwpm,
	struct hwpm_ip_aperture *aperture, u64 offset)
{
	tegra_hwpm_dbg(hwpm, hwpm_register,
		"Aperture (0x%llx-0x%llx) offset(0x%x)",
		aperture->start_abs_pa, aperture->end_abs_pa, offset);

	if (hwpm->fake_registers_enabled) {
		return fake_readl(hwpm, aperture, offset);
	} else {
		u32 reg_val = 0U;
		struct tegra_soc_hwpm_ip_ops *ip_ops_ptr = &aperture->ip_ops;
		if (ip_ops_ptr->hwpm_ip_reg_op != NULL) {
			int err = 0;

			err = (*ip_ops_ptr->hwpm_ip_reg_op)(ip_ops_ptr->ip_dev,
				TEGRA_SOC_HWPM_IP_REG_OP_READ,
				offset, &reg_val);
			if (err < 0) {
				tegra_hwpm_err(hwpm, "Aperture (0x%llx-0x%llx) "
					"read offset(0x%llx) failed",
					aperture->start_abs_pa,
					aperture->end_abs_pa, offset);
				return 0U;
			}
		} else {
			/* Fall back to un-registered IP method */
			void __iomem *ptr = NULL;

			ptr = ioremap(aperture->start_abs_pa + offset, 0x4);
			if (!ptr) {
				tegra_hwpm_err(hwpm,
					"Failed to map register(0x%llx)",
					aperture->start_abs_pa + offset);
				return 0U;
			}
			reg_val = __raw_readl(ptr);
			iounmap(ptr);
		}
		return reg_val;
	}
}

/*
 * Write to IP domain registers
 * IP(except PMA and RTR) perfmux fall in this category
 */
static void ip_writel(struct tegra_soc_hwpm *hwpm,
	struct hwpm_ip_aperture *aperture, u64 offset, u32 val)
{
	tegra_hwpm_dbg(hwpm, hwpm_register,
		"Aperture (0x%llx-0x%llx) offset(0x%llx) val(0x%x)",
		aperture->start_abs_pa, aperture->end_abs_pa, offset, val);

	if (hwpm->fake_registers_enabled) {
		fake_writel(hwpm, aperture, offset, val);
	} else {
		struct tegra_soc_hwpm_ip_ops *ip_ops_ptr = &aperture->ip_ops;
		if (ip_ops_ptr->hwpm_ip_reg_op != NULL) {
			int err = 0;

			err = (*ip_ops_ptr->hwpm_ip_reg_op)(ip_ops_ptr->ip_dev,
				TEGRA_SOC_HWPM_IP_REG_OP_WRITE,
				offset, &val);
			if (err < 0) {
				tegra_hwpm_err(hwpm, "Aperture (0x%llx-0x%llx) "
					"write offset(0x%llx) val 0x%x failed",
					aperture->start_abs_pa,
					aperture->end_abs_pa, offset, val);
				return;
			}
		} else {
			/* Fall back to un-registered IP method */
			void __iomem *ptr = NULL;

			ptr = ioremap(aperture->start_abs_pa + offset, 0x4);
			if (!ptr) {
				tegra_hwpm_err(hwpm,
					"Failed to map register(0x%llx)",
					aperture->start_abs_pa + offset);
				return;
			}
			__raw_writel(val, ptr);
			iounmap(ptr);
		}
	}
}

/*
 * Read HWPM domain registers
 * PERFMONs, PMA and RTR registers fall in this category
 */
static u32 hwpm_readl(struct tegra_soc_hwpm *hwpm,
	struct hwpm_ip_aperture *aperture, u32 offset)
{
	tegra_hwpm_dbg(hwpm, hwpm_register,
		"Aperture (0x%llx-0x%llx) base 0x%llx offset(0x%x)",
		aperture->start_abs_pa, aperture->end_abs_pa,
		(u64 *)aperture->dt_mmio, offset);

	if (aperture->dt_mmio == NULL) {
		tegra_hwpm_err(hwpm, "aperture is not iomapped as expected");
		return 0U;
	}

	if (hwpm->fake_registers_enabled) {
		return fake_readl(hwpm, aperture, offset);
	} else {
		return readl(aperture->dt_mmio + offset);
	}
}

/*
 * Write to HWPM domain registers
 * PERFMONs, PMA and RTR registers fall in this category
 */
static void hwpm_writel(struct tegra_soc_hwpm *hwpm,
	struct hwpm_ip_aperture *aperture, u32 offset, u32 val)
{
	tegra_hwpm_dbg(hwpm, hwpm_register,
		"Aperture (0x%llx-0x%llx) base 0x%llx offset(0x%x) val(0x%x)",
		aperture->start_abs_pa, aperture->end_abs_pa,
		(u64 *)aperture->dt_mmio, offset, val);

	if (aperture->dt_mmio == NULL) {
		tegra_hwpm_err(hwpm, "aperture is not iomapped as expected");
		return;
	}

	if (hwpm->fake_registers_enabled) {
		fake_writel(hwpm, aperture, offset, val);
	} else {
		writel(val, aperture->dt_mmio + offset);
	}
}

/*
 * Read a HWPM domain register. It is assumed that valid aperture
 * is passed to the function.
 */
u32 tegra_hwpm_readl(struct tegra_soc_hwpm *hwpm,
	struct hwpm_ip_aperture *aperture, u64 addr)
{
	u32 reg_val = 0;

	if (!aperture) {
		tegra_hwpm_err(hwpm, "aperture is NULL");
		return -EINVAL;
	}

	if (aperture->is_hwpm_element) {
		/* HWPM domain registers */
		reg_val = hwpm_readl(hwpm, aperture, addr - aperture->base_pa);
	} else {
		tegra_hwpm_err(hwpm, "IP aperture read is not expected");
		return -EINVAL;
	}
	return reg_val;
}

/*
 * Write to a HWPM domain register. It is assumed that valid aperture
 * is passed to the function.
 */
void tegra_hwpm_writel(struct tegra_soc_hwpm *hwpm,
	struct hwpm_ip_aperture *aperture, u64 addr, u32 val)
{
	if (!aperture) {
		tegra_hwpm_err(hwpm, "aperture is NULL");
		return;
	}

	if (aperture->is_hwpm_element) {
		/* HWPM domain internal registers */
		hwpm_writel(hwpm, aperture, addr - aperture->base_pa, val);
	} else {
		tegra_hwpm_err(hwpm, "IP aperture write is not expected");
		return;
	}
}

/*
 * Read a register from the EXEC_REG_OPS IOCTL. It is assumed that the allowlist
 * check has been done before calling this function.
 */
u32 regops_readl(struct tegra_soc_hwpm *hwpm,
	struct hwpm_ip_aperture *aperture, u64 addr)
{
	u32 reg_val = 0;

	if (!aperture) {
		tegra_hwpm_err(hwpm, "aperture is NULL");
		return 0;
	}

	if (aperture->is_hwpm_element) {
		/* HWPM unit internal registers */
		reg_val = hwpm_readl(hwpm, aperture,
			addr - aperture->start_abs_pa);
	} else {
		reg_val = ip_readl(hwpm, aperture,
			addr - aperture->start_abs_pa);
	}
	return reg_val;
}

/*
 * Write a register from the EXEC_REG_OPS IOCTL. It is assumed that the
 * allowlist check has been done before calling this function.
 */
void regops_writel(struct tegra_soc_hwpm *hwpm,
	struct hwpm_ip_aperture *aperture, u64 addr, u32 val)
{
	if (!aperture) {
		tegra_hwpm_err(hwpm, "aperture is NULL");
		return;
	}

	if (aperture->is_hwpm_element) {
		/* HWPM unit internal registers */
		hwpm_writel(hwpm, aperture, addr - aperture->start_abs_pa, val);
	} else {
		ip_writel(hwpm, aperture, addr - aperture->start_abs_pa, val);
	}
}
