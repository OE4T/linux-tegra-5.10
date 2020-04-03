/*
 * Copyright (c) 2018-2020, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_PRIV_CMDBUF_H
#define NVGPU_PRIV_CMDBUF_H

#include <nvgpu/types.h>

struct gk20a;
struct nvgpu_mem;
struct nvgpu_channel;

struct priv_cmd_entry {
	bool valid;
	struct nvgpu_mem *mem;
	u32 off;	/* offset in mem, in u32 entries */
	u32 fill_off;	/* write offset from off, in u32 entries */
	u64 gva;
	u32 get;	/* start of entry in queue */
	u32 size;	/* in words */
};

int nvgpu_alloc_priv_cmdbuf_queue(struct nvgpu_channel *ch, u32 num_in_flight);
void nvgpu_free_priv_cmdbuf_queue(struct nvgpu_channel *ch);

int nvgpu_channel_alloc_priv_cmdbuf(struct nvgpu_channel *c, u32 orig_size,
		struct priv_cmd_entry *e);
void nvgpu_channel_free_priv_cmd_entry(struct nvgpu_channel *c,
		struct priv_cmd_entry *e);
void nvgpu_channel_update_priv_cmd_q_and_free_entry(struct nvgpu_channel *ch,
		struct priv_cmd_entry *e);

void nvgpu_priv_cmdbuf_append(struct gk20a *g, struct priv_cmd_entry *e,
		u32 *data, u32 entries);
void nvgpu_priv_cmdbuf_append_zeros(struct gk20a *g, struct priv_cmd_entry *e,
		u32 entries);

void nvgpu_priv_cmdbuf_finish(struct gk20a *g, struct priv_cmd_entry *e,
		u64 *gva, u32 *size);

#endif
