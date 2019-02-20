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

#include <nvgpu/gk20a.h>
#include <nvgpu/list.h>
#include <nvgpu/log.h>
#include <nvgpu/log2.h>
#include <nvgpu/enabled.h>
#include <nvgpu/gr/global_ctx.h>
#include <nvgpu/gr/fecs_trace.h>

int nvgpu_gr_fecs_trace_add_context(struct gk20a *g, u32 context_ptr,
	pid_t pid, u32 vmid, struct nvgpu_list_node *list)
{
	struct nvgpu_gr_fecs_trace *trace = g->fecs_trace;
	struct nvgpu_fecs_trace_context_entry *entry;

	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_ctxsw,
		"adding hash entry context_ptr=%x -> pid=%d, vmid=%d",
		context_ptr, pid, vmid);

	entry = nvgpu_kzalloc(g, sizeof(*entry));
	if (entry == NULL) {
		nvgpu_err(g,
			"can't alloc new entry for context_ptr=%x pid=%d vmid=%d",
			context_ptr, pid, vmid);
		return -ENOMEM;
	}

	nvgpu_init_list_node(&entry->entry);
	entry->context_ptr = context_ptr;
	entry->pid = pid;
	entry->vmid = vmid;

	nvgpu_mutex_acquire(&trace->list_lock);
	nvgpu_list_add_tail(&entry->entry, list);
	nvgpu_mutex_release(&trace->list_lock);

	return 0;
}

void nvgpu_gr_fecs_trace_remove_context(struct gk20a *g, u32 context_ptr,
	struct nvgpu_list_node *list)
{
	struct nvgpu_gr_fecs_trace *trace = g->fecs_trace;
	struct nvgpu_fecs_trace_context_entry *entry, *tmp;

	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_ctxsw,
		"freeing entry context_ptr=%x", context_ptr);

	nvgpu_mutex_acquire(&trace->list_lock);
	nvgpu_list_for_each_entry_safe(entry, tmp, list,
			nvgpu_fecs_trace_context_entry,	entry) {
		if (entry->context_ptr == context_ptr) {
			nvgpu_list_del(&entry->entry);
			nvgpu_log(g, gpu_dbg_ctxsw,
				"freed entry=%p context_ptr=%x", entry,
				entry->context_ptr);
			nvgpu_kfree(g, entry);
			break;
		}
	}
	nvgpu_mutex_release(&trace->list_lock);
}

void nvgpu_gr_fecs_trace_remove_contexts(struct gk20a *g,
	struct nvgpu_list_node *list)
{
	struct nvgpu_gr_fecs_trace *trace = g->fecs_trace;
	struct nvgpu_fecs_trace_context_entry *entry, *tmp;

	nvgpu_mutex_acquire(&trace->list_lock);
	nvgpu_list_for_each_entry_safe(entry, tmp, list,
			nvgpu_fecs_trace_context_entry,	entry) {
		nvgpu_list_del(&entry->entry);
		nvgpu_kfree(g, entry);
	}
	nvgpu_mutex_release(&trace->list_lock);
}

void nvgpu_gr_fecs_trace_find_pid(struct gk20a *g, u32 context_ptr,
	struct nvgpu_list_node *list, pid_t *pid, u32 *vmid)
{
	struct nvgpu_gr_fecs_trace *trace = g->fecs_trace;
	struct nvgpu_fecs_trace_context_entry *entry;

	nvgpu_mutex_acquire(&trace->list_lock);
	nvgpu_list_for_each_entry(entry, list, nvgpu_fecs_trace_context_entry,
			entry) {
		if (entry->context_ptr == context_ptr) {
			nvgpu_log(g, gpu_dbg_ctxsw,
				"found context_ptr=%x -> pid=%d, vmid=%d",
				entry->context_ptr, entry->pid, entry->vmid);
			*pid = entry->pid;
			*vmid = entry->vmid;
			nvgpu_mutex_release(&trace->list_lock);
			return;
		}
	}
	nvgpu_mutex_release(&trace->list_lock);

	*pid = 0;
	*vmid = 0xffffffffU;
}

int nvgpu_gr_fecs_trace_init(struct gk20a *g)
{
	struct nvgpu_gr_fecs_trace *trace;
	int err;

	if (!is_power_of_2(GK20A_FECS_TRACE_NUM_RECORDS)) {
		nvgpu_err(g, "invalid NUM_RECORDS chosen");
		return -EINVAL;
	}

	trace = nvgpu_kzalloc(g, sizeof(struct nvgpu_gr_fecs_trace));
	if (trace == NULL) {
		nvgpu_err(g, "failed to allocate fecs_trace");
		return -ENOMEM;
	}
	g->fecs_trace = trace;

	err = nvgpu_mutex_init(&trace->poll_lock);
	if (err != 0) {
		goto clean;
	}

	err = nvgpu_mutex_init(&trace->list_lock);
	if (err != 0) {
		goto clean_poll_lock;
	}

	err = nvgpu_mutex_init(&trace->enable_lock);
	if (err != 0) {
		goto clean_list_lock;
	}

	nvgpu_init_list_node(&trace->context_list);

	nvgpu_set_enabled(g, NVGPU_SUPPORT_FECS_CTXSW_TRACE, true);

	trace->enable_count = 0;

	return 0;

clean_list_lock:
	nvgpu_mutex_destroy(&trace->list_lock);
clean_poll_lock:
	nvgpu_mutex_destroy(&trace->poll_lock);
clean:
	nvgpu_kfree(g, trace);
	g->fecs_trace = NULL;
	return err;
}

int nvgpu_gr_fecs_trace_deinit(struct gk20a *g)
{
	struct nvgpu_gr_fecs_trace *trace = g->fecs_trace;

	nvgpu_thread_stop(&trace->poll_task);

	nvgpu_gr_fecs_trace_remove_contexts(g, &trace->context_list);

	nvgpu_mutex_destroy(&g->fecs_trace->list_lock);
	nvgpu_mutex_destroy(&g->fecs_trace->poll_lock);
	nvgpu_mutex_destroy(&g->fecs_trace->enable_lock);

	nvgpu_kfree(g, g->fecs_trace);
	g->fecs_trace = NULL;
	return 0;
}

int nvgpu_gr_fecs_trace_num_ts(struct gk20a *g)
{
	return (g->ops.gr.ctxsw_prog.hw_get_ts_record_size_in_bytes()
		- sizeof(struct nvgpu_fecs_trace_record)) / sizeof(u64);
}

struct nvgpu_fecs_trace_record *nvgpu_gr_fecs_trace_get_record(
	struct gk20a *g, int idx)
{
	struct nvgpu_mem *mem = nvgpu_gr_global_ctx_buffer_get_mem(
					g->gr.global_ctx_buffer,
					NVGPU_GR_GLOBAL_CTX_FECS_TRACE_BUFFER);
	if (mem == NULL) {
		return NULL;
	}

	return (struct nvgpu_fecs_trace_record *)
		((u8 *) mem->cpu_va +
		(idx * g->ops.gr.ctxsw_prog.hw_get_ts_record_size_in_bytes()));
}

bool nvgpu_gr_fecs_trace_is_valid_record(struct gk20a *g,
	struct nvgpu_fecs_trace_record *r)
{
	/*
	 * testing magic_hi should suffice. magic_lo is sometimes used
	 * as a sequence number in experimental ucode.
	 */
	return g->ops.gr.ctxsw_prog.is_ts_valid_record(r->magic_hi);
}
