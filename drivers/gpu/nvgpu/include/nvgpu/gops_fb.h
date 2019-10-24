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
#ifndef NVGPU_GOPS_FB_H
#define NVGPU_GOPS_FB_H

#include <nvgpu/types.h>

/**
 * @file
 *
 * common.fb interface.
 */
struct gk20a;

/**
 * common.fb intr subunit hal operations.
 *
 * This structure stores common.fb interrupt subunit hal pointers.
 *
 * @see gpu_ops
 */
struct gops_fb_intr {
	void (*enable)(struct gk20a *g);
	void (*disable)(struct gk20a *g);
	void (*isr)(struct gk20a *g);
	bool (*is_mmu_fault_pending)(struct gk20a *g);
};

/**
 * common.fb unit hal operations.
 *
 * This structure stores common.fb unit hal pointers.
 *
 * @see gpu_ops
 */
struct gops_fb {

	void (*init_hw)(struct gk20a *g);
	void (*init_fs_state)(struct gk20a *g);
	void (*set_mmu_page_size)(struct gk20a *g);
	u32 (*mmu_ctrl)(struct gk20a *g);
	u32 (*mmu_debug_ctrl)(struct gk20a *g);
	u32 (*mmu_debug_wr)(struct gk20a *g);
	u32 (*mmu_debug_rd)(struct gk20a *g);
	void (*dump_vpr_info)(struct gk20a *g);
	void (*dump_wpr_info)(struct gk20a *g);
	int (*vpr_info_fetch)(struct gk20a *g);
	void (*read_wpr_info)(struct gk20a *g, u64 *wpr_base, u64 *wpr_size);
	int (*tlb_invalidate)(struct gk20a *g, struct nvgpu_mem *pdb);
	void (*fault_buf_configure_hw)(struct gk20a *g, u32 index);
	bool (*is_fault_buf_enabled)(struct gk20a *g, u32 index);
	void (*fault_buf_set_state_hw)(struct gk20a *g, u32 index, u32 state);
	u32 (*read_mmu_fault_buffer_get)(struct gk20a *g, u32 index);
	u32 (*read_mmu_fault_buffer_put)(struct gk20a *g, u32 index);
	u32 (*read_mmu_fault_buffer_size)(struct gk20a *g, u32 index);
	u32 (*read_mmu_fault_info)(struct gk20a *g);
	u32 (*read_mmu_fault_status)(struct gk20a *g);
	void (*write_mmu_fault_buffer_lo_hi)(struct gk20a *g, u32 index,
				u32 addr_lo, u32 addr_hi);
	void (*write_mmu_fault_buffer_get)(struct gk20a *g, u32 index,
				u32 reg_val);
	void (*write_mmu_fault_buffer_size)(struct gk20a *g, u32 index,
				u32 reg_val);
	void (*read_mmu_fault_addr_lo_hi)(struct gk20a *g,
				u32 *addr_lo, u32 *addr_hi);
	void (*read_mmu_fault_inst_lo_hi)(struct gk20a *g,
				u32 *inst_lo, u32 *inst_hi);
	void (*write_mmu_fault_status)(struct gk20a *g, u32 reg_val);
	int (*mmu_invalidate_replay)(struct gk20a *g,
						u32 invalidate_replay_val);

	struct gops_fb_intr intr;

	/** @cond DOXYGEN_SHOULD_SKIP_THIS */
	struct nvgpu_hw_err_inject_info_desc * (*get_hubmmu_err_desc)
							(struct gk20a *g);
#ifdef CONFIG_NVGPU_COMPRESSION
	void (*cbc_configure)(struct gk20a *g, struct nvgpu_cbc *cbc);
	bool (*set_use_full_comp_tag_line)(struct gk20a *g);

	/*
	 * Compression tag line coverage. When mapping a compressible
	 * buffer, ctagline is increased when the virtual address
	 * crosses over the compression page boundary.
	 */
	u64 (*compression_page_size)(struct gk20a *g);

	/*
	 * Minimum page size that can be used for compressible kinds.
	 */
	unsigned int (*compressible_page_size)(struct gk20a *g);

	/*
	 * Compressible kind mappings: Mask for the virtual and physical
	 * address bits that must match.
	 */
	u64 (*compression_align_mask)(struct gk20a *g);
#endif

#ifdef CONFIG_NVGPU_DEBUGGER
	bool (*is_debug_mode_enabled)(struct gk20a *g);
	void (*set_debug_mode)(struct gk20a *g, bool enable);
	void (*set_mmu_debug_mode)(struct gk20a *g, bool enable);
#endif

	void (*handle_replayable_fault)(struct gk20a *g);
	int (*mem_unlock)(struct gk20a *g);
	int (*init_nvlink)(struct gk20a *g);
	int (*enable_nvlink)(struct gk20a *g);

#ifdef CONFIG_NVGPU_DGPU
	size_t (*get_vidmem_size)(struct gk20a *g);
#endif
	int (*apply_pdb_cache_war)(struct gk20a *g);
	int (*init_fbpa)(struct gk20a *g);
	void (*handle_fbpa_intr)(struct gk20a *g, u32 fbpa_id);
	/** @endcond DOXYGEN_SHOULD_SKIP_THIS */
};

#endif /* NVGPU_GOPS_FB_H */

