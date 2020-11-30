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

#ifndef INCLUDED_OSI_CORE_H
#define INCLUDED_OSI_CORE_H

#include <osi_common.h>
#include "mmc.h"

/**
 * @addtogroup typedef related info
 *
 * @brief typedefs that indicate size and signness
 * @{
 */
/* Following added to avoid misraC 4.6
 * Here we are defining intermediate type
 */
/** intermediate type for unsigned short */
typedef unsigned short		my_uint16_t;
/** intermediate type for long long */
typedef long long		my_lint_64;

/* Actual type used in code */
/** typedef equivalent to unsigned short */
typedef my_uint16_t		nveu16_t;
/** typedef equivalent to long long */
typedef my_lint_64		nvel64_t;
/** @} */

/**
 * @addtogroup PTP related information
 *
 * @brief PTP SSINC values
 * @{
 */
#define OSI_PTP_SSINC_16	16U
#define OSI_PTP_SSINC_4		4U

/** @} */

/**
 * @addtogroup EQOS_PTP PTP Helper MACROS
 *
 * @brief EQOS PTP MAC Time stamp control reg bit fields
 * @{
 */
#define OSI_MAC_TCR_TSENA		OSI_BIT(0)
#define OSI_MAC_TCR_TSCFUPDT		OSI_BIT(1)
#define OSI_MAC_TCR_TSENALL		OSI_BIT(8)
#define OSI_MAC_TCR_TSCTRLSSR		OSI_BIT(9)
#define OSI_MAC_TCR_TSVER2ENA		OSI_BIT(10)
#define OSI_MAC_TCR_TSIPENA		OSI_BIT(11)
#define OSI_MAC_TCR_TSIPV6ENA		OSI_BIT(12)
#define OSI_MAC_TCR_TSIPV4ENA		OSI_BIT(13)
#define OSI_MAC_TCR_TSEVENTENA		OSI_BIT(14)
#define OSI_MAC_TCR_TSMASTERENA		OSI_BIT(15)
#define OSI_MAC_TCR_SNAPTYPSEL_1	OSI_BIT(16)
#define OSI_MAC_TCR_SNAPTYPSEL_2	OSI_BIT(17)
#define OSI_MAC_TCR_CSC			OSI_BIT(19)
#define OSI_MAC_TCR_AV8021ASMEN		OSI_BIT(28)
#ifndef OSI_STRIPPED_LIB
#define OSI_MAC_TCR_SNAPTYPSEL_3	(OSI_BIT(16) | OSI_BIT(17))
#endif /* !OSI_STRIPPED_LIB */
/** @} */

/**
 * @addtogroup Helper Helper MACROS
 *
 * @brief EQOS generic helper MACROS.
 * @{
 */
#define EQOS_DMA_CHX_IER(x)		((0x0080U * (x)) + 0x1134U)
#define EQOS_MAX_MAC_ADDRESS_FILTER	128U
#define EQOS_MAX_L3_L4_FILTER		8U
#define EQOS_MAX_HTR_REGS		8U
#define OSI_MGBE_MAX_MAC_ADDRESS_FILTER	32U
#define OSI_DA_MATCH			0U
#define OSI_INV_MATCH			1U
#define OSI_AMASK_DISABLE		0U
#define OSI_CHAN_ANY			0xFFU
#define OSI_MAX_TC_NUM			8U
#define OSI_DFLT_MTU_SIZE		1500U
#define OSI_MTU_SIZE_9000		9000U
/* HW supports 8 Hash table regs, but eqos_validate_core_regs only checks 4 */
#define OSI_EQOS_MAX_HASH_REGS		4U
#define OSI_ETH_ALEN			6U

#define OSI_FLOW_CTRL_TX		OSI_BIT(0)
#define OSI_FLOW_CTRL_RX		OSI_BIT(1)

#define OSI_FULL_DUPLEX			1
#define OSI_HALF_DUPLEX			0

#define OSI_IP4_FILTER			0U
#define OSI_IP6_FILTER			1U
#define OSI_IPV6_MATCH			1U
#define OSI_IPV4_MATCH			0U

/* L2 filter operations supported by OSI layer. These operation modes shall be
 * set by OSD driver as input to update registers accordingly.
 */
#define OSI_OPER_EN_PROMISC		OSI_BIT(0)
#define OSI_OPER_DIS_PROMISC		OSI_BIT(1)
#define OSI_OPER_EN_ALLMULTI		OSI_BIT(2)
#define OSI_OPER_DIS_ALLMULTI		OSI_BIT(3)
#define OSI_OPER_EN_L2_DA_INV		OSI_BIT(4)
#define OSI_OPER_DIS_L2_DA_INV		OSI_BIT(5)
#define OSI_OPER_EN_PERFECT		OSI_BIT(6)
#define OSI_OPER_DIS_PERFECT		OSI_BIT(7)
#define OSI_OPER_ADDR_UPDATE		OSI_BIT(8)
#define OSI_OPER_ADDR_DEL		OSI_BIT(9)

#define OSI_PAUSE_FRAMES_DISABLE	1U
#define OSI_PFT_MATCH		0U
#define OSI_SOURCE_MATCH	0U
#define OSI_SA_MATCH		1U

#define OSI_SPEED_10		10
#define OSI_SPEED_100		100
#define OSI_SPEED_1000		1000
#define OSI_SPEED_2500		2500
#define OSI_SPEED_5000		5000
#define OSI_SPEED_10000		10000

#define TEN_POWER_9		0x3B9ACA00U
#define TWO_POWER_32		0x100000000ULL
#define TWO_POWER_31		0x80000000U
/** @} */

/**
 * @addtogroup PTP-offload PTP offload defines
 * @{
 */
#define OSI_PTP_SNAP_ORDINARY	0U
#define OSI_PTP_SNAP_TRANSPORT	1U
#define OSI_PTP_SNAP_P2P	3U
#define OSI_PTP_MAX_PORTID	0xFFFFU
#define OSI_PTP_MAX_DOMAIN	0xFFU
/** @} */

/**
 * @brief OSI error macro definition,
 * @param[in] priv: OSD private data OR NULL
 * @param[in] type: error type
 * @param[in] err:  error string
 * @param[in] loga: error additional information
 */
#define OSI_CORE_ERR(priv, type, err, loga)			\
{								\
	osi_core->osd_ops.ops_log(priv, __func__, __LINE__,	\
				  OSI_LOG_ERR, type, err, loga);\
}

/**
 * @brief OSI info macro definition
 * @param[in] priv: OSD private data OR NULL
 * @param[in] type: error type
 * @param[in] err:  error string
 * @param[in] loga: error additional information
 */
#define OSI_CORE_INFO(priv, type, err, loga)				\
{									\
	osi_core->osd_ops.ops_log(priv, __func__, __LINE__,		\
				  OSI_LOG_INFO, type, err, loga);	\
}

#define VLAN_NUM_VID		4096U
#define OSI_VLAN_ACTION_ADD	OSI_BIT(31)
#define OSI_VLAN_ACTION_DEL	0x0U
#define OSI_RXQ_ROUTE_PTP	0U
#define OSI_DELAY_1000US	1000U
#define OSI_DELAY_1US		1U
/**
 * @addtogroup RSS related information
 *
 * @brief RSS hash key and table size.
 * @{
 */
#define OSI_RSS_HASH_KEY_SIZE	40U
#define OSI_RSS_MAX_TABLE_SIZE	128U
/** @} */

/**
 * @addtogroup PTP related information
 *
 * @brief PTP SSINC values
 * @{
 */
#define OSI_PTP_SSINC_16	16U
#define OSI_PTP_SSINC_4		4U
/** @} */

/**
 * @addtogroup Flexible Receive Parser related information
 *
 * @brief Flexible Receive Parser commands, table size and other defines
 * @{
 */
#define OSI_FRP_MAX_ENTRY		256U
#define OSI_FRP_OFFSET_MAX		64U
/* FRP Command types */
#define OSI_FRP_CMD_ADD			0U
#define OSI_FRP_CMD_UPDATE		1U
#define OSI_FRP_CMD_DEL			2U
#define OSI_FRP_CMD_MAX			3U
/* FRP Filter mode defines */
#define OSI_FRP_MODE_ROUTE		0U
#define OSI_FRP_MODE_DROP		1U
#define OSI_FRP_MODE_BYPASS		2U
#define OSI_FRP_MODE_LINK		3U
#define OSI_FRP_MODE_IM_ROUTE		4U
#define OSI_FRP_MODE_IM_DROP		5U
#define OSI_FRP_MODE_IM_BYPASS		6U
#define OSI_FRP_MODE_IM_LINK		7U
#define OSI_FRP_MODE_MAX		8U
/* Match data defines */
#define OSI_FRP_MATCH_DATA_MAX		12U
#define OSI_FRP_MATCH_NORMAL		0U
#define OSI_FRP_MATCH_L2_DA		1U
#define OSI_FRP_MATCH_L2_SA		2U
#define OSI_FRP_MATCH_L3_SIP		3U
#define OSI_FRP_MATCH_L3_DIP		4U
#define OSI_FRP_MATCH_L4_S_UPORT	5U
#define OSI_FRP_MATCH_L4_D_UPORT	6U
#define OSI_FRP_MATCH_L4_S_TPORT	7U
#define OSI_FRP_MATCH_L4_D_TPORT	8U
#define OSI_FRP_MATCH_VLAN		9U
#define OSI_FRP_MATCH_MAX		10U
/** @} */

struct osi_core_priv_data;

/**
 * @brief OSI core structure for filters
 */
struct osi_filter {
	/** indicates operation needs to perform. refer to OSI_OPER_* */
	nveu32_t oper_mode;
	/** Indicates the index of the filter to be modified.
	 * Filter index must be between 0 - 127 */
	nveu32_t index;
	/** Ethernet MAC address to be added */
	nveu8_t mac_address[OSI_ETH_ALEN];
	/** Indicates dma channel routing enable(1) disable (0) */
	nveu32_t dma_routing;
	/**  indicates dma channel number to program */
	nveu32_t dma_chan;
	/** filter will not consider byte in comparison
	 *	Bit 5: MAC_Address${i}_High[15:8]
	 *	Bit 4: MAC_Address${i}_High[7:0]
	 *	Bit 3: MAC_Address${i}_Low[31:24]
	 *	..
	 *	Bit 0: MAC_Address${i}_Low[7:0] */
	nveu32_t addr_mask;
	/** src_dest: SA(1) or DA(0) */
	nveu32_t src_dest;
	/**  indicates one hot encoded DMA receive channels to program */
	nveu32_t dma_chansel;
};

/**
 * @brief OSI core structure for RXQ route
 */
struct osi_rxq_route {
#define OSI_RXQ_ROUTE_PTP	0U
	/** Indicates RX routing type OSI_RXQ_ROUTE_* */
	unsigned int route_type;
	/** RXQ routing enable(1) disable (0) */
	unsigned int enable;
	/** RX queue index */
	unsigned int idx;
};

/**
 * @brief L3/L4 filter function dependent parameter
 */
struct osi_l3_l4_filter {
	/** Indicates the index of the filter to be modified.
	 * Filter index must be between 0 - 7 */
	nveu32_t filter_no;
	/** filter enable(1) or disable(0) */
	nveu32_t filter_enb_dis;
	/** source(0) or destination(1) */
	nveu32_t src_dst_addr_match;
	/** perfect(0) or inverse(1) */
	nveu32_t perfect_inverse_match;
	/** ipv4 address */
	nveu8_t ip4_addr[4];
	/** ipv6 address */
	nveu16_t ip6_addr[8];
	/** Port number */
	nveu16_t port_no;
};

/**
 * @brief struct osi_hw_features - MAC HW supported features.
 */
struct osi_hw_features {
	/** It is set to 1 when 10/100 Mbps is selected as the Mode of
	 * Operation */
	nveu32_t mii_sel;
	/** It is set to 1 when the RGMII Interface option is selected */
	nveu32_t rgmii_sel;
	/** It is set to 1 when the RMII Interface option is selected */
	nveu32_t rmii_sel;
	/** It sets to 1 when 1000 Mbps is selected as the Mode of Operation */
	nveu32_t gmii_sel;
	/** It sets to 1 when the half-duplex mode is selected */
	nveu32_t hd_sel;
	/** It sets to 1 when the TBI, SGMII, or RTBI PHY interface
	 * option is selected */
	nveu32_t pcs_sel;
	/** It sets to 1 when the Enable VLAN Hash Table Based Filtering
	 * option is selected */
	nveu32_t vlan_hash_en;
	/** It sets to 1 when the Enable Station Management (MDIO Interface)
	 * option is selected */
	nveu32_t sma_sel;
	/** It sets to 1 when the Enable Remote Wake-Up Packet Detection
	 * option is selected */
	nveu32_t rwk_sel;
	/** It sets to 1 when the Enable Magic Packet Detection option is
	 * selected */
	nveu32_t mgk_sel;
	/** It sets to 1 when the Enable MAC Management Counters (MMC) option
	 * is selected */
	nveu32_t mmc_sel;
	/** It sets to 1 when the Enable IPv4 ARP Offload option is selected */
	nveu32_t arp_offld_en;
	/** It sets to 1 when the Enable IEEE 1588 Timestamp Support option
	 * is selected */
	nveu32_t ts_sel;
	/** It sets to 1 when the Enable Energy Efficient Ethernet (EEE) option
	 * is selected */
	nveu32_t eee_sel;
	/** It sets to 1 when the Enable Transmit TCP/IP Checksum Insertion
	 * option is selected */
	nveu32_t tx_coe_sel;
	/** It sets to 1 when the Enable Receive TCP/IP Checksum Check option
	 * is selected */
	nveu32_t rx_coe_sel;
	/** It sets to 1 when the Enable Additional 1-31 MAC Address Registers
	 * option is selected */
	nveu32_t mac_addr_sel;
	/** It sets to 1 when the Enable Additional 32-63 MAC Address Registers
	 * option is selected */
	nveu32_t mac_addr32_sel;
	/** It sets to 1 when the Enable Additional 64-127 MAC Address Registers
	 * option is selected */
	nveu32_t mac_addr64_sel;
	/** It sets to 1 when the Enable IEEE 1588 Timestamp Support option
	 * is selected */
	nveu32_t tsstssel;
	/** It sets to 1 when the Enable SA and VLAN Insertion on Tx option
	 * is selected */
	nveu32_t sa_vlan_ins;
	/** Active PHY Selected
	 * When you have multiple PHY interfaces in your configuration,
	 * this field indicates the sampled value of phy_intf_sel_i during
	 * reset de-assertion:
	 * 000: GMII or MII
	 * 001: RGMII
	 * 010: SGMII
	 * 011: TBI
	 * 100: RMII
	 * 101: RTBI
	 * 110: SMII
	 * 111: RevMII
	 * All Others: Reserved */
	nveu32_t act_phy_sel;
	/** MTL Receive FIFO Size
	 * This field contains the configured value of MTL Rx FIFO in bytes
	 * expressed as Log to base 2 minus 7, that is, Log2(RXFIFO_SIZE) -7:
	 * 00000: 128 bytes
	 * 00001: 256 bytes
	 * 00010: 512 bytes
	 * 00011: 1,024 bytes
	 * 00100: 2,048 bytes
	 * 00101: 4,096 bytes
	 * 00110: 8,192 bytes
	 * 00111: 16,384 bytes
	 * 01000: 32,767 bytes
	 * 01000: 32 KB
	 * 01001: 64 KB
	 * 01010: 128 KB
	 * 01011: 256 KB
	 * 01100-11111: Reserved */
	nveu32_t rx_fifo_size;
	/** MTL Transmit FIFO Size.
	 * This field contains the configured value of MTL Tx FIFO in
	 * bytes expressed as Log to base 2 minus 7, that is,
	 * Log2(TXFIFO_SIZE) -7:
	 * 00000: 128 bytes
	 * 00001: 256 bytes
	 * 00010: 512 bytes
	 * 00011: 1,024 bytes
	 * 00100: 2,048 bytes
	 * 00101: 4,096 bytes
	 * 00110: 8,192 bytes
	 * 00111: 16,384 bytes
	 * 01000: 32 KB
	 * 01001: 64 KB
	 * 01010: 128 KB
	 * 01011-11111: Reserved */
	nveu32_t tx_fifo_size;
	/** It set to 1 when Advance timestamping High Word selected */
	nveu32_t adv_ts_hword;
	/** Address Width.
	 * This field indicates the configured address width:
	 * 00: 32
	 * 01: 40
	 * 10: 48
	 * 11: Reserved */
	nveu32_t addr_64;
	/** It sets to 1 when DCB Feature Enable */
	nveu32_t dcb_en;
	/** It sets to 1 when Split Header Feature Enable */
	nveu32_t sph_en;
	/** It sets to 1 when TCP Segmentation Offload Enable */
	nveu32_t tso_en;
	/** It sets to 1 when DMA debug registers are enabled */
	nveu32_t dma_debug_gen;
	/** It sets to 1 if AV Feature Enabled */
	nveu32_t av_sel;
	/** It sets to 1 if Receive side AV Feature Enabled */
	nveu32_t rav_sel;
	/** This field indicates the size of the hash table:
	 * 00: No hash table
	 * 01: 64
	 * 10: 128
	 * 11: 256 */
	nveu32_t hash_tbl_sz;
	/** This field indicates the total number of L3 or L4 filters:
	 * 0000: No L3 or L4 Filter
	 * 0001: 1 L3 or L4 Filter
	 * 0010: 2 L3 or L4 Filters
	 * ..
	 * 1000: 8 L3 or L4 */
	nveu32_t l3l4_filter_num;
	/** It holds number of MTL Receive Queues */
	nveu32_t rx_q_cnt;
	/** It holds number of MTL Transmit Queues */
	nveu32_t tx_q_cnt;
	/** It holds number of DMA Receive channels */
	nveu32_t rx_ch_cnt;
	/** This field indicates the number of DMA Transmit channels:
	 * 0000: 1 DMA Tx Channel
	 * 0001: 2 DMA Tx Channels
	 * ..
	 * 0111: 8 DMA Tx */
	nveu32_t tx_ch_cnt;
	/** This field indicates the number of PPS outputs:
	 * 000: No PPS output
	 * 001: 1 PPS output
	 * 010: 2 PPS outputs
	 * 011: 3 PPS outputs
	 * 100: 4 PPS outputs
	 * 101-111: Reserved */
	nveu32_t pps_out_num;
	/** Number of Auxiliary Snapshot Inputs
	 * This field indicates the number of auxiliary snapshot inputs:
	 * 000: No auxiliary input
	 * 001: 1 auxiliary input
	 * 010: 2 auxiliary inputs
	 * 011: 3 auxiliary inputs
	 * 100: 4 auxiliary inputs
	 * 101-111: Reserved */
	nveu32_t aux_snap_num;
	/** VxLAN/NVGRE Support */
	nveu32_t vxn;
	/** Enhanced DMA.
	 * This bit is set to 1 when the "Enhanced DMA" option is
	 * selected. */
	nveu32_t edma;
	/** Different Descriptor Cache
	 * When set to 1, then EDMA mode Separate Memory is
	 * selected for the Descriptor Cache.*/
	nveu32_t ediffc;
	/** PFC Enable
	 * This bit is set to 1 when the Enable PFC Feature is selected */
	nveu32_t pfc_en;
	/** One-Step Timestamping Enable */
	nveu32_t ost_en;
	/** PTO Offload Enable */
	nveu32_t pto_en;
	/** Receive Side Scaling Enable */
	nveu32_t rss_en;
	/** Number of Traffic Classes */
	nveu32_t num_tc;
	/** Number of Extended VLAN Tag Filters Enabled */
	nveu32_t num_vlan_filters;
	/** Supported Flexible Receive Parser.
	 * This bit is set to 1 when the Enable Flexible Programmable
	 * Receive Parser option is selected */
	nveu32_t frp_sel;
	/** Queue/Channel based VLAN tag insertion on Tx Enable
	 * This bit is set to 1 when the Enable Queue/Channel based
	 * VLAN tag insertion on Tx Feature is selected. */
	nveu32_t cbti_sel;
	/** Supported Parallel Instruction Processor Engines (PIPEs)
	 * This field indicates the maximum number of Instruction
	 * Processors supported by flexible receive parser. */
	nveu32_t num_frp_pipes;
	/** One Step for PTP over UDP/IP Feature Enable
	 * This bit is set to 1 when the Enable One step timestamp for
	 * PTP over UDP/IP feature is selected */
	nveu32_t ost_over_udp;
	/** Supported Flexible Receive Parser Parsable Bytes
	 * This field indicates the supported Max Number of bytes of the
	 * packet data to be Parsed by Flexible Receive Parser */
	nveu32_t max_frp_bytes;
	/** Supported Flexible Receive Parser Instructions
	 * This field indicates the Max Number of Parser Instructions
	 * supported by Flexible Receive Parser */
	nveu32_t max_frp_entries;
	/** Double VLAN Processing Enabled
	 * This bit is set to 1 when the Enable Double VLAN Processing
	 * feature is selected */
	nveu32_t double_vlan_en;
	/** Automotive Safety Package
	 * Following are the encoding for the different Safety features
	 * Values:
	 * 0x0 (NONE): No Safety features selected
	 * 0x1 (ECC_ONLY): Only "ECC protection for external
	 * memory" feature is selected
	 * 0x2 (AS_NPPE): All the Automotive Safety features are
	 * selected without the "Parity Port Enable for external interface"
	 * feature
	 * 0x3 (AS_PPE): All the Automotive Safety features are
	 * selected with the "Parity Port Enable for external interface"
	 * feature */
	nveu32_t auto_safety_pkg;
	/** Tx Timestamp FIFO Depth
	 * This value indicates the depth of the Tx Timestamp FIFO
	 * 3'b000: Reserved
	 * 3'b001: 1
	 * 3'b010: 2
	 * 3'b011: 4
	 * 3'b100: 8
	 * 3'b101: 16
	 * 3'b110: Reserved
	 * 3'b111: Reserved */
	nveu32_t tts_fifo_depth;
	/** Enhancements to Scheduling Traffic Enable
	 * This bit is set to 1 when the Enable Enhancements to
	 * Scheduling Traffic feature is selected.
	 * Values:
	 * 0x0 (INACTIVE): Enable Enhancements to Scheduling
	 * Traffic feature is not selected
	 * 0x1 (ACTIVE): Enable Enhancements to Scheduling
	 * Traffic feature is selected */
	nveu32_t est_sel;
	/** Depth of the Gate Control List
	 * This field indicates the depth of Gate Control list expressed as
	 * Log2(DWCXG_GCL_DEP)-5
	 * Values:
	 * 0x0 (NODEPTH): No Depth configured
	 * 0x1 (DEPTH64): 64
	 * 0x2 (DEPTH128): 128
	 * 0x3 (DEPTH256): 256
	 * 0x4 (DEPTH512): 512
	 * 0x5 (DEPTH1024): 1024
	 * 0x6 (RSVD): Reserved */
	nveu32_t gcl_depth;
	/** Width of the Time Interval field in the Gate Control List
	 * This field indicates the width of the Configured Time Interval
	 * Field
	 * Values:
	 * 0x0 (NOWIDTH): Width not configured
	 * 0x1 (WIDTH16): 16
	 * 0x2 (WIDTH20): 20
	 * 0x3 (WIDTH24): 24 */
	nveu32_t gcl_width;
	/** Frame Preemption Enable
	 * This bit is set to 1 when the Enable Frame preemption feature
	 * is selected.
	 * Values:
	 * 0x0 (INACTIVE): Frame Preemption Enable feature is not
	 * selected
	 * 0x1 (ACTIVE): Frame Preemption Enable feature is
	 * selected */
	nveu32_t fpe_sel;
	/** Time Based Scheduling Enable
	 * This bit is set to 1 when the Time Based Scheduling feature is
	 * selected.
	 * Values:
	 * 0x0 (INACTIVE): Time Based Scheduling Enable feature is
	 * not selected
	 * 0x1 (ACTIVE): Time Based Scheduling Enable feature is
	 * selected */
	nveu32_t tbs_sel;
	/** The number of DMA channels enabled for TBS (starting from
	 * the highest Tx Channel in descending order)
	 * This field provides the number of DMA channels enabled for
	 * TBS (starting from the highest Tx Channel in descending
	 * order):
	 * 0000: 1 DMA Tx Channel enabled for TBS
	 * 0001: 2 DMA Tx Channels enabled for TBS
	 * 0010: 3 DMA Tx Channels enabled for TBS
	 * ...
	 * 1111: 16 DMA Tx Channels enabled for TBS */
	nveu32_t num_tbs_ch;
};

#ifndef OSI_STRIPPED_LIB
/**
 * @brief Vlan filter Function dependent parameter
 */
struct osi_vlan_filter {
	/** vlan filter enable(1) or disable(0) */
	nveu32_t filter_enb_dis;
	/** perfect(0) or hash(1) */
	nveu32_t perfect_hash;
	/** perfect(0) or inverse(1) */
	nveu32_t perfect_inverse_match;
};

/**
 * @brief FRP Instruction configuration structure
 */
struct osi_core_frp_data {
	/* Entry Match Data */
	unsigned int match_data;
	/* Entry Match Enable mask */
	unsigned int match_en;
	/* Entry Accept frame flag */
	unsigned char accept_frame;
	/* Entry Reject Frame flag */
	unsigned char reject_frame;
	/* Entry Inverse match flag */
	unsigned char inverse_match;
	/* Entry Next Instruction Control match flag */
	unsigned char next_ins_ctrl;
	/* Entry Frame offset in the packet data */
	unsigned char frame_offset;
	/* Entry OK Index - Next Instruction */
	unsigned char ok_index;
	/* Entry DMA Channel selection (1-bit for each channel) */
	unsigned int dma_chsel;
};

/**
 * @brief FRP command structure for OSD to OSI
 */
struct osi_core_frp_cmd {
	/* FRP Command type */
	unsigned int cmd;
	/* OSD FRP ID */
	int frp_id;
	/* OSD match data type */
	unsigned char match_type;
	/* OSD match data */
	unsigned char match[OSI_FRP_MATCH_DATA_MAX];
	/* OSD match data length */
	unsigned char match_length;
	/* OSD Offset */
	unsigned char offset;
	/* OSD FRP filter mode flag */
	unsigned char filter_mode;
	/* OSD FRP Link ID */
	int next_frp_id;
	/* OSD DMA Channel Selection */
	unsigned int dma_sel;
};

/**
 * @brief FRP Instruction table entry configuration structure
 */
struct osi_core_frp_entry {
	/* FRP ID */
	int frp_id;
	/* FRP Entry data structure */
	struct osi_core_frp_data data;
};

/**
 * @brief L2 filter function dependent parameter
 */
struct osi_l2_da_filter {
	/** perfect(0) or hash(1) */
	nveu32_t perfect_hash;
	/** perfect(0) or inverse(1) */
	nveu32_t perfect_inverse_match;
};

/**
 * @brief OSI Core avb data structure per queue.
 */
struct  osi_core_avb_algorithm {
	/** TX Queue/TC index */
	nveu32_t qindex;
	/** CBS Algorithm enable(1) or disable(0) */
	nveu32_t algo;
	/** When this bit is set, the accumulated credit parameter in the
	 * credit-based shaper algorithm logic is not reset to zero when
	 * there is positive credit and no packet to transmit in Channel.
	 *
	 * Expected values are enable(1) or disable(0) */
	nveu32_t credit_control;
	/** idleSlopeCredit value required for CBS */
	  nveu32_t idle_slope;
	/** sendSlopeCredit value required for CBS */
	nveu32_t send_slope;
	/** hiCredit value required for CBS */
	nveu32_t hi_credit;
	/** lowCredit value required for CBS */
	nveu32_t low_credit;
	/** Transmit queue operating mode
	 *
	 * 00: disable
	 *
	 * 01: avb
	 *
	 * 10: enable */
	nveu32_t oper_mode;
	/** TC index */
	unsigned int tcindex;
};
#endif /* !OSI_STRIPPED_LIB */

/**
 * @brief struct ptp_offload_param - Parameter to support PTP offload.
 */
struct osi_pto_config {
	/** enable(0) / disable(1) */
	unsigned int en_dis;
	/** Flag for Master mode.
	 * OSI_ENABLE for master OSI_DISABLE for slave */
	unsigned int master;
	/** Flag to Select PTP packets for Taking Snapshots */
	unsigned int snap_type;
	/** ptp domain */
	unsigned int domain_num;
	/**  The PTP Offload function qualifies received PTP
	 *  packet with unicast Destination  address
	 *  0 - only multicast, 1 - unicast and multicast */
	unsigned int mc_uc;
	/** Port identification */
	unsigned int portid;
};

/**
 * @brief OSI Core EST structure
 */
struct osi_est_config {
	/** enable/disable */
	unsigned int en_dis;
	/** 64 bit base time register
	 * if both vlaues are 0, take ptp time to avoid BTRE
	 * index 0 for nsec, index 1 for sec
	 */
	unsigned int btr[2];
	/** 64 bit base time offset index 0 for nsec, index 1 for sec */
	unsigned int btr_offset[2];
	/** 40 bit cycle time register, index 0 for nsec, index 1 for sec */
	unsigned int ctr[2];
	/** Configured Time Interval width + 7 bit extension register */
	unsigned int ter;
	/** size of the gate control list */
	unsigned int llr;
	/** data array 8 bit gate op + 24 execution time
	 * MGBE HW support GCL depth 256 */
	unsigned int gcl[OSI_GCL_SIZE_256];
};

/**
 * @brief OSI Core FPE structure
 */
struct osi_fpe_config {
	/** Queue Mask 1 preemption 0- express bit representation */
	unsigned int tx_queue_preemption_enable;
	/** RQ for all preemptable packets  which are not filtered
	 * based on user priority or SA-DA
	 */
	unsigned int rq;
};

/**
 * @brief OSI Core TSN error stats structure
 */
struct osi_tsn_stats {
	/** Constant Gate Control Error */
	unsigned long const_gate_ctr_err;
	/** Head-Of-Line Blocking due to Scheduling */
	unsigned long head_of_line_blk_sch;
	/** Per TC Schedule Error */
	unsigned long hlbs_q[OSI_MAX_TC_NUM];
	/** Head-Of-Line Blocking due to Frame Size */
	unsigned long head_of_line_blk_frm;
	/** Per TC Frame Size Error */
	unsigned long hlbf_q[OSI_MAX_TC_NUM];
	/** BTR Error */
	unsigned long base_time_reg_err;
	/** Switch to Software Owned List Complete */
	unsigned long sw_own_list_complete;
};

/**
 * @brief PTP configuration structure
 */
struct osi_ptp_config {
	/** PTP filter parameters bit fields.
	 * 
	 * Enable Timestamp, Fine Timestamp, 1 nanosecond accuracy
	 * are enabled by default.
	 *
	 * Need to set below bit fields accordingly as per the requirements.
	 *
	 * Enable Timestamp for All Packets			OSI_BIT(8)
	 *
	 * Enable PTP Packet Processing for Version 2 Format	OSI_BIT(10)
	 *
	 * Enable Processing of PTP over Ethernet Packets	OSI_BIT(11)
	 *
	 * Enable Processing of PTP Packets Sent over IPv6-UDP	OSI_BIT(12)
	 *
	 * Enable Processing of PTP Packets Sent over IPv4-UDP	OSI_BIT(13)
	 *
	 * Enable Timestamp Snapshot for Event Messages		OSI_BIT(14)
	 *
	 * Enable Snapshot for Messages Relevant to Master	OSI_BIT(15)
	 *
	 * Select PTP packets for Taking Snapshots		OSI_BIT(16)
	 *
	 * Select PTP packets for Taking Snapshots		OSI_BIT(17)
	 *
	 * Select PTP packets for Taking Snapshots (OSI_BIT(16) + OSI_BIT(17))
	 *
	 * AV 802.1AS Mode Enable				OSI_BIT(28)
	 * 
	 * if ptp_filter is set to Zero then Time stamping is disabled */
	nveu32_t ptp_filter;
	/** seconds to be updated to MAC */
	nveu32_t sec;
	/** nano seconds to be updated to MAC */
	nveu32_t nsec;
	/** PTP reference clock read from DT */
	nveu32_t ptp_ref_clk_rate;
	/** Use one nsec accuracy (need to set 1) */
	nveu32_t one_nsec_accuracy;
	/** PTP system clock which is 62500000Hz */
	nveu32_t ptp_clock;
	/** PTP Packets RX Queue.*/
	nveu32_t ptp_rx_queue;
};

/**
 * @brief osi_core_rss - Struture used to store RSS Hash key and table
 * information.
 */
struct osi_core_rss {
	/** Flag to represent to enable RSS or not */
	unsigned int enable;
	/** Array for storing RSS Hash key */
	unsigned char key[OSI_RSS_HASH_KEY_SIZE];
	/** Array for storing RSS Hash table */
	unsigned int table[OSI_RSS_MAX_TABLE_SIZE];
};

/**
 * @brief Max num of MAC core registers to backup. It should be max of or >=
 * (EQOS_MAX_BAK_IDX=380, coreX,...etc) backup registers.
 */
#define CORE_MAX_BAK_IDX	700U

/**
 * @brief core_backup - Struct used to store backup of core HW registers.
 */
struct core_backup {
	/** Array of reg MMIO addresses (base of MAC + offset of reg) */
	void *reg_addr[CORE_MAX_BAK_IDX];
	/** Array of value stored in each corresponding register */
	nveu32_t reg_val[CORE_MAX_BAK_IDX];
};

/**
 *@brief OSD Core callbacks
 */
struct osd_core_ops {
	/** logging callback */
	void (*ops_log)(void *priv, const nve8_t *func, nveu32_t line,
			nveu32_t level, nveu32_t type, const nve8_t *err,
			nveul64_t loga);
	/** udelay callback */
	void (*udelay)(nveu64_t usec);
	/** usleep range callback */
	void (*usleep_range)(nveu64_t umin, nveu64_t umax);
	/** msleep callback */
	void (*msleep)(nveu32_t msec);
	/** ivcsend callback*/
	nve32_t (*ivc_send)(void *priv, void *data, nveu32_t len);
};

#ifdef MACSEC_SUPPORT
/**
 * @brief MACsec interrupt stats structure.
 */
struct osi_macsec_irq_stats {
	/** Tx debug buffer capture done */
	unsigned long tx_dbg_capture_done;
	/** Tx MTU check failed */
	unsigned long tx_mtu_check_fail;
	/** Tx MAC CRC err */
	unsigned long tx_mac_crc_error;
	/** Tx SC AN not valid */
	unsigned long tx_sc_an_not_valid;
	/** Tx AES GCM buffer overflow */
	unsigned long tx_aes_gcm_buf_ovf;
	/** Tx LUT lookup miss */
	unsigned long tx_lkup_miss;
	/** Tx uninitialized key slot */
	unsigned long tx_uninit_key_slot;
	/** Tx PN threshold reached */
	unsigned long tx_pn_threshold;
	/** Tx PN exhausted */
	unsigned long tx_pn_exhausted;
	/** Tx debug buffer capture done */
	unsigned long rx_dbg_capture_done;
	/** Rx ICV error threshold */
	unsigned long rx_icv_err_threshold;
	/** Rx replay error */
	unsigned long rx_replay_error;
	/** Rx MTU check failed */
	unsigned long rx_mtu_check_fail;
	/** Rx MAC CRC err */
	unsigned long rx_mac_crc_error;
	/** Rx AES GCM buffer overflow */
	unsigned long rx_aes_gcm_buf_ovf;
	/** Rx LUT lookup miss */
	unsigned long rx_lkup_miss;
	/** Rx uninitialized key slot */
	unsigned long rx_uninit_key_slot;
	/** Rx PN exhausted */
	unsigned long rx_pn_exhausted;
	/** Secure reg violation */
	unsigned long secure_reg_viol;
};
#endif /* MACSEC_SUPPORT */

/**
 * @brief The OSI Core (MAC & MTL) private data structure.
 */
struct osi_core_priv_data {
	/** Memory mapped base address of MAC IP */
	void *base;
	/** Memory mapped base address of DMA window of MAC IP */
	void *dma_base;
	/** Memory mapped base address of XPCS IP */
	void *xpcs_base;
#ifdef MACSEC_SUPPORT
	/** Memory mapped base address of MACsec IP */
	void *macsec_base;
	/** Memory mapped base address of MACsec TZ page */
	void *tz_base;
	/** Address of MACsec HW operations structure */
	struct macsec_core_ops *macsec_ops;
	/** Instance of macsec interrupt stats structure */
	struct osi_macsec_irq_stats macsec_irq_stats;
	/** Instance of macsec HW controller Tx/Rx LUT status */
	struct osi_macsec_lut_status *macsec_lut_status;
	/** macsec mmc counters */
	struct osi_macsec_mmc_counters macsec_mmc;
#endif /* MACSEC_SUPPORT */
	/** Pointer to OSD private data structure */
	void *osd;
	/** OSD callback ops structure */
	struct osd_core_ops osd_ops;
	/** Number of MTL queues enabled in MAC */
	nveu32_t num_mtl_queues;
	/** Array of MTL queues */
	nveu32_t mtl_queues[OSI_MGBE_MAX_NUM_CHANS];
	/** List of MTL Rx queue mode that need to be enabled */
	nveu32_t rxq_ctrl[OSI_MGBE_MAX_NUM_CHANS];
	/** Rx MTl Queue mapping based on User Priority field */
	nveu32_t rxq_prio[OSI_MGBE_MAX_NUM_CHANS];
	/** TQ:TC mapping */
	unsigned int tc[OSI_MGBE_MAX_NUM_CHANS];
	/** Residual queue valid with FPE support */
	unsigned int residual_queue;
	/** MAC HW type EQOS based on DT compatible */
	nveu32_t mac;
	/** MAC version */
	nveu32_t mac_ver;
	/** HW supported feature list */
	struct osi_hw_features *hw_feat;
	/** MDC clock rate */
	nveu32_t mdc_cr;
	/** MTU size */
	nveu32_t mtu;
	/** Ethernet MAC address */
	nveu8_t mac_addr[OSI_ETH_ALEN];
	/** DT entry to enable(0) or disable(1) pause frame support */
	nveu32_t pause_frames;
	/** Current flow control settings */
	nveu32_t flow_ctrl;
	/** PTP configuration settings */
	struct osi_ptp_config ptp_config;
	/** Default addend value */
	nveu32_t default_addend;
	/** mmc counter structure */
	struct osi_mmc_counters mmc;
	/** xtra sw error counters */
	struct osi_xtra_stat_counters xstats;
	/** DMA channel selection enable (1) */
	nveu32_t dcs_en;
	/** Functional safety config to do periodic read-verify of
	 * certain safety critical registers */
	void *safety_config;
	/** Backup config to save/restore registers during suspend/resume */
	struct core_backup backup_config;
	/** VLAN tag stripping enable(1) or disable(0) */
	nveu32_t strip_vlan_tag;
	/** L3L4 filter bit bask, set index corresponding bit for
	 * filter if filter enabled */
	nveu32_t l3l4_filter_bitmask;
	/** csr clock is to program LPI 1 us tick timer register.
	 * Value stored in MHz
	 */
	nveu32_t csr_clk_speed;
	/** Tegra Pre-si platform info */
	nveu32_t pre_si;
	/** Flag which decides virtualization is enabled(1) or disabled(0) */
	nveu32_t use_virtualization;
	unsigned long vf_bitmap;
	/** Array to maintaion VLAN filters */
	unsigned short vid[VLAN_NUM_VID];
	/** Count of number of VLAN filters in vid array */
	unsigned short vlan_filter_cnt;
	/** FRP Instruction Table */
	struct osi_core_frp_entry frp_table[OSI_FRP_MAX_ENTRY];
	/** Number of valid Entries in the FRP Instruction Table */
	unsigned int frp_cnt;
	/** RSS core structure */
	struct osi_core_rss rss;
	/** HW supported feature list */
	struct osi_hw_features *hw_feature;
	/** Switch to Software Owned List Complete.
	  *  1 - Successful and User configured GCL in placed */
	unsigned int est_ready;
	/** FPE enabled, verify and respose done with peer device
	  * 1- Sucessful and can be used between P2P device */
	unsigned int fpe_ready;
	/** TSN stats counters */
	struct osi_tsn_stats tsn_stats;
	/** MC packets Multiple DMA channel selection flags */
	nveu32_t mc_dmasel;
};

/**
 * @brief osi_poll_for_mac_reset_complete - Poll Software reset bit in MAC HW
 *
 * @note
 * Algorithm:
 *  - Invokes EQOS routine to check for SWR (software reset)
 *    bit in DMA Basic mode register to make sure IP reset was successful.
 *
 * @param[in] osi_core: OSI Core private data structure.
 *
 * @pre MAC needs to be out of reset and proper clock configured.
 *
 * @note
 * Traceability Details:
 * - SWUD_ID: ETHERNET_NVETHERNETRM_004
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
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

nve32_t osi_poll_for_mac_reset_complete(
			struct osi_core_priv_data *const osi_core);

/**
 * @brief osi_hw_core_init - EQOS MAC, MTL and common DMA initialization.
 * 
 * @note
 * Algorithm:
 *  - Invokes EQOS MAC, MTL and common DMA register init code.
 *
 * @param[in, out] osi_core: OSI core private data structure.
 * @param[in] tx_fifo_size: OSI core private data structure.
 * @param[in] rx_fifo_size: OSI core private data structure.
 *
 * @pre
 * - MAC should be out of reset. See osi_poll_for_mac_reset_complete()
 *   for details.
 * - osi_core->base needs to be filled based on ioremap.
 * - osi_core->num_mtl_queues needs to be filled.
 * - osi_core->mtl_queues[qinx] need to be filled.
 *
 * @note
 * Traceability Details:
 * - SWUD_ID: ETHERNET_NVETHERNETRM_006
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
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
nve32_t osi_hw_core_init(struct osi_core_priv_data *const osi_core,
			 nveu32_t tx_fifo_size, nveu32_t rx_fifo_size);

/**
 * @brief osi_hw_core_deinit - EQOS MAC deinitialization.
 *
 * @note
 * Algorithm:
 *  - Stops MAC transmission and reception.
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @pre MAC has to be out of reset.
 *
 * @note
 * Traceability Details:
 * - SWUD_ID: ETHERNET_NVETHERNETRM_007
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: No
 * - De-initialization: Yes
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
nve32_t osi_hw_core_deinit(struct osi_core_priv_data *const osi_core);

/**
 * @brief osi_start_mac - Start MAC Tx/Rx engine
 * 
 * @note
 * Algorithm:
 *  - Enable MAC Tx and Rx engine.
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @pre MAC init should be complete. See osi_hw_core_init() and
 *      osi_hw_dma_init()
 *
 * @note
 * Traceability Details:
 * - SWUD_ID: ETHERNET_NVETHERNETRM_008
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
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
nve32_t osi_start_mac(struct osi_core_priv_data *const osi_core);

/**
 * @brief osi_stop_mac - Stop MAC Tx/Rx engine
 * 
 * @note
 * Algorithm:
 *  - Stop MAC Tx and Rx engine
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @pre MAC DMA deinit should be complete. See osi_hw_dma_deinit()
 *
 * @note
 * Traceability Details:
 * - SWUD_ID: ETHERNET_NVETHERNETRM_009
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: No
 * - De-initialization: Yes
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
nve32_t osi_stop_mac(struct osi_core_priv_data *const osi_core);

/**
 * @brief osi_common_isr - Common ISR.
 * 
 * @note
 * Algorithm:
 *  - Takes care of handling the common interrupts accordingly as per
 *    the MAC IP
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @pre MAC should be init and started. see osi_start_mac()
 *
 * @note
 * Traceability Details:
 * - SWUD_ID: ETHERNET_NVETHERNETRM_010
 *
 * @note
 * Classification:
 * - Interrupt: Yes
 * - Signal handler: Yes
 * - Thread safe: No
 * - Required Privileges: None
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
nve32_t osi_common_isr(struct osi_core_priv_data *const osi_core);

/**
 * @brief osi_set_mode - Set FD/HD mode.
 *
 * @note
 * Algorithm:
 *  - Takes care of  setting HD or FD mode accordingly as per the MAC IP
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] mode: Operating mode. (OSI_FULL_DUPLEX/OSI_HALF_DUPLEX)
 *
 * @pre MAC should be init and started. see osi_start_mac()
 *
 * @note
 * Traceability Details:
 * - SWUD_ID: ETHERNET_NVETHERNETRM_011
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
nve32_t osi_set_mode(struct osi_core_priv_data *const osi_core,
		     const nve32_t mode);

/**
 * @brief osi_set_speed - Set operating speed.
 * 
 * @note
 * Algorithm:
 *  - Takes care of  setting the operating speed accordingly as per
 *    the MAC IP.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] speed: Operating speed.
 *
 * @pre MAC should be init and started. see osi_start_mac()
 *
 * @note
 * Traceability Details:
 * - SWUD_ID: ETHERNET_NVETHERNETRM_012
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
nve32_t osi_set_speed(struct osi_core_priv_data *const osi_core,
		      const nve32_t speed);

/**
 * @brief osi_pad_calibrate - PAD calibration
 *
 * @note
 * Algorithm:
 *  - Takes care of  doing the pad calibration
 *    accordingly as per the MAC IP.
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @pre
 *  - MAC should out of reset and clocks enabled.
 *  - RGMII and MDIO interface needs to be IDLE before performing PAD
 *    calibration.
 *
 * @note
 * Traceability Details:
 * - SWUD_ID: ETHERNET_NVETHERNETRM_013
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
nve32_t osi_pad_calibrate(struct osi_core_priv_data *const osi_core);

/**
 * @brief osi_config_fw_err_pkts - Configure forwarding of error packets
 *
 * @note
 * Algorithm:
 *  - Configure MAC to enable/disable forwarding of error packets.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] qinx: Q index. Max OSI_EQOS_MAX_NUM_QUEUES.
 * @param[in] fw_err: Enable or disable forwarding of error packets.
 *            0: Disable 1: Enable
 *
 * @pre MAC should be init and started. see osi_start_mac()
 *
 * @note
 * Traceability Details:
 * - SWUD_ID: ETHERNET_NVETHERNETRM_020
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
nve32_t osi_config_fw_err_pkts(struct osi_core_priv_data *const osi_core,
			       const nveu32_t qinx, const nveu32_t fw_err);

/**
 * @brief osi_config_rxcsum_offload - Configure RX checksum offload in MAC.
 *
 * @note
 * Algorithm:
 *  - Invokes EQOS config RX checksum offload routine.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] enable: Enable/disable flag. 0: Disable 1: Enable
 *
 * @pre MAC should be init and started. see osi_start_mac()
 *
 * @note
 * Traceability Details:
 * - SWUD_ID: ETHERNET_NVETHERNETRM_017
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
nve32_t osi_config_rxcsum_offload(struct osi_core_priv_data *const osi_core,
				  const nveu32_t enable);

/**
 * @brief osi_l2_filter - configure L2 mac filter.
 *
 * @note
 * Algorithm:
 *  - This sequence is used to configure MAC in different packet
 *    processing modes like promiscuous, multicast, unicast,
 *    hash unicast/multicast and perfect/inverse matching for L2 DA
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] filter: OSI filter structure.
 *
 * @pre
 *  - MAC should be initialized and started. see osi_start_mac()
 *
 * @note
 * Traceability Details:
 * - SWUD_ID: ETHERNET_NVETHERNETRM_018
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
nve32_t osi_l2_filter(struct osi_core_priv_data *const osi_core,
		      const struct osi_filter *filter);

/**
 * @brief osi_write_phy_reg - Write to a PHY register through MAC over MDIO bus.
 *
 * @note
 * Algorithm:
 * - Before proceeding for reading for PHY register check whether any MII
 *   operation going on MDIO bus by polling MAC_GMII_BUSY bit.
 * - Program data into MAC MDIO data register.
 * - Populate required parameters like phy address, phy register etc,,
 *   in MAC MDIO Address register. write and GMII busy bits needs to be set
 *   in this operation.
 * - Write into MAC MDIO address register poll for GMII busy for MDIO
 *   operation to complete.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] phyaddr: PHY address (PHY ID) associated with PHY
 * @param[in] phyreg: Register which needs to be write to PHY.
 * @param[in] phydata: Data to write to a PHY register.
 *
 * @pre MAC should be init and started. see osi_start_mac()
 *
 * @note
 * Traceability Details:
 * - SWUD_ID: ETHERNET_NVETHERNETRM_002
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
nve32_t osi_write_phy_reg(struct osi_core_priv_data *const osi_core,
			  const nveu32_t phyaddr, const nveu32_t phyreg,
			  const nveu16_t phydata);

/**
 * @brief osi_read_mmc - invoke function to read actual registers and update
 *	  structure variable mmc
 *
 * @note
 * Algorithm:
 *  - Read the registers, mask reserve bits if required, update
 *    structure.
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @pre
 *  - MAC should be init and started. see osi_start_mac()
 *  - osi_core->osd should be populated
 *
 * @note
 * Traceability Details:
 * - SWUD_ID: ETHERNET_NVETHERNETRM_014
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
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
nve32_t osi_read_mmc(struct osi_core_priv_data *const osi_core);

/**
 * @brief osi_read_phy_reg - Read from a PHY register through MAC over MDIO bus.
 *
 * @note
 * Algorithm:
 *  - Before proceeding for reading for PHY register check whether any MII
 *    operation going on MDIO bus by polling MAC_GMII_BUSY bit.
 *  - Populate required parameters like phy address, phy register etc,,
 *    in program it in MAC MDIO Address register. Read and GMII busy bits
 *    needs to be set in this operation.
 *  - Write into MAC MDIO address register poll for GMII busy for MDIO
 *    operation to complete. After this data will be available at MAC MDIO
 *    data register.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] phyaddr: PHY address (PHY ID) associated with PHY
 * @param[in] phyreg: Register which needs to be read from PHY.
 *
 * @pre MAC should be init and started. see osi_start_mac()
 *
 * @note
 * Traceability Details:
 * - SWUD_ID: ETHERNET_NVETHERNETRM_003
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval data from PHY register on success
 * @retval -1 on failure
 */
nve32_t osi_read_phy_reg(struct osi_core_priv_data *const osi_core,
			 const nveu32_t phyaddr,
			 const nveu32_t phyreg);

/**
 * @brief initializing the core operations
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @retval data from PHY register on success
 * @retval -1 on failure
 *
 * @note
 * Traceability Details:
 * - SWUD_ID: ETHERNET_NVETHERNETRM_001
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: No
 * - De-initialization: No
 *
 */
nve32_t osi_init_core_ops(struct osi_core_priv_data *const osi_core);

/**
 * @brief osi_config_ptp_offload - Enable/Disable PTP offload
 *
 * Algorithm: Based on input argument, update TSCR and PTO registers.
 * Update ptp_filter for TSCR register and call PTP configurtion API.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] pto_config: The PTP Offload configuration from function
 *	      driver.
 *
 * @note 1) MAC should be init and started. see osi_start_mac()
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
int osi_config_ptp_offload(struct osi_core_priv_data *const osi_core,
			   struct osi_pto_config *const pto_config);

/**
 * @brief osi_set_systime_to_mac - Handles setting of system time.
 *
 * @note
 * Algorithm:
 *  - Set current system time to MAC.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] sec: Seconds to be configured.
 * @param[in] nsec: Nano seconds to be configured.
 *
 * @pre MAC should be init and started. see osi_start_mac()
 *
 * @note
 * Traceability Details:
 * - SWUD_ID: ETHERNET_NVETHERNETRM_005
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
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
nve32_t osi_set_systime_to_mac(struct osi_core_priv_data *const osi_core,
			       const nveu32_t sec, const nveu32_t nsec);

/**
 * @brief osi_adjust_freq - Adjust frequency
 *
 * @note
 * Algorithm:
 *  - Adjust a drift of +/- comp nanoseconds per second.
 *    "Compensation" is the difference in frequency between
 *    the master and slave clocks in Parts Per Billion.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] ppb: Parts per Billion
 *
 * @pre MAC should be init and started. see osi_start_mac()
 *
 * @note
 * Traceability Details:
 * - SWUD_ID: ETHERNET_NVETHERNETRM_023
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
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
nve32_t osi_adjust_freq(struct osi_core_priv_data *const osi_core, nve32_t ppb);

/**
 * @brief osi_adjust_time - Adjust MAC time with system time
 *
 * @note
 * Algorithm:
 *  - Adjust/update the MAC time (delta time from MAC to system time
 *    passed in nanoseconds, can be + or -).
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] nsec_delta: Delta time in nano seconds
 *
 * @pre
 *  - MAC should be init and started. see osi_start_mac()
 *  - osi_core->ptp_config.one_nsec_accuracy need to be set to 1
 *
 * @note
 * Traceability Details:
 * - SWUD_ID: ETHERNET_NVETHERNETRM_022
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
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
nve32_t osi_adjust_time(struct osi_core_priv_data *const osi_core,
			nvel64_t nsec_delta);

/**
 * @brief osi_ptp_configuration - Configure PTP
 *
 * @note
 * Algorithm:
 *  - Configure the PTP registers that are required for PTP.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] enable: Enable or disable Time Stamping. 0: Disable 1: Enable
 *
 * @pre
 *  - MAC should be init and started. see osi_start_mac()
 *  - osi->ptp_config.ptp_filter need to be filled accordingly to the
 *    filter that need to be set for PTP packets. Please check osi_ptp_config
 *    structure declaration on the bit fields that need to be filled.
 *  - osi->ptp_config.ptp_clock need to be filled with the ptp system clk.
 *    Currently it is set to 62500000Hz.
 *  - osi->ptp_config.ptp_ref_clk_rate need to be filled with the ptp
 *    reference clock that platform supports.
 *  - osi->ptp_config.sec need to be filled with current time of seconds
 *  - osi->ptp_config.nsec need to be filled with current time of nseconds
 *  - osi->base need to be filled with the ioremapped base address
 *
 * @note
 * Traceability Details:
 * - SWUD_ID: ETHERNET_NVETHERNETRM_021
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
nve32_t osi_ptp_configuration(struct osi_core_priv_data *const osi_core,
			      const nveu32_t enable);

/* MAC version specific implementation function prototypes added here
 * for misra compliance to have
 * 1. Visible prototype for all functions.
 * 2. Only one prototype for all function.
 */
void *eqos_get_core_safety_config(void);

/**
 * @brief osi_l3l4_filter -  invoke OSI call to add L3/L4
 * filters.
 *
 * @note
 * Algorithm:
 *  - This routine is to enable/disable L3/l4 filter.
 *    Check for DCS enable as well as validate channel
 *    number if dcs_enable is set. After validation, configure L3(IPv4/IPv6)
 *    filters register for given address. Based on input arguments update
 *    IPv4/IPv6 source/destination address for L3 layer filtering or source and
 *    destination Port Number for L4(TCP/UDP) layer
 *    filtering.
 *
 * @param[in, out] osi_core: OSI core private data structure.
 * @param[in] l_filter: L3L4 filter data structure.
 * @param[in] type: L3 filter (ipv4(0) or ipv6(1))
 *            or L4 filter (tcp(0) or udp(1))
 * @param[in] dma_routing_enable: filter based dma routing enable(1)
 * @param[in] dma_chan: dma channel for routing based on filter.
 *            Max OSI_EQOS_MAX_NUM_CHANS.
 * @param[in] is_l4_filter: API call for L3 filter(0) or L4 filter(1)
 *
 * @pre
 *  - MAC should be init and started. see osi_start_mac()
 *  - Concurrent invocations to configure filters is not supported.
 *    OSD driver shall serialize calls.
 *
 * @note
 * Traceability Details:
 * - SWUD_ID: ETHERNET_NVETHERNETRM_019
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
nve32_t osi_l3l4_filter(struct osi_core_priv_data *const osi_core,
			const struct osi_l3_l4_filter l_filter,
			const nveu32_t type,
			const nveu32_t dma_routing_enable,
			const nveu32_t dma_chan,
			const nveu32_t is_l4_filter);

/**
 * @brief osi_get_mac_version - Reading MAC version
 *
 * @note
 * Algorithm:
 *  - Reads MAC version and check whether its valid or not.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[out] mac_ver: holds mac version.
 *
 * @pre MAC has to be out of reset.
 *
 * @note
 * Traceability Details:
 * - SWUD_ID: ETHERNET_NVETHERNETRM_015
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
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
nve32_t osi_get_mac_version(struct osi_core_priv_data *const osi_core,
			    nveu32_t *mac_ver);

/**
 * @brief osi_get_hw_features - Reading MAC HW features
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[out] hw_feat: holds the supported features of the hardware.
 *
 * @pre MAC has to be out of reset.
 *
 * @note
 * Traceability Details:
 * - SWUD_ID: ETHERNET_NVETHERNETRM_016
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
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
nve32_t osi_get_hw_features(struct osi_core_priv_data *const osi_core,
			    struct osi_hw_features *hw_feat);

#ifndef OSI_STRIPPED_LIB
/**
 * @brief osi_validate_core_regs - Read-validate HW registers for func safety.
 *
 * @note
 * Algorithm:
 *  - Reads pre-configured list of MAC/MTL configuration registers
 *    and compares with last written value for any modifications.
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @pre
 *  - MAC has to be out of reset.
 *  - osi_hw_core_init has to be called. Internally this would initialize
 *    the safety_config (see osi_core_priv_data) based on MAC version and
 *    which specific registers needs to be validated periodically.
 *  - Invoke this call if (osi_core_priv_data->safety_config != OSI_NULL)
 *
 * @note
 * Traceability Details:
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
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
nve32_t osi_validate_core_regs(struct osi_core_priv_data *const osi_core);

/**
 * @brief osi_flush_mtl_tx_queue - Flushing a MTL Tx Queue.
 *
 * @note
 * Algorithm:
 *  - Invokes EQOS flush Tx queue routine.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] qinx: MTL queue index.
 *
 * @pre
 *  - MAC should out of reset and clocks enabled.
 *  - hw core initialized. see osi_hw_core_init().
 *
 * @note
 * Traceability Details:
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
nve32_t osi_flush_mtl_tx_queue(struct osi_core_priv_data *const osi_core,
			       const nveu32_t qinx);

/**
 * @brief osi_set_avb - Set CBS algo and parameters
 *
 * @note
 * Algorithm:
 *  - Set AVB algo and  populated parameter from osi_core_avb
 *    structure for TC/TQ
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] avb: osi core avb data structure.
 *
 * @pre
 *  - MAC should be init and started. see osi_start_mac()
 *  - osi_core->osd should be populated.
 *
 * @note
 * Traceability Details:
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
nve32_t osi_set_avb(struct osi_core_priv_data *const osi_core,
		    const struct osi_core_avb_algorithm *avb);

/**
 * @brief osi_get_avb - Get CBS algo and parameters
 *
 * @note
 * Algorithm:
 *  - get AVB algo and  populated parameter from osi_core_avb
 *    structure for TC/TQ
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[out] avb: osi core avb data structure.
 *
 * @pre
 *  - MAC should be init and started. see osi_start_mac()
 *  - osi_core->osd should be populated.
 *
 * @note
 * Traceability Details:
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
nve32_t osi_get_avb(struct osi_core_priv_data *const osi_core,
		    struct osi_core_avb_algorithm *avb);

/**
 * @brief osi_configure_txstatus - Configure Tx packet status reporting
 *
 * @note
 * Algorithm:
 *  - Configure MAC to enable/disable Tx status error
 *    reporting.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] tx_status: Enable or disable tx packet status reporting
 *
 * @pre MAC should be init and started. see osi_start_mac()
 *
 * @note
 * Traceability Details:
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
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
nve32_t osi_configure_txstatus(struct osi_core_priv_data *const osi_core,
			       const nveu32_t tx_status);

/**
 * @brief osi_config_rx_crc_check - Configure CRC Checking for Received Packets
 *
 * @note
 * Algorithm:
 *  - When this bit is set, the MAC receiver does not check the CRC
 *    field in the received packets. When this bit is reset, the MAC receiver
 *    always checks the CRC field in the received packets.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] crc_chk: Enable or disable checking of CRC field in received pkts
 *
 * @pre MAC should be init and started. see osi_start_mac()
 *
 * @note
 * Traceability Details:
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
nve32_t osi_config_rx_crc_check(struct osi_core_priv_data *const osi_core,
				const nveu32_t crc_chk);

/**
 * @brief osi_configure_flow_ctrl - Configure flow control settings
 *
 * @note
 * Algorithm:
 *  - This will enable or disable the flow control.
 *    flw_ctrl BIT0 is for tx flow ctrl enable/disable
 *    flw_ctrl BIT1 is for rx flow ctrl enable/disable
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] flw_ctrl: Enable or disable flow control settings
 *
 * @pre MAC should be init and started. see osi_start_mac()
 *
 * @note
 * Traceability Details:
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
nve32_t osi_configure_flow_control(struct osi_core_priv_data *const osi_core,
				   const nveu32_t flw_ctrl);

/**
 * @brief osi_config_arp_offload - Configure ARP offload in MAC.
 *
 * @note
 * Algorithm:
 *  - Invokes EQOS config ARP offload routine.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] flags: Enable/disable flag.
 * @param[in] ip_addr: Char array representation of IP address
 *
 * @pre
 *  - MAC should be init and started. see osi_start_mac()
 *  - Valid 4 byte IP address as argument ip_addr
 *
 * @note
 * Traceability Details:
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
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
nve32_t osi_config_arp_offload(struct osi_core_priv_data *const osi_core,
			       const nveu32_t flags,
			       const nveu8_t *ip_addr);
/**
 * @brief osi_ptp_rxq - Enable PTP RX packets routing
 *
 * Algorithm: Program PTP RX queue index
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] rx_route: Pointer to the osi_rxq_route structure, which
 * contains RXQ routing parameters.
 *
 * @note MAC should be init and started. see osi_start_mac()
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
int osi_rxq_route(struct osi_core_priv_data *const osi_core,
		  const struct osi_rxq_route *rx_route);

/**
 * @brief osi_config_vlan_filtering - OSI call for configuring VLAN filter
 *
 * @note
 * Algorithm:
 *  - This sequence is used to enable/disable VLAN filtering and
 *    also selects VLAN filtering mode- perfect/hash
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] filter_enb_dis: vlan filter enable(1) disable(0)
 * @param[in] perfect_hash_filtering: perfect(0) or hash filter(1)
 * @param[in] perfect_inverse_match: normal(0) or inverse filter(1)
 *
 * @pre
 *  - MAC should be init and started. see osi_start_mac()
 *  - osi_core->osd should be populated
 *
 * @note
 * Traceability Details:
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
nve32_t osi_config_vlan_filtering(struct osi_core_priv_data *const osi_core,
				  const nveu32_t filter_enb_dis,
				  const nveu32_t perfect_hash_filtering,
				  const nveu32_t perfect_inverse_match);

/**
 * @brief osi_update_vlan_id - invoke osi call to update VLAN ID
 *
 * @note
 * Algorithm:
 *  - return 16 bit VLAN ID
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] vid: VLAN ID
 *
 * @pre MAC should be init and started. see osi_start_mac()
 *
 * @note
 * Traceability Details:
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
nve32_t osi_update_vlan_id(struct osi_core_priv_data *const osi_core,
			   const nveu32_t vid);

/**
 * @brief osi_reset_mmc - invoke function to reset MMC counter and data
 *	  structure
 *
 * @note
 * Algorithm:
 *  - Read the registers, mask reserve bits if required, update
 *    structure.
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @pre
 *  - MAC should be init and started. see osi_start_mac()
 *  - osi_core->osd should be populated
 *
 * @note
 * Traceability Details:
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
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
nve32_t osi_reset_mmc(struct osi_core_priv_data *const osi_core);

/**
 * @brief osi_configure_eee - Configure EEE LPI in MAC.
 *
 * @note
 * Algorithm:
 *  - This routine invokes configuration of EEE LPI in the MAC.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] tx_lpi_enabled: Enable (1)/disable (0) tx lpi
 * @param[in] tx_lpi_timer: Tx LPI entry timer in usecs upto
 *            OSI_MAX_TX_LPI_TIMER (in steps of 8usec)
 *
 * @pre
 *  - MAC and PHY should be init and started. see osi_start_mac()
 *
 * @note
 * Traceability Details:
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
nve32_t osi_configure_eee(struct osi_core_priv_data *const osi_core,
			  nveu32_t tx_lpi_enabled,
			  nveu32_t tx_lpi_timer);

/**
 * @brief osi_save_registers - Take backup of MAC MMIO address space
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @pre
 *  - MAC and PHY should be init and started. see osi_start_mac()
 *  - No further configuration change in MAC shall be done after invoking
 *    this API
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
nve32_t osi_save_registers(struct osi_core_priv_data *const osi_core);

/**
 * @brief osi_restore_registers - Restore backup of MAC MMIO address space
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @pre
 *  - MAC and PHY should be init and started. see osi_start_mac()
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
nve32_t osi_restore_registers(struct osi_core_priv_data *const osi_core);

/**
 * @brief osi_set_mdc_clk_rate - Derive MDC clock based on provided AXI_CBB clk.
 *
 * @note
 * Algorithm:
 *  - MDC clock rate will be populated in OSI core private data
 *    structure based on AXI_CBB clock rate.
 *
 * @param[in, out] osi_core: OSI core private data structure.
 * @param[in] csr_clk_rate: CSR (AXI CBB) clock rate.
 *
 * @note OSD layer needs get the AXI CBB clock rate with OSD clock API
 *	(ex - clk_get_rate())
 *
 * @note
 * Traceability Details:
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
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
nve32_t osi_set_mdc_clk_rate(struct osi_core_priv_data *const osi_core,
			     const nveu64_t csr_clk_rate);

/**
 * @brief osi_config_mac_loopback - Configure MAC loopback
 *
 * @note
 * Algorithm:
 *  - Configure the MAC to support the loopback.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] lb_mode: Enable or disable MAC loopback
 *
 * @pre MAC should be init and started. see osi_start_mac()
 *
 * @note
 * Traceability Details:
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
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
nve32_t osi_config_mac_loopback(struct osi_core_priv_data *const osi_core,
				const nveu32_t lb_mode);
#endif /* !OSI_STRIPPED_LIB */

/**
 * @brief osi_config_rss - Configuration of RSS.
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @note
 *	1) MAC and PHY should be init and started. see osi_start_mac()
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
nve32_t osi_config_rss(struct osi_core_priv_data *const osi_core);

/**
 * @brief osi_configure_frp - Configure the FRP offload entry in the
 * Instruction Table.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] cmd: FRP command data structure.
 *
 * @note
 *	1) MAC and PHY should be init and started. see osi_start_mac()
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
int osi_configure_frp(struct osi_core_priv_data *const osi_core,
		      struct osi_core_frp_cmd *const cmd);
/**
 * @brief osi_hw_config_est - Read Setting for GCL from input and update
 * registers.
 *
 * Algorithm:
 * 1) Write  TER, LLR and EST control register
 * 2) Update GCL to sw own GCL (MTL_EST_Status bit SWOL will tell which is
 *    owned by SW) and store which GCL is in use currently in sw.
 * 3) TODO set DBGB and DBGM for debugging
 * 4) EST_data and GCRR to 1, update entry sno in ADDR and put data at
 *    est_gcl_data enable GCL MTL_EST_SSWL and wait for self clear or use
 *    SWLC in MTL_EST_Status. Please note new GCL will be pushed for each entry.
 * 5) Configure btr. Update btr based on current time (current time
 *    should be updated based on PTP by this time)
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] est: Configuration osi_est_config structure.
 *
 * @note MAC should be init and started. see osi_start_mac()
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
int osi_hw_config_est(struct osi_core_priv_data *osi_core,
		      struct osi_est_config *est);

/**
 * @brief osi_hw_config_fep - Read Setting for preemption and express for TC
 * and update registers.
 *
 * Algorithm:
 * 1) Check for TC enable and TC has masked for setting to preemptable.
 * 2) update FPE control status register
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] fpe: Configuration osi_fpe_config structure.
 *
 * @note MAC should be init and started. see osi_start_mac()
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
int osi_hw_config_fpe(struct osi_core_priv_data *osi_core,
		      struct osi_fpe_config *fpe);
#endif /* INCLUDED_OSI_CORE_H */
