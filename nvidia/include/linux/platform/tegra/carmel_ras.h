/*
 * Copyright (c) 2017-2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <linux/arm64_ras.h>

/* Error Records Per Core */
/* ERR_CTLR bits for IFU */
#define ERR_CTL_IFU_IMQDP_ERR		RAS_BIT(41)
#define ERR_CTL_IFU_L2UC_ERR		RAS_BIT(40)
#define ERR_CTL_IFU_ICTPSNP_ERR		RAS_BIT(38)
#define ERR_CTL_IFU_ICMHSNP_ERR		RAS_BIT(37)
#define ERR_CTL_IFU_ITLBP_ERR		RAS_BIT(36)
#define ERR_CTL_IFU_THERR_ERR		RAS_BIT(35)
#define ERR_CTL_IFU_ICDP_ERR		RAS_BIT(34)
#define ERR_CTL_IFU_ICTP_ERR		RAS_BIT(33)
#define ERR_CTL_IFU_ICMH_ERR		RAS_BIT(32)

/* ERR_CTLR bits for RET_JSR */
#define ERR_CTL_RET_JSR_FRFP_ERR	RAS_BIT(35)
#define ERR_CTL_RET_JSR_IRFP_ERR	RAS_BIT(34)
#define ERR_CTL_RET_JSR_GB_ERR		RAS_BIT(33)
#define ERR_CTL_RET_JSR_TO_ERR		RAS_BIT(32)

/* ERR_CTLR bits for MTS_JSR */
#define ERR_CTL_MTS_JSR_DUEXUC_ERR	RAS_BIT(39)
#define ERR_CTL_MTS_JSR_DUEXC_ERR	RAS_BIT(38)
#define ERR_CTL_MTS_JSR_MMIO_ERR	RAS_BIT(37)
#define ERR_CTL_MTS_JSR_CRAB_ERR	RAS_BIT(36)
#define ERR_CTL_MTS_JSR_CARVE_ERR	RAS_BIT(35)
#define ERR_CTL_MTS_JSR_NAFLL_ERR	RAS_BIT(34)
#define ERR_CTL_MTS_JSR_ERRC_ERR	RAS_BIT(33)
#define ERR_CTL_MTS_JSR_ERRUC_ERR	RAS_BIT(32)

/* ERR_CTLR bits for LSD_1/LSD_STQ */
#define ERR_CTL_LSD1_CCDSMLECC_ERR	RAS_BIT(41)
#define ERR_CTL_LSD1_CCDSECC_D_ERR	RAS_BIT(40)
#define ERR_CTL_LSD1_CCDSECC_S_ERR	RAS_BIT(39)
#define ERR_CTL_LSD1_CCDLECC_D_ERR	RAS_BIT(38)
#define ERR_CTL_LSD1_CCDLECC_S_ERR	RAS_BIT(37)
#define ERR_CTL_LSD1_CCMH_ERR		RAS_BIT(35)
#define ERR_CTL_LSD1_CCTSP_ERR		RAS_BIT(33)
#define ERR_CTL_LSD1_CCTLP_ERR		RAS_BIT(32)

/* ERR_CTLR bits for LSD_2/LSD_DCC */
#define ERR_CTL_LSD2_BTMCMH_ERR		RAS_BIT(41)
#define ERR_CTL_LSD2_CCDEECC_D_ERR	RAS_BIT(39)
#define ERR_CTL_LSD2_CCDEECC_S_ERR	RAS_BIT(38)
#define ERR_CTL_LSD2_VRCBP_ERR		RAS_BIT(37)
#define ERR_CTL_LSD2_VRCDECC_D_ERR	RAS_BIT(36)
#define ERR_CTL_LSD2_VRCDECC_S_ERR	RAS_BIT(35)
#define ERR_CTL_LSD2_BTCCPPP_ERR	RAS_BIT(33)
#define ERR_CTL_LSD2_BTCCVPP_ERR	RAS_BIT(32)

/* ERR_CTLR bits for LSD_3/LSD_L1HPF */
#define ERR_CTL_LSD3_L2TLBP_ERR		RAS_BIT(32)

/* Error records per CCPLEX */
/* ERR_CTLR bits for CMU:CCPMU or DPMU*/
#define ERR_CTL_DPMU_CORESIGHT_ACCESS_ERR RAS_BIT(40)
#define ERR_CTL_DPMU_DMCE_UCODE_ERR	RAS_BIT(36)
#define ERR_CTL_DPMU_DMCE_IL1_PAR_ERR	RAS_BIT(35)
#define ERR_CTL_DPMU_DMCE_TIMEOUT_ERR	RAS_BIT(34)
#define ERR_CTL_DPMU_CRAB_ACC_ERR	RAS_BIT(33)
#define ERR_CTL_DPMU_DMCE_CRAB_ACC_ERR	RAS_BIT(32)

/* ERR_CTLR bits for SCF:IOB*/
#define ERR_CTL_SCFIOB_REQ_PAR_ERR	RAS_BIT(41)
#define ERR_CTL_SCFIOB_PUT_PAR_ERR	RAS_BIT(40)
#define ERR_CTL_SCFIOB_PUT_UECC_ERR	RAS_BIT(39)
#define ERR_CTL_SCFIOB_CBB_ERR		RAS_BIT(38)
#define ERR_CTL_SCFIOB_MMCRAB_ERR	RAS_BIT(37)
#define ERR_CTL_SCFIOB_IHI_ERR		RAS_BIT(36)
#define ERR_CTL_SCFIOB_CRI_ERR		RAS_BIT(35)
#define ERR_CTL_SCFIOB_TBX_ERR		RAS_BIT(34)
#define ERR_CTL_SCFIOB_EVP_ERR		RAS_BIT(33)
#define ERR_CTL_SCFIOB_PUT_CECC_ERR	RAS_BIT(32)
#define ERRX_SCFIOB			1025

/* ERR_CTLR bits for SCF:SNOC*/
#define ERR_CTL_SCFSNOC_MISC_RSP_ERR	RAS_BIT(42)
#define ERR_CTL_SCFSNOC_MISC_PAR_ERR	RAS_BIT(41)
#define ERR_CTL_SCFSNOC_MISC_UECC_ERR	RAS_BIT(40)
#define ERR_CTL_SCFSNOC_DVMU_PAR_ERR	RAS_BIT(39)
#define ERR_CTL_SCFSNOC_DVMU_TO_ERR	RAS_BIT(38)
#define ERR_CTL_SCFSNOC_CPE_REQ_ERR	RAS_BIT(37)
#define ERR_CTL_SCFSNOC_CPE_RSP_ERR	RAS_BIT(36)
#define ERR_CTL_SCFSNOC_CPE_TO_ERR	RAS_BIT(35)
#define ERR_CTL_SCFSNOC_CARVEOUT_ERR	RAS_BIT(34)
#define ERR_CTL_SCFSNOC_MISC_CECC_ERR	RAS_BIT(33)
#define ERR_CTL_SCFSNOC_CARVEOUT_CECC_ERR	RAS_BIT(32)

/* ERR_CTLR bits for SCF:CTU*/
#define ERR_CTL_CMUCTU_TRCDMA_REQ_ERR	RAS_BIT(39)
#define ERR_CTL_CMUCTU_CTU_SNP_ERR	RAS_BIT(38)
#define ERR_CTL_CMUCTU_TAG_PAR_ERR	RAS_BIT(37)
#define ERR_CTL_CMUCTU_CTU_DATA_PAR_ERR	RAS_BIT(36)
#define ERR_CTL_CMUCTU_RSP_PAR_ERR	RAS_BIT(35)
#define ERR_CTL_CMUCTU_TRL_PAR_ERR	RAS_BIT(34)
#define ERR_CTL_CMUCTU_MCF_PAR_ERR	RAS_BIT(33)
#define ERR_CTL_CMUCTU_TRCDMA_PAR_ERR	RAS_BIT(32)

/* ERR_CTLR bits for SCF:L3_* */
#define ERR_CTL_SCFL3_CECC_ERR		RAS_BIT(44)
#define ERR_CTL_SCFL3_SNOC_INTFC_ERR	RAS_BIT(43)
#define ERR_CTL_SCFL3_MCF_INTFC_ERR	RAS_BIT(42)
#define ERR_CTL_SCFL3_TAG_ERR		RAS_BIT(41)
#define ERR_CTL_SCFL3_L2DIR_ERR		RAS_BIT(41)
#define ERR_CTL_SCFL3_DIR_PAR_ERR	RAS_BIT(40)
#define ERR_CTL_SCFL3_UECC_ERR		RAS_BIT(39)
#define ERR_CTL_SCFL3_MH_CAM_ERR	RAS_BIT(37)
#define ERR_CTL_SCFL3_MH_TAG_ERR	RAS_BIT(36)
#define ERR_CTL_SCFL3_UNSUPP_REQ_ERR	RAS_BIT(35)
#define ERR_CTL_SCFL3_PROT_ERR		RAS_BIT(34)
#define ERRX_SCFL3			768

/* ERR_CTLR bits for SCFCMU_CLOCKS */
#define ERR_CTL_SCFCMU_FREQ3_MON_ERR	RAS_BIT(39)
#define ERR_CTL_SCFCMU_FREQ2_MON_ERR	RAS_BIT(38)
#define ERR_CTL_SCFCMU_FREQ1_MON_ERR	RAS_BIT(37)
#define ERR_CTL_SCFCMU_FREQ0_MON_ERR	RAS_BIT(36)
#define ERR_CTL_SCFCMU_ADC1_MON_ERR	RAS_BIT(35)
#define ERR_CTL_SCFCMU_ADC0_MON_ERR	RAS_BIT(34)
#define ERR_CTL_SCFCMU_LUT1_PAR_ERR	RAS_BIT(33)
#define ERR_CTL_SCFCMU_LUT0_PAR_ERR	RAS_BIT(32)

/* Error records per Core Cluster */
/* ERR_CTLR bits for L2 */
#define ERR_CTL_L2_MLD_ECCC_ERR		RAS_BIT(32)
#define ERR_CTL_L2_URD_ECCC_ERR		RAS_BIT(33)
#define ERR_CTL_L2_MLD_ECCUD_ERR	RAS_BIT(34)
#define ERR_CTL_L2_URD_ECCU_ERR		RAS_BIT(35)
#define ERR_CTL_L2_MLD_ECCUC_ERR	RAS_BIT(36)
#define ERR_CTL_L2_NTDP_ERR		RAS_BIT(38)
#define ERR_CTL_L2_URDP			RAS_BIT(39)
#define ERR_CTL_L2_MLTP_ERR		RAS_BIT(40)
#define ERR_CTL_L2_NTTP_ERR		RAS_BIT(41)
#define ERR_CTL_L2_URTP_ERR		RAS_BIT(42)
#define ERR_CTL_L2_L2MH_ERR		RAS_BIT(43)
#define ERR_CTL_L2_CORE02L2CP_ERR	RAS_BIT(44)
#define ERR_CTL_L2_CORE12L2CP_ERR	RAS_BIT(45)
#define ERR_CTL_L2_SCF2L2C_ECCC_ERR	RAS_BIT(46)
#define ERR_CTL_L2_SCF2L2C_ECCU_ERR	RAS_BIT(47)
#define ERR_CTL_L2_SCF2L2C_FILLDATAP_ERR	RAS_BIT(48)
#define ERR_CTL_L2_SCF2L2C_ADVNOTP_ERR	RAS_BIT(49)
#define ERR_CTL_L2_SCF2L2C_REQRSPP_ERR	RAS_BIT(50)
#define ERR_CTL_L2_SCF2L2C_DECWTERR_ERR	RAS_BIT(51)
#define ERR_CTL_L2_SCF2L2C_DECRDERR_ERR	RAS_BIT(52)
#define ERR_CTL_L2_SCF2L2C_SLVWTERR_ERR	RAS_BIT(53)
#define ERR_CTL_L2_SCF2L2C_SLVRDERR_ERR	RAS_BIT(54)
#define ERR_CTL_L2_L2PCL_ERR	RAS_BIT(55)
#define ERR_CTL_L2_URTTO_ERR	RAS_BIT(56)

/* ERR_CTLR bits for MMU */
#define ERR_CTL_MMU_WCPERR_ERR		RAS_BIT(33)
#define ERR_CTL_MMU_ACPERR_ERR		RAS_BIT(32)

/* ERR_CTLR bits for CLUSTER_CLOCKS */
#define ERR_CTL_CC_FREQ_MON_ERR		RAS_BIT(32)

/* Constants used by ras_trip */
#define ERRi_MISC0_CONST		0x2222222222222222UL
#define ERRi_MISC1_CONST		0x3333333333333333UL
#define ERRi_ADDR_CONST			0x4444444444444444UL

/* [64:32] bits in ERR<n>CTLR define the errors supported by that node.
 */
#define DEFAULT_ERR_CTLR_MASK	(0xFFFFFFFF00000000UL | RAS_CTL_ED |\
			RAS_CTL_UE | RAS_CTL_CFI)


enum {
	IFU,		/* 0 */
	JSR_RET,	/* 1 */
	JSR_MTS,	/* 2 */
	LSD_STQ,	/* 3 */
	LSD_DCC,	/* 4 */
	LSD_L1HPF,	/* 5 */
	L2,		/* 6 */
	Cluster_Clocks,	/* 7 */
	MMU,		/* 8 */
	L3,		/* 9 */
	CCPMU,		/* A */
	SCF_IOB,	/* B */
	SCF_SNOC,	/* C */
	SCF_CTU,	/* D */
	CMU_Clocks,	/* E */
};

struct tegra_ras_impl_err_bit {
	u64     uncorr_bit;
	u64     corr_bit;
};

/**
 * struct carmel_error_record - Platform specific error record inheriting the
 *				generic arm64 ras error record.
 * @err_ctlr_mask: Implementation defined field of ERR<n>CTLR field of a node
 *		holds info regarding which errors are valid in carmel.
 *		Typically it is the higher 32 bits, but in certain
 *		cases, it might be different.
 *		This mask can also be modified to disable
 *		certain errors at the driver's discretion.
 * @rec:	This is the arm64_ras error record that is inherited.
 *		See arm64_ras.h for the structure description.
 */
struct carmel_error_record {
	u64 err_ctlr_mask;
	struct error_record rec;
};
