/*
 * Copyright (c) 2017-2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/pmu/allocator.h>
#include <nvgpu/allocator.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/pmu.h>
#include <nvgpu/pmu/fw.h>

void nvgpu_pmu_dmem_allocator_init(struct gk20a *g,
	struct nvgpu_pmu *pmu, struct nvgpu_allocator *dmem,
	union pmu_init_msg_pmu *init)
{
	struct pmu_fw_ver_ops *fw_ops = &pmu->fw.ops;

	if (!nvgpu_alloc_initialized(dmem)) {
		/* Align start and end addresses */
		u32 start =
			ALIGN(U32(fw_ops->get_init_msg_sw_mngd_area_off(init)),
			PMU_DMEM_ALLOC_ALIGNMENT);
		u32 end = (U32(fw_ops->get_init_msg_sw_mngd_area_off(init)) +
			U32(fw_ops->get_init_msg_sw_mngd_area_size(init))) &
			~(PMU_DMEM_ALLOC_ALIGNMENT - 1U);
		u32 size = end - start;

		nvgpu_allocator_init(g, dmem, NULL, "gk20a_pmu_dmem", start,
				size, PMU_DMEM_ALLOC_ALIGNMENT, 0ULL, 0ULL,
				BITMAP_ALLOCATOR);
	}
}

void nvgpu_pmu_dmem_allocator_destroy(struct nvgpu_allocator *dmem)
{
	if (nvgpu_alloc_initialized(dmem)) {
		nvgpu_alloc_destroy(dmem);
	}
}
