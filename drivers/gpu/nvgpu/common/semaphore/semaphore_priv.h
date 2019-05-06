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


#ifndef NVGPU_SEMAPHORE_PRIV_H
#define NVGPU_SEMAPHORE_PRIV_H

#include <nvgpu/errno.h>
#include <nvgpu/types.h>
#include <nvgpu/list.h>
#include <nvgpu/lock.h>
#include <nvgpu/atomic.h>
#include <nvgpu/kref.h>
#include <nvgpu/nvgpu_mem.h>

struct gk20a;

#define SEMAPHORE_SIZE			16U
/*
 * Max number of channels that can be used is 512. This of course needs to be
 * fixed to be dynamic but still fast.
 */
#define SEMAPHORE_POOL_COUNT		512U
#define SEMAPHORE_SEA_GROWTH_RATE	32U

/*
 * A sea of semaphores pools. Each pool is owned by a single VM. Since multiple
 * channels can share a VM each channel gets it's own HW semaphore from the
 * pool. Channels then allocate regular semaphores - basically just a value that
 * signifies when a particular job is done.
 */
struct nvgpu_semaphore_sea {
	struct nvgpu_list_node pool_list;	/* List of pools in this sea. */
	struct gk20a *gk20a;

	size_t size;			/* Number of pages available. */
	u64 gpu_va;			/* GPU virtual address of sema sea. */
	u64 map_size;			/* Size of the mapping. */

	/*
	 * TODO:
	 * List of pages that we use to back the pools. The number of pages
	 * can grow dynamically since allocating 512 pages for all channels at
	 * once would be a tremendous waste.
	 */
	int page_count;			/* Pages allocated to pools. */

	/*
	 * The read-only memory for the entire semaphore sea. Each semaphore
	 * pool needs a sub-nvgpu_mem that will be mapped as RW in its address
	 * space. This sea_mem cannot be freed until all semaphore_pools have
	 * been freed.
	 */
	struct nvgpu_mem sea_mem;

	/*
	 * Can't use a regular allocator here since the full range of pools are
	 * not always allocated. Instead just use a bitmap.
	 */
	DECLARE_BITMAP(pools_alloced, SEMAPHORE_POOL_COUNT);

	struct nvgpu_mutex sea_lock;		/* Lock alloc/free calls. */
};

/*
 * A semaphore pool. Each address space will own exactly one of these.
 */
struct nvgpu_semaphore_pool {
	struct nvgpu_list_node pool_list_entry;	/* Node for list of pools. */
	u64 gpu_va;				/* GPU access to the pool. */
	u64 gpu_va_ro;				/* GPU access to the pool. */
	u64 page_idx;				/* Index into sea bitmap. */

	DECLARE_BITMAP(semas_alloced, PAGE_SIZE / SEMAPHORE_SIZE);

	struct nvgpu_semaphore_sea *sema_sea;	/* Sea that owns this pool. */

	struct nvgpu_mutex pool_lock;

	/*
	 * This is the address spaces's personal RW table. Other channels will
	 * ultimately map this page as RO. This is a sub-nvgpu_mem from the
	 * sea's mem.
	 */
	struct nvgpu_mem rw_mem;

	bool mapped;

	/*
	 * Sometimes a channel can be released before other channels are
	 * done waiting on it. This ref count ensures that the pool doesn't
	 * go away until all semaphores using this pool are cleaned up first.
	 */
	struct nvgpu_ref ref;
};

struct nvgpu_semaphore_loc {
	struct nvgpu_semaphore_pool *pool; /* Pool that owns this sema. */
	u32 offset;			   /* Byte offset into the pool. */
};

/*
 * Underlying semaphore data structure. This semaphore can be shared amongst
 * other semaphore instances.
 */
struct nvgpu_hw_semaphore {
	struct nvgpu_semaphore_loc location;
	nvgpu_atomic_t next_value;	/* Next available value. */
	struct nvgpu_channel *ch;	/* Channel that owns this sema. */
};

/*
 * A semaphore which the rest of the driver actually uses. This consists of a
 * pointer to a real semaphore and a value to wait for. This allows one physical
 * semaphore to be shared among an essentially infinite number of submits.
 */
struct nvgpu_semaphore {
	struct gk20a *g;
	struct nvgpu_semaphore_loc location;

	nvgpu_atomic_t value;
	bool ready_to_wait;

	struct nvgpu_ref ref;
};


static inline int semaphore_bitmap_alloc(unsigned long *bitmap,
		unsigned long len)
{
	unsigned long idx = find_first_zero_bit(bitmap, len);

	if (idx == len) {
		return -ENOSPC;
	}

	set_bit(idx, bitmap);

	return (int)idx;
}

/*
 * Check if "racer" is over "goal" with wraparound handling.
 */
static inline bool nvgpu_semaphore_value_released(u32 goal, u32 racer)
{
	/*
	 * Handle wraparound with the same heuristic as the hardware does:
	 * although the integer will eventually wrap around, consider a sema
	 * released against a threshold if its value has passed that threshold
	 * but has not wrapped over half of the u32 range over that threshold;
	 * such wrapping is unlikely to happen during a sema lifetime.
	 *
	 * Values for [goal, goal + 0x7fffffff] are considered signaled; that's
	 * precisely half of the 32-bit space. If racer == goal + 0x80000000,
	 * then it needs 0x80000000 increments to wrap again and signal.
	 *
	 * Unsigned arithmetic is used because it's well-defined. This is
	 * effectively the same as: signed_racer - signed_goal > 0.
	 */

	return racer - goal < 0x80000000U;
}

#endif /* NVGPU_SEMAPHORE_PRIV_H */