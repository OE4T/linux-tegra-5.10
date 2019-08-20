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

/**
 * @file
 * @page unit-fifo Unit FIFO
 *
 * Overview
 * ========
 *
 * The FIFO unit is responsible for managing xxxxx.
 * primarily of TODO types:
 *
 *   + TODO
 *   + TODO
 *
 * The FIFO code also makes sure that all of the necessary SW and HW
 * initialization for engines, pdbma, runlist, channel and tsg subsystems
 * are taken care of before the GPU begins executing work.
 *
 * Top level FIFO Unit
 * ---------------------
 *
 * The FIFO unit TODO.
 *
 * See include/nvgpu/fifo.h for more details.
 *
 * Runlist
 * -------
 *
 * TODO
 *
 *   + include/nvgpu/runlist.h
 *
 * Pbdma
 * -------
 *
 * TODO
 *
 *   + include/nvgpu/pbdma.h
 *
 * Engines
 * -------
 *
 * TODO
 *
 *   + include/nvgpu/engines.h
 *
 * Preempt
 * -------
 *
 * TODO
 *
 *   + include/nvgpu/preempt.h
 *
 * Channel
 * -------
 *
 * TODO
 *
 *   + include/nvgpu/channel.h
 *
 * Tsg
 * -------
 *
 * TODO
 *
 *   + include/nvgpu/tsg.h
 *
 * Data Structures
 * ===============
 *
 * The major data structures exposed to users of the FIFO unit in nvgpu relate
 * to managing Engines, Runlists, Channels and Tsgs.
 * Following is a list of these structures:
 *
 *   + struct nvgpu_fifo
 *
 *       TODO
 *
 *   + struct nvgpu_runlist_info
 *
 *       TODO
 *
 *   + struct nvgpu_engine_info
 *
 *       TODO
 *
 *   + struct nvgpu_channel
 *
 *       TODO
 *
 *   + struct nvgpu_tsg
 *
 *       TODO
 *
 * Static Design
 * =============
 *
 * Details of static design.
 *
 * Resource utilization
 * --------------------
 *
 * External APIs
 * -------------
 *
 *   + TODO
 *
 *
 * Supporting Functionality
 * ========================
 *
 * There's a fair amount of supporting functionality:
 *
 *   + TODO
 *     - TODO
 *   + TODO
 *   + TODO
 *     # TODO
 *     # TODO
 *
 * Documentation for this will be filled in!
 *
 * Dependencies
 * ------------
 *
 * Dynamic Design
 * ==============
 *
 * Use case descriptions go here. Some potentials:
 *
 *   - TODO
 *   - TODO
 *   - TODO
 *
 * Requirements
 * ============
 *
 * Added this section to link to unit level requirements. Seems like it's
 * missing from the IPP template.
 *
 * Requirement    | Link
 * -----------    | ------------------------------------------------------------
 * NVGPU-RQCD-xx  | https://nvidia.jamacloud.com/perspective.req#/items/xxxxxxxx
 *
 * Open Items
 * ==========
 *
 * Any open items can go here.
 */

#include <nvgpu/types.h>
#include <nvgpu/lock.h>
#include <nvgpu/kref.h>
#include <nvgpu/list.h>
/**
 * H/w defined value for Channel ID type
 */
#define ID_TYPE_CHANNEL			0U
/**
 * H/w defined value for Tsg ID type
 */
#define ID_TYPE_TSG			1U
/**
 * S/w defined value for unknown ID type.
 */
#define ID_TYPE_UNKNOWN			(~U32(0U))
/**
 * Invalid ID.
 */
#define INVAL_ID			(~U32(0U))
/**
 * Timeout after which ctxsw timeout interrupt (if enabled by s/w) will be
 * triggered by h/w if context fails to context switch.
 */
#define CTXSW_TIMEOUT_PERIOD_MS		100U

#define PBDMA_SUBDEVICE_ID		1U

#define CHANNEL_INFO_VEID0		0U

struct gk20a;
struct nvgpu_engine_info;
struct nvgpu_runlist_info;
struct nvgpu_channel;
struct nvgpu_tsg;

struct nvgpu_fifo {
	struct gk20a *g;
	unsigned int num_channels;
	unsigned int runlist_entry_size;
	unsigned int num_runlist_entries;

	unsigned int num_pbdma;
	u32 *pbdma_map;

	/**
	 * This is the area of memory alloced by kernel to keep information for
	 * #max_engines supported by the chip. This information is filled up
	 * with device info h/w registers' values. Pointer is indexed by
	 * engine_id defined by h/w.
	 */
	struct nvgpu_engine_info *engine_info;
	/**
	 * Total number of engines supported on the chip. This variable is
	 * updated with one of the h/w register's value defined for chip
	 * configuration related settings.
	 */
	u32 max_engines;
	/**
	 * This represents total number of active engines supported on the chip.
	 * This is calculated based on total number of available engines
	 * read from device info h/w registers. This variable can be less than
	 * or equal to #max_engines.
	 */
	u32 num_engines;
	/**
	 * This is the area of memory alloced by kernel for #max_engines
	 * supported by the chip. This is needed to map engine_id defined
	 * by s/w to engine_id defined by device info h/w registers.
	 * This area of memory is indexed by s/w defined engine_id starting
	 * with 0.
	 */
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
#ifdef CONFIG_NVGPU_USERD
	struct nvgpu_mutex userd_mutex;
	struct nvgpu_mem *userd_slabs;
	u32 num_userd_slabs;
	u32 num_channels_per_slab;
	u64 userd_gpu_va;
#endif

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

#ifdef CONFIG_NVGPU_DEBUGGER
	unsigned long deferred_fault_engines;
	bool deferred_reset_pending;
	struct nvgpu_mutex deferred_reset_mutex;
#endif

	/** max number of veid supported by the chip */
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
#ifndef CONFIG_NVGPU_RECOVERY
void nvgpu_fifo_sw_quiesce(struct gk20a *g);
#endif

#endif /* NVGPU_FIFO_COMMON_H */
