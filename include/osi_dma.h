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

#ifndef OSI_DMA_H
#define OSI_DMA_H

#include "osi_common.h"
#include "osi_dma_txrx.h"

#define OSI_PKT_CX_VLAN	OSI_BIT(0)

/**
 *	struct osi_rx_desc - Receive Descriptor
 *	@rdes0: Receive Descriptor 0
 *	@rdes1: Receive Descriptor 1
 *	@rdes2: Receive Descriptor 2
 *	@rdes3: Receive Descriptor 3
 */
struct osi_rx_desc {
	unsigned int rdes0;
	unsigned int rdes1;
	unsigned int rdes2;
	unsigned int rdes3;
};

/**
 *	struct osi_rx_swcx - Receive descriptor software context
 *	@buf_phy_addr: DMA buffer physical address
 *	@buf_virt_addr: DMA buffer virtual address
 *	@len: Length of buffer
 */
struct osi_rx_swcx {
	unsigned long buf_phy_addr;
	void *buf_virt_addr;
	unsigned int len;
};

/**
 *	struct osi_rx_pkt_cx - Received packet context.
 *	@flags: Bit map which holds the features that rx packets supports.
 *	@vlan_tag: Stores the VLAN tag ID in received packet.
 *	@pkt_len: Length of received packet.
 */
struct osi_rx_pkt_cx {
	unsigned int flags;
	unsigned int vlan_tag;
	unsigned int pkt_len;
};

/**
 *	struct osi_rx_ring - DMA Rx channel ring
 *	@rx_desc: Pointer to Rx DMA descriptor
 *	@rx_swcx: Pointer to Rx DMA descriptor software context information
 *	@dma_rx_desc: Physical address of Rx DMA descriptor
 *	@cur_rx_idx: Descriptor index current reception.
 *	@refill_idx: Descriptor index for descriptor re-allocation.
 *	@rx_pkt_cx: Received packet context.
 */
struct osi_rx_ring {
	struct osi_rx_desc *rx_desc;
	struct osi_rx_swcx *rx_swcx;
	unsigned long rx_desc_phy_addr;
	unsigned int cur_rx_idx;
	unsigned int refill_idx;
	struct osi_rx_pkt_cx rx_pkt_cx;
};

/**
 *      struct osi_tx_swcx - Transmit descriptor software context
 *      @buf_phy_addr: Physical address of DMA mapped buffer.
 *      @buf_virt_addr: Virtual address of DMA buffer.
 *      @len: Length of buffer
 */
struct osi_tx_swcx {
	unsigned long buf_phy_addr;
	void *buf_virt_addr;
	unsigned int len;
};

/**
 *      struct osi_tx_desc - Transmit descriptor
 *      @tdes0: Transmit descriptor 0
 *      @tdes1: Transmit descriptor 1
 *      @tdes2: Transmit descriptor 2
 *      @tdes3: Transmit descriptor 3
 */
struct osi_tx_desc {
	unsigned int tdes0;
	unsigned int tdes1;
	unsigned int tdes2;
	unsigned int tdes3;
};

/**
 *	struct osi_tx_pkt_cx - Transmit packet context for a packet
 *	@flags: Holds the features which a Tx packets supports.
 *	@vtag_id: Stores the VLAN tag ID.
 *	@desc_cnt: Descriptor count
 */
struct osi_tx_pkt_cx {
	unsigned int flags;
	unsigned int vtag_id;
	unsigned int desc_cnt;
};

/**
 *      struct osi_tx_ring - DMA channel Tx ring
 *	@tx_desc: Pointer to tx dma descriptor
 *	@tx_swcx: Pointer to tx dma descriptor software context information
 *	@tx_desc_phy_addr: Physical address of Tx descriptor.
 *	@cur_tx_idx: Descriptor index current transmission.
 *	@clean_idx: Descriptor index for descriptor cleanup.
 *	@tx_pkt_cx: Transmit packet context.
 */
struct osi_tx_ring {
	struct osi_tx_desc *tx_desc;
	struct osi_tx_swcx *tx_swcx;
	unsigned long tx_desc_phy_addr;
	unsigned int cur_tx_idx;
	unsigned int clean_idx;
	struct osi_tx_pkt_cx tx_pkt_cx;
};

struct osi_dma_priv_data;
/**
 *	struct osi_dma_chan_ops - MAC Hardware operations
 *	@set_tx_ring_len: Called to set Transmit Ring length.
 *	@set_tx_ring_start_addr: Called to set Transmit Ring Base address.
 *	@update_tx_tailptr: Called to update Tx Ring tail pointer.
 *	@set_rx_ring_len: Called to set Receive channel ring length.
 *	@set_rx_ring_start_addr: Called to set receive channel ring base address
 *	@update_rx_tailptr: Called to update Rx ring tail pointer.
 *	@clear_tx_intr: Invoked by OSD layer to clear Tx interrupt source.
 *	@clear_rx_intr: Invoked by OSD layer to clear Rx interrupt source.
 *	@disable_chan_tx_intr: Called to disable DMA tx channel interrupts at
 *	wrapper level.
 *	@enable_chan_tx_intr: Called to enable DMA tx channel interrupts at
 *	wrapper level.
 *	@disable_chan_rx_intr: Called to disable DMA Rx channel interrupts at
 *	wrapper level.
 *	@enable_chan_rx_intr: Called to enable DMA rx channel interrupts at
 *	wrapper level.
 *	@start_dma: Called to start the Tx/Rx DMA.
 *	@set_rx_buf_len: Called to set Rx buffer length.
 */
struct osi_dma_chan_ops {
	void (*set_tx_ring_len)(void *addr, unsigned int chan,
				unsigned int len);
	void (*set_tx_ring_start_addr)(void *addr, unsigned int chan,
				       unsigned long base_addr);
	void (*update_tx_tailptr)(void *addr, unsigned int chan,
				  unsigned long tailptr);
	void (*set_rx_ring_len)(void *addr, unsigned int chan,
				unsigned int len);
	void (*set_rx_ring_start_addr)(void *addr, unsigned int chan,
				       unsigned long base_addr);
	void (*update_rx_tailptr)(void *addr, unsigned int chan,
				  unsigned long tailptr);
	void (*clear_tx_intr)(void *addr, unsigned int chan);
	void (*clear_rx_intr)(void *addr, unsigned int chan);
	void (*disable_chan_tx_intr)(void *addr, unsigned int chan);
	void (*enable_chan_tx_intr)(void *addr, unsigned int chan);
	void (*disable_chan_rx_intr)(void *addr, unsigned int chan);
	void (*enable_chan_rx_intr)(void *addr, unsigned int chan);
	void (*start_dma)(void *addr, unsigned int chan);
	void (*stop_dma)(void *addr, unsigned int chan);
	void (*init_dma_channel) (struct osi_dma_priv_data *osi_dma);
	void (*set_rx_buf_len)(struct osi_dma_priv_data *osi_dma);
};

/**
 *	struct osi_dma_priv_data - The OSI private data structure.
 *	@tx_ring: Array of pointers to DMA Tx channel Ring.
 *	@rx_ring: Array of pointers to DMA Rx channel Ring.
 *	@base:	Memory mapped base address of MAC IP.
 *	@osd:	Pointer to OSD private data structure.
 *	@ops:	Address of HW operations structure.
 *	@mac:	MAC HW type (EQOS).
 *	@num_dma_chans:	Number of channels enabled in MAC.
 *	@dma_chans[]:	Array of supported DMA channels
 *	@rx_buf_len:	DMA Rx channel buffer length at HW level.
 *	@mtu:	MTU size
 */
struct osi_dma_priv_data {
	struct osi_tx_ring *tx_ring[OSI_EQOS_MAX_NUM_CHANS];
	struct osi_rx_ring *rx_ring[OSI_EQOS_MAX_NUM_CHANS];
	void *base;
	void *osd;
	struct osi_dma_chan_ops *ops;
	unsigned int mac;
	unsigned int num_dma_chans;
	unsigned int dma_chans[OSI_EQOS_MAX_NUM_CHANS];
	unsigned int rx_buf_len;
	unsigned int mtu;
};

/**
 *	osi_disable_chan_tx_intr - Disables DMA Tx channel interrupts.
 *	@osi_dma: DMA private data.
 *	@chan: DMA Tx channel number.
 *
 *	Algorithm: Disables Tx interrupts at wrapper level.
 *
 *	Dependencies: osi_dma_chan_ops needs to be assigned based on MAC.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
static inline void osi_disable_chan_tx_intr(struct osi_dma_priv_data *osi_dma,
					    unsigned int chan)
{
	if ((osi_dma != OSI_NULL) && (osi_dma->ops != OSI_NULL) &&
	    (osi_dma->ops->disable_chan_tx_intr != OSI_NULL)) {
		osi_dma->ops->disable_chan_tx_intr(osi_dma->base, chan);
	}
}

/**
 *	osi_enable_chan_tx_intr - Enable DMA Tx channel interrupts.
 *	@osi_dma: DMA private data.
 *	@chan: DMA Tx channel number.
 *
 *	Algorithm: Enables Tx interrupts at wrapper level.
 *
 *	Dependencies: osi_dma_chan_ops needs to be assigned based on MAC.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
static inline void osi_enable_chan_tx_intr(struct osi_dma_priv_data *osi_dma,
					   unsigned int chan)
{
	if ((osi_dma != OSI_NULL) && (osi_dma->ops != OSI_NULL) &&
	    (osi_dma->ops->enable_chan_tx_intr != OSI_NULL)) {
		osi_dma->ops->enable_chan_tx_intr(osi_dma->base, chan);
	}
}

/**
 *	osi_disable_chan_rx_intr - Disable DMA Rx channel interrupts.
 *	@osi_dma: DMA private data.
 *	@chan: DMA rx channel number.
 *
 *	Algorithm: Disables Tx interrupts at wrapper level.
 *
 *	Dependencies: osi_dma_chan_ops needs to be assigned based on MAC.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
static inline void osi_disable_chan_rx_intr(struct osi_dma_priv_data *osi_dma,
					    unsigned int chan)
{
	if ((osi_dma != OSI_NULL) && (osi_dma->ops != OSI_NULL) &&
	    (osi_dma->ops->disable_chan_rx_intr != OSI_NULL)) {
		osi_dma->ops->disable_chan_rx_intr(osi_dma->base, chan);
	}
}

/**
 *	osi_enable_chan_rx_intr - Enable DMA Rx channel interrupts.
 *	@osi_dma: DMA private data.
 *	@chan: DMA rx channel number.
 *
 *	Algorithm: Enables Tx interrupts at wrapper level.
 *
 *	Dependencies: osi_dma_chan_ops needs to be assigned based on MAC.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
static inline void osi_enable_chan_rx_intr(struct osi_dma_priv_data *osi_dma,
					   unsigned int chan)
{
	if ((osi_dma != OSI_NULL) && (osi_dma->ops != OSI_NULL) &&
	    (osi_dma->ops->enable_chan_rx_intr != OSI_NULL)) {
		osi_dma->ops->enable_chan_rx_intr(osi_dma->base, chan);
	}
}

/**
 *	osi_clear_tx_intr - Handles Tx interrupt source.
 *	@osi_dma: DMA private data.
 *	@chan: DMA tx channel number.
 *
 *	Algorithm: Clear Tx interrupt source at wrapper level and DMA level.
 *
 *	Dependencies: osi_dma_chan_ops needs to be assigned based on MAC.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
static inline void osi_clear_tx_intr(struct osi_dma_priv_data *osi_dma,
				      unsigned int chan)
{
	if ((osi_dma != OSI_NULL) && (osi_dma->ops != OSI_NULL) &&
	    (osi_dma->ops->clear_tx_intr != OSI_NULL)) {
		osi_dma->ops->clear_tx_intr(osi_dma->base, chan);
	}
}

/**
 *	osi_clear_rx_intr - Handles Rx interrupt source.
 *	@osi_dma: DMA private data.
 *	@chan: DMA rx channel number.
 *
 *	Algorithm: Clear Rx interrupt source at wrapper level and DMA level.
 *
 *	Dependencies: osi_dma_chan_ops needs to be assigned based on MAC.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
static inline void osi_clear_rx_intr(struct osi_dma_priv_data *osi_dma,
				      unsigned int chan)
{
	if ((osi_dma != OSI_NULL) && (osi_dma->ops != OSI_NULL) &&
	    (osi_dma->ops->clear_rx_intr != OSI_NULL)) {
		osi_dma->ops->clear_rx_intr(osi_dma->base, chan);
	}
}

/**
 *	osi_start_dma - Start DMA
 *	@osi_dma: DMA private data.
 *	@chan: DMA Tx/Rx channel number
 *
 *	Algorimthm: Start the DMA for specific MAC
 *	Dependencies: None
 *	Protection: None
 *	Return: None
 */
static inline void osi_start_dma(struct osi_dma_priv_data *osi_dma,
				 unsigned int chan)
{
	if ((osi_dma != OSI_NULL) && (osi_dma->ops != OSI_NULL) &&
	    (osi_dma->ops->start_dma != OSI_NULL)) {
		osi_dma->ops->start_dma(osi_dma->base, chan);
	}
}

/**
 *	osi_stop_dma - Stop DMA
 *	@osi_dma: DMA private data.
 *	@chan: DMA Tx/Rx channel number
 *
 *	Algorimthm: Stop the DMA for specific MAC
 *	Dependencies: None
 *	Protection: None
 *	Return: None
 */
static inline void osi_stop_dma(struct osi_dma_priv_data *osi_dma,
				unsigned int chan)
{
	if ((osi_dma != OSI_NULL) && (osi_dma->ops != OSI_NULL) &&
	    (osi_dma->ops->stop_dma != OSI_NULL)) {
		osi_dma->ops->stop_dma(osi_dma->base, chan);
	}
}

/**
 *      osi_get_refill_rx_desc_cnt - Rx descriptors count that needs to refill.
 *      @rx_ring: DMA channel Rx ring.
 *
 *      Algorithm: subtract current index with fill (need to cleanup)
 *      to get Rx descriptors count that needs to refill.
 *
 *      Dependencies: None.
 *
 *      Protection: None.
 *
 *      Return: Number of available free descriptors.
 */
static inline unsigned int osi_get_refill_rx_desc_cnt(struct osi_rx_ring *rx_ring)
{
	return (rx_ring->cur_rx_idx - rx_ring->refill_idx) & (RX_DESC_CNT - 1U);
}

/**
 *	osi_rx_dma_desc_init - DMA Rx descriptor init
 *	@rx_swcx: OSI DMA Rx ring software context
 *	@rx_desc: OSI DMA Rx ring descriptor
 *
 *	Algorithm: Initialise a Rx DMA descriptor.
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
static inline void osi_rx_dma_desc_init(struct osi_rx_swcx *rx_swcx,
					struct osi_rx_desc *rx_desc)
{
	rx_desc->rdes0 = (unsigned int)L32(rx_swcx->buf_phy_addr);
	rx_desc->rdes1 = (unsigned int)H32(rx_swcx->buf_phy_addr);
	rx_desc->rdes2 = 0;
	rx_desc->rdes3 = (RDES3_OWN | RDES3_IOC | RDES3_B1V);
}

/**
 *	osi_update_rx_tailptr - Updates DMA Rx ring tail pointer
 *	@osi_dma: OSI DMA private data struture.
 *	@rx_ring: Pointer to DMA Rx ring.
 *	@chan: DMA channel number.
 *
 *	Algorithm: Updates DMA Rx ring tail pointer.
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
static inline void osi_update_rx_tailptr(struct osi_dma_priv_data *osi_dma,
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

/**
 *	osi_set_rx_buf_len - Updates rx buffer length.
 *	@osi_dma: OSI DMA private data struture.
 *
 *	Algorithm: Updates Rx buffer length.
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: None.
 */

static inline void osi_set_rx_buf_len(struct osi_dma_priv_data *osi_dma)
{
	if (osi_dma != OSI_NULL && osi_dma->ops != OSI_NULL &&
	    osi_dma->ops->set_rx_buf_len != OSI_NULL) {
		osi_dma->ops->set_rx_buf_len(osi_dma);
	}
}

void osi_hw_transmit(struct osi_dma_priv_data *osi, unsigned int chan);
int osi_process_tx_completions(struct osi_dma_priv_data *osi,
			       unsigned int chan);
int osi_process_rx_completions(struct osi_dma_priv_data *osi,
			       unsigned int chan, int budget);
int osi_hw_dma_init(struct osi_dma_priv_data *osi_dma);
void osi_hw_dma_deinit(struct osi_dma_priv_data *osi_dma);
void osi_init_dma_ops(struct osi_dma_priv_data *osi_dma);
#endif /* OSI_DMA_H */
