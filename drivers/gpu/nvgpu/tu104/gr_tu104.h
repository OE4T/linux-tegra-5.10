/*
 * Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_GR_TU104_H
#define NVGPU_GR_TU104_H

#include <nvgpu/types.h>

struct gk20a;
struct nvgpu_preemption_modes_rec;

enum {
	TURING_CHANNEL_GPFIFO_A	= 0xC46F,
	TURING_A		= 0xC597,
	TURING_COMPUTE_A	= 0xC5C0,
	TURING_DMA_COPY_A	= 0xC5B5,
};

#define NVC5C0_SET_SHADER_EXCEPTIONS            0x1528
#define NVC5C0_SET_SKEDCHECK                    0x23c
#define NVC5C0_SET_SHADER_CUT_COLLECTOR         0x254

#define NVC5C0_SET_SM_DISP_CTRL                             0x250
#define NVC5C0_SET_SM_DISP_CTRL_COMPUTE_SHADER_QUAD_MASK    0x1
#define NVC5C0_SET_SM_DISP_CTRL_COMPUTE_SHADER_QUAD_DISABLE 0
#define NVC5C0_SET_SM_DISP_CTRL_COMPUTE_SHADER_QUAD_ENABLE  1

#define NVC597_SET_SHADER_EXCEPTIONS            0x1528
#define NVC597_SET_CIRCULAR_BUFFER_SIZE         0x1280
#define NVC597_SET_ALPHA_CIRCULAR_BUFFER_SIZE   0x02dc
#define NVC597_SET_GO_IDLE_TIMEOUT              0x022c
#define NVC597_SET_TEX_IN_DBG                   0x10bc
#define NVC597_SET_SKEDCHECK                    0x10c0
#define NVC597_SET_BES_CROP_DEBUG3              0x10c4
#define NVC597_SET_BES_CROP_DEBUG4              0x10b0
#define NVC597_SET_SM_DISP_CTRL                 0x10c8
#define NVC597_SET_SHADER_CUT_COLLECTOR         0x10d0

/* TODO: merge these into global context buffer list in gr_gk20a.h */
#define RTV_CIRCULAR_BUFFER		8
#define RTV_CIRCULAR_BUFFER_VA		5

bool gr_tu104_is_valid_class(struct gk20a *g, u32 class_num);
bool gr_tu104_is_valid_gfx_class(struct gk20a *g, u32 class_num);
bool gr_tu104_is_valid_compute_class(struct gk20a *g, u32 class_num);

int gr_tu104_init_sw_bundle64(struct gk20a *g);

void gr_tu10x_create_sysfs(struct gk20a *g);
void gr_tu10x_remove_sysfs(struct gk20a *g);

int gr_tu104_alloc_global_ctx_buffers(struct gk20a *g);
int gr_tu104_map_global_ctx_buffers(struct gk20a *g, struct vm_gk20a *vm,
		struct nvgpu_gr_ctx *gr_ctx, bool vpr);
int gr_tu104_commit_global_ctx_buffers(struct gk20a *g,
			struct nvgpu_gr_ctx *gr_ctx, bool patch);

void gr_tu104_bundle_cb_defaults(struct gk20a *g);
void gr_tu104_cb_size_default(struct gk20a *g);

int gr_tu104_get_preemption_mode_flags(struct gk20a *g,
	struct nvgpu_preemption_modes_rec *preemption_modes_rec);
void gr_tu104_enable_gpc_exceptions(struct gk20a *g);

int gr_tu104_get_offset_in_gpccs_segment(struct gk20a *g,
	enum ctxsw_addr_type addr_type, u32 num_tpcs, u32 num_ppcs,
	u32 reg_list_ppc_count, u32 *__offset_in_segment);

int gr_tu104_handle_sw_method(struct gk20a *g, u32 addr,
			      u32 class_num, u32 offset, u32 data);

void gr_tu104_init_sm_dsm_reg_info(void);
void gr_tu104_get_sm_dsm_perf_ctrl_regs(struct gk20a *g,
	u32 *num_sm_dsm_perf_ctrl_regs, u32 **sm_dsm_perf_ctrl_regs,
	u32 *ctrl_register_stride);
#endif /* NVGPU_GR_TU104_H */
