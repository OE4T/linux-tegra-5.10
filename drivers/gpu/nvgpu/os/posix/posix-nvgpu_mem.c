/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/dma.h>
#include <nvgpu/gmmu.h>
#include <nvgpu/kmem.h>
#include <nvgpu/nvgpu_mem.h>
#include <nvgpu/nvgpu_sgt.h>
#include <nvgpu/nvgpu_sgt_os.h>
#include <nvgpu/gk20a.h>
#include <os/posix/os_posix.h>

#define DMA_ERROR_CODE	(~(u64)0x0)

/*
 * These functions are somewhat meaningless.
 */
u64 nvgpu_mem_get_addr(struct gk20a *g, struct nvgpu_mem *mem)
{
	return (u64)(uintptr_t)mem->cpu_va;
}

u64 nvgpu_mem_get_phys_addr(struct gk20a *g, struct nvgpu_mem *mem)
{
	return (u64)(uintptr_t)mem->cpu_va;
}

struct nvgpu_sgl *nvgpu_mem_sgl_next(void *sgl)
{
	struct nvgpu_mem_sgl *mem = (struct nvgpu_mem_sgl *)sgl;

	return (struct nvgpu_sgl *) mem->next;
}

u64 nvgpu_mem_sgl_phys(struct gk20a *g, void *sgl)
{
	struct nvgpu_mem_sgl *mem = (struct nvgpu_mem_sgl *)sgl;

	return (u64)(uintptr_t)mem->phys;
}

u64 nvgpu_mem_sgl_ipa_to_pa(struct gk20a *g, struct nvgpu_sgl *sgl,
		u64 ipa, u64 *pa_len)
{
	return nvgpu_mem_sgl_phys(g, sgl);
}

u64 nvgpu_mem_sgl_dma(void *sgl)
{
	struct nvgpu_mem_sgl *mem = (struct nvgpu_mem_sgl *)sgl;

	return (u64)(uintptr_t)mem->dma;
}

u64 nvgpu_mem_sgl_length(void *sgl)
{
	struct nvgpu_mem_sgl *mem = (struct nvgpu_mem_sgl *)sgl;

	return (u64)mem->length;
}

u64 nvgpu_mem_sgl_gpu_addr(struct gk20a *g, void *sgl,
				  struct nvgpu_gmmu_attrs *attrs)
{
	struct nvgpu_mem_sgl *mem = (struct nvgpu_mem_sgl *)sgl;

	if (mem->dma == 0U) {
		return g->ops.mm.gmmu.gpu_phys_addr(g, attrs, mem->phys);
	}

	if (mem->dma == DMA_ERROR_CODE) {
		return 0x0;
	}

	return nvgpu_mem_iommu_translate(g, mem->dma);
}

bool nvgpu_mem_sgt_iommuable(struct gk20a *g, struct nvgpu_sgt *sgt)
{
	struct nvgpu_os_posix *p = nvgpu_os_posix_from_gk20a(g);

	return p->mm_sgt_is_iommuable;
}

void nvgpu_mem_sgl_free(struct gk20a *g, struct nvgpu_mem_sgl *sgl)
{
	struct nvgpu_mem_sgl *tptr;

	while (sgl != NULL) {
		tptr = sgl->next;
		nvgpu_kfree(g, sgl);
		sgl = tptr;
	}
}

void nvgpu_mem_sgt_free(struct gk20a *g, struct nvgpu_sgt *sgt)
{
	nvgpu_mem_sgl_free(g, (struct nvgpu_mem_sgl *)sgt->sgl);
	nvgpu_kfree(g, sgt);
}

static struct nvgpu_sgt_ops nvgpu_sgt_posix_ops = {
	.sgl_next	= nvgpu_mem_sgl_next,
	.sgl_phys	= nvgpu_mem_sgl_phys,
	.sgl_ipa    = nvgpu_mem_sgl_phys,
	.sgl_ipa_to_pa = nvgpu_mem_sgl_ipa_to_pa,
	.sgl_dma	= nvgpu_mem_sgl_dma,
	.sgl_length	= nvgpu_mem_sgl_length,
	.sgl_gpu_addr	= nvgpu_mem_sgl_gpu_addr,
	.sgt_iommuable	= nvgpu_mem_sgt_iommuable,
	.sgt_free	= nvgpu_mem_sgt_free,
};

struct nvgpu_mem_sgl *nvgpu_mem_sgl_posix_create_from_list(struct gk20a *g,
				struct nvgpu_mem_sgl *sgl_list, u32 nr_sgls,
				u64 *total_size)
{
	struct nvgpu_mem_sgl *sgl_ptr, *tptr, *head = NULL;
	u32 i;

	*total_size = 0;
	for (i = 0; i < nr_sgls; i++) {
		tptr = (struct nvgpu_mem_sgl *)nvgpu_kzalloc(g,
						sizeof(struct nvgpu_mem_sgl));
		if (tptr == NULL) {
			return NULL;
		}

		if (i == 0U) {
			sgl_ptr = tptr;
			head = sgl_ptr;
		} else {
			sgl_ptr->next = tptr;
			sgl_ptr = sgl_ptr->next;
		}
		sgl_ptr->next = NULL;
		sgl_ptr->phys = sgl_list[i].phys;
		sgl_ptr->dma = sgl_list[i].dma;
		sgl_ptr->length = sgl_list[i].length;
		*total_size += sgl_list[i].length;
	}

	return head;
}

struct nvgpu_sgt *nvgpu_mem_sgt_posix_create_from_list(struct gk20a *g,
				struct nvgpu_mem_sgl *sgl_list, u32 nr_sgls,
				u64 *total_size)
{
	struct nvgpu_sgt *sgt = nvgpu_kzalloc(g, sizeof(struct nvgpu_sgt));
	struct nvgpu_mem_sgl *sgl;

	if (sgt == NULL) {
		return NULL;
	}

	sgl = nvgpu_mem_sgl_posix_create_from_list(g, sgl_list, nr_sgls,
				total_size);
	if (sgl == NULL) {
		nvgpu_kfree(g, sgt);
		return NULL;
	}
	sgt->sgl = (struct nvgpu_sgl *)sgl;
	sgt->ops = &nvgpu_sgt_posix_ops;

	return sgt;
}

int nvgpu_mem_posix_create_from_list(struct gk20a *g, struct nvgpu_mem *mem,
				struct nvgpu_mem_sgl *sgl_list, u32 nr_sgls)
{
	u64 sgl_size;

	mem->priv.sgt = nvgpu_mem_sgt_posix_create_from_list(g, sgl_list,
				nr_sgls, &sgl_size);
	if (mem->priv.sgt == NULL) {
		return -ENOMEM;
	}

	mem->aperture = APERTURE_SYSMEM;
	mem->aligned_size = PAGE_ALIGN(sgl_size);
	mem->size = sgl_size;

	return 0;
}

struct nvgpu_sgt *nvgpu_sgt_os_create_from_mem(struct gk20a *g,
					       struct nvgpu_mem *mem)
{
	struct nvgpu_mem_sgl *sgl;
	struct nvgpu_sgt *sgt;

	if (mem->priv.sgt != NULL) {
		return mem->priv.sgt;
	}

	sgt = nvgpu_kzalloc(g, sizeof(*sgt));
	if (sgt == NULL) {
		return NULL;
	}

	sgt->ops = &nvgpu_sgt_posix_ops;

	/*
	 * The userspace implementation is simple: a single 'entry' (which we
	 * only need the nvgpu_mem_sgl struct to describe). A unit test can
	 * easily replace it if needed.
	 */
	sgl = (struct nvgpu_mem_sgl *) nvgpu_kzalloc(g, sizeof(
		struct nvgpu_mem_sgl));
	if (sgl == NULL) {
		nvgpu_kfree(g, sgt);
		return NULL;
	}

	sgl->length = mem->size;
	sgl->phys   = (u64) mem->cpu_va;
	sgt->sgl    = (struct nvgpu_sgl *) sgl;

	return sgt;
}

int nvgpu_mem_create_from_mem(struct gk20a *g,
			      struct nvgpu_mem *dest, struct nvgpu_mem *src,
			      u64 start_page, int nr_pages)
{
	u64 start = start_page * U64(PAGE_SIZE);
	u64 size = U64(nr_pages) * U64(PAGE_SIZE);

	if (src->aperture != APERTURE_SYSMEM) {
		return -EINVAL;
	}

	/* Some silly things a caller might do... */
	if (size > src->size) {
		return -EINVAL;
	}
	if ((start + size) > src->size) {
		return -EINVAL;
	}

	(void) memset(dest, 0, sizeof(*dest));

	dest->cpu_va    = ((char *)src->cpu_va) + start;
	dest->mem_flags = src->mem_flags | NVGPU_MEM_FLAG_SHADOW_COPY;
	dest->aperture  = src->aperture;
	dest->skip_wmb  = src->skip_wmb;
	dest->size      = size;

	return 0;
}

int __nvgpu_mem_create_from_phys(struct gk20a *g, struct nvgpu_mem *dest,
				 u64 src_phys, int nr_pages)
{
	BUG();
	return 0;
}
