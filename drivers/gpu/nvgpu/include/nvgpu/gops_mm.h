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

#ifndef NVGPU_GOPS_MM_H
#define NVGPU_GOPS_MM_H

#include <nvgpu/types.h>

struct gk20a;

struct gops_mm {
	int (*pd_cache_init)(struct gk20a *g);
	int (*init_mm_support)(struct gk20a *g);
	int (*mm_suspend)(struct gk20a *g);
	int (*vm_bind_channel)(struct vm_gk20a *vm, struct nvgpu_channel *ch);
	int (*setup_hw)(struct gk20a *g);
	bool (*is_bar1_supported)(struct gk20a *g);
	int (*init_bar2_vm)(struct gk20a *g);
	void (*remove_bar2_vm)(struct gk20a *g);
	void (*init_inst_block)(struct nvgpu_mem *inst_block,
			struct vm_gk20a *vm, u32 big_page_size);
	u32 (*get_flush_retries)(struct gk20a *g, enum nvgpu_flush_op op);
	u64 (*bar1_map_userd)(struct gk20a *g, struct nvgpu_mem *mem,
								u32 offset);
	int (*vm_as_alloc_share)(struct gk20a *g, struct vm_gk20a *vm);
	void (*vm_as_free_share)(struct vm_gk20a *vm);
	struct {
		int (*setup_sw)(struct gk20a *g);
		void (*setup_hw)(struct gk20a *g);
		void (*info_mem_destroy)(struct gk20a *g);
		void (*disable_hw)(struct gk20a *g);
	} mmu_fault;
	struct {
		int (*fb_flush)(struct gk20a *g);
		void (*l2_invalidate)(struct gk20a *g);
		int (*l2_flush)(struct gk20a *g, bool invalidate);
#ifdef CONFIG_NVGPU_COMPRESSION
		void (*cbc_clean)(struct gk20a *g);
#endif
	} cache;
	struct {
		const struct gk20a_mmu_level *
			(*get_mmu_levels)(struct gk20a *g, u64 big_page_size);
		u64 (*map)(struct vm_gk20a *vm,
			   u64 map_offset,
			   struct nvgpu_sgt *sgt,
			   u64 buffer_offset,
			   u64 size,
			   u32 pgsz_idx,
			   u8 kind_v,
			   u32 ctag_offset,
			   u32 flags,
			   enum gk20a_mem_rw_flag rw_flag,
			   bool clear_ctags,
			   bool sparse,
			   bool priv,
			   struct vm_gk20a_mapping_batch *batch,
			   enum nvgpu_aperture aperture);
		void (*unmap)(struct vm_gk20a *vm,
			      u64 vaddr,
			      u64 size,
			      u32 pgsz_idx,
			      bool va_allocated,
			      enum gk20a_mem_rw_flag rw_flag,
			      bool sparse,
			      struct vm_gk20a_mapping_batch *batch);
		u32 (*get_big_page_sizes)(void);
		u32 (*get_default_big_page_size)(void);
		u32 (*get_iommu_bit)(struct gk20a *g);
		u64 (*gpu_phys_addr)(struct gk20a *g,
				     struct nvgpu_gmmu_attrs *attrs,
				     u64 phys);
	} gmmu;
};

#endif
