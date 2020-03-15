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
#ifndef NVGPU_GOPS_FIFO_H
#define NVGPU_GOPS_FIFO_H

#include <nvgpu/types.h>

/**
 * @file
 *
 * FIFO HAL interface.
 */
struct gk20a;
struct nvgpu_channel;
struct nvgpu_tsg;
struct mmu_fault_info;

struct gops_fifo {
	/**
 	 * @brief Initialize FIFO unit.
 	 *
 	 * @param g [in]	Pointer to GPU driver struct.
 	 *
	 * This HAL is used to initialize FIFO software context,
	 * then do GPU h/w initializations. It always maps to
	 * #nvpgu_fifo_init_support, except for vgpu case.
	 *
 	 * @return 0 in case of success, < 0 in case of failure.
 	 */
	int (*fifo_init_support)(struct gk20a *g);

	/**
 	 * @brief Suspend FIFO unit.
 	 *
 	 * @param g [in]	Pointer to GPU driver struct.
 	 *
	 * - Disable BAR1 snooping when supported.
	 * - Disable FIFO interrupts
	 *   - Disable FIFO stalling interrupts
	 *   - Disable ctxsw timeout detection, and clear any pending
	 *     ctxsw timeout interrupt.
	 *   - Disable PBDMA interrupts.
	 *   - Disable FIFO non-stalling interrupts.
	 *
 	 * @return 0 in case of success, < 0 in case of failure.
 	 */
	int (*fifo_suspend)(struct gk20a *g);

	/**
 	 * @brief Preempt TSG.
 	 *
 	 * @param g [in]	Pointer to GPU driver struct.
	 * @param tsg [in]	Pointer to TSG struct.
 	 *
	 * Preempt TSG:
	 * - Acquire lock for active runlist.
	 * - Write h/w register to trigger TSG preempt for \a tsg.
	 * - Preemption mode (e.g. CTA or WFI) depends on the preemption
	 *   mode configured in the GR context.
	 * - Release lock acquired for active runlist.
	 * - Poll PBDMAs and engines status until preemption is complete,
	 *   or poll timeout occurs.
	 *
	 * On some chips, it is also needed to disable scheduling
	 * before preempting TSG.
	 *
	 * @see nvgpu_preempt_get_timeout
	 * @see nvgpu_gr_ctx::compute_preempt_mode
	 *
 	 * @return 0 in case preemption succeeded, < 0 in case of failure.
	 * @retval -ETIMEDOUT when preemption was triggered, but did not
	 *         complete within preemption poll timeout.
 	 */
	int (*preempt_tsg)(struct gk20a *g, struct nvgpu_tsg *tsg);

	/**
 	 * @brief Preempt a set of runlists.
 	 *
 	 * @param g [in]		Pointer to GPU driver struct.
	 * @param runlists_mask [in]	Bitmask of runlists to preempt.
 	 *
	 * Preempt runlists in \a runlists_mask:
	 * - Write h/w register to trigger preempt on runlists.
	 * - All TSG in those runlists are preempted.
	 *
	 * @note This HAL is called in case of critical error, and does
	 * not poll PBDMAs or engines to wait for preempt completion.
	 *
	 * @note This HAL should be called with runlist lock held for all
	 * the runlists in \a runlists_mask.
 	 */
	void (*preempt_runlists_for_rc)(struct gk20a *g,
			u32 runlists_bitmask);

	/**
	 * @brief Enable and configure FIFO.
	 *
	 * @param g [in]		Pointer to GPU driver struct.
	 *
	 * Enable and configure h/w settings for FIFO:
	 * - Enable PMC FIFO.
	 * - Configure clock gating:
	 *   - Set SLCG settings for CE2 and FIFO.
	 *   - Set BLCG settings for FIFO.
	 * - Set FB timeout for FIFO initiated requests.
	 * - Setup PBDMA timeouts.
	 * - Enable FIFO unit stalling and non-stalling interrupts at MC level.
	 * - Enable FIFO stalling and non-stalling interrupts.
	 *
	 * @return 0 in case of success, < 0 in case of failure.
	 */
	int (*reset_enable_hw)(struct gk20a *g);

	/**
	 * @brief ISR for stalling interrupts.
	 *
	 * @param g [in]		Pointer to GPU driver struct.
	 *
	 * Interrupt Service Routine for FIFO stalling interrupts:
	 * - Read interrupt status.
	 * - If sw_ready is false, clear interrupts and return, else
	 * - Acquire FIFO ISR mutex
	 * - Handle interrupts:
	 *  - Handle error interrupts:
	 *    - Report bind, chw, memop timeout and lb errors.
	 *  - Handle runlist event interrupts:
	 *    - Log and clear runlist events.
	 *  - Handle PBDMA interrupts:
	 *    - Set error notifier and reset method (if needed).
	 *    - Report timeout, extra, pb, method, signature, hce and
	 *      preempt errors.
	 *  - Handle scheduling errors interrupts:
	 *    - Log and report sched error.
	 *  - Handle ctxsw timeout interrupts:
	 *    - Get engines with ctxsw timeout.
	 *    - Report error for TSGs on those engines.
	 * - Release FIFO ISR mutex.
	 * - Clear interrupts.
	 *
	 * @note: This HAL is called from a threaded interrupt context.
	 */
	void (*intr_0_isr)(struct gk20a *g);

	/**
	 * @brief ISR for non-stalling interrupts.
	 *
	 * @param g [in]		Pointer to GPU driver struct.
	 *
	 * Interrupt Service Routine for FIFO non-stalling interrupts:
	 * - Read interrupt status.
	 * - Clear channel interrupt if pending.
	 *
	 * @return: #NVGPU_NONSTALL_OPS_WAKEUP_SEMAPHORE
	 */
	u32  (*intr_1_isr)(struct gk20a *g);

	/** @cond DOXYGEN_SHOULD_SKIP_THIS */

	int (*setup_sw)(struct gk20a *g);
	void (*cleanup_sw)(struct gk20a *g);
	int (*init_fifo_setup_hw)(struct gk20a *g);
	int (*preempt_channel)(struct gk20a *g, struct nvgpu_channel *ch);
	void (*preempt_trigger)(struct gk20a *g,
			u32 id, unsigned int id_type);
	int (*preempt_poll_pbdma)(struct gk20a *g, u32 tsgid,
			 u32 pbdma_id);
	void (*init_pbdma_map)(struct gk20a *g,
			u32 *pbdma_map, u32 num_pbdma);
	int (*is_preempt_pending)(struct gk20a *g, u32 id,
		unsigned int id_type);
	void (*intr_set_recover_mask)(struct gk20a *g);
	void (*intr_unset_recover_mask)(struct gk20a *g);
	void (*intr_top_enable)(struct gk20a *g, bool enable);
	void (*intr_0_enable)(struct gk20a *g, bool enable);
	void (*intr_1_enable)(struct gk20a *g, bool enable);
	bool (*handle_sched_error)(struct gk20a *g);
	void (*ctxsw_timeout_enable)(struct gk20a *g, bool enable);
	bool (*handle_ctxsw_timeout)(struct gk20a *g);
	void (*trigger_mmu_fault)(struct gk20a *g,
			unsigned long engine_ids_bitmask);
	void (*get_mmu_fault_info)(struct gk20a *g, u32 mmu_fault_id,
		struct mmu_fault_info *mmfault);
	void (*get_mmu_fault_desc)(struct mmu_fault_info *mmfault);
	void (*get_mmu_fault_client_desc)(
				struct mmu_fault_info *mmfault);
	void (*get_mmu_fault_gpc_desc)(struct mmu_fault_info *mmfault);
	u32 (*get_runlist_timeslice)(struct gk20a *g);
	u32 (*get_pb_timeslice)(struct gk20a *g);
	bool (*is_mmu_fault_pending)(struct gk20a *g);
	u32  (*mmu_fault_id_to_pbdma_id)(struct gk20a *g,
				u32 mmu_fault_id);
	void (*bar1_snooping_disable)(struct gk20a *g);

#ifdef CONFIG_NVGPU_RECOVERY
	void (*recover)(struct gk20a *g, u32 act_eng_bitmask,
		u32 id, unsigned int id_type, unsigned int rc_type,
		 struct mmu_fault_info *mmfault);
#endif

#ifdef CONFIG_NVGPU_DEBUGGER
	int (*set_sm_exception_type_mask)(struct nvgpu_channel *ch,
			u32 exception_mask);
#endif

	/** @endcond DOXYGEN_SHOULD_SKIP_THIS */

};

#endif
