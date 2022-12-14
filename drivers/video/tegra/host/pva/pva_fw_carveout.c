/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * PVA carveout handling
 *
 * Copyright (c) 2022, NVIDIA Corporation.  All rights reserved.
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

#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include <linux/iommu.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/version.h>
#include <linux/of_reserved_mem.h>
#include "pva.h"
#include "pva_fw_carveout.h"

struct nvpva_carveout_info *pva_fw_co_get_info(struct pva *pva)
{
	struct device_node *np;
	const char *status = NULL;
	u32 reg[4] = {0};

	np = of_find_compatible_node(NULL, NULL, "nvidia,pva-carveout");
	if (np == NULL) {
		dev_err(&pva->pdev->dev, "find node failed\n");
		goto err_out;
	}

	if (of_property_read_string(np, "status", &status)) {
		dev_err(&pva->pdev->dev, "read status failed\n");
		goto err_out;
	}

	if (strcmp(status, "okay")) {
		dev_err(&pva->pdev->dev, "status %s  compare failed\n", status);
		goto err_out;
	}

	if (of_property_read_u32_array(np, "reg", reg, 4)) {
		dev_err(&pva->pdev->dev, "reaf_32_array failed\n");
		goto err_out;
	}

	pva->fw_carveout.base = ((u64)reg[0] << 32 | (u64)reg[1]);
	pva->fw_carveout.size = ((u64)reg[2] << 32 | (u64)reg[3]);
	pva->fw_carveout.base_va = 0;
	pva->fw_carveout.base_pa = 0;
	pva->fw_carveout.initialized = true;

	nvpva_dbg_fn(pva, "get co success\n");

	return &pva->fw_carveout;
err_out:
	dev_err(&pva->pdev->dev, "get co fail\n");
	pva->fw_carveout.initialized = false;

	return NULL;
}

bool pva_fw_co_initialized(struct pva *pva)
{
	return pva->fw_carveout.initialized;
}
