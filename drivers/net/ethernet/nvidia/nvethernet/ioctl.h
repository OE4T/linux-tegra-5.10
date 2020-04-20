/*
 * Copyright (c) 2018-2020, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef IOCTL_H
#define IOCTL_H

/**
 *@addtogroup IOCTL Helper MACROS
 * @{
 */
#define NUM_BYTES_IN_IPADDR	4
#define MAX_IP_ADDR_BYTE	0xFFU

/* class E IP4 addr start range, reserved */
#define CLASS_E_IP4_ADDR_RANGE_START	240U
/* class D multicast addr range */
#define MIN_MC_ADDR_RANGE	224U
#define MAX_MC_ADDR_RANGE	239U

/* Remote wakeup filter */
#define EQOS_RWK_FILTER_LENGTH		8
#define ETHER_PRV_TS_IOCTL		(SIOCDEVPRIVATE + 1)
/* TX/RX channel/queue count */
#define EQOS_GET_TX_QCNT		23
#define EQOS_GET_RX_QCNT		24
/* Line speed */
#define EQOS_GET_CONNECTED_SPEED	25
/* private ioctl number*/
#define ETHER_AVB_ALGORITHM		27
/* L3/L4 filter */
#define EQOS_L3_L4_FILTER_CMD		29
/* IPv4/6 and TCP/UDP filtering */
#define EQOS_IPV4_FILTERING_CMD		30
#define EQOS_IPV6_FILTERING_CMD		31
#define EQOS_UDP_FILTERING_CMD		32
#define EQOS_TCP_FILTERING_CMD		33
/* VLAN filtering */
#define EQOS_VLAN_FILTERING_CMD		34
/* L2 DA filtering */
#define EQOS_L2_DA_FILTERING_CMD	35
#define ETHER_CONFIG_ARP_OFFLOAD	36
#define ETHER_CONFIG_LOOPBACK_MODE	40
#define ETHER_GET_AVB_ALGORITHM		46
#define ETHER_SAVE_RESTORE		47
#define ETHER_PTP_RXQUEUE		48
#define ETHER_CONFIG_EST		49
#define ETHER_CONFIG_FPE		50
/* FRP Command */
#define ETHER_CONFIG_FRP_CMD		51
/** @} */

/**
 * @brief struct ether_ifr_data - Private data of struct ifreq
 */
struct ether_ifr_data {
	/** Flags used for specific ioctl - like enable/disable */
	unsigned int if_flags;
	/** qinx: Queue index to be used for certain ioctls */
	unsigned int qinx;
	/** The private ioctl command number */
	unsigned int ifcmd;
	/** Used to indicate if context descriptor needs to be setup to
	 * handle ioctl */
	unsigned int context_setup;
	/** Used to query the connected link speed */
	unsigned int connected_speed;
 	/** Used to set Remote wakeup filters */
	unsigned int rwk_filter_values[EQOS_RWK_FILTER_LENGTH];
	/** Number of remote wakeup filters to use */
	unsigned int rwk_filter_length;
	/** The return value of IOCTL handler func */
	int command_error;
	/** test_done: Not in use, keep for app compatibility */
	int test_done;
	/** IOCTL cmd specific structure pointer */
	void *ptr;
};

/**
 * @brief struct arp_offload_param - Parameter to support ARP offload.
 */
struct arp_offload_param {
	/** ip_addr: Byte array for decimal representation of IP address.
	 * For example, 192.168.1.3 is represented as
	 * ip_addr[0] = '192' ip_addr[1] = '168' ip_addr[2] = '1' 
	 * ip_addr[3] = '3' */
	unsigned char ip_addr[NUM_BYTES_IN_IPADDR];
};

/**
 * @brief struct ifr_data_timestamp_struct - common data structure between
 *	driver and application for sharing info through private TS ioctl
 */
struct ifr_data_timestamp_struct {
	/** Clock ID */
	clockid_t clockid;
	/** Store kernel time */
	struct timespec64 kernel_ts;
	/** Store HW time */
	struct timespec64 hw_ptp_ts;
};

/**
 * @brief ether_priv_ioctl - Handle private IOCTLs
 *	Algorithm:
 *	1) Copy the priv command data from user space.
 *	2) Check the priv command cmd and invoke handler func.
 *	if it is supported.
 *	3) Copy result back to user space.
 *
 * @param[in] ndev: network device structure
 * @param[in] ifr: Interface request structure used for socket ioctl's.
 *
 * @note Interface should be running (enforced by caller).
 *
 * @retval 0 on success
 * @retval negative value on failure.
 */
int ether_handle_priv_ioctl(struct net_device *ndev,
			    struct ifreq *ifr);
#endif
