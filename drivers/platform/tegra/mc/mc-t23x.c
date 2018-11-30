//SPDX-License-Identifier: GPL-2.0

/*
 *
 * Copyright (c) 2019, NVIDIA Corporation. All rights reserved.
 *
 * Tegra 23x mc driver.
 */

#include <linux/mod_devicetable.h>

const struct of_device_id tegra_mc_of_ids[] = {
	{ .compatible = "nvidia,tegra-mc" },
	{ .compatible = "nvidia,tegra-t18x-mc" },
	{ .compatible = "nvidia,tegra-t19x-mc" },
	{ .compatible = "nvidia,tegra-t23x-mc" },
	{ }
};
