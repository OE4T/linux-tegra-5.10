/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION. All rights reserved.
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

#include "os_linux.h"

#include "cde_gm20b.h"
#include "debug_clk_gm20b.h"
#include "debug_fecs_trace.h"

static struct nvgpu_os_linux_ops gm20b_os_linux_ops = {
#ifdef CONFIG_NVGPU_SUPPORT_CDE
	.cde = {
		.get_program_numbers = gm20b_cde_get_program_numbers,
	},
#endif
	.clk = {
		.init_debugfs = gm20b_clk_init_debugfs,
	},

	.fecs_trace = {
		.init_debugfs = nvgpu_fecs_trace_init_debugfs,
	},
};

void nvgpu_gm20b_init_os_ops(struct nvgpu_os_linux *l)
{
#ifdef CONFIG_NVGPU_SUPPORT_CDE
	l->ops.cde = gm20b_os_linux_ops.cde;
#endif
	l->ops.clk = gm20b_os_linux_ops.clk;

	l->ops.fecs_trace = gm20b_os_linux_ops.fecs_trace;
}
