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

#include <nvgpu/gk20a.h>
#include <nvgpu/pmu.h>
#include <nvgpu/log.h>
#include <nvgpu/bug.h>
#include <nvgpu/timers.h>
#include <nvgpu/lock.h>
#include <nvgpu/sec2.h>
#include <nvgpu/engine_queue.h>
#include <nvgpu/sec2/queue.h>
#include <nvgpu/sec2if/sec2_if_sec2.h>
#include <nvgpu/sec2if/sec2_if_cmn.h>

/* command post operation functions */
static bool sec2_validate_cmd(struct nvgpu_sec2 *sec2,
	struct nv_flcn_cmd_sec2 *cmd, u32 queue_id)
{
	struct gk20a *g = sec2->g;
	u32 queue_size;

	if (queue_id != SEC2_NV_CMDQ_LOG_ID) {
		goto invalid_cmd;
	}

	if (cmd->hdr.size < PMU_CMD_HDR_SIZE) {
		goto invalid_cmd;
	}

	queue_size = nvgpu_sec2_queue_get_size(sec2->queues, queue_id);

	if (cmd->hdr.size > (queue_size >> 1)) {
		goto invalid_cmd;
	}

	if (!NV_SEC2_UNITID_IS_VALID(cmd->hdr.unit_id)) {
		goto invalid_cmd;
	}

	return true;

invalid_cmd:
	nvgpu_err(g, "invalid sec2 cmd :");
	nvgpu_err(g, "queue_id=%d, cmd_size=%d, cmd_unit_id=%d \n",
		queue_id, cmd->hdr.size, cmd->hdr.unit_id);

	return false;
}

static int sec2_write_cmd(struct nvgpu_sec2 *sec2,
	struct nv_flcn_cmd_sec2 *cmd, u32 queue_id,
	u32 timeout_ms)
{
	struct gk20a *g = sec2->g;
	struct nvgpu_timeout timeout;
	int err;

	nvgpu_log_fn(g, " ");

	nvgpu_timeout_init(g, &timeout, timeout_ms, NVGPU_TIMER_CPU_TIMER);

	do {
		err = nvgpu_sec2_queue_push(sec2->queues, queue_id, &sec2->flcn,
					    cmd, cmd->hdr.size);
		if ((err == -EAGAIN) && (nvgpu_timeout_expired(&timeout) == 0)) {
			nvgpu_usleep_range(1000U, 2000U);
		} else {
			break;
		}
	} while (true);

	if (err != 0) {
		nvgpu_err(g, "fail to write cmd to queue %d", queue_id);
	}

	return err;
}

int nvgpu_sec2_cmd_post(struct gk20a *g, struct nv_flcn_cmd_sec2 *cmd,
	u32 queue_id, sec2_callback callback,
	void *cb_param, u32 timeout)
{
	struct nvgpu_sec2 *sec2 = &g->sec2;
	struct sec2_sequence *seq = NULL;
	int err = 0;

	if ((cmd == NULL) || (!sec2->sec2_ready)) {
		if (cmd == NULL) {
			nvgpu_warn(g, "%s(): SEC2 cmd buffer is NULL", __func__);
		} else {
			nvgpu_warn(g, "%s(): SEC2 is not ready", __func__);
		}

		err = -EINVAL;
		goto exit;
	}

	/* Sanity check the command input. */
	if (!sec2_validate_cmd(sec2, cmd, queue_id)) {
		err = -EINVAL;
		goto exit;
	}

	/* Attempt to reserve a sequence for this command. */
	err = nvgpu_sec2_seq_acquire(g, &sec2->sequences, &seq,
				     callback, cb_param);
	if (err != 0) {
		goto exit;
	}

	/* Set the sequence number in the command header. */
	cmd->hdr.seq_id = nvgpu_sec2_seq_get_id(seq);

	cmd->hdr.ctrl_flags = 0U;
	cmd->hdr.ctrl_flags = PMU_CMD_FLAGS_STATUS;

	nvgpu_sec2_seq_set_state(seq, SEC2_SEQ_STATE_USED);

	err = sec2_write_cmd(sec2, cmd, queue_id, timeout);
	if (err != 0) {
		nvgpu_sec2_seq_set_state(seq, SEC2_SEQ_STATE_PENDING);
	}

exit:
	return err;
}

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

	*status = 0;

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

	if (msg->msg.init.msg_type != PMU_INIT_MSG_TYPE_PMU_INIT) {
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

	if (!nvgpu_alloc_initialized(&sec2->dmem)) {
		/* Align start and end addresses */
		u32 start = ALIGN(sec2_init->nv_managed_area_offset,
			PMU_DMEM_ALLOC_ALIGNMENT);

		u32 end = (sec2_init->nv_managed_area_offset +
			sec2_init->nv_managed_area_size) &
			~(PMU_DMEM_ALLOC_ALIGNMENT - 1U);
		u32 size = end - start;

		nvgpu_bitmap_allocator_init(g, &sec2->dmem, "sec2_dmem",
			start, size, PMU_DMEM_ALLOC_ALIGNMENT, 0U);
	}

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

int nvgpu_sec2_wait_message_cond(struct nvgpu_sec2 *sec2, u32 timeout_ms,
	void *var, u8 val)
{
	struct gk20a *g = sec2->g;
	struct nvgpu_timeout timeout;
	u32 delay = POLL_DELAY_MIN_US;

	nvgpu_timeout_init(g, &timeout, timeout_ms, NVGPU_TIMER_CPU_TIMER);

	do {
		if (*(u8 *)var == val) {
			return 0;
		}

		if (g->ops.sec2.is_interrupted(&g->sec2)) {
			g->ops.sec2.isr(g);
		}

		nvgpu_usleep_range(delay, delay * 2U);
		delay = min_t(u32, delay << 1U, POLL_DELAY_MAX_US);
	} while (nvgpu_timeout_expired(&timeout) == 0);

	return -ETIMEDOUT;
}
