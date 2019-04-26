/*
 * Copyright (c) 2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/io.h>
#include <nvgpu/debug.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/engine_status.h>
#include <nvgpu/fifo.h>

#include <nvgpu/hw/gm20b/hw_fifo_gm20b.h>

#include "engine_status_gm20b.h"

static void populate_invalid_ctxsw_status_info(
		struct nvgpu_engine_status_info *status_info)
{
	status_info->ctx_id = ENGINE_STATUS_CTX_ID_INVALID;
	status_info->ctx_id_type = ENGINE_STATUS_CTX_NEXT_ID_TYPE_INVALID;
	status_info->ctx_next_id =
		ENGINE_STATUS_CTX_NEXT_ID_INVALID;
	status_info->ctx_next_id_type = ENGINE_STATUS_CTX_NEXT_ID_TYPE_TSGID;
	status_info->ctxsw_status = NVGPU_CTX_STATUS_INVALID;
}

static void populate_valid_ctxsw_status_info(
		struct nvgpu_engine_status_info *status_info)
{
	bool id_type_tsg;
	u32 engine_status = status_info->reg_data;

	status_info->ctx_id =
		fifo_engine_status_id_v(status_info->reg_data);
	id_type_tsg = fifo_engine_status_id_type_v(engine_status) ==
			fifo_engine_status_id_type_tsgid_v();
	status_info->ctx_id_type =
		id_type_tsg ? ENGINE_STATUS_CTX_ID_TYPE_TSGID :
			ENGINE_STATUS_CTX_ID_TYPE_CHID;
	status_info->ctx_next_id =
		ENGINE_STATUS_CTX_NEXT_ID_INVALID;
	status_info->ctx_next_id_type = ENGINE_STATUS_CTX_NEXT_ID_TYPE_INVALID;
	status_info->ctxsw_status = NVGPU_CTX_STATUS_VALID;
}

static void populate_load_ctxsw_status_info(
		struct nvgpu_engine_status_info *status_info)
{
	bool next_id_type_tsg;
	u32 engine_status = status_info->reg_data;

	status_info->ctx_id = ENGINE_STATUS_CTX_ID_INVALID;
	status_info->ctx_id_type = ENGINE_STATUS_CTX_ID_TYPE_INVALID;
	status_info->ctx_next_id =
		fifo_engine_status_next_id_v(
			status_info->reg_data);
	next_id_type_tsg = fifo_engine_status_next_id_type_v(engine_status) ==
			fifo_engine_status_next_id_type_tsgid_v();
	status_info->ctx_next_id_type =
		next_id_type_tsg ? ENGINE_STATUS_CTX_NEXT_ID_TYPE_TSGID :
			ENGINE_STATUS_CTX_NEXT_ID_TYPE_CHID;
	status_info->ctxsw_status = NVGPU_CTX_STATUS_CTXSW_LOAD;
}

static void populate_save_ctxsw_status_info(
		struct nvgpu_engine_status_info *status_info)
{
	bool id_type_tsg;
	u32 engine_status = status_info->reg_data;

	status_info->ctx_id =
		fifo_engine_status_id_v(status_info->reg_data);
	id_type_tsg = fifo_engine_status_id_type_v(engine_status) ==
			fifo_engine_status_id_type_tsgid_v();
	status_info->ctx_id_type =
		id_type_tsg ? ENGINE_STATUS_CTX_ID_TYPE_TSGID :
			ENGINE_STATUS_CTX_ID_TYPE_CHID;
	status_info->ctx_next_id =
		ENGINE_STATUS_CTX_NEXT_ID_INVALID;
	status_info->ctx_next_id_type = ENGINE_STATUS_CTX_NEXT_ID_TYPE_INVALID;
	status_info->ctxsw_status = NVGPU_CTX_STATUS_CTXSW_SAVE;
}

static void populate_switch_ctxsw_status_info(
		struct nvgpu_engine_status_info *status_info)
{
	bool id_type_tsg;
	bool next_id_type_tsg;
	u32 engine_status = status_info->reg_data;

	status_info->ctx_id =
		fifo_engine_status_id_v(status_info->reg_data);
	id_type_tsg = fifo_engine_status_id_type_v(engine_status) ==
			fifo_engine_status_id_type_tsgid_v();
	status_info->ctx_id_type =
		id_type_tsg ? ENGINE_STATUS_CTX_ID_TYPE_TSGID :
			ENGINE_STATUS_CTX_ID_TYPE_CHID;
	status_info->ctx_next_id =
		fifo_engine_status_next_id_v(
			status_info->reg_data);
	next_id_type_tsg = fifo_engine_status_next_id_type_v(engine_status) ==
			fifo_engine_status_next_id_type_tsgid_v();
	status_info->ctx_next_id_type =
		next_id_type_tsg ? ENGINE_STATUS_CTX_NEXT_ID_TYPE_TSGID :
			ENGINE_STATUS_CTX_NEXT_ID_TYPE_CHID;
	status_info->ctxsw_status = NVGPU_CTX_STATUS_CTXSW_SWITCH;
}

void gm20b_read_engine_status_info(struct gk20a *g, u32 engine_id,
		struct nvgpu_engine_status_info *status)
{
	u32 engine_reg_data;
	u32 ctxsw_state;

	(void) memset(status, 0, sizeof(*status));

	engine_reg_data = nvgpu_readl(g, fifo_engine_status_r(engine_id));

	status->reg_data = engine_reg_data;

	/* populate the engine_state enum */
	status->is_busy = fifo_engine_status_engine_v(engine_reg_data) ==
			fifo_engine_status_engine_busy_v();

	/* populate the engine_faulted_state enum */
	status->is_faulted = fifo_engine_status_faulted_v(engine_reg_data) ==
			fifo_engine_status_faulted_true_v();

	/* populate the ctxsw_in_progress_state */
	status->ctxsw_in_progress = ((engine_reg_data &
			fifo_engine_status_ctxsw_in_progress_f()) != 0U);

	/* populate the ctxsw related info */
	ctxsw_state = fifo_engine_status_ctx_status_v(engine_reg_data);

	status->ctxsw_state = ctxsw_state;

	if (ctxsw_state == fifo_engine_status_ctx_status_valid_v()) {
		populate_valid_ctxsw_status_info(status);
	} else if (ctxsw_state ==
			fifo_engine_status_ctx_status_ctxsw_load_v()) {
		populate_load_ctxsw_status_info(status);
	} else if (ctxsw_state ==
			fifo_engine_status_ctx_status_ctxsw_save_v()) {
		populate_save_ctxsw_status_info(status);
	} else if (ctxsw_state ==
			fifo_engine_status_ctx_status_ctxsw_switch_v()) {
		populate_switch_ctxsw_status_info(status);
	} else {
		populate_invalid_ctxsw_status_info(status);
	}
}

void gm20b_dump_engine_status(struct gk20a *g, struct gk20a_debug_output *o)
{
	u32 i, host_num_engines;
	struct nvgpu_engine_status_info engine_status;

	host_num_engines = nvgpu_get_litter_value(g, GPU_LIT_HOST_NUM_ENGINES);

	gk20a_debug_output(o, "Engine status - chip %-5s", g->name);
	gk20a_debug_output(o, "--------------------------");

	for (i = 0; i < host_num_engines; i++) {
		g->ops.engine_status.read_engine_status_info(g, i, &engine_status);

		gk20a_debug_output(o,
			"Engine %d | "
			"ID: %d - %-9s next_id: %d %-9s | status: %s",
			i,
			engine_status.ctx_id,
			nvgpu_engine_status_is_ctx_type_tsg(
				&engine_status) ?
				"[tsg]" : "[channel]",
			engine_status.ctx_next_id,
			nvgpu_engine_status_is_next_ctx_type_tsg(
				&engine_status) ?
				"[tsg]" : "[channel]",
			nvgpu_fifo_decode_pbdma_ch_eng_status(
				engine_status.ctxsw_state));

		if (engine_status.is_faulted) {
			gk20a_debug_output(o, "  State: faulted");
		}
		if (engine_status.is_busy) {
			gk20a_debug_output(o, "  State: busy");
		}
	}
	gk20a_debug_output(o, "\n");
}
