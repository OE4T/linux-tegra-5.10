/*
 * Copyright (c) 2011-2019, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_RUNLIST_GK20A_H

#include <nvgpu/types.h>

struct channel_gk20a;
struct tsg_gk20a;
struct gk20a;

int gk20a_runlist_reschedule(struct channel_gk20a *ch, bool preempt_next);
int gk20a_fifo_reschedule_preempt_next(struct channel_gk20a *ch,
		bool wait_preempt);
int gk20a_runlist_set_interleave(struct gk20a *g,
		u32 id,
		u32 runlist_id,
		u32 new_level);
u32 gk20a_runlist_count_max(void);
u32 gk20a_runlist_entry_size(struct gk20a *g);
u32 gk20a_runlist_length_max(struct gk20a *g);
void gk20a_runlist_get_tsg_entry(struct tsg_gk20a *tsg,
		u32 *runlist, u32 timeslice);
void gk20a_runlist_get_ch_entry(struct channel_gk20a *ch, u32 *runlist);
void gk20a_runlist_hw_submit(struct gk20a *g, u32 runlist_id,
		u32 count, u32 buffer_index);
int gk20a_runlist_wait_pending(struct gk20a *g, u32 runlist_id);
void gk20a_runlist_write_state(struct gk20a *g, u32 runlists_mask,
		u32 runlist_state);

#endif /* NVGPU_RUNLIST_GK20A_H */
