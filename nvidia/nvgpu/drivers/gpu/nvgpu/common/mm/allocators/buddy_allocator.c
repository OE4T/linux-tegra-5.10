/*
 * Copyright (c) 2016-2022, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/allocator.h>
#include <nvgpu/kmem.h>
#include <nvgpu/bug.h>
#include <nvgpu/log2.h>
#include <nvgpu/barrier.h>
#include <nvgpu/mm.h>
#include <nvgpu/vm.h>
#include <nvgpu/static_analysis.h>
#include <nvgpu/string.h>

#include "buddy_allocator_priv.h"

/* Some other buddy allocator functions. */
static struct nvgpu_buddy *balloc_free_buddy(struct nvgpu_buddy_allocator *a,
					     u64 addr);
static void balloc_coalesce(struct nvgpu_buddy_allocator *a,
			    struct nvgpu_buddy *b);
static void balloc_do_free_fixed(struct nvgpu_buddy_allocator *a,
				 struct nvgpu_fixed_alloc *falloc);

/*
 * This function is not present in older kernel's list.h code.
 */
#ifndef list_last_entry
#define list_last_entry(ptr, type, member)	\
	list_entry((ptr)->prev, type, member)
#endif

/*
 * GPU buddy allocator for various address spaces.
 *
 * Current limitations:
 *   o  A fixed allocation could potentially be made that borders PDEs with
 *      different PTE sizes. This would require that fixed buffer to have
 *      different sized PTEs for different parts of the allocation. Probably
 *      best to just require PDE alignment for fixed address allocs.
 *
 *   o  It is currently possible to make an allocator that has a buddy alignment
 *      out of sync with the PDE block size alignment. A simple example is a
 *      32GB address space starting at byte 1. Every buddy is shifted off by 1
 *      which means each buddy corresponf to more than one actual GPU page. The
 *      best way to fix this is probably just require PDE blocksize alignment
 *      for the start of the address space. At the moment all allocators are
 *      easily PDE aligned so this hasn't been a problem.
 */

static u32 nvgpu_balloc_page_size_to_pte_size(struct nvgpu_buddy_allocator *a,
					      u32 page_size)
{
	if ((a->flags & GPU_ALLOC_GVA_SPACE) == 0ULL) {
		return BALLOC_PTE_SIZE_ANY;
	}

	/*
	 * Make sure the page size is actually valid!
	 */
	if (page_size == a->vm->big_page_size) {
		return BALLOC_PTE_SIZE_BIG;
	} else if (page_size == SZ_4K) {
		return BALLOC_PTE_SIZE_SMALL;
	} else if (page_size == BALLOC_PTE_SIZE_ANY) {
		/* With gva_space enabled, only 2 types of PTE sizes allowed */
		return BALLOC_PTE_SIZE_SMALL;
	} else {
		return BALLOC_PTE_SIZE_INVALID;
	}
}

/*
 * Pick a suitable maximum order for this allocator.
 *
 * Hueristic: Just guessing that the best max order is the largest single
 * block that will fit in the address space.
 */
static void balloc_compute_max_order(struct nvgpu_buddy_allocator *a)
{
	u64 true_max_order = nvgpu_ilog2(a->blks);

	if (true_max_order > GPU_BALLOC_MAX_ORDER) {
		alloc_dbg(balloc_owner(a),
			  "Oops: Can't manage more than 1 Exabyte memory");
		nvgpu_do_assert();
	}

	if ((a->max_order == 0ULL) || (a->max_order > true_max_order)) {
		a->max_order = true_max_order;
	}
}

/*
 * Since we can only allocate in chucks of a->blk_size we need to trim off
 * any excess data that is not aligned to a->blk_size.
 */
static int balloc_allocator_align(struct nvgpu_buddy_allocator *a)
{
	int err = 0;
	u64 blks;
	a->start = NVGPU_ALIGN(a->base, a->blk_size);
	WARN_ON(a->start != a->base);
	nvgpu_assert(a->blk_size > 0ULL);
	a->end   = nvgpu_safe_add_u64(a->base, a->length) &
						~(a->blk_size - 1U);
	a->count = nvgpu_safe_sub_u64(a->end, a->start);
	blks = a->count >> a->blk_shift;
	if (blks == 0ULL) {
		err = -EINVAL;
		goto fail;
	}
	a->blks  = blks;

fail:
	return err;
}

/*
 * Pass NULL for parent if you want a top level buddy.
 */
static struct nvgpu_buddy *balloc_new_buddy(struct nvgpu_buddy_allocator *a,
					    struct nvgpu_buddy *parent,
					    u64 start, u64 order)
{
	struct nvgpu_buddy *new_buddy;

	new_buddy = nvgpu_kmem_cache_alloc(a->buddy_cache);
	if (new_buddy == NULL) {
		return NULL;
	}

	(void) memset(new_buddy, 0, sizeof(struct nvgpu_buddy));

	new_buddy->parent = parent;
	new_buddy->start = start;
	new_buddy->order = order;
	new_buddy->end = nvgpu_safe_mult_u64(U64(1) << order, a->blk_size);
	new_buddy->end = nvgpu_safe_add_u64(new_buddy->end, start);
	new_buddy->pte_size = BALLOC_PTE_SIZE_ANY;

	return new_buddy;
}

static void balloc_buddy_list_do_add(struct nvgpu_buddy_allocator *a,
				     struct nvgpu_buddy *b,
				     struct nvgpu_list_node *list)
{
	if (buddy_is_in_list(b)) {
		alloc_dbg(balloc_owner(a),
			  "Oops: adding added buddy (%llu:0x%llx)",
			  b->order, b->start);
		nvgpu_do_assert();
	}

	/*
	 * Add big PTE blocks to the tail, small to the head for GVA spaces.
	 * This lets the code that checks if there are available blocks check
	 * without cycling through the entire list.
	 */
	if (((a->flags & GPU_ALLOC_GVA_SPACE) != 0ULL) &&
		(b->pte_size == BALLOC_PTE_SIZE_BIG)) {
		nvgpu_list_add_tail(&b->buddy_entry, list);
	} else {
		nvgpu_list_add(&b->buddy_entry, list);
	}

	buddy_set_in_list(b);
}

static void balloc_buddy_list_do_rem(struct nvgpu_buddy_allocator *a,
				     struct nvgpu_buddy *b)
{
	if (!buddy_is_in_list(b)) {
		alloc_dbg(balloc_owner(a),
			  "Oops: removing removed buddy (%llu:0x%llx)",
			  b->order, b->start);
		nvgpu_do_assert();
	}

	nvgpu_list_del(&b->buddy_entry);
	buddy_clr_in_list(b);
}

/*
 * Add a buddy to one of the buddy lists and deal with the necessary
 * book keeping. Adds the buddy to the list specified by the buddy's order.
 */
static void balloc_blist_add(struct nvgpu_buddy_allocator *a,
			     struct nvgpu_buddy *b)
{
	balloc_buddy_list_do_add(a, b, balloc_get_order_list(a, b->order));
	a->buddy_list_len[b->order] =
		nvgpu_safe_add_u64(a->buddy_list_len[b->order], 1ULL);
}

static void balloc_blist_rem(struct nvgpu_buddy_allocator *a,
			     struct nvgpu_buddy *b)
{
	balloc_buddy_list_do_rem(a, b);
	nvgpu_assert(a->buddy_list_len[b->order] > 0ULL);
	a->buddy_list_len[b->order]--;
}

static u64 balloc_get_order(struct nvgpu_buddy_allocator *a, u64 len)
{
	if (len == 0U) {
		return 0;
	}

	len--;
	len >>= a->blk_shift;

	return nvgpu_fls(len);
}

static u64 balloc_max_order_in(struct nvgpu_buddy_allocator *a,
				 u64 start, u64 end)
{
	u64 size = nvgpu_safe_sub_u64(end, start) >> a->blk_shift;

	if (size > 0U) {
		return min_t(u64, nvgpu_ilog2(size), a->max_order);
	} else {
		return GPU_BALLOC_MAX_ORDER;
	}
}

/*
 * Initialize the buddy lists.
 */
static int balloc_init_lists(struct nvgpu_buddy_allocator *a)
{
	u32 i;
	u64 bstart, bend, order;
	struct nvgpu_buddy *buddy;

	bstart = a->start;
	bend = a->end;

	/* First make sure the LLs are valid. */
	for (i = 0U; i < GPU_BALLOC_ORDER_LIST_LEN; i++) {
		nvgpu_init_list_node(balloc_get_order_list(a, i));
	}

	while (bstart < bend) {
		order = balloc_max_order_in(a, bstart, bend);

		buddy = balloc_new_buddy(a, NULL, bstart, order);
		if (buddy == NULL) {
			goto cleanup;
		}

		balloc_blist_add(a, buddy);
		bstart = nvgpu_safe_add_u64(bstart,
					balloc_order_to_len(a, order));
	}

	return 0;

cleanup:
	for (i = 0U; i < GPU_BALLOC_ORDER_LIST_LEN; i++) {
		if (!nvgpu_list_empty(balloc_get_order_list(a, i))) {
			buddy = nvgpu_list_first_entry(
					balloc_get_order_list(a, i),
					nvgpu_buddy, buddy_entry);
			balloc_blist_rem(a, buddy);
			nvgpu_kmem_cache_free(a->buddy_cache, buddy);
		}
	}

	return -ENOMEM;
}

/*
 * Clean up and destroy the passed allocator.
 * Walk the allocator for any pending allocations.
 * Free up all pending allocations.
 * Free any memory allocated a allocator init time.
 * Destroy the lock and bzero the allocator completely.
 */
static void nvgpu_buddy_allocator_destroy(struct nvgpu_allocator *na)
{
	u32 i;
	struct nvgpu_rbtree_node *node = NULL;
	struct nvgpu_buddy *bud;
	struct nvgpu_fixed_alloc *falloc;
	struct nvgpu_buddy_allocator *a = buddy_allocator(na);

	alloc_lock(na);

	nvgpu_fini_alloc_debug(na);

	/*
	 * Free the fixed allocs first.
	 */
	nvgpu_rbtree_enum_start(0, &node, a->fixed_allocs);
	while (node != NULL) {
		falloc = nvgpu_fixed_alloc_from_rbtree_node(node);

		nvgpu_rbtree_unlink(node, &a->fixed_allocs);
		balloc_do_free_fixed(a, falloc);

		nvgpu_rbtree_enum_start(0, &node, a->fixed_allocs);
	}

	/*
	 * And now free all outstanding allocations.
	 */
	nvgpu_rbtree_enum_start(0, &node, a->alloced_buddies);
	while (node != NULL) {
		bud = nvgpu_buddy_from_rbtree_node(node);

		(void) balloc_free_buddy(a, bud->start);
		balloc_blist_add(a, bud);
		balloc_coalesce(a, bud);

		nvgpu_rbtree_enum_start(0, &node, a->alloced_buddies);
	}

	/*
	 * Now clean up the unallocated buddies.
	 */
	for (i = 0U; i < GPU_BALLOC_ORDER_LIST_LEN; i++) {
		BUG_ON(a->buddy_list_alloced[i] != 0U);

		while (!nvgpu_list_empty(balloc_get_order_list(a, i))) {
			bud = nvgpu_list_first_entry(
						balloc_get_order_list(a, i),
						nvgpu_buddy, buddy_entry);
			balloc_blist_rem(a, bud);
			nvgpu_kmem_cache_free(a->buddy_cache, bud);
		}

		if (a->buddy_list_len[i] != 0U) {
			nvgpu_info(na->g,
					"Excess buddies!!! (%d: %llu)",
				i, a->buddy_list_len[i]);
			nvgpu_do_assert();
		}
		if (a->buddy_list_split[i] != 0U) {
			nvgpu_info(na->g,
					"Excess split nodes!!! (%d: %llu)",
				i, a->buddy_list_split[i]);
			nvgpu_do_assert();
		}
		if (a->buddy_list_alloced[i] != 0U) {
			nvgpu_info(na->g,
					"Excess alloced nodes!!! (%d: %llu)",
				i, a->buddy_list_alloced[i]);
			nvgpu_do_assert();
		}
	}

	nvgpu_kmem_cache_destroy(a->buddy_cache);
	nvgpu_kfree(nvgpu_alloc_to_gpu(na), a);

	alloc_unlock(na);
}

/*
 * Combine the passed buddy if possible. The pointer in @b may not be valid
 * after this as the buddy may be freed.
 *
 * @a must be locked.
 */
static void balloc_coalesce(struct nvgpu_buddy_allocator *a,
			    struct nvgpu_buddy *b)
{
	struct nvgpu_buddy *parent;

	while (!buddy_is_alloced(b) && !buddy_is_split(b)) {
		/*
		 * If both our buddy and I are both not allocated and not split
		 *  then we can coalesce ourselves.
		 */
		if (b->buddy == NULL) {
			return;
		}
		if (buddy_is_alloced(b->buddy) || buddy_is_split(b->buddy)) {
			return;
		}

		parent = b->parent;

		balloc_blist_rem(a, b);
		balloc_blist_rem(a, b->buddy);

		buddy_clr_split(parent);
		nvgpu_assert(a->buddy_list_split[parent->order] > 0ULL);
		a->buddy_list_split[parent->order]--;
		balloc_blist_add(a, parent);

		/* Clean up the remains. */
		nvgpu_kmem_cache_free(a->buddy_cache, b->buddy);
		nvgpu_kmem_cache_free(a->buddy_cache, b);

		b = parent;
	}
}

/*
 * Split a buddy into two new buddies who are 1/2 the size of the parent buddy.
 *
 * @a must be locked.
 */
static int balloc_split_buddy(struct nvgpu_buddy_allocator *a,
			      struct nvgpu_buddy *b, u32 pte_size)
{
	struct nvgpu_buddy *left, *right;
	u64 half;
	u64 new_buddy_start;

	nvgpu_assert(b->order > 0ULL);
	left = balloc_new_buddy(a, b, b->start, b->order - 1U);
	if (left == NULL) {
		return -ENOMEM;
	}

	half = nvgpu_safe_sub_u64(b->end, b->start) / 2U;

	new_buddy_start = nvgpu_safe_add_u64(b->start, half);
	right = balloc_new_buddy(a, b, new_buddy_start, b->order - 1U);
	if (right == NULL) {
		nvgpu_kmem_cache_free(a->buddy_cache, left);
		return -ENOMEM;
	}

	buddy_set_split(b);
	a->buddy_list_split[b->order] =
		nvgpu_safe_add_u64(a->buddy_list_split[b->order], 1ULL);

	b->left = left;
	b->right = right;
	left->buddy = right;
	right->buddy = left;
	left->parent = b;
	right->parent = b;

	/*
	 * Potentially assign a PTE size to the new buddies. The obvious case is
	 * when we don't have a GPU VA space; just leave it alone. When we do
	 * have a GVA space we need to assign the passed PTE size to the buddy
	 * only if the buddy is less than the PDE block size. This is because if
	 * the buddy is less than the PDE block size then the buddy's  parent
	 * may already have a PTE size. Thus we can only allocate this buddy to
	 * mappings with that PTE size (due to the large/small PTE separation
	 * requirement).
	 *
	 * When the buddy size is greater than or equal to the block size then
	 * we can leave the buddies PTE field alone since the PDE block has yet
	 * to be assigned a PTE size.
	 */
	if (((a->flags & GPU_ALLOC_GVA_SPACE) != 0ULL) &&
		(left->order < a->pte_blk_order)) {
		left->pte_size = pte_size;
		right->pte_size = pte_size;
	}

	balloc_blist_rem(a, b);
	balloc_blist_add(a, left);
	balloc_blist_add(a, right);

	return 0;
}

/*
 * Place the passed buddy into the RB tree for allocated buddies. Never fails
 * unless the passed entry is a duplicate which is a bug.
 *
 * @a must be locked.
 */
static void balloc_alloc_buddy(struct nvgpu_buddy_allocator *a,
			       struct nvgpu_buddy *b)
{
	b->alloced_entry.key_start = b->start;
	b->alloced_entry.key_end = b->end;

	nvgpu_rbtree_insert(&b->alloced_entry, &a->alloced_buddies);

	buddy_set_alloced(b);
	a->buddy_list_alloced[b->order] =
		nvgpu_safe_add_u64(a->buddy_list_alloced[b->order], 1ULL);
}

/*
 * Remove the passed buddy from the allocated buddy RB tree. Returns the
 * deallocated buddy for further processing.
 *
 * @a must be locked.
 */
static struct nvgpu_buddy *balloc_free_buddy(struct nvgpu_buddy_allocator *a,
					     u64 addr)
{
	struct nvgpu_rbtree_node *node = NULL;
	struct nvgpu_buddy *bud;

	nvgpu_rbtree_search(addr, &node, a->alloced_buddies);
	if (node == NULL) {
		return NULL;
	}

	bud = nvgpu_buddy_from_rbtree_node(node);

	nvgpu_rbtree_unlink(node, &a->alloced_buddies);
	buddy_clr_alloced(bud);
	nvgpu_assert(a->buddy_list_alloced[bud->order] > 0ULL);
	a->buddy_list_alloced[bud->order]--;

	return bud;
}

/*
 * Find a suitable buddy for the given order and PTE type (big or little).
 */
static struct nvgpu_buddy *balloc_find_buddy(struct nvgpu_buddy_allocator *a,
					     u64 order, u32 pte_size)
{
	struct nvgpu_buddy *bud;

	if ((order > a->max_order) ||
	    nvgpu_list_empty(balloc_get_order_list(a, order))) {
		return NULL;
	}

	if (((a->flags & GPU_ALLOC_GVA_SPACE) != 0ULL) &&
		(pte_size == BALLOC_PTE_SIZE_BIG)) {
		bud = nvgpu_list_last_entry(balloc_get_order_list(a, order),
				      nvgpu_buddy, buddy_entry);
	} else {
		bud = nvgpu_list_first_entry(balloc_get_order_list(a, order),
				       nvgpu_buddy, buddy_entry);
	}

	if ((pte_size != BALLOC_PTE_SIZE_ANY) &&
		(pte_size != bud->pte_size) &&
		(bud->pte_size != BALLOC_PTE_SIZE_ANY)) {
		return NULL;
	}

	return bud;
}

/*
 * Allocate a suitably sized buddy. If no suitable buddy exists split higher
 * order buddies until we have a suitable buddy to allocate.
 *
 * For PDE grouping add an extra check to see if a buddy is suitable: that the
 * buddy exists in a PDE who's PTE size is reasonable
 *
 * @a must be locked.
 */
static u64 balloc_do_alloc(struct nvgpu_buddy_allocator *a,
			   u64 order, u32 pte_size)
{
	u64 split_order;
	struct nvgpu_buddy *bud = NULL;

	for (split_order = order; split_order <= a->max_order; split_order++) {
		bud = balloc_find_buddy(a, split_order, pte_size);
		if (bud != NULL) {
			break;
		}
	}

	/* Out of memory! */
	if (bud == NULL) {
		return 0;
	}

	while (bud->order != order) {
		if (balloc_split_buddy(a, bud, pte_size) != 0) {
			balloc_coalesce(a, bud);
			return 0; /* No mem... */
		}
		bud = bud->left;
	}

	balloc_blist_rem(a, bud);
	balloc_alloc_buddy(a, bud);

	return bud->start;
}

/*
 * See if the passed range is actually available for allocation. If so, then
 * return 1, otherwise return 0.
 *
 * TODO: Right now this uses the unoptimal approach of going through all
 * outstanding allocations and checking their base/ends. This could be better.
 */
static bool balloc_is_range_free(struct nvgpu_buddy_allocator *a,
				u64 base, u64 end)
{
	struct nvgpu_rbtree_node *node = NULL;
	struct nvgpu_buddy *bud;

	nvgpu_rbtree_enum_start(0, &node, a->alloced_buddies);
	if (node == NULL) {
		return true; /* No allocs yet. */
	}

	bud = nvgpu_buddy_from_rbtree_node(node);

	while (bud->start < end) {
		if (((bud->start > base) && (bud->start < end)) ||
			((bud->end > base) && (bud->end < end))) {
			return false;
		}

		nvgpu_rbtree_enum_next(&node, node);
		if (node == NULL) {
			break;
		}
		bud = nvgpu_buddy_from_rbtree_node(node);
	}

	return true;
}

static void balloc_alloc_fixed(struct nvgpu_buddy_allocator *a,
			       struct nvgpu_fixed_alloc *f)
{
	f->alloced_entry.key_start = f->start;
	f->alloced_entry.key_end = f->end;

	nvgpu_rbtree_insert(&f->alloced_entry, &a->fixed_allocs);
}

/*
 * Remove the passed buddy from the allocated buddy RB tree. Returns the
 * deallocated buddy for further processing.
 *
 * @a must be locked.
 */
static struct nvgpu_fixed_alloc *balloc_free_fixed(
	struct nvgpu_buddy_allocator *a, u64 addr)
{
	struct nvgpu_fixed_alloc *falloc;
	struct nvgpu_rbtree_node *node = NULL;

	nvgpu_rbtree_search(addr, &node, a->fixed_allocs);
	if (node == NULL) {
		return NULL;
	}

	falloc = nvgpu_fixed_alloc_from_rbtree_node(node);

	nvgpu_rbtree_unlink(node, &a->fixed_allocs);

	return falloc;
}

/*
 * Find the parent range - doesn't necessarily need the parent to actually exist
 * as a buddy. Finding an existing parent comes later...
 */
static void balloc_get_parent_range(struct nvgpu_buddy_allocator *a,
				    u64 base, u64 order,
				    u64 *pbase, u64 *porder)
{
	u64 base_mask;
	u64 shifted_base = balloc_base_shift(a, base);

	nvgpu_assert(order < 63U);
	order++;
	base_mask = ~nvgpu_safe_sub_u64((a->blk_size << order), 1U);

	shifted_base &= base_mask;

	*pbase = balloc_base_unshift(a, shifted_base);
	*porder = order;
}

static struct nvgpu_buddy *balloc_get_target_buddy(
				struct nvgpu_buddy_allocator *a,
				struct nvgpu_buddy *bud,
				u64 base, u64 order, u32 pte_size)
{
	/* Split this buddy as necessary until we get the target buddy. */
	while ((bud->start != base) || (bud->order != order)) {
		if (balloc_split_buddy(a, bud, pte_size) != 0) {
			alloc_dbg(balloc_owner(a),
				  "split buddy failed? {0x%llx, %llu}",
				  bud->start, bud->order);
			balloc_coalesce(a, bud);
			return NULL;
		}

		if (base < bud->right->start) {
			bud = bud->left;
		} else {
			bud = bud->right;
		}
	}

	return bud;
}

/*
 * Makes a buddy at the passed address. This will make all parent buddies
 * necessary for this buddy to exist as well.
 */
static struct nvgpu_buddy *balloc_make_fixed_buddy(
	struct nvgpu_buddy_allocator *a, u64 base, u64 order, u32 pte_size)
{
	struct nvgpu_buddy *bud = NULL;
	struct nvgpu_list_node *order_list;
	u64 cur_order = order, cur_base = base;

	/*
	 * Algo:
	 *  1. Keep jumping up a buddy order until we find the real buddy that
	 *     this buddy exists in.
	 *  2. Then work our way down through the buddy tree until we hit a dead
	 *     end.
	 *  3. Start splitting buddies until we split to the one we need to
	 *     make.
	 */
	while (cur_order <= a->max_order) {
		bool found = false;

		order_list = balloc_get_order_list(a, cur_order);
		nvgpu_list_for_each_entry(bud, order_list,
					nvgpu_buddy, buddy_entry) {
			if (bud->start == cur_base) {
				/*
				 * Make sure page size matches if it's smaller
				 * than a PDE sized buddy.
				 */
				if ((bud->order <= a->pte_blk_order) &&
					(bud->pte_size !=
						BALLOC_PTE_SIZE_ANY) &&
					(bud->pte_size != pte_size)) {
					/* Welp, that's the end of that. */
					alloc_dbg(balloc_owner(a),
						  "Fixed buddy PTE "
						  "size mismatch!");
					return NULL;
				}

				found = true;
				break;
			}
		}

		if (found) {
			break;
		}

		balloc_get_parent_range(a, cur_base, cur_order,
					&cur_base, &cur_order);
	}

	if (cur_order > a->max_order) {
		alloc_dbg(balloc_owner(a), "No buddy for range ???");
		return NULL;
	}

	/* Get target buddy */
	bud = balloc_get_target_buddy(a, bud, base, order, pte_size);

	return bud;
}

static u64 balloc_do_alloc_fixed(struct nvgpu_buddy_allocator *a,
				 struct nvgpu_fixed_alloc *falloc,
				 u64 base, u64 len, u32 pte_size)
{
	u64 shifted_base, inc_base;
	u64 align_order;

	/*
	 * Ensure that we have a valid PTE size here (ANY is a valid size). If
	 * this is INVALID then we are going to experience imminent corruption
	 * in the lists that hold buddies. This leads to some very strange
	 * crashes.
	 */
	BUG_ON(pte_size == BALLOC_PTE_SIZE_INVALID);

	shifted_base = balloc_base_shift(a, base);
	if (shifted_base == 0U) {
		align_order = nvgpu_safe_sub_u64(
				nvgpu_ffs(len >> a->blk_shift), 1UL);
	} else {
		u64 shifted_base_order =
			nvgpu_safe_sub_u64(
				nvgpu_ffs(shifted_base >> a->blk_shift), 1UL);
		u64 len_order =
			nvgpu_safe_sub_u64(
				nvgpu_ffs(len >> a->blk_shift), 1UL);
		align_order = min_t(u64, shifted_base_order, len_order);
	}

	if (align_order > a->max_order) {
		alloc_dbg(balloc_owner(a),
			  "Align order too big: %llu > %llu",
			  align_order, a->max_order);
		return 0;
	}

	/*
	 * Generate a list of buddies that satisfy this allocation.
	 */
	inc_base = shifted_base;
	while (inc_base < nvgpu_safe_add_u64(shifted_base, len)) {
		u64 order_len = balloc_order_to_len(a, align_order);
		u64 remaining;
		struct nvgpu_buddy *bud;

		bud = balloc_make_fixed_buddy(a,
					balloc_base_unshift(a, inc_base),
					align_order, pte_size);
		if (bud == NULL) {
			alloc_dbg(balloc_owner(a),
				  "Fixed buddy failed: {0x%llx, %llu}!",
				  balloc_base_unshift(a, inc_base),
				  align_order);
			goto err_and_cleanup;
		}

		balloc_blist_rem(a, bud);
		balloc_alloc_buddy(a, bud);
		balloc_buddy_list_do_add(a, bud, &falloc->buddies);

		/* Book keeping. */
		inc_base = nvgpu_safe_add_u64(inc_base, order_len);
		remaining = (shifted_base + len) - inc_base;
		align_order = nvgpu_safe_sub_u64(
				nvgpu_ffs(inc_base >> a->blk_shift), 1UL);

		/* If we don't have much left - trim down align_order. */
		if (balloc_order_to_len(a, align_order) > remaining) {
			align_order = balloc_max_order_in(a, inc_base,
						nvgpu_safe_add_u64(inc_base,
								remaining));
		}
	}

	return base;

err_and_cleanup:
	while (!nvgpu_list_empty(&falloc->buddies)) {
		struct nvgpu_buddy *bud = nvgpu_list_first_entry(
						&falloc->buddies,
						nvgpu_buddy, buddy_entry);

		balloc_buddy_list_do_rem(a, bud);
		(void) balloc_free_buddy(a, bud->start);
		balloc_blist_add(a, bud);
		/*
		 * Attemp to defrag the allocation.
		 */
		balloc_coalesce(a, bud);
	}

	return 0;
}

static void balloc_do_free_fixed(struct nvgpu_buddy_allocator *a,
				 struct nvgpu_fixed_alloc *falloc)
{
	struct nvgpu_buddy *bud;

	while (!nvgpu_list_empty(&falloc->buddies)) {
		bud = nvgpu_list_first_entry(&falloc->buddies,
				       nvgpu_buddy,
				       buddy_entry);
		balloc_buddy_list_do_rem(a, bud);

		(void) balloc_free_buddy(a, bud->start);
		balloc_blist_add(a, bud);
		a->bytes_freed = nvgpu_safe_add_u64(a->bytes_freed,
					balloc_order_to_len(a, bud->order));

		/*
		 * Attemp to defrag the allocation.
		 */
		balloc_coalesce(a, bud);
	}

	nvgpu_kfree(nvgpu_alloc_to_gpu(a->owner), falloc);
}

/*
 * Allocate memory from the passed allocator.
 * Acquire the allocator lock.
 * Compute the order by calling balloc_get_order().
 * Compute the pte size supported for this allocation by calling
 *  nvgpu_balloc_page_size_to_pte_size().
 * If we could not satisfy the required size buddy then call
 *  balloc_split_buddy() to get the requiredsize by dividing the large size buddy.
 * Free the remaining buddy to the respective list.
 * Release the alloc_lock.
 */
static u64 nvgpu_buddy_balloc_pte(struct nvgpu_allocator *na, u64 len,
				  u32 page_size)
{
	u64 order, addr;
	u32 pte_size;
	struct nvgpu_buddy_allocator *a = buddy_allocator(na);

	if (len == 0ULL) {
		alloc_dbg(balloc_owner(a), "Alloc fail");
		return 0;
	}

	alloc_lock(na);

	order = balloc_get_order(a, len);

	if (order > a->max_order) {
		alloc_unlock(na);
		alloc_dbg(balloc_owner(a), "Alloc fail");
		return 0;
	}

	pte_size = nvgpu_balloc_page_size_to_pte_size(a, page_size);
	if (pte_size == BALLOC_PTE_SIZE_INVALID) {
		alloc_unlock(na);
		return 0;
	}

	addr = balloc_do_alloc(a, order, pte_size);

	if (addr != 0ULL) {
		a->bytes_alloced += len;
		a->bytes_alloced_real += balloc_order_to_len(a, order);
		alloc_dbg(balloc_owner(a),
			"Alloc 0x%-10llx %3lld:0x%-10llx pte_size=%s",
			addr, order, len,
			(pte_size == BALLOC_PTE_SIZE_BIG) ? "big" :
			(pte_size == BALLOC_PTE_SIZE_SMALL) ? "small" :
			"NA/any");
		a->alloc_made = true;
	} else {
		alloc_dbg(balloc_owner(a), "Alloc failed: no mem!");
	}

	alloc_unlock(na);

	return addr;
}

static u64 nvgpu_buddy_balloc(struct nvgpu_allocator *na, u64 len)
{
	return nvgpu_buddy_balloc_pte(na, len, BALLOC_PTE_SIZE_ANY);
}

/*
 * Check the input parameter validity.
 * Acquire the alloc_lock.
 * Compute the order with respective to the input size.
 * Compute the pte_size for the given page size and return error for
 *  invalid pte size.
 * Call balloc_is_range_free() to get the free range with the address given.
 * Call balloc_make_fixed_buddy() to generate the list of buddies.
 * Make the book keeping of allocated objects to the respective lists.
 * Release the alloc_lock.
 */
static u64 nvgpu_balloc_fixed_buddy_locked(struct nvgpu_allocator *na,
					   u64 base, u64 len, u32 page_size)
{
	u32 pte_size;
	u64 ret, real_bytes = 0;
	struct nvgpu_buddy *bud;
	struct nvgpu_fixed_alloc *falloc = NULL;
	struct nvgpu_buddy_allocator *a = buddy_allocator(na);

	/* If base isn't aligned to an order 0 block, fail. */
	nvgpu_assert(a->blk_size > 0ULL);
	if ((base & (a->blk_size - 1ULL)) != 0ULL) {
		goto fail;
	}

	if (len == 0ULL) {
		goto fail;
	}

	if (base < a->start) {
		goto fail;
	}

	if (a->end < nvgpu_safe_add_u64(base, len)) {
		goto fail;
	}

	pte_size = nvgpu_balloc_page_size_to_pte_size(a, page_size);
	if (pte_size == BALLOC_PTE_SIZE_INVALID) {
		goto fail;
	}

	falloc = nvgpu_kmalloc(nvgpu_alloc_to_gpu(na), sizeof(*falloc));
	if (falloc == NULL) {
		goto fail;
	}

	nvgpu_init_list_node(&falloc->buddies);
	falloc->start = base;
	falloc->end = base + len;

	if (!balloc_is_range_free(a, base, base + len)) {
		alloc_dbg(balloc_owner(a),
			  "Range not free: 0x%llx -> 0x%llx",
			  base, base + len);
		goto fail;
	}

	ret = balloc_do_alloc_fixed(a, falloc, base, len, pte_size);
	if (ret == 0ULL) {
		alloc_dbg(balloc_owner(a),
			  "Alloc-fixed failed ?? 0x%llx -> 0x%llx",
			  base, base + len);
		goto fail;
	}

	balloc_alloc_fixed(a, falloc);

	nvgpu_list_for_each_entry(bud, &falloc->buddies,
				nvgpu_buddy, buddy_entry) {
		real_bytes = nvgpu_safe_add_u64(real_bytes,
				nvgpu_safe_sub_u64(bud->end, bud->start));
	}

	a->bytes_alloced = nvgpu_safe_add_u64(a->bytes_alloced, len);
	a->bytes_alloced_real = nvgpu_safe_add_u64(a->bytes_alloced_real,
						   real_bytes);

	alloc_dbg(balloc_owner(a), "Alloc (fixed) 0x%llx", base);

	return base;

fail:
	nvgpu_kfree(nvgpu_alloc_to_gpu(na), falloc);
	return 0;
}

/*
 * Allocate a fixed address allocation. The address of the allocation is @base
 * and the length is @len. This is not a typical buddy allocator operation and
 * as such has a high posibility of failure if the address space is heavily in
 * use.
 *
 * Please do not use this function unless _absolutely_ necessary.
 */
static u64 nvgpu_balloc_fixed_buddy(struct nvgpu_allocator *na,
				    u64 base, u64 len, u32 page_size)
{
	u64 alloc;
	struct nvgpu_buddy_allocator *a = buddy_allocator(na);

	alloc_lock(na);
	alloc = nvgpu_balloc_fixed_buddy_locked(na, base, len, page_size);

	if (alloc != 0ULL) {
		a->alloc_made = true;
	}

	alloc_unlock(na);

	return alloc;
}

/*
 * @a must be locked.
 */
static void nvgpu_buddy_bfree_locked(struct nvgpu_allocator *na, u64 addr)
{
	struct nvgpu_buddy *bud;
	struct nvgpu_fixed_alloc *falloc;
	struct nvgpu_buddy_allocator *a = buddy_allocator(na);

	/*
	 * First see if this is a fixed alloc. If not fall back to a regular
	 * buddy.
	 */
	falloc = balloc_free_fixed(a, addr);
	if (falloc != NULL) {
		balloc_do_free_fixed(a, falloc);
		goto done;
	}

	bud = balloc_free_buddy(a, addr);
	if (bud == NULL) {
		goto done;
	}

	balloc_blist_add(a, bud);
	a->bytes_freed = nvgpu_safe_add_u64(a->bytes_freed,
				balloc_order_to_len(a, bud->order));

	/*
	 * Attemp to defrag the allocation.
	 */
	balloc_coalesce(a, bud);

done:
	alloc_dbg(balloc_owner(a), "Free 0x%llx", addr);
}

/*
 * Free the passed allocation.
 */
static void nvgpu_buddy_bfree(struct nvgpu_allocator *na, u64 addr)
{
	if (addr == 0ULL) {
		return;
	}

	alloc_lock(na);
	nvgpu_buddy_bfree_locked(na, addr);
	alloc_unlock(na);
}

static bool nvgpu_buddy_reserve_is_possible(struct nvgpu_buddy_allocator *a,
					    struct nvgpu_alloc_carveout *co)
{
	struct nvgpu_alloc_carveout *tmp;
	u64 co_base, co_end;

	co_base = co->base;
	co_end  = nvgpu_safe_add_u64(co->base, co->length);

	/*
	 * Not the fastest approach but we should not have that many carveouts
	 * for any reasonable allocator.
	 */
	nvgpu_list_for_each_entry(tmp, &a->co_list,
				nvgpu_alloc_carveout, co_entry) {
		u64 tmp_end = nvgpu_safe_add_u64(tmp->base, tmp->length);

		if (((co_base >= tmp->base) && (co_base < tmp_end)) ||
			((co_end >= tmp->base) && (co_end < tmp_end))) {
			return false;
		}
	}

	return true;
}

/*
 * Carveouts can only be reserved before any regular allocations have been
 * made.
 * - Check the validity of input paramemters.
 * - Acquire the allocator lock.
 * - Call nvgpu_balloc_fixed_buddy_locked() to reserve the object
 *   with \a co.base and \a co.length.
 * - Add the allocated object to the book keeping list.
 * - Release the allocator lock.
 */
static int nvgpu_buddy_reserve_co(struct nvgpu_allocator *na,
				  struct nvgpu_alloc_carveout *co)
{
	struct nvgpu_buddy_allocator *a = buddy_allocator(na);
	u64 addr;
	int err = 0;

	if (co->base < a->start) {
		return -EINVAL;
	}
	if (nvgpu_safe_add_u64(co->base, co->length) > a->end) {
		return -EINVAL;
	}
	if (a->alloc_made) {
		return -EINVAL;
	}

	alloc_lock(na);

	if (!nvgpu_buddy_reserve_is_possible(a, co)) {
		err = -EBUSY;
		goto done;
	}

	/* Should not be possible to fail... */
	addr = nvgpu_balloc_fixed_buddy_locked(na, co->base, co->length,
					       BALLOC_PTE_SIZE_ANY);
	if (addr == 0ULL) {
		err = -ENOMEM;
		nvgpu_warn(na->g,
				"%s: Failed to reserve a valid carveout!",
				__func__);
		goto done;
	}

	nvgpu_list_add(&co->co_entry, &a->co_list);

done:
	alloc_unlock(na);
	return err;
}

/*
 * Carveouts can be release at any time.
 * - Acquire the allocator lock.
 * - Remove the carve out from the allocator list.
 * - Call nvgpu_buddy_bfree_locked() to free the carve out
 *   - nvgpu_buddy_bfree_locked() will first check the address is fixed
 *     or not by calling balloc_free_fixed().
 *   - If the address is fixed then free it by calling balloc_do_free_fixed().
 *   - Else free it through balloc_free_buddy().
 *   - Recompute the size of the allocator and coalesce the objects.
 * - Release the lock.
 */
static void nvgpu_buddy_release_co(struct nvgpu_allocator *na,
				   struct nvgpu_alloc_carveout *co)
{
	alloc_lock(na);

	nvgpu_list_del(&co->co_entry);
	nvgpu_buddy_bfree_locked(na, co->base);

	alloc_unlock(na);
}

static u64 nvgpu_buddy_alloc_length(struct nvgpu_allocator *a)
{
	struct nvgpu_buddy_allocator *ba = buddy_allocator(a);

	return ba->length;
}

static u64 nvgpu_buddy_alloc_base(struct nvgpu_allocator *a)
{
	struct nvgpu_buddy_allocator *ba = buddy_allocator(a);

	return ba->start;
}

static bool nvgpu_buddy_alloc_inited(struct nvgpu_allocator *a)
{
	struct nvgpu_buddy_allocator *ba = buddy_allocator(a);
	bool inited = ba->initialized;

	nvgpu_smp_rmb();
	return inited;
}

static u64 nvgpu_buddy_alloc_end(struct nvgpu_allocator *a)
{
	struct nvgpu_buddy_allocator *ba = buddy_allocator(a);

	return ba->end;
}
/*
 * - Acquire the allocator lock.
 * - Check the availability of space between start and end of
 *   the allocator.
 * - Release the allocator lock.
 */
static u64 nvgpu_buddy_alloc_space(struct nvgpu_allocator *a)
{
	struct nvgpu_buddy_allocator *ba = buddy_allocator(a);
	u64 space;

	alloc_lock(a);
	space = nvgpu_safe_sub_u64(ba->end, ba->start);
	space = nvgpu_safe_sub_u64(space,
			nvgpu_safe_sub_u64(ba->bytes_alloced_real,
					   ba->bytes_freed));
	alloc_unlock(a);

	return space;
}

#ifdef __KERNEL__
/*
 * Print the buddy allocator top level stats. If you pass @s as NULL then the
 * stats are printed to the kernel log. This lets this code be used for
 * debugging purposes internal to the allocator.
 */
static void nvgpu_buddy_print_stats(struct nvgpu_allocator *na,
				    struct seq_file *s, int lock)
{
	int i = 0;
	struct nvgpu_rbtree_node *node = NULL;
	struct nvgpu_fixed_alloc *falloc;
	struct nvgpu_alloc_carveout *tmp;
	struct nvgpu_buddy_allocator *a = buddy_allocator(na);

	alloc_pstat(s, na, "base = %llu, limit = %llu, blk_size = %llu",
		      a->base, a->length, a->blk_size);
	alloc_pstat(s, na, "Internal params:");
	alloc_pstat(s, na, "  start = 0x%llx", a->start);
	alloc_pstat(s, na, "  end   = 0x%llx", a->end);
	alloc_pstat(s, na, "  count = 0x%llx", a->count);
	alloc_pstat(s, na, "  blks  = 0x%llx", a->blks);
	alloc_pstat(s, na, "  max_order = %llu", a->max_order);

	if (lock)
		alloc_lock(na);

	if (!nvgpu_list_empty(&a->co_list)) {
		alloc_pstat(s, na, "");
		alloc_pstat(s, na, "Carveouts:");
		nvgpu_list_for_each_entry(tmp, &a->co_list,
					nvgpu_alloc_carveout, co_entry)
			alloc_pstat(s, na,
				      "  CO %2d: %-20s 0x%010llx + 0x%llx",
				      i++, tmp->name, tmp->base, tmp->length);
	}

	alloc_pstat(s, na, "");
	alloc_pstat(s, na, "Buddy blocks:");
	alloc_pstat(s, na, "  Order   Free    Alloced   Split");
	alloc_pstat(s, na, "  -----   ----    -------   -----");

	for (i = a->max_order; i >= 0; i--) {
		if (a->buddy_list_len[i] == 0 &&
		    a->buddy_list_alloced[i] == 0 &&
		    a->buddy_list_split[i] == 0)
			continue;

		alloc_pstat(s, na, "  %3d     %-7llu %-9llu %llu", i,
			      a->buddy_list_len[i],
			      a->buddy_list_alloced[i],
			      a->buddy_list_split[i]);
	}

	alloc_pstat(s, na, "");

	nvgpu_rbtree_enum_start(0, &node, a->fixed_allocs);
	i = 1;
	while (node) {
		falloc = nvgpu_fixed_alloc_from_rbtree_node(node);

		alloc_pstat(s, na, "Fixed alloc (%d): [0x%llx -> 0x%llx]",
			      i, falloc->start, falloc->end);

		nvgpu_rbtree_enum_next(&node, a->fixed_allocs);
	}

	alloc_pstat(s, na, "");
	alloc_pstat(s, na, "Bytes allocated:        %llu",
		      a->bytes_alloced);
	alloc_pstat(s, na, "Bytes allocated (real): %llu",
		      a->bytes_alloced_real);
	alloc_pstat(s, na, "Bytes freed:            %llu",
		      a->bytes_freed);

	if (lock)
		alloc_unlock(na);
}
#endif

static const struct nvgpu_allocator_ops buddy_ops = {
	.alloc		= nvgpu_buddy_balloc,
	.alloc_pte	= nvgpu_buddy_balloc_pte,
	.free_alloc	= nvgpu_buddy_bfree,

	.alloc_fixed	= nvgpu_balloc_fixed_buddy,
	/* .free_fixed not needed. */

	.reserve_carveout	= nvgpu_buddy_reserve_co,
	.release_carveout	= nvgpu_buddy_release_co,

	.base		= nvgpu_buddy_alloc_base,
	.length		= nvgpu_buddy_alloc_length,
	.end		= nvgpu_buddy_alloc_end,
	.inited		= nvgpu_buddy_alloc_inited,
	.space		= nvgpu_buddy_alloc_space,

	.fini		= nvgpu_buddy_allocator_destroy,

#ifdef __KERNEL__
	.print_stats	= nvgpu_buddy_print_stats,
#endif
};

static int nvgpu_buddy_check_argument_limits(struct vm_gk20a *vm, u64 size,
				u64 blk_size, u64 max_order, u64 flags)
{
	bool is_blk_size_pwr_2;
	bool is_gva_space = (flags & GPU_ALLOC_GVA_SPACE) != 0ULL;

	/* blk_size must be greater than 0 and a power of 2. */
	if (blk_size == 0U) {
		return -EINVAL;
	}

	is_blk_size_pwr_2 = (blk_size & (blk_size - 1ULL)) == 0ULL;
	if (!is_blk_size_pwr_2) {
		return -EINVAL;
	}

	if (max_order > GPU_BALLOC_MAX_ORDER) {
		return -EINVAL;
	}

	if (size == 0U) {
		return -EINVAL;
	}

	/* If this is to manage a GVA space we need a VM. */
	if (is_gva_space && (vm == NULL)) {
		return -EINVAL;
	}

	return 0;
}

static int nvgpu_buddy_set_attributes(struct nvgpu_buddy_allocator *a,
				struct nvgpu_allocator *na, struct vm_gk20a *vm,
				u64 base, u64 size, u64 blk_size, u64 max_order,
				u64 flags)
{
	bool is_gva_space = (flags & GPU_ALLOC_GVA_SPACE) != 0ULL;

	a->base = base;
	a->length = size;
	a->blk_size = blk_size;
	a->blk_shift = nvgpu_safe_sub_u64(nvgpu_ffs(blk_size), 1UL);
	a->owner = na;

	/*
	 * If base is 0 then modfy base to be the size of one block so that we
	 * can return errors by returning addr == 0.
	 */
	if (a->base == 0U) {
		a->base = a->blk_size;
		a->length = nvgpu_safe_sub_u64(a->length, a->blk_size);
	}

	a->vm = vm;
	if (is_gva_space) {
		u64 pde_size_mask, base_pde_align, size_pde_align;
		u64 pde_size = BIT64(nvgpu_vm_pde_coverage_bit_count(
				gk20a_from_vm(vm), vm->big_page_size));
		a->pte_blk_order = balloc_get_order(a, pde_size);

		/*
		 * When we have a GVA space with big_pages enabled the size and
		 * base must be PDE aligned. If big_pages are not enabled then
		 * this requirement is not necessary.
		 */

		pde_size_mask = nvgpu_safe_sub_u64(pde_size, U64(1));
		base_pde_align = a->base & pde_size_mask;
		size_pde_align = a->length & pde_size_mask;
		if (vm->big_pages &&
			((base_pde_align != 0ULL) ||
				(size_pde_align != 0ULL))) {
			return -EINVAL;
		}
	}

	a->flags = flags;
	a->max_order = max_order;
	return 0;
}

/*
 * Initialize a buddy allocator. Returns 0 on success. This allocator does
 * not necessarily manage bytes. It manages distinct ranges of resources. This
 * allows the allocator to work for things like comp_tags, semaphores, etc.
 *
 * @allocator: Ptr to an allocator struct to init.
 * @vm: GPU VM to associate this allocator with. Can be NULL. Will be used to
 *      get PTE size for GVA spaces.
 * @name: Name of the allocator. Doesn't have to be static storage.
 * @base: The base address of the resource pool being managed.
 * @size: Number of resources in the pool.
 * @blk_size: Minimum number of resources to allocate at once. For things like
 *            semaphores this is 1. For GVA this might be as much as 64k. This
 *            corresponds to order 0. Must be power of 2.
 * @max_order: Pick a maximum order. If you leave this as 0, the buddy allocator
 *             will try and pick a reasonable max order.
 * @flags: Extra flags necessary. See GPU_BALLOC_*.
 */
int nvgpu_buddy_allocator_init(struct gk20a *g, struct nvgpu_allocator *na,
			       struct vm_gk20a *vm, const char *name,
			       u64 base, u64 size, u64 blk_size,
			       u64 max_order, u64 flags)
{
	int err;
	bool is_gva_space = (flags & GPU_ALLOC_GVA_SPACE) != 0ULL;
	struct nvgpu_buddy_allocator *a;

	err = nvgpu_buddy_check_argument_limits(vm, size, blk_size, max_order,
							flags);
	if (err != 0) {
		return err;
	}

	a = nvgpu_kzalloc(g, sizeof(struct nvgpu_buddy_allocator));
	if (a == NULL) {
		return -ENOMEM;
	}

	err = nvgpu_alloc_common_init(na, g, name, a, false, &buddy_ops);
	if (err != 0) {
		goto fail;
	}

	err = nvgpu_buddy_set_attributes(a, na, vm, base, size, blk_size,
					max_order, flags);
	if (err != 0) {
		goto fail;
	}

	err = balloc_allocator_align(a);
	if (err != 0) {
		goto fail;
	}
	balloc_compute_max_order(a);

	a->buddy_cache = nvgpu_kmem_cache_create(g, sizeof(struct nvgpu_buddy));
	if (a->buddy_cache == NULL) {
		err = -ENOMEM;
		goto fail;
	}

	a->alloced_buddies = NULL;
	a->fixed_allocs = NULL;
	nvgpu_init_list_node(&a->co_list);
	err = balloc_init_lists(a);
	if (err != 0) {
		goto fail;
	}

	nvgpu_smp_wmb();
	a->initialized = true;

	nvgpu_init_alloc_debug(g, na);
	alloc_dbg(na, "New allocator: type      buddy");
	alloc_dbg(na, "               base      0x%llx", a->base);
	alloc_dbg(na, "               size      0x%llx", a->length);
	alloc_dbg(na, "               blk_size  0x%llx", a->blk_size);
	if (is_gva_space) {
		alloc_dbg(balloc_owner(a),
		       "               pde_size  0x%llx",
			  balloc_order_to_len(a, a->pte_blk_order));
	}
	alloc_dbg(na, "               max_order %llu", a->max_order);
	alloc_dbg(na, "               flags     0x%llx", a->flags);

	return 0;

fail:
	if (a->buddy_cache != NULL) {
		nvgpu_kmem_cache_destroy(a->buddy_cache);
	}
	nvgpu_kfree(g, a);
	return err;
}
