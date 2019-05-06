/*
 * Copyright (c) 2014-2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/lock.h>
#include <nvgpu/kmem.h>
#include <nvgpu/nvgpu_mem.h>
#include <nvgpu/vm.h>
#include <nvgpu/bug.h>
#include <nvgpu/semaphore.h>
#include <nvgpu/channel.h>

#include "semaphore_priv.h"

int nvgpu_hw_semaphore_init(struct nvgpu_channel *ch)
{
	int hw_sema_idx;
	int ret = 0;
	struct nvgpu_hw_semaphore *hw_sema;
	struct nvgpu_semaphore_pool *p = ch->vm->sema_pool;
	int current_value;

	nvgpu_assert(p != NULL);

	nvgpu_mutex_acquire(&p->pool_lock);

	/* Find an available HW semaphore. */
	hw_sema_idx = semaphore_bitmap_alloc(p->semas_alloced,
					       PAGE_SIZE / SEMAPHORE_SIZE);
	if (hw_sema_idx < 0) {
		ret = hw_sema_idx;
		goto fail;
	}

	hw_sema = nvgpu_kzalloc(ch->g, sizeof(struct nvgpu_hw_semaphore));
	if (hw_sema == NULL) {
		ret = -ENOMEM;
		goto fail_free_idx;
	}

	ch->hw_sema = hw_sema;
	hw_sema->ch = ch;
	hw_sema->location.pool = p;
	hw_sema->location.offset = SEMAPHORE_SIZE * (u32)hw_sema_idx;
	current_value = (int)nvgpu_mem_rd(ch->g, &p->rw_mem,
			hw_sema->location.offset);
	nvgpu_atomic_set(&hw_sema->next_value, current_value);

	nvgpu_mutex_release(&p->pool_lock);

	return 0;

fail_free_idx:
	clear_bit(hw_sema_idx, p->semas_alloced);
fail:
	nvgpu_mutex_release(&p->pool_lock);
	return ret;
}

/*
 * Free the channel used semaphore index
 */
void nvgpu_hw_semaphore_free(struct nvgpu_channel *ch)
{
	struct nvgpu_semaphore_pool *p = ch->vm->sema_pool;
	struct nvgpu_hw_semaphore *hw_sema = ch->hw_sema;
	int idx = (int)(hw_sema->location.offset / SEMAPHORE_SIZE);

	nvgpu_assert(p != NULL);

	nvgpu_mutex_acquire(&p->pool_lock);

	clear_bit(idx, p->semas_alloced);

	nvgpu_kfree(ch->g, hw_sema);
	ch->hw_sema = NULL;

	nvgpu_mutex_release(&p->pool_lock);
}

u64 nvgpu_hw_semaphore_addr(struct nvgpu_hw_semaphore *hw_sema)
{
	return nvgpu_semaphore_pool_gpu_va(hw_sema->location.pool, true) +
		hw_sema->location.offset;
}

u32 nvgpu_hw_semaphore_read(struct nvgpu_hw_semaphore *hw_sema)
{
	return nvgpu_mem_rd(hw_sema->ch->g, &hw_sema->location.pool->rw_mem,
			hw_sema->location.offset);
}

/*
 * Fast-forward the hw sema to its tracked max value.
 *
 * Return true if the sema wasn't at the max value and needed updating, false
 * otherwise.
 */
bool nvgpu_hw_semaphore_reset(struct nvgpu_hw_semaphore *hw_sema)
{
	u32 threshold = (u32)nvgpu_atomic_read(&hw_sema->next_value);
	u32 current_val = nvgpu_hw_semaphore_read(hw_sema);

	/*
	 * If the semaphore has already reached the value we would write then
	 * this is really just a NO-OP. However, the sema value shouldn't be
	 * more than what we expect to be the max.
	 */

	bool is_released = nvgpu_semaphore_value_released(threshold + 1U,
				current_val);

	nvgpu_assert(!is_released);

	if (is_released) {
		return false;
	}

	if (current_val == threshold) {
		return false;
	}

	nvgpu_mem_wr(hw_sema->ch->g, &hw_sema->location.pool->rw_mem,
			hw_sema->location.offset, threshold);

	gpu_sema_verbose_dbg(hw_sema->ch->g, "(c=%d) RESET %u -> %u",
			hw_sema->ch->chid, current_val, threshold);

	return true;
}

int nvgpu_hw_semaphore_read_next(struct nvgpu_hw_semaphore *hw_sema)
{
	return nvgpu_atomic_read(&hw_sema->next_value);
}

int nvgpu_hw_semaphore_update_next(struct nvgpu_hw_semaphore *hw_sema)
{
	return nvgpu_atomic_add_return(1, &hw_sema->next_value);
}
