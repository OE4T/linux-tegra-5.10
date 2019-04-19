/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION.  All rights reserved.
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

#define NUM_BYTES_IN_IPADDR	4
#define MAX_IP_ADDR_BYTE	0xFF
/* Remote wakeup filter */
#define EQOS_RWK_FILTER_LENGTH	8

/* private ioctl number*/
#define ETHER_AVB_ALGORITHM	27
#define ETHER_GET_AVB_ALGORITHM	46
#define ETHER_CONFIG_ARP_OFFLOAD	36

/**
 *	struct ether_ifr_data - Private data of struct ifreq
 *	@flags: Flags used for specific ioctl - like enable/disable
 *	@qinx: Queue index to be used for certain ioctls.
 *		Not in use, keep for app compatibility.
 *		Some applications already use this same struct
 *	@cmd: The private ioctl command number.
 *	@context_setup: Used to indicate if context descriptor needs
 *		to be setup to handle ioctl.
 *		Not in use, keep for app compatibility.
 *	@connected_speed: Used to query the connected link speed.
 *		Not in use, keep for app compatibility.
 *	@rwk_filter_values: Used to set Remote wakeup filters.
 *		Not in use, keep for app compatibility.
 *	@rwk_filter_length: Number of remote wakeup filters to use.
 *		Not in use, keep for app compatibility.
 *	@command_error: The return value of IOCTL handler func.
 *		This is passed back to user space application.
 *	@test_done: Not in use, keep for app compatibility.
 *	@ptr: IOCTL cmd specific structure pointer.
 */
struct ether_ifr_data {
	unsigned int if_flags;
	unsigned int qinx;
	unsigned int ifcmd;
	unsigned int context_setup;
	unsigned int connected_speed;
	unsigned int rwk_filter_values[EQOS_RWK_FILTER_LENGTH];
	unsigned int rwk_filter_length;
	int command_error;
	int test_done;
	void *ptr;
};

/**
 *	struct arp_offload_param - Parameter to support ARP offload.
 *	@ip_addr: Byte array for decimal representation of IP address.
 *	For example, 192.168.1.3 is represented as
 *		ip_addr[0] = '192'
 *		ip_addr[1] = '168'
 *		ip_addr[2] = '1'
 *		ip_addr[3] = '3'
 */
struct arp_offload_param {
	unsigned char ip_addr[NUM_BYTES_IN_IPADDR];
};

/* Private ioctl handler function */
int ether_handle_priv_ioctl(struct net_device *ndev,
			    struct ifreq *ifr);
#endif

