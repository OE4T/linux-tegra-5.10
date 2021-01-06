/*
 * Copyright (c) 2018-2021, NVIDIA CORPORATION. All rights reserved.
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

#include "osi_dma_local.h"
#include <osd.h>
#include <local_common.h>
#include "hw_desc.h"

nve32_t osi_init_dma_ops(struct osi_dma_priv_data *osi_dma)
{
	if (osi_dma == OSI_NULL) {
		return -1;
	}
	/*
	 * Currently these osd_ops are optional to be filled in the OSD layer.
	 * If OSD updates these pointers, use the same. If not, fall back to the
	 * existing way of using osd_* API's.
	 * TODO: Once These API's are mandatory, return errors instead of
	 * default API usage.
	 */
	if (osi_dma->osd_ops.transmit_complete == OSI_NULL) {
		osi_dma->osd_ops.transmit_complete = osd_transmit_complete;
	}
	if (osi_dma->osd_ops.receive_packet == OSI_NULL) {
		osi_dma->osd_ops.receive_packet = osd_receive_packet;
	}
	if (osi_dma->osd_ops.ops_log == OSI_NULL) {
		osi_dma->osd_ops.ops_log = osd_log;
	}
	if (osi_dma->osd_ops.udelay == OSI_NULL) {
		osi_dma->osd_ops.udelay = osd_udelay;
	}

	if (osi_dma->mac == OSI_MAC_HW_EQOS) {
		/* Get EQOS HW ops */
		osi_dma->ops = eqos_get_dma_chan_ops();
		/* Explicitly set osi_dma->safety_config = OSI_NULL if
		 * a particular MAC version does not need SW safety mechanisms
		 * like periodic read-verify.
		 */
		osi_dma->safety_config = (void *)eqos_get_dma_safety_config();
		return 0;
	}

	OSI_DMA_ERR(OSI_NULL, OSI_LOG_ARG_INVALID, "dma: Invalid argument\n",
		    0ULL);
	return -1;
}

nve32_t osi_hw_dma_init(struct osi_dma_priv_data *osi_dma)
{
	nveu32_t i, chan;
	nve32_t ret = -1;

	if ((osi_dma != OSI_NULL) && (osi_dma->ops != OSI_NULL) &&
	    (osi_dma->base != OSI_NULL) &&
	    (osi_dma->ops->init_dma_channel != OSI_NULL) &&
	    (osi_dma->num_dma_chans <= OSI_EQOS_MAX_NUM_CHANS)) {
		ret = osi_dma->ops->init_dma_channel(osi_dma);
		if (ret < 0) {
			OSI_DMA_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
				    "dma: init dma channel failed\n", 0ULL);
			return ret;
		}
	} else {
		return ret;
	}

	ret = dma_desc_init(osi_dma);
	if (ret != 0) {
		return ret;
	}

	/* Enable channel interrupts at wrapper level and start DMA */
	for (i = 0; i < osi_dma->num_dma_chans; i++) {
		chan = osi_dma->dma_chans[i];

		ret = osi_enable_chan_tx_intr(osi_dma, chan);
		if (ret != 0) {
			OSI_DMA_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
				    "dma: enable tx intr failed\n", 0ULL);
			return ret;
		}

		ret = osi_enable_chan_rx_intr(osi_dma, chan);
		if (ret != 0) {
			OSI_DMA_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
				    "dma: enable rx intr failed\n", 0ULL);
			return ret;
		}

		ret = osi_start_dma(osi_dma, chan);
		if (ret != 0) {
			OSI_DMA_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
				    "dma: start dma failed\n", 0ULL);
			return ret;
		}
	}

	return ret;
}

nve32_t  osi_hw_dma_deinit(struct osi_dma_priv_data *osi_dma)
{
	nveu32_t i;
	nve32_t ret = 0;

	if ((osi_dma == OSI_NULL) ||
	    (osi_dma->num_dma_chans > OSI_EQOS_MAX_NUM_CHANS)) {
		return -1;
	}

	for (i = 0; i < osi_dma->num_dma_chans; i++) {
		ret = osi_stop_dma(osi_dma, osi_dma->dma_chans[i]);
		if (ret != 0) {
			return ret;
		}
	}

	return ret;
}

nve32_t osi_disable_chan_tx_intr(struct osi_dma_priv_data *osi_dma,
				 nveu32_t chan)
{
	if ((osi_dma != OSI_NULL) && (osi_dma->ops != OSI_NULL) &&
	    (osi_dma->base != OSI_NULL) && (chan < OSI_EQOS_MAX_NUM_CHANS) &&
	    (osi_dma->ops->disable_chan_tx_intr != OSI_NULL)) {
		osi_dma->ops->disable_chan_tx_intr(osi_dma->base, chan);
		return 0;
	}

	return -1;
}

nve32_t osi_enable_chan_tx_intr(struct osi_dma_priv_data *osi_dma,
				nveu32_t chan)
{
	if ((osi_dma != OSI_NULL) && (osi_dma->ops != OSI_NULL) &&
	    (osi_dma->base != OSI_NULL) && (chan < OSI_EQOS_MAX_NUM_CHANS) &&
	    (osi_dma->ops->enable_chan_tx_intr != OSI_NULL)) {
		osi_dma->ops->enable_chan_tx_intr(osi_dma->base, chan);
		return 0;
	}

	return -1;
}

nve32_t osi_disable_chan_rx_intr(struct osi_dma_priv_data *osi_dma,
				 nveu32_t chan)
{
	if ((osi_dma != OSI_NULL) && (osi_dma->ops != OSI_NULL) &&
	    (osi_dma->base != OSI_NULL) && (chan < OSI_EQOS_MAX_NUM_CHANS) &&
	    (osi_dma->ops->disable_chan_rx_intr != OSI_NULL)) {
		osi_dma->ops->disable_chan_rx_intr(osi_dma->base, chan);
		return 0;
	}

	return -1;
}

nve32_t osi_enable_chan_rx_intr(struct osi_dma_priv_data *osi_dma,
				nveu32_t chan)
{
	if ((osi_dma != OSI_NULL) && (osi_dma->ops != OSI_NULL) &&
	    (osi_dma->base != OSI_NULL) && (chan < OSI_EQOS_MAX_NUM_CHANS) &&
	    (osi_dma->ops->enable_chan_rx_intr != OSI_NULL)) {
		osi_dma->ops->enable_chan_rx_intr(osi_dma->base, chan);
		return 0;
	}

	return -1;
}

nve32_t osi_clear_vm_tx_intr(struct osi_dma_priv_data *osi_dma,
			     nveu32_t chan)
{
	if ((osi_dma != OSI_NULL) && (osi_dma->ops != OSI_NULL) &&
	    (osi_dma->ops->clear_vm_tx_intr != OSI_NULL)) {
		osi_dma->ops->clear_vm_tx_intr(osi_dma->base, chan);
		return 0;
	}

	return -1;
}

nve32_t osi_clear_vm_rx_intr(struct osi_dma_priv_data *osi_dma,
			     nveu32_t chan)
{
	if ((osi_dma != OSI_NULL) && (osi_dma->ops != OSI_NULL) &&
	    (osi_dma->ops->clear_vm_rx_intr != OSI_NULL)) {
		osi_dma->ops->clear_vm_rx_intr(osi_dma->base, chan);
		return 0;
	}

	return -1;
}

nveu32_t osi_get_global_dma_status(struct osi_dma_priv_data *osi_dma)
{
	if ((osi_dma != OSI_NULL) && (osi_dma->ops != OSI_NULL) &&
	    (osi_dma->ops->get_global_dma_status != OSI_NULL)) {
		return osi_dma->ops->get_global_dma_status(osi_dma->base);
	}

	return 0;
}

nve32_t  osi_start_dma(struct osi_dma_priv_data *osi_dma,
		       nveu32_t chan)
{
	if ((osi_dma != OSI_NULL) && (osi_dma->ops != OSI_NULL) &&
	    (osi_dma->base != OSI_NULL) && (chan < OSI_EQOS_MAX_NUM_CHANS) &&
	    (osi_dma->ops->start_dma != OSI_NULL)) {
		osi_dma->ops->start_dma(osi_dma, chan);
		return 0;
	}

	return -1;
}

nve32_t osi_stop_dma(struct osi_dma_priv_data *osi_dma,
		     nveu32_t chan)
{
	if ((osi_dma != OSI_NULL) && (osi_dma->ops != OSI_NULL) &&
	    (osi_dma->base != OSI_NULL) && (chan < OSI_EQOS_MAX_NUM_CHANS) &&
	    (osi_dma->ops->stop_dma != OSI_NULL)) {
		osi_dma->ops->stop_dma(osi_dma, chan);
		return 0;
	}

	return -1;
}

nveu32_t osi_get_refill_rx_desc_cnt(struct osi_rx_ring *rx_ring)
{
	if ((rx_ring == OSI_NULL) ||
	    (rx_ring->cur_rx_idx >= RX_DESC_CNT) ||
	    (rx_ring->refill_idx >= RX_DESC_CNT)) {
		return 0;
	}

	return (rx_ring->cur_rx_idx - rx_ring->refill_idx) & (RX_DESC_CNT - 1U);
}

/**
 * @brief rx_dma_desc_validate_args - DMA Rx descriptor init args Validate
 *
 * Algorithm: Validates DMA Rx descriptor init argments.
 *
 * @param[in] osi_dma: OSI DMA private data struture.
 * @param[in] rx_ring: HW ring corresponding to Rx DMA channel.
 * @param[in] chan: Rx DMA channel number
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static inline nve32_t rx_dma_desc_validate_args(
					    struct osi_dma_priv_data *osi_dma,
					    struct osi_rx_ring *rx_ring,
					    nveu32_t chan)
{
	/* Validate args */
	if (!((osi_dma != OSI_NULL) && (osi_dma->ops != OSI_NULL) &&
	      (osi_dma->ops->update_rx_tailptr != OSI_NULL))) {
		return -1;
	}

	if (!((rx_ring != OSI_NULL) && (rx_ring->rx_swcx != OSI_NULL) &&
	      (rx_ring->rx_desc != OSI_NULL))) {
		OSI_DMA_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
			    "dma: Invalid pointers\n", 0ULL);
		return -1;
	}

	if (chan >= OSI_EQOS_MAX_NUM_CHANS) {
		OSI_DMA_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
			    "dma: Invalid channel\n", 0ULL);
		return -1;
	}

	return 0;
}

/**
 * @brief rx_dma_handle_ioc - DMA Rx descriptor RWIT Handler
 *
 * Algorithm:
 * 1) Check RWIT enable and reset IOC bit
 * 2) Check rx_frames enable and update IOC bit
 *
 * @param[in] osi_dma: OSI DMA private data struture.
 * @param[in] rx_ring: HW ring corresponding to Rx DMA channel.
 * @param[in, out] rx_desc: Rx Rx descriptor.
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 *
 */
static inline void rx_dma_handle_ioc(struct osi_dma_priv_data *osi_dma,
				     struct osi_rx_ring *rx_ring,
				     struct osi_rx_desc *rx_desc)
{
	/* reset IOC bit if RWIT is enabled */
	if (osi_dma->use_riwt == OSI_ENABLE) {
		rx_desc->rdes3 &= ~RDES3_IOC;
		/* update IOC bit if rx_frames is enabled. Rx_frames
		 * can be enabled only along with RWIT.
		 */
		if (osi_dma->use_rx_frames == OSI_ENABLE) {
			if ((rx_ring->refill_idx %
			    osi_dma->rx_frames) == OSI_NONE) {
				rx_desc->rdes3 |= RDES3_IOC;
			}
		}
	}
}

nve32_t osi_rx_dma_desc_init(struct osi_dma_priv_data *osi_dma,
			     struct osi_rx_ring *rx_ring, nveu32_t chan)
{
	/* for CERT-C error */
	nveu64_t temp;
	nveu64_t tailptr = 0;
	struct osi_rx_swcx *rx_swcx = OSI_NULL;
	struct osi_rx_desc *rx_desc = OSI_NULL;

	if (rx_dma_desc_validate_args(osi_dma, rx_ring, chan) < 0) {
		/* Return on arguments validation failure */
		return -1;
	}

	/* Refill buffers */
	while ((rx_ring->refill_idx != rx_ring->cur_rx_idx) &&
	       (rx_ring->refill_idx < RX_DESC_CNT)) {
		rx_swcx = rx_ring->rx_swcx + rx_ring->refill_idx;
		rx_desc = rx_ring->rx_desc + rx_ring->refill_idx;

		if ((rx_swcx->flags & OSI_RX_SWCX_BUF_VALID) !=
		    OSI_RX_SWCX_BUF_VALID) {
			break;
		}

		rx_swcx->flags = 0;

		/* Populate the newly allocated buffer address */
		temp = L32(rx_swcx->buf_phy_addr);
		if (temp > UINT_MAX) {
			OSI_DMA_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
				    "dma: Invalid buf_phy_addr\n", 0ULL);
			/* error case do nothing */
		} else {
			/* Store Receive Descriptor 0 */
			rx_desc->rdes0 = (nveu32_t)temp;
		}

		temp = H32(rx_swcx->buf_phy_addr);
		if (temp <= UINT_MAX) {
			/* Store Receive Descriptor 1 */
			rx_desc->rdes1 = (nveu32_t)temp;
		} else {
			OSI_DMA_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
				    "dma: Invalid buf_phy_addr\n", 0ULL);
		}

		rx_desc->rdes2 = 0;
		rx_desc->rdes3 = (RDES3_OWN | RDES3_IOC | RDES3_B1V);

		/* Reset IOC bit if RWIT is enabled */
		rx_dma_handle_ioc(osi_dma, rx_ring, rx_desc);

		INCR_RX_DESC_INDEX(rx_ring->refill_idx, 1U);
	}

	/* Update the Rx tail ptr  whenever buffer is replenished to
	 * kick the Rx DMA to resume if it is in suspend. Always set
	 * Rx tailptr to 1 greater than last descriptor in the ring since HW
	 * knows to loop over to start of ring.
	 */
	tailptr = rx_ring->rx_desc_phy_addr +
		  (sizeof(struct osi_rx_desc) * (RX_DESC_CNT));

	if (osi_unlikely(tailptr < rx_ring->rx_desc_phy_addr)) {
		/* Will not hit this case, used for CERT-C compliance */
		OSI_DMA_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
			    "dma: Invalid tailptr\n", 0ULL);
		return -1;
	}

	osi_dma->ops->update_rx_tailptr(osi_dma->base, chan, tailptr);

	return 0;
}

nve32_t osi_set_rx_buf_len(struct osi_dma_priv_data *osi_dma)
{
	if ((osi_dma != OSI_NULL) && (osi_dma->ops != OSI_NULL) &&
	    (osi_dma->ops->set_rx_buf_len != OSI_NULL)) {
		osi_dma->ops->set_rx_buf_len(osi_dma);
	} else {
		return -1;
	}

	return 0;
}

nve32_t osi_dma_get_systime_from_mac(struct osi_dma_priv_data *const osi_dma,
				     nveu32_t *sec, nveu32_t *nsec)
{
	if ((osi_dma != OSI_NULL) && (osi_dma->base != OSI_NULL)) {
		common_get_systime_from_mac(osi_dma->base, osi_dma->mac, sec,
					    nsec);
	} else {
		return -1;
	}

	return 0;
}

nveu32_t osi_is_mac_enabled(struct osi_dma_priv_data *const osi_dma)
{
	if ((osi_dma != OSI_NULL) && (osi_dma->base != OSI_NULL)) {
		return common_is_mac_enabled(osi_dma->base, osi_dma->mac);
	} else {
		return OSI_DISABLE;
	}
}
#ifndef OSI_STRIPPED_LIB

/**
 * @brief osi_slot_args_validate - Validate slot function arguments
 *
 * @note
 * Algorithm:
 *  - Check set argument and return error.
 *  - Validate osi_dma structure pointers.
 *
 * @param[in] osi_dma: OSI DMA private data structure.
 * @param[in] set: Flag to set with OSI_ENABLE and reset with OSI_DISABLE
 *
 * @pre MAC should be init and started. see osi_start_mac()
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static inline nve32_t osi_slot_args_validate(struct osi_dma_priv_data *osi_dma,
					     nveu32_t set)
{
	if (osi_dma == OSI_NULL) {
		return -1;
	}

	/* return on invalid set argument */
	if ((set != OSI_ENABLE) && (set != OSI_DISABLE)) {
		OSI_DMA_ERR(osi_dma->osd, OSI_LOG_ARG_INVALID,
			    "dma: Invalid set argument\n", set);
		return -1;
	}

	/* NULL check for osi_dma, osi_dma->ops and osi_dma->ops->config_slot */
	if ((osi_dma->ops == OSI_NULL) ||
	    (osi_dma->ops->config_slot == OSI_NULL)) {
		OSI_DMA_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
			    "dma: Invalid set argument\n", 0ULL);
		return -1;
	}

	return 0;
}

nve32_t osi_config_slot_function(struct osi_dma_priv_data *osi_dma,
				 nveu32_t set)
{
	nveu32_t i = 0U, chan = 0U, interval = 0U;
	struct osi_tx_ring *tx_ring = OSI_NULL;

	/* Validate arguments */
	if (osi_slot_args_validate(osi_dma, set) < 0) {
		return -1;
	}

	for (i = 0; i < osi_dma->num_dma_chans; i++) {
		/* Get DMA channel and validate */
		chan = osi_dma->dma_chans[i];
		if ((chan == 0x0U) ||
		    (chan >= OSI_EQOS_MAX_NUM_CHANS)) {
			/* Ignore 0 and invalid channels */
			continue;
		}
		/* Check for slot enable */
		if (osi_dma->slot_enabled[chan] == OSI_ENABLE) {
			/* Get DMA slot interval and validate */
			interval = osi_dma->slot_interval[chan];
			if (interval > OSI_SLOT_INTVL_MAX) {
				OSI_DMA_ERR(osi_dma->osd,
					    OSI_LOG_ARG_INVALID,
					    "dma: Invalid interval arguments\n",
					    interval);
				return -1;
			}

			tx_ring = osi_dma->tx_ring[chan];
			if (tx_ring == OSI_NULL) {
				OSI_DMA_ERR(osi_dma->osd, OSI_LOG_ARG_INVALID,
					    "tx_ring is null\n", chan);
				return -1;
			}
			tx_ring->slot_check = set;
			osi_dma->ops->config_slot(osi_dma,
						  chan,
						  set,
						  interval);
		}
	}

	return 0;
}

nve32_t osi_validate_dma_regs(struct osi_dma_priv_data *osi_dma)
{
	nve32_t ret = -1;

	if ((osi_dma != OSI_NULL) && (osi_dma->ops != OSI_NULL) &&
	    (osi_dma->ops->validate_regs != OSI_NULL) &&
	    (osi_dma->safety_config != OSI_NULL)) {
		ret = osi_dma->ops->validate_regs(osi_dma);
	}

	return ret;
}

nve32_t osi_txring_empty(struct osi_dma_priv_data *osi_dma, nveu32_t chan)
{
	struct osi_tx_ring *tx_ring = osi_dma->tx_ring[chan];

	return (tx_ring->clean_idx == tx_ring->cur_tx_idx) ? 1 : 0;
}
#endif /* !OSI_STRIPPED_LIB */
