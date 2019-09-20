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
#ifndef NVGPU_GOPS_CHANNEL_H
#define NVGPU_GOPS_CHANNEL_H

#include <nvgpu/types.h>

struct gk20a;
struct nvgpu_channel;
struct nvgpu_channel_hw_state;
struct nvgpu_debug_context;
struct nvgpu_channel_dump_info;

struct gops_channel {
	int (*alloc_inst)(struct gk20a *g, struct nvgpu_channel *ch);
	void (*free_inst)(struct gk20a *g, struct nvgpu_channel *ch);
	void (*bind)(struct nvgpu_channel *ch);
	void (*unbind)(struct nvgpu_channel *ch);
	void (*enable)(struct nvgpu_channel *ch);
	void (*disable)(struct nvgpu_channel *ch);
	u32 (*count)(struct gk20a *g);
	void (*read_state)(struct gk20a *g, struct nvgpu_channel *ch,
			struct nvgpu_channel_hw_state *state);
	void (*force_ctx_reload)(struct nvgpu_channel *ch);
	void (*abort_clean_up)(struct nvgpu_channel *ch);
	int (*suspend_all_serviceable_ch)(struct gk20a *g);
	int (*resume_all_serviceable_ch)(struct gk20a *g);
	void (*set_error_notifier)(struct nvgpu_channel *ch, u32 error);
	void (*reset_faulted)(struct gk20a *g, struct nvgpu_channel *ch,
			bool eng, bool pbdma);
	void (*debug_dump)(struct gk20a *g,
			struct nvgpu_debug_context *o,
			struct nvgpu_channel_dump_info *info);

#ifdef CONFIG_NVGPU_KERNEL_MODE_SUBMIT
	int (*set_syncpt)(struct nvgpu_channel *ch);
#endif

};

#endif
