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

#ifndef TEGRA_HWPM_COMMON_H
#define TEGRA_HWPM_COMMON_H

struct tegra_soc_hwpm;
struct tegra_soc_hwpm_exec_reg_ops;
struct tegra_soc_hwpm_ip_floorsweep_info;
struct tegra_soc_hwpm_alloc_pma_stream;
struct tegra_soc_hwpm_update_get_put;
struct tegra_soc_hwpm_ip_ops;
struct hwpm_ip_aperture;
typedef struct hwpm_ip_aperture hwpm_ip_perfmon;
typedef struct hwpm_ip_aperture hwpm_ip_perfmux;

int tegra_hwpm_init_sw_components(struct tegra_soc_hwpm *hwpm);
void tegra_hwpm_release_sw_components(struct tegra_soc_hwpm *hwpm);

int tegra_hwpm_reserve_resource(struct tegra_soc_hwpm *hwpm, u32 resource);
int tegra_hwpm_release_resources(struct tegra_soc_hwpm *hwpm);
int tegra_hwpm_bind_resources(struct tegra_soc_hwpm *hwpm);

int tegra_hwpm_reserve_pma(struct tegra_soc_hwpm *hwpm);
int tegra_hwpm_reserve_rtr(struct tegra_soc_hwpm *hwpm);
int tegra_hwpm_release_pma(struct tegra_soc_hwpm *hwpm);
int tegra_hwpm_release_rtr(struct tegra_soc_hwpm *hwpm);

int tegra_hwpm_perfmon_reserve(struct tegra_soc_hwpm *hwpm,
	hwpm_ip_perfmon *perfmon);
int tegra_hwpm_perfmon_release(struct tegra_soc_hwpm *hwpm,
	hwpm_ip_perfmon *perfmon);
int tegra_hwpm_perfmux_reserve(struct tegra_soc_hwpm *hwpm,
	hwpm_ip_perfmux *perfmux);
int tegra_hwpm_perfmux_release(struct tegra_soc_hwpm *hwpm,
	hwpm_ip_perfmux *perfmux);

int tegra_hwpm_init_chip_ip_structures(struct tegra_soc_hwpm *hwpm);
int tegra_hwpm_set_fs_info_ip_ops(struct tegra_soc_hwpm *hwpm,
	struct tegra_soc_hwpm_ip_ops *hwpm_ip_ops,
	u64 base_address, u32 ip_idx, bool available);
int tegra_hwpm_finalize_chip_info(struct tegra_soc_hwpm *hwpm);

int tegra_hwpm_get_allowlist_size(struct tegra_soc_hwpm *hwpm);
int tegra_hwpm_update_allowlist(struct tegra_soc_hwpm *hwpm,
	void *ioctl_struct);
int tegra_hwpm_exec_regops(struct tegra_soc_hwpm *hwpm,
	struct tegra_soc_hwpm_exec_reg_ops *exec_reg_ops);

int tegra_hwpm_setup_hw(struct tegra_soc_hwpm *hwpm);
int tegra_hwpm_setup_sw(struct tegra_soc_hwpm *hwpm);
int tegra_hwpm_disable_triggers(struct tegra_soc_hwpm *hwpm);
int tegra_hwpm_release_hw(struct tegra_soc_hwpm *hwpm);
void tegra_hwpm_release_sw_setup(struct tegra_soc_hwpm *hwpm);

int tegra_hwpm_get_floorsweep_info(struct tegra_soc_hwpm *hwpm,
	struct tegra_soc_hwpm_ip_floorsweep_info *fs_info);

int tegra_hwpm_map_stream_buffer(struct tegra_soc_hwpm *hwpm,
	struct tegra_soc_hwpm_alloc_pma_stream *alloc_pma_stream);
int tegra_hwpm_clear_mem_pipeline(struct tegra_soc_hwpm *hwpm);
int tegra_hwpm_update_mem_bytes(struct tegra_soc_hwpm *hwpm,
	struct tegra_soc_hwpm_update_get_put *update_get_put);

#endif /* TEGRA_HWPM_COMMON_H */
