/*
 * eqos_ape_amisc.c
 *
 * AMISC register handling for eavb ape synchronization
 *
 * Copyright (C) 2016-2022, NVIDIA Corporation. All rights reserved.
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


#include <asm/io.h>
#include "eqos_ape_global.h"
#include <linux/module.h>

u32 amisc_readl(u32 reg)
{
	return readl(eqos_ape_drv_data->base_regs[AMISC_EAVB] +
			(reg - AMISC_APE_TSC_CTRL_0_0));
}

void amisc_writel(u32 val, u32 reg)
{
	writel(val, eqos_ape_drv_data->base_regs[AMISC_EAVB] +
			(reg - AMISC_APE_TSC_CTRL_0_0));
}

void amisc_idle_enable(void)
{
	writel((1 << 31) , eqos_ape_drv_data->base_regs[AMISC_IDLE]);
}

void amisc_idle_disable(void)
{
	writel(0 , eqos_ape_drv_data->base_regs[AMISC_IDLE]);
}

void amisc_clk_init(void)
{
	int ret = 0;
	int rate;
	struct platform_device *pdev = eqos_ape_drv_data->pdev;

	eqos_ape_drv_data->ape_clk = devm_clk_get(&pdev->dev, "eqos_ape.ape");
	if (IS_ERR_OR_NULL(eqos_ape_drv_data->ape_clk)) {
		dev_err(&pdev->dev, "Failed to find ape clk\n");
		ret = -EINVAL;
	}

	eqos_ape_drv_data->pll_a_out0_clk =
			devm_clk_get(&pdev->dev, "pll_a_out0");
	if (IS_ERR_OR_NULL(eqos_ape_drv_data->pll_a_out0_clk)) {
		dev_err(&pdev->dev, "Failed to find pll_a_out0_clk clk\n");
		ret = -EINVAL;
	}
	eqos_ape_drv_data->pll_a_clk = devm_clk_get(&pdev->dev, "pll_a");
	if (IS_ERR_OR_NULL(eqos_ape_drv_data->pll_a_clk)) {
		dev_err(&pdev->dev, "Failed to find pll_a clk\n");
		ret = -EINVAL;
	}

	eqos_ape_drv_data->pll_a_clk_rate =
			clk_get_rate(eqos_ape_drv_data->pll_a_clk);
	dev_dbg(&pdev->dev, "ape rate %d\n", eqos_ape_drv_data->pll_a_clk_rate);

	rate =  clk_get_rate(eqos_ape_drv_data->ape_clk);
	dev_dbg(&pdev->dev, "ape rate %d\n", rate);
}

void amisc_clk_deinit(void)
{
	struct platform_device *pdev = eqos_ape_drv_data->pdev;

	devm_clk_put(&pdev->dev, eqos_ape_drv_data->pll_a_clk);

	devm_clk_put(&pdev->dev, eqos_ape_drv_data->pll_a_out0_clk);

	devm_clk_put(&pdev->dev, eqos_ape_drv_data->ape_clk);
}

int amisc_ape_get_rate(void)
{
	int rate;

	rate = clk_get_rate(eqos_ape_drv_data->ape_clk);
	return rate;
}

void amisc_ape_set_rate(int rate)
{
	int err;

	err = clk_set_rate(eqos_ape_drv_data->ape_clk, rate);
}

int amisc_plla_get_rate(void)
{
	int rate;

	rate = clk_get_rate(eqos_ape_drv_data->pll_a_clk);
	return rate;
}

void amisc_plla_set_rate(int rate)
{
	int err;

	err = clk_set_rate(eqos_ape_drv_data->pll_a_clk, rate);
}

MODULE_AUTHOR("Sidharth R V <svarier@nvidia.com>");
MODULE_DESCRIPTION("EQOS APE driver IO control of AMISC");
MODULE_LICENSE("GPL");
