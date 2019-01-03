/*
 * Copyright (c) 2018-2019, NVIDIA Corporation.  All rights reserved.
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

#include "os/linux/os_linux.h"

#include "os/linux/debug_therm_gp106.h"
#include "os/linux/debug_clk_gv100.h"
#include "os/linux/debug_volt.h"

static struct nvgpu_os_linux_ops tu104_os_linux_ops = {
	.therm = {
		.init_debugfs = gp106_therm_init_debugfs,
	},
	.clk = {
		.init_debugfs = gv100_clk_init_debugfs,
	},
	.volt = {
		.init_debugfs = nvgpu_volt_init_debugfs,
	},
};

void nvgpu_tu104_init_os_ops(struct nvgpu_os_linux *l)
{
	l->ops.therm = tu104_os_linux_ops.therm;
	l->ops.clk = tu104_os_linux_ops.clk;
	l->ops.volt = tu104_os_linux_ops.volt;
}
