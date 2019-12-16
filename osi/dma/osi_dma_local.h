/*
 * Copyright (c) 2019-2020, NVIDIA CORPORATION. All rights reserved.
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


#ifndef INCLUDED_OSI_DMA_LOCAL_H
#define INCLUDED_OSI_DMA_LOCAL_H

#include <osi_dma.h>
#include "eqos_dma.h"
#include "mgbe_dma.h"

/**
 * @brief DMA descriptor operations
 */
struct desc_ops {
	/** Called to get receive checksum */
	void (*get_rx_csum)(struct osi_rx_desc *rx_desc,
			    struct osi_rx_pkt_cx *rx_pkt_cx);
	/** Called to get rx error stats */
	void (*update_rx_err_stats)(struct osi_rx_desc *rx_desc,
				    struct osi_pkt_err_stats pkt_err_stats);
	/** Called to get rx VLAN from descriptor */
	void (*get_rx_vlan)(struct osi_rx_desc *rx_desc,
			    struct osi_rx_pkt_cx *rx_pkt_cx);
	/** Called to get rx HASH from descriptor */
	void (*get_rx_hash)(struct osi_rx_desc *rx_desc,
			    struct osi_rx_pkt_cx *rx_pkt_cx);
};

/**
 * @brief eqos_get_desc_ops - EQOS get DMA descriptor operations
 *
 * Algorithm: Returns pointer DMA descriptor operations structure.
 *
 * @returns Pointer to DMA descriptor operations structure
 */
struct desc_ops *eqos_get_desc_ops(void);

/**
 * @brief mgbe_get_desc_ops - MGBE get DMA descriptor operations
 *
 * Algorithm: Returns pointer DMA descriptor operations structure.
 *
 * @returns Pointer to DMA descriptor operations structure
 */
struct desc_ops *mgbe_get_desc_ops(void);

/* Function prototype needed for misra */

/**
 * @brief dma_desc_init - Initialize DMA Tx/Rx descriptors
 *
 * @note
 * Algorithm:
 *  - Transmit and Receive descriptors will be initialized with
 *    required values so that MAC DMA can understand and act accordingly.
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
nve32_t dma_desc_init(struct osi_dma_priv_data *osi_dma);

/**
 * @addtogroup Helper Helper MACROS
 *
 * @brief EQOS generic helper MACROS.
 * @{
 */
#define CHECK_CHAN_BOUND(chan)						\
	{								\
		if ((chan) >= OSI_EQOS_MAX_NUM_CHANS) {			\
			return;						\
		}							\
	}								\

#define BOOLEAN_FALSE		(0U != 0U)
#define L32(data)       ((data) & 0xFFFFFFFFU)
#define H32(data)       (((data) & 0xFFFFFFFF00000000UL) >> 32UL)
/** @} */

#endif /* INCLUDED_OSI_DMA_LOCAL_H */
