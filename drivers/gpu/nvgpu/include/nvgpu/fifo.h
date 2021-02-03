/*
 * FIFO common definitions.
 *
 * Copyright (c) 2011-2021, NVIDIA CORPORATION.  All rights reserved.
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
 *   + include/nvgpu/fifo.h
 *   + include/nvgpu/gops/fifo.h
 *
 * Runlist
 * -------
 *
 * TODO
 *
 *   + include/nvgpu/runlist.h
 *   + include/nvgpu/gops/runlist.h
 *
 * Pbdma
 * -------
 *
 * TODO
 *
 *   + include/nvgpu/pbdma.h
 *   + include/nvgpu/pbdma_status.h
 *
 * Engines
 * -------
 *
 * TODO
 *
 *   + include/nvgpu/engines.h
 *   + include/nvgpu/engine_status.h
 *   + include/nvgpu/gops/engine.h
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
 *   + include/nvgpu/gops/channel.h
 *
 * Tsg
 * -------
 *
 * TODO
 *
 *   + include/nvgpu/tsg.h
 *
 * RAM
 * -------
 *
 * TODO
 *
 *   + include/nvgpu/gops/ramin.h
 *   + include/nvgpu/gops/ramfc.h
 *
 * Sync
 * ----
 *
 *   + include/nvgpu/channel_sync.h
 *   + include/nvgpu/channel_sync_syncpt.h
 *   + include/nvgpu/gops/sync.h
 *
 * Usermode
 * --------
 *
 * TODO
 *
 *   + include/nvgpu/gops/usermode.h
 *
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
 *   + struct nvgpu_runlist
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
 * Open Items
 * ==========
 *
 * Any open items can go here.
 */

#include <nvgpu/types.h>
#include <nvgpu/lock.h>
#include <nvgpu/kref.h>
#include <nvgpu/list.h>
#include <nvgpu/swprofile.h>

/**
 * H/w defined value for Channel ID type
 */
#define ID_TYPE_CHANNEL			0U
/**
 * H/w defined value for Tsg ID type
 */
#define ID_TYPE_TSG			1U
/**
 * S/w defined value for Runlist ID type
 */
#define ID_TYPE_RUNLIST			2U
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

/** Subctx id 0 */
#define CHANNEL_INFO_VEID0		0U

struct gk20a;
struct nvgpu_runlist;
struct nvgpu_channel;
struct nvgpu_tsg;
struct nvgpu_swprofiler;

struct nvgpu_fifo {
	/** Pointer to GPU driver struct. */
	struct gk20a *g;

	/** Number of channels supported by the h/w. */
	unsigned int num_channels;

	/** Runlist entry size in bytes as supported by h/w. */
	unsigned int runlist_entry_size;

	/** Number of runlist entries per runlist as supported by the h/w. */
	unsigned int num_runlist_entries;

	/**
	 * Array of pointers to the engines that host controls. The size is
	 * based on the GPU litter value HOST_NUM_ENGINES. This is indexed by
	 * engine ID. That is to say, if you want to get a device that
	 * corresponds to engine ID, E, then host_engines[E] will give you a
	 * pointer to that device.
	 *
	 * If a given element is NULL, that means that there is no engine for
	 * the given engine ID. This is expected for chips that do not populate
	 * the full set of possible engines for a given family of chips. E.g
	 * a GV100 has a lot more engines than a gv11b.
	 */
	const struct nvgpu_device **host_engines;

	/**
	 * Total number of engines supported by the chip family. See
	 * #host_engines above.
	 */
	u32 max_engines;

	/**
	 * The list of active engines; it can be (and often is) smaller than
	 * #host_engines. This list will have exactly #num_engines engines;
	 * use #num_engines to iterate over the list of devices with a for-loop.
	 */
	const struct nvgpu_device **active_engines;

	/**
	 * Length of the #active_engines array.
	 */
	u32 num_engines;

	/**
	 * Pointers to runlists, indexed by real hw runlist_id.
	 * If a runlist is active, then runlists[runlist_id] points
	 * to one entry in active_runlist_info. Otherwise, it is NULL.
	 */
	struct nvgpu_runlist **runlists;
	/** Number of runlists supported by the h/w. */
	u32 max_runlists;

	/** Array of actual HW runlists that are present on the GPU. */
	struct nvgpu_runlist *active_runlists;
	/** Number of active runlists. */
	u32 num_runlists;

	struct nvgpu_swprofiler kickoff_profiler;
	struct nvgpu_swprofiler recovery_profiler;
	struct nvgpu_swprofiler eng_reset_profiler;

#ifdef CONFIG_NVGPU_USERD
	struct nvgpu_mutex userd_mutex;
	struct nvgpu_mem *userd_slabs;
	u32 num_userd_slabs;
	u32 num_channels_per_slab;
	u64 userd_gpu_va;
#endif

	/**
	 * Number of channels in use. This is incremented by one when a
	 * channel is opened and decremented by one when a channel is closed by
	 * userspace.
	 */
	unsigned int used_channels;
	/**
	 * This is the zero initialized area of memory allocated by kernel for
	 * storing channel specific data i.e. #nvgpu_channel struct info for
	 * #num_channels number of channels.
	 */
	struct nvgpu_channel *channel;
	/** List of channels available for allocation */
	struct nvgpu_list_node free_chs;
	/**
	 * Lock used to read and update #free_chs list. Channel entry is
	 * removed when a channel is opened and added back to the #free_ch list
	 * when channel is closed by userspace.
	 * This lock is also used to protect #used_channels.
	 */
	struct nvgpu_mutex free_chs_mutex;

	/** Lock used to prevent multiple recoveries. */
	struct nvgpu_mutex engines_reset_mutex;

	/** Lock used to update h/w runlist registers for submitting runlist. */
	struct nvgpu_spinlock runlist_submit_lock;

	/**
	 * This is the zero initialized area of memory allocated by kernel for
	 * storing TSG specific data i.e. #nvgpu_tsg struct info for
	 * #num_channels number of TSG.
	 */
	struct nvgpu_tsg *tsg;
	/**
	 * Lock used to read and update #nvgpu_tsg.in_use. TSG entry is
	 * in use when a TSG is opened and not in use when TSG is closed
	 * by userspace. Refer #nvgpu_tsg.in_use in tsg.h.
	 */
	struct nvgpu_mutex tsg_inuse_mutex;

	/**
	 * Pointer to a function that will be executed when FIFO support
	 * is requested to be removed. This is supposed to clean up
	 * all s/w resources used by FIFO module e.g. Channel, TSG, PBDMA,
	 * Runlist, Engines and USERD.
	 */
	void (*remove_support)(struct nvgpu_fifo *f);

	/**
	 * nvgpu_fifo_setup_sw is skipped if this flag is set to true.
	 * This gets set to true after successful completion of
	 * nvgpu_fifo_setup_sw.
	 */
	bool sw_ready;

	/** FIFO interrupt related fields. */
	struct nvgpu_fifo_intr {
		/** Share info between isr and non-isr code. */
		struct nvgpu_fifo_intr_isr {
			/** Lock for fifo isr. */
			struct nvgpu_mutex mutex;
		} isr;
		/** PBDMA interrupt specific data. */
		struct nvgpu_fifo_intr_pbdma {
			/** H/w specific unrecoverable PBDMA interrupts. */
			u32 device_fatal_0;
			/**
			 * H/w specific recoverable PBDMA interrupts that are
			 * limited to channels. Fixing and clearing the
			 * interrupt will allow PBDMA to continue.
			 */
			u32 channel_fatal_0;
			/** H/w specific recoverable PBDMA interrupts. */
			u32 restartable_0;
		} pbdma;
	} intr;

#ifdef CONFIG_NVGPU_DEBUGGER
	unsigned long deferred_fault_engines;
	bool deferred_reset_pending;
	struct nvgpu_mutex deferred_reset_mutex;
#endif

	/** Max number of sub context i.e. veid supported by the h/w. */
	u32 max_subctx_count;
	/** Used for vgpu. */
	u32 channel_base;
};

static inline const char *nvgpu_id_type_to_str(unsigned int id_type)
{
	const char *str = NULL;

	switch (id_type) {
	case ID_TYPE_CHANNEL:
		str = "Channel";
		break;
	case ID_TYPE_TSG:
		str = "TSG";
		break;
	case ID_TYPE_RUNLIST:
		str = "Runlist";
		break;
	default:
		str = "Unknown";
		break;
	}

	return str;
}

/**
 * @brief Initialize FIFO software context.
 *
 * @param g [in]	The GPU driver struct.
 *
 * Calls function to do setup_sw. Refer #nvgpu_fifo_setup_sw.
 * If setup_sw was successful, call function to do setup_hw. This is to take
 * care of h/w specific setup related to FIFO module.
 *
 * @return 0 in case of success, < 0 in case of failure.
 * @retval -ENOMEM in case there is not enough memory available.
 * @retval -EINVAL in case condition variable has invalid value.
 * @retval -EBUSY in case reference condition variable pointer isn't NULL.
 * @retval -EFAULT in case any faults occurred while accessing condition
 * variable or attribute.
 */
int nvgpu_fifo_init_support(struct gk20a *g);

/**
 * @brief Initialize FIFO software context and mark it ready to be used.
 *
 * @param g [in]	The GPU driver struct.
 *
 * Return if #nvgpu_fifo.sw_ready is set to true i.e. s/w set up is already
 * done.
 * Call #nvgpu_fifo_setup_sw_common to do s/w set up.
 * Init channel worker.
 * Mark FIFO s/w ready by setting #nvgpu_fifo.sw_ready to true.
 *
 * @return 0 in case of success, < 0 in case of failure.
 * @retval -ENOMEM in case there is not enough memory available.
 * @retval -EINVAL in case condition variable has invalid value.
 * @retval -EBUSY in case reference condition variable pointer isn't NULL.
 * @retval -EFAULT in case any faults occurred while accessing condition
 * variable or attribute.
 */
int nvgpu_fifo_setup_sw(struct gk20a *g);

/**
 * @brief Initialize FIFO software context.
 *
 * @param g [in]	The GPU driver struct.
 *
 * - Init mutexes needed by FIFO module. Refer #nvgpu_fifo struct.
 * - Do #nvgpu_channel_setup_sw.
 * - Do #nvgpu_tsg_setup_sw.
 * - Do pbdma.setup_sw.
 * - Do #nvgpu_engine_setup_sw.
 * - Do #nvgpu_runlist_setup_sw.
 * - Do userd.setup_sw.
 * - Init #nvgpu_fifo.remove_support function pointer.
 *
 * @note In case of failure, cleanup_sw for the blocks that are already
 *       initialized is also taken care of by this function.
 * @return 0 in case of success, < 0 in case of failure.
 * @retval -ENOMEM in case there is not enough memory available.
 * @retval -EINVAL in case condition variable has invalid value.
 * @retval -EBUSY in case reference condition variable pointer isn't NULL.
 * @retval -EFAULT in case any faults occurred while accessing condition
 * variable or attribute.
 */
int nvgpu_fifo_setup_sw_common(struct gk20a *g);

/**
 * @brief Clean up FIFO software context.
 *
 * @param g [in]	The GPU driver struct.
 *
 * Deinit Channel worker thread.
 * Calls #nvgpu_fifo_cleanup_sw_common.
 */
void nvgpu_fifo_cleanup_sw(struct gk20a *g);

/**
 * @brief Clean up FIFO software context and related resources.
 *
 * @param g [in]	The GPU driver struct.
 *
 * - Do userd.cleanup_sw.
 * - Do #nvgpu_channel_cleanup_sw.
 * - Do #nvgpu_tsg_cleanup_sw.
 * - Do #nvgpu_runlist_cleanup_sw.
 * - Do #nvgpu_engine_cleanup_sw.
 * - Do pbdma.setup_sw.
 * - Destroy mutexes used by FIFO module. Refer #nvgpu_fifo struct.
 */
void nvgpu_fifo_cleanup_sw_common(struct gk20a *g);

/**
 * @brief Decode PBDMA channel status and Engine status read from h/w register.
 *
 * @param index [in]	Status value used to index into the constant array of
 *			constant characters.
 *
 * Decode PBDMA channel status and Engine status value read from h/w
 * register into string format.
 */
const char *nvgpu_fifo_decode_pbdma_ch_eng_status(u32 index);

/**
 * @brief Suspend FIFO support while preparing GPU for poweroff.
 *
 * @param g [in]	The GPU driver struct.
 *
 * Suspending FIFO will disable BAR1 snooping (if supported by h/w) and also
 * FIFO stalling and non-stalling interrupts at FIFO unit and MC level.
 */
int nvgpu_fifo_suspend(struct gk20a *g);

/**
 * @brief Emergency quiescing of FIFO.
 *
 * @param g [in]	The GPU driver struct.
 *
 * Put FIFO into a non-functioning state to ensure that no corrupted
 * work is completed because of the fault. This is because the freedom
 * from interference may not always be shown between the faulted and
 * the non-faulted TSG contexts.
 * - Disable all runlists
 * - Preempt all runlists
 */
void nvgpu_fifo_sw_quiesce(struct gk20a *g);

#endif /* NVGPU_FIFO_COMMON_H */
