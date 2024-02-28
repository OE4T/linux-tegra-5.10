/*
 * Copyright (c) 2018-2020, NVIDIA Corporation.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/slab.h>
#include <linux/version.h>
#include <linux/mmu_notifier.h>
#include <linux/module.h>
#include <linux/nv-p2p.h>

MODULE_DESCRIPTION("Nvidia Tegra P2P Driver");
MODULE_AUTHOR("Preetham Chandru pchandru@nvidia.com");
MODULE_LICENSE("GPL");

static void nvidia_p2p_mn_release(struct mmu_notifier *mn,
	struct mm_struct *mm)
{
	struct nvidia_p2p_page_table *page_table = container_of(mn,
						struct nvidia_p2p_page_table,
						mn);

	page_table->free_callback(page_table->data);
}

static void nvidia_p2p_mn_invl_range_start_legacy(struct mmu_notifier *mn,
	struct mm_struct *mm, unsigned long start, unsigned long end)
{
	struct nvidia_p2p_page_table *page_table = container_of(mn,
						struct  nvidia_p2p_page_table,
						mn);

	u64 vaddr = 0;
	u64 size = 0;

	vaddr = page_table->vaddr;
	size = page_table->size;

	if (vaddr >= start && vaddr <= end) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
		mmu_notifier_put(&page_table->mn);
#else
		mmu_notifier_unregister_no_release(&page_table->mn, page_table->mm);
#endif
		page_table->free_callback(page_table->data);
	}
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)
static int nvidia_p2p_mn_invl_range_start(struct mmu_notifier *mn,
	const struct mmu_notifier_range *range)
{
	nvidia_p2p_mn_invl_range_start_legacy(mn, NULL, range->start, range->end);

	return 0;
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
static void nvidia_p2p_free_notifier(struct mmu_notifier *mn)
{
}
#endif

static struct mmu_notifier_ops nvidia_p2p_mmu_ops = {
	.release		= nvidia_p2p_mn_release,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)
	.invalidate_range_start	= nvidia_p2p_mn_invl_range_start,
#else
	.invalidate_range_start	= nvidia_p2p_mn_invl_range_start_legacy,
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
	.free_notifier		= nvidia_p2p_free_notifier,
#endif
};

int nvidia_p2p_get_pages(u64 vaddr, u64 size,
		struct nvidia_p2p_page_table **page_table,
		void (*free_callback)(void *data), void *data)
{
	int ret = 0;
	int user_pages = 0;
	int locked = 0;
	int nr_pages = size >> PAGE_SHIFT;
	struct page **pages;

	if (nr_pages <= 0) {
		return -EINVAL;
	}

	*page_table = kzalloc(sizeof(**page_table), GFP_KERNEL);
	if (!*page_table) {
		return -ENOMEM;
	}

	pages = kcalloc(nr_pages, sizeof(*pages), GFP_KERNEL);
	if (!pages) {
		ret = -ENOMEM;
		goto free_page_table;
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
	down_read(&current->mm->mmap_lock);
#else
	down_read(&current->mm->mmap_sem);
#endif
	locked = 1;
	user_pages = get_user_pages_locked(vaddr & PAGE_MASK, nr_pages,
			FOLL_WRITE | FOLL_FORCE,
			pages, &locked);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
	up_read(&current->mm->mmap_lock);
#else
	up_read(&current->mm->mmap_sem);
#endif
	if (user_pages != nr_pages) {
		ret = user_pages < 0 ? user_pages : -ENOMEM;
		goto free_pages;
	}

	(*page_table)->version = NVIDIA_P2P_PAGE_TABLE_VERSION;
	(*page_table)->pages = pages;
	(*page_table)->entries = user_pages;
	(*page_table)->page_size = NVIDIA_P2P_PAGE_SIZE_4KB;
	(*page_table)->size = size;

	(*page_table)->mn.ops = &nvidia_p2p_mmu_ops;
	(*page_table)->mm = current->mm;
	(*page_table)->free_callback = free_callback;
	(*page_table)->data = data;
	(*page_table)->vaddr = vaddr;
	mutex_init(&(*page_table)->lock);
	(*page_table)->mapped = NVIDIA_P2P_PINNED;

	ret = mmu_notifier_register(&(*page_table)->mn, (*page_table)->mm);
	if (ret) {
		goto free_pages;
	}

	return 0;
free_pages:
	while (--user_pages >= 0) {
		put_page(pages[user_pages]);
	}
	kfree(pages);
free_page_table:
	kfree(*page_table);
	*page_table = NULL;
	return ret;
}
EXPORT_SYMBOL(nvidia_p2p_get_pages);

int nvidia_p2p_put_pages(struct nvidia_p2p_page_table *page_table)
{
	if (!page_table) {
		return -EINVAL;
	}

	mmu_notifier_unregister(&page_table->mn, page_table->mm);

	return 0;
}
EXPORT_SYMBOL(nvidia_p2p_put_pages);

int nvidia_p2p_free_page_table(struct nvidia_p2p_page_table *page_table)
{
	int user_pages = 0;
	struct page **pages = NULL;

	if (!page_table) {
		return 0;
	}

	mutex_lock(&page_table->lock);

	if (page_table->mapped & NVIDIA_P2P_MAPPED) {
		WARN(1, "Attempting to free unmapped pages");
	}

	if (page_table->mapped & NVIDIA_P2P_PINNED) {
		pages = page_table->pages;
		user_pages = page_table->entries;

		while (--user_pages >= 0) {
			put_page(pages[user_pages]);
		}

		kfree(pages);
		page_table->mapped &= (u32)~NVIDIA_P2P_PINNED;
	}

	mutex_unlock(&page_table->lock);

	return 0;
}
EXPORT_SYMBOL(nvidia_p2p_free_page_table);

int nvidia_p2p_dma_map_pages(struct device *dev,
		struct nvidia_p2p_page_table *page_table,
		struct nvidia_p2p_dma_mapping **dma_mapping,
		enum dma_data_direction direction)
{
	struct sg_table *sgt = NULL;
	struct scatterlist *sg;
	struct page **pages = NULL;
	u32 nr_pages = 0;
	int ret = 0;
	int i, count;

	if (!page_table) {
		return -EINVAL;
	}

	mutex_lock(&page_table->lock);

	pages = page_table->pages;
	nr_pages = page_table->entries;
	if (nr_pages <= 0) {
		mutex_unlock(&page_table->lock);
		return -EINVAL;
	}

	*dma_mapping = kzalloc(sizeof(**dma_mapping), GFP_KERNEL);
	if (!*dma_mapping) {
		mutex_unlock(&page_table->lock);
		return -ENOMEM;
	}
	sgt = kzalloc(sizeof(*sgt), GFP_KERNEL);
	if (!sgt) {
		ret = -ENOMEM;
		goto free_dma_mapping;
	}
	ret = sg_alloc_table_from_pages(sgt, pages,
				nr_pages, 0, page_table->size, GFP_KERNEL);
	if (ret) {
		goto free_sgt;
	}

	(*dma_mapping)->version = NVIDIA_P2P_DMA_MAPPING_VERSION;
	(*dma_mapping)->sgt = sgt;
	(*dma_mapping)->dev = dev;
	(*dma_mapping)->direction = direction;
	(*dma_mapping)->page_table = page_table;

	count = dma_map_sg(dev, sgt->sgl, sgt->nents, direction);
	if (count < 1) {
		goto free_sg_table;
	}

	(*dma_mapping)->entries = count;

	(*dma_mapping)->hw_address = kcalloc(count, sizeof(u64), GFP_KERNEL);
	if (!((*dma_mapping)->hw_address)) {
		ret = -ENOMEM;
		goto unmap_sg;
	}
	(*dma_mapping)->hw_len = kcalloc(count, sizeof(u64), GFP_KERNEL);
	if (!((*dma_mapping)->hw_len)) {
		ret = -ENOMEM;
		goto free_hw_address;
	}

	for_each_sg(sgt->sgl, sg, count, i) {
		(*dma_mapping)->hw_address[i] = sg_dma_address(sg);
		(*dma_mapping)->hw_len[i] = sg_dma_len(sg);
	}
	(*dma_mapping)->page_table->mapped |= NVIDIA_P2P_MAPPED;
	mutex_unlock(&page_table->lock);

	return 0;
free_hw_address:
	kfree((*dma_mapping)->hw_address);
unmap_sg:
	dma_unmap_sg(dev, sgt->sgl,
		sgt->nents, direction);
free_sg_table:
	sg_free_table(sgt);
free_sgt:
	kfree(sgt);
free_dma_mapping:
	kfree(*dma_mapping);
	*dma_mapping = NULL;
	mutex_unlock(&page_table->lock);

	return ret;
}
EXPORT_SYMBOL(nvidia_p2p_dma_map_pages);

int nvidia_p2p_dma_unmap_pages(struct nvidia_p2p_dma_mapping *dma_mapping)
{
	struct nvidia_p2p_page_table *page_table = NULL;

	if (!dma_mapping) {
		return -EINVAL;
	}

	page_table = dma_mapping->page_table;
	if (!page_table) {
		return -EFAULT;
	}

	mutex_lock(&page_table->lock);
	if (page_table->mapped & NVIDIA_P2P_MAPPED) {
		kfree(dma_mapping->hw_len);
		kfree(dma_mapping->hw_address);
		if (dma_mapping->entries)
			dma_unmap_sg(dma_mapping->dev,
				dma_mapping->sgt->sgl,
				dma_mapping->sgt->nents,
				dma_mapping->direction);
		sg_free_table(dma_mapping->sgt);
		kfree(dma_mapping->sgt);
		kfree(dma_mapping);
		page_table->mapped &= (u32)~NVIDIA_P2P_MAPPED;
	}
	mutex_unlock(&page_table->lock);

	return 0;
}
EXPORT_SYMBOL(nvidia_p2p_dma_unmap_pages);

int nvidia_p2p_free_dma_mapping(struct nvidia_p2p_dma_mapping *dma_mapping)
{
	return nvidia_p2p_dma_unmap_pages(dma_mapping);
}
EXPORT_SYMBOL(nvidia_p2p_free_dma_mapping);
