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

#ifndef NVGPU_SGT_H
#define NVGPU_SGT_H

#include <nvgpu/types.h>

struct gk20a;
struct nvgpu_mem;
struct nvgpu_gmmu_attrs;

struct nvgpu_sgt;

/*
 * Forward declared opaque placeholder type that does not really exist, but
 * helps the compiler help us about getting types right. In reality,
 * implementors of nvgpu_sgt_ops will have some concrete type in place of this.
 */
struct nvgpu_sgl;

struct nvgpu_sgt_ops {
	struct nvgpu_sgl *(*sgl_next)(void *sgl);
	u64   (*sgl_phys)(struct gk20a *g, void *sgl);
	u64   (*sgl_ipa)(struct gk20a *g, void *sgl);
	u64   (*sgl_ipa_to_pa)(struct gk20a *g, struct nvgpu_sgl *sgl,
			      u64 ipa, u64 *pa_len);
	u64   (*sgl_dma)(void *sgl);
	u64   (*sgl_length)(void *sgl);
	u64   (*sgl_gpu_addr)(struct gk20a *g, void *sgl,
			      struct nvgpu_gmmu_attrs *attrs);
	/*
	 * If left NULL then iommuable is assumed to be false.
	 */
	bool  (*sgt_iommuable)(struct gk20a *g, struct nvgpu_sgt *sgt);

	/*
	 * Note: this operates on the whole SGT not a specific SGL entry.
	 */
	void  (*sgt_free)(struct gk20a *g, struct nvgpu_sgt *sgt);
};

/*
 * Scatter gather table: this is a list of scatter list entries and the ops for
 * interacting with those entries.
 */
struct nvgpu_sgt {
	/*
	 * Ops for interacting with the underlying scatter gather list entries.
	 */
	const struct nvgpu_sgt_ops *ops;

	/*
	 * The first node in the scatter gather list.
	 */
	struct nvgpu_sgl *sgl;
};

/*
 * This struct holds the necessary information for describing a struct
 * nvgpu_mem's scatter gather list.
 *
 * This is one underlying implementation for nvgpu_sgl. Not all nvgpu_sgt's use
 * this particular implementation. Nor is a given OS required to use this at
 * all.
 */
struct nvgpu_mem_sgl {
	/*
	 * Internally this is implemented as a singly linked list.
	 */
	struct nvgpu_mem_sgl	*next;

	/*
	 * There is both a phys address and a DMA address since some systems,
	 * for example ones with an IOMMU, may see these as different addresses.
	 */
	u64			 phys;
	u64			 dma;
	u64			 length;
};

/*
 * Iterate over the SGL entries in an SGT.
 */
#define nvgpu_sgt_for_each_sgl(sgl, sgt)		\
	for ((sgl) = (sgt)->sgl;			\
	     (sgl) != NULL;				\
	     (sgl) = nvgpu_sgt_get_next(sgt, sgl))

/**
 * nvgpu_mem_sgt_create_from_mem - Create a scatter list from an nvgpu_mem.
 *
 * @g   - The GPU.
 * @mem - The source memory allocation to use.
 *
 * Create a scatter gather table from the passed @mem struct. This list lets the
 * calling code iterate across each chunk of a DMA allocation for when that DMA
 * allocation is not completely contiguous.
 */
struct nvgpu_sgt *nvgpu_sgt_create_from_mem(struct gk20a *g,
					    struct nvgpu_mem *mem);

struct nvgpu_sgl *nvgpu_sgt_get_next(struct nvgpu_sgt *sgt,
					  struct nvgpu_sgl *sgl);
u64 nvgpu_sgt_get_ipa(struct gk20a *g, struct nvgpu_sgt *sgt,
				struct nvgpu_sgl *sgl);
u64 nvgpu_sgt_ipa_to_pa(struct gk20a *g, struct nvgpu_sgt *sgt,
				struct nvgpu_sgl *sgl, u64 ipa, u64 *pa_len);
u64 nvgpu_sgt_get_phys(struct gk20a *g, struct nvgpu_sgt *sgt,
		       struct nvgpu_sgl *sgl);
u64 nvgpu_sgt_get_dma(struct nvgpu_sgt *sgt, struct nvgpu_sgl *sgl);
u64 nvgpu_sgt_get_length(struct nvgpu_sgt *sgt, struct nvgpu_sgl *sgl);
u64 nvgpu_sgt_get_gpu_addr(struct gk20a *g, struct nvgpu_sgt *sgt,
			   struct nvgpu_sgl *sgl,
			   struct nvgpu_gmmu_attrs *attrs);
void nvgpu_sgt_free(struct gk20a *g, struct nvgpu_sgt *sgt);

bool nvgpu_sgt_iommuable(struct gk20a *g, struct nvgpu_sgt *sgt);
u64 nvgpu_sgt_alignment(struct gk20a *g, struct nvgpu_sgt *sgt);

struct nvgpu_sgl *nvgpu_mem_sgl_next(void *sgl);
u64 nvgpu_mem_sgl_phys(struct gk20a *g, void *sgl);
u64 nvgpu_mem_sgl_ipa_to_pa(struct gk20a *g, struct nvgpu_sgl *sgl, u64 ipa,
				u64 *pa_len);
u64 nvgpu_mem_sgl_dma(void *sgl);
u64 nvgpu_mem_sgl_length(void *sgl);
u64 nvgpu_mem_sgl_gpu_addr(struct gk20a *g, void *sgl,
				  struct nvgpu_gmmu_attrs *attrs);
bool nvgpu_mem_sgt_iommuable(struct gk20a *g, struct nvgpu_sgt *sgt);
void nvgpu_mem_sgt_free(struct gk20a *g, struct nvgpu_sgt *sgt);

#endif /* NVGPU_SGT_H */
