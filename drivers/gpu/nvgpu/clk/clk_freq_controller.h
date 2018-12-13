/*
* Copyright (c) 2016-2018, NVIDIA CORPORATION.  All rights reserved.
*
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
*/

#ifndef NVGPU_CLK_FREQ_CONTROLLER_H
#define NVGPU_CLK_FREQ_CONTROLLER_H

#define CTRL_CLK_CLK_FREQ_CONTROLLER_ID_ALL  0xFFU
#define CTRL_CLK_CLK_FREQ_CONTROLLER_ID_SYS  0x00U
#define CTRL_CLK_CLK_FREQ_CONTROLLER_ID_LTC  0x01U
#define CTRL_CLK_CLK_FREQ_CONTROLLER_ID_XBAR 0x02U
#define CTRL_CLK_CLK_FREQ_CONTROLLER_ID_GPC0 0x03U
#define CTRL_CLK_CLK_FREQ_CONTROLLER_ID_GPC1 0x04U
#define CTRL_CLK_CLK_FREQ_CONTROLLER_ID_GPC2 0x05U
#define CTRL_CLK_CLK_FREQ_CONTROLLER_ID_GPC3 0x06U
#define CTRL_CLK_CLK_FREQ_CONTROLLER_ID_GPC4 0x07U
#define CTRL_CLK_CLK_FREQ_CONTROLLER_ID_GPC5 0x08U
#define CTRL_CLK_CLK_FREQ_CONTROLLER_ID_GPCS 0x09U

#define CTRL_CLK_CLK_FREQ_CONTROLLER_MASK_UNICAST_GPC     \
			(BIT32(CTRL_CLK_CLK_FREQ_CONTROLLER_ID_GPC0) | \
			BIT32(CTRL_CLK_CLK_FREQ_CONTROLLER_ID_GPC1) | \
			BIT32(CTRL_CLK_CLK_FREQ_CONTROLLER_ID_GPC2) | \
			BIT32(CTRL_CLK_CLK_FREQ_CONTROLLER_ID_GPC3) | \
			BIT32(CTRL_CLK_CLK_FREQ_CONTROLLER_ID_GPC4) | \
			BIT32(CTRL_CLK_CLK_FREQ_CONTROLLER_ID_GPC5))

#define CTRL_CLK_CLK_FREQ_CONTROLLER_TYPE_DISABLED  0x00U
#define CTRL_CLK_CLK_FREQ_CONTROLLER_TYPE_PI        0x01U


struct clk_freq_controller {
	struct boardobj    super;
	u8   controller_id;
	u8   parts_freq_mode;
	bool bdisable;
	u32  clk_domain;
	s16  freq_cap_noise_unaware_vmin_above;
	s16  freq_cap_noise_unaware_vmin_below;
	s16  freq_hyst_pos_mhz;
	s16  freq_hyst_neg_mhz;
};

struct clk_freq_controller_pi {
	struct clk_freq_controller super;
	s32 prop_gain;
	s32 integ_gain;
	s32 integ_decay;
	s32 volt_delta_min;
	s32 volt_delta_max;
	u8  slowdown_pct_min;
	bool bpoison;
};

#endif /* NVGPU_CLK_FREQ_CONTROLLER_H */
