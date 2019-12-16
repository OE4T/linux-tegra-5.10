/*
 * Copyright (c) 2019-2020, NVIDIA CORPORATION.  All rights reserved.
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

#ifdef CONFIG_DEBUG_FS
/* As per IAS Docs */
#define EOQS_MAX_REGISTER_ADDRESS 0x12FC
#endif

/**
 * @brief Shows the current setting of MAC loopback
 *
 * Algorithm: Display the current MAC loopback setting.
 *
 * @param[in] dev: Device data.
 * @param[in] attr: Device attribute
 * @param[in] buf: Buffer to store the current MAC loopback setting
 *
 * @note MAC and PHY need to be initialized.
 */
static ssize_t ether_mac_loopback_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct net_device *ndev = (struct net_device *)dev_get_drvdata(dev);
	struct ether_priv_data *pdata = netdev_priv(ndev);
	return scnprintf(buf, PAGE_SIZE, "%s\n",
			 (pdata->mac_loopback_mode == 1U) ?
			 "enabled" : "disabled");
}

/**
 * @brief Set the user setting of MAC loopback mode
 *
 * Algorithm: This is used to set the user mode settings of MAC loopback.
 *
 * @param[in] dev: Device data.
 * @param[in] attr: Device attribute
 * @param[in] buf: Buffer which contains the user settings of MAC loopback
 * @param[in] size: size of buffer
 *
 * @note MAC and PHY need to be initialized.
 *
 * @return size of buffer.
 */
static ssize_t ether_mac_loopback_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	struct net_device *ndev = (struct net_device *)dev_get_drvdata(dev);
	struct phy_device *phydev = ndev->phydev;
	struct ether_priv_data *pdata = netdev_priv(ndev);
	int ret = -1;

	/* Interface is not up so LB mode can't be set */
	if (!netif_running(ndev)) {
		dev_err(pdata->dev, "Not Allowed. Ether interface is not up\n");
		return size;
	}

	if (strncmp(buf, "enable", 6) == 0U) {
		if (!phydev->link) {
			/* If no phy link, then turn on carrier explicitly so
			 * that nw stack can send packets. If PHY link is
			 * present, PHY framework would have already taken care
			 * of netif_carrier_* status.
			 */
			netif_carrier_on(ndev);
		}
		/* Enabling the MAC Loopback Mode */
		ret = osi_config_mac_loopback(pdata->osi_core, OSI_ENABLE);
		if (ret < 0) {
			dev_err(pdata->dev, "Enabling MAC Loopback failed\n");
		} else {
			pdata->mac_loopback_mode = 1;
			dev_info(pdata->dev, "Enabling MAC Loopback\n");
		}
	} else if (strncmp(buf, "disable", 7) == 0U) {
		if (!phydev->link) {
			/* If no phy link, then turn off carrier explicitly so
			 * that nw stack doesn't send packets. If PHY link is
			 * present, PHY framework would have already taken care
			 * of netif_carrier_* status.
			 */
			netif_carrier_off(ndev);
		}
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

/**
 * @brief Sysfs attribute for MAC loopback
 *
 */
static DEVICE_ATTR(mac_loopback, (S_IRUGO | S_IWUSR),
		   ether_mac_loopback_show,
		   ether_mac_loopback_store);

/**
 * @brief Shows the current setting of PTP mode
 *
 * Algorithm: Display the current PTP mode setting.
 *
 * @param[in] dev: Device data.
 * @param[in] attr: Device attribute
 * @param[in] buf: Buffer to store the current PTP mode
 *
 * @note MAC and PHY need to be initialized.
 */
static ssize_t ether_ptp_mode_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct net_device *ndev = (struct net_device *)dev_get_drvdata(dev);
	struct ether_priv_data *pdata = netdev_priv(ndev);

	return scnprintf(buf, PAGE_SIZE, "%s\n",
			 ((pdata->osi_dma->ptp_flag & OSI_PTP_SYNC_MASTER) ==
			  OSI_PTP_SYNC_MASTER) ? "master" :
			   ((pdata->osi_dma->ptp_flag & OSI_PTP_SYNC_SLAVE) ==
			    OSI_PTP_SYNC_SLAVE) ? "slave" : " ");
}

/**
 * @brief Set the user setting of PTP mode
 *
 * Algorithm: This is used to set the user mode settings of PTP mode
 * @param[in] dev: Device data.
 * @param[in] attr: Device attribute
 * @param[in] buf: Buffer which contains the user settings of PTP mode
 * @param[in] size: size of buffer
 *
 * @note MAC and PHY need to be initialized.
 *
 * @return size of buffer.
 */
static ssize_t ether_ptp_mode_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t size)
{
	struct net_device *ndev = (struct net_device *)dev_get_drvdata(dev);
	struct ether_priv_data *pdata = netdev_priv(ndev);

	if (!netif_running(ndev)) {
		dev_err(pdata->dev, "Not Allowed. Ether interface is not up\n");
		return size;
	}

	if (strncmp(buf, "master", 6) == 0U) {
		pdata->osi_dma->ptp_flag &= ~(OSI_PTP_SYNC_MASTER |
					      OSI_PTP_SYNC_SLAVE);
		pdata->osi_dma->ptp_flag |= OSI_PTP_SYNC_MASTER;
	} else if (strncmp(buf, "slave", 5) == 0U) {
		pdata->osi_dma->ptp_flag &= ~(OSI_PTP_SYNC_MASTER |
					      OSI_PTP_SYNC_SLAVE);
		pdata->osi_dma->ptp_flag |= OSI_PTP_SYNC_SLAVE;
	} else {
		dev_err(pdata->dev,
			"Invalid entry. Valid Entries are master or slave\n");
	}

	return size;
}

/**
 * @brief Sysfs attribute for PTP MODE
 *
 */
static DEVICE_ATTR(ptp_mode, (S_IRUGO | S_IWUSR),
		   ether_ptp_mode_show,
		   ether_ptp_mode_store);

/**
 * @brief Shows the current setting of PTP sync method
 *
 * Algorithm: Display the current PTP sync method.
 *
 * @param[in] dev: Device data.
 * @param[in] attr: Device attribute
 * @param[in] buf: Buffer to store the current ptp sync method
 *
 * @note MAC and PHY need to be initialized.
 */
static ssize_t ether_ptp_sync_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct net_device *ndev = (struct net_device *)dev_get_drvdata(dev);
	struct ether_priv_data *pdata = netdev_priv(ndev);

	return scnprintf(buf, PAGE_SIZE, "%s\n",
			 ((pdata->osi_dma->ptp_flag & OSI_PTP_SYNC_TWOSTEP) ==
			  OSI_PTP_SYNC_TWOSTEP) ? "twostep" :
			   ((pdata->osi_dma->ptp_flag & OSI_PTP_SYNC_ONESTEP) ==
			    OSI_PTP_SYNC_ONESTEP) ? "onestep" : " ");
}

/**
 * @brief Set the user setting of PTP sync method
 *
 * Algorithm: This is used to set the user mode settings of PTP sync method
 * @param[in] dev: Device data.
 * @param[in] attr: Device attribute
 * @param[in] buf: Buffer which contains the user settings of PTP sync method
 * @param[in] size: size of buffer
 *
 * @note MAC and PHY need to be initialized.
 *
 * @return size of buffer.
 */
static ssize_t ether_ptp_sync_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t size)
{
	struct net_device *ndev = (struct net_device *)dev_get_drvdata(dev);
	struct ether_priv_data *pdata = netdev_priv(ndev);

	if (!netif_running(ndev)) {
		dev_err(pdata->dev, "Not Allowed. Ether interface is not up\n");
		return size;
	}

	if (strncmp(buf, "onestep", 7) == 0U) {
		pdata->osi_dma->ptp_flag &= ~(OSI_PTP_SYNC_ONESTEP |
					      OSI_PTP_SYNC_TWOSTEP);
		pdata->osi_dma->ptp_flag |= OSI_PTP_SYNC_ONESTEP;

	} else if (strncmp(buf, "twostep", 7) == 0U) {
		pdata->osi_dma->ptp_flag &= ~(OSI_PTP_SYNC_ONESTEP |
					      OSI_PTP_SYNC_TWOSTEP);
		pdata->osi_dma->ptp_flag |= OSI_PTP_SYNC_TWOSTEP;
	} else {
		dev_err(pdata->dev,
			"Invalid entry. Valid Entries are onestep or twostep\n");
	}

	return size;
}

/**
 * @brief Sysfs attribute for PTP sync method
 *
 */
static DEVICE_ATTR(ptp_sync, (S_IRUGO | S_IWUSR),
		   ether_ptp_sync_show,
		   ether_ptp_sync_store);

/**
 * @brief Attributes for nvethernet sysfs
 */
static struct attribute *ether_sysfs_attrs[] = {
	&dev_attr_mac_loopback.attr,
	&dev_attr_ptp_mode.attr,
	&dev_attr_ptp_sync.attr,
	NULL
};

/**
 * @brief Ethernet sysfs attribute group
 */
static struct attribute_group ether_attribute_group = {
	.name = "nvethernet",
	.attrs = ether_sysfs_attrs,
};

#ifdef CONFIG_DEBUG_FS
static char *timestamp_system_source(unsigned int source)
{
	switch (source) {
	case 1:
		return "Internal";
	case 2:
		return "External";
	case 3:
		return "Internal and External";
	case 0:
		return "Reserved";
	}

	return "None";
}

static char *active_phy_selected_interface(unsigned int act_phy_sel)
{
	switch (act_phy_sel) {
	case 0:
		return "GMII or MII";
	case 1:
		return "RGMII";
	case 2:
		return "SGMII";
	case 3:
		return "TBI";
	case 4:
		return "RMII";
	case 5:
		return "RTBI";
	case 6:
		return "SMII";
	case 7:
		return "RevMII";
	}

	return "None";
}

static char *mtl_fifo_size(unsigned int fifo_size)
{
	switch (fifo_size) {
	case 0:
		return "128 Bytes";
	case 1:
		return "256 Bytes";
	case 2:
		return "512 Bytes";
	case 3:
		return "1KB";
	case 4:
		return "2KB";
	case 5:
		return "4KB";
	case 6:
		return "8KB";
	case 7:
		return "16KB";
	case 8:
		return "32KB";
	case 9:
		return "64KB";
	case 10:
		return "128KB";
	case 11:
		return "256KB";
	default:
		return "Reserved";
	}
}

static char *address_width(unsigned int val)
{
	switch (val) {
	case 0:
		return "32";
	case 1:
		return "40";
	case 2:
		return "48";
	default:
		return "Reserved";
	}
}

static char *hash_table_size(unsigned int size)
{
	switch (size) {
	case 0:
		return "No Hash Table";
	case 1:
		return "64";
	case 2:
		return "128";
	case 3:
		return "256";
	default:
		return "Invalid size";
	}
}

static char *num_vlan_filters(unsigned int filters)
{
	switch (filters) {
	case 0:
		return "Zero";
	case 1:
		return "4";
	case 2:
		return "8";
	case 3:
		return "16";
	case 4:
		return "24";
	case 5:
		return "32";
	default:
		return "Unknown";
	}
}

static char *max_frp_bytes(unsigned int bytes)
{
	switch (bytes) {
	case 0:
		return "64 Bytes";
	case 1:
		return "128 Bytes";
	case 2:
		return "256 Bytes";
	case 3:
		return "Reserved";
	default:
		return "Invalid";
	}
}

static char *max_frp_instructions(unsigned int entries)
{
	switch (entries) {
	case 0:
		return "64";
	case 1:
		return "128";
	case 2:
		return "256";
	case 3:
		return "Reserved";
	default:
		return "Invalid";
	}
}

static char *auto_safety_package(unsigned int pkg)
{
	switch (pkg) {
	case 0:
		return "No Safety features selected";
	case 1:
		return "Only 'ECC protection for external memory' feature is selected";
	case 2:
		return "All the Automotive Safety features are selected without the 'Parity Port Enable for external interface' feature";
	case 3:
		return "All the Automotive Safety features are selected with the 'Parity Port Enable for external interface' feature";
	default:
		return "Invalid";
	}
}

static char *tts_fifo_depth(unsigned int depth)
{
	switch (depth) {
	case 1:
		return "1";
	case 2:
		return "2";
	case 3:
		return "4";
	case 4:
		return "8";
	case 5:
		return "16";
	default:
		return "Reserved";
	}
}

static char *gate_ctl_depth(unsigned int depth)
{
	switch (depth) {
	case 0:
		return "No Depth Configured";
	case 1:
		return "64";
	case 2:
		return "128";
	case 3:
		return "256";
	case 4:
		return "512";
	case 5:
		return "1024";
	default:
		return "Reserved";
	}
}

static char *gate_ctl_width(unsigned int width)
{
	switch (width) {
	case 0:
		return "Width not configured";
	case 1:
		return "16";
	case 2:
		return "20";
	case 3:
		return "24";
	default:
		return "Invalid";
	}
}

static int ether_hw_features_read(struct seq_file *seq, void *v)
{
	struct net_device *ndev = seq->private;
	struct ether_priv_data *pdata = netdev_priv(ndev);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	struct osi_hw_features *hw_feat = &pdata->hw_feat;

	if (!netif_running(ndev)) {
		dev_err(pdata->dev, "Not Allowed. Ether interface is not up\n");
		return 0;
	}

	seq_printf(seq, "==============================\n");
	seq_printf(seq, "\tHW features\n");
	seq_printf(seq, "==============================\n");

	seq_printf(seq, "\t10/100 Mbps: %s\n",
		   (hw_feat->mii_sel) ? "Y" : "N");
	seq_printf(seq, "\tRGMII Mode: %s\n",
		   (hw_feat->rgmii_sel) ? "Y" : "N");
	seq_printf(seq, "\tRMII Mode: %s\n",
		   (hw_feat->rmii_sel) ? "Y" : "N");
	seq_printf(seq, "\t1000 Mpbs: %s\n",
		   (hw_feat->gmii_sel) ? "Y" : "N");
	seq_printf(seq, "\tHalf duplex support: %s\n",
		   (hw_feat->hd_sel) ? "Y" : "N");
	seq_printf(seq, "\tTBI/SGMII/RTBI PHY interface: %s\n",
		   (hw_feat->pcs_sel) ? "Y" : "N");
	seq_printf(seq, "\tVLAN Hash Filtering: %s\n",
		   (hw_feat->vlan_hash_en) ? "Y" : "N");
	seq_printf(seq, "\tMDIO interface: %s\n",
		   (hw_feat->sma_sel) ? "Y" : "N");
	seq_printf(seq, "\tRemote Wake-Up Packet Detection: %s\n",
		   (hw_feat->rwk_sel) ? "Y" : "N");
	seq_printf(seq, "\tMagic Packet Detection: %s\n",
		   (hw_feat->mgk_sel) ? "Y" : "N");
	seq_printf(seq, "\tMAC Management Counters (MMC): %s\n",
		   (hw_feat->mmc_sel) ? "Y" : "N");
	seq_printf(seq, "\tARP Offload: %s\n",
		   (hw_feat->arp_offld_en) ? "Y" : "N");
	seq_printf(seq, "\tIEEE 1588 Timestamp Support: %s\n",
		   (hw_feat->ts_sel) ? "Y" : "N");
	seq_printf(seq, "\tEnergy Efficient Ethernet (EEE) Support: %s\n",
		   (hw_feat->eee_sel) ? "Y" : "N");
	seq_printf(seq, "\tTransmit TCP/IP Checksum Insertion Support: %s\n",
		   (hw_feat->tx_coe_sel) ? "Y" : "N");
	seq_printf(seq, "\tReceive TCP/IP Checksum Support: %s\n",
		   (hw_feat->rx_coe_sel) ? "Y" : "N");
	seq_printf(seq, "\t (1 - 31) MAC Address registers: %s\n",
		   (hw_feat->mac_addr_sel) ? "Y" : "N");
	seq_printf(seq, "\t(32 - 63) MAC Address Registers: %s\n",
		   (hw_feat->mac_addr32_sel) ? "Y" : "N");
	seq_printf(seq, "\t(64 - 127) MAC Address Registers: %s\n",
		   (hw_feat->mac_addr64_sel) ? "Y" : "N");
	seq_printf(seq, "\tTimestamp System Time Source: %s\n",
		   timestamp_system_source(hw_feat->tsstssel));
	seq_printf(seq, "\tSource Address or VLAN Insertion Enable: %s\n",
		   (hw_feat->sa_vlan_ins) ? "Y" : "N");
	seq_printf(seq, "\tActive PHY selected Interface: %s\n",
		   active_phy_selected_interface(hw_feat->sa_vlan_ins));
	seq_printf(seq, "\tVxLAN/NVGRE Support: %s\n",
		   (hw_feat->vxn) ? "Y" : "N");
	seq_printf(seq, "\tDifferent Descriptor Cache Support: %s\n",
		   (hw_feat->ediffc) ? "Y" : "N");
	seq_printf(seq, "\tEnhanced DMA Support: %s\n",
		   (hw_feat->edma) ? "Y" : "N");
	seq_printf(seq, "\tMTL Receive FIFO Size: %s\n",
		   mtl_fifo_size(hw_feat->rx_fifo_size));
	seq_printf(seq, "\tMTL Transmit FIFO Size: %s\n",
		   mtl_fifo_size(hw_feat->tx_fifo_size));
	seq_printf(seq, "\tPFC Enable: %s\n",
		   (hw_feat->pfc_en) ? "Y" : "N");
	seq_printf(seq, "\tOne-Step Timestamping Support: %s\n",
		   (hw_feat->ost_en) ? "Y" : "N");
	seq_printf(seq, "\tPTP Offload Enable: %s\n",
		   (hw_feat->pto_en) ? "Y" : "N");
	seq_printf(seq, "\tIEEE 1588 High Word Register Enable: %s\n",
		   (hw_feat->adv_ts_hword) ? "Y" : "N");
	seq_printf(seq, "\tAXI Address width: %s\n",
		   address_width(hw_feat->addr_64));
	seq_printf(seq, "\tDCB Feature Support: %s\n",
		   (hw_feat->dcb_en) ? "Y" : "N");
	seq_printf(seq, "\tSplit Header Feature Support: %s\n",
		   (hw_feat->sph_en) ? "Y" : "N");
	seq_printf(seq, "\tTCP Segmentation Offload Support: %s\n",
		   (hw_feat->tso_en) ? "Y" : "N");
	seq_printf(seq, "\tDMA Debug Registers Enable: %s\n",
		   (hw_feat->dma_debug_gen) ? "Y" : "N");
	seq_printf(seq, "\tAV Feature Enable: %s\n",
		   (hw_feat->av_sel) ? "Y" : "N");
	seq_printf(seq, "\tRx Side Only AV Feature Enable: %s\n",
		   (hw_feat->rav_sel) ? "Y" : "N");
	seq_printf(seq, "\tHash Table Size: %s\n",
		   hash_table_size(hw_feat->hash_tbl_sz));
	seq_printf(seq, "\tTotal number of L3 or L4 Filters: %u\n",
		   hw_feat->l3l4_filter_num);
	seq_printf(seq, "\tNumber of MTL Receive Queues: %u\n",
		   (hw_feat->rx_q_cnt + 1));
	seq_printf(seq, "\tNumber of MTL Transmit Queues: %u\n",
		   (hw_feat->tx_q_cnt + 1));
	seq_printf(seq, "\tNumber of Receive DMA channels: %u\n",
		   (hw_feat->rx_ch_cnt + 1));
	seq_printf(seq, "\tNumber of Transmit DMA channels: %u\n",
		   (hw_feat->tx_ch_cnt + 1));
	seq_printf(seq, "\tNumber of PPS outputs: %u\n",
		   hw_feat->pps_out_num);
	seq_printf(seq, "\tNumber of Auxiliary Snapshot Inputs: %u\n",
		   hw_feat->aux_snap_num);
	seq_printf(seq, "\tRSS Feature Enabled: %s\n",
		   (hw_feat->rss_en) ? "Y" : "N");
	seq_printf(seq, "\tNumber of Traffic Classes: %u\n",
		   (hw_feat->num_tc + 1));
	seq_printf(seq, "\tNumber of VLAN filters: %s\n",
		   num_vlan_filters(hw_feat->num_vlan_filters));
	seq_printf(seq,
		   "\tQueue/Channel based VLAN tag insert on Tx Enable: %s\n",
		   (hw_feat->cbti_sel) ? "Y" : "N");
	seq_printf(seq, "\tOne-Step for PTP over UDP/IP Feature Enable: %s\n",
		   (hw_feat->ost_over_udp) ? "Y" : "N");
	seq_printf(seq, "\tDouble VLAN processing support: %s\n",
		   (hw_feat->double_vlan_en) ? "Y" : "N");

	if (osi_core->mac_ver > OSI_EQOS_MAC_5_00) {
		seq_printf(seq, "\tSupported Flexible Receive Parser: %s\n",
			   (hw_feat->frp_sel) ? "Y" : "N");
		seq_printf(seq, "\tNumber of FRP Pipes: %u\n",
			   (hw_feat->num_frp_pipes + 1));
		seq_printf(seq, "\tNumber of FRP Parsable Bytes: %s\n",
			   max_frp_bytes(hw_feat->max_frp_bytes));
		seq_printf(seq, "\tNumber of FRP Instructions: %s\n",
			   max_frp_instructions(hw_feat->max_frp_entries));
		seq_printf(seq, "\tAutomotive Safety Package: %s\n",
			   auto_safety_package(hw_feat->auto_safety_pkg));
		seq_printf(seq, "\tTx Timestamp FIFO Depth: %s\n",
			   tts_fifo_depth(hw_feat->tts_fifo_depth));
		seq_printf(seq, "\tEnhancements to Scheduling Traffic Support: %s\n",
			   (hw_feat->est_sel) ? "Y" : "N");
		seq_printf(seq, "\tDepth of the Gate Control List: %s\n",
			   gate_ctl_depth(hw_feat->gcl_depth));
		seq_printf(seq, "\tWidth of the Time Interval field in GCL: %s\n",
			   gate_ctl_width(hw_feat->gcl_width));
		seq_printf(seq, "\tFrame Preemption Enable: %s\n",
			   (hw_feat->fpe_sel) ? "Y" : "N");
		seq_printf(seq, "\tTime Based Scheduling Enable: %s\n",
			   (hw_feat->tbs_sel) ? "Y" : "N");
		seq_printf(seq, "\tNumber of DMA channels enabled for TBS: %u\n",
			   (hw_feat->num_tbs_ch + 1));
	}

	return 0;
}

static int ether_hw_feat_open(struct inode *inode, struct file *file)
{
	return single_open(file, ether_hw_features_read, inode->i_private);
}

static const struct file_operations ether_hw_features_fops = {
	.owner = THIS_MODULE,
	.open = ether_hw_feat_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int ether_desc_dump_read(struct seq_file *seq, void *v)
{
	struct net_device *ndev = seq->private;
	struct ether_priv_data *pdata = netdev_priv(ndev);
	struct osi_dma_priv_data *osi_dma = pdata->osi_dma;
	unsigned int num_chan = osi_dma->num_dma_chans;
	struct osi_tx_ring *tx_ring = NULL;
	struct osi_rx_ring *rx_ring = NULL;
	struct osi_tx_desc *tx_desc = NULL;
	struct osi_rx_desc *rx_desc = NULL;
	unsigned int chan;
	unsigned int i;
	unsigned int j;

	if (!netif_running(ndev)) {
		dev_err(pdata->dev, "Not Allowed. Ether interface is not up\n");
		return 0;
	}

	for (i = 0; i < num_chan; i++) {
		chan = osi_dma->dma_chans[i];
		tx_ring = osi_dma->tx_ring[chan];
		rx_ring = osi_dma->rx_ring[chan];

		seq_printf(seq, "\n\tDMA Tx channel %u descriptor dump\n",
			   chan);
		seq_printf(seq, "\tcurrent Tx idx = %u, clean idx = %u\n",
			   tx_ring->cur_tx_idx, tx_ring->clean_idx);
		for (j = 0; j < TX_DESC_CNT; j++) {
			tx_desc = tx_ring->tx_desc + j;

			seq_printf(seq, "[%03u %p %#llx] = %#x:%#x:%#x:%#x\n",
				   j, tx_desc, virt_to_phys(tx_desc),
				   tx_desc->tdes3, tx_desc->tdes2,
				   tx_desc->tdes1, tx_desc->tdes0);
		}

		seq_printf(seq, "\n\tDMA Rx channel %u descriptor dump\n",
			   chan);
		seq_printf(seq, "\tcurrent Rx idx = %u, refill idx = %u\n",
			   rx_ring->cur_rx_idx, rx_ring->refill_idx);
		for (j = 0; j < RX_DESC_CNT; j++) {
			rx_desc = rx_ring->rx_desc + j;

			seq_printf(seq, "[%03u %p %#llx] = %#x:%#x:%#x:%#x\n",
				   j, rx_desc, virt_to_phys(rx_desc),
				   rx_desc->rdes3, rx_desc->rdes2,
				   rx_desc->rdes1, rx_desc->rdes0);
		}
	}

	return 0;
}

static int ether_desc_dump_open(struct inode *inode, struct file *file)
{
	return single_open(file, ether_desc_dump_read, inode->i_private);
}

static const struct file_operations ether_desc_dump_fops = {
	.owner = THIS_MODULE,
	.open = ether_desc_dump_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int ether_register_dump_read(struct seq_file *seq, void *v)
{
	struct net_device *ndev = seq->private;
	struct ether_priv_data *pdata = netdev_priv(ndev);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	int max_address = 0x0;
	int start_addr = 0x0;

	max_address = EOQS_MAX_REGISTER_ADDRESS;

	/* Interface is not up so register dump not allowed */
	if (!netif_running(ndev)) {
		dev_err(pdata->dev, "Not Allowed. Ether interface is not up\n");
		return -EBUSY;
	}

	while (1) {
		seq_printf(seq,
			   "\t Register offset 0x%x value 0x%x\n",
			   start_addr,
			   ioread32((void *)osi_core->base + start_addr));
		start_addr += 4;

		if (start_addr > max_address)
			break;
	}

	return 0;
}

static int ether_register_dump_open(struct inode *inode, struct file *file)
{
	return single_open(file, ether_register_dump_read, inode->i_private);
}

static const struct file_operations ether_register_dump_fops = {
	.owner = THIS_MODULE,
	.open = ether_register_dump_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int ether_create_debugfs(struct ether_priv_data *pdata)
{
	char *buf;
	int ret = 0;

	buf = kasprintf(GFP_KERNEL, "nvethernet-%s", pdata->ndev->name);
	if (!buf)
		return -ENOMEM;

	pdata->dbgfs_dir = debugfs_create_dir(buf, NULL);
	if (!pdata->dbgfs_dir || IS_ERR(pdata->dbgfs_dir)) {
		netdev_err(pdata->ndev,
			   "failed to create debugfs directory\n");
		ret = -ENOMEM;
		goto exit;
	}

	pdata->dbgfs_hw_feat = debugfs_create_file("hw_features", S_IRUGO,
						   pdata->dbgfs_dir,
						   pdata->ndev,
						   &ether_hw_features_fops);
	if (!pdata->dbgfs_hw_feat) {
		netdev_err(pdata->ndev,
			   "failed to create HW features debugfs\n");
		debugfs_remove_recursive(pdata->dbgfs_dir);
		ret = -ENOMEM;
		goto exit;
	}

	pdata->dbgfs_desc_dump = debugfs_create_file("descriptors_dump",
						     S_IRUGO,
						     pdata->dbgfs_dir,
						     pdata->ndev,
						     &ether_desc_dump_fops);
	if (!pdata->dbgfs_desc_dump) {
		netdev_err(pdata->ndev,
			   "failed to create descriptor dump debugfs\n");
		debugfs_remove_recursive(pdata->dbgfs_dir);
		ret = -ENOMEM;
		goto exit;
	}

	pdata->dbgfs_reg_dump = debugfs_create_file("register_dump", S_IRUGO,
						    pdata->dbgfs_dir,
						    pdata->ndev,
						    &ether_register_dump_fops);
	if (!pdata->dbgfs_reg_dump) {
		netdev_err(pdata->ndev,
			   "failed to create rgister dump debugfs\n");
		debugfs_remove_recursive(pdata->dbgfs_dir);
		ret = -ENOMEM;
		goto exit;
	}

exit:
	kfree(buf);
	return ret;
}

static void ether_remove_debugfs(struct ether_priv_data *pdata)
{
	debugfs_remove_recursive(pdata->dbgfs_dir);
}
#endif /* CONFIG_DEBUG_FS */

int ether_sysfs_register(struct ether_priv_data *pdata)
{
	struct device *dev = pdata->dev;
	int ret = 0;

#ifdef CONFIG_DEBUG_FS
	ret = ether_create_debugfs(pdata);
	if (ret < 0)
		return ret;
#endif
	/* Create nvethernet sysfs group under /sys/devices/<ether_device>/ */
	return sysfs_create_group(&dev->kobj, &ether_attribute_group);
}

void ether_sysfs_unregister(struct ether_priv_data *pdata)
{
	struct device *dev = pdata->dev;

#ifdef CONFIG_DEBUG_FS
	ether_remove_debugfs(pdata);
#endif
	/* Remove nvethernet sysfs group under /sys/devices/<ether_device>/ */
	sysfs_remove_group(&dev->kobj, &ether_attribute_group);
}
