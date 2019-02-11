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

#include <nvgpu/falcon.h>
#include <nvgpu/log.h>

#include "falcon_queue_priv.h"
#include "falcon_emem_queue.h"
#include "falcon_priv.h"

/* EMEM-Q specific ops */
static int falcon_emem_queue_push(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue, u32 dst, void *data, u32 size)
{
	struct gk20a *g = queue->g;
	int err = 0;

	err = nvgpu_falcon_copy_to_emem(flcn, dst, data, size, 0);
	if (err != 0) {
		nvgpu_err(g, "flcn-%d, queue-%d", flcn->flcn_id, queue->id);
		nvgpu_err(g, "emem queue write failed");
		goto exit;
	}

exit:
	return err;
}

static int falcon_emem_queue_pop(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue, u32 src, void *data, u32 size)
{
	struct gk20a *g = queue->g;
	int err = 0;

	err = nvgpu_falcon_copy_from_emem(flcn, src, data, size, 0);
	if (err != 0) {
		nvgpu_err(g, "flcn-%d, queue-%d", flcn->flcn_id, queue->id);
		nvgpu_err(g, "emem queue read failed");
		goto exit;
	}

exit:
	return err;
}

/* assign EMEM queue type specific ops */
void falcon_emem_queue_init(struct nvgpu_falcon_queue *queue)
{
	queue->push = falcon_emem_queue_push;
	queue->pop = falcon_emem_queue_pop;
}
