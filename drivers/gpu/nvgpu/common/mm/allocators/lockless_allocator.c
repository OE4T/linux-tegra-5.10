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

#include <nvgpu/atomic.h>
#include <nvgpu/allocator.h>
#include <nvgpu/kmem.h>
#include <nvgpu/barrier.h>
#include <nvgpu/bug.h>

#include "lockless_allocator_priv.h"

static u64 nvgpu_lockless_alloc_length(struct nvgpu_allocator *a)
{
	struct nvgpu_lockless_allocator *pa = a->priv;

	return pa->length;
}

static u64 nvgpu_lockless_alloc_base(struct nvgpu_allocator *a)
{
	struct nvgpu_lockless_allocator *pa = a->priv;

	return pa->base;
}

static bool nvgpu_lockless_alloc_inited(struct nvgpu_allocator *a)
{
	struct nvgpu_lockless_allocator *pa = a->priv;
	bool inited = pa->inited;

	nvgpu_smp_rmb();
	return inited;
}

static u64 nvgpu_lockless_alloc_end(struct nvgpu_allocator *a)
{
	struct nvgpu_lockless_allocator *pa = a->priv;

	return nvgpu_safe_add_u64(pa->base, pa->length);
}

static u64 nvgpu_lockless_alloc(struct nvgpu_allocator *a, u64 len)
{
	struct nvgpu_lockless_allocator *pa = a->priv;
	int head, new_head, ret;
	u64 addr = 0;

	if (len != pa->blk_size) {
		return 0;
	}

	head = NV_READ_ONCE(pa->head);
	while (head >= 0) {
		new_head = NV_READ_ONCE(pa->next[head]);
		ret = cmpxchg(&pa->head, head, new_head);
		if (ret == head) {
			addr = pa->base + U64(head) * pa->blk_size;
			nvgpu_atomic_inc(&pa->nr_allocs);
			alloc_dbg(a, "Alloc node # %d @ addr 0x%llx", head,
				  addr);
			break;
		}
		head = NV_READ_ONCE(pa->head);
	}

	if (addr != 0ULL) {
		alloc_dbg(a, "Alloc node # %d @ addr 0x%llx", head, addr);
	} else {
		alloc_dbg(a, "Alloc failed!");
	}

	return addr;
}

static void nvgpu_lockless_free(struct nvgpu_allocator *a, u64 addr)
{
	struct nvgpu_lockless_allocator *pa = a->priv;
	int head, ret;
	u64 cur_idx;

	cur_idx = nvgpu_safe_sub_u64(addr, pa->base) / pa->blk_size;

	alloc_dbg(a, "Free node # %llu @ addr 0x%llx", cur_idx, addr);

	while (true) {
		head = NV_READ_ONCE(pa->head);
		NV_WRITE_ONCE(pa->next[cur_idx], head);
		nvgpu_assert(cur_idx <= U64(INT_MAX));
		ret = cmpxchg(&pa->head, head, (int)cur_idx);
		if (ret == head) {
			nvgpu_atomic_dec(&pa->nr_allocs);
			alloc_dbg(a, "Free node # %llu", cur_idx);
			break;
		}
	}
}

static void nvgpu_lockless_alloc_destroy(struct nvgpu_allocator *a)
{
	struct nvgpu_lockless_allocator *pa = a->priv;

#ifdef CONFIG_DEBUG_FS
	nvgpu_fini_alloc_debug(a);
#endif

	nvgpu_vfree(a->g, pa->next);
	nvgpu_kfree(nvgpu_alloc_to_gpu(a), pa);
}

#ifdef __KERNEL__
static void nvgpu_lockless_print_stats(struct nvgpu_allocator *a,
				   struct seq_file *s, int lock)
{
	struct nvgpu_lockless_allocator *pa = a->priv;

	alloc_pstat(s, a, "Lockless allocator params:");
	alloc_pstat(s, a, "  start = 0x%llx", pa->base);
	alloc_pstat(s, a, "  end   = 0x%llx", pa->base + pa->length);

	/* Actual stats. */
	alloc_pstat(s, a, "Stats:");
	alloc_pstat(s, a, "  Number allocs = %d",
		      nvgpu_atomic_read(&pa->nr_allocs));
	alloc_pstat(s, a, "  Number free   = %d",
		      pa->nr_nodes - nvgpu_atomic_read(&pa->nr_allocs));
}
#endif

static const struct nvgpu_allocator_ops pool_ops = {
	.alloc		= nvgpu_lockless_alloc,
	.free_alloc	= nvgpu_lockless_free,

	.base		= nvgpu_lockless_alloc_base,
	.length		= nvgpu_lockless_alloc_length,
	.end		= nvgpu_lockless_alloc_end,
	.inited		= nvgpu_lockless_alloc_inited,

	.fini		= nvgpu_lockless_alloc_destroy,

#ifdef __KERNEL__
	.print_stats	= nvgpu_lockless_print_stats,
#endif
};

int nvgpu_lockless_allocator_init(struct gk20a *g, struct nvgpu_allocator *na,
			      const char *name, u64 base, u64 length,
			      u64 blk_size, u64 flags)
{
	int i;
	int err;
	int nr_nodes;
	u64 count;
	struct nvgpu_lockless_allocator *a;

	if (blk_size == 0ULL) {
		return -EINVAL;
	}

	/*
	 * Ensure we have space for at least one node & there's no overflow.
	 * In order to control memory footprint, we require count < INT_MAX
	 */
	count = length / blk_size;
	if (base == 0ULL || count == 0ULL || count > U64(INT_MAX)) {
		return -EINVAL;
	}

	a = nvgpu_kzalloc(g, sizeof(struct nvgpu_lockless_allocator));
	if (a == NULL) {
		return -ENOMEM;
	}

	err = nvgpu_alloc_common_init(na, g, name, a, false, &pool_ops);
	if (err != 0) {
		goto fail;
	}

	a->next = nvgpu_vzalloc(g, sizeof(*a->next) * count);
	if (a->next == NULL) {
		err = -ENOMEM;
		goto fail;
	}

	/* chain the elements together to form the initial free list  */
	nr_nodes = (int)count;
	for (i = 0; i < nr_nodes; i++) {
		a->next[i] = i + 1;
	}
	a->next[nr_nodes - 1] = -1;

	a->base = base;
	a->length = length;
	a->blk_size = blk_size;
	a->nr_nodes = nr_nodes;
	a->flags = flags;
	nvgpu_atomic_set(&a->nr_allocs, 0);

	nvgpu_smp_wmb();
	a->inited = true;

#ifdef CONFIG_DEBUG_FS
	nvgpu_init_alloc_debug(g, na);
#endif
	alloc_dbg(na, "New allocator: type          lockless");
	alloc_dbg(na, "               base          0x%llx", a->base);
	alloc_dbg(na, "               nodes         %d", a->nr_nodes);
	alloc_dbg(na, "               blk_size      0x%llx", a->blk_size);
	alloc_dbg(na, "               flags         0x%llx", a->flags);

	return 0;

fail:
	nvgpu_kfree(g, a);
	return err;
}
