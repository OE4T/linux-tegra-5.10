/*
 * Copyright (c) 2020, NVIDIA CORPORATION.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef NVGPU_NEXT_NETLIST_H
#define NVGPU_NEXT_NETLIST_H

/**
 * @file
 *
 */
#include <nvgpu/types.h>

struct gk20a;
struct nvgpu_netlist_vars;
struct netlist_av_list;

bool nvgpu_next_netlist_handle_sw_bundles_region_id(struct gk20a *g,
			u32 region_id, u8 *src, u32 size,
			struct nvgpu_netlist_vars *netlist_vars, int *err_code);
void nvgpu_next_netlist_deinit_ctx_vars(struct gk20a *g);

struct netlist_av_list *nvgpu_next_netlist_get_sw_non_ctx_local_compute_load_av_list(
							struct gk20a *g);
struct netlist_av_list *nvgpu_next_netlist_get_sw_non_ctx_global_compute_load_av_list(
							struct gk20a *g);
#ifdef CONFIG_NVGPU_GRAPHICS
struct netlist_av_list *nvgpu_next_netlist_get_sw_non_ctx_local_gfx_load_av_list(
							struct gk20a *g);
struct netlist_av_list *nvgpu_next_netlist_get_sw_non_ctx_global_gfx_load_av_list(
							struct gk20a *g);
#endif /* CONFIG_NVGPU_GRAPHICS */

#ifdef CONFIG_NVGPU_DEBUGGER
bool nvgpu_next_netlist_handle_debugger_region_id(struct gk20a *g,
			u32 region_id, u8 *src, u32 size,
			struct nvgpu_netlist_vars *netlist_vars, int *err_code);
void nvgpu_next_netlist_deinit_ctxsw_regs(struct gk20a *g);

struct netlist_aiv_list *nvgpu_next_netlist_get_sys_compute_ctxsw_regs(
							struct gk20a *g);
struct netlist_aiv_list *nvgpu_next_netlist_get_gpc_compute_ctxsw_regs(
							struct gk20a *g);
struct netlist_aiv_list *nvgpu_next_netlist_get_tpc_compute_ctxsw_regs(
							struct gk20a *g);
struct netlist_aiv_list *nvgpu_next_netlist_get_ppc_compute_ctxsw_regs(
							struct gk20a *g);
struct netlist_aiv_list *nvgpu_next_netlist_get_etpc_compute_ctxsw_regs(
							struct gk20a *g);
struct netlist_aiv_list *nvgpu_next_netlist_get_lts_ctxsw_regs(
							struct gk20a *g);
#ifdef CONFIG_NVGPU_GRAPHICS
struct netlist_aiv_list *nvgpu_next_netlist_get_sys_gfx_ctxsw_regs(
							struct gk20a *g);
struct netlist_aiv_list *nvgpu_next_netlist_get_gpc_gfx_ctxsw_regs(
							struct gk20a *g);
struct netlist_aiv_list *nvgpu_next_netlist_get_tpc_gfx_ctxsw_regs(
							struct gk20a *g);
struct netlist_aiv_list *nvgpu_next_netlist_get_ppc_gfx_ctxsw_regs(
							struct gk20a *g);
struct netlist_aiv_list *nvgpu_next_netlist_get_etpc_gfx_ctxsw_regs(
							struct gk20a *g);
#endif /* CONFIG_NVGPU_GRAPHICS */
u32 nvgpu_next_netlist_get_sys_ctxsw_regs_count(struct gk20a *g);
u32 nvgpu_next_netlist_get_ppc_ctxsw_regs_count(struct gk20a *g);
u32 nvgpu_next_netlist_get_gpc_ctxsw_regs_count(struct gk20a *g);
u32 nvgpu_next_netlist_get_tpc_ctxsw_regs_count(struct gk20a *g);
u32 nvgpu_next_netlist_get_etpc_ctxsw_regs_count(struct gk20a *g);
void nvgpu_next_netlist_print_ctxsw_reg_info(struct gk20a *g);
#endif /* CONFIG_NVGPU_DEBUGGER */

#endif /* NVGPU_NEXT_NETLIST_H */
