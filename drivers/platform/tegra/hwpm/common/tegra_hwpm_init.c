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

#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/reset.h>
#include <linux/clk.h>
#include <linux/dma-buf.h>
#include <soc/tegra/fuse.h>
#include <uapi/linux/tegra-soc-hwpm-uapi.h>

#include <tegra_hwpm_log.h>
#include <tegra_hwpm_io.h>
#include <tegra_hwpm.h>
#include <tegra_hwpm_common.h>
#include <hal/t234/t234_hwpm_init.h>

int tegra_soc_hwpm_init_chip_info(struct tegra_soc_hwpm *hwpm)
{
	int err = -EINVAL;

	tegra_hwpm_fn(hwpm, " ");

	hwpm->device_info.chip = tegra_get_chip_id();
	hwpm->device_info.chip_revision = tegra_get_major_rev();
	hwpm->device_info.revision = tegra_chip_get_revision();
	hwpm->device_info.platform = tegra_get_platform();

	hwpm->dbg_mask = TEGRA_HWPM_DEFAULT_DBG_MASK;

	switch (hwpm->device_info.chip) {
	case 0x23:
		switch (hwpm->device_info.chip_revision) {
		case 0x4:
			err = t234_hwpm_init_chip_info(hwpm);
			break;
		default:
			tegra_hwpm_err(hwpm, "Chip 0x%x rev 0x%x not supported",
			hwpm->device_info.chip,
			hwpm->device_info.chip_revision);
			break;
		}
		break;
	default:
		tegra_hwpm_err(hwpm, "Chip 0x%x not supported",
			hwpm->device_info.chip);
		break;
	}

	if (err != 0) {
		tegra_hwpm_err(hwpm, "init_chip_info failed");
	}

	return err;
}

int tegra_soc_hwpm_setup_sw(struct tegra_soc_hwpm *hwpm)
{
	int ret = 0;

	tegra_hwpm_fn(hwpm, " ");

	if (hwpm->active_chip->init_fs_info == NULL) {
		tegra_hwpm_err(hwpm, "init_fs_info uninitialized");
		goto enodev;
	}
	ret = hwpm->active_chip->init_fs_info(hwpm);
	if (ret < 0) {
		tegra_hwpm_err(hwpm, "Unable to initialize chip fs_info");
		goto fail;
	}

	/* Initialize SW state */
	hwpm->bind_completed = false;
	hwpm->full_alist_size = 0;

	return 0;
enodev:
	ret = -ENODEV;
fail:
	return ret;
}

int tegra_soc_hwpm_setup_hw(struct tegra_soc_hwpm *hwpm)
{
	int ret = 0;

	tegra_hwpm_fn(hwpm, " ");

	/*
	 * Map PMA and RTR apertures
	 * PMA and RTR are hwpm apertures which include hwpm config registers.
	 * Map/reserve these apertures to get MMIO address required for hwpm
	 * configuration (following steps).
	 */
	if (hwpm->active_chip->reserve_pma == NULL) {
		tegra_hwpm_err(hwpm, "reserve_pma uninitialized");
		goto enodev;
	}
	ret = hwpm->active_chip->reserve_pma(hwpm);
	if (ret < 0) {
		tegra_hwpm_err(hwpm, "Unable to reserve PMA aperture");
		goto fail;
	}

	if (hwpm->active_chip->reserve_rtr == NULL) {
		tegra_hwpm_err(hwpm, "reserve_rtr uninitialized");
		goto enodev;
	}
	ret = hwpm->active_chip->reserve_rtr(hwpm);
	if (ret < 0) {
		tegra_hwpm_err(hwpm, "Unable to reserve RTR aperture");
		goto fail;
	}

	/* Disable SLCG */
	if (hwpm->active_chip->disable_slcg == NULL) {
		tegra_hwpm_err(hwpm, "disable_slcg uninitialized");
		goto enodev;
	}
	ret = hwpm->active_chip->disable_slcg(hwpm);
	if (ret < 0) {
		tegra_hwpm_err(hwpm, "Unable to disable SLCG");
		goto fail;
	}

	/* Program PROD values */
	if (hwpm->active_chip->init_prod_values == NULL) {
		tegra_hwpm_err(hwpm, "init_prod_values uninitialized");
		goto enodev;
	}
	ret = hwpm->active_chip->init_prod_values(hwpm);
	if (ret < 0) {
		tegra_hwpm_err(hwpm, "Unable to set PROD values");
		goto fail;
	}

	return 0;
enodev:
	ret = -ENODEV;
fail:
	return ret;
}

int tegra_hwpm_disable_triggers(struct tegra_soc_hwpm *hwpm)
{
	tegra_hwpm_fn(hwpm, " ");

	if (hwpm->active_chip->disable_triggers == NULL) {
		tegra_hwpm_err(hwpm, "disable_triggers uninitialized");
		return -ENODEV;
	}
	return hwpm->active_chip->disable_triggers(hwpm);
}

int tegra_soc_hwpm_release_hw(struct tegra_soc_hwpm *hwpm)
{
	int ret = 0;

	tegra_hwpm_fn(hwpm, " ");

	/* Enable SLCG */
	if (hwpm->active_chip->enable_slcg == NULL) {
		tegra_hwpm_err(hwpm, "enable_slcg uninitialized");
		goto enodev;
	}
	ret = hwpm->active_chip->enable_slcg(hwpm);
	if (ret < 0) {
		tegra_hwpm_err(hwpm, "Unable to enable SLCG");
		goto fail;
	}

	/*
	 * Unmap PMA and RTR apertures
	 * Since, PMA and RTR hwpm apertures consist of hwpm config registers,
	 * these aperture mappings are required to reset hwpm config.
	 * Hence, explicitly unmap/release these apertures as a last step.
	 */
	if (hwpm->active_chip->release_rtr == NULL) {
		tegra_hwpm_err(hwpm, "release_rtr uninitialized");
		goto enodev;
	}
	ret = hwpm->active_chip->release_rtr(hwpm);
	if (ret < 0) {
		tegra_hwpm_err(hwpm, "Unable to release RTR aperture");
		goto fail;
	}

	if (hwpm->active_chip->release_pma == NULL) {
		tegra_hwpm_err(hwpm, "release_pma uninitialized");
		goto enodev;
	}
	ret = hwpm->active_chip->release_pma(hwpm);
	if (ret < 0) {
		tegra_hwpm_err(hwpm, "Unable to release PMA aperture");
		goto fail;
	}

	return 0;
enodev:
	ret = -ENODEV;
fail:
	return ret;
}

void tegra_soc_hwpm_release_sw_components(struct tegra_soc_hwpm *hwpm)
{
	tegra_hwpm_fn(hwpm, " ");

	if (hwpm->active_chip->release_sw_setup == NULL) {
		tegra_hwpm_err(hwpm, "release_sw_setup uninitialized");
	} else {
		hwpm->active_chip->release_sw_setup(hwpm);
	}

	kfree(hwpm->active_chip->chip_ips);
	kfree(hwpm);
	tegra_soc_hwpm_pdev = NULL;
}
