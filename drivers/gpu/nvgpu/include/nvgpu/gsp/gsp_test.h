/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * GSP Tests
 *
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
#ifndef NVGPU_GSP_TEST
#define NVGPU_GSP_TEST

#include <nvgpu/types.h>
struct gk20a;

u32 nvgpu_gsp_get_current_iteration(struct gk20a *g);
u32 nvgpu_gsp_get_current_test(struct gk20a *g);
bool nvgpu_gsp_get_test_fail_status(struct gk20a *g);
void nvgpu_gsp_set_test_fail_status(struct gk20a *g, bool val);
bool nvgpu_gsp_get_stress_test_start(struct gk20a *g);
int nvgpu_gsp_set_stress_test_start(struct gk20a *g, bool flag);
bool nvgpu_gsp_get_stress_test_load(struct gk20a *g);
int nvgpu_gsp_set_stress_test_load(struct gk20a *g, bool flag);
int nvgpu_gsp_stress_test_bootstrap(struct gk20a *g, bool start);
int nvgpu_gsp_stress_test_halt(struct gk20a *g, bool restart);
bool nvgpu_gsp_is_stress_test(struct gk20a *g);
int nvgpu_gsp_stress_test_sw_init(struct gk20a *g);
void nvgpu_gsp_test_sw_deinit(struct gk20a *g);
void nvgpu_gsp_write_test_sysmem_addr(struct gk20a *g);
void nvgpu_gsp_stest_isr(struct gk20a *g);
#endif /* NVGPU_GSP_TEST */
