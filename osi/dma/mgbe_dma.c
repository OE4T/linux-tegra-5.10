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

#include "../osi/common/common.h"
#include <osi_common.h>
#include "mgbe_dma.h"
#include "dma_local.h"

/**
 * @brief mgbe_disable_chan_tx_intr - Disables DMA Tx channel interrupts.
 *
 * @param[in] addr: Base address indicating the start of
 *	      memory mapped IO region of the MAC.
 * @param[in] chan: DMA Tx channel number.
 *
 * @note 1) MAC needs to be out of reset and proper clocks need to be configured
 *	 2) DMA HW init need to be completed successfully, see osi_hw_dma_init
 *	 3) Mapping of physical IRQ line to DMA channel need to be maintained at
 *	 OSDependent layer and pass corresponding channel number.
 */
static void mgbe_disable_chan_tx_intr(void *addr, nveu32_t chan)
{
	nveu32_t cntrl;

	MGBE_CHECK_CHAN_BOUND(chan);

	cntrl = osi_readl((nveu8_t *)addr +
			  MGBE_VIRT_INTR_CHX_CNTRL(chan));
	cntrl &= ~MGBE_VIRT_INTR_CHX_CNTRL_TX;
	osi_writel(cntrl, (nveu8_t *)addr +
		   MGBE_VIRT_INTR_CHX_CNTRL(chan));
}

/**
 * @brief mgbe_enable_chan_tx_intr - Enable Tx channel interrupts.
 *
 * @param[in] addr: Base address indicating the start of
 *	      memory mapped IO region of the MAC.
 * @param[in] chan: DMA Tx channel number.
 *
 * @note 1) MAC needs to be out of reset and proper clocks need to be configured
 *	 2) DMA HW init need to be completed successfully, see osi_hw_dma_init
 *	 3) Mapping of physical IRQ line to DMA channel need to be maintained at
 *	 OSDependent layer and pass corresponding channel number.
 */
static void mgbe_enable_chan_tx_intr(void *addr, nveu32_t chan)
{
	nveu32_t cntrl;

	MGBE_CHECK_CHAN_BOUND(chan);

	cntrl = osi_readl((nveu8_t *)addr +
			  MGBE_VIRT_INTR_CHX_CNTRL(chan));
	cntrl |= MGBE_VIRT_INTR_CHX_CNTRL_TX;
	osi_writel(cntrl, (nveu8_t *)addr +
		   MGBE_VIRT_INTR_CHX_CNTRL(chan));
}

/**
 * @brief mgbe_disable_chan_rx_intr - Disable Rx channel interrupts.
 *
 * @param[in] addr: Base address indicating the start of
 *	      memory mapped IO region of the MAC.
 * @param[in] chan: DMA Rx channel number.
 *
 * @note 1) MAC needs to be out of reset and proper clocks need to be configured
 *	 2) DMA HW init need to be completed successfully, see osi_hw_dma_init
 *	 3) Mapping of physical IRQ line to DMA channel need to be maintained at
 *	 OSDependent layer and pass corresponding channel number.
 */
static void mgbe_disable_chan_rx_intr(void *addr, nveu32_t chan)
{
	nveu32_t cntrl;

	MGBE_CHECK_CHAN_BOUND(chan);

	cntrl = osi_readl((nveu8_t *)addr +
			  MGBE_VIRT_INTR_CHX_CNTRL(chan));
	cntrl &= ~MGBE_VIRT_INTR_CHX_CNTRL_RX;
	osi_writel(cntrl, (nveu8_t *)addr +
		   MGBE_VIRT_INTR_CHX_CNTRL(chan));
}

/**
 * @brief mgbe_enable_chan_rx_intr - Enable Rx channel interrupts.
 *
 * @param[in] addr: Base address indicating the start of
 *	      memory mapped IO region of the MAC.
 * @param[in] chan: DMA Rx channel number.
 *
 * @note 1) MAC needs to be out of reset and proper clocks need to be configured
 *	 2) DMA HW init need to be completed successfully, see osi_hw_dma_init
 */
static void mgbe_enable_chan_rx_intr(void *addr, nveu32_t chan)
{
	nveu32_t cntrl;

	MGBE_CHECK_CHAN_BOUND(chan);

	cntrl = osi_readl((nveu8_t *)addr +
			  MGBE_VIRT_INTR_CHX_CNTRL(chan));
	cntrl |= MGBE_VIRT_INTR_CHX_CNTRL_RX;
	osi_writel(cntrl, (nveu8_t *)addr +
		   MGBE_VIRT_INTR_CHX_CNTRL(chan));
}

/**
 * @brief mgbe_set_tx_ring_len - Set DMA Tx ring length.
 *
 * Algorithm: Set DMA Tx channel ring length for specific channel.
 *
 * @param[in] osi_dma: OSI DMA data structure.
 * @param[in] chan: DMA Tx channel number.
 * @param[in] len: Length.
 */
static void mgbe_set_tx_ring_len(struct osi_dma_priv_data *osi_dma,
				 nveu32_t chan,
				 nveu32_t len)
{
	void *addr = osi_dma->base;
	MGBE_CHECK_CHAN_BOUND(chan);

	osi_writel(len, (nveu8_t *)addr + MGBE_DMA_CHX_TDRL(chan));
}

/**
 * @brief mgbe_set_tx_ring_start_addr - Set DMA Tx ring base address.
 *
 * Algorithm: Sets DMA Tx ring base address for specific channel.
 *
 * @param[in] addr: Base address indicating the start of
 *	      memory mapped IO region of the MAC.
 * @param[in] chan: DMA Tx channel number.
 * @param[in] tx_desc: Tx desc base addess.
 */
static void mgbe_set_tx_ring_start_addr(void *addr, nveu32_t chan,
					nveu64_t tx_desc)
{
	nveu64_t temp;

	MGBE_CHECK_CHAN_BOUND(chan);

	temp = H32(tx_desc);
	if (temp < UINT_MAX) {
		osi_writel((nveu32_t)temp, (nveu8_t *)addr +
			   MGBE_DMA_CHX_TDLH(chan));
	}

	temp = L32(tx_desc);
	if (temp < UINT_MAX) {
		osi_writel((nveu32_t)temp, (nveu8_t *)addr +
			   MGBE_DMA_CHX_TDLA(chan));
	}
}

/**
 * @brief mgbe_update_tx_tailptr - Updates DMA Tx ring tail pointer.
 *
 * Algorithm: Updates DMA Tx ring tail pointer for specific channel.
 *
 * @param[in] addr: Base address indicating the start of
 *	      memory mapped IO region of the MAC.
 * @param[in] chan: DMA Tx channel number.
 * @param[in] tailptr: DMA Tx ring tail pointer.
 *
 *
 * @note 1) MAC needs to be out of reset and proper clocks need to be configured
 *	2) DMA HW init need to be completed successfully, see osi_hw_dma_init
 */
static void mgbe_update_tx_tailptr(void *addr, nveu32_t chan,
				   nveu64_t tailptr)
{
	nveu64_t temp;

	MGBE_CHECK_CHAN_BOUND(chan);

	temp = L32(tailptr);
	if (temp < UINT_MAX) {
		osi_writel((nveu32_t)temp, (nveu8_t *)addr +
			   MGBE_DMA_CHX_TDTLP(chan));
	}
}

/**
 * @brief mgbe_set_rx_ring_len - Set Rx channel ring length.
 *
 * Algorithm: Sets DMA Rx channel ring length for specific DMA channel.
 *
 * @param[in] osi_dma: OSI DMA data structure.
 * @param[in] chan: DMA Rx channel number.
 * @param[in] len: Length
 */
static void mgbe_set_rx_ring_len(struct osi_dma_priv_data *osi_dma,
				 nveu32_t chan,
				 nveu32_t len)
{
	void *addr = osi_dma->base;
	MGBE_CHECK_CHAN_BOUND(chan);

	osi_writel(len, (nveu8_t *)addr + MGBE_DMA_CHX_RDRL(chan));
}

/**
 * @brief mgbe_set_rx_ring_start_addr - Set DMA Rx ring base address.
 *
 * Algorithm: Sets DMA Rx channel ring base address.
 *
 * @param[in] addr: Base address indicating the start of
 *	      memory mapped IO region of the MAC.
 * @param[in] chan: DMA Rx channel number.
 * @param[in] tx_desc: DMA Rx desc base address.
 */
static void mgbe_set_rx_ring_start_addr(void *addr, nveu32_t chan,
					nveu64_t tx_desc)
{
	nveu64_t temp;

	MGBE_CHECK_CHAN_BOUND(chan);

	temp = H32(tx_desc);
	if (temp < UINT_MAX) {
		osi_writel((nveu32_t)temp, (nveu8_t *)addr +
			   MGBE_DMA_CHX_RDLH(chan));
	}

	temp = L32(tx_desc);
	if (temp < UINT_MAX) {
		osi_writel((nveu32_t)temp, (nveu8_t *)addr +
			   MGBE_DMA_CHX_RDLA(chan));
	}
}

/**
 * @brief mgbe_update_rx_tailptr - Update Rx ring tail pointer
 *
 * Algorithm: Updates DMA Rx channel tail pointer for specific channel.
 *
 * @param[in] addr: Base address indicating the start of
 *	      memory mapped IO region of the MAC.
 * @param[in] chan: DMA Rx channel number.
 * @param[in] tailptr: Tail pointer
 *
 * @note 1) MAC needs to be out of reset and proper clocks need to be configured
 *	 2) DMA HW init need to be completed successfully, see osi_hw_dma_init
 */
static void mgbe_update_rx_tailptr(void *addr, nveu32_t chan,
				   nveu64_t tailptr)
{
	nveu64_t temp;

	MGBE_CHECK_CHAN_BOUND(chan);

	temp = H32(tailptr);
	if (temp < UINT_MAX) {
		osi_writel((nveu32_t)temp, (nveu8_t *)addr +
			   MGBE_DMA_CHX_RDTHP(chan));
	}

	temp = L32(tailptr);
	if (temp < UINT_MAX) {
		osi_writel((nveu32_t)temp, (nveu8_t *)addr +
			   MGBE_DMA_CHX_RDTLP(chan));
	}
}

/**
 * @brief mgbe_start_dma - Start DMA.
 *
 * Algorithm: Start Tx and Rx DMA for specific channel.
 *
 * @param[in] osi_dma: OSI DMA private data structure.
 * @param[in] chan: DMA Tx/Rx channel number.
 *
 * @note 1) MAC needs to be out of reset and proper clocks need to be configured
 *	 2) DMA HW init need to be completed successfully, see osi_hw_dma_init
 */
static void mgbe_start_dma(struct osi_dma_priv_data *osi_dma, nveu32_t chan)
{
	nveu32_t val;
	void *addr = osi_dma->base;

	MGBE_CHECK_CHAN_BOUND(chan);

	/* start Tx DMA */
	val = osi_readl((nveu8_t *)addr + MGBE_DMA_CHX_TX_CTRL(chan));
	val |= OSI_BIT(0);
	osi_writel(val, (nveu8_t *)addr + MGBE_DMA_CHX_TX_CTRL(chan));

	/* start Rx DMA */
	val = osi_readl((nveu8_t *)addr + MGBE_DMA_CHX_RX_CTRL(chan));
	val |= OSI_BIT(0);
	osi_writel(val, (nveu8_t *)addr + MGBE_DMA_CHX_RX_CTRL(chan));
}

/**
 * @brief mgbe_stop_dma - Stop DMA.
 *
 * Algorithm: Start Tx and Rx DMA for specific channel.
 *
 * @param[in] osi_dma: OSI DMA private data structure.
 * @param[in] chan: DMA Tx/Rx channel number.
 *
 * @note 1) MAC needs to be out of reset and proper clocks need to be configured
 *	 2) DMA HW init need to be completed successfully, see osi_hw_dma_init
 */
static void mgbe_stop_dma(struct osi_dma_priv_data *osi_dma, nveu32_t chan)
{
	nveu32_t val;
	void *addr = osi_dma->base;

	MGBE_CHECK_CHAN_BOUND(chan);

	/* stop Tx DMA */
	val = osi_readl((nveu8_t *)addr + MGBE_DMA_CHX_TX_CTRL(chan));
	val &= ~OSI_BIT(0);
	osi_writel(val, (nveu8_t *)addr + MGBE_DMA_CHX_TX_CTRL(chan));

	/* stop Rx DMA */
	val = osi_readl((nveu8_t *)addr + MGBE_DMA_CHX_RX_CTRL(chan));
	val &= ~OSI_BIT(0);
	osi_writel(val, (nveu8_t *)addr + MGBE_DMA_CHX_RX_CTRL(chan));
}

/**
 * @brief mgbe_configure_dma_channel - Configure DMA channel
 *
 * Algorithm: This takes care of configuring the  below
 *	parameters for the DMA channel
 *	1) Enabling DMA channel interrupts
 *	2) Enable 8xPBL mode
 *	3) Program Tx, Rx PBL
 *	4) Enable TSO if HW supports
 *	5) Program Rx Watchdog timer
 *
 * @param[in] chan: DMA channel number that need to be configured.
 * @param[in] osi_dma: OSI DMA private data structure.
 *
 * @note MAC has to be out of reset.
 */
static void mgbe_configure_dma_channel(nveu32_t chan,
				       struct osi_dma_priv_data *osi_dma)
{
	nveu32_t value;

	MGBE_CHECK_CHAN_BOUND(chan);

	/* enable DMA channel interrupts */
	/* Enable TIE and TBUE */
	/* TIE - Transmit Interrupt Enable */
	/* TBUE - Transmit Buffer Unavailable Enable */
	/* RIE - Receive Interrupt Enable */
	/* RBUE - Receive Buffer Unavailable Enable  */
	/* AIE - Abnormal Interrupt Summary Enable */
	/* NIE - Normal Interrupt Summary Enable */
	/* FBE - Fatal Bus Error Enable */
	value = osi_readl((nveu8_t *)osi_dma->base +
			  MGBE_DMA_CHX_INTR_ENA(chan));
	value |= MGBE_DMA_CHX_INTR_TIE | MGBE_DMA_CHX_INTR_TBUE |
		 MGBE_DMA_CHX_INTR_RIE | MGBE_DMA_CHX_INTR_RBUE |
		 MGBE_DMA_CHX_INTR_FBEE | MGBE_DMA_CHX_INTR_AIE |
		 MGBE_DMA_CHX_INTR_NIE;

	/* For multi-irqs to work nie needs to be disabled */
	/* TODO: do we need this ? */
	value &= ~(MGBE_DMA_CHX_INTR_NIE);
	osi_writel(value, (nveu8_t *)osi_dma->base +
		   MGBE_DMA_CHX_INTR_ENA(chan));

	/* Enable 8xPBL mode */
	value = osi_readl((nveu8_t *)osi_dma->base +
			  MGBE_DMA_CHX_CTRL(chan));
	value |= MGBE_DMA_CHX_CTRL_PBLX8;
	osi_writel(value, (nveu8_t *)osi_dma->base +
		   MGBE_DMA_CHX_CTRL(chan));

	/* Configure DMA channel Transmit control register */
	value = osi_readl((nveu8_t *)osi_dma->base +
			  MGBE_DMA_CHX_TX_CTRL(chan));
	/* Enable OSF mode */
	value |= MGBE_DMA_CHX_TX_CTRL_OSP;
	/* TxPBL = 16 */
	value |= MGBE_DMA_CHX_TX_CTRL_TXPBL_RECOMMENDED;
	/* enable TSO by default if HW supports */
	value |= MGBE_DMA_CHX_TX_CTRL_TSE;

	osi_writel(value, (nveu8_t *)osi_dma->base +
		   MGBE_DMA_CHX_TX_CTRL(chan));

	/* Configure DMA channel Receive control register */
	/* Select Rx Buffer size.  Needs to be rounded up to next multiple of
	 * bus width
	 */
	value = osi_readl((nveu8_t *)osi_dma->base +
			  MGBE_DMA_CHX_RX_CTRL(chan));

	value |= (osi_dma->rx_buf_len << MGBE_DMA_CHX_RBSZ_SHIFT);
	/* RXPBL = 16 */
	value |= MGBE_DMA_CHX_RX_CTRL_RXPBL_RECOMMENDED;
	osi_writel(value, (nveu8_t *)osi_dma->base +
		   MGBE_DMA_CHX_RX_CTRL(chan));

	/* Set Receive Interrupt Watchdog Timer Count */
	/* conversion of usec to RWIT value
	 * Eg:System clock is 62.5MHz, each clock cycle would then be 16ns
	 * For value 0x1 in watchdog timer,device would wait for 256 clk cycles,
	 * ie, (16ns x 256) => 4.096us (rounding off to 4us)
	 * So formula with above values is,ret = usec/4
	 */
	if ((osi_dma->use_riwt == OSI_ENABLE) &&
	    (osi_dma->rx_riwt < UINT_MAX)) {
		value = osi_readl((nveu8_t *)osi_dma->base +
				  MGBE_DMA_CHX_RX_WDT(chan));
		/* Mask the RWT value */
		value &= ~MGBE_DMA_CHX_RX_WDT_RWT_MASK;
		/* Conversion of usec to Rx Interrupt Watchdog Timer Count */
		/* TODO: Need to fix AXI clock for silicon */
		value |= ((osi_dma->rx_riwt *
			 ((nveu32_t)13000000 / OSI_ONE_MEGA_HZ)) /
			 MGBE_DMA_CHX_RX_WDT_RWTU) &
			 MGBE_DMA_CHX_RX_WDT_RWT_MASK;
		osi_writel(value, (nveu8_t *)osi_dma->base +
			   MGBE_DMA_CHX_RX_WDT(chan));
	}
}

/**
 * mgbe_dma_chan_to_vmirq_map - Map DMA channels to a specific VM IRQ.
 *
 * Algorithm: Programs HW to map DMA channels to specific VM.
 *
 * Dependencies: OSD layer needs to update number of VM channels and
 * DMA channel list in osi_vm_irq_data.
 *
 * @param[in] osi_dma: OSI private data structure.
 *
 * Return: None.
 */
static void mgbe_dma_chan_to_vmirq_map(struct osi_dma_priv_data *osi_dma)
{
	struct osi_vm_irq_data *irq_data;
	nveu32_t i, j;
	nveu32_t chan;

	for (i = 0; i < osi_dma->num_vm_irqs; i++) {
		irq_data = &osi_dma->irq_data[i];

		for (j = 0; j < irq_data->num_vm_chans; j++) {
			chan = irq_data->vm_chans[j];

			if (chan >= OSI_MGBE_MAX_NUM_CHANS) {
				continue;
			}

			osi_writel(OSI_BIT(i),
				   (nveu8_t *)osi_dma->base +
				   MGBE_VIRT_INTR_APB_CHX_CNTRL(chan));
		}
	}

	osi_writel(0xD, (nveu8_t *)osi_dma->base + 0x8400);
}

/**
 * @brief mgbe_init_dma_channel - DMA channel INIT
 *
 * @param[in] osi_dma: OSI DMA private data structure.
 */
static nve32_t mgbe_init_dma_channel(struct osi_dma_priv_data *osi_dma)
{
	nveu32_t chinx;

	/* configure MGBE DMA channels */
	for (chinx = 0; chinx < osi_dma->num_dma_chans; chinx++) {
		mgbe_configure_dma_channel(osi_dma->dma_chans[chinx], osi_dma);
	}

	mgbe_dma_chan_to_vmirq_map(osi_dma);

	return 0;
}

/**
 * @brief mgbe_set_rx_buf_len - Set Rx buffer length
 *	  Sets the Rx buffer length based on the new MTU size set.
 *
 * @param[in] osi_dma: OSI DMA private data structure.
 *
 * @note 1) MAC needs to be out of reset and proper clocks need to be configured
 *	 2) DMA HW init need to be completed successfully, see osi_hw_dma_init
 *	 3) osi_dma->mtu need to be filled with current MTU size <= 9K
 */
static void mgbe_set_rx_buf_len(struct osi_dma_priv_data *osi_dma)
{
	nveu32_t rx_buf_len;

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
	osi_dma->rx_buf_len = ((rx_buf_len + (MGBE_AXI_BUS_WIDTH - 1U)) &
			       ~(MGBE_AXI_BUS_WIDTH - 1U));
}

/**
 * mgbe_get_global_dma_status - Gets DMA status.
 *
 * Algorithm: Returns global DMA Tx/Rx interrupt status
 *
 * @param[in] addr: MAC base address.
 *
 * Dependencies: None.
 * Protection: None.
 * Return: None.
 */
static nveu32_t mgbe_get_global_dma_status(void *addr)
{
	return osi_readl((nveu8_t *)addr + MGBE_GLOBAL_DMA_STATUS);
}

/**
 * mgbe_clear_vm_tx_intr - Clear VM Tx interrupt
 *
 * Algorithm: Clear Tx interrupt source at DMA and wrapper level.
 *
 * @param[in] addr: MAC base address.
 * @param[in] chan: DMA Tx channel number.
 *
 * Dependencies: None.
 * Protection: None.
 * Return: None.
 */
static void mgbe_clear_vm_tx_intr(void *addr, nveu32_t chan)
{
	MGBE_CHECK_CHAN_BOUND(chan);

	osi_writel(MGBE_DMA_CHX_STATUS_CLEAR_TX,
		   (nveu8_t *)addr + MGBE_DMA_CHX_STATUS(chan));
	osi_writel(MGBE_VIRT_INTR_CHX_STATUS_TX,
		   (nveu8_t *)addr + MGBE_VIRT_INTR_CHX_STATUS(chan));
}

/**
 * mgbe_clear_vm_rx_intr - Clear VM Rx interrupt
 *
 * Algorithm: Clear Rx interrupt source at DMA and wrapper level.
 *
 * @param[in] addr: MAC base address.
 * @param[in] chan: DMA Rx channel number.
 *
 * Dependencies: None.
 * Protection: None.
 * Return: None.
 */
static void mgbe_clear_vm_rx_intr(void *addr, nveu32_t chan)
{
	MGBE_CHECK_CHAN_BOUND(chan);

	osi_writel(MGBE_DMA_CHX_STATUS_CLEAR_RX,
		   (nveu8_t *)addr + MGBE_DMA_CHX_STATUS(chan));
	osi_writel(MGBE_VIRT_INTR_CHX_STATUS_RX,
		   (nveu8_t *)addr + MGBE_VIRT_INTR_CHX_STATUS(chan));
}

void mgbe_init_dma_chan_ops(struct dma_chan_ops *ops)
{
	ops->set_tx_ring_len = mgbe_set_tx_ring_len;
	ops->set_rx_ring_len = mgbe_set_rx_ring_len;
	ops->set_tx_ring_start_addr = mgbe_set_tx_ring_start_addr;
	ops->set_rx_ring_start_addr = mgbe_set_rx_ring_start_addr;
	ops->update_tx_tailptr = mgbe_update_tx_tailptr;
	ops->update_rx_tailptr = mgbe_update_rx_tailptr;
	ops->disable_chan_tx_intr = mgbe_disable_chan_tx_intr;
	ops->enable_chan_tx_intr = mgbe_enable_chan_tx_intr;
	ops->disable_chan_rx_intr = mgbe_disable_chan_rx_intr;
	ops->enable_chan_rx_intr = mgbe_enable_chan_rx_intr;
	ops->start_dma = mgbe_start_dma;
	ops->stop_dma = mgbe_stop_dma;
	ops->init_dma_channel = mgbe_init_dma_channel;
	ops->set_rx_buf_len = mgbe_set_rx_buf_len;
	ops->validate_regs = OSI_NULL;
	ops->get_global_dma_status = mgbe_get_global_dma_status;
	ops->clear_vm_tx_intr = mgbe_clear_vm_tx_intr;
	ops->clear_vm_rx_intr = mgbe_clear_vm_rx_intr;
};
