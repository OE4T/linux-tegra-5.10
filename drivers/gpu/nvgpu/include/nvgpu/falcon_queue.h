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

#ifndef NVGPU_FALCON_QUEUE_H
#define NVGPU_FALCON_QUEUE_H

#include <nvgpu/types.h>

/* Queue Type */
#define QUEUE_TYPE_DMEM 0x0U
#define QUEUE_TYPE_EMEM 0x1U
#define QUEUE_TYPE_FB   0x2U

struct gk20a;
struct nvgpu_falcon;
struct nvgpu_falcon_queue;

struct nvgpu_falcon_queue_params {
	/* Queue Type (queue_type) */
	u8 queue_type;
	/* current write position */
	u32 position;
	/* physical dmem offset where this queue begins */
	u32 offset;
	/* logical queue identifier */
	u32 id;
	/* physical queue index */
	u32 index;
	/* in bytes */
	u32 size;
	/* open-flag */
	u32 oflag;

	/* fb queue params*/
	/* Holds the offset of queue data (0th element) */
	u32 fbq_offset;

	/* fb queue element size*/
	u32 fbq_element_size;

	/* Holds super surface base address */
	struct nvgpu_mem *super_surface_mem;
};

/* queue public functions */
int nvgpu_falcon_queue_init(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue **queue_p,
	struct nvgpu_falcon_queue_params params);
bool nvgpu_falcon_queue_is_empty(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue);
int nvgpu_falcon_queue_rewind(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue);
int nvgpu_falcon_queue_pop(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue, void *data, u32 size,
	u32 *bytes_read);
int nvgpu_falcon_queue_push(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue, void *data, u32 size);
void nvgpu_falcon_queue_free(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue **queue_p);
u32 nvgpu_falcon_queue_get_id(struct nvgpu_falcon_queue *queue);
u32 nvgpu_falcon_queue_get_position(struct nvgpu_falcon_queue *queue);
u32 nvgpu_falcon_queue_get_index(struct nvgpu_falcon_queue *queue);
u32 nvgpu_falcon_queue_get_size(struct nvgpu_falcon_queue *queue);
u32 nvgpu_falcon_fbq_get_element_size(struct nvgpu_falcon_queue *queue);
u32 nvgpu_falcon_queue_get_fbq_offset(struct nvgpu_falcon_queue *queue);
u8 *nvgpu_falcon_queue_get_fbq_work_buffer(struct nvgpu_falcon_queue *queue);
int nvgpu_falcon_queue_free_fbq_element(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue, u32 queue_pos);
void nvgpu_falcon_queue_lock_fbq_work_buffer(struct nvgpu_falcon_queue *queue);
void nvgpu_falcon_queue_unlock_fbq_work_buffer(
	struct nvgpu_falcon_queue *queue);

#endif /* NVGPU_FALCON_QUEUE_H */
