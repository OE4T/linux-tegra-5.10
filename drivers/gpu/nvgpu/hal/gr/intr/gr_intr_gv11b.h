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

#ifndef NVGPU_GR_INTR_GV11B_H
#define NVGPU_GR_INTR_GV11B_H

#include <nvgpu/types.h>

struct gk20a;
struct nvgpu_gr_config;
struct nvgpu_channel;
struct nvgpu_gr_isr_data;

#define NVC397_SET_SHADER_EXCEPTIONS		0x1528U
#define NVC397_SET_CIRCULAR_BUFFER_SIZE 	0x1280U
#define NVC397_SET_ALPHA_CIRCULAR_BUFFER_SIZE 	0x02dcU
#define NVC397_SET_GO_IDLE_TIMEOUT 		0x022cU
#define NVC397_SET_TEX_IN_DBG			0x10bcU
#define NVC397_SET_SKEDCHECK			0x10c0U
#define NVC397_SET_BES_CROP_DEBUG3		0x10c4U
#define NVC397_SET_BES_CROP_DEBUG4		0x10b0U
#define NVC397_SET_SHADER_CUT_COLLECTOR		0x10c8U

#define NVC397_SET_TEX_IN_DBG_TSL1_RVCH_INVALIDATE		BIT32(0)
#define NVC397_SET_TEX_IN_DBG_SM_L1TAG_CTRL_CACHE_SURFACE_LD	BIT32(1)
#define NVC397_SET_TEX_IN_DBG_SM_L1TAG_CTRL_CACHE_SURFACE_ST	BIT32(2)

#define NVC397_SET_SKEDCHECK_18_MASK				0x3U
#define NVC397_SET_SKEDCHECK_18_DEFAULT				0x0U
#define NVC397_SET_SKEDCHECK_18_DISABLE				0x1U
#define NVC397_SET_SKEDCHECK_18_ENABLE				0x2U

#define NVC397_SET_SHADER_CUT_COLLECTOR_STATE_DISABLE		0x0U
#define NVC397_SET_SHADER_CUT_COLLECTOR_STATE_ENABLE		0x1U

#define NVC3C0_SET_SKEDCHECK			0x23cU
#define NVC3C0_SET_SHADER_CUT_COLLECTOR		0x250U

#define NVA297_SET_SHADER_EXCEPTIONS_ENABLE_FALSE	U32(0)

int gv11b_gr_intr_handle_fecs_error(struct gk20a *g,
				struct nvgpu_channel *ch_ptr,
				struct nvgpu_gr_isr_data *isr_data);
void gv11b_gr_intr_set_shader_cut_collector(struct gk20a *g, u32 data);
void gv11b_gr_intr_set_skedcheck(struct gk20a *g, u32 data);
void gv11b_gr_intr_set_tex_in_dbg(struct gk20a *g, u32 data);
int gv11b_gr_intr_handle_sw_method(struct gk20a *g, u32 addr,
				     u32 class_num, u32 offset, u32 data);
void gv11b_gr_intr_set_shader_exceptions(struct gk20a *g, u32 data);
void gv11b_gr_intr_handle_gcc_exception(struct gk20a *g, u32 gpc,
			u32 tpc, u32 gpc_exception,
			u32 *corrected_err, u32 *uncorrected_err);
void gv11b_gr_intr_handle_gpc_gpcmmu_exception(struct gk20a *g, u32 gpc,
		u32 gpc_exception, u32 *corrected_err, u32 *uncorrected_err);
void gv11b_gr_intr_handle_gpc_gpccs_exception(struct gk20a *g, u32 gpc,
		u32 gpc_exception, u32 *corrected_err, u32 *uncorrected_err);
void gv11b_gr_intr_handle_tpc_mpc_exception(struct gk20a *g, u32 gpc, u32 tpc);
void gv11b_gr_intr_enable_hww_exceptions(struct gk20a *g);
void gv11b_gr_intr_enable_exceptions(struct gk20a *g,
				     struct nvgpu_gr_config *gr_config,
				     bool enable);
void gv11b_gr_intr_enable_gpc_exceptions(struct gk20a *g,
					 struct nvgpu_gr_config *gr_config);

#endif /* NVGPU_GR_INTR_GV11B_H */
