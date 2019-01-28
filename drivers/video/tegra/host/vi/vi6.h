/*
 * drivers/video/tegra/host/vi/vi6.h
 *
 * Tegra VI6
 *
 * Copyright (c) 2019 NVIDIA Corporation.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __NVHOST_VI6_H__
#define __NVHOST_VI6_H__

#include <linux/platform_device.h>
#include <media/mc_common.h>

int nvhost_vi6_aggregate_constraints(struct platform_device *dev,
				int clk_index,
				unsigned long floor_rate,
				unsigned long pixelrate,
				unsigned long bw_constraint);

int vi6_priv_early_probe(struct platform_device *pdev);
int vi6_priv_late_probe(struct platform_device *pdev);

#endif
