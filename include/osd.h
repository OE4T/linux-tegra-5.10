/*
 * Copyright (c) 2018-2020, NVIDIA CORPORATION. All rights reserved.
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

#ifndef INCLUDED_OSD_H
#define INCLUDED_OSD_H

#include "../osi/common/type.h"

/**
 * @brief osd_usleep_range - sleep in micro seconds
 *
 * @param[in] umin: Minimum time in usecs to sleep
 * @param[in] umax: Maximum time in usecs to sleep
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 */
void osd_usleep_range(nveu64_t umin, nveu64_t umax);
/**
 * @brief osd_msleep - sleep in milli seconds
 *
 * @param[in] msec: time in milli seconds
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 */
void osd_msleep(nveu32_t msec);
/**
 * @brief osd_udelay - delay in micro seconds
 *
 * @param[in] usec: time in usec
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 */
void osd_udelay(nveu64_t usec);
/**
 * @brief osd_receive_packet - Handover received packet to network stack.
 *
 * @note
 * Algorithm:
 *  - Unmap the DMA buffer address (not needed if buffers are
 *    allocated statically).
 *  - Refill the Rx ring based on threshold.
 *  - Consume the flag information and take decision to hand over the packet
 *    and related information to OS network stack.
 *
 * @param[in] priv: OSD private data structure.
 * @param[in] rxring: Pointer to DMA channel Rx ring.
 * @param[in] chan: DMA Rx channel number.
 * @param[in] dma_buf_len: Rx DMA buffer length.
 * @param[in] rxpkt_cx: Received packet context.
 * @param[out] rx_pkt_swcx: Received packet sw context.
 *
 * @pre Rx completion need to make sure that Rx descriptors processed properly.
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 */
void osd_receive_packet(void *priv, void *rxring, nveu32_t chan,
			nveu32_t dma_buf_len, void *rxpkt_cx,
			void *rx_pkt_swcx);

/**
 * @brief osd_transmit_complete - Transmit completion routine.
 *
 * @note
 * Algorithm:
 *  - Unmap and free the buffer DMA address and buffer (not needed if buffers
 *    are allocated statically).
 *  - Time stamp will be updated to stack if available.
 *
 * @param[in] priv: OSD private data structure.
 * @param[in] buffer: Buffer address to free.
 * @param[in] dmaaddr: DMA address to unmap.
 * @param[in] len: Length of data.
 * @param[out] txdone_pkt_cx: Pointer to struct which has tx done status info.
 *            This struct has flags to indicate tx error, whether DMA address
 *            is mapped from paged/linear buffer, Time stamp availability,
 *            if TS available txdone_pkt_cx->ns stores the time stamp.
 *            Below are the valid bit maps set for txdone_pkt_cx->flags
 *             OSI_TXDONE_CX_PAGED_BUF         OSI_BIT(0)
 *             OSI_TXDONE_CX_ERROR             OSI_BIT(1)
 *             OSI_TXDONE_CX_TS                OSI_BIT(2)
 *
 * @pre Tx completion need to make sure that Tx descriptors processed properly.
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 */
void osd_transmit_complete(void *priv, void *buffer, nveu64_t dmaaddr,
			   nveu32_t len, void *txdone_pkt_cx);
/**
 * @brief osd_log - OSD logging function
 *
 * @param[in] priv: OSD private data
 * @param[in] func: function name
 * @param[in] line: line number
 * @param[in] level: log level
 * @param[in] type: error type
 * @param[in] err:  error string
 * @param[in] loga: error additional information
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: Yes
 */
void osd_log(void *priv,
	     const nve8_t *func,
	     nveu32_t line,
	     nveu32_t level,
	     nveu32_t type,
	     const nve8_t *err,
	     nveul64_t loga);
#endif /* INCLUDED_OSD_H */
