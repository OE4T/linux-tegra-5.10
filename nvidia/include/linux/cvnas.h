/*
 * include/linux/cvnas.h
 *
 * Tegra cvnas driver
 *
 * Copyright (c) 2018-2022, NVIDIA Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#ifndef __LINUX_CVNAS_H
#define __LINUX_CVNAS_H

#include <linux/types.h>

int nvcvnas_busy(void);
int nvcvnas_busy_no_rpm(void);
int nvcvnas_idle(void);
int nvcvnas_idle_no_rpm(void);
int is_nvcvnas_probed(void);
phys_addr_t nvcvnas_get_cvsram_base(void);
size_t nvcvnas_get_cvsram_size(void);
int is_nvcvnas_clk_enabled(void);

#endif
