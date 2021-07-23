/*
 * tegra-soc-hwpm-io.h:
 * This header defines register read/write APIs for the Tegra SOC HWPM driver.
 *
 * Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef TEGRA_SOC_HWPM_IO_H
#define TEGRA_SOC_HWPM_IO_H

#include "tegra-soc-hwpm.h"

struct whitelist {
	u64 reg;
	bool zero_in_init;
};

struct hwpm_resource_aperture {
	/*
	 * If false, this is a HWPM aperture (PERFRMON, PMA or RTR). Else this
	 * is a non-HWPM aperture (ex: VIC).
	 */
	bool is_ip;

	/*
	 * If is_ip == false, specify dt_aperture for readl/writel operations.
	 * If is_ip == true, dt_aperture == TEGRA_SOC_HWPM_INVALID_DT.
	 */
	enum tegra_soc_hwpm_dt_aperture dt_aperture;

	/* Physical aperture */
	u64 start_abs_pa;
	u64 end_abs_pa;
	u64 start_pa;
	u64 end_pa;

	/* Whitelist */
	struct whitelist *wlist;
	u64 wlist_size;

	/* Fake registers for VDK which doesn't have a SOC HWPM fmodel */
	u32 *fake_registers;
};

struct hwpm_resource {
	bool reserved;
	u32 map_size;
	struct hwpm_resource_aperture *map;
};

/* Externs */
extern struct hwpm_resource hwpm_resources[TERGA_SOC_HWPM_NUM_RESOURCES];
extern u32 *pma_fake_regs;
extern u32 *mc_fake_regs[16];
extern struct hwpm_resource_aperture mss_channel_map[];
extern struct hwpm_resource_aperture mss_iso_niso_hub_map[];
extern struct hwpm_resource_aperture mss_mcf_map[];
extern struct hwpm_resource_aperture pma_map[];
extern struct hwpm_resource_aperture cmd_slice_rtr_map[];

struct hwpm_resource_aperture *find_hwpm_aperture(struct tegra_soc_hwpm *hwpm,
						  u64 phys_addr,
						  bool use_absolute_base,
						  bool check_reservation,
						  u64 *updated_pa);
u32 hwpm_readl(struct tegra_soc_hwpm *hwpm,
		enum tegra_soc_hwpm_dt_aperture dt_aperture,
		u32 reg_offset);
void hwpm_writel(struct tegra_soc_hwpm *hwpm,
		enum tegra_soc_hwpm_dt_aperture dt_aperture,
		u32 reg_offset, u32 val);
u32 ip_readl(struct tegra_soc_hwpm *hwpm, u64 phys_addr);
void ip_writel(struct tegra_soc_hwpm *hwpm, u64 phys_addr, u32 val);
u32 ioctl_readl(struct tegra_soc_hwpm *hwpm,
		struct hwpm_resource_aperture *aperture,
		u64 addr);
void ioctl_writel(struct tegra_soc_hwpm *hwpm,
		  struct hwpm_resource_aperture *aperture,
		  u64 addr,
		  u32 val);
int reg_rmw(struct tegra_soc_hwpm *hwpm,
	    struct hwpm_resource_aperture *aperture,
	    enum tegra_soc_hwpm_dt_aperture dt_aperture,
	    u64 addr,
	    u32 field_mask,
	    u32 field_val,
	    bool is_ioctl,
	    bool is_ip);

#endif /* TEGRA_SOC_HWPM_IO_H */
