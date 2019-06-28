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
#include "eqos_mmc.h"

struct osi_core_ops *eqos_get_hw_core_ops(void);

/**
 *	eqos_config_flow_control - Configure MAC flow control settings
 *	@addr: MAC base address.
 *	@flw_ctrl: flw_ctrl settings
 *
 *	Algorithm:
 *
 *	Dependencies: MAC has to be out of reset.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, -1 - failure
 */
static int eqos_config_flow_control(void *addr, unsigned int flw_ctrl)
{
	unsigned int val;

	/* return on invalid argument */
	if (flw_ctrl > (OSI_FLOW_CTRL_RX | OSI_FLOW_CTRL_TX)) {
		return -1;
	}

	/* Configure MAC Tx Flow control */
	/* Read MAC Tx Flow control Register of Q0 */
	val = osi_readl((unsigned char *)addr + EQOS_MAC_QX_TX_FLW_CTRL(0U));

	/* flw_ctrl BIT0: 1 is for tx flow ctrl enable
	 * flw_ctrl BIT0: 0 is for tx flow ctrl disable
	 */
	if ((flw_ctrl & OSI_FLOW_CTRL_TX) == OSI_FLOW_CTRL_TX) {
		/* Enable Tx Flow Control */
		val |= EQOS_MAC_QX_TX_FLW_CTRL_TFE;
		/* Mask and set Pause Time */
		val &= ~EQOS_MAC_PAUSE_TIME_MASK;
		val |= EQOS_MAC_PAUSE_TIME & EQOS_MAC_PAUSE_TIME_MASK;
	} else {
		/* Disable Tx Flow Control */
		val &= ~EQOS_MAC_QX_TX_FLW_CTRL_TFE;
	}

	/* Write to MAC Tx Flow control Register of Q0 */
	osi_writel(val, (unsigned char *)addr + EQOS_MAC_QX_TX_FLW_CTRL(0U));

	/* Configure MAC Rx Flow control*/
	/* Read MAC Rx Flow control Register */
	val = osi_readl((unsigned char *)addr + EQOS_MAC_RX_FLW_CTRL);

	/* flw_ctrl BIT1: 1 is for rx flow ctrl enable
	 * flw_ctrl BIT1: 0 is for rx flow ctrl disable
	 */
	if ((flw_ctrl & OSI_FLOW_CTRL_RX) == OSI_FLOW_CTRL_RX) {
		/* Enable Rx Flow Control */
		val |= EQOS_MAC_RX_FLW_CTRL_RFE;
	} else {
		/* Disable Rx Flow Control */
		val &= ~EQOS_MAC_RX_FLW_CTRL_RFE;
	}

	/* Write to MAC Rx Flow control Register */
	osi_writel(val, (unsigned char *)addr + EQOS_MAC_RX_FLW_CTRL);

	return 0;
}

/**
 *	eqos_config_rx_crc_check - Configure CRC Checking for Rx Packets
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
		value = osi_readl((unsigned char *)ioaddr +
				  EQOS_PAD_AUTO_CAL_STAT);
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
	value = osi_readl((unsigned char *)addr +
			  EQOS_MTL_CHX_TX_OP_MODE(qinx));
	value |= EQOS_MTL_QTOMR_FTQ;
	osi_writel(value, (unsigned char *)addr +
		   EQOS_MTL_CHX_TX_OP_MODE(qinx));

	/* Poll Until FTQ bit resets for Successful Tx Q flush */
	count = 0;
	while (cond == 1) {
		if (count > retry) {
			return -1;
		}

		count++;
		osd_msleep(1);

		value = osi_readl((unsigned char *)addr +
				  EQOS_MTL_CHX_TX_OP_MODE(qinx));

		if ((value & EQOS_MTL_QTOMR_FTQ_LPOS) == 0U) {
			cond = 0;
		}
	}

	return 0;
}

/**
 *	update_ehfc_rfa_rfd - Update EHFC, RFD and RSA values
 *	@rx_fifo: Rx FIFO size.
 *	@value: Stores RFD and RSA values
 *
 *	Algorithm: Calulates and stores the RSD (Threshold for Dectivating
 *	Flow control) and RSA (Threshold for Activating Flow Control) values
 *	based on the Rx FIFO size and also enables HW flow control
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
void update_ehfc_rfa_rfd(unsigned int rx_fifo, unsigned int *value)
{
	if (rx_fifo >= EQOS_4K) {
		/* Enable HW Flow Control */
		*value |= EQOS_MTL_RXQ_OP_MODE_EHFC;

		switch (rx_fifo) {
		case EQOS_4K:
			/* Update RFD */
			*value &= ~EQOS_MTL_RXQ_OP_MODE_RFD_MASK;
			*value |= (FULL_MINUS_2_5K <<
				   EQOS_MTL_RXQ_OP_MODE_RFD_SHIFT) &
				   EQOS_MTL_RXQ_OP_MODE_RFD_MASK;
			/* Update RFA */
			*value &= ~EQOS_MTL_RXQ_OP_MODE_RFA_MASK;
			*value |= (FULL_MINUS_1_5K <<
				   EQOS_MTL_RXQ_OP_MODE_RFA_SHIFT) &
				   EQOS_MTL_RXQ_OP_MODE_RFA_MASK;
			break;
		case EQOS_8K:
			/* Update RFD */
			*value &= ~EQOS_MTL_RXQ_OP_MODE_RFD_MASK;
			*value |= (FULL_MINUS_4_K <<
				   EQOS_MTL_RXQ_OP_MODE_RFD_SHIFT) &
				   EQOS_MTL_RXQ_OP_MODE_RFD_MASK;
			/* Update RFA */
			*value &= ~EQOS_MTL_RXQ_OP_MODE_RFA_MASK;
			*value |= (FULL_MINUS_6_K <<
				   EQOS_MTL_RXQ_OP_MODE_RFA_SHIFT) &
				   EQOS_MTL_RXQ_OP_MODE_RFA_MASK;
			break;
		case EQOS_9K:
			/* Update RFD */
			*value &= ~EQOS_MTL_RXQ_OP_MODE_RFD_MASK;
			*value |= (FULL_MINUS_3_K <<
				   EQOS_MTL_RXQ_OP_MODE_RFD_SHIFT) &
				   EQOS_MTL_RXQ_OP_MODE_RFD_MASK;
			/* Update RFA */
			*value &= ~EQOS_MTL_RXQ_OP_MODE_RFA_MASK;
			*value |= (FULL_MINUS_2_K <<
				   EQOS_MTL_RXQ_OP_MODE_RFA_SHIFT) &
				   EQOS_MTL_RXQ_OP_MODE_RFA_MASK;
			break;
		case EQOS_16K:
			/* Update RFD */
			*value &= ~EQOS_MTL_RXQ_OP_MODE_RFD_MASK;
			*value |= (FULL_MINUS_4_K <<
				   EQOS_MTL_RXQ_OP_MODE_RFD_SHIFT) &
				   EQOS_MTL_RXQ_OP_MODE_RFD_MASK;
			/* Update RFA */
			*value &= ~EQOS_MTL_RXQ_OP_MODE_RFA_MASK;
			*value |= (FULL_MINUS_10_K <<
				   EQOS_MTL_RXQ_OP_MODE_RFA_SHIFT) &
				   EQOS_MTL_RXQ_OP_MODE_RFA_MASK;
			break;
		case EQOS_32K:
			/* Update RFD */
			*value &= ~EQOS_MTL_RXQ_OP_MODE_RFD_MASK;
			*value |= (FULL_MINUS_4_K <<
				   EQOS_MTL_RXQ_OP_MODE_RFD_SHIFT) &
				   EQOS_MTL_RXQ_OP_MODE_RFD_MASK;
			/* Update RFA */
			*value &= ~EQOS_MTL_RXQ_OP_MODE_RFA_MASK;
			*value |= (FULL_MINUS_16_K <<
				   EQOS_MTL_RXQ_OP_MODE_RFA_SHIFT) &
				   EQOS_MTL_RXQ_OP_MODE_RFA_MASK;
			break;
		default:
			/* Use 9K values */
			/* Update RFD */
			*value &= ~EQOS_MTL_RXQ_OP_MODE_RFD_MASK;
			*value |= (FULL_MINUS_3_K <<
				   EQOS_MTL_RXQ_OP_MODE_RFD_SHIFT) &
				   EQOS_MTL_RXQ_OP_MODE_RFD_MASK;
			/* Update RFA */
			*value &= ~EQOS_MTL_RXQ_OP_MODE_RFA_MASK;
			*value |= (FULL_MINUS_2_K <<
				   EQOS_MTL_RXQ_OP_MODE_RFA_SHIFT) &
				   EQOS_MTL_RXQ_OP_MODE_RFA_MASK;
			break;
		}
	}
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
	osi_writel(value, (unsigned char *)osi_core->base +
		   EQOS_MTL_CHX_TX_OP_MODE(qinx));

	/* read RX Q0 Operating Mode Register */
	value = osi_readl((unsigned char *)osi_core->base +
			  EQOS_MTL_CHX_RX_OP_MODE(qinx));
	value |= (rx_fifo << EQOS_MTL_RXQ_SIZE_SHIFT);
	/* Enable Store and Forward mode */
	value |= EQOS_MTL_RSF;
	/* Update EHFL, RFA and RFD
	 * EHFL: Enable HW Flow Control
	 * RFA: Threshold for Activating Flow Control
	 * RFD: Threshold for Deactivating Flow Control
	 */
	update_ehfc_rfa_rfd(rx_fifo, &value);
	osi_writel(value, (unsigned char *)osi_core->base +
		   EQOS_MTL_CHX_RX_OP_MODE(qinx));

	/* Transmit Queue weight */
	value = osi_readl((unsigned char *)osi_core->base +
			  EQOS_MTL_TXQ_QW(qinx));
	value |= (EQOS_MTL_TXQ_QW_ISCQW + qinx);
	osi_writel(value, (unsigned char *)osi_core->base +
		   EQOS_MTL_TXQ_QW(qinx));

	/* Enable Rx Queue Control */
	value = osi_readl((unsigned char *)osi_core->base +
			  EQOS_MAC_RQC0R);
	value |= ((osi_core->rxq_ctrl[qinx] & 0x3U) << (qinx * 2U));
	osi_writel(value, (unsigned char *)osi_core->base +
		   EQOS_MAC_RQC0R);

	return 0;
}

/**
 *	eqos_config_rxcsum_offload - Enable/Disale rx checksum offload in HW
 *	@addr: EQOS virtual base address.
 *	@enabled: Flag to indicate feature is to be enabled/disabled.
 *
 *	Algorithm:
 *	1) Read the MAC configuration register.
 *	2) Enable the IP checksum offload engine COE in MAC receiver.
 *	3) Update the MAC configuration register.
 *
 *	Dependencies: MAC has to be out of reset.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, -1 - failure.
 */
static int eqos_config_rxcsum_offload(void *addr, unsigned int enabled)
{
	unsigned int mac_mcr;

	if (enabled != OSI_ENABLE && enabled != OSI_DISABLE) {
		return -1;
	}

	mac_mcr = osi_readl((unsigned char *)addr + EQOS_MAC_MCR);

	if (enabled == OSI_ENABLE) {
		mac_mcr |= EQOS_MCR_IPC;
	} else {
		mac_mcr &= ~EQOS_MCR_IPC;
	}

	osi_writel(mac_mcr, (unsigned char *)addr + EQOS_MAC_MCR);

	return 0;
}

/**
 *	eqos_configure_rxq_priority - Configure Priorities Selected in
 *	the Receive Queue
 *
 *	@osi_core: OSI private data structure.
 *
 *	Algorithm: This takes care of mapping user priority to Rx queue.
 *	User provided priority mask updated to register. Valid input can have
 *	all TC(0xFF) in one queue to None(0x00) in rx queue.
 *	The software must ensure that the content of this field is mutually
 *	exclusive to the PSRQ fields for other queues, that is, the same
 *	priority is not mapped to multiple Rx queues.
 *
 *	Dependencies: MAC has to be out of reset.
 *
 *	Protection: None
 *
 *	Return: None
 */
static void eqos_configure_rxq_priority(struct osi_core_priv_data *osi_core)
{
	unsigned int val;
	unsigned int temp;
	unsigned int qinx, mtlq;
	unsigned int pmask = 0x0U;
	unsigned int mfix_var1, mfix_var2;

	if (osi_core->dcs_en == OSI_ENABLE) {
		osd_err(osi_core->osd,
			"Invalid combination of DCS and RxQ-UP mapping, exiting %s()\n",
			__func__);
		return;
	}
	/* make sure EQOS_MAC_RQC2R is reset before programming */
	osi_writel(OSI_DISABLE, (unsigned char *)osi_core->base +
		   EQOS_MAC_RQC2R);

	for (qinx = 0; qinx < osi_core->num_mtl_queues; qinx++) {
		mtlq = osi_core->mtl_queues[qinx];
		/* check for PSRQ field mutual exclusive for all queues */
		if ((osi_core->rxq_prio[mtlq] <= 0xFFU) &&
		    (osi_core->rxq_prio[mtlq] > 0x0U) &&
		    ((pmask & osi_core->rxq_prio[mtlq]) == 0U)) {
			pmask |= osi_core->rxq_prio[mtlq];
			temp = osi_core->rxq_prio[mtlq];
		} else {
			osd_err(osi_core->osd,
				"Invalid rxq Priority for Q(%d)\n",
				mtlq);
			continue;

		}

		val = osi_readl((unsigned char *)osi_core->base +
				EQOS_MAC_RQC2R);
		mfix_var1 = mtlq * (unsigned int)EQOS_MAC_RQC2_PSRQ_SHIFT;
		mfix_var2 = (unsigned int)EQOS_MAC_RQC2_PSRQ_MASK;
		mfix_var2 <<= mfix_var1;
		val &= ~mfix_var2;
		temp = temp << (mtlq * EQOS_MAC_RQC2_PSRQ_SHIFT);
		mfix_var1 = mtlq * (unsigned int)EQOS_MAC_RQC2_PSRQ_SHIFT;
		mfix_var2 = (unsigned int)EQOS_MAC_RQC2_PSRQ_MASK;
		mfix_var2 <<= mfix_var1;
		val |= (temp & mfix_var2);
		/* Priorities Selected in the Receive Queue 0 */
		osi_writel(val, (unsigned char *)osi_core->base +
			   EQOS_MAC_RQC2R);
	}
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
	/* Enable Rx checksum offload engine by default */
	value |= EQOS_MCR_ACS | EQOS_MCR_CST | EQOS_MCR_DM | EQOS_MCR_IPC;

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
	value |= EQOS_MAC_RQC1R_MCBCQEN;
	/* Routing Multicast and Broadcast to Q1 */
	value |= EQOS_MAC_RQC1R_MCBCQ1;
	osi_writel(value, (unsigned char *)osi_core->base + EQOS_MAC_RQC1R);

	/* Disable all MMC interrupts */
	/* Disable all MMC Tx Interrupts */
	osi_writel(0xFFFFFFFFU, (unsigned char *)osi_core->base +
		   EQOS_MMC_TX_INTR_MASK);
	/* Disable all MMC RX interrupts */
	osi_writel(0xFFFFFFFFU, (unsigned char *)osi_core->base +
		   EQOS_MMC_RX_INTR_MASK);
	/* Disable MMC Rx interrupts for IPC */
	osi_writel(0xFFFFFFFFU, (unsigned char *)osi_core->base +
		   EQOS_MMC_IPC_RX_INTR_MASK);

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

	/* Configure default flow control settings */
	if (osi_core->pause_frames == OSI_PAUSE_FRAMES_ENABLE) {
		osi_core->flow_ctrl = (OSI_FLOW_CTRL_TX | OSI_FLOW_CTRL_RX);
		if (eqos_config_flow_control(osi_core->base,
					     osi_core->flow_ctrl) != 0) {
			osd_err(osi_core->osd, "Failed to set flow control"
				" configuration\n");
		}
	}
	/* USP (user Priority) to RxQ Mapping */
	eqos_configure_rxq_priority(osi_core);
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
	osi_writel(EQOS_MMC_CNTRL_CNTRST, (unsigned char *)osi_core->base +
		   EQOS_MMC_CNTRL);

	/* Mapping MTL Rx queue and DMA Rx channel */
	/* TODO: Need to add EQOS_MTL_RXQ_DMA_MAP1 for EQOS */
	value = osi_readl((unsigned char *)osi_core->base +
			  EQOS_MTL_RXQ_DMA_MAP0);
	if (osi_core->dcs_en == OSI_ENABLE) {
		value |= EQOS_RXQ_TO_DMA_CHAN_MAP_DCS_EN;
	} else {
		value |= EQOS_RXQ_TO_DMA_CHAN_MAP;
	}

	osi_writel(value, (unsigned char *)osi_core->base +
		   EQOS_MTL_RXQ_DMA_MAP0);

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
	} else if ((mac_pcs & EQOS_MAC_PCS_LNKSPEED) ==
		   EQOS_MAC_PCS_LNKSPEED_100) {
		eqos_set_speed(osi_core->base, OSI_SPEED_100);
	} else if ((mac_pcs & EQOS_MAC_PCS_LNKSPEED) ==
		   EQOS_MAC_PCS_LNKSPEED_1000) {
		eqos_set_speed(osi_core->base, OSI_SPEED_1000);
	} else {
		/* Nothing here */
	}
}

/**
 *	update_dma_sr_stats - stats for dma_status error
 *	@osi_core: OSI core private data structure.
 *	@dma_sr: Dma status register read value
 *	@qinx: Queue index
 *
 *	Algorithm: increament error stats based on corresponding bit filed.
 *
 *	Dependencies: None
 *
 *	Protection: None.
 *
 *	Return: None.
 */
static inline void update_dma_sr_stats(struct osi_core_priv_data *osi_core,
				       unsigned int dma_sr, unsigned int qinx)
{
	unsigned long val;

	if ((dma_sr & EQOS_DMA_CHX_STATUS_RBU) == EQOS_DMA_CHX_STATUS_RBU) {
		val = osi_core->xstats.rx_buf_unavail_irq_n[qinx];
		osi_core->xstats.rx_buf_unavail_irq_n[qinx] =
			osi_update_stats_counter(val, 1U);
	}
	if ((dma_sr & EQOS_DMA_CHX_STATUS_TPS) == EQOS_DMA_CHX_STATUS_TPS) {
		val = osi_core->xstats.tx_proc_stopped_irq_n[qinx];
		osi_core->xstats.tx_proc_stopped_irq_n[qinx] =
			osi_update_stats_counter(val, 1U);
	}
	if ((dma_sr & EQOS_DMA_CHX_STATUS_TBU) == EQOS_DMA_CHX_STATUS_TBU) {
		val = osi_core->xstats.tx_buf_unavail_irq_n[qinx];
		osi_core->xstats.tx_buf_unavail_irq_n[qinx] =
			osi_update_stats_counter(val, 1U);
	}
	if ((dma_sr & EQOS_DMA_CHX_STATUS_RPS) == EQOS_DMA_CHX_STATUS_RPS) {
		val = osi_core->xstats.rx_proc_stopped_irq_n[qinx];
		osi_core->xstats.rx_proc_stopped_irq_n[qinx] =
			osi_update_stats_counter(val, 1U);
	}
	if ((dma_sr & EQOS_DMA_CHX_STATUS_RWT) == EQOS_DMA_CHX_STATUS_RWT) {
		val = osi_core->xstats.rx_watchdog_irq_n;
		osi_core->xstats.rx_watchdog_irq_n =
			osi_update_stats_counter(val, 1U);
	}
	if ((dma_sr & EQOS_DMA_CHX_STATUS_FBE) == EQOS_DMA_CHX_STATUS_FBE) {
		val = osi_core->xstats.fatal_bus_error_irq_n;
		osi_core->xstats.fatal_bus_error_irq_n =
			osi_update_stats_counter(val, 1U);
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
			dma_sr = osi_readl((unsigned char *)base +
					   EQOS_DMA_CHX_STATUS(qinx));
			/* read dma channel interrupt enable register */
			dma_ier = osi_readl((unsigned char *)base +
					    EQOS_DMA_CHX_IER(qinx));

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
			osi_writel(dma_sr, (unsigned char *)base +
				   EQOS_DMA_CHX_STATUS(qinx));
			update_dma_sr_stats(osi_core, dma_sr, qinx);
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

/*
 *	eqos_config_mac_pkt_filter_reg - configure mac filter register.
 *	@osi_core: OSI private data structure.
 *	@filter: OSI filter structure.
 *
 *	Algorithm: This sequence is used to configure MAC in differnet pkt
 *	processing modes like promiscuous, multicast, unicast,
 *	hash unicast/multicast.
 *
 *	Dependencies: MAC IP should be out of reset
 *	and need to be initialized as the requirements
 *
 *	Protection: None
 *
 *	Return: None
 */
static void eqos_config_mac_pkt_filter_reg(struct osi_core_priv_data *osi_core,
					   struct osi_filter pfilter)
{
	unsigned int value = 0U;

	value = osi_readl((unsigned char *)osi_core->base + EQOS_MAC_PFR);
	/*Retain all other values */
	value &= (EQOS_MAC_PFR_DAIF | EQOS_MAC_PFR_DBF | EQOS_MAC_PFR_SAIF |
		  EQOS_MAC_PFR_SAF | EQOS_MAC_PFR_PCF | EQOS_MAC_PFR_VTFE |
		  EQOS_MAC_PFR_IPFE | EQOS_MAC_PFR_DNTU | EQOS_MAC_PFR_RA);
	value |= (pfilter.pr_mode & EQOS_MAC_PFR_PR) |
		  ((pfilter.huc_mode << EQOS_MAC_PFR_HUC_SHIFT) &
		   EQOS_MAC_PFR_HUC) |
		  ((pfilter.hmc_mode << EQOS_MAC_PFR_HMC_SHIFT) &
		   EQOS_MAC_PFR_HMC) |
		  ((pfilter.pm_mode << EQOS_MAC_PFR_PM_SHIFT) &
		   EQOS_MAC_PFR_PM) |
		  ((pfilter.hpf_mode << EQOS_MAC_PFR_HPF_SHIFT) &
		   EQOS_MAC_PFR_HPF);

	osi_writel(value, (unsigned char *)osi_core->base + EQOS_MAC_PFR);
}

/**
 *	eqos_update_mac_addr_low_high_reg- Update L2 address
 *	in filter register
 *
 *	@osi_core: OSI private data structure.
 *	@index: filter index
 *	@value: MAC address to write
 *	@dma_routing_enable: dma channel routing enable(1)
 *	@dma_chan: dma channel number
 *	@addr_mask: filter will not consider byte in comparison
 *	Bit 29: MAC_Address${i}_High[15:8]
 *	Bit 28: MAC_Address${i}_High[7:0]
 *	Bit 27: MAC_Address${i}_Low[31:24]
 *	..
 *	Bit 24: MAC_Address${i}_Low[7:0]
 *	@src_dest: SA(1) or DA(0)
 *
 *	Algorithm: This routine update MAC address to register for filtering
 *	based on dma_routing_enable, addr_mask and src_dest. Validation of
 *	dma_chan as well as DCS bit enabled in RXQ to DMA mapping register
 *	performed before updating DCS bits.
 *
 *	Dependencies: MAC IP should be out of reset
 *	and need to be initialized as the requirements
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure.
 */
static int eqos_update_mac_addr_low_high_reg(
				struct osi_core_priv_data *osi_core,
				unsigned int idx, unsigned char addr[],
				unsigned int dma_routing_enable,
				unsigned int dma_chan,
				unsigned int addr_mask, unsigned int src_dest)
{
	unsigned int value = 0x0U;

	if (idx > EQOS_MAX_MAC_ADDRESS_FILTER) {
		osd_err(osi_core->osd, "invalid MAC filter index\n");
		return -1;
	}

	/* High address clean should happen for filter index > 0 */
	if (idx >= 0x1U && addr == OSI_NULL) {
		osi_writel((unsigned int)0x0, (unsigned char *)osi_core->base +
			   EQOS_MAC_ADDRH((idx)));
		return 0;
	}

	/* PDC bit of MAC_Ext_Configuration register is not set so binary
	 * value representation.
	 */
	if ((dma_routing_enable == OSI_ENABLE) &&
	    (dma_chan < OSI_EQOS_MAX_NUM_CHANS) &&
	    (osi_core->dcs_en == OSI_ENABLE)) {
		value = ((dma_chan << EQOS_MAC_ADDRH_DCS_SHIFT) &
			 EQOS_MAC_ADDRH_DCS);
	} else if ((dma_routing_enable == OSI_ENABLE) &&
		   (dma_chan < OSI_EQOS_MAX_NUM_CHANS) &&
		   (osi_core->dcs_en != OSI_ENABLE)) {
		osd_err(osi_core->osd, "DCS disabled, please update DT\n");
	} else {
		/* Do nothing */
	}

	/* Address mask is valid for address 1 to 31 index only */
	if ((addr_mask <= 0x3FU && addr_mask > 0U) && (idx > 0U && idx < 32U)) {
		value = (value | ((addr_mask << EQOS_MAC_ADDRH_MBC_SHIFT) &
			EQOS_MAC_ADDRH_MBC));
	}

	/* Setting Source/Destination Address match valid for 1 to 32 index */
	if ((idx > 0U && idx < 32U) && (src_dest == OSI_SA_MATCH ||
					src_dest == OSI_DA_MATCH)) {
		value = (value | ((src_dest << EQOS_MAC_ADDRH_SA_SHIFT) &
			EQOS_MAC_ADDRH_SA));
	}

	osi_writel(((unsigned int)addr[4] |
		   ((unsigned int)addr[5] << 8) | OSI_BIT(31) | value),
		   (unsigned char *)osi_core->base + EQOS_MAC_ADDRH((idx)));

	osi_writel(((unsigned int)addr[0] | ((unsigned int)addr[1] << 8) |
		   ((unsigned int)addr[2] << 16) |
		   ((unsigned int)addr[3] << 24)),
		   (unsigned char *)osi_core->base +  EQOS_MAC_ADDRL((idx)));

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
	value = (value & EQOS_MTL_TXQEN_MASK) >> EQOS_MTL_TXQEN_MASK_SHIFT;
	avb->oper_mode = value;

	/* Get Algo and Credit control */
	value = osi_readl((unsigned char *)osi_core->base +
			  EQOS_MTL_TXQ_ETS_CR(qinx));
	avb->credit_control = (value & EQOS_MTL_TXQ_ETS_CR_CC) >>
		   EQOS_MTL_TXQ_ETS_CR_CC_SHIFT;
	avb->algo = (value & EQOS_MTL_TXQ_ETS_CR_AVALG) >>
		     EQOS_MTL_TXQ_ETS_CR_AVALG_SHIFT;

	/* Get Send slope credit */
	value = osi_readl((unsigned char *)osi_core->base +
			  EQOS_MTL_TXQ_ETS_SSCR(qinx));
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

/**
 *	eqos_config_arp_offload - Enable/Disable ARP offload
 *	@mac_ver: MAC version number (different MAC HW version
 *	need different register offset/fields for ARP offload.
 *	@addr: EQOS virtual base address.
 *	@enable: Flag variable to enable/disable ARP offload
 *	@ip_addr: IP address of device to be programmed in HW.
 *	HW will use this IP address to respond to ARP requests.
 *
 *	Algorithm:
 *	1) Read the MAC configuration register
 *	2) If ARP offload is to be enabled, program the IP address in
 *	ARPPA register
 *	3) Enable/disable the ARPEN bit in MCR and write back to the MCR.
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, -1 - failure.
 */
static int eqos_config_arp_offload(unsigned int mac_ver, void *addr,
				   unsigned int enable,
				   unsigned char *ip_addr)
{
	unsigned int mac_mcr;
	unsigned int val;

	if (enable != OSI_ENABLE && enable != OSI_DISABLE) {
		return -1;
	}

	mac_mcr = osi_readl((unsigned char *)addr + EQOS_MAC_MCR);

	if (enable == OSI_ENABLE) {
		val = (((unsigned int)ip_addr[0]) << 24) |
		      (((unsigned int)ip_addr[1]) << 16) |
		      (((unsigned int)ip_addr[2]) << 8) |
		      (((unsigned int)ip_addr[3]));

		if (mac_ver == OSI_EQOS_MAC_4_10) {
			osi_writel(val, (unsigned char *)addr +
				   EQOS_4_10_MAC_ARPPA);
		} else if (mac_ver == OSI_EQOS_MAC_5_00) {
			osi_writel(val, (unsigned char *)addr +
				   EQOS_5_00_MAC_ARPPA);
		} else {
			/* Unsupported MAC ver */
			return -1;
		}

		mac_mcr |= EQOS_MCR_ARPEN;
	} else {
		mac_mcr &= ~EQOS_MCR_ARPEN;
	}

	osi_writel(mac_mcr, (unsigned char *)addr + EQOS_MAC_MCR);

	return 0;
}

/*
 *	eqos_config_l3_l4_filter_enable -  register write to eanble L3/L4
 *	filters.
 *
 *	@base: Base address  from OSI private data structure.
 *	@enable: enable/disable
 *
 *	Algorithm: This routine to enable/disable L4/l4 filter
 *
 *	Dependencies: MAC IP should be out of reset
 *	and need to be initialized as the requirements
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure.
 */
static int eqos_config_l3_l4_filter_enable(void *base,
					   unsigned int filter_enb_dis)
{
	unsigned int value = 0U;

	value = osi_readl((unsigned char *)base + EQOS_MAC_PFR);
	value &= ~(EQOS_MAC_PFR_IPFE);
	value |= ((filter_enb_dis << 20) & EQOS_MAC_PFR_IPFE);
	osi_writel(value, (unsigned char *)base + EQOS_MAC_PFR);

	return 0;
}

/**
 *	eqos_config_l2_da_perfect_inverse_match - configure register for inverse
 *	or perfect match.
 *
 *	@base: Base address  from OSI private data structure.
 *	@perfect_inverse_match: 1 - inverse mode 0- normal mode
 *
 *	Algorithm: This sequence is used to select perfect/inverse matching
 *	for L2 DA
 *
 *	Dependencies: MAC IP should be out of reset
 *	and need to be initialized as the requirements
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure.
 */
static int eqos_config_l2_da_perfect_inverse_match(void *base, unsigned int
						   perfect_inverse_match)
{
	unsigned int value = 0U;

	value = osi_readl((unsigned char *)base + EQOS_MAC_PFR);
	value &= ~EQOS_MAC_PFR_DAIF;
	value |= ((perfect_inverse_match << EQOS_MAC_PFR_DAIF_SHIFT) &
		  EQOS_MAC_PFR_DAIF);
	osi_writel(value, (unsigned char *)base + EQOS_MAC_PFR);

	return 0;
}

/**
 *	eqos_update_ip4_addr
 *	@osi_core: OSI private data structure.
 *	@filter_no: filter index
 *	@addr: ipv4 address
 *	@src_dst_addr_match: 0 - source addr otherwise - dest addr
 *
 *	Algorithm:  This sequence is used to update IPv4 source/destination
 *	Address for L3 layer filtering
 *
 *	Dependencies: MAC IP should be out of reset
 *	and need to be initialized as the requirements
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure.
 */
static int eqos_update_ip4_addr(struct osi_core_priv_data *osi_core,
				unsigned int filter_no,
				unsigned char addr[],
				unsigned int src_dst_addr_match)
{
	void *base = osi_core->base;
	unsigned int value = 0U;
	unsigned int temp = 0U;

	if (filter_no > EQOS_MAX_L3_L4_FILTER) {
		osd_err(osi_core->osd, "filter index %d > %d for L3/L4 filter\n"
			, filter_no, EQOS_MAX_L3_L4_FILTER);
		return -1;
	}

	value = addr[3];
	temp = (unsigned int)addr[2] << 8;
	value |= temp;
	temp = (unsigned int)addr[1] << 16;
	value |= temp;
	temp = (unsigned int)addr[0] << 24;
	value |= temp;
	if (src_dst_addr_match == OSI_SOURCE_MATCH) {
		osi_writel(value, (unsigned char *)base +
			   EQOS_MAC_L3_AD0R(filter_no));
	} else {
		osi_writel(value, (unsigned char *)base +
			   EQOS_MAC_L3_AD1R(filter_no));
	}

	return 0;
}

/**
 *	eqos_update_ip6_addr - add ipv6 address in register
 *
 *	@osi_core: OSI private data structure.
 *	@filter_no: filter index
 *	@addr: ipv6 adderss
 *
 *	Algorithm:  This sequence is used to update IPv6 source/destination
 *	Address for L3 layer filtering
 *
 *	Dependencies: MAC IP should be out of reset
 *	and need to be initialized as the requirements
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure.
 */
static int eqos_update_ip6_addr(struct osi_core_priv_data *osi_core,
				unsigned int filter_no, unsigned short addr[])
{
	void *base = osi_core->base;
	unsigned int value = 0U;
	unsigned int temp = 0U;

	if (filter_no > EQOS_MAX_L3_L4_FILTER) {
		osd_err(osi_core->osd, "filter index %d > %d for L3/L4 filter\n"
			, filter_no, EQOS_MAX_L3_L4_FILTER);
		return -1;
	}

	/* update Bits[31:0] of 128-bit IP addr */
	value = addr[7];
	temp = (unsigned int)addr[6] << 16;
	value |= temp;
	osi_writel(value, (unsigned char *)base +
		    EQOS_MAC_L3_AD0R(filter_no));
	/* update Bits[63:32] of 128-bit IP addr */
	value = addr[5];
	temp = (unsigned int)addr[4] << 16;
	value |= temp;
	osi_writel(value, (unsigned char *)base +
		    EQOS_MAC_L3_AD1R(filter_no));
	/* update Bits[95:64] of 128-bit IP addr */
	value = addr[3];
	temp = (unsigned int)addr[2] << 16;
	value |= temp;
	osi_writel(value, (unsigned char *)base +
		   EQOS_MAC_L3_AD2R(filter_no));
	/* update Bits[127:96] of 128-bit IP addr */
	value = addr[1];
	temp = (unsigned int)addr[0] << 16;
	value |= temp;
	osi_writel(value, (unsigned char *)base +
		   EQOS_MAC_L3_AD3R(filter_no));

	return 0;
}

/**
 *	eqos_update_l4_port_no -program source  port no
 *
 *	@osi_core: OSI private data structure.
 *	@filter_no: filter index
 *	@port_no: port number
 *	@src_dst_port_match: 0 - source port, otherwise - dest port
 *
 *	Algorithm: sequence is used to update Source Port Number for
 *	L4(TCP/UDP) layer filtering.
 *
 *	Dependencies:
 *	1) MAC IP should be out of reset and need to be initialized
 *	as the requirements
 *	2) DCS bits should be enabled in RXQ to DMA mapping register
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure.
 */
static int eqos_update_l4_port_no(struct osi_core_priv_data *osi_core,
				  unsigned int filter_no,
				  unsigned short port_no,
				  unsigned int src_dst_port_match)
{
	void *base = osi_core->base;
	unsigned int value = 0U;
	unsigned int temp = 0U;

	if (filter_no > EQOS_MAX_L3_L4_FILTER) {
		osd_err(osi_core->osd, "filter index %d > %d for L3/L4 filter\n"
			, filter_no, EQOS_MAX_L3_L4_FILTER);
		return -1;
	}

	value = osi_readl((unsigned char *)base + EQOS_MAC_L4_ADR(filter_no));
	if (src_dst_port_match == OSI_SOURCE_MATCH) {
		value &= ~EQOS_MAC_L4_SP_MASK;
		value |= ((unsigned int)port_no  & EQOS_MAC_L4_SP_MASK);
	} else {
		value &= ~EQOS_MAC_L4_DP_MASK;
		temp = port_no;
		value |= ((temp << EQOS_MAC_L4_DP_SHIFT) & EQOS_MAC_L4_DP_MASK);
	}
	osi_writel(value, (unsigned char *)base +  EQOS_MAC_L4_ADR(filter_no));

	return 0;
}

/**
 *	eqos_set_dcs - check and update dma routing register
 *
 *	@osi_core: OSI private data structure.
 *	@value: unsigned int value for caller
 *	@dma_routing_enable: filter based dma routing enable(1)
 *	@dma_chan: dma channel for routing based on filter
 *
 *	Algorithm: Check for request for DCS_enable as well as validate chan
 *	number and dcs_enable is set. After validation, this sequence is used
 *	to configure L3((IPv4/IPv6) filters for address matching.
 *
 *	Dependencies:
 *	1) MAC IP should be out of reset and need to be initialized
 *	as the requirements.
 *	2) DCS bits should be enabled in RXQ to DMA mapping register
 *
 *	Protection: None
 *
 *	Return: updated unsigned int value
 */
static inline unsigned int eqos_set_dcs(struct osi_core_priv_data *osi_core,
					unsigned int value,
					unsigned int dma_routing_enable,
					unsigned int dma_chan)
{
	if ((dma_routing_enable == OSI_ENABLE) && (dma_chan <
	    OSI_EQOS_MAX_NUM_CHANS) && (osi_core->dcs_en ==
	    OSI_ENABLE)) {
		value |= ((dma_routing_enable <<
			  EQOS_MAC_L3L4_CTR_DMCHEN0_SHIFT) &
			  EQOS_MAC_L3L4_CTR_DMCHEN0);
		value |= ((dma_chan <<
			  EQOS_MAC_L3L4_CTR_DMCHN0_SHIFT) &
			  EQOS_MAC_L3L4_CTR_DMCHN0);
	} else if ((dma_routing_enable == OSI_ENABLE) && (dma_chan <
	    OSI_EQOS_MAX_NUM_CHANS) && (osi_core->dcs_en !=
	    OSI_ENABLE)) {
		osd_err(osi_core->osd, "DCS disabled, please update DT\n");
	} else {
		/* do noting */
	}

	return value;
}
/**
 *	eqos_config_l3_filters - config L3 filters.
 *
 *	@osi_core: OSI private data structure.
 *	@filter_no: filter index
 *	@enb_dis:  1 - enable otherwise - disable L3 filter
 *	@ipv4_ipv6_match: 1 - IPv6, otherwise - IPv4
 *	@src_dst_addr_match: 0 - source, otherwise - destination
 *	@perfect_inverse_match: normal match(0) or inverse map(1)
 *	@dma_routing_enable: filter based dma routing enable(1)
 *	@dma_chan: dma channel for routing based on filter
 *
 *	Algorithm: Check for DCS_enable as well as validate channel
 *	number and if dcs_enable is set. After validation, code flow
 *	is used to configure L3((IPv4/IPv6) filters resister
 *	for address matching.
 *
 *	Dependencies:
 *	1) MAC IP should be out of reset and need to be initialized
 *	as the requirements
 *	2) DCS bits should be enabled in RXQ to DMA map register
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure.
 */
static int eqos_config_l3_filters(struct osi_core_priv_data *osi_core,
				  unsigned int filter_no,
				  unsigned int enb_dis,
				  unsigned int ipv4_ipv6_match,
				  unsigned int src_dst_addr_match,
				  unsigned int perfect_inverse_match,
				  unsigned int dma_routing_enable,
				  unsigned int dma_chan)
{
	unsigned int value = 0U;
	void *base = osi_core->base;

	if (filter_no > EQOS_MAX_L3_L4_FILTER) {
		osd_err(osi_core->osd, "filter index %d > %d for L3/L4 filter\n"
			, filter_no, EQOS_MAX_L3_L4_FILTER);
		return -1;
	}

	value = osi_readl((unsigned char *)base +
			  EQOS_MAC_L3L4_CTR(filter_no));
	value &= ~EQOS_MAC_L3L4_CTR_L3PEN0;
	value |= (ipv4_ipv6_match  & EQOS_MAC_L3L4_CTR_L3PEN0);
	osi_writel(value, (unsigned char *)base +
		   EQOS_MAC_L3L4_CTR(filter_no));

	/* For IPv6 either SA/DA can be checked not both */
	if (ipv4_ipv6_match == OSI_IPV6_MATCH) {
		if (enb_dis == OSI_ENABLE) {
			if (src_dst_addr_match == OSI_SOURCE_MATCH) {
				/* Enable L3 filters for IPv6 SOURCE addr
				 *  matching
				 */
				value = osi_readl((unsigned char *)base +
						  EQOS_MAC_L3L4_CTR(filter_no));
				value &= ~EQOS_MAC_L3_IP6_CTRL_CLEAR;
				value |= ((EQOS_MAC_L3L4_CTR_L3SAM0 |
					  perfect_inverse_match <<
					  EQOS_MAC_L3L4_CTR_L3SAI_SHIFT) &
					  ((EQOS_MAC_L3L4_CTR_L3SAM0 |
					  EQOS_MAC_L3L4_CTR_L3SAIM0)));
				value |= eqos_set_dcs(osi_core, value,
						      dma_routing_enable,
						      dma_chan);
				osi_writel(value, (unsigned char *)base +
					   EQOS_MAC_L3L4_CTR(filter_no));

			} else {
				/* Enable L3 filters for IPv6 DESTINATION addr
				 * matching
				 */
				value = osi_readl((unsigned char *)base +
						  EQOS_MAC_L3L4_CTR(filter_no));
				value &= ~EQOS_MAC_L3_IP6_CTRL_CLEAR;
				value |= ((EQOS_MAC_L3L4_CTR_L3DAM0 |
					  perfect_inverse_match <<
					  EQOS_MAC_L3L4_CTR_L3DAI_SHIFT) &
					  ((EQOS_MAC_L3L4_CTR_L3DAM0 |
					  EQOS_MAC_L3L4_CTR_L3DAIM0)));
				value |= eqos_set_dcs(osi_core, value,
						      dma_routing_enable,
						      dma_chan);
				osi_writel(value, (unsigned char *)base +
					   EQOS_MAC_L3L4_CTR(filter_no));
			}
		} else {
			/* Disable L3 filters for IPv6 SOURCE/DESTINATION addr
			 * matching
			 */
			value = osi_readl((unsigned char *)base +
					  EQOS_MAC_L3L4_CTR(filter_no));
			value &= ~(EQOS_MAC_L3_IP6_CTRL_CLEAR |
				   EQOS_MAC_L3L4_CTR_L3PEN0);
			osi_writel(value, (unsigned char *)base +
				   EQOS_MAC_L3L4_CTR(filter_no));
		}
	} else {
		if (src_dst_addr_match == OSI_SOURCE_MATCH) {
			if (enb_dis == OSI_ENABLE) {
				/* Enable L3 filters for IPv4 SOURCE addr
				 * matching
				 */
				value = osi_readl((unsigned char *)base +
						  EQOS_MAC_L3L4_CTR(filter_no));
				value &= ~EQOS_MAC_L3_IP4_SA_CTRL_CLEAR;
				value |= ((EQOS_MAC_L3L4_CTR_L3SAM0 |
					  perfect_inverse_match <<
					  EQOS_MAC_L3L4_CTR_L3SAI_SHIFT) &
					  ((EQOS_MAC_L3L4_CTR_L3SAM0 |
					  EQOS_MAC_L3L4_CTR_L3SAIM0)));
				value |= eqos_set_dcs(osi_core, value,
						      dma_routing_enable,
						      dma_chan);
				osi_writel(value, (unsigned char *)base +
					   EQOS_MAC_L3L4_CTR(filter_no));
			} else {
				/* Disable L3 filters for IPv4 SOURCE addr
				 * matching
				 */
				value = osi_readl((unsigned char *)base +
						  EQOS_MAC_L3L4_CTR(filter_no));
				value &= ~EQOS_MAC_L3_IP4_SA_CTRL_CLEAR;
				osi_writel(value, (unsigned char *)base +
					   EQOS_MAC_L3L4_CTR(filter_no));
			}
		} else {
			if (enb_dis == OSI_ENABLE) {
				/* Enable L3 filters for IPv4 DESTINATION addr
				 * matching
				 */
				value = osi_readl((unsigned char *)base +
						  EQOS_MAC_L3L4_CTR(filter_no));
				value &= ~EQOS_MAC_L3_IP4_DA_CTRL_CLEAR;
				value |= ((EQOS_MAC_L3L4_CTR_L3DAM0 |
					  perfect_inverse_match <<
					  EQOS_MAC_L3L4_CTR_L3DAI_SHIFT) &
					  ((EQOS_MAC_L3L4_CTR_L3DAM0 |
					  EQOS_MAC_L3L4_CTR_L3DAIM0)));
				value |= eqos_set_dcs(osi_core, value,
						      dma_routing_enable,
						      dma_chan);
				osi_writel(value, (unsigned char *)base +
					   EQOS_MAC_L3L4_CTR(filter_no));
			} else {
				/* Disable L3 filters for IPv4 DESTINATION addr
				 * matching
				 */
				value = osi_readl((unsigned char *)base +
						  EQOS_MAC_L3L4_CTR(filter_no));
				value &= ~EQOS_MAC_L3_IP4_DA_CTRL_CLEAR;
				osi_writel(value, (unsigned char *)base +
					   EQOS_MAC_L3L4_CTR(filter_no));
			}
		}
	}

	return 0;
}

/**
 *	osi_config_l4_filters - Config L4 filters.
 *
 *	@osi_core: OSI private data structure.
 *	@filter_no: filter index
 *	@enb_dis: 1 - enable, otherwise - disable L4 filter
 *	@tcp_udp_match: 1 - udp, 0 - tcp
 *	@src_dst_port_match: 0 - source port, otherwise - dest port
 *	@perfect_inverse_match: normal match(0) or inverse map(1)
 *	@dma_routing_enable: filter based dma routing enable(1)
 *	@dma_chan: dma channel for routing based on filter
 *
 *	Algorithm: This sequence is used to configure L4(TCP/UDP) filters for
 *	SA and DA Port Number matching
 *
 *	Dependencies: MAC IP should be out of reset
 *	and need to be initialized as the requirements
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure.
 */
static int eqos_config_l4_filters(struct osi_core_priv_data *osi_core,
				  unsigned int filter_no,
				  unsigned int enb_dis,
				  unsigned int tcp_udp_match,
				  unsigned int src_dst_port_match,
				  unsigned int perfect_inverse_match,
				  unsigned int dma_routing_enable,
				  unsigned int dma_chan)
{
	void *base = osi_core->base;
	unsigned int value = 0U;

	if (filter_no > EQOS_MAX_L3_L4_FILTER) {
		osd_err(osi_core->osd, "filter index %d > %d for L3/L4 filter\n"
			, filter_no, EQOS_MAX_L3_L4_FILTER);
		return -1;
	}

	value = osi_readl((unsigned char *)base + EQOS_MAC_L3L4_CTR(filter_no));
	value &= ~EQOS_MAC_L3L4_CTR_L4PEN0;
	value |= ((tcp_udp_match << 16) & EQOS_MAC_L3L4_CTR_L4PEN0);
	osi_writel(value, (unsigned char *)base +
		   EQOS_MAC_L3L4_CTR(filter_no));

	if (src_dst_port_match == OSI_SOURCE_MATCH) {
		if (enb_dis == OSI_ENABLE) {
			/* Enable L4 filters for SOURCE Port No matching */
			value = osi_readl((unsigned char *)base +
					  EQOS_MAC_L3L4_CTR(filter_no));
			value &= ~EQOS_MAC_L4_SP_CTRL_CLEAR;
			value |= ((EQOS_MAC_L3L4_CTR_L4SPM0 |
				  perfect_inverse_match <<
				  EQOS_MAC_L3L4_CTR_L4SPI_SHIFT) &
				  (EQOS_MAC_L3L4_CTR_L4SPM0 |
				  EQOS_MAC_L3L4_CTR_L4SPIM0));
			value |= eqos_set_dcs(osi_core, value,
					      dma_routing_enable,
					      dma_chan);
			osi_writel(value, (unsigned char *)base +
				   EQOS_MAC_L3L4_CTR(filter_no));
		} else {
			/* Disable L4 filters for SOURCE Port No matching  */
			value = osi_readl((unsigned char *)base +
					  EQOS_MAC_L3L4_CTR(filter_no));
			value &= ~EQOS_MAC_L4_SP_CTRL_CLEAR;
			osi_writel(value, (unsigned char *)base +
				   EQOS_MAC_L3L4_CTR(filter_no));
		}
	} else {
		if (enb_dis == OSI_ENABLE) {
			/* Enable L4 filters for DESTINATION port No
			 * matching
			 */
			value = osi_readl((unsigned char *)base +
					  EQOS_MAC_L3L4_CTR(filter_no));
			value &= ~EQOS_MAC_L4_DP_CTRL_CLEAR;
			value |= ((EQOS_MAC_L3L4_CTR_L4DPM0 |
				  perfect_inverse_match <<
				  EQOS_MAC_L3L4_CTR_L4DPI_SHIFT) &
				  (EQOS_MAC_L3L4_CTR_L4DPM0 |
				  EQOS_MAC_L3L4_CTR_L4DPIM0));
			value |= eqos_set_dcs(osi_core, value,
					      dma_routing_enable,
					      dma_chan);
			osi_writel(value, (unsigned char *)base +
				   EQOS_MAC_L3L4_CTR(filter_no));
		} else {
			/* Disable L4 filters for DESTINATION port No
			 * matching
			 */
			value = osi_readl((unsigned char *)base +
					  EQOS_MAC_L3L4_CTR(filter_no));
			value &= ~EQOS_MAC_L4_DP_CTRL_CLEAR;
			osi_writel(value, (unsigned char *)base +
				   EQOS_MAC_L3L4_CTR(filter_no));
		}
	}

	return 0;
}

/**
 *	eqos_config_vlan_filter_reg - config vlan filter register
 *
 *	@base: Base address  from OSI private data structure.
 *	@filter_enb_dis: vlan filter enable/disable
 *	@perfect_hash_filtering: perfect or hash filter
 *	@perfect_inverse_match: normal or inverse filter
 *
 *	Algorithm: This sequence is used to enable/disable VLAN filtering and
 *	also selects VLAN filtering mode- perfect/hash
 *
 *	Dependencies: MAC IP should be out of reset
 *	and need to be initialized as the requirements
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure.
 */
static int eqos_config_vlan_filtering(struct osi_core_priv_data *osi_core,
				      unsigned int filter_enb_dis,
				      unsigned int perfect_hash_filtering,
				      unsigned int perfect_inverse_match)
{
	unsigned int value;
	void *base = osi_core->base;

	value = osi_readl((unsigned char *)base + EQOS_MAC_PFR);
	value &= ~(EQOS_MAC_PFR_VTFE);
	value |= ((filter_enb_dis << EQOS_MAC_PFR_SHIFT) & EQOS_MAC_PFR_VTFE);
	osi_writel(value, (unsigned char *)base + EQOS_MAC_PFR);

	value = osi_readl((unsigned char *)base + EQOS_MAC_VLAN_TR);
	value &= ~(EQOS_MAC_VLAN_TR_VTIM | EQOS_MAC_VLAN_TR_VTHM);
	value |= ((perfect_inverse_match << EQOS_MAC_VLAN_TR_VTIM_SHIFT) &
		  EQOS_MAC_VLAN_TR_VTIM);
	if (perfect_hash_filtering == OSI_HASH_FILTER_MODE) {
		osd_err(osi_core->osd, "VLAN hash filter is not supported not updating VTHM\n");
	}
	osi_writel(value, (unsigned char *)base + EQOS_MAC_VLAN_TR);
	return 0;
}

/**
 *	eqos_update_vlan_id - update VLAN ID in Tag register
 *
 *	@base: Base address from OSI private data structure.
 *
 *	Algorithm: update vid at VLAN tag register
 *
 *	Dependencies: MAC IP should be out of reset and need to be initialized
 *	as the requirements
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure.
 */
static inline int  eqos_update_vlan_id(void *base, unsigned int vid)
{
	unsigned int value;

	value = osi_readl((unsigned char *)base + EQOS_MAC_VLAN_TR);
	/* 0:15 of register */
	value &= ~EQOS_MAC_VLAN_TR_VL;
	value |= vid & EQOS_MAC_VLAN_TR_VL;
	osi_writel(value, (unsigned char *)base + EQOS_MAC_VLAN_TR);

	return 0;
}

/**
 *	eqos_poll_for_tsinit_complete - Poll for time stamp init complete
 *	@addr: MAC base address.
 *	@mac_tcr: Address to store time stamp control register read value
 *
 *	Algorithm: Read TSINIT value from MAC TCR register until it is
 *	equal to zero.
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, -1 - failure
 */
static inline int eqos_poll_for_tsinit_complete(void *addr,
						unsigned int *mac_tcr)
{
	unsigned int retry = 1000;
	unsigned int count;
	int cond = 1;

	/* Wait for previous(if any) Initialize Timestamp value
	 * update to complete
	 */
	count = 0;
	while (cond == 1) {
		if (count > retry) {
			return -1;
		}
		/* Read and Check TSINIT in MAC_Timestamp_Control register */
		*mac_tcr = osi_readl((unsigned char *)addr + EQOS_MAC_TCR);
		if ((*mac_tcr & EQOS_MAC_TCR_TSINIT) == 0U) {
			cond = 0;
		}
		count++;
		osd_msleep(1U);
	}

	return 0;
}

/**
 *	eqos_set_systime - Set system time
 *	@addr: MAC base address.
 *	@sec: Seconds to be configured
 *	@nsec: Nano Seconds to be configured
 *
 *	Algorithm: Updates system time (seconds and nano seconds)
 *	in hardware registers
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, -1 - failure
 */
static int eqos_set_systime_to_mac(void *addr, unsigned int sec,
				   unsigned int nsec)
{
	unsigned int mac_tcr;
	int ret;

	ret = eqos_poll_for_tsinit_complete(addr, &mac_tcr);
	if (ret == -1) {
		return -1;
	}

	/* write seconds value to MAC_System_Time_Seconds_Update register */
	osi_writel(sec, (unsigned char *)addr + EQOS_MAC_STSUR);

	/* write nano seconds value to MAC_System_Time_Nanoseconds_Update
	 * register
	 */
	osi_writel(nsec, (unsigned char *)addr + EQOS_MAC_STNSUR);

	/* issue command to update the configured secs and nsecs values */
	mac_tcr |= EQOS_MAC_TCR_TSINIT;
	osi_writel(mac_tcr, (unsigned char *)addr + EQOS_MAC_TCR);

	ret = eqos_poll_for_tsinit_complete(addr, &mac_tcr);
	if (ret == -1) {
		return -1;
	}

	return 0;
}

/**
 *	eqos_poll_for_tsinit_complete - Poll for addend value write complete
 *	@addr: MAC base address.
 *	@mac_tcr: Address to store time stamp control register read value
 *
 *	Algorithm: Read TSADDREG value from MAC TCR register until it is
 *	equal to zero.
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, -1 - failure
 */
static inline int eqos_poll_for_addend_complete(void *addr,
						unsigned int *mac_tcr)
{
	unsigned int retry = 1000;
	unsigned int count;
	int cond = 1;

	/* Wait for previous(if any) addend value update to complete */
	/* Poll */
	count = 0;
	while (cond == 1) {
		if (count > retry) {
			return -1;
		}
		/* Read and Check TSADDREG in MAC_Timestamp_Control register */
		*mac_tcr = osi_readl((unsigned char *)addr + EQOS_MAC_TCR);
		if ((*mac_tcr & EQOS_MAC_TCR_TSADDREG) == 0U) {
			cond = 0;
		}
		count++;
		osd_msleep(1U);
	}

	return 0;
}

/**
 *	eqos_config_addend - Configure addend
 *	@addr: MAC base address.
 *	@addend: Addend value to be configured
 *
 *	Algorithm: Updates the Addend value in HW register
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *   	Return: 0 - success, -1 - failure
 */
static int eqos_config_addend(void *addr, unsigned int addend)
{
	unsigned int mac_tcr;
	int ret;

	ret = eqos_poll_for_addend_complete(addr, &mac_tcr);
	if (ret == -1) {
		return -1;
	}

	/* write addend value to MAC_Timestamp_Addend register */
	osi_writel(addend, (unsigned char *)addr + EQOS_MAC_TAR);

	/* issue command to update the configured addend value */
	mac_tcr |= EQOS_MAC_TCR_TSADDREG;
	osi_writel(mac_tcr, (unsigned char *)addr + EQOS_MAC_TCR);

	ret = eqos_poll_for_addend_complete(addr, &mac_tcr);
	if (ret == -1) {
		return -1;
	}

	return 0;
}

/**
 *	eqos_poll_for_update_ts_complete - Poll for update time stamp
 *	@addr: MAC base address.
 *	@mac_tcr: Address to store time stamp control register read value
 *
 *	Algorithm: Read time stamp update value from TCR register until it is
 *	equal to zero.
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, -1 - failure
 */
static inline int eqos_poll_for_update_ts_complete(void *addr,
						   unsigned int *mac_tcr)
{
	unsigned int retry = 1000;
	unsigned int count;
	int cond = 1;

	/* Wait for previous(if any) time stamp  value update to complete */
	count = 0;
	while (cond == 1) {
		if (count > retry) {
			return -1;
		}
		/* Read and Check TSUPDT in MAC_Timestamp_Control register */
		*mac_tcr = osi_readl((unsigned char *)addr + EQOS_MAC_TCR);
		if ((*mac_tcr & EQOS_MAC_TCR_TSUPDT) == 0U) {
			cond = 0;
		}
		count++;
		osd_msleep(1U);
	}

	return 0;

}

/**
 *	eqos_adjust_systime - Adjust system time
 *	@addr: MAC base address.
 *	@sec: Seconds to be configured
 *	@nsec: Nano seconds to be configured
 *	@add_sub: To decide on add/sub with system time
 *	@one_nsec_accuracy: One nano second accuracy
 *
 *	Algorithm: Update the system time
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, -1 - failure
 */
static int eqos_adjust_systime(void *addr, unsigned int sec, unsigned int nsec,
			       unsigned int add_sub,
			       unsigned int one_nsec_accuracy)
{
	unsigned int mac_tcr;
	unsigned int value = 0;
	unsigned long long temp = 0;
	int ret;

	ret = eqos_poll_for_update_ts_complete(addr, &mac_tcr);
	if (ret == -1) {
		return -1;
	}

	if (add_sub != 0U) {
		/* If the new sec value needs to be subtracted with
		 * the system time, then MAC_STSUR reg should be
		 * programmed with (2^32 – <new_sec_value>)
		 */
		temp = (TWO_POWER_32 - sec);
		if (temp < UINT_MAX) {
			sec = (unsigned int)temp;
		} else {
			/* do nothing here */
		}

		/* If the new nsec value need to be subtracted with
		 * the system time, then MAC_STNSUR.TSSS field should be
		 * programmed with, (10^9 - <new_nsec_value>) if
		 * MAC_TCR.TSCTRLSSR is set or
		 * (2^32 - <new_nsec_value> if MAC_TCR.TSCTRLSSR is reset)
		 */
		if (one_nsec_accuracy == OSI_ENABLE) {
			if (nsec < UINT_MAX) {
				nsec = (TEN_POWER_9 - nsec);
			}
		} else {
			if (nsec < UINT_MAX) {
				nsec = (TWO_POWER_31 - nsec);
			}
		}
	}

	/* write seconds value to MAC_System_Time_Seconds_Update register */
	osi_writel(sec, (unsigned char *)addr + EQOS_MAC_STSUR);

	/* write nano seconds value and add_sub to
	 * MAC_System_Time_Nanoseconds_Update register
	 */
	value |= nsec;
	value |= add_sub << EQOS_MAC_STNSUR_ADDSUB_SHIFT;
	osi_writel(value, (unsigned char *)addr + EQOS_MAC_STNSUR);

	/* issue command to initialize system time with the value
	 * specified in MAC_STSUR and MAC_STNSUR
	 */
	mac_tcr |= EQOS_MAC_TCR_TSUPDT;
	osi_writel(mac_tcr, (unsigned char *)addr + EQOS_MAC_TCR);

	ret = eqos_poll_for_update_ts_complete(addr, &mac_tcr);
	if (ret == -1) {
		return -1;
	}

	return 0;
}

/**
 *	eqos_get_systime - Get system time from MAC
 *	@addr: MAC base address.
 *
 *	Algorithm: Get current system time
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: 0 - success, -1 - failure
 */
static unsigned long long eqos_get_systime_from_mac(void *addr)
{
	unsigned long long ns1, ns2, ns = 0;
	unsigned int varmac_stnsr, temp1;
	unsigned int varmac_stsr;

	varmac_stnsr = osi_readl((unsigned char *)addr + EQOS_MAC_STNSR);
	temp1 = (varmac_stnsr & EQOS_MAC_STNSR_TSSS_MASK);
	ns1 = (unsigned long long)temp1;

	varmac_stsr = osi_readl((unsigned char *)addr + EQOS_MAC_STSR);

	varmac_stnsr = osi_readl((unsigned char *)addr + EQOS_MAC_STNSR);
	temp1 = (varmac_stnsr & EQOS_MAC_STNSR_TSSS_MASK);
	ns2 = (unsigned long long)temp1;

	/* if ns1 is greater than ns2, it means nsec counter rollover
	 * happened. In that case read the updated sec counter again
	 */
	if (ns1 >= ns2) {
		varmac_stsr = osi_readl((unsigned char *)addr + EQOS_MAC_STSR);
		/* convert sec/high time value to nanosecond */
		if (varmac_stsr < UINT_MAX) {
			ns = ns2 + (varmac_stsr * OSI_NSEC_PER_SEC);
		}
	} else {
		/* convert sec/high time value to nanosecond */
		if (varmac_stsr < UINT_MAX) {
			ns = ns1 + (varmac_stsr * OSI_NSEC_PER_SEC);
		}
	}

	return ns;
}

/**
 *	eqos_config_tscr - Configure Time Stamp Register
 *	@addr: MAC base address.
 *	@ptp_filter: PTP rx filter parameters
 *
 *	Algorithm: Configure Time Stamp Register
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: None.
 **/
static void eqos_config_tscr(void *addr, unsigned int ptp_filter)
{
	unsigned int mac_tcr = 0;

	if (ptp_filter != OSI_DISABLE) {
		mac_tcr = (OSI_MAC_TCR_TSENA	|
			   OSI_MAC_TCR_TSCFUPDT |
			   OSI_MAC_TCR_TSCTRLSSR);

		if ((ptp_filter & OSI_MAC_TCR_SNAPTYPSEL_1) ==
		    OSI_MAC_TCR_SNAPTYPSEL_1) {
			mac_tcr |= OSI_MAC_TCR_SNAPTYPSEL_1;
		}
		if ((ptp_filter & OSI_MAC_TCR_SNAPTYPSEL_2) ==
		    OSI_MAC_TCR_SNAPTYPSEL_2) {
			mac_tcr |= OSI_MAC_TCR_SNAPTYPSEL_2;
		}
		if ((ptp_filter & OSI_MAC_TCR_SNAPTYPSEL_3) ==
		    OSI_MAC_TCR_SNAPTYPSEL_3) {
			mac_tcr |= OSI_MAC_TCR_SNAPTYPSEL_3;
		}
		if ((ptp_filter & OSI_MAC_TCR_TSIPV4ENA) ==
		    OSI_MAC_TCR_TSIPV4ENA) {
			mac_tcr |= OSI_MAC_TCR_TSIPV4ENA;
		}
		if ((ptp_filter & OSI_MAC_TCR_TSIPV6ENA) ==
		    OSI_MAC_TCR_TSIPV6ENA) {
			mac_tcr |= OSI_MAC_TCR_TSIPV6ENA;
		}
		if ((ptp_filter & OSI_MAC_TCR_TSEVENTENA) ==
		    OSI_MAC_TCR_TSEVENTENA) {
			mac_tcr |= OSI_MAC_TCR_TSEVENTENA;
		}
		if ((ptp_filter & OSI_MAC_TCR_TSMASTERENA) ==
		    OSI_MAC_TCR_TSMASTERENA) {
			mac_tcr |= OSI_MAC_TCR_TSMASTERENA;
		}
		if ((ptp_filter & OSI_MAC_TCR_TSVER2ENA) ==
		    OSI_MAC_TCR_TSVER2ENA) {
			mac_tcr |= OSI_MAC_TCR_TSVER2ENA;
		}
		if ((ptp_filter & OSI_MAC_TCR_TSIPENA) ==
		    OSI_MAC_TCR_TSIPENA) {
			mac_tcr |= OSI_MAC_TCR_TSIPENA;
		}
		if ((ptp_filter & OSI_MAC_TCR_AV8021ASMEN) ==
		    OSI_MAC_TCR_AV8021ASMEN) {
			mac_tcr |= OSI_MAC_TCR_AV8021ASMEN;
		}
		if ((ptp_filter & OSI_MAC_TCR_TSENALL) ==
		    OSI_MAC_TCR_TSENALL) {
			mac_tcr |= OSI_MAC_TCR_TSENALL;
		}
	} else {
		/* Disabling the MAC time stamping */
		mac_tcr = OSI_DISABLE;
	}

	osi_writel(mac_tcr, (unsigned char *)addr + EQOS_MAC_TCR);
}

/**
 *	eqos_config_ssir - Configure SSIR
 *	@addr: MAC base address.
 *	@ptp_clock: PTP clock
 *
 *	Algorithm: Configure Sub Second Increment Register
 *
 *	Dependencies: None.
 *
 *	Protection: None.
 *
 *	Return: None.
 */
static void eqos_config_ssir(void *addr, unsigned int ptp_clock)
{
	unsigned int val;
	unsigned int mac_tcr;

	mac_tcr = osi_readl((unsigned char *)addr + EQOS_MAC_TCR);

	/* convert the PTP_CLOCK to nano second.
	 * formula is : ((1/ptp_clock) * 1000000000)
	 * where, ptp_clock = 50MHz if FINE correction
	 * and ptp_clock = EQOS_SYSCLOCK if COARSE correction
	 */

	if ((mac_tcr & EQOS_MAC_TCR_TSCFUPDT) == EQOS_MAC_TCR_TSCFUPDT) {
		val = ((1U * (unsigned int)OSI_NSEC_PER_SEC) /
		       (unsigned int)OSI_ETHER_SYSCLOCK);
	} else {
		val = ((1U * (unsigned int)OSI_NSEC_PER_SEC) / ptp_clock);
	}

	/* 0.465ns accurecy */
	if ((mac_tcr & EQOS_MAC_TCR_TSCTRLSSR) == 0U) {
		if (val < UINT_MAX) {
			val = (val * 1000U) / 465U;
		}
	}

	val |= val << EQOS_MAC_SSIR_SSINC_SHIFT;
	/* update Sub-second Increment Value */
	osi_writel(val, (unsigned char *)addr + EQOS_MAC_SSIR);
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
	.config_flow_control = eqos_config_flow_control,
	.config_arp_offload = eqos_config_arp_offload,
	.config_rxcsum_offload = eqos_config_rxcsum_offload,
	.config_mac_pkt_filter_reg = eqos_config_mac_pkt_filter_reg,
	.update_mac_addr_low_high_reg = eqos_update_mac_addr_low_high_reg,
	.config_l3_l4_filter_enable = eqos_config_l3_l4_filter_enable,
	.config_l2_da_perfect_inverse_match =
				eqos_config_l2_da_perfect_inverse_match,
	.config_l3_filters = eqos_config_l3_filters,
	.update_ip4_addr = eqos_update_ip4_addr,
	.update_ip6_addr = eqos_update_ip6_addr,
	.config_l4_filters = eqos_config_l4_filters,
	.update_l4_port_no = eqos_update_l4_port_no,
	.config_vlan_filtering = eqos_config_vlan_filtering,
	.update_vlan_id = eqos_update_vlan_id,
	.set_systime_to_mac = eqos_set_systime_to_mac,
	.config_addend = eqos_config_addend,
	.adjust_systime = eqos_adjust_systime,
	.get_systime_from_mac = eqos_get_systime_from_mac,
	.config_tscr = eqos_config_tscr,
	.config_ssir = eqos_config_ssir,
	.read_mmc = eqos_read_mmc,
	.reset_mmc = eqos_reset_mmc,
};

struct osi_core_ops *eqos_get_hw_core_ops(void)
{
	return &eqos_core_ops;
}
