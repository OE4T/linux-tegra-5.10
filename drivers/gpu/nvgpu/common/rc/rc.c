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
#include <nvgpu/channel.h>
#include <nvgpu/tsg.h>
#include <nvgpu/error_notifier.h>
#include <nvgpu/nvgpu_err.h>
#include <nvgpu/pbdma_status.h>
#include <nvgpu/rc.h>

#include "gk20a/fifo_gk20a.h"

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
	gk20a_fifo_recover(g, eng_bitmask, tsg->tsgid, true, true, debug_dump,
			RC_TYPE_CTXSW_TIMEOUT);
}

void nvgpu_rc_pbdma_fault(struct gk20a *g, struct fifo_gk20a *f,
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
		struct tsg_gk20a *tsg = &f->tsg[id];

		nvgpu_tsg_set_error_notifier(g, tsg, error_notifier);
		nvgpu_tsg_recover(g, tsg, true, RC_TYPE_PBDMA_FAULT);
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
			nvgpu_tsg_recover(g, tsg, true, RC_TYPE_PBDMA_FAULT);
		} else {
			nvgpu_err(g, "chid: %d is not bound to tsg", ch->chid);
		}

		gk20a_channel_put(ch);
	} else {
		nvgpu_err(g, "Invalid pbdma_status.id_type");
	}
}