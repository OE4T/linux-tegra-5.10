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
#include <nvgpu/circ_buf.h>
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

/*
 * Converts HW entry format to userspace-facing format and pushes it to the
 * queue.
 */
static int gk20a_fecs_trace_ring_read(struct gk20a *g, int index)
{
	int i;
	struct nvgpu_gpu_ctxsw_trace_entry entry = { };
	struct nvgpu_gr_fecs_trace *trace = g->fecs_trace;
	pid_t cur_pid;
	pid_t new_pid;
	u32 cur_vmid, new_vmid;
	int count = 0;

	/* for now, only one VM */
	const int vmid = 0;

	struct nvgpu_fecs_trace_record *r =
		nvgpu_gr_fecs_trace_get_record(g, index);
	if (r == NULL) {
		return -EINVAL;
	}

	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_ctxsw,
		"consuming record trace=%p read=%d record=%p", trace, index, r);

	if (unlikely(!nvgpu_gr_fecs_trace_is_valid_record(g, r))) {
		nvgpu_warn(g,
			"trace=%p read=%d record=%p magic_lo=%08x magic_hi=%08x (invalid)",
			trace, index, r, r->magic_lo, r->magic_hi);
		return -EINVAL;
	}

	/* Clear magic_hi to detect cases where CPU could read write index
	 * before FECS record is actually written to DRAM. This should not
	 * as we force FECS writes to SYSMEM by reading through PRAMIN.
	 */
	r->magic_hi = 0;

	nvgpu_gr_fecs_trace_find_pid(g, r->context_ptr, &trace->context_list,
		&cur_pid, &cur_vmid);
	nvgpu_gr_fecs_trace_find_pid(g, r->new_context_ptr, &trace->context_list,
		&new_pid, &new_vmid);

	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_ctxsw,
		"context_ptr=%x (pid=%d) new_context_ptr=%x (pid=%d)",
		r->context_ptr, cur_pid, r->new_context_ptr, new_pid);

	entry.context_id = r->context_id;
	entry.vmid = vmid;

	/* break out FECS record into trace events */
	for (i = 0; i < nvgpu_gr_fecs_trace_num_ts(g); i++) {

		entry.tag = g->ops.gr.ctxsw_prog.hw_get_ts_tag(r->ts[i]);
		entry.timestamp =
			g->ops.gr.ctxsw_prog.hw_record_ts_timestamp(r->ts[i]);
		entry.timestamp <<= GK20A_FECS_TRACE_PTIMER_SHIFT;

		nvgpu_log(g, gpu_dbg_ctxsw,
			"tag=%x timestamp=%llx context_id=%08x new_context_id=%08x",
			entry.tag, entry.timestamp, r->context_id,
			r->new_context_id);

		switch (nvgpu_gpu_ctxsw_tags_to_common_tags(entry.tag)) {
		case NVGPU_GPU_CTXSW_TAG_RESTORE_START:
		case NVGPU_GPU_CTXSW_TAG_CONTEXT_START:
			entry.context_id = r->new_context_id;
			entry.pid = new_pid;
			break;

		case NVGPU_GPU_CTXSW_TAG_CTXSW_REQ_BY_HOST:
		case NVGPU_GPU_CTXSW_TAG_FE_ACK:
		case NVGPU_GPU_CTXSW_TAG_FE_ACK_WFI:
		case NVGPU_GPU_CTXSW_TAG_FE_ACK_GFXP:
		case NVGPU_GPU_CTXSW_TAG_FE_ACK_CTAP:
		case NVGPU_GPU_CTXSW_TAG_FE_ACK_CILP:
		case NVGPU_GPU_CTXSW_TAG_SAVE_END:
			entry.context_id = r->context_id;
			entry.pid = cur_pid;
			break;

		default:
			/* tags are not guaranteed to start at the beginning */
			WARN_ON(entry.tag && (entry.tag != NVGPU_GPU_CTXSW_TAG_INVALID_TIMESTAMP));
			continue;
		}

		nvgpu_log(g, gpu_dbg_ctxsw, "tag=%x context_id=%x pid=%lld",
			entry.tag, entry.context_id, entry.pid);

		if (!entry.context_id)
			continue;

		gk20a_ctxsw_trace_write(g, &entry);
		count++;
	}

	gk20a_ctxsw_trace_wake_up(g, vmid);
	return count;
}

int gk20a_fecs_trace_poll(struct gk20a *g)
{
	struct nvgpu_gr_fecs_trace *trace = g->fecs_trace;

	int read = 0;
	int write = 0;
	int cnt;
	int err;

	err = gk20a_busy(g);
	if (unlikely(err))
		return err;

	nvgpu_mutex_acquire(&trace->poll_lock);
	write = g->ops.fecs_trace.get_write_index(g);
	if (unlikely((write < 0) || (write >= GK20A_FECS_TRACE_NUM_RECORDS))) {
		nvgpu_err(g,
			"failed to acquire write index, write=%d", write);
		err = write;
		goto done;
	}

	read = g->ops.fecs_trace.get_read_index(g);

	cnt = CIRC_CNT(write, read, GK20A_FECS_TRACE_NUM_RECORDS);
	if (!cnt)
		goto done;

	nvgpu_log(g, gpu_dbg_ctxsw,
		"circular buffer: read=%d (mailbox=%d) write=%d cnt=%d",
		read, g->ops.fecs_trace.get_read_index(g), write, cnt);

	/* Ensure all FECS writes have made it to SYSMEM */
	g->ops.mm.fb_flush(g);

	while (read != write) {
		cnt = gk20a_fecs_trace_ring_read(g, read);
		if (cnt > 0) {
			nvgpu_log(g, gpu_dbg_ctxsw,
				"number of trace entries added: %d", cnt);
		}

		/* Get to next record. */
		read = (read + 1) & (GK20A_FECS_TRACE_NUM_RECORDS - 1);
	}

	/* ensure FECS records has been updated before incrementing read index */
	nvgpu_wmb();
	g->ops.fecs_trace.set_read_index(g, read);

	/*
	 * FECS ucode does a priv holdoff around the assertion of context
	 * reset. So, pri transactions (e.g. mailbox1 register write) might
	 * fail due to this. Hence, do write with ack i.e. write and read
	 * it back to make sure write happened for mailbox1.
	 */
	while (g->ops.fecs_trace.get_read_index(g) != read) {
		nvgpu_log(g, gpu_dbg_ctxsw, "mailbox1 update failed");
		g->ops.fecs_trace.set_read_index(g, read);
	}

done:
	nvgpu_mutex_release(&trace->poll_lock);
	gk20a_idle(g);
	return err;
}

int gk20a_fecs_trace_periodic_polling(void *arg)
{
	struct gk20a *g = (struct gk20a *)arg;
	struct nvgpu_gr_fecs_trace *trace = g->fecs_trace;

	pr_info("%s: running\n", __func__);

	while (!nvgpu_thread_should_stop(&trace->poll_task)) {

		nvgpu_usleep_range(GK20A_FECS_TRACE_FRAME_PERIOD_US,
				   GK20A_FECS_TRACE_FRAME_PERIOD_US * 2);

		gk20a_fecs_trace_poll(g);
	}

	return 0;
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
			gk20a_fecs_trace_poll(g);
		}

		nvgpu_gr_fecs_trace_remove_context(g, context_ptr,
			&trace->context_list);
	}
	return 0;
}

int gk20a_fecs_trace_reset(struct gk20a *g)
{
	nvgpu_log(g, gpu_dbg_fn|gpu_dbg_ctxsw, " ");

	if (!g->ops.fecs_trace.is_enabled(g))
		return 0;

	gk20a_fecs_trace_poll(g);
	return g->ops.fecs_trace.set_read_index(g, 0);
}

u32 gk20a_fecs_trace_get_buffer_full_mailbox_val(void)
{
	return 0x26;
}
#endif /* CONFIG_GK20A_CTXSW_TRACE */
