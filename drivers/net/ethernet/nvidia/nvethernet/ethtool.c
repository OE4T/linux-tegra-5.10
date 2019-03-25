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

#include "ether_linux.h"

static const struct ethtool_ops ether_ethtool_ops = {
	.get_link = ethtool_op_get_link,
	.get_link_ksettings = phy_ethtool_get_link_ksettings,
	.set_link_ksettings = phy_ethtool_set_link_ksettings,
};

/**
 *	ether_set_ethtool_ops - Set ethtool operations
 *	@ndev: network device instance
 *
 *	Algorithm: Sets ethtool operations in network
 *	device structure.
 *
 *	Dependencies: Network device needs to created.
 *	Protection: None.
 *	Return: None.
 */
void ether_set_ethtool_ops(struct net_device *ndev)
{
	ndev->ethtool_ops = &ether_ethtool_ops;
}
