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

#ifndef TEGRA_HWPM_LOG_H
#define TEGRA_HWPM_LOG_H

#include <linux/bits.h>

#define TEGRA_SOC_HWPM_MODULE_NAME	"tegra-soc-hwpm"

enum tegra_soc_hwpm_log_type {
	TEGRA_HWPM_ERROR,	/* Error prints */
	TEGRA_HWPM_DEBUG,	/* Debug prints */
};

#define TEGRA_HWPM_DEFAULT_DBG_MASK	(0)
#define hwpm_fn				BIT(0)
#define hwpm_info			BIT(1)
#define hwpm_register			BIT(2)
#define hwpm_verbose			BIT(3)

#define tegra_hwpm_err(hwpm, fmt, arg...)				\
	tegra_hwpm_err_impl(hwpm, __func__, __LINE__, fmt, ##arg)
#define tegra_hwpm_dbg(hwpm, dbg_mask, fmt, arg...)			\
	tegra_hwpm_dbg_impl(hwpm, dbg_mask, __func__, __LINE__, fmt, ##arg)
#define tegra_hwpm_fn(hwpm, fmt, arg...)				\
	tegra_hwpm_dbg_impl(hwpm, hwpm_fn, __func__, __LINE__, fmt, ##arg)

struct tegra_soc_hwpm;

void tegra_hwpm_err_impl(struct tegra_soc_hwpm *hwpm,
	const char *func, int line, const char *fmt, ...);
void tegra_hwpm_dbg_impl(struct tegra_soc_hwpm *hwpm,
	u32 dbg_mask, const char *func, int line, const char *fmt, ...);

#endif /* TEGRA_HWPM_LOG_H */
