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
 *	ether_set_avb_algo - function to handle private ioctl
 *	EQOS_AVB_ALGORITHM
 *	@ndev: network device structure
 *	@ifdata: interface private data structure
 *
 *	Algorithm:
 *	- Call osi_set_avb with user passed data
 *
 *	Dependencies: Ethernet interface need to be up.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, negative value - failure.
 */
static int ether_set_avb_algo(struct net_device *ndev,
			      struct ether_ifr_data *ifdata)
{
	struct ether_priv_data *pdata = netdev_priv(ndev);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	struct osi_core_avb_algorithm l_avb_struct;
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

	return osi_set_avb(osi_core, &l_avb_struct);
}

/**
 *	ether_get_avb_algo - function to get avb data from registers.
 *	This function is called for EQOS_GET_AVB_ALGORITHM
 *	@ndev: network device structure
 *	@ifdata: interface private data structure
 *
 *	Algorithm:
 *	- Call osi_get_avb with user passed data(qindex)
 *
 *	Dependencies: Ethernet interface need to be up. Caller should
 *	check for return vlaue before using return value.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, negative value - failure.
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

/*
 *	ether_config_arp_offload - Handle ioctl to enable/disable ARP offload
 *	@pdata: OS dependent private data structure.
 *	@ifrd_p: Interface request private data pointer.
 *
 *	Algorithm:
 *	1) Copy the priv data from user space. This includes the IP address
 *	to be updated in HW.
 *	2) Check if IP address provided in priv data is valid.
 *	3) If IP address is valid, invoke OSI API to update HW registers.
 *
 *	Dependencies: Interface should be running (enforced by caller).
 *
 *	Protection: None.
 *
 *	Return: 0 - success, -ve value - failure
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
 *	ether_config_l3_l4_filtering- This function is invoked by ioctl
 *	when user issues an ioctl command to enable/disable L3/L4 filtering.
 *
 *	@dev: pointer to net device structure.
 *	@filter_flags: flag to indicate whether L3/L4 filtering to be
 *	enabled/disabled.
 *
 *	Algorithm:
 *	1) check if filter enalbed/disable already and return success.
 *	2) OSI call to update register
 *
 *	Dependencies: MAC and PHY need to be initialized.
 *
 *	Protection: None.
 *
 *	Return 0- sucessful, non-zero - error
 *
 */
static int ether_config_l3_l4_filtering(struct net_device *dev,
					unsigned int filter_flags)
{
	struct ether_priv_data *pdata = netdev_priv(dev);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	int ret = 0;

	if (filter_flags == pdata->l3_l4_filter) {
		dev_err(pdata->dev, "L3/L4 filtering is already %d\n",
			filter_flags);
		return ret;
	}

	ret = osi_config_l3_l4_filter_enable(osi_core, filter_flags);
	if (ret == 0) {
		spin_lock_bh(&pdata->ioctl_lock);
		pdata->l3_l4_filter = filter_flags;
		spin_unlock_bh(&pdata->ioctl_lock);
	}

	return ret;
}

/**
 *	ether_config_ip4_filters - this function is invoked by ioctl function
 *	when user issues an ioctl command to configure L3(IPv4) filtering.
 *
 *	@dev: Pointer to net device structure.
 *	@ifdata: pointer to IOCTL specific structure.
 *
 *	Algorithm:
 *	1) Enable/disable IPv4 filtering.
 *	2) Select source/destination address matching.
 *	3) Select perfect/inverse matching.
 *	4) Update the IPv4 address into MAC register.
 *
 *	Dependencies: MAC and PHY need to be initialized.
 *
 *	Protection: None.
 *
 *	Return 0- sucessful, non-zero - error
 */
static int ether_config_ip4_filters(struct net_device *dev,
				    struct ether_ifr_data *ifdata)
{
	struct ether_priv_data *pdata = netdev_priv(dev);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	struct osi_l3_l4_filter *u_l3_filter =
		(struct osi_l3_l4_filter *)ifdata->ptr;
	struct osi_l3_l4_filter l_l3_filter;
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

	spin_lock_bh(&pdata->ioctl_lock);
	if (pdata->l3_l4_filter == OSI_DISABLE) {
		ret = osi_config_l3_l4_filter_enable(osi_core, 1);
		if (ret == 0) {
			pdata->l3_l4_filter = OSI_ENABLE;
		}
	}
	spin_unlock_bh(&pdata->ioctl_lock);

	/* configure the L3 filters */
	ret = osi_config_l3_filters(osi_core, l_l3_filter.filter_no,
				    l_l3_filter.filter_enb_dis,
				    OSI_IP4_FILTER,
				    l_l3_filter.src_dst_addr_match,
				    l_l3_filter.perfect_inverse_match);
	if (ret != 0) {
		dev_err(pdata->dev, "osi_config_l3_filters failed\n");
		return ret;
	}

	ret = osi_update_ip4_addr(osi_core, l_l3_filter.filter_no,
				  l_l3_filter.ip4_addr,
				  l_l3_filter.src_dst_addr_match);

	return ret;
}

/**
 *	ether_config_ip6_filters- This function is invoked by ioctl when user
 *	issues an ioctl command to configure L3(IPv6) filtering.
 *
 *	@dev: pointer to net device structure.
 *	@ifdata:pointer to IOCTL specific structure.
 *
 *	Algorithm:
 *	1) Enable/disable IPv6 filtering.
 *	2) Select source/destination address matching.
 *	3) Select perfect/inverse matching.
 *	4) Update the IPv6 address into MAC register.
 *
 *	Dependencies: MAC and PHY need to be initialized.
 *
 *	Protection: None.
 *	Return 0- sucessful, non-zero - error
 *
 */
static int ether_config_ip6_filters(struct net_device *dev,
				    struct ether_ifr_data *ifdata)
{
	struct ether_priv_data *pdata = netdev_priv(dev);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	struct osi_l3_l4_filter *u_l3_filter =
		(struct osi_l3_l4_filter *)ifdata->ptr;
	struct osi_l3_l4_filter l_l3_filter;
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

	spin_lock_bh(&pdata->ioctl_lock);
	if (pdata->l3_l4_filter == OSI_DISABLE) {
		ret = osi_config_l3_l4_filter_enable(osi_core, 1);
		if (ret == 0) {
			pdata->l3_l4_filter = OSI_ENABLE;
		}
	}
	spin_unlock_bh(&pdata->ioctl_lock);

	/* configure the L3 filters */
	ret = osi_config_l3_filters(osi_core, l_l3_filter.filter_no,
				    l_l3_filter.filter_enb_dis,
				    OSI_IP6_FILTER,
				    l_l3_filter.src_dst_addr_match,
				    l_l3_filter.perfect_inverse_match);
	if (ret != 0) {
		dev_err(pdata->dev, "osi_config_l3_filters failed\n");
		return ret;
	}

	return osi_update_ip6_addr(osi_core, l_l3_filter.filter_no,
				   l_l3_filter.ip6_addr);
}

/**
 *	ether_config_tcp_udp_filters-  This function is invoked by
 *	ioctl function when user issues an ioctl command to configure
 *	L4(TCP/UDP) filtering.
 *
 *	@dev: pointer to net device structure.
 *	@ifdata: pointer to IOCTL specific structure.
 *	@tcp_udp: flag to indicate TCP/UDP filtering.
 *
 *	Algorithm:
 *	1) Enable/disable L4 filtering.
 *	2) Select TCP/UDP filtering.
 *	3) Select source/destination port matching.
 *	4) select perfect/inverse matching.
 *	5) Update the port number into MAC register.
 *
 *	Dependencies: MAC and PHY need to be initialized.
 *
 *	Protection: None.
 *
 *	Return 0- sucessful, non-zero - error
 *
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

	/* configure the L4 filters */
	ret = osi_config_l4_filters(osi_core, l_l4_filter.filter_no,
				    l_l4_filter.filter_enb_dis,
				    tcp_udp,
				    l_l4_filter.src_dst_addr_match,
				    l_l4_filter.perfect_inverse_match);
	if (ret != 0) {
		dev_err(pdata->dev, "osi_config_l4_filters failed\n");
		return ret;
	}

	ret = osi_update_l4_port_no(osi_core, l_l4_filter.filter_no,
				    l_l4_filter.port_no,
				    l_l4_filter.src_dst_addr_match);

	return ret;
}

/**
 *	ether_config_vlan_filter- This function is invoked by ioctl function
 *	when user issues an ioctl command to configure VALN filtering.
 *
 *	@dev: pointer to net device structure.
 *	@ifdata: pointer to IOCTL specific structure.
 *
 *	Algorithm:
 *	1) enable/disable VLAN filtering.
 *	2) select perfect/hash filtering.
 *
 *	Dependencies: MAC and PHY need to be initialized.
 *
 *	Protection: None.
 *
 *	Return 0- sucessful, non-zero - error
 *
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
	if ((l_vlan_filter.perfect_hash == OSI_HASH_FILTER_MODE) &&
	    (pdata->hw_feat.vlan_hash_en == OSI_DISABLE)) {
		dev_err(pdata->dev, "VLAN HASH filtering is not supported\n");
		return ret;
	}
	/* configure the vlan filter FIXME: Current code supports VLAN
	   filtering for last VLAN tag/id added or default tag/vid 1. */
	ret = osi_config_vlan_filtering(osi_core, l_vlan_filter.filter_enb_dis,
					l_vlan_filter.perfect_hash,
					l_vlan_filter.perfect_inverse_match);
	if (ret == 0) {
		pdata->vlan_hash_filtering = l_vlan_filter.perfect_hash;
	}

	return ret;
}

/**
 *	ether_config_l2_da_filter- This function is invoked by ioctl function
 *	when user issues an ioctl command to configure L2 destination
 *	addressing filtering mode.
 *
 *	@dev: Pointer to net device structure.
 *	@ifdata: Pointer to IOCTL specific structure.
 *
 *	Algorithm:
 *	1) Selects perfect/hash filtering.
 *	2) Selects perfect/inverse matching.
 *
 *	Dependencies: MAC and PHY need to be initialized.
 *
 *	Protection: None.
 *
 *	Return 0- sucessful, non-zero - error
 */
static int ether_config_l2_da_filter(struct net_device *dev,
				     struct ether_ifr_data *ifdata)
{
	struct ether_priv_data *pdata = netdev_priv(dev);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	struct osi_l2_da_filter *u_l2_da_filter =
		(struct osi_l2_da_filter *)ifdata->ptr;
	struct osi_l2_da_filter l_l2_da_filter;
	int ret = -EINVAL;

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
	ret = osi_config_l2_da_perfect_inverse_match(osi_core,
				     l_l2_da_filter.perfect_inverse_match);

	return ret;
}

/**
 *	ether_priv_ioctl - Handle private IOCTLs
 *	@ndev: network device structure
 *	@ifr: Interface request structure used for socket ioctl's.
 *
 *	Algorithm:
 *	1) Copy the priv command data from user space.
 *	2) Check the priv command cmd and invoke handler func.
 *	if it is supported.
 *	3) Copy result back to user space.
 *
 *	Dependencies: Interface should be running (enforced by caller).
 *
 *	Protection: None.
 *
 *	Return: 0 - success, negative value - failure.
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
