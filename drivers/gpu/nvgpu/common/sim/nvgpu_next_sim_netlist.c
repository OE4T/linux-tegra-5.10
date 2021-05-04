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

#include <nvgpu/gk20a.h>
#include <nvgpu/sim.h>
#include <nvgpu/netlist.h>
#include "nvgpu/nvgpu_next_sim.h"

int nvgpu_next_init_sim_netlist_ctx_vars(struct gk20a *g)
{
	u32 i;
	struct netlist_av_list *sw_non_ctx_local_compute_load;
	struct netlist_av_list *sw_non_ctx_local_gfx_load;
	struct netlist_av_list *sw_non_ctx_global_compute_load;
	struct netlist_av_list *sw_non_ctx_global_gfx_load;

	sw_non_ctx_local_compute_load =
		nvgpu_next_netlist_get_sw_non_ctx_local_compute_load_av_list(g);

	/* query sizes and counts */
	g->sim->esc_readl(g, "GRCTX_NONCTXSW_LOCAL_COMPUTE_REG_SIZE", 0,
			    &sw_non_ctx_local_compute_load->count);

	if (nvgpu_netlist_alloc_av_list(g, sw_non_ctx_local_compute_load) ==
									NULL) {
		nvgpu_info(g, "sw_non_ctx_local_compute_load failed");
	}

	for (i = 0; i < sw_non_ctx_local_compute_load->count; i++) {
		struct netlist_av *l = sw_non_ctx_local_compute_load->l;
		g->sim->esc_readl(g, "GRCTX_NONCTXSW_LOCAL_COMPUTE_REG:REG",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_NONCTXSW_LOCAL_COMPUTE_REG:VALUE",
				    i, &l[i].value);
	}

#ifdef CONFIG_NVGPU_GRAPHICS
	sw_non_ctx_local_gfx_load =
		nvgpu_next_netlist_get_sw_non_ctx_local_gfx_load_av_list(g);

	/* query sizes and counts */
	g->sim->esc_readl(g, "GRCTX_NONCTXSW_LOCAL_GRAPHICS_REG_SIZE", 0,
			    &sw_non_ctx_local_gfx_load->count);

	if (nvgpu_netlist_alloc_av_list(g, sw_non_ctx_local_gfx_load) ==
									NULL) {
		nvgpu_info(g, "sw_non_ctx_local_gfx_load failed");
	}

	for (i = 0; i < sw_non_ctx_local_gfx_load->count; i++) {
		struct netlist_av *l = sw_non_ctx_local_gfx_load->l;
		g->sim->esc_readl(g, "GRCTX_NONCTXSW_LOCAL_GRAPHICS_REG:REG",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_NONCTXSW_LOCAL_GRAPHICS_REG:VALUE",
				    i, &l[i].value);
	}
#endif


	sw_non_ctx_global_compute_load =
		nvgpu_next_netlist_get_sw_non_ctx_global_compute_load_av_list(g);

	/* query sizes and counts */
	g->sim->esc_readl(g, "GRCTX_NONCTXSW_GLOBAL_COMPUTE_REG_SIZE", 0,
			    &sw_non_ctx_global_compute_load->count);

	if (nvgpu_netlist_alloc_av_list(g, sw_non_ctx_global_compute_load) ==
									NULL) {
		nvgpu_info(g, "sw_non_ctx_global_compute_load failed");
	}

	for (i = 0; i < sw_non_ctx_global_compute_load->count; i++) {
		struct netlist_av *l = sw_non_ctx_global_compute_load->l;
		g->sim->esc_readl(g, "GRCTX_NONCTXSW_GLOBAL_COMPUTE_REG:REG",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_NONCTXSW_GLOBAL_COMPUTE_REG:VALUE",
				    i, &l[i].value);
	}

#ifdef CONFIG_NVGPU_GRAPHICS
	sw_non_ctx_global_gfx_load =
		nvgpu_next_netlist_get_sw_non_ctx_global_gfx_load_av_list(g);

	/* query sizes and counts */
	g->sim->esc_readl(g, "GRCTX_NONCTXSW_GLOBAL_GRAPHICS_REG_SIZE", 0,
			    &sw_non_ctx_global_gfx_load->count);

	if (nvgpu_netlist_alloc_av_list(g, sw_non_ctx_global_gfx_load) ==
									NULL) {
		nvgpu_info(g, "sw_non_ctx_global_gfx_load failed");
	}

	for (i = 0; i < sw_non_ctx_global_gfx_load->count; i++) {
		struct netlist_av *l = sw_non_ctx_global_gfx_load->l;
		g->sim->esc_readl(g, "GRCTX_NONCTXSW_GLOBAL_GRAPHICS_REG:REG",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_NONCTXSW_GLOBAL_GRAPHICS_REG:VALUE",
				    i, &l[i].value);
	}
#endif
	return 0;
}

void nvgpu_next_init_sim_netlist_ctx_vars_free(struct gk20a *g)
{
	struct netlist_av_list *sw_non_ctx_local_compute_load;
	struct netlist_av_list *sw_non_ctx_local_gfx_load;
	struct netlist_av_list *sw_non_ctx_global_compute_load;
	struct netlist_av_list *sw_non_ctx_global_gfx_load;

	sw_non_ctx_local_compute_load =
		nvgpu_next_netlist_get_sw_non_ctx_local_compute_load_av_list(g);
	sw_non_ctx_global_compute_load =
		nvgpu_next_netlist_get_sw_non_ctx_global_compute_load_av_list(g);


	nvgpu_kfree(g, sw_non_ctx_local_compute_load->l);
	nvgpu_kfree(g, sw_non_ctx_global_compute_load->l);

#ifdef CONFIG_NVGPU_GRAPHICS
	sw_non_ctx_local_gfx_load =
		nvgpu_next_netlist_get_sw_non_ctx_local_gfx_load_av_list(g);
	sw_non_ctx_global_gfx_load =
		nvgpu_next_netlist_get_sw_non_ctx_global_gfx_load_av_list(g);

	nvgpu_kfree(g, sw_non_ctx_local_gfx_load->l);
	nvgpu_kfree(g, sw_non_ctx_global_gfx_load->l);
#endif
}

#ifdef CONFIG_NVGPU_DEBUGGER
int nvgpu_next_init_sim_netlist_ctxsw_regs(struct gk20a *g)
{
	u32 i;
	struct netlist_aiv_list *sys_compute_ctxsw_regs;
	struct netlist_aiv_list *gpc_compute_ctxsw_regs;
	struct netlist_aiv_list *tpc_compute_ctxsw_regs;
	struct netlist_aiv_list *ppc_compute_ctxsw_regs;
	struct netlist_aiv_list *etpc_compute_ctxsw_regs;
	struct netlist_aiv_list *lts_ctxsw_regs;
	struct netlist_aiv_list *sys_gfx_ctxsw_regs;
	struct netlist_aiv_list *gpc_gfx_ctxsw_regs;
	struct netlist_aiv_list *tpc_gfx_ctxsw_regs;
	struct netlist_aiv_list *ppc_gfx_ctxsw_regs;
	struct netlist_aiv_list *etpc_gfx_ctxsw_regs;

	sys_compute_ctxsw_regs =
			nvgpu_next_netlist_get_sys_compute_ctxsw_regs(g);

	/* query sizes and counts */
	g->sim->esc_readl(g, "GRCTX_REG_LIST_SYS_COMPUTE_COUNT", 0,
			    &sys_compute_ctxsw_regs->count);

	if (nvgpu_netlist_alloc_aiv_list(g, sys_compute_ctxsw_regs) == NULL) {
		nvgpu_info(g, "sys_compute_ctxsw_regs failed");
	}

	for (i = 0; i < sys_compute_ctxsw_regs->count; i++) {
		struct netlist_aiv *l = sys_compute_ctxsw_regs->l;
		g->sim->esc_readl(g, "GRCTX_REG_LIST_SYS_COMPUTE:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_SYS_COMPUTE:INDEX",
				    i, &l[i].index);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_SYS_COMPUTE:VALUE",
				    i, &l[i].value);
	}

	gpc_compute_ctxsw_regs =
			nvgpu_next_netlist_get_gpc_compute_ctxsw_regs(g);

	/* query sizes and counts */
	g->sim->esc_readl(g, "GRCTX_REG_LIST_GPC_COMPUTE_COUNT", 0,
			    &gpc_compute_ctxsw_regs->count);

	if (nvgpu_netlist_alloc_aiv_list(g, gpc_compute_ctxsw_regs) == NULL) {
		nvgpu_info(g, "gpc_compute_ctxsw_regs failed");
	}

	for (i = 0; i < gpc_compute_ctxsw_regs->count; i++) {
		struct netlist_aiv *l = gpc_compute_ctxsw_regs->l;
		g->sim->esc_readl(g, "GRCTX_REG_LIST_GPC_COMPUTE:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_GPC_COMPUTE:INDEX",
				    i, &l[i].index);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_GPC_COMPUTE:VALUE",
				    i, &l[i].value);
	}

	tpc_compute_ctxsw_regs =
			nvgpu_next_netlist_get_tpc_compute_ctxsw_regs(g);

	/* query sizes and counts */
	g->sim->esc_readl(g, "GRCTX_REG_LIST_TPC_COMPUTE_COUNT", 0,
			    &tpc_compute_ctxsw_regs->count);

	if (nvgpu_netlist_alloc_aiv_list(g, tpc_compute_ctxsw_regs) == NULL) {
		nvgpu_info(g, "tpc_compute_ctxsw_regs failed");
	}

	for (i = 0; i < tpc_compute_ctxsw_regs->count; i++) {
		struct netlist_aiv *l = tpc_compute_ctxsw_regs->l;
		g->sim->esc_readl(g, "GRCTX_REG_LIST_TPC_COMPUTE:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_TPC_COMPUTE:INDEX",
				    i, &l[i].index);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_TPC_COMPUTE:VALUE",
				    i, &l[i].value);
	}

	ppc_compute_ctxsw_regs =
			nvgpu_next_netlist_get_ppc_compute_ctxsw_regs(g);

	/* query sizes and counts */
	g->sim->esc_readl(g, "GRCTX_REG_LIST_PPC_COMPUTE_COUNT", 0,
			    &ppc_compute_ctxsw_regs->count);

	if (nvgpu_netlist_alloc_aiv_list(g, ppc_compute_ctxsw_regs) == NULL) {
		nvgpu_info(g, "ppc_compute_ctxsw_regs failed");
	}

	for (i = 0; i < ppc_compute_ctxsw_regs->count; i++) {
		struct netlist_aiv *l = ppc_compute_ctxsw_regs->l;
		g->sim->esc_readl(g, "GRCTX_REG_LIST_PPC_COMPUTE:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_PPC_COMPUTE:INDEX",
				    i, &l[i].index);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_PPC_COMPUTE:VALUE",
				    i, &l[i].value);
	}

	etpc_compute_ctxsw_regs =
			nvgpu_next_netlist_get_etpc_compute_ctxsw_regs(g);

	/* query sizes and counts */
	g->sim->esc_readl(g, "GRCTX_REG_LIST_ETPC_COMPUTE_COUNT", 0,
			    &etpc_compute_ctxsw_regs->count);

	if (nvgpu_netlist_alloc_aiv_list(g, etpc_compute_ctxsw_regs) == NULL) {
		nvgpu_info(g, "etpc_compute_ctxsw_regs failed");
	}

	for (i = 0; i < etpc_compute_ctxsw_regs->count; i++) {
		struct netlist_aiv *l = etpc_compute_ctxsw_regs->l;
		g->sim->esc_readl(g, "GRCTX_REG_LIST_ETPC_COMPUTE:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_ETPC_COMPUTE:INDEX",
				    i, &l[i].index);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_ETPC_COMPUTE:VALUE",
				    i, &l[i].value);
	}

	/*
	 * TODO: https://jirasw.nvidia.com/browse/NVGPU-5761
	 */
	lts_ctxsw_regs = nvgpu_next_netlist_get_lts_ctxsw_regs(g);

	/* query sizes and counts */
	g->sim->esc_readl(g, "GRCTX_REG_LIST_LTS_BC_COUNT", 0,
				&lts_ctxsw_regs->count);
	nvgpu_log_info(g, "total: %d lts registers", lts_ctxsw_regs->count);

	if (nvgpu_netlist_alloc_aiv_list(g, lts_ctxsw_regs) == NULL) {
		nvgpu_info(g, "lts_ctxsw_regs failed");
	}

	for (i = 0U; i < lts_ctxsw_regs->count; i++) {
		struct netlist_aiv *l = lts_ctxsw_regs->l;
		g->sim->esc_readl(g, "GRCTX_REG_LIST_LTS_BC:ADDR",
					i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_LTS_BC:INDEX",
					i, &l[i].index);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_LTS_BC:VALUE",
					i, &l[i].value);
		nvgpu_log_info(g, "entry(%d) a(0x%x) i(%d) v(0x%x)", i, l[i].addr,
				l[i].index, l[i].value);
	}

	sys_gfx_ctxsw_regs = nvgpu_next_netlist_get_sys_gfx_ctxsw_regs(g);

	/* query sizes and counts */
	g->sim->esc_readl(g, "GRCTX_REG_LIST_SYS_GRAPHICS_COUNT", 0,
			    &sys_gfx_ctxsw_regs->count);

	if (nvgpu_netlist_alloc_aiv_list(g, sys_gfx_ctxsw_regs) == NULL) {
		nvgpu_info(g, "sys_gfx_ctxsw_regs failed");
	}

	for (i = 0; i < sys_gfx_ctxsw_regs->count; i++) {
		struct netlist_aiv *l = sys_gfx_ctxsw_regs->l;
		g->sim->esc_readl(g, "GRCTX_REG_LIST_SYS_GRAPHICS:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_SYS_GRAPHICS:INDEX",
				    i, &l[i].index);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_SYS_GRAPHICS:VALUE",
				    i, &l[i].value);
	}

	gpc_gfx_ctxsw_regs = nvgpu_next_netlist_get_gpc_gfx_ctxsw_regs(g);

	/* query sizes and counts */
	g->sim->esc_readl(g, "GRCTX_REG_LIST_GPC_GRAPHICS_COUNT", 0,
			    &gpc_gfx_ctxsw_regs->count);

	if (nvgpu_netlist_alloc_aiv_list(g, gpc_gfx_ctxsw_regs) == NULL) {
		nvgpu_info(g, "gpc_gfx_ctxsw_regs failed");
	}

	for (i = 0; i < gpc_gfx_ctxsw_regs->count; i++) {
		struct netlist_aiv *l = gpc_gfx_ctxsw_regs->l;
		g->sim->esc_readl(g, "GRCTX_REG_LIST_GPC_GRAPHICS:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_GPC_GRAPHICS:INDEX",
				    i, &l[i].index);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_GPC_GRAPHICS:VALUE",
				    i, &l[i].value);
	}

	tpc_gfx_ctxsw_regs = nvgpu_next_netlist_get_tpc_gfx_ctxsw_regs(g);

	/* query sizes and counts */
	g->sim->esc_readl(g, "GRCTX_REG_LIST_TPC_GRAPHICS_COUNT", 0,
			    &tpc_gfx_ctxsw_regs->count);

	if (nvgpu_netlist_alloc_aiv_list(g, tpc_gfx_ctxsw_regs) == NULL) {
		nvgpu_info(g, "tpc_gfx_ctxsw_regs failed");
	}

	for (i = 0; i < tpc_gfx_ctxsw_regs->count; i++) {
		struct netlist_aiv *l = tpc_gfx_ctxsw_regs->l;
		g->sim->esc_readl(g, "GRCTX_REG_LIST_TPC_GRAPHICS:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_TPC_GRAPHICS:INDEX",
				    i, &l[i].index);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_TPC_GRAPHICS:VALUE",
				    i, &l[i].value);
	}

	ppc_gfx_ctxsw_regs = nvgpu_next_netlist_get_ppc_gfx_ctxsw_regs(g);

	/* query sizes and counts */
	g->sim->esc_readl(g, "GRCTX_REG_LIST_PPC_GRAPHICS_COUNT", 0,
			    &ppc_gfx_ctxsw_regs->count);

	if (nvgpu_netlist_alloc_aiv_list(g, ppc_gfx_ctxsw_regs) == NULL) {
		nvgpu_info(g, "ppc_gfx_ctxsw_regs failed");
	}

	for (i = 0; i < ppc_gfx_ctxsw_regs->count; i++) {
		struct netlist_aiv *l = ppc_gfx_ctxsw_regs->l;
		g->sim->esc_readl(g, "GRCTX_REG_LIST_PPC_GRAPHICS:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_PPC_GRAPHICS:INDEX",
				    i, &l[i].index);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_PPC_GRAPHICS:VALUE",
				    i, &l[i].value);
	}

	etpc_gfx_ctxsw_regs = nvgpu_next_netlist_get_etpc_gfx_ctxsw_regs(g);

	/* query sizes and counts */
	g->sim->esc_readl(g, "GRCTX_REG_LIST_ETPC_GRAPHICS_COUNT", 0,
			    &etpc_gfx_ctxsw_regs->count);

	if (nvgpu_netlist_alloc_aiv_list(g, etpc_gfx_ctxsw_regs) == NULL) {
		nvgpu_info(g, "etpc_gfx_ctxsw_regs failed");
	}

	for (i = 0; i < etpc_gfx_ctxsw_regs->count; i++) {
		struct netlist_aiv *l = etpc_gfx_ctxsw_regs->l;
		g->sim->esc_readl(g, "GRCTX_REG_LIST_ETPC_GRAPHICS:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_ETPC_GRAPHICS:INDEX",
				    i, &l[i].index);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_ETPC_GRAPHICS:VALUE",
				    i, &l[i].value);
	}

	return 0;
}

void nvgpu_next_init_sim_netlist_ctxsw_regs_free(struct gk20a *g)
{
	struct netlist_aiv_list *sys_compute_ctxsw_regs;
	struct netlist_aiv_list *gpc_compute_ctxsw_regs;
	struct netlist_aiv_list *tpc_compute_ctxsw_regs;
	struct netlist_aiv_list *ppc_compute_ctxsw_regs;
	struct netlist_aiv_list *etpc_compute_ctxsw_regs;
	struct netlist_aiv_list *lts_ctxsw_regs;
	struct netlist_aiv_list *sys_gfx_ctxsw_regs;
	struct netlist_aiv_list *gpc_gfx_ctxsw_regs;
	struct netlist_aiv_list *tpc_gfx_ctxsw_regs;
	struct netlist_aiv_list *ppc_gfx_ctxsw_regs;
	struct netlist_aiv_list *etpc_gfx_ctxsw_regs;

	sys_compute_ctxsw_regs =
			nvgpu_next_netlist_get_sys_compute_ctxsw_regs(g);
	gpc_compute_ctxsw_regs =
			nvgpu_next_netlist_get_gpc_compute_ctxsw_regs(g);
	tpc_compute_ctxsw_regs =
			nvgpu_next_netlist_get_tpc_compute_ctxsw_regs(g);
	ppc_compute_ctxsw_regs =
			nvgpu_next_netlist_get_ppc_compute_ctxsw_regs(g);
	etpc_compute_ctxsw_regs =
			nvgpu_next_netlist_get_etpc_compute_ctxsw_regs(g);
	lts_ctxsw_regs = nvgpu_next_netlist_get_lts_ctxsw_regs(g);

	nvgpu_kfree(g, sys_compute_ctxsw_regs->l);
	nvgpu_kfree(g, gpc_compute_ctxsw_regs->l);
	nvgpu_kfree(g, tpc_compute_ctxsw_regs->l);
	nvgpu_kfree(g, ppc_compute_ctxsw_regs->l);
	nvgpu_kfree(g, etpc_compute_ctxsw_regs->l);
	nvgpu_kfree(g, lts_ctxsw_regs->l);

	sys_gfx_ctxsw_regs = nvgpu_next_netlist_get_sys_gfx_ctxsw_regs(g);
	gpc_gfx_ctxsw_regs = nvgpu_next_netlist_get_gpc_gfx_ctxsw_regs(g);
	tpc_gfx_ctxsw_regs = nvgpu_next_netlist_get_tpc_gfx_ctxsw_regs(g);
	ppc_gfx_ctxsw_regs = nvgpu_next_netlist_get_ppc_gfx_ctxsw_regs(g);
	etpc_gfx_ctxsw_regs = nvgpu_next_netlist_get_etpc_gfx_ctxsw_regs(g);

	nvgpu_kfree(g, sys_gfx_ctxsw_regs->l);
	nvgpu_kfree(g, gpc_gfx_ctxsw_regs->l);
	nvgpu_kfree(g, tpc_gfx_ctxsw_regs->l);
	nvgpu_kfree(g, ppc_gfx_ctxsw_regs->l);
	nvgpu_kfree(g, etpc_gfx_ctxsw_regs->l);
}
#endif /* CONFIG_NVGPU_DEBUGGER */
