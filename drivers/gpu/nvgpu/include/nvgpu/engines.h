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

struct gk20a;
struct fifo_engine_info_gk20a;

enum nvgpu_fifo_engine {
	NVGPU_ENGINE_GR_GK20A	     = 0U,
	NVGPU_ENGINE_GRCE_GK20A      = 1U,
	NVGPU_ENGINE_ASYNC_CE_GK20A  = 2U,
	NVGPU_ENGINE_INVAL_GK20A     = 3U,
};

enum nvgpu_fifo_engine nvgpu_engine_enum_from_type(struct gk20a *g,
		u32 engine_type);

struct fifo_engine_info_gk20a *nvgpu_engine_get_active_eng_info(
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
			struct fifo_engine_info_gk20a *eng_info);
int nvgpu_engine_enable_activity_all(struct gk20a *g);
int nvgpu_engine_disable_activity(struct gk20a *g,
			struct fifo_engine_info_gk20a *eng_info,
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

#endif /*NVGPU_ENGINE_H*/
