/*
 * Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef __FB_TU104_H__
#define __FB_TU104_H__

#include <nvgpu/types.h>

struct gk20a;
struct gr_gk20a;
struct nvgpu_mem;

void tu104_fb_enable_hub_intr(struct gk20a *g);
void tu104_fb_disable_hub_intr(struct gk20a *g);
bool tu104_fb_mmu_fault_pending(struct gk20a *g);
void tu104_fb_hub_isr(struct gk20a *g);

void fb_tu104_write_mmu_fault_buffer_lo_hi(struct gk20a *g, u32 index,
	u32 addr_lo, u32 addr_hi);
u32 fb_tu104_read_mmu_fault_buffer_get(struct gk20a *g, u32 index);
void fb_tu104_write_mmu_fault_buffer_get(struct gk20a *g, u32 index,
	u32 reg_val);
u32 fb_tu104_read_mmu_fault_buffer_put(struct gk20a *g, u32 index);
u32 fb_tu104_read_mmu_fault_buffer_size(struct gk20a *g, u32 index);
void fb_tu104_write_mmu_fault_buffer_size(struct gk20a *g, u32 index,
	u32 reg_val);
void fb_tu104_read_mmu_fault_addr_lo_hi(struct gk20a *g,
	u32 *addr_lo, u32 *addr_hi);
void fb_tu104_read_mmu_fault_inst_lo_hi(struct gk20a *g,
	u32 *inst_lo, u32 *inst_hi);
u32 fb_tu104_read_mmu_fault_info(struct gk20a *g);
u32 fb_tu104_read_mmu_fault_status(struct gk20a *g);
void fb_tu104_write_mmu_fault_status(struct gk20a *g, u32 reg_val);

int fb_tu104_tlb_invalidate(struct gk20a *g, struct nvgpu_mem *pdb);
int fb_tu104_mmu_invalidate_replay(struct gk20a *g,
	u32 invalidate_replay_val);

void fb_tu104_init_cbc(struct gk20a *g, struct gr_gk20a *gr);

int tu104_fb_apply_pdb_cache_war(struct gk20a *g);

#endif
