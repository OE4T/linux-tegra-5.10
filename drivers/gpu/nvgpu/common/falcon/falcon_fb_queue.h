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

#ifndef NVGPU_FALCON_FB_QUEUE_H
#define NVGPU_FALCON_FB_QUEUE_H

u32 falcon_queue_get_element_size_fb(struct nvgpu_falcon_queue *queue);
u32 falcon_queue_get_offset_fb(struct nvgpu_falcon_queue *queue);
void falcon_queue_lock_work_buffer_fb(struct nvgpu_falcon_queue *queue);
void falcon_queue_unlock_work_buffer_fb(struct nvgpu_falcon_queue *queue);
u8 *falcon_queue_get_work_buffer_fb(struct nvgpu_falcon_queue *queue);
int falcon_queue_free_element_fb(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue, u32 queue_pos);
int falcon_fb_queue_init(struct nvgpu_falcon *flcn,
		struct nvgpu_falcon_queue *queue);

#endif /* NVGPU_FALCON_FB_QUEUE_H */
