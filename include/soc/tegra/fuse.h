/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2012-2020, NVIDIA CORPORATION.  All rights reserved.
 */

#ifndef __SOC_TEGRA_FUSE_H__
#define __SOC_TEGRA_FUSE_H__

#include <linux/types.h>

#define TEGRA20		0x20
#define TEGRA30		0x30
#define TEGRA114	0x35
#define TEGRA124	0x40
#define TEGRA132	0x13
#define TEGRA210	0x21
#define TEGRA186	0x18
#define TEGRA194	0x19
#define TEGRA234	0x23

#define TEGRA_FUSE_SKU_CALIB_0	0xf0
#define TEGRA30_FUSE_SATA_CALIB	0x124
#define TEGRA_FUSE_USB_CALIB_EXT_0 0x250
#define FUSE_TDIODE_CALIB	0x274

#ifndef __ASSEMBLY__

u32 tegra_read_chipid(void);
u8 tegra_get_chip_id(void);
int tegra_miscreg_set_erd(u64);

enum tegra_revision {
	TEGRA_REVISION_UNKNOWN = 0,
	TEGRA_REVISION_A01,
	TEGRA_REVISION_A01q,
	TEGRA_REVISION_A02,
	TEGRA_REVISION_A02p,
	TEGRA_REVISION_A03,
	TEGRA_REVISION_A03p,
	TEGRA_REVISION_A04,
	TEGRA_REVISION_A04p,
	TEGRA210_REVISION_A01,
	TEGRA210_REVISION_A01q,
	TEGRA210_REVISION_A02,
	TEGRA210_REVISION_A02p,
	TEGRA210_REVISION_A03,
	TEGRA210_REVISION_A03p,
	TEGRA210_REVISION_A04,
	TEGRA210_REVISION_A04p,
	TEGRA210_REVISION_B01,
	TEGRA210B01_REVISION_A01,
	TEGRA186_REVISION_A01,
	TEGRA186_REVISION_A01q,
	TEGRA186_REVISION_A02,
	TEGRA186_REVISION_A02p,
	TEGRA186_REVISION_A03,
	TEGRA186_REVISION_A03p,
	TEGRA186_REVISION_A04,
	TEGRA186_REVISION_A04p,
	TEGRA194_REVISION_A01,
	TEGRA194_REVISION_A02,
	TEGRA194_REVISION_A02p,
	TEGRA_REVISION_QT,
	TEGRA_REVISION_SIM,
	TEGRA_REVISION_MAX,
};

enum tegra_ucm {
	TEGRA_UCM1 = 0,
	TEGRA_UCM2,
};

struct tegra_sku_info {
	int sku_id;
	int cpu_process_id;
	int cpu_speedo_id;
	int cpu_speedo_value;
	int cpu_iddq_value;
	int core_process_id;
	int soc_process_id;
	int soc_speedo_id;
	int soc_speedo_value;
	int soc_iddq_value;
	int gpu_process_id;
	int gpu_speedo_id;
	int gpu_iddq_value;
	int gpu_speedo_value;
	enum tegra_revision revision;
	enum tegra_revision id_and_rev;
	enum tegra_ucm ucm;
	int speedo_rev;
};

u32 tegra_read_straps(void);
u32 tegra_read_ram_code(void);
int tegra_fuse_readl(unsigned long offset, u32 *value);

extern struct tegra_sku_info tegra_sku_info;

struct device *tegra_soc_device_register(void);

#endif /* __ASSEMBLY__ */

/* Adding downstream header at the end as it could depend on header above */
#include <soc/tegra/chip-id.h>

#endif /* __SOC_TEGRA_FUSE_H__ */
