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

#ifndef NVGPU_GR_FALCON_H
#define NVGPU_GR_FALCON_H

#include <nvgpu/types.h>

struct gk20a;
struct nvgpu_gr_falcon;

struct nvgpu_ctxsw_ucode_segment {
	u32 offset;
	u32 size;
};

struct nvgpu_ctxsw_ucode_segments {
	u32 boot_entry;
	u32 boot_imem_offset;
	u32 boot_signature;
	struct nvgpu_ctxsw_ucode_segment boot;
	struct nvgpu_ctxsw_ucode_segment code;
	struct nvgpu_ctxsw_ucode_segment data;
};

#define NVGPU_GR_FALCON_METHOD_CTXSW_STOP 0
#define NVGPU_GR_FALCON_METHOD_CTXSW_START 1
#define NVGPU_GR_FALCON_METHOD_HALT_PIPELINE 2
#define NVGPU_GR_FALCON_METHOD_FECS_TRACE_FLUSH 3
#define NVGPU_GR_FALCON_METHOD_CTXSW_DISCOVER_IMAGE_SIZE 4
#define NVGPU_GR_FALCON_METHOD_CTXSW_DISCOVER_ZCULL_IMAGE_SIZE 5
#define NVGPU_GR_FALCON_METHOD_CTXSW_DISCOVER_PM_IMAGE_SIZE 6
#define NVGPU_GR_FALCON_METHOD_REGLIST_DISCOVER_IMAGE_SIZE 7
#define NVGPU_GR_FALCON_METHOD_REGLIST_BIND_INSTANCE 8
#define NVGPU_GR_FALCON_METHOD_REGLIST_SET_VIRTUAL_ADDRESS 9
#define NVGPU_GR_FALCON_METHOD_ADDRESS_BIND_PTR 10
#define NVGPU_GR_FALCON_METHOD_GOLDEN_IMAGE_SAVE 11
#define NVGPU_GR_FALCON_METHOD_PREEMPT_IMAGE_SIZE 12
#define NVGPU_GR_FALCON_METHOD_CONFIGURE_CTXSW_INTR 13

#define NVGPU_GR_FALCON_FECS_CTXSW_MAILBOX0 0U
#define NVGPU_GR_FALCON_FECS_CTXSW_MAILBOX1 1U
#define NVGPU_GR_FALCON_FECS_CTXSW_MAILBOX2 2U
#define NVGPU_GR_FALCON_FECS_CTXSW_MAILBOX4 4U
#define NVGPU_GR_FALCON_FECS_CTXSW_MAILBOX6 6U
#define NVGPU_GR_FALCON_FECS_CTXSW_MAILBOX7 7U

struct nvgpu_fecs_host_intr_status {
	u32 ctxsw_intr0;
	u32 ctxsw_intr1;
	bool fault_during_ctxsw_active;
	bool unimp_fw_method_active;
	bool watchdog_active;
};

struct nvgpu_fecs_ecc_status {
	bool imem_corrected_err;
	bool imem_uncorrected_err;
	bool dmem_corrected_err;
	bool dmem_uncorrected_err;
	u32  ecc_addr;
	u32  corrected_delta;
	u32  uncorrected_delta;
};

struct nvgpu_gr_falcon *nvgpu_gr_falcon_init_support(struct gk20a *g);
void nvgpu_gr_falcon_remove_support(struct gk20a *g,
				struct nvgpu_gr_falcon *falcon);
int nvgpu_gr_falcon_bind_fecs_elpg(struct gk20a *g);
int nvgpu_gr_falcon_init_ctxsw(struct gk20a *g, struct nvgpu_gr_falcon *falcon);
int nvgpu_gr_falcon_init_ctx_state(struct gk20a *g,
		struct nvgpu_gr_falcon *falcon);
int nvgpu_gr_falcon_init_ctxsw_ucode(struct gk20a *g,
					struct nvgpu_gr_falcon *falcon);
int nvgpu_gr_falcon_load_ctxsw_ucode(struct gk20a *g,
					struct nvgpu_gr_falcon *falcon);
int nvgpu_gr_falcon_load_secure_ctxsw_ucode(struct gk20a *g,
					struct nvgpu_gr_falcon *falcon);

struct nvgpu_mutex *nvgpu_gr_falcon_get_fecs_mutex(
					struct nvgpu_gr_falcon *falcon);
struct nvgpu_ctxsw_ucode_segments *nvgpu_gr_falcon_get_fecs_ucode_segments(
					struct nvgpu_gr_falcon *falcon);
struct nvgpu_ctxsw_ucode_segments *nvgpu_gr_falcon_get_gpccs_ucode_segments(
					struct nvgpu_gr_falcon *falcon);
void *nvgpu_gr_falcon_get_surface_desc_cpu_va(
					struct nvgpu_gr_falcon *falcon);

u32 nvgpu_gr_falcon_get_golden_image_size(struct nvgpu_gr_falcon *falcon);
u32 nvgpu_gr_falcon_get_pm_ctxsw_image_size(struct nvgpu_gr_falcon *falcon);
u32 nvgpu_gr_falcon_get_preempt_image_size(struct nvgpu_gr_falcon *falcon);

#ifdef NVGPU_GRAPHICS
u32 nvgpu_gr_falcon_get_zcull_image_size(struct nvgpu_gr_falcon *falcon);
#endif

#endif /* NVGPU_GR_FALCON_H */
