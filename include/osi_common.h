/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION. All rights reserved.
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

#ifndef OSI_COMMON_H
#define OSI_COMMON_H

#define OSI_PAUSE_FRAMES_ENABLE		0U
#define OSI_PAUSE_FRAMES_DISABLE	1U
#define OSI_FLOW_CTRL_TX		OSI_BIT(0)
#define OSI_FLOW_CTRL_RX		OSI_BIT(1)
#define OSI_FLOW_CTRL_DISABLE		0U

#define OSI_ADDRESS_32BIT		0
#define OSI_ADDRESS_40BIT		1
#define OSI_ADDRESS_48BIT		2

#define ULONG_MAX			(~0UL)

/* Default maximum Gaint Packet Size Limit */
#define OSI_MAX_MTU_SIZE	9000U
#define OSI_DFLT_MTU_SIZE	1500U
#define OSI_MTU_SIZE_2K		2048U
#define OSI_MTU_SIZE_4K		4096U
#define OSI_MTU_SIZE_8K		8192U
#define OSI_MTU_SIZE_16K	16384U

#define EQOS_DMA_CHX_STATUS(x)		((0x0080U * (x)) + 0x1160U)
#define EQOS_DMA_CHX_IER(x)		((0x0080U * (x)) + 0x1134U)

#define MAC_VERSION		0x110
#define MAC_VERSION_SNVER_MASK	0x7FU

#define OSI_MAC_HW_EQOS		0U
#define OSI_ETH_ALEN		6U

#define OSI_NULL                ((void *)0)
#define OSI_ENABLE		1U
#define OSI_DISABLE		0U

#define OSI_EQOS_MAX_NUM_CHANS	4U

#define OSI_BIT(nr)             ((unsigned int)1 << (nr))

#define OSI_EQOS_MAC_4_10       0x41U
#define OSI_EQOS_MAC_5_00       0x50U
#define OSI_EQOS_MAC_5_10       0x51U

#define OSI_SPEED_10		10
#define OSI_SPEED_100		100
#define OSI_SPEED_1000		1000

#define OSI_FULL_DUPLEX		1
#define OSI_HALF_DUPLEX		0

#define NV_ETH_FRAME_LEN   1514U
#define NV_ETH_FCS_LEN	0x4U
#define NV_VLAN_HLEN		0x4U

#define MAX_ETH_FRAME_LEN_DEFAULT \
	(NV_ETH_FRAME_LEN + NV_ETH_FCS_LEN + NV_VLAN_HLEN)

#define L32(data)       ((data) & 0xFFFFFFFFUL)
#define H32(data)       (((data) & 0xFFFFFFFF00000000UL) >> 32UL)

#define OSI_INVALID_CHAN_NUM    0xFFU

#define TX_DESC_CNT     256U
#define RX_DESC_CNT     256U

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
#define EQOS_MAC_HFR1_LPMODEEN_MASK	0x1U
#define EQOS_MAC_HFR1_HASHTBLSZ_MASK	0x3U
#define EQOS_MAC_HFR1_L3L4FILTERNUM_MASK	0xfU
#define EQOS_MAC_HFR2_RXQCNT_MASK	0xfU
#define EQOS_MAC_HFR2_TXQCNT_MASK	0xfU
#define EQOS_MAC_HFR2_RXCHCNT_MASK	0xfU
#define EQOS_MAC_HFR2_TXCHCNT_MASK	0xfU
#define EQOS_MAC_HFR2_PPSOUTNUM_MASK	0x7U
#define EQOS_MAC_HFR2_AUXSNAPNUM_MASK	0x7U

/**
 *	struct osi_hw_features - MAC HW supported features.
 *	@mii_sel: It sets to 1 when 10/100 Mbps is selected as the Mode of
 *		  Operation
 *	@gmii_sel: It sets to 1 when 1000 Mbps is selected as the Mode of
 *		   Operation.
 *	@hd_sel: It sets to 1 when the half-duplex mode is selected.
 *	@pcs_sel: It sets to 1 when the TBI, SGMII, or RTBI PHY interface
 *		  option is selected.
 *	@vlan_hash_en: It sets to 1 when the Enable VLAN Hash Table Based
 *		       Filtering option is selected.
 *	@sma_sel: It sets to 1 when the Enable Station Management
 *		  (MDIO Interface) option is selected.
 *	@rwk_sel: It sets to 1 when the Enable Remote Wake-Up Packet Detection
 *		  option is selected.
 *	@mgk_sel: It sets to 1 when the Enable Magic Packet Detection option is
 *		  selected.
 *	@mmc_sel: It sets to 1 when the Enable MAC Management Counters (MMC)
 *		  option is selected.
 *	@arp_offld_en: It sets to 1 when the Enable IPv4 ARP Offload option is
 *		       selected.
 *	@ts_sel: It sets to 1 when the Enable IEEE 1588 Timestamp Support
 *		 option is selected.
 *	@eee_sel: It sets to 1 when the Enable Energy Efficient Ethernet (EEE)
 *		  option is selected.
 *	@tx_coe_sel: It sets to 1 when the Enable Transmit TCP/IP Checksum
 *		     Insertion option is selected.
 *	@rx_coe_sel: It sets to 1 when the Enable Receive TCP/IP Checksum Check
 *		     option is selected.
 *	@mac_addr16_sel: It sets to 1 when the Enable Additional 1-31 MAC
 *			 Address Registers option is selected.
 *	@mac_addr32_sel: It sets to 1 when the Enable Additional 32 MAC
 *			 Address Registers (32-63) option is selected
 *	@mac_addr64_sel: It sets to 1 when the Enable Additional 64 MAC
 *			 Address Registers (64-127) option is selected.
 *	@tsstssel: It sets to 1 when the Enable IEEE 1588 Timestamp Support
 *		   option is selected.
 *	@sa_vlan_ins: It sets to 1 when the Enable SA and VLAN Insertion on Tx
 *		      option is selected.
 *	@act_phy_sel: Active PHY Selected
 *		When you have multiple PHY interfaces in your configuration,
 *		this field indicates the sampled value of phy_intf_sel_i during
 *		reset de-assertion:
 *			000: GMII or MII
 *			001: RGMII
 *			010: SGMII
 *			011: TBI
 *			100: RMII
 *			101: RTBI
 *			110: SMII
 *			111: RevMII
 *			All Others: Reserved.
 *	@rx_fifo_size: MTL Receive FIFO Size
 *		This field contains the configured value of MTL Rx FIFO in
 *		bytes expressed as Log to base 2 minus 7, that is,
 *		Log2(RXFIFO_SIZE) -7:
 *			00000: 128 bytes
 *			00001: 256 bytes
 *			00010: 512 bytes
 *			00011: 1,024 bytes
 *			00100: 2,048 bytes
 *			00101: 4,096 bytes
 *			00110: 8,192 bytes
 *			00111: 16,384 bytes
 *			01000: 32,767 bytes
 *			01000: 32 KB
 *			01001: 64 KB
 *			01010: 128 KB
 *			01011: 256 KB
 *			01100-11111: Reserved.
 *	@tx_fifo_size:	MTL Transmit FIFO Size.
 *		This field contains the configured value of MTL Tx FIFO in
 *		bytes expressed as Log to base 2 minus 7, that is,
 *		Log2(TXFIFO_SIZE) -7:
 *			00000: 128 bytes
 *			00001: 256 bytes
 *			00010: 512 bytes
 *			00011: 1,024 bytes
 *			00100: 2,048 bytes
 *			00101: 4,096 bytes
 *			00110: 8,192 bytes
 *			00111: 16,384 bytes
 *			01000: 32 KB
 *			01001: 64 KB
 *			01010: 128 KB
 *			01011-11111: Reserved.
 *	@adv_ts_hword: It set to 1 when Advance timestamping High Word selected.
 *	@addr_64: Address Width.
 *		This field indicates the configured address width:
 *			00: 32
 *			01: 40
 *			10: 48
 *			11: Reserved
 *	@dcb_en: It sets to 1 when DCB Feature Enable.
 *	@sph_en: It sets to 1 when Split Header Feature Enable.
 *	@tso_en: It sets to 1 when TCP Segmentation Offload Enable.
 *	@dma_debug_gen:	It seys to 1 when DMA debug registers are enabled.
 *	@av_sel: It sets to 1 AV Feature Enabled.
 *	@hash_tbl_sz: This field indicates the size of the hash table:
 *			00: No hash table
 *			01: 64
 *			10: 128
 *			11: 256.
 *	@l3l4_filter_num: This field indicates the total number of L3 or L4
 *			  filters:
 *			0000: No L3 or L4 Filter
 *			0001: 1 L3 or L4 Filter
 *			0010: 2 L3 or L4 Filters
 *			..
 *			1000: 8 L3 or L4.
 *	@rx_q_cnt: It holds number of MTL Receive Queues.
 *	@tx_q_cnt: It holds number of MTL Transmit Queues.
 *	@rx_ch_cnt: It holds number of DMA Receive channels.
 *	@tx_ch_cnt: This field indicates the number of DMA Transmit channels:
 *		0000: 1 DMA Tx Channel
 *		0001: 2 DMA Tx Channels
 *		..
 *		0111: 8 DMA Tx.
 *	@pps_out_num: This field indicates the number of PPS outputs:
 *			000: No PPS output
 *			001: 1 PPS output
 *			010: 2 PPS outputs
 *			011: 3 PPS outputs
 *			100: 4 PPS outputs
 *			101-111: Reserved
 *	@aux_snap_num: Number of Auxiliary Snapshot Inputs
 *		This field indicates the number of auxiliary snapshot inputs:
 *			000: No auxiliary input
 *			001: 1 auxiliary input
 *			010: 2 auxiliary inputs
 *			011: 3 auxiliary inputs
 *			100: 4 auxiliary inputs
 *			101-111: Reserved
 */
struct osi_hw_features {
	/* HW Feature Register0 */
	unsigned int mii_sel;
	unsigned int gmii_sel;
	unsigned int hd_sel;
	unsigned int pcs_sel;
	unsigned int vlan_hash_en;
	unsigned int sma_sel;
	unsigned int rwk_sel;
	unsigned int mgk_sel;
	unsigned int mmc_sel;
	unsigned int arp_offld_en;
	unsigned int ts_sel;
	unsigned int eee_sel;
	unsigned int tx_coe_sel;
	unsigned int rx_coe_sel;
	unsigned int mac_addr16_sel;
	unsigned int mac_addr32_sel;
	unsigned int mac_addr64_sel;
	unsigned int tsstssel;
	unsigned int sa_vlan_ins;
	unsigned int act_phy_sel;
	/* HW Feature Register1 */
	unsigned int rx_fifo_size;
	unsigned int tx_fifo_size;
	unsigned int adv_ts_hword;
	unsigned int addr_64;
	unsigned int dcb_en;
	unsigned int sph_en;
	unsigned int tso_en;
	unsigned int dma_debug_gen;
	unsigned int av_sel;
	unsigned int hash_tbl_sz;
	unsigned int l3l4_filter_num;
	/* HW Feature Register2 */
	unsigned int rx_q_cnt;
	unsigned int tx_q_cnt;
	unsigned int rx_ch_cnt;
	unsigned int tx_ch_cnt;
	unsigned int pps_out_num;
	unsigned int aux_snap_num;
};

/**
 *	osi_readl - Read a memory mapped regsiter.
 *	@addr:	Memory mapped address.
 *
 *	Algorithm: None.
 *
 *	Dependencies: Physical address has to be memmory mapped.
 *
 *	Protection: None.
 *
 *	Return: Data from memory mapped register - success.
 */
static inline unsigned int osi_readl(void *addr)
{
	return *(volatile unsigned int *)addr;
}

/**
 *	osi_writel - Write to a memory mapped regsiter.
 *	@val:	Value to be written.
 *	@addr:	Memory mapped address.
 *
 *	Algorithm: None.
 *
 *	Dependencies: Physical address has to be memmory mapped.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
static inline void osi_writel(unsigned int val, void *addr)
{
	*(volatile unsigned int *)addr = val;
}

/**
 *	is_valid_mac_version - Check if read MAC IP is valid or not.
 *	@mac_ver: MAC version read.
 *
 *	Algorithm: None.
 *
 *      Dependencies: MAC has to be out of reset.
 *
 *	Protection: None.
 *
 *      Return: 0 - for not Valid MAC, 1 - for Valid MAC
 */
static inline int is_valid_mac_version(unsigned int mac_ver)
{
	if ((mac_ver == OSI_EQOS_MAC_4_10) ||
	    (mac_ver == OSI_EQOS_MAC_5_00) ||
	    (mac_ver == OSI_EQOS_MAC_5_10)) {
		return 1;
	}

	return 0;
}

/**
 *      osi_get_mac_version - Reading MAC version
 *      @addr: io-remap MAC base address.
 *	@mac_ver: holds mac version.
 *
 *      Algorithm: Reads MAC version and check whether its valid or not.
 *
 *      Dependencies: MAC has to be out of reset.
 *
 *      Protection: None
 *
 *      Return: 0 - success, -1 - failure
 */
int osi_get_mac_version(void *addr, unsigned int *mac_ver);

void osi_get_hw_features(void *base, struct osi_hw_features *hw_feat);
#endif /* OSI_COMMON_H */
