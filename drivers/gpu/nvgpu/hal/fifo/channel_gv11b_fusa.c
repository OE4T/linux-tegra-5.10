/*
 * Copyright (c) 2016-2020, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/gk20a.h>

#include "channel_gk20a.h"
#include "channel_gv11b.h"

#include <nvgpu/hw/gv11b/hw_ccsr_gv11b.h>

void gv11b_channel_unbind(struct nvgpu_channel *ch)
{
	struct gk20a *g = ch->g;

	nvgpu_log_fn(g, " ");

	if (nvgpu_atomic_cmpxchg(&ch->bound, 1, 0) != 0) {
		nvgpu_writel(g, ccsr_channel_inst_r(ch->chid),
			ccsr_channel_inst_ptr_f(0U) |
			ccsr_channel_inst_bind_false_f());

		nvgpu_writel(g, ccsr_channel_r(ch->chid),
			ccsr_channel_enable_clr_true_f() |
			ccsr_channel_pbdma_faulted_reset_f() |
			ccsr_channel_eng_faulted_reset_f());
	}
}

u32 gv11b_channel_count(struct gk20a *g)
{
	return ccsr_channel__size_1_v();
}

void gv11b_channel_read_state(struct gk20a *g, struct nvgpu_channel *ch,
		struct nvgpu_channel_hw_state *state)
{
	u32 reg = nvgpu_readl(g, ccsr_channel_r(ch->chid));

	gk20a_channel_read_state(g, ch, state);

	state->eng_faulted = ccsr_channel_eng_faulted_v(reg) ==
		ccsr_channel_eng_faulted_true_v();
}

void gv11b_channel_reset_faulted(struct gk20a *g, struct nvgpu_channel *ch,
		bool eng, bool pbdma)
{
	u32 reg = nvgpu_readl(g, ccsr_channel_r(ch->chid));

	if (eng) {
		reg |= ccsr_channel_eng_faulted_reset_f();
	}
	if (pbdma) {
		reg |= ccsr_channel_pbdma_faulted_reset_f();
	}

	nvgpu_writel(g, ccsr_channel_r(ch->chid), reg);
}

void gv11b_channel_debug_dump(struct gk20a *g,
			     struct nvgpu_debug_context *o,
			     struct nvgpu_channel_dump_info *info)
{
#ifdef CONFIG_NVGPU_IOCTL_NON_FUSA
	gk20a_debug_output(o, "%d-%s, TSG: %u, pid %d, refs: %d%s: ",
#else
	gk20a_debug_output(o, "%d-%s, TSG: %u, pid %d, refs: %d: ",
#endif
			info->chid,
			g->name,
			info->tsgid,
			info->pid,
#ifdef CONFIG_NVGPU_IOCTL_NON_FUSA
			info->refs,
			info->deterministic ? ", deterministic" : "");
#else
			info->refs);
#endif
	gk20a_debug_output(o, "channel status: %s in use %s %s\n",
			info->hw_state.enabled ? "" : "not",
			info->hw_state.status_string,
			info->hw_state.busy ? "busy" : "not busy");
	gk20a_debug_output(o,
			"RAMFC : TOP: %016llx PUT: %016llx GET: %016llx "
			"FETCH: %016llx\n"
			"HEADER: %08x COUNT: %08x\n"
			"SEMAPHORE: addr %016llx\n"
			"payload %016llx execute %08x\n",
			info->inst.pb_top_level_get,
			info->inst.pb_put,
			info->inst.pb_get,
			info->inst.pb_fetch,
			info->inst.pb_header,
			info->inst.pb_count,
			info->inst.sem_addr,
			info->inst.sem_payload,
			info->inst.sem_execute);

	if (info->sema.addr != 0ULL) {
		gk20a_debug_output(o, "SEMA STATE: value: 0x%08x "
				   "next_val: 0x%08x addr: 0x%010llx\n",
				  info->sema.value,
				  info->sema.next,
				  info->sema.addr);
	}

	gk20a_debug_output(o, "\n");
}
