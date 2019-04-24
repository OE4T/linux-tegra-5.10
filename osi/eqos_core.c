/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION. All rights reserved.
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

#include <osi_common.h>
#include <osi_core.h>
#include <osd.h>
#include "eqos_core.h"

struct osi_core_ops *eqos_get_hw_core_ops(void);

/**
 *	eqos_osi_config_rx_crc_check - Configure CRC Checking for Rx Packets
 *	@addr: MAC base address.
 *	@crc_chk: Enable or disable checking of CRC field in received packets
 *
 *	Algorithm: When this bit is set, the MAC receiver does not check the CRC
 *	field in the received packets. When this bit is reset, the MAC receiver
 *	always checks the CRC field in the received packets.
 *
 *	Dependencies: MAC has to be out of reset.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, -1 - failure
 */
static int eqos_config_rx_crc_check(void *addr, unsigned int crc_chk)
{
	unsigned int val;

	/* return on invalid argument */
	if (crc_chk != OSI_ENABLE && crc_chk != OSI_DISABLE) {
		return -1;
	}

	/* Read MAC Extension Register */
	val = osi_readl((unsigned char *)addr + EQOS_MAC_EXTR);

	/* crc_chk: 1 is for enable and 0 is for disable */
	if (crc_chk == OSI_ENABLE) {
		/* Enable Rx packets CRC check */
		val &= ~EQOS_MAC_EXTR_DCRCC;
	} else if (crc_chk == OSI_DISABLE) {
		/* Disable Rx packets CRC check */
		val |= EQOS_MAC_EXTR_DCRCC;
	} else {
		/* Nothing here */
	}

	/* Write to MAC Extension Register */
	osi_writel(val, (unsigned char *)addr + EQOS_MAC_EXTR);

	return 0;
}

/**
 *	eqos_config_fw_err_pkts - Configure forwarding of error packets
 *	@addr: MAC base address.
 *	@qinx: Q index
 *	@fw_err: Enable or Disable the forwarding of error packets
 *
 *	Algorithm: When this bit is reset, the Rx queue drops packets with error
 *	status (CRC error, GMII_ER, watchdog timeout, or overflow).
 *	When this bit is set, all packets except the runt error packets
 *	are forwarded to the application or DMA.
 *
 *	Dependencies: MAC has to be out of reset.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, -1 - failure
 */
static int eqos_config_fw_err_pkts(void *addr, unsigned int qinx,
				   unsigned int fw_err)
{
	unsigned int val;

	/* Check for valid fw_err and qinx values */
	if ((fw_err != OSI_ENABLE && fw_err != OSI_DISABLE) ||
	    (qinx >= OSI_EQOS_MAX_NUM_CHANS)) {
		return -1;
	}

	/* Read MTL RXQ Operation_Mode Register */
	val = osi_readl((unsigned char *)addr + EQOS_MTL_CHX_RX_OP_MODE(qinx));

	/* fw_err, 1 is for enable and 0 is for disable */
	if (fw_err == OSI_ENABLE) {
		/* When fw_err bit is set, all packets except the runt error
		 * packets are forwarded to the application or DMA.
		 */
		val |= EQOS_MTL_RXQ_OP_MODE_FEP;
	} else if (fw_err == OSI_DISABLE) {
		/* When this bit is reset, the Rx queue drops packets with error
		 * status (CRC error, GMII_ER, watchdog timeout, or overflow)
		 */
		val &= ~EQOS_MTL_RXQ_OP_MODE_FEP;
	} else {
		/* Nothing here */
	}

	/* Write to FEP bit of MTL RXQ Operation Mode Register to enable or
	 * disable the forwarding of error packets to DMA or application.
	 */
	osi_writel(val, (unsigned char *)addr + EQOS_MTL_CHX_RX_OP_MODE(qinx));

	return 0;
}

/**
 *	eqos_config_tx_status - Configure MAC to forward the tx pkt status
 *	@addr: MAC base address.
 *	@tx_status: Enable or Disable the forwarding of tx pkt status
 *
 *	Algorithm: When DTXSTS bit is reset, the Tx packet status received
 *	from the MAC is forwarded to the application.
 *	When DTXSTS bit is set, the Tx packet status received from the MAC
 *	are dropped in MTL.
 *
 *	Dependencies: MAC has to be out of reset.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, -1 - failure
 */
static int eqos_config_tx_status(void *addr, unsigned int tx_status)
{
	unsigned int val;

	/* don't allow if tx_status is other than 0 or 1 */
	if (tx_status != OSI_ENABLE && tx_status != OSI_DISABLE) {
		return -1;
	}

	/* Read MTL Operation Mode Register */
	val = osi_readl((unsigned char *)addr + EQOS_MTL_OP_MODE);

	if (tx_status == OSI_ENABLE) {
		/* When DTXSTS bit is reset, the Tx packet status received
		 * from the MAC are forwarded to the application.
		 */
		val &= ~EQOS_MTL_OP_MODE_DTXSTS;
	} else if (tx_status == OSI_DISABLE) {
		/* When DTXSTS bit is set, the Tx packet status received from
		 * the MAC are dropped in the MTL
		 */
		val |= EQOS_MTL_OP_MODE_DTXSTS;
	} else {
		/* Nothing here */
	}

	/* Write to DTXSTS bit of MTL Operation Mode Register to enable or
	 * disable the Tx packet status
	 */
	osi_writel(val, (unsigned char *)addr + EQOS_MTL_OP_MODE);

	return 0;
}

/**
 *	eqos_config_mac_loopback - Configure MAC to support loopback
 *	@addr: MAC base address.
 *	@lb_mode: Enable or Disable MAC loopback mode
 *
 *	Algorithm: Configure MAC to enable or disable loopback
 *
 *	Dependencies: MAC has to be out of reset.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, -1 - failure
 */
static int eqos_config_mac_loopback(void *addr, unsigned int lb_mode)
{
	unsigned int clk_ctrl_val;
	unsigned int mcr_val;

	/* don't allow only if loopback mode is other than 0 or 1 */
	if (lb_mode != OSI_ENABLE && lb_mode != OSI_DISABLE) {
		return -1;
	}

	/* Read MAC Configuration Register */
	mcr_val = osi_readl((unsigned char *)addr + EQOS_MAC_MCR);

	/* Read EQOS wrapper clock control 0 register */
	clk_ctrl_val = osi_readl((unsigned char *)addr + EQOS_CLOCK_CTRL_0);

	if (lb_mode == OSI_ENABLE) {
		/* Enable Loopback Mode */
		mcr_val |= EQOS_MAC_ENABLE_LM;
		/* Enable RX_CLK_SEL so that TX Clock is fed to RX domain */
		clk_ctrl_val |= EQOS_RX_CLK_SEL;
	} else if (lb_mode == OSI_DISABLE){
		/* Disable Loopback Mode */
		mcr_val &= ~EQOS_MAC_ENABLE_LM;
		/* Disable RX_CLK_SEL so that TX Clock is fed to RX domain */
		clk_ctrl_val &= ~EQOS_RX_CLK_SEL;
	} else {
		/* Nothing here */
	}

	/* Write to EQOS wrapper clock control 0 register */
	osi_writel(clk_ctrl_val, (unsigned char *)addr + EQOS_CLOCK_CTRL_0);

	/* Write to MAC Configuration Register */
	osi_writel(mcr_val, (unsigned char *)addr + EQOS_MAC_MCR);

	return 0;
}

/**
 *	eqos_poll_for_swr - Poll for software reset (SWR bit in DMA Mode)
 *	@addr: EQOS virtual base address.
 *
 *	Algorithm: CAR reset will be issued through MAC reset pin.
 *	Waits for SWR reset to be cleared in DMA Mode register.
 *
 *	Dependencies: None
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure
 */
static int eqos_poll_for_swr(void *addr)
{
	unsigned int retry = 1000;
	unsigned int count;
	unsigned int dma_bmr = 0;
	int cond = 1;

	/* add delay of 10 usec */
	osd_usleep_range(9, 11);

	/* Poll Until Poll Condition */
	count = 0;
	while (cond == 1) {
		if (count > retry) {
			return -1;
		}

		count++;
		osd_msleep(1U);

		dma_bmr = osi_readl((unsigned char *)addr + EQOS_DMA_BMR);

		if ((dma_bmr & EQOS_DMA_BMR_SWR) == 0U) {
			cond = 0;
		}
	}

	return 0;
}

/**
 *	eqos_set_mdc_clk_rate - Derive MDC clock based on provided AXI_CBB clk.
 *	@osi_core: OSI core private data structure.
 *	@csr_clk_rate: CSR (AXI CBB) clock rate.
 *
 *	Algorithm: MDC clock rate will be polulated OSI private data structure
 *	based on AXI_CBB clock rate.
 *
 *	Dependencies: OSD layer needs get the AXI CBB clock rate with OSD clock
 *	API (ex - clk_get_rate())
 *
 *	Return: None
 */
static void eqos_set_mdc_clk_rate(struct osi_core_priv_data *osi_core,
				  unsigned long csr_clk_rate)
{
	unsigned int csr_clk_speed = (unsigned int)(csr_clk_rate / 1000000U);

	if (csr_clk_speed > 500U) {
		osi_core->mdc_cr = EQOS_CSR_500_800M;
	} else if (csr_clk_speed > 300U) {
		osi_core->mdc_cr = EQOS_CSR_300_500M;
	} else if (csr_clk_speed > 250U) {
		osi_core->mdc_cr = EQOS_CSR_250_300M;
	} else if (csr_clk_speed > 150U) {
		osi_core->mdc_cr = EQOS_CSR_150_250M;
	} else if (csr_clk_speed > 100U) {
		osi_core->mdc_cr = EQOS_CSR_100_150M;
	} else if (csr_clk_speed > 60U) {
		osi_core->mdc_cr = EQOS_CSR_60_100M;
	} else if (csr_clk_speed > 35U) {
		osi_core->mdc_cr = EQOS_CSR_35_60M;
	} else {
		/* for CSR < 35mhz */
		osi_core->mdc_cr = EQOS_CSR_20_35M;
	}
}

/**
 *	eqos_set_speed - Set operating speed
 *	@base: EQOS virtual base address.
 *	@speed:	Operating speed.
 *
 *	Algorithm: Based on the speed (10/100/1000Mbps) MAC will be configured
 *	accordingly.
 *
 *	Dependencies: MAC has to be out of reset.
 *
 *	Protection: None
 *
 *	Return: None
 */
static void eqos_set_speed(void *base, int speed)
{
	unsigned int mcr_val;

	mcr_val = osi_readl((unsigned char *)base + EQOS_MAC_MCR);
	switch (speed) {
	default:
		mcr_val &= ~EQOS_MCR_PS;
		mcr_val &= ~EQOS_MCR_FES;
		break;
	case OSI_SPEED_1000:
		mcr_val &= ~EQOS_MCR_PS;
		mcr_val &= ~EQOS_MCR_FES;
		break;
	case OSI_SPEED_100:
		mcr_val |= EQOS_MCR_PS;
		mcr_val |= EQOS_MCR_FES;
		break;
	case OSI_SPEED_10:
		mcr_val |= EQOS_MCR_PS;
		mcr_val &= ~EQOS_MCR_FES;
		break;
	}

	osi_writel(mcr_val, (unsigned char *)base + EQOS_MAC_MCR);
}

/**
 *	eqos_set_mode - Set operating mode
 *	@base: EQOS virtual base address
 *	@mode:	Operating mode.
 *
 *	Algorithm: Based on the mode (HALF/FULL Duplex) MAC will be configured
 *	accordingly.
 *
 *	Dependencies: MAC has to be out of reset.
 *
 *	Protection: None
 *
 *	Return: None
 */
static void eqos_set_mode(void *base, int mode)
{
	unsigned int mcr_val;

	mcr_val = osi_readl((unsigned char *)base + EQOS_MAC_MCR);
	if (mode == OSI_FULL_DUPLEX) {
		mcr_val |= (0x00002000U);
	} else if (mode == OSI_HALF_DUPLEX) {
		mcr_val &= ~(0x00002000U);
	} else {
		/* Nothing here */
	}
	osi_writel(mcr_val, (unsigned char *)base + EQOS_MAC_MCR);
}

/**
 *	eqos_calculate_per_queue_fifo - Calculate per queue FIFO size
 *	@fifo_size:	Total Tx/RX HW FIFO size.
 *	@queue_count:	Total number of Queues configured.
 *
 *	Algorithm: Total Tx/Rx FIFO size which is read from
 *	MAC HW is being shared equally among the queues that are
 *	configured.
 *
 *	Dependencies: MAC has to be out of reset.
 *
 *	Protection: None
 *
 *	Return: Queue size that need to be programmed
 */
static unsigned int eqos_calculate_per_queue_fifo(unsigned int fifo_size,
						  unsigned int queue_count)
{
	unsigned int q_fifo_size = 0;  /* calculated fifo size per queue */
	unsigned int p_fifo = EQOS_256; /* per queue fifo size program value */

	/* calculate Tx/Rx fifo share per queue */
	switch (fifo_size) {
	case 0:
		q_fifo_size = FIFO_SIZE_B(128U);
		break;
	case 1:
		q_fifo_size = FIFO_SIZE_B(256U);
		break;
	case 2:
		q_fifo_size = FIFO_SIZE_B(512U);
		break;
	case 3:
		q_fifo_size = FIFO_SIZE_KB(1U);
		break;
	case 4:
		q_fifo_size = FIFO_SIZE_KB(2U);
		break;
	case 5:
		q_fifo_size = FIFO_SIZE_KB(4U);
		break;
	case 6:
		q_fifo_size = FIFO_SIZE_KB(8U);
		break;
	case 7:
		q_fifo_size = FIFO_SIZE_KB(16U);
		break;
	case 8:
		q_fifo_size = FIFO_SIZE_KB(32U);
		break;
	case 9:
		q_fifo_size = FIFO_SIZE_KB(36U);
		break;
	case 10:
		q_fifo_size = FIFO_SIZE_KB(128U);
		break;
	case 11:
		q_fifo_size = FIFO_SIZE_KB(256U);
		break;
	default:
		q_fifo_size = FIFO_SIZE_KB(36U);
		break;
	}

	q_fifo_size = q_fifo_size / queue_count;

	if (q_fifo_size >= FIFO_SIZE_KB(36U)) {
		p_fifo = EQOS_36K;
	} else if (q_fifo_size >= FIFO_SIZE_KB(32U)) {
		p_fifo = EQOS_32K;
	} else if (q_fifo_size >= FIFO_SIZE_KB(16U)) {
		p_fifo = EQOS_16K;
	} else if (q_fifo_size == FIFO_SIZE_KB(9U)) {
		p_fifo = EQOS_9K;
	} else if (q_fifo_size >= FIFO_SIZE_KB(8U)) {
		p_fifo = EQOS_8K;
	} else if (q_fifo_size >= FIFO_SIZE_KB(4U)) {
		p_fifo = EQOS_4K;
	} else if (q_fifo_size >= FIFO_SIZE_KB(2U)) {
		p_fifo = EQOS_2K;
	} else if (q_fifo_size >= FIFO_SIZE_KB(1U)) {
		p_fifo = EQOS_1K;
	} else if (q_fifo_size >= FIFO_SIZE_B(512U)) {
		p_fifo = EQOS_512;
	} else if (q_fifo_size >= FIFO_SIZE_B(256U)) {
		p_fifo = EQOS_256;
	} else {
		/* Nothing here */
	}

	return p_fifo;
}

/**
 *	eqos_pad_calibrate - PAD calibration
 *	@ioaddr:	Base address of the MAC HW.
 *
 *	Algorithm:
 *	1) Set field PAD_E_INPUT_OR_E_PWRD in reg ETHER_QOS_SDMEMCOMPPADCTRL_0
 *	2) Delay for 1 usec.
 *	3)Set AUTO_CAL_ENABLE and AUTO_CAL_START in reg
 *	ETHER_QOS_AUTO_CAL_CONFIG_0
 *	4) Wait on AUTO_CAL_ACTIVE until it is 0
 *	5) Re-program the value PAD_E_INPUT_OR_E_PWRD in
 *	ETHER_QOS_SDMEMCOMPPADCTRL_0 to save power
 *
 *	Dependencies: RGMII and MDIO interface needs to be IDLE
 *	before performing PAD calibration.
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure
 */
static int eqos_pad_calibrate(void *ioaddr)
{
	unsigned int retry = 1000;
	unsigned int count;
	int cond = 1;
	unsigned int value;

	/* 1. Set field PAD_E_INPUT_OR_E_PWRD in
	 * reg ETHER_QOS_SDMEMCOMPPADCTRL_0
	 */
	value = osi_readl((unsigned char *)ioaddr + EQOS_PAD_CRTL);
	value |= EQOS_PAD_CRTL_E_INPUT_OR_E_PWRD;
	osi_writel(value, (unsigned char *)ioaddr + EQOS_PAD_CRTL);

	/* 2. delay for 1 usec */
	osd_usleep_range(1, 3);

	/* 3. Set AUTO_CAL_ENABLE and AUTO_CAL_START in
	 * reg ETHER_QOS_AUTO_CAL_CONFIG_0.
	 */
	value = osi_readl((unsigned char *)ioaddr + EQOS_PAD_AUTO_CAL_CFG);
	value |= EQOS_PAD_AUTO_CAL_CFG_START |
		 EQOS_PAD_AUTO_CAL_CFG_ENABLE;
	osi_writel(value, (unsigned char *)ioaddr + EQOS_PAD_AUTO_CAL_CFG);

	/* 4. Wait on 1 to 3 us before start checking for calibration done.
	 *    This delay is consumed in delay inside while loop.
	 */

	/* 5. Wait on AUTO_CAL_ACTIVE until it is 0. 10ms is the timeout */
	count = 0;
	while (cond == 1) {
		if (count > retry) {
			return -1;
		}
		count++;
		osd_usleep_range(10, 12);
		value = osi_readl((unsigned char *)ioaddr + EQOS_PAD_AUTO_CAL_STAT);
		/* calibration done when CAL_STAT_ACTIVE is zero */
		if ((value & EQOS_PAD_AUTO_CAL_STAT_ACTIVE) == 0U) {
			cond = 0;
		}
	}

	/* 6. Re-program the value PAD_E_INPUT_OR_E_PWRD in
	 * ETHER_QOS_SDMEMCOMPPADCTRL_0 to save power
	 */
	value = osi_readl((unsigned char *)ioaddr + EQOS_PAD_CRTL);
	value &=  ~EQOS_PAD_CRTL_E_INPUT_OR_E_PWRD;
	osi_writel(value, (unsigned char *)ioaddr + EQOS_PAD_CRTL);

	return 0;
}

/**
 *	eqos_flush_mtl_tx_queue - Flush MTL Tx queue
 *	@addr: OSI core private data structure.
 *	@qinx: MTL queue index.
 *
 *	Algorithm: Flush a MTL Tx queue.
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
static int eqos_flush_mtl_tx_queue(void *addr, unsigned int qinx)
{
	unsigned int retry = 1000;
	unsigned int count;
	unsigned int value;
	int cond = 1;

	/* Read Tx Q Operating Mode Register and flush TxQ */
	value = osi_readl((unsigned char *)addr + EQOS_MTL_CHX_TX_OP_MODE(qinx));
	value |= EQOS_MTL_QTOMR_FTQ;
	osi_writel(value, (unsigned char *)addr + EQOS_MTL_CHX_TX_OP_MODE(qinx));

	/* Poll Until FTQ bit resets for Successful Tx Q flush */
	count = 0;
	while (cond == 1) {
		if (count > retry) {
			return -1;
		}

		count++;
		osd_msleep(1);

		value = osi_readl((unsigned char *)addr + EQOS_MTL_CHX_TX_OP_MODE(qinx));

		if ((value & EQOS_MTL_QTOMR_FTQ_LPOS) == 0U) {
			cond = 0;
		}
	}

	return 0;
}

/**
 *	eqos_configure_mtl_queue - Configure MTL Queue
 *	@qinx:	Queue number that need to be configured.
 *	@osi_core: OSI core private data.
 *	@tx_fifo: MTL TX queue size for a MTL queue.
 *	@rx_fifo: MTL RX queue size for a MTL queue.
 *
 *	Algorithm: This takes care of configuring the  below
 *	parameters for the MTL Queue
 *	1) Mapping MTL Rx queue and DMA Rx channel
 *	2) Flush TxQ
 *	3) Enable Store and Forward mode for Tx, Rx
 *	4) Configure Tx and Rx MTL Queue sizes
 *	5) Configure TxQ weight
 *	6) Enable Rx Queues
 *
 *	Dependencies: MAC has to be out of reset.
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure
 */
static int eqos_configure_mtl_queue(unsigned int qinx,
				    struct osi_core_priv_data *osi_core,
				    unsigned int tx_fifo,
				    unsigned int rx_fifo)
{
	unsigned int value = 0;
	int ret = 0;

	ret = eqos_flush_mtl_tx_queue(osi_core->base, qinx);
	if (ret < 0) {
		return ret;
	}

	value = (tx_fifo << EQOS_MTL_TXQ_SIZE_SHIFT);
	/* Enable Store and Forward mode */
	value |= EQOS_MTL_TSF;
	/* Enable TxQ */
	value |= EQOS_MTL_TXQEN;
	osi_writel(value, (unsigned char *)osi_core->base + EQOS_MTL_CHX_TX_OP_MODE(qinx));

	/* read RX Q0 Operating Mode Register */
	value = osi_readl((unsigned char *)osi_core->base +  EQOS_MTL_CHX_RX_OP_MODE(qinx));
	value |= (rx_fifo << EQOS_MTL_RXQ_SIZE_SHIFT);
	/* Enable Store and Forward mode */
	value |= EQOS_MTL_RSF;
	osi_writel(value, (unsigned char *)osi_core->base + EQOS_MTL_CHX_RX_OP_MODE(qinx));

	/* Transmit Queue weight */
	value = osi_readl((unsigned char *)osi_core->base + EQOS_MTL_TXQ_QW(qinx));
	value |= (EQOS_MTL_TXQ_QW_ISCQW + qinx);
	osi_writel(value, (unsigned char *)osi_core->base + EQOS_MTL_TXQ_QW(qinx));

	/* Enable Rx Queue Control */
	value = osi_readl((unsigned char *)osi_core->base + EQOS_MAC_RQC0R);
	value |= ((osi_core->rxq_ctrl[qinx] & 0x3U) << (qinx * 2U));
	osi_writel(value, (unsigned char *)osi_core->base + EQOS_MAC_RQC0R);

	return 0;
}

/**
 *	eqos_configure_mac - Configure MAC
 *	@osi_core: OSI private data structure.
 *
 *	Algorithm: This takes care of configuring the  below
 *	parameters for the MAC
 *	1) Programming the MAC address
 *	2) Enable required MAC control fields in MCR
 *	3) Enable Multicast and Broadcast Queue
 *	4) Disable MMC interrupts and Configure the MMC counters
 *	5) Enable required MAC interrupts
 *
 *	Dependencies: MAC has to be out of reset.
 *
 *	Protection: None
 *
 *	Return: NONE
 */
static void eqos_configure_mac(struct osi_core_priv_data *osi_core)
{
	unsigned int value;
	/* Update MAC address 0 high */
	osi_writel((((unsigned int)osi_core->mac_addr[5] << 8U) |
		   (unsigned int)(osi_core->mac_addr[4])),
		   (unsigned char *)osi_core->base + EQOS_MAC_MA0HR);
	/* Update MAC address 0 Low */
	osi_writel((((unsigned int)osi_core->mac_addr[3] << 24U) |
		   ((unsigned int)osi_core->mac_addr[2] << 16U) |
		   ((unsigned int)osi_core->mac_addr[1] << 8U) |
		   ((unsigned int)osi_core->mac_addr[0])),
		   (unsigned char *)osi_core->base + EQOS_MAC_MA0LR);

	/* Read MAC Configuration Register */
	value = osi_readl((unsigned char *)osi_core->base + EQOS_MAC_MCR);
	/* Enable Automatic Pad or CRC Stripping */
	/* Enable CRC stripping for Type packets */
	/* Enable Full Duplex mode */
	value |= EQOS_MCR_ACS | EQOS_MCR_CST | EQOS_MCR_DM;

	if (osi_core->mtu > OSI_DFLT_MTU_SIZE) {
		value |= EQOS_MCR_S2KP;
	}

	if (osi_core->mtu > OSI_MTU_SIZE_2K) {
		value |= EQOS_MCR_JE;
		value |= EQOS_MCR_JD;
	}

	osi_writel(value, (unsigned char *)osi_core->base + EQOS_MAC_MCR);

	/* Enable Multicast and Broadcast Queue, default is Q0 */
	value = osi_readl((unsigned char *)osi_core->base + EQOS_MAC_RQC1R);
	value |=  EQOS_MAC_RQC1R_MCBCQEN;
	osi_writel(value, (unsigned char *)osi_core->base + EQOS_MAC_RQC1R);

	/* Disable all MMC interrupts */
	/* Disable all MMC Tx Interrupts */
	osi_writel(0xFFFFFFFFU, (unsigned char *)osi_core->base + EQOS_MMC_TX_INTR_MASK);
	/* Disable all MMC RX interrupts */
	osi_writel(0xFFFFFFFFU, (unsigned char *)osi_core->base + EQOS_MMC_RX_INTR_MASK);
	/* Disable MMC Rx interrupts for IPC */
	osi_writel(0xFFFFFFFFU, (unsigned char *)osi_core->base + EQOS_MMC_IPC_RX_INTR_MASK);

	/* Configure MMC counters */
	value = osi_readl((unsigned char *)osi_core->base + EQOS_MMC_CNTRL);
	value |= EQOS_MMC_CNTRL_CNTRST | EQOS_MMC_CNTRL_RSTONRD |
		 EQOS_MMC_CNTRL_CNTPRST | EQOS_MMC_CNTRL_CNTPRSTLVL;
	osi_writel(value, (unsigned char *)osi_core->base + EQOS_MMC_CNTRL);

	/* Enable MAC interrupts */
	/* Read MAC IMR Register */
	value = osi_readl((unsigned char *)osi_core->base + EQOS_MAC_IMR);
	/* RGSMIIIM - RGMII/SMII interrupt Enable */
	/* TODO: LPI need to be enabled during EEE implementation */
	value |= EQOS_IMR_RGSMIIIE;

	osi_writel(value, (unsigned char *)osi_core->base + EQOS_MAC_IMR);

	/* Enable VLAN configuration */
	value = osi_readl((unsigned char *)osi_core->base + EQOS_MAC_VLAN_TAG);
	/* Enable VLAN Tag stripping always
	 * Enable operation on the outer VLAN Tag, if present
	 * Disable double VLAN Tag processing on TX and RX
	 * Enable VLAN Tag in RX Status
	 * Disable VLAN Type Check
	 */
	value |= EQOS_MAC_VLANTR_EVLS_ALWAYS_STRIP | EQOS_MAC_VLANTR_EVLRXS |
		 EQOS_MAC_VLANTR_DOVLTC;
	value &= ~EQOS_MAC_VLANTR_ERIVLT;
	osi_writel(value, (unsigned char *)osi_core->base + EQOS_MAC_VLAN_TAG);

	value = osi_readl((unsigned char *)osi_core->base + EQOS_MAC_VLANTIR);
	/* Enable VLAN tagging through context descriptor */
	value |= EQOS_MAC_VLANTIR_VLTI;
	/* insert/replace C_VLAN in 13th & 14th bytes of transmitted frames */
	value &= ~EQOS_MAC_VLANTIRR_CSVL;
	osi_writel(value, (unsigned char *)osi_core->base + EQOS_MAC_VLANTIR);
}

/**
 *	eqos_configure_dma - Configure DMA
 *	@base: EQOS virtual base address.
 *
 *	Algorithm: This takes care of configuring the  below
 *	parameters for the DMA
 *	1) Programming different burst length for the DMA
 *	2) Enable enhanced Address mode
 *	3) Programming max read outstanding request limit
 *
 *	Dependencies: MAC has to be out of reset.
 *
 *	Protection: None
 *
 *	Return: NONE
 */
static void eqos_configure_dma(void *base)
{
	unsigned int value = 0;

	/* AXI Burst Length 8*/
	value |= EQOS_DMA_SBUS_BLEN8;
	/* AXI Burst Length 16*/
	value |= EQOS_DMA_SBUS_BLEN16;
	/* Enhanced Address Mode Enable */
	value |= EQOS_DMA_SBUS_EAME;
	/* AXI Maximum Read Outstanding Request Limit = 31 */
	value |= EQOS_DMA_SBUS_RD_OSR_LMT;
	/* AXI Maximum Write Outstanding Request Limit = 31 */
	value |= EQOS_DMA_SBUS_WR_OSR_LMT;

	osi_writel(value, (unsigned char *)base + EQOS_DMA_SBUS);

	value = osi_readl((unsigned char *)base + EQOS_DMA_BMR);
	value |= EQOS_DMA_BMR_DPSW;
	osi_writel(value, (unsigned char *)base + EQOS_DMA_BMR);
}

/**
 *	eqos_core_init - EQOS MAC, MTL and common DMA Initialization
 *	@osi_core: OSI core private data structure.
 *	@tx_fifo_size: MTL TX FIFO size
 *	@rx_fifo_size: MTL RX FIFO size
 *
 *	Algorithm: This function will take care of initializing MAC, MTL and
 *	common DMA registers.
 *
 *	Dependencies: Required clks and resets has to be enabled
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure
 */
static int eqos_core_init(struct osi_core_priv_data *osi_core,
			  unsigned int tx_fifo_size,
			  unsigned int rx_fifo_size)
{
	int ret = 0;
	unsigned int qinx = 0;
	unsigned int value = 0;
	unsigned int tx_fifo = 0;
	unsigned int rx_fifo = 0;

	/* PAD calibration */
	ret = eqos_pad_calibrate(osi_core->base);
	if (ret < 0) {
		return ret;
	}

	/* reset mmc counters */
	osi_writel(EQOS_MMC_CNTRL_CNTRST, (unsigned char *)osi_core->base + EQOS_MMC_CNTRL);

	/* Mapping MTL Rx queue and DMA Rx channel */
	/* TODO: Need to add EQOS_MTL_RXQ_DMA_MAP1 for EQOS */
	value = osi_readl((unsigned char *)osi_core->base + EQOS_MTL_RXQ_DMA_MAP0);
	value |= EQOS_RXQ_TO_DMA_CHAN_MAP;
	osi_writel(value, (unsigned char *)osi_core->base + EQOS_MTL_RXQ_DMA_MAP0);

	/* Calculate value of Transmit queue fifo size to be programmed */
	tx_fifo = eqos_calculate_per_queue_fifo(tx_fifo_size,
						osi_core->num_mtl_queues);

	/* Calculate value of Receive queue fifo size to be programmed */
	rx_fifo = eqos_calculate_per_queue_fifo(rx_fifo_size,
						osi_core->num_mtl_queues);

	/* Configure MTL Queues */
	for (qinx = 0; qinx < osi_core->num_mtl_queues; qinx++) {
		ret = eqos_configure_mtl_queue(osi_core->mtl_queues[qinx],
					       osi_core, tx_fifo, rx_fifo);
		if (ret < 0) {
			return ret;
		}
	}

	/* configure EQOS MAC HW */
	eqos_configure_mac(osi_core);

	/* configure EQOS DMA */
	eqos_configure_dma(osi_core->base);

	return ret;
}

/**
 *	eqos_handle_mac_intrs - Hanle MAC interrupts
 *	@osi_core: OSI core private data structure.
 *	@dma_isr: DMA ISR register read value.
 *
 *	Algorithm: This function takes care of handling the
 *	MAC interrupts which includes speed, mode detection.
 *
 *	Dependencies: MAC interrupts need to be enabled
 *
 *	Protection: None
 *
 *	Return: NONE
 */
static void eqos_handle_mac_intrs(struct osi_core_priv_data *osi_core,
				  unsigned int dma_isr)
{
	unsigned int mac_imr = 0;
	unsigned int mac_pcs = 0;
	unsigned int mac_isr = 0;

	mac_isr = osi_readl((unsigned char *)osi_core->base + EQOS_MAC_ISR);

	/* Handle MAC interrupts */
	if ((dma_isr & EQOS_DMA_ISR_MACIS) != EQOS_DMA_ISR_MACIS) {
		return;
	}

	/* handle only those MAC interrupts which are enabled */
	mac_imr = osi_readl((unsigned char *)osi_core->base + EQOS_MAC_IMR);
	mac_isr = (mac_isr & mac_imr);
	/* RGMII/SMII interrupt */
	if ((mac_isr & EQOS_MAC_ISR_RGSMIIS) != EQOS_MAC_ISR_RGSMIIS) {
		return;
	}

	mac_pcs = osi_readl((unsigned char *)osi_core->base + EQOS_MAC_PCS);
	/* check whether Link is UP or NOT - if not return. */
	if ((mac_pcs & EQOS_MAC_PCS_LNKSTS) != EQOS_MAC_PCS_LNKSTS) {
		return;
	}

	/* check for Link mode (full/half duplex) */
	if ((mac_pcs & EQOS_MAC_PCS_LNKMOD) == EQOS_MAC_PCS_LNKMOD) {
		eqos_set_mode(osi_core->base, OSI_FULL_DUPLEX);
	} else {
		eqos_set_mode(osi_core->base, OSI_HALF_DUPLEX);
	}

	/* set speed at MAC level */
	/* TODO: set_tx_clk needs to be done */
	/* Maybe through workqueue for QNX */
	if ((mac_pcs & EQOS_MAC_PCS_LNKSPEED) == EQOS_MAC_PCS_LNKSPEED_10) {
		eqos_set_speed(osi_core->base, OSI_SPEED_10);
	} else if ((mac_pcs & EQOS_MAC_PCS_LNKSPEED) == EQOS_MAC_PCS_LNKSPEED_100) {
		eqos_set_speed(osi_core->base, OSI_SPEED_100);
	} else if ((mac_pcs & EQOS_MAC_PCS_LNKSPEED) == EQOS_MAC_PCS_LNKSPEED_1000) {
		eqos_set_speed(osi_core->base, OSI_SPEED_1000);
	} else {
		/* Nothing here */
	}
}

/**
 *	eqos_handle_common_intr - Handles common interrupt.
 *	@osi_core: OSI core private data structure.
 *
 *	Algorithm: Clear common interrupt source.
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
static void eqos_handle_common_intr(struct osi_core_priv_data *osi_core)
{
	void *base = osi_core->base;
	unsigned int dma_isr = 0;
	unsigned int qinx = 0;
	unsigned int i = 0;
	unsigned int dma_sr = 0;
	unsigned int dma_ier = 0;

	dma_isr = osi_readl((unsigned char *)base + EQOS_DMA_ISR);
	if (dma_isr == 0U) {
		return;
	}

	//FIXME Need to check how we can get the DMA channel here instead of
	//MTL Queues
	if ((dma_isr & 0xFU) != 0U) {
		/* Handle Non-TI/RI interrupts */
		for (i = 0; i < osi_core->num_mtl_queues; i++) {
			qinx = osi_core->mtl_queues[i];

			/* read dma channel status register */
			dma_sr = osi_readl((unsigned char *)base + EQOS_DMA_CHX_STATUS(qinx));
			/* read dma channel interrupt enable register */
			dma_ier = osi_readl((unsigned char *)base + EQOS_DMA_CHX_IER(qinx));

			/* process only those interrupts which we
			 * have enabled.
			 */
			dma_sr = (dma_sr & dma_ier);

			/* mask off RI and TI */
			dma_sr &= ~(OSI_BIT(6) | OSI_BIT(0));
			if (dma_sr == 0U) {
				return;
			}

			/* ack non ti/ri ints */
			osi_writel(dma_sr, (unsigned char *)base + EQOS_DMA_CHX_STATUS(qinx));
		}
	}

	eqos_handle_mac_intrs(osi_core, dma_isr);
}

/**
 *	eqos_start_mac - Start MAC Tx/Rx engine
 *	@addr: EQOS virtual base address.
 *
 *	Algorithm: Enable MAC Transmitter and Receiver
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
static void eqos_start_mac(void *addr)
{
	unsigned int value;

	value = osi_readl((unsigned char *)addr + EQOS_MAC_MCR);
	/* Enable MAC Transmit */
	/* Enable MAC Receive */
	value |= EQOS_MCR_TE | EQOS_MCR_RE;
	osi_writel(value, (unsigned char *)addr + EQOS_MAC_MCR);
}

/**
 *	eqos_stop_mac - Stop MAC Tx/Rx engine
 *	@addr: EQOS virtual base address.
 *
 *	Algorithm: Disables MAC Transmitter and Receiver
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
static void eqos_stop_mac(void *addr)
{
	unsigned int value;

	value = osi_readl((unsigned char *)addr + EQOS_MAC_MCR);
	/* Disable MAC Transmit */
	/* Disable MAC Receive */
	value &= ~EQOS_MCR_TE;
	value &= ~EQOS_MCR_RE;
	osi_writel(value, (unsigned char *)addr + EQOS_MAC_MCR);
}

/**
 *	eqos_set_avb_algorithm - Set TxQ/TC avb config
 *	@osi_core: osi core priv data structure
 *	@avb: structure having configuration for avb algorithm
 *
 *	Algorithm:
 *	1) Check if queue index is valid
 *	2) Update operation mode of TxQ/TC
 *	 2a) Set TxQ operation mode
 *	 2b) Set Algo and Credit contro
 *	 2c) Set Send slope credit
 *	 2d) Set Idle slope credit
 *	 2e) Set Hi credit
 *	 2f) Set low credit
 *	3) Update register values
 *
 *	Dependencies: MAC has to be out of reset.
 *
 *	Protection: None.
 *
 *	Return: 0: success, -1: error.
 */
static int eqos_set_avb_algorithm(struct osi_core_priv_data *osi_core,
				  struct osi_core_avb_algorithm *avb)
{
	unsigned int value;
	int ret = -1;
	unsigned int qinx;

	if (avb == OSI_NULL) {
		osd_err(osi_core->osd, "avb structure is NULL\n");
		return ret;
	}

	/* queue index in range */
	if (avb->qindex >= EQOS_MAX_TC) {
		osd_err(osi_core->osd, "Invalid Queue index (%d)\n"
			, avb->qindex);
		return ret;
	}

	/* can't set AVB mode for queue 0 */
	if ((avb->qindex == 0U) && (avb->oper_mode == EQOS_MTL_QUEUE_AVB)) {
		osd_err(osi_core->osd,
			"Not allowed to set CBS for Q0\n", avb->qindex);
		return ret;
	}

	qinx = avb->qindex;
	value = osi_readl((unsigned char *)osi_core->base +
			  EQOS_MTL_CHX_TX_OP_MODE(qinx));
	value &= ~EQOS_MTL_TXQEN_MASK;
	/* Set TxQ/TC mode as per input struct after masking 3 bit */
	value |= (avb->oper_mode << EQOS_MTL_TXQEN_MASK_SHIFT) &
		  EQOS_MTL_TXQEN_MASK;
	osi_writel(value, (unsigned char *)osi_core->base +
		   EQOS_MTL_CHX_TX_OP_MODE(qinx));

	/* Set Algo and Credit control */
	value = (avb->credit_control << EQOS_MTL_TXQ_ETS_CR_CC_SHIFT) &
		 EQOS_MTL_TXQ_ETS_CR_CC;
	value |= (avb->algo << EQOS_MTL_TXQ_ETS_CR_AVALG_SHIFT) &
		  EQOS_MTL_TXQ_ETS_CR_AVALG;
	osi_writel(value, (unsigned char *)osi_core->base +
		   EQOS_MTL_TXQ_ETS_CR(qinx));

	/* Set Send slope credit */
	value = avb->send_slope & EQOS_MTL_TXQ_ETS_SSCR_SSC_MASK;
	osi_writel(value, (unsigned char *)osi_core->base +
		   EQOS_MTL_TXQ_ETS_SSCR(qinx));

	/* Set Idle slope credit*/
	value = osi_readl((unsigned char *)osi_core->base +
			  EQOS_MTL_TXQ_QW(qinx));
	value &= ~EQOS_MTL_TXQ_ETS_QW_ISCQW_MASK;
	value |= avb->idle_slope & EQOS_MTL_TXQ_ETS_QW_ISCQW_MASK;
	osi_writel(value, (unsigned char *)osi_core->base +
		   EQOS_MTL_TXQ_QW(qinx));

	/* Set Hi credit */
	value = avb->hi_credit & EQOS_MTL_TXQ_ETS_HCR_HC_MASK;
	osi_writel(value, (unsigned char *)osi_core->base +
		   EQOS_MTL_TXQ_ETS_HCR(qinx));

	/* low credit  is -ve number, osi_write need a unsigned int
	 * take only 28:0 bits from avb->low_credit
	 */
	value = avb->low_credit & EQOS_MTL_TXQ_ETS_LCR_LC_MASK;
	osi_writel(value, (unsigned char *)osi_core->base +
		   EQOS_MTL_TXQ_ETS_LCR(qinx));

	return 0;
}

/**
 *	eqos_get_avb_algorithm - Get TxQ/TC avb config
 *	@osi_core: osi core priv data structure
 *	@avb: structure pointer having configuration for avb algorithm
 *
 *	Algorithm:
 *	1) Check if queue index is valid
 *	2) read operation mode of TxQ/TC
 *	 2a) read TxQ operation mode
 *	 2b) read Algo and Credit contro
 *	 2c) read Send slope credit
 *	 2d) read Idle slope credit
 *	 2e) read Hi credit
 *	 2f) read low credit
 *	3) updated pointer
 *
 *	Dependencies: MAC has to be out of reset.
 *
 *	Protection: None.
 *
 *	Return: 0: Success -1: Failure
 */

static int eqos_get_avb_algorithm(struct osi_core_priv_data *osi_core,
				  struct osi_core_avb_algorithm *avb)
{
	unsigned int value;
	int ret = -1;
	unsigned int qinx = 0U;

	if (avb == OSI_NULL) {
		osd_err(osi_core->osd, "avb structure is NULL\n");
		return ret;
	}

	if (avb->qindex >= EQOS_MAX_TC) {
		osd_err(osi_core->osd, "Invalid Queue index (%d)\n"
			, avb->qindex);
		return ret;
	}

	qinx = avb->qindex;
	value = osi_readl((unsigned char *)osi_core->base +
			  EQOS_MTL_CHX_TX_OP_MODE(qinx));

	/* Get TxQ/TC mode as per input struct after masking 3:2 bit */
	value = (value & EQOS_MTL_TXQEN) >> EQOS_MTL_TXQEN_MASK_SHIFT;
	avb->oper_mode = value;

	/* Get Algo and Credit control */
	value = osi_readl((unsigned char *)osi_core->base +
			  EQOS_MTL_TXQ_ETS_CR(qinx));
	avb->credit_control = (value & EQOS_MTL_TXQ_ETS_CR_CC) >>
		   EQOS_MTL_TXQ_ETS_CR_CC_SHIFT;
	avb->algo = (value & EQOS_MTL_TXQ_ETS_CR_AVALG) >>
		     EQOS_MTL_TXQ_ETS_CR_AVALG_SHIFT;

	/* Get Send slope credit */
	value = osi_readl(osi_core->base + EQOS_MTL_TXQ_ETS_SSCR(qinx));
	avb->send_slope = value & EQOS_MTL_TXQ_ETS_SSCR_SSC_MASK;

	/* Get Idle slope credit*/
	value = osi_readl((unsigned char *)osi_core->base +
			  EQOS_MTL_TXQ_QW(qinx));
	avb->idle_slope = value & EQOS_MTL_TXQ_ETS_QW_ISCQW_MASK;

	/* Get Hi credit */
	value = osi_readl((unsigned char *)osi_core->base +
			  EQOS_MTL_TXQ_ETS_HCR(qinx));
	avb->hi_credit = value & EQOS_MTL_TXQ_ETS_HCR_HC_MASK;

	/* Get Low credit for which bit 31:29 are unknown
	 * return 28:0 valid bits to application
	 */
	value = osi_readl((unsigned char *)osi_core->base +
			  EQOS_MTL_TXQ_ETS_LCR(qinx));
	avb->low_credit = value & EQOS_MTL_TXQ_ETS_LCR_LC_MASK;

	return 0;
}

static struct osi_core_ops eqos_core_ops = {
	.poll_for_swr = eqos_poll_for_swr,
	.core_init = eqos_core_init,
	.start_mac = eqos_start_mac,
	.stop_mac = eqos_stop_mac,
	.handle_common_intr = eqos_handle_common_intr,
	.set_mode = eqos_set_mode,
	.set_speed = eqos_set_speed,
	.pad_calibrate = eqos_pad_calibrate,
	.set_mdc_clk_rate = eqos_set_mdc_clk_rate,
	.flush_mtl_tx_queue = eqos_flush_mtl_tx_queue,
	.config_mac_loopback = eqos_config_mac_loopback,
	.set_avb_algorithm = eqos_set_avb_algorithm,
	.get_avb_algorithm = eqos_get_avb_algorithm,
	.config_fw_err_pkts = eqos_config_fw_err_pkts,
	.config_tx_status = eqos_config_tx_status,
	.config_rx_crc_check = eqos_config_rx_crc_check,
};

struct osi_core_ops *eqos_get_hw_core_ops(void)
{
	return &eqos_core_ops;
}
