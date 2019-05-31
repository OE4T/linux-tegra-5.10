/*
 * Copyright (c) 2011-2019, NVIDIA CORPORATION.  All rights reserved.
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
#ifndef NVGPU_NETLIST_H
#define NVGPU_NETLIST_H

#include <nvgpu/types.h>

struct gk20a;

struct netlist_av {
	u32 addr;
	u32 value;
};
struct netlist_av64 {
	u32 addr;
	u32 value_lo;
	u32 value_hi;
};
struct netlist_aiv {
	u32 addr;
	u32 index;
	u32 value;
};
struct netlist_aiv_list {
	struct netlist_aiv *l;
	u32 count;
};
struct netlist_av_list {
	struct netlist_av *l;
	u32 count;
};
struct netlist_av64_list {
	struct netlist_av64 *l;
	u32 count;
};
struct netlist_u32_list {
	u32 *l;
	u32 count;
};

struct ctxsw_buf_offset_map_entry {
	u32 addr;	/* Register address */
	u32 offset;	/* Offset in ctxt switch buffer */
};

struct netlist_av *nvgpu_netlist_alloc_av_list(struct gk20a *g, struct netlist_av_list *avl);
struct netlist_av64 *nvgpu_netlist_alloc_av64_list(struct gk20a *g, struct netlist_av64_list *avl);
struct netlist_aiv *nvgpu_netlist_alloc_aiv_list(struct gk20a *g, struct netlist_aiv_list *aivl);
u32 *nvgpu_netlist_alloc_u32_list(struct gk20a *g, struct netlist_u32_list *u32l);


int nvgpu_netlist_init_ctx_vars(struct gk20a *g);
void nvgpu_netlist_deinit_ctx_vars(struct gk20a *g);

struct netlist_av_list *nvgpu_netlist_get_sw_non_ctx_load_av_list(
							struct gk20a *g);
struct netlist_aiv_list *nvgpu_netlist_get_sw_ctx_load_aiv_list(
							struct gk20a *g);
struct netlist_av_list *nvgpu_netlist_get_sw_method_init_av_list(
							struct gk20a *g);
struct netlist_av_list *nvgpu_netlist_get_sw_bundle_init_av_list(
							struct gk20a *g);
struct netlist_av_list *nvgpu_netlist_get_sw_veid_bundle_init_av_list(
							struct gk20a *g);
struct netlist_av64_list *nvgpu_netlist_get_sw_bundle64_init_av64_list(
							struct gk20a *g);

u32 nvgpu_netlist_get_fecs_inst_count(struct gk20a *g);
u32 nvgpu_netlist_get_fecs_data_count(struct gk20a *g);
u32 nvgpu_netlist_get_gpccs_inst_count(struct gk20a *g);
u32 nvgpu_netlist_get_gpccs_data_count(struct gk20a *g);
void nvgpu_netlist_set_fecs_inst_count(struct gk20a *g, u32 count);
void nvgpu_netlist_set_fecs_data_count(struct gk20a *g, u32 count);
void nvgpu_netlist_set_gpccs_inst_count(struct gk20a *g, u32 count);
void nvgpu_netlist_set_gpccs_data_count(struct gk20a *g, u32 count);
u32 *nvgpu_netlist_get_fecs_inst_list(struct gk20a *g);
u32 *nvgpu_netlist_get_fecs_data_list(struct gk20a *g);
u32 *nvgpu_netlist_get_gpccs_inst_list(struct gk20a *g);
u32 *nvgpu_netlist_get_gpccs_data_list(struct gk20a *g);
struct netlist_u32_list *nvgpu_netlist_get_fecs_inst(struct gk20a *g);
struct netlist_u32_list *nvgpu_netlist_get_fecs_data(struct gk20a *g);
struct netlist_u32_list *nvgpu_netlist_get_gpccs_inst(struct gk20a *g);
struct netlist_u32_list *nvgpu_netlist_get_gpccs_data(struct gk20a *g);

struct netlist_aiv_list *nvgpu_netlist_get_sys_ctxsw_regs(struct gk20a *g);
struct netlist_aiv_list *nvgpu_netlist_get_gpc_ctxsw_regs(struct gk20a *g);
struct netlist_aiv_list *nvgpu_netlist_get_tpc_ctxsw_regs(struct gk20a *g);
#ifdef NVGPU_GRAPHICS
struct netlist_aiv_list *nvgpu_netlist_get_zcull_gpc_ctxsw_regs(
							struct gk20a *g);
#endif
struct netlist_aiv_list *nvgpu_netlist_get_ppc_ctxsw_regs(struct gk20a *g);
struct netlist_aiv_list *nvgpu_netlist_get_pm_sys_ctxsw_regs(struct gk20a *g);
struct netlist_aiv_list *nvgpu_netlist_get_pm_gpc_ctxsw_regs(struct gk20a *g);
struct netlist_aiv_list *nvgpu_netlist_get_pm_tpc_ctxsw_regs(struct gk20a *g);
struct netlist_aiv_list *nvgpu_netlist_get_pm_ppc_ctxsw_regs(struct gk20a *g);
struct netlist_aiv_list *nvgpu_netlist_get_perf_sys_ctxsw_regs(struct gk20a *g);
struct netlist_aiv_list *nvgpu_netlist_get_perf_gpc_ctxsw_regs(struct gk20a *g);
struct netlist_aiv_list *nvgpu_netlist_get_fbp_ctxsw_regs(struct gk20a *g);
struct netlist_aiv_list *nvgpu_netlist_get_fbp_router_ctxsw_regs(
							struct gk20a *g);
struct netlist_aiv_list *nvgpu_netlist_get_gpc_router_ctxsw_regs(
							struct gk20a *g);
struct netlist_aiv_list *nvgpu_netlist_get_pm_ltc_ctxsw_regs(struct gk20a *g);
struct netlist_aiv_list *nvgpu_netlist_get_pm_fbpa_ctxsw_regs(struct gk20a *g);
struct netlist_aiv_list *nvgpu_netlist_get_perf_sys_router_ctxsw_regs(
							struct gk20a *g);
struct netlist_aiv_list *nvgpu_netlist_get_perf_pma_ctxsw_regs(struct gk20a *g);
struct netlist_aiv_list *nvgpu_netlist_get_pm_rop_ctxsw_regs(struct gk20a *g);
struct netlist_aiv_list *nvgpu_netlist_get_pm_ucgpc_ctxsw_regs(struct gk20a *g);
struct netlist_aiv_list *nvgpu_netlist_get_etpc_ctxsw_regs(struct gk20a *g);
struct netlist_aiv_list *nvgpu_netlist_get_pm_cau_ctxsw_regs(struct gk20a *g);

void nvgpu_netlist_vars_set_dynamic(struct gk20a *g, bool set);
void nvgpu_netlist_vars_set_buffer_size(struct gk20a *g, u32 size);
void nvgpu_netlist_vars_set_regs_base_index(struct gk20a *g, u32 index);

#endif /* NVGPU_NETLIST_H */
