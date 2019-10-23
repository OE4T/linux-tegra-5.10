/*
 * Copyright (c) 2019, NVIDIA CORPORATION.  All rights reserved.
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

#include "ether_linux.h"

/**
 * @brief Function to handle private ioctl - EQOS_AVB_ALGORITHM
 *
 * Algorithm: Call osi_set_avb with user passed data
 *
 * @param[in] ndev: network device structure
 * @param[in] ifdata: interface private data structure
 *
 * @note Ethernet interface need to be up.
 *
 * @retval 0 on Sucess
 * @retval "nagative value" on Failure
 */
static int ether_set_avb_algo(struct net_device *ndev,
			      struct ether_ifr_data *ifdata)
{
	struct ether_priv_data *pdata = netdev_priv(ndev);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	struct osi_core_avb_algorithm l_avb_struct;
	struct osi_dma_priv_data *osi_dma = pdata->osi_dma;
	struct osi_tx_ring *tx_ring = NULL;
	int ret = -1;

	if (ifdata->ptr == NULL) {
		dev_err(pdata->dev, "%s: Invalid data for priv ioctl %d\n",
			__func__, ifdata->ifcmd);
		return ret;
	}

	if (copy_from_user(&l_avb_struct,
			   (struct osi_core_avb_algorithm *)ifdata->ptr,
			   sizeof(struct osi_core_avb_algorithm)) != 0U) {
		dev_err(pdata->dev,
			"Failed to fetch AVB Struct info from user\n");
		return ret;
	}

	/* Check AVB mode disable on slot function enable */
	tx_ring = osi_dma->tx_ring[l_avb_struct.qindex];
	if (tx_ring && tx_ring->slot_check == OSI_ENABLE &&
	    l_avb_struct.oper_mode == OSI_MTL_QUEUE_ENABLE) {
		dev_err(pdata->dev,
			"Can't disable queue:%d AVB mode when slot is enabled",
			l_avb_struct.qindex);
		return -EINVAL;
	}

	return osi_set_avb(osi_core, &l_avb_struct);
}

/**
 * @brief Function to get avb data from registers. This function is called for
 * EQOS_GET_AVB_ALGORITHM
 *
 * Algorithm: Call osi_get_avb with user passed data(qindex)
 *
 * @param[in] ndev: network device structure
 * @param[in] ifdata: interface private data structure
 *
 * @note Ethernet interface need to be up. Caller should check for return
 * value before using return value.
 *
 * @retval 0 on Sucess
 * @retval "negative value" on Failure
 */
static int ether_get_avb_algo(struct net_device *ndev,
			      struct ether_ifr_data *ifdata)
{
	struct ether_priv_data *pdata = netdev_priv(ndev);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	struct osi_core_avb_algorithm avb_data;
	int ret;

	if (ifdata->ptr == NULL) {
		dev_err(pdata->dev, "%s: Invalid data for priv ioctl %d\n",
			__func__, ifdata->ifcmd);
		return -EINVAL;
	}

	if (copy_from_user(&avb_data,
			   (struct osi_core_avb_algorithm *)ifdata->ptr,
			   sizeof(struct osi_core_avb_algorithm)) != 0U) {
		dev_err(pdata->dev,
			"Failed to fetch AVB Struct info from user\n");
		return -EFAULT;
	}

	ret = osi_get_avb(osi_core, &avb_data);
	if (ret != 0) {
		dev_err(pdata->dev,
			"Failed to get AVB Struct info from registers\n");
		return ret;
	}
	if (copy_to_user(ifdata->ptr, &avb_data,
			 sizeof(struct osi_core_avb_algorithm)) != 0U) {
		dev_err(pdata->dev, "%s: copy_to_user failed\n", __func__);
		return -EFAULT;
	}

	return ret;
}

/**
 * @brief Handle ioctl to enable/disable ARP offload
 *
 * Algorithm:
 * 1) Copy the priv data from user space. This includes the IP address
 * to be updated in HW.
 * 2) Check if IP address provided in priv data is valid.
 * 3) If IP address is valid, invoke OSI API to update HW registers.
 *
 * @param[in] pdata: OS dependent private data structure.
 * @param[in] ifrd_p: Interface request private data pointer.
 * 
 * @note Interface should be running (enforced by caller).
 *
 * @retval 0 on Sucess
 * @retval "negative value" on Failure
 */
static int ether_config_arp_offload(struct ether_priv_data *pdata,
				    struct ether_ifr_data *ifrd_p)
{
	int i, ret = -EINVAL;
	struct arp_offload_param param;
	/* TODO: Need Spin lock to prevent multiple apps from
	 * requesting same ioctls to the same MAC instance
	 */
	if (!ifrd_p->ptr) {
		dev_err(pdata->dev, "%s: Invalid data for priv ioctl %d\n",
			__func__, ifrd_p->ifcmd);
		return ret;
	}

	if (copy_from_user(&param, (struct arp_offload_param *)ifrd_p->ptr,
			   sizeof(struct arp_offload_param))) {
		dev_err(pdata->dev, "%s: copy_from_user failed\n", __func__);
		return ret;
	}

	for (i = 0; i < NUM_BYTES_IN_IPADDR; i++) {
		if (param.ip_addr[i] > MAX_IP_ADDR_BYTE) {
			dev_err(pdata->dev, "%s: Invalid IP addr\n", __func__);
			return ret;
		}
	}

	ret = osi_config_arp_offload(pdata->osi_core, ifrd_p->if_flags,
				     param.ip_addr);
	dev_err(pdata->dev, "ARP offload: %s : %s\n",
		ifrd_p->if_flags ? "Enable" : "Disable",
		ret ? "Failed" : "Success");

	return ret;
}

/**
 * @brief This function is invoked by ioctl when user issues an ioctl command
 * to enable/disable L3/L4 filtering.
 *
 * Algorithm:
 * 1) check if filter enalbed/disable already and return success.
 * 2) OSI call to update register
 *
 * @param[in] dev: pointer to net device structure.
 * @param[in] filter_flags: flag to indicate whether L3/L4 filtering to be
 *	      enabled/disabled.
 *
 * @note MAC and PHY need to be initialized.
 *
 * @retval 0 on Sucess
 * @retval "negative value" on Failure
 *
 */
static int ether_config_l3_l4_filtering(struct net_device *dev,
					unsigned int filter_flags)
{
	struct ether_priv_data *pdata = netdev_priv(dev);

	dev_err(pdata->dev, "%s: This ioctl is deprecated, directly set the filter using ioctl command EQOS_IPV4/IPV6/TCP/UDP_FILTERING_CMD instead\n",
		__func__);
	return -1;
}

/**
 * @brief This function is invoked by ioctl function when user issues an ioctl
 * command to configure L3(IPv4) filtering.
 *
 * Algorithm:
 * 1) Layer 3 and Layer 4 Filter Enable, if already not.
 * 2) Enable/disable IPv4 filtering.
 * 3) Select source/destination address matching.
 * 4) Select perfect/inverse matching.
 * 5) Update the IPv4 address into MAC register.
 *
 * @param[in] dev: Pointer to net device structure.
 * @param[in] ifdata: pointer to IOCTL specific structure.
 * 
 * @note MAC and PHY need to be initialized.
 *
 * @retval 0 on Sucess
 * @retval "negative value" on Failure
 */
static int ether_config_ip4_filters(struct net_device *dev,
				    struct ether_ifr_data *ifdata)
{
	struct ether_priv_data *pdata = netdev_priv(dev);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	struct osi_l3_l4_filter *u_l3_filter =
		(struct osi_l3_l4_filter *)ifdata->ptr;
	struct osi_l3_l4_filter l_l3_filter;
	unsigned int is_l4_filter = OSI_DISABLE;
	int ret = -EINVAL;

	if (pdata->hw_feat.l3l4_filter_num == OSI_DISABLE) {
		dev_err(pdata->dev, "ip4 filter is not supported\n");
		return ret;
	}

	if (ifdata->ptr == NULL) {
		dev_err(pdata->dev, "%s: Invalid data for priv ioctl %d\n",
			__func__, ifdata->ifcmd);
		return ret;
	}

	if (copy_from_user(&l_l3_filter, u_l3_filter,
			   sizeof(struct osi_l3_l4_filter)) != 0U) {
		dev_err(pdata->dev, "%s copy from user failed\n", __func__);
		return -EFAULT;
	}

	if (l_l3_filter.filter_no > (pdata->hw_feat.l3l4_filter_num - 1U)) {
		dev_err(pdata->dev, "%d filter is not supported in the HW\n",
			l_l3_filter.filter_no);
		return ret;
	}

	return osi_l3l4_filter(osi_core, l_l3_filter, OSI_IP4_FILTER,
			       OSI_DISABLE, OSI_CHAN_ANY, is_l4_filter);
}

/**
 * @brief This function is invoked by ioctl when user issues an ioctl command
 * to configure L3 (IPv6) filtering.
 *
 * Algorithm:
 * 1) Enable/disable IPv6 filtering.
 * 2) Select source/destination address matching.
 * 3) Select perfect/inverse matching.
 * 4) Update the IPv6 address into MAC register.
 *
 * @param[in] dev: net device structure instance.
 * @param[in] ifdata: IOCTL specific structure instance.
 *
 * @note MAC and PHY need to be initialized.
 *
 * @retval 0 on Sucess
 * @retval "negative value" on Failure
 */
static int ether_config_ip6_filters(struct net_device *dev,
				    struct ether_ifr_data *ifdata)
{
	struct ether_priv_data *pdata = netdev_priv(dev);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	struct osi_l3_l4_filter *u_l3_filter =
		(struct osi_l3_l4_filter *)ifdata->ptr;
	struct osi_l3_l4_filter l_l3_filter;
	unsigned int is_l4_filter = OSI_DISABLE;
	int ret = -EINVAL;

	if (pdata->hw_feat.l3l4_filter_num == OSI_DISABLE) {
		dev_err(pdata->dev, "ip6 filter is not supported in the HW\n");
		return ret;
	}

	if (ifdata->ptr == NULL) {
		dev_err(pdata->dev, "%s: Invalid data for priv ioctl %d\n",
			__func__, ifdata->ifcmd);
		return ret;
	}

	if (copy_from_user(&l_l3_filter, u_l3_filter,
			   sizeof(struct osi_l3_l4_filter)) != 0U) {
		dev_err(pdata->dev, "%s copy from user failed\n", __func__);
		return -EFAULT;
	}

	if (l_l3_filter.filter_no > (pdata->hw_feat.l3l4_filter_num - 1U)) {
		dev_err(pdata->dev, "%d filter is not supported in the HW\n",
			l_l3_filter.filter_no);
		return ret;
	}

	return osi_l3l4_filter(osi_core, l_l3_filter, OSI_IP6_FILTER,
			       OSI_DISABLE, OSI_CHAN_ANY, is_l4_filter);
}

/**
 * @brief This function is invoked by ioctl function when user issues an ioctl
 * command to configure L4(TCP/UDP) filtering.
 *
 * Algorithm:
 * 1) Enable/disable L4 filtering.
 * 2) Select TCP/UDP filtering.
 * 3) Select source/destination port matching.
 * 4) select perfect/inverse matching.
 * 5) Update the port number into MAC register.
 *
 * @param[in] dev: pointer to net device structure.
 * @param[in] ifdata: pointer to IOCTL specific structure.
 * @param[in] tcp_udp: flag to indicate TCP/UDP filtering.
 * 
 * @note MAC and PHY need to be initialized.
 *
 * @retval 0 on Sucess
 * @retval "negative value" on Failure
 */
static int ether_config_tcp_udp_filters(struct net_device *dev,
					struct ether_ifr_data *ifdata,
					unsigned int tcp_udp)
{
	struct ether_priv_data *pdata = netdev_priv(dev);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	struct osi_l3_l4_filter *u_l4_filter =
		(struct osi_l3_l4_filter *)ifdata->ptr;
	struct osi_l3_l4_filter l_l4_filter;
	unsigned int is_l4_filter = OSI_ENABLE;
	int ret = -EINVAL;

	if (ifdata->ptr == NULL) {
		dev_err(pdata->dev, "%s: Invalid data for priv ioctl %d\n",
			__func__, ifdata->ifcmd);
		return ret;
	}

	if (pdata->hw_feat.l3l4_filter_num == OSI_DISABLE) {
		dev_err(pdata->dev,
			"L4 is not supported in the HW\n");
		return ret;
	}

	if (copy_from_user(&l_l4_filter, u_l4_filter,
			   sizeof(struct osi_l3_l4_filter)) != 0U) {
		dev_err(pdata->dev, "%s copy from user failed", __func__);
		return -EFAULT;
	}

	if (l_l4_filter.filter_no > (pdata->hw_feat.l3l4_filter_num - 1U)) {
		dev_err(pdata->dev, "%d filter is not supported in the HW\n",
			l_l4_filter.filter_no);
		return ret;
	}

	return osi_l3l4_filter(osi_core, l_l4_filter, tcp_udp,
			       OSI_DISABLE, OSI_CHAN_ANY, is_l4_filter);
}

/**
 * @brief This function is invoked by ioctl functio when user issues an ioctl
 * command to configure VALN filtering.
 *
 * Algorithm:
 * 1) enable/disable VLAN filtering.
 * 2) select perfect/hash filtering.
 *
 * @param[in] dev: pointer to net device structure.
 * @param[in] ifdata: pointer to IOCTL specific structure.
 *
 * @note MAC and PHY need to be initialized.
 *
 * @retval 0 on Sucess
 * @retval "negative value" on Failure
 */
static int ether_config_vlan_filter(struct net_device *dev,
				    struct ether_ifr_data *ifdata)
{
	struct ether_priv_data *pdata = netdev_priv(dev);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	struct osi_vlan_filter *u_vlan_filter =
		(struct osi_vlan_filter *)ifdata->ptr;
	struct osi_vlan_filter l_vlan_filter;
	int ret = -EINVAL;

	if (ifdata->ptr == NULL) {
		dev_err(pdata->dev, "%s: Invalid data for priv ioctl %d\n",
			__func__, ifdata->ifcmd);
		return ret;
	}

	if (copy_from_user(&l_vlan_filter, u_vlan_filter,
			   sizeof(struct osi_vlan_filter)) != 0U) {
		dev_err(pdata->dev, "%s copy from user failed", __func__);
		return -EFAULT;
	}

	/*0 - perfect and 1 - hash filtering */
	if (l_vlan_filter.perfect_hash == OSI_HASH_FILTER_MODE) {
		dev_err(pdata->dev, "VLAN HASH filtering is not supported\n");
		return ret;
	}

	ret = osi_config_vlan_filtering(osi_core, l_vlan_filter.filter_enb_dis,
					l_vlan_filter.perfect_hash,
					l_vlan_filter.perfect_inverse_match);
	if (ret == 0) {
		pdata->vlan_hash_filtering = l_vlan_filter.perfect_hash;
	}

	return ret;
}

/**
 * @brief This function is invoked by ioctl function when user issues an ioctl
 * command to configure L2 destination addressing filtering mode.
 *
 * Algorithm:
 * 1) Selects perfect/hash filtering.
 * 2) Selects perfect/inverse matching.
 *
 * @param[in] dev: Pointer to net device structure.
 * @param[in] ifdata: Pointer to IOCTL specific structure.
 *
 * @note MAC and PHY need to be initialized.
 *
 * @retval 0 on Sucess
 * @retval "negative value" on Failure
 */
static int ether_config_l2_da_filter(struct net_device *dev,
				     struct ether_ifr_data *ifdata)
{
	struct ether_priv_data *pdata = netdev_priv(dev);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	struct osi_l2_da_filter *u_l2_da_filter =
		(struct osi_l2_da_filter *)ifdata->ptr;
	struct osi_l2_da_filter l_l2_da_filter;
	struct osi_filter filter;
	int ret = -EINVAL;

	memset(&filter, 0x0, sizeof(struct osi_filter));

	if (ifdata->ptr == NULL) {
		dev_err(pdata->dev, "%s: Invalid data for priv ioctl %d\n",
			__func__, ifdata->ifcmd);
		return ret;
	}

	if (copy_from_user(&l_l2_da_filter, u_l2_da_filter,
			   sizeof(struct osi_l2_da_filter)) != 0U) {
		return -EFAULT;
	}

	if (l_l2_da_filter.perfect_hash == OSI_HASH_FILTER_MODE) {
		dev_err(pdata->dev,
			"select HASH FILTERING for L2 DA is not Supported in SW\n");
		return ret;
	} else {
		/* FIXME: Need to understand if filtering will work on addr0.
		 * Do we need to have pdata->num_mac_addr_regs > 1 check?
		 */
		pdata->l2_filtering_mode = OSI_PERFECT_FILTER_MODE;
	}

	/* configure L2 DA perfect/inverse_matching */
	if (l_l2_da_filter.perfect_inverse_match == OSI_ENABLE) {
		filter.oper_mode |= OSI_OPER_EN_L2_DA_INV;
	} else {
		filter.oper_mode |= OSI_OPER_DIS_L2_DA_INV;
	}

	ret = osi_l2_filter(osi_core, &filter);
	if (ret != 0) {
		dev_err(pdata->dev, "setting L2_DA_INV failed\n");
	}

	return ret;
}

/**
 * @brief This function is invoked by ioctl when user issues an ioctl command
 * to enable/disable MAC loopback mode.
 *
 * Algorithm:
 * 1) check if loopback mode enalbed/disable already and return success.
 * 2) OSI call to configure loopback mode in HW.
 *
 * @param[in] ndev: pointer to net device structure.
 * @param[in] flags: flag to indicate whether MAC loopback mode to be enabled or
 *	disabled.
 *
 * @note MAC and PHY need to be initialized.
 * 
 * @retval 0 on Sucess
 * @retval "negative value" on Failure
 */
static int ether_config_loopback_mode(struct net_device *ndev,
				      unsigned int flags)
{
	struct ether_priv_data *pdata = netdev_priv(ndev);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	int ret = 0;

	if ((flags && (pdata->mac_loopback_mode == OSI_ENABLE)) ||
	    (!flags && (pdata->mac_loopback_mode == OSI_DISABLE))) {
		dev_info(pdata->dev, "Loopback mode is already configured\n");
		return ret;
	}

	if (flags) {
		netif_carrier_on(ndev);
		ret = osi_config_mac_loopback(osi_core, OSI_ENABLE);
		if (ret < 0) {
			dev_err(pdata->dev,
				"Failed to enable MAC Loopback\n");
		} else {
			pdata->mac_loopback_mode = OSI_ENABLE;
			dev_info(pdata->dev, "MAC loopback enabled\n");
		}
	} else {
		netif_carrier_off(ndev);
		ret = osi_config_mac_loopback(osi_core, OSI_DISABLE);
		if (ret < 0) {
			dev_err(pdata->dev,
				"Failed to disable MAC Loopback\n");
		} else {
			pdata->mac_loopback_mode = OSI_DISABLE;
			dev_info(pdata->dev, "MAC loopback disabled\n");
		}
	}

	return ret;
}

/**
 * @brief ether_priv_ioctl - Handle private IOCTLs
 *
 * Algorithm:
 * 1) Copy the priv command data from user space.
 * 2) Check the priv command cmd and invoke handler function if it is supported
 * 3) Copy result back to user space.
 *
 * @param[in] ndev: network device structure
 * @param[in] ifr: Interface request structure used for socket ioctl's.
 *
 * @note Interface should be running (enforced by caller).
 *
 * @retval 0 on Sucess
 * @retval "negative value" on Failure
 */
int ether_handle_priv_ioctl(struct net_device *ndev,
			    struct ifreq *ifr)
{
	struct ether_priv_data *pdata = netdev_priv(ndev);
	struct ether_ifr_data ifdata;
	int ret = -EOPNOTSUPP;

	if (copy_from_user(&ifdata, ifr->ifr_data, sizeof(ifdata)) != 0U) {
		dev_err(pdata->dev, "%s(): copy_from_user failed %d\n"
			, __func__, __LINE__);
		return -EFAULT;
	}

	switch (ifdata.ifcmd) {
	case ETHER_AVB_ALGORITHM:
		ret = ether_set_avb_algo(ndev, &ifdata);
		break;
	case ETHER_GET_AVB_ALGORITHM:
		ret = ether_get_avb_algo(ndev, &ifdata);
		break;
	case ETHER_CONFIG_ARP_OFFLOAD:
		ret = ether_config_arp_offload(pdata, &ifdata);
		break;
	case EQOS_L3_L4_FILTER_CMD:
		/* flags should be 0x0 or 0x1, discard any other */
		if (pdata->hw_feat.l3l4_filter_num > 0U &&
		    ((ifdata.if_flags == OSI_ENABLE) ||
		     (ifdata.if_flags == OSI_DISABLE))) {
			ret = ether_config_l3_l4_filtering(ndev,
							   ifdata.if_flags);
			if (ret == 0) {
				ret = EQOS_CONFIG_SUCCESS;
			} else {
				ret = EQOS_CONFIG_FAIL;
			}
		} else {
			dev_err(pdata->dev, "L3/L4 filters are not supported\n");
			ret = -EOPNOTSUPP;
		}
		break;
	case EQOS_IPV4_FILTERING_CMD:
		ret = ether_config_ip4_filters(ndev, &ifdata);
		break;
	case EQOS_IPV6_FILTERING_CMD:
		ret = ether_config_ip6_filters(ndev, &ifdata);
		break;
	case EQOS_UDP_FILTERING_CMD:
		ret = ether_config_tcp_udp_filters(ndev, &ifdata,
						   OSI_L4_FILTER_UDP);
		break;
	case EQOS_TCP_FILTERING_CMD:
		ret = ether_config_tcp_udp_filters(ndev, &ifdata,
						   OSI_L4_FILTER_TCP);
		break;
	case EQOS_VLAN_FILTERING_CMD:
		ret = ether_config_vlan_filter(ndev, &ifdata);
		break;
	case EQOS_L2_DA_FILTERING_CMD:
		ret = ether_config_l2_da_filter(ndev, &ifdata);
		break;
	case ETHER_CONFIG_LOOPBACK_MODE:
		ret = ether_config_loopback_mode(ndev, ifdata.if_flags);
		break;
	default:
		break;
	}

	ifdata.command_error = ret;
	if (copy_to_user(ifr->ifr_data, &ifdata, sizeof(ifdata)) != 0U) {
		dev_err(pdata->dev, "%s: copy_to_user failed\n", __func__);
		return -EFAULT;
	}

	return ret;
}
