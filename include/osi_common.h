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

/**
 * @addtogroup EQOS-MAC EQOS MAC HW supported features
 *
 * @brief Helps in identifying the features that are set in MAC HW
 * @{
 */
#define EQOS_MAC_HFR0		0x11c
#define EQOS_MAC_HFR1		0x120
#define EQOS_MAC_HFR2		0x124

#define EQOS_MAC_HFR0_MIISEL_MASK	0x1U
#define EQOS_MAC_HFR0_GMIISEL_MASK	0x1U
#define EQOS_MAC_HFR0_HDSEL_MASK	0x1U
#define EQOS_MAC_HFR0_PCSSEL_MASK	0x1U
#define EQOS_MAC_HFR0_SMASEL_MASK	0x1U
#define EQOS_MAC_HFR0_RWKSEL_MASK	0x1U
#define EQOS_MAC_HFR0_MGKSEL_MASK	0x1U
#define EQOS_MAC_HFR0_MMCSEL_MASK	0x1U
#define EQOS_MAC_HFR0_ARPOFFLDEN_MASK	0x1U
#define EQOS_MAC_HFR0_TSSSEL_MASK	0x1U
#define EQOS_MAC_HFR0_EEESEL_MASK	0x1U
#define EQOS_MAC_HFR0_TXCOESEL_MASK	0x1U
#define EQOS_MAC_HFR0_RXCOE_MASK	0x1U
#define EQOS_MAC_HFR0_ADDMACADRSEL_MASK	0x1fU
#define EQOS_MAC_HFR0_MACADR32SEL_MASK	0x1U
#define EQOS_MAC_HFR0_MACADR64SEL_MASK	0x1U
#define EQOS_MAC_HFR0_TSINTSEL_MASK	0x3U
#define EQOS_MAC_HFR0_SAVLANINS_MASK	0x1U
#define EQOS_MAC_HFR0_ACTPHYSEL_MASK	0x7U
#define EQOS_MAC_HFR1_RXFIFOSIZE_MASK	0x1fU
#define EQOS_MAC_HFR1_TXFIFOSIZE_MASK	0x1fU
#define EQOS_MAC_HFR1_ADVTHWORD_MASK	0x1U
#define EQOS_MAC_HFR1_ADDR64_MASK	0x3U
#define EQOS_MAC_HFR1_DCBEN_MASK	0x1U
#define EQOS_MAC_HFR1_SPHEN_MASK	0x1U
#define EQOS_MAC_HFR1_TSOEN_MASK	0x1U
#define EQOS_MAC_HFR1_DMADEBUGEN_MASK	0x1U
#define EQOS_MAC_HFR1_AVSEL_MASK	0x1U
#ifndef OSI_STRIPPED_LIB
#define EQOS_MAC_HFR1_LPMODEEN_MASK	0x1U
#endif /* OSI_STRIPPED_LIB */
#define EQOS_MAC_HFR1_HASHTBLSZ_MASK	0x3U
#define EQOS_MAC_HFR1_L3L4FILTERNUM_MASK	0xfU
#define EQOS_MAC_HFR2_RXQCNT_MASK	0xfU
#define EQOS_MAC_HFR2_TXQCNT_MASK	0xfU
#define EQOS_MAC_HFR2_RXCHCNT_MASK	0xfU
#define EQOS_MAC_HFR2_TXCHCNT_MASK	0xfU
#define EQOS_MAC_HFR2_PPSOUTNUM_MASK	0x7U
#define EQOS_MAC_HFR2_AUXSNAPNUM_MASK	0x7U
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

/**
 * @brief common_get_mac_version - Reading MAC version
 *
 * @note
 * Algorithm:
 *  - Reads MAC version and check whether its valid or not.
 *
 * @param[in] addr: io-remap MAC base address.
 * @param[out] mac_ver: holds mac version.
 *
 * @pre MAC has to be out of reset.
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
nve32_t common_get_mac_version(void *addr, nveu32_t *mac_ver);

/**
 * @brief comon_get_hw_features - Reading MAC HW features
 *
 * @param[in] base: io-remap MAC base address.
 * @param[out] hw_feat: holds the supported features of the hardware.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: No
 * - De-initialization: No
 *
 * @pre MAC has to be out of reset.
 */
void common_get_hw_features(void *base, struct osi_hw_features *hw_feat);
/**
 * @brief osi_memset - osi memset
 *
 * @param[out] s: source that need to be set
 * @param[in] c: value to fill in source
 * @param[in] count: first n bytes of source
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 */
void osi_memset(void *s, nveu32_t c, nveu64_t count);
#endif /* INCLUDED_OSI_COMMON_H */
