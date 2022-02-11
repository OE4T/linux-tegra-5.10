/*
 * Copyright (c) 2021-2022, NVIDIA Corporation.  All rights reserved.
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

#include <nvgpu/cic_mon.h>
#include <nvgpu/types.h>

struct gk20a;

int nvgpu_cic_mon_report_err_safety_services(struct gk20a *g,
		u32 err_id)
{
	/**
	 * ToDo: Add MISC_EC API to report error.
	 */
	return 0;
}
