/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION. All rights reserved.
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

#ifndef EQOS_CORE_H_
#define EQOS_CORE_H_

/* MDC Clock Selection define*/
#define EQOS_CSR_60_100M	0x0	/* MDC = clk_csr/42 */
#define EQOS_CSR_100_150M	0x1	/* MDC = clk_csr/62 */
#define EQOS_CSR_20_35M		0x2	/* MDC = clk_csr/16 */
#define EQOS_CSR_35_60M		0x3	/* MDC = clk_csr/26 */
#define EQOS_CSR_150_250M	0x4	/* MDC = clk_csr/102 */
#define EQOS_CSR_250_300M	0x5	/* MDC = clk_csr/124 */
#define EQOS_CSR_300_500M	0x6	/* MDC = clk_csr/204 */
#define EQOS_CSR_500_800M	0x7	/* MDC = clk_csr/324 */

#define FIFO_SIZE_B(x) (x)
#define FIFO_SIZE_KB(x) ((x) * 1024U)

/*  per queue fifo size programmable value */
#define EQOS_256	0x00U
#define EQOS_512	0x01U
#define EQOS_1K		0x03U
#define EQOS_2K		0x07U
#define EQOS_4K		0x0FU
#define EQOS_8K		0x1FU
#define EQOS_9K		0x23U
#define EQOS_16K	0x3FU
#define EQOS_32K	0x7FU
#define EQOS_36K	0x8FU

/* EQOS HW Registers */
#define EQOS_DMA_SBUS		0x1004
#define EQOS_DMA_BMR		0x1000
#define EQOS_MMC_CNTRL		0x0700
#define EQOS_MAC_MA0HR		0x0300
#define EQOS_MAC_MA0LR		0x0304
#define EQOS_MAC_MCR		0x0000
#define EQOS_MAC_VLAN_TAG	0x0050
#define EQOS_MAC_VLANTIR	0x0060
#define EQOS_MAC_IMR		0x00B4
#define EQOS_DMA_ISR		0x1008
#define EQOS_MAC_ISR		0x00B0
#define EQOS_MAC_RQC1R		0x00A4
#define EQOS_MMC_TX_INTR_MASK	0x0710
#define EQOS_MMC_RX_INTR_MASK	0x070C
#define EQOS_MMC_IPC_RX_INTR_MASK	0x0800
#define EQOS_MAC_RQC0R		0x00A0
#define EQOS_MAC_PMTCSR		0x00C0
#define EQOS_MAC_PCS		0x00F8
#define EQOS_MAC_ANS		0x00E4
#define EQOS_RXQ_TO_DMA_CHAN_MAP	0x03020100U

/* EQOS MTL registers*/
#define EQOS_MTL_CHX_TX_OP_MODE(x)	((0x0040U * (x)) + 0x0D00U)
#define EQOS_MTL_TXQ_QW(x)		((0x0040U * (x)) + 0x0D18U)
#define EQOS_MTL_CHX_RX_OP_MODE(x)	((0x0040U * (x)) + 0x0D30U)
#define EQOS_MTL_RXQ_DMA_MAP0		0x0C30

/* EQOS Wrapper registers*/
#define EQOS_PAD_AUTO_CAL_CFG		0x8804U
#define EQOS_PAD_AUTO_CAL_STAT		0x880CU
#define EQOS_PAD_CRTL			0x8800U
#define EQOS_CLOCK_CTRL_0		0x8000U

/* EQOS Register BIT Masks */
#define EQOS_PAD_AUTO_CAL_CFG_ENABLE		OSI_BIT(29)
#define EQOS_PAD_AUTO_CAL_CFG_START		OSI_BIT(31)
#define EQOS_PAD_AUTO_CAL_STAT_ACTIVE		OSI_BIT(31)
#define EQOS_PAD_CRTL_E_INPUT_OR_E_PWRD		OSI_BIT(31)
#define EQOS_MMC_CNTRL_CNTRST			OSI_BIT(0)
#define EQOS_MMC_CNTRL_RSTONRD			OSI_BIT(2)
#define EQOS_MMC_CNTRL_CNTPRST			OSI_BIT(4)
#define EQOS_MMC_CNTRL_CNTPRSTLVL		OSI_BIT(5)
#define EQOS_MTL_QTOMR_FTQ			OSI_BIT(0)
#define EQOS_MTL_TSF				OSI_BIT(1)
#define EQOS_MTL_TXQEN				OSI_BIT(3)
#define EQOS_MTL_RSF				OSI_BIT(5)
#define EQOS_MCR_TE				OSI_BIT(0)
#define EQOS_MCR_RE				OSI_BIT(1)
#define EQOS_MCR_DM				OSI_BIT(13)
#define EQOS_MCR_FES				OSI_BIT(14)
#define EQOS_MCR_PS				OSI_BIT(15)
#define EQOS_MCR_JE				OSI_BIT(16)
#define EQOS_MCR_JD				OSI_BIT(17)
#define EQOS_MCR_ACS				OSI_BIT(20)
#define EQOS_MCR_CST				OSI_BIT(21)
#define EQOS_MCR_S2KP				OSI_BIT(22)
#define EQOS_IMR_RGSMIIIE			OSI_BIT(0)
#define EQOS_IMR_PCSLCHGIE			OSI_BIT(1)
#define EQOS_IMR_PCSANCIE			OSI_BIT(2)
#define EQOS_IMR_PMTIE				OSI_BIT(4)
#define EQOS_IMR_LPIIE				OSI_BIT(5)
#define EQOS_MAC_PCS_LNKSTS			OSI_BIT(19)
#define EQOS_MAC_PCS_LNKMOD			OSI_BIT(16)
#define EQOS_MAC_PCS_LNKSPEED			(OSI_BIT(17) | OSI_BIT(18))
#define EQOS_MAC_PCS_LNKSPEED_10		0x0U
#define EQOS_MAC_PCS_LNKSPEED_100		OSI_BIT(17)
#define EQOS_MAC_PCS_LNKSPEED_1000		OSI_BIT(18)
#define EQOS_MAC_VLANTIR_VLTI			OSI_BIT(20)
#define EQOS_MAC_VLANTR_EVLS_ALWAYS_STRIP	((unsigned int)0x3 << 21U)
#define EQOS_MAC_VLANTR_EVLRXS			OSI_BIT(24)
#define EQOS_MAC_VLANTR_DOVLTC			OSI_BIT(20)
#define EQOS_MAC_VLANTR_ERIVLT			OSI_BIT(27)
#define EQOS_MAC_VLANTIRR_VLTI			OSI_BIT(20)
#define EQOS_MAC_VLANTIRR_CSVL			OSI_BIT(19)
#define EQOS_DMA_SBUS_BLEN4			OSI_BIT(1)
#define EQOS_DMA_SBUS_BLEN8			OSI_BIT(2)
#define EQOS_DMA_SBUS_BLEN16			OSI_BIT(3)
#define EQOS_DMA_SBUS_EAME			OSI_BIT(11)
#define EQOS_DMA_BMR_SWR			OSI_BIT(0)
#define EQOS_DMA_BMR_DPSW			OSI_BIT(8)
#define EQOS_MAC_RQC1R_MCBCQEN			OSI_BIT(20)
#define EQOS_MTL_QTOMR_FTQ_LPOS			OSI_BIT(0)
#define EQOS_DMA_ISR_MACIS			OSI_BIT(17)
#define EQOS_MAC_ISR_RGSMIIS			OSI_BIT(0)
#define EQOS_MTL_TXQ_QW_ISCQW			OSI_BIT(4)
#define EQOS_DMA_SBUS_RD_OSR_LMT		0x001F0000U
#define EQOS_DMA_SBUS_WR_OSR_LMT		0x1F000000U
#define EQOS_MTL_TXQ_SIZE_SHIFT			16U
#define EQOS_MTL_RXQ_SIZE_SHIFT			20U
#define EQOS_MAC_ENABLE_LM			OSI_BIT(12)
#define EQOS_RX_CLK_SEL				OSI_BIT(8)

#endif
