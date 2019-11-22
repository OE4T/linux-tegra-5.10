/*
 * Copyright (c) 2020, NVIDIA CORPORATION. All rights reserved.
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

#ifndef MGBE_CORE_H_
#define MGBE_CORE_H_

/**
 * @addtogroup - MGBE-LPI LPI configuration macros
 *
 * @brief LPI timers and config register field masks.
 * @{
 */
/* LPI LS timer - minimum time (in milliseconds) for which the link status from
 * PHY should be up before the LPI pattern can be transmitted to the PHY.
 * Default 1sec.
 */
#define MGBE_DEFAULT_LPI_LS_TIMER	(unsigned int)1000
#define MGBE_LPI_LS_TIMER_MASK		0x3FFU
#define MGBE_LPI_LS_TIMER_SHIFT		16U
/* LPI TW timer - minimum time (in microseconds) for which MAC wait after it
 * stops transmitting LPI pattern before resuming normal tx.
 * Default 21us
 */
#define MGBE_DEFAULT_LPI_TW_TIMER	0x15U
#define MGBE_LPI_TW_TIMER_MASK		0xFFFFU
/* LPI entry timer - Time in microseconds that MAC will wait to enter LPI mode
 * after all tx is complete.
 * Default 1sec.
 */
#define MGBE_LPI_ENTRY_TIMER_MASK	0xFFFF8U
/* 1US TIC counter - This counter should be programmed with the number of clock
 * cycles of CSR clock that constitutes a period of 1us.
 * it should be APB clock in MHZ i.e 408-1 for silicon and 13MHZ-1 for uFPGA
 */
#define MGBE_1US_TIC_COUNTER		0xC
/** @} */

/**
 * @addtogroup MGBE-MAC MAC register offsets
 *
 * @brief MGBE MAC register offsets
 * @{
 */
#define MGBE_MAC_TMCR			0x0000
#define MGBE_MAC_RMCR			0x0004
#define MGBE_MAC_PFR			0x0008
#define MGBE_MAC_HTR_REG(x)		((0x0004U * (x)) + 0x0010U)
#define MGBE_MAC_VLAN_TR		0x0050
#define MGBE_MAC_VLANTIR		0x0060
#define MGBE_MAC_QX_TX_FLW_CTRL(x)	((0x0004U * (x)) + 0x0070U)
#define MGBE_MAC_RX_FLW_CTRL		0x0090
#define MGBE_MAC_RQC0R			0x00A0
#define MGBE_MAC_RQC1R			0x00A4
#define MGBE_MAC_RQC2R			0x00A8
#define MGBE_MAC_ISR			0x00B0
#define MGBE_MAC_IER			0x00B4
#define MGBE_MAC_PMTCSR			0x00C0
#define MGBE_MAC_LPI_CSR		0x00D0
#define MGBE_MAC_LPI_TIMER_CTRL		0x00D4
#define MGBE_MAC_LPI_EN_TIMER		0x00D8
#define MGBE_MAC_1US_TIC_COUNT		0x00DC
#define MGBE_MDIO_SCCD			0x0204
#define MGBE_MDIO_SCCA			0x0200
#define MGBE_MAC_MA0HR			0x0300
#define MGBE_MAC_ADDRH(x)		((0x0008U * (x)) + 0x0300U)
#define MGBE_MAC_MA0LR			0x0304
#define MGBE_MAC_ADDRL(x)		((0x0008U * (x)) + 0x0304U)
#define MGBE_MMC_TX_INTR_EN		0x0810
#define MGBE_MMC_RX_INTR_EN		0x080C
#define MGBE_MMC_CNTRL			0x0800
#define MGBE_MAC_TCR			0x0D00
#define MGBE_MAC_SSIR			0x0D04
#define MGBE_MAC_STSR			0x0D08
#define MGBE_MAC_STNSR			0x0D0C
#define MGBE_MAC_STSUR			0x0D10
#define MGBE_MAC_STNSUR			0x0D14
#define MGBE_MAC_TAR			0x0D18
#define MGBE_MAC_ARPPA			0x0c10
#define MGBE_MAC_L3L4_ADDR_CTR		0x0C00
#define MGBE_MAC_L3L4_DATA		0x0C04
/** @} */

/**
 * @addtogroup MGBE MAC hash table defines
 *
 * @brief MGBE MAC hash table Control register
 * filed type defines.
 * @{
 */
#define MGBE_MAX_HTR_REGS		4U
/** @} */

/**
 * @addtogroup MGBE MAC L3L4 defines
 *
 * @brief MGBE L3L4 Address Control register
 * IDDR filter filed type defines
 * @{
 */
#define MGBE_MAX_VLAN_FILTER		32U
#define MGBE_MAC_XB_WAIT		10U
#define MGBE_MAC_L3L4_CTR		0x0
#define MGBE_MAC_L4_ADDR		0x1
#define MGBE_MAC_L3_AD0R		0x4
#define MGBE_MAC_L3_AD1R		0x5
#define MGBE_MAC_L3_AD2R		0x6
#define MGBE_MAC_L3_AD3R		0x7

#define MGBE_MAC_L3L4_CTR_DMCHEN0	OSI_BIT(31)
#define MGBE_MAC_L3L4_CTR_DMCHEN0_SHIFT	31
#define MGBE_MAC_L3L4_CTR_DMCHN0	(OSI_BIT(24) | OSI_BIT(25) | \
					 OSI_BIT(26) | OSI_BIT(27))
#define MGBE_MAC_L3L4_CTR_DMCHN0_SHIFT	24
#define MGBE_MAC_L3L4_CTR_L4DPIM0	OSI_BIT(21)
#define MGBE_MAC_L3L4_CTR_L4DPIM0_SHIFT	21
#define MGBE_MAC_L3L4_CTR_L4DPM0	OSI_BIT(20)
#define MGBE_MAC_L3L4_CTR_L4SPIM0	OSI_BIT(19)
#define MGBE_MAC_L3L4_CTR_L4SPIM0_SHIFT	19
#define MGBE_MAC_L3L4_CTR_L4SPM0	OSI_BIT(18)
#define MGBE_MAC_L3L4_CTR_L4PEN0	OSI_BIT(16)
#define MGBE_MAC_L3L4_CTR_L3HDBM0	(OSI_BIT(11) | OSI_BIT(12) | \
					 OSI_BIT(13) | OSI_BIT(14) | \
					 OSI_BIT(15))
#define MGBE_MAC_L3L4_CTR_L3HSBM0	(OSI_BIT(6) | OSI_BIT(7) | \
					 OSI_BIT(8) | OSI_BIT(9) | \
					 OSI_BIT(10))
#define MGBE_MAC_L3L4_CTR_L3DAIM0	OSI_BIT(5)
#define MGBE_MAC_L3L4_CTR_L3DAIM0_SHIFT	5
#define MGBE_MAC_L3L4_CTR_L3DAM0	OSI_BIT(4)
#define MGBE_MAC_L3L4_CTR_L3SAIM0	OSI_BIT(3)
#define MGBE_MAC_L3L4_CTR_L3SAIM0_SHIFT	3
#define MGBE_MAC_L3L4_CTR_L3SAM0	OSI_BIT(2)
#define MGBE_MAC_L3L4_CTR_L3PEN0	OSI_BIT(0)
#define MGBE_MAC_L3_IP6_CTRL_CLEAR	(MGBE_MAC_L3L4_CTR_L3SAM0  | \
					 MGBE_MAC_L3L4_CTR_L3SAIM0 | \
					 MGBE_MAC_L3L4_CTR_L3DAM0  | \
					 MGBE_MAC_L3L4_CTR_L3DAIM0 | \
					 MGBE_MAC_L3L4_CTR_DMCHEN0 | \
					 MGBE_MAC_L3L4_CTR_DMCHN0)
#define MGBE_MAC_L3_IP4_SA_CTRL_CLEAR	(MGBE_MAC_L3L4_CTR_L3SAM0  | \
					 MGBE_MAC_L3L4_CTR_L3SAIM0 | \
					 MGBE_MAC_L3L4_CTR_DMCHEN0 | \
					 MGBE_MAC_L3L4_CTR_DMCHN0)
#define MGBE_MAC_L3_IP4_DA_CTRL_CLEAR	(MGBE_MAC_L3L4_CTR_L3DAM0  | \
					 MGBE_MAC_L3L4_CTR_L3DAIM0 | \
					 MGBE_MAC_L3L4_CTR_DMCHEN0 | \
					 MGBE_MAC_L3L4_CTR_DMCHN0)
#define MGBE_MAC_L4_SP_CTRL_CLEAR	(MGBE_MAC_L3L4_CTR_L4SPM0  | \
					 MGBE_MAC_L3L4_CTR_L4SPIM0 | \
					 MGBE_MAC_L3L4_CTR_DMCHEN0 | \
					 MGBE_MAC_L3L4_CTR_DMCHN0)
#define MGBE_MAC_L4_DP_CTRL_CLEAR	(MGBE_MAC_L3L4_CTR_L4DPM0  | \
					 MGBE_MAC_L3L4_CTR_L4DPIM0 | \
					 MGBE_MAC_L3L4_CTR_DMCHEN0 | \
					 MGBE_MAC_L3L4_CTR_DMCHN0)
#define MGBE_MAC_L3L4_CTRL_ALL		(MGBE_MAC_L3_IP6_CTRL_CLEAR | \
					 MGBE_MAC_L3_IP4_SA_CTRL_CLEAR | \
					 MGBE_MAC_L3_IP4_DA_CTRL_CLEAR | \
					 MGBE_MAC_L4_SP_CTRL_CLEAR | \
					 MGBE_MAC_L4_DP_CTRL_CLEAR)
#define MGBE_MAC_L4_ADDR_SP_MASK	0x0000FFFFU
#define MGBE_MAC_L4_ADDR_DP_MASK	0xFFFF0000U
#define MGBE_MAC_L4_ADDR_DP_SHIFT	16
/** @} */

/**
 * @addtogroup MGBE-DMA DMA register offsets
 *
 * @brief MGBE DMA register offsets
 * @{
 */
#define MGBE_DMA_MODE			0x3000
#define MGBE_DMA_SBUS			0x3004
#define MGBE_DMA_ISR			0x3008
#define MGBE_DMA_CHX_STATUS(x)		((0x0080U * (x)) + 0x3160U)
#define MGBE_DMA_CHX_IER(x)		((0x0080U * (x)) + 0x3138U)
/** @} */

/**
 * @addtogroup MGBE-MTL MTL register offsets
 *
 * @brief MGBE MTL register offsets
 * @{
 */
#define MGBE_MTL_OP_MODE		0x1000
#define MGBE_MTL_RXQ_DMA_MAP0		0x1030
#define MGBE_MTL_RXQ_DMA_MAP1		0x1034
#define MGBE_MTL_RXQ_DMA_MAP2		0x1038
#define MGBE_MTL_RXQ_DMA_MAP3		0x103b
#define MGBE_MTL_CHX_TX_OP_MODE(x)	((0x0080U * (x)) + 0x1100U)
#define MGBE_MTL_TCQ_ETS_CR(x)		((0x0080U * (x)) + 0x1110U)
#define MGBE_MTL_TCQ_QW(x)		((0x0080U * (x)) + 0x1118U)
#define MGBE_MTL_TCQ_ETS_SSCR(x)	((0x0080U * (x)) + 0x111CU)
#define MGBE_MTL_TCQ_ETS_HCR(x)		((0x0080U * (x)) + 0x1120U)
#define MGBE_MTL_TCQ_ETS_LCR(x)		((0x0080U * (x)) + 0x1124U)
#define MGBE_MTL_CHX_RX_OP_MODE(x)	((0x0080U * (x)) + 0x1140U)
#define MGBE_MTL_RXQ_FLOW_CTRL(x)	((0x0080U * (x)) + 0x1150U)
#define MGBE_MTL_TC_PRTY_MAP0		0x1040
#define MGBE_MTL_TC_PRTY_MAP1		0x1044
/** @} */


/**
 * @addtogroup HW Register BIT values
 *
 * @brief consists of corresponding MGBE MAC, MTL register bit values
 * @{
 */
#define MGBE_DMA_MODE_SWR			OSI_BIT(0)
#define MGBE_MTL_TCQ_ETS_CR_SLC_MASK		(OSI_BIT(6) | OSI_BIT(5) | \
						 OSI_BIT(4))
#define MGBE_MTL_TCQ_ETS_CR_CC			OSI_BIT(3)
#define MGBE_MTL_TCQ_ETS_CR_CC_SHIFT		3U
#define MGBE_MTL_TCQ_ETS_CR_AVALG		(OSI_BIT(1) | OSI_BIT(0))
#define MGBE_MTL_TCQ_ETS_CR_AVALG_SHIFT		0U
#define MGBE_MTL_TCQ_ETS_QW_ISCQW_MASK		0x001FFFFFU
#define MGBE_MTL_TCQ_ETS_SSCR_SSC_MASK		0x0000FFFFU
#define MGBE_MTL_TCQ_ETS_HCR_HC_MASK		0x1FFFFFFFU
#define MGBE_MTL_TCQ_ETS_LCR_LC_MASK		0x1FFFFFFFU
#define MGBE_MTL_TX_OP_MODE_Q2TCMAP		(OSI_BIT(10) | OSI_BIT(9) |\
						 OSI_BIT(8))
#define MGBE_MTL_TX_OP_MODE_Q2TCMAP_SHIFT	8U
#define MGBE_MTL_TX_OP_MODE_TXQEN		(OSI_BIT(3) | OSI_BIT(2))
#define MGBE_MTL_TX_OP_MODE_TXQEN_SHIFT		2U
#define MGBE_MTL_QTOMR_FTQ			OSI_BIT(0)
#define MGBE_MTL_QTOMR_FTQ_LPOS			OSI_BIT(0)
#define MGBE_MTL_TSF				OSI_BIT(1)
#define MGBE_MTL_TXQEN				OSI_BIT(3)
#define MGBE_MTL_RSF				OSI_BIT(5)
#define MGBE_MTL_TCQ_QW_ISCQW                   OSI_BIT(4)
#define MGBE_MAC_RMCR_ACS			OSI_BIT(1)
#define MGBE_MAC_RMCR_CST			OSI_BIT(2)
#define MGBE_MAC_RMCR_IPC			OSI_BIT(9)
#define MGBE_MAC_RXQC0_RXQEN_MASK		0x3U
#define MGBE_MAC_RXQC0_RXQEN_SHIFT(x)		((x) * 2U)
#define MGBE_MAC_RMCR_LM			OSI_BIT(10)
#define MGBE_MAC_RMCR_ARPEN			OSI_BIT(31)
#define MGBE_MDIO_SCCD_SBUSY			OSI_BIT(22)
#define MGBE_MDIO_SCCA_DA_SHIFT			21U
#define MGBE_MDIO_SCCA_DA_MASK			0x1FU
#define MGBE_MDIO_C45_DA_SHIFT			16U
#define MGBE_MDIO_SCCA_PA_SHIFT			16U
#define MGBE_MDIO_SCCA_RA_MASK			0xFFFFU
#define MGBE_MDIO_SCCD_CMD_WR			1U
#define MGBE_MDIO_SCCD_CMD_RD			3U
#define MGBE_MDIO_SCCD_CMD_SHIFT		16U
#define MGBE_MDIO_SCCD_CR_SHIFT			19U
#define MGBE_MDIO_SCCD_CR_MASK			0x7U
#define MGBE_MDIO_SCCD_SDATA_MASK		0xFFFFU
#define MGBE_MDIO_SCCD_CRS			OSI_BIT(31)
#define MGBE_MAC_RMCR_GPSLCE			OSI_BIT(6)
#define MGBE_MAC_RMCR_WD			OSI_BIT(7)
#define MGBE_MAC_RMCR_JE			OSI_BIT(8)
#define MGBE_MAC_TMCR_JD			OSI_BIT(16)
#define MGBE_MMC_CNTRL_CNTRST			OSI_BIT(0)
#define MGBE_MMC_CNTRL_RSTONRD			OSI_BIT(2)
#define MGBE_MMC_CNTRL_CNTMCT			(OSI_BIT(4) | OSI_BIT(5))
#define MGBE_MMC_CNTRL_CNTPRST			OSI_BIT(7)
#define MGBE_MAC_RQC1R_MCBCQEN			OSI_BIT(15)
#define MGBE_MAC_RQC1R_MCBCQ1			OSI_BIT(8)
#define MGBE_IMR_RGSMIIIE			OSI_BIT(0)
#define MGBE_DMA_ISR_MACIS                      OSI_BIT(17)
#define MGBE_DMA_ISR_DCH0_DCH15_MASK		0xFFU
#define MGBE_DMA_CHX_STATUS_TPS			OSI_BIT(1)
#define MGBE_DMA_CHX_STATUS_TBU			OSI_BIT(2)
#define MGBE_DMA_CHX_STATUS_RBU			OSI_BIT(7)
#define MGBE_DMA_CHX_STATUS_RPS			OSI_BIT(8)
#define MGBE_DMA_CHX_STATUS_FBE			OSI_BIT(12)
#define MGBE_DMA_CHX_STATUS_TI			OSI_BIT(0)
#define MGBE_DMA_CHX_STATUS_RI			OSI_BIT(6)
#define MGBE_MAC_PFR_PR				OSI_BIT(0)
#define MGBE_MAC_PFR_HUC			OSI_BIT(1)
#define MGBE_MAC_PFR_DAIF			OSI_BIT(3)
#define MGBE_MAC_PFR_PM				OSI_BIT(4)
#define MGBE_MAC_PFR_DBF			OSI_BIT(5)
#define MGBE_MAC_PFR_PCF			(OSI_BIT(6) | OSI_BIT(7))
#define MGBE_MAC_PFR_SAIF			OSI_BIT(8)
#define MGBE_MAC_PFR_SAF			OSI_BIT(9)
#define MGBE_MAC_PFR_HPF			OSI_BIT(10)
#define MGBE_MAC_PFR_VTFE			OSI_BIT(16)
#define MGBE_MAC_PFR_VTFE_SHIFT			16
#define MGBE_MAC_PFR_IPFE			OSI_BIT(20)
#define MGBE_MAC_PFR_IPFE_SHIFT			20
#define MGBE_MAC_PFR_DNTU			OSI_BIT(21)
#define MGBE_MAC_PFR_VUCC			OSI_BIT(22)
#define MGBE_MAC_PFR_RA				OSI_BIT(31)
#define MGBE_MAC_ADDRH_AE			OSI_BIT(31)
#define MGBE_MAC_ADDRH_AE_SHIFT			31
#define MGBE_MAC_ADDRH_SA			OSI_BIT(30)
#define MGBE_MAC_ADDRH_SA_SHIFT			30
#define MGBE_MAB_ADDRH_MBC_MAX_MASK		0x3FU
#define MGBE_MAC_ADDRH_MBC			(OSI_BIT(29) | OSI_BIT(28) | \
						 OSI_BIT(27) | OSI_BIT(26) | \
						 OSI_BIT(25) | OSI_BIT(24))
#define MGBE_MAC_ADDRH_MBC_SHIFT		24
#define MGBE_MAC_ADDRH_DCS			(OSI_BIT(23) | OSI_BIT(22) | \
						 OSI_BIT(21) | OSI_BIT(20) | \
						 OSI_BIT(19) | OSI_BIT(18) | \
						 OSI_BIT(17) | OSI_BIT(16))
#define MGBE_MAC_ADDRH_DCS_SHIFT		16
#define MGBE_MAC_L3L4_ADDR_CTR_IDDR_FNUM	(OSI_BIT(12) | OSI_BIT(13) | \
						 OSI_BIT(14) | OSI_BIT(15))
#define MGBE_MAC_L3L4_ADDR_CTR_IDDR_FNUM_SHIFT	12
#define MGBE_MAC_L3L4_ADDR_CTR_IDDR_FTYPE	(OSI_BIT(8) | OSI_BIT(9) | \
						 OSI_BIT(10) | OSI_BIT(11))
#define MGBE_MAC_L3L4_ADDR_CTR_IDDR_FTYPE_SHIFT	8
#define MGBE_MAC_L3L4_ADDR_CTR_TT		OSI_BIT(1)
#define MGBE_MAC_L3L4_ADDR_CTR_XB		OSI_BIT(0)
#define MGBE_MAC_VLAN_TR_ETV			OSI_BIT(16)
#define MGBE_MAC_VLAN_TR_VTIM			OSI_BIT(17)
#define MGBE_MAC_VLAN_TR_VTIM_SHIFT		17
#define MGBE_MAC_VLAN_TR_VTHM			OSI_BIT(25)
#define MGBE_MAC_VLANTR_EVLS_ALWAYS_STRIP	((unsigned int)0x3 << 21U)
#define MGBE_MAC_VLANTR_EVLRXS			OSI_BIT(24)
#define MGBE_MAC_VLANTR_DOVLTC			OSI_BIT(20)
#define MGBE_MAC_VLANTIR_VLTI			OSI_BIT(20)
#define MGBE_MAC_VLANTIRR_CSVL			OSI_BIT(19)
#define MGBE_MAC_LPI_CSR_LPITE			OSI_BIT(20)
#define MGBE_MAC_LPI_CSR_LPITXA			OSI_BIT(19)
#define MGBE_MAC_LPI_CSR_PLS			OSI_BIT(17)
#define MGBE_MAC_LPI_CSR_LPIEN			OSI_BIT(16)

/* DMA SBUS */
#define MGBE_DMA_SBUS_BLEN8			OSI_BIT(2)
#define MGBE_DMA_SBUS_BLEN16			OSI_BIT(3)
#define MGBE_DMA_SBUS_EAME			OSI_BIT(11)
#define MGBE_DMA_SBUS_RD_OSR_LMT		0x001F0000U
#define MGBE_DMA_SBUS_WR_OSR_LMT		0x1F000000U
#define MGBE_MAC_TMCR_SS_2_5G			(OSI_BIT(31) | OSI_BIT(30))
#define MGBE_MAC_TMCR_SS_5G			(OSI_BIT(31) | OSI_BIT(29))
#define MGBE_MAC_TMCR_SS_10G			(OSI_BIT(31) | OSI_BIT(30) | OSI_BIT(29))
#define MGBE_MAC_TMCR_TE			OSI_BIT(0)
#define MGBE_MAC_RMCR_RE			OSI_BIT(0)
#define MGBE_MTL_TXQ_SIZE_SHIFT			16U
#define MGBE_MTL_RXQ_SIZE_SHIFT			16U
#define MGBE_RXQ_TO_DMA_CHAN_MAP0		0x03020100U
#define MGBE_RXQ_TO_DMA_CHAN_MAP1		0x07060504U
#define MGBE_RXQ_TO_DMA_CHAN_MAP2		0x0B0A0908U
#define MGBE_RXQ_TO_DMA_CHAN_MAP3		0x0F0E0D0CU
#define MGBE_MTL_TXQ_SIZE_SHIFT			16U
#define MGBE_MTL_RXQ_SIZE_SHIFT			16U
#define MGBE_MAC_RMCR_GPSL_MSK			0x3FFF0000U
#define MGBE_MTL_RXQ_OP_MODE_FEP		OSI_BIT(4)
#define MGBE_MAC_QX_TX_FLW_CTRL_TFE		OSI_BIT(1)
#define MGBE_MAC_RX_FLW_CTRL_RFE		OSI_BIT(0)
#define MGBE_MAC_PAUSE_TIME			0xFFFF0000U
#define MGBE_MAC_PAUSE_TIME_MASK		0xFFFF0000U
#define MGBE_MTL_RXQ_OP_MODE_EHFC		OSI_BIT(7)
#define MGBE_MTL_RXQ_OP_MODE_RFA_SHIFT		1U
#define MGBE_MTL_RXQ_OP_MODE_RFA_MASK		0x0000007EU
#define MGBE_MTL_RXQ_OP_MODE_RFD_SHIFT		17U
#define MGBE_MTL_RXQ_OP_MODE_RFD_MASK		0x007E0000U
/** @} */

/**
 * @addtogroup MGBE-QUEUE QUEUE fifo size programmable values
 *
 * @brief Queue FIFO size programmable values
 * @{
 */
/* Formula is "Programmed value = (x + 1 )*256"
 * Total Rx buf size is 192KB so 192*1024 = (x + 1)*256
 * which gives x as 0x2FF
 */
#define MGBE_19K	0x4BU  /* For Ten MTL queues */
#define MGBE_21K	0x53U  /* For Nine MTL queues */
#define MGBE_24K	0x5FU  /* For Eight MTL queues */
#define MGBE_27K	0x6BU  /* For Seven MTL queues */
#define MGBE_32K	0x7FU  /* For Six MTL queues */
#define MGBE_38K	0x97U  /* For Five MTL queues */
#define MGBE_48K	0xBFU  /* For Four MTL queues */
#define MGBE_64K	0xFFU  /* For Three MTL queues */
#define MGBE_96K	0x17FU /* For Two MTL queues */
#define MGBE_192K	0x2FFU /* For One MTL queue */
/** @} */

/**
 * @addtogroup MGBE-SIZE SIZE calculation helper Macros
 *
 * @brief SIZE calculation defines
 * @{
 */
#define FIFO_SIZE_B(x) (x)
#define FIFO_SIZE_KB(x) ((x) * 1024U)
/** @} */

/**
 * @addtogroup MGBE-HW-BACKUP
 *
 * @brief Definitions related to taking backup of MGBE core registers.
 * @{
 */

/* Hardware Register offsets to be backed up during suspend.
 *
 * Do not change the order of these macros. To add new registers to be
 * backed up, append to end of list before MGBE_MAX_MAC_BAK_IDX, and
 * update MGBE_MAX_MAC_BAK_IDX based on new macro.
 */
#define MGBE_MAC_TMCR_BAK_IDX		0U
#define MGBE_MAC_RMCR_BAK_IDX		((MGBE_MAC_TMCR_BAK_IDX + 1U))
#define MGBE_MAC_PFR_BAK_IDX		((MGBE_MAC_RMCR_BAK_IDX + 1U))
#define MGBE_MAC_VLAN_TAG_BAK_IDX	((MGBE_MAC_PFR_BAK_IDX + 1U))
#define MGBE_MAC_VLANTIR_BAK_IDX	((MGBE_MAC_VLAN_TAG_BAK_IDX + 1U))
#define MGBE_MAC_RX_FLW_CTRL_BAK_IDX	((MGBE_MAC_VLANTIR_BAK_IDX + 1U))
#define MGBE_MAC_RQC0R_BAK_IDX		((MGBE_MAC_RX_FLW_CTRL_BAK_IDX + 1U))
#define MGBE_MAC_RQC1R_BAK_IDX		((MGBE_MAC_RQC0R_BAK_IDX + 1U))
#define MGBE_MAC_RQC2R_BAK_IDX		((MGBE_MAC_RQC1R_BAK_IDX + 1U))
#define MGBE_MAC_ISR_BAK_IDX		((MGBE_MAC_RQC2R_BAK_IDX + 1U))
#define MGBE_MAC_IER_BAK_IDX		((MGBE_MAC_ISR_BAK_IDX + 1U))
#define MGBE_MAC_PMTCSR_BAK_IDX		((MGBE_MAC_IER_BAK_IDX + 1U))
#define MGBE_MAC_LPI_CSR_BAK_IDX	((MGBE_MAC_PMTCSR_BAK_IDX + 1U))
#define MGBE_MAC_LPI_TIMER_CTRL_BAK_IDX	((MGBE_MAC_LPI_CSR_BAK_IDX + 1U))
#define MGBE_MAC_LPI_EN_TIMER_BAK_IDX	((MGBE_MAC_LPI_TIMER_CTRL_BAK_IDX + 1U))
#define MGBE_MAC_TCR_BAK_IDX		((MGBE_MAC_LPI_EN_TIMER_BAK_IDX + 1U))
#define MGBE_MAC_SSIR_BAK_IDX		((MGBE_MAC_TCR_BAK_IDX + 1U))
#define MGBE_MAC_STSR_BAK_IDX		((MGBE_MAC_SSIR_BAK_IDX + 1U))
#define MGBE_MAC_STNSR_BAK_IDX		((MGBE_MAC_STSR_BAK_IDX + 1U))
#define MGBE_MAC_STSUR_BAK_IDX		((MGBE_MAC_STNSR_BAK_IDX + 1U))
#define MGBE_MAC_STNSUR_BAK_IDX		((MGBE_MAC_STSUR_BAK_IDX + 1U))
#define MGBE_MAC_TAR_BAK_IDX		((MGBE_MAC_STNSUR_BAK_IDX + 1U))
#define MGBE_DMA_BMR_BAK_IDX		((MGBE_MAC_TAR_BAK_IDX + 1U))
#define MGBE_DMA_SBUS_BAK_IDX		((MGBE_DMA_BMR_BAK_IDX + 1U))
#define MGBE_DMA_ISR_BAK_IDX		((MGBE_DMA_SBUS_BAK_IDX + 1U))
#define MGBE_MTL_OP_MODE_BAK_IDX	((MGBE_DMA_ISR_BAK_IDX + 1U))
#define MGBE_MTL_RXQ_DMA_MAP0_BAK_IDX	((MGBE_MTL_OP_MODE_BAK_IDX + 1U))
/* x varies from 0-3, 4 HTR registers total */
#define MGBE_MAC_HTR_REG_BAK_IDX(x)	((MGBE_MTL_RXQ_DMA_MAP0_BAK_IDX + 1U + \
					(x)))
/* x varies from 0-9, 10 queues total */
#define MGBE_MAC_QX_TX_FLW_CTRL_BAK_IDX(x)	((MGBE_MAC_HTR_REG_BAK_IDX(0U) \
						+ MGBE_MAX_HTR_REGS + (x)))
/* x varies from 0-31, 32 L2 DA/SA filters total */
#define MGBE_MAC_ADDRH_BAK_IDX(x)	((MGBE_MAC_QX_TX_FLW_CTRL_BAK_IDX(0U) \
					+ OSI_MGBE_MAX_NUM_QUEUES + (x)))
#define MGBE_MAC_ADDRL_BAK_IDX(x)	((MGBE_MAC_ADDRH_BAK_IDX(0U) + \
					OSI_MGBE_MAX_MAC_ADDRESS_FILTER + (x)))
/* MTL HW Register offsets
 *
 * Do not change the order of these macros. To add new registers to be
 * backed up, append to end of list before MGBE_MAX_MTL_BAK_IDX, and
 * update MGBE_MAX_MTL_BAK_IDX based on new macro.
 */
/* x varies from 0-9, 10 queues total */
#define MGBE_MTL_CHX_TX_OP_MODE_BAK_IDX(x) ((MGBE_MAC_ADDRL_BAK_IDX(0U) + \
					   OSI_MGBE_MAX_MAC_ADDRESS_FILTER + \
					   (x)))
#define MGBE_MTL_TXQ_ETS_CR_BAK_IDX(x)	((MGBE_MTL_CHX_TX_OP_MODE_BAK_IDX(0U) \
					+ OSI_MGBE_MAX_NUM_QUEUES + (x)))
#define MGBE_MTL_TXQ_QW_BAK_IDX(x)	((MGBE_MTL_TXQ_ETS_CR_BAK_IDX(0U) + \
					OSI_MGBE_MAX_NUM_QUEUES + (x)))
#define MGBE_MTL_TXQ_ETS_SSCR_BAK_IDX(x)	((MGBE_MTL_TXQ_QW_BAK_IDX(0U) \
						+ OSI_MGBE_MAX_NUM_QUEUES + \
						(x)))
#define MGBE_MTL_TXQ_ETS_HCR_BAK_IDX(x)	((MGBE_MTL_TXQ_ETS_SSCR_BAK_IDX(0U) + \
					OSI_MGBE_MAX_NUM_QUEUES + (x)))
#define MGBE_MTL_TXQ_ETS_LCR_BAK_IDX(x)	((MGBE_MTL_TXQ_ETS_HCR_BAK_IDX(0U) + \
					OSI_MGBE_MAX_NUM_QUEUES + (x)))
#define MGBE_MTL_CHX_RX_OP_MODE_BAK_IDX(x)	\
					((MGBE_MTL_TXQ_ETS_LCR_BAK_IDX(0U) + \
					OSI_MGBE_MAX_NUM_QUEUES + (x)))

/* MGBE Wrapper register offsets to be saved during suspend
 *
 * Do not change the order of these macros. To add new registers to be
 * backed up, append to end of list before MGBE_MAX_WRAPPER_BAK_IDX,
 * and update MGBE_MAX_WRAPPER_BAK_IDX based on new macro.
 */
#define MGBE_CLOCK_CTRL_0_BAK_IDX	((MGBE_MTL_CHX_RX_OP_MODE_BAK_IDX(0U) \
					+ OSI_MGBE_MAX_NUM_QUEUES))
#define MGBE_AXI_ASID_CTRL_BAK_IDX	((MGBE_CLOCK_CTRL_0_BAK_IDX + 1U))
#define MGBE_PAD_CRTL_BAK_IDX		((MGBE_AXI_ASID_CTRL_BAK_IDX + 1U))
#define MGBE_PAD_AUTO_CAL_CFG_BAK_IDX	((MGBE_PAD_CRTL_BAK_IDX + 1U))
/* MGBE_PAD_AUTO_CAL_STAT is Read-only. Skip backup/restore */

/* To add new direct access registers to backup during suspend,
 * and restore during resume add it before this line, and increment
 * MGBE_DIRECT_MAX_BAK_IDX accordingly.
 */
#define MGBE_DIRECT_MAX_BAK_IDX		((MGBE_PAD_AUTO_CAL_CFG_BAK_IDX + 1U))

/**
 * Start indirect addressing registers
 **/
/* x varies from 0-7, 8 L3/L4 filters total */
#define MGBE_MAC_L3L4_CTR_BAK_IDX(x)	(MGBE_DIRECT_MAX_BAK_IDX + (x))
#define MGBE_MAC_L4_ADR_BAK_IDX(x)	((MGBE_MAC_L3L4_CTR_BAK_IDX(0U) + \
					OSI_MGBE_MAX_L3_L4_FILTER + (x)))
#define MGBE_MAC_L3_AD0R_BAK_IDX(x)	((MGBE_MAC_L4_ADR_BAK_IDX(0U) + \
					OSI_MGBE_MAX_L3_L4_FILTER + (x)))
#define MGBE_MAC_L3_AD1R_BAK_IDX(x)	((MGBE_MAC_L3_AD0R_BAK_IDX(0U) + \
					OSI_MGBE_MAX_L3_L4_FILTER + (x)))
#define MGBE_MAC_L3_AD2R_BAK_IDX(x)	((MGBE_MAC_L3_AD1R_BAK_IDX(0U) + \
					OSI_MGBE_MAX_L3_L4_FILTER + (x)))
#define MGBE_MAC_L3_AD3R_BAK_IDX(x)	((MGBE_MAC_L3_AD2R_BAK_IDX(0U) + \
					OSI_MGBE_MAX_L3_L4_FILTER + (x)))

/* x varies from 0-31, 32 VLAN tag filters total */
#define MGBE_MAC_VLAN_BAK_IDX(x)	((MGBE_MAC_L3_AD3R_BAK_IDX(0) + \
					OSI_MGBE_MAX_L3_L4_FILTER + (x)))

#define MGBE_MAX_BAK_IDX		((MGBE_MAC_VLAN_BAK_IDX(0) + \
					MGBE_MAX_VLAN_FILTER + 1U))
/** @} */
#endif /* MGBE_CORE_H_ */
