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
#ifndef NVGPU_GOPS_ENGINE_H
#define NVGPU_GOPS_ENGINE_H

#include <nvgpu/types.h>

struct gk20a;
struct nvgpu_engine_status_info;
struct nvgpu_debug_context;

struct gops_engine_status {
	void (*read_engine_status_info)(struct gk20a *g,
		u32 engine_id, struct nvgpu_engine_status_info *status);
	void (*dump_engine_status)(struct gk20a *g,
			struct nvgpu_debug_context *o);
};

struct gops_engine {
	bool (*is_fault_engine_subid_gpc)(struct gk20a *g,
				 u32 engine_subid);
	u32 (*get_mask_on_id)(struct gk20a *g,
		u32 id, bool is_tsg);
	int (*init_info)(struct nvgpu_fifo *f);
	int (*init_ce_info)(struct nvgpu_fifo *f);
};

#endif
