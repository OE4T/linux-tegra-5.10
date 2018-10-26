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
#ifndef NVGPU_NETLIST_H
#define NVGPU_NETLIST_H

struct gk20a;

/* emulation netlists, match majorV with HW */
#define NVGPU_NETLIST_IMAGE_A	"NETA_img.bin"
#define NVGPU_NETLIST_IMAGE_B	"NETB_img.bin"
#define NVGPU_NETLIST_IMAGE_C	"NETC_img.bin"
#define NVGPU_NETLIST_IMAGE_D	"NETD_img.bin"

/*
 * Need to support multiple ARCH in same GPU family
 * then need to provide path like ARCH/NETIMAGE to
 * point to correct netimage within GPU family,
 * Example, gm20x can support gm204 or gm206,so path
 * for netimage is gm204/NETC_img.bin, and '/' char
 * will inserted at null terminator char of "GAxxx"
 * to get complete path like gm204/NETC_img.bin
 */
#define GPU_ARCH "GAxxx"

union __max_name {
#ifdef NVGPU_NETLIST_IMAGE_A
	char __name_a[sizeof(NVGPU_NETLIST_IMAGE_A)];
#endif
#ifdef NVGPU_NETLIST_IMAGE_B
	char __name_b[sizeof(NVGPU_NETLIST_IMAGE_B)];
#endif
#ifdef NVGPU_NETLIST_IMAGE_C
	char __name_c[sizeof(NVGPU_NETLIST_IMAGE_C)];
#endif
#ifdef NVGPU_NETLIST_IMAGE_D
	char __name_d[sizeof(NVGPU_NETLIST_IMAGE_D)];
#endif
};

#define MAX_NETLIST_NAME (sizeof(GPU_ARCH) + sizeof(union __max_name))

/* index for emulation netlists */
#define NETLIST_FINAL		-1
#define NETLIST_SLOT_A		0
#define NETLIST_SLOT_B		1
#define NETLIST_SLOT_C		2
#define NETLIST_SLOT_D		3
#define MAX_NETLIST		4

/* netlist regions */
#define NETLIST_REGIONID_FECS_UCODE_DATA	0
#define NETLIST_REGIONID_FECS_UCODE_INST	1
#define NETLIST_REGIONID_GPCCS_UCODE_DATA	2
#define NETLIST_REGIONID_GPCCS_UCODE_INST	3
#define NETLIST_REGIONID_SW_BUNDLE_INIT		4
#define NETLIST_REGIONID_SW_CTX_LOAD		5
#define NETLIST_REGIONID_SW_NON_CTX_LOAD	6
#define NETLIST_REGIONID_SW_METHOD_INIT		7
#define NETLIST_REGIONID_CTXREG_SYS		8
#define NETLIST_REGIONID_CTXREG_GPC		9
#define NETLIST_REGIONID_CTXREG_TPC		10
#define NETLIST_REGIONID_CTXREG_ZCULL_GPC	11
#define NETLIST_REGIONID_CTXREG_PM_SYS		12
#define NETLIST_REGIONID_CTXREG_PM_GPC		13
#define NETLIST_REGIONID_CTXREG_PM_TPC		14
#define NETLIST_REGIONID_MAJORV			15
#define NETLIST_REGIONID_BUFFER_SIZE		16
#define NETLIST_REGIONID_CTXSW_REG_BASE_INDEX	17
#define NETLIST_REGIONID_NETLIST_NUM		18
#define NETLIST_REGIONID_CTXREG_PPC		19
#define NETLIST_REGIONID_CTXREG_PMPPC		20
#define NETLIST_REGIONID_NVPERF_CTXREG_SYS	21
#define NETLIST_REGIONID_NVPERF_FBP_CTXREGS	22
#define NETLIST_REGIONID_NVPERF_CTXREG_GPC	23
#define NETLIST_REGIONID_NVPERF_FBP_ROUTER	24
#define NETLIST_REGIONID_NVPERF_GPC_ROUTER	25
#define NETLIST_REGIONID_CTXREG_PMLTC		26
#define NETLIST_REGIONID_CTXREG_PMFBPA		27
#define NETLIST_REGIONID_SWVEIDBUNDLEINIT       28
#define NETLIST_REGIONID_NVPERF_SYS_ROUTER      29
#define NETLIST_REGIONID_NVPERF_PMA             30
#define NETLIST_REGIONID_CTXREG_PMROP           31
#define NETLIST_REGIONID_CTXREG_PMUCGPC         32
#define NETLIST_REGIONID_CTXREG_ETPC            33
#define NETLIST_REGIONID_SW_BUNDLE64_INIT	34
#define NETLIST_REGIONID_NVPERF_PMCAU		35

struct netlist_region {
	u32 region_id;
	u32 data_size;
	u32 data_offset;
};

struct netlist_image_header {
	u32 version;
	u32 regions;
};

struct netlist_image {
	struct netlist_image_header header;
	struct netlist_region regions[1];
};

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

struct netlist_gr_ucode {
	struct {
		struct netlist_u32_list inst;
		struct netlist_u32_list data;
	} gpccs, fecs;
};

struct nvgpu_netlist_vars {
	bool dynamic;

	u32 regs_base_index;
	u32 buffer_size;

	struct netlist_gr_ucode ucode;

	struct netlist_av_list  sw_bundle_init;
	struct netlist_av64_list sw_bundle64_init;
	struct netlist_av_list  sw_method_init;
	struct netlist_aiv_list sw_ctx_load;
	struct netlist_av_list  sw_non_ctx_load;
	struct netlist_av_list  sw_veid_bundle_init;
	struct {
		struct netlist_aiv_list sys;
		struct netlist_aiv_list gpc;
		struct netlist_aiv_list tpc;
		struct netlist_aiv_list zcull_gpc;
		struct netlist_aiv_list ppc;
		struct netlist_aiv_list pm_sys;
		struct netlist_aiv_list pm_gpc;
		struct netlist_aiv_list pm_tpc;
		struct netlist_aiv_list pm_ppc;
		struct netlist_aiv_list perf_sys;
		struct netlist_aiv_list perf_gpc;
		struct netlist_aiv_list fbp;
		struct netlist_aiv_list fbp_router;
		struct netlist_aiv_list gpc_router;
		struct netlist_aiv_list pm_ltc;
		struct netlist_aiv_list pm_fbpa;
		struct netlist_aiv_list perf_sys_router;
		struct netlist_aiv_list perf_pma;
		struct netlist_aiv_list pm_rop;
		struct netlist_aiv_list pm_ucgpc;
		struct netlist_aiv_list etpc;
		struct netlist_aiv_list pm_cau;
	} ctxsw_regs;
};

int nvgpu_netlist_init_ctx_vars(struct gk20a *g);
int nvgpu_netlist_init_ctx_vars_sim(struct gk20a *g);
void nvgpu_netlist_deinit_ctx_vars(struct gk20a *g);

#endif /* NVGPU_NETLIST_H */
