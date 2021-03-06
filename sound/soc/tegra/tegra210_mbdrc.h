/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * tegra210_mbdrc.h - Definitions for Tegra210 MBDRC driver
 *
 * Copyright (c) 2014-2021 NVIDIA CORPORATION.  All rights reserved.
 *
 */

#ifndef __TEGRA210_MBDRC_H__
#define __TEGRA210_MBDRC_H__

/* Order of these enums are same as the order of band specific hw registers */
enum {
	MBDRC_LOW_BAND,
	MBDRC_MID_BAND,
	MBDRC_HIGH_BAND,
	MBDRC_NUM_BAND,
};

/* Register offsets from TEGRA210_MBDRC*_BASE */
#define TEGRA210_MBDRC_SOFT_RESET			0x4
#define TEGRA210_MBDRC_CG				0x8
#define TEGRA210_MBDRC_STATUS				0xc
#define TEGRA210_MBDRC_CONFIG				0x28
#define TEGRA210_MBDRC_CHANNEL_MASK			0x2c
#define TEGRA210_MBDRC_MASTER_VOLUME			0x30
#define TEGRA210_MBDRC_FAST_FACTOR			0x34

#define TEGRA210_MBDRC_FILTER_COUNT			3
#define TEGRA210_MBDRC_FILTER_PARAM_STRIDE		0x4

#define TEGRA210_MBDRC_IIR_CONFIG			0x38
#define TEGRA210_MBDRC_IN_ATTACK			0x44
#define TEGRA210_MBDRC_IN_RELEASE			0x50
#define TEGRA210_MBDRC_FAST_ATTACK			0x5c
#define TEGRA210_MBDRC_IN_THRESHOLD			0x68
#define TEGRA210_MBDRC_OUT_THRESHOLD			0x74
#define TEGRA210_MBDRC_RATIO_1ST			0x80
#define TEGRA210_MBDRC_RATIO_2ND			0x8c
#define TEGRA210_MBDRC_RATIO_3RD			0x98
#define TEGRA210_MBDRC_RATIO_4TH			0xa4
#define TEGRA210_MBDRC_RATIO_5TH			0xb0
#define TEGRA210_MBDRC_MAKEUP_GAIN			0xbc
#define TEGRA210_MBDRC_INIT_GAIN			0xc8
#define TEGRA210_MBDRC_GAIN_ATTACK			0xd4
#define TEGRA210_MBDRC_GAIN_RELEASE			0xe0
#define TEGRA210_MBDRC_FAST_RELEASE			0xec
#define TEGRA210_MBDRC_AHUBRAMCTL_CONFIG_RAM_CTRL	0xf8
#define TEGRA210_MBDRC_AHUBRAMCTL_CONFIG_RAM_DATA	0x104

#define TEGRA210_MBDRC_MAX_REG				(TEGRA210_MBDRC_AHUBRAMCTL_CONFIG_RAM_DATA + \
							(TEGRA210_MBDRC_FILTER_PARAM_STRIDE * \
							(TEGRA210_MBDRC_FILTER_COUNT - 1)))

/* Fields for TEGRA210_MBDRC_CONFIG */
#define TEGRA210_MBDRC_CONFIG_RMS_OFFSET_SHIFT		16
#define TEGRA210_MBDRC_CONFIG_RMS_OFFSET_MASK		(0x1ff << TEGRA210_MBDRC_CONFIG_RMS_OFFSET_SHIFT)

#define TEGRA210_MBDRC_CONFIG_PEAK_RMS_SHIFT		14
#define TEGRA210_MBDRC_CONFIG_PEAK_RMS_MASK		(0x1 << TEGRA210_MBDRC_CONFIG_PEAK_RMS_SHIFT)
#define TEGRA210_MBDRC_CONFIG_PEAK			(1 << TEGRA210_MBDRC_CONFIG_PEAK_RMS_SHIFT)

#define TEGRA210_MBDRC_CONFIG_FILTER_STRUCTURE_SHIFT	13
#define TEGRA210_MBDRC_CONFIG_FILTER_STRUCTURE_MASK	(0x1 << TEGRA210_MBDRC_CONFIG_FILTER_STRUCTURE_SHIFT)
#define TEGRA210_MBDRC_CONFIG_FILTER_STRUCTURE_FLEX	(1 << TEGRA210_MBDRC_CONFIG_FILTER_STRUCTURE_SHIFT)

#define TEGRA210_MBDRC_CONFIG_SHIFT_CTRL_SHIFT		8
#define TEGRA210_MBDRC_CONFIG_SHIFT_CTRL_MASK		(0x1f << TEGRA210_MBDRC_CONFIG_SHIFT_CTRL_SHIFT)

#define TEGRA210_MBDRC_CONFIG_FRAME_SIZE_SHIFT		4
#define TEGRA210_MBDRC_CONFIG_FRAME_SIZE_MASK		(0xf << TEGRA210_MBDRC_CONFIG_FRAME_SIZE_SHIFT)
#define TEGRA210_MBDRC_CONFIG_FRAME_SIZE_N1		(0 << TEGRA210_MBDRC_CONFIG_FRAME_SIZE_SHIFT)
#define TEGRA210_MBDRC_CONFIG_FRAME_SIZE_N2		(1 << TEGRA210_MBDRC_CONFIG_FRAME_SIZE_SHIFT)
#define TEGRA210_MBDRC_CONFIG_FRAME_SIZE_N4		(2 << TEGRA210_MBDRC_CONFIG_FRAME_SIZE_SHIFT)
#define TEGRA210_MBDRC_CONFIG_FRAME_SIZE_N8		(3 << TEGRA210_MBDRC_CONFIG_FRAME_SIZE_SHIFT)
#define TEGRA210_MBDRC_CONFIG_FRAME_SIZE_N16		(4 << TEGRA210_MBDRC_CONFIG_FRAME_SIZE_SHIFT)
#define TEGRA210_MBDRC_CONFIG_FRAME_SIZE_N32		(5 << TEGRA210_MBDRC_CONFIG_FRAME_SIZE_SHIFT)
#define TEGRA210_MBDRC_CONFIG_FRAME_SIZE_N64		(6 << TEGRA210_MBDRC_CONFIG_FRAME_SIZE_SHIFT)

#define TEGRA210_MBDRC_CONFIG_MBDRC_MODE_SHIFT		0
#define TEGRA210_MBDRC_CONFIG_MBDRC_MODE_MASK		(0x3 << TEGRA210_MBDRC_CONFIG_MBDRC_MODE_SHIFT)
#define TEGRA210_MBDRC_CONFIG_MBDRC_MODE_BYPASS		(0 << TEGRA210_MBDRC_CONFIG_MBDRC_MODE_SHIFT)
#define TEGRA210_MBDRC_CONFIG_MBDRC_MODE_FULLBAND	(1 << TEGRA210_MBDRC_CONFIG_MBDRC_MODE_SHIFT)
#define TEGRA210_MBDRC_CONFIG_MBDRC_MODE_DUALBAND	(2 << TEGRA210_MBDRC_CONFIG_MBDRC_MODE_SHIFT)
#define TEGRA210_MBDRC_CONFIG_MBDRC_MODE_MULTIBAND	(3 << TEGRA210_MBDRC_CONFIG_MBDRC_MODE_SHIFT)

/* Fields for TEGRA210_MBDRC_CHANNEL_MASK */
#define TEGRA210_MBDRC_CHANNEL_MASK_SHIFT		0
#define TEGRA210_MBDRC_CHANNEL_MASK_MASK		(0xff << TEGRA210_MBDRC_CHANNEL_MASK_SHIFT)

/* Fields for TEGRA210_MBDRC_MASTER_VOLUME */
#define TEGRA210_MBDRC_MASTER_VOLUME_SHIFT		23
#define TEGRA210_MBDRC_MASTER_VOL_MIN			-256
#define TEGRA210_MBDRC_MASTER_VOL_MAX			256

/* Fields for TEGRA210_MBDRC_FAST_FACTOR */
#define TEGRA210_MBDRC_FAST_FACTOR_RELEASE_SHIFT	16
#define TEGRA210_MBDRC_FAST_FACTOR_RELEASE_MASK		(0xffff << TEGRA210_MBDRC_FAST_FACTOR_RELEASE_SHIFT)

#define TEGRA210_MBDRC_FAST_FACTOR_ATTACK_SHIFT		0
#define TEGRA210_MBDRC_FAST_FACTOR_ATTACK_MASK		(0xffff << TEGRA210_MBDRC_FAST_FACTOR_ATTACK_SHIFT)

/* Fields for TEGRA210_MBDRC_IIR_CONFIG */
#define TEGRA210_MBDRC_IIR_CONFIG_NUM_STAGES_SHIFT	0
#define TEGRA210_MBDRC_IIR_CONFIG_NUM_STAGES_MASK	(0xf << TEGRA210_MBDRC_IIR_CONFIG_NUM_STAGES_SHIFT)

/* Fields for TEGRA210_MBDRC_IN_ATTACK */
#define TEGRA210_MBDRC_IN_ATTACK_TC_SHIFT		0
#define TEGRA210_MBDRC_IN_ATTACK_TC_MASK		(0xffffffff << TEGRA210_MBDRC_IN_ATTACK_TC_SHIFT)

/* Fields for TEGRA210_MBDRC_IN_RELEASE */
#define TEGRA210_MBDRC_IN_RELEASE_TC_SHIFT		0
#define TEGRA210_MBDRC_IN_RELEASE_TC_MASK		(0xffffffff << TEGRA210_MBDRC_IN_RELEASE_TC_SHIFT)

/* Fields for TEGRA210_MBDRC_FAST_ATTACK */
#define TEGRA210_MBDRC_FAST_ATTACK_TC_SHIFT		0
#define TEGRA210_MBDRC_FAST_ATTACK_TC_MASK		(0xffffffff << TEGRA210_MBDRC_FAST_ATTACK_TC_SHIFT)

/* Fields for TEGRA210_MBDRC_IN_THRESHOLD / TEGRA210_MBDRC_OUT_THRESHOLD */
#define TEGRA210_MBDRC_THRESH_4TH_SHIFT			24
#define TEGRA210_MBDRC_THRESH_4TH_MASK			(0xff << TEGRA210_MBDRC_THRESH_4TH_SHIFT)

#define TEGRA210_MBDRC_THRESH_3RD_SHIFT			16
#define TEGRA210_MBDRC_THRESH_3RD_MASK			(0xff << TEGRA210_MBDRC_THRESH_3RD_SHIFT)

#define TEGRA210_MBDRC_THRESH_2ND_SHIFT			8
#define TEGRA210_MBDRC_THRESH_2ND_MASK			(0xff << TEGRA210_MBDRC_THRESH_2ND_SHIFT)

#define TEGRA210_MBDRC_THRESH_1ST_SHIFT			0
#define TEGRA210_MBDRC_THRESH_1ST_MASK			(0xff << TEGRA210_MBDRC_THRESH_1ST_SHIFT)

/* Fields for TEGRA210_MBDRC_RATIO_1ST */
#define TEGRA210_MBDRC_RATIO_1ST_SHIFT			0
#define TEGRA210_MBDRC_RATIO_1ST_MASK			(0xffff << TEGRA210_MBDRC_RATIO_1ST_SHIFT)

/* Fields for TEGRA210_MBDRC_RATIO_2ND */
#define TEGRA210_MBDRC_RATIO_2ND_SHIFT			0
#define TEGRA210_MBDRC_RATIO_2ND_MASK			(0xffff << TEGRA210_MBDRC_RATIO_2ND_SHIFT)

/* Fields for TEGRA210_MBDRC_RATIO_3RD */
#define TEGRA210_MBDRC_RATIO_3RD_SHIFT			0
#define TEGRA210_MBDRC_RATIO_3RD_MASK			(0xffff << TEGRA210_MBDRC_RATIO_3RD_SHIFT)

/* Fields for TEGRA210_MBDRC_RATIO_4TH */
#define TEGRA210_MBDRC_RATIO_4TH_SHIFT			0
#define TEGRA210_MBDRC_RATIO_4TH_MASK			(0xffff << TEGRA210_MBDRC_RATIO_4TH_SHIFT)

/* Fields for TEGRA210_MBDRC_RATIO_5TH */
#define TEGRA210_MBDRC_RATIO_5TH_SHIFT			0
#define TEGRA210_MBDRC_RATIO_5TH_MASK			(0xffff << TEGRA210_MBDRC_RATIO_5TH_SHIFT)

/* Fields for TEGRA210_MBDRC_MAKEUP_GAIN */
#define TEGRA210_MBDRC_MAKEUP_GAIN_SHIFT		0
#define TEGRA210_MBDRC_MAKEUP_GAIN_MASK			(0x3f << TEGRA210_MBDRC_MAKEUP_GAIN_SHIFT)

/* Fields for TEGRA210_MBDRC_INIT_GAIN */
#define TEGRA210_MBDRC_INIT_GAIN_SHIFT			0
#define TEGRA210_MBDRC_INIT_GAIN_MASK			(0xffffffff << TEGRA210_MBDRC_INIT_GAIN_SHIFT)

/* Fields for TEGRA210_MBDRC_GAIN_ATTACK */
#define TEGRA210_MBDRC_GAIN_ATTACK_SHIFT		0
#define TEGRA210_MBDRC_GAIN_ATTACK_MASK			(0xffffffff << TEGRA210_MBDRC_GAIN_ATTACK_SHIFT)

/* Fields for TEGRA210_MBDRC_GAIN_RELEASE */
#define TEGRA210_MBDRC_GAIN_RELEASE_SHIFT		0
#define TEGRA210_MBDRC_GAIN_RELEASE_MASK		(0xffffffff << TEGRA210_MBDRC_GAIN_RELEASE_SHIFT)

/* Fields for TEGRA210_MBDRC_FAST_RELEASE */
#define TEGRA210_MBDRC_FAST_RELEASE_SHIFT		0
#define TEGRA210_MBDRC_FAST_RELEASE_MASK		(0xffffffff << TEGRA210_MBDRC_FAST_RELEASE_SHIFT)

/* Fields in TEGRA210_mbdrc ram ctrl */
#define TEGRA210_MBDRC_AHUBRAMCTL_CONFIG_RAM_CTRL_READ_BUSY_SHIFT		31
#define TEGRA210_MBDRC_AHUBRAMCTL_CONFIG_RAM_CTRL_READ_BUSY_MASK		(1 << TEGRA210_MBDRC_AHUBRAMCTL_CONFIG_RAM_CTRL_READ_BUSY_SHIFT)
#define TEGRA210_MBDRC_AHUBRAMCTL_CONFIG_RAM_CTRL_READ_BUSY			(1 << TEGRA210_MBDRC_AHUBRAMCTL_CONFIG_RAM_CTRL_READ_BUSY_SHIFT)

#define TEGRA210_MBDRC_AHUBRAMCTL_CONFIG_RAM_CTRL_SEQ_READ_COUNT_SHIFT		16
#define TEGRA210_MBDRC_AHUBRAMCTL_CONFIG_RAM_CTRL_SEQ_READ_COUNT_MASK		(0xff << TEGRA210_MBDRC_AHUBRAMCTL_CONFIG_RAM_CTRL_SEQ_READ_COUNT_SHIFT)

#define TEGRA210_MBDRC_AHUBRAMCTL_CONFIG_RAM_CTRL_RW_SHIFT			14
#define TEGRA210_MBDRC_AHUBRAMCTL_CONFIG_RAM_CTRL_RW_MASK			(1 << TEGRA210_MBDRC_AHUBRAMCTL_CONFIG_RAM_CTRL_RW_SHIFT)
#define TEGRA210_MBDRC_AHUBRAMCTL_CONFIG_RAM_CTRL_RW_READ			(0 << TEGRA210_MBDRC_AHUBRAMCTL_CONFIG_RAM_CTRL_RW_SHIFT)
#define TEGRA210_MBDRC_AHUBRAMCTL_CONFIG_RAM_CTRL_RW_WRITE			(1 << TEGRA210_MBDRC_AHUBRAMCTL_CONFIG_RAM_CTRL_RW_SHIFT)

#define TEGRA210_MBDRC_AHUBRAMCTL_CONFIG_RAM_CTRL_ADDR_INIT_EN_SHIFT		13
#define TEGRA210_MBDRC_AHUBRAMCTL_CONFIG_RAM_CTRL_ADDR_INIT_EN_MASK		(1 << TEGRA210_MBDRC_AHUBRAMCTL_CONFIG_RAM_CTRL_ADDR_INIT_EN_SHIFT)
#define TEGRA210_MBDRC_AHUBRAMCTL_CONFIG_RAM_CTRL_ADDR_INIT_EN			(1 << TEGRA210_MBDRC_AHUBRAMCTL_CONFIG_RAM_CTRL_ADDR_INIT_EN_SHIFT)

#define TEGRA210_MBDRC_AHUBRAMCTL_CONFIG_RAM_CTRL_SEQ_ACCESS_EN_SHIFT		12
#define TEGRA210_MBDRC_AHUBRAMCTL_CONFIG_RAM_CTRL_SEQ_ACCESS_EN_MASK		(1 << TEGRA210_MBDRC_AHUBRAMCTL_CONFIG_RAM_CTRL_SEQ_ACCESS_EN_SHIFT)
#define TEGRA210_MBDRC_AHUBRAMCTL_CONFIG_RAM_CTRL_SEQ_ACCESS_EN			(1 << TEGRA210_MBDRC_AHUBRAMCTL_CONFIG_RAM_CTRL_SEQ_ACCESS_EN_SHIFT)

#define TEGRA210_MBDRC_AHUBRAMCTL_CONFIG_RAM_CTRL_RAM_ADDR_SHIFT		0
#define TEGRA210_MBDRC_AHUBRAMCTL_CONFIG_RAM_CTRL_RAM_ADDR_MASK			(0x1ff << TEGRA210_MBDRC_AHUBRAMCTL_CONFIG_RAM_CTRL_RAM_ADDR_SHIFT)
/* MBDRC register definition ends here */

/* order and size of each structure element for following structures should not
   be altered size order of elements and their size are based on PEQ
   co-eff ram and shift ram layout.
*/
#define TEGRA210_MBDRC_THRESHOLD_NUM	4
#define TEGRA210_MBDRC_RATIO_NUM	(TEGRA210_MBDRC_THRESHOLD_NUM + 1)
#define TEGRA210_MBDRC_MAX_BIQUAD_STAGES 8

#define TEGRA210_MBDRC_BIQ_PARAMS_PER_STAGE	5

struct tegra210_mbdrc_band_params {
	u32 band;
	u32 iir_stages;
	u32 in_attack_tc;
	u32 in_release_tc;
	u32 fast_attack_tc;
	u32 in_threshold[TEGRA210_MBDRC_THRESHOLD_NUM];
	u32 out_threshold[TEGRA210_MBDRC_THRESHOLD_NUM];
	u32 ratio[TEGRA210_MBDRC_RATIO_NUM];
	u32 makeup_gain;
	u32 gain_init;
	u32 gain_attack_tc;
	u32 gain_release_tc;
	u32 fast_release_tc;
	/* For biquad_params[][5] order of coeff is b0, b1, a0, a1, a2 */
	u32 biquad_params[TEGRA210_MBDRC_MAX_BIQUAD_STAGES * 5];
};

struct tegra210_mbdrc_config {
	unsigned int mode;
	unsigned int rms_off;
	unsigned int peak_rms_mode;
	unsigned int fliter_structure;
	unsigned int shift_ctrl;
	unsigned int frame_size;
	unsigned int channel_mask;
	unsigned int fa_factor;	/* Fast attack factor */
	unsigned int fr_factor;	/* Fast release factor */
	struct tegra210_mbdrc_band_params band_params[MBDRC_NUM_BAND];
};

#endif
