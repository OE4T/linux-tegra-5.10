/*
 * Copyright (c) 2011-2020, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/log.h>
#if defined(CONFIG_NVGPU_NON_FUSA) && defined(CONFIG_NVGPU_NEXT)
#include "nvgpu/nvgpu_next_sim.h"
#endif

int nvgpu_init_sim_netlist_ctx_vars(struct gk20a *g)
{
	int err = -ENOMEM;
	u32 i, temp;
	u32 fecs_inst_count, fecs_data_count;
	u32 gpccs_inst_count, gpccs_data_count;
	struct netlist_av_list *sw_bundle_init;
	struct netlist_av_list *sw_method_init;
	struct netlist_aiv_list *sw_ctx_load;
	struct netlist_av_list *sw_non_ctx_load;
	struct netlist_av_list *sw_veid_bundle_init;
	struct netlist_av64_list *sw_bundle64_init;
#ifdef CONFIG_NVGPU_DEBUGGER
	struct netlist_aiv_list *sys_ctxsw_regs;
	struct netlist_aiv_list *gpc_ctxsw_regs;
	struct netlist_aiv_list *tpc_ctxsw_regs;
#ifdef CONFIG_NVGPU_GRAPHICS
	struct netlist_aiv_list *zcull_gpc_ctxsw_regs;
#endif
	struct netlist_aiv_list *pm_sys_ctxsw_regs;
	struct netlist_aiv_list *pm_gpc_ctxsw_regs;
	struct netlist_aiv_list *pm_tpc_ctxsw_regs;
	struct netlist_aiv_list *ppc_ctxsw_regs;
	struct netlist_aiv_list *etpc_ctxsw_regs;
#endif /* CONFIG_NVGPU_DEBUGGER */
	struct netlist_u32_list *fecs_inst, *fecs_data;
	struct netlist_u32_list *gpccs_inst, *gpccs_data;
	u32 regs_base_index;

	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_info,
		   "querying grctx info from chiplib");

	nvgpu_netlist_vars_set_dynamic(g, true);

	if (g->sim->esc_readl == NULL) {
		nvgpu_err(g, "Invalid pointer to query function.");
		err = -ENOENT;
		return err;
	}

	sw_bundle_init = nvgpu_netlist_get_sw_bundle_init_av_list(g);
	sw_method_init = nvgpu_netlist_get_sw_method_init_av_list(g);
	sw_ctx_load = nvgpu_netlist_get_sw_ctx_load_aiv_list(g);
	sw_non_ctx_load = nvgpu_netlist_get_sw_non_ctx_load_av_list(g);
	sw_veid_bundle_init = nvgpu_netlist_get_sw_veid_bundle_init_av_list(g);
	sw_bundle64_init = nvgpu_netlist_get_sw_bundle64_init_av64_list(g);

#ifdef CONFIG_NVGPU_DEBUGGER
	sys_ctxsw_regs = nvgpu_netlist_get_sys_ctxsw_regs(g);
	gpc_ctxsw_regs = nvgpu_netlist_get_gpc_ctxsw_regs(g);
	tpc_ctxsw_regs = nvgpu_netlist_get_tpc_ctxsw_regs(g);
#ifdef CONFIG_NVGPU_GRAPHICS
	zcull_gpc_ctxsw_regs = nvgpu_netlist_get_zcull_gpc_ctxsw_regs(g);
#endif
	pm_sys_ctxsw_regs = nvgpu_netlist_get_pm_sys_ctxsw_regs(g);
	pm_gpc_ctxsw_regs = nvgpu_netlist_get_pm_gpc_ctxsw_regs(g);
	pm_tpc_ctxsw_regs = nvgpu_netlist_get_pm_tpc_ctxsw_regs(g);
	ppc_ctxsw_regs = nvgpu_netlist_get_ppc_ctxsw_regs(g);
	etpc_ctxsw_regs = nvgpu_netlist_get_etpc_ctxsw_regs(g);
#endif /* CONFIG_NVGPU_DEBUGGER */

	fecs_inst = nvgpu_netlist_get_fecs_inst(g);
	fecs_data = nvgpu_netlist_get_fecs_data(g);
	gpccs_inst = nvgpu_netlist_get_gpccs_inst(g);
	gpccs_data = nvgpu_netlist_get_gpccs_data(g);

	/* query sizes and counts */
	g->sim->esc_readl(g, "GRCTX_UCODE_INST_FECS_COUNT", 0,
			&fecs_inst_count);
	nvgpu_netlist_set_fecs_inst_count(g, fecs_inst_count);
	g->sim->esc_readl(g, "GRCTX_UCODE_DATA_FECS_COUNT", 0,
			&fecs_data_count);
	nvgpu_netlist_set_fecs_data_count(g, fecs_data_count);
	g->sim->esc_readl(g, "GRCTX_UCODE_INST_GPCCS_COUNT", 0,
			&gpccs_inst_count);
	nvgpu_netlist_set_gpccs_inst_count(g, gpccs_inst_count);
	g->sim->esc_readl(g, "GRCTX_UCODE_DATA_GPCCS_COUNT", 0,
			&gpccs_data_count);
	nvgpu_netlist_set_gpccs_data_count(g, gpccs_data_count);
	g->sim->esc_readl(g, "GRCTX_ALL_CTX_TOTAL_WORDS", 0, &temp);
	nvgpu_netlist_vars_set_buffer_size(g, (temp << 2));
	g->sim->esc_readl(g, "GRCTX_SW_BUNDLE_INIT_SIZE", 0,
			    &sw_bundle_init->count);
	g->sim->esc_readl(g, "GRCTX_SW_METHOD_INIT_SIZE", 0,
			    &sw_method_init->count);
	g->sim->esc_readl(g, "GRCTX_SW_CTX_LOAD_SIZE", 0,
			    &sw_ctx_load->count);
	g->sim->esc_readl(g, "GRCTX_SW_VEID_BUNDLE_INIT_SIZE", 0,
			    &sw_veid_bundle_init->count);
	g->sim->esc_readl(g, "GRCTX_SW_BUNDLE64_INIT_SIZE", 0,
			    &sw_bundle64_init->count);
	g->sim->esc_readl(g, "GRCTX_NONCTXSW_REG_SIZE", 0,
			    &sw_non_ctx_load->count);
#ifdef CONFIG_NVGPU_DEBUGGER
	g->sim->esc_readl(g, "GRCTX_REG_LIST_SYS_COUNT", 0,
			    &sys_ctxsw_regs->count);
	g->sim->esc_readl(g, "GRCTX_REG_LIST_GPC_COUNT", 0,
			    &gpc_ctxsw_regs->count);
	g->sim->esc_readl(g, "GRCTX_REG_LIST_TPC_COUNT", 0,
			    &tpc_ctxsw_regs->count);
#ifdef CONFIG_NVGPU_GRAPHICS
	g->sim->esc_readl(g, "GRCTX_REG_LIST_ZCULL_GPC_COUNT", 0,
			    &zcull_gpc_ctxsw_regs->count);
#endif
	g->sim->esc_readl(g, "GRCTX_REG_LIST_PM_SYS_COUNT", 0,
			    &pm_sys_ctxsw_regs->count);
	g->sim->esc_readl(g, "GRCTX_REG_LIST_PM_GPC_COUNT", 0,
			    &pm_gpc_ctxsw_regs->count);
	g->sim->esc_readl(g, "GRCTX_REG_LIST_PM_TPC_COUNT", 0,
			    &pm_tpc_ctxsw_regs->count);
	g->sim->esc_readl(g, "GRCTX_REG_LIST_PPC_COUNT", 0,
			    &ppc_ctxsw_regs->count);
	g->sim->esc_readl(g, "GRCTX_REG_LIST_ETPC_COUNT", 0,
			    &etpc_ctxsw_regs->count);
#endif /* CONFIG_NVGPU_DEBUGGER */

	if (nvgpu_netlist_alloc_u32_list(g, fecs_inst) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_u32_list(g, fecs_data) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_u32_list(g, gpccs_inst) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_u32_list(g, gpccs_data) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_av_list(g, sw_bundle_init) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_av64_list(g, sw_bundle64_init) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_av_list(g, sw_method_init) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_aiv_list(g, sw_ctx_load) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_av_list(g, sw_non_ctx_load) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_av_list(g, sw_veid_bundle_init) == NULL) {
		goto fail;
	}
#ifdef CONFIG_NVGPU_DEBUGGER
	if (nvgpu_netlist_alloc_aiv_list(g, sys_ctxsw_regs) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_aiv_list(g, gpc_ctxsw_regs) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_aiv_list(g, tpc_ctxsw_regs) == NULL) {
		goto fail;
	}
#ifdef CONFIG_NVGPU_GRAPHICS
	if (nvgpu_netlist_alloc_aiv_list(g, zcull_gpc_ctxsw_regs) == NULL) {
		goto fail;
	}
#endif
	if (nvgpu_netlist_alloc_aiv_list(g, ppc_ctxsw_regs) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_aiv_list(g, pm_sys_ctxsw_regs) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_aiv_list(g, pm_gpc_ctxsw_regs) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_aiv_list(g, pm_tpc_ctxsw_regs) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_aiv_list(g, etpc_ctxsw_regs) == NULL) {
		goto fail;
	}
#endif /* CONFIG_NVGPU_DEBUGGER */
#if defined(CONFIG_NVGPU_NON_FUSA) && defined(CONFIG_NVGPU_NEXT)
	nvgpu_next_init_sim_netlist_ctx_vars(g);
#endif

	for (i = 0; i < nvgpu_netlist_get_fecs_inst_count(g); i++) {
		g->sim->esc_readl(g, "GRCTX_UCODE_INST_FECS",
				    i, &fecs_inst->l[i]);
	}

	for (i = 0; i < nvgpu_netlist_get_fecs_data_count(g); i++) {
		g->sim->esc_readl(g, "GRCTX_UCODE_DATA_FECS",
				    i, &fecs_data->l[i]);
	}

	for (i = 0; i < nvgpu_netlist_get_gpccs_inst_count(g); i++) {
		g->sim->esc_readl(g, "GRCTX_UCODE_INST_GPCCS",
				    i, &gpccs_inst->l[i]);
	}

	for (i = 0; i < nvgpu_netlist_get_gpccs_data_count(g); i++) {
		g->sim->esc_readl(g, "GRCTX_UCODE_DATA_GPCCS",
				    i, &gpccs_data->l[i]);
	}

	for (i = 0; i < sw_bundle_init->count; i++) {
		struct netlist_av *l = sw_bundle_init->l;
		g->sim->esc_readl(g, "GRCTX_SW_BUNDLE_INIT:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_SW_BUNDLE_INIT:VALUE",
				    i, &l[i].value);
	}

	for (i = 0; i < sw_method_init->count; i++) {
		struct netlist_av *l = sw_method_init->l;
		g->sim->esc_readl(g, "GRCTX_SW_METHOD_INIT:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_SW_METHOD_INIT:VALUE",
				    i, &l[i].value);
	}

	for (i = 0; i < sw_ctx_load->count; i++) {
		struct netlist_aiv *l = sw_ctx_load->l;
		g->sim->esc_readl(g, "GRCTX_SW_CTX_LOAD:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_SW_CTX_LOAD:INDEX",
				    i, &l[i].index);
		g->sim->esc_readl(g, "GRCTX_SW_CTX_LOAD:VALUE",
				    i, &l[i].value);
	}

	for (i = 0; i < sw_non_ctx_load->count; i++) {
		struct netlist_av *l = sw_non_ctx_load->l;
		g->sim->esc_readl(g, "GRCTX_NONCTXSW_REG:REG",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_NONCTXSW_REG:VALUE",
				    i, &l[i].value);
	}

	for (i = 0; i < sw_veid_bundle_init->count; i++) {
		struct netlist_av *l = sw_veid_bundle_init->l;

		g->sim->esc_readl(g, "GRCTX_SW_VEID_BUNDLE_INIT:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_SW_VEID_BUNDLE_INIT:VALUE",
				    i, &l[i].value);
	}

	for (i = 0; i < sw_bundle64_init->count; i++) {
		struct netlist_av64 *l = sw_bundle64_init->l;

		g->sim->esc_readl(g, "GRCTX_SW_BUNDLE64_INIT:ADDR",
				i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_SW_BUNDLE64_INIT:VALUE_LO",
				i, &l[i].value_lo);
		g->sim->esc_readl(g, "GRCTX_SW_BUNDLE64_INIT:VALUE_HI",
				i, &l[i].value_hi);
	}

#ifdef CONFIG_NVGPU_DEBUGGER
	for (i = 0; i < sys_ctxsw_regs->count; i++) {
		struct netlist_aiv *l = sys_ctxsw_regs->l;
		g->sim->esc_readl(g, "GRCTX_REG_LIST_SYS:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_SYS:INDEX",
				    i, &l[i].index);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_SYS:VALUE",
				    i, &l[i].value);
	}

	for (i = 0; i < gpc_ctxsw_regs->count; i++) {
		struct netlist_aiv *l = gpc_ctxsw_regs->l;
		g->sim->esc_readl(g, "GRCTX_REG_LIST_GPC:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_GPC:INDEX",
				    i, &l[i].index);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_GPC:VALUE",
				    i, &l[i].value);
	}

	for (i = 0; i < tpc_ctxsw_regs->count; i++) {
		struct netlist_aiv *l = tpc_ctxsw_regs->l;
		g->sim->esc_readl(g, "GRCTX_REG_LIST_TPC:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_TPC:INDEX",
				    i, &l[i].index);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_TPC:VALUE",
				    i, &l[i].value);
	}

	for (i = 0; i < ppc_ctxsw_regs->count; i++) {
		struct netlist_aiv *l = ppc_ctxsw_regs->l;
		g->sim->esc_readl(g, "GRCTX_REG_LIST_PPC:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_PPC:INDEX",
				    i, &l[i].index);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_PPC:VALUE",
				    i, &l[i].value);
	}

#ifdef CONFIG_NVGPU_GRAPHICS
	for (i = 0; i < zcull_gpc_ctxsw_regs->count; i++) {
		struct netlist_aiv *l = zcull_gpc_ctxsw_regs->l;
		g->sim->esc_readl(g, "GRCTX_REG_LIST_ZCULL_GPC:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_ZCULL_GPC:INDEX",
				    i, &l[i].index);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_ZCULL_GPC:VALUE",
				    i, &l[i].value);
	}
#endif

	for (i = 0; i < pm_sys_ctxsw_regs->count; i++) {
		struct netlist_aiv *l = pm_sys_ctxsw_regs->l;
		g->sim->esc_readl(g, "GRCTX_REG_LIST_PM_SYS:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_PM_SYS:INDEX",
				    i, &l[i].index);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_PM_SYS:VALUE",
				    i, &l[i].value);
	}

	for (i = 0; i < pm_gpc_ctxsw_regs->count; i++) {
		struct netlist_aiv *l = pm_gpc_ctxsw_regs->l;
		g->sim->esc_readl(g, "GRCTX_REG_LIST_PM_GPC:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_PM_GPC:INDEX",
				    i, &l[i].index);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_PM_GPC:VALUE",
				    i, &l[i].value);
	}

	for (i = 0; i < pm_tpc_ctxsw_regs->count; i++) {
		struct netlist_aiv *l = pm_tpc_ctxsw_regs->l;
		g->sim->esc_readl(g, "GRCTX_REG_LIST_PM_TPC:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_PM_TPC:INDEX",
				    i, &l[i].index);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_PM_TPC:VALUE",
				    i, &l[i].value);
	}

	nvgpu_log(g, gpu_dbg_info | gpu_dbg_fn, "query GRCTX_REG_LIST_ETPC");
	for (i = 0; i < etpc_ctxsw_regs->count; i++) {
		struct netlist_aiv *l = etpc_ctxsw_regs->l;
		g->sim->esc_readl(g, "GRCTX_REG_LIST_ETPC:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_ETPC:INDEX",
				    i, &l[i].index);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_ETPC:VALUE",
				    i, &l[i].value);
		nvgpu_log(g, gpu_dbg_info | gpu_dbg_fn,
				"addr:0x%#08x index:0x%08x value:0x%08x",
				l[i].addr, l[i].index, l[i].value);
	}
#endif /* CONFIG_NVGPU_DEBUGGER */

	g->netlist_valid = true;

	g->sim->esc_readl(g, "GRCTX_GEN_CTX_REGS_BASE_INDEX", 0,
			    &regs_base_index);
	nvgpu_netlist_vars_set_regs_base_index(g, regs_base_index);

	nvgpu_log(g, gpu_dbg_info | gpu_dbg_fn, "finished querying grctx info from chiplib");
	return 0;
fail:
	nvgpu_err(g, "failed querying grctx info from chiplib");

	nvgpu_kfree(g, fecs_inst->l);
	nvgpu_kfree(g, fecs_data->l);
	nvgpu_kfree(g, gpccs_inst->l);
	nvgpu_kfree(g, gpccs_data->l);
	nvgpu_kfree(g, sw_bundle_init->l);
	nvgpu_kfree(g, sw_bundle64_init->l);
	nvgpu_kfree(g, sw_method_init->l);
	nvgpu_kfree(g, sw_ctx_load->l);
	nvgpu_kfree(g, sw_non_ctx_load->l);
#if defined(CONFIG_NVGPU_NON_FUSA) && defined(CONFIG_NVGPU_NEXT)
	nvgpu_next_init_sim_netlist_ctx_vars_free(g);
#endif
	nvgpu_kfree(g, sw_veid_bundle_init->l);
#ifdef CONFIG_NVGPU_DEBUGGER
	nvgpu_kfree(g, sys_ctxsw_regs->l);
	nvgpu_kfree(g, gpc_ctxsw_regs->l);
	nvgpu_kfree(g, tpc_ctxsw_regs->l);
#ifdef CONFIG_NVGPU_GRAPHICS
	nvgpu_kfree(g, zcull_gpc_ctxsw_regs->l);
#endif
	nvgpu_kfree(g, ppc_ctxsw_regs->l);
	nvgpu_kfree(g, pm_sys_ctxsw_regs->l);
	nvgpu_kfree(g, pm_gpc_ctxsw_regs->l);
	nvgpu_kfree(g, pm_tpc_ctxsw_regs->l);
	nvgpu_kfree(g, etpc_ctxsw_regs->l);
#endif /* CONFIG_NVGPU_DEBUGGER */

	return err;
}
