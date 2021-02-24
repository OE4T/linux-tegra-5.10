/*
 * Copyright (c) 2020-2021, NVIDIA CORPORATION. All rights reserved.
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

#include <osi_common.h>
#include <osi_core.h>
#include <ivc_core.h>
#include "eqos_core.h"
#include "eqos_mmc.h"
#include "core_local.h"

/**
 * @brief ivc_safety_config - EQOS MAC core safety configuration
 */
static struct core_func_safety ivc_safety_config;

/**
 * @brief ivc_config_fw_err_pkts - Configure forwarding of error packets
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] qinx: Queue index
 * @param[in] fw_err: Enable or Disable the forwarding of error packets
 *
 * @pre MAC should be initialized and started. see osi_start_mac()
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
static nve32_t ivc_config_fw_err_pkts(
				   struct osi_core_priv_data *const osi_core,
				   const nveu32_t qinx,
				   const nveu32_t fw_err)

{
	ivc_msg_common msg_common;
	nve32_t index = 0;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = config_fw_err_pkts;
	msg_common.data.args.arguments[index++] = qinx;
	msg_common.data.args.arguments[index++] = fw_err;
	msg_common.data.args.count = index;

	return osi_core->osd_ops.ivc_send(osi_core, &msg_common,
					  sizeof(msg_common));
}

/**
 * @brief ivc_poll_for_swr - Poll for software reset (SWR bit in DMA Mode)
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @pre MAC needs to be out of reset and proper clock configured.
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
static nve32_t ivc_poll_for_swr(struct osi_core_priv_data *const osi_core)
{
	ivc_msg_common msg_common;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = poll_for_swr;

	return osi_core->osd_ops.ivc_send(osi_core, &msg_common,
					  sizeof(msg_common));
}

/**
 * @brief ivc_set_speed - Set operating speed
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] speed:	Operating speed.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @pre MAC should be initialized and started. see osi_start_mac()
 */
static void ivc_set_speed(struct osi_core_priv_data *const osi_core,
			  const nve32_t speed)
{
	ivc_msg_common msg_common;
	nve32_t index = 0;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = set_speed;
	msg_common.data.args.arguments[index++] = speed;
	msg_common.data.args.count = index;

	osi_core->osd_ops.ivc_send(osi_core, &msg_common,
				   sizeof(msg_common));
}

/**
 * @brief ivc_set_mode - Set operating mode
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] mode:	Operating mode.
 *
 * @pre MAC should be initialized and started. see osi_start_mac()
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
static nve32_t ivc_set_mode(struct osi_core_priv_data *const osi_core,
			    const nve32_t mode)
{
	ivc_msg_common msg_common;
	nve32_t index = 0;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = set_mode;
	msg_common.data.args.arguments[index++] = mode;
	msg_common.data.args.count = index;

	return osi_core->osd_ops.ivc_send(osi_core, &msg_common,
				   	  sizeof(msg_common));
}

/**
 * @brief ivc_pad_calibrate - PAD calibration
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @note
 *  - MAC should out of reset and clocks enabled.
 *  - RGMII and MDIO nve32_terface needs to be IDLE before performing PAD
 *    calibration.
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
static nve32_t ivc_pad_calibrate(struct osi_core_priv_data *const osi_core)
{
	ivc_msg_common msg_common;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = pad_calibrate;

	return osi_core->osd_ops.ivc_send(osi_core, &msg_common,
					  sizeof(msg_common));
}

/**
 * @brief ivc_config_rxcsum_offload - Enable/Disale rx checksum offload in HW
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] enabled: Flag to indicate feature is to be enabled/disabled.
 *
 * @note MAC should be init and started. see osi_start_mac()
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t ivc_config_rxcsum_offload(
				struct osi_core_priv_data *const osi_core,
				const nveu32_t enabled)
{
	ivc_msg_common msg_common;
	nve32_t index = 0;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = config_rxcsum_offload;
	msg_common.data.args.arguments[index++] = enabled;
	msg_common.data.args.count = index;

	return osi_core->osd_ops.ivc_send(osi_core, &msg_common,
					  sizeof(msg_common));
}

/**
 * @brief ivc_core_init - EQOS MAC, MTL and common DMA Initialization
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] tx_fifo_size: MTL TX FIFO size
 * @param[in] rx_fifo_size: MTL RX FIFO size
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t ivc_core_init(
			  struct osi_core_priv_data *const osi_core,
			  const nveu32_t tx_fifo_size,
			  const nveu32_t rx_fifo_size)
{
	ivc_msg_common msg_common;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = core_init;
	msg_common.data.init_args.tx_fifo_size = tx_fifo_size;
	msg_common.data.init_args.rx_fifo_size = rx_fifo_size;
	msg_common.data.init_args.strip_vlan_tag = osi_core->strip_vlan_tag;
	msg_common.data.init_args.pause_frames = osi_core->pause_frames;
	msg_common.data.init_args.flow_ctrl = osi_core->flow_ctrl;
	msg_common.data.init_args.num_mtl_queues = osi_core->num_mtl_queues;
	msg_common.data.init_args.pre_si = osi_core->pre_si;
	osi_memcpy(&msg_common.data.init_args.mtl_queues,
		   &osi_core->mtl_queues, sizeof(osi_core->mtl_queues));
	osi_memcpy(&msg_common.data.init_args.rxq_ctrl,
		   &osi_core->rxq_ctrl, sizeof(osi_core->rxq_ctrl));
	osi_memcpy(&msg_common.data.init_args.rxq_prio,
		   &osi_core->rxq_prio, sizeof(osi_core->rxq_prio));
	osi_memcpy(&msg_common.data.init_args.mac_addr,
		   &osi_core->mac_addr, OSI_ETH_ALEN);

	return osi_core->osd_ops.ivc_send(osi_core, &msg_common,
					  sizeof(msg_common));
}

/**
 * @brief ivc_handle_common_nve32_tr - Handles common nve32_terrupt.
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @note MAC should be init and started. see osi_start_mac()
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 */
static void ivc_handle_common_intr(
			struct osi_core_priv_data *const osi_core)
{
	ivc_msg_common msg_common;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = handle_common_intr;

	osi_core->osd_ops.ivc_send(osi_core, &msg_common,
				   sizeof(msg_common));
}

/**
 * @brief ivc_start_mac - Start MAC Tx/Rx engine
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @note 1) MAC init should be complete. See osi_hw_core_init() and
 *	 osi_hw_dma_init()
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 */
static void ivc_start_mac(struct osi_core_priv_data *const osi_core)
{
	ivc_msg_common msg_common;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = start_mac;

	osi_core->osd_ops.ivc_send(osi_core, &msg_common,
				   sizeof(msg_common));
}

/**
 * @brief ivc_stop_mac - Stop MAC Tx/Rx engine
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @note MAC DMA deinit should be complete. See osi_hw_dma_deinit()
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 */
static void ivc_stop_mac(struct osi_core_priv_data *const osi_core)
{
	ivc_msg_common msg_common;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = stop_mac;

	osi_core->osd_ops.ivc_send(osi_core, &msg_common,
				   sizeof(msg_common));
}

/**
 * @brief ivc_config_mac_pkt_filter_reg - configure mac filter register.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] filter: OSI filter structure.
 *
 * @note 1) MAC should be initialized and started. see osi_start_mac()
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 always
 */
static nve32_t ivc_config_mac_pkt_filter_reg(
				struct osi_core_priv_data *const osi_core,
				const struct osi_filter *filter)
{
	ivc_msg_common msg_filter;

	osi_memset(&msg_filter, 0, sizeof(msg_filter));

	msg_filter.cmd = config_mac_pkt_filter_reg;
	osi_memcpy((void *)&msg_filter.data.filter,
		   (void *)filter, sizeof(struct osi_filter));

	return osi_core->osd_ops.ivc_send(osi_core, (char *)&msg_filter,
					  sizeof(msg_filter));
}

/**
 * @brief ivc_update_mac_addr_low_high_reg- Update L2 address in filter
 *	  register
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] filter: OSI filter structure.
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
static nve32_t ivc_update_mac_addr_low_high_reg(
				struct osi_core_priv_data *const osi_core,
				const struct osi_filter *filter)
{
	ivc_msg_common msg_filter;

	osi_memset(&msg_filter, 0, sizeof(msg_filter));

	msg_filter.cmd = update_mac_addr_low_high_reg;
	osi_memcpy((void *) &msg_filter.data.filter,
		   (void *) filter, sizeof(struct osi_filter));

	return osi_core->osd_ops.ivc_send(osi_core, (char *)&msg_filter,
					  sizeof(msg_filter));
}

/**
 * @brief ivc_config_l3_l4_filter_enable - register write to enable L3/L4
 *	filters.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] filter_enb_dis: enable/disable
 *
 * @note MAC should be init and started. see osi_start_mac()
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t ivc_config_l3_l4_filter_enable(
				struct osi_core_priv_data *const osi_core,
				const nveu32_t filter_enb_dis)
{
	ivc_msg_common msg_common;
	nve32_t index = 0;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = config_l3_l4_filter_enable;
	msg_common.data.args.arguments[index++] = filter_enb_dis;
	msg_common.data.args.count = index;

	return osi_core->osd_ops.ivc_send(osi_core, &msg_common,
					  sizeof(msg_common));
}

/**
 * @brief ivc_update_ip4_addr - configure register for IPV4 address filtering
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] filter_no: filter index
 * @param[in] addr: ipv4 address
 * @param[in] src_dst_addr_match: 0 - source addr otherwise - dest addr
 *
 * @note 1) MAC should be init and started. see osi_start_mac()
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
static nve32_t ivc_update_ip4_addr(
				struct osi_core_priv_data *const osi_core,
				const nveu32_t filter_no,
				const nveu8_t addr[],
				const nveu32_t src_dst_addr_match)
{
	ivc_msg_common msg_common;
	nve32_t index = 0;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = update_ip4_addr;
	msg_common.data.args.arguments[index++] = filter_no;
	osi_memcpy((void *)(nveu8_t *)&msg_common.data.args.arguments[index++],
		   (void *)addr, 4);
	msg_common.data.args.arguments[index++] = src_dst_addr_match;
	msg_common.data.args.count = index;

	return osi_core->osd_ops.ivc_send(osi_core, &msg_common,
					  sizeof(msg_common));
}

/**
 * @brief ivc_update_ip6_addr - add ipv6 address in register
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] filter_no: filter index
 * @param[in] addr: ipv6 adderss
 *
 * @note 1) MAC should be init and started. see osi_start_mac()
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
static nve32_t ivc_update_ip6_addr(
				struct osi_core_priv_data *const osi_core,
				const nveu32_t filter_no,
				const nveu16_t addr[])
{
	ivc_msg_common msg_common;
	nve32_t index = 0;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = update_ip6_addr;
	msg_common.data.args.arguments[index++] = filter_no;
	msg_common.data.args.arguments[index++] = addr[0];
	msg_common.data.args.arguments[index++] = addr[1];
	msg_common.data.args.arguments[index++] = addr[2];
	msg_common.data.args.arguments[index++] = addr[3];
	msg_common.data.args.arguments[index++] = addr[4];
	msg_common.data.args.arguments[index++] = addr[5];
	msg_common.data.args.arguments[index++] = addr[6];
	msg_common.data.args.arguments[index++] = addr[7];
	msg_common.data.args.count = index;

	return osi_core->osd_ops.ivc_send(osi_core, &msg_common,
					  sizeof(msg_common));
}

/**
 * @brief ivc_update_l4_port_no -program source  port no
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] filter_no: filter index
 * @param[in] port_no: port number
 * @param[in] src_dst_port_match: 0 - source port, otherwise - dest port
 *
 * @note 1) MAC should be init and started. see osi_start_mac()
 *	 2) osi_core->osd should be populated
 *	 3) DCS bits should be enabled in RXQ to DMA mapping register
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
static nve32_t ivc_update_l4_port_no(
				  struct osi_core_priv_data *const osi_core,
				  const nveu32_t filter_no,
				  const nveu16_t port_no,
				  const nveu32_t src_dst_port_match)
{
	ivc_msg_common msg_common;
	nve32_t index = 0;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = update_l4_port_no;
	msg_common.data.args.arguments[index++] = filter_no;
	msg_common.data.args.arguments[index++] = port_no;
	msg_common.data.args.arguments[index++] = src_dst_port_match;
	msg_common.data.args.count = index;

	return osi_core->osd_ops.ivc_send(osi_core, &msg_common,
					  sizeof(msg_common));
}

/**
 * @brief ivc_config_l3_filters - config L3 filters.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] filter_no: filter index
 * @param[in] enb_dis:  1 - enable otherwise - disable L3 filter
 * @param[in] ipv4_ipv6_match: 1 - IPv6, otherwise - IPv4
 * @param[in] src_dst_addr_match: 0 - source, otherwise - destination
 * @param[in] perfect_inverse_match: normal match(0) or inverse map(1)
 * @param[in] dma_routing_enable: filter based dma routing enable(1)
 * @param[in] dma_chan: dma channel for routing based on filter
 *
 * @note 1) MAC should be init and started. see osi_start_mac()
 *	 2) osi_core->osd should be populated
 *	 3) DCS bit of RxQ should be enabled for dynamic channel selection
 *	    in filter support
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
static nve32_t ivc_config_l3_filters(
				  struct osi_core_priv_data *const osi_core,
				  const nveu32_t filter_no,
				  const nveu32_t enb_dis,
				  const nveu32_t ipv4_ipv6_match,
				  const nveu32_t src_dst_addr_match,
				  const nveu32_t perfect_inverse_match,
				  const nveu32_t dma_routing_enable,
				  const nveu32_t dma_chan)
{
	ivc_msg_common msg_common;
	nve32_t index = 0;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = config_l3_filters;
	msg_common.data.args.arguments[index++] = filter_no;
	msg_common.data.args.arguments[index++] = enb_dis;
	msg_common.data.args.arguments[index++] = ipv4_ipv6_match;
	msg_common.data.args.arguments[index++] = src_dst_addr_match;
	msg_common.data.args.arguments[index++] = perfect_inverse_match;
	msg_common.data.args.arguments[index++] = dma_routing_enable;
	msg_common.data.args.arguments[index++] = dma_chan;
	msg_common.data.args.count = index;

	return osi_core->osd_ops.ivc_send(osi_core, &msg_common,
					  sizeof(msg_common));
}

/**
 * @brief ivc_config_l4_filters - Config L4 filters.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] filter_no: filter index
 * @param[in] enb_dis: 1 - enable, otherwise - disable L4 filter
 * @param[in] tcp_udp_match: 1 - udp, 0 - tcp
 * @param[in] src_dst_port_match: 0 - source port, otherwise - dest port
 * @param[in] perfect_inverse_match: normal match(0) or inverse map(1)
 * @param[in] dma_routing_enable: filter based dma routing enable(1)
 * @param[in] dma_chan: dma channel for routing based on filter
 *
 * @note 1) MAC should be init and started. see osi_start_mac()
 *	 2) osi_core->osd should be populated
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
static nve32_t ivc_config_l4_filters(
				  struct osi_core_priv_data *const osi_core,
				  const nveu32_t filter_no,
				  const nveu32_t enb_dis,
				  const nveu32_t tcp_udp_match,
				  const nveu32_t src_dst_port_match,
				  const nveu32_t perfect_inverse_match,
				  const nveu32_t dma_routing_enable,
				  const nveu32_t dma_chan)
{
	ivc_msg_common msg_common;
	nve32_t index = 0;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = config_l4_filters;
	msg_common.data.args.arguments[index++] = filter_no;
	msg_common.data.args.arguments[index++] = enb_dis;
	msg_common.data.args.arguments[index++] = tcp_udp_match;
	msg_common.data.args.arguments[index++] = src_dst_port_match;
	msg_common.data.args.arguments[index++] = perfect_inverse_match;
	msg_common.data.args.arguments[index++] = dma_routing_enable;
	msg_common.data.args.arguments[index++] = dma_chan;
	msg_common.data.args.count = index;

	return osi_core->osd_ops.ivc_send(osi_core, &msg_common,
					  sizeof(msg_common));
}

/**
 * @brief ivc_set_systime_to_mac - Set system time
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] sec: Seconds to be configured
 * @param[in] nsec: Nano Seconds to be configured
 *
 * @note MAC should be init and started. see osi_start_mac()
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
static nve32_t ivc_set_systime_to_mac(
				   struct osi_core_priv_data *const osi_core,
				   const nveu32_t sec,
				   const nveu32_t nsec)
{
	ivc_msg_common msg_common;
	nve32_t index = 0;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = set_systime_to_mac;
	msg_common.data.args.arguments[index++] = sec;
	msg_common.data.args.arguments[index++] = nsec;
	msg_common.data.args.count = index;

	return osi_core->osd_ops.ivc_send(osi_core, &msg_common,
					  sizeof(msg_common));
}

/**
 * @brief ivc_config_addend - Configure addend
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] addend: Addend value to be configured
 *
 * @note MAC should be init and started. see osi_start_mac()
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
static nve32_t ivc_config_addend(struct osi_core_priv_data *const osi_core,
				 const nveu32_t addend)
{
	ivc_msg_common msg_common;
	nve32_t index = 0;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = config_addend;
	msg_common.data.args.arguments[index++] = addend;
	msg_common.data.args.count = index;

	return osi_core->osd_ops.ivc_send(osi_core, &msg_common,
					  sizeof(msg_common));
}

/**
 * @brief ivc_adjust_mactime - Adjust MAC time with system time
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] sec: Seconds to be configured
 * @param[in] nsec: Nano seconds to be configured
 * @param[in] add_sub: To decide on add/sub with system time
 * @param[in] one_nsec_accuracy: One nano second accuracy
 *
 * @note 1) MAC should be init and started. see osi_start_mac()
 *	 2) osi_core->ptp_config.one_nsec_accuracy need to be set to 1
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t ivc_adjust_mactime(struct osi_core_priv_data *const osi_core,
				  nveu32_t sec, nveu32_t nsec,
				  nveu32_t add_sub, nveu32_t one_nsec_accuracy)
{
	ivc_msg_common msg_common;
	nve32_t index = 0;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = adjust_mactime;
	msg_common.data.args.arguments[index++] = sec;
	msg_common.data.args.arguments[index++] = nsec;
	msg_common.data.args.arguments[index++] = add_sub;
	msg_common.data.args.arguments[index++] = one_nsec_accuracy;
	msg_common.data.args.count = index;

	return osi_core->osd_ops.ivc_send(osi_core, &msg_common,
					  sizeof(msg_common));
}

/**
 * @brief ivc_config_tscr - Configure Time Stamp Register
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] ptp_filter: PTP rx filter parameters
 *
 * @note MAC should be init and started. see osi_start_mac()
 */
static void ivc_config_tscr(struct osi_core_priv_data *const osi_core,
			    const nveu32_t ptp_filter)
{
	ivc_msg_common msg_common;
	nve32_t index = 0;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = config_tscr;
	msg_common.data.args.arguments[index++] = ptp_filter;
	msg_common.data.args.count = index;

	osi_core->osd_ops.ivc_send(osi_core, &msg_common,
				  sizeof(msg_common));
}

/**
 * @brief ivc_config_ssir - Configure SSIR
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @note MAC should be init and started. see osi_start_mac()
 */
static void ivc_config_ssir(struct osi_core_priv_data *const osi_core)
{
	nveu32_t ptp_clock = osi_core->ptp_config.ptp_clock;
	ivc_msg_common msg_common;
	nve32_t index = 0;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = config_ssir;
	msg_common.data.args.arguments[index++] = ptp_clock;
	msg_common.data.args.count = index;

	osi_core->osd_ops.ivc_send(osi_core, &msg_common,
				   sizeof(msg_common));
}

/**
 * @brief ivc_read_mmc - To read MMC registers and ether_mmc_counter structure
 *	   variable
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @note
 *	1) MAC should be init and started. see osi_start_mac()
 *	2) osi_core->osd should be populated
 */
static void ivc_read_mmc(struct osi_core_priv_data *osi_core)
{
	ivc_msg_common msg_common;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = read_mmc;

	osi_core->osd_ops.ivc_send(osi_core, &msg_common,
				   sizeof(msg_common));

}

/**
 * @brief ivc_core_deinit - EQOS MAC core deinitialization
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @note Required clks and resets has to be enabled
 */
static void ivc_core_deinit(struct osi_core_priv_data *const osi_core)
{
	/* Stop the MAC by disabling both MAC Tx and Rx */
	ivc_stop_mac(osi_core);
}

/**
 * @brief ivc_write_phy_reg - Write to a PHY register through MAC over MDIO bus
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] phyaddr: PHY address (PHY ID) associated with PHY
 * @param[in] phyreg: Register which needs to be write to PHY.
 * @param[in] phydata: Data to write to a PHY register.
 *
 * @note MAC should be init and started. see osi_start_mac()
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t ivc_write_phy_reg(struct osi_core_priv_data *const osi_core,
				 const nveu32_t phyaddr,
				 const nveu32_t phyreg,
				 const nveu16_t phydata)
{
	ivc_msg_common msg_common;
	nve32_t index = 0;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = write_phy_reg;
	msg_common.data.args.arguments[index++] = phyaddr;
	msg_common.data.args.arguments[index++] = phyreg;
	msg_common.data.args.arguments[index++] = phydata;
	msg_common.data.args.count = index;

	return osi_core->osd_ops.ivc_send(osi_core, &msg_common,
					  sizeof(msg_common));
}

/**
 * @brief ivc_read_phy_reg - Read from a PHY register through MAC over MDIO bus
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] phyaddr: PHY address (PHY ID) associated with PHY
 * @param[in] phyreg: Register which needs to be read from PHY.
 *
 * @note MAC should be init and started. see osi_start_mac()
 *
 * @retval data from PHY register on success
 * @retval -1 on failure
 */
static nve32_t ivc_read_phy_reg(struct osi_core_priv_data *const osi_core,
				const nveu32_t phyaddr,
				const nveu32_t phyreg)
{
	ivc_msg_common msg_common;
	nve32_t index = 0;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = read_phy_reg;
	msg_common.data.args.arguments[index++] = phyaddr;
	msg_common.data.args.arguments[index++] = phyreg;
	msg_common.data.args.count = index;

	return osi_core->osd_ops.ivc_send(osi_core, &msg_common,
					  sizeof(msg_common));
}

/**
 * @brief ivc_read_reg - Read a reg
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] reg: Register.
 *
 * @retval data from register on success
 * @retval -1 on failure
 */
static nveu32_t ivc_read_reg(struct osi_core_priv_data *const osi_core,
			     const nve32_t reg) {
	ivc_msg_common msg_common;
	nve32_t index = 0;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = reg_read;
	msg_common.data.args.arguments[index++] = reg;
	msg_common.data.args.count = index;

	return osi_core->osd_ops.ivc_send(osi_core, &msg_common,
					  sizeof(msg_common));
}

/**
 * @brief ivc_write_reg - Write a reg
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] val:  Value to be written.
 * @param[in] reg: Register address.
 *
 * @retval data from register on success
 * @retval -1 on failure
 */
static nveu32_t ivc_write_reg(struct osi_core_priv_data *const osi_core,
			      const nveu32_t val, const nve32_t reg) {
	ivc_msg_common msg_common;
	nve32_t index = 0;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = reg_write;
	msg_common.data.args.arguments[index++] = val;
	msg_common.data.args.arguments[index++] = reg;
	msg_common.data.args.count = index;

	return osi_core->osd_ops.ivc_send(osi_core, &msg_common,
					  sizeof(msg_common));
}

#ifndef OSI_STRIPPED_LIB
/**
 * @brief ivc_config_flow_control - Configure MAC flow control settings
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] flw_ctrl: flw_ctrl settings
 *
 * @pre MAC should be initialized and started. see osi_start_mac()
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
static nve32_t ivc_config_flow_control(
				struct osi_core_priv_data *const osi_core,
				const nveu32_t flw_ctrl)
{
	ivc_msg_common msg_common;
	nve32_t index = 0;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = config_flow_control;
	msg_common.data.args.arguments[index++] = flw_ctrl;
	msg_common.data.args.count = index;

	return osi_core->osd_ops.ivc_send(osi_core, &msg_common,
					  sizeof(msg_common));
}

/**
 * @brief Read-validate HW registers for functional safety.
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t ivc_validate_core_regs(struct osi_core_priv_data *const osi_core)
{
	ivc_msg_common msg_common;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = validate_regs;

	return osi_core->osd_ops.ivc_send(osi_core, &msg_common,
					  sizeof(msg_common));
}

/**
 * @brief ivc_config_rx_crc_check - Configure CRC Checking for Rx Packets
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] crc_chk: Enable or disable checking of CRC field in received pkts
 *
 * @note MAC should be init and started. see osi_start_mac()
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t ivc_config_rx_crc_check(
				struct osi_core_priv_data *const osi_core,
				const nveu32_t crc_chk)
{
	ivc_msg_common msg_common;
	nve32_t index = 0;

	osi_memset(&msg_common, 0, sizeof(msg_common));
	msg_common.cmd = config_rx_crc_check;
	msg_common.data.args.arguments[index++] = crc_chk;
	msg_common.data.args.count = index;

	return osi_core->osd_ops.ivc_send(osi_core, &msg_common,
					  sizeof(msg_common));
}

/**
 * @brief ivc_flush_mtl_tx_queue - Flush MTL Tx queue
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] qinx: MTL queue index.
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
static nve32_t ivc_flush_mtl_tx_queue(
				struct osi_core_priv_data *const osi_core,
				const nveu32_t qinx)
{
	ivc_msg_common msg_common;
	nve32_t index = 0;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = flush_mtl_tx_queue;
	msg_common.data.args.arguments[index++] = qinx;
	msg_common.data.args.count = index;

	return osi_core->osd_ops.ivc_send(osi_core, &msg_common,
					  sizeof(msg_common));
}

/**
 * @brief ivc_config_tx_status - Configure MAC to forward the tx pkt status
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] tx_status: Enable or Disable the forwarding of tx pkt status
 *
 * @note MAC should be init and started. see osi_start_mac()
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t ivc_config_tx_status(struct osi_core_priv_data *const osi_core,
				    const nveu32_t tx_status)
{
	ivc_msg_common msg_common;
	nve32_t index = 0;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = config_tx_status;
	msg_common.data.args.arguments[index++] = tx_status;
	msg_common.data.args.count = index;

	return osi_core->osd_ops.ivc_send(osi_core, &msg_common,
					  sizeof(msg_common));
}

/**
 * @brief ivc_set_avb_algorithm - Set TxQ/TC avb config
 *
 * @param[in] osi_core: osi core priv data structure
 * @param[in] avb: structure having configuration for avb algorithm
 *
 * @note 1) MAC should be init and started. see osi_start_mac()
 *	 2) osi_core->osd should be populated.
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t ivc_set_avb_algorithm(
				struct osi_core_priv_data *const osi_core,
				const struct osi_core_avb_algorithm *const avb)
{
	ivc_msg_common msg_avb;

	osi_memset(&msg_avb, 0, sizeof(msg_avb));

	msg_avb.cmd = set_avb_algorithm;
	osi_memcpy((void *)&msg_avb.data.avb_algo, (void *)avb,
		   sizeof(struct osi_core_avb_algorithm));

	return osi_core->osd_ops.ivc_send(osi_core, (char *)&msg_avb,
					  sizeof(msg_avb));
}

/**
 * @brief ivc_get_avb_algorithm - Get TxQ/TC avb config
 *
 * @param[in] osi_core: osi core priv data structure
 * @param[out] avb: structure ponve32_ter having configuration for avb algorithm
 *
 * @note 1) MAC should be init and started. see osi_start_mac()
 *	 2) osi_core->osd should be populated.
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t ivc_get_avb_algorithm(struct osi_core_priv_data *const osi_core,
				     struct osi_core_avb_algorithm *const avb)
{
	ivc_msg_common msg_avb;

	osi_memset(&msg_avb, 0, sizeof(msg_avb));

	msg_avb.cmd = get_avb_algorithm;
	osi_memcpy((void *) &msg_avb.data.avb_algo,
		   (void *)avb, sizeof(struct osi_core_avb_algorithm));

	return osi_core->osd_ops.ivc_send(osi_core, (char *)&msg_avb,
					  sizeof(msg_avb));
}

/**
 * @brief ivc_config_arp_offload - Enable/Disable ARP offload
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] enable: Flag variable to enable/disable ARP offload
 * @param[in] ip_addr: IP address of device to be programmed in HW.
 *	      HW will use this IP address to respond to ARP requests.
 *
 * @note 1) MAC should be init and started. see osi_start_mac()
 *	 2) Valid 4 byte IP address as argument ip_addr
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t ivc_config_arp_offload(
				struct osi_core_priv_data *const osi_core,
				const nveu32_t enable,
				const nveu8_t *ip_addr)
{
	ivc_msg_common msg_common;
	nve32_t index = 0;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = config_arp_offload;
	msg_common.data.args.arguments[index++] = enable;
	osi_memcpy((void *)(nveu8_t *)&msg_common.data.args.arguments[index++],
		   (void *)ip_addr, 4);
	msg_common.data.args.count = index;

	return osi_core->osd_ops.ivc_send(osi_core, &msg_common,
					  sizeof(msg_common));
}

/**
 * @brief ivc_config_vlan_filter_reg - config vlan filter register
 *
 * @param[in] osi_core: Base address  from OSI core private data structure.
 * @param[in] filter_enb_dis: vlan filter enable/disable
 * @param[in] perfect_hash_filtering: perfect or hash filter
 * @param[in] perfect_inverse_match: normal or inverse filter
 *
 * @note 1) MAC should be init and started. see osi_start_mac()
 *	 2) osi_core->osd should be populated
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t ivc_config_vlan_filtering(
				struct osi_core_priv_data *const osi_core,
				const nveu32_t filter_enb_dis,
				const nveu32_t perfect_hash_filtering,
				const nveu32_t perfect_inverse_match)
{
	ivc_msg_common msg_common;
	nve32_t index = 0;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = config_vlan_filtering;
	msg_common.data.args.arguments[index++] = filter_enb_dis;
	msg_common.data.args.arguments[index++] = perfect_hash_filtering;
	msg_common.data.args.arguments[index++] = perfect_inverse_match;
	msg_common.data.args.count = index;

	return osi_core->osd_ops.ivc_send(osi_core, &msg_common,
					  sizeof(msg_common));
}

/**
 * @brief ivc_update_vlan_id - update VLAN ID in Tag register
 *
 * @param[in] base: Base address from OSI core private data structure.
 * @param[in] vid: VLAN ID to be programmed.
 *
 * @retval 0 always
 */
static inline nve32_t ivc_update_vlan_id(
				  struct osi_core_priv_data *const osi_core,
				  nveu32_t vid)
{
	/* Don't add VLAN ID to TR register which is eventually set TR
	 * to 0x0 and allow all tagged packets
	 */
	ivc_msg_common msg_common;
	nve32_t index = 0;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = update_vlan_id;
	msg_common.data.args.arguments[index++] = vid;
	msg_common.data.args.count = index;

	return osi_core->osd_ops.ivc_send(osi_core, &msg_common,
					  sizeof(msg_common));
}

/**
 * @brief ivc_reset_mmc - To reset MMC registers and ether_mmc_counter
 *	structure variable
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @note
 *	1) MAC should be init and started. see osi_start_mac()
 *	2) osi_core->osd should be populated
 */
static void ivc_reset_mmc(struct osi_core_priv_data *osi_core)
{
	ivc_msg_common msg_common;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = reset_mmc;

	osi_core->osd_ops.ivc_send(osi_core, &msg_common,
				   sizeof(msg_common));
}

/**
 * @brief ivc_configure_eee - Configure the EEE LPI mode
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] tx_lpi_enabled: enable (1)/disable (0) flag for Tx lpi
 * @param[in] tx_lpi_timer: Tx LPI entry timer in usec. Valid upto
 *	      OSI_MAX_TX_LPI_TIMER in steps of 8usec.
 *
 * @note
 *	Required clks and resets has to be enabled.
 *	MAC/PHY should be initialized
 */
static void ivc_configure_eee(struct osi_core_priv_data *const osi_core,
			      const nveu32_t tx_lpi_enabled,
			      const nveu32_t tx_lpi_timer)
{
	ivc_msg_common msg_common;
	nve32_t index = 0;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = configure_eee;
	msg_common.data.args.arguments[index++] = tx_lpi_enabled;
	msg_common.data.args.arguments[index++] = tx_lpi_timer;
	msg_common.data.args.count = index;

	osi_core->osd_ops.ivc_send(osi_core, &msg_common,
				   sizeof(msg_common));
}

/*
 * @brief Function to store a backup of MAC register space during SOC suspend.
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @retval 0 on Success
 */
static inline nve32_t ivc_save_registers(
				struct osi_core_priv_data *const osi_core)
{
	ivc_msg_common msg_common;
	nve32_t index = 0;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = save_registers;
	msg_common.data.args.count = index;

	return osi_core->osd_ops.ivc_send(osi_core, &msg_common,
					  sizeof(msg_common));
}

/**
 * @brief Function to restore the backup of MAC registers during SOC resume.
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @retval 0 on Success
 */
static inline nve32_t ivc_restore_registers(
				struct osi_core_priv_data *const osi_core)
{
	ivc_msg_common msg_common;
	nve32_t index = 0;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = restore_registers;
	msg_common.data.args.count = index;

	return osi_core->osd_ops.ivc_send(osi_core, &msg_common,
					  sizeof(msg_common));
}

/**
 * @brief ivc_set_mdc_clk_rate - Derive MDC clock based on provided AXI_CBB clk
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] csr_clk_rate: CSR (AXI CBB) clock rate.
 *
 * @note OSD layer needs get the AXI CBB clock rate with OSD clock API
 *	 (ex - clk_get_rate())
 */
static void ivc_set_mdc_clk_rate(struct osi_core_priv_data *const osi_core,
				 const nveu64_t csr_clk_rate)
{
	ivc_msg_common msg_common;
	nve32_t index = 0;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = set_mdc_clk_rate;
	msg_common.data.args.arguments[index++] = csr_clk_rate;
	msg_common.data.args.count = index;

	osi_core->osd_ops.ivc_send(osi_core, &msg_common,
				   sizeof(msg_common));
}

/**
 * @brief ivc_config_mac_loopback - Configure MAC to support loopback
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] lb_mode: Enable or Disable MAC loopback mode
 *
 * @note MAC should be init and started. see osi_start_mac()
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t ivc_config_mac_loopback(
				struct osi_core_priv_data *const osi_core,
				const nveu32_t lb_mode)
{
	ivc_msg_common msg_common;
	nve32_t index = 0;

	osi_memset(&msg_common, 0, sizeof(msg_common));

	msg_common.cmd = config_mac_loopback;
	msg_common.data.args.arguments[index++] = lb_mode;
	msg_common.data.args.count = index;

	return osi_core->osd_ops.ivc_send(osi_core, &msg_common,
					  sizeof(msg_common));
}
#endif

/**
 * @brief ivc_init_core_ops - Initialize IVC core operations.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: No
 * - De-initialization: No
 */
void ivc_init_core_ops(struct core_ops *ops)
{
	ops->poll_for_swr = ivc_poll_for_swr;
	ops->core_init = ivc_core_init;
	ops->core_deinit = ivc_core_deinit;
	ops->start_mac = ivc_start_mac;
	ops->stop_mac = ivc_stop_mac;
	ops->handle_common_intr = ivc_handle_common_intr;
	ops->set_mode = ivc_set_mode;
	ops->set_speed = ivc_set_speed;
	ops->pad_calibrate = ivc_pad_calibrate;
	ops->config_fw_err_pkts = ivc_config_fw_err_pkts;
	ops->config_rxcsum_offload = ivc_config_rxcsum_offload;
	ops->config_mac_pkt_filter_reg = ivc_config_mac_pkt_filter_reg;
	ops->update_mac_addr_low_high_reg = ivc_update_mac_addr_low_high_reg;
	ops->config_l3_l4_filter_enable = ivc_config_l3_l4_filter_enable;
	ops->config_l3_filters = ivc_config_l3_filters;
	ops->update_ip4_addr = ivc_update_ip4_addr;
	ops->update_ip6_addr = ivc_update_ip6_addr;
	ops->config_l4_filters = ivc_config_l4_filters;
	ops->update_l4_port_no = ivc_update_l4_port_no;
	ops->set_systime_to_mac = ivc_set_systime_to_mac;
	ops->config_addend = ivc_config_addend;
	ops->adjust_mactime = ivc_adjust_mactime;
	ops->config_tscr = ivc_config_tscr;
	ops->config_ssir = ivc_config_ssir;
	ops->read_mmc = ivc_read_mmc;
	ops->write_phy_reg = ivc_write_phy_reg;
	ops->read_phy_reg = ivc_read_phy_reg;
	ops->read_reg = ivc_read_reg;
	ops->write_reg = ivc_write_reg;
#ifndef OSI_STRIPPED_LIB
	ops->config_tx_status = ivc_config_tx_status;
	ops->config_rx_crc_check = ivc_config_rx_crc_check;
	ops->config_flow_control = ivc_config_flow_control;
	ops->config_arp_offload = ivc_config_arp_offload;
	ops->validate_regs = ivc_validate_core_regs;
	ops->flush_mtl_tx_queue = ivc_flush_mtl_tx_queue;
	ops->set_avb_algorithm = ivc_set_avb_algorithm;
	ops->get_avb_algorithm = ivc_get_avb_algorithm;
	ops->config_vlan_filtering = ivc_config_vlan_filtering;
	ops->update_vlan_id = ivc_update_vlan_id;
	ops->reset_mmc = ivc_reset_mmc;
	ops->configure_eee = ivc_configure_eee;
	ops->save_registers = ivc_save_registers;
	ops->restore_registers = ivc_restore_registers;
	ops->set_mdc_clk_rate = ivc_set_mdc_clk_rate;
	ops->config_mac_loopback = ivc_config_mac_loopback;
#endif /* !OSI_STRIPPED_LIB */
};

/**
 * @brief ivc_get_core_safety_config - EQOS MAC safety configuration
 */
void *ivc_get_core_safety_config(void)
{
	return &ivc_safety_config;
}
