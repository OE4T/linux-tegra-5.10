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

#include <nvgpu/enabled.h>
#include <nvgpu/pmu.h>
#include <nvgpu/log.h>
#include <nvgpu/timers.h>
#include <nvgpu/bug.h>
#include <nvgpu/pmuif/nvgpu_gpmu_cmdif.h>
#include <nvgpu/pmuif/gpmu_super_surf_if.h>
#include <nvgpu/falcon.h>
#include <nvgpu/engine_fb_queue.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/string.h>
#include <nvgpu/pmu/seq.h>
#include <nvgpu/pmu/queue.h>

static bool pmu_validate_cmd(struct nvgpu_pmu *pmu, struct pmu_cmd *cmd,
			struct pmu_payload *payload, u32 queue_id)
{
	struct gk20a *g = gk20a_from_pmu(pmu);
	u32 queue_size;
	u32 in_size, out_size;

	if (!PMU_IS_SW_COMMAND_QUEUE(queue_id)) {
		goto invalid_cmd;
	}


	if (cmd->hdr.size < PMU_CMD_HDR_SIZE) {
		goto invalid_cmd;
	}

	queue_size = nvgpu_pmu_queue_get_size(&pmu->queues, queue_id);

	if (cmd->hdr.size > (queue_size >> 1)) {
		goto invalid_cmd;
	}

	if (!PMU_UNIT_ID_IS_VALID(cmd->hdr.unit_id)) {
		goto invalid_cmd;
	}

	if (payload == NULL) {
		return true;
	}

	if (payload->in.buf == NULL && payload->out.buf == NULL &&
		payload->rpc.prpc == NULL) {
		goto invalid_cmd;
	}

	if ((payload->in.buf != NULL && payload->in.size == 0U) ||
	    (payload->out.buf != NULL && payload->out.size == 0U) ||
		(payload->rpc.prpc != NULL && payload->rpc.size_rpc == 0U)) {
		goto invalid_cmd;
	}

	in_size = PMU_CMD_HDR_SIZE;
	if (payload->in.buf != NULL) {
		in_size += payload->in.offset;
		in_size += g->ops.pmu_ver.get_pmu_allocation_struct_size(pmu);
	}

	out_size = PMU_CMD_HDR_SIZE;
	if (payload->out.buf != NULL) {
		out_size += payload->out.offset;
		out_size += g->ops.pmu_ver.get_pmu_allocation_struct_size(pmu);
	}

	if (in_size > cmd->hdr.size || out_size > cmd->hdr.size) {
		goto invalid_cmd;
	}


	if ((payload->in.offset != 0U && payload->in.buf == NULL) ||
	    (payload->out.offset != 0U && payload->out.buf == NULL)) {
		goto invalid_cmd;
	}

	return true;

invalid_cmd:
	nvgpu_err(g, "invalid pmu cmd :\n"
		"queue_id=%d,\n"
		"cmd_size=%d, cmd_unit_id=%d,\n"
		"payload in=%p, in_size=%d, in_offset=%d,\n"
		"payload out=%p, out_size=%d, out_offset=%d",
		queue_id, cmd->hdr.size, cmd->hdr.unit_id,
		&payload->in, payload->in.size, payload->in.offset,
		&payload->out, payload->out.size, payload->out.offset);

	return false;
}

static int pmu_write_cmd(struct nvgpu_pmu *pmu, struct pmu_cmd *cmd,
			u32 queue_id)
{
	struct gk20a *g = gk20a_from_pmu(pmu);
	struct nvgpu_timeout timeout;
	int err;

	nvgpu_log_fn(g, " ");

	nvgpu_timeout_init(g, &timeout, U32_MAX, NVGPU_TIMER_CPU_TIMER);

	do {
		err = nvgpu_pmu_queue_push(&pmu->queues, &pmu->flcn,
					   queue_id, cmd);
		if (err == -EAGAIN && nvgpu_timeout_expired(&timeout) == 0) {
			nvgpu_usleep_range(1000, 2000);
		} else {
			break;
		}
	} while (true);

	if (err != 0) {
		nvgpu_err(g, "fail to write cmd to queue %d", queue_id);
	} else {
		nvgpu_log_fn(g, "done");
	}

	return err;
}

static int pmu_payload_allocate(struct gk20a *g, struct pmu_sequence *seq,
	struct falcon_payload_alloc *alloc)
{
	struct nvgpu_pmu *pmu = &g->pmu;
	u16 buffer_size;
	int err = 0;
	u64 tmp;

	if (alloc->fb_surface == NULL &&
		alloc->fb_size != 0x0U) {

		alloc->fb_surface = nvgpu_kzalloc(g, sizeof(struct nvgpu_mem));
		if (alloc->fb_surface == NULL) {
			err = -ENOMEM;
			goto clean_up;
		}
		nvgpu_pmu_vidmem_surface_alloc(g, alloc->fb_surface, alloc->fb_size);
	}

	if (nvgpu_pmu_fb_queue_enabled(&pmu->queues)) {
		buffer_size = nvgpu_pmu_seq_get_buffer_size(seq);
		nvgpu_pmu_seq_set_fbq_out_offset(seq, buffer_size);
		/* Save target address in FBQ work buffer. */
		alloc->dmem_offset = buffer_size;
		buffer_size += alloc->dmem_size;
		nvgpu_pmu_seq_set_buffer_size(seq, buffer_size);
	} else {
		tmp = nvgpu_alloc(&pmu->dmem, alloc->dmem_size);
		nvgpu_assert(tmp <= U32_MAX);
		alloc->dmem_offset = (u32)tmp;
		if (alloc->dmem_offset == 0U) {
			err = -ENOMEM;
			goto clean_up;
		}
	}

clean_up:
	return err;
}

static int pmu_cmd_payload_setup_rpc(struct gk20a *g, struct pmu_cmd *cmd,
	struct pmu_payload *payload, struct pmu_sequence *seq)
{
	struct nvgpu_pmu *pmu = &g->pmu;
	struct pmu_v *pv = &g->ops.pmu_ver;
	struct nvgpu_engine_fb_queue *queue = nvgpu_pmu_seq_get_cmd_queue(seq);
	struct falcon_payload_alloc alloc;
	int err = 0;

	nvgpu_log_fn(g, " ");

	memset(&alloc, 0, sizeof(struct falcon_payload_alloc));

	alloc.dmem_size = payload->rpc.size_rpc +
		payload->rpc.size_scratch;

	err = pmu_payload_allocate(g, seq, &alloc);
	if (err != 0) {
		goto clean_up;
	}

	alloc.dmem_size = payload->rpc.size_rpc;

	if (nvgpu_pmu_fb_queue_enabled(&pmu->queues)) {
		/* copy payload to FBQ work buffer */
		nvgpu_memcpy((u8 *)
			nvgpu_engine_fb_queue_get_work_buffer(queue) +
			alloc.dmem_offset,
			(u8 *)payload->rpc.prpc, payload->rpc.size_rpc);

		alloc.dmem_offset += nvgpu_pmu_seq_get_fbq_heap_offset(seq);

		nvgpu_pmu_seq_set_in_payload_fb_queue(seq, true);
		nvgpu_pmu_seq_set_out_payload_fb_queue(seq, true);
	} else {
		nvgpu_falcon_copy_to_dmem(&pmu->flcn, alloc.dmem_offset,
			payload->rpc.prpc, payload->rpc.size_rpc, 0);
	}

	cmd->cmd.rpc.rpc_dmem_size = payload->rpc.size_rpc;
	cmd->cmd.rpc.rpc_dmem_ptr  = alloc.dmem_offset;

	nvgpu_pmu_seq_set_out_payload(seq, payload->rpc.prpc);
	pv->pmu_allocation_set_dmem_size(pmu,
		pv->get_pmu_seq_out_a_ptr(seq),
		payload->rpc.size_rpc);
	pv->pmu_allocation_set_dmem_offset(pmu,
		pv->get_pmu_seq_out_a_ptr(seq),
		alloc.dmem_offset);

clean_up:
	if (err != 0) {
		nvgpu_log_fn(g, "fail");
	} else {
		nvgpu_log_fn(g, "done");
	}

	return err;
}

static int pmu_cmd_payload_setup(struct gk20a *g, struct pmu_cmd *cmd,
	struct pmu_payload *payload, struct pmu_sequence *seq)
{
	struct nvgpu_engine_fb_queue *fb_queue =
				nvgpu_pmu_seq_get_cmd_queue(seq);
	struct pmu_v *pv = &g->ops.pmu_ver;
	struct falcon_payload_alloc alloc;
	struct nvgpu_pmu *pmu = &g->pmu;
	void *in = NULL, *out = NULL;
	int err = 0;

	nvgpu_log_fn(g, " ");

	if (payload != NULL) {
		nvgpu_pmu_seq_set_out_payload(seq, payload->out.buf);
	}

	memset(&alloc, 0, sizeof(struct falcon_payload_alloc));

	if (payload != NULL && payload->in.offset != 0U) {
		pv->set_pmu_allocation_ptr(pmu, &in,
		((u8 *)&cmd->cmd + payload->in.offset));

		if (payload->in.buf != payload->out.buf) {
			pv->pmu_allocation_set_dmem_size(pmu, in,
			(u16)payload->in.size);
		} else {
			pv->pmu_allocation_set_dmem_size(pmu, in,
			(u16)max(payload->in.size, payload->out.size));
		}

		alloc.dmem_size = pv->pmu_allocation_get_dmem_size(pmu, in);

		if (payload->in.fb_size != 0x0U) {
			alloc.fb_size = payload->in.fb_size;
		}

		err = pmu_payload_allocate(g, seq, &alloc);
		if (err != 0) {
			goto clean_up;
		}

		*(pv->pmu_allocation_get_dmem_offset_addr(pmu, in)) =
			alloc.dmem_offset;

		if (payload->in.fb_size != 0x0U) {
			nvgpu_pmu_seq_set_in_mem(seq, alloc.fb_surface);
			nvgpu_pmu_surface_describe(g, alloc.fb_surface,
				(struct flcn_mem_desc_v0 *)
				pv->pmu_allocation_get_fb_addr(pmu, in));

			nvgpu_mem_wr_n(g, alloc.fb_surface, 0,
				payload->in.buf, payload->in.fb_size);

			if (nvgpu_pmu_fb_queue_enabled(&pmu->queues)) {
				alloc.dmem_offset +=
					nvgpu_pmu_seq_get_fbq_heap_offset(seq);
				*(pv->pmu_allocation_get_dmem_offset_addr(pmu, in)) =
					alloc.dmem_offset;
			}
		} else {
			if (nvgpu_pmu_fb_queue_enabled(&pmu->queues)) {
				/* copy payload to FBQ work buffer */
				nvgpu_memcpy((u8 *)
					nvgpu_engine_fb_queue_get_work_buffer(
							fb_queue) +
					alloc.dmem_offset,
					(u8 *)payload->in.buf,
					payload->in.size);

				alloc.dmem_offset +=
					nvgpu_pmu_seq_get_fbq_heap_offset(seq);
				*(pv->pmu_allocation_get_dmem_offset_addr(pmu, in)) =
					alloc.dmem_offset;

				nvgpu_pmu_seq_set_in_payload_fb_queue(seq,
								      true);
			} else {
				nvgpu_falcon_copy_to_dmem(&pmu->flcn,
						(pv->pmu_allocation_get_dmem_offset(pmu, in)),
						payload->in.buf, payload->in.size, 0);
			}
		}
		pv->pmu_allocation_set_dmem_size(pmu,
		pv->get_pmu_seq_in_a_ptr(seq),
		pv->pmu_allocation_get_dmem_size(pmu, in));
		pv->pmu_allocation_set_dmem_offset(pmu,
		pv->get_pmu_seq_in_a_ptr(seq),
		pv->pmu_allocation_get_dmem_offset(pmu, in));
	}

	if (payload != NULL && payload->out.offset != 0U) {
		pv->set_pmu_allocation_ptr(pmu, &out,
		((u8 *)&cmd->cmd + payload->out.offset));
		pv->pmu_allocation_set_dmem_size(pmu, out,
		(u16)payload->out.size);

		if (payload->in.buf != payload->out.buf) {
			alloc.dmem_size = pv->pmu_allocation_get_dmem_size(pmu, out);

			if (payload->out.fb_size != 0x0U) {
				alloc.fb_size = payload->out.fb_size;
			}

			err = pmu_payload_allocate(g, seq, &alloc);
			if (err != 0) {
				goto clean_up;
			}

			*(pv->pmu_allocation_get_dmem_offset_addr(pmu, out)) =
				alloc.dmem_offset;
			nvgpu_pmu_seq_set_out_mem(seq, alloc.fb_surface);
		} else {
			BUG_ON(in == NULL);
			nvgpu_pmu_seq_set_out_mem(seq,
						nvgpu_pmu_seq_get_in_mem(seq));
			pv->pmu_allocation_set_dmem_offset(pmu, out,
			pv->pmu_allocation_get_dmem_offset(pmu, in));
		}

		if (payload->out.fb_size != 0x0U) {
			nvgpu_pmu_surface_describe(g,
				nvgpu_pmu_seq_get_out_mem(seq),
				(struct flcn_mem_desc_v0 *)
				pv->pmu_allocation_get_fb_addr(pmu,
				out));
		}

		if (nvgpu_pmu_fb_queue_enabled(&pmu->queues)) {
			if (payload->in.buf != payload->out.buf) {
				*(pv->pmu_allocation_get_dmem_offset_addr(pmu,
					out)) +=
					nvgpu_pmu_seq_get_fbq_heap_offset(seq);
			}

			nvgpu_pmu_seq_set_out_payload_fb_queue(seq, true);
		}

		pv->pmu_allocation_set_dmem_size(pmu,
		pv->get_pmu_seq_out_a_ptr(seq),
		pv->pmu_allocation_get_dmem_size(pmu, out));
		pv->pmu_allocation_set_dmem_offset(pmu,
		pv->get_pmu_seq_out_a_ptr(seq),
		pv->pmu_allocation_get_dmem_offset(pmu, out));
	}

clean_up:
	if (err != 0) {
		nvgpu_log_fn(g, "fail");
		if (in != NULL) {
			nvgpu_free(&pmu->dmem,
				pv->pmu_allocation_get_dmem_offset(pmu, in));
		}
		if (out != NULL) {
			nvgpu_free(&pmu->dmem,
				pv->pmu_allocation_get_dmem_offset(pmu, out));
		}
	} else {
		nvgpu_log_fn(g, "done");
	}

	return err;
}

static int pmu_fbq_cmd_setup(struct gk20a *g, struct pmu_cmd *cmd,
	struct nvgpu_engine_fb_queue *queue, struct pmu_payload *payload,
	struct pmu_sequence *seq)
{
	struct nvgpu_pmu *pmu = &g->pmu;
	struct nv_falcon_fbq_hdr *fbq_hdr = NULL;
	struct pmu_cmd *flcn_cmd = NULL;
	u32 fbq_size_needed = 0;
	u16 heap_offset = 0;
	u64 tmp;
	int err = 0;

	fbq_hdr = (struct nv_falcon_fbq_hdr *)
		nvgpu_engine_fb_queue_get_work_buffer(queue);

	flcn_cmd = (struct pmu_cmd *)
		(nvgpu_engine_fb_queue_get_work_buffer(queue) +
		sizeof(struct nv_falcon_fbq_hdr));

	if (cmd->cmd.rpc.cmd_type == NV_PMU_RPC_CMD_ID) {
		if (payload != NULL) {
			fbq_size_needed = (u32)payload->rpc.size_rpc +
					(u32)payload->rpc.size_scratch;
		}
	} else {
		if (payload != NULL) {
			if (payload->in.offset != 0U) {
				if (payload->in.buf != payload->out.buf) {
					fbq_size_needed = payload->in.size;
				} else {
					fbq_size_needed = max(payload->in.size,
						payload->out.size);
				}
			}

			if (payload->out.offset != 0U) {
				if (payload->out.buf != payload->in.buf) {
					fbq_size_needed += (u16)payload->out.size;
				}
			}
		}
	}

	tmp = fbq_size_needed +
		sizeof(struct nv_falcon_fbq_hdr) +
		cmd->hdr.size;
	nvgpu_assert(tmp <= (size_t)U32_MAX);
	fbq_size_needed = (u32)tmp;

	fbq_size_needed = ALIGN_UP(fbq_size_needed, 4);

	tmp = nvgpu_alloc(&pmu->dmem, fbq_size_needed);
	nvgpu_assert(tmp <= U32_MAX);
	heap_offset = (u16) tmp;
	if (heap_offset == 0U) {
		err = -ENOMEM;
		goto exit;
	}

	/* clear work queue buffer */
	memset(nvgpu_engine_fb_queue_get_work_buffer(queue), 0,
		nvgpu_engine_fb_queue_get_element_size(queue));

	/* Need to save room for both FBQ hdr, and the CMD */
	tmp = sizeof(struct nv_falcon_fbq_hdr) +
		   cmd->hdr.size;
	nvgpu_assert(tmp <= (size_t)U16_MAX);
	nvgpu_pmu_seq_set_buffer_size(seq, (u16)tmp);

	/* copy cmd into the work buffer */
	nvgpu_memcpy((u8 *)flcn_cmd, (u8 *)cmd, cmd->hdr.size);

	/* Fill in FBQ hdr, and offset in seq structure */
	nvgpu_assert(fbq_size_needed < U16_MAX);
	fbq_hdr->heap_size = (u16)fbq_size_needed;
	fbq_hdr->heap_offset = heap_offset;
	nvgpu_pmu_seq_set_fbq_heap_offset(seq, heap_offset);

	/*
	 * save queue index in seq structure
	 * so can free queue element when response is received
	 */
	nvgpu_pmu_seq_set_fbq_element_index(seq,
				nvgpu_engine_fb_queue_get_position(queue));

exit:
	return err;
}

int nvgpu_pmu_cmd_post(struct gk20a *g, struct pmu_cmd *cmd,
		struct pmu_payload *payload,
		u32 queue_id, pmu_callback callback, void *cb_param)
{
	struct nvgpu_pmu *pmu = &g->pmu;
	struct pmu_sequence *seq = NULL;
	struct nvgpu_engine_fb_queue *fb_queue = NULL;
	int err;

	nvgpu_log_fn(g, " ");

	if (cmd == NULL || !pmu->pmu_ready) {
		if (cmd == NULL) {
			nvgpu_warn(g, "%s(): PMU cmd buffer is NULL", __func__);
		} else {
			nvgpu_warn(g, "%s(): PMU is not ready", __func__);
		}

		WARN_ON(true);
		return -EINVAL;
	}

	if (!pmu_validate_cmd(pmu, cmd, payload, queue_id)) {
		return -EINVAL;
	}

	err = nvgpu_pmu_seq_acquire(g, &pmu->sequences, &seq, callback,
				    cb_param);
	if (err != 0) {
		return err;
	}

	cmd->hdr.seq_id = nvgpu_pmu_seq_get_id(seq);

	cmd->hdr.ctrl_flags = 0;
	cmd->hdr.ctrl_flags |= PMU_CMD_FLAGS_STATUS;
	cmd->hdr.ctrl_flags |= PMU_CMD_FLAGS_INTR;

	if (nvgpu_pmu_fb_queue_enabled(&pmu->queues)) {
		fb_queue = nvgpu_pmu_fb_queue(&pmu->queues, queue_id);
		/* Save the queue in the seq structure. */
		nvgpu_pmu_seq_set_cmd_queue(seq, fb_queue);

		/* Lock the FBQ work buffer */
		nvgpu_engine_fb_queue_lock_work_buffer(fb_queue);

		/* Create FBQ work buffer & copy cmd to FBQ work buffer */
		err = pmu_fbq_cmd_setup(g, cmd, fb_queue, payload, seq);
		if (err != 0) {
			nvgpu_err(g, "FBQ cmd setup failed");
			nvgpu_pmu_seq_release(g, &pmu->sequences, seq);
			goto exit;
		}

		/*
		 * change cmd pointer to point to FBQ work
		 * buffer as cmd copied to FBQ work buffer
		 * in call pmu_fgq_cmd_setup()
		 */
		cmd = (struct pmu_cmd *)
			(nvgpu_engine_fb_queue_get_work_buffer(fb_queue) +
			sizeof(struct nv_falcon_fbq_hdr));
	}

	if (cmd->cmd.rpc.cmd_type == NV_PMU_RPC_CMD_ID) {
		err = pmu_cmd_payload_setup_rpc(g, cmd, payload, seq);
	} else {
		err = pmu_cmd_payload_setup(g, cmd, payload, seq);
	}

	if (err != 0) {
		nvgpu_err(g, "payload setup failed");
		g->ops.pmu_ver.pmu_allocation_set_dmem_size(pmu,
			g->ops.pmu_ver.get_pmu_seq_in_a_ptr(seq), 0);
		g->ops.pmu_ver.pmu_allocation_set_dmem_size(pmu,
			g->ops.pmu_ver.get_pmu_seq_out_a_ptr(seq), 0);

		nvgpu_pmu_seq_release(g, &pmu->sequences, seq);
		goto exit;
	}

	nvgpu_pmu_seq_set_state(seq, PMU_SEQ_STATE_USED);

	err = pmu_write_cmd(pmu, cmd, queue_id);
	if (err != 0) {
		nvgpu_pmu_seq_set_state(seq, PMU_SEQ_STATE_PENDING);
	}

exit:
	if (nvgpu_pmu_fb_queue_enabled(&pmu->queues)) {
		/* Unlock the FBQ work buffer */
		nvgpu_engine_fb_queue_unlock_work_buffer(fb_queue);
	}

	nvgpu_log_fn(g, "Done, err %x", err);
	return err;
}

static int pmu_payload_extract(struct nvgpu_pmu *pmu, struct pmu_sequence *seq)
{
	struct nvgpu_engine_fb_queue *fb_queue =
				nvgpu_pmu_seq_get_cmd_queue(seq);
	struct gk20a *g = gk20a_from_pmu(pmu);
	struct pmu_v *pv = &g->ops.pmu_ver;
	u32 fbq_payload_offset = 0U;
	int err = 0;

	nvgpu_log_fn(g, " ");

	if (nvgpu_pmu_seq_get_out_payload_fb_queue(seq)) {
		fbq_payload_offset =
			nvgpu_engine_fb_queue_get_offset(fb_queue) +
			nvgpu_pmu_seq_get_fbq_out_offset(seq) +
			(nvgpu_pmu_seq_get_fbq_element_index(seq) *
			 nvgpu_engine_fb_queue_get_element_size(fb_queue));

		nvgpu_mem_rd_n(g, &pmu->super_surface_buf, fbq_payload_offset,
			nvgpu_pmu_seq_get_out_payload(seq),
			pv->pmu_allocation_get_dmem_size(pmu,
			pv->get_pmu_seq_out_a_ptr(seq)));

	} else {
		if (pv->pmu_allocation_get_dmem_size(pmu,
			pv->get_pmu_seq_out_a_ptr(seq)) != 0U) {
			err = nvgpu_falcon_copy_from_dmem(&pmu->flcn,
					pv->pmu_allocation_get_dmem_offset(pmu,
					pv->get_pmu_seq_out_a_ptr(seq)),
					nvgpu_pmu_seq_get_out_payload(seq),
					pv->pmu_allocation_get_dmem_size(pmu,
					pv->get_pmu_seq_out_a_ptr(seq)), 0);
			if (err != 0) {
				nvgpu_err(g, "PMU falcon DMEM copy failed");
				return err;
			}
		}
	}

	return err;
}

static void pmu_payload_free(struct nvgpu_pmu *pmu, struct pmu_sequence *seq)
{
	struct nvgpu_engine_fb_queue *fb_queue =
				nvgpu_pmu_seq_get_cmd_queue(seq);
	struct gk20a *g = gk20a_from_pmu(pmu);
	struct pmu_v *pv = &g->ops.pmu_ver;
	struct nvgpu_mem *in_mem = nvgpu_pmu_seq_get_in_mem(seq);
	struct nvgpu_mem *out_mem = nvgpu_pmu_seq_get_out_mem(seq);
	void *seq_in_ptr = pv->get_pmu_seq_in_a_ptr(seq);
	void *seq_out_ptr = pv->get_pmu_seq_out_a_ptr(seq);
	int err;

	nvgpu_log_fn(g, " ");

	if (nvgpu_pmu_fb_queue_enabled(&pmu->queues)) {
		nvgpu_free(&pmu->dmem, nvgpu_pmu_seq_get_fbq_heap_offset(seq));

		/*
		 * free FBQ allocated work buffer
		 * set FBQ element work buffer to NULL
		 * Clear the in use bit for the queue entry this CMD used.
		 */
		err = nvgpu_engine_fb_queue_free_element(fb_queue,
				nvgpu_pmu_seq_get_fbq_element_index(seq));
		if (err != 0) {
			nvgpu_err(g, "fb queue element free failed %d", err);
		}
	} else {
		/* free DMEM space payload*/
		if (pv->pmu_allocation_get_dmem_size(pmu, seq_in_ptr) != 0U) {
			nvgpu_free(&pmu->dmem,
				    pv->pmu_allocation_get_dmem_offset(pmu,
					seq_in_ptr));

			pv->pmu_allocation_set_dmem_size(pmu, seq_in_ptr, 0);
		}

		if (pv->pmu_allocation_get_dmem_size(pmu, seq_out_ptr) != 0U) {
			nvgpu_free(&pmu->dmem,
				   pv->pmu_allocation_get_dmem_offset(pmu,
					seq_out_ptr));

			pv->pmu_allocation_set_dmem_size(pmu, seq_out_ptr, 0);
		}
	}

	if (out_mem != NULL) {
		(void) memset(pv->pmu_allocation_get_fb_addr(pmu, seq_out_ptr),
			      0x0,
			      pv->pmu_allocation_get_fb_size(pmu, seq_out_ptr));

		nvgpu_pmu_surface_free(g, out_mem);
		if (out_mem != in_mem) {
			nvgpu_kfree(g, out_mem);
		}
	}

	if (in_mem != NULL) {
		(void) memset(pv->pmu_allocation_get_fb_addr(pmu, seq_in_ptr),
			      0x0,
			      pv->pmu_allocation_get_fb_size(pmu, seq_in_ptr));

		nvgpu_pmu_surface_free(g, in_mem);
		nvgpu_kfree(g, in_mem);
	}

	nvgpu_pmu_seq_payload_free(g, seq);
}

static int pmu_response_handle(struct nvgpu_pmu *pmu,
			struct pmu_msg *msg)
{
	struct gk20a *g = gk20a_from_pmu(pmu);
	enum pmu_seq_state state;
	struct pmu_sequence *seq;
	int err = 0;
	u8 id;

	nvgpu_log_fn(g, " ");

	seq = nvgpu_pmu_sequences_get_seq(&pmu->sequences, msg->hdr.seq_id);
	state = nvgpu_pmu_seq_get_state(seq);
	id = nvgpu_pmu_seq_get_id(seq);

	if (state != PMU_SEQ_STATE_USED) {
		nvgpu_err(g, "msg for an unknown sequence %u", (u32) id);
		err = -EINVAL;
		goto exit;
	}

	if (msg->hdr.unit_id == PMU_UNIT_RC &&
		msg->msg.rc.msg_type == PMU_RC_MSG_TYPE_UNHANDLED_CMD) {
		nvgpu_err(g, "unhandled cmd: seq %u", (u32) id);
		err = -EINVAL;
	} else {
		err = pmu_payload_extract(pmu, seq);
	}

exit:
	/*
	 * free allocated space for payload in
	 * DMEM/FB-surface/FB_QUEUE as data is
	 * copied to buffer pointed by
	 * seq->out_payload
	 */
	pmu_payload_free(pmu, seq);

	nvgpu_pmu_seq_callback(g, seq, msg, err);

	nvgpu_pmu_seq_release(g, &pmu->sequences, seq);

	/* TBD: notify client waiting for available dmem */

	nvgpu_log_fn(g, "done err %d", err);

	return err;
}

static int pmu_handle_event(struct nvgpu_pmu *pmu, struct pmu_msg *msg)
{
	int err = 0;
	struct gk20a *g = gk20a_from_pmu(pmu);

	nvgpu_log_fn(g, " ");
	switch (msg->hdr.unit_id) {
	case PMU_UNIT_PERFMON:
	case PMU_UNIT_PERFMON_T18X:
		err = nvgpu_pmu_handle_perfmon_event(pmu, &msg->msg.perfmon);
		break;
	case PMU_UNIT_PERF:
		if (g->ops.pmu_perf.handle_pmu_perf_event != NULL) {
			err = g->ops.pmu_perf.handle_pmu_perf_event(g,
				(void *)&msg->msg.perf);
		} else {
			WARN_ON(true);
		}
		break;
	case PMU_UNIT_THERM:
		err = nvgpu_pmu_handle_therm_event(pmu, &msg->msg.therm);
		break;
	default:
		break;
	}

	return err;
}

static bool pmu_engine_mem_queue_read(struct nvgpu_pmu *pmu,
	u32 queue_id, void *data,
	u32 bytes_to_read, int *status)
{
	struct gk20a *g = gk20a_from_pmu(pmu);
	u32 bytes_read;
	int err;

	err = nvgpu_pmu_queue_pop(&pmu->queues, &pmu->flcn, queue_id, data,
				  bytes_to_read, &bytes_read);
	if (err != 0) {
		nvgpu_err(g, "fail to read msg: err %d", err);
		*status = err;
		return false;
	}
	if (bytes_read != bytes_to_read) {
		nvgpu_err(g, "fail to read requested bytes: 0x%x != 0x%x",
			bytes_to_read, bytes_read);
		*status = -EINVAL;
		return false;
	}

	return true;
}

static bool pmu_read_message(struct nvgpu_pmu *pmu, u32 queue_id,
	struct pmu_msg *msg, int *status)
{
	struct gk20a *g = gk20a_from_pmu(pmu);
	u32 read_size;
	int err;

	*status = 0;

	if (nvgpu_pmu_queue_is_empty(&pmu->queues, queue_id)) {
		return false;
	}

	if (!pmu_engine_mem_queue_read(pmu, queue_id, &msg->hdr,
				       PMU_MSG_HDR_SIZE, status)) {
		nvgpu_err(g, "fail to read msg from queue %d", queue_id);
		goto clean_up;
	}

	if (msg->hdr.unit_id == PMU_UNIT_REWIND) {
		if (!nvgpu_pmu_fb_queue_enabled(&pmu->queues)) {
			err = nvgpu_pmu_queue_rewind(&pmu->queues, queue_id,
						     &pmu->flcn);
			if (err != 0) {
				nvgpu_err(g, "fail to rewind queue %d",
					  queue_id);
				*status = err;
				goto clean_up;
			}
		}

		/* read again after rewind */
		if (!pmu_engine_mem_queue_read(pmu, queue_id, &msg->hdr,
				PMU_MSG_HDR_SIZE, status)) {
			nvgpu_err(g, "fail to read msg from queue %d",
				queue_id);
			goto clean_up;
		}
	}

	if (!PMU_UNIT_ID_IS_VALID(msg->hdr.unit_id)) {
		nvgpu_err(g, "read invalid unit_id %d from queue %d",
			msg->hdr.unit_id, queue_id);
			*status = -EINVAL;
			goto clean_up;
	}

	if (msg->hdr.size > PMU_MSG_HDR_SIZE) {
		read_size = msg->hdr.size - PMU_MSG_HDR_SIZE;
		if (!pmu_engine_mem_queue_read(pmu, queue_id, &msg->msg,
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

int nvgpu_pmu_process_message(struct nvgpu_pmu *pmu)
{
	struct pmu_msg msg;
	int status;
	struct gk20a *g = gk20a_from_pmu(pmu);

	if (unlikely(!pmu->pmu_ready)) {
		nvgpu_pmu_process_init_msg(pmu, &msg);
		if (g->ops.pmu.init_wpr_region != NULL) {
			g->ops.pmu.init_wpr_region(g);
		}

		if (nvgpu_is_enabled(g, NVGPU_PMU_PERFMON)) {
			g->ops.pmu.pmu_init_perfmon(pmu);
		}

		return 0;
	}

	while (pmu_read_message(pmu, PMU_MESSAGE_QUEUE, &msg, &status)) {

		nvgpu_pmu_dbg(g, "read msg hdr: ");
		nvgpu_pmu_dbg(g, "unit_id = 0x%08x, size = 0x%08x",
			msg.hdr.unit_id, msg.hdr.size);
		nvgpu_pmu_dbg(g, "ctrl_flags = 0x%08x, seq_id = 0x%08x",
			msg.hdr.ctrl_flags, msg.hdr.seq_id);

		msg.hdr.ctrl_flags &= ~PMU_CMD_FLAGS_PMU_MASK;

		if (msg.hdr.ctrl_flags == PMU_CMD_FLAGS_EVENT) {
			pmu_handle_event(pmu, &msg);
		} else {
			pmu_response_handle(pmu, &msg);
		}
	}

	return 0;
}

int pmu_wait_message_cond_status(struct nvgpu_pmu *pmu, u32 timeout_ms,
				 void *var, u8 val)
{
	struct gk20a *g = gk20a_from_pmu(pmu);
	struct nvgpu_timeout timeout;
	int err;
	unsigned int delay = POLL_DELAY_MIN_US;

	err = nvgpu_timeout_init(g, &timeout, timeout_ms,
		NVGPU_TIMER_CPU_TIMER);
	if (err != 0) {
		nvgpu_err(g, "PMU wait timeout init failed.");
		return err;
	}

	do {
		nvgpu_rmb();

		if (*(volatile u8 *)var == val) {
			return 0;
		}

		if (g->ops.pmu.pmu_is_interrupted(pmu)) {
			g->ops.pmu.pmu_isr(g);
		}

		nvgpu_usleep_range(delay, delay * 2U);
		delay = min_t(u32, delay << 1, POLL_DELAY_MAX_US);
	} while (nvgpu_timeout_expired(&timeout) == 0);

	return -ETIMEDOUT;
}

void pmu_wait_message_cond(struct nvgpu_pmu *pmu, u32 timeout_ms,
			void *var, u8 val)
{
	struct gk20a *g = gk20a_from_pmu(pmu);

	if (pmu_wait_message_cond_status(pmu, timeout_ms, var, val) != 0) {
		nvgpu_err(g, "PMU wait timeout expired.");
	}
}

static void pmu_rpc_handler(struct gk20a *g, struct pmu_msg *msg,
		void *param, u32 status)
{
	struct nv_pmu_rpc_header rpc;
	struct nvgpu_pmu *pmu = &g->pmu;
	struct rpc_handler_payload *rpc_payload =
		(struct rpc_handler_payload *)param;
	struct nv_pmu_rpc_struct_perfmon_query *rpc_param;

	(void) memset(&rpc, 0, sizeof(struct nv_pmu_rpc_header));
	nvgpu_memcpy((u8 *)&rpc, (u8 *)rpc_payload->rpc_buff,
		sizeof(struct nv_pmu_rpc_header));

	if (rpc.flcn_status != 0U) {
		nvgpu_err(g,
			"failed RPC response, unit-id=0x%x, func=0x%x, status=0x%x",
			rpc.unit_id, rpc.function, rpc.flcn_status);
		goto exit;
	}

	switch (msg->hdr.unit_id) {
	case PMU_UNIT_ACR:
		switch (rpc.function) {
		case NV_PMU_RPC_ID_ACR_INIT_WPR_REGION:
			nvgpu_pmu_dbg(g,
				"reply NV_PMU_RPC_ID_ACR_INIT_WPR_REGION");
			g->pmu_lsf_pmu_wpr_init_done = true;
			break;
		case NV_PMU_RPC_ID_ACR_BOOTSTRAP_GR_FALCONS:
			nvgpu_pmu_dbg(g,
				"reply NV_PMU_RPC_ID_ACR_BOOTSTRAP_GR_FALCONS");
			g->pmu_lsf_loaded_falcon_id = 1;
			break;
		}
		break;
	case PMU_UNIT_PERFMON_T18X:
	case PMU_UNIT_PERFMON:
		switch (rpc.function) {
		case NV_PMU_RPC_ID_PERFMON_T18X_INIT:
			nvgpu_pmu_dbg(g,
				"reply NV_PMU_RPC_ID_PERFMON_INIT");
			pmu->perfmon_ready = true;
			break;
		case NV_PMU_RPC_ID_PERFMON_T18X_START:
			nvgpu_pmu_dbg(g,
				"reply NV_PMU_RPC_ID_PERFMON_START");
			break;
		case NV_PMU_RPC_ID_PERFMON_T18X_STOP:
			nvgpu_pmu_dbg(g,
				"reply NV_PMU_RPC_ID_PERFMON_STOP");
			break;
		case NV_PMU_RPC_ID_PERFMON_T18X_QUERY:
			nvgpu_pmu_dbg(g,
				"reply NV_PMU_RPC_ID_PERFMON_QUERY");
			rpc_param = (struct nv_pmu_rpc_struct_perfmon_query *)
				rpc_payload->rpc_buff;
			pmu->load = rpc_param->sample_buffer[0];
			pmu->perfmon_query = 1;
			/* set perfmon_query to 1 after load is copied */
			break;
		}
		break;
	case PMU_UNIT_VOLT:
		switch (rpc.function) {
		case NV_PMU_RPC_ID_VOLT_BOARD_OBJ_GRP_CMD:
			nvgpu_pmu_dbg(g,
				"reply NV_PMU_RPC_ID_VOLT_BOARD_OBJ_GRP_CMD");
			break;
		case NV_PMU_RPC_ID_VOLT_VOLT_SET_VOLTAGE:
			nvgpu_pmu_dbg(g,
				"reply NV_PMU_RPC_ID_VOLT_VOLT_SET_VOLTAGE");
			break;
		case NV_PMU_RPC_ID_VOLT_VOLT_RAIL_GET_VOLTAGE:
			nvgpu_pmu_dbg(g,
				"reply NV_PMU_RPC_ID_VOLT_VOLT_RAIL_GET_VOLTAGE");
			break;
		case NV_PMU_RPC_ID_VOLT_LOAD:
			nvgpu_pmu_dbg(g,
				"reply NV_PMU_RPC_ID_VOLT_LOAD");
		}
		break;
	case PMU_UNIT_CLK:
		nvgpu_pmu_dbg(g, "reply PMU_UNIT_CLK");
		break;
	case PMU_UNIT_PERF:
		nvgpu_pmu_dbg(g, "reply PMU_UNIT_PERF");
		break;
	case PMU_UNIT_THERM:
			switch (rpc.function) {
			case NV_PMU_RPC_ID_THERM_BOARD_OBJ_GRP_CMD:
				nvgpu_pmu_dbg(g,
					"reply NV_PMU_RPC_ID_THERM_BOARD_OBJ_GRP_CMD");
				break;
			default:
				nvgpu_pmu_dbg(g, "reply PMU_UNIT_THERM");
				break;
			}
			break;
		/* TBD case will be added */
	default:
		nvgpu_err(g, " Invalid RPC response, stats 0x%x",
			rpc.flcn_status);
		break;
	}

exit:
	rpc_payload->complete = true;

	/* free allocated memory */
	if (rpc_payload->is_mem_free_set) {
		nvgpu_kfree(g, rpc_payload);
	}
}

int nvgpu_pmu_rpc_execute(struct nvgpu_pmu *pmu, struct nv_pmu_rpc_header *rpc,
	u16 size_rpc, u16 size_scratch, pmu_callback caller_cb,
	void *caller_cb_param, bool is_copy_back)
{
	struct gk20a *g = pmu->g;
	struct pmu_cmd cmd;
	struct pmu_payload payload;
	struct rpc_handler_payload *rpc_payload = NULL;
	pmu_callback callback = NULL;
	void *rpc_buff = NULL;
	int status = 0;

	if (!pmu->pmu_ready) {
		nvgpu_warn(g, "PMU is not ready to process RPC");
		status = EINVAL;
		goto exit;
	}

	if (caller_cb == NULL) {
		rpc_payload = nvgpu_kzalloc(g,
			sizeof(struct rpc_handler_payload) + size_rpc);
		if (rpc_payload == NULL) {
			status = ENOMEM;
			goto exit;
		}

		rpc_payload->rpc_buff = (u8 *)rpc_payload +
			sizeof(struct rpc_handler_payload);
		rpc_payload->is_mem_free_set =
			is_copy_back ? false : true;

		/* assign default RPC handler*/
		callback = pmu_rpc_handler;
	} else {
		if (caller_cb_param == NULL) {
			nvgpu_err(g, "Invalid cb param addr");
			status = EINVAL;
			goto exit;
		}
		rpc_payload = nvgpu_kzalloc(g,
			sizeof(struct rpc_handler_payload));
		if (rpc_payload == NULL) {
			status = ENOMEM;
			goto exit;
		}
		rpc_payload->rpc_buff = caller_cb_param;
		rpc_payload->is_mem_free_set = true;
		callback = caller_cb;
		WARN_ON(is_copy_back);
	}

	rpc_buff = rpc_payload->rpc_buff;
	(void) memset(&cmd, 0, sizeof(struct pmu_cmd));
	(void) memset(&payload, 0, sizeof(struct pmu_payload));

	cmd.hdr.unit_id = rpc->unit_id;
	cmd.hdr.size = (u8)(PMU_CMD_HDR_SIZE + sizeof(struct nv_pmu_rpc_cmd));
	cmd.cmd.rpc.cmd_type = NV_PMU_RPC_CMD_ID;
	cmd.cmd.rpc.flags = rpc->flags;

	nvgpu_memcpy((u8 *)rpc_buff, (u8 *)rpc, size_rpc);
	payload.rpc.prpc = rpc_buff;
	payload.rpc.size_rpc = size_rpc;
	payload.rpc.size_scratch = size_scratch;

	status = nvgpu_pmu_cmd_post(g, &cmd, &payload,
			PMU_COMMAND_QUEUE_LPQ, callback,
			rpc_payload);
	if (status != 0) {
		nvgpu_err(g, "Failed to execute RPC status=0x%x, func=0x%x",
				status, rpc->function);
		goto exit;
	}

	/*
	 * Option act like blocking call, which waits till RPC request
	 * executes on PMU & copy back processed data to rpc_buff
	 * to read data back in nvgpu
	 */
	if (is_copy_back) {
		/* wait till RPC execute in PMU & ACK */
		pmu_wait_message_cond(pmu, nvgpu_get_poll_timeout(g),
			&rpc_payload->complete, 1);
		/* copy back data to caller */
		nvgpu_memcpy((u8 *)rpc, (u8 *)rpc_buff, size_rpc);
		/* free allocated memory */
		nvgpu_kfree(g, rpc_payload);
	}

exit:
	if (status != 0) {
		if (rpc_payload != NULL) {
			nvgpu_kfree(g, rpc_payload);
		}
	}

	return status;
}
