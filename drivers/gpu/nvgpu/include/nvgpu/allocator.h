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

#ifndef NVGPU_ALLOCATOR_H
#define NVGPU_ALLOCATOR_H

/**
 * @file
 *
 * Allocator interface.
 */

#ifdef __KERNEL__
/*
 * The Linux kernel has this notion of seq_files for printing info to userspace.
 * One of the allocator function pointers takes advantage of this and allows the
 * debug output to be directed either to nvgpu_log() or a seq_file.
 */
#include <linux/seq_file.h>
#endif

#include <nvgpu/log.h>
#include <nvgpu/lock.h>
#include <nvgpu/list.h>
#include <nvgpu/types.h>
#include <nvgpu/utils.h>

/*
 * Enable this flag to get finer control over allocator debug messaging
 * (see below).
 */
#if 0
#define ALLOCATOR_DEBUG_FINE
#endif

struct nvgpu_allocator;
struct nvgpu_alloc_carveout;
struct vm_gk20a;
struct gk20a;

#define NVGPU_ALLOC_NAME_LEN	32U

/**
 * Structure containing operations for an allocator to implement.
 */
struct nvgpu_allocator_ops {
	/**
	 * @brief Interface to allocate memory.
	 *
	 * @param[in] a			Pointer to nvgpu allocator.
	 * @param[in] len		Size of allocation.
	 *
	 * @return Address of allocation in case of success, 0 otherwise.
	 */
	u64  (*alloc)(struct nvgpu_allocator *allocator, u64 len);
	/**
	 * @brief Interface to allocate specific pte size memory.
	 *
	 * @param[in] allocator		Pointer to nvgpu allocator.
	 * @param[in] len		Size of allocation.
	 * @param[in] page_size		Page size of resource
	 *
	 * @return Address of allocation in case of success, 0 otherwise.
	 */
	u64  (*alloc_pte)(struct nvgpu_allocator *allocator, u64 len,
			  u32 page_size);
	/**
	 * @brief Interface to free given address allocation.
	 *
	 * @param[in] allocator		Pointer to nvgpu allocator.
	 * @param[in] addr		Base address of allocation.
	 */
	void (*free_alloc)(struct nvgpu_allocator *allocator, u64 addr);

	/**
	 * @brief Interface to allocate a memory region with specific starting
	 * address.
	 *
	 * @param[in] allocator		Pointer to nvgpu allocator.
	 * @param[in] base		Start address of resource.
	 * @param[in] len		Size of allocation.
	 * @param[in] page_size		Page size of resource.
	 *
	 * For allocators where \a page_size field is not applicable it can
	 * be left as 0. Otherwise a valid page size should be passed (4k or
	 * what the large page size is).
	 *
	 * @return Address of allocation in case of success, 0 otherwise.
	 */
	u64  (*alloc_fixed)(struct nvgpu_allocator *allocator,
			    u64 base, u64 len, u32 page_size);
	/**
	 * @brief Free fixed allocation with specific base.
	 *
	 * @param[in] allocator		Pointer to nvgpu allocator.
	 * @param[in] base		Start address of resource.
	 * @param[in] len		Size of allocation.
	 *
	 * Note: if free_alloc() works for freeing both regular and fixed
	 * allocations then free_fixed() does not need to be implemented. This
	 * behavior exists for legacy reasons and should not be propagated to
	 * new allocators.
	 */
	void (*free_fixed)(struct nvgpu_allocator *allocator,
			   u64 base, u64 len);

	/**
	 * @brief Interface to allocate memory carveout.
	 *
	 * @param[in] allocator		Pointer to nvgpu allocator.
	 * @param[in] co		Pointer to carveout structure.
	 *
	 * @return 0 in case of success, < 0 in case of failure.
	 */
	int  (*reserve_carveout)(struct nvgpu_allocator *allocator,
				 struct nvgpu_alloc_carveout *co);
	/**
	 * @brief Interface to release memory carveout.
	 *
	 * @param allocator		Pointer to nvgpu allocator.
	 * @param co			Pointer to carveout structure.
	 */
	void (*release_carveout)(struct nvgpu_allocator *allocator,
				 struct nvgpu_alloc_carveout *co);

	/**
	 * @brief Interface to read base address of allocator.
	 *
	 * @param[in] allocator		Pointer to nvgpu allocator.
	 *
	 * @return Allocator start address.
	 */
	u64  (*base)(struct nvgpu_allocator *allocator);

	/**
	 * @brief Interface to read length of allocator.
	 *
	 * @param[in] allocator		Pointer to nvgpu allocator.
	 *
	 * @return Allocator end address.
	 */
	u64  (*length)(struct nvgpu_allocator *allocator);

	/**
	 * @brief Interface to read end address of allocator.
	 *
	 * @param[in] allocator		Pointer to nvgpu allocator.
	 *
	 * @return Allocator end address.
	 */
	u64  (*end)(struct nvgpu_allocator *allocator);

	/**
	 * @brief Interface to check if allocator is initialized.
	 *
	 * @param allocator		Pointer to nvgpu allocator.
	 *
	 * @return True if allocator is initialized, false otherwise.
	 */
	bool (*inited)(struct nvgpu_allocator *allocator);

	/**
	 * @brief Interface to read available memory space in allocator.
	 *
	 * @param[in] allocator		Pointer to nvgpu allocator.
	 *
	 * @return Available allocator space.
	 */
	u64  (*space)(struct nvgpu_allocator *allocator);

	/**
	 * @brief Interface to destroy allocator.
	 *
	 * @param[in] allocator		Pointer to nvgpu allocator.
	 */
	void (*fini)(struct nvgpu_allocator *allocator);

#ifdef __KERNEL__
	/**
	 * @brief Interface to print allocator details for debugging.
	 *
	 * @param[in] allocator		Pointer to nvgpu allocator.
	 * @param[in] s			File pointer. If NULL, details are
	 *				printed to kernel log.
	 * @param[in] lock		Lock variable.
	 */
	void (*print_stats)(struct nvgpu_allocator *allocator,
			    struct seq_file *s, int lock);
#endif
};

/**
 * Basic structure to hold details of an allocator.
 */
struct nvgpu_allocator {
	/**
	 * Pointer to GPU structure.
	 */
	struct gk20a *g;
	/**
	 * Name of allocator.
	 */
	char name[NVGPU_ALLOC_NAME_LEN];
	/**
	 * Synchronization mutex.
	 */
	struct nvgpu_mutex lock;
	/**
	 * Generally used to store pointer to specific type of allocator.
	 */
	void *priv;
	/**
	 * Pointer to allocator operations.
	 */
	const struct nvgpu_allocator_ops *ops;
	/**
	 * Pointer to debugfs node.
	 */
	struct dentry *debugfs_entry;
	/**
	 * Control for debug msgs.
	 */
	bool debug;
};

/**
 * Basic structure to hold details of allocated carveout.
 */
struct nvgpu_alloc_carveout {
	/**
	 * Name of allocated carveout.
	 */
	const char *name;
	/**
	 * Base address of carveout.
	 */
	u64 base;
	/**
	 * Length of carveout.
	 */
	u64 length;
	/**
	 * Pointer to allocator structure.
	 */
	struct nvgpu_allocator *allocator;
	/**
	 * List node for usage by the allocator implementation.
	 */
	struct nvgpu_list_node co_entry;
};

/**
 * @brief Get address of carveout structure from given nvgpu list node.
 *
 * @param[in] node		Pointer to nvgpu list node.
 *
 * @return allocated carveout structure pointer.
 */
static inline struct nvgpu_alloc_carveout *
nvgpu_alloc_carveout_from_co_entry(struct nvgpu_list_node *node)
{
	return (struct nvgpu_alloc_carveout *)
	((uintptr_t)node - offsetof(struct nvgpu_alloc_carveout, co_entry));
};

/**
 * @brief Macro to set carveout attributes.
 *
 * @param[in] local_name	Name of carveout.
 * @param[in] local_base	Base address of carveout.
 * @param[in] local_length	Length of carveout.
 */
#define NVGPU_CARVEOUT(local_name, local_base, local_length)	\
	{							\
		.name = (local_name),				\
		.base = (local_base),				\
		.length = (local_length)			\
	}

/**
 * Possible GPU allocation flags.
 * @addtogroup GPU_ALLOC_FLAGS
 * @{
 */

/**
 *     This flag makes sense for the buddy allocator only. It specifies that the
 *     allocator will be used for managing a GVA space. When managing GVA spaces
 *     special care has to be taken to ensure that allocations of similar PTE
 *     sizes are placed in the same PDE block. This allows the higher level
 *     code to skip defining both small and large PTE tables for every PDE. That
 *     can save considerable memory for address spaces that have a lot of
 *     allocations.
 */
#define GPU_ALLOC_GVA_SPACE		BIT64(0)

/**
 *     For any allocator that needs to manage a resource in a latency critical
 *     path this flag specifies that the allocator should not use any kmalloc()
 *     or similar functions during normal operation. Initialization routines
 *     may still use kmalloc(). This prevents the possibility of long waits for
 *     pages when using alloc_page(). Currently only the bitmap allocator
 *     implements this functionality.
 *
 *     Also note that if you accept this flag then you must also define the
 *     free_fixed() function. Since no meta-data is allocated to help free
 *     allocations you need to keep track of the meta-data yourself (in this
 *     case the base and length of the allocation as opposed to just the base
 *     of the allocation).
 */
#define GPU_ALLOC_NO_ALLOC_PAGE		BIT64(1)

/**
 *     We manage vidmem pages at a large page granularity for performance
 *     reasons; however, this can lead to wasting memory. For page allocators
 *     setting this flag will tell the allocator to manage pools of 4K pages
 *     inside internally allocated large pages.
 *
 *     Currently this flag is ignored since the only usage of the page allocator
 *     uses a 4K block size already. However, this flag has been reserved since
 *     it will be necessary in the future.
 */
#define GPU_ALLOC_4K_VIDMEM_PAGES	BIT64(2)

/**
 *     Force allocations to be contiguous. Currently only relevant for page
 *     allocators since all other allocators are naturally contiguous.
 */
#define GPU_ALLOC_FORCE_CONTIG		BIT64(3)

/**
 *     The page allocator normally returns a scatter gather data structure for
 *     allocations (to handle discontiguous pages). However, at times that can
 *     be annoying so this flag forces the page allocator to return a u64
 *     pointing to the allocation base (requires GPU_ALLOC_FORCE_CONTIG to be
 *     set as well).
 */
#define GPU_ALLOC_NO_SCATTER_GATHER	BIT64(4)

/** @}*/

/** Enumerated type used to identify various allocator types */
enum nvgpu_allocator_type {
	BUDDY_ALLOCATOR = 0,
#ifdef CONFIG_NVGPU_DGPU
	PAGE_ALLOCATOR,
#endif
	BITMAP_ALLOCATOR
};

/**
 * @brief Acquire mutex associated with allocator.
 *
 * @param[in] a		Pointer to nvgpu allocator.
 */
static inline void alloc_lock(struct nvgpu_allocator *a)
{
	nvgpu_mutex_acquire(&a->lock);
}

/**
 * @brief Release mutex associated with allocator.
 *
 * @param[in] a		Pointer to nvgpu allocator.
 */
static inline void alloc_unlock(struct nvgpu_allocator *a)
{
	nvgpu_mutex_release(&a->lock);
}

/**
 * @brief Initialize buddy allocator.
 *
 * @param[in] g		Pointer to GPU structure.
 * @param[in] na	Pointer to allocator structure.
 * @param[in] vm	Pointer to virtual memory.
 * @param[in] name	Name of buddy allocator.
 * @param[in] base	Base address of buddy allocator.
 * @param[in] size	Size of buddy allocator.
 * @param[in] blk_size	Block size of buddy allocator.
 * @param[in] max_order	Maximum allowed buddy order.
 * @param[in] flags	Flags indicating buddy allocator conditions.
 *
 * @return 0 in case of success, < 0 otherwise.
 * @retval -EINVAL in case of incorrect input value.
 * @retval -ENOMEM in case there is not enough memory for allocation.
 */
int nvgpu_buddy_allocator_init(struct gk20a *g, struct nvgpu_allocator *na,
			       struct vm_gk20a *vm, const char *name,
			       u64 base, u64 size, u64 blk_size,
			       u64 max_order, u64 flags);

/**
 * @brief Initialize bitmap allocator.
 *
 * @param[in] g		Pointer to GPU structure.
 * @param[in] na	Pointer to allocator structure.
 * @param[in] name	Name of bitmap allocator.
 * @param[in] base	Base address of bitmap allocator.
 * @param[in] length	Size of bitmap allocator.
 * @param[in] blk_size	Block size of bitmap allocator.
 * @param[in] flags	Flags indicating bitmap allocator conditions.
 *
 * @return 0 in case of success, < 0 otherwise.
 * @retval -EINVAL in case of incorrect input value.
 * @retval -ENOMEM in case there is not enough memory for allocation.
 */
int nvgpu_bitmap_allocator_init(struct gk20a *g, struct nvgpu_allocator *na,
				const char *name, u64 base, u64 length,
				u64 blk_size, u64 flags);

#ifdef CONFIG_NVGPU_DGPU

/**
 * @brief Initialize page allocator.
 *
 * @param[in] g		Pointer to GPU structure.
 * @param[in] na	Pointer to allocator structure.
 * @param[in] name	Name of page allocator.
 * @param[in] base	Base address of page allocator.
 * @param[in] length	Size of page allocator.
 * @param[in] blk_size	Block size of page allocator.
 * @param[in] flags	Flags indicating page allocator conditions.
 *
 * @return 0 in case of success, < 0 otherwise.
 * @retval -EINVAL in case of incorrect input value.
 * @retval -ENOMEM in case there is not enough memory for allocation.
 */
int nvgpu_page_allocator_init(struct gk20a *g, struct nvgpu_allocator *na,
			      const char *name, u64 base, u64 length,
			      u64 blk_size, u64 flags);
#endif

/**
 * @brief Common init function for any type of allocator.
 *
 * @param[in] g			Pointer to GPU structure.
 * @param[in] na		Pointer to allocator structure.
 * @param[in] vm		Pointer to virtual memory. Can be NULL.
 *				Applicable to buddy allocator only.
 * @param[in] name		Name of allocator.
 * @param[in] base		Base address of allocator.
 * @param[in] length		Size of allocator.
 * @param[in] blk_size		Lowest size of resources that can be allocated.
 * @param[in] max_order		Max order of resource slices that can be
 *				allocated. Applicable to buddy allocator only.
 * @param[in] flags		Flags indicating additional conditions.
 * @param[in] alloc_type	Allocator type.

 * Returns 0 in case of success, < 0 otherwise.
 */
int nvgpu_allocator_init(struct gk20a *g, struct nvgpu_allocator *na,
			      struct vm_gk20a *vm, const char *name,
			      u64 base, u64 length, u64 blk_size, u64 max_order,
			      u64 flags, enum nvgpu_allocator_type alloc_type);

#ifdef CONFIG_NVGPU_FENCE
/**
 * @brief Initialize lockless allocator.
 *
 * @param[in] g		Pointer to GPU structure.
 * @param[in] na	Pointer to allocator structure.
 * @param[in] name	Name of lockless allocator.
 * @param[in] base	Base address of lockless allocator.
 * @param[in] length	Size of lockless allocator.
 * @param[in] blk_size	Block size of lockless allocator.
 * @param[in] flags	Flags indicating lockless allocator conditions.
 *
 * @return 0 in case of success, < 0 otherwise.
 * @retval -EINVAL in case of incorrect input value.
 * @retval -ENOMEM in case there is not enough memory for allocation.
 */
int nvgpu_lockless_allocator_init(struct gk20a *g, struct nvgpu_allocator *na,
				  const char *name, u64 base, u64 length,
				  u64 blk_size, u64 flags);
#endif

/**
 * Largest block of resources that fits in address space.
 */
#define GPU_BALLOC_MAX_ORDER		63U

/**
 * @brief Interface to allocate resources.
 *
 * @param[in] a		Pointer to nvgpu allocator.
 * @param[in] len	Size of allocation.
 *
 * @return Address of allocation in case of success, 0 otherwise.
 */
u64  nvgpu_alloc(struct nvgpu_allocator *a, u64 len);

/**
 * @brief Interface to allocate resources with specific pte.
 *
 * @param[in] a		Pointer to nvgpu allocator.
 * @param[in] len	Size of allocation.
 * @param[in] page_size	Page size of resource.
 *
 * @return Address of allocation in case of success, 0 otherwise.
 */
u64  nvgpu_alloc_pte(struct nvgpu_allocator *a, u64 len, u32 page_size);

/**
 * @brief Interface to free allocated resources.
 *
 * @param[in] a		Pointer to nvgpu allocator.
 * @param[in] addr	Base address of allocation.
 */
void nvgpu_free(struct nvgpu_allocator *a, u64 addr);

/**
 * @brief Interface to allocate resources with specific start address.
 *
 * @param[in] a		Pointer to nvgpu allocator.
 * @param[in] base	Start address of resource.
 * @param[in] len	Size of allocation.
 * @param[in] page_size	Page size of resource.
 *
 * @return Address of allocation in case of success, 0 otherwise.
 */
u64  nvgpu_alloc_fixed(struct nvgpu_allocator *a, u64 base, u64 len,
		       u32 page_size);

/**
 * @brief Interface to free resources at specific start address.
 *
 * @param[in] a		Pointer to nvgpu allocator.
 * @param[in] base	Start address of resource.
 * @param[in] len	Size of allocation.
 */
void nvgpu_free_fixed(struct nvgpu_allocator *a, u64 base, u64 len);

/**
 * @brief Interface to reserve carveout.
 *
 * @param[in] a		Pointer to nvgpu allocator.
 * @param[in] co	Pointer to carveout structure.
 *
 * @return 0 in case of success, < 0 in case of failure.
 */
int  nvgpu_alloc_reserve_carveout(struct nvgpu_allocator *a,
				  struct nvgpu_alloc_carveout *co);

/**
 * @brief Interface to release carveout.
 *
 * @param a		Pointer to nvgpu allocator.
 * @param co		Pointer to carveout structure.
 */
void nvgpu_alloc_release_carveout(struct nvgpu_allocator *a,
				  struct nvgpu_alloc_carveout *co);

/**
 * @brief Interface to read allocator base.
 *
 * @param[in] a		Pointer to nvgpu allocator.
 *
 * @return Allocator start address.
 */
u64  nvgpu_alloc_base(struct nvgpu_allocator *a);

/**
 * @brief Interface to read allocator length.
 *
 * @param a		Pointer to nvgpu allocator.
 *
 * @return Allocator length address.
 */
u64  nvgpu_alloc_length(struct nvgpu_allocator *a);

/**
 * @brief Interface to read allocator end address.
 *
 * @param[in] a		Pointer to nvgpu allocator.
 *
 * @return Allocator end address.
 */
u64  nvgpu_alloc_end(struct nvgpu_allocator *a);

/**
 * @brief Interface to check if allocator is initialized.
 *
 * @param a		Pointer to nvgpu allocator.
 *
 * @return True if allocator is initialized, false otherwise.
 */
bool nvgpu_alloc_initialized(struct nvgpu_allocator *a);

/**
 * @brief Interface to get allocator space.
 *
 * @param[in] a		Pointer to nvgpu allocator.
 *
 * @return Available allocator space.
 */
u64  nvgpu_alloc_space(struct nvgpu_allocator *a);

/**
 * @brief Interface to destroy allocator.
 *
 * @param[in] a		Pointer to nvgpu allocator.
 */
void nvgpu_alloc_destroy(struct nvgpu_allocator *a);

#ifdef __KERNEL__
/**
 * @brief Interface to print allocator details.
 *
 * @param[in] a		Pointer to nvgpu allocator.
 * @param[in] s		File pointer.
 *			If NULL, details are printed to kernel log.
 * @param[in] lock	Lock variable.
 */
void nvgpu_alloc_print_stats(struct nvgpu_allocator *a,
			     struct seq_file *s, int lock);
#endif

/**
 * @brief Get GPU pointer from allocator pointer.
 *
 * @param[in] a		Pointer to nvgpu allocator.
 *
 * @return GPU pointer.
 */
static inline struct gk20a *nvgpu_alloc_to_gpu(struct nvgpu_allocator *a)
{
	return a->g;
}

#ifdef CONFIG_DEBUG_FS
/*
 * Common functionality for the internals of the allocators.
 */
/**
 * @brief Initialize debug allocator file.
 *
 * @param[in] g		Pointer to GPU structure.
 * @param[in] a		Pointer to nvgpu allocator.
 */
void nvgpu_init_alloc_debug(struct gk20a *g, struct nvgpu_allocator *a);

/**
 * @brief Destroy debug allocator file.
 *
 * @param[in] a		Pointer to nvgpu allocator.
 */
void nvgpu_fini_alloc_debug(struct nvgpu_allocator *a);
#endif

/**
 * @brief Initialize nvgpu allocator.
 *
 * @param[in] a		Pointer to nvgpu allocator.
 * @param[in] g		Pointer to GPU structure.
 * @param[in] name	Name of allocator.
 * @param[in] priv	Pointer to private data.
 * @param[in] dbg	Debug flag.
 * @param[in] ops	Pointer to allocator operations.
 *
 * @return 0 in case of success, < 0 in case of failure.
 */
int  nvgpu_alloc_common_init(struct nvgpu_allocator *a, struct gk20a *g,
			     const char *name, void *priv, bool dbg,
			     const struct nvgpu_allocator_ops *ops);

/*
 * Debug stuff.
 */
#ifdef __KERNEL__
#define alloc_pstat(seq, allocator, fmt, arg...)		\
	do {							\
		if (seq)					\
			seq_printf(seq, fmt "\n", ##arg);	\
		else						\
			alloc_dbg(allocator, fmt, ##arg);	\
	} while (false)
#endif

#define do_alloc_dbg(a, fmt, arg...)				\
	nvgpu_log((a)->g, gpu_dbg_alloc, "%25s " fmt, (a)->name, ##arg)

/*
 * This gives finer control over debugging messages. By defining the
 * ALLOCATOR_DEBUG_FINE macro prints for an allocator will only get made if
 * that allocator's debug flag is set.
 *
 * Otherwise debugging is as normal: debug statements for all allocators
 * if the GPU debugging mask bit is set. Note: even when ALLOCATOR_DEBUG_FINE
 * is set gpu_dbg_alloc must still also be set to true.
 */
#if defined(ALLOCATOR_DEBUG_FINE)
#define alloc_dbg(a, fmt, arg...)				\
	do {							\
		if ((a)->debug)					\
			do_alloc_dbg((a), fmt, ##arg);		\
	} while (false)
#else
#define alloc_dbg(a, fmt, arg...) do_alloc_dbg(a, fmt, ##arg)
#endif

#endif /* NVGPU_ALLOCATOR_H */
