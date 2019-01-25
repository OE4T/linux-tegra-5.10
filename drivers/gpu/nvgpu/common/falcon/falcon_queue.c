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

#include <nvgpu/lock.h>
#include <nvgpu/string.h>
#include <nvgpu/pmu.h>

#include "falcon_priv.h"

/* common falcon queue ops */
static int falcon_queue_head(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue, u32 *head, bool set)
{
	int err = -ENOSYS;

	if (flcn->flcn_engine_dep_ops.queue_head != NULL) {
		err = flcn->flcn_engine_dep_ops.queue_head(flcn->g, queue,
			head, set);
	}

	return err;
}

static int falcon_queue_tail(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue, u32 *tail, bool set)
{
	int err = -ENOSYS;

	if (flcn->flcn_engine_dep_ops.queue_tail != NULL) {
		err = flcn->flcn_engine_dep_ops.queue_tail(flcn->g, queue,
			tail, set);
	}

	return err;
}

static bool falcon_queue_has_room(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue, u32 size, bool *need_rewind)
{
	u32 q_head = 0;
	u32 q_tail = 0;
	u32 q_free = 0;
	bool q_rewind = false;
	int err = 0;

	size = ALIGN(size, QUEUE_ALIGNMENT);

	err = queue->head(flcn, queue, &q_head, QUEUE_GET);
	if (err != 0) {
		nvgpu_err(flcn->g, "queue head GET failed");
		goto exit;
	}

	err = queue->tail(flcn, queue, &q_tail, QUEUE_GET);
	if (err != 0) {
		nvgpu_err(flcn->g, "queue tail GET failed");
		goto exit;
	}

	if (q_head >= q_tail) {
		q_free = queue->offset + queue->size - q_head;
		q_free -= (u32)PMU_CMD_HDR_SIZE;

		if (size > q_free) {
			q_rewind = true;
			q_head = queue->offset;
		}
	}

	if (q_head < q_tail) {
		q_free = q_tail - q_head - 1U;
	}

	if (need_rewind != NULL) {
		*need_rewind = q_rewind;
	}

exit:
	return size <= q_free;
}

static int falcon_queue_rewind(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue)
{
	struct gk20a *g = flcn->g;
	struct pmu_cmd cmd;
	int err = 0;

	if (queue->oflag == OFLAG_WRITE) {
		cmd.hdr.unit_id = PMU_UNIT_REWIND;
		cmd.hdr.size = (u8)PMU_CMD_HDR_SIZE;
		err = queue->push(flcn, queue, &cmd, cmd.hdr.size);
		if (err != 0) {
			nvgpu_err(g, "flcn-%d queue-%d, rewind request failed",
				flcn->flcn_id, queue->id);
			goto exit;
		} else {
			nvgpu_pmu_dbg(g, "flcn-%d queue-%d, rewinded",
			flcn->flcn_id, queue->id);
		}
	}

	/* update queue position */
	queue->position = queue->offset;

	if (queue->oflag == OFLAG_READ) {
		err = queue->tail(flcn, queue, &queue->position,
			QUEUE_SET);
		if (err != 0){
			nvgpu_err(flcn->g, "flcn-%d queue-%d, position SET failed",
				flcn->flcn_id, queue->id);
			goto exit;
		}
	}

exit:
	return err;
}

/* FB-Q ops */
static int falcon_queue_tail_fb(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue, u32 *tail, bool set)
{
	struct gk20a *g = flcn->g;
	int err = -ENOSYS;

	if (set == false && PMU_IS_COMMAND_QUEUE(queue->id)) {
		*tail = queue->fbq.tail;
		err = 0;
	} else {
		if (flcn->flcn_engine_dep_ops.queue_tail != NULL) {
			err = flcn->flcn_engine_dep_ops.queue_tail(g,
				queue, tail, set);
		}
	}

	return err;
}

static inline u32 falcon_queue_get_next_fb(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue, u32 head)
{
		return (head + 1U) % queue->size;
}

static bool falcon_queue_has_room_fb(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue,
	u32 size, bool *need_rewind)
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

	next_head = falcon_queue_get_next_fb(flcn, queue, head);

exit:
	return next_head != tail;
}

static int falcon_queue_write_fb(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue, u32 offset,
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

static int falcon_queue_element_set_use_state_fb(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue, u32 queue_pos, bool set)
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

static int falcon_queue_push_fb(struct nvgpu_falcon *flcn,
			struct nvgpu_falcon_queue *queue, void *data, u32 size)
{
	struct gk20a *g = flcn->g;
	int err = 0;

	nvgpu_log_fn(g, " ");

	/* Bounds check size */
	if (size > queue->fbq.element_size) {
		nvgpu_err(g, "size too large size=0x%x", size);
		goto exit;
	}

	/* Set queue element in use */
	if (falcon_queue_element_set_use_state_fb(flcn, queue,
		queue->position, true) != 0 ) {
		nvgpu_err(g,
			"fb-queue element in use map is in invalid state");
		goto exit;
	}

	/* write data to FB */
	err = falcon_queue_write_fb(flcn, queue, queue->position, data, size);
	if (err != 0){
		nvgpu_err(g, "write to fb-queue failed");
		goto exit;
	}

	queue->position = falcon_queue_get_next_fb(flcn, queue,
			queue->position);

exit:
	if (err != 0) {
		nvgpu_err(flcn->g, "falcon id-%d, queue id-%d, failed",
			flcn->flcn_id, queue->id);
	}

	return err;
}

static int falcon_queue_pop_fb(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue, void *data, u32 size,
	u32 *bytes_read)
{
	struct gk20a *g = flcn->g;
	struct pmu_hdr *hdr = (struct pmu_hdr *)
		(void *)queue->fbq.work_buffer;
	u32 entry_offset = 0U;
	int err = 0;

	nvgpu_log_fn(g, " ");

	*bytes_read = 0U;

	/* Check size */
	if ((size + queue->fbq.read_position) >=
		queue->fbq.element_size) {
		nvgpu_err(g,
			"Attempt to read > than queue element size for queue id-%d",
			queue->id);
		err = -EINVAL ;
		goto exit;
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
			err = -ERANGE ;
			goto exit;
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
		queue->position = falcon_queue_get_next_fb(flcn, queue,
			queue->position);
	}

	*bytes_read = size;

exit:
	if (err != 0) {
		nvgpu_err(flcn->g, "falcon id-%d, queue id-%d, failed",
			flcn->flcn_id, queue->id);
	}

	return err;
}

static int falcon_queue_element_is_in_use_fb(struct nvgpu_falcon *flcn,
		struct nvgpu_falcon_queue *queue, u32 queue_pos, bool *in_use)
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

static int falcon_queue_sweep_fb(struct nvgpu_falcon *flcn,
		struct nvgpu_falcon_queue *queue)
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
		if (falcon_queue_element_is_in_use_fb(flcn, queue,
			tail, &in_use) != 0) {
			break;
		}

		if (in_use) {
			break;
		}

		tail = falcon_queue_get_next_fb(flcn, queue, tail);
	}

	/* Update tail */
	queue->fbq.tail = tail;

exit:
	return err;
}

/* assign FB queue type specific ops */
static int falcon_queue_init_fb_queue(struct nvgpu_falcon *flcn,
		struct nvgpu_falcon_queue *queue)
{
	struct gk20a *g = flcn->g;
	int err = 0;

	nvgpu_log_fn(flcn->g, " ");

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

	queue->offset = 0U;
	queue->position = 0U;
	queue->fbq.tail = 0U;
	queue->fbq.element_in_use = 0U;
	queue->fbq.read_position = 0U;

	queue->head = falcon_queue_head;
	queue->tail = falcon_queue_tail_fb;
	queue->has_room = falcon_queue_has_room_fb;
	queue->push = falcon_queue_push_fb;
	queue->pop = falcon_queue_pop_fb;
	queue->rewind = NULL; /* Not required for FB-Q */

exit:
	return err;
}

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
static void falcon_queue_init_emem_queue(struct nvgpu_falcon *flcn,
		struct nvgpu_falcon_queue *queue)
{
	queue->head = falcon_queue_head;
	queue->tail = falcon_queue_tail;
	queue->has_room = falcon_queue_has_room;
	queue->rewind = falcon_queue_rewind;
	queue->push = falcon_queue_push_emem;
	queue->pop = falcon_queue_pop_emem;
}

/* DMEM-Q specific ops */
static int falcon_queue_push_dmem(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue, void *data, u32 size)
{
	int err = 0;

	err = nvgpu_falcon_copy_to_dmem(flcn, queue->position, data, size, 0);
	if (err != 0) {
		nvgpu_err(flcn->g, "flcn-%d, queue-%d", flcn->flcn_id,
			queue->id);
		nvgpu_err(flcn->g, "dmem queue write failed");
		goto exit;
	}

	queue->position += ALIGN(size, QUEUE_ALIGNMENT);

exit:
	return err;
}

static int falcon_queue_pop_dmem(struct nvgpu_falcon *flcn,
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

	err = nvgpu_falcon_copy_from_dmem(flcn, q_tail, data, size, 0);
	if (err != 0) {
		nvgpu_err(g, "flcn-%d, queue-%d", flcn->flcn_id,
			queue->id);
		nvgpu_err(flcn->g, "dmem queue read failed");
		goto exit;
	}

	queue->position += ALIGN(size, QUEUE_ALIGNMENT);
	*bytes_read = size;

exit:
	return err;
}

/* assign DMEM queue type specific ops */
static void falcon_queue_init_dmem_queue(struct nvgpu_falcon *flcn,
		struct nvgpu_falcon_queue *queue)
{
	queue->head = falcon_queue_head;
	queue->tail = falcon_queue_tail;
	queue->has_room = falcon_queue_has_room;
	queue->push = falcon_queue_push_dmem;
	queue->pop = falcon_queue_pop_dmem;
	queue->rewind = falcon_queue_rewind;
}

static int falcon_queue_prepare_write(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue, u32 size)
{
	bool q_rewind = false;
	int err = 0;

	/* make sure there's enough free space for the write */
	if (!queue->has_room(flcn, queue, size, &q_rewind)) {
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

	if (q_rewind) {
		if (queue->rewind != NULL)  {
			err = queue->rewind(flcn, queue);
		}
	}

exit:
	return err;
}

/* queue public functions */

/* queue push operation with lock */
int nvgpu_falcon_queue_push(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue, void *data, u32 size)
{
	int err = 0;

	if ((flcn == NULL) || (queue == NULL)) {
		return -EINVAL;
	}

	if (queue->oflag != OFLAG_WRITE) {
		nvgpu_err(flcn->g, "flcn-%d, queue-%d not opened for write",
			flcn->flcn_id, queue->id);
		err = -EINVAL;
		goto exit;
	}

	/* acquire mutex */
	nvgpu_mutex_acquire(&queue->mutex);

	err = falcon_queue_prepare_write(flcn, queue, size);
	if (err != 0) {
		goto unlock_mutex;
	}

	err = queue->push(flcn, queue, data, size);
	if (err != 0) {
		nvgpu_err(flcn->g, "flcn-%d queue-%d, fail to write",
			flcn->flcn_id, queue->id);
	}

	err = queue->head(flcn, queue, &queue->position, QUEUE_SET);
	if (err != 0){
		nvgpu_err(flcn->g, "flcn-%d queue-%d, position SET failed",
			flcn->flcn_id, queue->id);
	}

unlock_mutex:
	/* release mutex */
	nvgpu_mutex_release(&queue->mutex);
exit:
	return err;
}

/* queue pop operation with lock */
int nvgpu_falcon_queue_pop(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue, void *data, u32 size,
	u32 *bytes_read)
{
	int err = 0;

	if ((flcn == NULL) || (queue == NULL)) {
		return -EINVAL;
	}

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

	err = queue->pop(flcn, queue, data, size, bytes_read);
	if (err != 0) {
		nvgpu_err(flcn->g, "flcn-%d queue-%d, fail to read",
			flcn->flcn_id, queue->id);
	}

	err = queue->tail(flcn, queue, &queue->position, QUEUE_SET);
	if (err != 0){
		nvgpu_err(flcn->g, "flcn-%d queue-%d, position SET failed",
			flcn->flcn_id, queue->id);
	}

unlock_mutex:
	/* release mutex */
	nvgpu_mutex_release(&queue->mutex);
exit:
	return err;
}

int nvgpu_falcon_queue_rewind(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue)
{
	int err = 0;

	if ((flcn == NULL) || (queue == NULL)) {
		return -EINVAL;
	}

	/* acquire mutex */
	nvgpu_mutex_acquire(&queue->mutex);

	if (queue->rewind != NULL) {
		err = queue->rewind(flcn, queue);
	}

	/* release mutex */
	nvgpu_mutex_release(&queue->mutex);

	return err;
}

/* queue is_empty check with lock */
bool nvgpu_falcon_queue_is_empty(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue)
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

void nvgpu_falcon_queue_free(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue **queue_p)
{
	struct nvgpu_falcon_queue *queue = NULL;
	struct gk20a *g = flcn->g;

	if ((queue_p == NULL) || (*queue_p == NULL)) {
		return;
	}

	queue = *queue_p;

	nvgpu_pmu_dbg(g, "flcn id-%d q-id %d: index %d ",
		      flcn->flcn_id, queue->id, queue->index);

	if (queue->queue_type == QUEUE_TYPE_FB) {
		nvgpu_kfree(g, queue->fbq.work_buffer);
		nvgpu_mutex_destroy(&queue->fbq.work_buffer_mutex);
	}

	/* destroy mutex */
	nvgpu_mutex_destroy(&queue->mutex);

	nvgpu_kfree(g, queue);
	*queue_p = NULL;
}

u32 nvgpu_falcon_queue_get_id(struct nvgpu_falcon_queue *queue)
{
	return queue->id;
}

u32 nvgpu_falcon_queue_get_position(struct nvgpu_falcon_queue *queue)
{
	return queue->position;
}

u32 nvgpu_falcon_queue_get_index(struct nvgpu_falcon_queue *queue)
{
	return queue->index;
}

u32 nvgpu_falcon_queue_get_size(struct nvgpu_falcon_queue *queue)
{
	return queue->size;
}

/* return the queue element size */
u32 nvgpu_falcon_fbq_get_element_size(struct nvgpu_falcon_queue *queue)
{
	return queue->fbq.element_size;
}

/* return the queue offset from super surface FBQ's */
u32 nvgpu_falcon_queue_get_fbq_offset(struct nvgpu_falcon_queue *queue)
{
	return queue->fbq.fb_offset;
}

/* lock work buffer of queue */
void nvgpu_falcon_queue_lock_fbq_work_buffer(struct nvgpu_falcon_queue *queue)
{
	/* acquire work buffer mutex */
	nvgpu_mutex_acquire(&queue->fbq.work_buffer_mutex);
}

/* unlock work buffer of queue */
void nvgpu_falcon_queue_unlock_fbq_work_buffer(struct nvgpu_falcon_queue *queue)
{
	/* release work buffer mutex */
	nvgpu_mutex_release(&queue->fbq.work_buffer_mutex);
}

/* return a pointer of queue work buffer */
u8* nvgpu_falcon_queue_get_fbq_work_buffer(struct nvgpu_falcon_queue *queue)
{
	return queue->fbq.work_buffer;
}

int nvgpu_falcon_queue_free_fbq_element(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue, u32 queue_pos)
{
	int err = 0;

	err = falcon_queue_element_set_use_state_fb(flcn, queue,
		queue_pos, false);
	if (err != 0) {
		nvgpu_err(flcn->g, "fb queue elelment %d free failed",
			queue_pos);
		goto exit;
	}

	err = falcon_queue_sweep_fb(flcn, queue);

exit:
	return err;
}

int nvgpu_falcon_queue_init(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue **queue_p,
	struct nvgpu_falcon_queue_params params)
{
	struct nvgpu_falcon_queue *queue = NULL;
	struct gk20a *g = flcn->g;
	int err = 0;

	if (queue_p == NULL) {
		return -EINVAL;
	}

	queue = (struct nvgpu_falcon_queue *)
		   nvgpu_kmalloc(g, sizeof(struct nvgpu_falcon_queue));

	if (queue == NULL) {
		return -ENOMEM;
	}

	queue->g = g;
	queue->id = params.id;
	queue->index = params.index;
	queue->offset = params.offset;
	queue->position = params.position;
	queue->size = params.size;
	queue->oflag = params.oflag;
	queue->queue_type = params.queue_type;

	nvgpu_log(g, gpu_dbg_pmu,
		"flcn id-%d q-id %d: index %d, offset 0x%08x, size 0x%08x",
		flcn->flcn_id, queue->id, queue->index,
		queue->offset, queue->size);

	switch (queue->queue_type) {
	case QUEUE_TYPE_DMEM:
		falcon_queue_init_dmem_queue(flcn, queue);
		break;
	case QUEUE_TYPE_EMEM:
		falcon_queue_init_emem_queue(flcn, queue);
		break;
	case QUEUE_TYPE_FB:
		queue->fbq.super_surface_mem = params.super_surface_mem;
		queue->fbq.element_size = params.fbq_element_size;
		queue->fbq.fb_offset = params.fbq_offset;

		err = falcon_queue_init_fb_queue(flcn, queue);
		if (err != 0x0) {
			goto exit;
		}
		break;
	default:
		err = -EINVAL;
		break;
	}

	if (err != 0) {
		nvgpu_err(flcn->g, "flcn-%d queue-%d, init failed",
			flcn->flcn_id, queue->id);
		nvgpu_kfree(g, queue);
		goto exit;
	}

	/* init mutex */
	err = nvgpu_mutex_init(&queue->mutex);
	if (err != 0) {
		goto exit;
	}

	*queue_p = queue;
exit:
	return err;
}
