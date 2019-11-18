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
static void eqos_get_rx_csum(struct osi_rx_desc *rx_desc,
			     struct osi_rx_pkt_cx *rx_pkt_cx)
{
	nveu32_t pkt_type;

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

	pkt_type = rx_desc->rdes1 & RDES1_PT_MASK;
	if ((rx_desc->rdes1 & RDES1_IPV4) == RDES1_IPV4) {
		if (pkt_type == RDES1_PT_UDP) {
			rx_pkt_cx->rxcsum |= OSI_CHECKSUM_UDPv4;
		} else if (pkt_type == RDES1_PT_TCP) {
			rx_pkt_cx->rxcsum |= OSI_CHECKSUM_TCPv4;

		} else {
			/* Do nothing */
		}
	} else if ((rx_desc->rdes1 & RDES1_IPV6) == RDES1_IPV6) {
		if (pkt_type == RDES1_PT_UDP) {
			rx_pkt_cx->rxcsum |= OSI_CHECKSUM_UDPv6;
		} else if (pkt_type == RDES1_PT_TCP) {
			rx_pkt_cx->rxcsum |= OSI_CHECKSUM_TCPv6;

		} else {
			/* Do nothing */
		}

	} else {
		/* Do nothing */
	}

	if ((rx_desc->rdes1 & RDES1_IPCE) == RDES1_IPCE) {
		rx_pkt_cx->rxcsum |= OSI_CHECKSUM_TCP_UDP_BAD;
	}
}

void eqos_init_desc_ops(struct desc_ops *d_ops)
{
	d_ops->get_rx_csum = eqos_get_rx_csum;
}
