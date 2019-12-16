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

#include "dma_local.h"
#include "hw_desc.h"

/**
 * @brief mgbe_get_rx_err_stats - Detect Errors from Rx Descriptor
 *
 * Algorithm: This routine will be invoked by OSI layer itself which
 *	checks for the Last Descriptor and updates the receive status errors
 *	accordingly.
 *
 * @param[in] rx_desc: Rx Descriptor.
 * @param[in] pkt_err_stats: Packet error stats which stores the errors reported
 */
static inline void mgbe_update_rx_err_stats(struct osi_rx_desc *rx_desc,
					    struct osi_pkt_err_stats
					    pkt_err_stats)
{
	/* increment rx crc if we see CE bit set */
	if ((rx_desc->rdes3 & RDES3_ERR_MGBE_CRC) == RDES3_ERR_MGBE_CRC) {
		pkt_err_stats.rx_crc_error =
			osi_update_stats_counter(pkt_err_stats.rx_crc_error,
						 1UL);
	}
}

/**
 * @brief mgbe_get_rx_csum - Get the Rx checksum from descriptor if valid
 *
 * Algorithm:
 *      1) Check if the descriptor has any checksum validation errors.
 *      2) If none, set a per packet context flag indicating no err in
 *              Rx checksum
 *      3) The OSD layer will mark the packet appropriately to skip
 *              IP/TCP/UDP checksum validation in software based on whether
 *              COE is enabled for the device.
 *
 * @param[in] rx_desc: Rx descriptor
 * @param[in] rx_pkt_cx: Per-Rx packet context structure
 */
static void mgbe_get_rx_csum(struct osi_rx_desc *rx_desc,
			     struct osi_rx_pkt_cx *rx_pkt_cx)
{
	unsigned int ellt = rx_desc->rdes3 & RDES3_ELLT;

	/* Always include either checksum none/unnecessary
	 * depending on status fields in desc.
	 * Hence no need to explicitly add OSI_PKT_CX_CSUM flag.
	 */
	if ((ellt != RDES3_ELLT_IPHE) && (ellt != RDES3_ELLT_CSUM_ERR)) {
		rx_pkt_cx->rxcsum |= OSI_CHECKSUM_UNNECESSARY;
	}
}

void mgbe_init_desc_ops(struct desc_ops *d_ops)
{
        d_ops->get_rx_csum = mgbe_get_rx_csum;
	d_ops->update_rx_err_stats = mgbe_update_rx_err_stats;
}
