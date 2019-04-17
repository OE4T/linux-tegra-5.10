/*
 * Copyright (c) 2017-2019, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_FIFO_VGPU_H
#define NVGPU_FIFO_VGPU_H

#include <nvgpu/types.h>

struct gk20a;
struct channel_gk20a;
struct fifo_gk20a;
struct tsg_gk20a;
struct tegra_vgpu_fifo_intr_info;
struct tegra_vgpu_channel_event_info;
struct tegra_vgpu_channel_set_error_notifier;

int vgpu_fifo_setup_sw(struct gk20a *g);
void vgpu_fifo_cleanup_sw(struct gk20a *g);
int vgpu_init_fifo_setup_hw(struct gk20a *g);
void vgpu_channel_bind(struct channel_gk20a *ch);
void vgpu_channel_unbind(struct channel_gk20a *ch);
int vgpu_channel_alloc_inst(struct gk20a *g, struct channel_gk20a *ch);
void vgpu_channel_free_inst(struct gk20a *g, struct channel_gk20a *ch);
void vgpu_channel_enable(struct channel_gk20a *ch);
void vgpu_channel_disable(struct channel_gk20a *ch);
u32 vgpu_channel_count(struct gk20a *g);
int vgpu_fifo_init_engine_info(struct fifo_gk20a *f);
int vgpu_fifo_preempt_channel(struct gk20a *g, struct channel_gk20a *ch);
int vgpu_fifo_preempt_tsg(struct gk20a *g, struct tsg_gk20a *tsg);
int vgpu_channel_set_timeslice(struct channel_gk20a *ch, u32 timeslice);
int vgpu_tsg_force_reset_ch(struct channel_gk20a *ch,
					u32 err_code, bool verbose);
u32 vgpu_tsg_default_timeslice_us(struct gk20a *g);
int vgpu_tsg_open(struct tsg_gk20a *tsg);
void vgpu_tsg_release(struct tsg_gk20a *tsg);
int vgpu_tsg_bind_channel(struct tsg_gk20a *tsg, struct channel_gk20a *ch);
int vgpu_tsg_unbind_channel(struct tsg_gk20a *tsg, struct channel_gk20a *ch);
int vgpu_tsg_set_timeslice(struct tsg_gk20a *tsg, u32 timeslice);
int vgpu_tsg_set_interleave(struct tsg_gk20a *tsg, u32 level);
void vgpu_tsg_enable(struct tsg_gk20a *tsg);
int vgpu_set_sm_exception_type_mask(struct channel_gk20a *ch, u32 mask);
void vgpu_channel_free_ctx_header(struct channel_gk20a *c);
int vgpu_fifo_isr(struct gk20a *g, struct tegra_vgpu_fifo_intr_info *info);
void vgpu_handle_channel_event(struct gk20a *g,
			struct tegra_vgpu_channel_event_info *info);
void vgpu_channel_abort_cleanup(struct gk20a *g, u32 chid);
void vgpu_set_error_notifier(struct gk20a *g,
		struct tegra_vgpu_channel_set_error_notifier *p);

#endif /* NVGPU_FIFO_VGPU_H */
