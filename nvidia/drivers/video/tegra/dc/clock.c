/*
 * clock.c: Functions required for internal dc clock utility.
 *
 * Copyright (C) 2010 Google, Inc.
 *
 * Copyright (c) 2010-2017, NVIDIA CORPORATION, All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/err.h>
#include <linux/types.h>
#include <linux/clk.h>
#include <linux/clk/tegra.h>

#include "dc.h"
#include "dc_reg.h"
#include "dc_priv.h"

unsigned long tegra_dc_pclk_round_rate(struct tegra_dc *dc, int pclk)
{
	unsigned long rate;
	unsigned long div;

	rate = tegra_dc_clk_get_rate(dc);

	if (TEGRA_DC_OUT_DSI == dc->out->type ||
		TEGRA_DC_OUT_FAKE_DSIA == dc->out->type ||
		TEGRA_DC_OUT_FAKE_DSIB == dc->out->type ||
		TEGRA_DC_OUT_FAKE_DSI_GANGED == dc->out->type) {
		div = DIV_ROUND_CLOSEST(rate * 2, pclk);
		if (tegra_dc_is_nvdisplay())
			return rate;	/*shift_clk_div is not available*/
	} else { /* round-up for divider for other display types */
		div = DIV_ROUND_UP(rate * 2, pclk);
	}

	if (tegra_dc_is_t21x()) {
		if (dc->out->type == TEGRA_DC_OUT_HDMI)
			return rate;
	}

	if (div < 2)
		return 0;

	return rate * 2 / div;
}

void tegra_dc_setup_clk(struct tegra_dc *dc, struct clk *clk)
{
	int pclk;

	if (dc->out_ops->setup_clk)
		pclk = dc->out_ops->setup_clk(dc, clk);
	else
		pclk = 0;
	if (tegra_dc_is_nvdisplay())
		tegra_nvdisp_set_compclk(dc);

	WARN_ONCE(!pclk, "pclk is 0\n");
#ifdef CONFIG_TEGRA_CORE_DVFS
	tegra_dvfs_set_rate(clk, pclk);
#endif
}
