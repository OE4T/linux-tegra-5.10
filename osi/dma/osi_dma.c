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

#include <osi_dma.h>

extern int dma_desc_init(struct osi_dma_priv_data *osi_dma);
extern struct osi_dma_chan_ops *eqos_get_dma_chan_ops(void);

void osi_init_dma_ops(struct osi_dma_priv_data *osi_dma)
{
	if (osi_dma->mac == OSI_MAC_HW_EQOS) {
		/* Get EQOS HW ops */
		osi_dma->ops = eqos_get_dma_chan_ops();
	}
}

int osi_hw_dma_init(struct osi_dma_priv_data *osi_dma)
{
	unsigned int i, chan;
	int ret;

	if (osi_dma->ops->init_dma_channel != OSI_NULL) {
		osi_dma->ops->init_dma_channel(osi_dma);
	}

	ret = dma_desc_init(osi_dma);
	if (ret != 0) {
		return ret;
	}

	/* Enable channel interrupts at wrapper level and start DMA */
	for (i = 0; i < osi_dma->num_dma_chans; i++) {
		chan = osi_dma->dma_chans[i];

		osi_enable_chan_tx_intr(osi_dma, chan);
		osi_enable_chan_rx_intr(osi_dma, chan);
		osi_start_dma(osi_dma, chan);
	}

	return 0;
}

/**
 *	osi_hw_deinit - De-init the HW
 *	@osi: OSI private data structure.
 *
 *	Algorithm:
 *	1) Stop the DMA
 *	2) free all allocated resources.
 *
 *	Dependencies: None
 *	Protection: None
 *	Return: None
 */
void osi_hw_dma_deinit(struct osi_dma_priv_data *osi_dma)
{
	unsigned int i;

	for (i = 0; i < osi_dma->num_dma_chans; i++) {
		osi_stop_dma(osi_dma, osi_dma->dma_chans[i]);
	}
}

void osi_disable_chan_tx_intr(struct osi_dma_priv_data *osi_dma,
			      unsigned int chan)
{
	if ((osi_dma != OSI_NULL) && (osi_dma->ops != OSI_NULL) &&
	    (osi_dma->ops->disable_chan_tx_intr != OSI_NULL)) {
		osi_dma->ops->disable_chan_tx_intr(osi_dma->base, chan);
	}
}

void osi_enable_chan_tx_intr(struct osi_dma_priv_data *osi_dma,
			     unsigned int chan)
{
	if ((osi_dma != OSI_NULL) && (osi_dma->ops != OSI_NULL) &&
	    (osi_dma->ops->enable_chan_tx_intr != OSI_NULL)) {
		osi_dma->ops->enable_chan_tx_intr(osi_dma->base, chan);
	}
}

void osi_disable_chan_rx_intr(struct osi_dma_priv_data *osi_dma,
			      unsigned int chan)
{
	if ((osi_dma != OSI_NULL) && (osi_dma->ops != OSI_NULL) &&
	    (osi_dma->ops->disable_chan_rx_intr != OSI_NULL)) {
		osi_dma->ops->disable_chan_rx_intr(osi_dma->base, chan);
	}
}

void osi_enable_chan_rx_intr(struct osi_dma_priv_data *osi_dma,
			     unsigned int chan)
{
	if ((osi_dma != OSI_NULL) && (osi_dma->ops != OSI_NULL) &&
	    (osi_dma->ops->enable_chan_rx_intr != OSI_NULL)) {
		osi_dma->ops->enable_chan_rx_intr(osi_dma->base, chan);
	}
}

void osi_clear_tx_intr(struct osi_dma_priv_data *osi_dma,
		       unsigned int chan)
{
	if ((osi_dma != OSI_NULL) && (osi_dma->ops != OSI_NULL) &&
	    (osi_dma->ops->clear_tx_intr != OSI_NULL)) {
		osi_dma->ops->clear_tx_intr(osi_dma->base, chan);
	}
}

void osi_clear_rx_intr(struct osi_dma_priv_data *osi_dma,
		       unsigned int chan)
{
	if ((osi_dma != OSI_NULL) && (osi_dma->ops != OSI_NULL) &&
	    (osi_dma->ops->clear_rx_intr != OSI_NULL)) {
		osi_dma->ops->clear_rx_intr(osi_dma->base, chan);
	}
}

void osi_start_dma(struct osi_dma_priv_data *osi_dma,
		   unsigned int chan)
{
	if ((osi_dma != OSI_NULL) && (osi_dma->ops != OSI_NULL) &&
	    (osi_dma->ops->start_dma != OSI_NULL)) {
		osi_dma->ops->start_dma(osi_dma->base, chan);
	}
}

void osi_stop_dma(struct osi_dma_priv_data *osi_dma,
		  unsigned int chan)
{
	if ((osi_dma != OSI_NULL) && (osi_dma->ops != OSI_NULL) &&
	    (osi_dma->ops->stop_dma != OSI_NULL)) {
		osi_dma->ops->stop_dma(osi_dma->base, chan);
	}
}

unsigned int osi_get_refill_rx_desc_cnt(struct osi_rx_ring *rx_ring)
{
	return (rx_ring->cur_rx_idx - rx_ring->refill_idx) & (RX_DESC_CNT - 1U);
}

void osi_rx_dma_desc_init(struct osi_rx_swcx *rx_swcx,
			  struct osi_rx_desc *rx_desc)
{
	rx_desc->rdes0 = (unsigned int)L32(rx_swcx->buf_phy_addr);
	rx_desc->rdes1 = (unsigned int)H32(rx_swcx->buf_phy_addr);
	rx_desc->rdes2 = 0;
	rx_desc->rdes3 = (RDES3_OWN | RDES3_IOC | RDES3_B1V);
}

void osi_update_rx_tailptr(struct osi_dma_priv_data *osi_dma,
			   struct osi_rx_ring *rx_ring,
			   unsigned int chan)
{
	unsigned long tailptr = 0;
	unsigned int refill_idx = rx_ring->refill_idx;

	DECR_RX_DESC_INDEX(refill_idx, 1U);
	tailptr = rx_ring->rx_desc_phy_addr +
		  (refill_idx * sizeof(struct osi_rx_desc));

	if (osi_dma != OSI_NULL && osi_dma->ops != OSI_NULL &&
	    osi_dma->ops->update_rx_tailptr != OSI_NULL) {
		osi_dma->ops->update_rx_tailptr(osi_dma->base, chan, tailptr);
	}
}

void osi_set_rx_buf_len(struct osi_dma_priv_data *osi_dma)
{
	if (osi_dma != OSI_NULL && osi_dma->ops != OSI_NULL &&
	    osi_dma->ops->set_rx_buf_len != OSI_NULL) {
		osi_dma->ops->set_rx_buf_len(osi_dma);
	}
}
