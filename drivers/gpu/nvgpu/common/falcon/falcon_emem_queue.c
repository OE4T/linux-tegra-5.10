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

#include <nvgpu/log.h>
#include <nvgpu/types.h>
#include <nvgpu/pmuif/gpmuif_cmn.h>
#include <nvgpu/falcon.h>

#include "falcon_queue_priv.h"
#include "falcon_priv.h"
#include "falcon_emem_queue.h"

/* EMEM-Q specific ops */
static int falcon_queue_push_emem(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue, void *data, u32 size)
{
	int err = 0;

	err = nvgpu_falcon_copy_to_emem(flcn, queue->position, data, size, 0);
	if (err != 0) {
		nvgpu_err(flcn->g, "flcn-%d, queue-%d", flcn->flcn_id,
			queue->id);
		nvgpu_err(flcn->g, "emem queue write failed");
		goto exit;
	}

	queue->position += ALIGN(size, QUEUE_ALIGNMENT);

exit:
	return err;
}

static int falcon_queue_pop_emem(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue, void *data, u32 size,
	u32 *bytes_read)
{
	struct gk20a *g = flcn->g;
	u32 q_tail = queue->position;
	u32 q_head = 0;
	u32 used = 0;
	int err = 0;

	*bytes_read = 0;

	err = queue->head(flcn, queue, &q_head, QUEUE_GET);
	if (err != 0) {
		nvgpu_err(flcn->g, "flcn-%d, queue-%d, head GET failed",
			flcn->flcn_id, queue->id);
		goto exit;
	}

	if (q_head == q_tail) {
		goto exit;
	} else if (q_head > q_tail) {
		used = q_head - q_tail;
	} else {
		used = queue->offset + queue->size - q_tail;
	}

	if (size > used) {
		nvgpu_warn(g, "queue size smaller than request read");
		size = used;
	}

	err = nvgpu_falcon_copy_from_emem(flcn, q_tail, data, size, 0);
	if (err != 0) {
		nvgpu_err(g, "flcn-%d, queue-%d", flcn->flcn_id,
			queue->id);
		nvgpu_err(flcn->g, "emem queue read failed");
		goto exit;
	}

	queue->position += ALIGN(size, QUEUE_ALIGNMENT);
	*bytes_read = size;

exit:
	return err;
}

/* assign EMEM queue type specific ops */
void falcon_emem_queue_init(struct nvgpu_falcon *flcn,
		struct nvgpu_falcon_queue *queue)
{
	queue->push = falcon_queue_push_emem;
	queue->pop = falcon_queue_pop_emem;
}
