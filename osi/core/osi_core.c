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

#include <osi_core.h>
#include <osd.h>

#define MAC_MDIO_ADDRESS	0x200
#define MAC_GMII_BUSY		0x00000001U

#define MAC_MDIO_DATA		0x204

#define MAC_GMIIDR_GD_WR_MASK	0xffff0000U
#define MAC_GMIIDR_GD_MASK	0xffffU

#define MDIO_PHY_ADDR_SHIFT	21U
#define MDIO_PHY_REG_SHIFT	16U
#define MDIO_MII_WRITE		OSI_BIT(2)

extern struct osi_core_ops *eqos_get_hw_core_ops(void);

/**
 *	osi_write_phy_reg - Write to a PHY register through MAC over MDIO bus.
 *	@osi: OSI private data structure.
 *	@phyaddr: PHY address (PHY ID) associated with PHY
 *	@phyreg: Register which needs to be write to PHY.
 *	@phydata: Data to write to a PHY register.
 *
 *	Algorithm:
 *	1) Before proceding for reading for PHY register check whether any MII
 *	operation going on MDIO bus by polling MAC_GMII_BUSY bit.
 *	2) Program data into MAC MDIO data register.
 *	3) Populate required parameters like phy address, phy register etc,,
 *	in MAC MDIO Address register. write and GMII busy bits needs to be set
 *	in this operation.
 *	4) Write into MAC MDIO address register poll for GMII busy for MDIO
 *	operation to complete.
 *
 *	Dependencies: MAC IP should be out of reset
 *
 *	Return: 0 - success, -1 - failure
 */
int osi_write_phy_reg(struct osi_core_priv_data *osi_core, unsigned int phyaddr,
		      unsigned int phyreg, unsigned short phydata)
{
	unsigned int retry = 1000;
	unsigned int count;
	unsigned int mac_gmiiar;
	unsigned int mac_gmiidr;
	int cond = 1;

	/* wait for any previous MII read/write operation to complete */
	count = 0;
	while (cond == 1) {
		if (count > retry) {
			return -1;
		}

		count++;
		osd_msleep(1U);

		mac_gmiiar = osi_readl((unsigned char *)osi_core->base + MAC_MDIO_ADDRESS);

		if ((mac_gmiiar & MAC_GMII_BUSY) == 0U) {
			cond = 0;
		}
	}

	mac_gmiidr = osi_readl((unsigned char *)osi_core->base + MAC_MDIO_DATA);

	mac_gmiidr = ((mac_gmiidr & MAC_GMIIDR_GD_WR_MASK) |
		      (((phydata) & MAC_GMIIDR_GD_MASK) << 0));

	osi_writel(mac_gmiidr, (unsigned char *)osi_core->base + MAC_MDIO_DATA);

	/* initiate the MII write operation by updating desired */
	/* phy address/id (0 - 31) */
	/* phy register offset */
	/* CSR Clock Range (20 - 35MHz) */
	/* Select write operation */
	/* set busy bit */
	mac_gmiiar = osi_readl((unsigned char *)osi_core->base + MAC_MDIO_ADDRESS);
	mac_gmiiar = (mac_gmiiar & 0x12U);
	mac_gmiiar = (mac_gmiiar | ((phyaddr) << MDIO_PHY_ADDR_SHIFT) |
		     ((phyreg) << MDIO_PHY_REG_SHIFT) |
		     ((osi_core->mdc_cr) << 8U) |
		     MDIO_MII_WRITE | MAC_GMII_BUSY);

	osi_writel(mac_gmiiar, (unsigned char *)osi_core->base + MAC_MDIO_ADDRESS);

	osd_usleep_range(9, 11);

	/* wait for MII write operation to complete */
	cond = 1;
	count = 0;
	while (cond == 1) {
		if (count > retry) {
			return -1;
		}

		count++;
		osd_msleep(1U);

		mac_gmiiar = osi_readl((unsigned char *)osi_core->base + MAC_MDIO_ADDRESS);

		if ((mac_gmiiar & MAC_GMII_BUSY) == 0U) {
			cond = 0;
		}
	}

	return 0;
}

/**
 *	osi_read_phy_reg - Read from a PHY register through MAC over MDIO bus.
 *	@osi: OSI private data structure.
 *	@phyaddr: PHY address (PHY ID) associated with PHY
 *	@phyreg: Register which needs to be read from PHY.
 *
 *	Algorithm:
 *	1) Before proceding for reading for PHY register check whether any MII
 *	operation going on MDIO bus by polling MAC_GMII_BUSY bit.
 *	2) Populate required parameters like phy address, phy register etc,,
 *	in program it in MAC MDIO Address register. Read and GMII busy bits
 *	needs to be set in this operation.
 *	3) Write into MAC MDIO address register poll for GMII busy for MDIO
 *	operation to complete. After this data will be available at MAC MDIO
 *	data register.
 *
 *	Dependencies: MAC IP should be out of reset
 *
 *	Return: data from PHY register - success, -1 - failure
 */

int osi_read_phy_reg(struct osi_core_priv_data *osi_core, unsigned int phyaddr,
		     unsigned int phyreg)
{
	unsigned int retry = 1000;
	unsigned int count;
	unsigned int mac_gmiiar;
	unsigned int mac_gmiidr;
	unsigned int data;
	int cond = 1;

	/* wait for any previous MII read/write operation to complete */
	/*Poll Until Poll Condition */
	count = 0;
	while (cond == 1) {
		if (count > retry) {
			return -1;
		}

		count++;
		osd_msleep(1U);

		mac_gmiiar = osi_readl((unsigned char *)osi_core->base + MAC_MDIO_ADDRESS);

		if ((mac_gmiiar & MAC_GMII_BUSY) == 0U) {
			cond = 0;
		}
	}

	mac_gmiiar = osi_readl((unsigned char *)osi_core->base + MAC_MDIO_ADDRESS);
	/* initiate the MII read operation by updating desired */
	/* phy address/id (0 - 31) */
	/* phy register offset */
	/* CSR Clock Range (20 - 35MHz) */
	/* Select read operation */
	/* set busy bit */
	mac_gmiiar = (mac_gmiiar & 0x12U);
	mac_gmiiar = mac_gmiiar | ((phyaddr) << MDIO_PHY_ADDR_SHIFT) |
		     ((phyreg) << MDIO_PHY_REG_SHIFT) |
		     (osi_core->mdc_cr) << 8U | ((0x3U) << 2U) | MAC_GMII_BUSY;
	osi_writel(mac_gmiiar, (unsigned char *)osi_core->base + MAC_MDIO_ADDRESS);

	osd_usleep_range(9, 11);

	/* wait for MII write operation to complete */
	/*Poll Until Poll Condition */
	cond = 1;
	count = 0;
	while (cond == 1) {
		if (count > retry) {
			return -1;
		}

		count++;
		osd_msleep(1U);

		mac_gmiiar = osi_readl((unsigned char *)osi_core->base + MAC_MDIO_ADDRESS);

		if ((mac_gmiiar & MAC_GMII_BUSY) == 0U) {
			cond = 0;
		}
	}

	mac_gmiidr = osi_readl((unsigned char *)osi_core->base + MAC_MDIO_DATA);
	data = (mac_gmiidr & 0x0000FFFFU);

	return (int)data;
}

void osi_init_core_ops(struct osi_core_priv_data *osi_core)
{
	if (osi_core->mac == OSI_MAC_HW_EQOS) {
		/* Get EQOS HW ops */
		osi_core->ops = eqos_get_hw_core_ops();
	}
}

int osi_poll_for_swr(struct osi_core_priv_data *osi_core)
{
	int ret = 0;

	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->poll_for_swr != OSI_NULL)) {
		ret = osi_core->ops->poll_for_swr(osi_core->base);
	}

	return ret;
}

void osi_set_mdc_clk_rate(struct osi_core_priv_data *osi_core,
			  unsigned long csr_clk_rate)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->set_mdc_clk_rate != OSI_NULL)) {
		osi_core->ops->set_mdc_clk_rate(osi_core, csr_clk_rate);
	}
}

int osi_hw_core_init(struct osi_core_priv_data *osi_core,
		     unsigned int tx_fifo_size,
		     unsigned int rx_fifo_size)
{
	int ret = 0;

	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->core_init != OSI_NULL)) {
		ret = osi_core->ops->core_init(osi_core, tx_fifo_size,
					       rx_fifo_size);
	}

	return ret;
}

void osi_start_mac(struct osi_core_priv_data *osi_core)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->start_mac != OSI_NULL)) {
		osi_core->ops->start_mac(osi_core->base);
	}
}

void osi_stop_mac(struct osi_core_priv_data *osi_core)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->stop_mac != OSI_NULL)) {
		osi_core->ops->stop_mac(osi_core->base);
	}
}

void osi_common_isr(struct osi_core_priv_data *osi_core)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->handle_common_intr != OSI_NULL)) {
		osi_core->ops->handle_common_intr(osi_core);
	}
}

void osi_set_mode(struct osi_core_priv_data *osi_core, int mode)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->set_mode != OSI_NULL)) {
		osi_core->ops->set_mode(osi_core->base, mode);
	}
}

void osi_set_speed(struct osi_core_priv_data *osi_core, int speed)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->set_speed != OSI_NULL)) {
		osi_core->ops->set_speed(osi_core->base, speed);
	}
}

int osi_pad_calibrate(struct osi_core_priv_data *osi_core)
{
	int ret = 0;

	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->pad_calibrate != OSI_NULL)) {
		ret = osi_core->ops->pad_calibrate(osi_core);
	}

	return ret;
}

int osi_flush_mtl_tx_queue(struct osi_core_priv_data *osi_core,
			   unsigned int qinx)
{
        int ret = 0;

        if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
            (osi_core->ops->flush_mtl_tx_queue != OSI_NULL)) {
                ret = osi_core->ops->flush_mtl_tx_queue(osi_core->base, qinx);
        }

        return ret;
}

int osi_config_mac_loopback(struct osi_core_priv_data *osi_core,
			    unsigned int lb_mode)
{
	int ret = -1;

	/* Configure MAC LoopBack */
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->config_mac_loopback != OSI_NULL)) {
		ret = osi_core->ops->config_mac_loopback(osi_core->base,
							 lb_mode);
	}

	return ret;
}

int osi_set_avb(struct osi_core_priv_data *osi_core,
		struct osi_core_avb_algorithm *avb)
{
	int ret = -1;

	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->set_avb_algorithm != OSI_NULL)) {
		ret = osi_core->ops->set_avb_algorithm(osi_core, avb);
	}

	return ret;
}

int osi_get_avb(struct osi_core_priv_data *osi_core,
		struct osi_core_avb_algorithm *avb)
{
	int ret = -1;

	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->get_avb_algorithm != OSI_NULL)) {
		ret = osi_core->ops->get_avb_algorithm(osi_core, avb);
	}

	return ret;
}

int osi_configure_txstatus(struct osi_core_priv_data *osi_core,
			   unsigned int tx_status)
{
	int ret = -1;

	/* Configure Drop Transmit Status */
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->config_tx_status != OSI_NULL)) {
		ret = osi_core->ops->config_tx_status(osi_core->base,
						      tx_status);
	}

	return ret;
}

int osi_config_fw_err_pkts(struct osi_core_priv_data *osi_core,
			   unsigned int qinx, unsigned int fw_err)
{
	int ret = -1;

	/* Configure Forwarding of Error packets */
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->config_fw_err_pkts != OSI_NULL)) {
		ret = osi_core->ops->config_fw_err_pkts(osi_core->base,
							qinx, fw_err);
	}

	return ret;
}

int osi_config_rx_crc_check(struct osi_core_priv_data *osi_core,
			    unsigned int crc_chk)
{
	int ret = -1;

	/* Configure CRC Checking for Received Packets */
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->config_rx_crc_check != OSI_NULL)) {
		ret = osi_core->ops->config_rx_crc_check(osi_core->base,
							 crc_chk);
	}

	return ret;
}

int osi_configure_flow_control(struct osi_core_priv_data *osi_core,
			       unsigned int flw_ctrl)
{
	int ret = -1;

	/* Configure Flow control settings */
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->config_flow_control != OSI_NULL)) {
		ret = osi_core->ops->config_flow_control(osi_core->base,
							 flw_ctrl);
	}

	return ret;
}
