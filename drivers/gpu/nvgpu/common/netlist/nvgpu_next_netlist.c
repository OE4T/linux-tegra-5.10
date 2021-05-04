/*
 * Copyright (c) 2020-2021, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/string.h>
#include <nvgpu/netlist.h>

#include "common/netlist/netlist_priv.h"

/* Copied from common/netlist/netlist.c */
static int nvgpu_netlist_alloc_load_av_list(struct gk20a *g, u8 *src, u32 len,
			struct netlist_av_list *av_list)
{
	av_list->count = len / U32(sizeof(struct netlist_av));
	if (nvgpu_netlist_alloc_av_list(g, av_list) == NULL) {
		return -ENOMEM;
	}

	nvgpu_memcpy((u8 *)av_list->l, src, len);

	return 0;
}

/* Copied from common/netlist/netlist.c */
static int nvgpu_netlist_alloc_load_aiv_list(struct gk20a *g, u8 *src, u32 len,
			struct netlist_aiv_list *aiv_list)
{
	aiv_list->count = len / U32(sizeof(struct netlist_aiv));
	if (nvgpu_netlist_alloc_aiv_list(g, aiv_list) == NULL) {
		return -ENOMEM;
	}

	nvgpu_memcpy((u8 *)aiv_list->l, src, len);

	return 0;
}

#ifdef CONFIG_NVGPU_DEBUGGER
bool nvgpu_next_netlist_handle_debugger_region_id(struct gk20a *g,
			u32 region_id, u8 *src, u32 size,
			struct nvgpu_netlist_vars *netlist_vars, int *err_code)
{
	int err = 0;
	bool handled = true;

	switch (region_id) {
	case NETLIST_REGIONID_CTXREG_SYS_COMPUTE:
		nvgpu_log_info(g, "NETLIST_REGIONID_CTXREG_SYS_COMPUTE");
		err = nvgpu_netlist_alloc_load_aiv_list(g, src, size,
			&netlist_vars->ctxsw_regs.nvgpu_next.sys_compute);
		break;
	case NETLIST_REGIONID_CTXREG_GPC_COMPUTE:
		nvgpu_log_info(g, "NETLIST_REGIONID_CTXREG_GPC_COMPUTE");
		err = nvgpu_netlist_alloc_load_aiv_list(g, src, size,
			&netlist_vars->ctxsw_regs.nvgpu_next.gpc_compute);
		break;
	case NETLIST_REGIONID_CTXREG_TPC_COMPUTE:
		nvgpu_log_info(g, "NETLIST_REGIONID_CTXREG_TPC_COMPUTE");
		err = nvgpu_netlist_alloc_load_aiv_list(g, src, size,
			&netlist_vars->ctxsw_regs.nvgpu_next.tpc_compute);
		break;
	case NETLIST_REGIONID_CTXREG_PPC_COMPUTE:
		nvgpu_log_info(g, "NETLIST_REGIONID_CTXREG_PPC_COMPUTE");
		err = nvgpu_netlist_alloc_load_aiv_list(g, src, size,
			&netlist_vars->ctxsw_regs.nvgpu_next.ppc_compute);
		break;
	case NETLIST_REGIONID_CTXREG_ETPC_COMPUTE:
		nvgpu_log_info(g, "NETLIST_REGIONID_CTXREG_ETPC_COMPUTE");
		err = nvgpu_netlist_alloc_load_aiv_list(g, src, size,
			&netlist_vars->ctxsw_regs.nvgpu_next.etpc_compute);
		break;
	case NETLIST_REGIONID_CTXREG_LTS_BC:
		nvgpu_log_info(g, "NETLIST_REGIONID_CTXREG_LTS_BC");
		err = nvgpu_netlist_alloc_load_aiv_list(g, src, size,
			&netlist_vars->ctxsw_regs.nvgpu_next.lts_bc);
		break;
	case NETLIST_REGIONID_CTXREG_LTS_UC:
		nvgpu_log_info(g, "NETLIST_REGIONID_CTXREG_LTS_UC");
		err = nvgpu_netlist_alloc_load_aiv_list(g, src, size,
			&netlist_vars->ctxsw_regs.nvgpu_next.lts_uc);
		break;
	default:
		handled = false;
		break;
	}

	if ((handled == false) && (!nvgpu_is_enabled(g, NVGPU_SUPPORT_MIG))) {
		handled = true;
		switch (region_id) {
#ifdef CONFIG_NVGPU_GRAPHICS
		case NETLIST_REGIONID_CTXREG_SYS_GFX:
			nvgpu_log_info(g, "NETLIST_REGIONID_CTXREG_SYS_GFX");
			err = nvgpu_netlist_alloc_load_aiv_list(g, src, size,
				&netlist_vars->ctxsw_regs.nvgpu_next.sys_gfx);
			break;
		case NETLIST_REGIONID_CTXREG_GPC_GFX:
			nvgpu_log_info(g, "NETLIST_REGIONID_CTXREG_GPC_GFX");
			err = nvgpu_netlist_alloc_load_aiv_list(g, src, size,
				&netlist_vars->ctxsw_regs.nvgpu_next.gpc_gfx);
			break;
		case NETLIST_REGIONID_CTXREG_TPC_GFX:
			nvgpu_log_info(g, "NETLIST_REGIONID_CTXREG_TPC_GFX");
			err = nvgpu_netlist_alloc_load_aiv_list(g, src, size,
				&netlist_vars->ctxsw_regs.nvgpu_next.tpc_gfx);
			break;
		case NETLIST_REGIONID_CTXREG_PPC_GFX:
			nvgpu_log_info(g, "NETLIST_REGIONID_CTXREG_PPC_GFX");
			err = nvgpu_netlist_alloc_load_aiv_list(g, src, size,
				&netlist_vars->ctxsw_regs.nvgpu_next.ppc_gfx);
			break;
		case NETLIST_REGIONID_CTXREG_ETPC_GFX:
			nvgpu_log_info(g, "NETLIST_REGIONID_CTXREG_ETPC_GFX");
			err = nvgpu_netlist_alloc_load_aiv_list(g, src, size,
				&netlist_vars->ctxsw_regs.nvgpu_next.etpc_gfx);
			break;
#endif
		default:
			handled = false;
			break;
		}
	}
	*err_code = err;

	return handled;
}

void nvgpu_next_netlist_deinit_ctxsw_regs(struct gk20a *g)
{
	struct nvgpu_netlist_vars *netlist_vars = g->netlist_vars;

	nvgpu_kfree(g, netlist_vars->ctxsw_regs.nvgpu_next.sys_compute.l);
	nvgpu_kfree(g, netlist_vars->ctxsw_regs.nvgpu_next.gpc_compute.l);
	nvgpu_kfree(g, netlist_vars->ctxsw_regs.nvgpu_next.tpc_compute.l);
	nvgpu_kfree(g, netlist_vars->ctxsw_regs.nvgpu_next.ppc_compute.l);
	nvgpu_kfree(g, netlist_vars->ctxsw_regs.nvgpu_next.etpc_compute.l);
	nvgpu_kfree(g, netlist_vars->ctxsw_regs.nvgpu_next.lts_bc.l);
	nvgpu_kfree(g, netlist_vars->ctxsw_regs.nvgpu_next.lts_uc.l);
	nvgpu_kfree(g, netlist_vars->ctxsw_regs.nvgpu_next.sys_gfx.l);
	nvgpu_kfree(g, netlist_vars->ctxsw_regs.nvgpu_next.gpc_gfx.l);
	nvgpu_kfree(g, netlist_vars->ctxsw_regs.nvgpu_next.tpc_gfx.l);
	nvgpu_kfree(g, netlist_vars->ctxsw_regs.nvgpu_next.ppc_gfx.l);
	nvgpu_kfree(g, netlist_vars->ctxsw_regs.nvgpu_next.etpc_gfx.l);
}
#endif /* CONFIG_NVGPU_DEBUGGER */

bool nvgpu_next_netlist_handle_sw_bundles_region_id(struct gk20a *g,
			u32 region_id, u8 *src, u32 size,
			struct nvgpu_netlist_vars *netlist_vars, int *err_code)
{
	int err = 0;
	bool handled = true;

	switch(region_id) {
	case NETLIST_REGIONID_SW_NON_CTX_LOCAL_COMPUTE_LOAD:
		nvgpu_log_info(g, "NETLIST_REGIONID_SW_NON_CTX_LOCAL_COMPUTE_LOAD");
		err = nvgpu_netlist_alloc_load_av_list(g, src, size,
			&netlist_vars->nvgpu_next.sw_non_ctx_local_compute_load);
		break;
	case NETLIST_REGIONID_SW_NON_CTX_GLOBAL_COMPUTE_LOAD:
		nvgpu_log_info(g, "NETLIST_REGIONID_SW_NON_CTX_GLOBAL_COMPUTE_LOAD");
		err = nvgpu_netlist_alloc_load_av_list(g, src, size,
			&netlist_vars->nvgpu_next.sw_non_ctx_global_compute_load);
		break;
	default:
		handled = false;
		break;
	}

	if ((handled == false) && (!nvgpu_is_enabled(g, NVGPU_SUPPORT_MIG))) {
		handled = true;
		switch (region_id) {
#ifdef CONFIG_NVGPU_GRAPHICS
		case NETLIST_REGIONID_SW_NON_CTX_LOCAL_GFX_LOAD:
			nvgpu_log_info(g, "NETLIST_REGIONID_SW_NON_CTX_LOCAL_GFX_LOAD");
			err = nvgpu_netlist_alloc_load_av_list(g, src, size,
				&netlist_vars->nvgpu_next.sw_non_ctx_local_gfx_load);
			break;
		case NETLIST_REGIONID_SW_NON_CTX_GLOBAL_GFX_LOAD:
			nvgpu_log_info(g, "NETLIST_REGIONID_SW_NON_CTX_GLOBAL_GFX_LOAD");
			err = nvgpu_netlist_alloc_load_av_list(g, src, size,
				&netlist_vars->nvgpu_next.sw_non_ctx_global_gfx_load);
			break;
#endif
		default:
			handled = false;
			break;
		}
	}
	*err_code = err;

	return handled;
}

void nvgpu_next_netlist_deinit_ctx_vars(struct gk20a *g)
{
	struct nvgpu_netlist_vars *netlist_vars = g->netlist_vars;

	nvgpu_kfree(g, netlist_vars->nvgpu_next.sw_non_ctx_local_compute_load.l);
	nvgpu_kfree(g, netlist_vars->nvgpu_next.sw_non_ctx_global_compute_load.l);
#ifdef CONFIG_NVGPU_GRAPHICS
	nvgpu_kfree(g, netlist_vars->nvgpu_next.sw_non_ctx_local_gfx_load.l);
	nvgpu_kfree(g, netlist_vars->nvgpu_next.sw_non_ctx_global_gfx_load.l);
#endif
}

#ifdef CONFIG_NVGPU_DEBUGGER
struct netlist_aiv_list *nvgpu_next_netlist_get_sys_compute_ctxsw_regs(
							struct gk20a *g)
{
	return &g->netlist_vars->ctxsw_regs.nvgpu_next.sys_compute;
}

struct netlist_aiv_list *nvgpu_next_netlist_get_gpc_compute_ctxsw_regs(
							struct gk20a *g)
{
	return &g->netlist_vars->ctxsw_regs.nvgpu_next.gpc_compute;
}

struct netlist_aiv_list *nvgpu_next_netlist_get_tpc_compute_ctxsw_regs(
							struct gk20a *g)
{
	return &g->netlist_vars->ctxsw_regs.nvgpu_next.tpc_compute;
}

struct netlist_aiv_list *nvgpu_next_netlist_get_ppc_compute_ctxsw_regs(
							struct gk20a *g)
{
	return &g->netlist_vars->ctxsw_regs.nvgpu_next.ppc_compute;
}

struct netlist_aiv_list *nvgpu_next_netlist_get_etpc_compute_ctxsw_regs(
							struct gk20a *g)
{
	return &g->netlist_vars->ctxsw_regs.nvgpu_next.etpc_compute;
}

struct netlist_aiv_list *nvgpu_next_netlist_get_lts_ctxsw_regs(
							struct gk20a *g)
{
	return &g->netlist_vars->ctxsw_regs.nvgpu_next.lts_bc;
}

struct netlist_aiv_list *nvgpu_next_netlist_get_sys_gfx_ctxsw_regs(
							struct gk20a *g)
{
	return &g->netlist_vars->ctxsw_regs.nvgpu_next.sys_gfx;
}

struct netlist_aiv_list *nvgpu_next_netlist_get_gpc_gfx_ctxsw_regs(
							struct gk20a *g)
{
	return &g->netlist_vars->ctxsw_regs.nvgpu_next.gpc_gfx;
}

struct netlist_aiv_list *nvgpu_next_netlist_get_tpc_gfx_ctxsw_regs(
							struct gk20a *g)
{
	return &g->netlist_vars->ctxsw_regs.nvgpu_next.tpc_gfx;
}

struct netlist_aiv_list *nvgpu_next_netlist_get_ppc_gfx_ctxsw_regs(
							struct gk20a *g)
{
	return &g->netlist_vars->ctxsw_regs.nvgpu_next.ppc_gfx;
}

struct netlist_aiv_list *nvgpu_next_netlist_get_etpc_gfx_ctxsw_regs(
							struct gk20a *g)
{
	return &g->netlist_vars->ctxsw_regs.nvgpu_next.etpc_gfx;
}

u32 nvgpu_next_netlist_get_sys_ctxsw_regs_count(struct gk20a *g)
{
	u32 count = nvgpu_next_netlist_get_sys_compute_ctxsw_regs(g)->count;

	count = nvgpu_safe_add_u32(count,
		nvgpu_next_netlist_get_sys_gfx_ctxsw_regs(g)->count);
	return count;
}

u32 nvgpu_next_netlist_get_ppc_ctxsw_regs_count(struct gk20a *g)
{
	u32 count = nvgpu_next_netlist_get_ppc_compute_ctxsw_regs(g)->count;

	count = nvgpu_safe_add_u32(count,
		nvgpu_next_netlist_get_ppc_gfx_ctxsw_regs(g)->count);
	return count;
}

u32 nvgpu_next_netlist_get_gpc_ctxsw_regs_count(struct gk20a *g)
{
	u32 count = nvgpu_next_netlist_get_gpc_compute_ctxsw_regs(g)->count;

	count = nvgpu_safe_add_u32(count,
		nvgpu_next_netlist_get_gpc_gfx_ctxsw_regs(g)->count);
	return count;
}

u32 nvgpu_next_netlist_get_tpc_ctxsw_regs_count(struct gk20a *g)
{
	u32 count = nvgpu_next_netlist_get_tpc_compute_ctxsw_regs(g)->count;

	count = nvgpu_safe_add_u32(count,
		nvgpu_next_netlist_get_tpc_gfx_ctxsw_regs(g)->count);
	return count;
}

u32 nvgpu_next_netlist_get_etpc_ctxsw_regs_count(struct gk20a *g)
{
	u32 count = nvgpu_next_netlist_get_etpc_compute_ctxsw_regs(g)->count;

	count = nvgpu_safe_add_u32(count,
		nvgpu_next_netlist_get_etpc_gfx_ctxsw_regs(g)->count);
	return count;
}

void nvgpu_next_netlist_print_ctxsw_reg_info(struct gk20a *g)
{
	nvgpu_log_info(g, "GRCTX_REG_LIST_SYS_(COMPUTE/GRAPICS)_COUNT   :%d %d",
		nvgpu_next_netlist_get_sys_compute_ctxsw_regs(g)->count,
		nvgpu_next_netlist_get_sys_gfx_ctxsw_regs(g)->count);
	nvgpu_log_info(g, "GRCTX_REG_LIST_GPC_(COMPUTE/GRAPHICS)_COUNT  :%d %d",
		nvgpu_next_netlist_get_gpc_compute_ctxsw_regs(g)->count,
		nvgpu_next_netlist_get_gpc_gfx_ctxsw_regs(g)->count);
	nvgpu_log_info(g, "GRCTX_REG_LIST_TPC_(COMPUTE/GRAPHICS)_COUNT  :%d %d",
		nvgpu_next_netlist_get_tpc_compute_ctxsw_regs(g)->count,
		nvgpu_next_netlist_get_tpc_gfx_ctxsw_regs(g)->count);
	nvgpu_log_info(g, "GRCTX_REG_LIST_PPC_(COMPUTE/GRAHPICS)_COUNT  :%d %d",
		nvgpu_next_netlist_get_ppc_compute_ctxsw_regs(g)->count,
		nvgpu_next_netlist_get_ppc_gfx_ctxsw_regs(g)->count);
	nvgpu_log_info(g, "GRCTX_REG_LIST_ETPC_(COMPUTE/GRAPHICS)_COUNT :%d %d",
		nvgpu_next_netlist_get_etpc_compute_ctxsw_regs(g)->count,
		nvgpu_next_netlist_get_etpc_gfx_ctxsw_regs(g)->count);
	nvgpu_log_info(g, "GRCTX_REG_LIST_LTS_BC_COUNT                  :%d",
		nvgpu_next_netlist_get_lts_ctxsw_regs(g)->count);
}
#endif /* CONFIG_NVGPU_DEBUGGER */

struct netlist_av_list *nvgpu_next_netlist_get_sw_non_ctx_local_compute_load_av_list(
							struct gk20a *g)
{
	return &g->netlist_vars->nvgpu_next.sw_non_ctx_local_compute_load;
}

struct netlist_av_list *nvgpu_next_netlist_get_sw_non_ctx_global_compute_load_av_list(
							struct gk20a *g)
{
	return &g->netlist_vars->nvgpu_next.sw_non_ctx_global_compute_load;
}

#ifdef CONFIG_NVGPU_GRAPHICS
struct netlist_av_list *nvgpu_next_netlist_get_sw_non_ctx_local_gfx_load_av_list(
							struct gk20a *g)
{
	return &g->netlist_vars->nvgpu_next.sw_non_ctx_local_gfx_load;
}

struct netlist_av_list *nvgpu_next_netlist_get_sw_non_ctx_global_gfx_load_av_list(
							struct gk20a *g)
{
	return &g->netlist_vars->nvgpu_next.sw_non_ctx_global_gfx_load;
}
#endif /* CONFIG_NVGPU_GRAPHICS */
