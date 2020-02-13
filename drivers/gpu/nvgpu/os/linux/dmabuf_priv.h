/*
 * Copyright (c) 2017-2020, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NVGPU_DMABUF_PRIV_H
#define NVGPU_DMABUF_PRIV_H

#ifdef CONFIG_NVGPU_DMABUF_HAS_DRVDATA

#include <nvgpu/comptags.h>
#include <nvgpu/list.h>
#include <nvgpu/lock.h>
#include <nvgpu/gmmu.h>

struct sg_table;
struct dma_buf;
struct dma_buf_attachment;
struct device;

struct gk20a;

struct gk20a_buffer_state {
	struct nvgpu_list_node list;

	/* The valid compbits and the fence must be changed atomically. */
	struct nvgpu_mutex lock;

	/*
	 * Offset of the surface within the dma-buf whose state is
	 * described by this struct (one dma-buf can contain multiple
	 * surfaces with different states).
	 */
	size_t offset;

	/* A bitmask of valid sets of compbits (0 = uncompressed). */
	u32 valid_compbits;

	/* The ZBC color used on this buffer. */
	u32 zbc_color;

	/*
	 * This struct reflects the state of the buffer when this
	 * fence signals.
	 */
	struct nvgpu_fence_type *fence;
};

static inline struct gk20a_buffer_state *
gk20a_buffer_state_from_list(struct nvgpu_list_node *node)
{
	return (struct gk20a_buffer_state *)
		((uintptr_t)node - offsetof(struct gk20a_buffer_state, list));
};

struct gk20a_dmabuf_priv {
	struct nvgpu_mutex lock;

	struct gk20a *g;

	struct gk20a_comptag_allocator *comptag_allocator;
	struct gk20a_comptags comptags;

	struct dma_buf_attachment *attach;
	struct sg_table *sgt;

	int pin_count;

	struct nvgpu_list_node states;

	u64 buffer_id;
};

struct sg_table *gk20a_mm_pin_has_drvdata(struct device *dev,
			struct dma_buf *dmabuf,
			struct dma_buf_attachment **attachment);

void gk20a_mm_unpin_has_drvdata(struct device *dev,
		struct dma_buf *dmabuf,
		struct dma_buf_attachment *attachment,
		struct sg_table *sgt);

int gk20a_dmabuf_alloc_drvdata(struct dma_buf *dmabuf, struct device *dev);

int gk20a_dmabuf_get_state(struct dma_buf *dmabuf, struct gk20a *g,
			   u64 offset, struct gk20a_buffer_state **state);
#endif
#endif
