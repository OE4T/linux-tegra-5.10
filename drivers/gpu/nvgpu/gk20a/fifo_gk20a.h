/*
 * GK20A graphics fifo (gr host)
 *
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
#ifndef FIFO_GK20A_H
#define FIFO_GK20A_H

#include <nvgpu/types.h>
#include <nvgpu/kref.h>
#include <nvgpu/fifo.h>
#include <nvgpu/engines.h>
#include <nvgpu/runlist.h>
#include <nvgpu/profile.h>

struct nvgpu_channel;
struct nvgpu_tsg;

struct nvgpu_fifo {
	struct gk20a *g;
	unsigned int num_channels;
	unsigned int runlist_entry_size;
	unsigned int num_runlist_entries;

	unsigned int num_pbdma;
	u32 *pbdma_map;

	struct nvgpu_engine_info *engine_info;
	u32 max_engines;
	u32 num_engines;
	u32 *active_engines_list;

	/* Pointers to runlists, indexed by real hw runlist_id.
	 * If a runlist is active, then runlist_info[runlist_id] points
	 * to one entry in active_runlist_info. Otherwise, it is NULL.
	 */
	struct nvgpu_runlist_info **runlist_info;
	u32 max_runlists;

	/* Array of runlists that are actually in use */
	struct nvgpu_runlist_info *active_runlist_info;
	u32 num_runlists; /* number of active runlists */
#ifdef CONFIG_DEBUG_FS
	struct {
		struct nvgpu_profile *data;
		nvgpu_atomic_t get;
		bool enabled;
		u64 *sorted;
		struct nvgpu_ref ref;
		struct nvgpu_mutex lock;
	} profile;
#endif
	struct nvgpu_mutex userd_mutex;
	struct nvgpu_mem *userd_slabs;
	u32 num_userd_slabs;
	u32 num_channels_per_slab;
	u32 userd_entry_size;
	u64 userd_gpu_va;

	unsigned int used_channels;
	struct nvgpu_channel *channel;
	/* zero-kref'd channels here */
	struct nvgpu_list_node free_chs;
	struct nvgpu_mutex free_chs_mutex;
	struct nvgpu_mutex engines_reset_mutex;
	struct nvgpu_spinlock runlist_submit_lock;

	struct nvgpu_tsg *tsg;
	struct nvgpu_mutex tsg_inuse_mutex;

	void (*remove_support)(struct nvgpu_fifo *f);
	bool sw_ready;
	struct {
		/* share info between isrs and non-isr code */
		struct {
			struct nvgpu_mutex mutex;
		} isr;
		struct {
			u32 device_fatal_0;
			u32 channel_fatal_0;
			u32 restartable_0;
		} pbdma;
		struct {

		} engine;


	} intr;

	unsigned long deferred_fault_engines;
	bool deferred_reset_pending;
	struct nvgpu_mutex deferred_reset_mutex;

	u32 max_subctx_count;
	u32 channel_base;
};

int gk20a_init_fifo_reset_enable_hw(struct gk20a *g);
int gk20a_init_fifo_setup_hw(struct gk20a *g);
void gk20a_fifo_bar1_snooping_disable(struct gk20a *g);
int gk20a_fifo_init_pbdma_map(struct gk20a *g, u32 *pbdma_map, u32 num_pbdma);
u32 gk20a_fifo_get_runlist_timeslice(struct gk20a *g);
u32 gk20a_fifo_get_pb_timeslice(struct gk20a *g);

#endif /* FIFO_GK20A_H */
