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

#ifndef MMC_H
#define MMC_H
/**
 *	osi_mmc_counters - The structure to hold RMON counter values
 *
 *	mmc_tx_octetcount_gb: This counter provides the number of bytes
 *	transmitted, exclusive of preamble and retried bytes, in good and
 *	bad packets.
 *	mmc_tx_framecount_gb: This counter provides the number of good and
 *	bad packets transmitted, exclusive of retried packets.
 *	mmc_tx_broadcastframe_g: This counter provides the number of good
 *	broadcast packets transmitted
 *	mmc_tx_multicastframe_g: This counter provides the number of good
 *	multicast packets transmitted
 *	mmc_tx_64_octets_gb: This counter provides the number of good and bad
 *	packets transmitted with length 64 bytes, exclusive of preamble and
 *	retried packets
 *	mmc_tx_65_to_127_octets_gb: This counter provides the number of good
 *	and bad packets transmitted with length 65-127 bytes, exclusive of
 *	preamble and retried packets
 *	mmc_tx_128_to_255_octets_gb: This counter provides the number of good
 *	and bad packets transmitted with length 128-255 bytes, exclusive of
 *	preamble and retried packets
 *	mmc_tx_256_to_511_octets_gb: This counter provides the number of good
 *	and bad packets transmitted with length 256-511 bytes, exclusive of
 *	preamble and retried packets
 *	mmc_tx_512_to_1023_octets_gb: This counter provides the number of good
 *	and bad packets transmitted with length 512-1023 bytes, exclusive of
 *	preamble and retried packets
 *	mmc_tx_1024_to_max_octets_gb: This counter provides the number of good
 *	and bad packets transmitted with length 1024-max bytes, exclusive of
 *	preamble and retried packets
 *	mmc_tx_unicast_gb: This counter provides the number of good and bad
 *	unicast packets
 *	mmc_tx_multicast_gb: This counter provides the number of good and bad
 *	multicast packets
 *	mmc_tx_broadcast_gb: This counter provides the number of good and bad
 *	braodcast packets
 *	mmc_tx_underflow_error: This counter provides the number of abort
 *	packets due to underflow error
 *	mmc_tx_singlecol_g: This counter provides the number of successfully
 *	transmitted packets after a single collision in the half-duplex mode
 *	mmc_tx_multicol_g: This counter provides the number of successfully
 *	transmitted packets after a multiple collision in the half-duplex mode
 *	mmc_tx_deferred: This counter provides the number of successfully
 *	transmitted after a deferral in the half-duplex mode
 *	mmc_tx_latecol: This counter provides the number of packets aborted
 *	because of late collision error
 *	mmc_tx_exesscol: This counter provides the number of packets aborted
 *	because of excessive (16) collision errors
 *	mmc_tx_carrier_error: This counter provides the number of packets
 *	aborted because of carrier sense error (no carrier or loss of carrier)
 *	mmc_tx_octetcount_g: This counter provides the number of bytes
 *	transmitted, exclusive of preamble, only in good packets.
 *	mmc_tx_framecount_g: This counter provides the number of good packets
 *	transmitted .
 *	mmc_tx_excessdef: This counter provides the number of packets aborted
 *	because of excessive deferral error (deferred for more than two
 *	max-sized packet times).
 *	mmc_tx_pause_frame: This counter provides the number of good Pause
 *	packets transmitted.
 *	mmc_tx_vlan_frame_g: This counter provides the number of good
 *	VLAN packets transmitted
 *	mmc_tx_osize_frame_g: This counter provides the number of packets
 *	transmitted without errors and with length greater than the maxsize
 *	(1,518 or 1,522 bytes for VLAN tagged packets; 2000 bytes.
 *	mmc_rx_framecount_gb: This counter provides the number of good and bad
 *	packets received
 *	mmc_rx_octetcount_gb: This counter provides the number of bytes
 *	received by DWC_ther_qos, exclusive of preamble, in good and bad packets
 *	mmc_rx_octetcount_g: This counter provides the number of bytes
 *	received by DWC_ther_qos, exclusive of preamble, in good and bad packets
 *	mmc_rx_broadcastframe_g: This counter provides the number of good
 *	broadcast packets received
 *	mmc_rx_multicastframe_g: This counter provides the number of good
 *	multicast packets received
 *	mmc_rx_crc_error: This counter provides the number of packets received
 *	with CRC error
 *	mmc_rx_align_error: This counter provides the number of packets
 *	received with alignment (dribble) error. It is valid only in 10/100
 *	mode.
 *	mmc_rx_runt_error: This counter provides the number of packets
 *	received  with runt (length less than 64 bytes and CRC error) error
 *	mmc_rx_jabber_error: This counter provides the number of giant packets
 *	received  with length (including CRC) greater than 1,518 bytes
 *	(1,522 bytes for VLAN tagged) and with CRC error.
 *	mmc_rx_undersize_g: This counter provides the number of packets
 *	received  with length less than 64 bytes, without any errors
 *	mmc_rx_oversize_g: This counter provides the number of packets
 *	received  without errors, with length greater than the maxsize
 *	mmc_rx_64_octets_gb: This counter provides the number of good and bad
 *	packets received with length 64 bytes, exclusive of the preamble.
 *	mmc_rx_65_to_127_octets_gb: This counter provides the number of good
 *	and bad packets received with length 65-127 bytes, exclusive of the
 *	preamble.
 *	mmc_rx_128_to_255_octets_gb: This counter provides the number of good
 *	and bad packets received with length 128-255 bytes, exclusive of the
 *	preamble.
 *	mmc_rx_256_to_511_octets_gb: This counter provides the number of good
 *	and bad packets received with length 256-511 bytes, exclusive of the
 *	preamble.
 *	mmc_rx_512_to_1023_octets_gb: This counter provides the number of good
 *	and bad packets received with length 512-1023 bytes, exclusive of the
 *	preamble.
 *	mmc_rx_1024_to_max_octets_gb: This counter provides the number of good
 *	and bad packets received with length 1024-max bytes, exclusive of the
 *	preamble.
 *	mmc_rx_unicast_g: This counter provides the number of good unicast
 *	packets received mmc_rx_length_error: This counter provides the
 *	number of packets received  with length error (Length Type field
 *	 not equal to packet size), for all packets with valid length field.
 *	mmc_rx_outofrangetype: This counter provides the number of packets
 *	received  with length field not equal to the
 *	valid packet size (greater than 1,500 but less than 1,536).
 *	mmc_rx_pause_frames: This counter provides the number of good and
 *	valid Pause packets received
 *	mmc_rx_fifo_overflow: This counter provides the number of missed
 *	received packets because of FIFO overflow in DWC_ether_qos
 *	mmc_rx_vlan_frames_gb: This counter provides the number of good and
 *	bad VLAN packets received
 *	mmc_rx_watchdog_error: This counter provides the number of packets
 *	received with error because of watchdog timeout error
 *	mmc_rx_receive_error: This counter provides the number of packets
 *	received with Receive error or Packet Extension error on the GMII or
 *	MII interface
 *	mmc_rx_ctrl_frames_g: This counter provides the number of packets
 *	received with Receive error or Packet Extension error on the GMII
 *	or MII interface
 *	mmc_rx_ipv4_gd: This counter provides the number of good IPv4
 *	datagrams received with the TCP, UDP, or ICMP payload
 *	mmc_rx_ipv4_hderr: RxIPv4 Header Error Packets
 *	mmc_rx_ipv4_nopay: This counter provides the number of IPv4 datagram
 *	packets received that did not have a TCP, UDP, or ICMP payload
 *	mmc_rx_ipv4_frag: This counter provides the number of good IPv4
 *	datagrams received with fragmentation.
 *	mmc_rx_ipv4_udsbl: This counter provides the number of good IPv4
 *	datagrams received that had a UDP payload with checksum disabled
 *	mmc_rx_ipv6_gd_octets: This counter provides the number of good IPv6
 *	datagrams received with the TCP, UDP, or ICMP payload
 *	mmc_rx_ipv6_hderr_octets: This counter provides the number of IPv6
 *	datagrams received with header (length or version mismatch) errors
 *	mmc_rx_ipv6_nopay_octets: This counter provides the number of IPv6
 *	datagram packets received that did not have a TCP, UDP, or ICMP
 *	payload
 *	mmc_rx_udp_gd: This counter provides the number of good IP datagrams
 *	received by DWC_ether_qos with a good UDP payload.
 *	mmc_rx_udp_err: This counter provides the number of good IP datagrams
 *	received by DWC_ether_qos with a good UDP payload. This counter is not
 *	updated when the RxIPv4_UDP_Checksum_Disabled_Packets counter is
 *	incremented.
 *	mmc_rx_tcp_gd: This counter provides the number of good IP datagrams
 *	received with a good TCP payload
 *	mmc_rx_tcp_err: This counter provides the number of good IP datagrams
 *	received with a good TCP payload
 *	mmc_rx_icmp_gd: This counter provides the number of good IP datagrams
 *	received with a good ICMP payload
 *	mmc_rx_icmp_err: This counter provides the number of good IP
 *	datagrams received whose ICMP payload has a checksum error
 *	mmc_rx_ipv4_gd_octets: This counter provides the number of bytes
 *	received by DWC_ether_qos in good IPv4 datagrams encapsulating TCP,
 *	UDP, or ICMP data. (Ethernet header, FCS, pad, or IP pad bytes are
 *	not included in this counter
 *	mmc_rx_ipv4_hderr_octets: This counter provides the number of bytes
 *	received in IPv4 datagrams with header errors (checksum, length,
 *	version mismatch). The value in the Length field of IPv4 header is
 *	used to update this counter. (Ethernet header, FCS, pad,
 *	or IP pad bytes are not included in this counter
 *	mmc_rx_ipv4_nopay_octets: This counter provides the number of bytes
 *	received in IPv4 datagrams that did not have a TCP, UDP, or
 *	ICMP payload. The value in the Length field of IPv4 header is used
 *	to update this counter. (Ethernet header, FCS, pad, or IP pad bytes
 *	are not included in this counter.
 *	mmc_rx_ipv4_frag_octets: This counter provides the number of bytes
 *	received in fragmented IPv4 datagrams. The value in the Length
 *	field of IPv4 header is used to update this counter. (Ethernet header,
 *	FCS, pad, or IP pad bytes are not included in this counter
 *	mmc_rx_ipv4_udsbl_octets: This counter provides the number of bytes
 *	received in a UDP segment that had the UDP checksum disabled. This
 *	counter does not count IP Header bytes. (Ethernet header, FCS, pad,
 *	or IP pad bytes are not included in this counter.
 *	mmc_rx_ipv6_gd: This counter provides the number of bytes received
 *	in good IPv6 datagrams encapsulating TCP, UDP, or ICMP data.
 *	(Ethernet header, FCS, pad, or IP pad bytes are not included in
 *	this counter
 *	mmc_rx_ipv6_hderr: This counter provides the number of bytes received
 *	in IPv6 datagrams with header errors (length, version mismatch). The
 *	value in the Length field of IPv6 header is used to update this
 *	counter. (Ethernet header, FCS, pad, or IP pad bytes are not included
 *	in this counter.
 *	mmc_rx_ipv6_nopay: This counter provides the number of bytes received
 *	in IPv6 datagrams that did not have a TCP, UDP, or ICMP payload. The
 *	value in the Length field of IPv6 header is used to update this
 *	counter. (Ethernet header, FCS, pad, or IP pad bytes are not included
 *	in this counter
 *	mmc_rx_udp_gd_octets: This counter provides the number of bytes
 *	received in a good UDP segment. This counter does not count IP header
 *	bytes.
 *	mmc_rx_udp_err_octets: This counter provides the number of bytes
 *	received in a UDP segment that had checksum errors. This counter
 *	does not count IP header bytes
 *	mmc_rx_tcp_gd_octets: This counter provides the number of bytes
 *	received in a good TCP segment. This counter does not count IP
 *	header bytes
 *	mmc_rx_tcp_err_octets: This counter provides the number of bytes
 *	received in a TCP segment that had checksum errors. This counter
 *	does not count IP header bytes
 *	mmc_rx_icmp_gd_octets: This counter provides the number of bytes
 *	received in a good ICMP segment. This counter does not count
 *	IP header bytes
 *	mmc_rx_icmp_err_octets: This counter provides the number of bytes
 *	received in a ICMP segment that had checksum errors.
 *	This counter does not count IP header bytes
 */
struct osi_mmc_counters {
	/* MMC TX counters */
	unsigned long mmc_tx_octetcount_gb;
	unsigned long mmc_tx_framecount_gb;
	unsigned long mmc_tx_broadcastframe_g;
	unsigned long mmc_tx_multicastframe_g;
	unsigned long mmc_tx_64_octets_gb;
	unsigned long mmc_tx_65_to_127_octets_gb;
	unsigned long mmc_tx_128_to_255_octets_gb;
	unsigned long mmc_tx_256_to_511_octets_gb;
	unsigned long mmc_tx_512_to_1023_octets_gb;
	unsigned long mmc_tx_1024_to_max_octets_gb;
	unsigned long mmc_tx_unicast_gb;
	unsigned long mmc_tx_multicast_gb;
	unsigned long mmc_tx_broadcast_gb;
	unsigned long mmc_tx_underflow_error;
	unsigned long mmc_tx_singlecol_g;
	unsigned long mmc_tx_multicol_g;
	unsigned long mmc_tx_deferred;
	unsigned long mmc_tx_latecol;
	unsigned long mmc_tx_exesscol;
	unsigned long mmc_tx_carrier_error;
	unsigned long mmc_tx_octetcount_g;
	unsigned long mmc_tx_framecount_g;
	unsigned long mmc_tx_excessdef;
	unsigned long mmc_tx_pause_frame;
	unsigned long mmc_tx_vlan_frame_g;
	unsigned long mmc_tx_osize_frame_g;

	/* MMC RX counters */
	unsigned long mmc_rx_framecount_gb;
	unsigned long mmc_rx_octetcount_gb;
	unsigned long mmc_rx_octetcount_g;
	unsigned long mmc_rx_broadcastframe_g;
	unsigned long mmc_rx_multicastframe_g;
	unsigned long mmc_rx_crc_error;
	unsigned long mmc_rx_align_error;
	unsigned long mmc_rx_runt_error;
	unsigned long mmc_rx_jabber_error;
	unsigned long mmc_rx_undersize_g;
	unsigned long mmc_rx_oversize_g;
	unsigned long mmc_rx_64_octets_gb;
	unsigned long mmc_rx_65_to_127_octets_gb;
	unsigned long mmc_rx_128_to_255_octets_gb;
	unsigned long mmc_rx_256_to_511_octets_gb;
	unsigned long mmc_rx_512_to_1023_octets_gb;
	unsigned long mmc_rx_1024_to_max_octets_gb;
	unsigned long mmc_rx_unicast_g;
	unsigned long mmc_rx_length_error;
	unsigned long mmc_rx_outofrangetype;
	unsigned long mmc_rx_pause_frames;
	unsigned long mmc_rx_fifo_overflow;
	unsigned long mmc_rx_vlan_frames_gb;
	unsigned long mmc_rx_watchdog_error;
	unsigned long mmc_rx_receive_error;
	unsigned long mmc_rx_ctrl_frames_g;

	/* IPv4 */
	unsigned long mmc_rx_ipv4_gd;
	unsigned long mmc_rx_ipv4_hderr;
	unsigned long mmc_rx_ipv4_nopay;
	unsigned long mmc_rx_ipv4_frag;
	unsigned long mmc_rx_ipv4_udsbl;

	/* IPV6 */
	unsigned long mmc_rx_ipv6_gd_octets;
	unsigned long mmc_rx_ipv6_hderr_octets;
	unsigned long mmc_rx_ipv6_nopay_octets;

	/* Protocols */
	unsigned long mmc_rx_udp_gd;
	unsigned long mmc_rx_udp_err;
	unsigned long mmc_rx_tcp_gd;
	unsigned long mmc_rx_tcp_err;
	unsigned long mmc_rx_icmp_gd;
	unsigned long mmc_rx_icmp_err;

	/* IPv4 */
	unsigned long mmc_rx_ipv4_gd_octets;
	unsigned long mmc_rx_ipv4_hderr_octets;
	unsigned long mmc_rx_ipv4_nopay_octets;
	unsigned long mmc_rx_ipv4_frag_octets;
	unsigned long mmc_rx_ipv4_udsbl_octets;

	/* IPV6 */
	unsigned long mmc_rx_ipv6_gd;
	unsigned long mmc_rx_ipv6_hderr;
	unsigned long mmc_rx_ipv6_nopay;

	/* Protocols */
	unsigned long mmc_rx_udp_gd_octets;
	unsigned long mmc_rx_udp_err_octets;
	unsigned long mmc_rx_tcp_gd_octets;
	unsigned long mmc_rx_tcp_err_octets;
	unsigned long mmc_rx_icmp_gd_octets;
	unsigned long mmc_rx_icmp_err_octets;
};

/**
 *	osi_xtra_stat_counters - OSI core extra stat counters
 *
 *	rx_buf_unavail_irq_n: RX buffer unavailable irq count
 *	tx_proc_stopped_irq_n: Transmit Process Stopped irq count
 *	tx_buf_unavail_irq_n: Transmit Buffer Unavailable irq count
 *	rx_proc_stopped_irq_n: Receive Process Stopped irq count
 *	rx_watchdog_irq_n: Receive Watchdog Timeout irq count
 *	fatal_bus_error_irq_n: Fatal Bus Error irq count
 *	q_re_alloc_rx_buf_failed: rx sbk allocation failure count
 *	tx_normal_irq_n: TX per channel interrupt count
 *	rx_normal_irq_n: RX per cannel interrupt count
 *	link_connect_count: link disconnect count
 *	link_disconnect_count: link connect count
 */
struct osi_xtra_stat_counters {
	unsigned long rx_buf_unavail_irq_n[OSI_EQOS_MAX_NUM_QUEUES];
	unsigned long tx_proc_stopped_irq_n[OSI_EQOS_MAX_NUM_QUEUES];
	unsigned long tx_buf_unavail_irq_n[OSI_EQOS_MAX_NUM_QUEUES];
	unsigned long rx_proc_stopped_irq_n[OSI_EQOS_MAX_NUM_QUEUES];
	unsigned long rx_watchdog_irq_n;
	unsigned long fatal_bus_error_irq_n;
	unsigned long re_alloc_rxbuf_failed[OSI_EQOS_MAX_NUM_QUEUES];
	unsigned long tx_normal_irq_n[OSI_EQOS_MAX_NUM_QUEUES];
	unsigned long rx_normal_irq_n[OSI_EQOS_MAX_NUM_QUEUES];
	unsigned long link_connect_count;
	unsigned long link_disconnect_count;
};

/**
 *	osi_xtra_dma_stat_counters -  OSI dma extra stats counters
 *	q_tx_pkt_n: Per Q TX packet count
 *	q_rx_pkt_n: Per Q RX packet count
 *	tx_clean_n: Per Q TX complete call count
 *	tx_pkt_n: Total number of tx packets count
 *	rx_pkt_n: Total number of rx packet count
 *	rx_vlan_pkt_n: Total number of VLAN RX packet count
 *	tx_vlan_pkt_n: Total number of VLAN TX packet count
 *	tx_tso_pkt_n: Total number of TSO packet count
 */
struct osi_xtra_dma_stat_counters {
	unsigned long q_tx_pkt_n[OSI_EQOS_MAX_NUM_QUEUES];
	unsigned long q_rx_pkt_n[OSI_EQOS_MAX_NUM_QUEUES];
	unsigned long tx_clean_n[OSI_EQOS_MAX_NUM_QUEUES];
	unsigned long tx_pkt_n;
	unsigned long rx_pkt_n;
	unsigned long rx_vlan_pkt_n;
	unsigned long tx_vlan_pkt_n;
	unsigned long tx_tso_pkt_n;
};

#endif
