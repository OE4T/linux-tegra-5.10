/*
 *
 * Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/gk20a.h>
#include <nvgpu/lock.h>
#include <nvgpu/cic.h>

void nvgpu_cic_intr_unit_vectorid_init(struct gk20a *g, u32 unit, u32 *vectorid,
		u32 num_entries)
{
	unsigned long flags = 0;
	u32 i = 0U;
	struct nvgpu_intr_unit_info *intr_unit_info;

	nvgpu_assert(num_entries <= NVGPU_CIC_INTR_VECTORID_SIZE_MAX);

	nvgpu_log(g, gpu_dbg_intr, "UNIT=%d, nvecs=%d", unit, num_entries);

	intr_unit_info = g->mc.nvgpu_next.intr_unit_info;

	nvgpu_spinlock_irqsave(&g->mc.intr_lock, flags);

	if (intr_unit_info[unit].valid == false) {
		for (i = 0U; i < num_entries; i++) {
			nvgpu_log(g, gpu_dbg_intr, " vec[%d] = %d", i,
					*(vectorid + i));
			intr_unit_info[unit].vectorid[i] = *(vectorid + i);
		}
		intr_unit_info[unit].vectorid_size = num_entries;
	}
	nvgpu_spinunlock_irqrestore(&g->mc.intr_lock, flags);
}

bool nvgpu_cic_intr_is_unit_info_valid(struct gk20a *g, u32 unit)
{
	struct nvgpu_intr_unit_info *intr_unit_info;
	bool info_valid = false;

	if (unit >= NVGPU_CIC_INTR_UNIT_MAX) {
		nvgpu_err(g, "invalid unit(%d)", unit);
		return false;
	}

	intr_unit_info = g->mc.nvgpu_next.intr_unit_info;

	if (intr_unit_info[unit].valid == true) {
		info_valid = true;
	}

	return info_valid;
}

bool nvgpu_cic_intr_get_unit_info(struct gk20a *g, u32 unit, u32 *subtree,
		u64 *subtree_mask)
{
	if (unit >= NVGPU_CIC_INTR_UNIT_MAX) {
		nvgpu_err(g, "invalid unit(%d)", unit);
		return false;
	}
	if (nvgpu_cic_intr_is_unit_info_valid(g, unit) != true) {
		if (g->ops.mc.intr_get_unit_info(g, unit) != true) {
			nvgpu_err(g, "failed to fetch info for unit(%d)", unit);
			return false;
		}
	}
	*subtree = g->mc.nvgpu_next.intr_unit_info[unit].subtree;
	*subtree_mask = g->mc.nvgpu_next.intr_unit_info[unit].subtree_mask;
	nvgpu_log(g, gpu_dbg_intr, "subtree(%d) subtree_mask(%llx)",
			*subtree, *subtree_mask);

	return true;
}
