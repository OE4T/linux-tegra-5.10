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
 *	ether_mac_loopback_show - Shows the current setting of MAC loopback
 *	@dev: Device data.
 *	@attr: Device attribute
 *	@buf: Buffer to store the current MAC loopback setting
 *
 *	Algorithm: Display the current MAC loopback setting.
 *
 *	Dependencies: MAC and PHY need to be initialized.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
static ssize_t ether_mac_loopback_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct net_device *ndev = (struct net_device *)dev_get_drvdata(dev);
	struct ether_priv_data *pdata = netdev_priv(ndev);

	return sprintf(buf, "%s\n", (pdata->mac_loopback_mode == 1) ?
		       "enabled" : "disabled");
}

/**
 *	ether_mac_loopback_store - Set the user setting of MAC loopback mode
 *	@dev: Device data.
 *	@attr: Device attribute
 *	@buf: Buffer which contains the user settings of MAC loopback
 *	@size: size of buffer
 *
 *	Algorithm: This is used to set the user mode settings of MAC loopback.
 *
 *	Dependencies: MAC and PHY need to be initialized.
 *
 *	Protection: None.
 *
 *	Return: size of buffer.
 */
static ssize_t ether_mac_loopback_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	struct net_device *ndev = (struct net_device *)dev_get_drvdata(dev);
	struct ether_priv_data *pdata = netdev_priv(ndev);
	int ret = -1;

	/* Interface is not up so LB mode can't be set */
	if (!netif_running(ndev)) {
		dev_err(pdata->dev, "Not Allowed. Ether interface is not up\n");
		return size;
	}

	if (strncmp(buf, "enable", 6) == 0U) {
		netif_carrier_on(ndev);
		/* Enabling the MAC Loopback Mode */
		ret = osi_config_mac_loopback(pdata->osi_core, OSI_ENABLE);
		if (ret < 0) {
			dev_err(pdata->dev, "Enabling MAC Loopback failed\n");
		} else {
			pdata->mac_loopback_mode = 1;
			dev_info(pdata->dev, "Enabling MAC Loopback\n");
		}
	} else if (strncmp(buf, "disable", 7) == 0U) {
		netif_carrier_off(ndev);
		/* Disabling the MAC Loopback Mode */
		ret = osi_config_mac_loopback(pdata->osi_core, OSI_DISABLE);
		if (ret < 0) {
			dev_err(pdata->dev, "Disabling MAC Loopback failed\n");
		} else {
			pdata->mac_loopback_mode = 0;
			dev_info(pdata->dev, "Disabling MAC Loopback\n");
		}
	} else {
		dev_err(pdata->dev,
			"Invalid entry. Valid Entries are enable or disable\n");
	}

	return size;
}

static DEVICE_ATTR(mac_loopback, (S_IRUGO | S_IWUSR),
		   ether_mac_loopback_show,
		   ether_mac_loopback_store);

static struct attribute *ether_sysfs_attrs[] = {
	&dev_attr_mac_loopback.attr,
	NULL
};

static struct attribute_group ether_attribute_group = {
	.name = "nvethernet",
	.attrs = ether_sysfs_attrs,
};

/**
 *	ether_sysfs_register - Creates nvethernet sysfs group
 *	@dev: Net device data.
 *
 *	Algorithm: Creates the sysfs group.
 *
 *	Dependencies: MAC and PHY need to be initialized.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, negative value - failure.
 */

int ether_sysfs_register(struct device *dev)
{
	/* Create nvethernet sysfs group under /sys/devices/<ether_device>/ */
	return sysfs_create_group(&dev->kobj, &ether_attribute_group);
}

/**
 *	ether_sysfs_unregister - Removes nvethernet sysfs group
 *	@dev: Net device data.
 *
 *	Algorithm: Removes the sysfs group.
 *
 *	Dependencies: nvethernet sysfs group need to be registered during probe.
 *
 *	Protection: None.
 *
 *	Return: None.
 */

void ether_sysfs_unregister(struct device *dev)
{
	/* Remove nvethernet sysfs group under /sys/devices/<ether_device>/ */
	sysfs_remove_group(&dev->kobj, &ether_attribute_group);
}
