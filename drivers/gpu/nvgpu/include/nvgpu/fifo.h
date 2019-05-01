/*
 * fifo common definitions (gr host)
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

#ifndef NVGPU_FIFO_COMMON_H
#define NVGPU_FIFO_COMMON_H

#include <nvgpu/types.h>
#include <nvgpu/lock.h>
#include <nvgpu/kref.h>
#include <nvgpu/list.h>

#define ID_TYPE_CHANNEL			0U
#define ID_TYPE_TSG			1U
#define ID_TYPE_UNKNOWN			(~U32(0U))

#define INVAL_ID			(~U32(0U))

#define CTXSW_TIMEOUT_PERIOD_MS		100U

#define PBDMA_SUBDEVICE_ID		1U

#define CHANNEL_INFO_VEID0		0U

struct gk20a;
struct nvgpu_engine_info;
struct nvgpu_runlist_info;
struct channel_gk20a;
struct tsg_gk20a;

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
	struct channel_gk20a *channel;
	/* zero-kref'd channels here */
	struct nvgpu_list_node free_chs;
	struct nvgpu_mutex free_chs_mutex;
	struct nvgpu_mutex engines_reset_mutex;
	struct nvgpu_spinlock runlist_submit_lock;

	struct tsg_gk20a *tsg;
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

int nvgpu_fifo_init_support(struct gk20a *g);
int nvgpu_fifo_setup_sw(struct gk20a *g);
int nvgpu_fifo_setup_sw_common(struct gk20a *g);
void nvgpu_fifo_cleanup_sw(struct gk20a *g);
void nvgpu_fifo_cleanup_sw_common(struct gk20a *g);

const char *nvgpu_fifo_decode_pbdma_ch_eng_status(u32 index);
int nvgpu_fifo_suspend(struct gk20a *g);

#endif /* NVGPU_FIFO_COMMON_H */
