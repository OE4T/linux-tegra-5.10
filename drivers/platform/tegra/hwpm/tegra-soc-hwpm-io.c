/*
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
 *
 * tegra-soc-hwpm-io.c:
 * This file contains register read/write functions for the Tegra SOC HWPM
 * driver.
 */

#include "tegra-soc-hwpm-io.h"
#include "include/hw/t234/hw_addr_map_soc_hwpm.h"

/* FIXME: Auto-generate whitelists */
struct whitelist perfmon_wlist[] = {
	{.reg = 0x0,	.zero_in_init = true,},
	{.reg = 0x4,	.zero_in_init = true,},
	{.reg = 0x8,	.zero_in_init = true,},
	{.reg = 0xc,	.zero_in_init = true,},
	{.reg = 0x10,	.zero_in_init = true,},
	{.reg = 0x14,	.zero_in_init = true,},
	{.reg = 0x20,	.zero_in_init = true,},
	{.reg = 0x24,	.zero_in_init = true,},
	{.reg = 0x28,	.zero_in_init = true,},
	{.reg = 0x2c,	.zero_in_init = true,},
	{.reg = 0x30,	.zero_in_init = true,},
	{.reg = 0x34,	.zero_in_init = true,},
	{.reg = 0x40,	.zero_in_init = true,},
	{.reg = 0x44,	.zero_in_init = true,},
	{.reg = 0x48,	.zero_in_init = true,},
	{.reg = 0x4c,	.zero_in_init = true,},
	{.reg = 0x50,	.zero_in_init = true,},
	{.reg = 0x54,	.zero_in_init = true,},
	{.reg = 0x58,	.zero_in_init = true,},
	{.reg = 0x5c,	.zero_in_init = true,},
	{.reg = 0x60,	.zero_in_init = true,},
	{.reg = 0x64,	.zero_in_init = true,},
	{.reg = 0x68,	.zero_in_init = true,},
	{.reg = 0x6c,	.zero_in_init = true,},
	{.reg = 0x70,	.zero_in_init = true,},
	{.reg = 0x74,	.zero_in_init = true,},
	{.reg = 0x78,	.zero_in_init = true,},
	{.reg = 0x7c,	.zero_in_init = true,},
	{.reg = 0x80,	.zero_in_init = true,},
	{.reg = 0x84,	.zero_in_init = true,},
	{.reg = 0x88,	.zero_in_init = true,},
	{.reg = 0x8c,	.zero_in_init = true,},
	{.reg = 0x90,	.zero_in_init = true,},
	{.reg = 0x98,	.zero_in_init = true,},
	{.reg = 0x9c,	.zero_in_init = true,},
	{.reg = 0xa0,	.zero_in_init = true,},
	{.reg = 0xa4,	.zero_in_init = true,},
	{.reg = 0xa8,	.zero_in_init = true,},
	{.reg = 0xac,	.zero_in_init = true,},
	{.reg = 0xb0,	.zero_in_init = true,},
	{.reg = 0xb4,	.zero_in_init = true,},
	{.reg = 0xb8,	.zero_in_init = true,},
	{.reg = 0xbc,	.zero_in_init = true,},
	{.reg = 0xc0,	.zero_in_init = true,},
	{.reg = 0xc4,	.zero_in_init = true,},
	{.reg = 0xc8,	.zero_in_init = true,},
	{.reg = 0xcc,	.zero_in_init = true,},
	{.reg = 0xd0,	.zero_in_init = true,},
	{.reg = 0xd4,	.zero_in_init = true,},
	{.reg = 0xd8,	.zero_in_init = true,},
	{.reg = 0xdc,	.zero_in_init = true,},
	{.reg = 0xe0,	.zero_in_init = true,},
	{.reg = 0xe4,	.zero_in_init = true,},
	{.reg = 0xe8,	.zero_in_init = true,},
	{.reg = 0xec,	.zero_in_init = true,},
	{.reg = 0xf8,	.zero_in_init = true,},
	{.reg = 0xfc,	.zero_in_init = true,},
	{.reg = 0x100,	.zero_in_init = true,},
	{.reg = 0x108,	.zero_in_init = true,},
	{.reg = 0x110,	.zero_in_init = true,},
	{.reg = 0x114,	.zero_in_init = true,},
	{.reg = 0x118,	.zero_in_init = true,},
	{.reg = 0x11c,	.zero_in_init = true,},
	{.reg = 0x120,	.zero_in_init = true,},
	{.reg = 0x124,	.zero_in_init = true,},
	{.reg = 0x128,	.zero_in_init = true,},
	{.reg = 0x130,	.zero_in_init = true,},
};

struct whitelist vi_thi_wlist[] = {
	{.reg = (0x1088 << 2),	.zero_in_init = false,},
	{.reg = (0x3a00 << 2),	.zero_in_init = false,},
	{.reg = (0x3a01 << 2),	.zero_in_init = false,},
	{.reg = (0x3a02 << 2),	.zero_in_init = true,},
	{.reg = (0x3a03 << 2),	.zero_in_init = true,},
	{.reg = (0x3a04 << 2),	.zero_in_init = true,},
	{.reg = (0x3a05 << 2),	.zero_in_init = true,},
	{.reg = (0x3a06 << 2),	.zero_in_init = true,},
};
struct whitelist vi2_thi_wlist[] = {
	{.reg = (0x1088 << 2),	.zero_in_init = false,},
	{.reg = (0x3a00 << 2),	.zero_in_init = false,},
	{.reg = (0x3a01 << 2),	.zero_in_init = false,},
	{.reg = (0x3a02 << 2),	.zero_in_init = true,},
	{.reg = (0x3a03 << 2),	.zero_in_init = true,},
	{.reg = (0x3a04 << 2),	.zero_in_init = true,},
	{.reg = (0x3a05 << 2),	.zero_in_init = true,},
	{.reg = (0x3a06 << 2),	.zero_in_init = true,},
};
/*
 * Aperture Ranges (start_pa/end_pa):
 *    - start_pa and end_pa is 0 for PERFMON, PMA, and RTR apertures. These
 *      ranges will be extracted from the device tree.
 *    - IP apertures are not listed in the device tree because we don't map them.
 *      Therefore, start_pa and end_pa for IP apertures are hardcoded here. IP
 *      apertures are listed here because we need to track their whitelists.
 */
struct hwpm_resource_aperture vi_map[] = {
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_VI0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_VI0_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_VI0_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_VI1_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_VI1_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_VI1_PERFMON_DT,
	},
	{
		.start_pa = addr_map_vi_thi_base_r(),
		.end_pa = addr_map_vi_thi_limit_r(),
		.start_abs_pa = addr_map_vi_thi_base_r(),
		.end_abs_pa = addr_map_vi_thi_limit_r(),
		.fake_registers = NULL,
		.wlist = vi_thi_wlist,
		.wlist_size = ARRAY_SIZE(vi_thi_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_vi2_thi_base_r(),
		.end_pa = addr_map_vi2_thi_limit_r(),
		.start_abs_pa = addr_map_vi2_thi_base_r(),
		.end_abs_pa = addr_map_vi2_thi_limit_r(),
		.fake_registers = NULL,
		.wlist = vi2_thi_wlist,
		.wlist_size = ARRAY_SIZE(vi2_thi_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
};

struct whitelist isp_thi_wlist[] = {
	{.reg = (0x1088 << 2),	.zero_in_init = false,},
	{.reg = (0x2470 << 2),	.zero_in_init = false,},
	{.reg = (0x2471 << 2),	.zero_in_init = false,},
	{.reg = (0x2472 << 2),	.zero_in_init = true,},
	{.reg = (0x2473 << 2),	.zero_in_init = true,},
	{.reg = (0x2474 << 2),	.zero_in_init = true,},
	{.reg = (0x2475 << 2),	.zero_in_init = true,},
	{.reg = (0x2476 << 2),	.zero_in_init = true,},
};
struct hwpm_resource_aperture isp_map[] = {
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_ISP0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_ISP0_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_ISP0_PERFMON_DT,
	},
	{
		.start_pa = addr_map_isp_thi_base_r(),
		.end_pa = addr_map_isp_thi_limit_r(),
		.start_abs_pa = addr_map_isp_thi_base_r(),
		.end_abs_pa = addr_map_isp_thi_limit_r(),
		.fake_registers = NULL,
		.wlist = isp_thi_wlist,
		.wlist_size = ARRAY_SIZE(isp_thi_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
};

struct whitelist vic_wlist[] = {
	{.reg = 0x1088,	.zero_in_init = false,},
	{.reg = 0x10a8,	.zero_in_init = false,},
	{.reg = 0x1c00,	.zero_in_init = true,},
	{.reg = 0x1c04,	.zero_in_init = true,},
	{.reg = 0x1c08,	.zero_in_init = true,},
	{.reg = 0x1c0c,	.zero_in_init = true,},
	{.reg = 0x1c10,	.zero_in_init = true,},
	{.reg = 0x1c14,	.zero_in_init = false,},
	{.reg = 0x1c18,	.zero_in_init = false,},
};
struct hwpm_resource_aperture vic_map[] = {
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_VICA0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_VICA0_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_VICA0_PERFMON_DT,
	},
	{
		.start_pa = addr_map_vic_base_r(),
		.end_pa = addr_map_vic_limit_r(),
		.start_abs_pa = addr_map_vic_base_r(),
		.end_abs_pa = addr_map_vic_limit_r(),
		.fake_registers = NULL,
		.wlist = vic_wlist,
		.wlist_size = ARRAY_SIZE(vic_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
};

struct whitelist ofa_wlist[] = {
	{.reg = 0x1088,	.zero_in_init = false,},
	{.reg = 0x3308,	.zero_in_init = true,},
	{.reg = 0x330c,	.zero_in_init = true,},
	{.reg = 0x3310,	.zero_in_init = true,},
	{.reg = 0x3314,	.zero_in_init = true,},
	{.reg = 0x3318,	.zero_in_init = false,},
	{.reg = 0x331c,	.zero_in_init = false,},
};
struct hwpm_resource_aperture ofa_map[] = {
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_OFAA0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_OFAA0_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_OFAA0_PERFMON_DT,
	},
	{
		.start_pa = addr_map_ofa_base_r(),
		.end_pa = addr_map_ofa_limit_r(),
		.start_abs_pa = addr_map_ofa_base_r(),
		.end_abs_pa = addr_map_ofa_limit_r(),
		.fake_registers = NULL,
		.wlist = ofa_wlist,
		.wlist_size = ARRAY_SIZE(ofa_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
};

struct whitelist pva0_pm_wlist[] = {
	{.reg = 0x8000,	.zero_in_init = false,},
	{.reg = 0x8004,	.zero_in_init = false,},
	{.reg = 0x8008,	.zero_in_init = false,},
	{.reg = 0x800c,	.zero_in_init = true,},
	{.reg = 0x8010,	.zero_in_init = true,},
	{.reg = 0x8014,	.zero_in_init = true,},
	{.reg = 0x8018,	.zero_in_init = true,},
	{.reg = 0x801c,	.zero_in_init = true,},
	{.reg = 0x8020,	.zero_in_init = true,},
};
/* FIXME: Any missing apertures? */
struct hwpm_resource_aperture pva_map[] = {
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_PVAV0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_PVAV0_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_PVAV0_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_PVAV1_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_PVAV1_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_PVAV1_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_PVAC0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_PVAC0_PERFMON_DT),
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.fake_registers = NULL,
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_PVAC0_PERFMON_DT,
	},
	{
		.start_pa = addr_map_pva0_pm_base_r(),
		.end_pa = addr_map_pva0_pm_limit_r(),
		.start_abs_pa = addr_map_pva0_pm_base_r(),
		.end_abs_pa = addr_map_pva0_pm_limit_r(),
		.fake_registers = NULL,
		.wlist = pva0_pm_wlist,
		.wlist_size = ARRAY_SIZE(pva0_pm_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
};

struct whitelist nvdla_wlist[] = {
	{.reg = 0x1088,		.zero_in_init = false,},
	{.reg = 0x1a000,	.zero_in_init = false,},
	{.reg = 0x1a004,	.zero_in_init = false,},
	{.reg = 0x1a008,	.zero_in_init = true,},
	{.reg = 0x1a01c,	.zero_in_init = true,},
	{.reg = 0x1a030,	.zero_in_init = true,},
	{.reg = 0x1a044,	.zero_in_init = true,},
	{.reg = 0x1a058,	.zero_in_init = true,},
	{.reg = 0x1a06c,	.zero_in_init = true,},
};
struct hwpm_resource_aperture nvdla_map[] = {
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_NVDLAB0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_NVDLAB0_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_NVDLAB0_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_NVDLAB1_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_NVDLAB1_PERFMON_DT),
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.fake_registers = NULL,
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_NVDLAB1_PERFMON_DT,
	},
	{
		.start_pa = addr_map_nvdla0_base_r(),
		.end_pa = addr_map_nvdla0_limit_r(),
		.start_abs_pa = addr_map_nvdla0_base_r(),
		.end_abs_pa = addr_map_nvdla0_limit_r(),
		.fake_registers = NULL,
		.wlist = nvdla_wlist,
		.wlist_size = ARRAY_SIZE(nvdla_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_nvdla1_base_r(),
		.end_pa = addr_map_nvdla1_limit_r(),
		.start_abs_pa = addr_map_nvdla1_base_r(),
		.end_abs_pa = addr_map_nvdla1_limit_r(),
		.fake_registers = NULL,
		.wlist = nvdla_wlist,
		.wlist_size = ARRAY_SIZE(nvdla_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
};

struct whitelist mgbe_wlist[] = {
	{.reg = 0x8020,	.zero_in_init = true,},
	{.reg = 0x8024,	.zero_in_init = false,},
};
struct hwpm_resource_aperture mgbe_map[] = {
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MGBE0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MGBE0_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MGBE0_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MGBE1_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MGBE1_PERFMON_DT),
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.fake_registers = NULL,
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MGBE1_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MGBE2_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MGBE2_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MGBE2_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MGBE3_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MGBE3_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MGBE3_PERFMON_DT,
	},
	{
		.start_pa = addr_map_mgbe0_base_r(),
		.end_pa = addr_map_mgbe0_limit_r(),
		.start_abs_pa = addr_map_mgbe0_base_r(),
		.end_abs_pa = addr_map_mgbe0_limit_r(),
		.fake_registers = NULL,
		.wlist = mgbe_wlist,
		.wlist_size = ARRAY_SIZE(mgbe_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mgbe1_base_r(),
		.end_pa = addr_map_mgbe1_limit_r(),
		.start_abs_pa = addr_map_mgbe1_base_r(),
		.end_abs_pa = addr_map_mgbe1_limit_r(),
		.fake_registers = NULL,
		.wlist = mgbe_wlist,
		.wlist_size = ARRAY_SIZE(mgbe_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mgbe2_base_r(),
		.end_pa = addr_map_mgbe2_limit_r(),
		.start_abs_pa = addr_map_mgbe2_base_r(),
		.end_abs_pa = addr_map_mgbe2_limit_r(),
		.fake_registers = NULL,
		.wlist = mgbe_wlist,
		.wlist_size = ARRAY_SIZE(mgbe_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mgbe3_base_r(),
		.end_pa = addr_map_mgbe3_limit_r(),
		.start_abs_pa = addr_map_mgbe3_base_r(),
		.end_abs_pa = addr_map_mgbe3_limit_r(),
		.fake_registers = NULL,
		.wlist = mgbe_wlist,
		.wlist_size = ARRAY_SIZE(mgbe_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
};

struct hwpm_resource_aperture scf_map[] = {
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_SCF0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_SCF0_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_SCF0_PERFMON_DT,
	},
};

struct whitelist nvdec_wlist[] = {
	{.reg = 0x1088,	.zero_in_init = false,},
	{.reg = 0x1b48,	.zero_in_init = false,},
	{.reg = 0x1b4c,	.zero_in_init = false,},
	{.reg = 0x1b50,	.zero_in_init = true,},
	{.reg = 0x1b54,	.zero_in_init = true,},
	{.reg = 0x1b58,	.zero_in_init = true,},
	{.reg = 0x1b5c,	.zero_in_init = true,},
};
struct hwpm_resource_aperture nvdec_map[] = {
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_NVDECA0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_NVDECA0_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_NVDECA0_PERFMON_DT,
	},
	{
		.start_pa = addr_map_nvdec_base_r(),
		.end_pa = addr_map_nvdec_limit_r(),
		.start_abs_pa = addr_map_nvdec_base_r(),
		.end_abs_pa = addr_map_nvdec_limit_r(),
		.fake_registers = NULL,
		.wlist = nvdec_wlist,
		.wlist_size = ARRAY_SIZE(nvdec_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
};

struct whitelist nvenc_wlist[] = {
	{.reg = 0x1088,	.zero_in_init = false,},
	{.reg = 0x212c,	.zero_in_init = false,},
	{.reg = 0x2130,	.zero_in_init = false,},
	{.reg = 0x2134,	.zero_in_init = true,},
};
struct hwpm_resource_aperture nvenc_map[] = {
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_NVENCA0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_NVENCA0_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_NVENCA0_PERFMON_DT,
	},
	{
		.start_pa = addr_map_nvenc_base_r(),
		.end_pa = addr_map_nvenc_limit_r(),
		.start_abs_pa = addr_map_nvenc_base_r(),
		.end_abs_pa = addr_map_nvenc_limit_r(),
		.fake_registers = NULL,
		.wlist = nvenc_wlist,
		.wlist_size = ARRAY_SIZE(nvenc_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
};

struct whitelist pcie_ctl_wlist[] = {
	{.reg = 0x174,	.zero_in_init = true,},
	{.reg = 0x178,	.zero_in_init = false,},
};
struct hwpm_resource_aperture pcie_map[] = {
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_PCIE0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_PCIE0_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_PCIE0_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_PCIE1_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_PCIE1_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_PCIE1_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_PCIE2_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_PCIE2_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_PCIE2_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_PCIE3_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_PCIE3_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_PCIE3_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_PCIE4_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_PCIE4_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_PCIE4_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_PCIE5_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_PCIE5_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_PCIE5_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_PCIE6_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_PCIE6_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_PCIE6_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_PCIE7_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_PCIE7_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_PCIE7_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_PCIE8_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_PCIE8_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_PCIE8_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_PCIE9_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_PCIE9_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_PCIE9_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_PCIE10_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_PCIE10_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_PCIE10_PERFMON_DT,
	},
	{
		.start_pa = addr_map_pcie_c0_ctl_base_r(),
		.end_pa = addr_map_pcie_c0_ctl_limit_r(),
		.start_abs_pa = addr_map_pcie_c0_ctl_base_r(),
		.end_abs_pa = addr_map_pcie_c0_ctl_limit_r(),
		.fake_registers = NULL,
		.wlist = pcie_ctl_wlist,
		.wlist_size = ARRAY_SIZE(pcie_ctl_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_pcie_c1_ctl_base_r(),
		.end_pa = addr_map_pcie_c1_ctl_limit_r(),
		.start_abs_pa = addr_map_pcie_c1_ctl_base_r(),
		.end_abs_pa = addr_map_pcie_c1_ctl_limit_r(),
		.fake_registers = NULL,
		.wlist = pcie_ctl_wlist,
		.wlist_size = ARRAY_SIZE(pcie_ctl_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_pcie_c2_ctl_base_r(),
		.end_pa = addr_map_pcie_c2_ctl_limit_r(),
		.start_abs_pa = addr_map_pcie_c2_ctl_base_r(),
		.end_abs_pa = addr_map_pcie_c2_ctl_limit_r(),
		.fake_registers = NULL,
		.wlist = pcie_ctl_wlist,
		.wlist_size = ARRAY_SIZE(pcie_ctl_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_pcie_c3_ctl_base_r(),
		.end_pa = addr_map_pcie_c3_ctl_limit_r(),
		.start_abs_pa = addr_map_pcie_c3_ctl_base_r(),
		.end_abs_pa = addr_map_pcie_c3_ctl_limit_r(),
		.fake_registers = NULL,
		.wlist = pcie_ctl_wlist,
		.wlist_size = ARRAY_SIZE(pcie_ctl_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_pcie_c4_ctl_base_r(),
		.end_pa = addr_map_pcie_c4_ctl_limit_r(),
		.start_abs_pa = addr_map_pcie_c4_ctl_base_r(),
		.end_abs_pa = addr_map_pcie_c4_ctl_limit_r(),
		.fake_registers = NULL,
		.wlist = pcie_ctl_wlist,
		.wlist_size = ARRAY_SIZE(pcie_ctl_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_pcie_c5_ctl_base_r(),
		.end_pa = addr_map_pcie_c5_ctl_limit_r(),
		.start_abs_pa = addr_map_pcie_c5_ctl_base_r(),
		.end_abs_pa = addr_map_pcie_c5_ctl_limit_r(),
		.fake_registers = NULL,
		.wlist = pcie_ctl_wlist,
		.wlist_size = ARRAY_SIZE(pcie_ctl_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_pcie_c6_ctl_base_r(),
		.end_pa = addr_map_pcie_c6_ctl_limit_r(),
		.start_abs_pa = addr_map_pcie_c6_ctl_base_r(),
		.end_abs_pa = addr_map_pcie_c6_ctl_limit_r(),
		.fake_registers = NULL,
		.wlist = pcie_ctl_wlist,
		.wlist_size = ARRAY_SIZE(pcie_ctl_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_pcie_c7_ctl_base_r(),
		.end_pa = addr_map_pcie_c7_ctl_limit_r(),
		.start_abs_pa = addr_map_pcie_c7_ctl_base_r(),
		.end_abs_pa = addr_map_pcie_c7_ctl_limit_r(),
		.fake_registers = NULL,
		.wlist = pcie_ctl_wlist,
		.wlist_size = ARRAY_SIZE(pcie_ctl_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_pcie_c8_ctl_base_r(),
		.end_pa = addr_map_pcie_c8_ctl_limit_r(),
		.start_abs_pa = addr_map_pcie_c8_ctl_base_r(),
		.end_abs_pa = addr_map_pcie_c8_ctl_limit_r(),
		.fake_registers = NULL,
		.wlist = pcie_ctl_wlist,
		.wlist_size = ARRAY_SIZE(pcie_ctl_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_pcie_c9_ctl_base_r(),
		.end_pa = addr_map_pcie_c9_ctl_limit_r(),
		.start_abs_pa = addr_map_pcie_c9_ctl_base_r(),
		.end_abs_pa = addr_map_pcie_c9_ctl_limit_r(),
		.fake_registers = NULL,
		.wlist = pcie_ctl_wlist,
		.wlist_size = ARRAY_SIZE(pcie_ctl_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_pcie_c10_ctl_base_r(),
		.end_pa = addr_map_pcie_c10_ctl_limit_r(),
		.start_abs_pa = addr_map_pcie_c10_ctl_base_r(),
		.end_abs_pa = addr_map_pcie_c10_ctl_limit_r(),
		.fake_registers = NULL,
		.wlist = pcie_ctl_wlist,
		.wlist_size = ARRAY_SIZE(pcie_ctl_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
};

struct whitelist disp_wlist[] = {
	{.reg = 0x1e118,	.zero_in_init = true,},
	{.reg = 0x1e120,	.zero_in_init = true,},
	{.reg = 0x1e124,	.zero_in_init = false,},
};
struct hwpm_resource_aperture display_map[] = {
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_NVDISPLAY0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_NVDISPLAY0_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_NVDISPLAY0_PERFMON_DT,
	},
	{
		.start_pa = addr_map_disp_base_r(),
		.end_pa = addr_map_disp_limit_r(),
		.start_abs_pa = addr_map_disp_base_r(),
		.end_abs_pa = addr_map_disp_limit_r(),
		.fake_registers = NULL,
		.wlist = disp_wlist,
		.wlist_size = ARRAY_SIZE(disp_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
};

/*
 * Normally there is a 1-to-1 mapping between an MMIO aperture and a
 * hwpm_resource_aperture struct. But MC MMIO apertures are used in multiple
 * hwpm_resource_aperture structs. Therefore, we have to share the fake register
 * arrays between these hwpm_resource_aperture structs. This is why we have to
 * define the fake register arrays globally. For all other 1-to-1 mapping
 * apertures the fake register arrays are directly embedded inside the
 * hwpm_resource_aperture structs.
 */
u32 *mc_fake_regs[16] = {NULL};
/* FIXME: Any missing registers? */
struct whitelist mc_res_mss_channel_wlist[] = {
	{.reg = 0x814,	.zero_in_init = true,},
};
struct hwpm_resource_aperture mss_channel_map[] = {
	{
		.start_pa = addr_map_mc0_base_r(),
		.end_pa = addr_map_mc0_limit_r(),
		.start_abs_pa = addr_map_mc0_base_r(),
		.end_abs_pa = addr_map_mc0_limit_r(),
		.fake_registers = NULL,
		.wlist = mc_res_mss_channel_wlist,
		.wlist_size = ARRAY_SIZE(mc_res_mss_channel_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc1_base_r(),
		.end_pa = addr_map_mc1_limit_r(),
		.start_abs_pa = addr_map_mc1_base_r(),
		.end_abs_pa = addr_map_mc1_limit_r(),
		.fake_registers = NULL,
		.wlist = mc_res_mss_channel_wlist,
		.wlist_size = ARRAY_SIZE(mc_res_mss_channel_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc2_base_r(),
		.end_pa = addr_map_mc2_limit_r(),
		.start_abs_pa = addr_map_mc2_base_r(),
		.end_abs_pa = addr_map_mc2_limit_r(),
		.fake_registers = NULL,
		.wlist = mc_res_mss_channel_wlist,
		.wlist_size = ARRAY_SIZE(mc_res_mss_channel_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc3_base_r(),
		.end_pa = addr_map_mc3_limit_r(),
		.start_abs_pa = addr_map_mc3_base_r(),
		.end_abs_pa = addr_map_mc3_limit_r(),
		.fake_registers = NULL,
		.wlist = mc_res_mss_channel_wlist,
		.wlist_size = ARRAY_SIZE(mc_res_mss_channel_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc4_base_r(),
		.end_pa = addr_map_mc4_limit_r(),
		.start_abs_pa = addr_map_mc4_base_r(),
		.end_abs_pa = addr_map_mc4_limit_r(),
		.fake_registers = NULL,
		.wlist = mc_res_mss_channel_wlist,
		.wlist_size = ARRAY_SIZE(mc_res_mss_channel_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc5_base_r(),
		.end_pa = addr_map_mc5_limit_r(),
		.start_abs_pa = addr_map_mc5_base_r(),
		.end_abs_pa = addr_map_mc5_limit_r(),
		.fake_registers = NULL,
		.wlist = mc_res_mss_channel_wlist,
		.wlist_size = ARRAY_SIZE(mc_res_mss_channel_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc6_base_r(),
		.end_pa = addr_map_mc6_limit_r(),
		.start_abs_pa = addr_map_mc6_base_r(),
		.end_abs_pa = addr_map_mc6_limit_r(),
		.fake_registers = NULL,
		.wlist = mc_res_mss_channel_wlist,
		.wlist_size = ARRAY_SIZE(mc_res_mss_channel_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc7_base_r(),
		.end_pa = addr_map_mc7_limit_r(),
		.start_abs_pa = addr_map_mc7_base_r(),
		.end_abs_pa = addr_map_mc7_limit_r(),
		.fake_registers = NULL,
		.wlist = mc_res_mss_channel_wlist,
		.wlist_size = ARRAY_SIZE(mc_res_mss_channel_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc8_base_r(),
		.end_pa = addr_map_mc8_limit_r(),
		.start_abs_pa = addr_map_mc8_base_r(),
		.end_abs_pa = addr_map_mc8_limit_r(),
		.fake_registers = NULL,
		.wlist = mc_res_mss_channel_wlist,
		.wlist_size = ARRAY_SIZE(mc_res_mss_channel_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc9_base_r(),
		.end_pa = addr_map_mc9_limit_r(),
		.start_abs_pa = addr_map_mc9_base_r(),
		.end_abs_pa = addr_map_mc9_limit_r(),
		.fake_registers = NULL,
		.wlist = mc_res_mss_channel_wlist,
		.wlist_size = ARRAY_SIZE(mc_res_mss_channel_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc10_base_r(),
		.end_pa = addr_map_mc10_limit_r(),
		.start_abs_pa = addr_map_mc10_base_r(),
		.end_abs_pa = addr_map_mc10_limit_r(),
		.fake_registers = NULL,
		.wlist = mc_res_mss_channel_wlist,
		.wlist_size = ARRAY_SIZE(mc_res_mss_channel_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc11_base_r(),
		.end_pa = addr_map_mc11_limit_r(),
		.start_abs_pa = addr_map_mc11_base_r(),
		.end_abs_pa = addr_map_mc11_limit_r(),
		.fake_registers = NULL,
		.wlist = mc_res_mss_channel_wlist,
		.wlist_size = ARRAY_SIZE(mc_res_mss_channel_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc12_base_r(),
		.end_pa = addr_map_mc12_limit_r(),
		.start_abs_pa = addr_map_mc12_base_r(),
		.end_abs_pa = addr_map_mc12_limit_r(),
		.fake_registers = NULL,
		.wlist = mc_res_mss_channel_wlist,
		.wlist_size = ARRAY_SIZE(mc_res_mss_channel_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc13_base_r(),
		.end_pa = addr_map_mc13_limit_r(),
		.start_abs_pa = addr_map_mc13_base_r(),
		.end_abs_pa = addr_map_mc13_limit_r(),
		.fake_registers = NULL,
		.wlist = mc_res_mss_channel_wlist,
		.wlist_size = ARRAY_SIZE(mc_res_mss_channel_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc14_base_r(),
		.end_pa = addr_map_mc14_limit_r(),
		.start_abs_pa = addr_map_mc14_base_r(),
		.end_abs_pa = addr_map_mc14_limit_r(),
		.fake_registers = NULL,
		.wlist = mc_res_mss_channel_wlist,
		.wlist_size = ARRAY_SIZE(mc_res_mss_channel_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc15_base_r(),
		.end_pa = addr_map_mc15_limit_r(),
		.start_abs_pa = addr_map_mc15_base_r(),
		.end_abs_pa = addr_map_mc15_limit_r(),
		.fake_registers = NULL,
		.wlist = mc_res_mss_channel_wlist,
		.wlist_size = ARRAY_SIZE(mc_res_mss_channel_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSCHANNELPARTA0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSCHANNELPARTA0_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSCHANNELPARTA0_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSCHANNELPARTA1_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSCHANNELPARTA1_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSCHANNELPARTA1_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSCHANNELPARTA2_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSCHANNELPARTA2_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSCHANNELPARTA2_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSCHANNELPARTA3_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSCHANNELPARTA3_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSCHANNELPARTA3_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSCHANNELPARTB0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSCHANNELPARTB0_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSCHANNELPARTB0_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSCHANNELPARTB1_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSCHANNELPARTB1_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSCHANNELPARTB1_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSCHANNELPARTB2_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSCHANNELPARTB2_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSCHANNELPARTB2_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSCHANNELPARTB3_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSCHANNELPARTB3_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSCHANNELPARTB3_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSCHANNELPARTC0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSCHANNELPARTC0_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSCHANNELPARTC0_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSCHANNELPARTC1_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSCHANNELPARTC1_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSCHANNELPARTC1_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSCHANNELPARTC2_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSCHANNELPARTC2_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSCHANNELPARTC2_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSCHANNELPARTC3_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSCHANNELPARTC3_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSCHANNELPARTC3_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSCHANNELPARTD0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSCHANNELPARTD0_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSCHANNELPARTD0_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSCHANNELPARTD1_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSCHANNELPARTD1_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSCHANNELPARTD1_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSCHANNELPARTD2_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSCHANNELPARTD2_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSCHANNELPARTD2_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSCHANNELPARTD3_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSCHANNELPARTD3_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSCHANNELPARTD3_PERFMON_DT,
	},
};

/* FIXME: Any missing registers? */
struct whitelist mss_nvlink_wlist[] = {
	{.reg = 0xa30,	.zero_in_init = true,},
};
struct hwpm_resource_aperture mss_gpu_hub_map[] = {
	{
		.start_pa = addr_map_mss_nvlink_1_base_r(),
		.end_pa = addr_map_mss_nvlink_1_limit_r(),
		.start_abs_pa = addr_map_mss_nvlink_1_base_r(),
		.end_abs_pa = addr_map_mss_nvlink_1_limit_r(),
		.fake_registers = NULL,
		.wlist = mss_nvlink_wlist,
		.wlist_size = ARRAY_SIZE(mss_nvlink_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mss_nvlink_2_base_r(),
		.end_pa = addr_map_mss_nvlink_2_limit_r(),
		.start_abs_pa = addr_map_mss_nvlink_2_base_r(),
		.end_abs_pa = addr_map_mss_nvlink_2_limit_r(),
		.fake_registers = NULL,
		.wlist = mss_nvlink_wlist,
		.wlist_size = ARRAY_SIZE(mss_nvlink_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mss_nvlink_3_base_r(),
		.end_pa = addr_map_mss_nvlink_3_limit_r(),
		.start_abs_pa = addr_map_mss_nvlink_3_base_r(),
		.end_abs_pa = addr_map_mss_nvlink_3_limit_r(),
		.fake_registers = NULL,
		.wlist = mss_nvlink_wlist,
		.wlist_size = ARRAY_SIZE(mss_nvlink_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mss_nvlink_4_base_r(),
		.end_pa = addr_map_mss_nvlink_4_limit_r(),
		.start_abs_pa = addr_map_mss_nvlink_4_base_r(),
		.end_abs_pa = addr_map_mss_nvlink_4_limit_r(),
		.fake_registers = NULL,
		.wlist = mss_nvlink_wlist,
		.wlist_size = ARRAY_SIZE(mss_nvlink_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mss_nvlink_5_base_r(),
		.end_pa = addr_map_mss_nvlink_5_limit_r(),
		.start_abs_pa = addr_map_mss_nvlink_5_base_r(),
		.end_abs_pa = addr_map_mss_nvlink_5_limit_r(),
		.fake_registers = NULL,
		.wlist = mss_nvlink_wlist,
		.wlist_size = ARRAY_SIZE(mss_nvlink_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mss_nvlink_6_base_r(),
		.end_pa = addr_map_mss_nvlink_6_limit_r(),
		.start_abs_pa = addr_map_mss_nvlink_6_base_r(),
		.end_abs_pa = addr_map_mss_nvlink_6_limit_r(),
		.fake_registers = NULL,
		.wlist = mss_nvlink_wlist,
		.wlist_size = ARRAY_SIZE(mss_nvlink_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mss_nvlink_7_base_r(),
		.end_pa = addr_map_mss_nvlink_7_limit_r(),
		.start_abs_pa = addr_map_mss_nvlink_7_base_r(),
		.end_abs_pa = addr_map_mss_nvlink_7_limit_r(),
		.fake_registers = NULL,
		.wlist = mss_nvlink_wlist,
		.wlist_size = ARRAY_SIZE(mss_nvlink_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mss_nvlink_8_base_r(),
		.end_pa = addr_map_mss_nvlink_8_limit_r(),
		.start_abs_pa = addr_map_mss_nvlink_8_base_r(),
		.end_abs_pa = addr_map_mss_nvlink_8_limit_r(),
		.fake_registers = NULL,
		.wlist = mss_nvlink_wlist,
		.wlist_size = ARRAY_SIZE(mss_nvlink_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSNVLHSH0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSNVLHSH0_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSNVLHSH0_PERFMON_DT,
	},
};

/* FIXME: Any missing registers? */
struct whitelist mc0to7_res_mss_iso_niso_hub_wlist[] = {
	{.reg = 0x818,	.zero_in_init = true,},
	{.reg = 0x81c,	.zero_in_init = true,},
};
/* FIXME: Any missing registers? */
struct whitelist mc8_res_mss_iso_niso_hub_wlist[] = {
	{.reg = 0x828,	.zero_in_init = true,},
};
struct hwpm_resource_aperture mss_iso_niso_hub_map[] = {
	{
		.start_pa = addr_map_mc0_base_r(),
		.end_pa = addr_map_mc0_limit_r(),
		.start_abs_pa = addr_map_mc0_base_r(),
		.end_abs_pa = addr_map_mc0_limit_r(),
		.fake_registers = NULL,
		.wlist = mc0to7_res_mss_iso_niso_hub_wlist,
		.wlist_size = ARRAY_SIZE(mc0to7_res_mss_iso_niso_hub_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc1_base_r(),
		.end_pa = addr_map_mc1_limit_r(),
		.start_abs_pa = addr_map_mc1_base_r(),
		.end_abs_pa = addr_map_mc1_limit_r(),
		.fake_registers = NULL,
		.wlist = mc0to7_res_mss_iso_niso_hub_wlist,
		.wlist_size = ARRAY_SIZE(mc0to7_res_mss_iso_niso_hub_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc2_base_r(),
		.end_pa = addr_map_mc2_limit_r(),
		.start_abs_pa = addr_map_mc2_base_r(),
		.end_abs_pa = addr_map_mc2_limit_r(),
		.fake_registers = NULL,
		.wlist = mc0to7_res_mss_iso_niso_hub_wlist,
		.wlist_size = ARRAY_SIZE(mc0to7_res_mss_iso_niso_hub_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc3_base_r(),
		.end_pa = addr_map_mc3_limit_r(),
		.start_abs_pa = addr_map_mc3_base_r(),
		.end_abs_pa = addr_map_mc3_limit_r(),
		.fake_registers = NULL,
		.wlist = mc0to7_res_mss_iso_niso_hub_wlist,
		.wlist_size = ARRAY_SIZE(mc0to7_res_mss_iso_niso_hub_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc4_base_r(),
		.end_pa = addr_map_mc4_limit_r(),
		.start_abs_pa = addr_map_mc4_base_r(),
		.end_abs_pa = addr_map_mc4_limit_r(),
		.fake_registers = NULL,
		.wlist = mc0to7_res_mss_iso_niso_hub_wlist,
		.wlist_size = ARRAY_SIZE(mc0to7_res_mss_iso_niso_hub_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc5_base_r(),
		.end_pa = addr_map_mc5_limit_r(),
		.start_abs_pa = addr_map_mc5_base_r(),
		.end_abs_pa = addr_map_mc5_limit_r(),
		.fake_registers = NULL,
		.wlist = mc0to7_res_mss_iso_niso_hub_wlist,
		.wlist_size = ARRAY_SIZE(mc0to7_res_mss_iso_niso_hub_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc6_base_r(),
		.end_pa = addr_map_mc6_limit_r(),
		.start_abs_pa = addr_map_mc6_base_r(),
		.end_abs_pa = addr_map_mc6_limit_r(),
		.fake_registers = NULL,
		.wlist = mc0to7_res_mss_iso_niso_hub_wlist,
		.wlist_size = ARRAY_SIZE(mc0to7_res_mss_iso_niso_hub_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc7_base_r(),
		.end_pa = addr_map_mc7_limit_r(),
		.start_abs_pa = addr_map_mc7_base_r(),
		.end_abs_pa = addr_map_mc7_limit_r(),
		.fake_registers = NULL,
		.wlist = mc0to7_res_mss_iso_niso_hub_wlist,
		.wlist_size = ARRAY_SIZE(mc0to7_res_mss_iso_niso_hub_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc8_base_r(),
		.end_pa = addr_map_mc8_limit_r(),
		.start_abs_pa = addr_map_mc8_base_r(),
		.end_abs_pa = addr_map_mc8_limit_r(),
		.fake_registers = NULL,
		.wlist = mc8_res_mss_iso_niso_hub_wlist,
		.wlist_size = ARRAY_SIZE(mc8_res_mss_iso_niso_hub_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSHUB0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSHUB0_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSHUB0_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSHUB1_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSHUB1_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSHUB1_PERFMON_DT,
	},
};

/* FIXME: Any missing registers? */
struct whitelist mcb_res_mss_mcf_wlist[] = {
	{.reg = 0x800,	.zero_in_init = true,},
	{.reg = 0x820,	.zero_in_init = true,},
	{.reg = 0x80c,	.zero_in_init = true,},
	{.reg = 0x824,	.zero_in_init = true,},
};
/* FIXME: Any missing registers? */
struct whitelist mc0to1_res_mss_mcf_wlist[] = {
	{.reg = 0x810,	.zero_in_init = true,},
	{.reg = 0x808,	.zero_in_init = true,},
	{.reg = 0x804,	.zero_in_init = true,},
};
/* FIXME: Any missing registers? */
struct whitelist mc2to7_res_mss_mcf_wlist[] = {
	{.reg = 0x810,	.zero_in_init = true,},
};
struct hwpm_resource_aperture mss_mcf_map[] = {
	{
		.start_pa = addr_map_mc0_base_r(),
		.end_pa = addr_map_mc0_limit_r(),
		.start_abs_pa = addr_map_mc0_base_r(),
		.end_abs_pa = addr_map_mc0_limit_r(),
		.fake_registers = NULL,
		.wlist = mc0to1_res_mss_mcf_wlist,
		.wlist_size = ARRAY_SIZE(mc0to1_res_mss_mcf_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc1_base_r(),
		.end_pa = addr_map_mc1_limit_r(),
		.start_abs_pa = addr_map_mc1_base_r(),
		.end_abs_pa = addr_map_mc1_limit_r(),
		.fake_registers = NULL,
		.wlist = mc0to1_res_mss_mcf_wlist,
		.wlist_size = ARRAY_SIZE(mc0to1_res_mss_mcf_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc2_base_r(),
		.end_pa = addr_map_mc2_limit_r(),
		.start_abs_pa = addr_map_mc2_base_r(),
		.end_abs_pa = addr_map_mc2_limit_r(),
		.fake_registers = NULL,
		.wlist = mc2to7_res_mss_mcf_wlist,
		.wlist_size = ARRAY_SIZE(mc2to7_res_mss_mcf_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc3_base_r(),
		.end_pa = addr_map_mc3_limit_r(),
		.start_abs_pa = addr_map_mc3_base_r(),
		.end_abs_pa = addr_map_mc3_limit_r(),
		.fake_registers = NULL,
		.wlist = mc2to7_res_mss_mcf_wlist,
		.wlist_size = ARRAY_SIZE(mc2to7_res_mss_mcf_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc4_base_r(),
		.end_pa = addr_map_mc4_limit_r(),
		.start_abs_pa = addr_map_mc4_base_r(),
		.end_abs_pa = addr_map_mc4_limit_r(),
		.fake_registers = NULL,
		.wlist = mc2to7_res_mss_mcf_wlist,
		.wlist_size = ARRAY_SIZE(mc2to7_res_mss_mcf_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc5_base_r(),
		.end_pa = addr_map_mc5_limit_r(),
		.start_abs_pa = addr_map_mc5_base_r(),
		.end_abs_pa = addr_map_mc5_limit_r(),
		.fake_registers = NULL,
		.wlist = mc2to7_res_mss_mcf_wlist,
		.wlist_size = ARRAY_SIZE(mc2to7_res_mss_mcf_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc6_base_r(),
		.end_pa = addr_map_mc6_limit_r(),
		.start_abs_pa = addr_map_mc6_base_r(),
		.end_abs_pa = addr_map_mc6_limit_r(),
		.fake_registers = NULL,
		.wlist = mc2to7_res_mss_mcf_wlist,
		.wlist_size = ARRAY_SIZE(mc2to7_res_mss_mcf_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc7_base_r(),
		.end_pa = addr_map_mc7_limit_r(),
		.start_abs_pa = addr_map_mc7_base_r(),
		.end_abs_pa = addr_map_mc7_limit_r(),
		.fake_registers = NULL,
		.wlist = mc2to7_res_mss_mcf_wlist,
		.wlist_size = ARRAY_SIZE(mc2to7_res_mss_mcf_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mcb_base_r(),
		.end_pa = addr_map_mcb_limit_r(),
		.start_abs_pa = addr_map_mcb_base_r(),
		.end_abs_pa = addr_map_mcb_limit_r(),
		.fake_registers = NULL,
		.wlist = mcb_res_mss_mcf_wlist,
		.wlist_size = ARRAY_SIZE(mcb_res_mss_mcf_wlist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSMCFCLIENT0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSMCFCLIENT0_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSMCFCLIENT0_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSMCFMEM0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSMCFMEM0_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSMCFMEM0_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSMCFMEM1_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSMCFMEM1_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSMCFMEM1_PERFMON_DT,
	},
};

/*
 * Normally there is a 1-to-1 mapping between an MMIO aperture and a
 * hwpm_resource_aperture struct. But the PMA MMIO aperture is used in
 * multiple hwpm_resource_aperture structs. Therefore, we have to share the fake
 * register array between these hwpm_resource_aperture structs. This is why we
 * have to define the fake register array globally. For all other 1-to-1
 * mapping apertures the fake register arrays are directly embedded inside the
 * hwpm_resource_aperture structs.
 */
u32 *pma_fake_regs = NULL;
struct whitelist pma_res_pma_wlist[] = {
	{.reg = 0x628,	.zero_in_init = true,},
};
struct hwpm_resource_aperture pma_map[] = {
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_SYS0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_SYS0_PERFMON_DT),
		.fake_registers = NULL,
		.wlist = perfmon_wlist,
		.wlist_size = ARRAY_SIZE(perfmon_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_SYS0_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = addr_map_pma_base_r(),
		.end_abs_pa = addr_map_pma_limit_r(),
		.fake_registers = NULL,
		.wlist = pma_res_pma_wlist,
		.wlist_size = ARRAY_SIZE(pma_res_pma_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_PMA_DT,
	},
};

struct whitelist pma_res_cmd_slice_rtr_wlist[] = {
	{.reg = 0x0,	.zero_in_init = false,},
	{.reg = 0x8,	.zero_in_init = false,},
	{.reg = 0xc,	.zero_in_init = false,},
	{.reg = 0x10,	.zero_in_init = false,},
	{.reg = 0x14,	.zero_in_init = false,},
	{.reg = 0x3c,	.zero_in_init = false,},
	{.reg = 0x44,	.zero_in_init = false,},
	{.reg = 0x70,	.zero_in_init = false,},
	{.reg = 0x8c,	.zero_in_init = false,},
	{.reg = 0x600,	.zero_in_init = false,},
	{.reg = 0x604,	.zero_in_init = false,},
	{.reg = 0x608,	.zero_in_init = false,},
	{.reg = 0x60c,	.zero_in_init = false,},
	{.reg = 0x610,	.zero_in_init = false,},
	{.reg = 0x618,	.zero_in_init = false,},
	{.reg = 0x61c,	.zero_in_init = false,},
	{.reg = 0x620,	.zero_in_init = false,},
	{.reg = 0x624,	.zero_in_init = false,},
	{.reg = 0x62c,	.zero_in_init = false,},
	{.reg = 0x630,	.zero_in_init = false,},
	{.reg = 0x634,	.zero_in_init = false,},
	{.reg = 0x638,	.zero_in_init = false,},
	{.reg = 0x63c,	.zero_in_init = false,},
	{.reg = 0x640,	.zero_in_init = false,},
	{.reg = 0x644,	.zero_in_init = false,},
	{.reg = 0x648,	.zero_in_init = false,},
	{.reg = 0x64c,	.zero_in_init = false,},
	{.reg = 0x650,	.zero_in_init = false,},
	{.reg = 0x654,	.zero_in_init = false,},
	{.reg = 0x658,	.zero_in_init = false,},
	{.reg = 0x65c,	.zero_in_init = false,},
	{.reg = 0x660,	.zero_in_init = false,},
	{.reg = 0x664,	.zero_in_init = false,},
	{.reg = 0x668,	.zero_in_init = false,},
	{.reg = 0x66c,	.zero_in_init = false,},
	{.reg = 0x670,	.zero_in_init = false,},
	{.reg = 0x674,	.zero_in_init = false,},
	{.reg = 0x678,	.zero_in_init = false,},
	{.reg = 0x67c,	.zero_in_init = false,},
	{.reg = 0x680,	.zero_in_init = false,},
	{.reg = 0x684,	.zero_in_init = false,},
	{.reg = 0x688,	.zero_in_init = false,},
	{.reg = 0x68c,	.zero_in_init = false,},
	{.reg = 0x690,	.zero_in_init = false,},
	{.reg = 0x694,	.zero_in_init = false,},
	{.reg = 0x698,	.zero_in_init = false,},
	{.reg = 0x69c,	.zero_in_init = false,},
	{.reg = 0x6a0,	.zero_in_init = false,},
	{.reg = 0x6a4,	.zero_in_init = false,},
	{.reg = 0x6a8,	.zero_in_init = false,},
	{.reg = 0x6ac,	.zero_in_init = false,},
	{.reg = 0x6b0,	.zero_in_init = false,},
	{.reg = 0x6b4,	.zero_in_init = false,},
	{.reg = 0x6b8,	.zero_in_init = false,},
	{.reg = 0x6bc,	.zero_in_init = false,},
	{.reg = 0x6c0,	.zero_in_init = false,},
	{.reg = 0x6c4,	.zero_in_init = false,},
	{.reg = 0x6c8,	.zero_in_init = false,},
	{.reg = 0x6cc,	.zero_in_init = false,},
	{.reg = 0x6d0,	.zero_in_init = false,},
	{.reg = 0x6d4,	.zero_in_init = false,},
	{.reg = 0x6d8,	.zero_in_init = false,},
	{.reg = 0x6dc,	.zero_in_init = false,},
	{.reg = 0x6e0,	.zero_in_init = false,},
	{.reg = 0x6e4,	.zero_in_init = false,},
	{.reg = 0x6e8,	.zero_in_init = false,},
	{.reg = 0x6ec,	.zero_in_init = false,},
	{.reg = 0x6f0,	.zero_in_init = false,},
	{.reg = 0x6f4,	.zero_in_init = false,},
	{.reg = 0x6f8,	.zero_in_init = false,},
	{.reg = 0x6fc,	.zero_in_init = false,},
	{.reg = 0x700,	.zero_in_init = false,},
	{.reg = 0x704,	.zero_in_init = false,},
	{.reg = 0x708,	.zero_in_init = false,},
	{.reg = 0x70c,	.zero_in_init = false,},
	{.reg = 0x710,	.zero_in_init = false,},
	{.reg = 0x714,	.zero_in_init = false,},
	{.reg = 0x718,	.zero_in_init = false,},
	{.reg = 0x71c,	.zero_in_init = false,},
	{.reg = 0x720,	.zero_in_init = false,},
	{.reg = 0x724,	.zero_in_init = false,},
	{.reg = 0x728,	.zero_in_init = false,},
	{.reg = 0x72c,	.zero_in_init = false,},
	{.reg = 0x730,	.zero_in_init = false,},
	{.reg = 0x734,	.zero_in_init = false,},
	{.reg = 0x75c,	.zero_in_init = false,},
};
struct whitelist rtr_wlist[] = {
	{.reg = 0x0,	.zero_in_init = false,},
	{.reg = 0x8,	.zero_in_init = false,},
	{.reg = 0xc,	.zero_in_init = false,},
	{.reg = 0x10,	.zero_in_init = false,},
	{.reg = 0x14,	.zero_in_init = false,},
	{.reg = 0x18,	.zero_in_init = false,},
	{.reg = 0x150,	.zero_in_init = false,},
	{.reg = 0x154,	.zero_in_init = false,},
};
struct hwpm_resource_aperture cmd_slice_rtr_map[] = {
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = addr_map_pma_base_r(),
		.end_abs_pa = addr_map_pma_limit_r(),
		.fake_registers = NULL,
		.wlist = pma_res_cmd_slice_rtr_wlist,
		.wlist_size = ARRAY_SIZE(pma_res_cmd_slice_rtr_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_PMA_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = addr_map_rtr_base_r(),
		.end_abs_pa = addr_map_rtr_limit_r(),
		.fake_registers = NULL,
		.wlist = rtr_wlist,
		.wlist_size = ARRAY_SIZE(rtr_wlist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_RTR_DT,
	},
};

struct hwpm_resource hwpm_resources[TERGA_SOC_HWPM_NUM_RESOURCES] = {
	[TEGRA_SOC_HWPM_RESOURCE_VI] = {
		.reserved = false,
		.map_size = ARRAY_SIZE(vi_map),
		.map = vi_map,
	},
	[TEGRA_SOC_HWPM_RESOURCE_ISP] = {
		.reserved = false,
		.map_size = ARRAY_SIZE(isp_map),
		.map = isp_map,
	},
	[TEGRA_SOC_HWPM_RESOURCE_VIC] = {
		.reserved = false,
		.map_size = ARRAY_SIZE(vic_map),
		.map = vic_map,
	},
	[TEGRA_SOC_HWPM_RESOURCE_OFA] = {
		.reserved = false,
		.map_size = ARRAY_SIZE(ofa_map),
		.map = ofa_map,
	},
	[TEGRA_SOC_HWPM_RESOURCE_PVA] = {
		.reserved = false,
		.map_size = ARRAY_SIZE(pva_map),
		.map = pva_map,
	},
	[TEGRA_SOC_HWPM_RESOURCE_NVDLA] = {
		.reserved = false,
		.map_size = ARRAY_SIZE(nvdla_map),
		.map = nvdla_map,
	},
	[TEGRA_SOC_HWPM_RESOURCE_MGBE] = {
		.reserved = false,
		.map_size = ARRAY_SIZE(mgbe_map),
		.map = mgbe_map,
	},
	[TEGRA_SOC_HWPM_RESOURCE_SCF] = {
		.reserved = false,
		.map_size = ARRAY_SIZE(scf_map),
		.map = scf_map,
	},
	[TEGRA_SOC_HWPM_RESOURCE_NVDEC] = {
		.reserved = false,
		.map_size = ARRAY_SIZE(nvdec_map),
		.map = nvdec_map,
	},
	[TEGRA_SOC_HWPM_RESOURCE_NVENC] = {
		.reserved = false,
		.map_size = ARRAY_SIZE(nvenc_map),
		.map = nvenc_map,
	},
	[TEGRA_SOC_HWPM_RESOURCE_PCIE] = {
		.reserved = false,
		.map_size = ARRAY_SIZE(pcie_map),
		.map = pcie_map,
	},
	[TEGRA_SOC_HWPM_RESOURCE_DISPLAY] = {
		.reserved = false,
		.map_size = ARRAY_SIZE(display_map),
		.map = display_map,
	},
	[TEGRA_SOC_HWPM_RESOURCE_MSS_CHANNEL] = {
		.reserved = false,
		.map_size = ARRAY_SIZE(mss_channel_map),
		.map = mss_channel_map,
	},
	[TEGRA_SOC_HWPM_RESOURCE_MSS_GPU_HUB] = {
		.reserved = false,
		.map_size = ARRAY_SIZE(mss_gpu_hub_map),
		.map = mss_gpu_hub_map,
	},
	[TEGRA_SOC_HWPM_RESOURCE_MSS_ISO_NISO_HUBS] = {
		.reserved = false,
		.map_size = ARRAY_SIZE(mss_iso_niso_hub_map),
		.map = mss_iso_niso_hub_map,
	},
	[TEGRA_SOC_HWPM_RESOURCE_MSS_MCF] = {
		.reserved = false,
		.map_size = ARRAY_SIZE(mss_mcf_map),
		.map = mss_mcf_map,
	},
	[TEGRA_SOC_HWPM_RESOURCE_PMA] = {
		.reserved = false,
		.map_size = ARRAY_SIZE(pma_map),
		.map = pma_map,
	},
	[TEGRA_SOC_HWPM_RESOURCE_CMD_SLICE_RTR] = {
		.reserved = false,
		.map_size = ARRAY_SIZE(cmd_slice_rtr_map),
		.map = cmd_slice_rtr_map,
	},
};

static bool whitelist_check(struct hwpm_resource_aperture *aperture,
			    u64 phys_addr, bool use_absolute_base,
			    u64 *updated_pa)
{
	u32 idx = 0U;
	u64 start_pa = 0ULL;

	if (!aperture) {
		tegra_soc_hwpm_err("Aperture is NULL");
		return false;
	}
	if (!aperture->wlist) {
		tegra_soc_hwpm_err("NULL whitelist in dt_aperture(%d)",
				aperture->dt_aperture);
		return false;
	}

	start_pa = use_absolute_base ? aperture->start_abs_pa :
					aperture->start_pa;

	for (idx = 0; idx < aperture->wlist_size; idx++) {
		if (phys_addr == start_pa + aperture->wlist[idx].reg) {
			*updated_pa = aperture->start_pa +
						aperture->wlist[idx].reg;
			return true;
		}
	}

	return false;
}

static bool ip_reg_check(struct hwpm_resource_aperture *aperture,
			    u64 phys_addr, bool use_absolute_base,
			    u64 *updated_pa)
{
	u64 start_pa = 0ULL;
	u64 end_pa = 0ULL;

	if (!aperture) {
		tegra_soc_hwpm_err("Aperture is NULL");
		return false;
	}

	if (use_absolute_base) {
		start_pa = aperture->start_abs_pa;
		end_pa = aperture->end_abs_pa;
	} else {
		start_pa = aperture->start_pa;
		end_pa = aperture->end_pa;
	}

	if ((phys_addr >= start_pa) && (phys_addr <= end_pa)) {
		tegra_soc_hwpm_dbg("Found aperture:"
			" phys_addr(0x%llx), aperture(0x%llx - 0x%llx)",
			phys_addr, start_pa, end_pa);
		*updated_pa = phys_addr - start_pa + aperture->start_pa;
		return true;
	}
	return false;
}

/*
 * Find an aperture in which phys_addr lies. If check_reservation is true, then
 * we also have to do a whitelist check.
 */
struct hwpm_resource_aperture *find_hwpm_aperture(struct tegra_soc_hwpm *hwpm,
						  u64 phys_addr,
						  bool use_absolute_base,
						  bool check_reservation,
						  u64 *updated_pa)
{
	struct hwpm_resource_aperture *aperture = NULL;
	int res_idx = 0;
	int aprt_idx = 0;

	for (res_idx = 0; res_idx < TERGA_SOC_HWPM_NUM_RESOURCES; res_idx++) {
		if (check_reservation && !hwpm_resources[res_idx].reserved)
			continue;

		for (aprt_idx = 0;
		     aprt_idx < hwpm_resources[res_idx].map_size;
		     aprt_idx++) {
			aperture = &(hwpm_resources[res_idx].map[aprt_idx]);
			if (check_reservation) {
				if (whitelist_check(aperture, phys_addr,
					use_absolute_base, updated_pa)) {
					return aperture;
				}
			} else {
				if (ip_reg_check(aperture, phys_addr,
					use_absolute_base, updated_pa)) {
					return aperture;
				}
			}
		}
	}

	tegra_soc_hwpm_err("Unable to find aperture: phys(0x%llx)", phys_addr);
	return NULL;
}

static u32 fake_readl(struct tegra_soc_hwpm *hwpm, u64 phys_addr)
{
	u32 reg_val = 0;
	u64 updated_pa = 0ULL;
	struct hwpm_resource_aperture *aperture = NULL;

	if (!hwpm->fake_registers_enabled) {
		tegra_soc_hwpm_err("Fake registers are disabled!");
		return 0;
	}

	aperture = find_hwpm_aperture(hwpm, phys_addr, false, false, &updated_pa);
	if (!aperture) {
		tegra_soc_hwpm_err("Invalid reg op address(0x%llx)", phys_addr);
		return 0;
	}

	reg_val = aperture->fake_registers[(updated_pa - aperture->start_pa)/4];
	return reg_val;
}

static void fake_writel(struct tegra_soc_hwpm *hwpm,
			u64 phys_addr,
			u32 val)
{
	struct hwpm_resource_aperture *aperture = NULL;
	u64 updated_pa = 0ULL;

	if (!hwpm->fake_registers_enabled) {
		tegra_soc_hwpm_err("Fake registers are disabled!");
		return;
	}

	aperture = find_hwpm_aperture(hwpm, phys_addr, false, false, &updated_pa);
	if (!aperture) {
		tegra_soc_hwpm_err("Invalid reg op address(0x%llx)", phys_addr);
		return;
	}

	aperture->fake_registers[(updated_pa - aperture->start_pa)/4] = val;
}

/* Read a HWPM (PERFMON, PMA, or RTR) register */
u32 hwpm_readl(struct tegra_soc_hwpm *hwpm,
		enum tegra_soc_hwpm_dt_aperture dt_aperture, u32 reg_offset)
{
	if ((dt_aperture < 0) ||
	    (dt_aperture >= TEGRA_SOC_HWPM_NUM_DT_APERTURES)) {
		tegra_soc_hwpm_err("Invalid dt aperture(%d)", dt_aperture);
		return 0;
	}

	tegra_soc_hwpm_dbg(
		"dt_aperture(%d): dt_aperture addr(0x%llx) reg_offset(0x%x)",
		dt_aperture, hwpm->dt_apertures[dt_aperture], reg_offset);

	if (hwpm->fake_registers_enabled) {
		u64 base_pa = 0;

		if (IS_PERFMON(dt_aperture))
			base_pa = PERFMON_BASE(dt_aperture);
		else if (dt_aperture == TEGRA_SOC_HWPM_PMA_DT)
			base_pa = addr_map_pma_base_r();
		else
			base_pa = addr_map_rtr_base_r();

		return fake_readl(hwpm, base_pa + reg_offset);
	} else {
		return readl(hwpm->dt_apertures[dt_aperture] + reg_offset);
	}
}

/* Write a HWPM (PERFMON, PMA, or RTR) register */
void hwpm_writel(struct tegra_soc_hwpm *hwpm,
		enum tegra_soc_hwpm_dt_aperture dt_aperture,
		u32 reg_offset, u32 val)
{
	if ((dt_aperture < 0) ||
		(dt_aperture >= TEGRA_SOC_HWPM_NUM_DT_APERTURES)) {
		tegra_soc_hwpm_err("Invalid dt aperture(%d)", dt_aperture);
		return;
	}

	tegra_soc_hwpm_dbg(
		"dt_aperture(%d): dt_aperture addr(0x%llx) "
		"reg_offset(0x%x), val(0x%x)",
		dt_aperture, hwpm->dt_apertures[dt_aperture], reg_offset, val);

	if (hwpm->fake_registers_enabled) {
		u64 base_pa = 0;

		if (IS_PERFMON(dt_aperture))
			base_pa = PERFMON_BASE(dt_aperture);
		else if (dt_aperture == TEGRA_SOC_HWPM_PMA_DT)
			base_pa = addr_map_pma_base_r();
		else
			base_pa = addr_map_rtr_base_r();

		fake_writel(hwpm, base_pa + reg_offset, val);
	} else {
		writel(val, hwpm->dt_apertures[dt_aperture] + reg_offset);
	}
}

/*
 * FIXME: Remove all non-HWPM register reads from the driver.
 * Replace them with inter-driver APIs?
 */
u32 ip_readl(struct tegra_soc_hwpm *hwpm, u64 phys_addr)
{
	tegra_soc_hwpm_dbg("reg read: phys_addr(0x%llx)", phys_addr);

	if (hwpm->fake_registers_enabled) {
		return fake_readl(hwpm, phys_addr);
	} else {
		void __iomem *ptr = NULL;
		u32 val = 0;

		ptr = ioremap(phys_addr, 0x4);
		if (!ptr) {
			tegra_soc_hwpm_err("Failed to map register(0x%llx)",
					   phys_addr);
			return 0;
		}
		val = __raw_readl(ptr);
		iounmap(ptr);
		return val;
	}
}

/*
 * FIXME: Remove all non-HWPM register writes from the driver.
 * Replace them with inter-driver APIs?
 */
void ip_writel(struct tegra_soc_hwpm *hwpm, u64 phys_addr, u32 val)
{
	tegra_soc_hwpm_dbg("reg write: phys_addr(0x%llx), val(0x%x)",
			   phys_addr, val);

	if (hwpm->fake_registers_enabled) {
		fake_writel(hwpm, phys_addr, val);
	} else {
		void __iomem *ptr = NULL;

		ptr = ioremap(phys_addr, 0x4);
		if (!ptr) {
			tegra_soc_hwpm_err("Failed to map register(0x%llx)",
					   phys_addr);
			return;
		}
		__raw_writel(val, ptr);
		iounmap(ptr);
	}
}

/*
 * Read a register from the EXEC_REG_OPS IOCTL. It is assumed that the whitelist
 * check has been done before calling this function.
 */
u32 ioctl_readl(struct tegra_soc_hwpm *hwpm,
		struct hwpm_resource_aperture *aperture,
		u64 addr)
{
	u32 reg_val = 0;

	if (!aperture) {
		tegra_soc_hwpm_err("aperture is NULL");
		return 0;
	}

	if (aperture->is_ip) {
		reg_val = ip_readl(hwpm, addr);
	} else {
		reg_val = hwpm_readl(hwpm, aperture->dt_aperture,
				addr - aperture->start_pa);
	}
	return reg_val;
}

/*
 * Write a register from the EXEC_REG_OPS IOCTL. It is assumed that the
 * whitelist check has been done before calling this function.
 */
void ioctl_writel(struct tegra_soc_hwpm *hwpm,
		  struct hwpm_resource_aperture *aperture,
		  u64 addr,
		  u32 val)
{
	if (!aperture) {
		tegra_soc_hwpm_err("aperture is NULL");
		return;
	}

	if (aperture->is_ip) {
		ip_writel(hwpm, addr, val);
	} else {
		hwpm_writel(hwpm, aperture->dt_aperture,
			addr - aperture->start_pa, val);
	}
}

/* Read Modify Write register operation */
int reg_rmw(struct tegra_soc_hwpm *hwpm,
	    struct hwpm_resource_aperture *aperture,
	    enum tegra_soc_hwpm_dt_aperture dt_aperture,
	    u64 addr,
	    u32 field_mask,
	    u32 field_val,
	    bool is_ioctl,
	    bool is_ip)
{
	u32 reg_val = 0;

	if (is_ioctl) {
		if (!aperture) {
			tegra_soc_hwpm_err("aperture is NULL");
			return -EIO;
		}
	}
	if (!is_ip) {
		if ((dt_aperture < 0) ||
		    (dt_aperture > TEGRA_SOC_HWPM_NUM_DT_APERTURES)) {
			tegra_soc_hwpm_err("Invalid dt_aperture(%d)",
					   dt_aperture);
			return -EIO;
		}
	}

	/* Read current register value */
	if (is_ioctl)
		reg_val = ioctl_readl(hwpm, aperture, addr);
	else if (is_ip)
		reg_val = ip_readl(hwpm, addr);
	else
		reg_val = hwpm_readl(hwpm, dt_aperture, addr);

	/* Clear and write masked bits */
	reg_val &= ~field_mask;
	reg_val |=  field_val & field_mask;

	/* Write modified value to register */
	if (is_ioctl)
		ioctl_writel(hwpm, aperture, addr, reg_val);
	else if (is_ip)
		ip_writel(hwpm, addr, reg_val);
	else
		hwpm_writel(hwpm, dt_aperture, addr, reg_val);

	return 0;
}
