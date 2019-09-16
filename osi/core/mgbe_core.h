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
 * @addtogroup MGBE-MAC MAC register offsets
 *
 * @brief MGBE MAC register offsets
 * @{
 */
#define MGBE_MAC_RQC0R			0x00A0
#define MGBE_MAC_MA0HR			0x0300
#define MGBE_MAC_MA0LR			0x0304
#define MGBE_MAC_TMCR			0x0000
#define MGBE_MAC_RMCR			0x0004
#define MGBE_MMC_TX_INTR_EN		0x0810
#define MGBE_MMC_RX_INTR_EN		0x080C
#define MGBE_MMC_CNTRL			0x0800
#define MGBE_MAC_IER			0x00B4
#define MGBE_MAC_ISR			0x00B0
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
#define MGBE_MTL_RXQ_DMA_MAP0		0x1030
#define MGBE_MTL_RXQ_DMA_MAP1		0x1034
#define MGBE_MTL_RXQ_DMA_MAP2		0x1038
#define MGBE_MTL_RXQ_DMA_MAP3		0x103b
#define MGBE_MTL_CHX_TX_OP_MODE(x)	((0x0080U * (x)) + 0x1100U)
#define MGBE_MTL_CHX_RX_OP_MODE(x)	((0x0080U * (x)) + 0x1140U)
#define MGBE_MTL_TXQ_QW(x)		((0x0080U * (x)) + 0x1118U)
/** @} */


/**
 * @addtogroup HW Register BIT values
 *
 * @brief consists of corresponding MGBE MAC, MTL register bit values
 * @{
 */
#define MGBE_DMA_MODE_SWR			OSI_BIT(0)
#define MGBE_MTL_QTOMR_FTQ			OSI_BIT(0)
#define MGBE_MTL_QTOMR_FTQ_LPOS			OSI_BIT(0)
#define MGBE_MTL_TSF				OSI_BIT(1)
#define MGBE_MTL_TXQEN				OSI_BIT(3)
#define MGBE_MTL_RSF				OSI_BIT(5)
#define MGBE_MTL_TXQ_QW_ISCQW                   OSI_BIT(4)
#define MGBE_MAC_RMCR_ACS			OSI_BIT(1)
#define MGBE_MAC_RMCR_CST			OSI_BIT(2)
#define MGBE_MAC_RMCR_IPC			OSI_BIT(9)
#define MGBE_MAC_RXQC0_RXQEN_MASK		0x3U
#define MGBE_MAC_RXQC0_RXQEN_SHIFT(x)		((x) * 2U)
#define MGBE_MMC_CNTRL_CNTRST			OSI_BIT(0)
#define MGBE_MMC_CNTRL_RSTONRD			OSI_BIT(2)
#define MGBE_MMC_CNTRL_CNTMCT			(OSI_BIT(4) | OSI_BIT(5))
#define MGBE_MMC_CNTRL_CNTPRST			OSI_BIT(7)
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
/* DMA SBUS */
#define MGBE_DMA_SBUS_BLEN8			OSI_BIT(2)
#define MGBE_DMA_SBUS_BLEN16			OSI_BIT(3)
#define MGBE_DMA_SBUS_EAME			OSI_BIT(11)
#define MGBE_DMA_SBUS_RD_OSR_LMT		0x001F0000U
#define MGBE_DMA_SBUS_WR_OSR_LMT		0x1F000000U
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
#endif /* MGBE_CORE_H_ */
