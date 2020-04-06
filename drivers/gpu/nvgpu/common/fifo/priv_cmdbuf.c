/*
 * Copyright (c) 2011-2020, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/utils.h>
#include <nvgpu/log2.h>
#include <nvgpu/barrier.h>
#include <nvgpu/dma.h>
#include <nvgpu/nvgpu_mem.h>
#include <nvgpu/kmem.h>
#include <nvgpu/channel.h>
#include <nvgpu/priv_cmdbuf.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/trace.h>
#include <nvgpu/circ_buf.h>

struct priv_cmd_queue {
	struct nvgpu_mem mem; /* pushbuf */
	u32 size;	/* allocated length in words */
	u32 put;	/* next entry will begin here */
	u32 get;	/* next entry to free begins here */

	/* an entry is a fragment of the pushbuf memory */
	struct priv_cmd_entry *entries;
	u32 entries_len; /* allocated length */
	u32 entry_put;
	u32 entry_get;
};

/* allocate private cmd buffer queue.
   used for inserting commands before/after user submitted buffers. */
int nvgpu_priv_cmdbuf_queue_alloc(struct nvgpu_channel *ch,
	u32 num_in_flight)
{
	struct gk20a *g = ch->g;
	struct vm_gk20a *ch_vm = ch->vm;
	struct priv_cmd_queue *q;
	u64 size, tmp_size;
	int err = 0;
	u32 wait_size, incr_size;

	/*
	 * sema size is at least as much as syncpt size, but semas may not be
	 * enabled in the build. If neither semas nor syncpts are enabled, priv
	 * cmdbufs and as such kernel mode submits with job tracking won't be
	 * supported.
	 */
#ifdef CONFIG_NVGPU_SW_SEMAPHORE
	wait_size = g->ops.sync.sema.get_wait_cmd_size();
	incr_size = g->ops.sync.sema.get_incr_cmd_size();
#else
	wait_size = g->ops.sync.syncpt.get_wait_cmd_size();
	incr_size = g->ops.sync.syncpt.get_incr_cmd_size(true);
#endif

	/*
	 * Compute the amount of priv_cmdbuf space we need. In general the
	 * worst case is the kernel inserts both a semaphore pre-fence and
	 * post-fence. Any sync-pt fences will take less memory so we can
	 * ignore them unless they're the only supported type. Jobs can also
	 * have more than one pre-fence but that's abnormal and we'll -EAGAIN
	 * if such jobs would fill the queue.
	 *
	 * A semaphore ACQ (fence-wait) is 8 words: semaphore_a, semaphore_b,
	 * semaphore_c, and semaphore_d. A semaphore INCR (fence-get) will be
	 * 10 words: all the same as an ACQ plus a non-stalling intr which is
	 * another 2 words. In reality these numbers vary by chip but we'll use
	 * 8 and 10 as examples.
	 *
	 * We have two cases to consider: the first is we base the size of the
	 * queue on the gpfifo count. Here we multiply by a factor of 1/3
	 * because at most a third of the GPFIFO entries can be used for
	 * user-submitted jobs; another third goes to wait entries, and the
	 * final third to incr entries. There will be one pair of acq and incr
	 * commands for each job.
	 *
	 *   gpfifo entry num * (1 / 3) * (8 + 10) * 4 bytes
	 *
	 * If instead num_in_flight is specified then we will use that to size
	 * the queue instead of a third of the gpfifo entry count. The worst
	 * case is still both sync commands (one ACQ and one INCR) per submit so
	 * we have a queue size of:
	 *
	 *   num_in_flight * (8 + 10) * 4 bytes
	 */
	if (num_in_flight == 0U) {
		/* round down to ensure space for all priv cmds */
		num_in_flight = ch->gpfifo.entry_num / 3U;
	}

	size = num_in_flight * (wait_size + incr_size) * sizeof(u32);

	tmp_size = PAGE_ALIGN(roundup_pow_of_two(size));
	if (tmp_size > U32_MAX) {
		return -ERANGE;
	}
	size = (u32)tmp_size;

	q = nvgpu_kzalloc(g, sizeof(*q));
	if (q == NULL) {
		return -ENOMEM;
	}

	if (num_in_flight > U32_MAX / 2U) {
		err = -ERANGE;
		goto err_free_queue;
	}

	q->entries_len = 2U * num_in_flight;
	q->entries = nvgpu_vzalloc(g,
			nvgpu_safe_mult_u64((u64)q->entries_len,
				sizeof(*q->entries)));
	if (q->entries == NULL) {
		err = -ENOMEM;
		goto err_free_queue;
	}

	err = nvgpu_dma_alloc_map_sys(ch_vm, size, &q->mem);
	if (err != 0) {
		nvgpu_err(g, "%s: memory allocation failed", __func__);
		goto err_free_entries;
	}

	tmp_size = q->mem.size / sizeof(u32);
	nvgpu_assert(tmp_size <= U32_MAX);
	q->size = (u32)tmp_size;

	ch->priv_cmd_q = q;

	return 0;
err_free_entries:
	nvgpu_vfree(g, q->entries);
err_free_queue:
	nvgpu_kfree(g, q);
	return err;
}

void nvgpu_priv_cmdbuf_queue_free(struct nvgpu_channel *ch)
{
	struct vm_gk20a *ch_vm = ch->vm;
	struct priv_cmd_queue *q = ch->priv_cmd_q;
	struct gk20a *g = ch->g;

	if (q == NULL) {
		return;
	}

	nvgpu_dma_unmap_free(ch_vm, &q->mem);
	nvgpu_vfree(g, q->entries);
	nvgpu_kfree(g, q);

	ch->priv_cmd_q = NULL;
}

/* allocate a cmd buffer with given size. size is number of u32 entries */
static int nvgpu_priv_cmdbuf_alloc_buf(struct nvgpu_channel *c, u32 orig_size,
			     struct priv_cmd_entry *e)
{
	struct priv_cmd_queue *q = c->priv_cmd_q;
	u32 size = orig_size;
	u32 free_count;

	nvgpu_log_fn(c->g, "size %d", orig_size);

	/*
	 * If free space in the end is less than requested, increase the size
	 * to make the real allocated space start from beginning. The hardware
	 * expects each cmdbuf to be contiguous in the dma space.
	 *
	 * This too small extra space in the end may happen because the
	 * requested wait and incr command buffers do not necessarily align
	 * with the whole buffer capacity. They don't always align because the
	 * buffer size is rounded to the next power of two and because not all
	 * jobs necessarily use exactly one wait command.
	 */
	if (nvgpu_safe_add_u32(q->put, size) > q->size) {
		size = orig_size + (q->size - q->put);
	}

	nvgpu_log_info(c->g, "ch %d: priv cmd queue get:put %d:%d",
			c->chid, q->get, q->put);

	nvgpu_assert(q->put < q->size);
	nvgpu_assert(q->get < q->size);
	nvgpu_assert(q->size > 0U);
	free_count = (q->size - q->put + q->get - 1U) & (q->size - 1U);

	if (size > free_count) {
		return -EAGAIN;
	}

	e->fill_off = 0;
	e->size = orig_size;
	e->alloc_size = size;
	e->mem = &q->mem;

	/*
	 * if we have increased size to skip free space in the end, set put
	 * to beginning of cmd buffer + size, as if the prev put was at
	 * position 0.
	 */
	if (size != orig_size) {
		e->off = 0;
		e->gva = q->mem.gpu_va;
		q->put = orig_size;
	} else {
		e->off = q->put;
		e->gva = nvgpu_safe_add_u64(q->mem.gpu_va,
				nvgpu_safe_mult_u64((u64)q->put, sizeof(u32)));
		q->put = (q->put + orig_size) & (q->size - 1U);
	}

	/* we already handled q->put + size > q->size so BUG_ON this */
	BUG_ON(q->put > q->size);

	/*
	 * commit the previous writes before making the entry valid.
	 * see the corresponding nvgpu_smp_rmb() in
	 * nvgpu_priv_cmdbuf_free().
	 */
	nvgpu_smp_wmb();

	e->valid = true;
	nvgpu_log_fn(c->g, "done");

	return 0;
}

int nvgpu_priv_cmdbuf_alloc(struct nvgpu_channel *c, u32 size,
			     struct priv_cmd_entry **e)
{
	struct priv_cmd_queue *q = c->priv_cmd_q;
	u32 next_put = nvgpu_safe_add_u32(q->entry_put, 1U) % q->entries_len;
	struct priv_cmd_entry *entry;
	int err;

	if (next_put == q->entry_get) {
		return -EAGAIN;
	}
	entry = &q->entries[q->entry_put];

	err = nvgpu_priv_cmdbuf_alloc_buf(c, size, entry);
	if (err != 0) {
		return err;
	}

	q->entry_put = next_put;
	*e = entry;

	return 0;
}

void nvgpu_priv_cmdbuf_rollback(struct nvgpu_channel *ch,
		struct priv_cmd_entry *e)
{
	struct priv_cmd_queue *q = ch->priv_cmd_q;

	nvgpu_assert(q->put < q->size);
	nvgpu_assert(q->size > 0U);
	nvgpu_assert(e->alloc_size <= q->size);
	q->put = (q->put + q->size - e->alloc_size) & (q->size - 1U);

	(void)memset(e, 0, sizeof(*e));

	nvgpu_assert(q->entry_put < q->entries_len);
	nvgpu_assert(q->entries_len > 0U);
	q->entry_put = (q->entry_put + q->entries_len - 1U)
		% q->entries_len;
}

void nvgpu_priv_cmdbuf_free(struct nvgpu_channel *ch,
		struct priv_cmd_entry *e)
{
	struct priv_cmd_queue *q = ch->priv_cmd_q;
	struct gk20a *g = ch->g;

	if (e->valid) {
		/* read the entry's valid flag before reading its contents */
		nvgpu_smp_rmb();
		if ((q->get != e->off) && e->off != 0U) {
			nvgpu_err(g, "requests out-of-order, ch=%d",
				  ch->chid);
		}
		nvgpu_assert(q->size > 0U);
		q->get = nvgpu_safe_add_u32(e->off, e->size) & (q->size - 1U);
		q->entry_get = nvgpu_safe_add_u32(q->entry_get, 1U)
			% q->entries_len;
	}

	(void)memset(e, 0, sizeof(*e));
}

void nvgpu_priv_cmdbuf_append(struct gk20a *g, struct priv_cmd_entry *e,
		u32 *data, u32 entries)
{
	nvgpu_assert(e->fill_off + entries <= e->size);
	nvgpu_mem_wr_n(g, e->mem, (e->off + e->fill_off) * sizeof(u32),
			data, entries * sizeof(u32));
	e->fill_off += entries;
}

void nvgpu_priv_cmdbuf_append_zeros(struct gk20a *g, struct priv_cmd_entry *e,
		u32 entries)
{
	nvgpu_assert(e->fill_off + entries <= e->size);
	nvgpu_memset(g, e->mem, (e->off + e->fill_off) * sizeof(u32),
			0, entries * sizeof(u32));
	e->fill_off += entries;
}

void nvgpu_priv_cmdbuf_finish(struct gk20a *g, struct priv_cmd_entry *e,
		u64 *gva, u32 *size)
{
	/*
	 * The size is written to the pushbuf entry, so make sure this buffer
	 * is complete at this point. The responsibility of the channel sync is
	 * to be consistent in allocation and usage, and the matching size and
	 * add gops (e.g., get_wait_cmd_size, add_wait_cmd) help there.
	 */
	nvgpu_assert(e->fill_off == e->size);

#ifdef CONFIG_NVGPU_TRACE
	if (e->mem->aperture == APERTURE_SYSMEM) {
		trace_gk20a_push_cmdbuf(g->name, 0, e->size, 0,
				(u32 *)e->mem->cpu_va + e->off);
	}
#endif
	*gva = e->gva;
	*size = e->size;
}
