/*
 * drivers/video/tegra/nvmap/nvmap_init_t19x.c
 *
 * Copyright (c) 2016-2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#define pr_fmt(fmt)	"nvmap: %s() " fmt, __func__

#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>
#include <linux/nvmap_t19x.h>
#include <linux/io.h>

#include "nvmap_priv.h"

bool nvmap_version_t19x;
extern struct static_key nvmap_updated_cache_config;

struct gosmem_priv {
	struct device *dev;
	void *cpu_addr;
	void *memremap_addr;
	dma_addr_t dma_addr;
	int cvdevs;
};
static struct gosmem_priv *gos;

const struct of_device_id nvmap_of_ids[] = {
	{ .compatible = "nvidia,carveouts" },
	{ .compatible = "nvidia,carveouts-t18x" },
	{ .compatible = "nvidia,carveouts-t19x" },
	{ }
};

int nvmap_register_cvsram_carveout(struct device *dma_dev,
		phys_addr_t base, size_t size, int (*busy)(void),
		int (*idle)(void))
{
	static struct nvmap_platform_carveout cvsram = {
		.name = "cvsram",
		.usage_mask = NVMAP_HEAP_CARVEOUT_CVSRAM,
		.disable_dynamic_dma_map = true,
		.no_cpu_access = true,
	};

	cvsram.pm_ops.busy = busy;
	cvsram.pm_ops.idle = idle;

	if (!base || !size || (base != PAGE_ALIGN(base)) ||
	    (size != PAGE_ALIGN(size)))
		return -EINVAL;
	cvsram.base = base;
	cvsram.size = size;

	cvsram.dma_dev = &cvsram.dev;
	return nvmap_create_carveout(&cvsram);
}
EXPORT_SYMBOL(nvmap_register_cvsram_carveout);

static struct cv_dev_info *cvdev_info;

static void nvmap_gosmem_device_release(struct reserved_mem *rmem,
		struct device *dev)
{
	int cvdev_count = gos->cvdevs;
	struct sg_table *sgt;
	int i;

	sgt = (struct sg_table *)(cvdev_info + cvdev_count);

	for (i = 0; i < (cvdev_count * cvdev_count); i++)
		sg_free_table(sgt++);

	for (i = 0; i < cvdev_count; i++)
		of_node_put(cvdev_info[i].np);

	memunmap(gos->memremap_addr);
	dma_free_coherent(gos->dev, cvdev_count * SZ_4K, gos->cpu_addr,
			gos->dma_addr);

	kfree(cvdev_info);
	cvdev_info = NULL;
	kfree(gos);
}

static int __init nvmap_gosmem_device_init(struct reserved_mem *rmem,
		struct device *dev)
{
	struct of_phandle_args outargs;
	struct device_node *np;
	DEFINE_DMA_ATTRS(attrs);
	int ret = 0, i, idx, bytes;
	struct sg_table *sgt;
	int cvdev_count;

	dma_set_attr(DMA_ATTR_ALLOC_EXACT_SIZE, __DMA_ATTR(attrs));

	np = of_find_node_by_phandle(rmem->phandle);
	if (!np) {
		pr_err("Can't find the node using compatible\n");
		return -ENODEV;
	}

	if (!of_device_is_available(np)) {
		dev_err(dev, "device is disabled\n");
		return -ENODEV;
	}

	gos = kzalloc(sizeof(*gos), GFP_KERNEL);
	if (!gos)
		return -ENOMEM;

	gos->cvdevs = of_count_phandle_with_args(np, "cvdevs", NULL);
	if (!gos->cvdevs) {
		pr_err("No cvdevs to use the gosmem!!\n");
		ret = -EINVAL;
		goto free_gos;
	}
	cvdev_count = gos->cvdevs;

	gos->cpu_addr = dma_alloc_coherent(dev, cvdev_count * SZ_4K,
				&gos->dma_addr, GFP_KERNEL);
	if (!gos->cpu_addr) {
		pr_err("Failed to allocate from Gos mem carveout\n");
		ret = -ENOMEM;
		goto free_gos;
	}

	gos->memremap_addr = memremap(virt_to_phys(gos->cpu_addr),
			cvdev_count * SZ_4K, MEMREMAP_WB);

	bytes = sizeof(*cvdev_info) * cvdev_count;
	bytes += sizeof(struct sg_table) * cvdev_count * cvdev_count;
	cvdev_info = kzalloc(bytes, GFP_KERNEL);
	if (!cvdev_info) {
		pr_err("kzalloc failed. No memory!!!\n");
		ret = -ENOMEM;
		goto unmap_dma;
	}

	for (idx = 0; idx < cvdev_count; idx++) {
		struct device_node *temp;

		ret = of_parse_phandle_with_args(np, "cvdevs",
			NULL, idx, &outargs);
		if (ret < 0) {
			/* skip empty (null) phandles */
			if (ret == -ENOENT)
				continue;
			else
				goto free_cvdev;
		}
		temp = outargs.np;

		spin_lock_init(&cvdev_info[idx].goslock);
		cvdev_info[idx].np = of_node_get(temp);
		if (!cvdev_info[idx].np)
			continue;
		cvdev_info[idx].count = cvdev_count;
		cvdev_info[idx].idx = idx;
		cvdev_info[idx].sgt =
			(struct sg_table *)(cvdev_info + cvdev_count);
		cvdev_info[idx].sgt += idx * cvdev_count;
		cvdev_info[idx].cpu_addr = gos->memremap_addr + idx * SZ_4K;

		for (i = 0; i < cvdev_count; i++) {
			sgt = cvdev_info[idx].sgt + i;

			ret = sg_alloc_table(sgt, 1, GFP_KERNEL);
			if (ret) {
				pr_err("sg_alloc_table failed:%d\n", ret);
				goto free;
			}
			sg_set_buf(sgt->sgl,
				(gos->memremap_addr + i * SZ_4K), SZ_4K);
		}
	}

	return 0;
free:
	sgt = (struct sg_table *)(cvdev_info + cvdev_count);
	for (i = 0; i < cvdev_count * cvdev_count; i++)
		sg_free_table(sgt++);
free_cvdev:
	kfree(cvdev_info);
	cvdev_info = NULL;
	cvdev_count = 0;
unmap_dma:
	memunmap(gos->memremap_addr);
	dma_free_coherent(dev, cvdev_count * SZ_4K, gos->cpu_addr,
				gos->dma_addr);
free_gos:
	kfree(gos);
	gos = NULL;
	return ret;
}

static struct reserved_mem_ops gosmem_rmem_ops = {
	.device_init = nvmap_gosmem_device_init,
	.device_release = nvmap_gosmem_device_release,
};

static int __init nvmap_gosmem_setup(struct reserved_mem *rmem)
{
	rmem->priv = NULL;
	rmem->ops = &gosmem_rmem_ops;

	return 0;
}
RESERVEDMEM_OF_DECLARE(nvmap_gosmem, "nvidia,gosmem", nvmap_gosmem_setup);

struct cv_dev_info *nvmap_fetch_cv_dev_info(struct device *dev);

static int nvmap_gosmem_notifier(struct notifier_block *nb,
		unsigned long event, void *_dev)
{
	struct device *dev = _dev;
	int ents, i;
	struct cv_dev_info *gos_owner;

	if ((event != BUS_NOTIFY_BOUND_DRIVER) &&
		(event != BUS_NOTIFY_UNBIND_DRIVER))
		return NOTIFY_DONE;

	if ((event == BUS_NOTIFY_BOUND_DRIVER) &&
		nvmap_dev && (dev == nvmap_dev->dev_user.parent)) {
		struct of_device_id nvmap_t19x_of_ids[] = {
			{.compatible = "nvidia,carveouts-t19x"},
			{ }
		};

		/*
		 * user space IOCTL and dmabuf ops happen much later in boot
		 * flow. So, setting the version here to ensure all of those
		 * callbacks can safely query the proper version of nvmap
		 */
		if (of_match_node((struct of_device_id *)&nvmap_t19x_of_ids,
				dev->of_node))
			nvmap_version_t19x = 1;
		static_key_slow_inc(&nvmap_updated_cache_config);
		return NOTIFY_DONE;
	}

	gos_owner = nvmap_fetch_cv_dev_info(dev);
	if (!gos_owner)
		return NOTIFY_DONE;

	for (i = 0; i < gos->cvdevs; i++) {
		DEFINE_DMA_ATTRS(attrs);
		enum dma_data_direction dir;

		dir = DMA_BIDIRECTIONAL;
		dma_set_attr(DMA_ATTR_SKIP_IOVA_GAP, __DMA_ATTR(attrs));
		if (cvdev_info[i].np != dev->of_node) {
			dma_set_attr(DMA_ATTR_READ_ONLY, __DMA_ATTR(attrs));
			dir = DMA_TO_DEVICE;
		}

		switch (event) {
		case BUS_NOTIFY_BOUND_DRIVER:
			ents = dma_map_sg_attrs(dev, gos_owner->sgt[i].sgl,
					gos_owner->sgt[i].nents, dir, __DMA_ATTR(attrs));
			if (ents != 1) {
				pr_err("mapping gosmem chunk %d for %s failed\n",
					i, dev_name(dev));
				return NOTIFY_DONE;
			}
			break;
		case BUS_NOTIFY_UNBIND_DRIVER:
			dma_unmap_sg_attrs(dev, gos_owner->sgt[i].sgl,
					gos_owner->sgt[i].nents, dir, __DMA_ATTR(attrs));
			break;
		default:
			return NOTIFY_DONE;
		};
	}
	return NOTIFY_DONE;
}

static struct notifier_block nvmap_gosmem_nb = {
	.notifier_call = nvmap_gosmem_notifier,
};

static int nvmap_t19x_init(void)
{
	return bus_register_notifier(&platform_bus_type,
			&nvmap_gosmem_nb);
}
core_initcall(nvmap_t19x_init);

struct cv_dev_info *nvmap_fetch_cv_dev_info(struct device *dev)
{
	int i;

	if (!dev || !cvdev_info || !dev->of_node)
		return NULL;

	for (i = 0; i < gos->cvdevs; i++)
		if (cvdev_info[i].np == dev->of_node)
			return &cvdev_info[i];
	return NULL;
}

int nvmap_alloc_gos_slot(struct device *dev,
		u32 *return_index,
		u32 *return_offset,
		u32 **return_address)
{
	u32 offset;
	int i;

	for (i = 0; i < gos->cvdevs; i++) {
		if (cvdev_info[i].np != dev->of_node)
			continue;

		spin_lock(&cvdev_info[i].goslock);

		offset = find_first_zero_bit(cvdev_info[i].gosmap, NVMAP_MAX_GOS_COUNT);
		if (offset < NVMAP_MAX_GOS_COUNT)
			set_bit(offset, cvdev_info[i].gosmap);

		spin_unlock(&cvdev_info[i].goslock);

		if (offset >= NVMAP_MAX_GOS_COUNT)
			continue;

		*return_index = cvdev_info[i].idx;
		*return_offset = offset;
		*return_address = (u32 *)
			(cvdev_info[i].cpu_addr + (offset * sizeof(u32)));

		return 0;
	}

	return -EBUSY;
}

void nvmap_free_gos_slot(u32 index, u32 offset)
{
	if (WARN_ON(index >= gos->cvdevs) ||
		WARN_ON(offset >= NVMAP_MAX_GOS_COUNT))
		return;

	spin_lock(&cvdev_info[index].goslock);

	clear_bit(offset, cvdev_info[index].gosmap);

	spin_unlock(&cvdev_info[index].goslock);
}
