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

#ifndef NVGPU_POSIX_QUEUE_H
#define NVGPU_POSIX_QUEUE_H

#include <nvgpu/lock.h>

struct nvgpu_queue {
	unsigned int in;
	unsigned int out;
	unsigned int mask;
	unsigned char *data;
};

unsigned int nvgpu_queue_unused(struct nvgpu_queue *queue);
unsigned int nvgpu_queue_available(struct nvgpu_queue *queue);
int nvgpu_queue_alloc(struct nvgpu_queue *queue, unsigned int size);
void nvgpu_queue_free(struct nvgpu_queue *queue);
int nvgpu_queue_in(struct nvgpu_queue *queue, const void *buf,
		unsigned int len);
int nvgpu_queue_in_locked(struct nvgpu_queue *queue, const void *buf,
		unsigned int len, struct nvgpu_mutex *lock);
int nvgpu_queue_out(struct nvgpu_queue *queue, void *buf,
		unsigned int len);
int nvgpu_queue_out_locked(struct nvgpu_queue *queue, void *buf,
		unsigned int len, struct nvgpu_mutex *lock);
#endif
