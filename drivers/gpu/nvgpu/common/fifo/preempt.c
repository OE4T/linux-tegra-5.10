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

#include <nvgpu/gk20a.h>
#include <nvgpu/runlist.h>
#include <nvgpu/types.h>
#include <nvgpu/channel.h>
#include <nvgpu/tsg.h>
#include <nvgpu/preempt.h>


u32 nvgpu_preempt_get_timeout(struct gk20a *g)
{
	return g->ctxsw_timeout_period_ms;
}

int nvgpu_preempt_channel(struct gk20a *g, struct channel_gk20a *ch)
{
	int err;
	struct tsg_gk20a *tsg = tsg_gk20a_from_ch(ch);

	if (tsg != NULL) {
		err = g->ops.fifo.preempt_tsg(ch->g, tsg);
	} else {
		err = g->ops.fifo.preempt_channel(ch->g, ch);
	}

	return err;
}

/* called from rc */
void nvgpu_preempt_poll_tsg_on_pbdma(struct gk20a *g,
		struct tsg_gk20a *tsg)
{
	struct nvgpu_fifo *f = &g->fifo;
	u32 runlist_id;
	unsigned long runlist_served_pbdmas;
	unsigned long pbdma_id_bit;
	u32 tsgid, pbdma_id;

	if (g->ops.fifo.preempt_poll_pbdma == NULL) {
		return;
	}

	if (tsg == NULL) {
		return;
	}

	tsgid = tsg->tsgid;
	runlist_id = tsg->runlist_id;
	runlist_served_pbdmas = f->runlist_info[runlist_id]->pbdma_bitmask;

	for_each_set_bit(pbdma_id_bit, &runlist_served_pbdmas, f->num_pbdma) {
		pbdma_id = U32(pbdma_id_bit);
		/*
		 * If pbdma preempt fails the only option is to reset
		 * GPU. Any sort of hang indicates the entire GPU’s
		 * memory system would be blocked.
		 */
		if (g->ops.fifo.preempt_poll_pbdma(g, tsgid,
				pbdma_id) != 0) {
			nvgpu_report_host_error(g, 0,
					GPU_HOST_PBDMA_PREEMPT_ERROR,
					pbdma_id);
			nvgpu_err(g, "PBDMA preempt failed");
		}
	}
}
