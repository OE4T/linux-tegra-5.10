/*
 * Copyright (c) 2016-2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/kmem.h>
#include <nvgpu/dma.h>
#include <nvgpu/enabled.h>
#include <nvgpu/bug.h>
#include <nvgpu/thread.h>
#include <nvgpu/barrier.h>
#include <nvgpu/mm.h>
#include <nvgpu/enabled.h>
#include <nvgpu/ctxsw_trace.h>
#include <nvgpu/io.h>
#include <nvgpu/utils.h>
#include <nvgpu/timers.h>
#include <nvgpu/channel.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/gr/global_ctx.h>
#include <nvgpu/gr/subctx.h>
#include <nvgpu/gr/ctx.h>
#include <nvgpu/gr/fecs_trace.h>

#include "fecs_trace_gk20a.h"
#include "gr_gk20a.h"

#include <nvgpu/log.h>

#ifdef CONFIG_GK20A_CTXSW_TRACE
static u32 gk20a_fecs_trace_fecs_context_ptr(struct gk20a *g, struct channel_gk20a *ch)
{
	return (u32) (nvgpu_inst_block_addr(g, &ch->inst_block) >> 12LL);
}

int gk20a_fecs_trace_bind_channel(struct gk20a *g,
		struct channel_gk20a *ch, u32 vmid, struct nvgpu_gr_ctx *gr_ctx)
{
	/*
	 * map our circ_buf to the context space and store the GPU VA
	 * in the context header.
	 */

	u64 addr;
	struct nvgpu_gr_fecs_trace *trace = g->fecs_trace;
	struct nvgpu_mem *mem;
	u32 context_ptr = gk20a_fecs_trace_fecs_context_ptr(g, ch);
	u32 aperture_mask;
	struct tsg_gk20a *tsg;
	int ret;

	tsg = tsg_gk20a_from_ch(ch);
	if (tsg == NULL) {
		nvgpu_err(g, "chid: %d is not bound to tsg", ch->chid);
		return -EINVAL;
	}

	nvgpu_log(g, gpu_dbg_fn|gpu_dbg_ctxsw,
			"chid=%d context_ptr=%x inst_block=%llx",
			ch->chid, context_ptr,
			nvgpu_inst_block_addr(g, &ch->inst_block));

	if (!trace)
		return -ENOMEM;

	mem = nvgpu_gr_global_ctx_buffer_get_mem(g->gr.global_ctx_buffer,
					NVGPU_GR_GLOBAL_CTX_FECS_TRACE_BUFFER);
	if (mem == NULL) {
		return -EINVAL;
	}

	if (nvgpu_is_enabled(g, NVGPU_FECS_TRACE_VA)) {
		addr = nvgpu_gr_ctx_get_global_ctx_va(gr_ctx,
				NVGPU_GR_CTX_FECS_TRACE_BUFFER_VA);
		nvgpu_log(g, gpu_dbg_ctxsw, "gpu_va=%llx", addr);
		aperture_mask = 0;
	} else {
		addr = nvgpu_inst_block_addr(g, mem);
		nvgpu_log(g, gpu_dbg_ctxsw, "pa=%llx", addr);
		aperture_mask =
		       g->ops.gr.ctxsw_prog.get_ts_buffer_aperture_mask(g, mem);
	}
	if (!addr)
		return -ENOMEM;

	mem = &gr_ctx->mem;

	nvgpu_log(g, gpu_dbg_ctxsw, "addr=%llx count=%d", addr,
		GK20A_FECS_TRACE_NUM_RECORDS);

	g->ops.gr.ctxsw_prog.set_ts_num_records(g, mem,
		GK20A_FECS_TRACE_NUM_RECORDS);

	if (nvgpu_is_enabled(g, NVGPU_FECS_TRACE_VA) && ch->subctx != NULL)
		mem = &ch->subctx->ctx_header;

	g->ops.gr.ctxsw_prog.set_ts_buffer_ptr(g, mem, addr, aperture_mask);

	/* pid (process identifier) in user space, corresponds to tgid (thread
	 * group id) in kernel space.
	 */
	ret = nvgpu_gr_fecs_trace_add_context(g, context_ptr, tsg->tgid, 0,
		&trace->context_list);

	return ret;
}

int gk20a_fecs_trace_unbind_channel(struct gk20a *g, struct channel_gk20a *ch)
{
	u32 context_ptr = gk20a_fecs_trace_fecs_context_ptr(g, ch);
	struct nvgpu_gr_fecs_trace *trace = g->fecs_trace;

	if (trace) {
		nvgpu_log(g, gpu_dbg_fn|gpu_dbg_ctxsw,
			"ch=%p context_ptr=%x", ch, context_ptr);

		if (g->ops.fecs_trace.is_enabled(g)) {
			if (g->ops.fecs_trace.flush)
				g->ops.fecs_trace.flush(g);
			nvgpu_gr_fecs_trace_poll(g);
		}

		nvgpu_gr_fecs_trace_remove_context(g, context_ptr,
			&trace->context_list);
	}
	return 0;
}

#endif /* CONFIG_GK20A_CTXSW_TRACE */
