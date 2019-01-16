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

#include <nvgpu/bug.h>
#include <nvgpu/kmem.h>
#include <nvgpu/nvgpu_mem.h>
#include <nvgpu/dma.h>
#include <nvgpu/vidmem.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/string.h>

/*
 * Make sure to use the right coherency aperture if you use this function! This
 * will not add any checks. If you want to simply use the default coherency then
 * use nvgpu_aperture_mask().
 */
u32 nvgpu_aperture_mask_coh(struct gk20a *g, enum nvgpu_aperture aperture,
			    u32 sysmem_mask, u32 sysmem_coh_mask,
			    u32 vidmem_mask)
{
	/*
	 * Some iGPUs treat sysmem (i.e SoC DRAM) as vidmem. In these cases the
	 * "sysmem" aperture should really be translated to VIDMEM.
	 */
	if (!nvgpu_is_enabled(g, NVGPU_MM_HONORS_APERTURE)) {
		aperture = APERTURE_VIDMEM;
	}

	switch (aperture) {
	case APERTURE_SYSMEM_COH:
		return sysmem_coh_mask;
	case APERTURE_SYSMEM:
		return sysmem_mask;
	case APERTURE_VIDMEM:
		return vidmem_mask;
	case APERTURE_INVALID:
		(void)WARN(true, "Bad aperture");
	}
	return 0;
}

u32 nvgpu_aperture_mask(struct gk20a *g, struct nvgpu_mem *mem,
			u32 sysmem_mask, u32 sysmem_coh_mask, u32 vidmem_mask)
{
	enum nvgpu_aperture ap = mem->aperture;

	/*
	 * Handle the coherent aperture: ideally most of the driver is not
	 * aware of the difference between coherent and non-coherent sysmem so
	 * we add this translation step here.
	 */
	if (nvgpu_is_enabled(g, NVGPU_USE_COHERENT_SYSMEM) &&
	    ap == APERTURE_SYSMEM) {
		ap = APERTURE_SYSMEM_COH;
	}

	return nvgpu_aperture_mask_coh(g, ap,
				       sysmem_mask,
				       sysmem_coh_mask,
				       vidmem_mask);
}

bool nvgpu_aperture_is_sysmem(enum nvgpu_aperture ap)
{
	return ap == APERTURE_SYSMEM_COH || ap == APERTURE_SYSMEM;
}

bool nvgpu_mem_is_sysmem(struct nvgpu_mem *mem)
{
	return nvgpu_aperture_is_sysmem(mem->aperture);
}

u64 nvgpu_mem_iommu_translate(struct gk20a *g, u64 phys)
{
	/* ensure it is not vidmem allocation */
	WARN_ON(nvgpu_addr_is_vidmem_page_alloc(phys));

	if (nvgpu_iommuable(g) && g->ops.mm.get_iommu_bit != NULL) {
		return phys | 1ULL << g->ops.mm.get_iommu_bit(g);
	}

	return phys;
}

u32 nvgpu_mem_rd32(struct gk20a *g, struct nvgpu_mem *mem, u32 w)
{
	u32 data = 0;

	if (mem->aperture == APERTURE_SYSMEM) {
		u32 *ptr = mem->cpu_va;

		WARN_ON(ptr == NULL);
		data = ptr[w];
	} else if (mem->aperture == APERTURE_VIDMEM) {
		nvgpu_pramin_rd_n(g, mem, w * (u32)sizeof(u32),
				(u32)sizeof(u32), &data);
	} else {
		(void)WARN(true, "Accessing unallocated nvgpu_mem");
	}

	return data;
}

u64 nvgpu_mem_rd32_pair(struct gk20a *g, struct nvgpu_mem *mem, u32 lo, u32 hi)
{
	u64 lo_data = U64(nvgpu_mem_rd32(g, mem, lo));
	u64 hi_data = U64(nvgpu_mem_rd32(g, mem, hi));

	return lo_data | (hi_data << 32ULL);
}

u32 nvgpu_mem_rd(struct gk20a *g, struct nvgpu_mem *mem, u32 offset)
{
	WARN_ON((offset & 3U) != 0U);
	return nvgpu_mem_rd32(g, mem, offset / (u32)sizeof(u32));
}

void nvgpu_mem_rd_n(struct gk20a *g, struct nvgpu_mem *mem,
		u32 offset, void *dest, u32 size)
{
	WARN_ON((offset & 3U) != 0U);
	WARN_ON((size & 3U) != 0U);

	if (mem->aperture == APERTURE_SYSMEM) {
		u8 *src = (u8 *)mem->cpu_va + offset;

		WARN_ON(mem->cpu_va == NULL);
		nvgpu_memcpy((u8 *)dest, src, size);
	} else if (mem->aperture == APERTURE_VIDMEM) {
		nvgpu_pramin_rd_n(g, mem, offset, size, dest);
	} else {
		(void)WARN(true, "Accessing unallocated nvgpu_mem");
	}
}

void nvgpu_mem_wr32(struct gk20a *g, struct nvgpu_mem *mem, u32 w, u32 data)
{
	if (mem->aperture == APERTURE_SYSMEM) {
		u32 *ptr = mem->cpu_va;

		WARN_ON(ptr == NULL);
		ptr[w] = data;
	} else if (mem->aperture == APERTURE_VIDMEM) {
		nvgpu_pramin_wr_n(g, mem, w * (u32)sizeof(u32),
				  (u32)sizeof(u32), &data);
		if (!mem->skip_wmb) {
			nvgpu_wmb();
		}
	} else {
		(void)WARN(true, "Accessing unallocated nvgpu_mem");
	}
}

void nvgpu_mem_wr(struct gk20a *g, struct nvgpu_mem *mem, u32 offset, u32 data)
{
	WARN_ON((offset & 3U) != 0U);
	nvgpu_mem_wr32(g, mem, offset / (u32)sizeof(u32), data);
}

void nvgpu_mem_wr_n(struct gk20a *g, struct nvgpu_mem *mem, u32 offset,
		void *src, u32 size)
{
	WARN_ON((offset & 3U) != 0U);
	WARN_ON((size & 3U) != 0U);

	if (mem->aperture == APERTURE_SYSMEM) {
		u8 *dest = (u8 *)mem->cpu_va + offset;

		WARN_ON(mem->cpu_va == NULL);
		nvgpu_memcpy(dest, (u8 *)src, size);
	} else if (mem->aperture == APERTURE_VIDMEM) {
		nvgpu_pramin_wr_n(g, mem, offset, size, src);
		if (!mem->skip_wmb) {
			nvgpu_wmb();
		}
	} else {
		(void)WARN(true, "Accessing unallocated nvgpu_mem");
	}
}

void nvgpu_memset(struct gk20a *g, struct nvgpu_mem *mem, u32 offset,
		u32 c, u32 size)
{
	WARN_ON((offset & 3U) != 0U);
	WARN_ON((size & 3U) != 0U);
	WARN_ON((c & ~0xffU) != 0U);

	c &= 0xffU;

	if (mem->aperture == APERTURE_SYSMEM) {
		u8 *dest = (u8 *)mem->cpu_va + offset;

		WARN_ON(mem->cpu_va == NULL);
		(void) memset(dest, (int)c, size);
	} else if (mem->aperture == APERTURE_VIDMEM) {
		u32 repeat_value = c | (c << 8) | (c << 16) | (c << 24);

		nvgpu_pramin_memset(g, mem, offset, size, repeat_value);
		if (!mem->skip_wmb) {
			nvgpu_wmb();
		}
	} else {
		(void)WARN(true, "Accessing unallocated nvgpu_mem");
	}
}
