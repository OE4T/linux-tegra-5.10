/*
 * include/linux/platform/tegra12_emc.h
 *
 * Copyright (c) 2013-2014, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _MACH_TEGRA_TEGRA12_EMC_H
#define _MACH_TEGRA_TEGRA12_EMC_H

#include <linux/platform/tegra/tegra_emc.h>
#include <linux/platform_data/tegra_emc_pdata.h>

int tegra12_emc_init(void);

enum {
	DRAM_DEV_SEL_ALL = 0,
	DRAM_DEV_SEL_0	 = (2 << 30),
	DRAM_DEV_SEL_1   = (1 << 30),
};

void tegra12_mc_holdoff_enable(void);

#define DRAM_BROADCAST(num)			\
	(((num) > 1) ? DRAM_DEV_SEL_ALL : DRAM_DEV_SEL_0)

#define EMC_INTSTATUS				0x0
#define EMC_INTSTATUS_CLKCHANGE_COMPLETE	(0x1 << 4)

#define EMC_DBG					0x8
#define EMC_DBG_WRITE_MUX_ACTIVE		(0x1 << 1)

#define EMC_CFG					0xc
#define EMC_CFG_DRAM_CLKSTOP_PD		(0x1 << 31)
#define EMC_CFG_DRAM_CLKSTOP_SR		(0x1 << 30)
#define EMC_CFG_DRAM_ACPD			(0x1 << 29)
#define EMC_CFG_DYN_SREF			(0x1 << 28)
#define EMC_CFG_PWR_MASK			((0xF << 28) | (0x1 << 18))
#define EMC_CFG_REQACT_ASYNC		(0x1 << 26)
#define EMC_CFG_AUTO_PRE_WR			(0x1 << 25)
#define EMC_CFG_AUTO_PRE_RD			(0x1 << 24)
#define EMC_CFG_MAM_PRE_WR			(0x1 << 23)
#define EMC_CFG_MAN_PRE_RD			(0x1 << 22)
#define EMC_CFG_PERIODIC_QRST			(0x1 << 21)
#define EMC_CFG_EN_DYNAMIC_PUTERM		(0x1 << 20)
#define EMC_CFG_DLY_WR_DQ_HALF_CLOCK		(0x1 << 19)
#define EMC_CFG_DSR_VTTGEN_DRV_EN		(0x1 << 18)
#define EMC_CFG_EMC2MC_CLK_RATIO		(0x3 << 16)
#define EMC_CFG_WAIT_FOR_ISP2B_READY_B4_CC	(0x1 << 9)
#define EMC_CFG_WAIT_FOR_VI2_READY_B4_CC	(0x1 << 8)
#define EMC_CFG_WAIT_FOR_ISP2_READY_B4_CC	(0x1 << 7)
#define EMC_CFG_INVERT_DQM			(0x1 << 6)
#define EMC_CFG_WAIT_FOR_DISPLAYB_READY_B4_CC (0x1 << 5)
#define EMC_CFG_WAIT_FOR_DISPLAY_READY_B4_CC (0x1 << 4)
#define EMC_CFG_EMC2PMACRO_CFG_BYPASS_DATAPIPE2	(0x1 << 3)
#define EMC_CFG_EMC2PMACRO_CFG_BYPASS_DATAPIPE1	(0x1 << 2)
#define EMC_CFG_EMC2PMACRO_CFG_BYPASS_ADDRPIPE	(0x1 << 1)


#define EMC_ADR_CFG				0x10
#define EMC_REFCTRL				0x20
#define EMC_REFCTRL_DEV_SEL_SHIFT		0
#define EMC_REFCTRL_DEV_SEL_MASK		\
	(0x3 << EMC_REFCTRL_DEV_SEL_SHIFT)
#define EMC_REFCTRL_ENABLE			(0x1 << 31)
#define EMC_REFCTRL_ENABLE_ALL(num)		\
	(((((num) > 1) ? 0 : 2) << EMC_REFCTRL_DEV_SEL_SHIFT) \
	 | EMC_REFCTRL_ENABLE)
#define EMC_REFCTRL_DISABLE_ALL(num)		\
	((((num) > 1) ? 0 : 2) << EMC_REFCTRL_DEV_SEL_SHIFT)

#define EMC_TIMING_CONTROL			0x28
#define EMC_RC					0x2c
#define EMC_RFC					0x30
#define EMC_RAS					0x34
#define EMC_RP					0x38
#define EMC_R2W					0x3c
#define EMC_W2R					0x40
#define EMC_R2P					0x44
#define EMC_W2P					0x48
#define EMC_RD_RCD				0x4c
#define EMC_WR_RCD				0x50
#define EMC_RRD					0x54
#define EMC_REXT				0x58
#define EMC_WDV					0x5c
#define EMC_QUSE				0x60
#define EMC_QRST				0x64
#define EMC_QSAFE				0x68
#define EMC_RDV					0x6c
#define EMC_REFRESH				0x70
#define EMC_BURST_REFRESH_NUM			0x74
#define EMC_PDEX2WR				0x78
#define EMC_PDEX2RD				0x7c
#define EMC_PCHG2PDEN				0x80
#define EMC_ACT2PDEN				0x84
#define EMC_AR2PDEN				0x88
#define EMC_RW2PDEN				0x8c
#define EMC_TXSR				0x90
#define EMC_TCKE				0x94
#define EMC_TFAW				0x98
#define EMC_TRPAB				0x9c
#define EMC_TCLKSTABLE				0xa0
#define EMC_TCLKSTOP				0xa4
#define EMC_TREFBW				0xa8
#define EMC_ODT_WRITE				0xb0
#define EMC_ODT_READ				0xb4
#define EMC_WEXT				0xb8
#define EMC_CTT					0xbc
#define EMC_RFC_SLR				0xc0
#define EMC_MRS_WAIT_CNT2			0xc4

#define EMC_MRS_WAIT_CNT			0xc8
#define EMC_MRS_WAIT_CNT_SHORT_WAIT_SHIFT	0
#define EMC_MRS_WAIT_CNT_SHORT_WAIT_MASK	\
	(0x3FF << EMC_MRS_WAIT_CNT_SHORT_WAIT_SHIFT)
#define EMC_MRS_WAIT_CNT_LONG_WAIT_SHIFT	16
#define EMC_MRS_WAIT_CNT_LONG_WAIT_MASK		\
	(0x3FF << EMC_MRS_WAIT_CNT_LONG_WAIT_SHIFT)

#define EMC_MRS					0xcc
#define EMC_MODE_SET_DLL_RESET			(0x1 << 8)
#define EMC_MODE_SET_LONG_CNT			(0x1 << 26)
#define EMC_EMRS				0xd0
#define EMC_REF					0xd4
#define EMC_REF_FORCE_CMD                       1
#define EMC_PRE					0xd8
#define EMC_NOP					0xdc

#define EMC_SELF_REF				0xe0
#define EMC_SELF_REF_CMD_ENABLED		(0x1 << 0)
#define EMC_SELF_REF_DEV_SEL_SHIFT		30
#define EMC_SELF_REF_DEV_SEL_MASK		\
	(0x3 << EMC_SELF_REF_DEV_SEL_SHIFT)

#define EMC_DPD					0xe4
#define EMC_MRW					0xe8

#define EMC_MRR					0xec
#define EMC_MRR_DEV_SEL_SHIFT			30
#define EMC_MRR_DEV_SEL_MASK			\
	(0x3 << EMC_SELF_REF_DEV_SEL_SHIFT)
#define EMC_MRR_MA_SHIFT			16
#define EMC_MRR_MA_MASK				(0xFF << EMC_MRR_MA_SHIFT)
#define EMC_MRR_DATA_MASK			((0x1 << EMC_MRR_MA_SHIFT) - 1)
#define LPDDR2_MR4_TEMP_SHIFT			0
#define LPDDR2_MR4_TEMP_MASK			(0x7 << LPDDR2_MR4_TEMP_SHIFT)

#define EMC_CMDQ				0xf0
#define EMC_MC2EMCQ				0xf4
#define EMC_XM2DQSPADCTRL3			0xf8
#define EMC_XM2DQSPADCTRL3_VREF_ENABLE		(0x1 << 5)
#define EMC_FBIO_SPARE				0x100

#define EMC_FBIO_CFG5				0x104
#define EMC_CFG5_TYPE_SHIFT			0x0
#define EMC_CFG5_TYPE_MASK			(0x3 << EMC_CFG5_TYPE_SHIFT)
#define EMC_CFG5_QUSE_MODE_SHIFT		13
#define EMC_CFG5_QUSE_MODE_MASK			\
	(0x7 << EMC_CFG5_QUSE_MODE_SHIFT)
enum {
	EMC_CFG5_QUSE_MODE_NORMAL = 0,
	EMC_CFG5_QUSE_MODE_ALWAYS_ON,
	EMC_CFG5_QUSE_MODE_INTERNAL_LPBK,
	EMC_CFG5_QUSE_MODE_PULSE_INTERN,
	EMC_CFG5_QUSE_MODE_PULSE_EXTERN,
	EMC_CFG5_QUSE_MODE_DIRECT_QUSE,
};

#define EMC_FBIO_WRPTR_EQ_2			0x108
#define EMC_FBIO_CFG6				0x114
#define EMC_CFG_RSV				0x120
#define EMC_ACPD_CONTROL			0x124
#define EMC_EMRS2				0x12c
#define EMC_EMRS3				0x130
#define EMC_MRW2				0x134
#define EMC_MRW3				0x138
#define EMC_MRW4				0x13c
#define EMC_CLKEN_OVERRIDE			0x140
#define EMC_R2R					0x144
#define EMC_W2W					0x148
#define EMC_EINPUT				0x14c
#define EMC_EINPUT_DURATION			0x150
#define EMC_PUTERM_EXTRA			0x154
#define EMC_TCKESR				0x158
#define EMC_TPD					0x15c

#define EMC_AUTO_CAL_CONFIG			0x2a4
#define EMC_AUTO_CAL_CONFIG_AUTO_CAL_START_SHIFT 31
#define EMC_AUTO_CAL_INTERVAL			0x2a8
#define EMC_AUTO_CAL_STATUS			0x2ac
#define EMC_AUTO_CAL_STATUS_ACTIVE		(0x1 << 31)
#define EMC_AUTO_CAL_STATUS_SHIFT	31
#define EMC_REQ_CTRL				0x2b0
#define EMC_STATUS				0x2b4
#define EMC_STATUS_TIMING_UPDATE_STALLED	(0x1 << 23)
#define EMC_STATUS_MRR_DIVLD			(0x1 << 20)

#define EMC_CFG_2				0x2b8
#define EMC_CFG_2_MODE_SHIFT			0
#define EMC_CFG_2_MODE_MASK			(0x3 << EMC_CFG_2_MODE_SHIFT)
#define EMC_CFG_2_SREF_MODE			0x1
#define EMC_CFG_2_PD_MODE			0x3
#define EMC_CFG_2_DIS_STP_OB_CLK_DURING_NON_WR (0x1 << 6)

#define EMC_CFG_DIG_DLL				0x2bc
#define EMC_CFG_DIG_DLL_PERIOD			0x2c0
#define EMC_DIG_DLL_STATUS			0x2c8
#define EMC_RDV_MASK				0x2cc
#define EMC_WDV_MASK				0x2d0
#define EMC_CTT_DURATION			0x2d8
#define EMC_CTT_TERM_CTRL			0x2dc
#define EMC_ZCAL_INTERVAL			0x2e0
#define EMC_ZCAL_WAIT_CNT			0x2e4
#define EMC_ZCAL_MRW_CMD			0x2e8

#define EMC_ZQ_CAL				0x2ec
#define EMC_ZQ_CAL_DEV_SEL_SHIFT		30
#define EMC_ZQ_CAL_DEV_SEL_MASK			\
	(0x3 << EMC_SELF_REF_DEV_SEL_SHIFT)
#define EMC_ZQ_CAL_CMD				(0x1 << 0)
#define EMC_ZQ_CAL_LONG				(0x1 << 4)
#define EMC_ZQ_CAL_LONG_CMD_DEV0		\
	(DRAM_DEV_SEL_0 | EMC_ZQ_CAL_LONG | EMC_ZQ_CAL_CMD)
#define EMC_ZQ_CAL_LONG_CMD_DEV1		\
	(DRAM_DEV_SEL_1 | EMC_ZQ_CAL_LONG | EMC_ZQ_CAL_CMD)

#define EMC_XM2CMDPADCTRL			0x2f0
#define EMC_XM2CMDPADCTRL2			0x2f4
#define EMC_XM2DQSPADCTRL			0x2f8
#define EMC_XM2DQSPADCTRL2			0x2fc
#define EMC_XM2DQSPADCTRL2_RX_FT_REC_ENABLE	(0x1 << 0)
#define EMC_XM2DQSPADCTRL2_VREF_ENABLE		(0x1 << 5)
#define EMC_XM2DQPADCTRL			0x300
#define EMC_XM2DQPADCTRL2			0x304
#define EMC_XM2CLKPADCTRL			0x308
#define EMC_XM2COMPPADCTRL			0x30c
#define EMC_XM2COMPPADCTRL_VREF_CAL_ENABLE	(0x1 << 10)
#define EMC_XM2VTTGENPADCTRL			0x310
#define EMC_XM2VTTGENPADCTRL2			0x314
#define EMC_XM2VTTGENPADCTRL3			0x318
#define EMC_EMCPADEN				0x31c
#define EMC_XM2DQSPADCTRL4			0x320
#define EMC_SCRATCH0				0x324
#define EMC_DLL_XFORM_DQS0			0x328
#define EMC_DLL_XFORM_DQS1			0x32c
#define EMC_DLL_XFORM_DQS2			0x330
#define EMC_DLL_XFORM_DQS3			0x334
#define EMC_DLL_XFORM_DQS4			0x338
#define EMC_DLL_XFORM_DQS5			0x33c
#define EMC_DLL_XFORM_DQS6			0x340
#define EMC_DLL_XFORM_DQS7			0x344
#define EMC_DLL_XFORM_QUSE0			0x348
#define EMC_DLL_XFORM_QUSE1			0x34c
#define EMC_DLL_XFORM_QUSE2			0x350
#define EMC_DLL_XFORM_QUSE3			0x354
#define EMC_DLL_XFORM_QUSE4			0x358
#define EMC_DLL_XFORM_QUSE5			0x35c
#define EMC_DLL_XFORM_QUSE6			0x360
#define EMC_DLL_XFORM_QUSE7			0x364
#define EMC_DLL_XFORM_DQ0			0x368
#define EMC_DLL_XFORM_DQ1			0x36c
#define EMC_DLL_XFORM_DQ2			0x370
#define EMC_DLL_XFORM_DQ3			0x374
#define EMC_DLI_RX_TRIM0			0x378
#define EMC_DLI_RX_TRIM1			0x37c
#define EMC_DLI_RX_TRIM2			0x380
#define EMC_DLI_RX_TRIM3			0x384
#define EMC_DLI_RX_TRIM4			0x388
#define EMC_DLI_RX_TRIM5			0x38c
#define EMC_DLI_RX_TRIM6			0x390
#define EMC_DLI_RX_TRIM7			0x394
#define EMC_DLI_TX_TRIM0			0x398
#define EMC_DLI_TX_TRIM1			0x39c
#define EMC_DLI_TX_TRIM2			0x3a0
#define EMC_DLI_TX_TRIM3			0x3a4
#define EMC_DLI_TRIM_TXDQS0			0x3a8
#define EMC_DLI_TRIM_TXDQS1			0x3ac
#define EMC_DLI_TRIM_TXDQS2			0x3b0
#define EMC_DLI_TRIM_TXDQS3			0x3b4
#define EMC_DLI_TRIM_TXDQS4			0x3b8
#define EMC_DLI_TRIM_TXDQS5			0x3bc
#define EMC_DLI_TRIM_TXDQS6			0x3c0
#define EMC_DLI_TRIM_TXDQS7			0x3c4
#define EMC_STALL_THEN_EXE_AFTER_CLKCHANGE	0x3cc
#define EMC_AUTO_CAL_CLK_STATUS			0x3d4
#define EMC_SEL_DPD_CTRL			0x3d8
#define EMC_SEL_DPD_CTRL_DATA_SEL_DPD	(0x1 << 8)
#define EMC_SEL_DPD_CTRL_ODT_SEL_DPD	(0x1 << 5)
#define EMC_SEL_DPD_CTRL_RESET_SEL_DPD	(0x1 << 4)
#define EMC_SEL_DPD_CTRL_CA_SEL_DPD	(0x1 << 3)
#define EMC_SEL_DPD_CTRL_CLK_SEL_DPD	(0x1 << 2)
#define EMC_SEL_DPD_CTRL_DDR3_MASK	\
	((0xf << 2) | (0x1 << 8))
#define EMC_SEL_DPD_CTRL_MASK \
	((0x3 << 2) | (0x1 << 5) | (0x1 << 8))
#define EMC_PRE_REFRESH_REQ_CNT			0x3dc
#define EMC_DYN_SELF_REF_CONTROL		0x3e0
#define EMC_TXSRDLL				0x3e4
#define EMC_CCFIFO_ADDR				0x3e8
#define EMC_CCFIFO_DATA				0x3ec
#define EMC_CCFIFO_STATUS			0x3f0
#define EMC_CDB_CNTL_1				0x3f4
#define EMC_CDB_CNTL_2				0x3f8
#define EMC_XM2CLKPADCTRL2			0x3fc
#define EMC_SWIZZLE_RANK0_BYTE_CFG		0x400
#define EMC_SWIZZLE_RANK0_BYTE0			0x404
#define EMC_SWIZZLE_RANK0_BYTE1			0x408
#define EMC_SWIZZLE_RANK0_BYTE2			0x40c
#define EMC_SWIZZLE_RANK0_BYTE3			0x410
#define EMC_SWIZZLE_RANK1_BYTE_CFG		0x414
#define EMC_SWIZZLE_RANK1_BYTE0			0x418
#define EMC_SWIZZLE_RANK1_BYTE1			0x41c
#define EMC_SWIZZLE_RANK1_BYTE2			0x420
#define EMC_SWIZZLE_RANK1_BYTE3			0x424
#define EMC_CA_TRAINING_START			0x428
#define EMC_CA_TRAINING_BUSY			0x42c
#define EMC_CA_TRAINING_CFG			0x430
#define EMC_CA_TRAINING_TIMING_CNTL1		0x434
#define EMC_CA_TRAINING_TIMING_CNTL2		0x438
#define EMC_CA_TRAINING_CA_LEAD_IN		0x43c
#define EMC_CA_TRAINING_CA			0x440
#define EMC_CA_TRAINING_CA_LEAD_OUT		0x444
#define EMC_CA_TRAINING_RESULT1			0x448
#define EMC_CA_TRAINING_RESULT2			0x44c
#define EMC_CA_TRAINING_RESULT3			0x450
#define EMC_CA_TRAINING_RESULT4			0x454
#define EMC_AUTO_CAL_CONFIG2			0x458
#define EMC_AUTO_CAL_CONFIG3			0x45c
#define EMC_AUTO_CAL_STATUS2			0x460
#define EMC_XM2CMDPADCTRL3			0x464
#define EMC_IBDLY				0x468
#define EMC_DLL_XFORM_ADDR0			0x46c
#define EMC_DLL_XFORM_ADDR1			0x470
#define EMC_DLL_XFORM_ADDR2			0x474
#define EMC_DLI_ADDR_TRIM			0x478
#define EMC_DSR_VTTGEN_DRV			0x47c
#define EMC_TXDSRVTTGEN				0x480
#define EMC_XM2CMDPADCTRL4			0x484
#define EMC_XM2CMDPADCTRL5			0x488
#define EMC_DLL_XFORM_DQS8			0x4a0
#define EMC_DLL_XFORM_DQS9			0x4a4
#define EMC_DLL_XFORM_DQS10			0x4a8
#define EMC_DLL_XFORM_DQS11			0x4ac
#define EMC_DLL_XFORM_DQS12			0x4b0
#define EMC_DLL_XFORM_DQS13			0x4b4
#define EMC_DLL_XFORM_DQS14			0x4b8
#define EMC_DLL_XFORM_DQS15			0x4bc
#define EMC_DLL_XFORM_QUSE8			0x4c0
#define EMC_DLL_XFORM_QUSE9			0x4c4
#define EMC_DLL_XFORM_QUSE10			0x4c8
#define EMC_DLL_XFORM_QUSE11			0x4cc
#define EMC_DLL_XFORM_QUSE12			0x4d0
#define EMC_DLL_XFORM_QUSE13			0x4d4
#define EMC_DLL_XFORM_QUSE14			0x4d8
#define EMC_DLL_XFORM_QUSE15			0x4dc
#define EMC_DLL_XFORM_DQ4				0x4e0
#define EMC_DLL_XFORM_DQ5				0x4e4
#define EMC_DLL_XFORM_DQ6				0x4e8
#define EMC_DLL_XFORM_DQ7				0x4ec
#define EMC_DLI_TRIM_TXDQS8				0x520
#define EMC_DLI_TRIM_TXDQS9				0x524
#define EMC_DLI_TRIM_TXDQS10				0x528
#define EMC_DLI_TRIM_TXDQS11				0x52c
#define EMC_DLI_TRIM_TXDQS12				0x530
#define EMC_DLI_TRIM_TXDQS13				0x534
#define EMC_DLI_TRIM_TXDQS14				0x538
#define EMC_DLI_TRIM_TXDQS15				0x53c
#define EMC_CDB_CNTL_3				0x540
#define EMC_XM2DQSPADCTRL5			0x544
#define EMC_XM2DQSPADCTRL6			0x548
#define EMC_XM2DQPADCTRL3			0x54c
#define EMC_DLL_XFORM_ADDR3			0x550
#define EMC_DLL_XFORM_ADDR4			0x554
#define EMC_DLL_XFORM_ADDR5			0x558
#define EMC_CFG_PIPE				0x560
#define EMC_QPOP					0x564
#define EMC_QUSE_WIDTH				0x568
#define EMC_PUTERM_WIDTH			0x56c
#define EMC_BGBIAS_CTL0				0x570
#define EMC_BGBIAS_CTL0_BIAS0_DSC_E_PWRD_IBIAS_RX_SHIFT	0x3
#define EMC_BGBIAS_CTL0_BIAS0_DSC_E_PWRD_IBIAS_VTTGEN_SHIFT	0x2
#define EMC_BGBIAS_CTL0_BIAS0_DSC_E_PWRD_SHIFT	0x1
#define EMC_PUTERM_ADJ				0x574

#define MC_EMEM_CFG				0x50
#define MC_EMEM_ADR_CFG				0x54
#define MC_EMEM_ADR_CFG_DEV0			0x58
#define MC_EMEM_ADR_CFG_DEV1			0x5c
#define MC_EMEM_ADR_CFG_BANK_MASK_0		0x64
#define MC_EMEM_ADR_CFG_BANK_MASK_1		0x68
#define MC_EMEM_ADR_CFG_BANK_MASK_2		0x6c

#define MC_EMEM_ARB_CFG				0x90
#define MC_EMEM_ARB_OUTSTANDING_REQ		0x94
#define MC_EMEM_ARB_TIMING_RCD			0x98
#define MC_EMEM_ARB_TIMING_RP			0x9c
#define MC_EMEM_ARB_TIMING_RC			0xa0
#define MC_EMEM_ARB_TIMING_RAS			0xa4
#define MC_EMEM_ARB_TIMING_FAW			0xa8
#define MC_EMEM_ARB_TIMING_RRD			0xac
#define MC_EMEM_ARB_TIMING_RAP2PRE		0xb0
#define MC_EMEM_ARB_TIMING_WAP2PRE		0xb4
#define MC_EMEM_ARB_TIMING_R2R			0xb8
#define MC_EMEM_ARB_TIMING_W2W			0xbc
#define MC_EMEM_ARB_TIMING_R2W			0xc0
#define MC_EMEM_ARB_TIMING_W2R			0xc4
#define MC_EMEM_ARB_DA_TURNS			0xd0
#define MC_EMEM_ARB_DA_COVERS			0xd4
#define MC_EMEM_ARB_MISC0			0xd8
#define MC_EMEM_ARB_MISC0_EMC_SAME_FREQ		(0x1 << 27)
#define MC_EMEM_ARB_MISC1			0xdc
#define MC_EMEM_ARB_RING1_THROTTLE		0xe0
#define MC_EMEM_ARB_RING3_THROTTLE		0xe4
#define MC_EMEM_ARB_OVERRIDE			0xe8
#define MC_EMEM_ARB_RSV				0xec

#define MC_CLKEN_OVERRIDE			0xf4
#define MC_TIMING_CONTROL_DBG			0xf8
#define MC_TIMING_CONTROL			0xfc
#define MC_EMEM_ARB_ISOCHRONOUS_0		0x208
#define MC_EMEM_ARB_ISOCHRONOUS_1		0x20c
#define MC_EMEM_ARB_ISOCHRONOUS_2		0x210
#define MC_EMEM_ARB_HYSTERESIS_0_0		0x218
#define MC_EMEM_ARB_HYSTERESIS_1_0		0x21c
#define MC_EMEM_ARB_HYSTERESIS_2_0		0x220
#define MC_EMEM_ARB_HYSTERESIS_3_0		0x224

#define HYST_SATAR				(0x1 << 31)
#define HYST_PPCSAHBSLVR			(0x1 << 30)
#define HYST_PPCSAHBDMAR			(0x1 << 29)
#define HYST_MSENCSRD				(0x1 << 28)
#define HYST_HOST1XR				(0x1 << 23)
#define HYST_HOST1XDMAR				(0x1 << 22)
#define HYST_HDAR				(0x1 << 21)
#define HYST_DISPLAYHCB				(0x1 << 17)
#define HYST_DISPLAYHC				(0x1 << 16)
#define HYST_AVPCARM7R				(0x1 << 15)
#define HYST_AFIR				(0x1 << 14)
#define HYST_DISPLAY0CB				(0x1 << 6)
#define HYST_DISPLAY0C				(0x1 << 5)
#define HYST_DISPLAY0BB				(0x1 << 4)
#define HYST_DISPLAY0B				(0x1 << 3)
#define HYST_DISPLAY0AB				(0x1 << 2)
#define HYST_DISPLAY0A				(0x1 << 1)
#define HYST_PTCR				(0x1 << 0)

#define HYST_VDEDBGW				(0x1 << 31)
#define HYST_VDEBSEVW				(0x1 << 30)
#define HYST_SATAW				(0x1 << 29)
#define HYST_PPCSAHBSLVW			(0x1 << 28)
#define HYST_PPCSAHBDMAW			(0x1 << 27)
#define HYST_MPCOREW				(0x1 << 25)
#define HYST_MPCORELPW				(0x1 << 24)
#define HYST_HOST1XW				(0x1 << 22)
#define HYST_HDAW				(0x1 << 21)
#define HYST_AVPCARM7W				(0x1 << 18)
#define HYST_AFIW				(0x1 << 17)
#define HYST_MSENCSWR				(0x1 << 11)
#define HYST_MPCORER				(0x1 << 7)
#define HYST_MPCORELPR				(0x1 << 6)
#define HYST_VDETPER				(0x1 << 5)
#define HYST_VDEMCER				(0x1 << 4)
#define HYST_VDEMBER				(0x1 << 3)
#define HYST_VDEBSEVR				(0x1 << 2)

#define HYST_DISPLAYT				(0x1 << 26)
#define HYST_GPUSWR				(0x1 << 25)
#define HYST_GPUSRD				(0x1 << 24)
#define HYST_A9AVPSCW				(0x1 << 23)
#define HYST_A9AVPSCR				(0x1 << 22)
#define HYST_TSECSWR				(0x1 << 21)
#define HYST_TSECSRD				(0x1 << 20)
#define HYST_ISPWBB				(0x1 << 17)
#define HYST_ISPWAB				(0x1 << 16)
#define HYST_ISPRAB				(0x1 << 14)
#define HYST_XUSB_DEVW				(0x1 << 13)
#define HYST_XUSB_DEVR				(0x1 << 12)
#define HYST_XUSB_HOSTW				(0x1 << 11)
#define HYST_XUSB_HOSTR				(0x1 << 10)
#define HYST_ISPWB				(0x1 << 7)
#define HYST_ISPWA				(0x1 << 6)
#define HYST_ISPRA				(0x1 << 4)
#define HYST_VDETPMW				(0x1 << 1)
#define HYST_VDEMBEW				(0x1 << 0)

#define HYST_DISPLAYD				(0x1 << 19)
#define HYST_VIW				(0x1 << 18)
#define HYST_VICSWR				(0x1 << 13)
#define HYST_VICSRD				(0x1 << 12)
#define HYST_SDMMCWAB				(0x1 << 7)
#define HYST_SDMMCW				(0x1 << 6)
#define HYST_SDMMCWAA				(0x1 << 5)
#define HYST_SDMMCWA				(0x1 << 4)
#define HYST_SDMMCRAB				(0x1 << 3)
#define HYST_SDMMCR				(0x1 << 2)
#define HYST_SDMMCRAA				(0x1 << 1)
#define HYST_SDMMCRA				(0x1 << 0)

#define MC_DIS_EXTRA_SNAP_LEVELS		0x2ac

#define MC_LATENCY_ALLOWANCE_AFI_0		0x2e0
#define MC_LATENCY_ALLOWANCE_AVPC_0		0x2e4
#define MC_LATENCY_ALLOWANCE_DC_0		0x2e8
#define MC_LATENCY_ALLOWANCE_DC_1		0x2ec
#define MC_LATENCY_ALLOWANCE_DC_2		0x2f0
#define MC_LATENCY_ALLOWANCE_DCB_0		0x2f4
#define MC_LATENCY_ALLOWANCE_DCB_1		0x2f8
#define MC_LATENCY_ALLOWANCE_DCB_2		0x2fc
#define MC_LATENCY_ALLOWANCE_HC_0		0x310
#define MC_LATENCY_ALLOWANCE_HC_1		0x314
#define MC_LATENCY_ALLOWANCE_HDA_0		0x318
#define MC_LATENCY_ALLOWANCE_MPCORE_0	0x320
#define MC_LATENCY_ALLOWANCE_MPCORELP_0	0x324
#define MC_LATENCY_ALLOWANCE_MSENC_0	0x328
#define MC_LATENCY_ALLOWANCE_PPCS_0		0x344
#define MC_LATENCY_ALLOWANCE_PPCS_1		0x348
#define MC_LATENCY_ALLOWANCE_PTC_0		0x34c
#define MC_LATENCY_ALLOWANCE_SATA_0		0x350
#define MC_LATENCY_ALLOWANCE_VDE_0		0x354
#define MC_LATENCY_ALLOWANCE_VDE_1		0x358
#define MC_LATENCY_ALLOWANCE_VDE_2		0x35c
#define MC_LATENCY_ALLOWANCE_VDE_3		0x360
#define MC_LATENCY_ALLOWANCE_ISP2_0		0x370
#define MC_LATENCY_ALLOWANCE_ISP2_1		0x374
#define MC_LATENCY_ALLOWANCE_XUSB_0		0x37c
#define MC_LATENCY_ALLOWANCE_XUSB_1		0x380
#define MC_LATENCY_ALLOWANCE_ISP2B_0	0x384
#define MC_LATENCY_ALLOWANCE_ISP2B_1	0x388
#define MC_LATENCY_ALLOWANCE_TSEC_0		0x390
#define MC_LATENCY_ALLOWANCE_VIC_0		0x394
#define MC_LATENCY_ALLOWANCE_VI2_0		0x398
#define MC_LATENCY_ALLOWANCE_A9AVP_0	0x3a4
#define MC_LATENCY_ALLOWANCE_GPU_0		0x3ac
#define MC_LATENCY_ALLOWANCE_SDMMCA_0	0x3b8
#define MC_LATENCY_ALLOWANCE_SDMMCAA_0	0x3bc
#define MC_LATENCY_ALLOWANCE_SDMMC_0	0x3c0
#define MC_LATENCY_ALLOWANCE_SDMMCAB_0	0x3c4
#define MC_LATENCY_ALLOWANCE_DC_3		0x3c8

/* NOTE: this must match the # of MC_LATENCY_XXX above */
#define T12X_MC_LATENCY_ALLOWANCE_NUM_REGS 38

#define MC_VIDEO_PROTECT_VPR_OVERRIDE		0x418
#define MC_MLL_MPCORER_PTSA_RATE			0x44c
#define MC_VIDEO_PROTECT_BOM			0x648
#define MC_VIDEO_PROTECT_SIZE_MB		0x64c
#define MC_VIDEO_PROTECT_REG_CTRL		0x650

#define MC_SEC_CARVEOUT_BOM			0x670
#define MC_SEC_CARVEOUT_SIZE_MB			0x674
#define MC_SEC_CARVEOUT_REG_CTRL		0x678

#define MC_PTSA_GRANT_DECREMENT			0x960

#define MC_RESERVED_RSV				0x3fc
#define MC_RESERVED_RSV_1			0x958

#define MC_EMEM_ARB_OUTSTANDING_REQ_RING3	0x66c
#define MC_EMEM_ARB_OVERRIDE_1			0x968

#endif
