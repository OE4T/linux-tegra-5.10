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

#ifndef OSI_CORE_H
#define OSI_CORE_H

#include "osi_common.h"
#include "mmc.h"

struct osi_core_priv_data;

/**
 *	struct osi_filter - The OSI core structure for filters
 *	@pr_mode: promiscuous mode
 *	@huc_mode: hash unicast
 *	@hmc_mode: hash milticast
 *	@pm_mode: pass all multicast
 *	@hpf_mode: hash or perfact filter
 */
struct osi_filter {
	unsigned int pr_mode;
	unsigned int huc_mode;
	unsigned int hmc_mode;
	unsigned int pm_mode;
	unsigned int hpf_mode;
};

/**
 *	Structure osi_l3_l4_filter - L3/L4 filter Function dependent
 *	parameter
 *
 *	@filter_no:	filter index 0- 7
 *	@filter_enb_dis:	enable/disable
 *	@src_dst_addr_match:	source(0) or destination(1)
 *	@perfect_inverse_match:	perfect(0) or inverse(1)
 *	@ip4_addr:	ipv4 address
 *	@ip6_addr:	ipv6 address
 *	@port_no:	Port number
 */
struct osi_l3_l4_filter {
	unsigned int filter_no;
	unsigned int filter_enb_dis;
	unsigned int src_dst_addr_match;
	unsigned int perfect_inverse_match;
	unsigned char ip4_addr[4];
	unsigned short ip6_addr[8];
	unsigned short port_no;
};

/**
 *	Structure osi_vlan_filter - Vlan filter Function dependent parameter
 *
 *	@filter_enb_dis: enable/disable
 *	@perfect_hash: perfect(0) or hash(1)
 *	@perfect_inverse_match:	perfect(0) or inverse(1)
 */
struct osi_vlan_filter {
	unsigned int filter_enb_dis;
	unsigned int perfect_hash;
	unsigned int perfect_inverse_match;
};

/**
 *	Struct osi_l2_da_filter - L2 filter function depedent parameter
 *	@perfect_hash: perfect(0) or hash(1)
 *	@perfect_inverse_match:	perfect(0) or inverse(1)
 */
struct osi_l2_da_filter {
	unsigned int perfect_hash;
	unsigned int perfect_inverse_match;
};

/**
 *	struct osi_core_avb_algorithm - The OSI Core avb data structure per
 *	queue.
 *	@qindex: TX Queue/TC index
 *	@algo: AVB algorithm 1:CBS
 *	@credit_control: credit control When this bit is set, the accumulated
 *	credit. Parameter in the credit-based shaper algorithm logic is not
 *	reset to zero when there is positive credit and no packet to transmit
 *	in Channel
 *	@idle_slope: idleSlopeCredit value required for CBS
 *	@send_slope: sendSlopeCredit value required for CBS
 *	@hi_credit: hiCredit value required for CBS
 *	@low_credit: lowCredit value required for CBS
 *	@oper_mode: Transmit queue enable 01: avb 10: enable 00: disable
 */
struct  osi_core_avb_algorithm {
	unsigned int qindex;
	unsigned int algo;
	unsigned int credit_control;
	unsigned int idle_slope;
	unsigned int send_slope;
	unsigned int hi_credit;
	unsigned int low_credit;
	unsigned int oper_mode;
};

/**
 *	struct osi_core_ops - Core (MAC & MTL) operations.
 *	@poll_for_swr: Called to poll for software reset bit.
 *	@core_init: Called to initialize MAC and MTL registers.
 *	@start_mac: Called to start MAC Tx and Rx engine.
 *	@stop_mac: Called to stop MAC Tx and Rx engine.
 *	@handle_common_intr: Called to handle common interrupt.
 *	@set_mode: Called to set the mode at MAC (full/duplex).
 *	@set_speed: Called to set the speed (10/100/1000) at MAC.
 *	@pad_calibrate: Called to do pad caliberation.
 *	@set_mdc_clk_rate: Called to set MDC clock rate for MDIO operation.
 *	@flush_mtl_tx_queue: Called to flush MTL Tx queue.
 *	@config_mac_loopback: Called to configure MAC in loopback mode.
 *	@set_avb_algorithm: Called to set av parameter.
 *	@get_avb_algorithm: Called to get av parameter,
 *	@config_fw_err_pkts: Called to configure MTL RxQ to forward the err pkt.
 *	@config_tx_status: Called to configure the MTL to forward/drop tx status
 *	@config_rx_crc_check: Called to configure the MAC rx crc.
 *	@config_flow_control: Called to configure the MAC flow control.
 *	@config_arp_offload: Called to enable/disable HW ARP offload feature.
 *	@config_rxcsum_offload: Called to configure Rx Checksum offload engine.
 *	@config_mac_pkt_filter_reg: Called to config mac packet filter.
 *	@update_mac_addr_low_high_reg: Called to update MAC address 1-127.
 *	@config_l3_l4_filter_enable: Called to configure l3/L4 filter.
 *	@config_l2_da_perfect_inverse_match: Called to configure L2 DA filter.
 *	@config_l3_filters: Called to configure L3 filter.
 *	@update_ip4_addr: Called to update ip4 src or desc address.
 *	@update_ip6_addr: Called to update ip6 address.
 *	@config_l4_filters: Called to configure L4 filter.
 *	@update_l4_port_no: Called to update L4 Port for filter packet.
 *	@config_vlan_filtering: Called to configure VLAN filtering.
 *	@update_vlan_id: called to update VLAN id.
 *	@set_systime_to_mac: Called to set current system time to MAC.
 *	@config_addend: Called to set the addend value to adjust the time.
 *	@adjust_systime: Called to adjust the system time.
 *	@get_systime_from_mac: Called to get the current time from MAC.
 *	@config_tscr: Called to configure the TimeStampControl register.
 *	@config_ssir: Called to configure the sub second increment register.
 *	@read_mmc: called to update MMC counter from HW register
 *	@reset_mmc: called to reset MMC HW counter and Structure
 */
struct osi_core_ops {
	/* initialize MAC/MTL/DMA Common registers */
	int (*poll_for_swr)(void *ioaddr);
	int (*core_init)(struct osi_core_priv_data *osi_core,
			 unsigned int tx_fifo_size,
			 unsigned int rx_fifo_size);
	void (*start_mac)(void *addr);
	void (*stop_mac)(void *addr);
	void (*handle_common_intr)(struct osi_core_priv_data *osi_core);
	void (*set_mode)(void *ioaddr, int mode);
	void (*set_speed)(void *ioaddr, int speed);
	int (*pad_calibrate)(void *ioaddr);
	void (*set_mdc_clk_rate)(struct osi_core_priv_data *osi_core,
				 unsigned long csr_clk_rate);
	int (*flush_mtl_tx_queue)(void *ioaddr, unsigned int qinx);
	int (*config_mac_loopback)(void *addr, unsigned int lb_mode);
	int (*set_avb_algorithm)(struct osi_core_priv_data *osi_core,
				 struct osi_core_avb_algorithm *avb);
	int (*get_avb_algorithm)(struct osi_core_priv_data *osi_core,
				 struct osi_core_avb_algorithm *avb);
	int (*config_fw_err_pkts)(void *addr, unsigned int qinx,
				  unsigned int fw_err);
	int (*config_tx_status)(void *addr, unsigned int tx_status);
	int (*config_rx_crc_check)(void *addr, unsigned int crc_chk);
	int (*config_flow_control)(void *addr, unsigned int flw_ctrl);
	int (*config_arp_offload)(unsigned int mac_ver, void *addr,
				  unsigned int enable, unsigned char *ip_addr);
	int (*config_rxcsum_offload)(void *addr, unsigned int enabled);
	void (*config_mac_pkt_filter_reg)(struct osi_core_priv_data *osi_core,
					  struct osi_filter filter);
	int (*update_mac_addr_low_high_reg)(
					struct osi_core_priv_data *osi_core,
					    unsigned int index,
					    unsigned char value[],
					    unsigned int dma_routing_enable,
					    unsigned int dma_chan,
					    unsigned int addr_mask,
					    unsigned int src_dest);
	int (*config_l3_l4_filter_enable)(void *base, unsigned int enable);
	int (*config_l2_da_perfect_inverse_match)(void *base, unsigned int
						  perfect_inverse_match);
	int (*config_l3_filters)(struct osi_core_priv_data *osi_core,
				 unsigned int filter_no, unsigned int enb_dis,
				 unsigned int ipv4_ipv6_match,
				 unsigned int src_dst_addr_match,
				 unsigned int perfect_inverse_match,
				 unsigned int dma_routing_enable,
				 unsigned int dma_chan);
	int (*update_ip4_addr)(struct osi_core_priv_data *osi_core,
			       unsigned int filter_no, unsigned char addr[],
			       unsigned int src_dst_addr_match);
	int (*update_ip6_addr)(struct osi_core_priv_data *osi_core,
			       unsigned int filter_no, unsigned short addr[]);
	int (*config_l4_filters)(struct osi_core_priv_data *osi_core,
				 unsigned int filter_no, unsigned int enb_dis,
				 unsigned int tcp_udp_match,
				 unsigned int src_dst_port_match,
				 unsigned int perfect_inverse_match,
				 unsigned int dma_routing_enable,
				 unsigned int dma_chan);
	int (*update_l4_port_no)(struct osi_core_priv_data *osi_core,
				 unsigned int filter_no, unsigned short port_no,
				 unsigned int src_dst_port_match);

	/* for VLAN filtering */
	int (*config_vlan_filtering)(struct osi_core_priv_data *osi_core,
				     unsigned int filter_enb_dis,
				     unsigned int perfect_hash_filtering,
				     unsigned int perfect_inverse_match);
	int (*update_vlan_id)(void *base, unsigned int vid);
	int (*set_systime_to_mac)(void *addr, unsigned int sec,
				  unsigned int nsec);
	int (*config_addend)(void *addr, unsigned int addend);
	int (*adjust_systime)(void *addr, unsigned int sec, unsigned int nsec,
			      unsigned int neg_adj,
			      unsigned int one_nsec_accuracy);
	unsigned long long (*get_systime_from_mac)(void *addr);
	void (*config_tscr)(void *addr, unsigned int ptp_filter);
	void (*config_ssir)(void *addr, unsigned int ptp_clock);
	void (*read_mmc)(struct osi_core_priv_data *osi_core);
	void (*reset_mmc)(struct osi_core_priv_data *osi_core);
};

/**
 *	struct osi_ptp_config - PTP configuration
 *	@ptp_filter: PTP filter parameters bit fields.
 *	Enable Time stamp,Fine Timestamp,1 nanosecond accuracy are enabled by
 *	default.
 *	Need to set below bit fields accordingly as per the requirements.
 *	Enable Timestamp for All Packets			OSI_BIT(8)
 *	Enable PTP Packet Processing for Version 2 Format	OSI_BIT(10)
 *	Enable Processing of PTP over Ethernet Packets		OSI_BIT(11)
 *	Enable Processing of PTP Packets Sent over IPv6-UDP	OSI_BIT(12)
 *	Enable Processing of PTP Packets Sent over IPv4-UDP	OSI_BIT(13)
 *	Enable Timestamp Snapshot for Event Messages		OSI_BIT(14)
 *	Enable Snapshot for Messages Relevant to Master		OSI_BIT(15)
 *	Select PTP packets for Taking Snapshots			OSI_BIT(16)
 *	Select PTP packets for Taking Snapshots			OSI_BIT(17)
 *	Select PTP packets for Taking Snapshots	(OSI_BIT(16) | OSI_BIT(17))
 *	AV 802.1AS Mode Enable					OSI_BIT(28)
 *	if ptp_fitler is set to Zero then Time stamping is disabled.
 *	@sec: Seconds
 *	@nsec: Nano seconds
 *	@ptp_ref_clk_rate: PTP reference clock read from DT
 *	@one_nsec_accuracy: Use one nsec accuracy (need to set 1)
 *	@ptp_clock: PTP system clock which is 62500000Hz
 */
struct osi_ptp_config {
	unsigned int ptp_filter;
	unsigned int sec;
	unsigned int nsec;
	unsigned int ptp_ref_clk_rate;
	unsigned int one_nsec_accuracy;
	unsigned int ptp_clock;
};

/**
 *	struct osi_core_priv_data - The OSI Core (MAC & MTL) private data
				    structure.
 *	@base: Memory mapped base address of MAC IP.
 *	@osd:	Pointer to OSD private data structure.
 *	@ops: Address of HW Core operations structure.
 *	@num_mtl_queues: Number of MTL queues enabled in MAC.
 *	@mtl_queues: Array of MTL queues.
 *	@rxq_ctrl: List of MTL Rx queue mode that need to be enabled
 *	@rxq_prio: Rx MTl Queue mapping based on User Priority field
 *	@mac: MAC HW type EQOS based on DT compatible.
 *	@mac_ver: MAC version.
 *	@mdc_cr: MDC clock rate.
 *	@mtu: MTU size
 *	@mac_addr: Ethernet MAC address.
 *	@pause_frames: DT entry to enable(0) or disable(1) pause frame support
 *	@flow_ctrl: Current flow control settings
 *	@ptp_config: PTP configuration settings.
 *	@default_addend: Default addend value.
 *	@mmc: mmc counter structure
 *	@xstats: xtra sw error counters
 *	@dcs_en: DMA channel selection enable (1)
 */
struct osi_core_priv_data {
	void *base;
	void *osd;
	struct osi_core_ops *ops;
	unsigned int num_mtl_queues;
	unsigned int mtl_queues[OSI_EQOS_MAX_NUM_CHANS];
	unsigned int rxq_ctrl[OSI_EQOS_MAX_NUM_CHANS];
	unsigned int rxq_prio[OSI_EQOS_MAX_NUM_CHANS];
	unsigned int mac;
	unsigned int mac_ver;
	unsigned int mdc_cr;
	unsigned int mtu;
	unsigned char mac_addr[OSI_ETH_ALEN];
	unsigned int pause_frames;
	unsigned int flow_ctrl;
	struct osi_ptp_config ptp_config;
	unsigned int default_addend;
	struct osi_mmc_counters mmc;
	struct osi_xtra_stat_counters xstats;
	unsigned int dcs_en;
};

/**
 *	osi_poll_for_swr - Poll Software reset bit in MAC HW
 *	@osi: OSI Core private data structure.
 *
 *	Algorithm: Invokes EQOS routine to check for SWR (software reset)
 *	bit in DMA Basic mooe register to make sure IP reset was successful.
 *
 *	Dependencies:
 *	1) MAC needs to be out of reset and proper clock configured.
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure
 */

int osi_poll_for_swr(struct osi_core_priv_data *osi_core);

/**
 *	osi_set_mdc_clk_rate - Derive MDC clock based on provided AXI_CBB clk.
 *	@osi: OSI core private data structure.
 *	@csr_clk_rate: CSR (AXI CBB) clock rate.
 *
 *	Algorithm: MDC clock rate will be populated in OSI core private data
 *	structure based on AXI_CBB clock rate.
 *
 *	Dependencies:
 *	1) OSD layer needs get the AXI CBB clock rate with OSD clock API
 *	(ex - clk_get_rate())
 *
 *	Return: None
 */
void osi_set_mdc_clk_rate(struct osi_core_priv_data *osi_core,
			  unsigned long csr_clk_rate);

/**
 *	osi_hw_core_init - EQOS MAC, MTL and common DMA initialization.
 *	@osi: OSI core private data structure.
 *
 *	Algorithm: Invokes EQOS MAC, MTL and common DMA register init code.
 *
 *	Dependencies:
 *	1) MAC should be out of reset. See osi_poll_for_swr() for details.
 *	2) osi_core->base needs to be filled based on ioremap.
 *	3) osi_core->num_mtl_queues needs to be filled.
 *	4) osi_core->mtl_queues[qinx] need to be filled.
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure
 */
int osi_hw_core_init(struct osi_core_priv_data *osi_core,
		     unsigned int tx_fifo_size,
		     unsigned int rx_fifo_size);

/**
 *	osi_start_mac - Start MAC Tx/Rx engine
 *	@osi_core: OSI core private data.
 *
 *	Algorimthm: Enable MAC Tx and Rx engine.
 *
 *	Dependencies:
 *	1) MAC init should be complete. See osi_hw_core_init() and
 *	osi_hw_dma_init()
 *
 *	Protection: None
 *
 *	Return: None
 */
void osi_start_mac(struct osi_core_priv_data *osi_core);

/**
 *	osi_stop_mac - Stop MAC Tx/Rx engine
 *	@osi_core: OSI core private data.
 *
 *	Algorimthm: Stop MAC Tx and Rx engine
 *
 *	Dependencies:
 *	1) MAC DMA deinit should be complete. See osi_hw_dma_deinit()
 *
 *	Protection: None
 *
 *	Return: None
 */
void osi_stop_mac(struct osi_core_priv_data *osi_core);

/**
 *	osi_common_isr - Common ISR.
 *	@osi_core: OSI core private data structure.
 *
 *	Algorithm: Takes care of handling the
 *	common interrupts accordingly as per the
 *	MAC IP
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure
 */
void osi_common_isr(struct osi_core_priv_data *osi_core);

/**
 *	osi_set_mode - Set FD/HD mode.
 *	@osi: OSI private data structure.
 *	@mode: Operating mode.
 *
 *	Algorithm: Takes care of  setting HD or FD mode
 *	accordingly as per the MAC IP.
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *
 *	Protection: None
 *
 *	Return: NONE
 */
void osi_set_mode(struct osi_core_priv_data *osi_core, int mode);

/**
 *	osi_set_speed - Set operating speed.
 *	@osi: OSI private data structure.
 *	@speed: Operating speed.
 *
 *	Algorithm: Takes care of  setting the operating
 *	speed accordingly as per the MAC IP.
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *
 *	Protection: None
 *
 *	Return: NONE
 */
void osi_set_speed(struct osi_core_priv_data *osi_core, int speed);

/**
 *	osi_pad_calibrate - PAD calibration
 *	@osi: OSI core private data structure.
 *
 *	Algorithm: Takes care of  doing the pad calibration
 *	accordingly as per the MAC IP.
 *
 *	Dependencies:
 *	1) MAC should out of reset and clocks enabled.
 *	2) RGMII and MDIO interface needs to be IDLE before performing PAD
 *	calibration.
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure
 */
int osi_pad_calibrate(struct osi_core_priv_data *osi_core);

/**
 *	osi_flush_mtl_tx_queue - Flushing a MTL Tx Queue.
 *	@osi_core: OSI private data structure.
 *	@qinx: MTL queue index.
 *
 *	Algorithm: Invokes EQOS flush Tx queue routine.
 *
 *	Dependencies:
 *	1) MAC should out of reset and clocks enabled.
 *	2) hw core initialized. see osi_hw_core_init().
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure.
 */
int osi_flush_mtl_tx_queue(struct osi_core_priv_data *osi_core,
			   unsigned int qinx);

/**
 *	osi_config_mac_loopback - Configure MAC loopback
 *	@osi: OSI private data structure.
 *	@lb_mode: Enable or disable MAC loopback
 *
 *	Algorithm: Configure the MAC to support the loopback.
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure.
 */
int osi_config_mac_loopback(struct osi_core_priv_data *osi_core,
			    unsigned int lb_mode);

/**
 *	osi_set_avb - Set CBS algo and parameters
 *	@osi: OSI core private data structure.
 *	@avb: osi core avb data structure.
 *
 *	Algorithm: Set AVB algo and  populated parameter from osi_core_avb
 *	structure for TC/TQ
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *	2) osi_core->osd should be populated.
 *
 *	Return: Success = 0; failure = -1;
 */
int osi_set_avb(struct osi_core_priv_data *osi_core,
		struct osi_core_avb_algorithm *avb);

/**	osi_get_avb - Get CBS algo and parameters
 *	@osi: OSI core private data structure.
 *	@avb: osi core avb data structure.
 *
 *	Algorithm: get AVB algo and  populated parameter from osi_core_avb
 *	structure for TC/TQ
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *	2) osi_core->osd should be populated.
 *
 *	Return: Success = 0; failure = -1;
 */
int osi_get_avb(struct osi_core_priv_data *osi_core,
		struct osi_core_avb_algorithm *avb);

/**
 *	osi_configure_txstatus - Configure Tx packet status reporting
 *	@osi_core: OSI private data structure.
 *	@tx_status: Enable or disable tx packet status reporting
 *
 *	Algorithm: Configure MAC to enable/disable Tx status error
 *	reporting.
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure.
 */
int osi_configure_txstatus(struct osi_core_priv_data *osi_core,
			   unsigned int tx_status);

/**
 *	osi_config_fw_err_pkts - Configure forwarding of error packets
 *	@osi_core: OSI core private data structure.
 *	@qinx: Q index
 *	@fw_err: Enable or disable forwarding of error packets
 *
 *	Algorithm: Configure MAC to enable/disable forwarding of error packets.
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure.
 */
int osi_config_fw_err_pkts(struct osi_core_priv_data *osi_core,
			   unsigned int qinx, unsigned int fw_err);

/**
 *	osi_config_rx_crc_check - Configure CRC Checking for Received Packets
 *	@osi_core: OSI core private data structure.
 *	@crc_chk: Enable or disable checking of CRC field in received packets
 *
 *	Algorithm: When this bit is set, the MAC receiver does not check the CRC
 *	field in the received packets. When this bit is reset, the MAC receiver
 *	always checks the CRC field in the received packets.
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure.
 */
int osi_config_rx_crc_check(struct osi_core_priv_data *osi_core,
			    unsigned int crc_chk);

/**
 *	osi_configure_flow_ctrl - Configure flow control settings
 *	@osi_core: OSI core private data structure.
 *	@crc_chk: Enable or disable flow control settings
 *
 *	Algorithm: This will enable or disable the flow control.
 *	flw_ctrl BIT0 is for tx flow ctrl enable/disable
 *	flw_ctrl BIT1 is for rx flow ctrl enable/disable
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure.
 */
int osi_configure_flow_control(struct osi_core_priv_data *osi_core,
			       unsigned int flw_ctrl);

/**	osi_config_arp_offload - Configure ARP offload in MAC.
 *	@osi_core: OSI private data structure.
 *	@flags: Enable/disable flag.
 *	@ip_addr: Char array representation of IP address
 *	to be set in HW to compare with ARP requests received.
 *
 *	Algorithm: Invokes EQOS config ARP offload routine.
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *	2) Valid 4 byte IP address as argument @ip_addr
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure
 */
int osi_config_arp_offload(struct osi_core_priv_data *osi_core,
			   unsigned int flags,
			   unsigned char *ip_addr);

/*
 *	osi_config_rxcsum_offload - Configure RX checksum offload in MAC.
 *	@osi_core: OSI private data structure.
 *	@enable: Enable/disable flag.
 *
 *	Algorithm: Invokes EQOS config RX checksum offload routine.
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure
 */
int osi_config_rxcsum_offload(struct osi_core_priv_data *osi_core,
			      unsigned int enable);

/**
 *	osi_config_mac_pkt_filter_reg - configure mac filter register.
 *	@osi_core: OSI private data structure.
 *	@pfilter: OSI filter structure.
 *
 *	Algorithm: This sequence is used to configure MAC in differnet pkt
 *	processing modes like promiscuous, multicast, unicast,
 *	hash unicast/multicast.
 *
 *	Dependencies:
 *	1) MAC should be initialized and started. see osi_start_mac()
 *	2) MAC addresses should be configured in HW registers. see
 *	osi_update_mac_addr_low_high_reg().
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure.
 */
int osi_config_mac_pkt_filter_reg(struct osi_core_priv_data *osi_core,
				  struct osi_filter pfilter);

/**
 *	osi_update_mac_addr_low_high_reg- invoke API to update L2 address
 *	in filter register
 *
 *	@osi_core: OSI private data structure.
 *	@index: filter index
 *	@value: MAC address to write
 *	@dma_routing_enable: dma channel routing enable(1)
 *	@dma_chan: dma channel number
 *	@addr_mask: filter will not consider byte in comparison
 *	Bit 29: MAC_Address${i}_High[15:8]
 *	Bit 28: MAC_Address${i}_High[7:0]
 *	Bit 27: MAC_Address${i}_Low[31:24]
 *	..
 *	Bit 24: MAC_Address${i}_Low[7:0]
 *	@src_dest: SA(1) or DA(0)
 *
 *	Algorithm: This routine update MAC address to register for filtering
 *	based on dma_routing_enable, addr_mask and src_dest. Validation of
 *	dma_chan as well as DCS bit enabled in RXQ to DMA mapping register
 *	performed before updating DCS bits.
 *
 *	Dependencies:
 *	1) MAC should be initialized and stated. see osi_start_mac()
 *	2) osi_core->osd should be populated.
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure.
 */
int osi_update_mac_addr_low_high_reg(struct osi_core_priv_data *osi_core,
				     unsigned int index,
				     unsigned char value[],
				     unsigned int dma_routing_enable,
				     unsigned int dma_chan,
				     unsigned int addr_mask,
				     unsigned int src_dest);

/**
 *	osi_config_l3_l4_filter_enable -  invoke OSI call to eanble L3/L4
 *	filters.
 *
 *	@osi_core: OSI private data structure.
 *	@enable: enable/disable
 *
 *	Algorithm: This routine to enable/disable L4/l4 filter
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure.
 */
int osi_config_l3_l4_filter_enable(struct osi_core_priv_data *osi_core,
				   unsigned int enable);

/**
 *	osi_config_l3_filters - invoke OSI call config_l3_filters.
 *
 *	@osi_core: OSI private data structure.
 *	@filter_no: filter index
 *	@enb_dis:  1 - enable otherwise - disable L3 filter
 *	@ipv4_ipv6_match: 1 - IPv6, otherwise - IPv4
 *	@src_dst_addr_match: 0 - source, otherwise - destination
 *	@perfect_inverse_match: normal match(0) or inverse map(1)
 *	@dma_routing_enable: filter based dma routing enable(1)
 *	@dma_chan: dma channel for routing based on filter
 *
 *	Algorithm: Check for DCS_enable as well as validate channel
 *      number and if dcs_enable is set. After validation, code flow
 *	is used to configure L3((IPv4/IPv6) filters resister
 *	for address matching.
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *	2) L3/L4 filtering should be enabled in MAC PFR register. See
 *	osi_config_l3_l4_filter_enable()
 *	3) osi_core->osd should be populated
 *	4) DCS bits should be enabled in RXQ to DMA map register
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure.
 */
int osi_config_l3_filters(struct osi_core_priv_data *osi_core,
			  unsigned int filter_no,
			  unsigned int enb_dis,
			  unsigned int ipv4_ipv6_match,
			  unsigned int src_dst_addr_match,
			  unsigned int perfect_inverse_match,
			  unsigned int dma_routing_enable,
			  unsigned int dma_chan);

/**
 *	osi_update_ip4_addr -  invoke OSI call update_ip4_addr.
 *	@osi_core: OSI private data structure.
 *	@filter_no: filter index
 *	@addr: ipv4 address
 *	@src_dst_addr_match: 0- source(addr0) 1- dest (addr1)
 *
 *	Algorithm:  This sequence is used to update IPv4 source/destination
 *	Address for L3 layer filtering
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *	2) L3/L4 filtering should be enabled in MAC PFR register. See
 *	osi_config_l3_l4_filter_enable()
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure.
 */
int osi_update_ip4_addr(struct osi_core_priv_data *osi_core,
			unsigned int filter_no,
			unsigned char addr[],
			unsigned int src_dst_addr_match);

/**
 *	osi_update_ip6_addr -  invoke OSI call update_ip6_addr.
 *	@osi_core: OSI private data structure.
 *	@filter_no: filter index
 *	@addr: ipv6 adderss
 *
 *	Algorithm:  This sequence is used to update IPv6 source/destination
 *	Address for L3 layer filtering
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *	2) L3/L4 filtering should be enabled in MAC PFR register. See
 *	osi_config_l3_l4_filter_enable()
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure.
 */
int osi_update_ip6_addr(struct osi_core_priv_data *osi_core,
			unsigned int filter_no,
			unsigned short addr[]);

/**
 *	osi_config_l4_filters - invoke OSI call config_l4_filters.
 *
 *	@osi_core: OSI private data structure.
 *	@filter_no: filter index
 *	@enb_dis: enable/disable L4 filter
 *	@tcp_udp_match: 1 - udp, 0 - tcp
 *	@src_dst_port_match: port matching enable/disable
 *	@perfect_inverse_match: normal match(0) or inverse map(1)
 *	@dma_routing_enable: filter based dma routing enable(1)
 *	@dma_chan: dma channel for routing based on filter
 *
 *	Algorithm: This sequence is used to configure L4(TCP/UDP) filters for
 *	SA and DA Port Number matching
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *	2) L3/L4 filtering should be enabled in MAC PFR register. See
 *	osi_config_l3_l4_filter_enable()
 *	3) osi_core->osd should be populated
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure.
 */
int osi_config_l4_filters(struct osi_core_priv_data *osi_core,
			  unsigned int filter_no,
			  unsigned int enb_dis,
			  unsigned int tcp_udp_match,
			  unsigned int src_dst_port_match,
			  unsigned int perfect_inverse_match,
			  unsigned int dma_routing_enable,
			  unsigned int dma_chan);

/**
 *	osi_update_l4_port_no - invoke OSI call for
 *	update_l4_port_no.
 *
 *	@osi_core: OSI private data structure.
 *	@filter_no: filter index
 *	@port_no: port number
 *	@src_dst_port_match: source port - 0, dest port - 1
 *
 *	Algoriths sequence is used to update Source Port Number for
 *	L4(TCP/UDP) layer filtering.
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *	2) L3/L4 filtering should be enabled in MAC PFR register. See
 *	osi_config_l3_l4_filter_enable()
 *	3) osi_core->osd should be populated
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure.
 */
int osi_update_l4_port_no(struct osi_core_priv_data *osi_core,
			  unsigned int filter_no, unsigned short port_no,
			  unsigned int src_dst_port_match);

/**
 *	osi_config_vlan_filter_reg - invoke OSI call for
 *	config_vlan_filtering.
 *
 *	@osi_core: OSI private data structure.
 *	@filter_enb_dis: vlan filter enable/disable
 *	@perfect_hash_filtering: perfect or hash filter
 *	@perfect_inverse_match: normal or inverse filter
 *
 *	Algorithm: This sequence is used to enable/disable VLAN filtering and
 *	also selects VLAN filtering mode- perfect/hash
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *	2) osi_core->osd should be populated
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure.
 */
int osi_config_vlan_filtering(struct osi_core_priv_data *osi_core,
			      unsigned int filter_enb_dis,
			      unsigned int perfect_hash_filtering,
			      unsigned int perfect_inverse_match);

/**
 *	osi_config_l2_da_perfect_inverse_match - trigger OSI call for
 *	config_l2_da_perfect_inverse_match.
 *
 *	@osi_core: OSI private data structure.
 *	@perfect_inverse_match: 1 - inverse mode 0- normal mode
 *
 *	Algorithm: This sequence is used to select perfect/inverse matching
 *	for L2 DA
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure.
 */
int  osi_config_l2_da_perfect_inverse_match(struct osi_core_priv_data *osi_core,
					    unsigned int perfect_inverse_match);

/**
 *	osi_update_vlan_id - invoke osi call to update VLAN ID
 *
 *	@osi_core: OSI private data structure.
 *	@vid: VLAN ID
 *
 *	Algorithm: return 16 bit VLAN ID
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure.
 */
int  osi_update_vlan_id(struct osi_core_priv_data *osi_core,
			unsigned int vid);

/**
 *	osi_write_phy_reg - Write to a PHY register through MAC over MDIO bus.
 *	@osi_core: OSI private data structure.
 *	@phyaddr: PHY address (PHY ID) associated with PHY
 *	@phyreg: Register which needs to be write to PHY.
 *	@phydata: Data to write to a PHY register.
 *
 *	Algorithm:
 *	1) Before proceding for reading for PHY register check whether any MII
 *	operation going on MDIO bus by polling MAC_GMII_BUSY bit.
 *	2) Program data into MAC MDIO data register.
 *	3) Populate required parameters like phy address, phy register etc,,
 *	in MAC MDIO Address register. write and GMII busy bits needs to be set
 *	in this operation.
 *	4) Write into MAC MDIO address register poll for GMII busy for MDIO
 *	operation to complete.
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure
 */
int osi_write_phy_reg(struct osi_core_priv_data *osi_core, unsigned int phyaddr,
		      unsigned int phyreg, unsigned short phydata);

/**
 *	osi_read_mmc - invoke function to read actual registers and update
 *	structure variable mmc
 *
 *	@osi_core: OSI core private data structure.
 *
 *	Algorithm: Read the registers, mask reserve bits if requied, update
 *	structure.
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *	2) osi_core->osd should be populated
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure.
 */
int osi_read_mmc(struct osi_core_priv_data *osi_core);

/**
 *	osi_reset_mmc - invoke function to reset MMC counter and data structure
 *
 *	@osi_core: OSI core private data structure.
 *
 *	Algorithm: Read the registers, mask reserve bits if requied, update
 *	structure.
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *	2) osi_core->osd should be populated
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure.
 */
int osi_reset_mmc(struct osi_core_priv_data *osi_core);

/**
 *	osi_read_phy_reg - Read from a PHY register through MAC over MDIO bus.
 *	@osi_core: OSI private data structure.
 *	@phyaddr: PHY address (PHY ID) associated with PHY
 *	@phyreg: Register which needs to be read from PHY.
 *
 *	Algorithm:
 *	1) Before proceding for reading for PHY register check whether any MII
 *	operation going on MDIO bus by polling MAC_GMII_BUSY bit.
 *	2) Populate required parameters like phy address, phy register etc,,
 *	in program it in MAC MDIO Address register. Read and GMII busy bits
 *	needs to be set in this operation.
 *	3) Write into MAC MDIO address register poll for GMII busy for MDIO
 *	operation to complete. After this data will be available at MAC MDIO
 *	data register.
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *
 *	Protection: None
 *
 *	Return: data from PHY register - success, -1 - failure
 */
int osi_read_phy_reg(struct osi_core_priv_data *osi_core, unsigned int phyaddr,
		     unsigned int phyreg);
void osi_init_core_ops(struct osi_core_priv_data *osi_core);

/**
 *	osi_set_systime_to_mac - Handles setting of system time.
 *	@osi_core: OSI private data structure.
 *	@sec: Seconds to be configured.
 *	@nsec: Nano seconds to be configured.
 *
 *	Algorithm: Set current system time to MAC.
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *
 *	Protection: None.
 *
 *	Return: 0 - success, -1 - failure.
 */
int osi_set_systime_to_mac(struct osi_core_priv_data *osi_core,
			   unsigned int sec, unsigned int nsec);
/**
 *	osi_adjust_freq - Adjust frequency
 *	@osi: OSI private data structure.
 *	@ppb: Parts per Billion
 *
 *	Algorithm: Adjust a drift of +/- comp nanoseconds per second.
 *	"Compensation" is the difference in frequency between
 *	the master and slave clocks in Parts Per Billion.
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure
 */
int osi_adjust_freq(struct osi_core_priv_data *osi_core, int ppb);

/**
 *
 *	osi_adjust_time - Adjust time
 *	@osi_core: OSI private data structure.
 *	@delta: Delta time
 *
 *	Algorithm: Adjust/update the MAC system time (delta passed in
 *		   nanoseconds, can be + or -).
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *	2) osi_core->ptp_config.one_nsec_accuracy need to be set to 1
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure
 */
int osi_adjust_time(struct osi_core_priv_data *osi_core, long delta);

/**
 *	osi_get_systime_from_mac - Get system time
 *	@osi: OSI private data structure.
 *	@sec: Value read in Seconds
 *	@nsec: Value read in Nano seconds
 *
 *	Algorithm: Gets the current system time
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *
 *	Protection: None
 *
 *	Return: None (sec and nsec stores the read seconds and nanoseconds
 *	values from MAC)
 */
void osi_get_systime_from_mac(struct osi_core_priv_data *osi_core,
			      unsigned int *sec,
			      unsigned int *nsec);
/**
 *	osi_ptp_configuration - Configure PTP
 *	@osi: OSI private data structure.
 *	@enable: Enable or disable Time Stamping.
 *		 0: Disable 1: Enable
 *
 *	Algorithm: Configure the PTP registers that are required for PTP.
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *	2) osi->ptp_config.ptp_filter need to be filled accordingly to the
 *	filter that need to be set for PTP packets. Please check osi_ptp_config
 *	structure declaration on the bit fields that need to be filled.
 *	3) osi->ptp_config.ptp_clock need to be filled with the ptp system clk.
 *	Currently it is set to 62500000Hz.
 *	4) osi->ptp_config.ptp_ref_clk_rate need to be filled with the ptp
 *	reference clock that platform supports.
 *	5) osi->ptp_config.sec need to be filled with current time of seconds
 *	6) osi->ptp_config.nsec need to be filled with current time of nseconds
 *	7) osi->base need to be filled with the ioremapped base address
 *
 *	Protection: None
 *
 *	Return: None
 */
void osi_ptp_configuration(struct osi_core_priv_data *osi_core,
			   unsigned int enable);

#endif /* OSI_CORE_H */
