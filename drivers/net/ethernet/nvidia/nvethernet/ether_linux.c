/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ether_linux.h"

/**
 *	ether_adjust_link - Adjust link call back
 *	@dev: Net device data.
 *
 *	Algorithm: Callback function called by the PHY subsystem
 *	whenever there is a link detected or link changed on the
 *	physical layer.
 *
 *	Dependencies: MAC and PHY need to be initialized.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
static void ether_adjust_link(struct net_device *dev)
{
	struct ether_priv_data *pdata = netdev_priv(dev);
	struct phy_device *phydev = pdata->phydev;
	int new_state = 0, speed_changed = 0;

	if (phydev == NULL) {
		return;
	}
	if (phydev->link) {
		if (phydev->duplex != pdata->oldduplex) {
			new_state = 1;
			osi_set_mode(pdata->osi_core, phydev->duplex);
			pdata->oldduplex = phydev->duplex;
		}

		if (phydev->speed != pdata->speed) {
			new_state = 1;
			speed_changed = 1;
			osi_set_speed(pdata->osi_core, phydev->speed);
			pdata->speed = phydev->speed;
		}

		if (!pdata->oldlink) {
			new_state = 1;
			pdata->oldlink = 1;
		}
	} else if (pdata->oldlink) {
		new_state = 1;
		pdata->oldlink = 0;
		pdata->speed = 0;
		pdata->oldduplex = -1;
	} else {
		/* Nothing here */
	}

	if (new_state) {
		phy_print_status(phydev);
	}

	if (speed_changed) {
		clk_set_rate(pdata->tx_clk,
			     (phydev->speed == SPEED_10) ? 2500 * 1000 :
			     (phydev->speed ==
			     SPEED_100) ? 25000 * 1000 : 125000 * 1000);
		if (phydev->speed != SPEED_10) {
			if (osi_pad_calibrate(pdata->osi_core) < 0)
				dev_err(pdata->dev,
					"failed to do pad caliberation\n");
		}
	}
}

/**
 *	ether_phy_init - Initialize the PHY
 *	@dev: Net device data.
 *
 *	Algorithm:
 *	1) Reset the PHY
 *	2) Connect to the phy described in the device tree.
 *
 *	Dependencies: MAC and PHY need to be initialized.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, negative value - failure.
 */
static int ether_phy_init(struct net_device *dev)
{
	struct ether_priv_data *pdata = netdev_priv(dev);
	struct phy_device *phydev = NULL;

	pdata->oldlink = 0;
	pdata->speed = SPEED_UNKNOWN;
	pdata->oldduplex = SPEED_UNKNOWN;

	/* Reset the PHY */
	if (gpio_is_valid(pdata->phy_reset)) {
		gpio_set_value(pdata->phy_reset, 0);
		usleep_range(10, 11);
		gpio_set_value(pdata->phy_reset, 1);
	}

	if (pdata->phy_node != NULL) {
		phydev = of_phy_connect(dev, pdata->phy_node,
					&ether_adjust_link, 0,
					pdata->interface);
	}

	if (phydev == NULL) {
		dev_err(pdata->dev, "failed to connect PHY\n");
		return -ENODEV;
	}

	if ((pdata->phy_node == NULL) && phydev->phy_id == 0U) {
		phy_disconnect(phydev);
		return -ENODEV;
	}

	pdata->phydev = phydev;

	return 0;
}

/**
 *	ether_tx_chan_isr - Tx done ISR Routine.
 *	@irq: IRQ number.
 *	@data: Tx NAPI private data structure.
 *
 *	Algorithm:
 *	1) Get channel number private data passed to ISR.
 *	2) Invoke OSI layer to clear Tx interrupt source.
 *	3) Disable DMA Tx channel interrupt.
 *	4) Schedule TX NAPI poll handler to cleanup the buffer.
 *
 *	Dependencies: MAC and PHY need to be initialized.
 *
 *	Protection: None.
 *
 *	Return: IRQ_HANDLED - success, IRQ_NONE - failure.
 */
static irqreturn_t ether_tx_chan_isr(int irq, void *data)
{
	struct ether_tx_napi *tx_napi = (struct ether_tx_napi *)data;
	struct ether_priv_data *pdata = tx_napi->pdata;
	struct osi_dma_priv_data *osi_dma = pdata->osi_dma;
	unsigned int chan = tx_napi->chan;

	osi_clear_tx_intr(osi_dma, chan);

	if (likely(napi_schedule_prep(&tx_napi->napi))) {
		osi_disable_chan_tx_intr(osi_dma, chan);
		__napi_schedule(&tx_napi->napi);
	}

	return IRQ_HANDLED;
}

/**
 *	ether_rx_chan_isr - Rx done ISR Routine
 *	@irq: IRQ number
 *	@data: Rx NAPI private data structure.
 *
 *	Algorithm:
 *	1) Get Rx channel number from Rx NAPI private data which will be passed
 *	during request_irq() API.
 *	2) Invoke OSI layer to clear Rx interrupt source.
 *	3) Disable DMA Rx channel interrupt.
 *	4) Schedule Rx NAPI poll handler to get data from HW and pass to the
 *	Linux network stack.
 *
 *	Dependencies: MAC and PHY need to be initialized.
 *
 *	Protection: None.
 *
 *	Return: IRQ_HANDLED - success, IRQ_NONE - failure.
 */
static irqreturn_t ether_rx_chan_isr(int irq, void *data)
{
	struct ether_rx_napi *rx_napi = (struct ether_rx_napi *)data;
	struct ether_priv_data *pdata = rx_napi->pdata;
	struct osi_dma_priv_data *osi_dma = pdata->osi_dma;
	unsigned int chan = rx_napi->chan;

	osi_clear_rx_intr(osi_dma, chan);

	if (likely(napi_schedule_prep(&rx_napi->napi))) {
		osi_disable_chan_rx_intr(osi_dma, chan);
		__napi_schedule(&rx_napi->napi);
	}

	return IRQ_HANDLED;
}

/**
 *	ether_common_isr - Common ISR Routine
 *	@irq: IRQ number.
 *	@data: Private data from ISR.
 *
 *	Algorithm: Invoker OSI layer to handle common interrupt.
 *
 *	Dependencies: MAC and PHY need to be initialized.
 *
 *	Protection: None.
 *
 *	Return: IRQ_HANDLED - success, IRQ_NONE - failure.
 */
static irqreturn_t ether_common_isr(int irq, void *data)
{
	struct ether_priv_data *pdata =	(struct ether_priv_data *)data;

	osi_common_isr(pdata->osi_core);
	return IRQ_HANDLED;
}

/**
 *	ether_free_irqs - Free IRQs
 *	@pdata: OS dependent private data structure.
 *
 *	Algorithm: This routine takes care of freeing
 *	below IRQs
 *	1) Common IRQ
 *	2) TX IRQ
 *	3) RX IRQ
 *
 *	Dependencies: IRQs should have registered.
 *
 *	Protection: None.
 *
 *	Return: NONE.
 */
static void ether_free_irqs(struct ether_priv_data *pdata)
{
	unsigned int i;
	unsigned int chan;

	if (pdata->common_irq_alloc_mask & 1U) {
		devm_free_irq(pdata->dev, pdata->common_irq, pdata);
		pdata->common_irq_alloc_mask = 0U;
	}

	for (i = 0; i < pdata->osi_dma->num_dma_chans; i++) {
		chan = pdata->osi_dma->dma_chans[i];

		if (pdata->rx_irq_alloc_mask & (1U << i)) {
			devm_free_irq(pdata->dev, pdata->rx_irqs[i],
				      pdata->rx_napi[chan]);
			pdata->rx_irq_alloc_mask &= (~(1U << i));
		}
		if (pdata->tx_irq_alloc_mask & (1U << i)) {
			devm_free_irq(pdata->dev, pdata->tx_irqs[i],
				      pdata->tx_napi[chan]);
			pdata->tx_irq_alloc_mask &= (~(1U << i));
		}
	}
}

/**
 *	ether_request_irqs - register IRQs
 *	@pdata: OS dependent private data structure.
 *
 *	Algorithm: This routine takes care of requesting
 *	below IRQs
 *	1) Common IRQ
 *	2) TX IRQ
 *	3) RX IRQ
 *
 *	Dependencies: IRQ numbers need to be known.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, negative value - failure.
 */
static int ether_request_irqs(struct ether_priv_data *pdata)
{
	struct osi_dma_priv_data *osi_dma = pdata->osi_dma;
	static char irq_names[ETHER_IRQ_MAX_IDX][ETHER_IRQ_NAME_SZ] = {0};
	int ret = 0, i, j = 0;
	unsigned int chan;

	ret = devm_request_irq(pdata->dev, (unsigned int)pdata->common_irq,
			       ether_common_isr, IRQF_SHARED,
			       "ether_common_irq", pdata);
	if (unlikely(ret < 0)) {
		dev_err(pdata->dev, "failed to register common interrupt: %d\n",
			pdata->common_irq);
		return ret;
	}
	pdata->common_irq_alloc_mask = 1;

	for (i = 0; i < osi_dma->num_dma_chans; i++) {
		chan = osi_dma->dma_chans[i];

		snprintf(irq_names[j], ETHER_IRQ_NAME_SZ, "%s.rx%d",
			 netdev_name(pdata->ndev), chan);
		ret = devm_request_irq(pdata->dev, pdata->rx_irqs[i],
				       ether_rx_chan_isr, IRQF_TRIGGER_NONE,
				       irq_names[j++], pdata->rx_napi[chan]);
		if (unlikely(ret < 0)) {
			dev_err(pdata->dev,
				"failed to register Rx chan interrupt: %d\n",
				pdata->rx_irqs[i]);
			goto err_chan_irq;
		}

		pdata->rx_irq_alloc_mask |= (1U << i);

		snprintf(irq_names[j], ETHER_IRQ_NAME_SZ, "%s.tx%d",
			 netdev_name(pdata->ndev), chan);
		ret = devm_request_irq(pdata->dev, (unsigned int)pdata->tx_irqs[i],
				       ether_tx_chan_isr, IRQF_TRIGGER_NONE,
				       irq_names[j++], pdata->tx_napi[chan]);
		if (unlikely(ret < 0)) {
			dev_err(pdata->dev,
				"failed to register Tx chan interrupt: %d\n",
				pdata->tx_irqs[i]);
			goto err_chan_irq;
		}

		pdata->tx_irq_alloc_mask |= (1U << i);
	}

	return ret;

err_chan_irq:
	ether_free_irqs(pdata);
	return ret;
}

/**
 *	ether_napi_disable - Disable NAPI.
 *	@pdata: OSD private data structure.
 *
 *	Algorithm: Disable Tx and Rx NAPI for the channels which are enabled.
 *
 *	Dependencies: NAPI resources need to be allocated as part of probe().
 *
 *	Protection: None.
 *
 *	Return: None.
 */
static void ether_napi_disable(struct ether_priv_data *pdata)
{
	struct osi_dma_priv_data *osi_dma = pdata->osi_dma;
	unsigned int chan;
	unsigned int i;

	for (i = 0; i < osi_dma->num_dma_chans; i++) {
		chan = osi_dma->dma_chans[i];

		napi_disable(&pdata->tx_napi[chan]->napi);
		napi_disable(&pdata->rx_napi[chan]->napi);
	}
}

/**
 *	ether_napi_enable - Enable NAPI.
 *	@pdata: OSD private data structure.
 *
 *	Algorithm: Enable Tx and Rx NAPI for the channels which are enabled.
 *
 *	Dependencies: NAPI resources need to be allocated as part of probe().
 *
 *	Protection: None.
 *
 *	Return: None.
 */
static void ether_napi_enable(struct ether_priv_data *pdata)
{
	struct osi_dma_priv_data *osi_dma = pdata->osi_dma;
	unsigned int chan;
	unsigned int i;

	for (i = 0; i < osi_dma->num_dma_chans; i++) {
		chan = osi_dma->dma_chans[i];

		napi_enable(&pdata->tx_napi[chan]->napi);
		napi_enable(&pdata->rx_napi[chan]->napi);
	}
}

static void ether_free_rx_skbs(struct osi_rx_swcx *rx_swcx, struct device *dev,
			       unsigned int rx_buf_len)
{
	struct osi_rx_swcx *prx_swcx = NULL;
	unsigned int i;

	for (i = 0; i < RX_DESC_CNT; i++) {
		prx_swcx = rx_swcx + i;

		if (prx_swcx->buf_virt_addr != NULL) {
			dma_unmap_single(dev, prx_swcx->buf_phy_addr,
					 rx_buf_len, DMA_FROM_DEVICE);
			dev_kfree_skb_any(prx_swcx->buf_virt_addr);
			prx_swcx->buf_virt_addr = NULL;
			prx_swcx->buf_phy_addr = 0;
		}
	}
}

/**
 *	free_rx_dma_resources - Frees allocated Rx DMA resources.
 *	@osi:	OSI private data structure.
 *
 *	Algorithm: Release all DMA Rx resources which are allocated as part of
 *	allocated_rx_dma_ring() API.
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
static void free_rx_dma_resources(struct osi_dma_priv_data *osi_dma,
				  struct device *dev)
{
	unsigned long rx_desc_size = sizeof(struct osi_rx_desc) * RX_DESC_CNT;
	struct osi_rx_ring *rx_ring = NULL;
	unsigned int i;

	for (i = 0; i < OSI_EQOS_MAX_NUM_CHANS; i++) {
		rx_ring = osi_dma->rx_ring[i];

		if (rx_ring != NULL) {
			if (rx_ring->rx_swcx != NULL) {
				ether_free_rx_skbs(rx_ring->rx_swcx, dev,
						   osi_dma->rx_buf_len);
				kfree(rx_ring->rx_swcx);
			}

			if (rx_ring->rx_desc != NULL) {
				dma_free_coherent(dev, rx_desc_size,
						  rx_ring->rx_desc,
						  rx_ring->rx_desc_phy_addr);
			}
			kfree(rx_ring);
			osi_dma->rx_ring[i] = NULL;
			rx_ring = NULL;
		}
	}
}

/**
 *	allocate_rx_dma_resource - Allocate Rx DMA ring
 *	@osi: OSI private data structure.
 *	@chan: Rx DMA channel number.
 *
 *	Algorithm: DMA receive ring will be created for valid channel number.
 *	Receive ring updates with descriptors and software context associated
 *	with each receive descriptor.
 *
 *	Dependencies: Invalid channel need to be updated.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, negative value - failure
 */
static int allocate_rx_dma_resource(struct osi_dma_priv_data *osi_dma,
				    struct device *dev,
				    unsigned int chan)
{
	unsigned long rx_desc_size;
	unsigned long rx_swcx_size;
	int ret = 0;

	rx_desc_size = sizeof(struct osi_rx_desc) * (unsigned long)RX_DESC_CNT;
	rx_swcx_size = sizeof(struct osi_rx_swcx) * (unsigned long)RX_DESC_CNT;

	osi_dma->rx_ring[chan] = kzalloc(sizeof(struct osi_rx_ring),
					 GFP_KERNEL);
	if (osi_dma->rx_ring[chan] == NULL) {
		dev_err(dev, "failed to allocate Rx ring\n");
		return -ENOMEM;
	}

	osi_dma->rx_ring[chan]->rx_desc = dma_zalloc_coherent(dev, rx_desc_size,
			(dma_addr_t *)&osi_dma->rx_ring[chan]->rx_desc_phy_addr,
			GFP_KERNEL);
	if (osi_dma->rx_ring[chan]->rx_desc == NULL) {
		dev_err(dev, "failed to allocate receive descriptor\n");
		ret = -ENOMEM;
		goto err_rx_desc;
	}

	osi_dma->rx_ring[chan]->rx_swcx = kzalloc(rx_swcx_size, GFP_KERNEL);
	if (osi_dma->rx_ring[chan]->rx_swcx == NULL) {
		dev_err(dev, "failed to allocate Rx ring software context\n");
		ret = -ENOMEM;
		goto err_rx_swcx;
	}

	return 0;

err_rx_swcx:
	dma_free_coherent(dev, rx_desc_size,
			  osi_dma->rx_ring[chan]->rx_desc,
			  osi_dma->rx_ring[chan]->rx_desc_phy_addr);
	osi_dma->rx_ring[chan]->rx_desc = NULL;
err_rx_desc:
	kfree(osi_dma->rx_ring[chan]);
	osi_dma->rx_ring[chan] = NULL;

	return ret;
}

static int ether_allocate_rx_buffers(struct ether_priv_data *pdata,
				     struct osi_rx_ring *rx_ring)
{
	unsigned int rx_buf_len = pdata->osi_dma->rx_buf_len;
	struct osi_rx_swcx *rx_swcx = NULL;
	unsigned int i = 0;

	for (i = 0; i < RX_DESC_CNT; i++) {
		struct sk_buff *skb = NULL;
		dma_addr_t dma_addr = 0;

		rx_swcx = rx_ring->rx_swcx + i;

		skb = __netdev_alloc_skb_ip_align(pdata->ndev, rx_buf_len,
						  GFP_KERNEL);
		if (unlikely(skb == NULL)) {
			dev_err(pdata->dev, "RX skb allocation failed\n");
			return -ENOMEM;
		}

		dma_addr = dma_map_single(pdata->dev, skb->data, rx_buf_len,
					  DMA_FROM_DEVICE);
		if (unlikely(dma_mapping_error(pdata->dev, dma_addr) != 0)) {
			dev_err(pdata->dev, "RX skb dma map failed\n");
			dev_kfree_skb_any(skb);
			return -ENOMEM;
		}

		rx_swcx->buf_virt_addr = skb;
		rx_swcx->buf_phy_addr = dma_addr;
	}

	return 0;
}

/**
 *	allocate_rx_dma_resources - Allocate rx DMA resources.
 *	@osi: OSI private data structure.
 *
 *	Algorithm: DMA receive ring will be created for valid channel number
 *	provided through DT
 *
 *	Dependencies: Invalid channel need to be updated.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, negative value - failure
 */
static int ether_allocate_rx_dma_resources(struct osi_dma_priv_data *osi_dma,
					   struct ether_priv_data *pdata)
{
	unsigned int chan;
	unsigned int i;
	int ret = 0;

	for (i = 0; i < OSI_EQOS_MAX_NUM_CHANS; i++) {
		chan = osi_dma->dma_chans[i];

		if (chan != OSI_INVALID_CHAN_NUM) {
			ret = allocate_rx_dma_resource(osi_dma, pdata->dev,
						       chan);
			if (ret != 0) {
				goto exit;
			}

			ret = ether_allocate_rx_buffers(pdata,
							osi_dma->rx_ring[chan]);
			if (ret < 0) {
				goto exit;
			}
		}
	}

	return 0;
exit:
	free_rx_dma_resources(osi_dma, pdata->dev);
	return ret;
}

/**
 *	free_tx_dma_resources - Frees allocated DMA resources.
 *	@osi:	OSI private data structure.
 *
 *	Algorithm: Release all DMA Tx resources which are allocated as part of
 *	allocated_tx_dma_ring() API.
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
static void free_tx_dma_resources(struct osi_dma_priv_data *osi_dma,
				  struct device *dev)
{
	unsigned long tx_desc_size = sizeof(struct osi_tx_desc) * TX_DESC_CNT;
	struct osi_tx_ring *tx_ring = NULL;
	unsigned int i;

	for (i = 0; i < OSI_EQOS_MAX_NUM_CHANS; i++) {
		tx_ring = osi_dma->tx_ring[i];

		if (tx_ring != NULL) {
			if (tx_ring->tx_swcx != NULL) {
				kfree(tx_ring->tx_swcx);
			}

			if (tx_ring->tx_desc != NULL) {
				dma_free_coherent(dev, tx_desc_size,
						  tx_ring->tx_desc,
						  tx_ring->tx_desc_phy_addr);
			}

			kfree(tx_ring);
			tx_ring = NULL;
			osi_dma->tx_ring[i] = NULL;
		}
	}
}

/**
 *	allocate_tx_dma_resource - Allocate Tx DMA ring
 *	@osi: OSI private data structure.
 *	@chan: Channel number.
 *
 *	Algorithm: DMA transmit ring will be created for valid channel number.
 *	Transmit ring updates with descriptors and software context associated
 *	with each transmit descriptor.
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, negative value - failure
 */
static int allocate_tx_dma_resource(struct osi_dma_priv_data *osi_dma,
				    struct device *dev,
				    unsigned int chan)
{
	unsigned long tx_desc_size;
	unsigned long tx_swcx_size;
	int ret = 0;

	tx_desc_size = sizeof(struct osi_tx_desc) * (unsigned long)TX_DESC_CNT;
	tx_swcx_size = sizeof(struct osi_tx_swcx) * (unsigned long)TX_DESC_CNT;

	osi_dma->tx_ring[chan] = kzalloc(sizeof(struct osi_tx_ring),
					 GFP_KERNEL);
	if (osi_dma->tx_ring[chan] == NULL) {
		dev_err(dev, "failed to allocate Tx ring\n");
		return -ENOMEM;
	}

	osi_dma->tx_ring[chan]->tx_desc = dma_zalloc_coherent(dev, tx_desc_size,
			(dma_addr_t *)&osi_dma->tx_ring[chan]->tx_desc_phy_addr,
			GFP_KERNEL);
	if (osi_dma->tx_ring[chan]->tx_desc == NULL) {
		dev_err(dev, "failed to allocate transmit descriptor\n");
		ret = -ENOMEM;
		goto err_tx_desc;
	}

	osi_dma->tx_ring[chan]->tx_swcx = kzalloc(tx_swcx_size, GFP_KERNEL);
	if (osi_dma->tx_ring[chan]->tx_swcx == NULL) {
		dev_err(dev, "failed to allocate Tx ring software context\n");
		ret = -ENOMEM;
		goto err_tx_swcx;
	}

	return 0;

err_tx_swcx:
	dma_free_coherent(dev, tx_desc_size,
			  osi_dma->tx_ring[chan]->tx_desc,
			  osi_dma->tx_ring[chan]->tx_desc_phy_addr);
	osi_dma->tx_ring[chan]->tx_desc = NULL;
err_tx_desc:
	kfree(osi_dma->tx_ring[chan]);
	osi_dma->tx_ring[chan] = NULL;
	return ret;
}

/**
 *	ether_allocate_tx_dma_resources - Allocate Tx DMA resources.
 *	@osi: OSI private data structure.
 *
 *	Algorithm: DMA transmit ring will be created for valid channel number
 *	provided through DT
 *
 *	Dependencies: Invalid channel need to be updated.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, negative value - failure
 */
static int ether_allocate_tx_dma_resources(struct osi_dma_priv_data *osi_dma,
					   struct device *dev)
{
	unsigned int chan;
	unsigned int i;
	int ret = 0;

	for (i = 0; i < OSI_EQOS_MAX_NUM_CHANS; i++) {
		chan = osi_dma->dma_chans[i];

		if (chan != OSI_INVALID_CHAN_NUM) {
			ret = allocate_tx_dma_resource(osi_dma, dev, chan);
			if (ret != 0) {
				goto exit;
			}
		}
	}

	return 0;
exit:
	free_tx_dma_resources(osi_dma, dev);
	return ret;
}

/**
 *	ether_init_invalid_chan_ring -
 *				Updates invalid channels list and DMA rings.
 *	@osi:	OSI private data structure.
 *
 *	Algorithm:
 *	1) Initialize all DMA Tx/Rx pointers to NULL so that for valid
 *	dma_addr_t *channels Tx/Rx rings will be created.
 *
 *	For ex: If the number of channels are 2 (nvidia,num_dma_chans = <2>)
 *	and channel numbers are 2 and 3 (nvidia,dma_chans = <2 3>),
 *	then only for channel 2 and 3 DMA rings will be allocated in
 *	allocate_tx/rx_dma_resources() function.
 *
 *	Representation of Tx ring will be like -
 *	osi->tx_ring[0] = NULL,
 *	osi->tx_ring[1] = NULL,
 *	osi->tx_ring[2] = allocated pointer for DMA Tx 2 ring.
 *	osi->tx_ring[3] = allocated pointer for DMA Tx 3 ring.
 *	osi_>tx_ring[4] = NULL,
 *	.
 *	.
 *	osi_>tx_ring[9] = NULL.
 *
 *	This is useful in start_xmit() to get directly Tx ring based on the
 *	channel number return from select_queue() API.
 *
 *	2) Second for loop makes remaing channel numbers as invalid numbers
 *	so that only valid channel will go and allocate/free the DMA resources.
 *
 *	Dependencies: OSD needs to be update number of channels and channel
 *	numbers in OSI private data structure.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
static void ether_init_invalid_chan_ring(struct osi_dma_priv_data *osi_dma)
{
	unsigned int i;

	for (i = 0; i < OSI_EQOS_MAX_NUM_CHANS; i++) {
		osi_dma->tx_ring[i] = NULL;
		osi_dma->rx_ring[i] = NULL;
	}

	for (i = osi_dma->num_dma_chans; i < OSI_EQOS_MAX_NUM_CHANS; i++) {
		osi_dma->dma_chans[i] = OSI_INVALID_CHAN_NUM;
	}
}

/**
 *	free_dma_resources - Freeing allocated DMA resources.
 *	@osi: OSI private data structure.
 *
 *	Algorithm: Frees all DMA resources which are allocates with
 *	allocate_dma_resources() API.
 *
 *	Dependencies: None.
 *	Protection: None.
 *	Return: None.
 *
 */
void free_dma_resources(struct osi_dma_priv_data *osi_dma, struct device *dev)
{
	free_tx_dma_resources(osi_dma, dev);
	free_rx_dma_resources(osi_dma, dev);
}

/**
 *	allocate_dma_resources - Allocate DMA resources for Tx and Rx.
 *	@osi: OSI private data structure.
 *
 *	Algorithm:
 *	1) updates invalid channels with numbers.
 *	2) Allocate Tx DMA resources.
 *	3) Allocate Rx DMA resources.
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return:	0 - success, -1 - failure.
 */
static int ether_allocate_dma_resources(struct ether_priv_data *pdata)
{
	struct osi_dma_priv_data *osi_dma = pdata->osi_dma;
	int ret = 0;

	ether_init_invalid_chan_ring(osi_dma);

	ret = ether_allocate_tx_dma_resources(osi_dma, pdata->dev);
	if (ret != 0) {
		return ret;
	}

	ret = ether_allocate_rx_dma_resources(osi_dma, pdata);
	if (ret != 0) {
		free_tx_dma_resources(osi_dma, pdata->dev);
		return ret;
	}

	return ret;
}

/**
 *	ether_open - Call back to handle bring up of Ethernet interface
 *	@dev: Net device data structure.
 *
 *	Algorithm: This routine takes care of below
 *	1) PHY initialization
 *	2) request tx/rx/common irqs
 *	3) HW initialization
 *	4) Starting the PHY
 *
 *	Dependencies: Ethernet driver probe need to be completed successfully
 *	with ethernet network device created.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, negative value - failure.
 */
static int ether_open(struct net_device *dev)
{
	struct ether_priv_data *pdata = netdev_priv(dev);
	int ret = 0;

	/* PHY reset and initialization */
	ret = ether_phy_init(dev);
	if (ret < 0) {
		dev_err(&dev->dev, "%s: Cannot attach to PHY (error: %d)\n",
			__func__, ret);
		return ret;
	}

	/* request tx/rx/common irq */
	ret = ether_request_irqs(pdata);
	if (ret < 0) {
		dev_err(&dev->dev,
			"%s: failed to get tx rx irqs with reason %d\n",
			__func__, ret);
		goto err_r_irq;
	}

	osi_set_rx_buf_len(pdata->osi_dma);

	ret = ether_allocate_dma_resources(pdata);
	if (ret < 0) {
		dev_err(pdata->dev, "failed to allocate DMA resources\n");
		goto err_alloc;
	}

	/* initialize MAC/MTL/DMA Common registers */
	ret = osi_hw_core_init(pdata->osi_core,
			       pdata->hw_feat.tx_fifo_size,
			       pdata->hw_feat.rx_fifo_size);
	if (ret < 0) {
		dev_err(pdata->dev,
			"%s: failed to initialize MAC HW core with reason %d\n",
			__func__, ret);
		goto err_hw_init;
	}

	/* DMA init */
	ret = osi_hw_dma_init(pdata->osi_dma);
	if (ret < 0) {
		dev_err(pdata->dev,
			"%s: failed to initialize MAC HW DMA with reason %d\n",
			__func__, ret);
		goto err_hw_init;
	}

	/* Start the MAC */
	osi_start_mac(pdata->osi_core);

	ether_napi_enable(pdata);

	/* start PHY */
	phy_start(pdata->phydev);

	/* start network queues */
	netif_tx_start_all_queues(pdata->ndev);

	return ret;

err_hw_init:
	free_dma_resources(pdata->osi_dma, pdata->dev);
err_alloc:
	ether_free_irqs(pdata);
err_r_irq:
	if (pdata->phydev)
		phy_disconnect(pdata->phydev);

	return ret;
}

/**
 *	ether_close - Call back to handle bring down of Ethernet interface
 *	@dev: Net device data structure.
 *
 *	Algorithm: This routine takes care of below
 *	1) Stopping PHY
 *	2) Freeing tx/rx/common irqs
 *
 *	Dependencies: MAC Interface need to be registered.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, negative value - failure.
 */
static int ether_close(struct net_device *dev)
{
	int ret = 0;
	struct ether_priv_data *pdata = netdev_priv(dev);

	/* Stop and disconnect the PHY */
	if (pdata->phydev != NULL) {
		phy_stop(pdata->phydev);
		phy_disconnect(pdata->phydev);

		if (gpio_is_valid(pdata->phy_reset))
			gpio_set_value(pdata->phy_reset, 0);

		pdata->phydev = NULL;
	}

	/* turn off sources of data into dev */
	netif_tx_disable(pdata->ndev);

	/* Free tx rx and common irqs */
	ether_free_irqs(pdata);

	/* DMA De init */
	osi_hw_dma_deinit(pdata->osi_dma);

	/* free DMA resources after DMA stop */
	free_dma_resources(pdata->osi_dma, pdata->dev);

	/* Stop the MAC */
	osi_stop_mac(pdata->osi_core);

	ether_napi_disable(pdata);

	return ret;
}

/**
 *	ether_tx_swcx_alloc - Tx ring software context allocation.
 *	@dev: device instance associated with driver.
 *	@tx_ring: Tx ring instance associated with channel number.
 *	@skb: socket buffer.
 *
 *	Algorithm:
 *	1) Map skb data buffer to DMA mappable address.
 *	2) Updated dma address, len and buffer address. This info will be used
 *	OSI layer for data transmission and buffer cleanup.
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: number of descriptors - success, -1 - failure.
 */
static int ether_tx_swcx_alloc(struct device *dev,
			       struct osi_tx_ring *tx_ring,
			       struct sk_buff *skb)
{
	struct osi_tx_pkt_cx *tx_pkt_cx = &tx_ring->tx_pkt_cx;
	unsigned int cur_tx_idx = tx_ring->cur_tx_idx;
	struct osi_tx_swcx *tx_swcx = NULL;
	unsigned int len = 0;
	int cnt = 0;

	memset(tx_pkt_cx, 0, sizeof(*tx_pkt_cx));

	if (unlikely(skb_vlan_tag_present(skb))) {
		tx_pkt_cx->vtag_id = skb_vlan_tag_get(skb);
		tx_pkt_cx->vtag_id |= (skb->priority << VLAN_PRIO_SHIFT);
		tx_pkt_cx->flags = OSI_PKT_CX_VLAN;

		tx_swcx = tx_ring->tx_swcx + cur_tx_idx;
		if (tx_swcx->len) {
			return 0;
		}

		tx_swcx->len = -1;
		cnt++;
		INCR_TX_DESC_INDEX(cur_tx_idx, 1U);
	}

	len = skb_headlen(skb);

	tx_swcx = tx_ring->tx_swcx + cur_tx_idx;
	if (tx_swcx->len) {
		return 0;
	}

	tx_swcx->buf_phy_addr = dma_map_single(dev, skb->data,
					       len, DMA_TO_DEVICE);
	if (unlikely(dma_mapping_error(dev, tx_swcx->buf_phy_addr))) {
		dev_err(dev, "failed to map Tx buffer\n");
		return -ENOMEM;
	}

	tx_swcx->len = len;
	tx_swcx->buf_virt_addr = skb;

	cnt++;

	tx_pkt_cx->desc_cnt = cnt;

	return cnt;
}

/**
 *	ether_select_queue - Select queue based on user priority
 *	@dev: Network device pointer
 *	@skb: sk_buff pointer, buffer data to send
 *	@accel_priv: private data used for L2 forwarding offload
 *	@fallback: fallback function pointer
 *
 *	Algorithm:
 *	1) Select the correct queue index based which has priority of queue
 *	same as skb-<priority
 *	2) default select queue index 0
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: tx queu index - success, -1 - failure.
 */
static unsigned short ether_select_queue(struct net_device *dev,
					 struct sk_buff *skb,
					 void *accel_priv,
					 select_queue_fallback_t fallback)
{
	struct ether_priv_data *pdata = netdev_priv(dev);
	struct osi_dma_priv_data *osi_dma = pdata->osi_dma;
	unsigned short txqueue_select = 0;
	unsigned int i, chan;

	for (i = 0; i < OSI_EQOS_MAX_NUM_CHANS; i++) {
		chan = osi_dma->dma_chans[i];
		if (pdata->q_prio[chan] == skb->priority) {
			txqueue_select = (unsigned short)chan;
			break;
		}
	}

	return txqueue_select;
}

/**
 *	ether_start_xmit - Network layer hook for data transmission.
 *	@skb: SKB data structure.
 *	@ndev: Net device structure.
 *
 *	Algorithm:
 *	1) Allocate software context (DMA address for the buffer) for the data.
 *	2) Invoke OSI for data transmission.
 *
 *	Dependencies: MAC and PHY need to be initialized.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, negative value - failure.
 */
static int ether_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	struct ether_priv_data *pdata = netdev_priv(ndev);
	struct osi_dma_priv_data *osi_dma = pdata->osi_dma;
	unsigned int chan = skb_get_queue_mapping(skb);
	struct osi_tx_ring *tx_ring = osi_dma->tx_ring[chan];
	int count = 0;

	count = ether_tx_swcx_alloc(pdata->dev, tx_ring, skb);
	if (count <= 0) {
		if (count == 0) {
			netif_stop_queue(ndev);
			netdev_err(ndev, "Tx ring is full\n");
			return NETDEV_TX_BUSY;
		}
		dev_kfree_skb_any(skb);
		return NETDEV_TX_OK;
	}

	osi_hw_transmit(osi_dma, chan);

	return NETDEV_TX_OK;
}

/**
 *	ether_ioctl - network stack IOCTL hook to driver
 *	@ndev: network device structure
 *	@rq: Interface request structure used for socket
 *	@cmd: IOCTL command code
 *
 *	Algorithm:
 *	1) Invokes MII API for phy read/write based on IOCTL command
 *	2) SIOCDEVPRIVATE for private ioctl
 *
 *	Dependencies: Ethernet interface need to be up.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, negative value - failure.
 */
static int ether_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	int ret = -EOPNOTSUPP;

	if (!netif_running(dev))
		return -EINVAL;

	switch (cmd) {
	case SIOCGMIIPHY:
	case SIOCGMIIREG:
	case SIOCSMIIREG:
		if (!dev->phydev)
			return -EINVAL;

		/* generic PHY MII ioctl interface */
		ret = phy_mii_ioctl(dev->phydev, rq, cmd);
		break;

	case SIOCDEVPRIVATE:
		ret = ether_handle_priv_ioctl(dev, rq);
		break;

	default:
		break;
	}

	return ret;
}

/**
 *	ether_set_mac_addr - Set MAC address
 *	@ndev: Network device structure
 *	@addr: MAC address to be programmed.
 *
 *	Algorithm:
 *	1) Checks whether given MAC address is valid or not
 *	2) Stores the MAC address in OSI core structure
 *
 *	Dependencies: Ethernet interface need to be down to set MAC address
 *
 *	Protection: None.
 *
 *	Return: 0 - success, negative value - failure.
 */
static int ether_set_mac_addr(struct net_device *ndev, void *addr)
{
	struct ether_priv_data *pdata = netdev_priv(ndev);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	int ret = 0;

	ret = eth_mac_addr(ndev, addr);
	if (ret) {
		dev_err(pdata->dev, "failed to set MAC address\n");
		return ret;
	}

	/* MAC address programmed in HW registers during osi_hw_core_init() */
	memcpy(osi_core->mac_addr, ndev->dev_addr, ETH_ALEN);

	return ret;
}

/**
 *     ether_change_mtu - Change MAC MTU size
 *     @ndev: Network device structure
 *     @new_mtu: New MTU size to set.
 *
 *     Algorithm:
 *     1) Check and return if interface is up.
 *     2) Stores new MTU size set by user in OSI core data structure.
 *
 *     Dependencies: Ethernet interface need to be down to change MTU size
 *
 *     Protection: None.
 *
 *     Return: 0 - success, negative value - failure.
 */
static int ether_change_mtu(struct net_device *ndev, int new_mtu)
{
	struct ether_priv_data *pdata = netdev_priv(ndev);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	struct osi_dma_priv_data *osi_dma = pdata->osi_dma;

	if (netif_running(ndev)) {
		netdev_err(pdata->ndev, "must be stopped to change its MTU\n");
		return -EBUSY;
	}

	ndev->mtu = new_mtu;
	osi_core->mtu = new_mtu;
	osi_dma->mtu = new_mtu;

	netdev_update_features(ndev);

	return 0;
}

static const struct net_device_ops ether_netdev_ops = {
	.ndo_open = ether_open,
	.ndo_stop = ether_close,
	.ndo_start_xmit = ether_start_xmit,
	.ndo_do_ioctl = ether_ioctl,
	.ndo_set_mac_address = ether_set_mac_addr,
	.ndo_change_mtu = ether_change_mtu,
	.ndo_select_queue = ether_select_queue,
};

/**
 *	ether_napi_poll_rx - NAPI poll handler for receive.
 *	@napi: NAPI instance for Rx NAPI.
 *	@budget: NAPI budget.
 *
 *	Algorithm: Invokes OSI layer to read data from HW and pass onto the
 *	Linux network stack.
 *
 *	Dependencies: Probe and INIT needs to be completed successfully.
 *
 *	Protection: None.
 *
 *	Return: number of packets received.
 */
static int ether_napi_poll_rx(struct napi_struct *napi, int budget)
{
	struct ether_rx_napi *rx_napi =
		container_of(napi, struct ether_rx_napi, napi);
	struct ether_priv_data *pdata = rx_napi->pdata;
	struct osi_dma_priv_data *osi_dma = pdata->osi_dma;
	unsigned int chan = rx_napi->chan;
	int received = 0;

	received = osi_process_rx_completions(osi_dma, chan, budget);
	if (received < budget) {
		napi_complete(napi);
		osi_enable_chan_rx_intr(osi_dma, chan);
	}

	return received;
}

/**
 *	ether_napi_poll_tx - NAPI poll handler for transmission.
 *	@napi: NAPI instance for tx NAPI.
 *	@budget: NAPI budget.
 *
 *	Algorithm: Invokes OSI layer to read data from HW and pass onto the
 *	Linux network stack.
 *
 *	Dependencies: Probe and INIT needs to be completed successfully.
 *
 *	Protection: None.
 *
 *	Return: Number of Tx buffer cleaned.
 */
static int ether_napi_poll_tx(struct napi_struct *napi, int budget)
{
	struct ether_tx_napi *tx_napi =
		container_of(napi, struct ether_tx_napi, napi);
	struct ether_priv_data *pdata = tx_napi->pdata;
	struct osi_dma_priv_data *osi_dma = pdata->osi_dma;
	unsigned int chan = tx_napi->chan;
	int processed;

	processed = osi_process_tx_completions(osi_dma, chan);
	if (processed == 0) {
		napi_complete(napi);
		osi_enable_chan_tx_intr(osi_dma, chan);
		return 0;
	}

	return budget;
}

/**
 *	ether_alloc_napi - Allocate NAPI resources.
 *	@pdata: OSD private data structure.
 *
 *	Algorithm: Allocate NAPI instances for the channels which are enabled.
 *
 *	Dependencies: Number of channels and channel numbers needs to be
 *	updated in OSI private data structure.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
static int ether_alloc_napi(struct ether_priv_data *pdata)
{
	struct osi_dma_priv_data *osi_dma = pdata->osi_dma;
	struct net_device *ndev = pdata->ndev;
	struct device *dev = pdata->dev;
	unsigned int chan;
	unsigned int i;

	for (i = 0; i < osi_dma->num_dma_chans; i++) {
		chan = osi_dma->dma_chans[i];

		pdata->tx_napi[chan] = devm_kzalloc(dev,
						sizeof(struct ether_tx_napi),
						GFP_KERNEL);
		if (pdata->tx_napi[chan] == NULL) {
			dev_err(dev, "failed to allocate Tx NAPI resource\n");
			return -ENOMEM;
		}

		pdata->tx_napi[chan]->pdata = pdata;
		pdata->tx_napi[chan]->chan = chan;
		netif_tx_napi_add(ndev, &pdata->tx_napi[chan]->napi,
				  ether_napi_poll_tx, 64);

		pdata->rx_napi[chan] = devm_kzalloc(dev,
						sizeof(struct ether_rx_napi),
						GFP_KERNEL);
		if (pdata->rx_napi[chan] == NULL) {
			dev_err(dev, "failed to allocate RX NAPI resource\n");
			return -ENOMEM;
		}

		pdata->rx_napi[chan]->pdata = pdata;
		pdata->rx_napi[chan]->chan = chan;
		netif_napi_add(ndev, &pdata->rx_napi[chan]->napi,
			       ether_napi_poll_rx, 64);
	}

	return 0;
}

/**
 *	ether_mdio_write - MII call back for MDIO register write.
 *	@bus: MDIO bus instances.
 *	@phyaddr: PHY address (ID).
 *	@phyreg: PHY register to write.
 *	@phydata: Data to be written in register.
 *
 *	Algorimthm: Invoke OSI layer for PHY register write.
 *	phy_write() API from Linux PHY subsystem will call this.
 *
 *	Dependencies: MAC has to be out of reset.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, -1 - failure.
 */
static int ether_mdio_write(struct mii_bus *bus, int phyaddr, int phyreg,
			    u16 phydata)
{
	struct net_device *ndev = bus->priv;
	struct ether_priv_data *pdata = netdev_priv(ndev);

	return osi_write_phy_reg(pdata->osi_core, (unsigned int)phyaddr,
				 (unsigned int)phyreg, phydata);
}

/**
 *	ether_mdio_read - MII call back for MDIO register read.
 *	@bus: MDIO bus instances.
 *	@phyaddr: PHY address (ID).
 *	@phyreg: PHY register to read.
 *
 *	Algorimthm: Invoke OSI layer for PHY register read.
 *	phy_read() API from Linux subsystem will call this.
 *
 *	Dependencies: MAC has to be out of reset.
 *
 *	Protection: None.
 *
 *	Return: data from PHY register - success, -1 - failure.
 */
static int ether_mdio_read(struct mii_bus *bus, int phyaddr, int phyreg)
{
	struct net_device *ndev = bus->priv;
	struct ether_priv_data *pdata = netdev_priv(ndev);

	return osi_read_phy_reg(pdata->osi_core, (unsigned int)phyaddr,
				(unsigned int)phyreg);
}

/**
 *	ether_mdio_register - MDIO bus register
 *	@pdata: OSD private data.
 *
 *	Algorithm: Registers MDIO bus if there is mdio sub DT node
 *	as part of MAC DT node.
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, negative value - failure.
 */
static int ether_mdio_register(struct ether_priv_data *pdata)
{
	struct device *dev = pdata->dev;
	struct mii_bus *new_bus = NULL;
	int ret = 0;

	if (pdata->mdio_node == NULL) {
		pdata->mii = NULL;
		return 0;
	}

	new_bus = devm_mdiobus_alloc(dev);
	if (new_bus == NULL) {
		ret = -ENOMEM;
		dev_err(dev, "failed to allocate MDIO bus\n");
		goto exit;
	}

	new_bus->name = "nvethernet_mdio_bus";
	new_bus->read = ether_mdio_read;
	new_bus->write = ether_mdio_write;
	snprintf(new_bus->id, MII_BUS_ID_SIZE, "%s", dev_name(dev));
	new_bus->priv = pdata->ndev;
	new_bus->parent = dev;

	ret = of_mdiobus_register(new_bus, pdata->mdio_node);
	if (ret) {
		dev_err(dev, "failed to register MDIO bus (%s)\n",
			new_bus->name);
		goto exit;
	}

	pdata->mii = new_bus;

exit:
	return ret;
}

/**
 *	ether_get_irqs - Read IRQ numbers from DT.
 *	@pdev: Platform device associated with driver.
 *	@pdata: OSD private data.
 *	@num_chans: Number of channels.
 *
 *	Algorithm: Reads the IRQ numbers from DT based on number of channels.
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, negative value - failure.
 */
static int ether_get_irqs(struct platform_device *pdev,
			  struct ether_priv_data *pdata,
			  unsigned int num_chans)
{
	unsigned int i, j;

	/* get common IRQ*/
	pdata->common_irq = platform_get_irq(pdev, 0);
	if (pdata->common_irq < 0) {
		dev_err(&pdev->dev, "failed to get common IRQ number\n");
		return pdata->common_irq;
	}

	/* get TX IRQ numbers */
	/* TODO: Need to get VM based IRQ numbers based on MAC version */
	for (i = 0, j = 1; i < num_chans; i++) {
		pdata->tx_irqs[i] = platform_get_irq(pdev, j++);
		if (pdata->tx_irqs[i] < 0) {
			dev_err(&pdev->dev, "failed to get TX IRQ number\n");
			return pdata->tx_irqs[i];
		}
	}

	for (i = 0; i < num_chans; i++) {
		pdata->rx_irqs[i] = platform_get_irq(pdev, j++);
		if (pdata->rx_irqs[i] < 0) {
			dev_err(&pdev->dev, "failed to get RX IRQ number\n");
			return pdata->rx_irqs[i];
		}
	}

	return 0;
}

/**
 *	ether_get_mac_address_dtb - Get MAC address from DT
 *	@node_name: Device tree node name.
 *	@property_name: DT property name inside DT node.
 *	@mac_addr: MAC address.
 *
 *	Algorithm: Populates MAC address by reading DT node.
 *
 *	Dependencies: Bootloader needs to updates chosen DT node with MAC
 *	address.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, -1 - failure.
 */
static int ether_get_mac_address_dtb(const char *node_name,
				     const char *property_name,
				     unsigned char *mac_addr)
{
	struct device_node *np = of_find_node_by_path(node_name);
	const char *mac_str = NULL;
	int values[6] = {0};
	unsigned char mac_temp[6] = {0};
	int i, ret = 0;

	if (np == NULL)
		return -EADDRNOTAVAIL;

	/* If the property is present but contains an invalid value,
	 * then something is wrong. Log the error in that case.
	 */
	if (of_property_read_string(np, property_name, &mac_str)) {
		ret = -EADDRNOTAVAIL;
		goto err_out;
	}

	/* The DTB property is a string of the form xx:xx:xx:xx:xx:xx
	 * Convert to an array of bytes.
	 */
	if (sscanf(mac_str, "%x:%x:%x:%x:%x:%x",
		   &values[0], &values[1], &values[2],
		   &values[3], &values[4], &values[5]) != 6) {
		ret = -EINVAL;
		goto err_out;
	}

	for (i = 0; i < 6; ++i)
		mac_temp[i] = (unsigned char)values[i];

	if (!is_valid_ether_addr(mac_temp)) {
		ret = -EINVAL;
		goto err_out;
	}

	memcpy(mac_addr, mac_temp, 6);

	of_node_put(np);
	return ret;

err_out:
	pr_err("%s: bad mac address at %s/%s: %s.\n",
	       __func__, node_name, property_name,
	       (mac_str != NULL) ? mac_str : "NULL");

	of_node_put(np);

	return ret;
}

/**
 *	ether_get_mac_address - Get MAC address from DT
 *	@pdata: OSD private data.
 *
 *	Algorithm: Populates MAC address by reading DT node.
 *
 *	Dependencies: Bootloader needs to updates chosen DT node with MAC
 *	address.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, -1 - failure.
 */
static int ether_get_mac_address(struct ether_priv_data *pdata)
{
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	struct net_device *ndev = pdata->ndev;
	unsigned char mac_addr[6] = {0};
	int ret = 0, i;

	/* read MAC address */
	ret = ether_get_mac_address_dtb("/chosen",
					"nvidia,ether-mac", mac_addr);
	if (ret < 0)
		return ret;

	/* Set up MAC address */
	for (i = 0; i < 6; i++) {
		ndev->dev_addr[i] = mac_addr[i];
		osi_core->mac_addr[i] = mac_addr[i];
	}

	dev_info(pdata->dev, "Ethernet MAC address: %pM\n", ndev->dev_addr);

	return ret;
}

/**
 *	ether_disable_clks - Disable all MAC related clks.
 *	@pdata: OSD private data.
 *
 *	Algorithm: Release the reference counter for the clks by using
 *	clock subsystem provided API's.
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
static void ether_disable_clks(struct ether_priv_data *pdata)
{
	if (!IS_ERR_OR_NULL(pdata->axi_cbb_clk))
		clk_disable_unprepare(pdata->axi_cbb_clk);

	if (!IS_ERR_OR_NULL(pdata->axi_clk))
		clk_disable_unprepare(pdata->axi_clk);

	if (!IS_ERR_OR_NULL(pdata->rx_clk))
		clk_disable_unprepare(pdata->rx_clk);

	if (!IS_ERR_OR_NULL(pdata->ptp_ref_clk))
		clk_disable_unprepare(pdata->ptp_ref_clk);

	if (!IS_ERR_OR_NULL(pdata->tx_clk))
		clk_disable_unprepare(pdata->tx_clk);

	if (!IS_ERR_OR_NULL(pdata->pllrefe_clk))
		clk_disable_unprepare(pdata->pllrefe_clk);
}

/**
 *	ether_enable_clks - Enable all MAC related clks.
 *	@pdata: OSD private data.
 *
 *	Algorithm: Enables the clks by using clock subsystem provided API's.
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, negative value - failure.
 */
static int ether_enable_clks(struct ether_priv_data *pdata)
{
	int ret;

	if (!IS_ERR_OR_NULL(pdata->pllrefe_clk)) {
		ret = clk_prepare_enable(pdata->pllrefe_clk);
		if (ret < 0)
			return ret;
	}

	if (!IS_ERR_OR_NULL(pdata->axi_cbb_clk)) {
		ret = clk_prepare_enable(pdata->axi_cbb_clk);
		if (ret)
			goto err_axi_cbb;
	}

	if (!IS_ERR_OR_NULL(pdata->axi_clk)) {
		ret = clk_prepare_enable(pdata->axi_clk);
		if (ret < 0)
			goto err_axi;
	}

	if (!IS_ERR_OR_NULL(pdata->rx_clk)) {
		ret = clk_prepare_enable(pdata->rx_clk);
		if (ret < 0)
			goto err_rx;
	}

	if (!IS_ERR_OR_NULL(pdata->ptp_ref_clk)) {
		ret = clk_prepare_enable(pdata->ptp_ref_clk);
		if (ret < 0)
			goto err_ptp_ref;
	}

	if (!IS_ERR_OR_NULL(pdata->tx_clk)) {
		ret = clk_prepare_enable(pdata->tx_clk);
		if (ret < 0)
			goto err_tx;
	}

	return 0;

err_tx:
	if (!IS_ERR_OR_NULL(pdata->ptp_ref_clk))
		clk_disable_unprepare(pdata->ptp_ref_clk);
err_ptp_ref:
	if (!IS_ERR_OR_NULL(pdata->rx_clk))
		clk_disable_unprepare(pdata->rx_clk);
err_rx:
	if (!IS_ERR_OR_NULL(pdata->axi_clk))
		clk_disable_unprepare(pdata->axi_clk);
err_axi:
	if (!IS_ERR_OR_NULL(pdata->axi_cbb_clk))
		clk_disable_unprepare(pdata->axi_cbb_clk);
err_axi_cbb:
	if (!IS_ERR_OR_NULL(pdata->pllrefe_clk))
		clk_disable_unprepare(pdata->pllrefe_clk);

	return ret;
}

/**
 *	ether_get_clks - Get MAC related clocks.
 *	@pdata: OSD private data.
 *
 *	Algorithm: Get the clocks from DT and stores in OSD private data.
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, negative value - failure.
 */
static int ether_get_clks(struct ether_priv_data *pdata)
{
	struct device *dev = pdata->dev;

	pdata->pllrefe_clk = devm_clk_get(dev, "pllrefe_vcoout");
	if (IS_ERR(pdata->pllrefe_clk))
		dev_info(dev, "failed to get pllrefe_vcoout clk\n");

	pdata->axi_cbb_clk = devm_clk_get(dev, "axi_cbb");
	if (IS_ERR(pdata->axi_cbb_clk)) {
		dev_err(dev, "failed to get axi_cbb clk\n");
		return PTR_ERR(pdata->axi_cbb_clk);
	}

	pdata->axi_clk = devm_clk_get(dev, "eqos_axi");
	if (IS_ERR(pdata->axi_clk)) {
		dev_err(dev, "failed to get eqos_axi clk\n");
		return PTR_ERR(pdata->axi_clk);
	}

	pdata->rx_clk = devm_clk_get(dev, "eqos_rx");
	if (IS_ERR(pdata->rx_clk)) {
		dev_err(dev, "failed to get eqos_rx clk\n");
		return PTR_ERR(pdata->rx_clk);
	}

	pdata->ptp_ref_clk = devm_clk_get(dev, "eqos_ptp_ref");
	if (IS_ERR(pdata->ptp_ref_clk)) {
		dev_err(dev, "failed to get eqos_ptp_ref clk\n");
		return PTR_ERR(pdata->ptp_ref_clk);
	}

	pdata->tx_clk = devm_clk_get(dev, "eqos_tx");
	if (IS_ERR(pdata->tx_clk)) {
		dev_err(dev, "failed to get eqos_tx clk\n");
		return PTR_ERR(pdata->tx_clk);
	}

	return 0;
}

/**
 *	ether_configure_car - Get Reset and MAC related clocks.
 *	@pdev: Platform device.
 *	@pdata: OSD private data.
 *
 *	Algorithm: Get the resets and MAC related clocks from DT and stores in
 *	OSD private data. It also sets MDC clock rate by invoking OSI layer
 *	with osi_set_mdc_clk_rate().
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, negative value - failure.
 */
static int ether_configure_car(struct platform_device *pdev,
			       struct ether_priv_data *pdata)
{
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	struct device *dev = pdata->dev;
	struct device_node *np = dev->of_node;
	unsigned long csr_clk_rate = 0;
	int ret = 0;

	/* get MAC reset */
	pdata->mac_rst = devm_reset_control_get(&pdev->dev, "mac_rst");
	if (IS_ERR_OR_NULL(pdata->mac_rst)) {
		dev_err(&pdev->dev, "failed to get MAC reset\n");
		return PTR_ERR(pdata->mac_rst);
	}

	/* get PHY reset */
	pdata->phy_reset = of_get_named_gpio(np, "nvidia,phy-reset-gpio", 0);
	if (pdata->phy_reset < 0)
		dev_info(dev, "failed to get phy reset gpio\n");

	if (gpio_is_valid(pdata->phy_reset)) {
		ret = devm_gpio_request_one(dev, (unsigned int)pdata->phy_reset,
					    GPIOF_OUT_INIT_HIGH,
					    "phy_reset");
		if (ret < 0) {
			dev_err(dev, "failed to request PHY reset gpio\n");
			goto exit;
		}

		gpio_set_value(pdata->phy_reset, 0);
		usleep_range(10, 11);
		gpio_set_value(pdata->phy_reset, 1);
	}

	ret = ether_get_clks(pdata);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to get clks\n");
		return ret;
	}

	ret = ether_enable_clks(pdata);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to enable clks\n");
		return ret;
	}

	if (pdata->mac_rst) {
		ret = reset_control_reset(pdata->mac_rst);
		if (ret < 0) {
			dev_err(&pdev->dev, "failed to reset MAC HW\n");
			goto err_rst;
		}
	}

	ret = osi_poll_for_swr(osi_core);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to poll MAC Software reset\n");
		goto err_swr;
	}

	csr_clk_rate = clk_get_rate(pdata->axi_cbb_clk);
	osi_set_mdc_clk_rate(pdata->osi_core, csr_clk_rate);

	return ret;

err_swr:
	if (pdata->mac_rst)
		reset_control_deassert(pdata->mac_rst);
err_rst:
	ether_disable_clks(pdata);
exit:
	return ret;
}

/**
 *	ether_init_plat_resources - Get platform resources
 *	@pdev: Platform device associated with platform driver.
 *	@pdata: OSD private data.
 *
 *	Algorithm: Populates base address, clks, reset and MAC address.
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, negative value - failure.
 */
static int ether_init_plat_resources(struct platform_device *pdev,
				     struct ether_priv_data *pdata)
{
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	struct resource *res;
	int ret = 0;

	/* get base address and remap */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	osi_core->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(osi_core->base)) {
		dev_err(&pdev->dev, "failed to ioremap MAC base address\n");
		return PTR_ERR(osi_core->base);
	}

	ret = ether_configure_car(pdev, pdata);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to get clks/reset");
		goto rst_clk_fail;
	}

	/*FIXME Need to program different MAC address for other FDs into
	 * different MAC address registers. Need to add DA based filtering
	 * support. Get MAC address from DT
	 */
	ret = ether_get_mac_address(pdata);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to get MAC address");
		goto mac_addr_fail;
	}

	return 0;

mac_addr_fail:
	ether_disable_clks(pdata);
rst_clk_fail:
	return ret;
}

/**
 *	ether_parse_phy_dt - Parse PHY DT.
 *	@pdata: OS dependent private data structure.
 *	@node: DT node entry of MAC
 *
 *	Algorithm: Reads PHY DT. Updates required data.
 *
 *	Dependencies: None
 *
 *	Protection: None
 *
 *	Return: 0 - success, negative value - failure
 */
static int ether_parse_phy_dt(struct ether_priv_data *pdata,
			      struct device_node *node)
{
	pdata->interface = of_get_phy_mode(node);

	pdata->phy_node = of_parse_phandle(node, "phy-handle", 0);
	if (pdata->phy_node == NULL)
		pr_debug("%s(): phy handle not found\n", __func__);

	/* If nvidia,eqos-mdio is passed from DT, always register the MDIO */
	for_each_child_of_node(node, pdata->mdio_node) {
		if (of_device_is_compatible(pdata->mdio_node,
					    "nvidia,eqos-mdio"))
			break;
	}

	/* In the case of a fixed PHY, the DT node associated
	 * to the PHY is the Ethernet MAC DT node.
	 */
	if ((pdata->phy_node == NULL) && of_phy_is_fixed_link(node)) {
		if ((of_phy_register_fixed_link(node) < 0))
			return -ENODEV;
		pdata->phy_node = of_node_get(node);
	}

	return 0;
}

/**
 *	ether_parse_queue_prio - Parse queue priority DT.
 *	@pdata: OS dependent private data structure.
 *	@pdt_prop: name of property
 *	@pval: structure pointer where value will be filed
 *	@val_def: default value if DT entry not reset
 *	@num_entries: number of entries to be read form DT
 *
 *	Algorithm: Reads queue priority form DT. Updates
 *	data either by DT values or by default value.
 *
 *	Dependencies: None
 *
 *	Protection: None
 *
 *	Return: void
 */
static void ether_parse_queue_prio(struct ether_priv_data *pdata,
				   const char *pdt_prop,
				   unsigned int *pval, unsigned int val_def,
				   unsigned int val_max,
				   unsigned int num_entries)
{
	struct device_node *pnode = pdata->dev->of_node;
	unsigned int i, pmask = 0x0U;
	int ret = 0;

	ret = of_property_read_u32_array(pnode, pdt_prop, pval, num_entries);
	if (ret < 0) {
		dev_err(pdata->dev, "%s(): \"%s\" read failed %d. Using default\n",
			__func__, pdt_prop, ret);
		for (i = 0; i < num_entries; i++) {
			pval[i] = val_def;
		}

		return;
	}
	/* If Some priority is alreay give to queue or priority in DT more than
	 * MAX priority, assig default priority to queue with error message
	 */
	for (i = 0; i < num_entries; i++) {
		if ((pval[i] > val_max) || ((pmask & (1U << pval[i])) != 0U)) {
			dev_err(pdata->dev, "%s():Wrong or duplicate priority in DT entry for Q(%d)\n",
				__func__, i);
			pval[i] = val_def;
		}
		pmask |= 1U << pval[i];
	}
}

/**
 *	ether_parse_dt - Parse MAC and PHY DT.
 *	@pdata: OS dependent private data structure.
 *
 *	Algorithm: Reads MAC and PHY DT. Updates required data.
 *
 *	Dependencies: None
 *
 *	Protection: None
 *
 *	Return: 0 - success, negative value - failure
 */
static int ether_parse_dt(struct ether_priv_data *pdata)
{
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	struct osi_dma_priv_data *osi_dma = pdata->osi_dma;
	struct device *dev = pdata->dev;
	struct device_node *np = dev->of_node;
	struct platform_device *pdev = to_platform_device(dev);
	int ret = -EINVAL;

	/* Check if IOMMU is enabled */
	if (pdev->dev.archdata.iommu != NULL) {
		/* Read and set dma-mask from DT only if IOMMU is enabled*/
		ret = of_property_read_u64(np, "dma-mask", &pdata->dma_mask);
	}

	if (ret != 0) {
		dev_info(dev, "setting to default DMA bit mask\n");
		pdata->dma_mask = DMA_MASK_NONE;
	}

	ret = of_property_read_u32_array(np, "nvidia,mtl-queues",
					 osi_core->mtl_queues,
					 osi_core->num_mtl_queues);
	if (ret < 0) {
		dev_err(dev, "failed to read MTL Queue numbers\n");
		if (osi_core->num_mtl_queues == 1) {
			osi_core->mtl_queues[0] = 0;
			dev_info(dev, "setting default MTL queue: 0\n");
		} else {
			goto exit;
		}
	}

	ret = of_property_read_u32_array(np, "nvidia,dma-chans",
					 osi_dma->dma_chans,
					 osi_dma->num_dma_chans);
	if (ret < 0) {
		dev_err(dev, "failed to read DMA channel numbers\n");
		if (osi_dma->num_dma_chans == 1) {
			osi_dma->dma_chans[0] = 0;
			dev_info(dev, "setting default DMA channel: 0\n");
		} else {
			goto exit;
		}
	}

	ret = of_property_read_u32_array(np, "nvidia,rxq_enable_ctrl",
					 osi_core->rxq_ctrl,
					 osi_core->num_mtl_queues);
	if (ret < 0) {
		dev_err(dev, "failed to read rxq enable ctrl\n");
		return ret;
	}

	ether_parse_queue_prio(pdata, "nvidia,queue_prio", pdata->q_prio,
			       ETHER_QUEUE_PRIO_DEFAULT, ETHER_QUEUE_PRIO_MAX,
			       osi_core->num_mtl_queues);

	ret = ether_parse_phy_dt(pdata, np);
	if (ret < 0) {
		dev_err(dev, "failed to parse PHY DT\n");
		goto exit;
	}

exit:
	return ret;
}

/**
 *	ether_get_num_dma_chan_mtl_q - Populate number of MTL and DMA channels.
 *	@pdev: Platform device
 *	@num_chans: Number of channels
 *	@mac:	MAC type based on compatible property
 *	@num_mtl_queues: Number of MTL queues.
 *
 *	Algorithm:
 *	1) Updates MAC HW type based on DT compatible property.
 *	2) Read number of channels from DT.
 *	3) Updates number of channels based on min and max number of channels
 *
 *	Dependencies: None
 *
 *	Protection: None
 *
 *	Return: None.
 */
static void ether_get_num_dma_chan_mtl_q(struct platform_device *pdev,
					 unsigned int *num_dma_chans,
					 unsigned int *mac,
					 unsigned int *num_mtl_queues)
{
	struct device_node *np = pdev->dev.of_node;
	/* intializing with 1 channel */
	unsigned int max_chans = 1;
	int ret = 0;

	ret = of_device_is_compatible(np, "nvidia,nveqos");
	if (ret != 0) {
		*mac = OSI_MAC_HW_EQOS;
		max_chans = OSI_EQOS_MAX_NUM_CHANS;
	}

	/* parse the number of DMA channels */
	ret = of_property_read_u32(np, "nvidia,num-dma-chans", num_dma_chans);
	if (ret != 0) {
		dev_err(&pdev->dev,
			"failed to get number of DMA channels (%d)\n", ret);
		dev_info(&pdev->dev, "Setting number of channels to one\n");
		*num_dma_chans = 1;
	} else if (*num_dma_chans < 1U || *num_dma_chans > max_chans) {
		dev_warn(&pdev->dev, "Invalid num_dma_chans(=%d), setting to 1\n",
			 *num_dma_chans);
		*num_dma_chans = 1;
	} else {
		/* No action required */
	}

	/* parse the number of mtl queues */
	ret = of_property_read_u32(np, "nvidia,num-mtl-queues", num_mtl_queues);
	if (ret != 0) {
		dev_err(&pdev->dev,
			"failed to get number of MTL queueus (%d)\n", ret);
		dev_info(&pdev->dev, "Setting number of queues to one\n");
		*num_mtl_queues = 1;
	} else if (*num_mtl_queues < 1U || *num_mtl_queues > max_chans) {
		dev_warn(&pdev->dev, "Invalid num_mtl_queues(=%d), setting to 1\n",
			 *num_mtl_queues);
		*num_mtl_queues = 1;
	} else {
		/* No action required */
	}
}

/**
 *	ether_set_dma_mask - set dma mask.
 *	@pdata: OS dependent private data structure.
 *
 *	Algorithm:
 *	Based on the value read from HW addressing mode is set accordingly
 *
 *	Dependencies: MAC_HW_Feature1 register need to read and store the
 *	value of ADDR64.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, negative value - failure.
 *
 */
static int ether_set_dma_mask(struct ether_priv_data *pdata)
{
	int ret = 0;

	/* Set DMA addressing limitations based on the value read from HW if
	 * dma_mask is not defined in DT
	 */
	if (pdata->dma_mask == DMA_MASK_NONE) {
		switch (pdata->hw_feat.addr_64) {
		case OSI_ADDRESS_32BIT:
			pdata->dma_mask = DMA_BIT_MASK(32);
			break;
		case OSI_ADDRESS_40BIT:
			pdata->dma_mask = DMA_BIT_MASK(40);
			break;
		case OSI_ADDRESS_48BIT:
			pdata->dma_mask = DMA_BIT_MASK(48);
			break;
		default:
			pdata->dma_mask = DMA_BIT_MASK(40);
			break;
		}
	}

	ret = dma_set_mask_and_coherent(pdata->dev, pdata->dma_mask);
	if (ret < 0) {
		dev_err(pdata->dev, "dma_set_mask_and_coherent failed\n");
		return ret;
	}

	return ret;
}

/**
 *	ether_probe - Ethernet platform driver probe.
 *	@pdev:	platform device associated with platform driver.
 *
 *	Algorithm:
 *	1) Get the number of channels from DT.
 *	2) Allocate the network device for those many channels.
 *	3) Parse MAC and PHY DT.
 *	4) Get all required clks/reset/IRQ's
 *	5) Register MDIO bus and network device.
 *
 *	Dependencies: Device tree need to be updated with proper DT properties.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, negative value - failure.
 *
 */
static int ether_probe(struct platform_device *pdev)
{
	struct ether_priv_data *pdata;
	unsigned int num_dma_chans, mac, num_mtl_queues;
	struct osi_core_priv_data *osi_core;
	struct osi_dma_priv_data *osi_dma;
	struct net_device *ndev;
	int ret = 0;

	ether_get_num_dma_chan_mtl_q(pdev, &num_dma_chans,
				     &mac, &num_mtl_queues);

	osi_core = devm_kzalloc(&pdev->dev, sizeof(*osi_core), GFP_KERNEL);
	if (osi_core == NULL) {
		return -ENOMEM;
	}

	osi_dma = devm_kzalloc(&pdev->dev, sizeof(*osi_dma), GFP_KERNEL);
	if (osi_dma == NULL) {
		return -ENOMEM;
	}

	/* allocate and set up the ethernet device */
	ndev = alloc_etherdev_mq((int)sizeof(struct ether_priv_data),
				 num_dma_chans);
	if (ndev == NULL) {
		dev_err(&pdev->dev, "failed to allocate net device\n");
		return -ENOMEM;
	}

	SET_NETDEV_DEV(ndev, &pdev->dev);

	pdata = netdev_priv(ndev);
	pdata->dev = &pdev->dev;
	pdata->ndev = ndev;
	platform_set_drvdata(pdev, ndev);

	pdata->osi_core = osi_core;
	pdata->osi_dma = osi_dma;
	osi_core->osd = pdata;
	osi_dma->osd = pdata;

	osi_core->num_mtl_queues = num_mtl_queues;
	osi_dma->num_dma_chans = num_dma_chans;

	osi_core->mac = mac;
	osi_dma->mac = mac;

	osi_core->mtu = ndev->mtu;
	osi_dma->mtu = ndev->mtu;
	ndev->max_mtu = OSI_MAX_MTU_SIZE;

	/* Initialize core and DMA ops based on MAC type */
	osi_init_core_ops(osi_core);
	osi_init_dma_ops(osi_dma);

	/* Parse the ethernet DT node */
	ret = ether_parse_dt(pdata);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to parse DT\n");
		goto err_parse_dt;
	}

	/* get base address, clks, reset ID's and MAC address*/
	ret = ether_init_plat_resources(pdev, pdata);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to allocate platform resources\n");
		goto err_init_res;
	}

	/* Assign core base to dma/common base, since we are using single VM */
	osi_dma->base = osi_core->base;

	osi_get_hw_features(osi_core->base, &pdata->hw_feat);

	ret = ether_set_dma_mask(pdata);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to set dma mask\n");
		goto err_dma_mask;
	}

	ret = osi_get_mac_version(osi_core->base, &osi_core->mac_ver);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to get MAC version (%u)\n",
			osi_core->mac_ver);
		goto err_dma_mask;
	}

	ret = ether_get_irqs(pdev, pdata, num_dma_chans);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to get IRQ's\n");
		goto err_dma_mask;
	}

	ret = ether_mdio_register(pdata);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to register MDIO bus\n");
		goto err_dma_mask;
	}

	ndev->netdev_ops = &ether_netdev_ops;
	ether_set_ethtool_ops(ndev);

	ndev->hw_features |= NETIF_F_HW_VLAN_CTAG_RX;
	if (pdata->hw_feat.sa_vlan_ins) {
		ndev->hw_features |= NETIF_F_HW_VLAN_CTAG_TX;
	}

	ndev->features |= ndev->hw_features;

	ret = ether_alloc_napi(pdata);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to allocate NAPI\n");
		goto err_napi;
	}

	/* Register sysfs entry */
	ret = ether_sysfs_register(pdata->dev);
	if (ret < 0) {
		dev_err(&pdev->dev,
			"failed to create nvethernet sysfs group\n");
		goto err_sysfs;
	}

	ret = register_netdev(ndev);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to register netdev\n");
		goto err_netdev;
	}

	dev_info(&pdev->dev,
		 "%s (HW ver: %02x) created with %u DMA channels\n",
		 netdev_name(ndev), osi_core->mac_ver, num_dma_chans);

	return 0;

err_netdev:
	ether_sysfs_unregister(pdata->dev);
err_sysfs:
err_napi:
	mdiobus_unregister(pdata->mii);
err_dma_mask:
	ether_disable_clks(pdata);
err_init_res:
err_parse_dt:
	free_netdev(ndev);
	return ret;
}

/**
 *	ether_remove - Ethernet platform driver remove.
 *	@pdev:	Platform device associated with platform driver.
 *
 *	Alogorithm: Release all the resources
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, negative value - failure.
 */
static int ether_remove(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct ether_priv_data *pdata = netdev_priv(ndev);

	unregister_netdev(ndev);

	/* remove nvethernet sysfs group under /sys/devices/<ether_device>/ */
	ether_sysfs_unregister(pdata->dev);

	if (pdata->mii != NULL) {
		mdiobus_unregister(pdata->mii);
	}

	ether_disable_clks(pdata);
	free_netdev(ndev);

	return 0;
}

static const struct of_device_id ether_of_match[] = {
	{ .compatible = "nvidia,nveqos" },
	{},
};
MODULE_DEVICE_TABLE(of, ether_of_match);

static struct platform_driver ether_driver = {
	.probe = ether_probe,
	.remove = ether_remove,
	.driver = {
		.name = "nvethernet",
		.of_match_table = ether_of_match,
	},
};

module_platform_driver(ether_driver);
MODULE_LICENSE("GPL v2");
