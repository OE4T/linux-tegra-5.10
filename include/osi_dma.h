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
#include "mmc.h"

#define OSI_PKT_CX_VLAN			OSI_BIT(0)
#define OSI_PKT_CX_VALID		OSI_BIT(10)
#define OSI_PKT_CX_CSUM			OSI_BIT(1)
#define OSI_PKT_CX_TSO			OSI_BIT(2)
#define OSI_PKT_CX_PTP			OSI_BIT(3)

/* Flag to indicate if buffer programmed in desc. is DMA map'd from
 * linear/Paged buffer from OS layer.
 */
#define OSI_TXDONE_CX_PAGED_BUF		OSI_BIT(0)
/* Flag to indicate if there was any tx error */
#define OSI_TXDONE_CX_ERROR		OSI_BIT(1)
#define OSI_TXDONE_CX_TS		OSI_BIT(2)

/* Checksum offload result flags */
#define OSI_CHECKSUM_NONE		0x0U
#define OSI_CHECKSUM_UNNECESSARY	0x1U

/**
 *	struct osi_pkt_err_stats: OSI packet error stats
 *	@ip_header_error: IP Header Error
 *	@jabber_timeout_error: Jabber time out Error
 *	@pkt_flush_error: Packet Flush Error
 *	@payload_cs_error: Payload Checksum Error
 *	@loss_of_carrier_error: Loss of Carrier Error
 *	@no_carrier_error: No Carrier Error
 *	@late_collision_error: Late Collision Error
 *	@excessive_collision_error: Excessive Collision Error
 *	@excessive_deferal_error: Excessive Deferal Error
 *	@underflow_error: Under Flow Error
 *	@rx_crc_error: Rx CRC Error
 */
struct osi_pkt_err_stats {
	/* Transmit errors */
	unsigned long ip_header_error;
	unsigned long jabber_timeout_error;
	unsigned long pkt_flush_error;
	unsigned long payload_cs_error;
	unsigned long loss_of_carrier_error;
	unsigned long no_carrier_error;
	unsigned long late_collision_error;
	unsigned long excessive_collision_error;
	unsigned long excessive_deferal_error;
	unsigned long underflow_error;
	/* Receive Errors */
	unsigned long rx_crc_error;
};

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
 *	@rxcsum: Stores the Rx csum
 *	@vlan_tag: Stores the VLAN tag ID in received packet.
 *	@pkt_len: Length of received packet.
 *	@ns: TS in nsec for the received packet
 */
struct osi_rx_pkt_cx {
	unsigned int flags;
	unsigned int rxcsum;
	unsigned int vlan_tag;
	unsigned int pkt_len;
	unsigned long long ns;
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
 *	struct osi_tx_swcx - Transmit descriptor software context
 *	@buf_phy_addr: Physical address of DMA mapped buffer.
 *	@buf_virt_addr: Virtual address of DMA buffer.
 *	@len: Length of buffer
 *	@is_paged_buf: Flag to keep track of whether buffer pointed
 *	by buf_phy_addr is a paged buffer/linear buffer.
 */
struct osi_tx_swcx {
	unsigned long buf_phy_addr;
	void *buf_virt_addr;
	unsigned int len;
	unsigned int is_paged_buf;
};

/**
 *	struct osi_tx_desc - Transmit descriptor
 *	@tdes0: Transmit descriptor 0
 *	@tdes1: Transmit descriptor 1
 *	@tdes2: Transmit descriptor 2
 *	@tdes3: Transmit descriptor 3
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
 *	@mss: Max. segment size for TSO/USO/GSO/LSO packet
 *	@payload_len: Length of application payload
 *	@tcp_udp_hdrlen: Length of transport layer tcp/udp header
 *	@total_hdrlen: Length of all headers (ethernet/ip/tcp/udp)
 */
struct osi_tx_pkt_cx {
	unsigned int flags;
	unsigned int vtag_id;
	unsigned int desc_cnt;
	unsigned int mss;
	unsigned int payload_len;
	unsigned int tcp_udp_hdrlen;
	unsigned int total_hdrlen;
};

/**
 *	struct osi_txdone_pkt_cx - Transmit done packet context for a packet
 *	@flags: Indicates status flags for Tx complete (tx error occured, or
 *	indicate whether desc. had buf mapped from paged/linear memory etc.)
 *	@ns: TS captured for the tx packet and this is valid only when the PTP
 *	bit is set in fields
 */
struct osi_txdone_pkt_cx {
	unsigned int flags;
	unsigned long long ns;
};

/**
 *	struct osi_tx_ring - DMA channel Tx ring
 *	@tx_desc: Pointer to tx dma descriptor
 *	@tx_swcx: Pointer to tx dma descriptor software context information
 *	@tx_desc_phy_addr: Physical address of Tx descriptor.
 *	@cur_tx_idx: Descriptor index current transmission.
 *	@clean_idx: Descriptor index for descriptor cleanup.
 *	@tx_pkt_cx: Transmit packet context.
 *	@txdone_pkt_cx: Transmit complete packet context information.
 */
struct osi_tx_ring {
	struct osi_tx_desc *tx_desc;
	struct osi_tx_swcx *tx_swcx;
	unsigned long tx_desc_phy_addr;
	unsigned int cur_tx_idx;
	unsigned int clean_idx;
	struct osi_tx_pkt_cx tx_pkt_cx;
	struct osi_txdone_pkt_cx txdone_pkt_cx;
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
 *	@pkt_err_stats: Packet error stats
 *	@dstats: Extra DMA stats
 *	@rx_riwt: Receive Interrupt Watchdog Timer Count Units
 *	@use_riwt: Flag which decides riwt is enabled(1) or disabled(0)
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
	struct osi_pkt_err_stats pkt_err_stats;
	struct osi_xtra_dma_stat_counters dstats;
	unsigned int rx_riwt;
	unsigned int use_riwt;
};

/**
 *	osi_disable_chan_tx_intr - Disables DMA Tx channel interrupts.
 *	@osi_dma: DMA private data.
 *	@chan: DMA Tx channel number.
 *
 *	Algorithm: Disables Tx interrupts at wrapper level.
 *
 *	Dependencies:
 *	1) MAC needs to be out of reset and proper clocks need to be configured.
 *	2) DMA HW init need to be completed successfully, see osi_hw_dma_init
 *	3) Mapping of physical IRQ line to DMA channel need to be maintained at
 *	OSDependent layer and pass corresponding channel number.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
void osi_disable_chan_tx_intr(struct osi_dma_priv_data *osi_dma,
			      unsigned int chan);

/**
 *	osi_enable_chan_tx_intr - Enable DMA Tx channel interrupts.
 *	@osi_dma: DMA private data.
 *	@chan: DMA Tx channel number.
 *
 *	Algorithm: Enables Tx interrupts at wrapper level.
 *
 *	Dependencies:
 *	1) MAC needs to be out of reset and proper clocks need to be configured.
 *	2) DMA HW init need to be completed successfully, see osi_hw_dma_init
 *	3) Mapping of physical IRQ line to DMA channel need to be maintained at
 *	OSDependent layer and pass corresponding channel number.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
void osi_enable_chan_tx_intr(struct osi_dma_priv_data *osi_dma,
			     unsigned int chan);

/**
 *	osi_disable_chan_rx_intr - Disable DMA Rx channel interrupts.
 *	@osi_dma: DMA private data.
 *	@chan: DMA rx channel number.
 *
 *	Algorithm: Disables Rx interrupts at wrapper level.
 *
 *	Dependencies:
 *	1) MAC needs to be out of reset and proper clocks need to be configured.
 *	2) DMA HW init need to be completed successfully, see osi_hw_dma_init
 *	3) Mapping of physical IRQ line to DMA channel need to be maintained at
 *	OSDependent layer and pass corresponding channel number.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
void osi_disable_chan_rx_intr(struct osi_dma_priv_data *osi_dma,
			      unsigned int chan);

/**
 *	osi_enable_chan_rx_intr - Enable DMA Rx channel interrupts.
 *	@osi_dma: DMA private data.
 *	@chan: DMA rx channel number.
 *
 *	Algorithm: Enables Rx interrupts at wrapper level.
 *
 *	Dependencies:
 *	1) MAC needs to be out of reset and proper clocks need to be configured.
 *	2) DMA HW init need to be completed successfully, see osi_hw_dma_init
 *	3) Mapping of physical IRQ line to DMA channel need to be maintained at
 *	OSDependent layer and pass corresponding channel number.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
void osi_enable_chan_rx_intr(struct osi_dma_priv_data *osi_dma,
			     unsigned int chan);

/**
 *	osi_clear_tx_intr - Handles Tx interrupt source.
 *	@osi_dma: DMA private data.
 *	@chan: DMA tx channel number.
 *
 *	Algorithm: Clear Tx interrupt source at wrapper level and DMA level.
 *
 *	Dependencies:
 *	1) MAC needs to be out of reset and proper clocks need to be configured.
 *	2) DMA HW init need to be completed successfully, see osi_hw_dma_init
 *
 *	Protection: None.
 *
 *	Return: None.
 */
void osi_clear_tx_intr(struct osi_dma_priv_data *osi_dma,
		       unsigned int chan);

/**
 *	osi_clear_rx_intr - Handles Rx interrupt source.
 *	@osi_dma: DMA private data.
 *	@chan: DMA rx channel number.
 *
 *	Algorithm: Clear Rx interrupt source at wrapper level and DMA level.
 *
 *	Dependencies:
 *	1) MAC needs to be out of reset and proper clocks need to be configured.
 *	2) DMA HW init need to be completed successfully, see osi_hw_dma_init
 *	3) Mapping of physical IRQ line to DMA channel need to be maintained at
 *	OSDependent layer and pass corresponding channel number.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
void osi_clear_rx_intr(struct osi_dma_priv_data *osi_dma,
		       unsigned int chan);

/**
 *	osi_start_dma - Start DMA
 *	@osi_dma: DMA private data.
 *	@chan: DMA Tx/Rx channel number
 *
 *	Algorimthm: Start the DMA for specific MAC
 *
 *	Dependencies:
 *	1) MAC needs to be out of reset and proper clocks need to be configured.
 *	2) DMA HW init need to be completed successfully, see osi_hw_dma_init
 *
 *	Protection: None
 *
 *	Return: None
 */
void osi_start_dma(struct osi_dma_priv_data *osi_dma,
		   unsigned int chan);

/**
 *	osi_stop_dma - Stop DMA
 *	@osi_dma: DMA private data.
 *	@chan: DMA Tx/Rx channel number
 *
 *	Algorimthm: Stop the DMA for specific MAC
 *
 *	Dependencies:
 *	1) MAC needs to be out of reset and proper clocks need to be configured.
 *	2) DMA HW init need to be completed successfully, see osi_hw_dma_init
 *
 *	Protection: None
 *	Return: None
 */
void osi_stop_dma(struct osi_dma_priv_data *osi_dma,
		  unsigned int chan);

/**
 *	osi_get_refill_rx_desc_cnt - Rx descriptors count that needs to refill.
 *	@rx_ring: DMA channel Rx ring.
 *
 *	Algorithm: subtract current index with fill (need to cleanup)
 *	to get Rx descriptors count that needs to refill.
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: Number of available free descriptors.
 */
unsigned int osi_get_refill_rx_desc_cnt(struct osi_rx_ring *rx_ring);

/**
 *	osi_rx_dma_desc_init - DMA Rx descriptor init
 *	@rx_swcx: OSI DMA Rx ring software context
 *	@rx_desc: OSI DMA Rx ring descriptor
 *	@use_riwt: to enable Rx WDT and disable IOC
 *
 *	Algorithm: Initialise a Rx DMA descriptor.
 *
 *	Dependencies:
 *	1) MAC needs to be out of reset and proper clocks need to be configured.
 *	2) rx_swcx->buf_phy_addr need to be filled with DMA mapped address
 *	2) DMA HW init need to be completed successfully, see osi_hw_dma_init
 *
 *	Protection: None.
 *
 *	Return: None.
 */
void osi_rx_dma_desc_init(struct osi_rx_swcx *rx_swcx,
			  struct osi_rx_desc *rx_desc,
			  unsigned int use_riwt);

/**
 *	osi_update_rx_tailptr - Updates DMA Rx ring tail pointer
 *	@osi_dma: OSI DMA private data struture.
 *	@rx_ring: Pointer to DMA Rx ring.
 *	@chan: DMA channel number.
 *
 *	Algorithm: Updates DMA Rx ring tail pointer.
 *
 *	Dependencies:
 *	1) MAC needs to be out of reset and proper clocks need to be configured.
 *	2) DMA HW init need to be completed successfully, see osi_hw_dma_init
 *
 *	Protection: None.
 *
 *	Return: None.
 */
void osi_update_rx_tailptr(struct osi_dma_priv_data *osi_dma,
			   struct osi_rx_ring *rx_ring,
			   unsigned int chan);

/**
 *	osi_set_rx_buf_len - Updates rx buffer length.
 *	@osi_dma: OSI DMA private data struture.
 *
 *	Algorithm: Updates Rx buffer length.
 *
 *	Dependencies:
 *	1) MAC needs to be out of reset and proper clocks need to be configured.
 *	2) DMA HW init need to be completed successfully, see osi_hw_dma_init
 *	3) osi_dma->mtu need to be filled with current MTU size <= 9K
 *
 *	Protection: None.
 *
 *	Return: None.
 */
void osi_set_rx_buf_len(struct osi_dma_priv_data *osi_dma);

/**
 *	osi_hw_transmit - Initialize Tx DMA descriptors for a channel
 *	@osi_dma: DMA private data.
 *	@chan: DMA Tx channel number.
 *
 *	Algorithm: Initialize Transmit descriptors with DMA mappabled buffers,
 *	set OWN bit, Tx ring length and set starting address of Tx DMA channel.
 *	Tx ring base address in Tx DMA registers.
 *
 *	Dependencies:
 *	1) MAC needs to be out of reset and proper clocks need to be configured.
 *	2) DMA HW init need to be completed successfully, see osi_hw_dma_init
 *	3) DMA channel need to be started, see osi_start_dma
 *	4) Need to set update tx_pkt_cx->flags accordingly as per the
 *	requirements
 *	#define OSI_PKT_CX_VLAN                 OSI_BIT(0)
 *	#define OSI_PKT_CX_CSUM                 OSI_BIT(1)
 *	#define OSI_PKT_CX_TSO                  OSI_BIT(2)
 *	#define OSI_PKT_CX_PTP                  OSI_BIT(3)
 *	5) tx_pkt_cx->desc_cnt need to be populated which holds the number
 *	of swcx descriptors allocated for that packet
 *	6) tx_swcx structure need to be filled for per packet with the
 *	buffer len, DMA mapped address of buffer for each descriptor
 *	consumed by the packet
 *
 *	Protection: None.
 *
 *	Return: None.
 */
void osi_hw_transmit(struct osi_dma_priv_data *osi, unsigned int chan);

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
 *	Dependencies:
 *	1) MAC needs to be out of reset and proper clocks need to be configured.
 *	2) DMA HW init need to be completed successfully, see osi_hw_dma_init
 *	3) DMA need to be started, see osi_start_dma
 *
 *	Protection: None
 *
 *	Return: Number of decriptors (buffers) proccessed.
 */
int osi_process_tx_completions(struct osi_dma_priv_data *osi,
			       unsigned int chan);

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
 *	Dependencies:
 *	1) MAC needs to be out of reset and proper clocks need to be configured.
 *	2) DMA HW init need to be completed successfully, see osi_hw_dma_init
 *	3) DMA need to be started, see osi_start_dma
 *
 *	Protection: None.
 *
 *	Return: None.
 */
int osi_process_rx_completions(struct osi_dma_priv_data *osi,
			       unsigned int chan, int budget);

/**
 *	osi_hw_dma_init - Initialize DMA
 *	@osi_dma: DMA private data.
 *
 *	Algorithm: Takes care of initializing the tx, rx ring and descriptors
 *	based on the number of channels selected.
 *
 *	Dependencies:
 *	1) Allocate memory for osi_dma
 *	2) MAC needs to be out of reset and proper clocks need to be configured.
 *	3) Numer of dma channels osi_dma->num_dma_chans
 *	4) channel list osi_dma->dma_chan
 *	5) base address osi_dma->base
 *	6) allocate tx ring osi_dma->tx_ring[chan] for each channel
 *	based on TX_DESC_CNT (256)
 *	7) allocate tx descriptors osi_dma->tx_ring[chan]->tx_desc for all
 *	channels and dma map it.
 *	8) allocate tx sw descriptors osi_dma->tx_ring[chan]->tx_swcx for all
 *	channels
 *	9) allocate rx ring osi_dma->rx_ring[chan] for each channel
 *	based on RX_DESC_CNT (256)
 *	10) allocate rx descriptors osi_dma->rx_ring[chan]->rx_desc for all
 *	channels and dma map it.
 *	11) allocate rx sw descriptors osi_dma->rx_ring[chan]->rx_swcx for all
 *	channels
 *	12) osi_dma->use_riwt  ==> OSI_DISABLE/OSI_ENABLE
 *	13) osi_dma->rx_riwt  ===> Actual value read from DT
 *
 *	Protection: None.
 *
 *	Return: None.
 */
int osi_hw_dma_init(struct osi_dma_priv_data *osi_dma);

/**
 *	osi_hw_dma_deinit - Deinitialize DMA
 *	@osi_dma: DMA private data.
 *
 *	Algorithm: Takes care of stopping the MAC
 *
 *	Dependencies:
 *	1) MAC needs to be out of reset and proper clocks need to be configured.
 *	2) DMA HW init need to be completed successfully, see osi_hw_dma_init
 *
 *	Protection: None.
 *
 *	Return: None.
 */
void osi_hw_dma_deinit(struct osi_dma_priv_data *osi_dma);

/**
 *	osi_init_dma_ops - Initialize DMA operations
 *	@osi_dma: DMA private data.
 *
 *	Algorithm: Takes care of initializing the DMA operations
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
void osi_init_dma_ops(struct osi_dma_priv_data *osi_dma);

/**
 *	osi_clear_tx_pkt_err_stats - Clear tx packet error stats.
 *	@osi: OSI dma private data structure.
 *
 *	Algorithm: This function will be invoked by OSD layer to clear the
 *	tx stats mentioned in osi_dma->pkt_err_stats structure

 *	Dependencies:
 *	1) MAC needs to be out of reset and proper clocks need to be configured.
 *	2) DMA HW init need to be completed successfully, see osi_hw_dma_init
 *
 *	Protection: None
 *
 *	Return: None
 */
void osi_clear_tx_pkt_err_stats(struct osi_dma_priv_data *osi_dma);

/**
 *	osi_clear_rx_pkt_err_stats - Clear rx packet error stats.
 *	@osi: OSI dma private data structure.
 *
 *	Algorithm: This function will be invoked by OSD layer to clear the
 *	rx_crc_error mentioned in osi_dma->pkt_err_stats structure.
 *
 *	Dependencies:
 *	1) MAC needs to be out of reset and proper clocks need to be configured.
 *	2) DMA HW init need to be completed successfully, see osi_hw_dma_init
 *
 *	Protection: None
 *
 *	Return: None
 */
void osi_clear_rx_pkt_err_stats(struct osi_dma_priv_data *osi_dma);
#endif /* OSI_DMA_H */
