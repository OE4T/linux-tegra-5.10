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

#define OSI_ARRAY_SIZE(x)  ((int)sizeof((x)) / (int)sizeof((x)[0]))
#define EQOS_MMC_STATS_LEN OSI_ARRAY_SIZE(eqos_mmc)

struct eqos_stats {
	char stat_string[ETH_GSTRING_LEN];
	size_t sizeof_stat;
	size_t stat_offset;
};

/* DMA extra status */
/* Structure variable name MUST up to MAX length of ETH_GSTRING_LEN */
#define EQOS_DMA_EXTRA_STAT(a) \
{ (#a), FIELD_SIZEOF(struct osi_xtra_dma_stat_counters, a), \
	offsetof(struct osi_dma_priv_data, dstats.a)}

static const struct eqos_stats eqos_dstrings_stats[] = {
	EQOS_DMA_EXTRA_STAT(tx_clean_n[0]),
	EQOS_DMA_EXTRA_STAT(tx_clean_n[1]),
	EQOS_DMA_EXTRA_STAT(tx_clean_n[2]),
	EQOS_DMA_EXTRA_STAT(tx_clean_n[3]),

	/* Tx/Rx frames */
	EQOS_DMA_EXTRA_STAT(tx_pkt_n),
	EQOS_DMA_EXTRA_STAT(rx_pkt_n),
	EQOS_DMA_EXTRA_STAT(tx_vlan_pkt_n),
	EQOS_DMA_EXTRA_STAT(rx_vlan_pkt_n),
	EQOS_DMA_EXTRA_STAT(tx_tso_pkt_n),

	/* Tx/Rx frames per channels/queues */
	EQOS_DMA_EXTRA_STAT(q_tx_pkt_n[0]),
	EQOS_DMA_EXTRA_STAT(q_tx_pkt_n[1]),
	EQOS_DMA_EXTRA_STAT(q_tx_pkt_n[2]),
	EQOS_DMA_EXTRA_STAT(q_tx_pkt_n[3]),
	EQOS_DMA_EXTRA_STAT(q_rx_pkt_n[0]),
	EQOS_DMA_EXTRA_STAT(q_rx_pkt_n[1]),
	EQOS_DMA_EXTRA_STAT(q_rx_pkt_n[2]),
	EQOS_DMA_EXTRA_STAT(q_rx_pkt_n[3]),
};

#define EQOS_EXTRA_DMA_STAT_LEN OSI_ARRAY_SIZE(eqos_dstrings_stats)

/* core extra status */
/* Structure variable name MUST up to MAX length of ETH_GSTRING_LEN */
#define EQOS_EXTRA_STAT(b) \
{ #b, FIELD_SIZEOF(struct osi_xtra_stat_counters, b), \
	offsetof(struct osi_core_priv_data, xstats.b)}

static const struct eqos_stats eqos_gstrings_stats[] = {
	EQOS_EXTRA_STAT(re_alloc_rxbuf_failed[0]),
	EQOS_EXTRA_STAT(re_alloc_rxbuf_failed[1]),
	EQOS_EXTRA_STAT(re_alloc_rxbuf_failed[2]),
	EQOS_EXTRA_STAT(re_alloc_rxbuf_failed[3]),

	/* Tx/Rx IRQ error info */
	EQOS_EXTRA_STAT(tx_proc_stopped_irq_n[0]),
	EQOS_EXTRA_STAT(tx_proc_stopped_irq_n[1]),
	EQOS_EXTRA_STAT(tx_proc_stopped_irq_n[2]),
	EQOS_EXTRA_STAT(tx_proc_stopped_irq_n[3]),
	EQOS_EXTRA_STAT(rx_proc_stopped_irq_n[0]),
	EQOS_EXTRA_STAT(rx_proc_stopped_irq_n[1]),
	EQOS_EXTRA_STAT(rx_proc_stopped_irq_n[2]),
	EQOS_EXTRA_STAT(rx_proc_stopped_irq_n[3]),
	EQOS_EXTRA_STAT(tx_buf_unavail_irq_n[0]),
	EQOS_EXTRA_STAT(tx_buf_unavail_irq_n[1]),
	EQOS_EXTRA_STAT(tx_buf_unavail_irq_n[2]),
	EQOS_EXTRA_STAT(tx_buf_unavail_irq_n[3]),
	EQOS_EXTRA_STAT(rx_buf_unavail_irq_n[0]),
	EQOS_EXTRA_STAT(rx_buf_unavail_irq_n[1]),
	EQOS_EXTRA_STAT(rx_buf_unavail_irq_n[2]),
	EQOS_EXTRA_STAT(rx_buf_unavail_irq_n[3]),
	EQOS_EXTRA_STAT(rx_watchdog_irq_n),
	EQOS_EXTRA_STAT(fatal_bus_error_irq_n),

	/* Tx/Rx IRQ Events */
	EQOS_EXTRA_STAT(tx_normal_irq_n[0]),
	EQOS_EXTRA_STAT(tx_normal_irq_n[1]),
	EQOS_EXTRA_STAT(tx_normal_irq_n[2]),
	EQOS_EXTRA_STAT(tx_normal_irq_n[3]),
	EQOS_EXTRA_STAT(rx_normal_irq_n[0]),
	EQOS_EXTRA_STAT(rx_normal_irq_n[1]),
	EQOS_EXTRA_STAT(rx_normal_irq_n[2]),
	EQOS_EXTRA_STAT(rx_normal_irq_n[3]),
	EQOS_EXTRA_STAT(link_disconnect_count),
	EQOS_EXTRA_STAT(link_connect_count),
};

#define EQOS_EXTRA_STAT_LEN OSI_ARRAY_SIZE(eqos_gstrings_stats)

/* HW MAC Management counters */
/* Structure variable name MUST up to MAX length of ETH_GSTRING_LEN */
#define EQOS_MMC_STAT(c) \
{ #c, FIELD_SIZEOF(struct osi_mmc_counters, c), \
	offsetof(struct osi_core_priv_data, mmc.c)}

static const struct eqos_stats eqos_mmc[] = {
	/* MMC TX counters */
	EQOS_MMC_STAT(mmc_tx_octetcount_gb),
	EQOS_MMC_STAT(mmc_tx_framecount_gb),
	EQOS_MMC_STAT(mmc_tx_broadcastframe_g),
	EQOS_MMC_STAT(mmc_tx_multicastframe_g),
	EQOS_MMC_STAT(mmc_tx_64_octets_gb),
	EQOS_MMC_STAT(mmc_tx_65_to_127_octets_gb),
	EQOS_MMC_STAT(mmc_tx_128_to_255_octets_gb),
	EQOS_MMC_STAT(mmc_tx_256_to_511_octets_gb),
	EQOS_MMC_STAT(mmc_tx_512_to_1023_octets_gb),
	EQOS_MMC_STAT(mmc_tx_1024_to_max_octets_gb),
	EQOS_MMC_STAT(mmc_tx_unicast_gb),
	EQOS_MMC_STAT(mmc_tx_multicast_gb),
	EQOS_MMC_STAT(mmc_tx_broadcast_gb),
	EQOS_MMC_STAT(mmc_tx_underflow_error),
	EQOS_MMC_STAT(mmc_tx_singlecol_g),
	EQOS_MMC_STAT(mmc_tx_multicol_g),
	EQOS_MMC_STAT(mmc_tx_deferred),
	EQOS_MMC_STAT(mmc_tx_latecol),
	EQOS_MMC_STAT(mmc_tx_exesscol),
	EQOS_MMC_STAT(mmc_tx_carrier_error),
	EQOS_MMC_STAT(mmc_tx_octetcount_g),
	EQOS_MMC_STAT(mmc_tx_framecount_g),
	EQOS_MMC_STAT(mmc_tx_excessdef),
	EQOS_MMC_STAT(mmc_tx_pause_frame),
	EQOS_MMC_STAT(mmc_tx_vlan_frame_g),

	/* MMC RX counters */
	EQOS_MMC_STAT(mmc_rx_framecount_gb),
	EQOS_MMC_STAT(mmc_rx_octetcount_gb),
	EQOS_MMC_STAT(mmc_rx_octetcount_g),
	EQOS_MMC_STAT(mmc_rx_broadcastframe_g),
	EQOS_MMC_STAT(mmc_rx_multicastframe_g),
	EQOS_MMC_STAT(mmc_rx_crc_error),
	EQOS_MMC_STAT(mmc_rx_align_error),
	EQOS_MMC_STAT(mmc_rx_runt_error),
	EQOS_MMC_STAT(mmc_rx_jabber_error),
	EQOS_MMC_STAT(mmc_rx_undersize_g),
	EQOS_MMC_STAT(mmc_rx_oversize_g),
	EQOS_MMC_STAT(mmc_rx_64_octets_gb),
	EQOS_MMC_STAT(mmc_rx_65_to_127_octets_gb),
	EQOS_MMC_STAT(mmc_rx_128_to_255_octets_gb),
	EQOS_MMC_STAT(mmc_rx_256_to_511_octets_gb),
	EQOS_MMC_STAT(mmc_rx_512_to_1023_octets_gb),
	EQOS_MMC_STAT(mmc_rx_1024_to_max_octets_gb),
	EQOS_MMC_STAT(mmc_rx_unicast_g),
	EQOS_MMC_STAT(mmc_rx_length_error),
	EQOS_MMC_STAT(mmc_rx_outofrangetype),
	EQOS_MMC_STAT(mmc_rx_pause_frames),
	EQOS_MMC_STAT(mmc_rx_fifo_overflow),
	EQOS_MMC_STAT(mmc_rx_vlan_frames_gb),
	EQOS_MMC_STAT(mmc_rx_watchdog_error),

	/* IPv4 */
	EQOS_MMC_STAT(mmc_rx_ipv4_gd),
	EQOS_MMC_STAT(mmc_rx_ipv4_hderr),
	EQOS_MMC_STAT(mmc_rx_ipv4_nopay),
	EQOS_MMC_STAT(mmc_rx_ipv4_frag),
	EQOS_MMC_STAT(mmc_rx_ipv4_udsbl),

	/* IPV6 */
	EQOS_MMC_STAT(mmc_rx_ipv6_gd_octets),
	EQOS_MMC_STAT(mmc_rx_ipv6_hderr_octets),
	EQOS_MMC_STAT(mmc_rx_ipv6_nopay_octets),

	/* Protocols */
	EQOS_MMC_STAT(mmc_rx_udp_gd),
	EQOS_MMC_STAT(mmc_rx_udp_err),
	EQOS_MMC_STAT(mmc_rx_tcp_gd),
	EQOS_MMC_STAT(mmc_rx_tcp_err),
	EQOS_MMC_STAT(mmc_rx_icmp_gd),
	EQOS_MMC_STAT(mmc_rx_icmp_err),

	/* IPv4 */
	EQOS_MMC_STAT(mmc_rx_ipv4_gd_octets),
	EQOS_MMC_STAT(mmc_rx_ipv4_hderr_octets),
	EQOS_MMC_STAT(mmc_rx_ipv4_nopay_octets),
	EQOS_MMC_STAT(mmc_rx_ipv4_frag_octets),
	EQOS_MMC_STAT(mmc_rx_ipv4_udsbl_octets),

	/* IPV6 */
	EQOS_MMC_STAT(mmc_rx_ipv6_gd),
	EQOS_MMC_STAT(mmc_rx_ipv6_hderr),
	EQOS_MMC_STAT(mmc_rx_ipv6_nopay),

	/* Protocols */
	EQOS_MMC_STAT(mmc_rx_udp_gd_octets),
	EQOS_MMC_STAT(mmc_rx_udp_err_octets),
	EQOS_MMC_STAT(mmc_rx_tcp_gd_octets),
	EQOS_MMC_STAT(mmc_rx_tcp_err_octets),
	EQOS_MMC_STAT(mmc_rx_icmp_gd_octets),
	EQOS_MMC_STAT(mmc_rx_icmp_err_octets),
};

/**
 *	ether_get_ethtool_stats: This function is invoked by kernel when user
 *	requests to get the extended statistics about the device.
 *
 *	@dev: pointer to net device structure.
 *	@dummy: dummy parameter of ethtool_stats type.
 *	@data: Pointer in which MMC statistics should be put.
 *
 *	Algorithm: read mmc register and create sctrings
 *
 *	Dependencies: Network device needs to created.
 *
 *	Protection: None.
 *
 *	Return: void
 */
static void ether_get_ethtool_stats(struct net_device *dev,
				    struct ethtool_stats *dummy,
				    u64 *data)
{
	struct ether_priv_data *pdata = netdev_priv(dev);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	struct osi_dma_priv_data *osi_dma = pdata->osi_dma;
	int i, j = 0;
	int ret;

	if (pdata->hw_feat.mmc_sel == 1U) {
		ret = osi_read_mmc(osi_core);
		if (ret == -1) {
			dev_err(pdata->dev, "Error in reading MMC counter\n");
			return;
		}

		for (i = 0; i < EQOS_MMC_STATS_LEN; i++) {
			char *p = (char *)osi_core + eqos_mmc[i].stat_offset;

			data[j++] = (eqos_mmc[i].sizeof_stat ==
					sizeof(u64)) ? (*(u64 *)p) :
				     (*(u32 *)p);
		}

		for (i = 0; i < EQOS_EXTRA_STAT_LEN; i++) {
			char *p = (char *)osi_core +
				  eqos_gstrings_stats[i].stat_offset;

			data[j++] = (eqos_gstrings_stats[i].sizeof_stat ==
				     sizeof(u64)) ? (*(u64 *)p) : (*(u32 *)p);
		}

		for (i = 0; i < EQOS_EXTRA_DMA_STAT_LEN; i++) {
			char *p = (char *)osi_dma +
				  eqos_dstrings_stats[i].stat_offset;

			data[j++] = (eqos_dstrings_stats[i].sizeof_stat ==
				     sizeof(u64)) ? (*(u64 *)p) : (*(u32 *)p);
		}
	}
}

/**
 *	ether_get_sset_count- This function gets number of strings that
 *	@get_strings will write.
 *
 *	@dev: Pointer to net device structure.
 *	@sset: String set value.
 *
 *	Algorithm: retrun number of strings.
 *
 *	Dependencies: Network device needs to created.
 *
 *	Protection: None.
 *
 *	Return: Numbers of strings(total length)
 */
static int ether_get_sset_count(struct net_device *dev, int sset)
{
	struct ether_priv_data *pdata = netdev_priv(dev);
	int len = 0;

	if (sset == ETH_SS_STATS) {
		if (pdata->hw_feat.mmc_sel == OSI_ENABLE) {
			if (INT_MAX < EQOS_MMC_STATS_LEN) {
				/* do nothing*/
			} else {
				len = EQOS_MMC_STATS_LEN;
			}
		}
		if (INT_MAX - EQOS_EXTRA_STAT_LEN < len) {
			/* do nothing */
		} else {
			len += EQOS_EXTRA_STAT_LEN;
		}
		if (INT_MAX - EQOS_EXTRA_DMA_STAT_LEN < len) {
			/* do nothing */
		} else {
			len += EQOS_EXTRA_DMA_STAT_LEN;
		}
	} else {
		len = -EOPNOTSUPP;
	}

	return len;
}

/**	ether_get_strings - This function returns a set of strings that describe
 *	the requested objects.
 *
 *	@dev: Pointer to net device structure.
 *	@stringset:  String set value.
 *	@data: Pointer in which requested string should be put.
 *
 *	Algorithm: retrun number of strings.
 *
 *	Dependencies: Network device needs to created.
 *
 *	Protection: None.
 *
 *	Return: void
 */
static void ether_get_strings(struct net_device *dev, u32 stringset, u8 *data)
{
	struct ether_priv_data *pdata = netdev_priv(dev);
	u8 *p = data;
	u8 *str;
	int i;

	if (stringset == (u32)ETH_SS_STATS) {
		if (pdata->hw_feat.mmc_sel == OSI_ENABLE) {
			for (i = 0; i < EQOS_MMC_STATS_LEN; i++) {
				str = (u8 *)eqos_mmc[i].stat_string;
				if (memcpy(p, str, ETH_GSTRING_LEN) ==
				    OSI_NULL) {
					return;
				}
				p += ETH_GSTRING_LEN;
			}

			for (i = 0; i < EQOS_EXTRA_STAT_LEN; i++) {
				str = (u8 *)eqos_gstrings_stats[i].stat_string;
				if (memcpy(p, str, ETH_GSTRING_LEN) ==
				    OSI_NULL) {
					return;
				}
				p += ETH_GSTRING_LEN;
			}
			for (i = 0; i < EQOS_EXTRA_DMA_STAT_LEN; i++) {
				str = (u8 *)eqos_dstrings_stats[i].stat_string;
				if (memcpy(p, str, ETH_GSTRING_LEN) ==
				    OSI_NULL) {
					return;
				}
				p += ETH_GSTRING_LEN;
			}
		}
	} else {
		dev_err(pdata->dev, "%s() Unsupported stringset\n", __func__);
	}
}

/**
 *	ether_get_pauseparam - Get pause frame settings
 *	@ndev: network device instance
 *	@pause: Pause parameters that are set currently
 *
 *	Algorithm: Gets pause frame configuration
 *
 *	Dependencies: Network device needs to created.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
static void ether_get_pauseparam(struct net_device *ndev,
				 struct ethtool_pauseparam *pause)
{
	struct ether_priv_data *pdata = netdev_priv(ndev);
	struct phy_device *phydev = pdata->phydev;

	if (!netif_running(ndev)) {
		netdev_err(pdata->ndev, "interface must be up\n");
		return;
	}

	/* return if pause frame is not supported */
	if ((pdata->osi_core->pause_frames == OSI_PAUSE_FRAMES_DISABLE) ||
	    (!(phydev->supported & SUPPORTED_Pause) ||
	    !(phydev->supported & SUPPORTED_Asym_Pause))) {
		dev_err(pdata->dev, "FLOW control not supported\n");
		return;
	}

	/* update auto negotiation */
	pause->autoneg = phydev->autoneg;

	/* update rx pause parameter */
	if ((pdata->osi_core->flow_ctrl & OSI_FLOW_CTRL_RX) ==
	    OSI_FLOW_CTRL_RX) {
		pause->rx_pause = 1;
	}

	/* update tx pause parameter */
	if ((pdata->osi_core->flow_ctrl & OSI_FLOW_CTRL_TX) ==
	    OSI_FLOW_CTRL_TX) {
		pause->tx_pause = 1;
	}
}

/**
 *	ether_set_pauseparam - Set pause frame settings
 *	@ndev: network device instance
 *	@pause: Pause frame settings
 *
 *	Algorithm: Sets pause frame settings
 *
 *	Dependencies: Network device needs to created.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, negative value - failure.
 */
static int ether_set_pauseparam(struct net_device *ndev,
				struct ethtool_pauseparam *pause)
{
	struct ether_priv_data *pdata = netdev_priv(ndev);
	struct phy_device *phydev = pdata->phydev;
	int curflow_ctrl = OSI_FLOW_CTRL_DISABLE;
	int ret;

	if (!netif_running(ndev)) {
		netdev_err(pdata->ndev, "interface must be up\n");
		return -EINVAL;
	}

	/* return if pause frame is not supported */
	if ((pdata->osi_core->pause_frames == OSI_PAUSE_FRAMES_DISABLE) ||
	    (!(phydev->supported & SUPPORTED_Pause) ||
	    !(phydev->supported & SUPPORTED_Asym_Pause))) {
		dev_err(pdata->dev, "FLOW control not supported\n");
		return -EOPNOTSUPP;
	}

	dev_err(pdata->dev, "autoneg = %d tx_pause = %d rx_pause = %d\n",
		pause->autoneg, pause->tx_pause, pause->rx_pause);

	if (pause->tx_pause)
		curflow_ctrl |= OSI_FLOW_CTRL_TX;

	if (pause->rx_pause)
		curflow_ctrl |= OSI_FLOW_CTRL_RX;

	/* update flow control setting */
	pdata->osi_core->flow_ctrl = curflow_ctrl;
	/* update autonegotiation */
	phydev->autoneg = pause->autoneg;

	/*If autonegotiation is enabled,start auto-negotiation
	 * for this PHY device and return, so that flow control
	 * settings will be done once we receive the link changed
	 * event i.e in ether_adjust_link
	 */
	if (phydev->autoneg && netif_running(ndev)) {
		return phy_start_aneg(phydev);
	}

	/* Configure current flow control settings */
	ret = osi_configure_flow_control(pdata->osi_core,
					 pdata->osi_core->flow_ctrl);

	return ret;
}

/**
 *	ether_get_ts_info: Get HW supported time stamping.
 *	@net: Net device data.
 *	@info: Holds device supported timestamping types
 *
 *	Algorithm: Function used to query the PTP capabilities for given netdev.
 *
 *	Dependencies: HW need to support PTP functionality.
 *
 *	Protection: None.
 *
 *	Return: Return can't be a -ve value.
 */
static int ether_get_ts_info(struct net_device *net,
		struct ethtool_ts_info *info)
{
	info->so_timestamping = SOF_TIMESTAMPING_TX_HARDWARE |
				SOF_TIMESTAMPING_RX_HARDWARE |
				SOF_TIMESTAMPING_TX_SOFTWARE |
				SOF_TIMESTAMPING_RX_SOFTWARE |
				SOF_TIMESTAMPING_RAW_HARDWARE |
				SOF_TIMESTAMPING_SOFTWARE;
	info->phc_index = 0;

	info->tx_types = (1 << HWTSTAMP_TX_OFF) | (1 << HWTSTAMP_TX_ON);

	info->rx_filters |= ((1 << HWTSTAMP_FILTER_PTP_V1_L4_SYNC) |
			     (1 << HWTSTAMP_FILTER_PTP_V2_L2_SYNC) |
			     (1 << HWTSTAMP_FILTER_PTP_V2_L4_SYNC) |
			     (1 << HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ) |
			     (1 << HWTSTAMP_FILTER_PTP_V2_L2_DELAY_REQ) |
			     (1 << HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ) |
			     (1 << HWTSTAMP_FILTER_PTP_V2_EVENT) |
			     (1 << HWTSTAMP_FILTER_NONE));

	return 0;
}

/**
 *	ether_set_coalesce: Set interrupt coalescing parameters.
 *	@dev: Net device data.
 *	@ec: pointer to ethtool_coalesce structure
 *
 *	Algorithm: This function is invoked by kernel when user request to set
 *	interrupt coalescing parameters. This driver maintains same coalescing
 *	parameters for all the channels, hence same changes will be applied to
 *	all the channels.
 *
 *	Dependencies:
 *	1) Interface need to be bring down for setting these parameters
 *
 *	Protection: None.
 *
 *	Return: 0 on success and -ve number on failure.
 */
static int ether_set_coalesce(struct net_device *dev,
			      struct ethtool_coalesce *ec)
{
	struct ether_priv_data *pdata = netdev_priv(dev);
	struct osi_dma_priv_data *osi_dma = pdata->osi_dma;

	if (netif_running(dev)) {
		netdev_err(dev, "Coalesce parameters can be changed"
			   " only if interface is down\n");
		return -EINVAL;
	}

	/* Check for not supported parameters  */
	if ((ec->rx_coalesce_usecs_irq) ||
	    (ec->rx_max_coalesced_frames_irq) || (ec->tx_coalesce_usecs_irq) ||
	    (ec->use_adaptive_rx_coalesce) || (ec->use_adaptive_tx_coalesce) ||
	    (ec->pkt_rate_low) || (ec->rx_coalesce_usecs_low) ||
	    (ec->rx_max_coalesced_frames_low) || (ec->tx_coalesce_usecs_high) ||
	    (ec->tx_max_coalesced_frames_low) || (ec->pkt_rate_high) ||
	    (ec->tx_coalesce_usecs_low) || (ec->rx_coalesce_usecs_high) ||
	    (ec->rx_max_coalesced_frames_high) ||
	    (ec->tx_max_coalesced_frames_irq)  ||
	    (ec->stats_block_coalesce_usecs)   ||
	    (ec->tx_max_coalesced_frames_high) || (ec->rate_sample_interval) ||
	    (ec->tx_coalesce_usecs) || (ec->tx_max_coalesced_frames) ||
	    (ec->rx_max_coalesced_frames)) {
		return -EOPNOTSUPP;
	}

	if (ec->rx_coalesce_usecs == OSI_DISABLE) {
		osi_dma->use_riwt = OSI_DISABLE;
	} else if ((ec->rx_coalesce_usecs > OSI_MAX_RX_COALESCE_USEC) ||
		   (ec->rx_coalesce_usecs < OSI_MIN_RX_COALESCE_USEC)) {
		netdev_err(dev,
			   "invalid rx_usecs, must be in a range of"
			   " %d to %d usec\n", OSI_MIN_RX_COALESCE_USEC,
			   OSI_MAX_RX_COALESCE_USEC);
		return -EINVAL;
	} else {
		osi_dma->use_riwt = OSI_ENABLE;
	}

	netdev_err(dev, "RX COALESCING is %s\n", osi_dma->use_riwt ? "ENABLED" :
		   "DISABLED");

	osi_dma->rx_riwt = ec->rx_coalesce_usecs;

	return 0;
}

/**
 *	ether_get_coalesce: Set interrupt coalescing parameters.
 *	@dev: Net device data.
 *	@ec: pointer to ethtool_coalesce structure
 *
 *	Algorithm: This function is invoked by kernel when user request to get
 *	interrupt coalescing parameters. As coalescing parameters are same
 *	for all the channels, so this function will get coalescing
 *	details from channel zero and return.
 *
 *	Dependencies:
 *	1) MAC and PHY need to be initialized.
 *
 *	Protection: None.
 *
 *	Return: 0.
 */
static int ether_get_coalesce(struct net_device *dev,
			      struct ethtool_coalesce *ec)
{
	struct ether_priv_data *pdata = netdev_priv(dev);
	struct osi_dma_priv_data *osi_dma = pdata->osi_dma;

	memset(ec, 0, sizeof(struct ethtool_coalesce));
	ec->rx_coalesce_usecs = osi_dma->rx_riwt;

	return 0;
}

static const struct ethtool_ops ether_ethtool_ops = {
	.get_link = ethtool_op_get_link,
	.get_link_ksettings = phy_ethtool_get_link_ksettings,
	.set_link_ksettings = phy_ethtool_set_link_ksettings,
	.get_pauseparam = ether_get_pauseparam,
	.set_pauseparam = ether_set_pauseparam,
	.get_ts_info = ether_get_ts_info,
	.get_strings = ether_get_strings,
	.get_ethtool_stats = ether_get_ethtool_stats,
	.get_sset_count = ether_get_sset_count,
	.get_coalesce = ether_get_coalesce,
	.set_coalesce = ether_set_coalesce,
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
