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
#include <nvgpu/errno.h>
#include <nvgpu/types.h>
#include <nvgpu/pmuif/gpmuif_cmn.h>
#include <nvgpu/flcnif_cmn.h>
#include <nvgpu/nvgpu_mem.h>
#include <nvgpu/pmu.h>
#include <nvgpu/string.h>
#include <nvgpu/kmem.h>
#include <nvgpu/falcon_fb_queue.h>

#include "falcon_priv.h"
#include "falcon_fb_queue_priv.h"

/* FB-Q ops */
static int falcon_fb_queue_head(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_fb_queue *queue, u32 *head, bool set)
{
	int err = -EINVAL;

	if (flcn->flcn_engine_dep_ops.queue_head != NULL) {
		err = flcn->flcn_engine_dep_ops.queue_head(flcn->g, queue->id,
			queue->index, head, set);
	}

	return err;
}

static int falcon_fb_queue_tail(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_fb_queue *queue, u32 *tail, bool set)
{
	struct gk20a *g = flcn->g;
	int err = -EINVAL;

	if (set == false && PMU_IS_COMMAND_QUEUE(queue->id)) {
		*tail = queue->fbq.tail;
		err = 0;
	} else {
		if (flcn->flcn_engine_dep_ops.queue_tail != NULL) {
			err = flcn->flcn_engine_dep_ops.queue_tail(g,
				queue->id, queue->index, tail, set);
		}
	}

	return err;
}

static inline u32 falcon_fb_queue_get_next(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_fb_queue *queue, u32 head)
{
		return (head + 1U) % queue->size;
}

static bool falcon_fb_queue_has_room(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_fb_queue *queue,
	u32 size)
{
	u32 head = 0;
	u32 tail = 0;
	u32 next_head = 0;
	int err = 0;

	err = queue->head(flcn, queue, &head, QUEUE_GET);
	if (err != 0) {
		nvgpu_err(flcn->g, "queue head GET failed");
		goto exit;
	}

	err = queue->tail(flcn, queue, &tail, QUEUE_GET);
	if (err != 0) {
		nvgpu_err(flcn->g, "queue tail GET failed");
		goto exit;
	}

	next_head = falcon_fb_queue_get_next(flcn, queue, head);

exit:
	return next_head != tail;
}

static int falcon_fb_queue_write(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_fb_queue *queue, u32 offset,
	u8 *src, u32 size)
{
	struct gk20a *g = flcn->g;
	struct nv_falcon_fbq_hdr *fb_q_hdr = (struct nv_falcon_fbq_hdr *)
		(void *)queue->fbq.work_buffer;
	u32 entry_offset = 0U;
	int err = 0;

	if (queue->fbq.work_buffer == NULL) {
		nvgpu_err(g, "Invalid/Unallocated work buffer");
		err = -EINVAL;
		goto exit;
	}

	/* Fill out FBQ hdr, that is in the work buffer */
	fb_q_hdr->element_index = (u8)offset;

	/* check queue entry size */
	if (fb_q_hdr->heap_size >= (u16)queue->fbq.element_size) {
		err = -EINVAL;
		goto exit;
	}

	/* get offset to this element entry */
	entry_offset = offset * queue->fbq.element_size;

	/* copy cmd to super-surface */
	nvgpu_mem_wr_n(g, queue->fbq.super_surface_mem,
		queue->fbq.fb_offset + entry_offset,
		queue->fbq.work_buffer, queue->fbq.element_size);

exit:
	return err;
}

static int falcon_fb_queue_set_element_use_state(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_fb_queue *queue, u32 queue_pos, bool set)
{
	int err = 0;

	if (queue_pos >= queue->size) {
		err = -EINVAL;
		goto exit;
	}

	if (test_bit((int)queue_pos,
		(void *)&queue->fbq.element_in_use) && set) {
		nvgpu_err(flcn->g,
			"FBQ last received queue element not processed yet"
			" queue_pos %d", queue_pos);
		err = -EINVAL;
		goto exit;
	}

	if (set) {
		set_bit((int)queue_pos, (void *)&queue->fbq.element_in_use);
	} else {
		clear_bit((int)queue_pos, (void *)&queue->fbq.element_in_use);
	}

exit:
	return err;
}

static int falcon_fb_queue_is_element_in_use(struct nvgpu_falcon *flcn,
		struct nvgpu_falcon_fb_queue *queue,
		u32 queue_pos, bool *in_use)
{
	int err = 0;

	if (queue_pos >= queue->size) {
		err = -EINVAL;
		goto exit;
	}

	*in_use = test_bit((int)queue_pos, (void *)&queue->fbq.element_in_use);

exit:
	return err;
}

static int falcon_fb_queue_sweep(struct nvgpu_falcon *flcn,
		struct nvgpu_falcon_fb_queue *queue)
{
	u32 head;
	u32 tail;
	bool in_use = false;
	int err = 0;

	tail = queue->fbq.tail;
	err = queue->head(flcn, queue, &head, QUEUE_GET);
	if (err != 0) {
		nvgpu_err(flcn->g, "flcn-%d queue-%d, position GET failed",
			flcn->flcn_id, queue->id);
		goto exit;
	}

	/*
	 * Step from tail forward in the queue,
	 * to see how many consecutive entries
	 * can be made available.
	 */
	while (tail != head) {
		if (falcon_fb_queue_is_element_in_use(flcn, queue,
			tail, &in_use) != 0) {
			break;
		}

		if (in_use) {
			break;
		}

		tail = falcon_fb_queue_get_next(flcn, queue, tail);
	}

	/* Update tail */
	queue->fbq.tail = tail;

exit:
	return err;
}

u32 nvgpu_falcon_fb_queue_get_id(struct nvgpu_falcon_fb_queue *queue)
{
	return queue->id;
}

u32 nvgpu_falcon_fb_queue_get_index(struct nvgpu_falcon_fb_queue *queue)
{
	return queue->index;
}

u32 nvgpu_falcon_fb_queue_get_size(struct nvgpu_falcon_fb_queue *queue)
{
	return queue->size;
}

u32 nvgpu_falcon_fb_queue_get_position(struct nvgpu_falcon_fb_queue *queue)
{
	return queue->position;
}

/* return the queue element size */
u32 nvgpu_falcon_fb_queue_get_element_size(struct nvgpu_falcon_fb_queue *queue)
{
	return queue->fbq.element_size;
}

/* return the queue offset from super surface FBQ's */
u32 nvgpu_falcon_fb_queue_get_offset(struct nvgpu_falcon_fb_queue *queue)
{
	return queue->fbq.fb_offset;
}

/* lock work buffer of queue */
void nvgpu_falcon_fb_queue_lock_work_buffer(struct nvgpu_falcon_fb_queue *queue)
{
	/* acquire work buffer mutex */
	nvgpu_mutex_acquire(&queue->fbq.work_buffer_mutex);
}

/* unlock work buffer of queue */
void nvgpu_falcon_fb_queue_unlock_work_buffer(
					struct nvgpu_falcon_fb_queue *queue)
{
	/* release work buffer mutex */
	nvgpu_mutex_release(&queue->fbq.work_buffer_mutex);
}

/* return a pointer of queue work buffer */
u8 *nvgpu_falcon_fb_queue_get_work_buffer(struct nvgpu_falcon_fb_queue *queue)
{
	return queue->fbq.work_buffer;
}

int nvgpu_falcon_fb_queue_free_element(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_fb_queue *queue, u32 queue_pos)
{
	int err = 0;

	err = falcon_fb_queue_set_element_use_state(flcn, queue,
		queue_pos, false);
	if (err != 0) {
		nvgpu_err(flcn->g, "fb queue elelment %d free failed",
			queue_pos);
		goto exit;
	}

	err = falcon_fb_queue_sweep(flcn, queue);

exit:
	return err;
}

/* queue is_empty check with lock */
bool nvgpu_falcon_fb_queue_is_empty(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_fb_queue *queue)
{
	u32 q_head = 0;
	u32 q_tail = 0;
	int err = 0;

	if ((flcn == NULL) || (queue == NULL)) {
		return true;
	}

	/* acquire mutex */
	nvgpu_mutex_acquire(&queue->mutex);

	err = queue->head(flcn, queue, &q_head, QUEUE_GET);
	if (err != 0) {
		nvgpu_err(flcn->g, "flcn-%d queue-%d, head GET failed",
			flcn->flcn_id, queue->id);
		goto exit;
	}

	err = queue->tail(flcn, queue, &q_tail, QUEUE_GET);
	if (err != 0) {
		nvgpu_err(flcn->g, "flcn-%d queue-%d, tail GET failed",
			flcn->flcn_id, queue->id);
		goto exit;
	}

exit:
	/* release mutex */
	nvgpu_mutex_release(&queue->mutex);

	return q_head == q_tail;
}

static int falcon_fb_queue_prepare_write(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_fb_queue *queue, u32 size)
{
	int err = 0;

	/* make sure there's enough free space for the write */
	if (!falcon_fb_queue_has_room(flcn, queue, size)) {
		nvgpu_pmu_dbg(flcn->g, "queue full: queue-id %d: index %d",
			queue->id, queue->index);
		err = -EAGAIN;
		goto exit;
	}

	err = queue->head(flcn, queue, &queue->position, QUEUE_GET);
	if (err != 0) {
		nvgpu_err(flcn->g, "flcn-%d queue-%d, position GET failed",
			flcn->flcn_id, queue->id);
		goto exit;
	}

exit:
	return err;
}

/* queue push operation with lock */
int nvgpu_falcon_fb_queue_push(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_fb_queue *queue, void *data, u32 size)
{
	struct gk20a *g;
	int err = 0;

	if ((flcn == NULL) || (queue == NULL)) {
		return -EINVAL;
	}

	g = queue->g;

	nvgpu_log_fn(g, " ");

	if (queue->oflag != OFLAG_WRITE) {
		nvgpu_err(flcn->g, "flcn-%d, queue-%d not opened for write",
			flcn->flcn_id, queue->id);
		err = -EINVAL;
		goto exit;
	}

	/* acquire mutex */
	nvgpu_mutex_acquire(&queue->mutex);

	err = falcon_fb_queue_prepare_write(flcn, queue, size);
	if (err != 0) {
		goto unlock_mutex;
	}

	/* Bounds check size */
	if (size > queue->fbq.element_size) {
		nvgpu_err(g, "size too large size=0x%x", size);
		goto unlock_mutex;
	}

	/* Set queue element in use */
	if (falcon_fb_queue_set_element_use_state(flcn, queue,
		queue->position, true) != 0) {
		nvgpu_err(g,
			"fb-queue element in use map is in invalid state");
		err = -EINVAL;
		goto unlock_mutex;
	}

	/* write data to FB */
	err = falcon_fb_queue_write(flcn, queue, queue->position, data, size);
	if (err != 0) {
		nvgpu_err(g, "write to fb-queue failed");
		goto unlock_mutex;
	}

	queue->position = falcon_fb_queue_get_next(flcn, queue,
			queue->position);

	err = queue->head(flcn, queue, &queue->position, QUEUE_SET);
	if (err != 0) {
		nvgpu_err(flcn->g, "flcn-%d queue-%d, position SET failed",
			flcn->flcn_id, queue->id);
		goto unlock_mutex;
	}

unlock_mutex:
	/* release mutex */
	nvgpu_mutex_release(&queue->mutex);
exit:
	if (err != 0) {
		nvgpu_err(flcn->g, "falcon id-%d, queue id-%d, failed",
			flcn->flcn_id, queue->id);
	}

	return err;
}

/* queue pop operation with lock */
int nvgpu_falcon_fb_queue_pop(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_fb_queue *queue, void *data, u32 size,
	u32 *bytes_read)
{
	struct gk20a *g;
	struct pmu_hdr *hdr;
	u32 entry_offset = 0U;
	int err = 0;

	if ((flcn == NULL) || (queue == NULL)) {
		return -EINVAL;
	}

	g = flcn->g;
	hdr = (struct pmu_hdr *) (void *) queue->fbq.work_buffer;

	nvgpu_log_fn(g, " ");

	if (queue->oflag != OFLAG_READ) {
		nvgpu_err(flcn->g, "flcn-%d, queue-%d, not opened for read",
			flcn->flcn_id, queue->id);
		err = -EINVAL;
		goto exit;
	}

	/* acquire mutex */
	nvgpu_mutex_acquire(&queue->mutex);

	err = queue->tail(flcn, queue, &queue->position, QUEUE_GET);
	if (err != 0) {
		nvgpu_err(flcn->g, "flcn-%d queue-%d, position GET failed",
			flcn->flcn_id, queue->id);
		goto unlock_mutex;
	}

	*bytes_read = 0U;

	/* Check size */
	if ((size + queue->fbq.read_position) >= queue->fbq.element_size) {
		nvgpu_err(g,
			"Attempt to read > than queue element size "
			"for queue id-%d", queue->id);
		err = -EINVAL;
		goto unlock_mutex;
	}

	entry_offset = queue->position * queue->fbq.element_size;

	/*
	 * If first read for this queue element then read whole queue
	 * element into work buffer.
	 */
	if (queue->fbq.read_position == 0U) {
		nvgpu_mem_rd_n(g, queue->fbq.super_surface_mem,
			/* source (FBQ data) offset*/
			queue->fbq.fb_offset + entry_offset,
			/* destination buffer */
			(void *)queue->fbq.work_buffer,
			/* copy size */
			queue->fbq.element_size);

		/* Check size in hdr of MSG just read */
		if (hdr->size >= queue->fbq.element_size) {
			nvgpu_err(g, "Super Surface read failed");
			err = -ERANGE;
			goto unlock_mutex;
		}
	}

	nvgpu_memcpy((u8 *)data, (u8 *)queue->fbq.work_buffer +
		queue->fbq.read_position, size);

	/* update current position */
	queue->fbq.read_position += size;

	/* If reached end of this queue element, move on to next. */
	if (queue->fbq.read_position >= hdr->size) {
		queue->fbq.read_position = 0U;
		/* Increment queue index. */
		queue->position = falcon_fb_queue_get_next(flcn, queue,
			queue->position);
	}

	*bytes_read = size;

	err = queue->tail(flcn, queue, &queue->position, QUEUE_SET);
	if (err != 0) {
		nvgpu_err(flcn->g, "flcn-%d queue-%d, position SET failed",
			flcn->flcn_id, queue->id);
		goto unlock_mutex;
	}

unlock_mutex:
	/* release mutex */
	nvgpu_mutex_release(&queue->mutex);
exit:
	if (err != 0) {
		nvgpu_err(flcn->g, "falcon id-%d, queue id-%d, failed",
			flcn->flcn_id, queue->id);
	}

	return err;
}

void nvgpu_falcon_fb_queue_free(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_fb_queue **queue_p)
{
	struct nvgpu_falcon_fb_queue *queue = NULL;
	struct gk20a *g = flcn->g;

	if ((queue_p == NULL) || (*queue_p == NULL)) {
		return;
	}

	queue = *queue_p;

	nvgpu_pmu_dbg(g, "flcn id-%d q-id %d: index %d ",
		      flcn->flcn_id, queue->id, queue->index);

	nvgpu_kfree(g, queue->fbq.work_buffer);
	nvgpu_mutex_destroy(&queue->fbq.work_buffer_mutex);

	/* destroy mutex */
	nvgpu_mutex_destroy(&queue->mutex);

	nvgpu_kfree(g, queue);
	*queue_p = NULL;
}

int nvgpu_falcon_fb_queue_init(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_fb_queue **queue_p,
	struct nvgpu_falcon_fb_queue_params params)
{
	struct nvgpu_falcon_fb_queue *queue = NULL;
	struct gk20a *g = flcn->g;
	int err = 0;

	if (queue_p == NULL) {
		return -EINVAL;
	}

	queue = (struct nvgpu_falcon_fb_queue *)
		   nvgpu_kmalloc(g, sizeof(struct nvgpu_falcon_fb_queue));

	if (queue == NULL) {
		return -ENOMEM;
	}

	queue->g = g;
	queue->id = params.id;
	queue->index = params.index;
	queue->size = params.size;
	queue->oflag = params.oflag;

	queue->fbq.tail = 0U;
	queue->fbq.element_in_use = 0U;
	queue->fbq.read_position = 0U;
	queue->fbq.super_surface_mem = params.super_surface_mem;
	queue->fbq.element_size = params.fbq_element_size;
	queue->fbq.fb_offset = params.fbq_offset;

	queue->position = 0U;

	queue->head = falcon_fb_queue_head;
	queue->tail = falcon_fb_queue_tail;

	/* init mutex */
	err = nvgpu_mutex_init(&queue->mutex);
	if (err != 0) {
		goto exit;
	}

	/* init mutex */
	err = nvgpu_mutex_init(&queue->fbq.work_buffer_mutex);
	if (err != 0) {
		goto exit;
	}

	queue->fbq.work_buffer = nvgpu_kzalloc(g, queue->fbq.element_size);
	if (queue->fbq.work_buffer == NULL) {
		err = -ENOMEM;
		goto exit;
	}

	nvgpu_log(g, gpu_dbg_pmu,
		"flcn id-%d q-id %d: index %d, size 0x%08x",
		flcn->flcn_id, queue->id, queue->index,
		queue->size);

	*queue_p = queue;
exit:
	return err;
}
