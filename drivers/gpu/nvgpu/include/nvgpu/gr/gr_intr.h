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

#ifndef NVGPU_GR_INTR_H
#define NVGPU_GR_INTR_H

#include <nvgpu/types.h>

struct gk20a;
struct nvgpu_channel;
struct nvgpu_gr_intr_info;
struct nvgpu_gr_tpc_exception;
struct nvgpu_gr_isr_data;
struct nvgpu_gr_intr;

int nvgpu_gr_intr_handle_fecs_error(struct gk20a *g, struct nvgpu_channel *ch,
					struct nvgpu_gr_isr_data *isr_data);
int nvgpu_gr_intr_handle_gpc_exception(struct gk20a *g, bool *post_event,
	struct nvgpu_gr_config *gr_config, struct nvgpu_channel *fault_ch,
	u32 *hww_global_esr);
void nvgpu_gr_intr_handle_notify_pending(struct gk20a *g,
					struct nvgpu_gr_isr_data *isr_data);
void nvgpu_gr_intr_handle_semaphore_pending(struct gk20a *g,
					   struct nvgpu_gr_isr_data *isr_data);
void nvgpu_gr_intr_report_exception(struct gk20a *g, u32 inst,
				u32 err_type, u32 status);
struct nvgpu_channel *nvgpu_gr_intr_get_channel_from_ctx(struct gk20a *g,
				u32 curr_ctx, u32 *curr_tsgid);
void nvgpu_gr_intr_set_error_notifier(struct gk20a *g,
		  struct nvgpu_gr_isr_data *isr_data, u32 error_notifier);
int nvgpu_gr_intr_handle_sm_exception(struct gk20a *g, u32 gpc, u32 tpc, u32 sm,
		bool *post_event, struct nvgpu_channel *fault_ch,
		u32 *hww_global_esr);
int nvgpu_gr_intr_stall_isr(struct gk20a *g);

void nvgpu_gr_intr_flush_channel_tlb(struct gk20a *g);
struct nvgpu_gr_intr *nvgpu_gr_intr_init_support(struct gk20a *g);
void nvgpu_gr_intr_remove_support(struct gk20a *g, struct nvgpu_gr_intr *intr);
#endif /* NVGPU_GR_INTR_H */
