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

#include <nvgpu/sec2if/sec2_if_cmn.h>
#include <nvgpu/sec2/allocator.h>
#include <nvgpu/engine_queue.h>
#include <nvgpu/sec2/queue.h>
#include <nvgpu/flcnif_cmn.h>
#include <nvgpu/sec2/msg.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/sec2.h>

/* Message/Event request handlers */
static int sec2_response_handle(struct nvgpu_sec2 *sec2,
	struct nv_flcn_msg_sec2 *msg)
{
	struct gk20a *g = sec2->g;

	return nvgpu_sec2_seq_response_handle(g, &sec2->sequences,
					      msg, msg->hdr.seq_id);
}

static int sec2_handle_event(struct nvgpu_sec2 *sec2,
	struct nv_flcn_msg_sec2 *msg)
{
	int err = 0;

	switch (msg->hdr.unit_id) {
	default:
		break;
	}

	return err;
}

static bool sec2_read_message(struct nvgpu_sec2 *sec2,
	u32 queue_id, struct nv_flcn_msg_sec2 *msg, int *status)
{
	struct gk20a *g = sec2->g;
	u32 read_size;
	int err;

	*status = 0U;

	if (nvgpu_sec2_queue_is_empty(sec2->queues, queue_id)) {
		return false;
	}

	if (!nvgpu_sec2_queue_read(g, sec2->queues, queue_id,
				   &sec2->flcn,  &msg->hdr,
				   PMU_MSG_HDR_SIZE, status)) {
		nvgpu_err(g, "fail to read msg from queue %d", queue_id);
		goto clean_up;
	}

	if (msg->hdr.unit_id == NV_SEC2_UNIT_REWIND) {
		err = nvgpu_sec2_queue_rewind(&sec2->flcn,
					      sec2->queues, queue_id);
		if (err != 0) {
			nvgpu_err(g, "fail to rewind queue %d", queue_id);
			*status = err;
			goto clean_up;
		}

		/* read again after rewind */
		if (!nvgpu_sec2_queue_read(g, sec2->queues, queue_id,
					   &sec2->flcn, &msg->hdr,
					   PMU_MSG_HDR_SIZE, status)) {
			nvgpu_err(g, "fail to read msg from queue %d",
				queue_id);
			goto clean_up;
		}
	}

	if (!NV_SEC2_UNITID_IS_VALID(msg->hdr.unit_id)) {
		nvgpu_err(g, "read invalid unit_id %d from queue %d",
			msg->hdr.unit_id, queue_id);
			*status = -EINVAL;
			goto clean_up;
	}

	if (msg->hdr.size > PMU_MSG_HDR_SIZE) {
		read_size = msg->hdr.size - PMU_MSG_HDR_SIZE;
		if (!nvgpu_sec2_queue_read(g, sec2->queues, queue_id,
					   &sec2->flcn, &msg->msg,
					   read_size, status)) {
			nvgpu_err(g, "fail to read msg from queue %d",
				queue_id);
			goto clean_up;
		}
	}

	return true;

clean_up:
	return false;
}

static int sec2_process_init_msg(struct nvgpu_sec2 *sec2,
	struct nv_flcn_msg_sec2 *msg)
{
	struct gk20a *g = sec2->g;
	struct sec2_init_msg_sec2_init *sec2_init;
	u32 tail = 0;
	int err = 0;

	g->ops.sec2.msgq_tail(g, sec2, &tail, QUEUE_GET);

	err = nvgpu_falcon_copy_from_emem(&sec2->flcn, tail,
		(u8 *)&msg->hdr, PMU_MSG_HDR_SIZE, 0U);
	if (err != 0) {
		goto exit;
	}

	if (msg->hdr.unit_id != NV_SEC2_UNIT_INIT) {
		nvgpu_err(g, "expecting init msg");
		err = -EINVAL;
		goto exit;
	}

	err = nvgpu_falcon_copy_from_emem(&sec2->flcn, tail + PMU_MSG_HDR_SIZE,
		(u8 *)&msg->msg, msg->hdr.size - PMU_MSG_HDR_SIZE, 0U);
	if (err != 0) {
		goto exit;
	}

	if (msg->msg.init.msg_type != NV_SEC2_INIT_MSG_ID_SEC2_INIT) {
		nvgpu_err(g, "expecting init msg");
		err = -EINVAL;
		goto exit;
	}

	tail += ALIGN(msg->hdr.size, PMU_DMEM_ALIGNMENT);
	g->ops.sec2.msgq_tail(g, sec2, &tail, QUEUE_SET);

	sec2_init = &msg->msg.init.sec2_init;

	err = nvgpu_sec2_queues_init(g, sec2->queues, sec2_init);
	if (err != 0) {
		return err;
	}

	nvgpu_sec2_dmem_allocator_init(g, &sec2->dmem, sec2_init);

	sec2->sec2_ready = true;

exit:
	return err;
}

int nvgpu_sec2_process_message(struct nvgpu_sec2 *sec2)
{
	struct gk20a *g = sec2->g;
	struct nv_flcn_msg_sec2 msg;
	int status = 0;

	if (unlikely(!sec2->sec2_ready)) {
		status = sec2_process_init_msg(sec2, &msg);
		goto exit;
	}

	while (sec2_read_message(sec2,
		SEC2_NV_MSGQ_LOG_ID, &msg, &status)) {

		nvgpu_sec2_dbg(g, "read msg hdr: ");
		nvgpu_sec2_dbg(g, "unit_id = 0x%08x, size = 0x%08x",
			msg.hdr.unit_id, msg.hdr.size);
		nvgpu_sec2_dbg(g, "ctrl_flags = 0x%08x, seq_id = 0x%08x",
			msg.hdr.ctrl_flags, msg.hdr.seq_id);

		msg.hdr.ctrl_flags &= ~PMU_CMD_FLAGS_PMU_MASK;

		if (msg.hdr.ctrl_flags == PMU_CMD_FLAGS_EVENT) {
			sec2_handle_event(sec2, &msg);
		} else {
			sec2_response_handle(sec2, &msg);
		}
	}

exit:
	return status;
}
