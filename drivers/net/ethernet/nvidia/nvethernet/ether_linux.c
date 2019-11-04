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
 * @brief Work Queue function to call osi_read_mmc() periodically.
 *
 * Algorithm: call osi_read_mmc in periodic manner to avoid possibility of
 * overrun of 32 bit MMC hw registers.
 *
 * @param[in] work: work structure
 *
 * @note MAC and PHY need to be initialized.
 */
static inline void ether_stats_work_func(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct ether_priv_data *pdata = container_of(dwork,
			struct ether_priv_data, ether_stats_work);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	int ret;

	ret = osi_read_mmc(osi_core);
	if (ret < 0) {
		dev_err(pdata->dev, "failed to read MMC counters %s\n",
			__func__);
	}
	schedule_delayed_work(&pdata->ether_stats_work,
			      msecs_to_jiffies(ETHER_STATS_TIMER * 1000));
}

/**
 * @brief Start delayed workqueue.
 *
 * Algorithm: Start workqueue to read RMON HW counters periodically.
 * Workqueue will get schedule in every ETHER_STATS_TIMER sec.
 * Workqueue will be scheduled only if HW supports RMON HW counters.
 *
 * @param[in] pdata:OSD private data
 *
 * @note MAC and PHY need to be initialized.
 */
static inline void ether_stats_work_queue_start(struct ether_priv_data *pdata)
{
	if (pdata->hw_feat.mmc_sel == OSI_ENABLE) {
		schedule_delayed_work(&pdata->ether_stats_work,
				      msecs_to_jiffies(ETHER_STATS_TIMER *
						       1000));
	}
}

/**
 * @brief Stop delayed workqueue.
 *
 * Algorithm:
 *  Cancel workqueue.
 *
 * @param[in] pdata:OSD private data
 */
static inline void ether_stats_work_queue_stop(struct ether_priv_data *pdata)
{
	if (pdata->hw_feat.mmc_sel == OSI_ENABLE) {
		cancel_delayed_work_sync(&pdata->ether_stats_work);
	}
}

/**
 * @brief Disable all MAC related clks
 *
 * Algorithm: Release the reference counter for the clks by using
 * clock subsystem provided API's
 *
 * @param[in] pdata: OSD private data.
 */
static void ether_disable_clks(struct ether_priv_data *pdata)
{
	if (!IS_ERR_OR_NULL(pdata->axi_cbb_clk)) {
		clk_disable_unprepare(pdata->axi_cbb_clk);
	}

	if (!IS_ERR_OR_NULL(pdata->axi_clk)) {
		clk_disable_unprepare(pdata->axi_clk);
	}

	if (!IS_ERR_OR_NULL(pdata->rx_clk)) {
		clk_disable_unprepare(pdata->rx_clk);
	}

	if (!IS_ERR_OR_NULL(pdata->ptp_ref_clk)) {
		clk_disable_unprepare(pdata->ptp_ref_clk);
	}

	if (!IS_ERR_OR_NULL(pdata->tx_clk)) {
		clk_disable_unprepare(pdata->tx_clk);
	}

	if (!IS_ERR_OR_NULL(pdata->pllrefe_clk)) {
		clk_disable_unprepare(pdata->pllrefe_clk);
	}
	pdata->clks_enable = false;
}

/**
 * @brief Enable all MAC related clks.
 *
 * Algorithm: Enables the clks by using clock subsystem provided API's.
 *
 * @param[in] pdata: OSD private data.
 *
 * @retval 0 on success
 * @retval "negative value" on failure.
 */
static int ether_enable_clks(struct ether_priv_data *pdata)
{
	int ret;

	if (!IS_ERR_OR_NULL(pdata->pllrefe_clk)) {
		ret = clk_prepare_enable(pdata->pllrefe_clk);
		if (ret < 0) {
			return ret;
		}
	}

	if (!IS_ERR_OR_NULL(pdata->axi_cbb_clk)) {
		ret = clk_prepare_enable(pdata->axi_cbb_clk);
		if (ret) {
			goto err_axi_cbb;
		}
	}

	if (!IS_ERR_OR_NULL(pdata->axi_clk)) {
		ret = clk_prepare_enable(pdata->axi_clk);
		if (ret < 0) {
			goto err_axi;
		}
	}

	if (!IS_ERR_OR_NULL(pdata->rx_clk)) {
		ret = clk_prepare_enable(pdata->rx_clk);
		if (ret < 0) {
			goto err_rx;
		}
	}

	if (!IS_ERR_OR_NULL(pdata->ptp_ref_clk)) {
		ret = clk_prepare_enable(pdata->ptp_ref_clk);
		if (ret < 0) {
			goto err_ptp_ref;
		}
	}

	if (!IS_ERR_OR_NULL(pdata->tx_clk)) {
		ret = clk_prepare_enable(pdata->tx_clk);
		if (ret < 0) {
			goto err_tx;
		}
	}

	pdata->clks_enable = true;

	return 0;

err_tx:
	if (!IS_ERR_OR_NULL(pdata->ptp_ref_clk)) {
		clk_disable_unprepare(pdata->ptp_ref_clk);
	}
err_ptp_ref:
	if (!IS_ERR_OR_NULL(pdata->rx_clk)) {
		clk_disable_unprepare(pdata->rx_clk);
	}
err_rx:
	if (!IS_ERR_OR_NULL(pdata->axi_clk)) {
		clk_disable_unprepare(pdata->axi_clk);
	}
err_axi:
	if (!IS_ERR_OR_NULL(pdata->axi_cbb_clk)) {
		clk_disable_unprepare(pdata->axi_cbb_clk);
	}
err_axi_cbb:
	if (!IS_ERR_OR_NULL(pdata->pllrefe_clk)) {
		clk_disable_unprepare(pdata->pllrefe_clk);
	}

	return ret;
}

/**
 * @brief Adjust link call back
 *
 * Algorithm: Callback function called by the PHY subsystem
 * whenever there is a link detected or link changed on the
 * physical layer.
 *
 * @param[in] dev: Network device data.
 *
 * @note MAC and PHY need to be initialized.
 */
static void ether_adjust_link(struct net_device *dev)
{
	struct ether_priv_data *pdata = netdev_priv(dev);
	struct phy_device *phydev = pdata->phydev;
	int new_state = 0, speed_changed = 0;
	unsigned long val;

	if (phydev == NULL) {
		return;
	}
	if (phydev->link) {
		if ((pdata->osi_core->pause_frames == OSI_PAUSE_FRAMES_ENABLE)
		    && (phydev->pause || phydev->asym_pause)) {
			osi_configure_flow_control(pdata->osi_core,
						   pdata->osi_core->flow_ctrl);
		}

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
			val = pdata->osi_core->xstats.link_connect_count;
			pdata->osi_core->xstats.link_connect_count =
				osi_update_stats_counter(val, 1UL);
		}
	} else if (pdata->oldlink) {
		new_state = 1;
		pdata->oldlink = 0;
		pdata->speed = 0;
		pdata->oldduplex = -1;
		val = pdata->osi_core->xstats.link_disconnect_count;
		pdata->osi_core->xstats.link_disconnect_count =
			osi_update_stats_counter(val, 1UL);
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
 * @brief Initialize the PHY
 *
 * Algorithm: 1) Resets the PHY. 2) Connect to the phy described in the device tree.
 *
 * @param[in] dev: Network device data.
 *
 * @note MAC needs to be out of reset.
 *
 * @retval 0 on success
 * @retval "negative value" on failure.
 */
static int ether_phy_init(struct net_device *dev)
{
	struct ether_priv_data *pdata = netdev_priv(dev);
	struct phy_device *phydev = NULL;

	pdata->oldlink = 0;
	pdata->speed = SPEED_UNKNOWN;
	pdata->oldduplex = SPEED_UNKNOWN;


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

	/* In marvel phy driver pause is disabled. Instead of enabling
	 * in PHY driver, handle this in eqos driver so that enabling/disabling
	 * of the pause frame feature can be controlled based on the platform
	 */
	phydev->supported |= (SUPPORTED_Pause | SUPPORTED_Asym_Pause);
	if (pdata->osi_core->pause_frames == OSI_PAUSE_FRAMES_DISABLE)
		phydev->supported &= ~(SUPPORTED_Pause | SUPPORTED_Asym_Pause);

	phydev->advertising = phydev->supported;

	pdata->phydev = phydev;

	return 0;
}

/**
 * @brief Transmit done ISR Routine.
 *
 * Algorithm:
 * 1) Get channel number private data passed to ISR.
 * 2) Invoke OSI layer to clear Tx interrupt source.
 * 3) Disable DMA Tx channel interrupt.
 * 4) Schedule TX NAPI poll handler to cleanup the buffer.
 *
 * @param[in] irq: IRQ number.
 * @param[in] data: Tx NAPI private data structure.
 *
 * @note MAC and PHY need to be initialized.
 *
 * @retval IRQ_HANDLED on success
 * @retval IRQ_NONE on failure.
 */
static irqreturn_t ether_tx_chan_isr(int irq, void *data)
{
	struct ether_tx_napi *tx_napi = (struct ether_tx_napi *)data;
	struct ether_priv_data *pdata = tx_napi->pdata;
	struct osi_dma_priv_data *osi_dma = pdata->osi_dma;
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	unsigned int chan = tx_napi->chan;
	unsigned long flags;
	unsigned long val;

	osi_clear_tx_intr(osi_dma, chan);
	val = osi_core->xstats.tx_normal_irq_n[chan];
	osi_core->xstats.tx_normal_irq_n[chan] =
		osi_update_stats_counter(val, 1U);

	if (likely(napi_schedule_prep(&tx_napi->napi))) {
		spin_lock_irqsave(&pdata->rlock, flags);
		osi_disable_chan_tx_intr(osi_dma, chan);
		spin_unlock_irqrestore(&pdata->rlock, flags);
		__napi_schedule_irqoff(&tx_napi->napi);
	}

	return IRQ_HANDLED;
}

/**
 * @brief Receive done ISR Routine
 *
 * Algorithm:
 * 1) Get Rx channel number from Rx NAPI private data which will be passed
 * during request_irq() API.
 * 2) Invoke OSI layer to clear Rx interrupt source.
 * 3) Disable DMA Rx channel interrupt.
 * 4) Schedule Rx NAPI poll handler to get data from HW and pass to the
 * Linux network stack.
 *
 * @param[in] irq: IRQ number
 * @param[in] data: Rx NAPI private data structure.
 *
 * @note MAC and PHY need to be initialized.
 *
 * @retval IRQ_HANDLED on success
 * @retval IRQ_NONE on failure.
 */
static irqreturn_t ether_rx_chan_isr(int irq, void *data)
{
	struct ether_rx_napi *rx_napi = (struct ether_rx_napi *)data;
	struct ether_priv_data *pdata = rx_napi->pdata;
	struct osi_dma_priv_data *osi_dma = pdata->osi_dma;
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	unsigned int chan = rx_napi->chan;
	unsigned long val, flags;

	osi_clear_rx_intr(osi_dma, chan);
	val = osi_core->xstats.rx_normal_irq_n[chan];
	osi_core->xstats.rx_normal_irq_n[chan] =
		osi_update_stats_counter(val, 1U);


	if (likely(napi_schedule_prep(&rx_napi->napi))) {
		spin_lock_irqsave(&pdata->rlock, flags);
		osi_disable_chan_rx_intr(osi_dma, chan);
		spin_unlock_irqrestore(&pdata->rlock, flags);
		__napi_schedule_irqoff(&rx_napi->napi);
	}

	return IRQ_HANDLED;
}

/**
 * @brief Common ISR Routine
 *
 * Algorithm: Invoke OSI layer to handle common interrupt.
 *
 * @param[in] irq: IRQ number.
 * @param[in] data: Private data from ISR.
 *
 * @note MAC and PHY need to be initialized.
 *
 * @retval IRQ_HANDLED on success
 * @retval IRQ_NONE on failure.
 */
static irqreturn_t ether_common_isr(int irq, void *data)
{
	struct ether_priv_data *pdata =	(struct ether_priv_data *)data;

	osi_common_isr(pdata->osi_core);
	return IRQ_HANDLED;
}

/**
 * @brief Free IRQs
 *
 * Algorithm: This routine takes care of freeing below IRQs
 * 1) Common IRQ
 * 2) TX IRQ
 * 3) RX IRQ
 *
 * @param[in] pdata: OS dependent private data structure.
 *
 * @note IRQs should have registered.
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
 * @brief Register IRQs
 *
 * Algorithm: This routine takes care of requesting below IRQs
 * 1) Common IRQ
 * 2) TX IRQ
 * 3) RX IRQ
 *
 * @param[in] pdata: OS dependent private data structure.
 *
 * @note IRQ numbers need to be known.
 *
 * @retval 0 on success
 * @retval "negative value" on failure.
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
 * @brief Disable NAPI.
 *
 * Algorithm: Disable Tx and Rx NAPI for the channels which are enabled.
 *
 * @param[in] pdata: OSD private data structure.
 *
 * @note NAPI resources need to be allocated as part of probe().
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
 * @brief Enable NAPI.
 *
 * Algorithm: Enable Tx and Rx NAPI for the channels which are enabled.
 *
 * @param[in] pdata: OSD private data structure.
 *
 * @note NAPI resources need to be allocated as part of probe().
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

/**
 * @brief Free receive skbs
 * 
 * @param[in] rx_swcx: Rx pkt SW context
 * @param[in] dev: device instance associated with driver.
 * @param[in] rx_buf_len: Receive buffer length 
 */
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
 * @brief free_rx_dma_resources - Frees allocated Rx DMA resources.
 *	Algorithm: Release all DMA Rx resources which are allocated as part of
 *	allocated_rx_dma_ring() API.
 *
 * @param[in] osi_dma: OSI DMA private data structure.
 * @param[in] dev: device instance associated with driver.
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
 * @brief Allocate Rx DMA channel ring resources
 *
 * Algorithm: DMA receive ring will be created for valid channel number.
 * Receive ring updates with descriptors and software context associated
 * with each receive descriptor.
 *
 * @param[in] osi_dma: OSI DMA private data structure.
 * @param[in] dev: device instance associated with driver.
 * @param[in] chan: Rx DMA channel number.
 *
 * @note Invalid channel need to be updated.
 *
 * @retval 0 on success
 * @retval "negative value" on failure.
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

/**
 * @brief Allocate receive buffers for a DMA channel ring
 *
 * @param[in] pdata: OSD private data.
 * @param[in] rx_ring: rxring data structure.
 *
 * @retval 0 on success
 * @retval "negative value" on failure.
 */
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
 * @brief Allocate Receive DMA channel ring resources.
 *
 * Algorithm: DMA receive ring will be created for valid channel number
 * provided through DT
 *
 * @param[in] osi_dma: OSI private data structure.
 * @param[in] pdata: OSD private data.
 *
 * @note Invalid channel need to be updated.
 *
 * @retval 0 on success
 * @retval "negative value" on failure.
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
 * @brief Frees allocated DMA resources.
 *
 * Algorithm: Release all DMA Tx resources which are allocated as part of
 * allocated_tx_dma_ring() API.
 *
 * @param[in] osi_dma: OSI private data structure.
 * @param[in] dev: device instance associated with driver.
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
 * @brief Allocate Tx DMA ring
 *
 * Algorithm: DMA transmit ring will be created for valid channel number.
 * Transmit ring updates with descriptors and software context associated
 * with each transmit descriptor.
 *
 * @param[in] osi_dma: OSI  DMA private data structure.
 * @param[in] dev: device instance associated with driver.
 * @param[in] chan: Channel number.
 *
 * @retval 0 on success
 * @retval "negative value" on failure.
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
 * @brief Allocate Tx DMA resources.
 *
 * Algorithm: DMA transmit ring will be created for valid channel number
 * provided through DT
 *
 * @param[in] osi_dma: OSI  DMA private data structure.
 * @param[in] dev: device instance associated with driver.
 *
 * @note Invalid channel need to be updated.
 *
 * @retval 0 on success
 * @retval "negative value" on failure.
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
 * @brief Updates invalid channels list and DMA rings.
 *
 * Algorithm: Initialize all DMA Tx/Rx pointers to NULL so that for valid
 * dma_addr_t *channels Tx/Rx rings will be created.
 *
 * For ex: If the number of channels are 2 (nvidia,num_dma_chans = <2>)
 * and channel numbers are 2 and 3 (nvidia,dma_chans = <2 3>),
 * then only for channel 2 and 3 DMA rings will be allocated in
 * allocate_tx/rx_dma_resources() function.
 *
 * @param[in] osi_dma: OSI  DMA private data structure.
 *
 * @note OSD needs to be update number of channels and channel
 * numbers in OSI private data structure.
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
 * @brief Frees allocated DMA resources.
 *
 * Algorithm: Frees all DMA resources which are allocates with
 * allocate_dma_resources() API.
 *
 * @param[in] osi_dma: OSI private data structure.
 * @param[in] dev: device instance associated with driver.
 */
void free_dma_resources(struct osi_dma_priv_data *osi_dma, struct device *dev)
{
	free_tx_dma_resources(osi_dma, dev);
	free_rx_dma_resources(osi_dma, dev);
}

/**
 * @brief Allocate DMA resources for Tx and Rx.
 *
 * Algorithm:
 * 1) Allocate Tx DMA resources.
 * 2) Allocate Rx DMA resources.
 *
 * @param[in] pdata: OSD private data structure.
 *
 * @retval 0 on success
 * @retval "negative value" on failure.
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

#ifdef THERMAL_CAL
/**
 * @brief Set current thermal state.
 *
 * Algorithm: Fill the max supported thermal state for ethernet cooling
 * device in variable provided by caller.
 *
 * @param[in] tcd: Ethernet thermal cooling device pointer.
 * @param[in] state: Variable to read max thermal state.
 *
 * @note MAC needs to be out of reset. Once cooling device ops are
 * registered, it can be called anytime from kernel. MAC has to be in
 * sufficient state to allow pad calibration.
 *
 * return: 0 - succcess. Does not fail as function is only reading variable.
 */
static int ether_get_max_therm_state(struct thermal_cooling_device *tcd,
				     unsigned long *state)
{
	*state = ETHER_MAX_THERM_STATE;

	return 0;
}

/**
 * @brief Get current thermal state.
 *
 * Algorithm: Atomically get the current thermal state of etherent
 * cooling device.
 *
 * @param[in] tcd: Ethernet thermal cooling device pointer.
 * @param[in] state: Variable to read current thermal state.
 *
 * @note MAC needs to be out of reset. Once cooling device ops are
 * registered, it can be called anytime from kernel. MAC has to be in
 * sufficient state to allow pad calibration.
 *
 * @return: succcess on 0.
 */
static int ether_get_cur_therm_state(struct thermal_cooling_device *tcd,
				     unsigned long *state)
{
	struct ether_priv_data *pdata = tcd->devdata;

	*state = (unsigned long)atomic_read(&pdata->therm_state);

	return 0;
}

/**
 * @brief Set current thermal state.
 *
 * Algorithm: Atomically set the desired state provided as argument.
 * Trigger pad calibration for each state change.
 *
 * @param[in] tcd: Ethernet thermal cooling device pointer.
 * @param[in] state: The thermal state to set.
 *
 * @note MAC needs to be out of reset. Once cooling device ops are
 * registered, it can be called anytime from kernel. MAC has to be in
 * sufficient state to allow pad calibration.
 *
 * @retval 0 on success
 * @retval negative value on failure.
 */
static int ether_set_cur_therm_state(struct thermal_cooling_device *tcd,
				     unsigned long state)
{
	struct ether_priv_data *pdata = tcd->devdata;
	struct device *dev = pdata->dev;

	/* Thermal framework will take care of making sure state is within
	 * valid bounds, based on the get_max_state callback. So no need
	 * to validate bounds again here.
	 */
	dev_info(dev, "Therm state change from %d to %lu\n",
		 atomic_read(&pdata->therm_state), state);

	atomic_set(&pdata->therm_state, state);

	if (osi_pad_calibrate(pdata->osi_core) < 0) {
		dev_err(dev, "Therm state changed, failed pad calibration\n");
		return -1;
	}

	return 0;
}

static struct thermal_cooling_device_ops ether_cdev_ops = {
	.get_max_state = ether_get_max_therm_state,
	.get_cur_state = ether_get_cur_therm_state,
	.set_cur_state = ether_set_cur_therm_state,
};

/**
 * @brief Register thermal cooling device with kernel.
 *
 * Algorithm: Register thermal cooling device read from DT. The cooling
 * device ops struct passed as argument will be used by thermal framework
 * to callback the ethernet driver when temperature trip points are
 * triggered, so that ethernet driver can do pad calibration.
 *
 * @param[in] pdata: Pointer to driver private data structure.
 *
 * @ote MAC needs to be out of reset. Once cooling device ops are
 * registered, it can be called anytime from kernel. MAC has to be in
 * sufficient state to allow pad calibration.
 *
 * @retval 0 on success
 * @retval negative value on failure.
 */
static int ether_therm_init(struct ether_priv_data *pdata)
{
	struct device_node *np = NULL;

	np = of_find_node_by_name(NULL, "eqos-cool-dev");
	if (!np) {
		dev_err(pdata->dev, "failed to get eqos-cool-dev\n");
		return -ENODEV;
	}
	pdata->tcd = thermal_of_cooling_device_register(np,
							"tegra-eqos", pdata,
							&ether_cdev_ops);
	if (!pdata->tcd) {
		return -ENODEV;
	} else if (IS_ERR(pdata->tcd)) {
		return PTR_ERR(pdata->tcd);
	}

	return 0;
}
#endif /* THERMAL_CAL */

/**
 * @brief Call back to handle bring up of Ethernet interface
 *
 * Algorithm: This routine takes care of below
 * 1) PHY initialization
 * 2) request tx/rx/common irqs
 * 3) HW initialization
 * 4) OSD private data structure initialization
 * 5) Starting the PHY
 *
 * @param[in] dev: Net device data structure.
 *
 * @note Ethernet driver probe need to be completed successfully
 * with ethernet network device created.
 *
 * @retval 0 on success
 * @retval "negative value" on failure.
 */
static int ether_open(struct net_device *dev)
{
	struct ether_priv_data *pdata = netdev_priv(dev);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	int ret = 0;

	/* Reset the PHY */
	if (gpio_is_valid(pdata->phy_reset)) {
		gpio_set_value(pdata->phy_reset, 0);
		usleep_range(10, 11);
		gpio_set_value(pdata->phy_reset, 1);
	}

	ret = ether_enable_clks(pdata);
	if (ret < 0) {
		dev_err(&dev->dev, "failed to enable clks\n");
		return ret;
	}

	if (pdata->mac_rst) {
		ret = reset_control_reset(pdata->mac_rst);
		if (ret < 0) {
			dev_err(&dev->dev, "failed to reset MAC HW\n");
			goto err_mac_rst;
		}
	}

	ret = osi_poll_for_swr(osi_core);
	if (ret < 0) {
		dev_err(&dev->dev, "failed to poll MAC Software reset\n");
		goto err_poll_swr;
	}

	/* PHY reset and initialization */
	ret = ether_phy_init(dev);
	if (ret < 0) {
		dev_err(&dev->dev, "%s: Cannot attach to PHY (error: %d)\n",
			__func__, ret);
		goto err_phy_init;
	}

	osi_set_rx_buf_len(pdata->osi_dma);

	ret = ether_allocate_dma_resources(pdata);
	if (ret < 0) {
		dev_err(pdata->dev, "failed to allocate DMA resources\n");
		goto err_alloc;
	}

#ifdef THERMAL_CAL
	atomic_set(&pdata->therm_state, 0);
	ret = ether_therm_init(pdata);
	if (ret < 0) {
		dev_err(pdata->dev, "Failed to register cooling device (%d)\n",
			ret);
		goto err_therm;
	}
#endif /* THERMAL_CAL */

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

	/* As all registers reset as part of ether_close(), reset private
	 * structure variable as well */
	pdata->vlan_hash_filtering = OSI_PERFECT_FILTER_MODE;
	pdata->l2_filtering_mode = OSI_PERFECT_FILTER_MODE;

	/* Initialize PTP */
	ret = ether_ptp_init(pdata);
	if (ret < 0) {
		dev_err(pdata->dev,
			"%s:failed to initialize PTP with reason %d\n",
			__func__, ret);
		goto err_hw_init;
	}

	/* Enable napi before requesting irq to be ready to handle it */
	ether_napi_enable(pdata);

	/* request tx/rx/common irq */
	ret = ether_request_irqs(pdata);
	if (ret < 0) {
		dev_err(&dev->dev,
			"%s: failed to get tx rx irqs with reason %d\n",
			__func__, ret);
		goto err_r_irq;
	}

	/* Start the MAC */
	osi_start_mac(pdata->osi_core);

	/* start PHY */
	phy_start(pdata->phydev);

	/* start network queues */
	netif_tx_start_all_queues(pdata->ndev);

	/* call function to schedule workqueue */
	ether_stats_work_queue_start(pdata);

	return ret;

err_r_irq:
	ether_napi_disable(pdata);
	ether_ptp_remove(pdata);
err_hw_init:
#ifdef THERMAL_CAL
	thermal_cooling_device_unregister(pdata->tcd);
err_therm:
#endif /* THERMAL_CAL */
	free_dma_resources(pdata->osi_dma, pdata->dev);
err_alloc:
	if (pdata->phydev) {
		phy_disconnect(pdata->phydev);
	}
err_phy_init:
err_poll_swr:
	if (pdata->mac_rst) {
		reset_control_assert(pdata->mac_rst);
	}
err_mac_rst:
	ether_disable_clks(pdata);
	if (gpio_is_valid(pdata->phy_reset)) {
		gpio_set_value(pdata->phy_reset, OSI_DISABLE);
	}

	return ret;
}

/**
 * @brief Helper function to clear the sw stats structures.
 *
 * Algorithm: This routine clears the following sw stats structures.
 * 1) struct osi_mmc_counters
 * 2) struct osi_xtra_stat_counters
 * 3) struct osi_xtra_dma_stat_counters
 * 4) struct osi_pkt_err_stats
 *
 * @param[in] pdata: Pointer to OSD private data structure.
 *
 * @note  This routine is invoked when interface is going down, not for suspend.
 */
static inline void ether_reset_stats(struct ether_priv_data *pdata)
{
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	struct osi_dma_priv_data *osi_dma = pdata->osi_dma;

	memset(&osi_core->mmc, 0U, sizeof(struct osi_mmc_counters));
	memset(&osi_core->xstats, 0U,
	       sizeof(struct osi_xtra_stat_counters));
	memset(&osi_dma->dstats, 0U,
	       sizeof(struct osi_xtra_dma_stat_counters));
	memset(&osi_dma->pkt_err_stats, 0U, sizeof(struct osi_pkt_err_stats));
}

/**
 * @brief Call back to handle bring down of Ethernet interface
 *
 * Algorithm: This routine takes care of below
 * 1) Stopping PHY
 * 2) Freeing tx/rx/common irqs
 *
 * @param[in] dev: Net device data structure.
 *
 * @note  MAC Interface need to be registered.
 *
 * @retval 0 on success
 * @retval "negative value" on failure.
 */
static int ether_close(struct net_device *dev)
{
	struct ether_priv_data *pdata = netdev_priv(dev);
	int ret = 0;

	/* Unregister broadcasting MAC timestamp to clients */
	tegra_unregister_hwtime_source();

	/* Stop workqueue to get further scheduled */
	ether_stats_work_queue_stop(pdata);

	/* Stop and disconnect the PHY */
	if (pdata->phydev != NULL) {
		phy_stop(pdata->phydev);
		phy_disconnect(pdata->phydev);

		if (gpio_is_valid(pdata->phy_reset)) {
			gpio_set_value(pdata->phy_reset, 0);
		}
		pdata->phydev = NULL;
	}

	/* turn off sources of data into dev */
	netif_tx_disable(pdata->ndev);

	/* Free tx rx and common irqs */
	ether_free_irqs(pdata);

	/* DMA De init */
	osi_hw_dma_deinit(pdata->osi_dma);

#ifdef THERMAL_CAL
	thermal_cooling_device_unregister(pdata->tcd);
#endif /* THERMAL_CAL */

	/* free DMA resources after DMA stop */
	free_dma_resources(pdata->osi_dma, pdata->dev);

	/* PTP de-init */
	ether_ptp_remove(pdata);

	/* MAC deinit which inturn stop MAC Tx,Rx */
	osi_hw_core_deinit(pdata->osi_core);

	ether_napi_disable(pdata);

	/* Assert MAC RST gpio */
	if (pdata->mac_rst) {
		reset_control_assert(pdata->mac_rst);
	}

	/* Disable clock */
	ether_disable_clks(pdata);

	/* Reset stats since interface is going down */
	ether_reset_stats(pdata);

	return ret;
}

/**
 * @brief Helper function to check if TSO is used in given skb.
 *
 * Algorithm:
 * 1) Check if driver received a TSO/LSO/GSO packet
 * 2) If so, store the packet details like MSS(Maximum Segment Size),
 * packet header length, packet payload length, tcp/udp header length.
 *
 * @param[in] tx_pkt_cx: Pointer to packet context information structure.
 * @param[in] skb: socket buffer.
 *
 * @retval 0 if not a TSO packet
 * @retval 1 on success
 * @retval "negative value" on failure.
 */
static int ether_handle_tso(struct osi_tx_pkt_cx *tx_pkt_cx,
			    struct sk_buff *skb)
{
	int ret = 1;

	if (skb_is_gso(skb) == 0) {
		return 0;
	}

	if (skb_header_cloned(skb)) {
		ret = pskb_expand_head(skb, 0, 0, GFP_ATOMIC);
		if (ret) {
			return ret;
		}
	}

	/* Start filling packet details in Tx_pkt_cx */
	if (skb_shinfo(skb)->gso_type & (SKB_GSO_UDP)) {
		tx_pkt_cx->tcp_udp_hdrlen = sizeof(struct udphdr);
		tx_pkt_cx->mss = skb_shinfo(skb)->gso_size -
			sizeof(struct udphdr);
	} else {
		tx_pkt_cx->tcp_udp_hdrlen = tcp_hdrlen(skb);
		tx_pkt_cx->mss = skb_shinfo(skb)->gso_size;
	}
	tx_pkt_cx->total_hdrlen = skb_transport_offset(skb) +
			tx_pkt_cx->tcp_udp_hdrlen;
	tx_pkt_cx->payload_len = (skb->len - tx_pkt_cx->total_hdrlen);

	netdev_dbg(skb->dev, "mss           =%u\n", tx_pkt_cx->mss);
	netdev_dbg(skb->dev, "payload_len   =%u\n", tx_pkt_cx->payload_len);
	netdev_dbg(skb->dev, "tcp_udp_hdrlen=%u\n", tx_pkt_cx->tcp_udp_hdrlen);
	netdev_dbg(skb->dev, "total_hdrlen  =%u\n", tx_pkt_cx->total_hdrlen);

	return ret;
}

/**
 * @brief Tx ring software context allocation.
 *
 * Algorithm:
 * 1) Map skb data buffer to DMA mappable address.
 * 2) Updated dma address, len and buffer address. This info will be used
 * OSI layer for data transmission and buffer cleanup.
 *
 * @param[in] dev: device instance associated with driver.
 * @param[in] tx_ring: Tx ring instance associated with channel number.
 * @param[in] skb: socket buffer.
 *
 * @retval "number of descriptors" on success
 * @retval "negative value"  on failure.
 */
static int ether_tx_swcx_alloc(struct device *dev,
			       struct osi_tx_ring *tx_ring,
			       struct sk_buff *skb)
{
	struct osi_tx_pkt_cx *tx_pkt_cx = &tx_ring->tx_pkt_cx;
	unsigned int cur_tx_idx = tx_ring->cur_tx_idx;
	struct osi_tx_swcx *tx_swcx = NULL;
	unsigned int len = 0, offset = 0, size = 0;
	int cnt = 0, ret = 0, i, num_frags;
	struct skb_frag_struct *frag;
	unsigned int page_idx, page_offset;
	unsigned int max_data_len_per_txd = (unsigned int)
					ETHER_TX_MAX_BUFF_SIZE;

	memset(tx_pkt_cx, 0, sizeof(*tx_pkt_cx));

	ret = ether_handle_tso(tx_pkt_cx, skb);
	if (unlikely(ret < 0)) {
		dev_err(dev, "Unable to handle TSO packet (%d)\n", ret);
		/* Caller will take care of consuming skb */
		return ret;
	}

	if (ret == 0) {
		dev_dbg(dev, "Not a TSO packet\n");
		if (skb->ip_summed == CHECKSUM_PARTIAL) {
			tx_pkt_cx->flags |= OSI_PKT_CX_CSUM;
		}
	} else {
		tx_pkt_cx->flags |= OSI_PKT_CX_TSO;
	}

	if (unlikely(skb_vlan_tag_present(skb))) {
		tx_pkt_cx->vtag_id = skb_vlan_tag_get(skb);
		tx_pkt_cx->vtag_id |= (skb->priority << VLAN_PRIO_SHIFT);
		tx_pkt_cx->flags |= OSI_PKT_CX_VLAN;
	}

	if (unlikely(skb_shinfo(skb)->tx_flags & SKBTX_HW_TSTAMP)) {
		skb_shinfo(skb)->tx_flags |= SKBTX_IN_PROGRESS;
		tx_pkt_cx->flags |= OSI_PKT_CX_PTP;
	}

	if (((tx_pkt_cx->flags & OSI_PKT_CX_VLAN) == OSI_PKT_CX_VLAN) ||
	    ((tx_pkt_cx->flags & OSI_PKT_CX_TSO) == OSI_PKT_CX_TSO)) {
		tx_swcx = tx_ring->tx_swcx + cur_tx_idx;
		if (tx_swcx->len) {
			return 0;
		}

		tx_swcx->len = -1;
		cnt++;
		INCR_TX_DESC_INDEX(cur_tx_idx, 1U);
	}

	if ((tx_pkt_cx->flags & OSI_PKT_CX_TSO) == OSI_PKT_CX_TSO) {
		/* For TSO map the header in separate desc. */
		len = tx_pkt_cx->total_hdrlen;
	} else {
		len = skb_headlen(skb);
	}

	/* Map the linear buffers from the skb first.
	 * For TSO only upto TCP header is filled in first desc.
	 */
	while (len) {
		tx_swcx = tx_ring->tx_swcx + cur_tx_idx;
		if (unlikely(tx_swcx->len)) {
			goto desc_not_free;
		}

		size = min(len, max_data_len_per_txd);

		tx_swcx->buf_phy_addr = dma_map_single(dev,
						skb->data + offset,
						size,
						DMA_TO_DEVICE);
		if (unlikely(dma_mapping_error(dev,
			     tx_swcx->buf_phy_addr))) {
			dev_err(dev, "failed to map Tx buffer\n");
			ret = -ENOMEM;
			goto dma_map_failed;
		}
		tx_swcx->is_paged_buf = 0;

		tx_swcx->len = size;
		len -= size;
		offset += size;
		cnt++;
		INCR_TX_DESC_INDEX(cur_tx_idx, 1U);
	}

	/* Map remaining payload from linear buffer
	 * to subsequent descriptors in case of TSO
	 */
	if ((tx_pkt_cx->flags & OSI_PKT_CX_TSO) == OSI_PKT_CX_TSO) {
		len = skb_headlen(skb) - tx_pkt_cx->total_hdrlen;
		while (len) {
			tx_swcx = tx_ring->tx_swcx + cur_tx_idx;

			if (unlikely(tx_swcx->len)) {
				goto desc_not_free;
			}

			size = min(len, max_data_len_per_txd);
			tx_swcx->buf_phy_addr = dma_map_single(dev,
							skb->data + offset,
							size,
							DMA_TO_DEVICE);
			if (unlikely(dma_mapping_error(dev,
				     tx_swcx->buf_phy_addr))) {
				dev_err(dev, "failed to map Tx buffer\n");
				ret = -ENOMEM;
				goto dma_map_failed;
			}

			tx_swcx->is_paged_buf = 0;
			tx_swcx->len = size;
			len -= size;
			offset += size;
			cnt++;
			INCR_TX_DESC_INDEX(cur_tx_idx, 1U);
		}
	}

	/* Process fragmented skb's */
	num_frags = skb_shinfo(skb)->nr_frags;
	for (i = 0; i < num_frags; i++) {
		offset = 0;
		frag = &skb_shinfo(skb)->frags[i];
		len = skb_frag_size(frag);
		while (len) {
			tx_swcx = tx_ring->tx_swcx + cur_tx_idx;
			if (unlikely(tx_swcx->len)) {
				goto desc_not_free;
			}

			size = min(len, max_data_len_per_txd);

			page_idx = (frag->page_offset + offset) >> PAGE_SHIFT;
			page_offset = (frag->page_offset + offset) & ~PAGE_MASK;
			tx_swcx->buf_phy_addr = dma_map_page(dev,
						(frag->page.p + page_idx),
						page_offset, size,
						DMA_TO_DEVICE);
			if (unlikely(dma_mapping_error(dev,
				     tx_swcx->buf_phy_addr))) {
				dev_err(dev, "failed to map Tx buffer\n");
				ret = -ENOMEM;
				goto dma_map_failed;
			}
			tx_swcx->is_paged_buf = 1;

			tx_swcx->len = size;
			len -= size;
			offset += size;
			cnt++;
			INCR_TX_DESC_INDEX(cur_tx_idx, 1U);
		}
	}

	tx_swcx->buf_virt_addr = skb;
	tx_pkt_cx->desc_cnt = cnt;

	return cnt;

desc_not_free:
	ret = 0;

dma_map_failed:
	/* Failed to fill current desc. Rollback previous desc's */
	while (cnt) {
		DECR_TX_DESC_INDEX(cur_tx_idx, 1U);
		tx_swcx = tx_ring->tx_swcx + cur_tx_idx;
		if (tx_swcx->buf_phy_addr) {
			if (tx_swcx->is_paged_buf) {
				dma_unmap_page(dev, tx_swcx->buf_phy_addr,
					       tx_swcx->len, DMA_TO_DEVICE);
			} else {
				dma_unmap_single(dev, tx_swcx->buf_phy_addr,
						 tx_swcx->len, DMA_TO_DEVICE);
			}
			tx_swcx->buf_phy_addr = 0;
		}
		tx_swcx->len = 0;

		tx_swcx->is_paged_buf = 0;
		cnt--;
	}
	return ret;
}

/**
 * @brief Select queue based on user priority
 *
 * Algorithm:
 * 1) Select the correct queue index based which has priority of queue
 * same as skb->priority
 * 2) default select queue array index 0
 *
 * @param[in] dev: Network device pointer
 * @param[in] skb: sk_buff pointer, buffer data to send
 * @param[in] accel_priv: private data used for L2 forwarding offload
 * @param[in] fallback: fallback function pointer
 *
 * @retval "transmit queue index"
 */
static unsigned short ether_select_queue(struct net_device *dev,
					 struct sk_buff *skb,
					 void *accel_priv,
					 select_queue_fallback_t fallback)
{
	struct ether_priv_data *pdata = netdev_priv(dev);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	unsigned short txqueue_select = 0;
	unsigned int i, mtlq;

	for (i = 0; i < osi_core->num_mtl_queues; i++) {
		mtlq = osi_core->mtl_queues[i];
		if (pdata->txq_prio[mtlq] == skb->priority) {
			txqueue_select = (unsigned short)i;
			break;
		}
	}

	return txqueue_select;
}

/**
 * @brief Network layer hook for data transmission.
 *
 * Algorithm:
 * 1) Allocate software context (DMA address for the buffer) for the data.
 * 2) Invoke OSI for data transmission.
 *
 * @param[in] skb: SKB data structure.
 * @param[in] ndev: Net device structure.
 *
 * @note  MAC and PHY need to be initialized.
 *
 * @retval 0 on success
 * @retval "negative value" on failure.
 */
static int ether_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	struct ether_priv_data *pdata = netdev_priv(ndev);
	struct osi_dma_priv_data *osi_dma = pdata->osi_dma;
	unsigned int qinx = skb_get_queue_mapping(skb);
	unsigned int chan = osi_dma->dma_chans[qinx];
	struct osi_tx_ring *tx_ring = osi_dma->tx_ring[chan];
	int count = 0;

	count = ether_tx_swcx_alloc(pdata->dev, tx_ring, skb);
	if (count <= 0) {
		if (count == 0) {
			netif_stop_subqueue(ndev, qinx);
			netdev_err(ndev, "Tx ring[%d] is full\n", chan);
			return NETDEV_TX_BUSY;
		}
		dev_kfree_skb_any(skb);
		return NETDEV_TX_OK;
	}

	osi_hw_transmit(osi_dma, chan);

	if (ether_avail_txdesc_cnt(tx_ring) <= ETHER_TX_DESC_THRESHOLD) {
		netif_stop_subqueue(ndev, qinx);
		netdev_dbg(ndev, "Tx ring[%d] insufficient desc.\n", chan);
	}

	return NETDEV_TX_OK;
}

/**
 * @brief Function to configure the multicast address in device.
 *
 * Algorithm: This function collects all the multicast addresses and updates the
 * device.
 *
 * @param[in] dev: Pointer to net_device structure.
 *
 * @note  MAC and PHY need to be initialized.
 *
 * @retval 0 on success
 * @retval "negative value" on failure.
 */
static int ether_prepare_mc_list(struct net_device *dev,
				 struct osi_filter *filter)
{
	struct ether_priv_data *pdata = netdev_priv(dev);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	struct netdev_hw_addr *ha;
	unsigned int i = 1;
	int ret = -1;

	if (filter == NULL) {
		dev_err(pdata->dev, "filter is NULL\n");
		return ret;
	}

	memset(filter, 0x0, sizeof(struct osi_filter));

	if (pdata->l2_filtering_mode == OSI_HASH_FILTER_MODE) {
		dev_err(pdata->dev,
			"HASH FILTERING for mc addresses not Supported in SW\n");
		filter->oper_mode = (OSI_OPER_EN_PERFECT |
				     OSI_OPER_DIS_PROMISC |
				     OSI_OPER_DIS_ALLMULTI);

		return osi_l2_filter(osi_core, filter);
	/* address 0 is used for DUT DA so compare with
	 *  pdata->num_mac_addr_regs - 1
	 */
	} else if (netdev_mc_count(dev) > (pdata->num_mac_addr_regs - 1)) {
		/* switch to PROMISCUOUS mode */
		filter->oper_mode = (OSI_OPER_DIS_PERFECT |
				     OSI_OPER_EN_PROMISC |
				     OSI_OPER_DIS_ALLMULTI);
		dev_dbg(pdata->dev, "enabling Promiscuous mode\n");

		return osi_l2_filter(osi_core, filter);
	} else {
		dev_dbg(pdata->dev,
			"select PERFECT FILTERING for mc addresses, mc_count = %d, num_mac_addr_regs = %d\n",
			netdev_mc_count(dev), pdata->num_mac_addr_regs);

		filter->oper_mode = (OSI_OPER_EN_PERFECT |
				     OSI_OPER_ADDR_UPDATE |
				     OSI_OPER_DIS_PROMISC |
				     OSI_OPER_DIS_ALLMULTI);
		netdev_for_each_mc_addr(ha, dev) {
			dev_dbg(pdata->dev,
				"mc addr[%d] = %#x:%#x:%#x:%#x:%#x:%#x\n",
				i, ha->addr[0], ha->addr[1], ha->addr[2],
				ha->addr[3], ha->addr[4], ha->addr[5]);
			filter->index = i;
			filter->mac_address = ha->addr;
			filter->dma_routing = OSI_DISABLE;
			filter->dma_chan = 0x0;
			filter->addr_mask = OSI_AMASK_DISABLE;
			filter->src_dest = OSI_DA_MATCH;
			ret = osi_l2_filter(osi_core, filter);
			if (ret < 0) {
				dev_err(pdata->dev, "issue in creating mc list\n");
				pdata->last_filter_index = i - 1;
				return ret;
			}

			if (i == EQOS_MAX_MAC_ADDRESS_FILTER - 1) {
				dev_err(pdata->dev, "Configured max number of supported MAC, ignoring it\n");
				break;
			}
			i++;
		}
		/* preserve last filter index to pass on to UC */
		pdata->last_filter_index = i - 1;
	}

	return ret;
}

/**
 * @brief Function to configure the unicast address
 * in device.
 *
 * Algorithm: This function collects all the unicast addresses and updates the
 * device.
 *
 * @param[in] dev: pointer to net_device structure.
 *
 * @note  MAC and PHY need to be initialized.
 *
 * @retval 0 on success
 * @retval "negative value" on failure.
 */
static int ether_prepare_uc_list(struct net_device *dev,
				 struct osi_filter *filter)
{
	struct ether_priv_data *pdata = netdev_priv(dev);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	/* last valid MC/MAC DA + 1 should be start of UC addresses */
	unsigned int i = pdata->last_filter_index + 1;
	struct netdev_hw_addr *ha;
	int ret = -1;

	if (filter == NULL) {
		dev_err(pdata->dev, "filter is NULL\n");
		return ret;
	}

	memset(filter, 0x0, sizeof(struct osi_filter));

	if (pdata->l2_filtering_mode == OSI_HASH_FILTER_MODE) {
		dev_err(pdata->dev,
			"HASH FILTERING for uc addresses not Supported in SW\n");
		/* Perfect filtering for multicast */
		filter->oper_mode = (OSI_OPER_EN_PERFECT |
				     OSI_OPER_DIS_PROMISC |
				     OSI_OPER_DIS_ALLMULTI);

		return osi_l2_filter(osi_core, filter);
	} else if (netdev_uc_count(dev) > (pdata->num_mac_addr_regs - i)) {
		/* switch to PROMISCUOUS mode */
		filter->oper_mode = (OSI_OPER_DIS_PERFECT |
				     OSI_OPER_EN_PROMISC |
				     OSI_OPER_DIS_ALLMULTI);
		dev_dbg(pdata->dev, "enabling Promiscuous mode\n");

		return osi_l2_filter(osi_core, filter);
	} else {
		dev_dbg(pdata->dev,
				"select PERFECT FILTERING for uc addresses: uc_count = %d\n",
			netdev_uc_count(dev));

		filter->oper_mode = (OSI_OPER_EN_PERFECT |
				     OSI_OPER_ADDR_UPDATE |
				     OSI_OPER_DIS_PROMISC |
				     OSI_OPER_DIS_ALLMULTI);
		netdev_for_each_uc_addr(ha, dev) {
			dev_dbg(pdata->dev,
				"uc addr[%d] = %#x:%#x:%#x:%#x:%#x:%#x\n",
				i, ha->addr[0], ha->addr[1], ha->addr[2],
				ha->addr[3], ha->addr[4], ha->addr[5]);
			filter->index = i;
			filter->mac_address = ha->addr;
			filter->dma_routing = OSI_DISABLE;
			filter->dma_chan = 0x0;
			filter->addr_mask = OSI_AMASK_DISABLE;
			filter->src_dest = OSI_DA_MATCH;
			ret = osi_l2_filter(osi_core, filter);
			if (ret < 0) {
				dev_err(pdata->dev, "issue in creating uc list\n");
				pdata->last_filter_index = i - 1;
				return ret;
			}

			if (i == EQOS_MAX_MAC_ADDRESS_FILTER - 1) {
				dev_err(pdata->dev, "Already MAX MAC added\n");
				break;
			}
			i++;
		}
		pdata->last_filter_index  = i - 1;
	}

	return ret;
}

/**
 * @brief This function is used to set RX mode.
 *
 * Algorithm: Based on Network interface flag, MAC registers are programmed to
 * set mode.
 *
 * @param[in] dev - pointer to net_device structure.
 *
 * @note MAC and PHY need to be initialized.
 */
static void ether_set_rx_mode(struct net_device *dev)
{
	struct ether_priv_data *pdata = netdev_priv(dev);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	struct osi_filter filter;
	/* store last call last_uc_filter_index in temporary variable */
	int last_index = pdata->last_filter_index;
	int ret = -1;
	unsigned int i = 0;

	memset(&filter, 0x0, sizeof(struct osi_filter));
	if ((dev->flags & IFF_PROMISC) == IFF_PROMISC) {
		filter.oper_mode = (OSI_OPER_DIS_PERFECT | OSI_OPER_EN_PROMISC |
				    OSI_OPER_DIS_ALLMULTI);
		dev_dbg(pdata->dev, "enabling Promiscuous mode\n");
		ret = osi_l2_filter(osi_core, &filter);
		if (ret < 0) {
			dev_err(pdata->dev, "Setting Promiscuous mode failed\n");
		}

		return;
	} else if ((dev->flags & IFF_ALLMULTI) == IFF_ALLMULTI) {
		filter.oper_mode = (OSI_OPER_EN_ALLMULTI |
				    OSI_OPER_DIS_PERFECT |
				    OSI_OPER_DIS_PROMISC);
		dev_dbg(pdata->dev, "pass all multicast pkt\n");
		ret = osi_l2_filter(osi_core, &filter);
		if (ret < 0) {
			dev_err(pdata->dev, "Setting All Multicast allow mode failed\n");
		}

		return;
	} else if (!netdev_mc_empty(dev)) {
		if (ether_prepare_mc_list(dev, &filter) != 0) {
			dev_err(pdata->dev, "Setting MC address failed\n");
		}
	} else {
		pdata->last_filter_index = 0;
	}

	if (!netdev_uc_empty(dev)) {
		if (ether_prepare_uc_list(dev, &filter) != 0) {
			dev_err(pdata->dev, "Setting UC address failed\n");
		}
	}

	/* Reset the filter structure to avoid any old value */
	memset(&filter, 0x0, sizeof(struct osi_filter));
	/* invalidate remaining ealier address */
	for (i = pdata->last_filter_index + 1; i <= last_index; i++) {
		filter.oper_mode = OSI_OPER_ADDR_UPDATE;
		filter.index = i;
		filter.mac_address = NULL;
		filter.dma_routing = OSI_DISABLE;
		filter.dma_chan = OSI_CHAN_ANY;
		filter.addr_mask = OSI_AMASK_DISABLE;
		filter.src_dest = OSI_DA_MATCH;
		ret = osi_l2_filter(osi_core, &filter);
		if (ret < 0) {
			dev_err(pdata->dev, "Invalidating expired L2 filter failed\n");
			return;
		}
	}

	return;
}

/**
 * @brief Network stack IOCTL hook to driver
 *
 * Algorithm:
 * 1) Invokes MII API for phy read/write based on IOCTL command
 * 2) SIOCDEVPRIVATE for private ioctl
 *
 * @param[in] dev: network device structure
 * @param[in] rq: Interface request structure used for socket
 * @param[in] cmd: IOCTL command code
 *
 * @note  Ethernet interface need to be up.
 *
 * @retval 0 on success
 * @retval "negative value" on failure.
 */
static int ether_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	int ret = -EOPNOTSUPP;
	struct ether_priv_data *pdata = netdev_priv(dev);

	if (!dev || !rq) {
		dev_err(pdata->dev, "%s: Invalid arg\n", __func__);
		return -EINVAL;
	}

	if (!netif_running(dev)) {
		dev_err(pdata->dev, "%s: Interface not up\n", __func__);
		return -EINVAL;
	}

	switch (cmd) {
	case SIOCGMIIPHY:
	case SIOCGMIIREG:
	case SIOCSMIIREG:
		if (!dev->phydev) {
			return -EINVAL;
		}

		/* generic PHY MII ioctl interface */
		ret = phy_mii_ioctl(dev->phydev, rq, cmd);
		break;

	case SIOCDEVPRIVATE:
		ret = ether_handle_priv_ioctl(dev, rq);
		break;

	case ETHER_PRV_TS_IOCTL:
		ret = ether_handle_priv_ts_ioctl(pdata, rq);
		break;

	case SIOCSHWTSTAMP:
		ret = ether_handle_hwtstamp_ioctl(pdata, rq);
		break;

	default:
		netdev_err(dev, "%s: Unsupported ioctl %d\n",
			   __func__, cmd);
		break;
	}

	return ret;
}

/**
 * @brief Set MAC address
 *
 * Algorithm:
 * 1) Checks whether given MAC address is valid or not
 * 2) Stores the MAC address in OSI core structure
 *
 * @param[in] ndev: Network device structure
 * @param[in] addr: MAC address to be programmed.
 *
 * @note  Ethernet interface need to be down to set MAC address
 *
 * @retval 0 on success
 * @retval "negative value" on failure.
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
 * @brief Change MAC MTU size
 *
 * Algorithm:
 * 1) Check and return if interface is up.
 * 2) Stores new MTU size set by user in OSI core data structure.
 *     
 * @param[in] ndev: Network device structure
 * @param[in] new_mtu: New MTU size to set.
 *
 * @note  Ethernet interface need to be down to change MTU size
 *
 * @retval 0 on success
 * @retval "negative value" on failure.
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

/**
 * @brief Change HW features for the given network device.
 *
 * Algorithm:
 * 1) Check if HW supports feature requested to be changed
 * 2) If supported, check the current status of the feature and if it
 * needs to be toggled, do so.
 *
 * @param[in] ndev: Network device structure
 * @param[in] feat: New features to be updated
 *
 * @note  Ethernet interface needs to be up. Stack will enforce
 * the check.
 *
 * @retval 0 on success
 * @retval "negative value" on failure.
 */
static int ether_set_features(struct net_device *ndev, netdev_features_t feat)
{
	int ret = 0;
	struct ether_priv_data *pdata = netdev_priv(ndev);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	netdev_features_t hw_feat_cur_state = pdata->hw_feat_cur_state;

	if (pdata->hw_feat.rx_coe_sel == 0U) {
		return ret;
	}

	if ((feat & NETIF_F_RXCSUM) == NETIF_F_RXCSUM) {
		if (!(hw_feat_cur_state & NETIF_F_RXCSUM)) {
			ret = osi_config_rxcsum_offload(osi_core,
							OSI_ENABLE);
			dev_info(pdata->dev, "Rx Csum offload: Enable: %s\n",
				 ret ? "Failed" : "Success");
			pdata->hw_feat_cur_state |= NETIF_F_RXCSUM;
		}
	} else {
		if ((hw_feat_cur_state & NETIF_F_RXCSUM)) {
			ret = osi_config_rxcsum_offload(osi_core,
							OSI_DISABLE);
			dev_info(pdata->dev, "Rx Csum offload: Disable: %s\n",
				 ret ? "Failed" : "Success");
			pdata->hw_feat_cur_state &= ~NETIF_F_RXCSUM;
		}
	}

	return ret;
}

/**
 * @brief Adds VLAN ID. This function is invoked by upper
 * layer when a new VLAN id is registered. This function updates the HW
 * filter with new VLAN id. New vlan id can be added with vconfig -
 * vconfig add interface_name  vlan_id
 *
 * Algorithm:
 * 1) Check for hash or perfect filtering.
 * 2) invoke osi call accordingly.
 *
 * @param[in] ndev: Network device structure
 * @param[in] vlan_proto: VLAN proto VLAN_PROTO_8021Q = 0 VLAN_PROTO_8021AD = 1
 * @param[in] vid: VLAN ID.
 *
 * @note Ethernet interface should be up
 *
 * @retval 0 on success
 * @retval "negative value" on failure.
 */
static int ether_vlan_rx_add_vid(struct net_device *ndev, __be16 vlan_proto,
				 u16 vid)
{
	struct ether_priv_data *pdata = netdev_priv(ndev);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	int ret = -1;

	if (pdata->vlan_hash_filtering == OSI_HASH_FILTER_MODE) {
		dev_err(pdata->dev,
			"HASH FILTERING for VLAN tag is not supported in SW\n");
	} else {
		ret = osi_update_vlan_id(osi_core, vid);
	}

	return ret;
}

/**
 * @brief Removes VLAN ID. This function is invoked by
 * upper layer when a new VALN id is removed. This function updates the
 * HW filter. vlan id can be removed with vconfig - 
 * vconfig rem interface_name vlan_id
 *
 * Algorithm:
 * 1) Check for hash or perfect filtering.
 * 2) invoke osi call accordingly.
 *
 * @param[in] ndev: Network device structure
 * @param[in] vlan_proto: VLAN proto VLAN_PROTO_8021Q = 0 VLAN_PROTO_8021AD = 1
 * @param[in] vid: VLAN ID.
 *
 * @note Ethernet interface should be up
 *
 * @retval 0 on success
 * @retval "negative value" on failure.
 */
static int ether_vlan_rx_kill_vid(struct net_device *ndev, __be16 vlan_proto,
				  u16 vid)
{
	struct ether_priv_data *pdata = netdev_priv(ndev);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	int ret = -1;

	if (!netif_running(ndev)) {
		return 0;
	}

	if (pdata->vlan_hash_filtering == OSI_HASH_FILTER_MODE) {
		dev_err(pdata->dev,
			"HASH FILTERING for VLAN tag is not supported in SW\n");
	} else {
		/* By default, receive only VLAN pkt with VID = 1 because
		 * writing 0 will pass all VLAN pkt
		 */
		ret = osi_update_vlan_id(osi_core, 0x1U);
	}

	return ret;
}
/**
 * @brief Ethernet network device operations
 */
static const struct net_device_ops ether_netdev_ops = {
	.ndo_open = ether_open,
	.ndo_stop = ether_close,
	.ndo_start_xmit = ether_start_xmit,
	.ndo_do_ioctl = ether_ioctl,
	.ndo_set_mac_address = ether_set_mac_addr,
	.ndo_change_mtu = ether_change_mtu,
	.ndo_select_queue = ether_select_queue,
	.ndo_set_features = ether_set_features,
	.ndo_set_rx_mode = ether_set_rx_mode,
	.ndo_vlan_rx_add_vid = ether_vlan_rx_add_vid,
	.ndo_vlan_rx_kill_vid = ether_vlan_rx_kill_vid,
};

/**
 * @brief NAPI poll handler for receive.
 *
 * Algorithm: Invokes OSI layer to read data from HW and pass onto the
 * Linux network stack.
 *
 * @param[in] napi: NAPI instance for Rx NAPI.
 * @param[in] budget: NAPI budget.
 *
 * @note  Probe and INIT needs to be completed successfully.
 *
 *@return number of packets received.
 */
static int ether_napi_poll_rx(struct napi_struct *napi, int budget)
{
	struct ether_rx_napi *rx_napi =
		container_of(napi, struct ether_rx_napi, napi);
	struct ether_priv_data *pdata = rx_napi->pdata;
	struct osi_dma_priv_data *osi_dma = pdata->osi_dma;
	unsigned int chan = rx_napi->chan;
	unsigned long flags;
	int received = 0;

	received = osi_process_rx_completions(osi_dma, chan, budget);
	if (received < budget) {
		napi_complete(napi);
		spin_lock_irqsave(&pdata->rlock, flags);
		osi_enable_chan_rx_intr(osi_dma, chan);
		spin_unlock_irqrestore(&pdata->rlock, flags);
	}

	return received;
}

/**
 * @brief NAPI poll handler for transmission.
 *
 * Algorithm: Invokes OSI layer to read data from HW and pass onto the
 * Linux network stack.
 *
 * @param[in] napi: NAPI instance for tx NAPI.
 * @param[in] budget: NAPI budget.
 *
 * @note  Probe and INIT needs to be completed successfully.
 *
 * @return Number of Tx buffer cleaned.
 */
static int ether_napi_poll_tx(struct napi_struct *napi, int budget)
{
	struct ether_tx_napi *tx_napi =
		container_of(napi, struct ether_tx_napi, napi);
	struct ether_priv_data *pdata = tx_napi->pdata;
	struct osi_dma_priv_data *osi_dma = pdata->osi_dma;
	unsigned int chan = tx_napi->chan;
	unsigned long flags;
	int processed;

	processed = osi_process_tx_completions(osi_dma, chan);
	if (processed == 0) {
		napi_complete(napi);
		spin_lock_irqsave(&pdata->rlock, flags);
		osi_enable_chan_tx_intr(osi_dma, chan);
		spin_unlock_irqrestore(&pdata->rlock, flags);
		return 0;
	}

	return budget;
}

/**
 * @brief Allocate NAPI resources.
 *
 * Algorithm: Allocate NAPI instances for the channels which are enabled.
 *
 * @param[in] pdata: OSD private data structure.
 *
 * @note Number of channels and channel numbers needs to be
 * updated in OSI private data structure.
 * 
 * @retval 0 on success
 * @retval "negative value" on failure.
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
 * @brief MII call back for MDIO register write.
 *
 * Algorithm: Invoke OSI layer for PHY register write.
 * phy_write() API from Linux PHY subsystem will call this.
 *
 * @param[in] bus: MDIO bus instances.
 * @param[in] phyaddr: PHY address (ID).
 * @param[in] phyreg: PHY register to write.
 * @param[in] phydata: Data to be written in register.
 *
 * @note  MAC has to be out of reset.
 *
 * @retval 0 on success
 * @retval "negative value" on failure.
 */
static int ether_mdio_write(struct mii_bus *bus, int phyaddr, int phyreg,
			    u16 phydata)
{
	struct net_device *ndev = bus->priv;
	struct ether_priv_data *pdata = netdev_priv(ndev);

	if (!pdata->clks_enable) {
		dev_err(pdata->dev,
			"%s:No clks available, skipping PHY write\n", __func__);
		return -ENODEV;
	}

	return osi_write_phy_reg(pdata->osi_core, (unsigned int)phyaddr,
				 (unsigned int)phyreg, phydata);
}

/**
 * @brief MII call back for MDIO register read.
 *
 * Algorithm: Invoke OSI layer for PHY register read.
 * phy_read() API from Linux subsystem will call this.
 *
 * @param[in] bus: MDIO bus instances.
 * @param[in] phyaddr: PHY address (ID).
 * @param[in] phyreg: PHY register to read.
 *
 * @note  MAC has to be out of reset.
 *
 * @retval data from PHY register on success
 * @retval "nagative value" on failure.
 */
static int ether_mdio_read(struct mii_bus *bus, int phyaddr, int phyreg)
{
	struct net_device *ndev = bus->priv;
	struct ether_priv_data *pdata = netdev_priv(ndev);

	if (!pdata->clks_enable) {
		dev_err(pdata->dev,
			"%s:No clks available, skipping PHY read\n", __func__);
		return -ENODEV;
	}

	return osi_read_phy_reg(pdata->osi_core, (unsigned int)phyaddr,
				(unsigned int)phyreg);
}

/**
 * @brief MDIO bus registration.
 *
 * Algorithm: Registers MDIO bus if there is mdio sub DT node
 * as part of MAC DT node.
 *
 * @param[in] pdata: OSD private data.
 *
 * @retval 0 on success
 * @retval "negative value" on failure.
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
 * @brief Read IRQ numbers from DT.
 *
 * Algorithm: Reads the IRQ numbers from DT based on number of channels.
 *
 * @param[in] pdev: Platform device associated with driver.
 * @param[in] pdata: OSD private data.
 * @param[in] num_chans: Number of channels.
 *
 * @retval 0 on success
 * @retval "negative value" on failure.
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
 * @brief Get MAC address from DT
 *
 * Algorithm: Populates MAC address by reading DT node.
 *
 * @param[in] node_name: Device tree node name.
 * @param[in] property_name: DT property name inside DT node.
 * @param[in] mac_addr: MAC address.
 *
 * @note Bootloader needs to updates chosen DT node with MAC
 * address.
 *
 * @retval 0 on success
 * @retval "negative value" on failure.
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
 * @brief Get MAC address from DT
 *
 * Algorithm: Populates MAC address by reading DT node.
 *
 * @param[in] pdata: OSD private data.
 *
 * @note Bootloader needs to updates chosen DT node with MAC address.
 *
 * @retval 0 on success
 * @retval "negative value" on failure.
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
 * @brief Put back MAC related clocks.
 *
 * Algorithm: Put back or release the MAC related clocks.
 *
 * @param[in] pdata: OSD private data.
 */
static inline void ether_put_clks(struct ether_priv_data *pdata)
{
	struct device *dev = pdata->dev;

	if (!IS_ERR_OR_NULL(pdata->tx_clk)) {
		devm_clk_put(dev, pdata->tx_clk);
	}
	if (!IS_ERR_OR_NULL(pdata->ptp_ref_clk)) {
		devm_clk_put(dev, pdata->ptp_ref_clk);
	}
	if (!IS_ERR_OR_NULL(pdata->rx_clk)) {
		devm_clk_put(dev, pdata->rx_clk);
	}
	if (!IS_ERR_OR_NULL(pdata->axi_clk)) {
		devm_clk_put(dev, pdata->axi_clk);
	}
	if (!IS_ERR_OR_NULL(pdata->axi_cbb_clk)) {
		devm_clk_put(dev, pdata->axi_cbb_clk);
	}
	if (!IS_ERR_OR_NULL(pdata->pllrefe_clk)) {
		devm_clk_put(dev, pdata->pllrefe_clk);
	}
}

/**
 * @brief Get MAC related clocks.
 *
 * Algorithm: Get the clocks from DT and stores in OSD private data.
 *
 * @param[in] pdata: OSD private data.
 *
 * @retval 0 on success
 * @retval "negative value" on failure.
 */
static int ether_get_clks(struct ether_priv_data *pdata)
{
	struct device *dev = pdata->dev;
	int ret;

	pdata->pllrefe_clk = devm_clk_get(dev, "pllrefe_vcoout");
	if (IS_ERR(pdata->pllrefe_clk)) {
		dev_info(dev, "failed to get pllrefe_vcoout clk\n");
		return PTR_ERR(pdata->pllrefe_clk);
	}

	pdata->axi_cbb_clk = devm_clk_get(dev, "axi_cbb");
	if (IS_ERR(pdata->axi_cbb_clk)) {
		ret = PTR_ERR(pdata->axi_cbb_clk);
		dev_err(dev, "failed to get axi_cbb clk\n");
		goto err_axi_cbb;
	}

	pdata->axi_clk = devm_clk_get(dev, "eqos_axi");
	if (IS_ERR(pdata->axi_clk)) {
		ret = PTR_ERR(pdata->axi_clk);
		dev_err(dev, "failed to get eqos_axi clk\n");
		goto err_axi;
	}

	pdata->rx_clk = devm_clk_get(dev, "eqos_rx");
	if (IS_ERR(pdata->rx_clk)) {
		ret = PTR_ERR(pdata->rx_clk);
		dev_err(dev, "failed to get eqos_rx clk\n");
		goto err_rx;
	}

	pdata->ptp_ref_clk = devm_clk_get(dev, "eqos_ptp_ref");
	if (IS_ERR(pdata->ptp_ref_clk)) {
		ret = PTR_ERR(pdata->ptp_ref_clk);
		dev_err(dev, "failed to get eqos_ptp_ref clk\n");
		goto err_ptp_ref;
	}

	pdata->tx_clk = devm_clk_get(dev, "eqos_tx");
	if (IS_ERR(pdata->tx_clk)) {
		ret = PTR_ERR(pdata->tx_clk);
		dev_err(dev, "failed to get eqos_tx clk\n");
		goto err_tx;
	}

	return 0;

err_tx:
	devm_clk_put(dev, pdata->ptp_ref_clk);
err_ptp_ref:
	devm_clk_put(dev, pdata->rx_clk);
err_rx:
	devm_clk_put(dev, pdata->axi_clk);
err_axi:
	devm_clk_put(dev, pdata->axi_cbb_clk);
err_axi_cbb:
	devm_clk_put(dev, pdata->pllrefe_clk);

	return ret;
}

/**
 * @brief Get Reset and MAC related clocks.
 *
 * Algorithm: Get the resets and MAC related clocks from DT and stores in
 * OSD private data. It also sets MDC clock rate by invoking OSI layer
 * with osi_set_mdc_clk_rate().
 *
 * @param[in] pdev: Platform device.
 * @param[in] pdata: OSD private data.
 *
 * @retval 0 on success
 * @retval "negative value" on failure.
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
		goto err_get_clks;
	}

	/* set PTP clock rate*/
	ret = clk_set_rate(pdata->ptp_ref_clk, pdata->ptp_ref_clock_speed);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to set ptp clk rate\n");
		goto err_set_ptp_rate;
	} else {
		osi_core->ptp_config.ptp_ref_clk_rate = pdata->ptp_ref_clock_speed;
	}

	ret = ether_enable_clks(pdata);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to enable clks\n");
		goto err_enable_clks;
	}

	if (pdata->mac_rst) {
		ret = reset_control_reset(pdata->mac_rst);
		if (ret < 0) {
			dev_err(&pdev->dev, "failed to reset MAC HW\n");
			goto err_rst;
		}
	}

	csr_clk_rate = clk_get_rate(pdata->axi_cbb_clk);
	osi_set_mdc_clk_rate(pdata->osi_core, csr_clk_rate);

	return ret;

err_rst:
	ether_disable_clks(pdata);
err_enable_clks:
err_set_ptp_rate:
	ether_put_clks(pdata);
err_get_clks:
	if (gpio_is_valid(pdata->phy_reset)) {
		gpio_set_value(pdata->phy_reset, OSI_DISABLE);
	}
exit:
	return ret;
}

/**
 * @brief Get platform resources
 *
 * Algorithm: Populates base address, clks, reset and MAC address.
 *
 * @param[in] pdev: Platform device associated with platform driver.
 * @param[in] pdata: OSD private data.
 *
 * @retval 0 on success
 * @retval "negative value" on failure.
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
	ether_put_clks(pdata);
	if (gpio_is_valid(pdata->phy_reset)) {
		gpio_set_value(pdata->phy_reset, OSI_DISABLE);
	}
rst_clk_fail:
	return ret;
}

/**
 * @brief Parse PHY DT.
 *
 * Algorithm: Reads PHY DT. Updates required data.
 *
 * @param[in] pdata: OS dependent private data structure.
 * @param[in] node: DT node entry of MAC
 *
 * @retval 0 on success
 * @retval "negative value" on failure.
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
 * @brief Parse queue priority DT.
 *
 * Algorithm: Reads queue priority form DT. Updates
 * data either by DT values or by default value.
 *
 * @param[in] pdata: OS dependent private data structure.
 * @param[in] pdt_prop: name of property
 * @param[in] pval: structure pointer where value will be filed
 * @param[in] val_def: default value if DT entry not reset
 * @param[in] val_max: max value supported
 * @param[in] num_entries: number of entries to be read form DT
 *
 * @note All queue priorities should be different from DT.
 */
static void ether_parse_queue_prio(struct ether_priv_data *pdata,
				   const char *pdt_prop,
				   unsigned int *pval, unsigned int val_def,
				   unsigned int val_max,
				   unsigned int num_entries)
{
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	struct device_node *pnode = pdata->dev->of_node;
	unsigned int i, pmask = 0x0U;
	unsigned int mtlq;
	unsigned int tval[OSI_EQOS_MAX_NUM_QUEUES];
	int ret = 0;

	ret = of_property_read_u32_array(pnode, pdt_prop, pval, num_entries);
	if (ret < 0) {
		dev_info(pdata->dev, "%s(): \"%s\" read failed %d."
			 "Using default\n", __func__, pdt_prop, ret);
		for (i = 0; i < num_entries; i++) {
			pval[i] = val_def;
		}

		return;
	}

	for (i = 0; i < num_entries; i++) {
		tval[i] = pval[i];
	}

	/* If Some priority is already given to queue or priority in DT is
	 * more than MAX priority, assign default priority to queue with
	 * error message
	 */
	for (i = 0; i < num_entries; i++) {
		mtlq = osi_core->mtl_queues[i];
		if ((tval[i] > val_max) || ((pmask & (1U << tval[i])) != 0U)) {
			dev_err(pdata->dev, "%s():Wrong or duplicate priority"
				" in DT entry for Q(%d)\n", __func__, mtlq);
			pval[mtlq] = val_def;
			continue;
		}
		pval[mtlq] = tval[i];
		pmask |= 1U << tval[i];
	}
}

/**
 * @brief Parse MAC and PHY DT.
 *
 * Algorithm: Reads MAC and PHY DT. Updates required data.
 *
 * @param[in] pdata: OS dependent private data structure.
 *
 * @retval 0 on success
 * @retval "negative value" on failure.
 */
static int ether_parse_dt(struct ether_priv_data *pdata)
{
	struct device *dev = pdata->dev;
	struct platform_device *pdev = to_platform_device(dev);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	struct osi_dma_priv_data *osi_dma = pdata->osi_dma;
	unsigned int tmp_value[OSI_EQOS_MAX_NUM_QUEUES];
	struct device_node *np = dev->of_node;
	int ret = -EINVAL;
	unsigned int i, mtlq;

	/* read ptp clock */
	ret = of_property_read_u32(np, "nvidia,ptp_ref_clock_speed",
				   &pdata->ptp_ref_clock_speed);
	if (ret != 0) {
		dev_err(dev, "setting default PTP clk rate as 312.5MHz\n");
		pdata->ptp_ref_clock_speed = ETHER_DFLT_PTP_CLK;
	}
	/* Read Pause frame feature support */
	ret = of_property_read_u32(np, "nvidia,pause_frames",
				   &pdata->osi_core->pause_frames);
	if (ret < 0) {
		dev_info(dev, "Failed to read nvida,pause_frames, so"
			 " setting to default support as disable\n");
		pdata->osi_core->pause_frames = OSI_PAUSE_FRAMES_DISABLE;
	}

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

	if (osi_dma->num_dma_chans != osi_core->num_mtl_queues) {
		dev_err(dev, "mismatch in numbers of DMA channel and MTL Q\n");
		return -EINVAL;
	}

	ret = -1;
	for (i = 0; i < osi_dma->num_dma_chans; i++) {
		if (osi_dma->dma_chans[i] != osi_core->mtl_queues[i]) {
			dev_err(dev,
				"mismatch in DMA channel and MTL Q number at index %d\n",
				i);
			return -EINVAL;
		}
		if (osi_dma->dma_chans[i] == 0) {
			ret = 0;
		}
	}

	if (ret != 0) {
		dev_err(dev, "Q0 Must be enabled for rx path\n");
		return -EINVAL;
	}

	ret = of_property_read_u32_array(np, "nvidia,rxq_enable_ctrl",
					 tmp_value,
					 osi_core->num_mtl_queues);
	if (ret < 0) {
		dev_err(dev, "failed to read rxq enable ctrl\n");
		return ret;
	} else {
		for (i = 0; i < osi_core->num_mtl_queues; i++) {
			mtlq = osi_core->mtl_queues[i];
			osi_core->rxq_ctrl[mtlq] = tmp_value[i];
		}
	}

	/* Read tx queue priority */
	ether_parse_queue_prio(pdata, "nvidia,tx-queue-prio", pdata->txq_prio,
			       ETHER_QUEUE_PRIO_DEFAULT, ETHER_QUEUE_PRIO_MAX,
			       osi_core->num_mtl_queues);

	/* Read Rx Queue - User priority mapping for tagged packets */
	ret = of_property_read_u32_array(np, "nvidia,rx-queue-prio",
					 tmp_value,
					 osi_core->num_mtl_queues);
	if (ret < 0) {
		dev_info(dev, "failed to read rx Queue priority mapping, Setting default 0x0\n");
		for (i = 0; i < osi_core->num_mtl_queues; i++) {
			osi_core->rxq_prio[i] = 0x0U;
		}
	} else {
		for (i = 0; i < osi_core->num_mtl_queues; i++) {
			mtlq = osi_core->mtl_queues[i];
			osi_core->rxq_prio[mtlq] = tmp_value[i];
		}
	}

	/* Read DCS enable/disable input, default disable */
	ret = of_property_read_u32(np, "nvidia,dcs-enable", &osi_core->dcs_en);
	if (ret < 0 || osi_core->dcs_en != OSI_ENABLE) {
		osi_core->dcs_en = OSI_DISABLE;
	}

	/* Read MAX MTU size supported */
	ret = of_property_read_u32(np, "nvidia,max-platform-mtu",
				   &pdata->max_platform_mtu);
	if (ret < 0) {
		dev_info(dev, "max-platform-mtu DT entry missing, setting default %d\n",
			 ETHER_DEFAULT_PLATFORM_MTU);
		pdata->max_platform_mtu = ETHER_DEFAULT_PLATFORM_MTU;
	} else {
		if (pdata->max_platform_mtu > ETHER_MAX_HW_MTU ||
		    pdata->max_platform_mtu < ETH_MIN_MTU) {
			dev_info(dev, "Invalid max-platform-mtu, setting default %d\n",
				 ETHER_DEFAULT_PLATFORM_MTU);
			pdata->max_platform_mtu = ETHER_DEFAULT_PLATFORM_MTU;
		}
	}

	/* RIWT value to be set */
	ret = of_property_read_u32(np, "nvidia,rx_riwt", &osi_dma->rx_riwt);
	if (ret < 0) {
		osi_dma->use_riwt = OSI_DISABLE;
	} else {
		if ((osi_dma->rx_riwt > OSI_MAX_RX_COALESCE_USEC) ||
		    (osi_dma->rx_riwt < OSI_MIN_RX_COALESCE_USEC)) {
			dev_err(dev,
				"invalid rx_riwt, must be inrange %d to %d\n",
				OSI_MIN_RX_COALESCE_USEC,
				OSI_MAX_RX_COALESCE_USEC);
			return -EINVAL;
		}
		osi_dma->use_riwt = OSI_ENABLE;
	}
	/* Enable VLAN strip by default */
	osi_core->strip_vlan_tag = OSI_ENABLE;

	ret = ether_parse_phy_dt(pdata, np);
	if (ret < 0) {
		dev_err(dev, "failed to parse PHY DT\n");
		goto exit;
	}

exit:
	return ret;
}

/**
 * @brief Populate number of MTL and DMA channels.
 *
 * Algorithm:
 * 1) Updates MAC HW type based on DT compatible property.
 * 2) Read number of channels from DT.
 * 3) Updates number of channels based on min and max number of channels
 *
 * @param[in] pdev: Platform device
 * @param[in] num_dma_chans: Number of channels
 * @param[in] mac: MAC type based on compatible property
 * @param[in] num_mtl_queues: Number of MTL queues.
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
 * @brief Set DMA address mask.
 *
 * Algorithm:
 * Based on the addressing capability (address bit length) supported in the HW,
 * the dma mask is set accordingly.
 *
 * @param[in] pdata: OS dependent private data structure.
 *
 * @note MAC_HW_Feature1 register need to read and store the value of ADDR64.
 *
 * @retval 0 on success
 * @retval "negative value" on failure.
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
 * @brief Set the network device feature flags
 *
 * Algorithm:
 * 1) Check the HW features supported
 * 2) Enable corresponding feature flag so that network subsystem of OS
 * is aware of device capabilities.
 * 3) Update current enable/disable state of features currently enabled
 * 
 * @param[in] ndev: Network device instance
 * @param[in] pdata: OS dependent private data structure.
 *
 * @note Netdev allocated and HW features are already parsed.
 *
 */
static void ether_set_ndev_features(struct net_device *ndev,
				    struct ether_priv_data *pdata)
{
	netdev_features_t features = 0;

	if (pdata->hw_feat.tso_en) {
		features |= NETIF_F_TSO;
		features |= NETIF_F_SG;
	}

	if (pdata->hw_feat.tx_coe_sel) {
		features |= NETIF_F_IP_CSUM;
		features |= NETIF_F_IPV6_CSUM;
	}

	if (pdata->hw_feat.rx_coe_sel) {
		features |= NETIF_F_RXCSUM;
	}

	/* GRO is independent of HW features */
	features |= NETIF_F_GRO;

	if (pdata->hw_feat.sa_vlan_ins) {
		features |= NETIF_F_HW_VLAN_CTAG_TX;
	}

	/* Rx VLAN tag stripping/filtering enabled by default */
	features |= NETIF_F_HW_VLAN_CTAG_RX;
	features |= NETIF_F_HW_VLAN_CTAG_FILTER;

	/* Features available in HW */
	ndev->hw_features = features;
	/* Features that can be changed by user */
	ndev->features = features;
	/* Features that can be inherited by vlan devices */
	ndev->vlan_features = features;

	/* Set current state of features enabled by default in HW */
	pdata->hw_feat_cur_state = features;
}

/**
 * @brief Static function to initialize filter register count
 * in private data structure
 *
 * Algorithm: Updates addr_reg_cnt based on HW feature
 *
 * @param[in] pdata: ethernet private data structure
 *
 * @note MAC_HW_Feature1 register need to read and store the value of ADDR64.
 */
static void init_filter_values(struct ether_priv_data *pdata)
{
	if (pdata->hw_feat.mac_addr64_sel == OSI_ENABLE) {
		pdata->num_mac_addr_regs = ETHER_ADDR_REG_CNT_128;
	} else if (pdata->hw_feat.mac_addr32_sel == OSI_ENABLE) {
		pdata->num_mac_addr_regs = ETHER_ADDR_REG_CNT_64;
	} else if (pdata->hw_feat.mac_addr16_sel == OSI_ENABLE) {
		pdata->num_mac_addr_regs = ETHER_ADDR_REG_CNT_32;
	} else {
		pdata->num_mac_addr_regs = ETHER_ADDR_REG_CNT_1;
	}
}

/**
 * @brief Ethernet platform driver probe.
 *
 * Algorithm:
 * 1) Get the number of channels from DT.
 * 2) Allocate the network device for those many channels.
 * 3) Parse MAC and PHY DT.
 * 4) Get all required clks/reset/IRQ's.
 * 5) Register MDIO bus and network device.
 * 6) Initialize spinlock.
 * 7) Update filter value based on HW feature.
 * 8) Initialize Workqueue to read MMC counters periodically.
 *
 * @param[in] pdev: platform device associated with platform driver.
 *
 * @retval 0 on success
 * @retval "negative value" on failure
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

	/* Initialize core and DMA ops based on MAC type */
	if (osi_init_core_ops(osi_core) != 0) {
		dev_err(&pdev->dev, "failed to get osi_init_core_ops\n");
		goto err_core_ops;
	}

	if (osi_init_dma_ops(osi_dma) != 0) {
		dev_err(&pdev->dev, "failed to get osi_init_dma_ops\n");
		goto err_dma_ops;
	}

	/* Parse the ethernet DT node */
	ret = ether_parse_dt(pdata);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to parse DT\n");
		goto err_parse_dt;
	}

	ndev->max_mtu = pdata->max_platform_mtu;

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

	/* Set netdev features based on hw features */
	ether_set_ndev_features(ndev, pdata);

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

	spin_lock_init(&pdata->rlock);
	init_filter_values(pdata);
	/* Disable Clocks */
	ether_disable_clks(pdata);

	dev_info(&pdev->dev,
		 "%s (HW ver: %02x) created with %u DMA channels\n",
		 netdev_name(ndev), osi_core->mac_ver, num_dma_chans);

	if (gpio_is_valid(pdata->phy_reset)) {
		gpio_set_value(pdata->phy_reset, OSI_DISABLE);
	}
	/* Initialization of delayed workqueue */
	INIT_DELAYED_WORK(&pdata->ether_stats_work, ether_stats_work_func);

	return 0;

err_netdev:
	ether_sysfs_unregister(pdata->dev);
err_sysfs:
err_napi:
	mdiobus_unregister(pdata->mii);
err_dma_mask:
	ether_disable_clks(pdata);
	ether_put_clks(pdata);
	if (gpio_is_valid(pdata->phy_reset)) {
		gpio_set_value(pdata->phy_reset, OSI_DISABLE);
	}
err_init_res:
err_parse_dt:
err_core_ops:
err_dma_ops:
	free_netdev(ndev);
	return ret;
}

/**
 * @brief Ethernet platform driver remove.
 *
 * Alogorithm: Release all the resources
 *
 * @param[in] pdev: Platform device associated with platform driver.
 *
 * @retval 0 on success
 * @retval "negative value" on failure.
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

	ether_put_clks(pdata);

	/* Assert MAC RST gpio */
	if (pdata->mac_rst) {
		reset_control_assert(pdata->mac_rst);
	}
	free_netdev(ndev);

	return 0;
}

#ifdef CONFIG_PM
static int ether_suspend_noirq(struct device *dev)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct ether_priv_data *pdata = netdev_priv(ndev);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	struct osi_dma_priv_data *osi_dma = pdata->osi_dma;
	unsigned int i = 0, chan = 0;

	if (!netif_running(ndev))
		return 0;

	/* Stop workqueue while DUT is going to suspend state */
	ether_stats_work_queue_stop(pdata);

	if (pdata->phydev) {
		phy_stop(pdata->phydev);
		if (gpio_is_valid(pdata->phy_reset))
			gpio_set_value(pdata->phy_reset, 0);
	}

	netif_tx_disable(ndev);
	ether_napi_disable(pdata);

	osi_hw_dma_deinit(osi_dma);
	osi_hw_core_deinit(osi_core);

	for (i = 0; i < osi_dma->num_dma_chans; i++) {
		chan = osi_dma->dma_chans[i];
		osi_disable_chan_tx_intr(osi_dma, chan);
		osi_disable_chan_rx_intr(osi_dma, chan);
	}

	free_dma_resources(osi_dma, dev);

	/* disable MAC clocks */
	ether_disable_clks(pdata);

	return 0;
}

static int ether_resume(struct ether_priv_data *pdata)
{
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	struct osi_dma_priv_data *osi_dma = pdata->osi_dma;
	struct device *dev = pdata->dev;
	struct net_device *ndev = pdata->ndev;
	int ret = 0;

	if (pdata->mac_rst) {
		ret = reset_control_reset(pdata->mac_rst);
		if (ret < 0) {
			dev_err(dev, "failed to reset mac hw\n");
			return -1;
		}
	}

	ret = osi_poll_for_swr(osi_core);
	if (ret < 0) {
		dev_err(dev, "failed to poll mac software reset\n");
		return ret;
	}

	ret = osi_pad_calibrate(osi_core);
	if (ret < 0) {
		dev_err(dev, "failed to do pad caliberation\n");
		return ret;
	}

	osi_set_rx_buf_len(osi_dma);

	ret = ether_allocate_dma_resources(pdata);
	if (ret < 0) {
		dev_err(dev, "failed to allocate dma resources\n");
		return ret;
	}

	/* initialize mac/mtl/dma common registers */
	ret = osi_hw_core_init(osi_core,
			       pdata->hw_feat.tx_fifo_size,
			       pdata->hw_feat.rx_fifo_size);
	if (ret < 0) {
		dev_err(dev,
			"%s: failed to initialize mac hw core with reason %d\n",
			__func__, ret);
		goto err_core;
	}

	/* dma init */
	ret = osi_hw_dma_init(osi_dma);
	if (ret < 0) {
		dev_err(dev,
			"%s: failed to initialize mac hw dma with reason %d\n",
			__func__, ret);
		goto err_dma;
	}

	/* enable NAPI */
	ether_napi_enable(pdata);
	/* start the mac */
	osi_start_mac(osi_core);
	/* start phy */
	phy_start(pdata->phydev);
	/* start network queues */
	netif_tx_start_all_queues(ndev);
	/* re-start workqueue */
	ether_stats_work_queue_start(pdata);

	return 0;

err_dma:
	osi_hw_core_deinit(osi_core);
err_core:
	free_dma_resources(osi_dma, dev);

	return ret;
}

static int ether_resume_noirq(struct device *dev)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct ether_priv_data *pdata = netdev_priv(ndev);
	int ret = 0;

	if (!netif_running(ndev))
		return 0;

	ether_enable_clks(pdata);

	if (gpio_is_valid(pdata->phy_reset) &&
	    !gpio_get_value(pdata->phy_reset)) {
		/* deassert phy reset */
		gpio_set_value(pdata->phy_reset, 1);
	}

	ret = ether_resume(pdata);
	if (ret < 0) {
		dev_err(dev, "failed to resume the MAC\n");
		return ret;
	}

	return 0;
}

static const struct dev_pm_ops ether_pm_ops = {
	.suspend_noirq = ether_suspend_noirq,
	.resume_noirq = ether_resume_noirq,
};
#endif

/**
 * @brief Ethernet device tree compatible match name
 */
static const struct of_device_id ether_of_match[] = {
	{ .compatible = "nvidia,nveqos" },
	{},
};
MODULE_DEVICE_TABLE(of, ether_of_match);

/**
 * @brief Ethernet platform driver instance
 */
static struct platform_driver ether_driver = {
	.probe = ether_probe,
	.remove = ether_remove,
	.driver = {
		.name = "nvethernet",
		.of_match_table = ether_of_match,
#ifdef CONFIG_PM
		.pm = &ether_pm_ops,
#endif
	},
};

module_platform_driver(ether_driver);
MODULE_LICENSE("GPL v2");
