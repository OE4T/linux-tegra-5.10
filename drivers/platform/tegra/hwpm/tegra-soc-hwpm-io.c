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

#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>

#include "tegra-soc-hwpm-io.h"
#include "include/hw/t234/hw_addr_map_soc_hwpm.h"
#include <reg_allowlist.h>

/*
 * Aperture Ranges (start_pa/end_pa):
 *    - start_pa and end_pa is 0 for PERFMON, PMA, and RTR apertures. These
 *      ranges will be extracted from the device tree.
 *    - IP apertures are not listed in the device tree because we don't map them.
 *      Therefore, start_pa and end_pa for IP apertures are hardcoded here. IP
 *      apertures are listed here because we need to track their allowlists.
 */
struct hwpm_resource_aperture vi_map[] = {
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_VI0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_VI0_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_VI0_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_VI1_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_VI1_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_VI1_PERFMON_DT,
	},
	{
		.start_pa = addr_map_vi_thi_base_r(),
		.end_pa = addr_map_vi_thi_limit_r(),
		.start_abs_pa = addr_map_vi_thi_base_r(),
		.end_abs_pa = addr_map_vi_thi_limit_r(),
		.fake_registers = NULL,
		.alist = vi_thi_alist,
		.alist_size = ARRAY_SIZE(vi_thi_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_vi2_thi_base_r(),
		.end_pa = addr_map_vi2_thi_limit_r(),
		.start_abs_pa = addr_map_vi2_thi_base_r(),
		.end_abs_pa = addr_map_vi2_thi_limit_r(),
		.fake_registers = NULL,
		.alist = vi_thi_alist,
		.alist_size = ARRAY_SIZE(vi_thi_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
};

struct hwpm_resource_aperture isp_map[] = {
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_ISP0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_ISP0_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_ISP0_PERFMON_DT,
	},
	{
		.start_pa = addr_map_isp_thi_base_r(),
		.end_pa = addr_map_isp_thi_limit_r(),
		.start_abs_pa = addr_map_isp_thi_base_r(),
		.end_abs_pa = addr_map_isp_thi_limit_r(),
		.fake_registers = NULL,
		.alist = isp_thi_alist,
		.alist_size = ARRAY_SIZE(isp_thi_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
};

struct hwpm_resource_aperture vic_map[] = {
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_VICA0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_VICA0_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_VICA0_PERFMON_DT,
	},
	{
		.start_pa = addr_map_vic_base_r(),
		.end_pa = addr_map_vic_limit_r(),
		.start_abs_pa = addr_map_vic_base_r(),
		.end_abs_pa = addr_map_vic_limit_r(),
		.fake_registers = NULL,
		.alist = vic_alist,
		.alist_size = ARRAY_SIZE(vic_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
};

struct hwpm_resource_aperture ofa_map[] = {
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_OFAA0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_OFAA0_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_OFAA0_PERFMON_DT,
	},
	{
		.start_pa = addr_map_ofa_base_r(),
		.end_pa = addr_map_ofa_limit_r(),
		.start_abs_pa = addr_map_ofa_base_r(),
		.end_abs_pa = addr_map_ofa_limit_r(),
		.fake_registers = NULL,
		.alist = ofa_alist,
		.alist_size = ARRAY_SIZE(ofa_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
};

struct hwpm_resource_aperture pva_map[] = {
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_PVAV0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_PVAV0_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_PVAV0_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_PVAV1_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_PVAV1_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_PVAV1_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_PVAC0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_PVAC0_PERFMON_DT),
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
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
		.alist = pva0_pm_alist,
		.alist_size = ARRAY_SIZE(pva0_pm_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
};

struct hwpm_resource_aperture nvdla_map[] = {
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_NVDLAB0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_NVDLAB0_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_NVDLAB0_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_NVDLAB1_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_NVDLAB1_PERFMON_DT),
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
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
		.alist = nvdla_alist,
		.alist_size = ARRAY_SIZE(nvdla_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_nvdla1_base_r(),
		.end_pa = addr_map_nvdla1_limit_r(),
		.start_abs_pa = addr_map_nvdla1_base_r(),
		.end_abs_pa = addr_map_nvdla1_limit_r(),
		.fake_registers = NULL,
		.alist = nvdla_alist,
		.alist_size = ARRAY_SIZE(nvdla_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
};

struct hwpm_resource_aperture mgbe_map[] = {
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MGBE0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MGBE0_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MGBE0_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MGBE1_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MGBE1_PERFMON_DT),
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
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
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MGBE2_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MGBE3_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MGBE3_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MGBE3_PERFMON_DT,
	},
	{
		.start_pa = addr_map_mgbe0_base_r(),
		.end_pa = addr_map_mgbe0_limit_r(),
		.start_abs_pa = addr_map_mgbe0_base_r(),
		.end_abs_pa = addr_map_mgbe0_limit_r(),
		.fake_registers = NULL,
		.alist = mgbe_alist,
		.alist_size = ARRAY_SIZE(mgbe_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mgbe1_base_r(),
		.end_pa = addr_map_mgbe1_limit_r(),
		.start_abs_pa = addr_map_mgbe1_base_r(),
		.end_abs_pa = addr_map_mgbe1_limit_r(),
		.fake_registers = NULL,
		.alist = mgbe_alist,
		.alist_size = ARRAY_SIZE(mgbe_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mgbe2_base_r(),
		.end_pa = addr_map_mgbe2_limit_r(),
		.start_abs_pa = addr_map_mgbe2_base_r(),
		.end_abs_pa = addr_map_mgbe2_limit_r(),
		.fake_registers = NULL,
		.alist = mgbe_alist,
		.alist_size = ARRAY_SIZE(mgbe_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mgbe3_base_r(),
		.end_pa = addr_map_mgbe3_limit_r(),
		.start_abs_pa = addr_map_mgbe3_base_r(),
		.end_abs_pa = addr_map_mgbe3_limit_r(),
		.fake_registers = NULL,
		.alist = mgbe_alist,
		.alist_size = ARRAY_SIZE(mgbe_alist),
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
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_SCF0_PERFMON_DT,
	},
};

struct hwpm_resource_aperture nvdec_map[] = {
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_NVDECA0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_NVDECA0_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_NVDECA0_PERFMON_DT,
	},
	{
		.start_pa = addr_map_nvdec_base_r(),
		.end_pa = addr_map_nvdec_limit_r(),
		.start_abs_pa = addr_map_nvdec_base_r(),
		.end_abs_pa = addr_map_nvdec_limit_r(),
		.fake_registers = NULL,
		.alist = nvdec_alist,
		.alist_size = ARRAY_SIZE(nvdec_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
};

struct hwpm_resource_aperture nvenc_map[] = {
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_NVENCA0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_NVENCA0_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_NVENCA0_PERFMON_DT,
	},
	{
		.start_pa = addr_map_nvenc_base_r(),
		.end_pa = addr_map_nvenc_limit_r(),
		.start_abs_pa = addr_map_nvenc_base_r(),
		.end_abs_pa = addr_map_nvenc_limit_r(),
		.fake_registers = NULL,
		.alist = nvenc_alist,
		.alist_size = ARRAY_SIZE(nvenc_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
};

struct hwpm_resource_aperture pcie_map[] = {
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_PCIE0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_PCIE0_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_PCIE0_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_PCIE1_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_PCIE1_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_PCIE1_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_PCIE2_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_PCIE2_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_PCIE2_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_PCIE3_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_PCIE3_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_PCIE3_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_PCIE4_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_PCIE4_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_PCIE4_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_PCIE5_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_PCIE5_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_PCIE5_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_PCIE6_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_PCIE6_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_PCIE6_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_PCIE7_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_PCIE7_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_PCIE7_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_PCIE8_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_PCIE8_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_PCIE8_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_PCIE9_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_PCIE9_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_PCIE9_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_PCIE10_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_PCIE10_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_PCIE10_PERFMON_DT,
	},
	{
		.start_pa = addr_map_pcie_c0_ctl_base_r(),
		.end_pa = addr_map_pcie_c0_ctl_limit_r(),
		.start_abs_pa = addr_map_pcie_c0_ctl_base_r(),
		.end_abs_pa = addr_map_pcie_c0_ctl_limit_r(),
		.fake_registers = NULL,
		.alist = pcie_ctl_alist,
		.alist_size = ARRAY_SIZE(pcie_ctl_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_pcie_c1_ctl_base_r(),
		.end_pa = addr_map_pcie_c1_ctl_limit_r(),
		.start_abs_pa = addr_map_pcie_c1_ctl_base_r(),
		.end_abs_pa = addr_map_pcie_c1_ctl_limit_r(),
		.fake_registers = NULL,
		.alist = pcie_ctl_alist,
		.alist_size = ARRAY_SIZE(pcie_ctl_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_pcie_c2_ctl_base_r(),
		.end_pa = addr_map_pcie_c2_ctl_limit_r(),
		.start_abs_pa = addr_map_pcie_c2_ctl_base_r(),
		.end_abs_pa = addr_map_pcie_c2_ctl_limit_r(),
		.fake_registers = NULL,
		.alist = pcie_ctl_alist,
		.alist_size = ARRAY_SIZE(pcie_ctl_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_pcie_c3_ctl_base_r(),
		.end_pa = addr_map_pcie_c3_ctl_limit_r(),
		.start_abs_pa = addr_map_pcie_c3_ctl_base_r(),
		.end_abs_pa = addr_map_pcie_c3_ctl_limit_r(),
		.fake_registers = NULL,
		.alist = pcie_ctl_alist,
		.alist_size = ARRAY_SIZE(pcie_ctl_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_pcie_c4_ctl_base_r(),
		.end_pa = addr_map_pcie_c4_ctl_limit_r(),
		.start_abs_pa = addr_map_pcie_c4_ctl_base_r(),
		.end_abs_pa = addr_map_pcie_c4_ctl_limit_r(),
		.fake_registers = NULL,
		.alist = pcie_ctl_alist,
		.alist_size = ARRAY_SIZE(pcie_ctl_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_pcie_c5_ctl_base_r(),
		.end_pa = addr_map_pcie_c5_ctl_limit_r(),
		.start_abs_pa = addr_map_pcie_c5_ctl_base_r(),
		.end_abs_pa = addr_map_pcie_c5_ctl_limit_r(),
		.fake_registers = NULL,
		.alist = pcie_ctl_alist,
		.alist_size = ARRAY_SIZE(pcie_ctl_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_pcie_c6_ctl_base_r(),
		.end_pa = addr_map_pcie_c6_ctl_limit_r(),
		.start_abs_pa = addr_map_pcie_c6_ctl_base_r(),
		.end_abs_pa = addr_map_pcie_c6_ctl_limit_r(),
		.fake_registers = NULL,
		.alist = pcie_ctl_alist,
		.alist_size = ARRAY_SIZE(pcie_ctl_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_pcie_c7_ctl_base_r(),
		.end_pa = addr_map_pcie_c7_ctl_limit_r(),
		.start_abs_pa = addr_map_pcie_c7_ctl_base_r(),
		.end_abs_pa = addr_map_pcie_c7_ctl_limit_r(),
		.fake_registers = NULL,
		.alist = pcie_ctl_alist,
		.alist_size = ARRAY_SIZE(pcie_ctl_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_pcie_c8_ctl_base_r(),
		.end_pa = addr_map_pcie_c8_ctl_limit_r(),
		.start_abs_pa = addr_map_pcie_c8_ctl_base_r(),
		.end_abs_pa = addr_map_pcie_c8_ctl_limit_r(),
		.fake_registers = NULL,
		.alist = pcie_ctl_alist,
		.alist_size = ARRAY_SIZE(pcie_ctl_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_pcie_c9_ctl_base_r(),
		.end_pa = addr_map_pcie_c9_ctl_limit_r(),
		.start_abs_pa = addr_map_pcie_c9_ctl_base_r(),
		.end_abs_pa = addr_map_pcie_c9_ctl_limit_r(),
		.fake_registers = NULL,
		.alist = pcie_ctl_alist,
		.alist_size = ARRAY_SIZE(pcie_ctl_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_pcie_c10_ctl_base_r(),
		.end_pa = addr_map_pcie_c10_ctl_limit_r(),
		.start_abs_pa = addr_map_pcie_c10_ctl_base_r(),
		.end_abs_pa = addr_map_pcie_c10_ctl_limit_r(),
		.fake_registers = NULL,
		.alist = pcie_ctl_alist,
		.alist_size = ARRAY_SIZE(pcie_ctl_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
};

struct hwpm_resource_aperture display_map[] = {
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_NVDISPLAY0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_NVDISPLAY0_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_NVDISPLAY0_PERFMON_DT,
	},
	{
		.start_pa = addr_map_disp_base_r(),
		.end_pa = addr_map_disp_limit_r(),
		.start_abs_pa = addr_map_disp_base_r(),
		.end_abs_pa = addr_map_disp_limit_r(),
		.fake_registers = NULL,
		.alist = disp_alist,
		.alist_size = ARRAY_SIZE(disp_alist),
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
struct hwpm_resource_aperture mss_channel_map[] = {
	{
		.start_pa = addr_map_mc0_base_r(),
		.end_pa = addr_map_mc0_limit_r(),
		.start_abs_pa = addr_map_mc0_base_r(),
		.end_abs_pa = addr_map_mc0_limit_r(),
		.fake_registers = NULL,
		.alist = mss_channel_alist,
		.alist_size = ARRAY_SIZE(mss_channel_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc1_base_r(),
		.end_pa = addr_map_mc1_limit_r(),
		.start_abs_pa = addr_map_mc1_base_r(),
		.end_abs_pa = addr_map_mc1_limit_r(),
		.fake_registers = NULL,
		.alist = mss_channel_alist,
		.alist_size = ARRAY_SIZE(mss_channel_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc2_base_r(),
		.end_pa = addr_map_mc2_limit_r(),
		.start_abs_pa = addr_map_mc2_base_r(),
		.end_abs_pa = addr_map_mc2_limit_r(),
		.fake_registers = NULL,
		.alist = mss_channel_alist,
		.alist_size = ARRAY_SIZE(mss_channel_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc3_base_r(),
		.end_pa = addr_map_mc3_limit_r(),
		.start_abs_pa = addr_map_mc3_base_r(),
		.end_abs_pa = addr_map_mc3_limit_r(),
		.fake_registers = NULL,
		.alist = mss_channel_alist,
		.alist_size = ARRAY_SIZE(mss_channel_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc4_base_r(),
		.end_pa = addr_map_mc4_limit_r(),
		.start_abs_pa = addr_map_mc4_base_r(),
		.end_abs_pa = addr_map_mc4_limit_r(),
		.fake_registers = NULL,
		.alist = mss_channel_alist,
		.alist_size = ARRAY_SIZE(mss_channel_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc5_base_r(),
		.end_pa = addr_map_mc5_limit_r(),
		.start_abs_pa = addr_map_mc5_base_r(),
		.end_abs_pa = addr_map_mc5_limit_r(),
		.fake_registers = NULL,
		.alist = mss_channel_alist,
		.alist_size = ARRAY_SIZE(mss_channel_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc6_base_r(),
		.end_pa = addr_map_mc6_limit_r(),
		.start_abs_pa = addr_map_mc6_base_r(),
		.end_abs_pa = addr_map_mc6_limit_r(),
		.fake_registers = NULL,
		.alist = mss_channel_alist,
		.alist_size = ARRAY_SIZE(mss_channel_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc7_base_r(),
		.end_pa = addr_map_mc7_limit_r(),
		.start_abs_pa = addr_map_mc7_base_r(),
		.end_abs_pa = addr_map_mc7_limit_r(),
		.fake_registers = NULL,
		.alist = mss_channel_alist,
		.alist_size = ARRAY_SIZE(mss_channel_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc8_base_r(),
		.end_pa = addr_map_mc8_limit_r(),
		.start_abs_pa = addr_map_mc8_base_r(),
		.end_abs_pa = addr_map_mc8_limit_r(),
		.fake_registers = NULL,
		.alist = mss_channel_alist,
		.alist_size = ARRAY_SIZE(mss_channel_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc9_base_r(),
		.end_pa = addr_map_mc9_limit_r(),
		.start_abs_pa = addr_map_mc9_base_r(),
		.end_abs_pa = addr_map_mc9_limit_r(),
		.fake_registers = NULL,
		.alist = mss_channel_alist,
		.alist_size = ARRAY_SIZE(mss_channel_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc10_base_r(),
		.end_pa = addr_map_mc10_limit_r(),
		.start_abs_pa = addr_map_mc10_base_r(),
		.end_abs_pa = addr_map_mc10_limit_r(),
		.fake_registers = NULL,
		.alist = mss_channel_alist,
		.alist_size = ARRAY_SIZE(mss_channel_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc11_base_r(),
		.end_pa = addr_map_mc11_limit_r(),
		.start_abs_pa = addr_map_mc11_base_r(),
		.end_abs_pa = addr_map_mc11_limit_r(),
		.fake_registers = NULL,
		.alist = mss_channel_alist,
		.alist_size = ARRAY_SIZE(mss_channel_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc12_base_r(),
		.end_pa = addr_map_mc12_limit_r(),
		.start_abs_pa = addr_map_mc12_base_r(),
		.end_abs_pa = addr_map_mc12_limit_r(),
		.fake_registers = NULL,
		.alist = mss_channel_alist,
		.alist_size = ARRAY_SIZE(mss_channel_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc13_base_r(),
		.end_pa = addr_map_mc13_limit_r(),
		.start_abs_pa = addr_map_mc13_base_r(),
		.end_abs_pa = addr_map_mc13_limit_r(),
		.fake_registers = NULL,
		.alist = mss_channel_alist,
		.alist_size = ARRAY_SIZE(mss_channel_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc14_base_r(),
		.end_pa = addr_map_mc14_limit_r(),
		.start_abs_pa = addr_map_mc14_base_r(),
		.end_abs_pa = addr_map_mc14_limit_r(),
		.fake_registers = NULL,
		.alist = mss_channel_alist,
		.alist_size = ARRAY_SIZE(mss_channel_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc15_base_r(),
		.end_pa = addr_map_mc15_limit_r(),
		.start_abs_pa = addr_map_mc15_base_r(),
		.end_abs_pa = addr_map_mc15_limit_r(),
		.fake_registers = NULL,
		.alist = mss_channel_alist,
		.alist_size = ARRAY_SIZE(mss_channel_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSCHANNELPARTA0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSCHANNELPARTA0_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSCHANNELPARTA0_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSCHANNELPARTA1_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSCHANNELPARTA1_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSCHANNELPARTA1_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSCHANNELPARTA2_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSCHANNELPARTA2_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSCHANNELPARTA2_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSCHANNELPARTA3_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSCHANNELPARTA3_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSCHANNELPARTA3_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSCHANNELPARTB0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSCHANNELPARTB0_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSCHANNELPARTB0_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSCHANNELPARTB1_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSCHANNELPARTB1_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSCHANNELPARTB1_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSCHANNELPARTB2_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSCHANNELPARTB2_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSCHANNELPARTB2_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSCHANNELPARTB3_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSCHANNELPARTB3_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSCHANNELPARTB3_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSCHANNELPARTC0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSCHANNELPARTC0_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSCHANNELPARTC0_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSCHANNELPARTC1_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSCHANNELPARTC1_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSCHANNELPARTC1_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSCHANNELPARTC2_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSCHANNELPARTC2_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSCHANNELPARTC2_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSCHANNELPARTC3_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSCHANNELPARTC3_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSCHANNELPARTC3_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSCHANNELPARTD0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSCHANNELPARTD0_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSCHANNELPARTD0_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSCHANNELPARTD1_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSCHANNELPARTD1_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSCHANNELPARTD1_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSCHANNELPARTD2_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSCHANNELPARTD2_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSCHANNELPARTD2_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSCHANNELPARTD3_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSCHANNELPARTD3_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSCHANNELPARTD3_PERFMON_DT,
	},
};

struct hwpm_resource_aperture mss_gpu_hub_map[] = {
	{
		.start_pa = addr_map_mss_nvlink_1_base_r(),
		.end_pa = addr_map_mss_nvlink_1_limit_r(),
		.start_abs_pa = addr_map_mss_nvlink_1_base_r(),
		.end_abs_pa = addr_map_mss_nvlink_1_limit_r(),
		.fake_registers = NULL,
		.alist = mss_nvlink_alist,
		.alist_size = ARRAY_SIZE(mss_nvlink_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mss_nvlink_2_base_r(),
		.end_pa = addr_map_mss_nvlink_2_limit_r(),
		.start_abs_pa = addr_map_mss_nvlink_2_base_r(),
		.end_abs_pa = addr_map_mss_nvlink_2_limit_r(),
		.fake_registers = NULL,
		.alist = mss_nvlink_alist,
		.alist_size = ARRAY_SIZE(mss_nvlink_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mss_nvlink_3_base_r(),
		.end_pa = addr_map_mss_nvlink_3_limit_r(),
		.start_abs_pa = addr_map_mss_nvlink_3_base_r(),
		.end_abs_pa = addr_map_mss_nvlink_3_limit_r(),
		.fake_registers = NULL,
		.alist = mss_nvlink_alist,
		.alist_size = ARRAY_SIZE(mss_nvlink_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mss_nvlink_4_base_r(),
		.end_pa = addr_map_mss_nvlink_4_limit_r(),
		.start_abs_pa = addr_map_mss_nvlink_4_base_r(),
		.end_abs_pa = addr_map_mss_nvlink_4_limit_r(),
		.fake_registers = NULL,
		.alist = mss_nvlink_alist,
		.alist_size = ARRAY_SIZE(mss_nvlink_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mss_nvlink_5_base_r(),
		.end_pa = addr_map_mss_nvlink_5_limit_r(),
		.start_abs_pa = addr_map_mss_nvlink_5_base_r(),
		.end_abs_pa = addr_map_mss_nvlink_5_limit_r(),
		.fake_registers = NULL,
		.alist = mss_nvlink_alist,
		.alist_size = ARRAY_SIZE(mss_nvlink_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mss_nvlink_6_base_r(),
		.end_pa = addr_map_mss_nvlink_6_limit_r(),
		.start_abs_pa = addr_map_mss_nvlink_6_base_r(),
		.end_abs_pa = addr_map_mss_nvlink_6_limit_r(),
		.fake_registers = NULL,
		.alist = mss_nvlink_alist,
		.alist_size = ARRAY_SIZE(mss_nvlink_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mss_nvlink_7_base_r(),
		.end_pa = addr_map_mss_nvlink_7_limit_r(),
		.start_abs_pa = addr_map_mss_nvlink_7_base_r(),
		.end_abs_pa = addr_map_mss_nvlink_7_limit_r(),
		.fake_registers = NULL,
		.alist = mss_nvlink_alist,
		.alist_size = ARRAY_SIZE(mss_nvlink_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mss_nvlink_8_base_r(),
		.end_pa = addr_map_mss_nvlink_8_limit_r(),
		.start_abs_pa = addr_map_mss_nvlink_8_base_r(),
		.end_abs_pa = addr_map_mss_nvlink_8_limit_r(),
		.fake_registers = NULL,
		.alist = mss_nvlink_alist,
		.alist_size = ARRAY_SIZE(mss_nvlink_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSNVLHSH0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSNVLHSH0_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSNVLHSH0_PERFMON_DT,
	},
};

struct hwpm_resource_aperture mss_iso_niso_hub_map[] = {
	{
		.start_pa = addr_map_mc0_base_r(),
		.end_pa = addr_map_mc0_limit_r(),
		.start_abs_pa = addr_map_mc0_base_r(),
		.end_abs_pa = addr_map_mc0_limit_r(),
		.fake_registers = NULL,
		.alist = mc0to7_res_mss_iso_niso_hub_alist,
		.alist_size = ARRAY_SIZE(mc0to7_res_mss_iso_niso_hub_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc1_base_r(),
		.end_pa = addr_map_mc1_limit_r(),
		.start_abs_pa = addr_map_mc1_base_r(),
		.end_abs_pa = addr_map_mc1_limit_r(),
		.fake_registers = NULL,
		.alist = mc0to7_res_mss_iso_niso_hub_alist,
		.alist_size = ARRAY_SIZE(mc0to7_res_mss_iso_niso_hub_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc2_base_r(),
		.end_pa = addr_map_mc2_limit_r(),
		.start_abs_pa = addr_map_mc2_base_r(),
		.end_abs_pa = addr_map_mc2_limit_r(),
		.fake_registers = NULL,
		.alist = mc0to7_res_mss_iso_niso_hub_alist,
		.alist_size = ARRAY_SIZE(mc0to7_res_mss_iso_niso_hub_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc3_base_r(),
		.end_pa = addr_map_mc3_limit_r(),
		.start_abs_pa = addr_map_mc3_base_r(),
		.end_abs_pa = addr_map_mc3_limit_r(),
		.fake_registers = NULL,
		.alist = mc0to7_res_mss_iso_niso_hub_alist,
		.alist_size = ARRAY_SIZE(mc0to7_res_mss_iso_niso_hub_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc4_base_r(),
		.end_pa = addr_map_mc4_limit_r(),
		.start_abs_pa = addr_map_mc4_base_r(),
		.end_abs_pa = addr_map_mc4_limit_r(),
		.fake_registers = NULL,
		.alist = mc0to7_res_mss_iso_niso_hub_alist,
		.alist_size = ARRAY_SIZE(mc0to7_res_mss_iso_niso_hub_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc5_base_r(),
		.end_pa = addr_map_mc5_limit_r(),
		.start_abs_pa = addr_map_mc5_base_r(),
		.end_abs_pa = addr_map_mc5_limit_r(),
		.fake_registers = NULL,
		.alist = mc0to7_res_mss_iso_niso_hub_alist,
		.alist_size = ARRAY_SIZE(mc0to7_res_mss_iso_niso_hub_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc6_base_r(),
		.end_pa = addr_map_mc6_limit_r(),
		.start_abs_pa = addr_map_mc6_base_r(),
		.end_abs_pa = addr_map_mc6_limit_r(),
		.fake_registers = NULL,
		.alist = mc0to7_res_mss_iso_niso_hub_alist,
		.alist_size = ARRAY_SIZE(mc0to7_res_mss_iso_niso_hub_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc7_base_r(),
		.end_pa = addr_map_mc7_limit_r(),
		.start_abs_pa = addr_map_mc7_base_r(),
		.end_abs_pa = addr_map_mc7_limit_r(),
		.fake_registers = NULL,
		.alist = mc0to7_res_mss_iso_niso_hub_alist,
		.alist_size = ARRAY_SIZE(mc0to7_res_mss_iso_niso_hub_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc8_base_r(),
		.end_pa = addr_map_mc8_limit_r(),
		.start_abs_pa = addr_map_mc8_base_r(),
		.end_abs_pa = addr_map_mc8_limit_r(),
		.fake_registers = NULL,
		.alist = mc8_res_mss_iso_niso_hub_alist,
		.alist_size = ARRAY_SIZE(mc8_res_mss_iso_niso_hub_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSHUB0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSHUB0_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSHUB0_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSHUB1_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSHUB1_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSHUB1_PERFMON_DT,
	},
};

struct hwpm_resource_aperture mss_mcf_map[] = {
	{
		.start_pa = addr_map_mc0_base_r(),
		.end_pa = addr_map_mc0_limit_r(),
		.start_abs_pa = addr_map_mc0_base_r(),
		.end_abs_pa = addr_map_mc0_limit_r(),
		.fake_registers = NULL,
		.alist = mc0to1_mss_mcf_alist,
		.alist_size = ARRAY_SIZE(mc0to1_mss_mcf_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc1_base_r(),
		.end_pa = addr_map_mc1_limit_r(),
		.start_abs_pa = addr_map_mc1_base_r(),
		.end_abs_pa = addr_map_mc1_limit_r(),
		.fake_registers = NULL,
		.alist = mc0to1_mss_mcf_alist,
		.alist_size = ARRAY_SIZE(mc0to1_mss_mcf_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc2_base_r(),
		.end_pa = addr_map_mc2_limit_r(),
		.start_abs_pa = addr_map_mc2_base_r(),
		.end_abs_pa = addr_map_mc2_limit_r(),
		.fake_registers = NULL,
		.alist = mc2to7_mss_mcf_alist,
		.alist_size = ARRAY_SIZE(mc2to7_mss_mcf_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc3_base_r(),
		.end_pa = addr_map_mc3_limit_r(),
		.start_abs_pa = addr_map_mc3_base_r(),
		.end_abs_pa = addr_map_mc3_limit_r(),
		.fake_registers = NULL,
		.alist = mc2to7_mss_mcf_alist,
		.alist_size = ARRAY_SIZE(mc2to7_mss_mcf_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc4_base_r(),
		.end_pa = addr_map_mc4_limit_r(),
		.start_abs_pa = addr_map_mc4_base_r(),
		.end_abs_pa = addr_map_mc4_limit_r(),
		.fake_registers = NULL,
		.alist = mc2to7_mss_mcf_alist,
		.alist_size = ARRAY_SIZE(mc2to7_mss_mcf_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc5_base_r(),
		.end_pa = addr_map_mc5_limit_r(),
		.start_abs_pa = addr_map_mc5_base_r(),
		.end_abs_pa = addr_map_mc5_limit_r(),
		.fake_registers = NULL,
		.alist = mc2to7_mss_mcf_alist,
		.alist_size = ARRAY_SIZE(mc2to7_mss_mcf_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc6_base_r(),
		.end_pa = addr_map_mc6_limit_r(),
		.start_abs_pa = addr_map_mc6_base_r(),
		.end_abs_pa = addr_map_mc6_limit_r(),
		.fake_registers = NULL,
		.alist = mc2to7_mss_mcf_alist,
		.alist_size = ARRAY_SIZE(mc2to7_mss_mcf_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mc7_base_r(),
		.end_pa = addr_map_mc7_limit_r(),
		.start_abs_pa = addr_map_mc7_base_r(),
		.end_abs_pa = addr_map_mc7_limit_r(),
		.fake_registers = NULL,
		.alist = mc2to7_mss_mcf_alist,
		.alist_size = ARRAY_SIZE(mc2to7_mss_mcf_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = addr_map_mcb_base_r(),
		.end_pa = addr_map_mcb_limit_r(),
		.start_abs_pa = addr_map_mcb_base_r(),
		.end_abs_pa = addr_map_mcb_limit_r(),
		.fake_registers = NULL,
		.alist = mcb_mss_mcf_alist,
		.alist_size = ARRAY_SIZE(mcb_mss_mcf_alist),
		.is_ip = true,
		.dt_aperture = TEGRA_SOC_HWPM_INVALID_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSMCFCLIENT0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSMCFCLIENT0_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSMCFCLIENT0_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSMCFMEM0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSMCFMEM0_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_MSSMCFMEM0_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_MSSMCFMEM1_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_MSSMCFMEM1_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
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
struct hwpm_resource_aperture pma_map[] = {
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = PERFMON_BASE(TEGRA_SOC_HWPM_SYS0_PERFMON_DT),
		.end_abs_pa = PERFMON_LIMIT(TEGRA_SOC_HWPM_SYS0_PERFMON_DT),
		.fake_registers = NULL,
		.alist = perfmon_alist,
		.alist_size = ARRAY_SIZE(perfmon_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_SYS0_PERFMON_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = addr_map_pma_base_r(),
		.end_abs_pa = addr_map_pma_limit_r(),
		.fake_registers = NULL,
		.alist = pma_res_pma_alist,
		.alist_size = ARRAY_SIZE(pma_res_pma_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_PMA_DT,
	},
};

struct hwpm_resource_aperture cmd_slice_rtr_map[] = {
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = addr_map_pma_base_r(),
		.end_abs_pa = addr_map_pma_limit_r(),
		.fake_registers = NULL,
		.alist = pma_res_cmd_slice_rtr_alist,
		.alist_size = ARRAY_SIZE(pma_res_cmd_slice_rtr_alist),
		.is_ip = false,
		.dt_aperture = TEGRA_SOC_HWPM_PMA_DT,
	},
	{
		.start_pa = 0,
		.end_pa = 0,
		.start_abs_pa = addr_map_rtr_base_r(),
		.end_abs_pa = addr_map_rtr_limit_r(),
		.fake_registers = NULL,
		.alist = rtr_alist,
		.alist_size = ARRAY_SIZE(rtr_alist),
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

void tegra_soc_hwpm_zero_alist_regs(struct tegra_soc_hwpm *hwpm,
		struct hwpm_resource_aperture *aperture)
{
	u32 alist_idx = 0U;

	for (alist_idx = 0; alist_idx < aperture->alist_size; alist_idx++) {
		if (aperture->alist[alist_idx].zero_at_init) {
			ioctl_writel(hwpm, aperture,
				aperture->start_pa +
				aperture->alist[alist_idx].reg_offset, 0);
		}
	}
}

int tegra_soc_hwpm_update_allowlist(struct tegra_soc_hwpm *hwpm,
				 void *ioctl_struct)
{
	int err = 0;
	int res_idx = 0;
	int aprt_idx = 0;
	u32 full_alist_idx = 0;
	u32 aprt_alist_idx = 0;
	long pinned_pages = 0;
	long page_idx = 0;
	u64 alist_buf_size = 0;
	u64 num_pages = 0;
	u64 *full_alist_u64 = NULL;
	void *full_alist = NULL;
	struct page **pages = NULL;
	struct hwpm_resource_aperture *aperture = NULL;
	struct tegra_soc_hwpm_query_allowlist *query_allowlist =
			(struct tegra_soc_hwpm_query_allowlist *)ioctl_struct;
	unsigned long user_va = (unsigned long)(query_allowlist->allowlist);
	unsigned long offset = user_va & ~PAGE_MASK;

	if (hwpm->full_alist_size < 0) {
		tegra_soc_hwpm_err("Invalid allowlist size");
		return -EINVAL;
	}
	alist_buf_size = hwpm->full_alist_size * sizeof(struct allowlist);

	/* Memory map user buffer into kernel address space */
	num_pages = DIV_ROUND_UP(offset + alist_buf_size, PAGE_SIZE);
	pages = (struct page **)kzalloc(sizeof(*pages) * num_pages, GFP_KERNEL);
	if (!pages) {
		tegra_soc_hwpm_err("Couldn't allocate memory for pages array");
		err = -ENOMEM;
		goto alist_unmap;
	}
	pinned_pages = get_user_pages(user_va & PAGE_MASK, num_pages, 0,
				pages, NULL);
	if (pinned_pages != num_pages) {
		tegra_soc_hwpm_err("Requested %llu pages / Got %ld pages",
				num_pages, pinned_pages);
		err = -ENOMEM;
		goto alist_unmap;
	}
	full_alist = vmap(pages, num_pages, VM_MAP, PAGE_KERNEL);
	if (!full_alist) {
		tegra_soc_hwpm_err("Couldn't map allowlist buffer into"
				   " kernel address space");
		err = -ENOMEM;
		goto alist_unmap;
	}
	full_alist_u64 = (u64 *)(full_alist + offset);

	/* Fill in allowlist buffer */
	for (res_idx = 0, full_alist_idx = 0;
	     res_idx < TERGA_SOC_HWPM_NUM_RESOURCES;
	     res_idx++) {
		if (!(hwpm_resources[res_idx].reserved))
			continue;
		tegra_soc_hwpm_dbg("Found reserved IP(%d)", res_idx);

		for (aprt_idx = 0;
		     aprt_idx < hwpm_resources[res_idx].map_size;
		     aprt_idx++) {
			aperture = &(hwpm_resources[res_idx].map[aprt_idx]);
			if (aperture->alist) {
				for (aprt_alist_idx = 0;
				     aprt_alist_idx < aperture->alist_size;
				     aprt_alist_idx++, full_alist_idx++) {
					full_alist_u64[full_alist_idx] =
						aperture->start_pa +
						aperture->alist[aprt_alist_idx].reg_offset;
				}
			} else {
				tegra_soc_hwpm_err("NULL allowlist in aperture(0x%llx - 0x%llx)",
						   aperture->start_pa,
						   aperture->end_pa);
			}
		}
	}

alist_unmap:
	if (full_alist)
		vunmap(full_alist);
	if (pinned_pages > 0) {
		for (page_idx = 0; page_idx < pinned_pages; page_idx++) {
			set_page_dirty(pages[page_idx]);
			put_page(pages[page_idx]);
		}
	}
	if (pages) {
		kfree(pages);
	}

	return err;
}

static bool allowlist_check(struct hwpm_resource_aperture *aperture,
			    u64 phys_addr, bool use_absolute_base,
			    u64 *updated_pa)
{
	u32 idx = 0U;
	u64 start_pa = 0ULL;

	if (!aperture) {
		tegra_soc_hwpm_err("Aperture is NULL");
		return false;
	}
	if (!aperture->alist) {
		tegra_soc_hwpm_err("NULL allowlist in dt_aperture(%d)",
				aperture->dt_aperture);
		return false;
	}

	start_pa = use_absolute_base ? aperture->start_abs_pa :
					aperture->start_pa;

	for (idx = 0; idx < aperture->alist_size; idx++) {
		if (phys_addr == start_pa + aperture->alist[idx].reg_offset) {
			*updated_pa = aperture->start_pa +
						aperture->alist[idx].reg_offset;
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
 * we also have to do a allowlist check.
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
				if (allowlist_check(aperture, phys_addr,
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
 * Read a register from the EXEC_REG_OPS IOCTL. It is assumed that the allowlist
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
 * allowlist check has been done before calling this function.
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
