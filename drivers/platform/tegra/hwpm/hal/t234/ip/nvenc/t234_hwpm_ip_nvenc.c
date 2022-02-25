/*
 * Copyright (c) 2021-2022, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#include "t234_hwpm_ip_nvenc.h"

#include <tegra_hwpm.h>
#include <hal/t234/t234_hwpm_regops_allowlist.h>
#include <hal/t234/hw/t234_addr_map_soc_hwpm.h>

hwpm_ip_perfmon t234_nvenc_perfmon_static_array[
	T234_HWPM_IP_NVENC_NUM_PERFMON_PER_INST *
	T234_HWPM_IP_NVENC_NUM_INSTANCES] = {
	{
		.is_hwpm_element = true,
		.hw_inst_mask = BIT(0),
		.dt_mmio = NULL,
		.name = "perfmon_nvenca0",
		.ip_ops = {
			.ip_dev = NULL,
			.hwpm_ip_pm = NULL,
			.hwpm_ip_reg_op = NULL,
		},
		.start_abs_pa = addr_map_rpg_pm_nvenc0_base_r(),
		.end_abs_pa = addr_map_rpg_pm_nvenc0_limit_r(),
		.start_pa = 0ULL,
		.end_pa = 0ULL,
		.base_pa = addr_map_rpg_pm_base_r(),
		.alist = t234_perfmon_alist,
		.alist_size = ARRAY_SIZE(t234_perfmon_alist),
		.fake_registers = NULL,
	},
};

hwpm_ip_perfmux t234_nvenc_perfmux_static_array[
	T234_HWPM_IP_NVENC_NUM_PERFMUX_PER_INST *
	T234_HWPM_IP_NVENC_NUM_INSTANCES] = {
	{
		.is_hwpm_element = false,
		.hw_inst_mask = BIT(0),
		.dt_mmio = NULL,
		.name = {'\0'},
		.ip_ops = {
			.ip_dev = NULL,
			.hwpm_ip_pm = NULL,
			.hwpm_ip_reg_op = NULL,
		},
		.start_abs_pa = addr_map_nvenc_base_r(),
		.end_abs_pa = addr_map_nvenc_limit_r(),
		.start_pa = 0ULL,
		.end_pa = 0ULL,
		.base_pa = 0ULL,
		.alist = t234_nvenc_alist,
		.alist_size = ARRAY_SIZE(t234_nvenc_alist),
		.fake_registers = NULL,
	},
};

struct hwpm_ip t234_hwpm_ip_nvenc = {
	.num_instances = T234_HWPM_IP_NVENC_NUM_INSTANCES,
	.num_perfmon_per_inst = T234_HWPM_IP_NVENC_NUM_PERFMON_PER_INST,
	.num_perfmux_per_inst = T234_HWPM_IP_NVENC_NUM_PERFMUX_PER_INST,

	.perfmon_range_start = addr_map_rpg_pm_nvenc0_base_r(),
	.perfmon_range_end = addr_map_rpg_pm_nvenc0_limit_r(),
	.inst_perfmon_stride = addr_map_rpg_pm_nvenc0_limit_r() -
			addr_map_rpg_pm_nvenc0_base_r() + 1ULL,
	.num_perfmon_slots = 0U,
	.ip_perfmon = NULL,

	.perfmux_range_start = addr_map_nvenc_base_r(),
	.perfmux_range_end = addr_map_nvenc_limit_r(),
	.inst_perfmux_stride = addr_map_nvenc_limit_r() -
			addr_map_nvenc_base_r() + 1ULL,
	.num_perfmux_slots = 0U,
	.ip_perfmux = NULL,

	.override_enable = false,
	.fs_mask = 0U,
	.perfmon_static_array = t234_nvenc_perfmon_static_array,
	.perfmux_static_array = t234_nvenc_perfmux_static_array,
	.reserved = false,
};
