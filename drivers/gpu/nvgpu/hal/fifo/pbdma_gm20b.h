/*
 * Copyright (c) 2014-2018, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_PBDMA_GM20B_H
#define NVGPU_PBDMA_GM20B_H

#include <nvgpu/types.h>

struct gk20a;
struct gk20a_debug_output;

void gm20b_pbdma_intr_enable(struct gk20a *g, bool enable);

unsigned int gm20b_pbdma_handle_intr_0(struct gk20a *g, u32 pbdma_id,
			u32 pbdma_intr_0, u32 *handled, u32 *error_notifier);
unsigned int gm20b_pbdma_handle_intr_1(struct gk20a *g, u32 pbdma_id,
			u32 pbdma_intr_1, u32 *handled, u32 *error_notifier);
u32 gm20b_pbdma_get_signature(struct gk20a *g);
u32 gm20b_pbdma_read_data(struct gk20a *g, u32 pbdma_id);
void gm20b_pbdma_reset_header(struct gk20a *g, u32 pbdma_id);
void gm20b_pbdma_reset_method(struct gk20a *g, u32 pbdma_id,
			u32 pbdma_method_index);
u32 gm20b_pbdma_acquire_val(u64 timeout);
void gm20b_pbdma_dump_status(struct gk20a *g, struct gk20a_debug_output *o);

u32 gm20b_pbdma_device_fatal_0_intr_descs(void);
u32 gm20b_pbdma_channel_fatal_0_intr_descs(void);
u32 gm20b_pbdma_restartable_0_intr_descs(void);

void gm20b_pbdma_clear_all_intr(struct gk20a *g, u32 pbdma_id);
void gm20b_pbdma_disable_and_clear_all_intr(struct gk20a *g);
unsigned int gm20b_pbdma_handle_intr(struct gk20a *g, u32 pbdma_id,
			u32 *error_notifier);

#endif /* NVGPU_PBDMA_GM20B_H */
