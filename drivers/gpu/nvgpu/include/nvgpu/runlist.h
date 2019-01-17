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

#ifndef NVGPU_RUNLIST_H
#define NVGPU_RUNLIST_H

#include <nvgpu/types.h>

struct gk20a;
struct fifo_runlist_info_gk20a;
struct tsg_gk20a;
struct fifo_gk20a;
struct channel_gk20a;

#define RUNLIST_APPEND_FAILURE 0xffffffffU
u32 nvgpu_runlist_construct_locked(struct fifo_gk20a *f,
				struct fifo_runlist_info_gk20a *runlist,
				u32 buf_id,
				u32 max_entries);
int gk20a_fifo_update_runlist_locked(struct gk20a *g, u32 runlist_id,
					    struct channel_gk20a *ch, bool add,
					    bool wait_for_finish);

int nvgpu_fifo_reschedule_runlist(struct channel_gk20a *ch, bool preempt_next,
		bool wait_preempt);

int gk20a_fifo_update_runlist(struct gk20a *g, u32 runlist_id,
			      struct channel_gk20a *ch,
			      bool add, bool wait_for_finish);
int gk20a_fifo_update_runlist_ids(struct gk20a *g, u32 runlist_ids,
				bool add);

const char *gk20a_fifo_interleave_level_name(u32 interleave_level);

void gk20a_fifo_set_runlist_state(struct gk20a *g, u32 runlists_mask,
		 u32 runlist_state);

void gk20a_fifo_delete_runlist(struct fifo_gk20a *f);
int nvgpu_init_runlist(struct gk20a *g, struct fifo_gk20a *f);

#endif /* NVGPU_RUNLIST_H */
