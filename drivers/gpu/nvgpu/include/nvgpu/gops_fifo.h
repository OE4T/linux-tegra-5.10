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
#ifndef NVGPU_GOPS_FIFO_H
#define NVGPU_GOPS_FIFO_H

#include <nvgpu/types.h>

struct gk20a;
struct nvgpu_channel;
struct nvgpu_tsg;
struct mmu_fault_info;

struct gops_fifo {
	int (*fifo_init_support)(struct gk20a *g);
	int (*fifo_suspend)(struct gk20a *g);
	int (*setup_sw)(struct gk20a *g);
	void (*cleanup_sw)(struct gk20a *g);
	int (*init_fifo_setup_hw)(struct gk20a *g);
	int (*preempt_channel)(struct gk20a *g, struct nvgpu_channel *ch);
	int (*preempt_tsg)(struct gk20a *g, struct nvgpu_tsg *tsg);
	void (*preempt_runlists_for_rc)(struct gk20a *g,
			u32 runlists_bitmask);
	void (*preempt_trigger)(struct gk20a *g,
			u32 id, unsigned int id_type);
	int (*preempt_poll_pbdma)(struct gk20a *g, u32 tsgid,
			 u32 pbdma_id);
	void (*init_pbdma_map)(struct gk20a *g,
			u32 *pbdma_map, u32 num_pbdma);
	int (*is_preempt_pending)(struct gk20a *g, u32 id,
		unsigned int id_type);
	int (*reset_enable_hw)(struct gk20a *g);
#ifdef CONFIG_NVGPU_RECOVERY
	void (*recover)(struct gk20a *g, u32 act_eng_bitmask,
		u32 id, unsigned int id_type, unsigned int rc_type,
		 struct mmu_fault_info *mmfault);
#endif
	void (*intr_set_recover_mask)(struct gk20a *g);
	void (*intr_unset_recover_mask)(struct gk20a *g);
#ifdef CONFIG_NVGPU_DEBUGGER
	int (*set_sm_exception_type_mask)(struct nvgpu_channel *ch,
			u32 exception_mask);
#endif
	void (*intr_0_enable)(struct gk20a *g, bool enable);
	void (*intr_0_isr)(struct gk20a *g);
	void (*intr_1_enable)(struct gk20a *g, bool enable);
	u32  (*intr_1_isr)(struct gk20a *g);
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

};

#endif
