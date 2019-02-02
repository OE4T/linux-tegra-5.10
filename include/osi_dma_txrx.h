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

#ifndef OSI_DMA_TXRX_H
#define OSI_DMA_TXRX_H

#define TX_DESC_CNT	256U
#define RX_DESC_CNT	256U

#define INCR_TX_DESC_INDEX(idx, i) ((idx) = ((idx) + (i)) & (TX_DESC_CNT - 1U))
#define INCR_RX_DESC_INDEX(idx, i) ((idx) = ((idx) + (i)) & (RX_DESC_CNT - 1U))
#define DECR_RX_DESC_INDEX(idx, i) ((idx) = ((idx) - (i)) & (RX_DESC_CNT - 1U))

#define RDES3_OWN		OSI_BIT(31)
#define RDES3_IOC		OSI_BIT(30)
#define RDES3_B1V		OSI_BIT(24)
#define RDES3_LD		OSI_BIT(28)
#define RDES3_ERR_CRC		OSI_BIT(24)
#define RDES3_ERR_GP		OSI_BIT(23)
#define RDES3_ERR_WD		OSI_BIT(22)
#define RDES3_ERR_ORUN		OSI_BIT(21)
#define RDES3_ERR_RE		OSI_BIT(20)
#define RDES3_ERR_DRIB		OSI_BIT(19)
#define RDES3_PKT_LEN		0x00007fffU

#define RDES3_ES_BITS \
	(RDES3_ERR_CRC | RDES3_ERR_GP | RDES3_ERR_WD | \
	RDES3_ERR_ORUN | RDES3_ERR_RE | RDES3_ERR_DRIB)

#define TDES2_IOC		OSI_BIT(31)
#define TDES3_OWN		OSI_BIT(31)
#define TDES3_CTX		OSI_BIT(30)
#define TDES3_FD		OSI_BIT(29)
#define TDES3_LD		OSI_BIT(28)

#endif /* OSI_DMA_TXRX_H */
