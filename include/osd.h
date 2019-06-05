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

#ifndef OSD_H
#define OSD_H

void osd_usleep_range(unsigned long umin, unsigned long umax);
void osd_msleep(unsigned int msec);
void osd_udelay(unsigned long usec);
void osd_info(void *priv, const char *fmt, ...);
void osd_err(void *priv, const char *fmt, ...);

/**
 *	osd_receive_packet - Handover received packet to network stack.
 *	@priv: OSD private data structure.
 *	@rxring: Pointer to DMA channel Rx ring.
 *	@chan: DMA Rx channel number.
 *	@dma_buf_len: Rx DMA buffer length.
 *	@rxpkt_cx: Received packet context.
 *	@rx_pkt_swcx: Received packet sw context.
 *
 *	Algorithm:
 *	1) Unmap the DMA buffer address.
 *	2) Updates socket buffer with len and ether type and handover to
 *	OS network stack.
 *	3) Refill the Rx ring based on threshold.
 *	4) Fills the rxpkt_cx->flags with the below bit fields accordingly
 *	OSI_PKT_CX_VLAN
 *	OSI_PKT_CX_VALID
 *	OSI_PKT_CX_CSUM
 *	OSI_PKT_CX_TSO
 *	OSI_PKT_CX_PTP
 *
 *	Dependencies: Rx completion need to make sure that Rx descriptors
 *	processed properly.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
void osd_receive_packet(void *priv, void *rxring, unsigned int chan,
			unsigned int dma_buf_len, void *rxpkt_cx,
			void *rx_pkt_swcx);

/**
 *	osd_transmit_complete - Transmit completion routine.
 *	@priv: OSD private data structure.
 *	@buffer: Buffer address to free.
 *	@dmaaddr: DMA address to unmap.
 *	@len: Length of data.
 *	@tx_done_pkt_cx: Pointer to struct which has tx done status info.
 *	This struct has flags to indicate tx error, whether DMA address
 *	is mapped from paged/linear buffer, Time stamp availability,
 *	if TS available txdone_pkt_cx->ns stores the time stamp.
 *	Below are the valid bit maps set for txdone_pkt_cx->flags
 *	#define OSI_TXDONE_CX_PAGED_BUF         OSI_BIT(0)
 *	#define OSI_TXDONE_CX_ERROR             OSI_BIT(1)
 *	#define OSI_TXDONE_CX_TS                OSI_BIT(2)
 *
 *	Algorithm:
 *	1) Updates stats for linux network stack.
 *	2) unmap and free the buffer DMA address and buffer.
 *	3) Time stamp will be updated to stack if available.
 *
 *	Dependencies: Tx completion need to make sure that Tx descriptors
 *	processed properly.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
void osd_transmit_complete(void *priv, void *buffer, unsigned long dmaaddr,
			   unsigned int len, void *txdone_pkt_cx);
#endif
