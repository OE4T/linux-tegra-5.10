/*
 * Copyright (c) 2011-2019, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/gk20a.h>
#include <nvgpu/fifo.h>
#include <nvgpu/engines.h>
#include <nvgpu/debug.h>
#include <nvgpu/channel.h>
#include <nvgpu/tsg.h>
#include <nvgpu/error_notifier.h>
#include <nvgpu/nvgpu_err.h>
#include <nvgpu/pbdma_status.h>
#include <nvgpu/debug.h>
#include <nvgpu/rc.h>

void nvgpu_rc_fifo_recover(struct gk20a *g, u32 eng_bitmask,
			u32 hw_id, bool id_is_tsg,
			bool id_is_known, bool debug_dump, u32 rc_type)
{
	unsigned int id_type;

	if (debug_dump) {
		gk20a_debug_dump(g);
	}

	if (g->ops.ltc.flush != NULL) {
		g->ops.ltc.flush(g);
	}

	if (id_is_known) {
		id_type = id_is_tsg ? ID_TYPE_TSG : ID_TYPE_CHANNEL;
	} else {
		id_type = ID_TYPE_UNKNOWN;
	}

	g->ops.fifo.recover(g, eng_bitmask, hw_id, id_type,
				 rc_type, NULL);
}

void nvgpu_rc_ctxsw_timeout(struct gk20a *g, u32 eng_bitmask,
				struct tsg_gk20a *tsg, bool debug_dump)
{
	nvgpu_tsg_set_error_notifier(g, tsg,
		NVGPU_ERR_NOTIFIER_FIFO_ERROR_IDLE_TIMEOUT);
	/*
	 * Cancel all channels' wdt since ctxsw timeout might
	 * trigger multiple watchdogs at a time
	 */
	nvgpu_channel_wdt_restart_all_channels(g);
	nvgpu_rc_fifo_recover(g, eng_bitmask, tsg->tsgid, true, true, debug_dump,
			RC_TYPE_CTXSW_TIMEOUT);
}

void nvgpu_rc_pbdma_fault(struct gk20a *g, struct nvgpu_fifo *f,
			u32 pbdma_id, u32 error_notifier)
{
	u32 id;
	struct nvgpu_pbdma_status_info pbdma_status;

	nvgpu_log(g, gpu_dbg_info, "pbdma id %d error notifier %d",
			pbdma_id, error_notifier);

	g->ops.pbdma_status.read_pbdma_status_info(g, pbdma_id,
		&pbdma_status);
	/* Remove channel from runlist */
	id = pbdma_status.id;
	if (pbdma_status.id_type == PBDMA_STATUS_ID_TYPE_TSGID) {
		struct tsg_gk20a *tsg = nvgpu_tsg_get_from_id(g, id);

		nvgpu_tsg_set_error_notifier(g, tsg, error_notifier);
		nvgpu_rc_tsg_and_related_engines(g, tsg, true,
			RC_TYPE_PBDMA_FAULT);
	} else if(pbdma_status.id_type == PBDMA_STATUS_ID_TYPE_CHID) {
		struct channel_gk20a *ch = gk20a_channel_from_id(g, id);
		struct tsg_gk20a *tsg;
		if (ch == NULL) {
			nvgpu_err(g, "channel is not referenceable");
			return;
		}

		tsg = tsg_gk20a_from_ch(ch);
		if (tsg != NULL) {
			nvgpu_tsg_set_error_notifier(g, tsg, error_notifier);
			nvgpu_rc_tsg_and_related_engines(g, tsg, true,
				RC_TYPE_PBDMA_FAULT);
		} else {
			nvgpu_err(g, "chid: %d is not bound to tsg", ch->chid);
		}

		gk20a_channel_put(ch);
	} else {
		nvgpu_err(g, "Invalid pbdma_status.id_type");
	}
}

void nvgpu_rc_runlist_update(struct gk20a *g, u32 runlist_id)
{
	u32 eng_bitmask = nvgpu_engine_get_runlist_busy_engines(g, runlist_id);

	if (eng_bitmask != 0U) {
		nvgpu_rc_fifo_recover(g, eng_bitmask, INVAL_ID, false, false, true,
				RC_TYPE_RUNLIST_UPDATE_TIMEOUT);
	}
}

void nvgpu_rc_preempt_timeout(struct gk20a *g, struct tsg_gk20a *tsg)
{
	nvgpu_tsg_set_error_notifier(g, tsg,
		NVGPU_ERR_NOTIFIER_FIFO_ERROR_IDLE_TIMEOUT);

	nvgpu_rc_tsg_and_related_engines(g, tsg, true, RC_TYPE_PREEMPT_TIMEOUT);
}

void nvgpu_rc_gr_fault(struct gk20a *g, struct tsg_gk20a *tsg,
		struct channel_gk20a *ch)
{
	u32 gr_engine_id;
	u32 gr_eng_bitmask = 0U;

	gr_engine_id = nvgpu_engine_get_gr_id(g);
	if (gr_engine_id != NVGPU_INVALID_ENG_ID) {
		gr_eng_bitmask = BIT32(gr_engine_id);
	} else {
		nvgpu_warn(g, "gr_engine_id is invalid");
	}

	if (tsg != NULL) {
		nvgpu_rc_fifo_recover(g, gr_eng_bitmask, tsg->tsgid,
				   true, true, true, RC_TYPE_GR_FAULT);
	} else {
		if (ch != NULL) {
			nvgpu_err(g, "chid: %d referenceable but not "
				"bound to tsg", ch->chid);
		}
		nvgpu_rc_fifo_recover(g, gr_eng_bitmask, INVAL_ID,
				   false, false, true, RC_TYPE_GR_FAULT);
	}
}

void nvgpu_rc_sched_error_bad_tsg(struct gk20a *g)
{
	/* id is unknown, preempt all runlists and do recovery */
	nvgpu_rc_fifo_recover(g, 0, INVAL_ID, false, false, false,
		RC_TYPE_SCHED_ERR);
}

void nvgpu_rc_tsg_and_related_engines(struct gk20a *g, struct tsg_gk20a *tsg,
			 bool debug_dump, u32 rc_type)
{
	u32 eng_bitmask = 0U;
	int err;

	nvgpu_mutex_acquire(&g->dbg_sessions_lock);

	/* disable tsg so that it does not get scheduled again */
	g->ops.tsg.disable(tsg);

	/*
	 * On hitting engine reset, h/w drops the ctxsw_status to INVALID in
	 * fifo_engine_status register. Also while the engine is held in reset
	 * h/w passes busy/idle straight through. fifo_engine_status registers
	 * are correct in that there is no context switch outstanding
	 * as the CTXSW is aborted when reset is asserted.
	 */
	nvgpu_log_info(g, "acquire engines_reset_mutex");
	nvgpu_mutex_acquire(&g->fifo.engines_reset_mutex);

	/*
	 * stop context switching to prevent engine assignments from
	 * changing until engine status is checked to make sure tsg
	 * being recovered is not loaded on the engines
	 */
	err = g->ops.gr.disable_ctxsw(g);

	if (err != 0) {
		/* if failed to disable ctxsw, just abort tsg */
		nvgpu_err(g, "failed to disable ctxsw");
	} else {
		/* recover engines if tsg is loaded on the engines */
		eng_bitmask = g->ops.engine.get_mask_on_id(g,
				tsg->tsgid, true);

		/*
		 * it is ok to enable ctxsw before tsg is recovered. If engines
		 * is 0, no engine recovery is needed and if it is  non zero,
		 * gk20a_fifo_recover will call get_mask_on_id again.
		 * By that time if tsg is not on the engine, engine need not
		 * be reset.
		 */
		err = g->ops.gr.enable_ctxsw(g);
		if (err != 0) {
			nvgpu_err(g, "failed to enable ctxsw");
		}
	}
	nvgpu_log_info(g, "release engines_reset_mutex");
	nvgpu_mutex_release(&g->fifo.engines_reset_mutex);

	if (eng_bitmask != 0U) {
		nvgpu_rc_fifo_recover(g, eng_bitmask, tsg->tsgid, true, true,
			debug_dump, rc_type);
	} else {
		if (nvgpu_tsg_mark_error(g, tsg) && debug_dump) {
			gk20a_debug_dump(g);
		}

		nvgpu_tsg_abort(g, tsg, false);
	}

	nvgpu_mutex_release(&g->dbg_sessions_lock);
}
