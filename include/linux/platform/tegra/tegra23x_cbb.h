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
 */

#define FABRIC_EN_CFG_INTERRUPT_ENABLE_0_0		0x0
#define FABRIC_EN_CFG_STATUS_0_0			0x40
#define FABRIC_EN_CFG_ADDR_INDEX_0_0			0x60
#define FABRIC_EN_CFG_ADDR_LOW_0			0x80
#define FABRIC_EN_CFG_ADDR_HI_0				0x84

#define FABRIC_MN_MASTER_ERR_EN_0			0x200
#define FABRIC_MN_MASTER_ERR_FORCE_0			0x204
#define FABRIC_MN_MASTER_ERR_STATUS_0			0x208
#define FABRIC_MN_MASTER_ERR_OVERFLOW_STATUS_0		0x20c

#define FABRIC_MN_MASTER_LOG_ERR_STATUS_0		0x300
#define FABRIC_MN_MASTER_LOG_ADDR_LOW_0			0x304
#define FABRIC_MN_MASTER_LOG_ADDR_HIGH_0		0x308
#define FABRIC_MN_MASTER_LOG_ATTRIBUTES0_0		0x30c
#define FABRIC_MN_MASTER_LOG_ATTRIBUTES1_0		0x310
#define FABRIC_MN_MASTER_LOG_ATTRIBUTES2_0		0x314
#define FABRIC_MN_MASTER_LOG_USER_BITS0_0		0x318


#define get_em_el_subfield(_x_, _msb_, _lsb_) CBB_EXTRACT(_x_, _msb_, _lsb_)


extern int tegra_cbb_axi2apb_bridge_data(struct platform_device *pdev,
		int *apb_bridge_cnt,
		void __iomem ***axi2abp_bases);

static struct tegra_noc_errors tegra234_errmon_errors[] = {
	{.errcode = "SLAVE_ERR",
	 .type = "Slave being accessed responded with an error. \
			Can be due to Unsupported access, power gated, \
			firewall(SCR), address hole within the slave, etc"
	},
	{.errcode = "DECODE_ERR",
	 .type = "Attempt to access an address hole or Reserved region of \
			memory or AXI Slave"
	},
	{.errcode = "FIREWALL_ERR",
	 .type = "Attempt to access a region which is firewalled"
	},
	{.errcode = "TIMEOUT_ERR",
	 .type = "No response returned by slave"
	},
	{.errcode = "PWRDOWN_ERR",
	 .type = "Attempt to access a portion of fabric that is powered down"
	},
	{.errcode = "UNSUPPORTED_ERR",
	 .type = "Attempt to access a slave through an unsupported access"
	}
};

static char *t234_master_id[] = {
	"TZ",				/* 0x0  */
	"CCPLEX",			/* 0x1  */
	"CCPMU",			/* 0x2  */
	"BPMP_FW",			/* 0x3  */
	"AON",				/* 0x4  */
	"SCE",				/* 0x5  */
	"GPCDMA_P",			/* 0x6  */
	"TSECA_NONSECURE",		/* 0x7  */
	"TSECA_LIGHTSECURE",		/* 0x8  */
	"TSECA_HEAVYSECURE",		/* 0x9  */
	"CORESIGHT",			/* 0xA  */
	"APE",				/* 0xB  */
	"PEATRANS",			/* 0xC  */
	"JTAGM_DFT",			/* 0xD  */
	"RCE",				/* 0xE  */
	"DCE",				/* 0xF  */
	"PSC_FW_USER",			/* 0x10 */
	"PSC_FW_SUPERVISOR",		/* 0x11 */
	"PSC_FW_MACHINE",		/* 0x12 */
	"PSC_BOOT",			/* 0x13 */
	"BPMP_BOOT",			/* 0x14 */
	"NVDEC_NONSECURE",		/* 0x15 */
	"NVDEC_LIGHTSECURE",		/* 0x16 */
	"NVDEC_HEAVYSECURE",		/* 0x17 */
	"CBB_INTERNAL",			/* 0x18 */
	"RSVD"				/* 0x3F */
};

