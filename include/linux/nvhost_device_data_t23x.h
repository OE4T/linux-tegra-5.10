/*
 * include/linux/nvhost_device_data_t23x.h
 *
 * Tegra graphics host driver
 *
 * Copyright (c) 2021, NVIDIA Corporation. All rights reserved.
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */

//struct nvhost_device_data {
	bool enable_riscv_boot;		/* enable risc-v boot */
	void *riscv_data;		/* store the risc-v info */
	char *riscv_desc_bin;		/* name of riscv descriptor binary */
	char *riscv_image_bin;		/* name of riscv image binary */
//};

