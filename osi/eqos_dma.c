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

#include <osi_common.h>
#include <osi_dma.h>
#include "eqos_dma.h"

struct osi_dma_chan_ops *eqos_get_dma_chan_ops(void);

/**
 *	eqos_disable_chan_tx_intr - Disable Tx channel interrupts.
 *	@addr: MAC base address.
 *	@chan: DMA Tx channel number.
 *
 *	Algorithm: Disables EQOS DMA tx channel interrupts.
 *
 *	Dependencies: None.
 *	Protection: None.
 *	Return: None.
 */
static void eqos_disable_chan_tx_intr(void *addr, unsigned int chan)
{
	unsigned int cntrl;

	cntrl = osi_readl((unsigned char *)addr + EQOS_VIRT_INTR_CHX_CNTRL(chan));
	cntrl &= ~EQOS_VIRT_INTR_CHX_CNTRL_TX;
	osi_writel(cntrl, (unsigned char *)addr + EQOS_VIRT_INTR_CHX_CNTRL(chan));
}

/**
 *	eqos_enable_chan_tx_intr - Enable Tx channel interrupts.
 *	@addr: MAC base address.
 *	@chan: DMA Tx channel number.
 *
 *	Algorithm: Enables EQOS DMA tx channel interrupts.
 *
 *	Dependencies: None.
 *	Protection: None.
 *	Return: None.
 */
static void eqos_enable_chan_tx_intr(void *addr, unsigned int chan)
{
	unsigned int cntrl;

	cntrl = osi_readl((unsigned char *)addr + EQOS_VIRT_INTR_CHX_CNTRL(chan));
	cntrl |= EQOS_VIRT_INTR_CHX_CNTRL_TX;
	osi_writel(cntrl, (unsigned char *)addr + EQOS_VIRT_INTR_CHX_CNTRL(chan));
}

/**
 *	eqos_disable_chan_rx_intr - Disable Rx channel interrupts.
 *	@addr: MAC base address.
 *	@chan: DMA Rx channel number.
 *
 *	Algorithm: Disables EQOS DMA rx channel interrupts.
 *
 *	Dependencies: None.
 *	Protection: None.
 *	Return: None.
 */
static void eqos_disable_chan_rx_intr(void *addr, unsigned int chan)
{
	unsigned int cntrl;

	cntrl = osi_readl((unsigned char *)addr + EQOS_VIRT_INTR_CHX_CNTRL(chan));
	cntrl &= ~EQOS_VIRT_INTR_CHX_CNTRL_RX;
	osi_writel(cntrl, (unsigned char *)addr + EQOS_VIRT_INTR_CHX_CNTRL(chan));
}

/**
 *	eqos_enable_chan_rx_intr - Enable Rx channel interrupts.
 *	@addr: MAC base address.
 *	@chan: DMA Rx channel number.
 *
 *	Algorithm: Enables EQOS DMA Rx channel interrupts.
 *
 *	Dependencies: None.
 *	Protection: None.
 *	Return: None.
 */
static void eqos_enable_chan_rx_intr(void *addr, unsigned int chan)
{
	unsigned int cntrl;

	cntrl = osi_readl((unsigned char *)addr + EQOS_VIRT_INTR_CHX_CNTRL(chan));
	cntrl |= EQOS_VIRT_INTR_CHX_CNTRL_RX;
	osi_writel(cntrl, (unsigned char *)addr + EQOS_VIRT_INTR_CHX_CNTRL(chan));
}

/**
 *	eqos_clear_tx_intr - Handle EQOS DMA Tx channel interrupts.
 *	@addr: MAC base address.
 *	@chan: DMA Tx channel number.
 *
 *	Algorithm: Clear DMA Tx interrupt source at wrapper and DMA level.
 *
 *	Dependencies: None.
 *	Protection: None.
 *	Return: None.
 */
static void eqos_clear_tx_intr(void *addr, unsigned int chan)
{
	unsigned int status;

	status = osi_readl((unsigned char *)addr + EQOS_VIRT_INTR_CHX_STATUS(chan));
	if ((status & EQOS_VIRT_INTR_CHX_STATUS_TX) == 1U) {
		osi_writel(EQOS_DMA_CHX_STATUS_CLEAR_TX,
			   (unsigned char *)addr + EQOS_DMA_CHX_STATUS(chan));
		osi_writel(EQOS_VIRT_INTR_CHX_STATUS_TX,
			   (unsigned char *)addr + EQOS_VIRT_INTR_CHX_STATUS(chan));
	}
}

/**
 *	eqos_clear_rx_intr - Handles DMA Rx channel interrupts.
 *	@addr: MAC base address.
 *	@chan: DMA Rx channel number.
 *
 *	Algorithm: Clear DMA Rx interrupt source at wrapper and DMA level.
 *
 *	Dependencies: None.
 *	Protection: None.
 *	Return: None.
 */
static void eqos_clear_rx_intr(void *addr, unsigned int chan)
{
	unsigned int status;

	status = osi_readl((unsigned char *)addr + EQOS_VIRT_INTR_CHX_STATUS(chan));
	if ((status & EQOS_VIRT_INTR_CHX_STATUS_RX) == 2U) {
		osi_writel(EQOS_DMA_CHX_STATUS_CLEAR_RX,
			   (unsigned char *)addr + EQOS_DMA_CHX_STATUS(chan));
		osi_writel(EQOS_VIRT_INTR_CHX_STATUS_RX,
			   (unsigned char *)addr + EQOS_VIRT_INTR_CHX_STATUS(chan));
	}
}

/**
 *	eqos_set_tx_ring_len - Set DMA Tx ring length.
 *	@addr: MAC base address.
 *	@chan: DMA Tx channel number.
 *	@len: Length.
 *
 *	Algorithm: Set DMA Tx channel ring length for specific channel.
 *
 *	Dependencies: None.
 *	Protection: None.
 *	Return: None.
 */
static void eqos_set_tx_ring_len(void *addr, unsigned int chan,
				 unsigned int len)
{
	osi_writel(len, (unsigned char *)addr + EQOS_DMA_CHX_TDRL(chan));
}

/**
 *	eqos_set_tx_ring_start_addr - Set DMA Tx ring base address.
 *	@addr: MAC base address.
 *	@chan: DMA Tx channel number.
 *	@tx_desc: Tx desc base addess.
 *
 *	Algorithm: Sets DMA Tx ring base address for specific channel.
 *
 *	Dependencies: None.
 *	Protection: None.
 *	Return: None.
 */
static void eqos_set_tx_ring_start_addr(void *addr, unsigned int chan,
					unsigned long tx_desc)
{
	osi_writel((unsigned int)H32(tx_desc), (unsigned char *)addr + EQOS_DMA_CHX_TDLH(chan));
	osi_writel((unsigned int)L32(tx_desc), (unsigned char *)addr + EQOS_DMA_CHX_TDLA(chan));
}

/**
 *	eqos_update_tx_tailptr - Updates DMA Tx ring tail pointer.
 *	@addr: MAC base address.
 *	@chan: DMA Tx channel number.
 *	@tailptr: DMA Tx ring tail pointer.
 *
 *	Algorithm: Updates DMA Tx ring tail pointer for specific channel.
 *
 *	Dependencies: None.
 *	Protection: None.
 *	Return: None.
 */
static void eqos_update_tx_tailptr(void *addr, unsigned int chan,
				   unsigned long tailptr)
{
	osi_writel((unsigned int)L32(tailptr), (unsigned char *)addr + EQOS_DMA_CHX_TDTP(chan));
}

/**
 *	eqos_set_rx_ring_len - Set Rx channel ring length.
 *	@addr: MAC base address.
 *	@chan: DMA Rx channel number.
 *	@len: Length
 *
 *	Algorithm: Sets DMA Rx channel ring length for specific DMA channel.
 *
 *	Dependencies: None.
 *	Protection: None.
 *	Return: None.
 */
static void eqos_set_rx_ring_len(void *addr, unsigned int chan,
				 unsigned int len)
{
	osi_writel(len, (unsigned char *)addr + EQOS_DMA_CHX_RDRL(chan));
}

/**
 *	eqos_set_rx_ring_start_addr - Set DMA Rx ring base address.
 *	@addr: MAC base address.
 *	@chan: DMA Rx channel number.
 *	@tx_desc: DMA Rx desc base address.
 *
 *	Algorithm: Sets DMA Rx channel ring base address.
 *
 *	Dependencies: None.
 *	Protection: None.
 *	Return: None.
 */
static void eqos_set_rx_ring_start_addr(void *addr, unsigned int chan,
					unsigned long tx_desc)
{
	osi_writel((unsigned int)H32(tx_desc), (unsigned char *)addr + EQOS_DMA_CHX_RDLH(chan));
	osi_writel((unsigned int)L32(tx_desc), (unsigned char *)addr + EQOS_DMA_CHX_RDLA(chan));
}

/**
 *	eqos_update_rx_tailptr - Update Rx ring tail pointer
 *	@addr: MAC base address.
 *	@chan: DMA Rx channel number.
 *	@tailptr: Tail pointer
 *
 *	Algorithm: Updates DMA Rx channel tail pointer for specific channel.
 *
 *	Dependencies: None.
 *	Protection: None.
 *	Return: None.
 */
static void eqos_update_rx_tailptr(void *addr, unsigned int chan,
				   unsigned long tailptr)
{
	osi_writel((unsigned int)L32(tailptr), (unsigned char *)addr + EQOS_DMA_CHX_RDTP(chan));
}

/**
 *	eqos_start_dma - Start DMA.
 *	@addr: MAC base address.
 *	@chan: DMA Tx/Rx channel number.
 *
 *	Algorithm: Start Tx and Rx DMA for specific channel.
 *
 *	Dependencies: None.
 *	Protection: None.
 *	Return: None.
 */
static void eqos_start_dma(void *addr, unsigned int chan)
{
	unsigned int val;

	/* start Tx DMA */
	val = osi_readl((unsigned char *)addr + EQOS_DMA_CHX_TX_CTRL(chan));
	val |= OSI_BIT(0);
	osi_writel(val, (unsigned char *)addr + EQOS_DMA_CHX_TX_CTRL(chan));

	/* start Rx DMA */
	val = osi_readl((unsigned char *)addr + EQOS_DMA_CHX_RX_CTRL(chan));
	val |= OSI_BIT(0);
	osi_writel(val, (unsigned char *)addr + EQOS_DMA_CHX_RX_CTRL(chan));
}

/**
 *	eqos_stop_dma - Stop DMA.
 *	@addr: MAC base address.
 *	@chan: DMA Tx/Rx channel number.
 *
 *	Algorithm: Start Tx and Rx DMA for specific channel.
 *
 *	Dependencies: None.
 *	Protection: None.
 *	Return: None.
 */
static void eqos_stop_dma(void *addr, unsigned int chan)
{
	unsigned int val;

	/* stop Tx DMA */
	val = osi_readl((unsigned char *)addr + EQOS_DMA_CHX_TX_CTRL(chan));
	val &= ~OSI_BIT(0);
	osi_writel(val, (unsigned char *)addr + EQOS_DMA_CHX_TX_CTRL(chan));

	/* stop Rx DMA */
	val = osi_readl((unsigned char *)addr + EQOS_DMA_CHX_RX_CTRL(chan));
	val &= ~OSI_BIT(0);
	osi_writel(val, (unsigned char *)addr + EQOS_DMA_CHX_RX_CTRL(chan));
}

/**
 *	eqos_configure_dma_channel - Configure DMA channel
 *	@chan: DMA channel number that need to be configured.
 *	@osi_dma: OSI DMA private data structure.
 *
 *	Algorithm: This takes care of configuring the  below
 *	parameters for the DMA channel
 *	1) Enabling DMA channel interrupts
 *	2) Enable 8xPBL mode
 *	3) Program Tx, Rx PBL
 *	4) Enable TSO if HW supports
 *	5) Program Rx Watchdog timer
 *
 *	Dependencies: MAC has to be out of reset.
 *
 *	Protection: None
 *
 *	Return: NONE
 */
static void eqos_configure_dma_channel(unsigned int chan,
				       struct osi_dma_priv_data *osi_dma)
{
	unsigned int value;

	/* enable DMA channel interrupts */
	/* Enable TIE and TBUE */
	/* TIE - Transmit Interrupt Enable */
	/* TBUE - Transmit Buffer Unavailable Enable */
	/* RIE - Receive Interrupt Enable */
	/* RBUE - Receive Buffer Unavailable Enable  */
	/* AIE - Abnormal Interrupt Summary Enable */
	/* NIE - Normal Interrupt Summary Enable */
	/* FBE - Fatal Bus Error Enable */
	value = osi_readl((unsigned char *)osi_dma->base + EQOS_DMA_CHX_INTR_ENA(chan));
	value |= EQOS_DMA_CHX_INTR_TIE | EQOS_DMA_CHX_INTR_TBUE |
		 EQOS_DMA_CHX_INTR_RIE | EQOS_DMA_CHX_INTR_RBUE |
		 EQOS_DMA_CHX_INTR_FBEE | EQOS_DMA_CHX_INTR_AIE |
		 EQOS_DMA_CHX_INTR_NIE;

	/* For multi-irqs to work nie needs to be disabled */
	value &= ~(EQOS_DMA_CHX_INTR_NIE);
	osi_writel(value, (unsigned char *)osi_dma->base + EQOS_DMA_CHX_INTR_ENA(chan));

	/* Enable 8xPBL mode */
	value = osi_readl((unsigned char *)osi_dma->base + EQOS_DMA_CHX_CTRL(chan));
	value |= EQOS_DMA_CHX_CTRL_PBLX8;
	osi_writel(value, (unsigned char *)osi_dma->base + EQOS_DMA_CHX_CTRL(chan));

	/* Configure DMA channel Transmit control register */
	value = osi_readl((unsigned char *)osi_dma->base + EQOS_DMA_CHX_TX_CTRL(chan));
	/* Enable OSF mode */
	value |= EQOS_DMA_CHX_TX_CTRL_OSF;
	/* TxPBL = 32*/
	value |= EQOS_DMA_CHX_TX_CTRL_TXPBL_RECOMMENDED;
	/* enable TSO by default if HW supports */
	value |= EQOS_DMA_CHX_TX_CTRL_TSE;

	osi_writel(value, (unsigned char *)osi_dma->base + EQOS_DMA_CHX_TX_CTRL(chan));

	/* Configure DMA channel Receive control register */
	/* Select Rx Buffer size.  Needs to be rounded up to next multiple of
	 * bus width
	 */
	value = osi_readl((unsigned char *)osi_dma->base + EQOS_DMA_CHX_RX_CTRL(chan));

	value |= (osi_dma->rx_buf_len << EQOS_DMA_CHX_RBSZ_SHIFT);
	/* RXPBL = 12 */
	value |= EQOS_DMA_CHX_RX_CTRL_RXPBL_RECOMMENDED;
	osi_writel(value, (unsigned char *)osi_dma->base + EQOS_DMA_CHX_RX_CTRL(chan));
}

/**
 *	eqos_init_dma_channel - DMA channel INIT
 *	@osi_dma: OSI DMA private data structure.
 *
 *	Description: Initialise all DMA channels.
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
static void eqos_init_dma_channel(struct osi_dma_priv_data *osi_dma)
{
	unsigned int chinx;

	/* configure EQOS DMA channels */
	for (chinx = 0; chinx < osi_dma->num_dma_chans; chinx++) {
		eqos_configure_dma_channel(osi_dma->dma_chans[chinx], osi_dma);
	}
}

/**
 *	eqos_set_rx_buf_len - Set Rx buffer length
 *	@osi_dma: OSI DMA private data structure.
 *
 *	Description: Sets the Rx buffer lenght based on the new MTU size set.
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
static void eqos_set_rx_buf_len(struct osi_dma_priv_data *osi_dma)
{
	unsigned int rx_buf_len;

	if (osi_dma->mtu >= OSI_MTU_SIZE_8K) {
		rx_buf_len = OSI_MTU_SIZE_16K;
	} else if (osi_dma->mtu >= OSI_MTU_SIZE_4K) {
		rx_buf_len = OSI_MTU_SIZE_8K;
	} else if (osi_dma->mtu >= OSI_MTU_SIZE_2K) {
		rx_buf_len = OSI_MTU_SIZE_4K;
	} else if (osi_dma->mtu > MAX_ETH_FRAME_LEN_DEFAULT) {
		rx_buf_len = OSI_MTU_SIZE_2K;
	} else {
		rx_buf_len = MAX_ETH_FRAME_LEN_DEFAULT;
	}

	/* Buffer alignment */
	osi_dma->rx_buf_len = ((rx_buf_len + (EQOS_AXI_BUS_WIDTH - 1U)) &
			       ~(EQOS_AXI_BUS_WIDTH - 1U));
}

static struct osi_dma_chan_ops eqos_dma_chan_ops = {
	.set_tx_ring_len = eqos_set_tx_ring_len,
	.set_rx_ring_len = eqos_set_rx_ring_len,
	.set_tx_ring_start_addr = eqos_set_tx_ring_start_addr,
	.set_rx_ring_start_addr = eqos_set_rx_ring_start_addr,
	.update_tx_tailptr = eqos_update_tx_tailptr,
	.update_rx_tailptr = eqos_update_rx_tailptr,
	.clear_tx_intr = eqos_clear_tx_intr,
	.clear_rx_intr = eqos_clear_rx_intr,
	.disable_chan_tx_intr = eqos_disable_chan_tx_intr,
	.enable_chan_tx_intr = eqos_enable_chan_tx_intr,
	.disable_chan_rx_intr = eqos_disable_chan_rx_intr,
	.enable_chan_rx_intr = eqos_enable_chan_rx_intr,
	.start_dma = eqos_start_dma,
	.stop_dma = eqos_stop_dma,
	.init_dma_channel = eqos_init_dma_channel,
	.set_rx_buf_len = eqos_set_rx_buf_len,
};

struct osi_dma_chan_ops *eqos_get_dma_chan_ops(void)
{
	return &eqos_dma_chan_ops;
}
