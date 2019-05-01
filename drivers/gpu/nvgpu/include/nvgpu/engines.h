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

#ifndef NVGPU_ENGINE_H
#define NVGPU_ENGINE_H

#include <nvgpu/types.h>

#define NVGPU_INVALID_ENG_ID		(~U32(0U))

struct gk20a;
struct nvgpu_fifo;

enum nvgpu_fifo_engine {
	NVGPU_ENGINE_GR        = 0U,
	NVGPU_ENGINE_GRCE      = 1U,
	NVGPU_ENGINE_ASYNC_CE  = 2U,
	NVGPU_ENGINE_INVAL     = 3U,
};

struct nvgpu_pbdma_exception_info {
	u32 status_r; /* raw register value from hardware */
	u32 id, next_id;
	u32 chan_status_v; /* raw value from hardware */
	bool id_is_chid, next_id_is_chid;
	bool chsw_in_progress;
};

struct nvgpu_engine_exception_info {
	u32 status_r; /* raw register value from hardware */
	u32 id, next_id;
	u32 ctx_status_v; /* raw value from hardware */
	bool id_is_chid, next_id_is_chid;
	bool faulted, idle, ctxsw_in_progress;
};

struct nvgpu_engine_info {
	u32 engine_id;
	u32 runlist_id;
	u32 intr_mask;
	u32 reset_mask;
	u32 pbdma_id;
	u32 inst_id;
	u32 pri_base;
	u32 fault_id;
	enum nvgpu_fifo_engine engine_enum;
	struct nvgpu_pbdma_exception_info pbdma_exception_info;
	struct nvgpu_engine_exception_info engine_exception_info;
};

enum nvgpu_fifo_engine nvgpu_engine_enum_from_type(struct gk20a *g,
		u32 engine_type);

struct nvgpu_engine_info *nvgpu_engine_get_active_eng_info(
		struct gk20a *g, u32 engine_id);

u32 nvgpu_engine_get_ids(struct gk20a *g,
		u32 *engine_ids, u32 engine_id_sz,
		enum nvgpu_fifo_engine engine_enum);

bool nvgpu_engine_check_valid_id(struct gk20a *g, u32 engine_id);
u32 nvgpu_engine_get_gr_id(struct gk20a *g);
u32 nvgpu_engine_interrupt_mask(struct gk20a *g);
u32 nvgpu_engine_act_interrupt_mask(struct gk20a *g, u32 act_eng_id);
u32 nvgpu_engine_get_all_ce_reset_mask(struct gk20a *g);
int nvgpu_engine_setup_sw(struct gk20a *g);
void nvgpu_engine_cleanup_sw(struct gk20a *g);

int nvgpu_engine_enable_activity(struct gk20a *g,
			struct nvgpu_engine_info *eng_info);
int nvgpu_engine_enable_activity_all(struct gk20a *g);
int nvgpu_engine_disable_activity(struct gk20a *g,
			struct nvgpu_engine_info *eng_info,
			bool wait_for_idle);
int nvgpu_engine_disable_activity_all(struct gk20a *g,
				bool wait_for_idle);

int nvgpu_engine_wait_for_idle(struct gk20a *g);
void nvgpu_engine_reset(struct gk20a *g, u32 engine_id);

u32 nvgpu_engine_get_fast_ce_runlist_id(struct gk20a *g);
u32 nvgpu_engine_get_gr_runlist_id(struct gk20a *g);
bool nvgpu_engine_is_valid_runlist_id(struct gk20a *g, u32 runlist_id);
u32 nvgpu_engine_id_to_mmu_fault_id(struct gk20a *g, u32 engine_id);
u32 nvgpu_engine_mmu_fault_id_to_engine_id(struct gk20a *g, u32 fault_id);

u32 nvgpu_engine_get_mask_on_id(struct gk20a *g, u32 id, bool is_tsg);
int nvgpu_engine_init_info(struct nvgpu_fifo *f);

void nvgpu_engine_get_id_and_type(struct gk20a *g, u32 engine_id,
					  u32 *id, u32 *type);
u32 nvgpu_engine_find_busy_doing_ctxsw(struct gk20a *g,
		u32 *id_ptr, bool *is_tsg_ptr);
u32 nvgpu_engine_get_runlist_busy_engines(struct gk20a *g, u32 runlist_id);

bool nvgpu_engine_should_defer_reset(struct gk20a *g, u32 engine_id,
			u32 engine_subid, bool fake_fault);
u32 nvgpu_engine_mmu_fault_id_to_veid(struct gk20a *g, u32 mmu_fault_id,
			u32 gr_eng_fault_id);
u32 nvgpu_engine_mmu_fault_id_to_eng_id_and_veid(struct gk20a *g,
			 u32 mmu_fault_id, u32 *veid);
void nvgpu_engine_mmu_fault_id_to_eng_ve_pbdma_id(struct gk20a *g,
	u32 mmu_fault_id, u32 *act_eng_id, u32 *veid, u32 *pbdma_id);
#endif /*NVGPU_ENGINE_H*/
