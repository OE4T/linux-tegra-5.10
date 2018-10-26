/*
 * Copyright (c) 2011-2018, NVIDIA CORPORATION.  All rights reserved.
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

int nvgpu_netlist_init_ctx_vars_sim(struct gk20a *g)
{
	struct nvgpu_netlist_vars *netlist_vars = g->netlist_vars;
	int err = -ENOMEM;
	u32 i, temp;

	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_info,
		   "querying grctx info from chiplib");

	netlist_vars->dynamic = true;

	if (g->sim->esc_readl == NULL) {
		nvgpu_err(g, "Invalid pointer to query function.");
		err = -ENOENT;
		goto fail;
	}

	/* query sizes and counts */
	g->sim->esc_readl(g, "GRCTX_UCODE_INST_FECS_COUNT", 0,
			    &netlist_vars->ucode.fecs.inst.count);
	g->sim->esc_readl(g, "GRCTX_UCODE_DATA_FECS_COUNT", 0,
			    &netlist_vars->ucode.fecs.data.count);
	g->sim->esc_readl(g, "GRCTX_UCODE_INST_GPCCS_COUNT", 0,
			    &netlist_vars->ucode.gpccs.inst.count);
	g->sim->esc_readl(g, "GRCTX_UCODE_DATA_GPCCS_COUNT", 0,
			    &netlist_vars->ucode.gpccs.data.count);
	g->sim->esc_readl(g, "GRCTX_ALL_CTX_TOTAL_WORDS", 0, &temp);
	netlist_vars->buffer_size = temp << 2;
	g->sim->esc_readl(g, "GRCTX_SW_BUNDLE_INIT_SIZE", 0,
			    &netlist_vars->sw_bundle_init.count);
	g->sim->esc_readl(g, "GRCTX_SW_METHOD_INIT_SIZE", 0,
			    &netlist_vars->sw_method_init.count);
	g->sim->esc_readl(g, "GRCTX_SW_CTX_LOAD_SIZE", 0,
			    &netlist_vars->sw_ctx_load.count);
	g->sim->esc_readl(g, "GRCTX_SW_VEID_BUNDLE_INIT_SIZE", 0,
			    &netlist_vars->sw_veid_bundle_init.count);
	g->sim->esc_readl(g, "GRCTX_SW_BUNDLE64_INIT_SIZE", 0,
			    &netlist_vars->sw_bundle64_init.count);

	g->sim->esc_readl(g, "GRCTX_NONCTXSW_REG_SIZE", 0,
			    &netlist_vars->sw_non_ctx_load.count);
	g->sim->esc_readl(g, "GRCTX_REG_LIST_SYS_COUNT", 0,
			    &netlist_vars->ctxsw_regs.sys.count);
	g->sim->esc_readl(g, "GRCTX_REG_LIST_GPC_COUNT", 0,
			    &netlist_vars->ctxsw_regs.gpc.count);
	g->sim->esc_readl(g, "GRCTX_REG_LIST_TPC_COUNT", 0,
			    &netlist_vars->ctxsw_regs.tpc.count);
	g->sim->esc_readl(g, "GRCTX_REG_LIST_ZCULL_GPC_COUNT", 0,
			    &netlist_vars->ctxsw_regs.zcull_gpc.count);
	g->sim->esc_readl(g, "GRCTX_REG_LIST_PM_SYS_COUNT", 0,
			    &netlist_vars->ctxsw_regs.pm_sys.count);
	g->sim->esc_readl(g, "GRCTX_REG_LIST_PM_GPC_COUNT", 0,
			    &netlist_vars->ctxsw_regs.pm_gpc.count);
	g->sim->esc_readl(g, "GRCTX_REG_LIST_PM_TPC_COUNT", 0,
			    &netlist_vars->ctxsw_regs.pm_tpc.count);
	g->sim->esc_readl(g, "GRCTX_REG_LIST_PPC_COUNT", 0,
			    &netlist_vars->ctxsw_regs.ppc.count);
	g->sim->esc_readl(g, "GRCTX_REG_LIST_ETPC_COUNT", 0,
			    &netlist_vars->ctxsw_regs.etpc.count);
	g->sim->esc_readl(g, "GRCTX_REG_LIST_PPC_COUNT", 0,
			    &netlist_vars->ctxsw_regs.ppc.count);

	if (nvgpu_netlist_alloc_u32_list(g, &netlist_vars->ucode.fecs.inst) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_u32_list(g, &netlist_vars->ucode.fecs.data) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_u32_list(g, &netlist_vars->ucode.gpccs.inst) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_u32_list(g, &netlist_vars->ucode.gpccs.data) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_av_list(g, &netlist_vars->sw_bundle_init) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_av64_list(g,
			&netlist_vars->sw_bundle64_init) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_av_list(g, &netlist_vars->sw_method_init) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_aiv_list(g, &netlist_vars->sw_ctx_load) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_av_list(g, &netlist_vars->sw_non_ctx_load) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_av_list(g,
			&netlist_vars->sw_veid_bundle_init) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_aiv_list(g, &netlist_vars->ctxsw_regs.sys) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_aiv_list(g, &netlist_vars->ctxsw_regs.gpc) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_aiv_list(g, &netlist_vars->ctxsw_regs.tpc) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_aiv_list(g,
			&netlist_vars->ctxsw_regs.zcull_gpc) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_aiv_list(g, &netlist_vars->ctxsw_regs.ppc) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_aiv_list(g,
			&netlist_vars->ctxsw_regs.pm_sys) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_aiv_list(g,
			&netlist_vars->ctxsw_regs.pm_gpc) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_aiv_list(g,
			&netlist_vars->ctxsw_regs.pm_tpc) == NULL) {
		goto fail;
	}
	if (nvgpu_netlist_alloc_aiv_list(g, &netlist_vars->ctxsw_regs.etpc) == NULL) {
		goto fail;
	}

	for (i = 0; i < netlist_vars->ucode.fecs.inst.count; i++) {
		g->sim->esc_readl(g, "GRCTX_UCODE_INST_FECS",
				    i, &netlist_vars->ucode.fecs.inst.l[i]);
	}

	for (i = 0; i < netlist_vars->ucode.fecs.data.count; i++) {
		g->sim->esc_readl(g, "GRCTX_UCODE_DATA_FECS",
				    i, &netlist_vars->ucode.fecs.data.l[i]);
	}

	for (i = 0; i < netlist_vars->ucode.gpccs.inst.count; i++) {
		g->sim->esc_readl(g, "GRCTX_UCODE_INST_GPCCS",
				    i, &netlist_vars->ucode.gpccs.inst.l[i]);
	}

	for (i = 0; i < netlist_vars->ucode.gpccs.data.count; i++) {
		g->sim->esc_readl(g, "GRCTX_UCODE_DATA_GPCCS",
				    i, &netlist_vars->ucode.gpccs.data.l[i]);
	}

	for (i = 0; i < netlist_vars->sw_bundle_init.count; i++) {
		struct netlist_av *l = netlist_vars->sw_bundle_init.l;
		g->sim->esc_readl(g, "GRCTX_SW_BUNDLE_INIT:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_SW_BUNDLE_INIT:VALUE",
				    i, &l[i].value);
	}

	for (i = 0; i < netlist_vars->sw_method_init.count; i++) {
		struct netlist_av *l = netlist_vars->sw_method_init.l;
		g->sim->esc_readl(g, "GRCTX_SW_METHOD_INIT:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_SW_METHOD_INIT:VALUE",
				    i, &l[i].value);
	}

	for (i = 0; i < netlist_vars->sw_ctx_load.count; i++) {
		struct netlist_aiv *l = netlist_vars->sw_ctx_load.l;
		g->sim->esc_readl(g, "GRCTX_SW_CTX_LOAD:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_SW_CTX_LOAD:INDEX",
				    i, &l[i].index);
		g->sim->esc_readl(g, "GRCTX_SW_CTX_LOAD:VALUE",
				    i, &l[i].value);
	}

	for (i = 0; i < netlist_vars->sw_non_ctx_load.count; i++) {
		struct netlist_av *l = netlist_vars->sw_non_ctx_load.l;
		g->sim->esc_readl(g, "GRCTX_NONCTXSW_REG:REG",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_NONCTXSW_REG:VALUE",
				    i, &l[i].value);
	}

	for (i = 0; i < netlist_vars->sw_veid_bundle_init.count; i++) {
		struct netlist_av *l = netlist_vars->sw_veid_bundle_init.l;

		g->sim->esc_readl(g, "GRCTX_SW_VEID_BUNDLE_INIT:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_SW_VEID_BUNDLE_INIT:VALUE",
				    i, &l[i].value);
	}

	for (i = 0; i < netlist_vars->sw_bundle64_init.count; i++) {
		struct netlist_av64 *l = netlist_vars->sw_bundle64_init.l;

		g->sim->esc_readl(g, "GRCTX_SW_BUNDLE64_INIT:ADDR",
				i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_SW_BUNDLE64_INIT:VALUE_LO",
				i, &l[i].value_lo);
		g->sim->esc_readl(g, "GRCTX_SW_BUNDLE64_INIT:VALUE_HI",
				i, &l[i].value_hi);
	}

	for (i = 0; i < netlist_vars->ctxsw_regs.sys.count; i++) {
		struct netlist_aiv *l = netlist_vars->ctxsw_regs.sys.l;
		g->sim->esc_readl(g, "GRCTX_REG_LIST_SYS:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_SYS:INDEX",
				    i, &l[i].index);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_SYS:VALUE",
				    i, &l[i].value);
	}

	for (i = 0; i < netlist_vars->ctxsw_regs.gpc.count; i++) {
		struct netlist_aiv *l = netlist_vars->ctxsw_regs.gpc.l;
		g->sim->esc_readl(g, "GRCTX_REG_LIST_GPC:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_GPC:INDEX",
				    i, &l[i].index);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_GPC:VALUE",
				    i, &l[i].value);
	}

	for (i = 0; i < netlist_vars->ctxsw_regs.tpc.count; i++) {
		struct netlist_aiv *l = netlist_vars->ctxsw_regs.tpc.l;
		g->sim->esc_readl(g, "GRCTX_REG_LIST_TPC:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_TPC:INDEX",
				    i, &l[i].index);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_TPC:VALUE",
				    i, &l[i].value);
	}

	for (i = 0; i < netlist_vars->ctxsw_regs.ppc.count; i++) {
		struct netlist_aiv *l = netlist_vars->ctxsw_regs.ppc.l;
		g->sim->esc_readl(g, "GRCTX_REG_LIST_PPC:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_PPC:INDEX",
				    i, &l[i].index);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_PPC:VALUE",
				    i, &l[i].value);
	}

	for (i = 0; i < netlist_vars->ctxsw_regs.zcull_gpc.count; i++) {
		struct netlist_aiv *l = netlist_vars->ctxsw_regs.zcull_gpc.l;
		g->sim->esc_readl(g, "GRCTX_REG_LIST_ZCULL_GPC:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_ZCULL_GPC:INDEX",
				    i, &l[i].index);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_ZCULL_GPC:VALUE",
				    i, &l[i].value);
	}

	for (i = 0; i < netlist_vars->ctxsw_regs.pm_sys.count; i++) {
		struct netlist_aiv *l = netlist_vars->ctxsw_regs.pm_sys.l;
		g->sim->esc_readl(g, "GRCTX_REG_LIST_PM_SYS:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_PM_SYS:INDEX",
				    i, &l[i].index);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_PM_SYS:VALUE",
				    i, &l[i].value);
	}

	for (i = 0; i < netlist_vars->ctxsw_regs.pm_gpc.count; i++) {
		struct netlist_aiv *l = netlist_vars->ctxsw_regs.pm_gpc.l;
		g->sim->esc_readl(g, "GRCTX_REG_LIST_PM_GPC:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_PM_GPC:INDEX",
				    i, &l[i].index);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_PM_GPC:VALUE",
				    i, &l[i].value);
	}

	for (i = 0; i < netlist_vars->ctxsw_regs.pm_tpc.count; i++) {
		struct netlist_aiv *l = netlist_vars->ctxsw_regs.pm_tpc.l;
		g->sim->esc_readl(g, "GRCTX_REG_LIST_PM_TPC:ADDR",
				    i, &l[i].addr);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_PM_TPC:INDEX",
				    i, &l[i].index);
		g->sim->esc_readl(g, "GRCTX_REG_LIST_PM_TPC:VALUE",
				    i, &l[i].value);
	}

	nvgpu_log(g, gpu_dbg_info | gpu_dbg_fn, "query GRCTX_REG_LIST_ETPC");
	for (i = 0; i < netlist_vars->ctxsw_regs.etpc.count; i++) {
		struct netlist_aiv *l = netlist_vars->ctxsw_regs.etpc.l;
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

	g->netlist_valid = true;

	g->sim->esc_readl(g, "GRCTX_GEN_CTX_REGS_BASE_INDEX", 0,
			    &netlist_vars->regs_base_index);

	nvgpu_log(g, gpu_dbg_info | gpu_dbg_fn, "finished querying grctx info from chiplib");
	return 0;
fail:
	nvgpu_err(g, "failed querying grctx info from chiplib");

	nvgpu_kfree(g, netlist_vars->ucode.fecs.inst.l);
	nvgpu_kfree(g, netlist_vars->ucode.fecs.data.l);
	nvgpu_kfree(g, netlist_vars->ucode.gpccs.inst.l);
	nvgpu_kfree(g, netlist_vars->ucode.gpccs.data.l);
	nvgpu_kfree(g, netlist_vars->sw_bundle_init.l);
	nvgpu_kfree(g, netlist_vars->sw_bundle64_init.l);
	nvgpu_kfree(g, netlist_vars->sw_method_init.l);
	nvgpu_kfree(g, netlist_vars->sw_ctx_load.l);
	nvgpu_kfree(g, netlist_vars->sw_non_ctx_load.l);
	nvgpu_kfree(g, netlist_vars->sw_veid_bundle_init.l);
	nvgpu_kfree(g, netlist_vars->ctxsw_regs.sys.l);
	nvgpu_kfree(g, netlist_vars->ctxsw_regs.gpc.l);
	nvgpu_kfree(g, netlist_vars->ctxsw_regs.tpc.l);
	nvgpu_kfree(g, netlist_vars->ctxsw_regs.zcull_gpc.l);
	nvgpu_kfree(g, netlist_vars->ctxsw_regs.ppc.l);
	nvgpu_kfree(g, netlist_vars->ctxsw_regs.pm_sys.l);
	nvgpu_kfree(g, netlist_vars->ctxsw_regs.pm_gpc.l);
	nvgpu_kfree(g, netlist_vars->ctxsw_regs.pm_tpc.l);
	nvgpu_kfree(g, netlist_vars->ctxsw_regs.etpc.l);

	return err;
}
