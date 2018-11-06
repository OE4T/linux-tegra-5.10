// SPDX-License-Identifier: GPL-2.0
/*
 * mods_mem.c - This file is part of NVIDIA MODS kernel driver.
 *
 * Copyright (c) 2008-2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA MODS kernel driver is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * NVIDIA MODS kernel driver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with NVIDIA MODS kernel driver.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include "mods_internal.h"

#include <linux/pagemap.h>

#ifdef CONFIG_BIGPHYS_AREA
#include <linux/bigphysarea.h>
#endif

#if defined(MODS_HAS_SET_DMA_MASK)
#include <linux/dma-mapping.h>
#include <linux/of.h>
#endif

static int mods_post_alloc(struct MODS_PHYS_CHUNK *chunk,
			   u64			   phys_addr,
			   struct MODS_MEM_INFO   *p_mem_info);
static void mods_pre_free(struct MODS_PHYS_CHUNK *chunk,
			  struct MODS_MEM_INFO	 *p_mem_info);

/****************************
 * DMA MAP HELPER FUNCTIONS *
 ****************************/

/*
 * Starting on Power9 systems, DMA addresses for NVLink are no longer
 * the same as used over PCIE.
 *
 * Power9 supports a 56-bit Real Address. This address range is compressed
 * when accessed over NvLink to allow the GPU to access all of memory using
 * its 47-bit Physical address.
 *
 * If there is an NPU device present on the system, it implies that NvLink
 * sysmem links are present and we need to apply the required address
 * conversion for NvLink within the driver. This is intended to be temporary
 * to ease the transition to kernel APIs to handle NvLink DMA mappings
 * via the NPU device.
 *
 * Note, a deviation from the documented compression scheme is that the
 * upper address bits (i.e. bit 56-63) instead of being set to zero are
 * preserved during NvLink address compression so the orignal PCIE DMA
 * address can be reconstructed on expansion. These bits can be safely
 * ignored on NvLink since they are truncated by the GPU.
 */
#if defined(CONFIG_PPC64) && defined(CONFIG_PCI)
static u64 mods_compress_nvlink_addr(struct pci_dev *dev, u64 addr)
{
	u64 addr47 = addr;

	/* Note, one key difference from the documented compression scheme
	 * is that BIT59 used for TCE bypass mode on PCIe is preserved during
	 * NVLink address compression to allow for the resulting DMA address to
	 * be used transparently on PCIe.
	 */
	if (has_npu_dev(dev, 0)) {
		addr47 = addr & (1LLU << 59);
		addr47 |= ((addr >> 45) & 0x3) << 43;
		addr47 |= ((addr >> 49) & 0x3) << 45;
		addr47 |= addr & ((1LLU << 43) - 1);
	}

	return addr47;
}
#else
#define mods_compress_nvlink_addr(dev, addr) (addr)
#endif

#if defined(CONFIG_PPC64) && defined(CONFIG_PCI)
static u64 mods_expand_nvlink_addr(struct pci_dev *dev, u64 addr47)
{
	u64 addr = addr47;

	if (has_npu_dev(dev, 0)) {
		addr = addr47 & ((1LLU << 43) - 1);
		addr |= (addr47 & (3ULL << 43)) << 2;
		addr |= (addr47 & (3ULL << 45)) << 4;
		addr |= addr47 & ~((1ULL << 56) - 1);
	}

	return addr;
}
#else
#define mods_expand_nvlink_addr(dev, addr) (addr)
#endif

#ifdef CONFIG_PCI
/* Unmap a page if it was mapped */
static void mods_dma_unmap_page(struct pci_dev *dev,
				u64             dev_addr,
				u32             order)
{
	dev_addr = mods_expand_nvlink_addr(dev, dev_addr);

	pci_unmap_page(dev,
		       dev_addr,
		       PAGE_SIZE << order,
		       DMA_BIDIRECTIONAL);

	mods_debug_printk(DEBUG_MEM_DETAILED,
		"dma unmap dev_addr=0x%llx on dev %04x:%02x:%02x.%x\n",
		(unsigned long long)dev_addr,
		pci_domain_nr(dev->bus),
		dev->bus->number,
		PCI_SLOT(dev->devfn),
		PCI_FUNC(dev->devfn));
}

/* Unmap and delete the specified DMA mapping */
static int mods_dma_unmap_and_free(struct MODS_MEM_INFO *p_mem_info,
				   struct MODS_DMA_MAP  *p_del_map)

{
	int		  found = 0;
	struct list_head *head  = &p_mem_info->dma_map_list;
	struct list_head *iter;

	list_for_each(iter, head) {
		struct MODS_DMA_MAP *p_dma_map = list_entry(iter,
							    struct MODS_DMA_MAP,
							    list);

		if (p_dma_map == p_del_map) {
			list_del(iter);
			found = 1;
			break;
		}
	}

	if (!found) {
		mods_error_printk("failed to unmap and free %p\n",
				  p_del_map);
		return -EINVAL;
	}

	/* Safeguard check, all mappings should have a
	 * non-null device
	 */
	if (p_del_map->dev) {
		int i;

		for (i = 0; i < p_mem_info->num_chunks; i++)
			mods_dma_unmap_page(p_del_map->dev,
					    p_del_map->dev_addr[i],
					    p_mem_info->pages[i].order);
		pci_dev_put(p_del_map->dev);
	}

	kfree(p_del_map);

	return OK;
}
#endif

/* Unmap and delete all DMA mappings on the specified allocation */
int mods_dma_unmap_all(struct MODS_MEM_INFO *p_mem_info,
		       struct pci_dev       *dev)
{
#ifdef CONFIG_PCI
	int               err  = OK;
	struct list_head *head = &p_mem_info->dma_map_list;
	struct list_head *iter;
	struct list_head *tmp;

	list_for_each_safe(iter, tmp, head) {
		struct MODS_DMA_MAP *p_dma_map;

		p_dma_map = list_entry(iter, struct MODS_DMA_MAP, list);

		if (!dev || (p_dma_map->dev == dev)) {
			err = mods_dma_unmap_and_free(p_mem_info, p_dma_map);
			if (err || dev)
				break;
		}
	}

	return err;
#else
	return OK;
#endif
}

#ifdef CONFIG_PCI
/* DMA map all pages in an allocation */
static void mods_dma_map_pages(struct MODS_MEM_INFO *p_mem_info,
			       struct MODS_DMA_MAP  *p_dma_map)
{
	int i;
	struct pci_dev *dev = p_dma_map->dev;

	for (i = 0; i < p_mem_info->num_chunks; i++) {
		struct MODS_PHYS_CHUNK *chunk = &p_mem_info->pages[i];
		u64 dev_addr;

		dev_addr = pci_map_page(dev,
					chunk->p_page,
					0,
					PAGE_SIZE << chunk->order,
					DMA_BIDIRECTIONAL);

		dev_addr = mods_compress_nvlink_addr(dev, dev_addr);

		p_dma_map->dev_addr[i] = dev_addr;

		mods_debug_printk(DEBUG_MEM_DETAILED,
			"dma map dev_addr=0x%llx, phys_addr=0x%llx on dev %04x:%02x:%02x.%x\n",
			(unsigned long long)dev_addr,
			(unsigned long long)chunk->dma_addr,
			pci_domain_nr(p_dma_map->dev->bus),
			p_dma_map->dev->bus->number,
			PCI_SLOT(p_dma_map->dev->devfn),
			PCI_FUNC(p_dma_map->dev->devfn));
	}
}

/* Create a DMA map on the specified allocation for the pci device.
 * Lazy-initialize the map list structure if one does not yet exist.
 */
static int mods_create_dma_map(struct MODS_MEM_INFO *p_mem_info,
			       struct pci_dev       *dev)
{
	struct MODS_DMA_MAP *p_dma_map;
	u32                  alloc_size;

	alloc_size = sizeof(*p_dma_map) +
		     (p_mem_info->num_chunks - 1) * sizeof(u64);

	p_dma_map = kzalloc(alloc_size, GFP_KERNEL | __GFP_NORETRY);
	if (unlikely(!p_dma_map)) {
		mods_error_printk("failed to allocate device map data\n");
		return -ENOMEM;
	}

	p_dma_map->dev = pci_dev_get(dev);
	mods_dma_map_pages(p_mem_info, p_dma_map);
	list_add(&p_dma_map->list, &p_mem_info->dma_map_list);

	return OK;
}
#endif

/* Find the dma mapping chunk for the specified memory. */
static struct MODS_DMA_MAP *find_dma_map(struct MODS_MEM_INFO  *p_mem_info,
					 struct mods_pci_dev_2 *pcidev)
{
	struct MODS_DMA_MAP *p_dma_map = NULL;
	struct list_head    *head      = &p_mem_info->dma_map_list;
	struct list_head    *iter;

	if (!head)
		return NULL;

	list_for_each(iter, head) {
		p_dma_map = list_entry(iter, struct MODS_DMA_MAP, list);

		if (mods_is_pci_dev(p_dma_map->dev, pcidev))
			return p_dma_map;
	}
	return NULL;
}

#if !defined(MODS_TEGRA) || defined(CONFIG_CPA)
static int mods_set_mem_type(u64 virt_addr, u64 pages, u32 type)
{
	if (type == MODS_MEMORY_UNCACHED)
		return MODS_SET_MEMORY_UC(virt_addr, pages);
	else if (type == MODS_MEMORY_WRITECOMBINE)
		return MODS_SET_MEMORY_WC(virt_addr, pages);
	return 0;
}
#endif

static int mods_restore_mem_type(u64 virt_addr,
				 u64 pages,
				 u32 type_override)
{
	if ((type_override == MODS_MEMORY_UNCACHED) ||
			(type_override == MODS_MEMORY_WRITECOMBINE)) {
		return MODS_SET_MEMORY_WB(virt_addr, pages);
	}
	return 0;
}

static void mods_restore_cache(struct MODS_MEM_INFO *p_mem_info)
{
	unsigned int i;

	for (i = 0; i < p_mem_info->num_chunks; i++) {
		struct MODS_PHYS_CHUNK *chunk = &p_mem_info->pages[i];

		if (!chunk->allocated)
			break;

		mods_pre_free(chunk, p_mem_info);
	}
}

static void mods_free_pages(struct MODS_MEM_INFO *p_mem_info)
{
	unsigned int i;

	/* release in reverse order */
	for (i = p_mem_info->num_chunks; i > 0; ) {
		struct MODS_PHYS_CHUNK *chunk;

		--i;
		chunk = &p_mem_info->pages[i];
		if (!chunk->allocated)
			continue;

		if (p_mem_info->dev) {
			u64 dev_addr = chunk->dev_addr;

			dev_addr = mods_expand_nvlink_addr(p_mem_info->dev,
							   dev_addr);

			pci_unmap_page(p_mem_info->dev,
				       dev_addr,
				       PAGE_SIZE << chunk->order,
				       DMA_BIDIRECTIONAL);
		}

#ifdef CONFIG_BIGPHYS_AREA
		if (p_mem_info->alloc_type == MODS_ALLOC_TYPE_BIGPHYS_AREA) {
			bigphysarea_free_pages((void *)
					       p_mem_info->logical_addr);
		} else
#endif
			__free_pages(chunk->p_page, chunk->order);

		chunk->allocated = 0;
	}
}

static gfp_t mods_alloc_flags(struct MODS_MEM_INFO *p_mem_info)
{
	gfp_t flags = GFP_KERNEL | __GFP_NORETRY | __GFP_NOWARN;

#ifdef MODS_HAS_DEV_TO_NUMA_NODE
	flags |= __GFP_THISNODE;
#endif

	if (p_mem_info->alloc_type != MODS_ALLOC_TYPE_NON_CONTIG)
		flags |= __GFP_COMP;
	if (p_mem_info->addr_bits == 32)
#ifdef MODS_HAS_DMA32
		flags |= __GFP_DMA32;
#else
		flags |= __GFP_DMA;
#endif
	else
		flags |= __GFP_HIGHMEM;

	return flags;
}

static int mods_alloc_contig_sys_pages(struct MODS_MEM_INFO *p_mem_info)
{
	u64 phys_addr;
	u64 end_addr  = 0;
	u32 order     = 0;

	LOG_ENT();

	while ((1U << order) < p_mem_info->num_pages)
		order++;
	p_mem_info->pages[0].order = order;

	p_mem_info->pages[0].p_page = alloc_pages_node(
			p_mem_info->numa_node,
			mods_alloc_flags(p_mem_info),
			order);

#ifdef CONFIG_BIGPHYS_AREA
	if (!p_mem_info->pages[0].p_page) {
		mods_debug_printk(DEBUG_MEM, "falling back to bigphysarea\n");
		p_mem_info->logical_addr = (u64)
			bigphysarea_alloc_pages(1U << order, 0, GFP_KERNEL);
		p_mem_info->alloc_type = MODS_ALLOC_TYPE_BIGPHYS_AREA;
	}
#endif

	if (!p_mem_info->pages[0].p_page &&
	    p_mem_info->logical_addr == 0) {
		LOG_EXT();
		return -ENOMEM;
	}

	p_mem_info->pages[0].allocated = 1;

#ifdef CONFIG_BIGPHYS_AREA
	if (p_mem_info->alloc_type == MODS_ALLOC_TYPE_BIGPHYS_AREA) {
		phys_addr = __pa(p_mem_info->logical_addr);
	} else
#endif
		phys_addr = page_to_phys(p_mem_info->pages[0].p_page);
	if (phys_addr == 0) {
		mods_error_printk("failed to determine physical address\n");
		mods_free_pages(p_mem_info);
		p_mem_info->logical_addr = 0;
		LOG_EXT();
		return -ENOMEM;
	}
	p_mem_info->pages[0].dma_addr = MODS_PHYS_TO_DMA(phys_addr);

	mods_debug_printk(DEBUG_MEM,
	    "alloc contig 0x%lx bytes%s, 2^%u pages, %s, node %d, addrbits %u, phys 0x%llx\n",
	    (unsigned long)p_mem_info->length,
	    p_mem_info->alloc_type == MODS_ALLOC_TYPE_BIGPHYS_AREA ?
		" bigphys" : "",
	    p_mem_info->pages[0].order,
	    mods_get_prot_str(p_mem_info->cache_type),
	    p_mem_info->numa_node,
	    (unsigned int)p_mem_info->addr_bits,
	    (unsigned long long)p_mem_info->pages[0].dma_addr);

	end_addr = p_mem_info->pages[0].dma_addr + p_mem_info->length;
	if ((p_mem_info->addr_bits == 32) &&
	    (end_addr > 0x100000000ULL)) {
		mods_error_printk("allocation exceeds 32-bit addressing\n");
		mods_free_pages(p_mem_info);
		p_mem_info->logical_addr = 0;
		LOG_EXT();
		return -ENOMEM;
	}

	if (mods_post_alloc(p_mem_info->pages, phys_addr, p_mem_info)) {
		mods_free_pages(p_mem_info);
		p_mem_info->logical_addr = 0;
		LOG_EXT();
		return -EINVAL;
	}
	LOG_EXT();
	return 0;
}

static u32 mods_get_max_order_needed(u32 num_pages)
{
	u32 order = 0;

	while (order < 10 && (1U<<(order+1)) <= num_pages)
		++order;
	return order;
}

static int mods_alloc_noncontig_sys_pages(struct MODS_MEM_INFO *p_mem_info)
{
	u32 pages_left = p_mem_info->num_pages;
	u32 num_chunks = 0;

	LOG_ENT();

	memset(p_mem_info->pages, 0,
	       p_mem_info->num_chunks * sizeof(p_mem_info->pages[0]));

	/* alloc pages */
	while (pages_left > 0) {
		u64 phys_addr = 0;
		u32 order     = mods_get_max_order_needed(pages_left);
		struct MODS_PHYS_CHUNK *chunk = &p_mem_info->pages[num_chunks];

		for (;;) {
			chunk->p_page = alloc_pages_node(
					p_mem_info->numa_node,
					mods_alloc_flags(p_mem_info),
					order);
			if (chunk->p_page)
				break;
			if (order == 0)
				break;
			--order;
		}

		if (!chunk->p_page) {
			mods_error_printk("out of memory\n");
			goto failed;
		}
		chunk->allocated = 1;

		pages_left -= 1U << order;
		chunk->order = order;

		phys_addr = page_to_phys(chunk->p_page);
		if (phys_addr == 0) {
			mods_error_printk("phys addr lookup failed\n");
			goto failed;
		}
		chunk->dma_addr = MODS_PHYS_TO_DMA(phys_addr);
		mods_debug_printk(DEBUG_MEM,
		    "alloc 0x%lx bytes [%u], 2^%u pages, %s, node %d, addrbits %u, phys 0x%llx\n",
		    (unsigned long)p_mem_info->length,
		    (unsigned int)num_chunks,
		    chunk->order,
		    mods_get_prot_str(p_mem_info->cache_type),
		    p_mem_info->numa_node,
		    (unsigned int)p_mem_info->addr_bits,
		    (unsigned long long)chunk->dma_addr);

		++num_chunks;

		if (mods_post_alloc(chunk, phys_addr, p_mem_info))
			goto failed;
	}

	return 0;

failed:
	mods_restore_cache(p_mem_info);
	mods_free_pages(p_mem_info);
	return -ENOMEM;
}

static int mods_register_alloc(struct file          *fp,
			       struct MODS_MEM_INFO *p_mem_info)
{
	struct mods_client *client = fp->private_data;

	if (unlikely(mutex_lock_interruptible(&client->mtx)))
		return -EINTR;
	list_add(&p_mem_info->list, &client->mem_alloc_list);
	mutex_unlock(&client->mtx);
	return OK;
}

static int mods_unregister_and_free(struct file          *fp,
				    struct MODS_MEM_INFO *p_del_mem)
{
	struct MODS_MEM_INFO *p_mem_info;
	struct mods_client   *client = fp->private_data;
	struct list_head     *head;
	struct list_head     *iter;

	mods_debug_printk(DEBUG_MEM_DETAILED, "free %p\n", p_del_mem);

	if (unlikely(mutex_lock_interruptible(&client->mtx)))
		return -EINTR;

	head = &client->mem_alloc_list;

	list_for_each(iter, head) {
		p_mem_info = list_entry(iter, struct MODS_MEM_INFO, list);

		if (p_del_mem == p_mem_info) {
			list_del(iter);

			mutex_unlock(&client->mtx);

			mods_dma_unmap_all(p_mem_info, NULL);
			mods_restore_cache(p_mem_info);
			mods_free_pages(p_mem_info);
			pci_dev_put(p_mem_info->dev);

			kfree(p_mem_info);

			return OK;
		}
	}

	mutex_unlock(&client->mtx);

	mods_error_printk("failed to unregister allocation %p\n",
			  p_del_mem);
	return -EINVAL;
}

int mods_unregister_all_alloc(struct file *fp)
{
	int                 err    = OK;
	struct mods_client *client = fp->private_data;
	struct list_head   *head   = &client->mem_alloc_list;
	struct list_head   *iter;
	struct list_head   *tmp;

	list_for_each_safe(iter, tmp, head) {
		struct MODS_MEM_INFO *p_mem_info;

		p_mem_info = list_entry(iter, struct MODS_MEM_INFO, list);
		err = mods_unregister_and_free(fp, p_mem_info);
		if (err)
			break;
	}

	return err;
}

static int get_addr_range(struct file                   *fp,
			  struct MODS_GET_ADDRESS_RANGE *p,
			  struct mods_pci_dev_2         *pcidev)
{
	struct MODS_MEM_INFO *p_mem_info;
	struct MODS_DMA_MAP  *p_dma_map = NULL;
	u64                  *out;
	u32                   num_out;
	u32                   skip_pages;
	u32                   i;
	int                   err       = OK;
	u32                   page_offs;

	LOG_ENT();

	p_mem_info = (struct MODS_MEM_INFO *)(size_t)p->memory_handle;
	if (unlikely(!p_mem_info)) {
		mods_error_printk("no allocation given\n");
		LOG_EXT();
		return -EINVAL;
	}

	if (unlikely(pcidev && (pcidev->bus > 0xFFU ||
				pcidev->device > 0xFFU))) {
		mods_error_printk("PCI device %04x:%02x:%02x.%x not found\n",
				  pcidev->domain,
				  pcidev->bus,
				  pcidev->device,
				  pcidev->function);
		LOG_EXT();
		return -EINVAL;
	}

	if (unlikely(p->stride != PAGE_SIZE)) {
		mods_error_printk("stride is 0x%x, but expected 0x%lx\n",
				  p->stride, PAGE_SIZE);
		LOG_EXT();
		return -EINVAL;
	}

	out     = &p->physical_addresses[0];
	num_out = p->num_entries;

	if (unlikely(num_out == 0 || num_out > MAX_PA_ENTRIES)) {
		mods_error_printk("invalid number of pages requested: %u\n",
				  num_out);
		LOG_EXT();
		return -EINVAL;
	}

	if (pcidev && !mods_is_pci_dev(p_mem_info->dev, pcidev)) {
		p_dma_map = find_dma_map(p_mem_info, pcidev);
		if (unlikely(!p_dma_map)) {
			mods_error_printk("allocation %p is not mapped to device %04x:%02x:%02x.%x\n",
					  p_mem_info,
					  pcidev->domain,
					  pcidev->bus,
					  pcidev->device,
					  pcidev->function);
			LOG_EXT();
			return -EINVAL;
		}
	}

	page_offs  = p->offset & (~PAGE_MASK);
	skip_pages = p->offset >> PAGE_SHIFT;

	for (i = 0; i < p_mem_info->num_chunks && num_out; i++) {
		u32 num_pages;
		u64 addr;
		struct MODS_PHYS_CHUNK *chunk = &p_mem_info->pages[i];

		num_pages = 1U << chunk->order;
		if (num_pages <= skip_pages) {
			skip_pages -= num_pages;
			continue;
		}

		addr = pcidev ?
			(p_dma_map ? p_dma_map->dev_addr[i] : chunk->dev_addr)
			: chunk->dma_addr;

		if (skip_pages) {
			num_pages -= skip_pages;
			addr += skip_pages << PAGE_SHIFT;
			skip_pages = 0;
		}

		if (num_pages > num_out)
			num_pages = num_out;

		while (num_pages) {
			*out = addr + page_offs;
			++out;
			--num_out;
			addr += PAGE_SIZE;
			--num_pages;
		}
	}

	if (unlikely(num_out)) {
		mods_error_printk("invalid offset 0x%llx or size 0x%llx requested for allocation %p\n",
				  p->offset,
				  (u64)p->stride * p->num_entries,
				  p_mem_info);
		err = -EINVAL;
	}

	LOG_EXT();
	return err;
}

/* Returns an offset within an allocation deduced from physical address.
 * If dma address doesn't belong to the allocation, returns non-zero.
 */
int mods_get_alloc_offset(struct MODS_MEM_INFO *p_mem_info,
			  u64			dma_addr,
			  u64		       *ret_offs)
{
	u32 i;
	u64 offset = 0;

	for (i = 0; i < p_mem_info->num_chunks; i++) {
		struct MODS_PHYS_CHUNK *chunk = &p_mem_info->pages[i];
		u64 addr = chunk->dma_addr;
		u32 size = PAGE_SIZE << chunk->order;

		if (dma_addr >= addr &&
		    dma_addr <  addr + size) {
			*ret_offs = dma_addr - addr + offset;
			return 0;
		}

		offset += size;
	}

	/* The physical address doesn't belong to the allocation */
	return -EINVAL;
}

struct MODS_MEM_INFO *mods_find_alloc(struct file *fp, u64 phys_addr)
{
	struct mods_client   *client     = fp->private_data;
	struct list_head     *plist_head = &client->mem_alloc_list;
	struct list_head     *plist_iter;
	struct MODS_MEM_INFO *p_mem_info;
	u64		      offset;

	list_for_each(plist_iter, plist_head) {
		p_mem_info = list_entry(plist_iter,
					struct MODS_MEM_INFO,
					list);
		if (!mods_get_alloc_offset(p_mem_info, phys_addr, &offset))
			return p_mem_info;
	}

	/* The physical address doesn't belong to any allocation */
	return NULL;
}

static u32 mods_estimate_num_chunks(u32 num_pages)
{
	u32 num_chunks = 0;
	u32 bit_scan;

	/* Count each contiguous block <=256KB */
	for (bit_scan = num_pages; bit_scan && num_chunks < 6; bit_scan >>= 1)
		++num_chunks;

	/* Count remaining contiguous blocks >256KB */
	num_chunks += bit_scan;

	/* 4x slack for medium memory fragmentation */
	num_chunks <<= 2;

	/* No sense to allocate more chunks than pages */
	if (num_chunks > num_pages)
		num_chunks = num_pages;

	/* Now, if memory is heavily fragmented, we are screwed */

	return num_chunks;
}

/* For large non-contiguous allocations, we typically use significantly less
 * chunks than originally estimated.  This function reallocates the
 * MODS_MEM_INFO struct so that it uses only as much memory as it needs.
 */
static struct MODS_MEM_INFO *optimize_chunks(struct MODS_MEM_INFO *p_mem_info)
{
	u32 i;
	u32 num_chunks;
	u32 alloc_size = 0;
	struct MODS_MEM_INFO *p_new_mem_info = NULL;

	for (i = 0; i < p_mem_info->num_chunks; i++)
		if (!p_mem_info->pages[i].allocated)
			break;

	num_chunks = i;

	if (num_chunks < p_mem_info->num_chunks) {
		alloc_size = sizeof(*p_mem_info) +
			 (num_chunks - 1) * sizeof(struct MODS_PHYS_CHUNK);

		p_new_mem_info = kzalloc(alloc_size,
					 GFP_KERNEL | __GFP_NORETRY);
	}

	if (p_new_mem_info) {
		memcpy(p_new_mem_info, p_mem_info, alloc_size);
		p_new_mem_info->num_chunks = num_chunks;
		INIT_LIST_HEAD(&p_new_mem_info->dma_map_list);
		kfree(p_mem_info);
		p_mem_info = p_new_mem_info;
	}

	return p_mem_info;
}

/************************
 * ESCAPE CALL FUNCTONS *
 ************************/

int esc_mods_device_alloc_pages_2(struct file	*fp,
				  struct MODS_DEVICE_ALLOC_PAGES_2 *p)
{
	struct MODS_MEM_INFO *p_mem_info = NULL;
	u32    num_pages;
	u32    alloc_size;
	u32    num_chunks;
	int    err = OK;
	struct pci_dev *dev = NULL;

	LOG_ENT();

	if (!p->num_bytes) {
		mods_error_printk("zero bytes requested\n");
		err = -EINVAL;
		goto failed;
	}

	mods_debug_printk(DEBUG_MEM_DETAILED,
			  "alloc 0x%x bytes %s %s on %04x:%02x:%02x.%x\n",
			  p->num_bytes,
			  p->contiguous ? "contiguous" : "noncontiguous",
			  mods_get_prot_str(p->attrib),
			  p->pci_device.domain,
			  p->pci_device.bus,
			  p->pci_device.device,
			  p->pci_device.function);

	switch (p->attrib) {
	case MODS_MEMORY_CACHED:
#if !defined(CONFIG_PPC64)
	case MODS_MEMORY_UNCACHED:
	case MODS_MEMORY_WRITECOMBINE:
#endif
		break;

	default:
		mods_error_printk("invalid memory type: %u\n",
				  p->attrib);
		err = -ENOMEM;
		goto failed;
	}

	num_pages = (u32)(((u64)p->num_bytes + PAGE_SIZE - 1) >> PAGE_SHIFT);
	if (p->contiguous)
		num_chunks = 1;
	else
		num_chunks = mods_estimate_num_chunks(num_pages);
	alloc_size = sizeof(*p_mem_info) +
		     (num_chunks - 1) * sizeof(struct MODS_PHYS_CHUNK);

	p_mem_info = kzalloc(alloc_size, GFP_KERNEL | __GFP_NORETRY);
	if (unlikely(!p_mem_info)) {
		mods_error_printk("failed to allocate auxiliary 0x%x bytes\n",
				  alloc_size);
		err = -ENOMEM;
		goto failed;
	}

	p_mem_info->num_chunks   = num_chunks;
	p_mem_info->alloc_type	 = p->contiguous
		? MODS_ALLOC_TYPE_CONTIG : MODS_ALLOC_TYPE_NON_CONTIG;
	p_mem_info->cache_type	 = p->attrib;
	p_mem_info->length	 = p->num_bytes;
	p_mem_info->logical_addr = 0;
	p_mem_info->addr_bits	 = p->address_bits;
	p_mem_info->num_pages	 = num_pages;
	p_mem_info->numa_node    = numa_node_id();
	p_mem_info->dev          = NULL;

	INIT_LIST_HEAD(&p_mem_info->dma_map_list);

	if (p->pci_device.bus <= 0xFFU && p->pci_device.device <= 0xFFU) {
		err = mods_find_pci_dev(fp, &p->pci_device, &dev);
		if (unlikely(err))
			goto failed;

		p_mem_info->dev = dev;
#if defined(MODS_HAS_DEV_TO_NUMA_NODE)
		p_mem_info->numa_node = dev_to_node(&dev->dev);
#endif
#if defined(CONFIG_PPC64) && defined(CONFIG_PCI)
		if (!mods_is_nvlink_sysmem_trained(fp, dev)) {
			/* Until NvLink is trained, we must use memory
			 * on node 0.
			 */
			if (has_npu_dev(dev, 0))
				p_mem_info->numa_node = 0;
		}
#endif
		mods_debug_printk(DEBUG_MEM_DETAILED,
				  "affinity %04x:%02x:%02x.%x node %d\n",
				  p->pci_device.domain,
				  p->pci_device.bus,
				  p->pci_device.device,
				  p->pci_device.function,
				  p_mem_info->numa_node);
	}

	p->memory_handle = 0;

	if (p->contiguous)
		err = mods_alloc_contig_sys_pages(p_mem_info);
	else {
		err = mods_alloc_noncontig_sys_pages(p_mem_info);

		if (!err)
			p_mem_info = optimize_chunks(p_mem_info);
	}

	if (err) {
		mods_error_printk(
			"failed to alloc 0x%x %s bytes, %s, node %d, addrbits %u\n",
			p_mem_info->length,
			p->contiguous ? "contiguous" : "non-contiguous",
			mods_get_prot_str(p_mem_info->cache_type),
			p_mem_info->numa_node,
			(unsigned int)p_mem_info->addr_bits);
		goto failed;
	}

	p->memory_handle = (u64)(size_t)p_mem_info;

	mods_debug_printk(DEBUG_MEM_DETAILED, "alloc %p\n", p_mem_info);

	err = mods_register_alloc(fp, p_mem_info);

failed:
	if (err) {
		if (p_mem_info) {
			mods_free_pages(p_mem_info);
			pci_dev_put(p_mem_info->dev);
		}
		kfree(p_mem_info);
	}

	LOG_EXT();
	return err;
}

int esc_mods_device_alloc_pages(struct file                    *fp,
				struct MODS_DEVICE_ALLOC_PAGES *p)
{
	int err;
	struct MODS_DEVICE_ALLOC_PAGES_2 dev_alloc_pages = {0};

	LOG_ENT();

	dev_alloc_pages.num_bytes		= p->num_bytes;
	dev_alloc_pages.contiguous		= p->contiguous;
	dev_alloc_pages.address_bits		= p->address_bits;
	dev_alloc_pages.attrib			= p->attrib;
	dev_alloc_pages.pci_device.domain	= 0;
	dev_alloc_pages.pci_device.bus		= p->pci_device.bus;
	dev_alloc_pages.pci_device.device	= p->pci_device.device;
	dev_alloc_pages.pci_device.function	= p->pci_device.function;

	err = esc_mods_device_alloc_pages_2(fp, &dev_alloc_pages);
	if (!err)
		p->memory_handle = dev_alloc_pages.memory_handle;

	LOG_EXT();
	return err;
}

int esc_mods_alloc_pages(struct file *fp, struct MODS_ALLOC_PAGES *p)
{
	int err;
	struct MODS_DEVICE_ALLOC_PAGES_2 dev_alloc_pages;

	LOG_ENT();

	dev_alloc_pages.num_bytes	    = p->num_bytes;
	dev_alloc_pages.contiguous	    = p->contiguous;
	dev_alloc_pages.address_bits	    = p->address_bits;
	dev_alloc_pages.attrib		    = p->attrib;
	dev_alloc_pages.pci_device.domain   = 0xFFFFU;
	dev_alloc_pages.pci_device.bus	    = 0xFFFFU;
	dev_alloc_pages.pci_device.device   = 0xFFFFU;
	dev_alloc_pages.pci_device.function = 0xFFFFU;

	err = esc_mods_device_alloc_pages_2(fp, &dev_alloc_pages);
	if (!err)
		p->memory_handle = dev_alloc_pages.memory_handle;

	LOG_EXT();
	return err;
}

int esc_mods_free_pages(struct file *fp, struct MODS_FREE_PAGES *p)
{
	int err;

	LOG_ENT();

	err = mods_unregister_and_free(fp,
	    (struct MODS_MEM_INFO *)(size_t)p->memory_handle);

	LOG_EXT();

	return err;
}

int esc_mods_set_mem_type(struct file *fp, struct MODS_MEMORY_TYPE *p)
{
	struct MODS_MEM_INFO *p_mem_info;
	struct mods_client   *client = fp->private_data;

	LOG_ENT();

	switch (p->type) {
	case MODS_MEMORY_CACHED:
	case MODS_MEMORY_UNCACHED:
	case MODS_MEMORY_WRITECOMBINE:
		break;

	default:
		mods_error_printk("unsupported memory type: %u\n", p->type);
		LOG_EXT();
		return -EINVAL;
	}

	if (unlikely(mutex_lock_interruptible(&client->mtx))) {
		LOG_EXT();
		return -EINTR;
	}

	p_mem_info = mods_find_alloc(fp, p->physical_address);
	if (p_mem_info) {
		mutex_unlock(&client->mtx);
		mods_error_printk("cannot set mem type on phys addr 0x%llx\n",
				  p->physical_address);
		LOG_EXT();
		return -EINVAL;
	}

	client->mem_type.dma_addr = p->physical_address;
	client->mem_type.size     = p->size;
	client->mem_type.type     = p->type;

	mutex_unlock(&client->mtx);

	LOG_EXT();
	return OK;
}

int esc_mods_get_phys_addr(struct file *fp, struct MODS_GET_PHYSICAL_ADDRESS *p)
{
	struct MODS_GET_ADDRESS_RANGE range;
	int err;

	LOG_ENT();

	range.memory_handle = p->memory_handle;
	range.offset        = p->offset;
	range.stride        = PAGE_SIZE;
	range.num_entries   = 1;
	memset(&range.pci_device, 0, sizeof(range.pci_device));

	err = get_addr_range(fp, &range, NULL);

	if (!err)
		p->physical_address = range.physical_addresses[0];

	LOG_EXT();
	return err;
}

int esc_mods_get_phys_addr_2(struct file *fp,
			     struct MODS_GET_PHYSICAL_ADDRESS_3 *p)
{
	struct MODS_GET_ADDRESS_RANGE range;
	int err;

	LOG_ENT();

	range.memory_handle = p->memory_handle;
	range.offset        = p->offset;
	range.stride        = PAGE_SIZE;
	range.num_entries   = 1;
	memset(&range.pci_device, 0, sizeof(range.pci_device));

	err = get_addr_range(fp, &range, NULL);

	if (!err)
		p->physical_address = range.physical_addresses[0];

	LOG_EXT();
	return err;
}

int esc_mods_get_phys_addr_range(struct file *fp,
				 struct MODS_GET_ADDRESS_RANGE *p)
{
	return get_addr_range(fp, p, NULL);
}

int esc_mods_get_mapped_phys_addr(struct file *fp,
				  struct MODS_GET_PHYSICAL_ADDRESS *p)
{
	struct MODS_GET_ADDRESS_RANGE range;
	struct MODS_MEM_INFO *p_mem_info;
	int err;

	LOG_ENT();

	range.memory_handle = p->memory_handle;
	range.offset        = p->offset;
	range.stride        = PAGE_SIZE;
	range.num_entries   = 1;

	p_mem_info = (struct MODS_MEM_INFO *)(size_t)p->memory_handle;
	if (p_mem_info->dev) {
		range.pci_device.domain   =
			pci_domain_nr(p_mem_info->dev->bus);
		range.pci_device.bus	   =
			p_mem_info->dev->bus->number;
		range.pci_device.device   =
			PCI_SLOT(p_mem_info->dev->devfn);
		range.pci_device.function =
			PCI_FUNC(p_mem_info->dev->devfn);

		err = get_addr_range(fp, &range, &range.pci_device);
	} else {
		memset(&range.pci_device, 0, sizeof(range.pci_device));
		err = get_addr_range(fp, &range, NULL);
	}

	if (!err)
		p->physical_address = range.physical_addresses[0];

	LOG_EXT();
	return err;
}

int esc_mods_get_mapped_phys_addr_2(struct file *fp,
				  struct MODS_GET_PHYSICAL_ADDRESS_2 *p)
{
	struct MODS_GET_ADDRESS_RANGE range;
	int err;

	LOG_ENT();

	range.memory_handle = p->memory_handle;
	range.offset        = p->offset;
	range.stride        = PAGE_SIZE;
	range.num_entries   = 1;
	range.pci_device    = p->pci_device;

	err = get_addr_range(fp, &range, &range.pci_device);

	if (!err)
		p->physical_address = range.physical_addresses[0];

	LOG_EXT();
	return err;
}

int esc_mods_get_mapped_phys_addr_3(struct file *fp,
				  struct MODS_GET_PHYSICAL_ADDRESS_3 *p)
{
	struct MODS_GET_ADDRESS_RANGE range;
	int err;

	LOG_ENT();

	range.memory_handle = p->memory_handle;
	range.offset        = p->offset;
	range.stride        = PAGE_SIZE;
	range.num_entries   = 1;
	range.pci_device    = p->pci_device;

	err = get_addr_range(fp, &range, &range.pci_device);

	if (!err)
		p->physical_address = range.physical_addresses[0];

	LOG_EXT();
	return err;
}

int esc_mods_get_dma_addr_range(struct file *fp,
				struct MODS_GET_ADDRESS_RANGE *p)
{
	return get_addr_range(fp, p, &p->pci_device);
}

int esc_mods_virtual_to_phys(struct file *fp,
			     struct MODS_VIRTUAL_TO_PHYSICAL *p)
{
	struct MODS_GET_PHYSICAL_ADDRESS get_phys_addr;
	struct mods_client *client = fp->private_data;
	struct list_head   *head;
	struct list_head   *iter;

	LOG_ENT();

	if (unlikely(mutex_lock_interruptible(&client->mtx))) {
		LOG_EXT();
		return -EINTR;
	}

	head = &client->mem_map_list;

	list_for_each(iter, head) {
		struct SYS_MAP_MEMORY *p_map_mem;
		u64		       begin, end;
		u64                    phys_offs;

		p_map_mem = list_entry(iter, struct SYS_MAP_MEMORY, list);

		begin = p_map_mem->virtual_addr;
		end   = p_map_mem->virtual_addr + p_map_mem->mapping_length;

		if (p->virtual_address >= begin && p->virtual_address < end) {

			u64 virt_offs = p->virtual_address - begin;
			int err;

			/* device memory mapping */
			if (!p_map_mem->p_mem_info) {
				p->physical_address = p_map_mem->dma_addr
						      + virt_offs;
				mutex_unlock(&client->mtx);

				mods_debug_printk(DEBUG_MEM_DETAILED,
				    "get phys: map %p virt 0x%llx -> 0x%llx\n",
				    p_map_mem, p->virtual_address,
				    p->physical_address);

				LOG_EXT();
				return OK;
			}

			if (mods_get_alloc_offset(p_map_mem->p_mem_info,
						  p_map_mem->dma_addr,
						  &phys_offs) != OK)
				break;

			get_phys_addr.memory_handle =
				(u64)(size_t)p_map_mem->p_mem_info;
			get_phys_addr.offset = virt_offs + phys_offs;

			mutex_unlock(&client->mtx);

			err = esc_mods_get_phys_addr(fp, &get_phys_addr);
			if (err)
				return err;

			p->physical_address = get_phys_addr.physical_address;

			mods_debug_printk(DEBUG_MEM_DETAILED,
			    "get phys: map %p virt 0x%llx -> 0x%llx\n",
			    p_map_mem, p->virtual_address, p->physical_address);

			LOG_EXT();
			return OK;
		}
	}

	mutex_unlock(&client->mtx);

	mods_error_printk("invalid virtual address 0x%llx\n",
			  p->virtual_address);
	return -EINVAL;
}

int esc_mods_phys_to_virtual(struct file *fp,
			     struct MODS_PHYSICAL_TO_VIRTUAL *p)
{
	struct SYS_MAP_MEMORY *p_map_mem;
	struct mods_client    *client = fp->private_data;
	struct list_head      *head;
	struct list_head      *iter;
	u64	               offset;
	u64	               map_offset;

	LOG_ENT();

	if (unlikely(mutex_lock_interruptible(&client->mtx))) {
		LOG_EXT();
		return -EINTR;
	}

	head = &client->mem_map_list;

	list_for_each(iter, head) {
		p_map_mem = list_entry(iter, struct SYS_MAP_MEMORY, list);

		/* device memory mapping */
		if (!p_map_mem->p_mem_info) {
			u64 end = p_map_mem->dma_addr
				+ p_map_mem->mapping_length;
			if (p->physical_address <  p_map_mem->dma_addr ||
			    p->physical_address >= end)
				continue;

			offset = p->physical_address
				 - p_map_mem->dma_addr;
			p->virtual_address = p_map_mem->virtual_addr
					     + offset;
			mutex_unlock(&client->mtx);

			mods_debug_printk(DEBUG_MEM_DETAILED,
			    "get virt: map %p phys 0x%llx -> 0x%llx\n",
			    p_map_mem, p->physical_address, p->virtual_address);

			LOG_EXT();
			return OK;
		}

		/* offset from the beginning of the allocation */
		if (mods_get_alloc_offset(p_map_mem->p_mem_info,
					  p->physical_address,
					  &offset))
			continue;

		/* offset from the beginning of the mapping */
		if (mods_get_alloc_offset(p_map_mem->p_mem_info,
					  p_map_mem->dma_addr,
					  &map_offset))
			continue;

		if ((offset >= map_offset) &&
		    (offset <  map_offset + p_map_mem->mapping_length)) {
			p->virtual_address = p_map_mem->virtual_addr
					   + offset - map_offset;

			mutex_unlock(&client->mtx);
			mods_debug_printk(DEBUG_MEM_DETAILED,
			    "get virt: map %p phys 0x%llx -> 0x%llx\n",
			    p_map_mem, p->physical_address, p->virtual_address);

			LOG_EXT();
			return OK;
		}
	}
	mutex_unlock(&client->mtx);
	mods_error_printk("phys addr 0x%llx is not mapped\n",
			  p->physical_address);
	return -EINVAL;
}

int esc_mods_memory_barrier(struct file *fp)
{
#if defined(CONFIG_ARM)
	/* Full memory barrier on ARMv7 */
	wmb();
	return OK;
#else
	return -EINVAL;
#endif
}

#ifdef CONFIG_PCI
int esc_mods_dma_map_memory(struct file *fp,
			    struct MODS_DMA_MAP_MEMORY *p)
{
	struct MODS_MEM_INFO *p_mem_info;
	struct MODS_DMA_MAP  *p_dma_map;
	struct pci_dev       *dev = NULL;
	int                   err = -EINVAL;

	LOG_ENT();

	p_mem_info = (struct MODS_MEM_INFO *)(size_t)p->memory_handle;
	if (unlikely(!p_mem_info)) {
		mods_error_printk("no allocation given\n");
		LOG_EXT();
		return -EINVAL;
	}

	if (mods_is_pci_dev(p_mem_info->dev, &p->pci_device)) {
		mods_debug_printk(DEBUG_MEM_DETAILED,
				  "memory %p already mapped to dev %04x:%02x:%02x.%x\n",
				  p_mem_info,
				  p->pci_device.domain,
				  p->pci_device.bus,
				  p->pci_device.device,
				  p->pci_device.function);
		LOG_EXT();
		return 0;
	}

	p_dma_map = find_dma_map(p_mem_info, &p->pci_device);
	if (p_dma_map) {
		mods_debug_printk(DEBUG_MEM_DETAILED,
				  "memory %p already mapped to dev %04x:%02x:%02x.%x\n",
				  p_mem_info,
				  p->pci_device.domain,
				  p->pci_device.bus,
				  p->pci_device.device,
				  p->pci_device.function);
		LOG_EXT();
		return 0;
	}

	err = mods_find_pci_dev(fp, &p->pci_device, &dev);
	if (unlikely(err)) {
		if (err == -ENODEV)
			mods_error_printk("PCI device %04x:%02x:%02x.%x not found\n",
					  p->pci_device.domain,
					  p->pci_device.bus,
					  p->pci_device.device,
					  p->pci_device.function);
		LOG_EXT();
		return err;
	}

	err = mods_create_dma_map(p_mem_info, dev);

	pci_dev_put(dev);
	LOG_EXT();
	return err;
}

int esc_mods_dma_unmap_memory(struct file *fp,
			      struct MODS_DMA_MAP_MEMORY *p)
{
	struct MODS_MEM_INFO *p_mem_info;
	struct pci_dev       *dev = NULL;
	int                   err = -EINVAL;

	LOG_ENT();

	p_mem_info = (struct MODS_MEM_INFO *)(size_t)p->memory_handle;
	if (unlikely(!p_mem_info)) {
		mods_error_printk("no allocation given\n");
		LOG_EXT();
		return -EINVAL;
	}

	err = mods_find_pci_dev(fp, &p->pci_device, &dev);
	if (unlikely(err)) {
		if (err == -ENODEV)
			mods_error_printk("PCI device %04x:%02x:%02x.%x not found\n",
					  p->pci_device.domain,
					  p->pci_device.bus,
					  p->pci_device.device,
					  p->pci_device.function);
	} else
		err = mods_dma_unmap_all(p_mem_info, dev);

	pci_dev_put(dev);
	LOG_EXT();
	return err;
}
#endif

#ifdef MODS_TEGRA

static void clear_contiguous_cache
(
	u64 virt_start,
	u64 phys_start,
	u32 size
)
{
	mods_debug_printk(DEBUG_MEM_DETAILED,
	    "clear cache virt 0x%llx phys 0x%llx size 0x%x\n",
	    virt_start, phys_start, size);

#ifdef CONFIG_ARM64
	/* Flush L1 cache */
	__flush_dcache_area((void *)(size_t)(virt_start), size);
#else
	/* Flush L1 cache */
	__cpuc_flush_dcache_area((void *)(size_t)(virt_start), size);

	/* Now flush L2 cache. */
	outer_flush_range(phys_start, phys_start + size);
#endif
}

static void clear_entry_cache_mappings
(
	struct SYS_MAP_MEMORY *p_map_mem,
	u64		       virt_offs,
	u64		       virt_offs_end
)
{
	struct MODS_MEM_INFO *p_mem_info = p_map_mem->p_mem_info;
	u64	     cur_vo = p_map_mem->virtual_addr;
	unsigned int i;

	if (!p_mem_info)
		return;

	if (p_mem_info->cache_type != MODS_MEMORY_CACHED)
		return;

	for (i = 0; i < p_mem_info->num_chunks; i++) {
		struct MODS_PHYS_CHUNK *chunk = &p_mem_info->pages[i];
		u32 chunk_offs     = 0;
		u32 chunk_offs_end = PAGE_SIZE << chunk->order;
		u64 cur_vo_end     = cur_vo + chunk_offs_end;

		if (virt_offs_end <= cur_vo)
			break;

		if (virt_offs >= cur_vo_end) {
			cur_vo = cur_vo_end;
			continue;
		}

		if (cur_vo < virt_offs)
			chunk_offs = (u32)(virt_offs - cur_vo);

		if (virt_offs_end < cur_vo_end)
			chunk_offs_end -= (u32)(cur_vo_end - virt_offs_end);

		mods_debug_printk(DEBUG_MEM_DETAILED,
		    "clear cache %p [%u]\n", p_mem_info, i);

		while (chunk_offs < chunk_offs_end) {
			u32 i_page     = chunk_offs >> PAGE_SHIFT;
			u32 page_offs  = chunk_offs - (i_page << PAGE_SHIFT);
			u64 page_va    =
			    (u64)(size_t)kmap(chunk->p_page + i_page);
			u64 clear_va   = page_va + page_offs;
			u64 clear_pa   = MODS_DMA_TO_PHYS(chunk->dma_addr)
							  + chunk_offs;
			u32 clear_size = PAGE_SIZE - page_offs;
			u64 remaining  = chunk_offs_end - chunk_offs;

			if ((u64)clear_size > remaining)
				clear_size = (u32)remaining;

			mods_debug_printk(DEBUG_MEM_DETAILED,
			    "clear page %u, chunk offs 0x%x, page va 0x%llx\n",
			    i_page, chunk_offs, page_va);

			clear_contiguous_cache(clear_va, clear_pa, clear_size);

			kunmap((void *)(size_t)page_va);

			chunk_offs += clear_size;
		}

		cur_vo = cur_vo_end;
	}
}

int esc_mods_flush_cpu_cache_range(struct file *fp,
				   struct MODS_FLUSH_CPU_CACHE_RANGE *p)
{
	struct mods_client *client = fp->private_data;
	struct list_head   *head;
	struct list_head   *iter;

	if (irqs_disabled() || in_interrupt() ||
	    p->virt_addr_start > p->virt_addr_end ||
	    p->flags == MODS_INVALIDATE_CPU_CACHE) {

		mods_debug_printk(DEBUG_MEM_DETAILED, "cannot clear cache\n");
		return -EINVAL;
	}

	if (unlikely(mutex_lock_interruptible(&client->mtx))) {
		LOG_EXT();
		return -EINTR;
	}

	head = &client->mem_map_list;

	list_for_each(iter, head) {
		struct SYS_MAP_MEMORY *p_map_mem
			= list_entry(iter, struct SYS_MAP_MEMORY, list);

		u64 mapped_va = p_map_mem->virtual_addr;

		/* Note: mapping end points to the first address of next range*/
		u64 mapping_end = mapped_va + p_map_mem->mapping_length;

		int start_on_page = p->virt_addr_start >= mapped_va
				    && p->virt_addr_start < mapping_end;
		int start_before_page = p->virt_addr_start < mapped_va;
		int end_on_page = p->virt_addr_end >= mapped_va
				  && p->virt_addr_end < mapping_end;
		int end_after_page = p->virt_addr_end >= mapping_end;
		u64 virt_start = p->virt_addr_start;

		/* Kernel expects end to point to the first address of next
		 * range
		 */
		u64 virt_end = p->virt_addr_end + 1;

		if ((start_on_page || start_before_page)
			&& (end_on_page || end_after_page)) {

			if (!start_on_page)
				virt_start = p_map_mem->virtual_addr;
			if (!end_on_page)
				virt_end = mapping_end;
			clear_entry_cache_mappings(p_map_mem,
						   virt_start,
						   virt_end);
		}
	}
	mutex_unlock(&client->mtx);
	return OK;
}

#endif

static int mods_post_alloc(struct MODS_PHYS_CHUNK *chunk,
			   u64			   phys_addr,
			   struct MODS_MEM_INFO   *p_mem_info)
{
	u32 num_pages = 1U << chunk->order;
	u32 i;

	for (i = 0; i < num_pages; i++) {
		u64 ptr = 0;
		int err = 0;

#ifdef CONFIG_BIGPHYS_AREA
		if (p_mem_info->alloc_type == MODS_ALLOC_TYPE_BIGPHYS_AREA) {
			ptr = p_mem_info->logical_addr + (i << PAGE_SHIFT);
		} else
#endif
			ptr = (u64)(size_t)kmap(chunk->p_page + i);
		if (!ptr) {
			mods_error_printk("kmap failed\n");
			return -EINVAL;
		}
#if defined(MODS_TEGRA) && !defined(CONFIG_CPA)
		clear_contiguous_cache(ptr,
				       phys_addr + (i << PAGE_SHIFT),
				       PAGE_SIZE);
#else
		err = mods_set_mem_type(ptr, 1, p_mem_info->cache_type);
#endif
#ifdef CONFIG_BIGPHYS_AREA
		if (p_mem_info->alloc_type != MODS_ALLOC_TYPE_BIGPHYS_AREA)
#endif
			kunmap((void *)(size_t)ptr);
		if (err) {
			mods_error_printk("set cache type failed\n");
			return -EINVAL;
		}
	}

	if (p_mem_info->dev) {
		u64 dev_addr = pci_map_page(p_mem_info->dev,
					    chunk->p_page,
					    0,
					    num_pages << PAGE_SHIFT,
					    DMA_BIDIRECTIONAL);

		if (!dev_addr) {
			struct pci_dev *dev = p_mem_info->dev;

			mods_error_printk("failed to map page to device %04x:%02x:%02x.%x\n",
					  pci_domain_nr(dev->bus),
					  dev->bus->number,
					  PCI_SLOT(dev->devfn),
					  PCI_FUNC(dev->devfn));
			return -EINVAL;
		}

		dev_addr = mods_compress_nvlink_addr(p_mem_info->dev,
						     dev_addr);
		chunk->dev_addr = dev_addr;

		mods_debug_printk(DEBUG_MEM_DETAILED,
			"auto dma map dev_addr=0x%llx, phys_addr=0x%llx on dev %04x:%02x:%02x.%x\n",
			(unsigned long long)dev_addr,
			(unsigned long long)chunk->dma_addr,
			pci_domain_nr(p_mem_info->dev->bus),
			p_mem_info->dev->bus->number,
			PCI_SLOT(p_mem_info->dev->devfn),
			PCI_FUNC(p_mem_info->dev->devfn));
	}

	return 0;
}

static void mods_pre_free(struct MODS_PHYS_CHUNK *chunk,
			  struct MODS_MEM_INFO	 *p_mem_info)
{
	u32 num_pages = 1U << chunk->order;
	u32 i;

	for (i = 0; i < num_pages; i++) {
		u64 ptr = 0;
#ifdef CONFIG_BIGPHYS_AREA
		if (p_mem_info->alloc_type == MODS_ALLOC_TYPE_BIGPHYS_AREA)
			ptr = p_mem_info->logical_addr + (i << PAGE_SHIFT);
		else
#endif
			ptr = (u64)(size_t)kmap(chunk->p_page + i);
		if (ptr)
			mods_restore_mem_type(ptr, 1, p_mem_info->cache_type);
#ifdef CONFIG_BIGPHYS_AREA
		if (p_mem_info->alloc_type != MODS_ALLOC_TYPE_BIGPHYS_AREA)
#endif
			kunmap((void *)(size_t)ptr);
	}
}
