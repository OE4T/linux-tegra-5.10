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

#ifndef EQOS_DMA_H_
#define EQOS_DMA_H_

#define EQOS_AXI_BUS_WIDTH	0x10U

/* EQOS DMA channel registers */
#define EQOS_DMA_CHX_CTRL(x)		((0x0080U * (x)) + 0x1100U)
#define EQOS_DMA_CHX_TX_CTRL(x)		((0x0080U * (x)) + 0x1104U)
#define EQOS_DMA_CHX_RX_CTRL(x)		((0x0080U * (x)) + 0x1108U)
#define EQOS_DMA_CHX_INTR_ENA(x)	((0x0080U * (x)) + 0x1134U)
#define EQOS_DMA_CHX_RX_WDT(x)		((0x0080U * (x)) + 0x1138U)

#define EQOS_DMA_CHX_RDTP(x)		((0x0080U * (x)) + 0x1128U)
#define EQOS_DMA_CHX_RDLH(x)		((0x0080U * (x)) + 0x1118U)
#define EQOS_DMA_CHX_RDLA(x)		((0x0080U * (x)) + 0x111CU)
#define EQOS_DMA_CHX_RDRL(x)		((0x0080U * (x)) + 0x1130U)
#define EQOS_DMA_CHX_TDTP(x)		((0x0080U * (x)) + 0x1120U)
#define EQOS_DMA_CHX_TDLH(x)		((0x0080U * (x)) + 0x1110U)
#define EQOS_DMA_CHX_TDLA(x)		((0x0080U * (x)) + 0x1114U)
#define EQOS_DMA_CHX_TDRL(x)		((0x0080U * (x)) + 0x112CU)
#define EQOS_VIRT_INTR_CHX_STATUS(x)	(0x8604U + ((x) * 8U))
#define EQOS_VIRT_INTR_CHX_CNTRL(x)	(0x8600U + ((x) * 8U))
#define EQOS_VIRT_INTR_CHX_STATUS_TX	OSI_BIT(0)
#define EQOS_VIRT_INTR_CHX_STATUS_RX	OSI_BIT(1)
#define EQOS_DMA_CHX_STATUS_TI		OSI_BIT(0)
#define EQOS_DMA_CHX_STATUS_RI		OSI_BIT(6)
#define EQOS_DMA_CHX_STATUS_NIS		OSI_BIT(15)
#define EQOS_DMA_CHX_STATUS_CLEAR_TX	\
	(EQOS_DMA_CHX_STATUS_TI | EQOS_DMA_CHX_STATUS_NIS)
#define EQOS_DMA_CHX_STATUS_CLEAR_RX	\
	(EQOS_DMA_CHX_STATUS_RI | EQOS_DMA_CHX_STATUS_NIS)

#define EQOS_VIRT_INTR_CHX_CNTRL_TX		OSI_BIT(0)
#define EQOS_VIRT_INTR_CHX_CNTRL_RX		OSI_BIT(1)

#define EQOS_DMA_CHX_INTR_TIE			OSI_BIT(0)
#define EQOS_DMA_CHX_INTR_TBUE			OSI_BIT(2)
#define EQOS_DMA_CHX_INTR_RIE			OSI_BIT(6)
#define EQOS_DMA_CHX_INTR_RBUE			OSI_BIT(7)
#define EQOS_DMA_CHX_INTR_FBEE			OSI_BIT(12)
#define EQOS_DMA_CHX_INTR_AIE			OSI_BIT(14)
#define EQOS_DMA_CHX_INTR_NIE			OSI_BIT(15)
#define EQOS_DMA_CHX_TX_CTRL_OSF		OSI_BIT(4)
#define EQOS_DMA_CHX_TX_CTRL_TSE		OSI_BIT(12)
#define EQOS_DMA_CHX_CTRL_PBLX8			OSI_BIT(16)
#define EQOS_DMA_CHX_RBSZ_SHIFT			1U
#define EQOS_DMA_CHX_TX_CTRL_TXPBL_RECOMMENDED	0x200000U
#define EQOS_DMA_CHX_RX_CTRL_RXPBL_RECOMMENDED	0xC0000U

#endif
