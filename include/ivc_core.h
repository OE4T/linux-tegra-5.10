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

#ifndef IVC_CORE_H
#define IVC_CORE_H

#include "osi_core.h"
/**
 * @brief Ethernet Maximum IVC BUF
 */
#define ETHER_MAX_IVC_BUF	1024

/**
 * @brief IVC maximum arguments
 */
#define MAX_ARGS	10

/**
 * @brief IVC commands between OSD & OSI.
 */
typedef enum ivc_cmd {
	poll_for_swr = 1,
	core_init,
	core_deinit,
	start_mac,
	stop_mac,
	handle_common_intr,
	set_mode,
	set_speed,
	pad_calibrate,
	config_fw_err_pkts,
	config_rxcsum_offload,
	config_mac_pkt_filter_reg,
	update_mac_addr_low_high_reg,
	config_l3_l4_filter_enable,
	config_l3_filters,
	update_ip4_addr,
	update_ip6_addr,
	config_l4_filters,
	update_l4_port_no,
	set_systime_to_mac,
	config_addend,
	adjust_mactime,
	config_tscr,
	config_ssir,
	read_mmc,
	write_phy_reg,
	read_phy_reg,
	reg_read,
	reg_write,
	get_hw_features,
#ifndef OSI_STRIPPED_LIB
	config_tx_status,
	config_rx_crc_check,
	config_flow_control,
	config_arp_offload,
	validate_regs,
	flush_mtl_tx_queue,
	set_avb_algorithm,
	get_avb_algorithm,
	config_vlan_filtering,
	update_vlan_id,
	reset_mmc,
	configure_eee,
	save_registers,
	restore_registers,
	set_mdc_clk_rate,
	config_mac_loopback,
#endif /* !OSI_STRIPPED_LIB */
}ivc_cmd;

/**
 * @brief IVC arguments structure.
 */
typedef struct ivc_args {
	/** Number of arguments */
	nveu32_t count;
	/** arguments */
	nveu32_t arguments[MAX_ARGS];
} ivc_args;

/**
 * @brief IVC core argument structure.
 */
typedef struct ivc_core_args {
	/** Number of MTL queues enabled in MAC */
	nveu32_t num_mtl_queues;
	/** Array of MTL queues */
	nveu32_t mtl_queues[OSI_EQOS_MAX_NUM_CHANS];
	/** List of MTL Rx queue mode that need to be enabled */
	nveu32_t rxq_ctrl[OSI_EQOS_MAX_NUM_CHANS];
	/** Rx MTl Queue mapping based on User Priority field */
	nveu32_t rxq_prio[OSI_EQOS_MAX_NUM_CHANS];
	/** Ethernet MAC address */
	nveu8_t mac_addr[OSI_ETH_ALEN];
	/** Tegra Pre-si platform info */
	nveu32_t pre_si;
	/** VLAN tag stripping enable(1) or disable(0) */
	nveu32_t strip_vlan_tag;
	/** pause frame support */
	nveu32_t pause_frames;
	/** Current flow control settings */
	nveu32_t flow_ctrl;
	/** Rx fifo size */
	nveu32_t rx_fifo_size;
	/** Tx fifo size */
	nveu32_t tx_fifo_size;
} ivc_core_args;

/**
 * @brief IVC message structure.
 */
typedef struct ivc_msg_common {
	/**
	 * Status code returned as part of response message of IVC messages.
	 * Status code value is "0" for success and "< 0" for failure.
	 */
	nve32_t status;
	/**
	 * ID of the CMD.
	 */
	ivc_cmd cmd;
	/**
	 * message count, used for debug
	 */
	nveu32_t count;

	union {
		/**
		 * IVC argument structure
		 */
		ivc_args args;
#ifndef OSI_STRIPPED_LIB
		/**
		 * avb algorithm structure
		 */
		struct osi_core_avb_algorithm avb_algo;
#endif
		/**
		 * OSI filter structure
		 */
		struct osi_filter filter;
		/** OSI HW features */
		struct osi_hw_features hw_feat;
		/**
		 * core argument structure
		 */
		ivc_core_args init_args;
	}data;
} ivc_msg_common;

/**
 * @brief osd_ivc_send_cmd - OSD ivc send cmd
 *
 * @param[in] priv: OSD private data
 * @param[in] data: data
 * @param[in] len: length of data
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: Yes
 *
 * @retval ivc status
 * @retval -1 on failure
 */
nve32_t osd_ivc_send_cmd(void *priv, void *data, nveu32_t len);

/**
 * @brief ivc_get_core_safety_config - Get core safety config
 *
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: Yes
 */
void *ivc_get_core_safety_config(void);
#endif /* IVC_CORE_H */
