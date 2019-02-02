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

#include <osd.h>
#include <osi_dma.h>
#include <osi_dma_txrx.h>

/**
 *	osi_process_rx_completions - Read data from receive channel descriptors
 *	@osi: OSI private data structure.
 *	@chan: Rx DMA channel number
 *	@budget: Threshould for reading the packets at a time.
 *
 *	Algorimthm: This routine will be invoked by OSD layer to get the
 *	data from Rx descriptors and deliver the packet to the stack.
 *	1) Checks descriptor owned by DMA or not.
 *	2) Get the length from Rx descriptor
 *	3) Invokes OSD layer to deliver the packet to network stack.
 *	4) Re-allocate the receive buffers, populate Rx descriptor and
 *	handover to DMA.
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
int osi_process_rx_completions(struct osi_dma_priv_data *osi,
			       unsigned int chan, int budget)
{
	struct osi_rx_ring *rx_ring = osi->rx_ring[chan];
	struct osi_rx_desc *rx_desc = OSI_NULL;
	unsigned int pkt_len = 0;
	int received = 0;

	while (received < budget) {
		rx_desc = rx_ring->rx_desc + rx_ring->cur_rx_idx;

		/* check for data availability */
		if ((rx_desc->rdes3 & RDES3_OWN) == RDES3_OWN) {
			break;
		}

		/* get the length of the packet */
		pkt_len = rx_desc->rdes3 & RDES3_PKT_LEN;

		if (((rx_desc->rdes3 & RDES3_ES_BITS) == 0U) &&
		    ((rx_desc->rdes3 & RDES3_LD) == RDES3_LD)) {
			osd_receive_packet(osi->osd, rx_ring, chan,
					   osi->rx_buf_len, pkt_len);
		}

		received++;
	}

	return received;
}

/**
 *	osi_process_tx_completions - Process Tx complete on DMA channel ring.
 *	@osi: OSI private data structure.
 *	@chan: Channel number on which Tx complete need to be done.
 *
 *	Algorithm: This function will be invoked by OSD layer to process Tx
 *	complete interrupt.
 *	1) First checks whether descriptor owned by DMA or not.
 *	2) Invokes OSD layer to release DMA address and Tx buffer which are
 *	updated as part of transmit routine.
 *
 *	Dependencies: None.
 *
 *	Protection: None
 *
 *	Return: Number of decriptors (buffers) proccessed.
 */
int osi_process_tx_completions(struct osi_dma_priv_data *osi,
			       unsigned int chan)
{
	struct osi_tx_ring *tx_ring = osi->tx_ring[chan];
	struct osi_tx_swcx *tx_swcx = OSI_NULL;
	struct osi_tx_desc *tx_desc = OSI_NULL;
	unsigned int entry = tx_ring->clean_idx;
	int processed = 0;

	while (entry != tx_ring->cur_tx_idx) {
		tx_desc = tx_ring->tx_desc + entry;
		tx_swcx = tx_ring->tx_swcx + entry;

		if ((tx_desc->tdes3 & TDES3_OWN) == TDES3_OWN) {
			break;
		}

		osd_transmit_complete(osi->osd, tx_swcx->buf_virt_addr,
				      tx_swcx->buf_phy_addr, tx_swcx->len);

		tx_desc->tdes3 = 0;
		tx_desc->tdes2 = 0;
		tx_desc->tdes1 = 0;
		tx_desc->tdes0 = 0;
		tx_swcx->len = 0;

		tx_swcx->buf_virt_addr = OSI_NULL;
		tx_swcx->buf_phy_addr = 0;
		INCR_TX_DESC_INDEX(entry, 1U);
		processed++;
	}

	tx_ring->clean_idx = entry;

	return processed;
}

/**
 *	osi_hw_transmit - Initialize Tx DMA descriptors for a channel
 *	@osi:	OSI private data structure.
 *	@chan:	DMA Tx channel number
 *
 *	Algorithm: Initialize Transmit descriptors with DMA mappabled buffers,
 *	set OWN bit, Tx ring length and set starting address of Tx DMA channel.
 *	Tx ring base address in Tx DMA registers.
 *
 *	Dependencies: Transmit routine of OSD needs to map Tx buffer
 *	to dma mappable address and update in the software context descriptors.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
void osi_hw_transmit(struct osi_dma_priv_data *osi, unsigned int chan)
{
	struct osi_tx_ring *tx_ring = osi->tx_ring[chan];
	struct osi_dma_chan_ops *ops = osi->ops;
	unsigned int entry = tx_ring->cur_tx_idx;
	struct osi_tx_desc *tx_desc = tx_ring->tx_desc + entry;
	struct osi_tx_swcx *tx_swcx = tx_ring->tx_swcx + entry;
	unsigned long tailptr;

	/* update the first buffer pointer and length */
	tx_desc->tdes0 = (unsigned int)L32(tx_swcx->buf_phy_addr);
	tx_desc->tdes1 = (unsigned int)H32(tx_swcx->buf_phy_addr);
	tx_desc->tdes2 = tx_swcx->len;
	/* Mark it as First descriptor */
	tx_desc->tdes3 |= TDES3_FD;
	/* Mark it as LAST descriptor */
	tx_desc->tdes3 |= TDES3_LD;
	/* set Interrupt on Completion*/
	tx_desc->tdes2 |= TDES2_IOC;
	/* set HW OWN bit for descriptor*/
	tx_desc->tdes3 |= TDES3_OWN;

	INCR_TX_DESC_INDEX(entry, 1U);

	tailptr = tx_ring->tx_desc_phy_addr +
		  (entry * sizeof(struct osi_tx_desc));

	ops->update_tx_tailptr(osi->base, chan, tailptr);
	tx_ring->cur_tx_idx = entry;
}

/**
 *	rx_dma_desc_initialization - Initialize DMA Receive descriptors for Rx.
 *	@osi:	OSI private data structure.
 *	@chan:	Rx channel number.
 *
 *	Algorithm: Initialize Receive descriptors with DMA mappable buffers,
 *	set OWN bit, Rx ring length and set starting address of Rx DMA channel.
 *	Tx ring base address in Tx DMA registers.
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, -1 - failure.
 */
static int rx_dma_desc_initialization(struct osi_dma_priv_data *osi,
				      unsigned int chan)
{
	struct osi_rx_ring *rx_ring = osi->rx_ring[chan];
	struct osi_rx_desc *rx_desc = OSI_NULL;
	struct osi_rx_swcx *rx_swcx = OSI_NULL;
	struct osi_dma_chan_ops *ops = osi->ops;
	unsigned long tailptr = 0;
	unsigned int i;
	int ret = 0;

	rx_ring->cur_rx_idx = 0;
	rx_ring->refill_idx = 0;

	for (i = 0; i < RX_DESC_CNT; i++) {
		rx_swcx = rx_ring->rx_swcx + i;
		rx_desc = rx_ring->rx_desc + i;

		rx_desc->rdes0 = (unsigned int)L32(rx_swcx->buf_phy_addr);
		rx_desc->rdes1 = (unsigned int)H32(rx_swcx->buf_phy_addr);
		rx_desc->rdes2 = 0;
		//TODO: RDES3_IOC needs to be reset for Rx watchdog
		rx_desc->rdes3 = (RDES3_OWN | RDES3_IOC | RDES3_B1V);
	}

	tailptr = rx_ring->rx_desc_phy_addr +
		  sizeof(struct osi_rx_desc) * (RX_DESC_CNT - 1U);

	ops->set_rx_ring_len(osi->base, chan, (RX_DESC_CNT - 1U));
	ops->update_rx_tailptr(osi->base, chan, tailptr);
	ops->set_rx_ring_start_addr(osi->base, chan, rx_ring->rx_desc_phy_addr);

	return ret;
}

/**
 *	rx_dma_desc_init - Initialize DMA Receive descriptors for Rx channel.
 *	@osi:	OSI private data structure.
 *
 *	Algorithm: Initialize Receive descriptors with DMA mappabled buffers,
 *	set OWN bit, Rx ring length and set starting address of Rx DMA channel.
 *	Tx ring base address in Tx DMA registers.
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, -1 - failure.
 */
static int rx_dma_desc_init(struct osi_dma_priv_data *osi)
{
	unsigned int chan = 0;
	unsigned int i;
	int ret = 0;

	for (i = 0; i < osi->num_dma_chans; i++) {
		chan = osi->dma_chans[i];

		ret = rx_dma_desc_initialization(osi, chan);
		if (ret != 0) {
			return ret;
		}
	}

	return ret;
}

/**
 *	tx_dma_desc_init - Initialize DMA Transmit descriptors.
 *	@osi:	OSI private data structure.
 *
 *	Algorithm: Initialize Trannsmit descriptors and set Tx ring length,
 *	Tx ring base address in Tx DMA registers.
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
static void tx_dma_desc_init(struct osi_dma_priv_data *osi_dma)
{
	struct osi_tx_ring *tx_ring = OSI_NULL;
	struct osi_tx_desc *tx_desc = OSI_NULL;
	struct osi_tx_swcx *tx_swcx = OSI_NULL;
	struct osi_dma_chan_ops *ops = osi_dma->ops;
	unsigned int chan = 0;
	unsigned int i;

	for (i = 0; i < osi_dma->num_dma_chans; i++) {
		chan = osi_dma->dma_chans[i];

		tx_ring = osi_dma->tx_ring[chan];
		tx_desc = tx_ring->tx_desc;
		tx_swcx = tx_ring->tx_swcx;

		/* FIXME: does it require */
		tx_desc->tdes0 = 0;
		tx_desc->tdes1 = 0;
		tx_desc->tdes2 = 0;
		tx_desc->tdes3 = 0;

		tx_ring->cur_tx_idx = 0;
		tx_ring->clean_idx = 0;

		ops->set_tx_ring_len(osi_dma->base, chan,
				(TX_DESC_CNT - 1U));
		ops->set_tx_ring_start_addr(osi_dma->base, chan,
				tx_ring->tx_desc_phy_addr);
	}
}

/**
 *	dma_desc_init - Initialize DMA Tx/Rx descriptors
 *	@osi:	OSI private data structure.
 *
 *	Algorithm: Transmit and Receive desctiptors will be initialized with
 *	required values so that MAC DMA can understand and act accordingly.
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, -1 - failure.
 */
int dma_desc_init(struct osi_dma_priv_data *osi_dma)
{
	int ret = 0;

	tx_dma_desc_init(osi_dma);

	ret = rx_dma_desc_init(osi_dma);
	if (ret != 0) {
		return ret;
	}

	return ret;
}

