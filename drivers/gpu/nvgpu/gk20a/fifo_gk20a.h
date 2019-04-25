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

struct gk20a_debug_output;
struct nvgpu_semaphore;
struct channel_gk20a;
struct tsg_gk20a;

#define MAX_RUNLIST_BUFFERS		2U

#define FIFO_INVAL_ENGINE_ID		(~U32(0U))
#define FIFO_INVAL_MMU_ID		(~U32(0U))
#define FIFO_INVAL_CHANNEL_ID		(~U32(0U))
#define FIFO_INVAL_TSG_ID		(~U32(0U))
#define FIFO_INVAL_RUNLIST_ID		(~U32(0U))
#define FIFO_INVAL_SYNCPT_ID		(~U32(0U))

/*
 * Number of entries in the kickoff latency buffer, used to calculate
 * the profiling and histogram. This number is calculated to be statistically
 * significative on a histogram on a 5% step
 */
#ifdef CONFIG_DEBUG_FS
#define FIFO_PROFILING_ENTRIES	16384U
#endif

#define	RUNLIST_DISABLED		0U
#define	RUNLIST_ENABLED			1U

/* generally corresponds to the "pbdma" engine */

struct fifo_runlist_info_gk20a {
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

struct fifo_pbdma_exception_info_gk20a {
	u32 status_r; /* raw register value from hardware */
	u32 id, next_id;
	u32 chan_status_v; /* raw value from hardware */
	bool id_is_chid, next_id_is_chid;
	bool chsw_in_progress;
};

struct fifo_engine_exception_info_gk20a {
	u32 status_r; /* raw register value from hardware */
	u32 id, next_id;
	u32 ctx_status_v; /* raw value from hardware */
	bool id_is_chid, next_id_is_chid;
	bool faulted, idle, ctxsw_in_progress;
};

struct fifo_engine_info_gk20a {
	u32 engine_id;
	u32 runlist_id;
	u32 intr_mask;
	u32 reset_mask;
	u32 pbdma_id;
	u32 inst_id;
	u32 pri_base;
	u32 fault_id;
	enum nvgpu_fifo_engine engine_enum;
	struct fifo_pbdma_exception_info_gk20a pbdma_exception_info;
	struct fifo_engine_exception_info_gk20a engine_exception_info;
};

enum {
	PROFILE_IOCTL_ENTRY = 0U,
	PROFILE_ENTRY,
	PROFILE_JOB_TRACKING,
	PROFILE_APPEND,
	PROFILE_END,
	PROFILE_IOCTL_EXIT,
	PROFILE_MAX
};

struct fifo_profile_gk20a {
	u64 timestamp[PROFILE_MAX];
};

struct fifo_gk20a {
	struct gk20a *g;
	unsigned int num_channels;
	unsigned int runlist_entry_size;
	unsigned int num_runlist_entries;

	unsigned int num_pbdma;
	u32 *pbdma_map;

	struct fifo_engine_info_gk20a *engine_info;
	u32 max_engines;
	u32 num_engines;
	u32 *active_engines_list;

	/* Pointers to runlists, indexed by real hw runlist_id.
	 * If a runlist is active, then runlist_info[runlist_id] points
	 * to one entry in active_runlist_info. Otherwise, it is NULL.
	 */
	struct fifo_runlist_info_gk20a **runlist_info;
	u32 max_runlists;

	/* Array of runlists that are actually in use */
	struct fifo_runlist_info_gk20a *active_runlist_info;
	u32 num_runlists; /* number of active runlists */
#ifdef CONFIG_DEBUG_FS
	struct {
		struct fifo_profile_gk20a *data;
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
	struct channel_gk20a *channel;
	/* zero-kref'd channels here */
	struct nvgpu_list_node free_chs;
	struct nvgpu_mutex free_chs_mutex;
	struct nvgpu_mutex engines_reset_mutex;
	struct nvgpu_spinlock runlist_submit_lock;

	struct tsg_gk20a *tsg;
	struct nvgpu_mutex tsg_inuse_mutex;

	void (*remove_support)(struct fifo_gk20a *f);
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

#ifdef CONFIG_DEBUG_FS
struct fifo_profile_gk20a *gk20a_fifo_profile_acquire(struct gk20a *g);
void gk20a_fifo_profile_release(struct gk20a *g,
	struct fifo_profile_gk20a *profile);
void gk20a_fifo_profile_snapshot(struct fifo_profile_gk20a *profile, int idx);
#else
static inline struct fifo_profile_gk20a *
gk20a_fifo_profile_acquire(struct gk20a *g)
{
	return NULL;
}
static inline void gk20a_fifo_profile_release(struct gk20a *g,
	struct fifo_profile_gk20a *profile)
{
}
static inline void gk20a_fifo_profile_snapshot(
		struct fifo_profile_gk20a *profile, int idx)
{
}
#endif

#endif /* FIFO_GK20A_H */
