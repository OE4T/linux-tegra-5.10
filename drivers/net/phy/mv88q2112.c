/*
 * Driver for Marvell 88Q2112 PHY
 *
 * Author: Abdul Mohammed <amohammed@nvidia.com>
 *
 * Copyright (c) 2018-2021, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/mii.h>
#include <linux/ethtool.h>
#include <linux/phy.h>
#include <linux/mdio.h>
#include <linux/of.h>

/* PHY Device ID */
#define MARVELL_PHY_ID_88Q2112		0x002b0980
#define MARVELL_PHY_ID_MASK		0xfffffff0
#define MARVELL_PHY_REV_MASK		0x0000000f

/* Interrupt Registers */
#define MDIO_INT_STAT		0x8011

/* PMA Registers */
#define MDIO_PMA_CTRL		0x0834
#define MDIO_PMA_SPEED1000	BIT(0)
#define MDIO_PMA_CTRL_MASTER	BIT(14)

/* PCS Status Registers */
#define MDIO_PCS_STAT1_1000		0x0901
#define MDIO_AN_PCS_STAT		0x8001

#define MDIO_PCS_STAT1_100		0x8109
#define MDIO_PCS_STAT2_100		0x8108

#define MDIO_PCS_RXSTAT		(BIT(12) | BIT(13))

/* Packet Checker Registers */
#define MDIO_PCS_PC_CNT		0xFD08

#define MDIO_PCS_PC_RXMASK	0x00FF
#define MDIO_PCS_PC_ERRMASK	0xFF00

/* wrapper for indirect/direct clause 45 access */
#define MDIO_READ		mv88q2112_mdio_read
#define MDIO_WRITE		mv88q2112_mdio_write

struct mv88q2112_stat {
	const char *string;
	int devad;
	u32 regnum;
	u32 mask;
};

/* private data specific to 88q2112's state */
struct mv88q2112_priv {
	int rev;
	int is_c45;
};

static struct mv88q2112_stat mv88q2112_stats[] = {
	{"phy_rx_count", MDIO_MMD_PCS, MDIO_PCS_PC_CNT, MDIO_PCS_PC_RXMASK},
	{"phy_error_count", MDIO_MMD_PCS, MDIO_PCS_PC_CNT,
	 MDIO_PCS_PC_ERRMASK},
};

/* Indirect clause 45 read as per IEEE 802.3 Annex 22D */
static int mv88q2112_read_mmd_indirect(struct phy_device *phydev, int devad,
				       u32 regnum)
{
	struct mii_bus *bus = phydev->mdio.bus;
	int addr = phydev->mdio.addr;
	int val;

	bus->write(bus, addr, MII_MMD_CTRL, devad);
	bus->write(bus, addr, MII_MMD_DATA, regnum);
	bus->write(bus, addr, MII_MMD_CTRL, (MII_MMD_CTRL_NOINCR | devad));
	val = bus->read(bus, addr, MII_MMD_DATA);
	return val;
}

static int mv88q2112_mdio_read(struct phy_device *phydev, int devad, u32 regnum)
{
	struct mv88q2112_priv *priv = phydev->priv;
	struct mii_bus *bus = phydev->mdio.bus;
	int addr = phydev->mdio.addr;
	unsigned int phyreg = 0;
	int val = 0;

	mutex_lock(&bus->mdio_lock);

	if (priv->is_c45) {
		phyreg |= MII_ADDR_C45;
		phyreg |= ((devad & 0x1f) << 16);
		phyreg |= (regnum & 0xffff);

		val = bus->read(bus, addr, phyreg);
	} else {
		val = mv88q2112_read_mmd_indirect(phydev, devad, regnum);
	}

	mutex_unlock(&bus->mdio_lock);

	return val;
}

/* Updates the PHY link status and speed */
static int mv88q2112_read_status(struct phy_device *phydev)
{
	int link, status;

	status = MDIO_READ(phydev, MDIO_MMD_PMAPMD, MDIO_PMA_CTRL);

	if (status & MDIO_PMA_SPEED1000) {
		link = MDIO_READ(phydev, MDIO_MMD_PCS, MDIO_PCS_STAT1_1000);
		link = MDIO_READ(phydev, MDIO_MMD_PCS, MDIO_PCS_STAT1_1000);
		status = MDIO_READ(phydev, MDIO_MMD_AN, MDIO_AN_PCS_STAT);
		phydev->speed = SPEED_1000;
	} else {
		link = MDIO_READ(phydev, MDIO_MMD_PCS, MDIO_PCS_STAT1_100);
		status = MDIO_READ(phydev, MDIO_MMD_PCS, MDIO_PCS_STAT2_100);
		phydev->speed = SPEED_100;
	}

	if (status < 0)
		return status;

	if (link < 0)
		return link;

	if ((link & MDIO_STAT1_LSTATUS) && (status & MDIO_PCS_RXSTAT))
		phydev->link = 1;
	else
		phydev->link = 0;

	phydev->duplex = DUPLEX_FULL;

	return 0;
}

static int mv88q2112_phy_config_intr(struct phy_device *phydev)
{
	return 0;
}

static int mv88q2112_phy_ack_interrupt(struct phy_device *phydev)
{
	int rc = MDIO_READ(phydev, MDIO_MMD_PCS, MDIO_INT_STAT);
	return rc;
}

static int mv88q2112_config_aneg(struct phy_device *phydev)
{
	/* Autonegotiation not required
	 * as we implement a fixed link speed
	 */
	return 0;
}

static int mv88q2112_aneg_done(struct phy_device *phydev)
{
	/* Always signal that Autoneg is complete
	 * since it is not configured, and the PHY
	 * state machine will not wait
	 */
	return 1;
}

#ifdef CONFIG_OF_MDIO
static int mv88q2112_is_c45(struct phy_device *phydev)
{
	struct device_node *node = phydev->mdio.dev.of_node;
	int ret = 0;

	ret = of_device_is_compatible(node, "ethernet-phy-ieee802.3-c45");
	return ret;
}

#else
static int mv88q2112_of_get_speed(struct phy_device *phydev)
{
	return 0;
}

static int mv88q2112_is_c45(struct phy_device *phydev)
{
	return 0;
}
#endif

static int mv88q2112_config_init(struct phy_device *phydev)
{
	return 0;
}

static int mv88q2112_get_id(struct phy_device *phydev, u32 *phy_id)
{
	int reg;

	/* Read Device ID first byte */
	reg = MDIO_READ(phydev, MDIO_MMD_PMAPMD, MII_PHYSID1);
	if (reg < 0) {
		phydev_err(phydev, "error in first read\n");
		return reg;
	}

	*phy_id = (reg & 0xFFFF) << 16;

	/* Read Device ID second byte */
	reg = MDIO_READ(phydev, MDIO_MMD_PMAPMD, MII_PHYSID2);
	if (reg < 0) {
		phydev_err(phydev, "error in second read\n");
		return reg;
	}

	*phy_id |= (reg & 0xFFFF);

	if ((*phy_id & MARVELL_PHY_ID_MASK) != MARVELL_PHY_ID_88Q2112) {
		phydev_err(phydev, "error in id:%x\n", *phy_id);
		return -ENODEV;
	}

	return 0;
}

static int mv88q2112_probe(struct phy_device *phydev)
{
	int ret;
	u32 phy_id = 0;
	u32 rev;
	struct mv88q2112_priv *priv;

	priv = devm_kzalloc(&phydev->mdio.dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->is_c45 = mv88q2112_is_c45(phydev);
	phydev->priv = priv;

	ret = mv88q2112_get_id(phydev, &phy_id);
	rev = phy_id & MARVELL_PHY_REV_MASK;
	if (ret < 0)
		return ret;

	rev = phy_id & MARVELL_PHY_REV_MASK;
	phydev_err(phydev, "%s: got rev %d\n", __func__, rev);

	priv->rev = rev;

	return 0;
}

static void mv88q2112_remove(struct phy_device *phydev)
{
	struct device *dev = &phydev->mdio.dev;
	struct mv88q2112_priv *priv = phydev->priv;

	if (phydev->priv)
		devm_kfree(dev, priv);

	phydev->priv = NULL;
}

static int mv88q2112_get_sset_count(struct phy_device *phydev)
{
	return ARRAY_SIZE(mv88q2112_stats);
}

static void mv88q2112_get_strings(struct phy_device *phydev, u8 *data)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mv88q2112_stats); i++) {
		strlcpy(data + i * ETH_GSTRING_LEN, mv88q2112_stats[i].string,
			ETH_GSTRING_LEN);
	}
}

static void mv88q2112_get_stats(struct phy_device *phydev,
				struct ethtool_stats *stats, u64 *data)
{
	int i;
	u64 val;

	for (i = 0; i < ARRAY_SIZE(mv88q2112_stats); i++) {
		val = MDIO_READ(phydev, mv88q2112_stats[i].devad,
				mv88q2112_stats[i].regnum);
		if (val < 0)
			val = U64_MAX;
		val = val & mv88q2112_stats[i].mask;
		data[i] = val;
	}
}

static struct phy_driver mv88q2112_driver[] = {
	{
	 .phy_id = MARVELL_PHY_ID_88Q2112,
	 .phy_id_mask = MARVELL_PHY_ID_MASK,
	 .name = "Marvell 88Q2112",
	 .probe = &mv88q2112_probe,
	 .remove = &mv88q2112_remove,
	 .features = PHY_GBIT_FEATURES,
	 .flags = PHY_HAS_INTERRUPT,
	 .config_init = &mv88q2112_config_init,
	 //.soft_reset = &mv88q2112_soft_reset,
	 .read_status = &mv88q2112_read_status,
	 .config_aneg = &mv88q2112_config_aneg,
	 .aneg_done = &mv88q2112_aneg_done,
	 .config_intr = &mv88q2112_phy_config_intr,
	 .ack_interrupt = &mv88q2112_phy_ack_interrupt,
	 .get_sset_count = &mv88q2112_get_sset_count,
	 .get_strings = &mv88q2112_get_strings,
	 .get_stats = &mv88q2112_get_stats,
	},
};

module_phy_driver(mv88q2112_driver);

static struct mdio_device_id __maybe_unused mv88q2112_tbl[] = {
	{MARVELL_PHY_ID_88Q2112, MARVELL_PHY_ID_MASK},
	{}
};

MODULE_DEVICE_TABLE(mdio, mv88q2112_tbl);

MODULE_DESCRIPTION("Marvell 88Q2112 Ethernet PHY (ver A0/A1/A2) driver (MV88Q2112-A0/1/2)");
MODULE_AUTHOR("Abdul Mohammed <amohammed@nvidia.com>");
MODULE_LICENSE("GPL");
