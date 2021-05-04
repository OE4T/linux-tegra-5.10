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

#ifndef NVGPU_NEXT_NETLIST_PRIV_H
#define NVGPU_NEXT_NETLIST_PRIV_H

/**
 * @file
 *
 * Declare netlist_vars specific struct and defines.
 */
#include <nvgpu/types.h>

struct gk20a;
struct netlist_av_list;
struct netlist_aiv_list;

#ifdef CONFIG_NVGPU_DEBUGGER
#define NETLIST_REGIONID_CTXREG_SYS_COMPUTE		36
#define NETLIST_REGIONID_CTXREG_GPC_COMPUTE		38
#define NETLIST_REGIONID_CTXREG_TPC_COMPUTE		40
#define NETLIST_REGIONID_CTXREG_PPC_COMPUTE		42
#define NETLIST_REGIONID_CTXREG_ETPC_COMPUTE		44
#ifdef CONFIG_NVGPU_GRAPHICS
#define NETLIST_REGIONID_CTXREG_SYS_GFX			37
#define NETLIST_REGIONID_CTXREG_GPC_GFX			39
#define NETLIST_REGIONID_CTXREG_TPC_GFX			41
#define NETLIST_REGIONID_CTXREG_PPC_GFX			43
#define NETLIST_REGIONID_CTXREG_ETPC_GFX		45
#endif  /* CONFIG_NVGPU_GRAPHICS */
#endif /* CONFIG_NVGPU_DEBUGGER */

#define NETLIST_REGIONID_SW_NON_CTX_LOCAL_COMPUTE_LOAD	48
#define NETLIST_REGIONID_SW_NON_CTX_GLOBAL_COMPUTE_LOAD	50
#ifdef CONFIG_NVGPU_GRAPHICS
#define NETLIST_REGIONID_SW_NON_CTX_LOCAL_GFX_LOAD	49
#define NETLIST_REGIONID_SW_NON_CTX_GLOBAL_GFX_LOAD	51
#endif  /* CONFIG_NVGPU_GRAPHICS */

#ifdef CONFIG_NVGPU_DEBUGGER
#define NETLIST_REGIONID_CTXREG_LTS_BC			57
#define NETLIST_REGIONID_CTXREG_LTS_UC			58
#endif /* CONFIG_DEBUGGER */

struct nvgpu_next_netlist_vars {
	struct netlist_av_list  sw_non_ctx_local_compute_load;
	struct netlist_av_list  sw_non_ctx_global_compute_load;
#ifdef CONFIG_NVGPU_GRAPHICS
	struct netlist_av_list  sw_non_ctx_local_gfx_load;
	struct netlist_av_list  sw_non_ctx_global_gfx_load;
#endif  /* CONFIG_NVGPU_GRAPHICS */
};

#ifdef CONFIG_NVGPU_DEBUGGER
struct nvgpu_next_ctxsw_regs {
	struct netlist_aiv_list sys_compute;
	struct netlist_aiv_list gpc_compute;
	struct netlist_aiv_list tpc_compute;
	struct netlist_aiv_list ppc_compute;
	struct netlist_aiv_list etpc_compute;
	struct netlist_aiv_list lts_bc;
	struct netlist_aiv_list lts_uc;
#ifdef CONFIG_NVGPU_GRAPHICS
	struct netlist_aiv_list sys_gfx;
	struct netlist_aiv_list gpc_gfx;
	struct netlist_aiv_list tpc_gfx;
	struct netlist_aiv_list ppc_gfx;
	struct netlist_aiv_list etpc_gfx;
#endif /* CONFIG_NVGPU_GRAPHICS */
};
#endif /* CONFIG_NVGPU_DEBUGGER */

#endif /* NVGPU_NEXT_NETLIST_PRIV_H */
