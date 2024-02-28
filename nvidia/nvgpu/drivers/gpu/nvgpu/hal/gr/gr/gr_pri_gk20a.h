/*
 * GK20A Graphics Context Pri Register Addressing
 *
 * Copyright (c) 2014-2020, NVIDIA CORPORATION.  All rights reserved.
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
#ifndef GR_PRI_GK20A_H
#define GR_PRI_GK20A_H

#ifdef CONFIG_NVGPU_DEBUGGER

/*
 * These convenience macros are generally for use in the management/modificaiton
 * of the context state store for gr/compute contexts.
 */

/*
 * GPC pri addressing
 * For "from where?" refer *_PGRAPH_Memory_Map.xlsx
 */
static inline u32 pri_gpccs_addr_width(struct gk20a *g)
{
	return nvgpu_get_litter_value(g, GPU_LIT_GPC_ADDR_WIDTH);
}
static inline u32 pri_gpccs_addr_mask(struct gk20a *g, u32 addr)
{
	return addr & (BIT32(pri_gpccs_addr_width(g)) - 1U);
}
static inline u32 pri_gpc_addr(struct gk20a *g, u32 addr, u32 gpc)
{
	u32 gpc_base = nvgpu_get_litter_value(g, GPU_LIT_GPC_BASE);
	u32 gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_GPC_STRIDE);
	return gpc_base + (gpc * gpc_stride) + addr;
}
static inline bool pri_is_gpc_addr_shared(struct gk20a *g, u32 addr)
{
	u32 gpc_shared_base = nvgpu_get_litter_value(g, GPU_LIT_GPC_SHARED_BASE);
	u32 gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_GPC_STRIDE);
	return (addr >= gpc_shared_base) &&
		(addr < gpc_shared_base + gpc_stride);
}
static inline bool pri_is_gpc_addr(struct gk20a *g, u32 addr)
{
	u32 gpc_base = nvgpu_get_litter_value(g, GPU_LIT_GPC_BASE);
	u32 gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_GPC_STRIDE);
	u32 num_gpcs = nvgpu_get_litter_value(g, GPU_LIT_NUM_GPCS);
	return	((addr >= gpc_base) &&
		 (addr < gpc_base + num_gpcs * gpc_stride)) ||
		pri_is_gpc_addr_shared(g, addr);
}
static inline u32 pri_get_gpc_num(struct gk20a *g, u32 addr)
{
	u32 i, start;
	u32 num_gpcs = nvgpu_get_litter_value(g, GPU_LIT_NUM_GPCS);
	u32 gpc_base = nvgpu_get_litter_value(g, GPU_LIT_GPC_BASE);
	u32 gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_GPC_STRIDE);
	for (i = 0; i < num_gpcs; i++) {
		start = gpc_base + (i * gpc_stride);
		if ((addr >= start) && (addr < (start + gpc_stride))) {
			return i;
		}
	}
	return 0;
}

/*
 * PPC pri addressing
 */
static inline bool pri_is_ppc_addr_shared(struct gk20a *g, u32 addr)
{
	u32 ppc_in_gpc_shared_base = nvgpu_get_litter_value(g,
						GPU_LIT_PPC_IN_GPC_SHARED_BASE);
	u32 ppc_in_gpc_stride = nvgpu_get_litter_value(g,
						GPU_LIT_PPC_IN_GPC_STRIDE);

	return ((addr >= ppc_in_gpc_shared_base) &&
		(addr < (ppc_in_gpc_shared_base + ppc_in_gpc_stride)));
}

static inline bool pri_is_ppc_addr(struct gk20a *g, u32 addr)
{
	u32 ppc_in_gpc_base = nvgpu_get_litter_value(g,
						GPU_LIT_PPC_IN_GPC_BASE);
	u32 num_pes_per_gpc = nvgpu_get_litter_value(g,
						GPU_LIT_NUM_PES_PER_GPC);
	u32 ppc_in_gpc_stride = nvgpu_get_litter_value(g,
						GPU_LIT_PPC_IN_GPC_STRIDE);

	return ((addr >= ppc_in_gpc_base) &&
		(addr < ppc_in_gpc_base + num_pes_per_gpc * ppc_in_gpc_stride))
		|| pri_is_ppc_addr_shared(g, addr);
}

/*
 * TPC pri addressing
 */
static inline u32 pri_tpccs_addr_width(struct gk20a *g)
{
	return nvgpu_get_litter_value(g, GPU_LIT_TPC_ADDR_WIDTH);
}
static inline u32 pri_tpccs_addr_mask(struct gk20a *g, u32 addr)
{
	return addr & (BIT32(pri_tpccs_addr_width(g)) - 1U);
}
static inline u32 pri_fbpa_addr_mask(struct gk20a *g, u32 addr)
{
	return addr & (nvgpu_get_litter_value(g, GPU_LIT_FBPA_STRIDE) - 1U);
}
static inline u32 pri_tpc_addr(struct gk20a *g, u32 addr, u32 gpc, u32 tpc)
{
	u32 gpc_base = nvgpu_get_litter_value(g, GPU_LIT_GPC_BASE);
	u32 gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_GPC_STRIDE);
	u32 tpc_in_gpc_base = nvgpu_get_litter_value(g, GPU_LIT_TPC_IN_GPC_BASE);
	u32 tpc_in_gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_TPC_IN_GPC_STRIDE);
	return gpc_base + (gpc * gpc_stride) +
		tpc_in_gpc_base + (tpc * tpc_in_gpc_stride) +
		addr;
}
static inline bool pri_is_tpc_addr_shared(struct gk20a *g, u32 addr)
{
	u32 tpc_in_gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_TPC_IN_GPC_STRIDE);
	u32 tpc_in_gpc_shared_base = nvgpu_get_litter_value(g, GPU_LIT_TPC_IN_GPC_SHARED_BASE);
	return (addr >= tpc_in_gpc_shared_base) &&
		(addr < (tpc_in_gpc_shared_base +
			 tpc_in_gpc_stride));
}
static inline u32 pri_fbpa_addr(struct gk20a *g, u32 addr, u32 fbpa)
{
	return (nvgpu_get_litter_value(g, GPU_LIT_FBPA_BASE) + addr +
			(fbpa * nvgpu_get_litter_value(g, GPU_LIT_FBPA_STRIDE)));
}
static inline bool pri_is_fbpa_addr_shared(struct gk20a *g, u32 addr)
{
	u32 fbpa_shared_base = nvgpu_get_litter_value(g, GPU_LIT_FBPA_SHARED_BASE);
	u32 fbpa_stride = nvgpu_get_litter_value(g, GPU_LIT_FBPA_STRIDE);
	return ((addr >= fbpa_shared_base) &&
		(addr < (fbpa_shared_base + fbpa_stride)));
}
static inline bool pri_is_fbpa_addr(struct gk20a *g, u32 addr)
{
	u32 fbpa_base = nvgpu_get_litter_value(g, GPU_LIT_FBPA_BASE);
	u32 fbpa_stride = nvgpu_get_litter_value(g, GPU_LIT_FBPA_STRIDE);
	u32 num_fbpas = nvgpu_get_litter_value(g, GPU_LIT_NUM_FBPAS);
	return (((addr >= fbpa_base) &&
		(addr < (fbpa_base + num_fbpas * fbpa_stride)))
		|| pri_is_fbpa_addr_shared(g, addr));
}
/*
 * BE pri addressing
 */
static inline u32 pri_becs_addr_width(void)
{
	return 10U;/* from where? */
}
static inline u32 pri_becs_addr_mask(u32 addr)
{
	return addr & (BIT32(pri_becs_addr_width()) - 1U);
}
static inline bool pri_is_rop_addr_shared(struct gk20a *g, u32 addr)
{
	u32 rop_shared_base = nvgpu_get_litter_value(g, GPU_LIT_ROP_SHARED_BASE);
	u32 rop_stride = nvgpu_get_litter_value(g, GPU_LIT_ROP_STRIDE);
	return (addr >= rop_shared_base) &&
		(addr < rop_shared_base + rop_stride);
}
static inline u32 pri_rop_shared_addr(struct gk20a *g, u32 addr)
{
	u32 rop_shared_base = nvgpu_get_litter_value(g, GPU_LIT_ROP_SHARED_BASE);
	return rop_shared_base + pri_becs_addr_mask(addr);
}
static inline bool pri_is_rop_addr(struct gk20a *g, u32 addr)
{
	u32 rop_base = nvgpu_get_litter_value(g, GPU_LIT_ROP_BASE);
	u32 rop_stride = nvgpu_get_litter_value(g, GPU_LIT_ROP_STRIDE);
	return	((addr >= rop_base) &&
		 (addr < rop_base + nvgpu_ltc_get_ltc_count(g) * rop_stride)) ||
		pri_is_rop_addr_shared(g, addr);
}

static inline u32 pri_get_rop_num(struct gk20a *g, u32 addr)
{
	u32 i, start;
	u32 num_fbps = nvgpu_get_litter_value(g, GPU_LIT_NUM_FBPS);
	u32 rop_base = nvgpu_get_litter_value(g, GPU_LIT_ROP_BASE);
	u32 rop_stride = nvgpu_get_litter_value(g, GPU_LIT_ROP_STRIDE);
	for (i = 0; i < num_fbps; i++) {
		start = rop_base + (i * rop_stride);
		if ((addr >= start) && (addr < (start + rop_stride))) {
			return i;
		}
	}
	return 0;
}

/*
 * PPC pri addressing
 */
static inline u32 pri_ppccs_addr_width(void)
{
	return 9U; /* from where? */
}
static inline u32 pri_ppccs_addr_mask(u32 addr)
{
	return addr & (BIT32(pri_ppccs_addr_width()) - 1U);
}
static inline u32 pri_ppc_addr(struct gk20a *g, u32 addr, u32 gpc, u32 ppc)
{
	u32 gpc_base = nvgpu_get_litter_value(g, GPU_LIT_GPC_BASE);
	u32 gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_GPC_STRIDE);
	u32 ppc_in_gpc_base = nvgpu_get_litter_value(g, GPU_LIT_PPC_IN_GPC_BASE);
	u32 ppc_in_gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_PPC_IN_GPC_STRIDE);
	return gpc_base + (gpc * gpc_stride) +
		ppc_in_gpc_base + (ppc * ppc_in_gpc_stride) + addr;
}

enum ctxsw_addr_type {
	CTXSW_ADDR_TYPE_SYS  = 0,
	CTXSW_ADDR_TYPE_GPC  = 1,
	CTXSW_ADDR_TYPE_TPC  = 2,
	CTXSW_ADDR_TYPE_ROP   = 3,
	CTXSW_ADDR_TYPE_PPC  = 4,
	CTXSW_ADDR_TYPE_LTCS = 5,
	CTXSW_ADDR_TYPE_FBPA = 6,
	CTXSW_ADDR_TYPE_EGPC = 7,
	CTXSW_ADDR_TYPE_ETPC = 8,
	CTXSW_ADDR_TYPE_PMM_FBPGS_ROP  = 9,
	CTXSW_ADDR_TYPE_FBP  = 10,
	CTXSW_ADDR_TYPE_LTS_MAIN  = 11,
};

#define PRI_BROADCAST_FLAGS_NONE		0U
#define PRI_BROADCAST_FLAGS_GPC			BIT32(0)
#define PRI_BROADCAST_FLAGS_TPC			BIT32(1)
#define PRI_BROADCAST_FLAGS_ROP			BIT32(2)
#define PRI_BROADCAST_FLAGS_PPC			BIT32(3)
#define PRI_BROADCAST_FLAGS_LTCS		BIT32(4)
#define PRI_BROADCAST_FLAGS_LTSS		BIT32(5)
#define PRI_BROADCAST_FLAGS_FBPA		BIT32(6)
#define PRI_BROADCAST_FLAGS_EGPC		BIT32(7)
#define PRI_BROADCAST_FLAGS_ETPC		BIT32(8)
#define PRI_BROADCAST_FLAGS_PMMGPC		BIT32(9)
#define PRI_BROADCAST_FLAGS_PMM_GPCS		BIT32(10)
#define PRI_BROADCAST_FLAGS_PMM_GPCGS_GPCTPCA	BIT32(11)
#define PRI_BROADCAST_FLAGS_PMM_GPCGS_GPCTPCB	BIT32(12)
#define PRI_BROADCAST_FLAGS_PMMFBP		BIT32(13)
#define PRI_BROADCAST_FLAGS_PMM_FBPS		BIT32(14)
#define PRI_BROADCAST_FLAGS_PMM_FBPGS_LTC	BIT32(15)
#define PRI_BROADCAST_FLAGS_PMM_FBPGS_ROP	BIT32(16)
#define PRI_BROADCAST_FLAGS_SM			BIT32(17)

#endif /* CONFIG_NVGPU_DEBUGGER */
#endif /* GR_PRI_GK20A_H */
