/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * This header contains all downstream defines related to fuse and chipid
 *
 * Copyright (c) 2012-2020, NVIDIA CORPORATION.  All rights reserved.
 */

#ifndef __SOC_TEGRA_CHIP_ID_H_
#define __SOC_TEGRA_CHIP_ID_H_

#include <linux/types.h>

/* supported tegra chip id list */
#define TEGRA148	0x14
#define TEGRA210B01	0x21

/* production mode */
#define TEGRA_FUSE_PRODUCTION_MODE      0x0

/* control read/write calls for below offsets */
#define FUSE_FUSEBYPASS_0		0x24
#define FUSE_WRITE_ACCESS_SW_0		0x30

/* read/write calls for below offsets */
#define FUSE_GCPLEX_CONFIG_FUSE_0	0x1c8
#define FUSE_RESERVED_CALIB0_0		0x204
#define FUSE_OPT_GPU_TPC0_DISABLE_0	0x20c
#define FUSE_OPT_GPU_TPC1_DISABLE_0	0x23c

/* T186+ */
#define FUSE_PDI0			0x300
#define FUSE_PDI1			0x304

#define FUSE_IP_DISABLE_0			0x4b0
#define FUSE_IP_DISABLE_0_NVLINK_MASK		0x10

#define FUSE_UCODE_MINION_REV_0			0x4d4
#define FUSE_UCODE_MINION_REV_0_MASK		0x7

#define FUSE_SECURE_MINION_DEBUG_DIS_0		0x4d8
#define FUSE_SECURE_MINION_DEBUG_DIS_0_MASK	0x1

#ifndef __ASSEMBLY__

#include <linux/of.h>

/* Tegra HIDREV/ChipID helper macros */
#define HIDREV_CHIPID_SHIFT		0x8
#define HIDREV_CHIPID_MASK		0xff
#define HIDREV_MAJORREV_SHIFT		0x4
#define HIDREV_MAJORREV_MASK		0xf
#define HIDREV_MINORREV_SHIFT		0x10
#define HIDREV_MINORREV_MASK		0xf
#define HIDREV_PRE_SI_PLAT_SHIFT	0x14
#define HIDREV_PRE_SI_PLAT_MASK		0xf

/* Helper functions to read HIDREV fields */
static inline u32 tegra_hidrev_get_chipid(u32 chipid)
{
	return (chipid >> HIDREV_CHIPID_SHIFT) & HIDREV_CHIPID_MASK;
}

static inline u32 tegra_hidrev_get_majorrev(u32 chipid)
{
	return (chipid >> HIDREV_MAJORREV_SHIFT) & HIDREV_MAJORREV_MASK;
}

static inline u32 tegra_hidrev_get_minorrev(u32 chipid)
{
	return (chipid >> HIDREV_MINORREV_SHIFT) & HIDREV_MINORREV_MASK;
}

static inline u32 tegra_hidrev_get_pre_si_plat(u32 chipid)
{
	return (chipid >> HIDREV_PRE_SI_PLAT_SHIFT) & HIDREV_PRE_SI_PLAT_MASK;
}

#define TEGRA_FUSE_HAS_PLATFORM_APIS

enum tegra_chipid {
	TEGRA_CHIPID_UNKNOWN = 0,
	TEGRA_CHIPID_TEGRA14 = 0x14,
	TEGRA_CHIPID_TEGRA2 = 0x20,
	TEGRA_CHIPID_TEGRA3 = 0x30,
	TEGRA_CHIPID_TEGRA11 = 0x35,
	TEGRA_CHIPID_TEGRA12 = 0x40,
	TEGRA_CHIPID_TEGRA13 = 0x13,
	TEGRA_CHIPID_TEGRA21 = 0x21,
	TEGRA_CHIPID_TEGRA18 = 0x18,
	TEGRA_CHIPID_TEGRA19 = 0x19,
	TEGRA_CHIPID_TEGRA23 = 0x23,
};

#define TEGRA_FUSE_HAS_PLATFORM_APIS

enum tegra_platform {
	TEGRA_PLATFORM_SILICON = 0,
	TEGRA_PLATFORM_QT,
	TEGRA_PLATFORM_LINSIM,
	TEGRA_PLATFORM_FPGA,
	TEGRA_PLATFORM_UNIT_FPGA,
	TEGRA_PLATFORM_VDK,
	TEGRA_PLATFORM_VSP,
	TEGRA_PLATFORM_MAX,
};

extern int tegra_set_erd(u64 err_config);

extern struct tegra_sku_info tegra_sku_info;

extern enum tegra_revision tegra_revision;

extern u32 tegra_read_emu_revid(void);
extern u32 tegra_get_sku_id(void);
extern enum tegra_revision tegra_chip_get_revision(void);
extern bool is_t210b01_sku(void);

/* check if in hypervisor mode */
bool is_tegra_hypervisor_mode(void);

/* check if safety build */
bool is_tegra_safety_build(void);

bool tegra_cpu_is_asim(void);
bool tegra_cpu_is_dsim(void);
enum tegra_platform tegra_get_platform(void);

static inline bool tegra_platform_is_silicon(void)
{
	return tegra_get_platform() == TEGRA_PLATFORM_SILICON;
}
static inline bool tegra_is_silicon(void)
{
	return tegra_platform_is_silicon();
}
static inline bool tegra_platform_is_qt(void)
{
	return tegra_get_platform() == TEGRA_PLATFORM_QT;
}
static inline bool tegra_platform_is_fpga(void)
{
	return tegra_get_platform() == TEGRA_PLATFORM_FPGA;
}
static inline bool tegra_platform_is_vdk(void)
{
	return tegra_get_platform() == TEGRA_PLATFORM_VDK;
}
static inline bool tegra_platform_is_sim(void)
{
	return tegra_platform_is_vdk();
}
static inline bool tegra_platform_is_vsp(void)
{
	return tegra_get_platform() == TEGRA_PLATFORM_VSP;
}

extern void tegra_set_tegraid_from_hw(void);
#endif /* !__ASSEMBLY__ */

#define TEGRA_FUSE_PRODUCTION_MODE	0x0
#define FUSE_FUSEBYPASS_0		0x24
#define FUSE_WRITE_ACCESS_SW_0		0x30

#define FUSE_SKU_INFO			0x10
#define FUSE_SKU_MSB_MASK		0xFF00
#define FUSE_SKU_MSB_SHIFT		8

#define FUSE_OPT_FT_REV_0		0x28

#define FUSE_SKU_USB_CALIB_0		0xf0
#define TEGRA_FUSE_SKU_CALIB_0		0xf0

#define FUSE_OPT_VENDOR_CODE		0x100
#define FUSE_OPT_VENDOR_CODE_MASK	0xf
#define FUSE_OPT_FAB_CODE		0x104
#define FUSE_OPT_FAB_CODE_MASK		0x3f
#define FUSE_OPT_LOT_CODE_0		0x108
#define FUSE_OPT_LOT_CODE_1		0x10c
#define FUSE_OPT_WAFER_ID		0x118
#define FUSE_OPT_WAFER_ID_MASK		0x3f
#define FUSE_OPT_X_COORDINATE		0x114
#define FUSE_OPT_X_COORDINATE_MASK	0x1ff
#define FUSE_OPT_Y_COORDINATE		0x118
#define FUSE_OPT_Y_COORDINATE_MASK	0x1ff

#define TEGRA30_FUSE_SATA_CALIB		0x124

#define FUSE_OPT_SUBREVISION		0x148
#define FUSE_OPT_SUBREVISION_MASK	0xF

#define FUSE_GCPLEX_CONFIG_FUSE_0	0x1c8

#define FUSE_TDIODE_CALIB		0x274
#define FUSE_RESERVED_CALIB0_0		0x204

#define FUSE_OPT_GPU_TPC0_DISABLE_0	0x20c
#define FUSE_OPT_GPU_TPC1_DISABLE_0	0x23c

#define FUSE_USB_CALIB_EXT_0		0x250
#define TEGRA_FUSE_USB_CALIB_EXT_0	0x250

#define FUSE_CP_REV                    0x90
#define TEGRA_FUSE_CP_REV_0_3          (3)

#define FUSE_OPT_SUBREVISION		0x148
#define FUSE_OPT_SUBREVISION_MASK	0xF

#define FUSE_IP_DISABLE_0			0x4b0
#define FUSE_IP_DISABLE_0_NVLINK_MASK		0x10

#define FUSE_UCODE_MINION_REV_0			0x4d4
#define FUSE_UCODE_MINION_REV_0_MASK		0x7

#define FUSE_SECURE_MINION_DEBUG_DIS_0		0x4d8
#define FUSE_SECURE_MINION_DEBUG_DIS_0_MASK	0x1

/* T186+ */
#define FUSE_PDI0			0x300
#define FUSE_PDI1			0x304

#ifndef __ASSEMBLY__

u32 tegra_read_chipid(void);
enum tegra_chipid tegra_get_chipid(void);
unsigned long long tegra_chip_uid(void);

int tegra_fuse_control_read(unsigned long offset, u32 *value);
void tegra_fuse_control_write(u32 value, unsigned long offset);

void tegra_fuse_writel(u32 val, unsigned long offset);
enum tegra_revision tegra_chip_get_revision(void);
int tegra_fuse_clock_enable(void);
int tegra_fuse_clock_disable(void);
u32 tegra_get_sku_id(void);

/* TODO: Dummy implementation till upstream fuse driver implements these*/
static inline bool tegra_spare_fuse(int bit)
{ return 0; }
static inline int tegra_get_sku_override(void)
{ return 0; }

#endif /* __ASSEMBLY__ */
u32 tegra_fuse_get_subrevision(void);

#endif /* __SOC_TEGRA_CHIP_ID_H_ */
