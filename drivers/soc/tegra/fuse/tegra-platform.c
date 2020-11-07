// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2014-2020, NVIDIA CORPORATION.  All rights reserved.
 */

#include <soc/tegra/chip-id.h>

#define MINOR_QT		0
#define MINOR_FPGA		1
#define MINOR_ASIM_QT		2
#define MINOR_ASIM_LINSIM	3
#define MINOR_DSIM_ASIM_LINSIM	4
#define MINOR_UNIT_FPGA		5
#define MINOR_VDK		6

#define PRE_SI_QT		1
#define PRE_SI_FPGA		2
#define PRE_SI_UNIT_FPGA	3
#define PRE_SI_ASIM_QT		4
#define PRE_SI_ASIM_LINSIM	5
#define PRE_SI_DSIM_ASIM_LINSIM	6
#define PRE_SI_VDK		8
#define PRE_SI_VSP		9

static enum tegra_platform __tegra_get_platform(void)
{
	u32 chipid, major, pre_si_plat;

	chipid = tegra_read_chipid();
	major = tegra_hidrev_get_majorrev(chipid);
	pre_si_plat = tegra_hidrev_get_pre_si_plat(chipid);

	if (!major) {
		u32 minor;

		minor = tegra_hidrev_get_minorrev(chipid);
		switch (minor) {
		case MINOR_QT:
			return TEGRA_PLATFORM_QT;
		case MINOR_FPGA:
			return TEGRA_PLATFORM_FPGA;
		case MINOR_ASIM_QT:
			return TEGRA_PLATFORM_QT;
		case MINOR_ASIM_LINSIM:
			return TEGRA_PLATFORM_LINSIM;
		case MINOR_DSIM_ASIM_LINSIM:
			return TEGRA_PLATFORM_LINSIM;
		case MINOR_UNIT_FPGA:
			return TEGRA_PLATFORM_UNIT_FPGA;
		case MINOR_VDK:
			return TEGRA_PLATFORM_VDK;
		}
	} else if (pre_si_plat) {
		switch (pre_si_plat) {
		case PRE_SI_QT:
			return TEGRA_PLATFORM_QT;
		case PRE_SI_FPGA:
			return TEGRA_PLATFORM_FPGA;
		case PRE_SI_UNIT_FPGA:
			return TEGRA_PLATFORM_UNIT_FPGA;
		case PRE_SI_ASIM_QT:
			return TEGRA_PLATFORM_QT;
		case PRE_SI_ASIM_LINSIM:
			return TEGRA_PLATFORM_LINSIM;
		case PRE_SI_DSIM_ASIM_LINSIM:
			return TEGRA_PLATFORM_LINSIM;
		case PRE_SI_VDK:
			return TEGRA_PLATFORM_VDK;
		case PRE_SI_VSP:
			return TEGRA_PLATFORM_VSP;
		}
	}

	return TEGRA_PLATFORM_SILICON;
}

static enum tegra_platform tegra_platform_id = TEGRA_PLATFORM_MAX;

enum tegra_platform tegra_get_platform(void)
{
	if (unlikely(tegra_platform_id == TEGRA_PLATFORM_MAX))
		tegra_platform_id = __tegra_get_platform();

	return tegra_platform_id;
}
EXPORT_SYMBOL(tegra_get_platform);

bool tegra_cpu_is_asim(void)
{
	u32 chipid, major, pre_si_plat;

	chipid = tegra_read_chipid();
	major = tegra_hidrev_get_majorrev(chipid);
	pre_si_plat = tegra_hidrev_get_pre_si_plat(chipid);

	if (!major) {
		u32 minor;

		minor = tegra_hidrev_get_minorrev(chipid);
		switch (minor) {
		case MINOR_QT:
		case MINOR_FPGA:
			return false;
		case MINOR_ASIM_QT:
		case MINOR_ASIM_LINSIM:
		case MINOR_DSIM_ASIM_LINSIM:
		case MINOR_UNIT_FPGA:
		case MINOR_VDK:
			return true;
		}
	} else if (pre_si_plat) {
		switch (pre_si_plat) {
		case PRE_SI_QT:
		case PRE_SI_FPGA:
			return false;
		case PRE_SI_UNIT_FPGA:
		case PRE_SI_ASIM_QT:
		case PRE_SI_ASIM_LINSIM:
		case PRE_SI_DSIM_ASIM_LINSIM:
		case PRE_SI_VDK:
			return true;
		}
	}

	return false;
}
EXPORT_SYMBOL_GPL(tegra_cpu_is_asim);

bool tegra_cpu_is_dsim(void)
{
	u32 chipid, major, pre_si_plat;

	chipid = tegra_read_chipid();
	major = tegra_hidrev_get_majorrev(chipid);
	pre_si_plat = tegra_hidrev_get_pre_si_plat(chipid);

	if (!major) {
		u32 minor;

		minor = tegra_hidrev_get_minorrev(chipid);
		switch (minor) {
		case MINOR_QT:
		case MINOR_FPGA:
		case MINOR_ASIM_QT:
		case MINOR_ASIM_LINSIM:
		case MINOR_UNIT_FPGA:
		case MINOR_VDK:
			return false;
		case MINOR_DSIM_ASIM_LINSIM:
			return true;
		}
	} else if (pre_si_plat) {
		switch (pre_si_plat) {
		case PRE_SI_QT:
		case PRE_SI_FPGA:
		case PRE_SI_UNIT_FPGA:
		case PRE_SI_ASIM_QT:
		case PRE_SI_ASIM_LINSIM:
		case PRE_SI_VDK:
			return false;
		case PRE_SI_DSIM_ASIM_LINSIM:
			return true;
		}
	}

	return false;
}
