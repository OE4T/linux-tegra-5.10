/*
 * Copyright (c) 2020, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_PROFILER_H
#define NVGPU_PROFILER_H

#ifdef CONFIG_NVGPU_PROFILER

#include <nvgpu/list.h>

struct gk20a;
struct nvgpu_channel;

struct dbg_profiler_object_data {
	struct gk20a *g;
	int session_id;
	u32 prof_handle;
	struct nvgpu_tsg *tsg;
	bool has_reservation;
	struct nvgpu_list_node prof_obj_entry;
};

static inline struct dbg_profiler_object_data *
dbg_profiler_object_data_from_prof_obj_entry(struct nvgpu_list_node *node)
{
	return (struct dbg_profiler_object_data *)
	((uintptr_t)node - offsetof(struct dbg_profiler_object_data, prof_obj_entry));
};

int nvgpu_profiler_alloc(struct gk20a *g,
	struct dbg_profiler_object_data **_prof);
void nvgpu_profiler_free(struct dbg_profiler_object_data *prof);

#endif /* CONFIG_NVGPU_PROFILER */
#endif /* NVGPU_PROFILER_H */
