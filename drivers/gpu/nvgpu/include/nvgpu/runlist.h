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
#include <nvgpu/nvgpu_mem.h>
#include <nvgpu/lock.h>

struct gk20a;
struct tsg_gk20a;
struct nvgpu_fifo;
struct channel_gk20a;

#define NVGPU_FIFO_RUNLIST_INTERLEAVE_LEVEL_LOW     0U
#define NVGPU_FIFO_RUNLIST_INTERLEAVE_LEVEL_MEDIUM  1U
#define NVGPU_FIFO_RUNLIST_INTERLEAVE_LEVEL_HIGH    2U
#define NVGPU_FIFO_RUNLIST_INTERLEAVE_NUM_LEVELS    3U

#define RUNLIST_APPEND_FAILURE U32_MAX
#define RUNLIST_INVALID_ID U32_MAX

#define RUNLIST_DISABLED		0U
#define RUNLIST_ENABLED			1U

#define MAX_RUNLIST_BUFFERS		2U

#define NVGPU_INVALID_RUNLIST_ID		(~U32(0U))

struct nvgpu_runlist_info {
	u32 runlist_id;
	unsigned long *active_channels;
	unsigned long *active_tsgs;
	/* Each engine has its own SW and HW runlist buffer.*/
	struct nvgpu_mem mem[MAX_RUNLIST_BUFFERS];
	u32  cur_buffer;
	u32  total_entries;
	u32  pbdma_bitmask;      /* pbdmas supported for this runlist*/
	u32  eng_bitmask;        /* engines using this runlist */
	u32  reset_eng_bitmask;  /* engines to be reset during recovery */
	u32  count;              /* cached hw_submit parameter */
	bool stopped;
	bool support_tsg;
	/* protect ch/tsg/runlist preempt & runlist update */
	struct nvgpu_mutex runlist_lock;
};


u32 nvgpu_runlist_construct_locked(struct nvgpu_fifo *f,
				struct nvgpu_runlist_info *runlist,
				u32 buf_id,
				u32 max_entries);
int nvgpu_runlist_update_locked(struct gk20a *g, u32 runlist_id,
					    struct channel_gk20a *ch, bool add,
					    bool wait_for_finish);

int nvgpu_runlist_reschedule(struct channel_gk20a *ch, bool preempt_next,
		bool wait_preempt);

int nvgpu_runlist_update_for_channel(struct gk20a *g, u32 runlist_id,
			      struct channel_gk20a *ch,
			      bool add, bool wait_for_finish);
int nvgpu_runlist_reload(struct gk20a *g, u32 runlist_id,
			      bool add, bool wait_for_finish);
int nvgpu_runlist_reload_ids(struct gk20a *g, u32 runlist_ids, bool add);

const char *nvgpu_runlist_interleave_level_name(u32 interleave_level);

void nvgpu_fifo_runlist_set_state(struct gk20a *g, u32 runlists_mask,
		 u32 runlist_state);

int nvgpu_runlist_setup_sw(struct gk20a *g);
void nvgpu_runlist_cleanup_sw(struct gk20a *g);

void nvgpu_runlist_lock_active_runlists(struct gk20a *g);
void nvgpu_runlist_unlock_active_runlists(struct gk20a *g);

u32 nvgpu_runlist_get_runlists_mask(struct gk20a *g, u32 id,
	unsigned int id_type, u32 act_eng_bitmask, u32 pbdma_bitmask);

void nvgpu_runlist_unlock_runlists(struct gk20a *g, u32 runlists_mask);

#endif /* NVGPU_RUNLIST_H */
