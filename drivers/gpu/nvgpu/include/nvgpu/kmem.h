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

#ifndef NVGPU_KMEM_H
#define NVGPU_KMEM_H

/**
 * @file
 *
 * Kmem cache support
 * ------------------
 *
 * In Linux there is support for the notion of a kmem_cache. It gives better
 * memory usage characteristics for lots of allocations of the same size. Think
 * structs that get allocated over and over. Normal kmalloc() type routines
 * typically round to the next power-of-2 since that's easy.
 *
 * But if we know the size ahead of time the packing for the allocations can be
 * much better. This is the benefit of a slab allocator. This type hides the
 * underlying kmem_cache (or absense thereof).
 */

#include <nvgpu/types.h>
#include <nvgpu/utils.h>

struct gk20a;

/**
 * When there's other implementations make sure they are included instead of
 * Linux (i.e Qnx) when not compiling on Linux!
 */
#ifdef __KERNEL__
#include <nvgpu/linux/kmem.h>
#else
#include <nvgpu/posix/kmem.h>
#endif

struct nvgpu_kmem_cache;

#ifdef CONFIG_NVGPU_TRACK_MEM_USAGE
/**
 * Uncomment this if you want to enable stack traces in the memory profiling.
 * Since this is a fairly high overhead operation and is only necessary for
 * debugging actual bugs it's left here for developers to enable.
 */

#if 0
#define NVGPU_SAVE_KALLOC_STACK_TRACES
#endif

/**
 * Defined per-OS.
 */
struct nvgpu_mem_alloc_tracker;
#endif


/**
 * @brief Create an nvgpu memory cache.
 *
 * The internal implementation of the function is OS specific. In Posix
 * implementation, the function would just allocate a normal malloc memory for
 * the cache structure and stores the value of \a size inside the structure.
 * This cache can be used to allocate objects of size \a size. Common usage
 * would be for a struct that gets allocated a lot. In that case \a size
 * should be sizeof(struct my_struct). A given implementation of this need not
 * do anything special. The allocation routines can simply be passed on to
 * nvgpu_kzalloc() if desired, so packing and alignment of the structs cannot
 * be assumed. In Posix implementation, the allocation from this cache would
 * just return a normal malloc memory of size \a size.
 *
 * @param g [in] The GPU driver struct using this cache.
 * @param size [in] Size of the object allocated by the cache.
 *
 * @return Pointer to #nvgpu_kmem_cache in case of success, else NULL.
 *
 * @retval NULL in case of failure.
 */
struct nvgpu_kmem_cache *nvgpu_kmem_cache_create(struct gk20a *g, size_t size);

/**
 * @brief Destroy a cache created by #nvgpu_kmem_cache_create().
 *
 * Destroy the allocated OS specific internal structure to avoid the memleak.
 *
 * @param cache [in] The cache to destroy.
 */
void nvgpu_kmem_cache_destroy(struct nvgpu_kmem_cache *cache);

/**
 * @brief Allocate an object from the cache
 *
 * Allocate an object from a cache created using #nvgpu_kmem_cache_create().
 * In Posix implementation, this function would just return a normal malloc
 * memory.
 *
 * @param cache [in] The cache to alloc from.
 *
 * @return Pointer to an object in case of success, else NULL.
 *
 * @retval NULL in case of failure.
 */
void *nvgpu_kmem_cache_alloc(struct nvgpu_kmem_cache *cache);

/**
 * @brief Free an object back to a cache
 *
 * Free an object back to a cache allocated using #nvgpu_kmem_cache_alloc().
 *
 * @param cache [in] The cache to return the object to.
 * @param ptr [in] Pointer to the object to free.
 */
void nvgpu_kmem_cache_free(struct nvgpu_kmem_cache *cache, void *ptr);

/**
 * @brief Allocate from the OS allocator.
 *
 * Allocate a chunk of system memory from the process address space.
 * This function may sleep so cannot be used in IRQs.
 *
 * @param g [in] Current GPU.
 * @param size [in] Size of the allocation.
 *
 * @return Pointer to an object in case of success, else NULL.
 *
 * @retval NULL in case of failure.
 */
#define nvgpu_kmalloc(g, size)	nvgpu_kmalloc_impl(g, size, NVGPU_GET_IP)

/**
 * @brief Allocate from the OS allocator.
 *
 * Identical to nvgpu_kalloc() except the memory will be zeroed before being
 * returned.
 *
 * @param g [in] Current GPU.
 * @param size [in] Size of the allocation.
 *
 * @return Pointer to an object in case of success, else NULL.
 *
 * @retval NULL in case of failure.
 */
#define nvgpu_kzalloc(g, size)	nvgpu_kzalloc_impl(g, size, NVGPU_GET_IP)

/**
 * @brief Allocate from the OS allocator.
 *
 * Identical to nvgpu_kalloc() except the size of the memory chunk returned is
 * \a n * \a size.
 *
 * @param g [in] Current GPU.
 * @param n [in] Number of objects.
 * @param size [in] Size of each object.
 *
 * @return Pointer to an object in case of success, else NULL.
 *
 * @retval NULL in case of failure.
 */
#define nvgpu_kcalloc(g, n, size)	\
	nvgpu_kcalloc_impl(g, n, size, NVGPU_GET_IP)

/**
 * @brief Allocate memory and return a map to it.
 *
 * Allocate some memory and return a pointer to a virtual memory mapping of
 * that memory (using malloc for Qnx). The underlying physical
 * memory is not guaranteed to be contiguous (and indeed likely isn't). This
 * allows for much larger allocations to be done without worrying about as much
 * about physical memory fragmentation. This function may sleep.
 *
 * @param g [in] Current GPU.
 * @param size [in] Size of the allocation.
 *
 * @return Pointer to an object in case of success, else NULL.
 *
 * @retval NULL in case of failure.
 */
#define nvgpu_vmalloc(g, size)	nvgpu_vmalloc_impl(g, size, NVGPU_GET_IP)

/**
 * @brief Allocate memory and return a map to it.
 *
 * Identical to nvgpu_vmalloc() except this will return zero'ed memory.
 *
 * @param g [in] Current GPU.
 * @param size [in] Size of the allocation.
 *
 * @return Pointer to an object in case of success, else NULL.
 *
 * @retval NULL in case of failure.
 */
#define nvgpu_vzalloc(g, size)	nvgpu_vzalloc_impl(g, size, NVGPU_GET_IP)

/**
 * @brief Frees an alloc from nvgpu_kmalloc, nvgpu_kzalloc, nvgpu_kcalloc.
 *
 * @param g [in] Current GPU.
 * @param addr [in] Address of object to free.
 */
#define nvgpu_kfree(g, addr)	nvgpu_kfree_impl(g, addr)

/**
 * @brief Frees an alloc from nvgpu_vmalloc, nvgpu_vzalloc.
 *
 * @param g [in] Current GPU.
 * @param addr [in] Address of object to free.
 */
#define nvgpu_vfree(g, addr)	nvgpu_vfree_impl(g, addr)

#define kmem_dbg(g, fmt, args...)		\
	nvgpu_log(g, gpu_dbg_kmem, fmt, ##args)

/**
 * @brief Initialize the kmem tracking stuff.
 *
 * Initialize the kmem tracking internal structure. Internal implementation
 * is specific to OS.
 *
 * @param g [in] The driver to init.
 *
 * @return 0 in case of success, non-zero in case of failure. Posix
 * implementation is a dummy function which just returns 0.
 *
 * @retval 0 for posix implementation.
 */
int nvgpu_kmem_init(struct gk20a *g);

/**
 * @brief Finalize the kmem tracking code
 *
 * @param g [in] The GPU.
 * @param flags [in] Flags that control operation of this finalization.
 *
 * Cleanup resources used by nvgpu_kmem. Available flags for cleanup are:
 *
 *   - NVGPU_KMEM_FINI_DO_NOTHING
 *   - NVGPU_KMEM_FINI_FORCE_CLEANUP
 *   - NVGPU_KMEM_FINI_DUMP_ALLOCS
 *   - NVGPU_KMEM_FINI_WARN
 *   - NVGPU_KMEM_FINI_BUG
 *
 * NVGPU_KMEM_FINI_DO_NOTHING will be overridden by anything else specified.
 * Put another way don't just add NVGPU_KMEM_FINI_DO_NOTHING and expect that
 * to suppress other flags from doing anything.
 *
 * Internal implementation is specific to OS.
 */
void nvgpu_kmem_fini(struct gk20a *g, int flags);

/**
 * These will simply be ignored if CONFIG_NVGPU_TRACK_MEM_USAGE is not defined.
 */
#define NVGPU_KMEM_FINI_DO_NOTHING		0
#define NVGPU_KMEM_FINI_FORCE_CLEANUP		(1 << 0)
#define NVGPU_KMEM_FINI_DUMP_ALLOCS		(1 << 1)
#define NVGPU_KMEM_FINI_WARN			(1 << 2)
#define NVGPU_KMEM_FINI_BUG			(1 << 3)

/**
 * @brief Wrapper for memory allocation functions.
 *
 * The internal implementation of this function is OS specific. For Posix
 * implementation, the requested \a size of memory is allocated and returns
 * a pointer to that memory. The parameter \a clear is used to decide if the
 * allocated memory has to be zero filled or not. Based on \a clear value,
 * calloc or malloc is used internally in Posix implementation.
 *
 * @param g [in] The GPU.
 * @param size [in] Size of the allocation.
 * @param clear [in] Flag indicates the need of zeroed memory.
 *
 * @return upon successful allocation a pointer to a virtual address range
 * that the nvgpu can access is returned, else NULL.
 *
 * @retval NULL in case of failure.
 */
void *nvgpu_big_alloc_impl(struct gk20a *g, size_t size, bool clear);

/**
 * @brief Pick virtual or physical alloc based on @size
 *
 * It is wrapper around nvgpu_big_alloc_impl()
 *
 * @param g [in] The GPU.
 * @param size [in] Size of the allocation.
 *
 * @return upon successful allocation a pointer to a virtual address range
 * that the nvgpu can access is returned, else NULL.
 *
 * @retval NULL in case of failure.
 */
static inline void *nvgpu_big_malloc(struct gk20a *g, size_t size)
{
	return nvgpu_big_alloc_impl(g, size, false);
}

/**
 * @brief Pick virtual or physical alloc based on @size
 *
 * @param g [in] The GPU.
 * @param size [in] Size of the allocation.
 *
 * Zeroed memory version of nvgpu_big_malloc().
 *
 * @return upon successful allocation a pointer to a virtual address range
 * that the nvgpu can access is returned, else NULL.
 *
 * @retval NULL in case of failure.
 */
static inline void *nvgpu_big_zalloc(struct gk20a *g, size_t size)
{
	return nvgpu_big_alloc_impl(g, size, true);
}

/**
 * @brief Free any alloc from nvgpu_big_zalloc() or nvgpu_big_malloc().
 *
 * @param g [in] The GPU.
 * @param p [in] A pointer allocated by nvgpu_big_zalloc() or nvgpu_big_malloc().
 */
void nvgpu_big_free(struct gk20a *g, void *p);

#endif /* NVGPU_KMEM_H */
