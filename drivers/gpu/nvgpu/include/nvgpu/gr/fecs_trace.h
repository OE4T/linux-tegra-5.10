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

#ifndef NVGPU_GR_FECS_TRACE_H
#define NVGPU_GR_FECS_TRACE_H

#include <nvgpu/types.h>
#include <nvgpu/list.h>
#include <nvgpu/lock.h>
#include <nvgpu/thread.h>

/*
 * If HW circular buffer is getting too many "buffer full" conditions,
 * increasing this constant should help (it drives Linux' internal buffer size).
 */
#define GK20A_FECS_TRACE_NUM_RECORDS		(1 << 10)
#define GK20A_FECS_TRACE_FRAME_PERIOD_US	(1000000ULL/60ULL)
#define GK20A_FECS_TRACE_PTIMER_SHIFT		5

struct gk20a;

struct nvgpu_gr_fecs_trace {
	struct nvgpu_list_node context_list;
	struct nvgpu_mutex list_lock;

	struct nvgpu_mutex poll_lock;
	struct nvgpu_thread poll_task;

	struct nvgpu_mutex enable_lock;
	u32 enable_count;
};

struct nvgpu_fecs_trace_record {
	u32 magic_lo;
	u32 magic_hi;
	u32 context_id;
	u32 context_ptr;
	u32 new_context_id;
	u32 new_context_ptr;
	u64 ts[];
};

struct nvgpu_fecs_trace_context_entry {
	u32 context_ptr;

	pid_t pid;
	u32 vmid;

	struct nvgpu_list_node entry;
};

static inline struct nvgpu_fecs_trace_context_entry *
nvgpu_fecs_trace_context_entry_from_entry(struct nvgpu_list_node *node)
{
	return (struct nvgpu_fecs_trace_context_entry *)
		((uintptr_t)node -
		offsetof(struct nvgpu_fecs_trace_context_entry, entry));
};

int nvgpu_gr_fecs_trace_init(struct gk20a *g);
int nvgpu_gr_fecs_trace_deinit(struct gk20a *g);

int nvgpu_gr_fecs_trace_num_ts(struct gk20a *g);
struct nvgpu_fecs_trace_record *nvgpu_gr_fecs_trace_get_record(struct gk20a *g,
	int idx);
bool nvgpu_gr_fecs_trace_is_valid_record(struct gk20a *g,
	struct nvgpu_fecs_trace_record *r);

int nvgpu_gr_fecs_trace_add_context(struct gk20a *g, u32 context_ptr,
	pid_t pid, u32 vmid, struct nvgpu_list_node *list);
void nvgpu_gr_fecs_trace_remove_context(struct gk20a *g, u32 context_ptr,
	struct nvgpu_list_node *list);
void nvgpu_gr_fecs_trace_remove_contexts(struct gk20a *g,
	struct nvgpu_list_node *list);
void nvgpu_gr_fecs_trace_find_pid(struct gk20a *g, u32 context_ptr,
	struct nvgpu_list_node *list, pid_t *pid, u32 *vmid);

#endif /* NVGPU_GR_FECS_TRACE_H */
