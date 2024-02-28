/*
 * Copyright (c) 2017-2021, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/bug.h>
#include <nvgpu/ce_app.h>
#include <nvgpu/timers.h>
#include <nvgpu/dma.h>
#include <nvgpu/vidmem.h>
#include <nvgpu/page_allocator.h>
#include <nvgpu/enabled.h>
#include <nvgpu/sizes.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/nvgpu_sgt.h>
#include <nvgpu/fence.h>


/*
 * This is expected to be called from the shutdown path (or the error path in
 * the vidmem init code). As such we do not expect new vidmem frees to be
 * enqueued.
 */
void nvgpu_vidmem_destroy(struct gk20a *g)
{
	struct nvgpu_timeout timeout;

	if (g->ops.fb.get_vidmem_size == NULL) {
		return;
	}

	nvgpu_timeout_init_retry(g, &timeout, 100);

	/*
	 * Ensure that the thread runs one last time to flush anything in the
	 * queue.
	 */
	nvgpu_cond_signal_interruptible(&g->mm.vidmem.clearing_thread_cond);

	/*
	 * Wait for at most 1 second before just continuing on. It doesn't make
	 * sense to hang the system over some potential memory leaks.
	 */
	do {
		bool empty;

		nvgpu_mutex_acquire(&g->mm.vidmem.clear_list_mutex);
		empty = nvgpu_list_empty(&g->mm.vidmem.clear_list_head);
		nvgpu_mutex_release(&g->mm.vidmem.clear_list_mutex);

		if (empty) {
			break;
		}

		nvgpu_msleep(10);
	} while (nvgpu_timeout_expired(&timeout) == 0);

	/*
	 * Kill the vidmem clearing thread now. This will wake the thread up
	 * automatically and cause the wait_interruptible condition trigger.
	 */
	nvgpu_thread_stop(&g->mm.vidmem.clearing_thread);

	if (nvgpu_alloc_initialized(&g->mm.vidmem.allocator)) {
		nvgpu_alloc_destroy(&g->mm.vidmem.allocator);
	}

	if (nvgpu_alloc_initialized(&g->mm.vidmem.bootstrap_allocator)) {
		nvgpu_alloc_destroy(&g->mm.vidmem.bootstrap_allocator);
	}
}

static int nvgpu_vidmem_clear_fence_wait(struct gk20a *g,
		struct nvgpu_fence_type *fence_out)
{
	struct nvgpu_timeout timeout;
	bool done;
	int err;

	nvgpu_timeout_init_cpu_timer(g, &timeout, nvgpu_get_poll_timeout(g));

	do {
		err = nvgpu_fence_wait(g, fence_out,
				       nvgpu_get_poll_timeout(g));
		if (err != -ERESTARTSYS) {
			done = true;
		} else if (nvgpu_timeout_expired(&timeout) != 0) {
			done = true;
		} else {
			done = false;
		}
	} while (!done);

	nvgpu_fence_put(fence_out);
	if (err != 0) {
		nvgpu_err(g,
			"fence wait failed for CE execute ops");
		return err;
	}

	return 0;
}

static int nvgpu_vidmem_do_clear_all(struct gk20a *g)
{
	struct mm_gk20a *mm = &g->mm;
	struct nvgpu_fence_type *fence_out = NULL;
	int err = 0;

	if (mm->vidmem.ce_ctx_id == NVGPU_CE_INVAL_CTX_ID) {
		return -EINVAL;
	}

	vidmem_dbg(g, "Clearing all VIDMEM:");

#ifdef CONFIG_NVGPU_DGPU
	err = nvgpu_ce_execute_ops(g,
			mm->vidmem.ce_ctx_id,
			0,
			mm->vidmem.base,
			mm->vidmem.bootstrap_base - mm->vidmem.base,
			0x00000000,
			NVGPU_CE_DST_LOCATION_LOCAL_FB,
			NVGPU_CE_MEMSET,
			0,
			&fence_out);
	if (err != 0) {
		nvgpu_err(g,
			"Failed to clear vidmem : %d", err);
		return err;
	}
#else
	/* fail due to lack of ce app support */
	return -ENOSYS;
#endif

	if (fence_out != NULL) {
		err = nvgpu_vidmem_clear_fence_wait(g, fence_out);
		if (err != 0) {
			return err;
		}
	}

	mm->vidmem.cleared = true;

	vidmem_dbg(g, "Done!");

	return 0;
}

void nvgpu_vidmem_thread_pause_sync(struct mm_gk20a *mm)
{
	/*
	 * On the first increment of the pause_count (0 -> 1) take the pause
	 * lock and prevent the vidmem clearing thread from processing work
	 * items.
	 *
	 * Otherwise the increment is all that's needed - it's essentially a
	 * ref-count for the number of pause() calls.
	 *
	 * The sync component is implemented by waiting for the lock to be
	 * released by the clearing thread in case the thread is currently
	 * processing work items.
	 */
	if (nvgpu_atomic_inc_return(&mm->vidmem.pause_count) == 1) {
		nvgpu_mutex_acquire(&mm->vidmem.clearing_thread_lock);
	}

	vidmem_dbg(mm->g, "Clearing thread paused; new count=%d",
		   nvgpu_atomic_read(&mm->vidmem.pause_count));
}

void nvgpu_vidmem_thread_unpause(struct mm_gk20a *mm)
{
	vidmem_dbg(mm->g, "Unpausing clearing thread; current count=%d",
		   nvgpu_atomic_read(&mm->vidmem.pause_count));

	/*
	 * And on the last decrement (1 -> 0) release the pause lock and let
	 * the vidmem clearing thread continue.
	 */
	if (nvgpu_atomic_dec_return(&mm->vidmem.pause_count) == 0) {
		nvgpu_mutex_release(&mm->vidmem.clearing_thread_lock);
		vidmem_dbg(mm->g, "  > Clearing thread really unpaused!");
	}
}

int nvgpu_vidmem_clear_list_enqueue(struct gk20a *g, struct nvgpu_mem *mem)
{
	struct mm_gk20a *mm = &g->mm;

	/*
	 * Crap. Can't enqueue new vidmem bufs! CE may be gone!
	 *
	 * However, an errant app can hold a vidmem dma_buf FD open past when
	 * the nvgpu driver has exited. Thus when the FD does get closed
	 * eventually the dma_buf release function will try to call the vidmem
	 * free function which will attempt to enqueue the vidmem into the
	 * vidmem clearing thread.
	 */
	if (nvgpu_is_enabled(g, NVGPU_DRIVER_IS_DYING)) {
		return -ENOSYS;
	}

	nvgpu_mutex_acquire(&mm->vidmem.clear_list_mutex);
	nvgpu_list_add_tail(&mem->clear_list_entry,
			    &mm->vidmem.clear_list_head);
	nvgpu_atomic64_add((long)mem->aligned_size, &mm->vidmem.bytes_pending);
	nvgpu_mutex_release(&mm->vidmem.clear_list_mutex);

	nvgpu_cond_signal_interruptible(&mm->vidmem.clearing_thread_cond);

	return 0;
}

static struct nvgpu_mem *nvgpu_vidmem_clear_list_dequeue(struct mm_gk20a *mm)
{
	struct nvgpu_mem *mem = NULL;

	nvgpu_mutex_acquire(&mm->vidmem.clear_list_mutex);
	if (!nvgpu_list_empty(&mm->vidmem.clear_list_head)) {
		mem = nvgpu_list_first_entry(&mm->vidmem.clear_list_head,
				nvgpu_mem, clear_list_entry);
		nvgpu_list_del(&mem->clear_list_entry);
	}
	nvgpu_mutex_release(&mm->vidmem.clear_list_mutex);

	return mem;
}

static void nvgpu_vidmem_clear_pending_allocs(struct mm_gk20a *mm)
{
	struct gk20a *g = mm->g;
	struct nvgpu_mem *mem;
	int err;

	vidmem_dbg(g, "Running VIDMEM clearing thread:");

	while ((mem = nvgpu_vidmem_clear_list_dequeue(mm)) != NULL) {
		err = nvgpu_vidmem_clear(g, mem);
		if (err != 0) {
			nvgpu_err(g, "nvgpu_vidmem_clear() failed err=%d", err);
		}

		WARN_ON(nvgpu_atomic64_sub_return((long)mem->aligned_size,
					&g->mm.vidmem.bytes_pending) < 0);
		mem->size = 0;
		mem->aperture = APERTURE_INVALID;

		nvgpu_mem_free_vidmem_alloc(g, mem);
		nvgpu_kfree(g, mem);
	}

	vidmem_dbg(g, "Done!");
}

static int nvgpu_vidmem_clear_pending_allocs_thr(void *mm_ptr)
{
	struct mm_gk20a *mm = mm_ptr;

	/*
	 * Simple thread who's sole job is to periodically clear userspace
	 * vidmem allocations that have been recently freed.
	 *
	 * Since it doesn't make sense to run unless there's pending work a
	 * condition field is used to wait for work. When the DMA API frees a
	 * userspace vidmem buf it enqueues it into the clear list and alerts us
	 * that we have some work to do.
	 */

	while (!nvgpu_thread_should_stop(&mm->vidmem.clearing_thread)) {
		int ret;

		/*
		 * Wait for work but also make sure we should not be paused.
		 */
		ret = NVGPU_COND_WAIT_INTERRUPTIBLE(
				&mm->vidmem.clearing_thread_cond,
				nvgpu_thread_should_stop(
					&mm->vidmem.clearing_thread) ||
				!nvgpu_list_empty(&mm->vidmem.clear_list_head),
				0U);
		if (ret == -ERESTARTSYS) {
			continue;
		}

		/*
		 * Use this lock to implement a pause mechanism. By taking this
		 * lock some other code can prevent this thread from processing
		 * work items.
		 */
		if (nvgpu_mutex_tryacquire(&mm->vidmem.clearing_thread_lock)
									== 0) {
			continue;
		}

		nvgpu_vidmem_clear_pending_allocs(mm);

		nvgpu_mutex_release(&mm->vidmem.clearing_thread_lock);
	}

	return 0;
}

int nvgpu_vidmem_init(struct mm_gk20a *mm)
{
	struct gk20a *g = mm->g;
	u64 bootstrap_base, base;
	u64 bootstrap_size = SZ_512M;
	u64 default_page_size = SZ_64K;
	size_t size;
	int err;
	static struct nvgpu_alloc_carveout bootstrap_co =
		NVGPU_CARVEOUT("bootstrap-region", 0, 0);

	if (g->ops.fb.get_vidmem_size ==  NULL) {

		/*
		 * As it is a common function, the return value
		 * need to be handled for igpu.
		 */
		return 0;
	} else {
		size = g->ops.fb.get_vidmem_size(g);
		if (size == 0UL) {
			nvgpu_err(g, "Found zero vidmem");
			return -ENOMEM;
		}
	}

	vidmem_dbg(g, "init begin");

#ifdef CONFIG_NVGPU_SIM
	if (nvgpu_is_enabled(g, NVGPU_IS_FMODEL)) {
		bootstrap_size = SZ_32M;
	}
#endif

	bootstrap_co.base = size - bootstrap_size;
	bootstrap_co.length = bootstrap_size;

	bootstrap_base = bootstrap_co.base;
	base = default_page_size;

	/*
	 * Bootstrap allocator for use before the CE is initialized (CE
	 * initialization requires vidmem but we want to use the CE to zero
	 * out vidmem before allocating it...
	 */
	err = nvgpu_allocator_init(g, &g->mm.vidmem.bootstrap_allocator,
				NULL, "vidmem-bootstrap", bootstrap_base,
				bootstrap_size,	SZ_4K, 0ULL,
				GPU_ALLOC_FORCE_CONTIG, PAGE_ALLOCATOR);

	err = nvgpu_allocator_init(g, &g->mm.vidmem.allocator, NULL,
			"vidmem", base, size - base, default_page_size, 0ULL,
			GPU_ALLOC_4K_VIDMEM_PAGES, PAGE_ALLOCATOR);
	if (err != 0) {
		nvgpu_err(g, "Failed to register vidmem for size %zu: %d",
				size, err);
		return err;
	}

	/* Reserve bootstrap region in vidmem allocator */
	err = nvgpu_alloc_reserve_carveout(&g->mm.vidmem.allocator,
		&bootstrap_co);
	if (err != 0) {
		nvgpu_err(g, "nvgpu_alloc_reserve_carveout() failed err=%d",
			err);
		goto fail;
	}

	mm->vidmem.base = base;
	mm->vidmem.size = size - base;
	mm->vidmem.bootstrap_base = bootstrap_base;
	mm->vidmem.bootstrap_size = bootstrap_size;

	err = nvgpu_cond_init(&mm->vidmem.clearing_thread_cond);
	if (err != 0) {
		goto fail;
	}

	nvgpu_atomic64_set(&mm->vidmem.bytes_pending, 0);
	nvgpu_init_list_node(&mm->vidmem.clear_list_head);

	nvgpu_mutex_init(&mm->vidmem.clear_list_mutex);
	nvgpu_mutex_init(&mm->vidmem.clearing_thread_lock);
	nvgpu_mutex_init(&mm->vidmem.first_clear_mutex);

	nvgpu_atomic_set(&mm->vidmem.pause_count, 0);

	/*
	 * Start the thread off in the paused state. The thread doesn't have to
	 * be running for this to work. It will be woken up later on in
	 * finalize_poweron(). We won't necessarily have a CE context yet
	 * either, so hypothetically one could cause a race where we try to
	 * clear a vidmem struct before we have a CE context to do so.
	 */
	nvgpu_vidmem_thread_pause_sync(mm);

	err = nvgpu_thread_create(&mm->vidmem.clearing_thread, mm,
				  nvgpu_vidmem_clear_pending_allocs_thr,
				  "vidmem-clear");
	if (err != 0) {
		goto fail;
	}

	vidmem_dbg(g, "VIDMEM Total: %zu MB", size >> 20);
	vidmem_dbg(g, "VIDMEM Ranges:");
	vidmem_dbg(g, "  0x%-10llx -> 0x%-10llx Primary",
		   mm->vidmem.base, mm->vidmem.base + mm->vidmem.size);
	vidmem_dbg(g, "  0x%-10llx -> 0x%-10llx Bootstrap",
		   mm->vidmem.bootstrap_base,
		   mm->vidmem.bootstrap_base + mm->vidmem.bootstrap_size);
	vidmem_dbg(g, "VIDMEM carveouts:");
	vidmem_dbg(g, "  0x%-10llx -> 0x%-10llx %s",
		   bootstrap_co.base, bootstrap_co.base + bootstrap_co.length,
		   bootstrap_co.name);

	return 0;

fail:
	nvgpu_cond_destroy(&mm->vidmem.clearing_thread_cond);
	nvgpu_vidmem_destroy(g);
	return err;
}

int nvgpu_vidmem_get_space(struct gk20a *g, u64 *space)
{
	struct nvgpu_allocator *allocator = &g->mm.vidmem.allocator;

	nvgpu_log_fn(g, " ");

	if (!nvgpu_alloc_initialized(allocator)) {
		return -ENOSYS;
	}

	*space = nvgpu_alloc_space(allocator) +
		U64(nvgpu_atomic64_read(&g->mm.vidmem.bytes_pending));
	return 0;
}

int nvgpu_vidmem_clear(struct gk20a *g, struct nvgpu_mem *mem)
{
	struct nvgpu_fence_type *fence_out = NULL;
	struct nvgpu_fence_type *last_fence = NULL;
	struct nvgpu_page_alloc *alloc = NULL;
	void *sgl = NULL;
	int err = 0;

	if (g->mm.vidmem.ce_ctx_id == NVGPU_CE_INVAL_CTX_ID) {
		return -EINVAL;
	}

	alloc = mem->vidmem_alloc;

	nvgpu_sgt_for_each_sgl(sgl, &alloc->sgt) {
		if (last_fence != NULL) {
			nvgpu_fence_put(last_fence);
		}

#ifdef CONFIG_NVGPU_DGPU
		err = nvgpu_ce_execute_ops(g,
			g->mm.vidmem.ce_ctx_id,
			0,
			nvgpu_sgt_get_phys(g, &alloc->sgt, sgl),
			nvgpu_sgt_get_length(&alloc->sgt, sgl),
			0x00000000,
			NVGPU_CE_DST_LOCATION_LOCAL_FB,
			NVGPU_CE_MEMSET,
			0,
			&fence_out);
#else
		/* fail due to lack of ce app support */
		err = -ENOSYS;
#endif

		if (err != 0) {
#ifdef CONFIG_NVGPU_DGPU
			nvgpu_err(g,
				"Failed nvgpu_ce_execute_ops[%d]", err);
#endif
			return err;
		}

		vidmem_dbg(g, "  > [0x%llx  +0x%llx]",
			   nvgpu_sgt_get_phys(g, &alloc->sgt, sgl),
			   nvgpu_sgt_get_length(&alloc->sgt, sgl));

		last_fence = fence_out;
	}

	if (last_fence != NULL) {
		err = nvgpu_vidmem_clear_fence_wait(g, last_fence);
		if (err != 0) {
			return err;
		}
	}

	vidmem_dbg(g, "  Done");

	return err;
}

static int nvgpu_vidmem_clear_all(struct gk20a *g)
{
	int err;

	if (g->mm.vidmem.cleared) {
		return 0;
	}

	nvgpu_mutex_acquire(&g->mm.vidmem.first_clear_mutex);
	if (!g->mm.vidmem.cleared) {
		err = nvgpu_vidmem_do_clear_all(g);
		if (err != 0) {
			nvgpu_mutex_release(&g->mm.vidmem.first_clear_mutex);
			nvgpu_err(g, "failed to clear whole vidmem");
			return err;
		}
	}
	nvgpu_mutex_release(&g->mm.vidmem.first_clear_mutex);

	return 0;
}

int nvgpu_vidmem_user_alloc(struct gk20a *g, size_t bytes,
				struct nvgpu_vidmem_buf **vidmem_buf)
{
	struct nvgpu_vidmem_buf *buf;
	int err;

	if (vidmem_buf == NULL) {
		return -EINVAL;
	}

	err = nvgpu_vidmem_clear_all(g);
	if (err != 0) {
		return -ENOMEM;
	}

	buf = nvgpu_kzalloc(g, sizeof(*buf));
	if (buf == NULL) {
		return -ENOMEM;
	}

	buf->g = g;
	buf->mem = nvgpu_kzalloc(g, sizeof(*buf->mem));
	if (buf->mem == NULL) {
		err = -ENOMEM;
		goto fail;
	}

	err = nvgpu_dma_alloc_vid(g, bytes, buf->mem);
	if (err != 0) {
		goto fail;
	}

	/*
	 * Alerts the DMA API that when we free this vidmem buf we have to
	 * clear it to avoid leaking data to userspace.
	 */
	buf->mem->mem_flags |= NVGPU_MEM_FLAG_USER_MEM;

	*vidmem_buf = buf;

	return 0;

fail:
	/* buf will never be NULL here. */
	nvgpu_kfree(g, buf->mem);
	nvgpu_kfree(g, buf);
	return err;
}

void nvgpu_vidmem_buf_free(struct gk20a *g, struct nvgpu_vidmem_buf *buf)
{
	/*
	 * In some error paths it's convenient to be able to "free" a NULL buf.
	 */
	if (buf == NULL) {
		return;
	}

	nvgpu_dma_free(g, buf->mem);

	/*
	 * We don't free buf->mem here. This is handled by nvgpu_dma_free()!
	 * Since these buffers are cleared in the background the nvgpu_mem
	 * struct must live on through that. We transfer ownership here to the
	 * DMA API and let the DMA API free the buffer.
	 */
	nvgpu_kfree(g, buf);
}
