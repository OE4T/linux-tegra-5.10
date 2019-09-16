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

#ifndef INCLUDED_MGBE_DMA_H
#define INCLUDED_MGBE_DMA_H

/**
 * @addtogroup MGBE-DMA-CH DMA Channel Register offsets
 *
 * @brief MGBE DMA Channel register offsets
 * @{
 */
#define MGBE_DMA_CHX_TX_CTRL(x)		((0x0080U * (x)) + 0x3104U)
#define MGBE_DMA_CHX_RX_CTRL(x)		((0x0080U * (x)) + 0x3108U)
#define MGBE_DMA_CHX_INTR_ENA(x)	((0x0080U * (x)) + 0x3138U)
#define MGBE_DMA_CHX_CTRL(x)		((0x0080U * (x)) + 0x3100U)
#define MGBE_DMA_CHX_RX_WDT(x)          ((0x0080U * (x)) + 0x313CU)
#define MGBE_DMA_CHX_TDRL(x)		((0x0080U * (x)) + 0x3130U)
#define MGBE_DMA_CHX_RDRL(x)		((0x0080U * (x)) + 0x3134U)
#define MGBE_DMA_CHX_TDLH(x)		((0x0080U * (x)) + 0x3110U)
#define MGBE_DMA_CHX_TDLA(x)		((0x0080U * (x)) + 0x3114U)
#define MGBE_DMA_CHX_TDTLP(x)		((0x0080U * (x)) + 0x3124U)
#define MGBE_DMA_CHX_TDTHP(x)		((0x0080U * (x)) + 0x3120U)
#define MGBE_DMA_CHX_RDLH(x)		((0x0080U * (x)) + 0x3118U)
#define MGBE_DMA_CHX_RDLA(x)		((0x0080U * (x)) + 0x311CU)
#define MGBE_DMA_CHX_RDTHP(x)		((0x0080U * (x)) + 0x3128U)
#define MGBE_DMA_CHX_RDTLP(x)		((0x0080U * (x)) + 0x312CU)
/** @} */

/**
 * @addtogroup MGBE-INTR INT Channel Register offsets
 *
 * @brief MGBE Virtural Interrupt Channel register offsets
 * @{
 */
#define MGBE_VIRT_INTR_CHX_STATUS(x)	(0x8604U + ((x) * 8U))
#define MGBE_VIRT_INTR_CHX_CNTRL(x)	(0x8600U + ((x) * 8U))
#define MGBE_VIRT_INTR_APB_CHX_CNTRL(x)	(0x8200U + ((x) * 4U))
/** @} */

/**
 * @addtogroup MGBE BIT fields for MGBE Rgisters
 *
 * @brief Values defined for the MGBE registers
 * @{
 */
#define MGBE_DMA_CHX_TX_CTRL_OSP		OSI_BIT(4)
#define MGBE_DMA_CHX_TX_CTRL_TSE		OSI_BIT(12)
#define MGBE_DMA_CHX_RX_WDT_RWT_MASK		0xFFU
#define MGBE_DMA_CHX_RX_WDT_RWTU		256U
#define MGBE_DMA_CHX_RBSZ_SHIFT			1U
#define MGBE_DMA_CHX_TX_CTRL_TXPBL_RECOMMENDED	0x100000U
#define MGBE_DMA_CHX_RX_CTRL_RXPBL_RECOMMENDED	0x100000U
#define MGBE_AXI_BUS_WIDTH			0x10U
#define MGBE_GLOBAL_DMA_STATUS			0x8700U
#define MGBE_DMA_CHX_CTRL_PBLX8			OSI_BIT(16)
#define MGBE_DMA_CHX_INTR_TIE			OSI_BIT(0)
#define MGBE_DMA_CHX_INTR_TBUE			OSI_BIT(2)
#define MGBE_DMA_CHX_INTR_RIE			OSI_BIT(6)
#define MGBE_DMA_CHX_INTR_RBUE			OSI_BIT(7)
#define MGBE_DMA_CHX_INTR_FBEE			OSI_BIT(12)
#define MGBE_DMA_CHX_INTR_AIE			OSI_BIT(14)
#define MGBE_DMA_CHX_INTR_NIE			OSI_BIT(15)
#define MGBE_DMA_CHX_STATUS_TI			OSI_BIT(0)
#define MGBE_DMA_CHX_STATUS_RI			OSI_BIT(6)
#define MGBE_DMA_CHX_STATUS_NIS			OSI_BIT(15)
#define MGBE_DMA_CHX_STATUS_CLEAR_TX		(MGBE_DMA_CHX_STATUS_TI | \
						 MGBE_DMA_CHX_STATUS_NIS)
#define	MGBE_DMA_CHX_STATUS_CLEAR_RX		(MGBE_DMA_CHX_STATUS_RI | \
						 MGBE_DMA_CHX_STATUS_NIS)
#define MGBE_VIRT_INTR_CHX_STATUS_TX		OSI_BIT(0)
#define MGBE_VIRT_INTR_CHX_STATUS_RX		OSI_BIT(1)
#define MGBE_VIRT_INTR_CHX_CNTRL_TX		OSI_BIT(0)
#define MGBE_VIRT_INTR_CHX_CNTRL_RX		OSI_BIT(1)
/** @} */
#endif /* INCLUDED_MGBE_DMA_H */
