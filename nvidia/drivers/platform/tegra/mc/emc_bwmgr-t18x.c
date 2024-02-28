/**
 * Copyright (c) 2015-2018, NVIDIA CORPORATION. All rights reserved.
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

#include <linux/platform/tegra/bwmgr_mc.h>
#include <linux/platform/tegra/emc_bwmgr.h>
#include <linux/io.h>

static u32 bwmgr_t186_iso_bw_table[] = { /* MHz */
	  5,  10,  20,  30,  40,  60,  80, 100, 120, 140,
	160, 180, 200, 250, 300, 350, 360, 370, 380, 400,
	450, 500, 550, 600, 650, 700, 750
};

/* value of 1 indicates the range over max ISO Bw allowed for the DRAM type */
static u32 bwmgr_t186_lpddr4_4ch_ecc_iso_eff[] = {
	 15,  15,  15,  15,  15,  15,  15,  15,  15,  15,
	 15,  15,  15,  15,   1,   1,   1,   1,   1,   1,
	  1,   1,   1,   1,   1,   1,   1
};

static u32 bwmgr_t186_lpddr4_2ch_ecc_iso_eff[] = {
	 28,  28,  28,  28,  28,  28,  28,  28,  28,  28,
	 28,  28,  28,  28,  28,  28,  28,  28,  28,  28,
	 28,   1,   1,   1,   1,   1,   1
};

static u32 bwmgr_t186_lpddr4_4ch_iso_eff[] = {
	 15,  19,  27,  35,  40,  42,  43,  44,  45,  46,
	 48,  49,  50,  50,  50,  50,  50,  50,  50,  50,
	 50,  50,  50,  50,  48,  46,  45
};

static u32 bwmgr_t186_lpddr4_2ch_iso_eff[] = {
	 27,  28,  29,  30,  33,  39,  44,  47,  47,  47,
	 47,  47,  47,  47,  47,  47,  47,  47,  47,  47,
	 47,  47,  47,  47,  47,  47,  47
};

static u32 bwmgr_t186_lpddr3_iso_eff[] = {
	31,  32,  33,  34,  36,  40,  44,  48,  48,  48,
	48,  48,  48,  48,  47,  31,   1,   1,   1,   1,
	 1,   1,   1,   1,   1,   1,   1
};

static u32 bwmgr_t186_ddr3_iso_eff[] = {
	28,  29,  32,  34,  36,  41,  45,  47,  47,  47,
	47,  47,  47,  47,  47,  47,  47,  47,  47,  47,
	47,  47,   1,   1,   1,   1,   1
};

#define MC_BASE 0x02c10000
#define EMC_BASE 0x02c60000

#define MC_EMEM_ADR_CFG_CHANNEL_ENABLE_0 0xdf8
#define MC_ECC_CONTROL_0 0x1880
#define EMC_FBIO_CFG5_0 0x104

#define CH_MASK 0xf /* Change bit counting if this mask changes */
#define CH4 0xf
#define CH2 0x3

#define ECC_MASK 0x1 /* 1 = enabled, 0 = disabled */

#define DRAM_MASK 0x3
#define DRAM_DDR3 0
#define DRAM_LPDDR4 1
#define DRAM_LPDDR3 2 /* On T186 this value is LPDDR3 */
#define DRAM_DDR2 3

static int get_iso_bw_table_idx(unsigned long iso_bw)
{
	int i = ARRAY_SIZE(bwmgr_t186_iso_bw_table) - 1;

	/* Input is in Hz, iso_bw table's unit is MHz */
	iso_bw /= 1000000;

	while (i > 0 && bwmgr_t186_iso_bw_table[i] > iso_bw)
		i--;

	return i;
}

static unsigned long freq_to_bw(unsigned long freq)
{
	if (bwmgr_dram_type == DRAM_TYPE_LPDDR4_4CH_ECC ||
			bwmgr_dram_type == DRAM_TYPE_LPDDR4_4CH ||
			bwmgr_dram_type == DRAM_TYPE_LPDDR3_2CH ||
			bwmgr_dram_type == DRAM_TYPE_DDR3_2CH)
		return freq * 32;

	return freq * 16;
}

static unsigned long bw_to_freq(unsigned long bw)
{
	if (bwmgr_dram_type == DRAM_TYPE_LPDDR4_4CH_ECC ||
			bwmgr_dram_type == DRAM_TYPE_LPDDR4_4CH ||
			bwmgr_dram_type == DRAM_TYPE_LPDDR3_2CH ||
			bwmgr_dram_type == DRAM_TYPE_DDR3_2CH)
		return (bw + 32 - 1) / 32;

	return (bw + 16 - 1) / 16;
}

static u32 dvfs_latency(u32 ufreq)
{
	u32 lt = 4000; /* default value of 4000 nsec */
	int i;

	if (bwmgr_emc_dvfs.num_pairs <= 0)
		return lt / 1000; /* convert nsec to usec, Bug 1697424 */

	for (i = 0; i < bwmgr_emc_dvfs.num_pairs; i++) {
		if (ufreq <= bwmgr_emc_dvfs.pairs[i].freq) {
			lt = bwmgr_emc_dvfs.pairs[i].latency;
			break;
		}
	}

	if (i >= bwmgr_emc_dvfs.num_pairs)
		lt =
		bwmgr_emc_dvfs.pairs[bwmgr_emc_dvfs.num_pairs - 1].latency;

	return lt / 1000; /* convert nsec to usec, Bug 1697424 */
}

static unsigned long t18x_bwmgr_apply_efficiency(
		unsigned long total_bw, unsigned long iso_bw,
		unsigned long max_rate, u64 usage_flags,
		unsigned long *iso_bw_min, unsigned long iso_bw_nvdis,
		unsigned long iso_bw_vi)
{
	u8 efficiency = bwmgr_dram_efficiency;

	if (total_bw && efficiency && (efficiency < 100)) {
		total_bw = total_bw / efficiency;
		total_bw = (total_bw < max_rate / 100) ?
			(total_bw * 100) : max_rate;
	}

	efficiency = bwmgr_dram_iso_eff_table[get_iso_bw_table_idx(iso_bw)];
	WARN_ON_ONCE(efficiency == 1);
	if (iso_bw && efficiency && (efficiency < 100)) {
		iso_bw /= efficiency;
		iso_bw = (iso_bw < max_rate / 100) ?
		(iso_bw * 100) : max_rate;
	}

	if (iso_bw_min)
		*iso_bw_min = iso_bw;

	return max(total_bw, iso_bw);
}

static struct bwmgr_ops bwmgr_ops_t18x = {
	.freq_to_bw = freq_to_bw,
	.bw_to_freq = bw_to_freq,
	.dvfs_latency = dvfs_latency,
	.bwmgr_apply_efficiency = t18x_bwmgr_apply_efficiency,
};

struct bwmgr_ops *bwmgr_eff_init_t18x(void)
{
	int i, ch_num;
	u32 dram, ch, ecc;
	void __iomem *mc_base, *emc_base;

	mc_base = ioremap(MC_BASE, 0x00010000);
	emc_base = ioremap(EMC_BASE, 0x00010000);

	dram = readl(emc_base + EMC_FBIO_CFG5_0) & DRAM_MASK;
	ch = readl(mc_base + MC_EMEM_ADR_CFG_CHANNEL_ENABLE_0) & CH_MASK;
	ecc = readl(mc_base + MC_ECC_CONTROL_0) & ECC_MASK;

	iounmap(emc_base);
	iounmap(mc_base);

	ch_num = ((ch >> 3) & 1) + ((ch >> 2) & 1) + ((ch >> 1) & 1) + (ch & 1);

	/* T186 platforms should only have LPDDR4 */
	switch (dram) {
	case DRAM_LPDDR4:
		if (ecc) {
			if (ch_num == 4) {
				bwmgr_dram_type =
					DRAM_TYPE_LPDDR4_4CH_ECC;
				bwmgr_dram_iso_eff_table =
				bwmgr_t186_lpddr4_4ch_ecc_iso_eff;
			} else if (ch_num == 2) {
				bwmgr_dram_type =
					DRAM_TYPE_LPDDR4_2CH_ECC;
				bwmgr_dram_iso_eff_table =
				bwmgr_t186_lpddr4_2ch_ecc_iso_eff;
			}
		} else {
			if (ch_num == 4) {
				bwmgr_dram_type = DRAM_TYPE_LPDDR4_4CH;
				bwmgr_dram_iso_eff_table =
				bwmgr_t186_lpddr4_4ch_iso_eff;
			} else if (ch_num == 2) {
				bwmgr_dram_type = DRAM_TYPE_LPDDR4_2CH;
				bwmgr_dram_iso_eff_table =
				bwmgr_t186_lpddr4_2ch_iso_eff;
			}
		}

		if ((ch_num == 0) || (ch_num == 1) || (ch_num == 3)) {
			pr_err("bwmgr: Unknown memory channel configuration\n");
			bwmgr_dram_type =
				DRAM_TYPE_LPDDR4_2CH_ECC;
			bwmgr_dram_iso_eff_table =
				bwmgr_t186_lpddr4_2ch_ecc_iso_eff;
		}

		bwmgr_dram_efficiency = 70;
		emc_to_dram_freq_factor = 2;
		/* valid ddr configuration */
		bwmgr_dram_config_supported = 1;
		break;

	case DRAM_LPDDR3:
		bwmgr_dram_type = DRAM_TYPE_LPDDR3_2CH;
		bwmgr_dram_efficiency = 80;
		bwmgr_dram_iso_eff_table =
			bwmgr_t186_lpddr3_iso_eff;
		emc_to_dram_freq_factor = 1;
		/* valid ddr configuration */
		bwmgr_dram_config_supported = 1;
		break;

	case DRAM_DDR3:
		bwmgr_dram_type = DRAM_TYPE_DDR3_2CH;
		bwmgr_dram_efficiency = 80;
		bwmgr_dram_iso_eff_table =
			bwmgr_t186_ddr3_iso_eff;
		emc_to_dram_freq_factor = 1;
		/* valid ddr configuration */
		bwmgr_dram_config_supported = 1;
		break;

	case DRAM_DDR2:
		pr_err("bwmgr: ddr config not supported\n");
		WARN_ON(true);
		break;

	default:
		pr_err("bwmgr: ddr config not supported\n");
		WARN_ON(true);
	}
	bwmgr_dram_num_channels = ch_num;

	if (bwmgr_dram_config_supported) {
		for (i = ARRAY_SIZE(bwmgr_t186_iso_bw_table) - 1; i >= 0; i--) {
			if (bwmgr_dram_iso_eff_table[i] > 1) {
				bwmgr_iso_bw_percentage =
					bwmgr_dram_iso_eff_table[i];
				break;
			}
		}
	}

	return &bwmgr_ops_t18x;
}
