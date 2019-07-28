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

/**
 *	ether_adjust_time: Adjust hardware time
 *	@ptp: Pointer to ptp_clock_info structure.
 * 	@delta: Desired change in nanoseconds.
 *
 *	Algorithm: This function is used to shift/adjust the time of the
 *	hardware clock.
 *
 *	Dependencies:
 *
 *	Protection: None.
 *
 *	Return: 0 - success, negative value - failure.
 */
static int ether_adjust_time(struct ptp_clock_info *ptp, s64 delta)
{
	struct ether_priv_data *pdata = container_of(ptp,
						     struct ether_priv_data,
						     ptp_clock_ops);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	int ret = -1;

	ret = osi_adjust_time(osi_core, delta);
	if (ret < 0) {
		dev_err(pdata->dev,
			"%s:failed to adjust time with reason %d\n",
			__func__, ret);
	}

	return ret;
}

/**
 *	ether_adjust_freq: Adjust hardware time
 *	@ptp: Pointer to ptp_clock_info structure.
 *	@ppb: Desired period change in parts per billion.
 *
 *	Algorithm: This function is used to adjust the frequency of the
 *	hardware clock.
 *
 *	Dependencies:
 *
 *	Protection: None.
 *
 *	Return: 0 - success, negative value - failure.
 */
static int ether_adjust_freq(struct ptp_clock_info *ptp, s32 ppb)
{
	struct ether_priv_data *pdata = container_of(ptp,
						     struct ether_priv_data,
						     ptp_clock_ops);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	int ret = -1;

	ret = osi_adjust_freq(osi_core, ppb);
	if (ret < 0) {
		dev_err(pdata->dev,
			"%s:failed to adjust frequency with reason code %d\n",
			__func__, ret);
	}

	return ret;
}

/**
 *	ether_get_time: Get current time
 *	@ptp: Pointer to ptp_clock_info structure.
 *	@ts: Pointer to hole time.
 *
 *	Algorithm: This function is used to read the current time from the
 *	hardware clock
 *
 *	Dependencies:
 *
 *	Protection: None.
 *
 *	Return: 0 - success, negative value - failure.
 */
static int ether_get_time(struct ptp_clock_info *ptp, struct timespec *ts)
{
	struct ether_priv_data *pdata = container_of(ptp,
						     struct ether_priv_data,
						     ptp_clock_ops);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	unsigned int sec, nsec;

	osi_get_systime_from_mac(osi_core, &sec, &nsec);

	ts->tv_sec = sec;
	ts->tv_nsec = nsec;

	return 0;
}

/**
 *	ether_set_time: Set current time
 *	@ptp: Pointer to ptp_clock_info structure.
 *	@ts: Time value to set.
 *
 *	Algorithm: This function is used to set the current time to the
 *	hardware clock.
 *
 *	Dependencies:
 *
 *	Protection: None.
 *
 *	Return: 0 - success, negative value - failure.
 */
static int ether_set_time(struct ptp_clock_info *ptp,
		const struct timespec *ts)
{
	struct ether_priv_data *pdata = container_of(ptp,
						     struct ether_priv_data,
						     ptp_clock_ops);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	int ret = -1;

	ret = osi_set_systime_to_mac(osi_core, ts->tv_sec, ts->tv_nsec);
	if (ret < 0) {
		dev_err(pdata->dev,
			"%s:failed to set system time with reason %d\n",
			__func__, ret);
	}

	return ret;
}

/* structure describing a PTP hardware clock */
static struct ptp_clock_info ether_ptp_clock_ops = {
	.owner = THIS_MODULE,
	.name = "ether_ptp_clk",
	.max_adj = OSI_ETHER_SYSCLOCK,
	.n_alarm = 0,
	.n_ext_ts = 0,
	.n_per_out = 0,
	.pps = 0,
	.adjfreq = ether_adjust_freq,
	.adjtime = ether_adjust_time,
	.gettime64 = ether_get_time,
	.settime64 = ether_set_time,
};

/**
 *	ether_ptp_init: Function to register ptp clock driver.
 *	@pdata:	Pointer to private data structure.
 *
 *	Algorithm: This function is used to register the ptp clock
 *	driver to kernel.
 *
 *	Dependencies: Ethernet driver probe need to be completed successfully
 *	with ethernet network device created.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, negative value - failure.
 */
int ether_ptp_init(struct ether_priv_data *pdata)
{
	if (pdata->hw_feat.tsstssel == OSI_DISABLE) {
		pdata->ptp_clock = NULL;
		dev_err(pdata->dev, "No PTP supports in HW\n"
			"Aborting PTP clock driver registration\n");
		return -1;
	}

	pdata->ptp_clock_ops = ether_ptp_clock_ops;
	pdata->ptp_clock = ptp_clock_register(&pdata->ptp_clock_ops,
					      pdata->dev);
	if (IS_ERR(pdata->ptp_clock)) {
		pdata->ptp_clock = NULL;
		dev_err(pdata->dev, "Fail to register PTP clock\n");
		return -1;
	}

	/* By default enable nano second accuracy */
	pdata->osi_core->ptp_config.one_nsec_accuracy = OSI_ENABLE;

	return 0;
}

/**
 *	ether_ptp_remove: Function to de register ptp clock driver.
 *	@pdata:	Pointer to private data structure.
 *
 *	Algorithm: This function is used to de register the ptp clock
 *
 *	Dependencies: PTP clock driver need to be sucessfully registered during
 *	initialization
 *
 *	Protection: None.
 *
 *	Return: None.
 */
void ether_ptp_remove(struct ether_priv_data *pdata)
{
	if (pdata->ptp_clock) {
		ptp_clock_unregister(pdata->ptp_clock);
	}
}

/**
 *	ether_handle_hwtstamp_ioctl: Function to handle PTP settings.
 *	@pdata:	Pointer to private data structure.
 *	@ifr:	Interface request structure used for socket ioctl
 *
 *	Algorithm: This function is used to handle the hardware PTP settings.
 *
 *	Dependencies: PTP clock driver need to be sucessfully registered during
 *	initialization and HW need to support PTP functionality.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
int ether_handle_hwtstamp_ioctl(struct ether_priv_data *pdata,
		struct ifreq *ifr)
{
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	struct hwtstamp_config config;
	unsigned int hwts_rx_en = 1;
	struct timespec now;

	if (pdata->hw_feat.tsstssel == OSI_DISABLE) {
		dev_info(pdata->dev, "HW timestamping not available\n");
		return -EOPNOTSUPP;
	}

	if (copy_from_user(&config, ifr->ifr_data,
			   sizeof(struct hwtstamp_config))) {
		return -EFAULT;
	}

	dev_info(pdata->dev, "config.flags = %#x, tx_type = %#x,"
		 "rx_filter = %#x\n", config.flags, config.tx_type,
		 config.rx_filter);

	/* reserved for future extensions */
	if (config.flags) {
		return -EINVAL;
	}

	switch (config.tx_type) {
	case HWTSTAMP_TX_OFF:
		pdata->hwts_tx_en = OSI_DISABLE;
		break;

	case HWTSTAMP_TX_ON:
		pdata->hwts_tx_en = OSI_ENABLE;
		break;

	default:
		dev_err(pdata->dev, "tx_type is out of range\n");
		return -ERANGE;
	}
	/* Initialize ptp filter to 0 */
	osi_core->ptp_config.ptp_filter = 0;

	switch (config.rx_filter) {
		/* time stamp no incoming packet at all */
	case HWTSTAMP_FILTER_NONE:
		hwts_rx_en = 0;
		break;

		/* PTP v1, UDP, any kind of event packet */
	case HWTSTAMP_FILTER_PTP_V1_L4_EVENT:
		osi_core->ptp_config.ptp_filter = OSI_MAC_TCR_SNAPTYPSEL_1 |
						  OSI_MAC_TCR_TSIPV4ENA    |
						  OSI_MAC_TCR_TSIPV6ENA;
		break;

		/* PTP v1, UDP, Sync packet */
	case HWTSTAMP_FILTER_PTP_V1_L4_SYNC:
		osi_core->ptp_config.ptp_filter = OSI_MAC_TCR_TSEVENTENA |
						  OSI_MAC_TCR_TSIPV4ENA	 |
						  OSI_MAC_TCR_TSIPV6ENA;
		break;

		/* PTP v1, UDP, Delay_req packet */
	case HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ:
		osi_core->ptp_config.ptp_filter = OSI_MAC_TCR_TSMASTERENA |
						  OSI_MAC_TCR_TSEVENTENA  |
						  OSI_MAC_TCR_TSIPV4ENA   |
						  OSI_MAC_TCR_TSIPV6ENA;
		break;

		/* PTP v2, UDP, any kind of event packet */
	case HWTSTAMP_FILTER_PTP_V2_L4_EVENT:
		osi_core->ptp_config.ptp_filter = OSI_MAC_TCR_SNAPTYPSEL_1 |
						  OSI_MAC_TCR_TSIPV4ENA    |
						  OSI_MAC_TCR_TSIPV6ENA    |
						  OSI_MAC_TCR_TSVER2ENA;
		break;

		/* PTP v2, UDP, Sync packet */
	case HWTSTAMP_FILTER_PTP_V2_L4_SYNC:
		osi_core->ptp_config.ptp_filter = OSI_MAC_TCR_TSEVENTENA   |
						  OSI_MAC_TCR_TSIPV4ENA    |
						  OSI_MAC_TCR_TSIPV6ENA    |
						  OSI_MAC_TCR_TSVER2ENA;
		break;

		/* PTP v2, UDP, Delay_req packet */
	case HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ:
		osi_core->ptp_config.ptp_filter = OSI_MAC_TCR_TSEVENTENA   |
						  OSI_MAC_TCR_TSMASTERENA  |
						  OSI_MAC_TCR_TSIPV4ENA    |
						  OSI_MAC_TCR_TSIPV6ENA    |
						  OSI_MAC_TCR_TSVER2ENA;
		break;

		/* PTP v2/802.AS1, any layer, any kind of event packet */
	case HWTSTAMP_FILTER_PTP_V2_EVENT:
		osi_core->ptp_config.ptp_filter = OSI_MAC_TCR_SNAPTYPSEL_1 |
						  OSI_MAC_TCR_TSIPV4ENA    |
						  OSI_MAC_TCR_TSIPV6ENA    |
						  OSI_MAC_TCR_TSVER2ENA    |
						  OSI_MAC_TCR_TSIPENA;
		break;

		/* PTP v2/802.AS1, any layer, Sync packet */
	case HWTSTAMP_FILTER_PTP_V2_SYNC:
		osi_core->ptp_config.ptp_filter = OSI_MAC_TCR_TSIPV4ENA    |
						  OSI_MAC_TCR_TSIPV6ENA    |
						  OSI_MAC_TCR_TSVER2ENA    |
						  OSI_MAC_TCR_TSEVENTENA   |
						  OSI_MAC_TCR_TSIPENA      |
						  OSI_MAC_TCR_AV8021ASMEN;
		break;

		/* PTP v2/802.AS1, any layer, Delay_req packet */
	case HWTSTAMP_FILTER_PTP_V2_DELAY_REQ:
		osi_core->ptp_config.ptp_filter = OSI_MAC_TCR_TSIPV4ENA    |
						  OSI_MAC_TCR_TSIPV6ENA    |
						  OSI_MAC_TCR_TSVER2ENA    |
						  OSI_MAC_TCR_TSEVENTENA   |
						  OSI_MAC_TCR_AV8021ASMEN  |
						  OSI_MAC_TCR_TSMASTERENA  |
						  OSI_MAC_TCR_TSIPENA;
		break;

		/* time stamp any incoming packet */
	case HWTSTAMP_FILTER_ALL:
		osi_core->ptp_config.ptp_filter = OSI_MAC_TCR_TSENALL;
		break;

	default:
		dev_err(pdata->dev, "rx_filter is out of range\n");
		return -ERANGE;
	}

	if (!pdata->hwts_tx_en && !hwts_rx_en) {
		/* disable the PTP configuration */
		osi_ptp_configuration(osi_core, OSI_DISABLE);
	} else {
		/* Store SYS CLOCK */
		osi_core->ptp_config.ptp_clock = OSI_ETHER_SYSCLOCK;
		/* initialize system time */
		getnstimeofday(&now);
		/* Store sec and nsec */
		osi_core->ptp_config.sec = now.tv_sec;
		osi_core->ptp_config.nsec = now.tv_nsec;
		/* one nsec accuracy */
		osi_core->ptp_config.one_nsec_accuracy = OSI_ENABLE;
		/* Enable the PTP configuration */
		osi_ptp_configuration(osi_core, OSI_ENABLE);
	}

	return (copy_to_user(ifr->ifr_data, &config,
			     sizeof(struct hwtstamp_config))) ? -EFAULT : 0;
}

