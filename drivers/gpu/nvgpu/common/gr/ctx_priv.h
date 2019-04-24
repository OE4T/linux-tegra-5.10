/*
 * Copyright (c) 2019, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_GR_CTX_PRIV_H
#define NVGPU_GR_CTX_PRIV_H

struct nvgpu_mem;

struct patch_desc {
	struct nvgpu_mem mem;
	u32 data_count;
};

struct zcull_ctx_desc {
	u64 gpu_va;
	u32 ctx_sw_mode;
};

struct pm_ctx_desc {
	struct nvgpu_mem mem;
	u64 gpu_va;
	u32 pm_mode;
};

struct nvgpu_gr_ctx_desc {
	u32 size[NVGPU_GR_CTX_COUNT];

	bool force_preemption_gfxp;
	bool force_preemption_cilp;

	bool dump_ctxsw_stats_on_channel_close;
};

struct nvgpu_gr_ctx {
	u32 ctx_id;
	bool ctx_id_valid;
	struct nvgpu_mem mem;

	struct nvgpu_mem preempt_ctxsw_buffer;
	struct nvgpu_mem spill_ctxsw_buffer;
	struct nvgpu_mem betacb_ctxsw_buffer;
	struct nvgpu_mem pagepool_ctxsw_buffer;
	struct nvgpu_mem gfxp_rtvcb_ctxsw_buffer;

	struct patch_desc	patch_ctx;
	struct zcull_ctx_desc	zcull_ctx;
	struct pm_ctx_desc	pm_ctx;

	u32 graphics_preempt_mode;
	u32 compute_preempt_mode;

	bool golden_img_loaded;
	bool cilp_preempt_pending;
	bool boosted_ctx;

#ifdef CONFIG_TEGRA_GR_VIRTUALIZATION
	u64 virt_ctx;
#endif

	u64	global_ctx_buffer_va[NVGPU_GR_CTX_VA_COUNT];
	u32	global_ctx_buffer_index[NVGPU_GR_CTX_VA_COUNT];
	bool	global_ctx_buffer_mapped;

	u32 tsgid;
};

#endif /* NVGPU_GR_CTX_PRIV_H */
