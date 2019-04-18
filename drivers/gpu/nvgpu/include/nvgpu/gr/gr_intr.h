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

struct channel_gk20a;

struct nvgpu_gr_intr_info {
	u32 notify;
	u32 semaphore;
	u32 illegal_notify;
	u32 illegal_method;
	u32 illegal_class;
	u32 fecs_error;
	u32 class_error;
	u32 fw_method;
	u32 exception;
};

struct nvgpu_gr_tpc_exception {
	bool tex_exception;
	bool sm_exception;
	bool mpc_exception;
};

struct nvgpu_gr_isr_data {
	u32 addr;
	u32 data_lo;
	u32 data_hi;
	u32 curr_ctx;
	struct channel_gk20a *ch;
	u32 offset;
	u32 sub_chan;
	u32 class_num;
};

int nvgpu_gr_intr_handle_gpc_exception(struct gk20a *g, bool *post_event,
	struct nvgpu_gr_config *gr_config, struct channel_gk20a *fault_ch,
	u32 *hww_global_esr);
void nvgpu_gr_intr_handle_notify_pending(struct gk20a *g,
					struct nvgpu_gr_isr_data *isr_data);
void nvgpu_gr_intr_handle_semaphore_pending(struct gk20a *g,
					   struct nvgpu_gr_isr_data *isr_data);
void nvgpu_gr_intr_report_exception(struct gk20a *g, u32 inst,
				u32 err_type, u32 status);
struct channel_gk20a *nvgpu_gr_intr_get_channel_from_ctx(struct gk20a *g,
				u32 curr_ctx, u32 *curr_tsgid);
void nvgpu_gr_intr_set_error_notifier(struct gk20a *g,
		  struct nvgpu_gr_isr_data *isr_data, u32 error_notifier);
int nvgpu_gr_intr_stall_isr(struct gk20a *g);
#endif /* NVGPU_GR_INTR_H */
