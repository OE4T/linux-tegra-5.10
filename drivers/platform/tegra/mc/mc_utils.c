/**
 * Copyright (c) 2020, NVIDIA CORPORATION. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <linux/types.h>
#include <linux/platform/tegra/mc.h>
#include <linux/platform/tegra/mc_utils.h>
#include <soc/tegra/chip-id.h>

#define BYTES_PER_CLK_PER_CH	4
#define CH_16			16
#define CH_8			8
#define CH_4			4
#define CH_16_BYTES_PER_CLK	(BYTES_PER_CLK_PER_CH * CH_16)
#define CH_8_BYTES_PER_CLK	(BYTES_PER_CLK_PER_CH * CH_8)
#define CH_4_BYTES_PER_CLK	(BYTES_PER_CLK_PER_CH * CH_4)

int ch_num;

static unsigned long freq_to_bw(unsigned long freq)
{
	if (ch_num == CH_16)
		return freq * CH_16_BYTES_PER_CLK;

	if (ch_num == CH_8)
		return freq * CH_8_BYTES_PER_CLK;

	/*4CH and 4CH_ECC*/
	return freq * CH_4_BYTES_PER_CLK;
}

static unsigned long bw_to_freq(unsigned long bw)
{
	if (ch_num == CH_16)
		return (bw + CH_16_BYTES_PER_CLK - 1) / CH_16_BYTES_PER_CLK;

	if (ch_num == CH_8)
		return (bw + CH_8_BYTES_PER_CLK - 1) / CH_8_BYTES_PER_CLK;

	/*4CH and 4CH_ECC*/
	return (bw + CH_4_BYTES_PER_CLK - 1) / CH_4_BYTES_PER_CLK;
}

unsigned long emc_freq_to_bw(unsigned long freq)
{
	return freq_to_bw(freq);
}
EXPORT_SYMBOL_GPL(emc_freq_to_bw);

unsigned long emc_bw_to_freq(unsigned long bw)
{
	return bw_to_freq(bw);
}
EXPORT_SYMBOL_GPL(emc_bw_to_freq);

void tegra_mc_utils_init(int channels)
{
	ch_num = channels;
}
