/*
 * sn65dsi86_dsi2edp.h: dsi to edp controller sn65dsi86 driver.
 *
 * Copyright (c) 2013-2018, NVIDIA CORPORATION. All rights reserved.
 *
 * Author:
 *	Bibek Basu <bbasu@nvidia.com>
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

#ifndef __DRIVERS_VIDEO_TEGRA_DC_SN65DSI86_DSI2EDP_H
#define __DRIVERS_VIDEO_TEGRA_DC_SN65DSI86_DSI2EDP_H

struct tegra_dc_dsi2edp_data {
	struct tegra_dc_dsi_data *dsi;
	struct i2c_client *client_i2c;
	struct regmap *regmap;
	struct tegra_dc_mode *mode;
	bool dsi2edp_enabled;
	struct mutex lock;
	struct {
		int  en_gpio; /* GPIO */
		int  en_gpio_flags;
		int  pll_refclk_cfg;  /* reg.0x0a */
		int  dp_ssc_cfg;      /* reg.0x93 */
		int  negative_hsync;
		int  negative_vsync;
		int  disable_assr;
		int  enable_colorbar;
	}  init;
};

#define  SN65DSI86_DEVICE_ID			0x00
#define  SN65DSI86_DEVICE_REV			0x08
#define  SN65DSI86_SOFT_RESET			0X09
#define  SN65DSI86_PLL_REFCLK_CFG		0x0A
#define  SN65DSI86_PLL_EN			0x0D
#define  SN65DSI86_DSI_CFG1			0x10
#define  SN65DSI86_DSI_CFG2			0x11
#define  SN65DSI86_DSI_CHA_CLK_RANGE		0x12
#define  SN65DSI86_DSI_CHB_CLK_RANGE		0x13
#define  SN65DSI86_VIDEO_CHA_LINE_LOW		0x20
#define  SN65DSI86_VIDEO_CHA_LINE_HIGH		0x21
#define  SN65DSI86_VIDEO_CHB_LINE_LOW		0x22
#define  SN65DSI86_VIDEO_CHB_LINE_HIGH		0x23
#define  SN65DSI86_CHA_VERT_DISP_SIZE_LOW	0x24
#define  SN65DSI86_CHA_VERT_DISP_SIZE_HIGH	0x25
#define  SN65DSI86_CHA_HSYNC_PULSE_WIDTH_LOW	0x2C
#define  SN65DSI86_CHA_HSYNC_PULSE_WIDTH_HIGH	0x2D
#define  SN65DSI86_CHA_VSYNC_PULSE_WIDTH_LOW	0x30
#define  SN65DSI86_CHA_VSYNC_PULSE_WIDTH_HIGH	0x31
#define  SN65DSI86_CHA_HORIZONTAL_BACK_PORCH	0x34
#define  SN65DSI86_CHA_VERTICAL_BACK_PORCH	0x36
#define  SN65DSI86_CHA_HORIZONTAL_FRONT_PORCH	0x38
#define  SN65DSI86_CHA_VERTICAL_FRONT_PORCH	0x3a
#define  SN65DSI86_COLOR_BAR_CFG		0x3c
#define  SN65DSI86_FRAMING_CFG			0x5a
#define  SN65DSI86_DP_18BPP_EN			0x5b
#define  SN65DSI86_REG_0x5c			0x5c
#define  SN65DSI86_GPIO_CTRL_CFG		0x5f
#define  SN65DSI86_DP_SSC_CFG			0x93
#define  SN65DSI86_DP_CFG			0x94
#define  SN65DSI86_TRAINING_CFG			0x95
#define  SN65DSI86_ML_TX_MODE			0x96

#define RETRY_PLL  45   /* 2mSec */
#define RETRY_HPD  25   /* 5mSec */
#define RETRY_LT   300  /* 5mSec */
#endif

