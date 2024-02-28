/*
 * drivers/platform/tegra/tegra_bootloader_debug.h
 *
 * Copyright (C) 2015-2022 NVIDIA Corporation. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __TEGRA_BOOTLOADER_DEBUG_H
#define __TEGRA_BOOTLOADER_DEBUG_H
extern phys_addr_t tegra_bl_debug_data_start;
extern phys_addr_t tegra_bl_debug_data_size;
extern phys_addr_t tegra_bl_prof_start;
extern phys_addr_t tegra_bl_prof_size;
extern phys_addr_t tegra_bl_bcp_start;
extern phys_addr_t tegra_bl_bcp_size;

size_t tegra_bl_add_profiler_entry(const char *buf, size_t len);

#endif
