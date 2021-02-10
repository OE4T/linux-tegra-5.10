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
#include <osi_dma_txrx.h>
#include "../osi/common/type.h"
#include "hw_desc.h"

/**
 * @brief get_rx_csum - Get the Rx checksum from descriptor if valid
 *
 * @note
 * Algorithm:
 *  - Check if the descriptor has any checksum validation errors.
 *  - If none, set a per packet context flag indicating no err in
 *    Rx checksum
 *  - The OSD layer will mark the packet appropriately to skip
 *    IP/TCP/UDP checksum validation in software based on whether
 *    COE is enabled for the device.
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 *
 * @param[in, out] rx_desc: Rx descriptor
 * @param[in, out] rx_pkt_cx: Per-Rx packet context structure
 */
static inline void get_rx_csum(struct osi_rx_desc *rx_desc,
			       struct osi_rx_pkt_cx *rx_pkt_cx)
{
	/* Set rxcsum flags based on RDES1 values. These are required
	 * for QNX as it requires more granularity.
	 * Set none/unnecessary bit as well for other OS to check and
	 * take proper actions.
	 */
	if ((rx_desc->rdes3 & RDES3_RS1V) != RDES3_RS1V) {
		return;
	}

	if ((rx_desc->rdes1 &
		(RDES1_IPCE | RDES1_IPCB | RDES1_IPHE)) == OSI_DISABLE) {
		rx_pkt_cx->rxcsum |= OSI_CHECKSUM_UNNECESSARY;
	}

	if ((rx_desc->rdes1 & RDES1_IPCB) != OSI_DISABLE) {
		return;
	}

	rx_pkt_cx->rxcsum |= OSI_CHECKSUM_IPv4;
	if ((rx_desc->rdes1 & RDES1_IPHE) == RDES1_IPHE) {
		rx_pkt_cx->rxcsum |= OSI_CHECKSUM_IPv4_BAD;
	}

	if ((rx_desc->rdes1 & RDES1_PT_UDP) == RDES1_PT_UDP) {
		if ((rx_desc->rdes1 & RDES1_IPV4) == RDES1_IPV4) {
			rx_pkt_cx->rxcsum |= OSI_CHECKSUM_UDPv4;
		} else if ((rx_desc->rdes1 & RDES1_IPV6) == RDES1_IPV6) {
			rx_pkt_cx->rxcsum |= OSI_CHECKSUM_UDPv6;
		} else {
				/* Do nothing here */
		}
	} else if ((rx_desc->rdes1 & RDES1_PT_TCP) == RDES1_PT_TCP) {
		if ((rx_desc->rdes1 & RDES1_IPV4) == RDES1_IPV4) {
			rx_pkt_cx->rxcsum |= OSI_CHECKSUM_TCPv4;
		} else if ((rx_desc->rdes1 & RDES1_IPV6) == RDES1_IPV6) {
			rx_pkt_cx->rxcsum |= OSI_CHECKSUM_TCPv6;
		} else {
			/* Do nothing here */
		}
	} else {
			/* Do nothing here */
	}

	if ((rx_desc->rdes1 & RDES1_IPCE) == RDES1_IPCE) {
		rx_pkt_cx->rxcsum |= OSI_CHECKSUM_TCP_UDP_BAD;
	}
}

/**
 * @brief get_rx_vlan_from_desc - Get Rx VLAN from descriptor
 *
 * @note
 * Algorithm:
 *  - Check if the descriptor has any type set.
 *  - If set, set a per packet context flag indicating packet is VLAN
 *    tagged.
 *  - Extract VLAN tag ID from the descriptor
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 *
 * @param[in] rx_desc: Rx descriptor
 * @param[in, out] rx_pkt_cx: Per-Rx packet context structure
 */
static inline void get_rx_vlan_from_desc(struct osi_rx_desc *rx_desc,
					 struct osi_rx_pkt_cx *rx_pkt_cx)
{
	nveu32_t lt;

	/* Check for Receive Status rdes0 */
	if ((rx_desc->rdes3 & RDES3_RS0V) == RDES3_RS0V) {
		/* get length or type */
		lt = rx_desc->rdes3 & RDES3_LT;
		if ((lt == RDES3_LT_VT) || (lt == RDES3_LT_DVT)) {
			rx_pkt_cx->flags |= OSI_PKT_CX_VLAN;
			rx_pkt_cx->vlan_tag = rx_desc->rdes0 & RDES0_OVT;
		}
	}
}

/**
 * @brief get_rx_tstamp_status - Get Tx Time stamp status
 *
 * @note
 * Algorithm:
 *  - Check if the received descriptor is a context descriptor.
 *  - If yes, check whether the time stamp is valid or not.
 *
 * @param[in] context_desc: Rx context descriptor
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval -1 if TimeStamp is not valid
 * @retval 0 if TimeStamp is valid.
 */
static inline nve32_t get_rx_tstamp_status(struct osi_rx_desc *context_desc)
{
	if (((context_desc->rdes3 & RDES3_OWN) == 0U) &&
			((context_desc->rdes3 & RDES3_CTXT) == RDES3_CTXT)) {
		if (((context_desc->rdes0 == OSI_INVALID_VALUE) &&
		    (context_desc->rdes1 == OSI_INVALID_VALUE))) {
			/* Invalid time stamp */
			return -1;
		}
		/* tstamp can be read */
		return 0;
	}
	/* Busy */
	return -2;
}

/**
 * @brief get_rx_hwstamp - Get Rx HW Time stamp
 *
 * @note
 * Algorithm:
 *  - Check for TS availability.
 *  - call get_tx_tstamp_status if TS is valid or not.
 *  - If yes, set a bit and update nano seconds in rx_pkt_cx so that OSD
 *    layer can extract the time by checking this bit.
 *
 * @param[in] osi_dma: OSI private data structure.
 * @param[in] rx_desc: Rx descriptor
 * @param[in] context_desc: Rx context descriptor
 * @param[in, out] rx_pkt_cx: Rx packet context
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval -1 if TimeStamp is not available
 * @retval 0 if TimeStamp is available.
 */
static nve32_t get_rx_hwstamp(struct osi_dma_priv_data *osi_dma,
			      struct osi_rx_desc *rx_desc,
			      struct osi_rx_desc *context_desc,
			      struct osi_rx_pkt_cx *rx_pkt_cx)
{
	nve32_t retry, ret = -1;

	/* Check for RS1V/TSA/TD valid */
	if (((rx_desc->rdes3 & RDES3_RS1V) == RDES3_RS1V) &&
	    ((rx_desc->rdes1 & RDES1_TSA) == RDES1_TSA) &&
	    ((rx_desc->rdes1 & RDES1_TD) == 0U)) {
		for (retry = 0; retry < 10; retry++) {
			ret = get_rx_tstamp_status(context_desc);
			if (ret == 0) {
				/* Update rx pkt context flags to indicate PTP */
				rx_pkt_cx->flags |= OSI_PKT_CX_PTP;
				/* Time Stamp can be read */
				break;
			} else if (ret != -2) {
				/* Failed to get Rx timestamp */
				return ret;
			} else {
				/* Do nothing here */
			}
			/* TS not available yet, so retrying */
			osi_dma->osd_ops.udelay(1U);
		}
		if (ret != 0) {
			/* Timed out waiting for Rx timestamp */
			return ret;
		}

		if (OSI_NSEC_PER_SEC > (OSI_ULLONG_MAX / context_desc->rdes1)) {
			/* Will not hit this case */
		} else if ((OSI_ULLONG_MAX -
			    (context_desc->rdes1 * OSI_NSEC_PER_SEC)) <
			   context_desc->rdes0) {
			/* Will not hit this case */
		} else {
			rx_pkt_cx->ns = context_desc->rdes0 +
				(OSI_NSEC_PER_SEC * context_desc->rdes1);
		}

		if (rx_pkt_cx->ns < context_desc->rdes0) {
			/* Will not hit this case */
			return -1;
		}
	}
	return ret;
}

/**
 * @brief get_rx_err_stats - Detect Errors from Rx Descriptor
 *
 * @note
 * Algorithm:
 *  - This routine will be invoked by OSI layer itself which
 *    checks for the Last Descriptor and updates the receive status errors
 *    accordingly.
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 *
 * @param[in] rx_desc: Rx Descriptor.
 * @param[in, out] pkt_err_stats: Packet error stats which stores the errors
 *  reported
 */
static inline void get_rx_err_stats(struct osi_rx_desc *rx_desc,
				    struct osi_pkt_err_stats *pkt_err_stats)
{
	/* increment rx crc if we see CE bit set */
	if ((rx_desc->rdes3 & RDES3_ERR_CRC) == RDES3_ERR_CRC) {
		pkt_err_stats->rx_crc_error =
			osi_update_stats_counter(
					pkt_err_stats->rx_crc_error,
					1UL);
	}

	/* increment rx frame error if we see RE bit set */
	if ((rx_desc->rdes3 & RDES3_ERR_RE) == RDES3_ERR_RE) {
		pkt_err_stats->rx_frame_error =
			osi_update_stats_counter(
					pkt_err_stats->rx_frame_error,
					1UL);
	}
}

/**
 * @brief validate_rx_completions_arg- Validate input argument of rx_completions
 *
 * @note
 * Algorithm:
 *  - This routine validate input arguments to osi_process_rx_completions()
 *
 * @param[in] osi_dma: OSI DMA private data structure.
 * @param[in] chan: Rx DMA channel number
 * @param[in] more_data_avail: Pointer to more data available flag. OSI fills
 *         this flag if more rx packets available to read(1) or not(0).
 * @param[out] rx_ring: OSI DMA channel Rx ring
 * @param[out] rx_pkt_cx: OSI DMA receive packet context
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
static inline nve32_t validate_rx_completions_arg(
					      struct osi_dma_priv_data *osi_dma,
					      nveu32_t chan,
					      nveu32_t *more_data_avail,
					      struct osi_rx_ring **rx_ring,
					      struct osi_rx_pkt_cx **rx_pkt_cx)
{
	if (osi_unlikely((osi_dma == OSI_NULL) ||
			 (more_data_avail == OSI_NULL) ||
			 (chan >= OSI_EQOS_MAX_NUM_CHANS))) {
		return -1;
	}

	*rx_ring = osi_dma->rx_ring[chan];
	if (osi_unlikely(*rx_ring == OSI_NULL)) {
		OSI_DMA_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
			    "validate_input_rx_completions: Invalid pointers\n",
			    0ULL);
		return -1;
	}
	*rx_pkt_cx = &(*rx_ring)->rx_pkt_cx;
	if (osi_unlikely(*rx_pkt_cx == OSI_NULL)) {
		OSI_DMA_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
			    "validate_input_rx_completions: Invalid pointers\n",
			    0ULL);
		return -1;
	}

	return 0;
}

nve32_t osi_process_rx_completions(struct osi_dma_priv_data *osi_dma,
				   nveu32_t chan, nve32_t budget,
				   nveu32_t *more_data_avail)
{
	struct osi_rx_ring *rx_ring = OSI_NULL;
	struct osi_rx_pkt_cx *rx_pkt_cx = OSI_NULL;
	struct osi_rx_desc *rx_desc = OSI_NULL;
	struct osi_rx_swcx *rx_swcx = OSI_NULL;
	struct osi_rx_swcx *ptp_rx_swcx = OSI_NULL;
	struct osi_rx_desc *context_desc = OSI_NULL;
	nve32_t received = 0;
	nve32_t received_resv = 0;
	nve32_t ret = 0;

	ret = validate_rx_completions_arg(osi_dma, chan, more_data_avail,
					  &rx_ring, &rx_pkt_cx);
	if (osi_unlikely(ret < 0)) {
		return ret;
	}

	if (rx_ring->cur_rx_idx >= RX_DESC_CNT) {
		OSI_DMA_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
			    "dma_txrx: Invalid cur_rx_idx\n", 0ULL);
		return -1;
	}

	/* Reset flag to indicate if more Rx frames available to OSD layer */
	*more_data_avail = OSI_NONE;

	while ((received < budget) && (received_resv < budget)) {
		osi_memset(rx_pkt_cx, 0U, sizeof(*rx_pkt_cx));
		rx_desc = rx_ring->rx_desc + rx_ring->cur_rx_idx;
		rx_swcx = rx_ring->rx_swcx + rx_ring->cur_rx_idx;

		/* check for data availability */
		if ((rx_desc->rdes3 & RDES3_OWN) == RDES3_OWN) {
			break;
		}

		INCR_RX_DESC_INDEX(rx_ring->cur_rx_idx, 1U);

		if (osi_unlikely(rx_swcx->buf_virt_addr ==
		    osi_dma->resv_buf_virt_addr)) {
			rx_swcx->buf_virt_addr  = OSI_NULL;
			rx_swcx->buf_phy_addr  = 0;
			/* Reservered buffer used */
			received_resv++;
			if (osi_dma->osd_ops.realloc_buf != OSI_NULL) {
				osi_dma->osd_ops.realloc_buf(osi_dma->osd,
							     rx_ring, chan);
			}
			continue;
		}

		/* packet already processed */
		if ((rx_swcx->flags & OSI_RX_SWCX_PROCESSED) ==
		     OSI_RX_SWCX_PROCESSED) {
			break;
		}

		/* When JE is set, HW will accept any valid packet on Rx upto
		 * 9K or 16K (depending on GPSCLE bit), irrespective of whether
		 * MTU set is lower than these specific values. When Rx buf len
		 * is allocated to be exactly same as MTU, HW will consume more
		 * than 1 Rx desc. to place the larger packet and will set the
		 * LD bit in RDES3 accordingly.
		 * Restrict such Rx packets (which are longer than currently
		 * set MTU on DUT), and drop them in driver since HW cannot
		 * drop them. Also make use of swcx flags so that OSD can skip
		 * DMA buffer allocation and DMA mapping for those descriptors.
		 * If data is spread across multiple descriptors, drop packet
		 */
		if ((((rx_desc->rdes3 & RDES3_FD) == RDES3_FD) &&
		     ((rx_desc->rdes3 & RDES3_LD) == RDES3_LD)) ==
		    BOOLEAN_FALSE) {
			rx_swcx->flags |= OSI_RX_SWCX_REUSE;
			continue;
		}

		/* get the length of the packet */
		rx_pkt_cx->pkt_len = rx_desc->rdes3 & RDES3_PKT_LEN;

		/* Mark pkt as valid by default */
		rx_pkt_cx->flags |= OSI_PKT_CX_VALID;

		if ((rx_desc->rdes3 & RDES3_LD) == RDES3_LD) {
			if ((rx_desc->rdes3 & RDES3_ES_BITS) != 0U) {
				/* reset validity if any of the error bits
				 * are set
				 */
				rx_pkt_cx->flags &= ~OSI_PKT_CX_VALID;
				get_rx_err_stats(rx_desc,
						 &osi_dma->pkt_err_stats);
			}

			/* Check if COE Rx checksum is valid */
			get_rx_csum(rx_desc, rx_pkt_cx);

			get_rx_vlan_from_desc(rx_desc, rx_pkt_cx);
			context_desc = rx_ring->rx_desc + rx_ring->cur_rx_idx;
			/* Get rx time stamp */
			ret = get_rx_hwstamp(osi_dma, rx_desc, context_desc,
					     rx_pkt_cx);
			if (ret == 0) {
				ptp_rx_swcx = rx_ring->rx_swcx +
					      rx_ring->cur_rx_idx;
				/* Marking software context as PTP software
				 * context so that OSD can skip DMA buffer
				 * allocation and DMA mapping. DMA can use PTP
				 * software context addresses directly since
				 * those are valid.
				 */
				ptp_rx_swcx->flags |= OSI_RX_SWCX_REUSE;
				/* Context descriptor was consumed. Its skb
				 * and DMA mapping will be recycled
				 */
				INCR_RX_DESC_INDEX(rx_ring->cur_rx_idx, 1U);
			}
			if (osi_likely(osi_dma->osd_ops.receive_packet !=
				       OSI_NULL)) {
				osi_dma->osd_ops.receive_packet(osi_dma->osd,
							    rx_ring, chan,
							    osi_dma->rx_buf_len,
							    rx_pkt_cx, rx_swcx);
			} else {
				OSI_DMA_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
					    "dma_txrx: Invalid function pointer\n",
					    0ULL);
				return -1;
			}
		}
		osi_dma->dstats.q_rx_pkt_n[chan] =
			osi_update_stats_counter(
					osi_dma->dstats.q_rx_pkt_n[chan],
					1UL);
		osi_dma->dstats.rx_pkt_n =
			osi_update_stats_counter(osi_dma->dstats.rx_pkt_n, 1UL);
		received++;
	}

	/* If budget is done, check if HW ring still has unprocessed
	 * Rx packets, so that the OSD layer can decide to schedule
	 * this function again.
	 */
	if ((received + received_resv) >= budget) {
		rx_desc = rx_ring->rx_desc + rx_ring->cur_rx_idx;
		rx_swcx = rx_ring->rx_swcx + rx_ring->cur_rx_idx;
		if (((rx_swcx->flags & OSI_RX_SWCX_PROCESSED) !=
		    OSI_RX_SWCX_PROCESSED) &&
		    ((rx_desc->rdes3 & RDES3_OWN) != RDES3_OWN)) {
			/* Next descriptor has owned by SW
			 * So set more data avail flag here.
			 */
			*more_data_avail = OSI_ENABLE;
		}
	}

	return received;
}

/**
 * @brief inc_tx_pkt_stats - Increment Tx packet count Stats
 *
 * @note
 * Algorithm:
 *  - This routine will be invoked by OSI layer internally to increment
 *    stats for successfully transmitted packets on certain DMA channel.
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 *
 * @param[in, out] osi_dma: Pointer to OSI DMA private data structure.
 * @param[in] chan: DMA channel number for which stats should be incremented.
 */
static inline void inc_tx_pkt_stats(struct osi_dma_priv_data *osi_dma,
				    nveu32_t chan)
{
	osi_dma->dstats.q_tx_pkt_n[chan] =
		osi_update_stats_counter(osi_dma->dstats.q_tx_pkt_n[chan], 1UL);
	osi_dma->dstats.tx_pkt_n =
		osi_update_stats_counter(osi_dma->dstats.tx_pkt_n, 1UL);
}

/**
 * @brief get_tx_err_stats - Detect Errors from Tx Status
 *
 * @note
 * Algorithm:
 *  - This routine will be invoked by OSI layer itself which
 *    checks for the Last Descriptor and updates the transmit status errors
 *    accordingly.
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 *
 * @param[in] tx_desc: Tx Descriptor.
 * @param[in, out] pkt_err_stats: Packet error stats which stores the errors
 *  reported
 */
static inline void get_tx_err_stats(struct osi_tx_desc *tx_desc,
				    struct osi_pkt_err_stats *pkt_err_stats)
{
	/* IP Header Error */
	if ((tx_desc->tdes3 & TDES3_IP_HEADER_ERR) == TDES3_IP_HEADER_ERR) {
		pkt_err_stats->ip_header_error =
			osi_update_stats_counter(
					pkt_err_stats->ip_header_error,
					1UL);
	}

	/* Jabber timeout Error */
	if ((tx_desc->tdes3 & TDES3_JABBER_TIMEO_ERR) ==
	    TDES3_JABBER_TIMEO_ERR) {
		pkt_err_stats->jabber_timeout_error =
			osi_update_stats_counter(
					pkt_err_stats->jabber_timeout_error,
					1UL);
	}

	/* Packet Flush Error */
	if ((tx_desc->tdes3 & TDES3_PKT_FLUSH_ERR) == TDES3_PKT_FLUSH_ERR) {
		pkt_err_stats->pkt_flush_error =
			osi_update_stats_counter(
					pkt_err_stats->pkt_flush_error, 1UL);
	}

	/* Payload Checksum Error */
	if ((tx_desc->tdes3 & TDES3_PL_CHK_SUM_ERR) == TDES3_PL_CHK_SUM_ERR) {
		pkt_err_stats->payload_cs_error =
			osi_update_stats_counter(
					pkt_err_stats->payload_cs_error, 1UL);
	}

	/* Loss of Carrier Error */
	if ((tx_desc->tdes3 & TDES3_LOSS_CARRIER_ERR) ==
	    TDES3_LOSS_CARRIER_ERR) {
		pkt_err_stats->loss_of_carrier_error =
			osi_update_stats_counter(
					pkt_err_stats->loss_of_carrier_error,
					1UL);
	}

	/* No Carrier Error */
	if ((tx_desc->tdes3 & TDES3_NO_CARRIER_ERR) == TDES3_NO_CARRIER_ERR) {
		pkt_err_stats->no_carrier_error =
			osi_update_stats_counter(
					pkt_err_stats->no_carrier_error, 1UL);
	}

	/* Late Collision Error */
	if ((tx_desc->tdes3 & TDES3_LATE_COL_ERR) == TDES3_LATE_COL_ERR) {
		pkt_err_stats->late_collision_error =
			osi_update_stats_counter(
					pkt_err_stats->late_collision_error,
					1UL);
	}

	/* Excessive Collision Error */
	if ((tx_desc->tdes3 & TDES3_EXCESSIVE_COL_ERR) ==
	    TDES3_EXCESSIVE_COL_ERR) {
		pkt_err_stats->excessive_collision_error =
			osi_update_stats_counter(
				pkt_err_stats->excessive_collision_error,
				1UL);
	}

	/* Excessive Deferal Error */
	if ((tx_desc->tdes3 & TDES3_EXCESSIVE_DEF_ERR) ==
	    TDES3_EXCESSIVE_DEF_ERR) {
		pkt_err_stats->excessive_deferal_error =
			osi_update_stats_counter(
					pkt_err_stats->excessive_deferal_error,
					1UL);
	}

	/* Under Flow Error */
	if ((tx_desc->tdes3 & TDES3_UNDER_FLOW_ERR) == TDES3_UNDER_FLOW_ERR) {
		pkt_err_stats->underflow_error =
			osi_update_stats_counter(pkt_err_stats->underflow_error,
						 1UL);
	}
}

#ifndef OSI_STRIPPED_LIB
nve32_t osi_clear_tx_pkt_err_stats(struct osi_dma_priv_data *osi_dma)
{
	nve32_t ret = -1;
	struct osi_pkt_err_stats *pkt_err_stats;

	if (osi_dma != OSI_NULL) {
		pkt_err_stats = &osi_dma->pkt_err_stats;
		/* Reset tx packet errors */
		pkt_err_stats->ip_header_error = 0U;
		pkt_err_stats->jabber_timeout_error = 0U;
		pkt_err_stats->pkt_flush_error = 0U;
		pkt_err_stats->payload_cs_error = 0U;
		pkt_err_stats->loss_of_carrier_error = 0U;
		pkt_err_stats->no_carrier_error = 0U;
		pkt_err_stats->late_collision_error = 0U;
		pkt_err_stats->excessive_collision_error = 0U;
		pkt_err_stats->excessive_deferal_error = 0U;
		pkt_err_stats->underflow_error = 0U;
		pkt_err_stats->clear_tx_err =
			osi_update_stats_counter(pkt_err_stats->clear_tx_err,
						 1UL);
		ret = 0;
	}

	return ret;
}

nve32_t osi_clear_rx_pkt_err_stats(struct osi_dma_priv_data *osi_dma)
{
	nve32_t ret = -1;
	struct osi_pkt_err_stats *pkt_err_stats;

	if (osi_dma != OSI_NULL) {
		pkt_err_stats = &osi_dma->pkt_err_stats;
		/* Reset Rx packet errors */
		pkt_err_stats->rx_crc_error = 0U;
		pkt_err_stats->clear_tx_err =
			osi_update_stats_counter(pkt_err_stats->clear_rx_err,
						 1UL);
		ret = 0;
	}

	return ret;
}
#endif /* !OSI_STRIPPED_LIB */

/**
 * @brief validate_tx_completions_arg- Validate input argument of tx_completions
 *
 * @note
 * Algorithm:
 *  - This routine validate input arguments to osi_process_tx_completions()
 *
 * @param[in] osi_dma: OSI DMA private data structure.
 * @param[in] chan: Tx DMA channel number
 * @param[out] tx_ring: OSI DMA channel Rx ring
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
static inline nve32_t validate_tx_completions_arg(
					      struct osi_dma_priv_data *osi_dma,
					      nveu32_t chan,
					      struct osi_tx_ring **tx_ring)
{
	if (osi_unlikely((osi_dma == OSI_NULL) ||
			 (chan >= OSI_EQOS_MAX_NUM_CHANS))) {
		return -1;
	}

	*tx_ring = osi_dma->tx_ring[chan];

	if (osi_unlikely(*tx_ring == OSI_NULL)) {
		OSI_DMA_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
			    "validate_tx_completions_arg: Invalid pointers\n",
			    0ULL);
		return -1;
	}

	return 0;
}

nve32_t osi_process_tx_completions(struct osi_dma_priv_data *osi_dma,
				   nveu32_t chan, nve32_t budget)
{
	struct osi_tx_ring *tx_ring = OSI_NULL;
	struct osi_txdone_pkt_cx *txdone_pkt_cx = OSI_NULL;
	struct osi_tx_swcx *tx_swcx = OSI_NULL;
	struct osi_tx_desc *tx_desc = OSI_NULL;
	nveu32_t entry = 0U;
	nveu64_t vartdes1;
	nveul64_t ns;
	nve32_t processed = 0;
	nve32_t ret;

	ret = validate_tx_completions_arg(osi_dma, chan, &tx_ring);
	if (osi_unlikely(ret < 0)) {
		return ret;
	}

	txdone_pkt_cx = &tx_ring->txdone_pkt_cx;
	entry = tx_ring->clean_idx;

	osi_dma->dstats.tx_clean_n[chan] =
		osi_update_stats_counter(osi_dma->dstats.tx_clean_n[chan], 1U);

	while ((entry != tx_ring->cur_tx_idx) && (entry < TX_DESC_CNT) &&
	       (processed < budget)) {
		osi_memset(txdone_pkt_cx, 0U, sizeof(*txdone_pkt_cx));

		tx_desc = tx_ring->tx_desc + entry;
		tx_swcx = tx_ring->tx_swcx + entry;

		if ((tx_desc->tdes3 & TDES3_OWN) == TDES3_OWN) {
			break;
		}

		/* check for Last Descriptor */
		if ((tx_desc->tdes3 & TDES3_LD) == TDES3_LD) {
			if ((tx_desc->tdes3 & TDES3_ES_BITS) != 0U) {
				txdone_pkt_cx->flags |= OSI_TXDONE_CX_ERROR;
				/* fill packet error stats */
				get_tx_err_stats(tx_desc,
						 &osi_dma->pkt_err_stats);
			} else {
				inc_tx_pkt_stats(osi_dma, chan);
			}

			if (processed < INT_MAX) {
				processed++;
			}
		}

		if (((tx_desc->tdes3 & TDES3_LD) == TDES3_LD) &&
		    ((tx_desc->tdes3 & TDES3_CTXT) == 0U)) {
			/* check tx tstamp status */
			if ((tx_desc->tdes3 & TDES3_TTSS) != 0U) {
				/* tx timestamp captured for this packet */
				ns = tx_desc->tdes0;
				vartdes1 = tx_desc->tdes1;
				if (OSI_NSEC_PER_SEC >
						(OSI_ULLONG_MAX / vartdes1)) {
					/* Will not hit this case */
				} else if ((OSI_ULLONG_MAX -
					(vartdes1 * OSI_NSEC_PER_SEC)) < ns) {
					/* Will not hit this case */
				} else {
					txdone_pkt_cx->flags |=
						OSI_TXDONE_CX_TS;
					txdone_pkt_cx->ns = ns +
						(vartdes1 * OSI_NSEC_PER_SEC);
				}
			} else {
				/* Do nothing here */
			}
		}

		if (tx_swcx->is_paged_buf == 1U) {
			txdone_pkt_cx->flags |= OSI_TXDONE_CX_PAGED_BUF;
		}

		if (osi_likely(osi_dma->osd_ops.transmit_complete !=
			       OSI_NULL)) {
			osi_dma->osd_ops.transmit_complete(osi_dma->osd,
						       tx_swcx->buf_virt_addr,
						       tx_swcx->buf_phy_addr,
						       tx_swcx->len,
						       txdone_pkt_cx);
		} else {
			OSI_DMA_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
				    "dma_txrx: Invalid function pointer\n",
				    0ULL);
			return -1;
		}

		tx_desc->tdes3 = 0;
		tx_desc->tdes2 = 0;
		tx_desc->tdes1 = 0;
		tx_desc->tdes0 = 0;
		tx_swcx->len = 0;

		tx_swcx->buf_virt_addr = OSI_NULL;
		tx_swcx->buf_phy_addr = 0;
		tx_swcx->is_paged_buf = 0;
		INCR_TX_DESC_INDEX(entry, 1U);

		/* Don't wait to update tx_ring->clean-idx. It will
		 * be used by OSD layer to determine the num. of available
		 * descriptors in the ring, which will in turn be used to
		 * wake the corresponding transmit queue in OS layer.
		 */
		tx_ring->clean_idx = entry;
	}

	return processed;
}

/**
 * @brief need_cntx_desc - Helper function to check if context desc is needed.
 *
 * @note
 * Algorithm:
 *  - Check if transmit packet context flags are set
 *  - If set, set the context descriptor bit along
 *    with other context information in the transmit descriptor.
 *
 * @param[in, out] tx_pkt_cx: Pointer to transmit packet context structure
 * @param[in, out] tx_desc: Pointer to transmit descriptor to be filled.
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 - cntx desc not used
 * @retval 1 - cntx desc used.
 */
static inline nve32_t need_cntx_desc(struct osi_tx_pkt_cx *tx_pkt_cx,
				     struct osi_tx_desc *tx_desc)
{
	nve32_t ret = 0;

	if (((tx_pkt_cx->flags & OSI_PKT_CX_VLAN) == OSI_PKT_CX_VLAN) ||
	    ((tx_pkt_cx->flags & OSI_PKT_CX_TSO) == OSI_PKT_CX_TSO)) {
		/* Set context type */
		tx_desc->tdes3 |= TDES3_CTXT;

		if ((tx_pkt_cx->flags & OSI_PKT_CX_VLAN) == OSI_PKT_CX_VLAN) {
			/* Remove any overflow bits. VT field is 16bit field */
			tx_pkt_cx->vtag_id &= TDES3_VT_MASK;
			/* Fill VLAN Tag ID */
			tx_desc->tdes3 |= tx_pkt_cx->vtag_id;
			/* Set VLAN TAG Valid */
			tx_desc->tdes3 |= TDES3_VLTV;
		}

		if ((tx_pkt_cx->flags & OSI_PKT_CX_TSO) == OSI_PKT_CX_TSO) {
			/* Remove any overflow bits. MSS is 13bit field */
			tx_pkt_cx->mss &= TDES2_MSS_MASK;
			/* Fill MSS */
			tx_desc->tdes2 |= tx_pkt_cx->mss;
			/* Set MSS valid */
			tx_desc->tdes3 |= TDES3_TCMSSV;
		}

		ret = 1;
	}

	return ret;
}

/**
 * @brief fill_first_desc - Helper function to fill the first transmit
 *	descriptor.
 *
 * @note
 * Algorithm:
 *  - Update the buffer address and length of buffer in first desc.
 *  - Check if any features like HW checksum offload, TSO, VLAN insertion
 *    etc. are flagged in transmit packet context. If so, set the fields in
 *    first desc corresponding to those features.
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 *
 * @param[in, out] tx_ring: DMA channel TX ring.
 * @param[in, out] tx_pkt_cx: Pointer to transmit packet context structure
 * @param[in, out] tx_desc: Pointer to transmit descriptor to be filled.
 * @param[in] tx_swcx: Pointer to corresponding tx descriptor software context.
 */
static inline void fill_first_desc(struct osi_tx_ring *tx_ring,
				   struct osi_tx_pkt_cx *tx_pkt_cx,
				   struct osi_tx_desc *tx_desc,
				   struct osi_tx_swcx *tx_swcx)
{
	nveu64_t tmp;

	/* update the first buffer pointer and length */
	tmp = L32(tx_swcx->buf_phy_addr);
	if (tmp < UINT_MAX) {
		tx_desc->tdes0 = (nveu32_t)tmp;
	}

	tmp = H32(tx_swcx->buf_phy_addr);
	if (tmp < UINT_MAX) {
		tx_desc->tdes1 = (nveu32_t)tmp;
	}

	tx_desc->tdes2 = tx_swcx->len;
	/* Mark it as First descriptor */
	tx_desc->tdes3 |= TDES3_FD;

	/* If HW checksum offload enabled, mark CIC bits of FD */
	if ((tx_pkt_cx->flags & OSI_PKT_CX_CSUM) == OSI_PKT_CX_CSUM) {
		tx_desc->tdes3 |= TDES3_HW_CIC_ALL;
	} else {
		if ((tx_pkt_cx->flags & OSI_PKT_CX_IP_CSUM) ==
		    OSI_PKT_CX_IP_CSUM) {
			/* If IP only Checksum enabled, mark fist bit of CIC */
			tx_desc->tdes3 |= TDES3_HW_CIC_IP_ONLY;
		}
	}

	/* Enable VTIR in normal descriptor for VLAN packet */
	if ((tx_pkt_cx->flags & OSI_PKT_CX_VLAN) == OSI_PKT_CX_VLAN) {
		tx_desc->tdes2 |= TDES2_VTIR;
	}

	/* if TS is set enable timestamping */
	if ((tx_pkt_cx->flags & OSI_PKT_CX_PTP) == OSI_PKT_CX_PTP) {
		tx_desc->tdes2 |= TDES2_TTSE;
	}

	/* if LEN bit is set, update packet payload len */
	if ((tx_pkt_cx->flags & OSI_PKT_CX_LEN) == OSI_PKT_CX_LEN) {
		/* Remove any overflow bits. PL field is 15 bit wide */
		tx_pkt_cx->payload_len &= ~TDES3_PL_MASK;
		/* Update packet len in desc */
		tx_desc->tdes3 |= tx_pkt_cx->payload_len;
	}

	/* Enable TSE bit and update TCP hdr, payload len */
	if ((tx_pkt_cx->flags & OSI_PKT_CX_TSO) == OSI_PKT_CX_TSO) {
		tx_desc->tdes3 |= TDES3_TSE;

		/* Minimum value for THL field is 5 for TSO
		 * So divide L4 hdr len by 4
		 * Typical TCP hdr len = 20B / 4 = 5
		 */
		tx_pkt_cx->tcp_udp_hdrlen /= OSI_TSO_HDR_LEN_DIVISOR;
		/* Remove any overflow bits. THL field is only 4 bit wide */
		tx_pkt_cx->tcp_udp_hdrlen &= TDES3_THL_MASK;
		/* Update hdr len in desc */
		tx_desc->tdes3 |= (tx_pkt_cx->tcp_udp_hdrlen <<
				   TDES3_THL_SHIFT);
		/* Remove any overflow bits. TPL field is 18 bit wide */
		tx_pkt_cx->payload_len &= TDES3_TPL_MASK;
		/* Update TCP payload len in desc */
		tx_desc->tdes3 &= ~TDES3_TPL_MASK;
		tx_desc->tdes3 |= tx_pkt_cx->payload_len;
	} else {
		if ((tx_ring->slot_check == OSI_ENABLE) &&
		    (tx_ring->slot_number < OSI_SLOT_NUM_MAX)) {
			/* Fill Slot number */
			tx_desc->tdes3 |= (tx_ring->slot_number <<
					   TDES3_THL_SHIFT);
			tx_ring->slot_number = ((tx_ring->slot_number + 1U) %
						OSI_SLOT_NUM_MAX);
		}
	}
}

/**
 * @brief dmb_oshst() - Data memory barrier operation that waits only
 * for stores to complete, and only to the outer shareable domain.
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 */
static inline void dmb_oshst(void)
{
	asm volatile("dmb oshst" : : : "memory");
}

/**
 * @brief validate_hw_transmit_arg- Validate input argument of hw_transmit
 *
 * @note
 * Algorithm:
 *  - This routine validate input arguments to osi_hw_transmit()
 *
 * @param[in] osi_dma: OSI DMA private data structure.
 * @param[in] chan: Tx DMA channel number
 * @param[out] ops: OSI DMA Channel operations
 * @param[out] tx_ring: OSI DMA channel Tx ring
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
static inline nve32_t validate_hw_transmit_arg(
					   struct osi_dma_priv_data *osi_dma,
					   nveu32_t chan,
					   struct osi_dma_chan_ops **ops,
					   struct osi_tx_ring **tx_ring)
{
	if (osi_unlikely((osi_dma == OSI_NULL) ||
			 (chan >= OSI_EQOS_MAX_NUM_CHANS))) {
		return -1;
	}

	*tx_ring = osi_dma->tx_ring[chan];
	*ops = osi_dma->ops;

	if (osi_unlikely((*tx_ring == OSI_NULL) || (*ops == OSI_NULL))) {
		OSI_DMA_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
			    "validate_hw_transmit_arg: Invalid pointers\n",
			    0ULL);
		return -1;
	}

	return 0;
}

void osi_hw_transmit(struct osi_dma_priv_data *osi_dma, nveu32_t chan)
{
	struct osi_tx_ring *tx_ring = OSI_NULL;
	struct osi_dma_chan_ops *ops = OSI_NULL;
	nveu32_t entry = 0U;
	struct osi_tx_desc *tx_desc = OSI_NULL;
	struct osi_tx_swcx *tx_swcx = OSI_NULL;
	struct osi_tx_pkt_cx *tx_pkt_cx = OSI_NULL;
	nveu32_t desc_cnt = 0U;
	struct osi_tx_desc *last_desc = OSI_NULL;
	struct osi_tx_desc *first_desc = OSI_NULL;
	struct osi_tx_desc *cx_desc = OSI_NULL;
	nveu64_t tailptr, tmp;
	nve32_t cntx_desc_consumed;
	nveu32_t i;
	nve32_t ret = 0;

	ret = validate_hw_transmit_arg(osi_dma, chan, &ops, &tx_ring);
	if (osi_unlikely(ret < 0)) {
		return;
	}

	entry = tx_ring->cur_tx_idx;
	if (entry >= TX_DESC_CNT) {
		OSI_DMA_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
			    "dma_txrx: Invalid pointers\n", 0ULL);
		return;
	}

	tx_desc = tx_ring->tx_desc + entry;
	tx_swcx = tx_ring->tx_swcx + entry;
	tx_pkt_cx = &tx_ring->tx_pkt_cx;

	desc_cnt = tx_pkt_cx->desc_cnt;
	if (osi_unlikely(desc_cnt == 0U)) {
		/* Will not hit this case */
		OSI_DMA_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
			    "dma_txrx: Invalid value\n", 0ULL);
		return;
	}
	/* Context descriptor for VLAN/TSO */
	if ((tx_pkt_cx->flags & OSI_PKT_CX_VLAN) == OSI_PKT_CX_VLAN) {
		osi_dma->dstats.tx_vlan_pkt_n =
			osi_update_stats_counter(osi_dma->dstats.tx_vlan_pkt_n,
						 1UL);
	}

	if ((tx_pkt_cx->flags & OSI_PKT_CX_TSO) == OSI_PKT_CX_TSO) {
		osi_dma->dstats.tx_tso_pkt_n =
			osi_update_stats_counter(osi_dma->dstats.tx_tso_pkt_n,
						 1UL);
	}

	cntx_desc_consumed = need_cntx_desc(tx_pkt_cx, tx_desc);
	if (cntx_desc_consumed == 1) {
		INCR_TX_DESC_INDEX(entry, 1U);

		/* Storing context descriptor to set DMA_OWN at last */
		cx_desc = tx_desc;
		tx_desc = tx_ring->tx_desc + entry;
		tx_swcx = tx_ring->tx_swcx + entry;
		desc_cnt--;
	}

	/* Fill first descriptor */
	fill_first_desc(tx_ring, tx_pkt_cx, tx_desc, tx_swcx);

	INCR_TX_DESC_INDEX(entry, 1U);

	first_desc = tx_desc;
	last_desc = tx_desc;
	tx_desc = tx_ring->tx_desc + entry;
	tx_swcx = tx_ring->tx_swcx + entry;
	desc_cnt--;

	/* Fill remaining descriptors */
	for (i = 0; i < desc_cnt; i++) {
		tmp = L32(tx_swcx->buf_phy_addr);
		if (tmp < UINT_MAX) {
			tx_desc->tdes0 = (nveu32_t)tmp;
		}

		tmp = H32(tx_swcx->buf_phy_addr);
		if (tmp < UINT_MAX) {
			tx_desc->tdes1 = (nveu32_t)tmp;
		}
		tx_desc->tdes2 = tx_swcx->len;
		/* set HW OWN bit for descriptor*/
		tx_desc->tdes3 |= TDES3_OWN;

		INCR_TX_DESC_INDEX(entry, 1U);
		last_desc = tx_desc;
		tx_desc = tx_ring->tx_desc + entry;
		tx_swcx = tx_ring->tx_swcx + entry;
	}

	/* Mark it as LAST descriptor */
	last_desc->tdes3 |= TDES3_LD;
	/* set Interrupt on Completion*/
	last_desc->tdes2 |= TDES2_IOC;

	if (tx_ring->frame_cnt < UINT_MAX) {
		tx_ring->frame_cnt++;
	} else if ((osi_dma->use_tx_frames == OSI_ENABLE) &&
		   ((tx_ring->frame_cnt % osi_dma->tx_frames) < UINT_MAX)) {
		/* make sure count for tx_frame interrupt logic is retained */
		tx_ring->frame_cnt = (tx_ring->frame_cnt % osi_dma->tx_frames)
					+ 1U;
	} else {
		tx_ring->frame_cnt = 1U;
	}

	/* clear IOC bit if tx SW timer based coalescing is enabled */
	if (osi_dma->use_tx_usecs == OSI_ENABLE) {
		last_desc->tdes2 &= ~TDES2_IOC;

		/* update IOC bit if tx_frames is enabled. Tx_frames
		 * can be enabled only along with tx_usecs.
		 */
		if (osi_dma->use_tx_frames == OSI_ENABLE) {
			if ((tx_ring->frame_cnt % osi_dma->tx_frames) ==
			    OSI_NONE) {
				last_desc->tdes2 |= TDES2_IOC;
			}
		}
	}
	/* Set OWN bit for first and context descriptors
	 * at the end to avoid race condition
	 */
	first_desc->tdes3 |= TDES3_OWN;
	if (cntx_desc_consumed == 1) {
		cx_desc->tdes3 |= TDES3_OWN;
	}

	tailptr = tx_ring->tx_desc_phy_addr +
		  (entry * sizeof(struct osi_tx_desc));
	if (osi_unlikely(tailptr < tx_ring->tx_desc_phy_addr)) {
		/* Will not hit this case */
		OSI_DMA_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
			    "dma_txrx: Invalid argument\n", 0ULL);
		return;
	}

	tx_ring->cur_tx_idx = entry;

	/*
	 * We need to make sure Tx descriptor updated above is really updated
	 * before setting up the DMA, hence add memory write barrier here.
	 */
	dmb_oshst();
	if (osi_unlikely(ops->update_tx_tailptr == OSI_NULL)) {
		OSI_DMA_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
			    "dma_txrx: Invalid argument\n", 0ULL);
		return;
	}
	ops->update_tx_tailptr(osi_dma->base, chan, tailptr);
}

/**
 * @brief rx_dma_desc_initialization - Initialize DMA Receive descriptors for Rx
 *
 * @note
 * Algorithm:
 *  - Initialize Receive descriptors with DMA mappable buffers,
 *    set OWN bit, Rx ring length and set starting address of Rx DMA channel.
 *    Tx ring base address in Tx DMA registers.
 *
 * @param[in, out] osi_dma:	OSI private data structure.
 * @param[in] chan:	Rx channel number.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: No
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t rx_dma_desc_initialization(struct osi_dma_priv_data *osi_dma,
					  nveu32_t chan)
{
	struct osi_rx_ring *rx_ring = OSI_NULL;
	struct osi_rx_desc *rx_desc = OSI_NULL;
	struct osi_rx_swcx *rx_swcx = OSI_NULL;
	struct osi_dma_chan_ops *ops = osi_dma->ops;
	nveu64_t tailptr = 0, tmp;
	nveu32_t i;
	nve32_t ret = 0;

	rx_ring = osi_dma->rx_ring[chan];
	if (osi_unlikely(rx_ring == OSI_NULL)) {
		OSI_DMA_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
			    "dma_txrx: Invalid argument\n", 0ULL);
		return -1;
	};

	rx_ring->cur_rx_idx = 0;
	rx_ring->refill_idx = 0;

	for (i = 0; i < RX_DESC_CNT; i++) {
		rx_swcx = rx_ring->rx_swcx + i;
		rx_desc = rx_ring->rx_desc + i;

		/* Zero initialize the descriptors first */
		rx_desc->rdes0 = 0;
		rx_desc->rdes1 = 0;
		rx_desc->rdes2 = 0;
		rx_desc->rdes3 = 0;

		tmp = L32(rx_swcx->buf_phy_addr);
		if (tmp < UINT_MAX) {
			rx_desc->rdes0 = (nveu32_t)tmp;
		} else {
			OSI_DMA_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
				    "dma_txrx: Invalid buf_phy_addr\n", 0ULL);
			return -1;
		}

		tmp = H32(rx_swcx->buf_phy_addr);
		if (tmp < UINT_MAX) {
			rx_desc->rdes1 = (nveu32_t)tmp;
		} else {
			OSI_DMA_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
				    "dma_txrx: Invalid buf_phy_addr\n", 0ULL);
			return -1;
		}

		rx_desc->rdes2 = 0;
		rx_desc->rdes3 = (RDES3_IOC | RDES3_B1V);
		/* reconfigure INTE bit if RX watchdog timer is enabled */
		if (osi_dma->use_riwt == OSI_ENABLE) {
			rx_desc->rdes3 &= ~RDES3_IOC;
			if (osi_dma->use_rx_frames == OSI_ENABLE) {
				if ((i % osi_dma->rx_frames) == OSI_NONE) {
					/* update IOC bit if rx_frames is
					 * enabled. Rx_frames can be enabled
					 * only along with RWIT.
					 */
					rx_desc->rdes3 |= RDES3_IOC;
				}
			}
		}
		rx_desc->rdes3 |= RDES3_OWN;

		rx_swcx->flags = 0;
	}

	tailptr = rx_ring->rx_desc_phy_addr +
		  (sizeof(struct osi_rx_desc) * (RX_DESC_CNT));

	if (osi_unlikely((tailptr < rx_ring->rx_desc_phy_addr) ||
			 (ops->set_rx_ring_len == OSI_NULL) ||
			 (ops->update_rx_tailptr == OSI_NULL) ||
			 (ops->set_rx_ring_start_addr == OSI_NULL))) {
		/* Will not hit this case */
		OSI_DMA_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
			    "dma_txrx: Invalid pointers\n", 0ULL);
		return -1;
	}

	ops->set_rx_ring_len(osi_dma, chan, (RX_DESC_CNT - 1U));
	ops->update_rx_tailptr(osi_dma->base, chan, tailptr);
	ops->set_rx_ring_start_addr(osi_dma->base, chan,
				    rx_ring->rx_desc_phy_addr);

	return ret;
}

/**
 * @brief rx_dma_desc_init - Initialize DMA Receive descriptors for Rx channel.
 *
 * @note
 * Algorithm:
 *  - Initialize Receive descriptors with DMA mappable buffers,
 *    set OWN bit, Rx ring length and set starting address of Rx DMA channel.
 *    Tx ring base address in Tx DMA registers.
 *
 * @param[in, out] osi_dma: OSI private data structure.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: No
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t rx_dma_desc_init(struct osi_dma_priv_data *osi_dma)
{
	nveu32_t chan = 0;
	nveu32_t i;
	nve32_t ret = 0;

	for (i = 0; i < osi_dma->num_dma_chans; i++) {
		chan = osi_dma->dma_chans[i];

		ret = rx_dma_desc_initialization(osi_dma, chan);
		if (ret != 0) {
			return ret;
		}
	}

	return ret;
}

/**
 * @brief tx_dma_desc_init - Initialize DMA Transmit descriptors.
 *
 * @note
 * Algorithm:
 *  - Initialize Transmit descriptors and set Tx ring length,
 *    Tx ring base address in Tx DMA registers.
 *
 * @param[in, out] osi_dma: OSI DMA private data structure.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: No
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t tx_dma_desc_init(struct osi_dma_priv_data *osi_dma)
{
	struct osi_tx_ring *tx_ring = OSI_NULL;
	struct osi_tx_desc *tx_desc = OSI_NULL;
	struct osi_tx_swcx *tx_swcx = OSI_NULL;
	struct osi_dma_chan_ops *ops = osi_dma->ops;
	nveu32_t chan = 0;
	nveu32_t i, j;

	for (i = 0; i < osi_dma->num_dma_chans; i++) {
		chan = osi_dma->dma_chans[i];

		tx_ring = osi_dma->tx_ring[chan];
		if (osi_unlikely(tx_ring == OSI_NULL)) {
			OSI_DMA_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
				    "dma_txrx: Invalid pointers\n", 0ULL);
			return -1;
		}

		for (j = 0; j < TX_DESC_CNT; j++) {
			tx_desc = tx_ring->tx_desc + j;
			tx_swcx = tx_ring->tx_swcx + j;

			tx_desc->tdes0 = 0;
			tx_desc->tdes1 = 0;
			tx_desc->tdes2 = 0;
			tx_desc->tdes3 = 0;

			tx_swcx->len = 0;
			tx_swcx->buf_virt_addr = OSI_NULL;
			tx_swcx->buf_phy_addr = 0;
			tx_swcx->is_paged_buf = 0;
		}

		tx_ring->cur_tx_idx = 0;
		tx_ring->clean_idx = 0;

		/* Slot function parameter initialization */
		tx_ring->slot_number = 0U;
		tx_ring->slot_check = OSI_DISABLE;

		if (osi_likely((ops->set_tx_ring_len != OSI_NULL) &&
			       (ops->set_tx_ring_start_addr != OSI_NULL))) {
			ops->set_tx_ring_len(osi_dma, chan,
					     (TX_DESC_CNT - 1U));
			ops->set_tx_ring_start_addr(osi_dma->base, chan,
						    tx_ring->tx_desc_phy_addr);
		} else {
			OSI_DMA_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
				    "dma_txrx: Invalid pointers\n", 0ULL);
			return -1;
		}
	}

	return 0;
}

nve32_t dma_desc_init(struct osi_dma_priv_data *osi_dma)
{
	nve32_t ret = 0;

	ret = tx_dma_desc_init(osi_dma);
	if (ret != 0) {
		return ret;
	}

	ret = rx_dma_desc_init(osi_dma);
	if (ret != 0) {
		return ret;
	}

	return ret;
}

