/*
 * Copyright (c) 2019-2020, NVIDIA CORPORATION.  All rights reserved.
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
#ifndef NVGPU_GOPS_RUNLIST_H
#define NVGPU_GOPS_RUNLIST_H

#include <nvgpu/types.h>
#include <nvgpu/pbdma.h>

/**
 * @file
 *
 * Runlist HAL interface.
 */
struct gk20a;
struct nvgpu_channel;

/**
 * Runlist HAL operations.
 *
 * @see gpu_ops
 */
struct gops_runlist {
	/**
	 * @brief Reload runlist.
	 *
	 * @param g [in]		The GPU driver struct.
	 * @param runlist_id [in]	Runlist identifier.
	 * @param add [in]		True to submit a runlist buffer with
	 * 				all active channels. False to submit
	 * 				an empty runlist buffer.
	 * @param wait_for_finish [in]	True to wait for runlist update
	 * 				completion.
	 *
	 * When \a add is true, all entries are updated for the runlist.
	 * A runlist buffer is built with all active channels/TSGs for the
	 * runlist and submitted to H/W.
	 *
	 * When \a add is false, an empty runlist buffer is submitted to H/W.
	 * Submitting a NULL runlist results in Host expiring the current
	 * timeslices and effectively disabling scheduling for that runlist
	 * processor until the next runlist is submitted.
	 *
	 * @return 0 in case of success, < 0 in case of failure.
	 * @retval -ETIMEDOUT if transition to the new runlist takes too long,
	 *         and \a wait_for_finish was requested.
	 * @retval -E2BIG in case there are not enough entries in the runlist
	 *         buffer to accommodate all active channels/TSGs.
	 */
	int (*reload)(struct gk20a *g, u32 runlist_id,
			bool add, bool wait_for_finish);

	/** @cond DOXYGEN_SHOULD_SKIP_THIS */

	int (*update_for_channel)(struct gk20a *g, u32 runlist_id,
			struct nvgpu_channel *ch, bool add,
			bool wait_for_finish);
	u32 (*count_max)(struct gk20a *g);
	u32 (*entry_size)(struct gk20a *g);
	u32 (*length_max)(struct gk20a *g);
	void (*get_tsg_entry)(struct nvgpu_tsg *tsg,
			u32 *runlist, u32 timeslice);
	void (*get_ch_entry)(struct nvgpu_channel *ch, u32 *runlist);
	void (*hw_submit)(struct gk20a *g, u32 runlist_id,
		u32 count, u32 buffer_index);
	int (*wait_pending)(struct gk20a *g, u32 runlist_id);
	void (*write_state)(struct gk20a *g, u32 runlists_mask,
			u32 runlist_state);
	int (*reschedule)(struct nvgpu_channel *ch, bool preempt_next);
	int (*reschedule_preempt_next_locked)(struct nvgpu_channel *ch,
			bool wait_preempt);
	void (*init_enginfo)(struct gk20a *g, struct nvgpu_fifo *f);
#if defined(CONFIG_NVGPU_HAL_NON_FUSA) && defined(CONFIG_NVGPU_NEXT)
#include "include/nvgpu/nvgpu_next_gops_runlist.h"
#endif

	/** @endcond DOXYGEN_SHOULD_SKIP_THIS */
};

#endif
