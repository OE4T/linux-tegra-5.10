/*
 * Copyright (c) 2020, NVIDIA CORPORATION.  All rights reserved.
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

#include <linux/interconnect-provider.h>
#include "tegra_icc.h"

static int tegra23x_icc_aggregate(struct icc_node *node, u32 avg_bw,
			u32 peak_bw, u32 *agg_avg, u32 *agg_peak)
{
	return 0;
}

static int tegra23x_icc_set(struct icc_node *src, struct icc_node *dst)
{
	return 0;
}

const struct tegra_icc_ops tegra23x_icc_ops = {
	.plat_icc_set = tegra23x_icc_set,
	.plat_icc_aggregate = tegra23x_icc_aggregate,
};
