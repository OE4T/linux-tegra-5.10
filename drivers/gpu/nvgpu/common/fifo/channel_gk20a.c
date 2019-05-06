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

#include <nvgpu/channel.h>
#include <nvgpu/log.h>
#include <nvgpu/atomic.h>
#include <nvgpu/io.h>
#include <nvgpu/barrier.h>
#include <nvgpu/bug.h>
#include <nvgpu/gk20a.h>

#include "hal/fifo/pbdma_gm20b.h"

#include "channel_gk20a.h"

#include <nvgpu/hw/gk20a/hw_ccsr_gk20a.h>

void gk20a_channel_enable(struct nvgpu_channel *ch)
{
	gk20a_writel(ch->g, ccsr_channel_r(ch->chid),
		gk20a_readl(ch->g, ccsr_channel_r(ch->chid)) |
		ccsr_channel_enable_set_true_f());
}

void gk20a_channel_disable(struct nvgpu_channel *ch)
{
	gk20a_writel(ch->g, ccsr_channel_r(ch->chid),
		gk20a_readl(ch->g,
			ccsr_channel_r(ch->chid)) |
			ccsr_channel_enable_clr_true_f());
}

void gk20a_channel_unbind(struct nvgpu_channel *ch)
{
	struct gk20a *g = ch->g;

	nvgpu_log_fn(g, " ");

	if (nvgpu_atomic_cmpxchg(&ch->bound, (int)true, (int)false) != 0) {
		gk20a_writel(g, ccsr_channel_inst_r(ch->chid),
			ccsr_channel_inst_ptr_f(0) |
			ccsr_channel_inst_bind_false_f());
	}
}

/* ccsr_channel_status_v is four bits long */
static const char * const ccsr_chan_status_str[] = {
	"idle",
	"pending",
	"pending_ctx_reload",
	"pending_acquire",
	"pending_acq_ctx_reload",
	"on_pbdma",
	"on_pbdma_and_eng",
	"on_eng",
	"on_eng_pending_acquire",
	"on_eng_pending",
	"on_pbdma_ctx_reload",
	"on_pbdma_and_eng_ctx_reload",
	"on_eng_ctx_reload",
	"on_eng_pending_ctx_reload",
	"on_eng_pending_acq_ctx_reload",
	"N/A",
};

void gk20a_channel_read_state(struct gk20a *g, struct nvgpu_channel *ch,
		struct nvgpu_channel_hw_state *state)
{
	u32 reg = gk20a_readl(g, ccsr_channel_r(ch->chid));
	u32 status_v = ccsr_channel_status_v(reg);

	state->next = ccsr_channel_next_v(reg) == ccsr_channel_next_true_v();
	state->enabled = ccsr_channel_enable_v(reg) ==
			    ccsr_channel_enable_in_use_v();
	state->ctx_reload =
		status_v == ccsr_channel_status_pending_ctx_reload_v() ||
		status_v == ccsr_channel_status_pending_acq_ctx_reload_v() ||
		status_v == ccsr_channel_status_on_pbdma_ctx_reload_v() ||
		status_v == ccsr_channel_status_on_pbdma_and_eng_ctx_reload_v() ||
		status_v == ccsr_channel_status_on_eng_ctx_reload_v() ||
		status_v == ccsr_channel_status_on_eng_pending_ctx_reload_v() ||
		status_v == ccsr_channel_status_on_eng_pending_acq_ctx_reload_v();
	state->busy = ccsr_channel_busy_v(reg) == ccsr_channel_busy_true_v();
	state->pending_acquire =
		status_v == ccsr_channel_status_pending_acquire_v() ||
		status_v == ccsr_channel_status_on_eng_pending_acquire_v();
	state->status_string = ccsr_chan_status_str[status_v];
}

void gk20a_channel_debug_dump(struct gk20a *g,
				struct gk20a_debug_output *o,
				struct nvgpu_channel_dump_info *info)
{
	gk20a_debug_output(o, "Channel ID: %d, TSG ID: %u, pid %d, refs %d; "
				"deterministic = %s",
			   info->chid,
			   info->tsgid,
			   info->pid,
			   info->refs,
			   info->deterministic ? "yes" : "no");
	gk20a_debug_output(o, "  In use: %-3s  busy: %-3s  status: %s",
			   info->hw_state.enabled ? "yes" : "no",
			   info->hw_state.busy ? "yes" : "no",
			   info->hw_state.status_string);
	gk20a_debug_output(o,
			   "  TOP       %016llx"
			   "  PUT       %016llx  GET %016llx",
			   info->inst.pb_top_level_get,
			   info->inst.pb_put,
			   info->inst.pb_get);
	gk20a_debug_output(o,
			   "  FETCH     %016llx"
			   "  HEADER    %08x          COUNT %08x",
			   info->inst.pb_fetch,
			   info->inst.pb_header,
			   info->inst.pb_count);
	gk20a_debug_output(o,
			   "  SYNCPOINT %08x %08x "
			   "SEMAPHORE %08x %08x %08x %08x",
			   info->inst.syncpointa,
			   info->inst.syncpointb,
			   info->inst.semaphorea,
			   info->inst.semaphoreb,
			   info->inst.semaphorec,
			   info->inst.semaphored);

	if (info->sema.addr == 0ULL) {
		gk20a_debug_output(o,
			"  SEMA STATE: val: %u next_val: %u addr: 0x%010llx",
			info->sema.value,
			info->sema.next,
			info->sema.addr);
	}
	gm20b_pbdma_syncpoint_debug_dump(g, o, info);

	gk20a_debug_output(o, " ");
}
