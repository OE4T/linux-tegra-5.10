// SPDX-License-Identifier: GPL-2.0
/*
 * Coherent per-device memory handling.
 * Borrowed from i386
 *
 * Copyright (c) 2020, NVIDIA CORPORATION. All rights reserved.
 */
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/dma-direct.h>
#include <linux/dma-map-ops.h>
#include <linux/debugfs.h>
#include <linux/kthread.h>

#ifdef CONFIG_ARM_DMA_IOMMU_ALIGNMENT
#define DMA_BUF_ALIGNMENT CONFIG_ARM_DMA_IOMMU_ALIGNMENT
#else
#define DMA_BUF_ALIGNMENT 8
#endif

#define RESIZE_DEFAULT_SHRINK_AGE 3

#ifdef pr_fmt
#undef pr_fmt
#endif

#define pr_fmt(fmt) "%s:%d: " fmt, __func__, __LINE__

#define RESIZE_MAGIC 0xC11A900d

struct timer_data {
	/* The timer used to wakeup the shrink thread */
	struct timer_list shrink_timer;
	/* Pointer to the current shrink thread for this resizable heap */
	struct task_struct *task;
};
struct heap_info {
	int magic;
	char *name;
	/* number of chunks memory to manage in */
	unsigned int num_chunks;
	/* dev to manage cma/coherent memory allocs, if resize allowed */
	struct device dev;
	/* device to allocate memory from cma */
	struct device *cma_dev;
	/* lock to synchronise heap resizing */
	struct mutex resize_lock;
	/* CMA chunk size if resize supported */
	size_t cma_chunk_size;
	/* heap current base */
	phys_addr_t curr_base;
	/* heap current length */
	/* heap current allocated memory in bytes */
	size_t curr_used;
	size_t curr_len;
	/* heap lowest base */
	phys_addr_t cma_base;
	/* heap max length */
	size_t cma_len;
	size_t rem_chunk_size;
	struct dentry *dma_debug_root;
	int (*update_resize_cfg)(phys_addr_t, size_t);
	struct timer_data shrink_timer_data;
	unsigned long shrink_interval;
	size_t floor_size;
};

struct dma_coherent_mem {
	void		*virt_base;
	dma_addr_t	device_base;
	unsigned long	pfn_base;
	int		size;
	int 		flags;
	unsigned long	*bitmap;
	spinlock_t	spinlock;
	bool		use_dev_dma_pfn_offset;
};

static struct dma_coherent_mem *dma_coherent_default_memory __ro_after_init;

int dma_alloc_from_dev_coherent_attr_at(struct device *dev, ssize_t size,
		dma_addr_t *dma_handle, void **ret, unsigned long attrs,
		ulong start);

static inline struct dma_coherent_mem *dev_get_coherent_memory(struct device *dev)
{
	if (dev && dev->dma_mem)
		return dev->dma_mem;
	return NULL;
}

static inline dma_addr_t dma_get_device_base(struct device *dev,
					     struct dma_coherent_mem * mem)
{
	if (mem->use_dev_dma_pfn_offset)
		return phys_to_dma(dev, PFN_PHYS(mem->pfn_base));
	return mem->device_base;
}

bool dma_is_coherent_dev(struct device *dev)
{
	struct heap_info *h;

	if (!dev)
		return false;

	h = dev_get_drvdata(dev);
	if (!h)
		return false;

	if (h->magic != RESIZE_MAGIC)
		return false;

	return true;
}
EXPORT_SYMBOL(dma_is_coherent_dev);

static void dma_debugfs_init(struct device *dev, struct heap_info *heap)
{
	if (!heap->dma_debug_root) {
		heap->dma_debug_root = debugfs_create_dir(dev_name(dev), NULL);
		if (IS_ERR_OR_NULL(heap->dma_debug_root)) {
			dev_err(dev, "couldn't create debug files\n");
			return;
		}
	}

	if (sizeof(phys_addr_t) == sizeof(u64)) {
		debugfs_create_x64("curr_base", S_IRUGO,
			heap->dma_debug_root, (u64 *)&heap->curr_base);
		debugfs_create_x64("curr_size", S_IRUGO,
			heap->dma_debug_root, (u64 *)&heap->curr_len);

		debugfs_create_x64("cma_base", S_IRUGO,
			heap->dma_debug_root, (u64 *)&heap->cma_base);
		debugfs_create_x64("cma_size", S_IRUGO,
			heap->dma_debug_root, (u64 *)&heap->cma_len);
		debugfs_create_x64("cma_chunk_size", S_IRUGO,
			heap->dma_debug_root, (u64 *)&heap->cma_chunk_size);

		debugfs_create_x64("floor_size", S_IRUGO,
			heap->dma_debug_root, (u64 *)&heap->floor_size);

	} else {
		debugfs_create_x32("curr_base", S_IRUGO,
			heap->dma_debug_root, (u32 *)&heap->curr_base);
		debugfs_create_x32("curr_size", S_IRUGO,
			heap->dma_debug_root, (u32 *)&heap->curr_len);

		debugfs_create_x32("cma_base", S_IRUGO,
			heap->dma_debug_root, (u32 *)&heap->cma_base);
		debugfs_create_x32("cma_size", S_IRUGO,
			heap->dma_debug_root, (u32 *)&heap->cma_len);
		debugfs_create_x32("cma_chunk_size", S_IRUGO,
			heap->dma_debug_root, (u32 *)&heap->cma_chunk_size);

		debugfs_create_x32("floor_size", S_IRUGO,
			heap->dma_debug_root, (u32 *)&heap->floor_size);
	}
	debugfs_create_x32("num_cma_chunks", S_IRUGO,
		heap->dma_debug_root, (u32 *)&heap->num_chunks);
}

static phys_addr_t alloc_from_contiguous_heap(
				struct heap_info *h,
				phys_addr_t base, size_t len)
{
	size_t count;
	struct page *page;
	unsigned long order;

	dev_dbg(h->cma_dev, "req at base (%pa) size (0x%zx)\n",
		&base, len);

	order = get_order(len);
	count = PAGE_ALIGN(len) >> PAGE_SHIFT;
	page = dma_alloc_at_from_contiguous(h->cma_dev, count, order,
						GFP_KERNEL, base, true);
	if (!page) {
		dev_err(h->cma_dev, "dma_alloc_at_from_contiguous failed\n");
		goto dma_alloc_err;
	}
	base = page_to_phys(page);
	dev_dbg(h->cma_dev, "allocated at base (%pa) size (0x%zx)\n",
		&base, len);
	BUG_ON(base < h->cma_base ||
		base - h->cma_base + len > h->cma_len);
	return base;

dma_alloc_err:
	return DMA_ERROR_CODE;
}

static void release_from_contiguous_heap(
				struct heap_info *h,
				phys_addr_t base, size_t len)
{
	struct page *page = phys_to_page(base);
	size_t count = PAGE_ALIGN(len) >> PAGE_SHIFT;

	dma_release_from_contiguous(h->cma_dev, page, count);
	dev_dbg(h->cma_dev, "released at base (%pa) size (0x%zx)\n",
		&base, len);
}

static void update_alloc_range(struct heap_info *h)
{
	if (!h->curr_len)
		h->dev.dma_mem->size = 0;
	else
		h->dev.dma_mem->size = (h->curr_base - h->cma_base +
					h->curr_len) >> PAGE_SHIFT;
}

static int update_vpr_config(struct heap_info *h)
{
	/* Handle VPR configuration updates*/
	if (h->update_resize_cfg) {
		int err = h->update_resize_cfg(h->curr_base, h->curr_len);
		if (err) {
			dev_err(&h->dev, "Failed to update heap resize\n");
			return err;
		}
		dev_dbg(&h->dev, "update vpr base to %pa, size=%zx\n",
			&h->curr_base, h->curr_len);
	}

	update_alloc_range(h);
	return 0;
}

static void get_first_and_last_idx(struct heap_info *h,
				   int *first_alloc_idx, int *last_alloc_idx)
{
	if (!h->curr_len) {
		*first_alloc_idx = -1;
		*last_alloc_idx = h->num_chunks;
	} else {
		*first_alloc_idx = div_u64(h->curr_base - h->cma_base,
					   h->cma_chunk_size);
		*last_alloc_idx = div_u64(h->curr_base - h->cma_base +
					  h->curr_len + h->cma_chunk_size -
					  h->rem_chunk_size,
					  h->cma_chunk_size) - 1;
 	}
}

static int heap_resize_locked(struct heap_info *h, bool skip_vpr_config)
{
	phys_addr_t base = -1;
	size_t len = h->cma_chunk_size;
	phys_addr_t prev_base = h->curr_base;
	size_t prev_len = h->curr_len;
	int alloc_at_idx = 0;
	int first_alloc_idx;
	int last_alloc_idx;
	phys_addr_t start_addr = h->cma_base;

	get_first_and_last_idx(h, &first_alloc_idx, &last_alloc_idx);
	pr_debug("req resize, fi=%d,li=%d\n", first_alloc_idx, last_alloc_idx);

	/* All chunks are in use. Can't grow it. */
	if (first_alloc_idx == 0 && last_alloc_idx == h->num_chunks - 1)
		return -ENOMEM;

	/* All chunks are free. Attempt to allocate the first chunk. */
	if (first_alloc_idx == -1) {
		base = alloc_from_contiguous_heap(h, start_addr, len);
		if (base == start_addr)
			goto alloc_success;
		BUG_ON(!dma_mapping_error(h->cma_dev, base));
	}

	/* Free chunk before previously allocated chunk. Attempt
	 * to allocate only immediate previous chunk.
	 */
	if (first_alloc_idx > 0) {
		alloc_at_idx = first_alloc_idx - 1;
		start_addr = alloc_at_idx * h->cma_chunk_size + h->cma_base;
		base = alloc_from_contiguous_heap(h, start_addr, len);
		if (base == start_addr)
			goto alloc_success;
		BUG_ON(!dma_mapping_error(h->cma_dev, base));
	}

	/* Free chunk after previously allocated chunk. */
	if (last_alloc_idx < h->num_chunks - 1) {
		alloc_at_idx = last_alloc_idx + 1;
		len = (alloc_at_idx == h->num_chunks - 1) ?
				h->rem_chunk_size : h->cma_chunk_size;
		start_addr = alloc_at_idx * h->cma_chunk_size + h->cma_base;
		base = alloc_from_contiguous_heap(h, start_addr, len);
		if (base == start_addr)
			goto alloc_success;
		BUG_ON(!dma_mapping_error(h->cma_dev, base));
	}

	if (dma_mapping_error(h->cma_dev, base))
		dev_err(&h->dev,
		"Failed to allocate contiguous memory on heap grow req\n");

	return -ENOMEM;

alloc_success:
	if (!h->curr_len || h->curr_base > base)
		h->curr_base = base;
	h->curr_len += len;

	if (!skip_vpr_config && update_vpr_config(h))
		goto fail_update;

	dev_dbg(&h->dev, "grow heap base from=%pa to=%pa,"
			" len from=0x%zx to=0x%zx\n",
			&prev_base, &h->curr_base, prev_len, h->curr_len);
	return 0;

fail_update:
	release_from_contiguous_heap(h, base, len);
	h->curr_base = prev_base;
	h->curr_len = prev_len;
	return -ENOMEM;
}

static bool shrink_chunk_locked(struct heap_info *h, int idx)
{
	size_t chunk_size;
	int resize_err;
	void *ret = NULL;
	dma_addr_t dev_base;
	unsigned long attrs = DMA_ATTR_ALLOC_EXACT_SIZE;

	/* check if entire chunk is free */
	chunk_size = (idx == h->num_chunks - 1) ? h->rem_chunk_size :
						  h->cma_chunk_size;

	/* Do not attempt to downsize if we will violate the floor */
	if ((h->curr_len - chunk_size) < h->floor_size)
		return false;

	resize_err = dma_alloc_from_dev_coherent_attr_at(&h->dev,
				chunk_size, &dev_base, &ret, attrs,
				idx * h->cma_chunk_size >> PAGE_SHIFT);
	if (!resize_err) {
		goto out;
	} else if (dev_base != h->cma_base + idx * h->cma_chunk_size) {
		resize_err = dma_release_from_dev_coherent_attr(
				&h->dev, chunk_size,
				(void *)(uintptr_t)dev_base, attrs);
		BUG_ON(!resize_err);
		goto out;
	} else {
		dev_dbg(&h->dev,
			"prep to remove chunk b=%pa, s=0x%zx\n",
			&dev_base, chunk_size);
		resize_err = dma_release_from_dev_coherent_attr(
				&h->dev, chunk_size,
				(void *)(uintptr_t)dev_base, attrs);
		BUG_ON(!resize_err);
		if (!resize_err) {
			dev_err(&h->dev, "failed to rel mem\n");
			goto out;
		}

		/* Handle VPR configuration updates */
		if (h->update_resize_cfg) {
			phys_addr_t new_base = h->curr_base;
			size_t new_len = h->curr_len - chunk_size;

			if (h->curr_base == dev_base)
				new_base += chunk_size;
			dev_dbg(&h->dev, "update vpr base to %pa, size=%zx\n",
				&new_base, new_len);
			resize_err =
				h->update_resize_cfg(new_base, new_len);
			if (resize_err) {
				dev_err(&h->dev,
					"update resize failed\n");
				goto out;
			}
		}
		if (h->curr_base == dev_base)
			h->curr_base += chunk_size;
		h->curr_len -= chunk_size;
		update_alloc_range(h);
		release_from_contiguous_heap(h, dev_base, chunk_size);
		dev_dbg(&h->dev, "removed chunk b=%pa, s=0x%zx"
			" new heap b=%pa, s=0x%zx\n", &dev_base,
			chunk_size, &h->curr_base, h->curr_len);
		return true;
	}
out:
	return false;
}

static void shrink_resizable_heap(struct heap_info *h)
{
	bool unlock = false;
	int first_alloc_idx, last_alloc_idx;

check_next_chunk:
	if (unlock) {
		mutex_unlock(&h->resize_lock);
		cond_resched();
	}
	mutex_lock(&h->resize_lock);
	unlock = true;
	if (h->curr_len <= h->floor_size)
		goto out_unlock;
	get_first_and_last_idx(h, &first_alloc_idx, &last_alloc_idx);
	/* All chunks are free. Exit. */
	if (first_alloc_idx == -1)
		goto out_unlock;
	if (shrink_chunk_locked(h, first_alloc_idx))
		goto check_next_chunk;
	/* Only one chunk is in use. */
	if (first_alloc_idx == last_alloc_idx)
		goto out_unlock;
	if (shrink_chunk_locked(h, last_alloc_idx))
		goto check_next_chunk;

out_unlock:
	mutex_unlock(&h->resize_lock);
}


/*
 * Helper function used to manage resizable heap shrink timeouts
 */

static void shrink_timeout(struct timer_list *t)
{
	struct timer_data *data = from_timer(data, t, shrink_timer);

	wake_up_process(data->task);
}

static int shrink_thread(void *arg)
{
	struct heap_info *h = arg;

	/*
	 * Set up an interval timer which can be used to trigger a commit wakeup
	 * after the commit interval expires
	 */
	timer_setup(&h->shrink_timer_data.shrink_timer, shrink_timeout, 0);
	h->shrink_timer_data.task = current;

	while (1) {
		if (kthread_should_stop())
			break;

		shrink_resizable_heap(h);
		/* resize done. goto sleep */
		set_current_state(TASK_INTERRUPTIBLE);
		schedule();
	}

	return 0;
}

int dma_set_resizable_heap_floor_size(struct device *dev, size_t floor_size)
{
	int ret = 0;
	struct heap_info *h = NULL;
	phys_addr_t orig_base, prev_base, left_chunks_base, right_chunks_base;
	size_t orig_len, prev_len, left_chunks_len, right_chunks_len, orig_floor;

	if (!dma_is_coherent_dev(dev))
		return -ENODEV;

	h = dev_get_drvdata(dev);
	if (!h)
		return -ENOENT;

	mutex_lock(&h->resize_lock);
	orig_base = h->curr_base;
	orig_len = h->curr_len;
	orig_floor = h->floor_size;
	right_chunks_base = h->curr_base + h->curr_len;
	left_chunks_len = right_chunks_len = 0;

	h->floor_size = floor_size > h->cma_len ? h->cma_len : floor_size;
	while (h->curr_len < h->floor_size) {
		prev_base = h->curr_base;
		prev_len = h->curr_len;

		ret = heap_resize_locked(h, true);
		if (ret)
			goto fail_set_floor;

		if (h->curr_base < prev_base) {
			left_chunks_base = h->curr_base;
			left_chunks_len += (h->curr_len - prev_len);
		} else {
			right_chunks_len += (h->curr_len - prev_len);
		}
	}

	if ((h->curr_base != orig_base) || (h->curr_len != orig_len)) {
		ret = update_vpr_config(h);
		if (!ret) {
			dev_dbg(&h->dev,
				"grow heap base from=%pa to=%pa,"
				" len from=0x%zx to=0x%zx\n",
				&orig_base, &h->curr_base, orig_len, h->curr_len);
			goto success_set_floor;
		}
	} else {
		goto success_set_floor;
	}

fail_set_floor:
	if (left_chunks_len != 0)
		release_from_contiguous_heap(h, left_chunks_base,
				left_chunks_len);
	if (right_chunks_len != 0)
		release_from_contiguous_heap(h, right_chunks_base,
				right_chunks_len);
	h->curr_base = orig_base;
	h->curr_len = orig_len;
	h->floor_size = orig_floor;

success_set_floor:
	if (h->shrink_timer_data.task)
		mod_timer(&h->shrink_timer_data.shrink_timer, jiffies + h->shrink_interval);
	mutex_unlock(&h->resize_lock);
	if (!h->shrink_timer_data.task)
		shrink_resizable_heap(h);
	return ret;
}
EXPORT_SYMBOL(dma_set_resizable_heap_floor_size);

static int declare_coherent_heap(struct device *dev, phys_addr_t base,
					size_t size, int map)
{
	int err;
	int flags = map ? 0 : DMA_MEMORY_NOMAP;

	BUG_ON(dev->dma_mem);
	dma_set_coherent_mask(dev,  DMA_BIT_MASK(64));
	err = dma_declare_coherent_memory(dev, 0,
			base, size, flags);
	if (!err) {
		dev_dbg(dev, "dma coherent mem base (%pa) size (0x%zx) %x\n",
			&base, size, flags);
		return 0;
	}
	dev_err(dev, "declare dma coherent_mem fail %pa 0x%zx %x\n",
		&base, size, flags);
	return -ENOMEM;
}

int dma_declare_coherent_resizable_cma_memory(struct device *dev,
					struct dma_declare_info *dma_info)
{
#ifdef CONFIG_DMA_CMA
	int err = 0;
	struct heap_info *heap_info = NULL;
	struct dma_contiguous_stats stats;

	if (!dev || !dma_info || !dma_info->name || !dma_info->cma_dev)
		return -EINVAL;

	heap_info = kzalloc(sizeof(*heap_info), GFP_KERNEL);
	if (!heap_info)
		return -ENOMEM;

	heap_info->magic = RESIZE_MAGIC;
	heap_info->name = kmalloc(strlen(dma_info->name) + 1, GFP_KERNEL);
	if (!heap_info->name) {
		kfree(heap_info);
		return -ENOMEM;
	}

	dma_get_contiguous_stats(dma_info->cma_dev, &stats);
	pr_info("resizable heap=%s, base=%pa, size=0x%zx\n",
		dma_info->name, &stats.base, stats.size);
	strcpy(heap_info->name, dma_info->name);
	dev_set_name(dev, "dma-%s", heap_info->name);
	heap_info->cma_dev = dma_info->cma_dev;
	heap_info->cma_chunk_size = dma_info->size ? : stats.size;
	heap_info->cma_base = stats.base;
	heap_info->cma_len = stats.size;
	heap_info->curr_base = stats.base;
	dev_set_name(heap_info->cma_dev, "cma-%s-heap", heap_info->name);
	mutex_init(&heap_info->resize_lock);

	if (heap_info->cma_len < heap_info->cma_chunk_size) {
		dev_err(dev, "error cma_len(0x%zx) < cma_chunk_size(0x%zx)\n",
			heap_info->cma_len, heap_info->cma_chunk_size);
		err = -EINVAL;
		goto fail;
	}

	heap_info->num_chunks = div64_u64_rem(heap_info->cma_len,
				(u64)heap_info->cma_chunk_size,
				(u64 *)&heap_info->rem_chunk_size);
	if (heap_info->rem_chunk_size) {
		heap_info->num_chunks++;
		dev_info(dev, "heap size is not multiple of cma_chunk_size "
			"heap_info->num_chunks (%d) rem_chunk_size(0x%zx)\n",
			heap_info->num_chunks, heap_info->rem_chunk_size);
	} else
		heap_info->rem_chunk_size = heap_info->cma_chunk_size;

	dev_set_name(&heap_info->dev, "%s-heap", heap_info->name);

	if (dma_info->notifier.ops)
		heap_info->update_resize_cfg =
			dma_info->notifier.ops->resize;

	dev_set_drvdata(dev, heap_info);
	dma_debugfs_init(dev, heap_info);

	if (declare_coherent_heap(&heap_info->dev,
				  heap_info->cma_base, heap_info->cma_len,
				  (dma_info->notifier.ops &&
					dma_info->notifier.ops->resize) ? 0 : 1))
		goto declare_fail;
	heap_info->dev.dma_mem->size = 0;
	heap_info->shrink_interval = HZ * RESIZE_DEFAULT_SHRINK_AGE;
	kthread_run(shrink_thread, heap_info, "%s-shrink_thread",
		heap_info->name);

	if (dma_info->notifier.ops && dma_info->notifier.ops->resize)
		dma_contiguous_enable_replace_pages(dma_info->cma_dev);

	pr_info("resizable cma heap=%s create successful", heap_info->name);
	return 0;
declare_fail:
	kfree(heap_info->name);
fail:
	kfree(heap_info);
	return err;
#else
	return -EINVAL;
#endif
}
EXPORT_SYMBOL(dma_declare_coherent_resizable_cma_memory);

static int dma_init_coherent_memory(phys_addr_t phys_addr,
		dma_addr_t device_addr, size_t size, int flags,
		struct dma_coherent_mem **mem)
{
	struct dma_coherent_mem *dma_mem = NULL;
	void *mem_base = NULL;
	int pages = size >> PAGE_SHIFT;
	int bitmap_size = BITS_TO_LONGS(pages) * sizeof(long);
	int ret;

	if (!size)
		return -EINVAL;

	if (!(flags & DMA_MEMORY_NOMAP)) {
		mem_base = memremap(phys_addr, size, MEMREMAP_WC);
		if (!mem_base)
			ret = -EINVAL;
	}

	dma_mem = kzalloc(sizeof(struct dma_coherent_mem), GFP_KERNEL);
	if (!dma_mem) {
		ret = -ENOMEM;
		goto err_memunmap;
	}
	dma_mem->bitmap = kzalloc(bitmap_size, GFP_KERNEL);
	if (!dma_mem->bitmap) {
		ret = -ENOMEM;
		goto err_free_dma_mem;
	}

	dma_mem->virt_base = mem_base;
	dma_mem->device_base = device_addr;
	dma_mem->pfn_base = PFN_DOWN(phys_addr);
	dma_mem->size = pages;
	dma_mem->flags = flags;
	spin_lock_init(&dma_mem->spinlock);

	*mem = dma_mem;
	return 0;

err_free_dma_mem:
	kfree(dma_mem);
err_memunmap:
	if (mem_base)
		memunmap(mem_base);
	return ret;
}

static void dma_release_coherent_memory(struct dma_coherent_mem *mem)
{
	if (!mem)
		return;
	if (!(mem->flags & DMA_MEMORY_NOMAP))
		memunmap(mem->virt_base);
	kfree(mem->bitmap);
	kfree(mem);
}

static int dma_assign_coherent_memory(struct device *dev,
				      struct dma_coherent_mem *mem)
{
	if (!dev)
		return -ENODEV;

	if (dev->dma_mem)
		return -EBUSY;

	dev->dma_mem = mem;
	return 0;
}

/*
 * Declare a region of memory to be handed out by dma_alloc_coherent() when it
 * is asked for coherent memory for this device.  This shall only be used
 * from platform code, usually based on the device tree description.
 * 
 * phys_addr is the CPU physical address to which the memory is currently
 * assigned (this will be ioremapped so the CPU can access the region).
 *
 * device_addr is the DMA address the device needs to be programmed with to
 * actually address this memory (this will be handed out as the dma_addr_t in
 * dma_alloc_coherent()).
 *
 * size is the size of the area (must be a multiple of PAGE_SIZE).
 *
 * As a simplification for the platforms, only *one* such region of memory may
 * be declared per device.
 */
int dma_declare_coherent_memory(struct device *dev, phys_addr_t phys_addr,
				dma_addr_t device_addr, size_t size, int flags)
{
	struct dma_coherent_mem *mem;
	int ret;

	ret = dma_init_coherent_memory(phys_addr, device_addr, size, flags, &mem);
	if (ret)
		return ret;

	ret = dma_assign_coherent_memory(dev, mem);
	if (ret)
		dma_release_coherent_memory(mem);
	return ret;
}

static inline struct page **kvzalloc_pages(u32 count)
{
        if (count * sizeof(struct page *) <= PAGE_SIZE)
                return kzalloc(count * sizeof(struct page *), GFP_KERNEL);
        else
                return vzalloc(count * sizeof(struct page *));
}

static void *__dma_alloc_from_coherent(struct device *dev,
				       struct dma_coherent_mem *mem,
				       ssize_t size, dma_addr_t *dma_handle,
					unsigned long attrs,
					unsigned long start)
{
        int order = get_order(size);
        unsigned long flags;
        int pageno, i = 0;
        unsigned int count;
        unsigned int alloc_size;
        unsigned long align;
        void *ret = NULL;
        struct page **pages = NULL;
        int do_memset = 0;

        if (dma_get_attr(DMA_ATTR_ALLOC_EXACT_SIZE, attrs))
                count = PAGE_ALIGN(size) >> PAGE_SHIFT;
        else
                count = 1 << order;

        if (!count)
                return 0;

        if ((mem->flags & DMA_MEMORY_NOMAP) &&
            dma_get_attr(DMA_ATTR_ALLOC_SINGLE_PAGES, attrs)) {
                alloc_size = 1;
                pages = kvzalloc_pages(count);
                if (!pages)
                        return 0;
        } else {
                alloc_size = count;
        }

        spin_lock_irqsave(&mem->spinlock, flags);

        if (unlikely(size > (mem->size << PAGE_SHIFT)))
                goto err;

        if ((mem->flags & DMA_MEMORY_NOMAP) &&
            dma_get_attr(DMA_ATTR_ALLOC_SINGLE_PAGES, attrs)) {
                align = 0;
        } else  {
                if (order > DMA_BUF_ALIGNMENT)
                        align = (1 << DMA_BUF_ALIGNMENT) - 1;
                else
                        align = (1 << order) - 1;
        }

        while (count) {
                pageno = bitmap_find_next_zero_area(mem->bitmap, mem->size,
                                start, alloc_size, align);

                if (pageno >= mem->size)
                        goto err;

                count -= alloc_size;
                if (pages)
                        pages[i++] = pfn_to_page(mem->pfn_base + pageno);
                bitmap_set(mem->bitmap, pageno, alloc_size);
        }

        /*
         * Memory was found in the coherent area.
         */
        *dma_handle = mem->device_base + (pageno << PAGE_SHIFT);
        if (!(mem->flags & DMA_MEMORY_NOMAP)) {
                ret = mem->virt_base + (pageno << PAGE_SHIFT);
                do_memset = 1;
        } else if (dma_get_attr(DMA_ATTR_ALLOC_SINGLE_PAGES, attrs)) {
                ret = pages;
        }

        spin_unlock_irqrestore(&mem->spinlock, flags);

        if (do_memset)
                memset(ret, 0, size);

        return ret;
err:
        while (i--)
                bitmap_clear(mem->bitmap, page_to_pfn(pages[i]) -
                                        mem->pfn_base, alloc_size);

        spin_unlock_irqrestore(&mem->spinlock, flags);
        kvfree(pages);
        return NULL;
}

/**
 * dma_alloc_from_dev_coherent_attr() - allocate memory from device coherent pool
 * @dev:	device from which we allocate memory
 * @size:	size of requested memory area
 * @dma_handle:	This will be filled with the correct dma handle
 * @ret:	This pointer will be filled with the virtual address
 *		to allocated area.
 * @attrs:	DMA attribute
 *
 * This function should be only called from per-arch dma_alloc_coherent()
 * to support allocation from per-device coherent memory pools.
 *
 * Returns 0 if dma_alloc_coherent should continue with allocating from
 * generic memory areas, or !0 if dma_alloc_coherent should return @ret.
 */
int dma_alloc_from_dev_coherent_attr(struct device *dev, ssize_t size,
		dma_addr_t *dma_handle, void **ret, unsigned long attrs)
{
	struct dma_coherent_mem *mem = dev_get_coherent_memory(dev);

	if (!mem)
		return 0;

	*ret = __dma_alloc_from_coherent(dev, mem, size, dma_handle, attrs, 0);
	return 1;
}

int dma_alloc_from_dev_coherent_attr_at(struct device *dev, ssize_t size,
		dma_addr_t *dma_handle, void **ret, unsigned long attrs,
		ulong start)
{
	struct dma_coherent_mem *mem = dev_get_coherent_memory(dev);

	if (!mem)
		return 0;

	*dma_handle = DMA_ERROR_CODE;

	*ret = __dma_alloc_from_coherent(dev, mem, size, dma_handle, attrs, start);
	if (*dma_handle != DMA_ERROR_CODE)
		return 1;

	/*
	 * In the case where the allocation can not be satisfied from the
	 * per-device area, try to fall back to generic memory if the
	 * constraints allow it.
	 */
	return mem->flags & DMA_MEMORY_EXCLUSIVE;
}

void *dma_alloc_from_global_coherent(struct device *dev, ssize_t size,
				     dma_addr_t *dma_handle)
{
	if (!dma_coherent_default_memory)
		return NULL;

	return __dma_alloc_from_coherent(dev, dma_coherent_default_memory, size,
					 dma_handle, 0, 0);
}

static int __dma_release_from_coherent(struct dma_coherent_mem *mem,
				       ssize_t size, void *vaddr,
				       unsigned long attrs)
{
        void *mem_addr;
        unsigned long flags;
        unsigned int pageno;

        if (!mem)
                return 0;

        if ((mem->flags & DMA_MEMORY_NOMAP) &&
            dma_get_attr(DMA_ATTR_ALLOC_SINGLE_PAGES, attrs)) {
                struct page **pages = vaddr;
                int i;

                spin_lock_irqsave(&mem->spinlock, flags);
                for (i = 0; i < (size >> PAGE_SHIFT); i++) {
                        pageno = page_to_pfn(pages[i]) - mem->pfn_base;
                        if (WARN_ONCE(pageno > mem->size,
                                "invalid pageno:%d\n", pageno))
                                continue;
                        bitmap_clear(mem->bitmap, pageno, 1);
                }
                spin_unlock_irqrestore(&mem->spinlock, flags);
                kvfree(pages);
                return 1;
        }

        if (mem->flags & DMA_MEMORY_NOMAP)
                mem_addr =  (void *)(uintptr_t)mem->device_base;
        else
                mem_addr =  mem->virt_base;

        if (mem && vaddr >= mem_addr &&
            vaddr - mem_addr < mem->size << PAGE_SHIFT) {

                int page = (vaddr - mem_addr) >> PAGE_SHIFT;
                unsigned long flags;
                unsigned int count;

                if (DMA_ATTR_ALLOC_EXACT_SIZE & attrs)
                        count = PAGE_ALIGN(size) >> PAGE_SHIFT;
                else
                        count = 1 << get_order(size);

                spin_lock_irqsave(&mem->spinlock, flags);
                bitmap_clear(mem->bitmap, page, count);
                spin_unlock_irqrestore(&mem->spinlock, flags);
                return 1;
        }
        return 0;
}

/* retval: !0 on success, 0 on failure */
static int dma_alloc_from_coherent_heap_dev(struct device *dev, ssize_t len,
					dma_addr_t *dma_handle, void **ret,
					unsigned long attrs)
{
	struct heap_info *h = NULL;

	if (!dma_is_coherent_dev(dev))
		return 0;

	*dma_handle = DMA_ERROR_CODE;

	h = dev_get_drvdata(dev);
	BUG_ON(!h);
	if (!h)
		return DMA_MEMORY_EXCLUSIVE;

	attrs |= DMA_ATTR_ALLOC_EXACT_SIZE;

	mutex_lock(&h->resize_lock);
retry_alloc:
	/* Try allocation from already existing CMA chunks */
	if (dma_alloc_from_dev_coherent_attr_at(
		&h->dev, len, dma_handle, ret, attrs,
		(h->curr_base - h->cma_base) >> PAGE_SHIFT)) {
		if (*dma_handle != DMA_ERROR_CODE) {
			dev_dbg(&h->dev, "allocated addr %pa len 0x%zx\n",
				dma_handle, len);
			h->curr_used += len;
		}
		goto out;
	}

	if (!heap_resize_locked(h, false))
		goto retry_alloc;

out:
	mutex_unlock(&h->resize_lock);
	return DMA_MEMORY_EXCLUSIVE;
}

/* retval: !0 on success, 0 on failure */
static int dma_release_from_coherent_heap_dev(struct device *dev, ssize_t len,
					void *base, unsigned long attrs)
{
	int idx = 0;
	int err = 0;
	struct heap_info *h = NULL;

	if (!dma_is_coherent_dev(dev))
		return 0;

	h = dev_get_drvdata(dev);
	BUG_ON(!h);
	if (!h)
		return 1;

	mutex_lock(&h->resize_lock);
	if (!dma_get_attr(DMA_ATTR_ALLOC_SINGLE_PAGES, attrs)) {
		if ((uintptr_t)base < h->curr_base || len > h->curr_len ||
		    (uintptr_t)base - h->curr_base > h->curr_len - len) {
			BUG();
			mutex_unlock(&h->resize_lock);
			return 1;
		}

		idx = div_u64((uintptr_t)base - h->cma_base, h->cma_chunk_size);
		dev_dbg(&h->dev, "req free addr (%p) size (0x%zx) idx (%d)\n",
			base, len, idx);
	}

	attrs |= DMA_ATTR_ALLOC_EXACT_SIZE;

	err = dma_release_from_dev_coherent_attr(&h->dev, len, base, attrs);
	/* err = 0 on failure, !0 on successful release */
	if (err && h->shrink_timer_data.task)
		mod_timer(&h->shrink_timer_data.shrink_timer, jiffies + h->shrink_interval);

	if (err)
		h->curr_used -= len;

	mutex_unlock(&h->resize_lock);

	if (err && !h->shrink_timer_data.task)
		shrink_resizable_heap(h);
	return err;
}

/**
 * dma_alloc_from_coherent_attr() - try to allocate memory from the per-device
 * coherent area
 *
 * @dev:	device from which we allocate memory
 * @size:	size of requested memory area
 * @dma_handle:	This will be filled with the correct dma handle
 * @ret:	This pointer will be filled with the virtual address
 *		to allocated area.
 * @attrs:	DMA Attribute
 * This function should be only called from per-arch dma_alloc_coherent()
 * to support allocation from per-device coherent memory pools.
 *
 * Returns 0 if dma_alloc_coherent_attr should continue with allocating from
 * generic memory areas, or !0 if dma_alloc_coherent should return @ret.
 */
int dma_alloc_from_coherent_attr(struct device *dev, ssize_t size,
				       dma_addr_t *dma_handle, void **ret,
				       unsigned long attrs)
{
	if (!dev)
		return 0;

	if (dev->dma_mem)
		return dma_alloc_from_dev_coherent_attr(dev, size, dma_handle,
							ret, attrs);
	else
		return dma_alloc_from_coherent_heap_dev(dev, size, dma_handle,
							ret, attrs);
}
EXPORT_SYMBOL(dma_alloc_from_coherent_attr);

/**
 * dma_release_from_coherent_attr() - try to free the memory allocated from
 * per-device coherent memory pool
 * @dev:	device from which the memory was allocated
 * @size:	size of the memory area to free
 * @vaddr:	virtual address of allocated pages
 * @attrs:	DMA Attribute
 *
 * This checks whether the memory was allocated from the per-device
 * coherent memory pool and if so, releases that memory.
 *
 * Returns 1 if we correctly released the memory, or 0 if
 * dma_release_coherent_attr() should proceed with releasing memory from
 * generic pools.
 */
int dma_release_from_coherent_attr(struct device *dev, ssize_t size,
			void *vaddr, unsigned long attrs)
{
	if (!dev)
		return 0;

	if (dev->dma_mem)
		return dma_release_from_dev_coherent_attr(dev, size, vaddr,
								attrs);
	else
		return dma_release_from_coherent_heap_dev(dev, size, vaddr,
								attrs);
}
EXPORT_SYMBOL(dma_release_from_coherent_attr);

/**
 * dma_release_from_dev_coherent_attr() - free memory to device coherent memory pool
 * @dev:	device from which the memory was allocated
 * @order:	the order of pages allocated
 * @vaddr:	virtual address of allocated pages
 * @attrs:	DMA attribute
 *
 * This checks whether the memory was allocated from the per-device
 * coherent memory pool and if so, releases that memory.
 *
 * Returns 1 if we correctly released the memory, or 0 if the caller should
 * proceed with releasing memory from generic pools.
 */
int dma_release_from_dev_coherent_attr(struct device *dev, ssize_t size,
					void *vaddr, unsigned long attrs)
{
	struct dma_coherent_mem *mem = dev_get_coherent_memory(dev);

	return __dma_release_from_coherent(mem, size, vaddr, attrs);
}

int dma_release_from_global_coherent(size_t size, void *vaddr)
{
	if (!dma_coherent_default_memory)
		return 0;

	return __dma_release_from_coherent(dma_coherent_default_memory, size,
			vaddr, 0);
}

static int __dma_mmap_from_coherent(struct dma_coherent_mem *mem,
		struct vm_area_struct *vma, void *vaddr, size_t size, int *ret)
{
	void *mem_addr;

	if (!mem)
		return 0;

	if (mem->flags & DMA_MEMORY_NOMAP)
		mem_addr =  (void *)mem->device_base;
	else
		mem_addr =  mem->virt_base;

	if (mem && vaddr >= mem_addr && vaddr + size <=
		   (mem_addr + ((dma_addr_t)mem->size << PAGE_SHIFT))) {
		unsigned long off = vma->vm_pgoff;
		int start = (vaddr - mem_addr) >> PAGE_SHIFT;
		unsigned long user_count = vma_pages(vma);
		int count = PAGE_ALIGN(size) >> PAGE_SHIFT;

		*ret = -ENXIO;
		if (off < count && user_count <= count - off) {
			unsigned long pfn = mem->pfn_base + start + off;
			*ret = remap_pfn_range(vma, vma->vm_start, pfn,
					       user_count << PAGE_SHIFT,
					       vma->vm_page_prot);
		}
		return 1;
	}
	return 0;
}

/**
 * dma_mmap_from_dev_coherent() - mmap memory from the device coherent pool
 * @dev:	device from which the memory was allocated
 * @vma:	vm_area for the userspace memory
 * @vaddr:	cpu address returned by dma_alloc_from_dev_coherent
 * @size:	size of the memory buffer allocated
 * @ret:	result from remap_pfn_range()
 *
 * This checks whether the memory was allocated from the per-device
 * coherent memory pool and if so, maps that memory to the provided vma.
 *
 * Returns 1 if @vaddr belongs to the device coherent pool and the caller
 * should return @ret, or 0 if they should proceed with mapping memory from
 * generic areas.
 */
int dma_mmap_from_dev_coherent(struct device *dev, struct vm_area_struct *vma,
			   void *vaddr, size_t size, int *ret)
{
	struct dma_coherent_mem *mem = dev_get_coherent_memory(dev);

	return __dma_mmap_from_coherent(mem, vma, vaddr, size, ret);
}

int dma_mmap_from_global_coherent(struct vm_area_struct *vma, void *vaddr,
				   size_t size, int *ret)
{
	if (!dma_coherent_default_memory)
		return 0;

	return __dma_mmap_from_coherent(dma_coherent_default_memory, vma,
					vaddr, size, ret);
}

int dma_get_coherent_stats(struct device *dev,
			struct dma_coherent_stats *stats)
{
	struct heap_info *h = NULL;
	struct dma_coherent_mem *mem = dev->dma_mem;

	if ((!dev) || !stats)
		return -EINVAL;

	h = dev_get_drvdata(dev);
	if (h && (h->magic == RESIZE_MAGIC)) {
		stats->size = h->curr_len;
		stats->base = h->curr_base;
		stats->used = h->curr_used;
		stats->max = h->cma_len;
		goto out;
	}

	if (!mem)
		return -EINVAL;
	stats->size = mem->size << PAGE_SHIFT;
	stats->base = mem->device_base;
out:
	return 0;
}
EXPORT_SYMBOL(dma_get_coherent_stats);

/*
 * Support for reserved memory regions defined in device tree
 */
#ifdef CONFIG_OF_RESERVED_MEM
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>

static struct reserved_mem *dma_reserved_default_memory __initdata;

static int rmem_dma_device_init(struct reserved_mem *rmem, struct device *dev)
{
	struct dma_coherent_mem *mem = rmem->priv;
	int ret;

	if (!mem) {
		ret = dma_init_coherent_memory(rmem->base, rmem->base,
					       rmem->size, 0,  &mem);
		if (ret) {
			pr_err("Reserved memory: failed to init DMA memory pool at %pa, size %ld MiB\n",
				&rmem->base, (unsigned long)rmem->size / SZ_1M);
			return ret;
		}
	}
	mem->use_dev_dma_pfn_offset = true;
	rmem->priv = mem;
	dma_assign_coherent_memory(dev, mem);
	return 0;
}

static void rmem_dma_device_release(struct reserved_mem *rmem,
				    struct device *dev)
{
	if (dev)
		dev->dma_mem = NULL;
}

static const struct reserved_mem_ops rmem_dma_ops = {
	.device_init	= rmem_dma_device_init,
	.device_release	= rmem_dma_device_release,
};

static int __init rmem_dma_setup(struct reserved_mem *rmem)
{
	unsigned long node = rmem->fdt_node;

	if (of_get_flat_dt_prop(node, "reusable", NULL))
		return -EINVAL;

#ifdef CONFIG_ARM
	if (!of_get_flat_dt_prop(node, "no-map", NULL)) {
		pr_err("Reserved memory: regions without no-map are not yet supported\n");
		return -EINVAL;
	}

	if (of_get_flat_dt_prop(node, "linux,dma-default", NULL)) {
		WARN(dma_reserved_default_memory,
		     "Reserved memory: region for default DMA coherent area is redefined\n");
		dma_reserved_default_memory = rmem;
	}
#endif

	rmem->ops = &rmem_dma_ops;
	pr_info("Reserved memory: created DMA memory pool at %pa, size %ld MiB\n",
		&rmem->base, (unsigned long)rmem->size / SZ_1M);
	return 0;
}

static int __init dma_init_reserved_memory(void)
{
	const struct reserved_mem_ops *ops;
	int ret;

	if (!dma_reserved_default_memory)
		return -ENOMEM;

	ops = dma_reserved_default_memory->ops;

	/*
	 * We rely on rmem_dma_device_init() does not propagate error of
	 * dma_assign_coherent_memory() for "NULL" device.
	 */
	ret = ops->device_init(dma_reserved_default_memory, NULL);

	if (!ret) {
		dma_coherent_default_memory = dma_reserved_default_memory->priv;
		pr_info("DMA: default coherent area is set\n");
	}

	return ret;
}

core_initcall(dma_init_reserved_memory);

RESERVEDMEM_OF_DECLARE(dma, "shared-dma-pool", rmem_dma_setup);
#endif
