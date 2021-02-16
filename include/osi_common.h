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

#ifndef INCLUDED_OSI_COMMON_H
#define INCLUDED_OSI_COMMON_H

#include "../osi/common/type.h"

/**
 * @addtogroup Helper Helper MACROS
 *
 * @brief EQOS generic helper MACROS.
 * @{
 */
#define OSI_UNLOCKED		0x0U
#define OSI_LOCKED		0x1U
#define OSI_NSEC_PER_SEC	1000000000ULL

#ifndef OSI_STRIPPED_LIB
#define OSI_MAX_RX_COALESCE_USEC	1020U
#define OSI_MIN_RX_COALESCE_USEC	3U
#define OSI_MIN_RX_COALESCE_FRAMES	1U
#define OSI_MAX_TX_COALESCE_USEC	1020U
#define OSI_MIN_TX_COALESCE_USEC	32U
#define OSI_MIN_TX_COALESCE_FRAMES	1U
#endif /* !OSI_STRIPPED_LIB */

/* Compiler hints for branch prediction */
#define osi_unlikely(x)			__builtin_expect(!!(x), 0)
/** @} */

#ifndef OSI_STRIPPED_LIB
/**
 * @addtogroup - LPI-Timers LPI configuration macros
 *
 * @brief LPI timers and config register field masks.
 * @{
 */
/* LPI LS timer - minimum time (in milliseconds) for which the link status from
 * PHY should be up before the LPI pattern can be transmitted to the PHY.
 * Default 1sec.
 */
#define OSI_DEFAULT_LPI_LS_TIMER	(nveu32_t)1000
#define OSI_LPI_LS_TIMER_MASK		0x3FFU
#define OSI_LPI_LS_TIMER_SHIFT		16U
/* LPI TW timer - minimum time (in microseconds) for which MAC wait after it
 * stops transmitting LPI pattern before resuming normal tx.
 * Default 21us
 */
#define OSI_DEFAULT_LPI_TW_TIMER	0x15U
#define OSI_LPI_TW_TIMER_MASK		0xFFFFU
/* LPI entry timer - Time in microseconds that MAC will wait to enter LPI mode
 * after all tx is complete.
 * Default 1sec.
 */
#define OSI_LPI_ENTRY_TIMER_MASK	0xFFFF8U

/* LPI entry timer - Time in microseconds that MAC will wait to enter LPI mode
 * after all tx is complete. Default 1sec.
 */
#define OSI_DEFAULT_TX_LPI_TIMER	0xF4240U

/* Max Tx LPI timer (in usecs) based on the timer value field length in HW
 * MAC_LPI_ENTRY_TIMER register */
#define OSI_MAX_TX_LPI_TIMER		0xFFFF8U

/* Min Tx LPI timer (in usecs) based on the timer value field length in HW
 * MAC_LPI_ENTRY_TIMER register */
#define OSI_MIN_TX_LPI_TIMER		0x8U

/* Time in 1 microseconds tic counter used as reference for all LPI timers.
 * It is clock rate of CSR slave port (APB clock[eqos_pclk] in eqos) minus 1
 * Current eqos_pclk is 204MHz
 */
#define OSI_LPI_1US_TIC_COUNTER_DEFAULT	0xCBU
#define OSI_LPI_1US_TIC_COUNTER_MASK	0xFFFU
/** @} */
#endif /* !OSI_STRIPPED_LIB */

/**
 * @addtogroup Helper Helper MACROS
 *
 * @brief EQOS generic helper MACROS.
 * @{
 */
#ifndef OSI_STRIPPED_LIB
#define OSI_PAUSE_FRAMES_ENABLE		0U
#define OSI_PTP_REQ_CLK_FREQ		250000000U
#define OSI_FLOW_CTRL_DISABLE		0U

#define OSI_ADDRESS_32BIT		0
#define OSI_ADDRESS_40BIT		1
#define OSI_ADDRESS_48BIT		2
#endif /* !OSI_STRIPPED_LIB */

#ifndef UINT_MAX
#define UINT_MAX			(~0U)
#endif
#ifndef INT_MAX
#define INT_MAX				(0x7FFFFFFF)
#endif
/** @} */


/**
 * @addtogroup Helper Helper MACROS
 *
 * @brief EQOS generic helper MACROS.
 * @{
 */
#define OSI_UCHAR_MAX			(0xFFU)

/* Logging defines */
/* log levels */
#define OSI_LOG_ERR			3U
/* Error types */
#define OSI_LOG_ARG_INVALID		2U
#ifndef OSI_STRIPPED_LIB
#define OSI_LOG_WARN			2U
#define OSI_LOG_ARG_OPNOTSUPP		3U
#endif /* !OSI_STRIPPED_LIB */
/* Default maximum Giant Packet Size Limit is 16K */
#define OSI_MAX_MTU_SIZE	16383U

#define EQOS_DMA_CHX_STATUS(x)		((0x0080U * (x)) + 0x1160U)

/* FIXME add logic based on HW version */
#define OSI_EQOS_MAX_NUM_CHANS		4U
#define OSI_EQOS_MAX_NUM_QUEUES		4U

#define MAC_VERSION		0x110
#define MAC_VERSION_SNVER_MASK	0x7FU

#define OSI_MAC_HW_EQOS		0U

#define OSI_NULL                ((void *)0)
#define OSI_ENABLE		1U
#define OSI_NONE		0U
#define OSI_DISABLE		0U

#define OSI_BIT(nr)             ((nveu32_t)1 << (nr))

#define OSI_EQOS_MAC_4_10       0x41U
#define OSI_EQOS_MAC_5_00       0x50U
#define OSI_EQOS_MAC_5_30       0x53U
#define OSI_MAX_VM_IRQS              5U

#ifndef OSI_STRIPPED_LIB
#define OSI_L2_FILTER_INDEX_ANY		127U
#define OSI_HASH_FILTER_MODE	1U
#define OSI_L4_FILTER_TCP	0U
#define OSI_L4_FILTER_UDP	1U
#define OSI_PERFECT_FILTER_MODE	0U

#define NV_ETH_FCS_LEN		0x4U
#define NV_ETH_FRAME_LEN	1514U

#define MAX_ETH_FRAME_LEN_DEFAULT \
	(NV_ETH_FRAME_LEN + NV_ETH_FCS_LEN + NV_VLAN_HLEN)
#define OSI_INVALID_CHAN_NUM    0xFFU
#endif /* OSI_STRIPPED_LIB */

/** @} */

#ifndef OSI_STRIPPED_LIB
/**
 * @addtogroup MTL queue operation mode
 *
 * @brief MTL queue operation mode options
 * @{
 */
#define OSI_MTL_QUEUE_DISABLED	0x0U
#define OSI_MTL_QUEUE_AVB	0x1U
#define OSI_MTL_QUEUE_ENABLE	0x2U
#define OSI_MTL_QUEUE_MODEMAX	0x3U
/** @} */

/**
 * @addtogroup EQOS_MTL MTL queue AVB algorithm mode
 *
 * @brief MTL AVB queue algorithm type
 * @{
 */
#define OSI_MTL_TXQ_AVALG_CBS	1U
#define OSI_MTL_TXQ_AVALG_SP	0U
/** @} */
#endif /* OSI_STRIPPED_LIB */

#endif /* OSI_COMMON_H */
