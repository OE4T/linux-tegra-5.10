/*
 * Host1x Application Specific Virtual Memory
 *
 * Copyright (c) 2015-2019, NVIDIA Corporation.  All rights reserved.
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

#include <linux/dma-mapping.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include <linux/module.h>
#include <linux/iommu.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/of.h>
#include <linux/version.h>
#include <linux/dma-buf.h>
#include <linux/nvhost.h>

#include <iommu_context_dev.h>

#include "chip_support.h"

static struct of_device_id tegra_iommu_context_dev_of_match[] = {
	{ .compatible = "nvidia,tegra186-iommu-context" },
	{ },
};

struct iommu_static_mapping {
	struct list_head list;
	dma_addr_t paddr;
	void *vaddr;
	size_t size;
};

struct iommu_ctx {
	struct nvhost_device_data pdata;
	struct platform_device *pdev;
	struct list_head list;
	struct device_dma_parameters dma_parms;
	bool allocated;
	void *prev_identifier;
};

static LIST_HEAD(iommu_ctx_list);
static DEFINE_MUTEX(iommu_ctx_list_mutex);

struct platform_device *iommu_context_dev_allocate(void *identifier)
{
	struct iommu_ctx *ctx, *ctx_new = NULL;
	bool dirty = false;

	mutex_lock(&iommu_ctx_list_mutex);
	/*
	 * First check if we have same identifier stashed into
	 * some context device
	 * If yes, use that context device since it will have all
	 * the mappings stashed too
	 */
	list_for_each_entry(ctx, &iommu_ctx_list, list) {
		if (!ctx->allocated && identifier == ctx->prev_identifier) {
			ctx->allocated = true;
			mutex_unlock(&iommu_ctx_list_mutex);
			return ctx->pdev;
		}
	}

	/*
	 * Otherwise, find a device which does not have any identifier stashed
	 * If there is no device left without identifier stashed, use any of
	 * the free device and explicitly remove all the stashings from it
	 */
	list_for_each_entry(ctx, &iommu_ctx_list, list) {
		if (!ctx->allocated && !ctx_new) {
			ctx_new = ctx;
			dirty = true;
		}
		if (!ctx->allocated && !ctx->prev_identifier) {
			ctx_new = ctx;
			dirty = false;
			break;
		}
	}

	if (ctx_new) {
#ifdef CONFIG_NVMAP
		if (dirty) {
			/*
			 * Ensure that all stashed mappings are removed from this context device
			 * before this context device gets reassigned to some other process
			 */
			dma_buf_release_stash(&ctx_new->pdev->dev);
		}
#endif
		ctx_new->prev_identifier = identifier;
		ctx_new->allocated = true;
		mutex_unlock(&iommu_ctx_list_mutex);
		return ctx_new->pdev;
	}

	mutex_unlock(&iommu_ctx_list_mutex);

	return NULL;
}

void iommu_context_dev_release(struct platform_device *pdev)
{
	struct iommu_ctx *ctx = platform_get_drvdata(pdev);

	mutex_lock(&iommu_ctx_list_mutex);
	ctx->allocated = false;
	mutex_unlock(&iommu_ctx_list_mutex);
}

static int iommu_context_dev_probe(struct platform_device *pdev)
{
	struct iommu_ctx *ctx;

	if (!nvhost_get_chip_ops()) {
		dev_err(&pdev->dev, "nvhost was not initialized. aborting.");
		return -ENODEV;
	}

	if (!pdev->dev.archdata.iommu) {
		dev_err(&pdev->dev, "iommu is not enabled for context device. aborting.");
		return -ENOSYS;
	}

	ctx = devm_kzalloc(&pdev->dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx) {
		dev_err(&pdev->dev,
			   "%s: could not allocate iommu ctx\n", __func__);
		return -ENOMEM;
	}

	INIT_LIST_HEAD(&ctx->list);
	ctx->pdev = pdev;

	mutex_lock(&iommu_ctx_list_mutex);
	list_add_tail(&ctx->list, &iommu_ctx_list);
	mutex_unlock(&iommu_ctx_list_mutex);

	platform_set_drvdata(pdev, ctx);

	pdev->dev.dma_parms = &ctx->dma_parms;
	dma_set_max_seg_size(&pdev->dev, UINT_MAX);

#ifdef CONFIG_NVMAP
	/* flag required to handle stashings in context devices */
	pdev->dev.context_dev = true;
#endif

	dev_info(&pdev->dev, "initialized (streamid=%d)",
		 iommu_get_hwid(pdev->dev.archdata.iommu, &pdev->dev, 0));

	if (vm_op().init_syncpt_interface)
		vm_op().init_syncpt_interface(pdev);

	return 0;
}

static int __exit iommu_context_dev_remove(struct platform_device *pdev)
{
	struct iommu_ctx *ctx = platform_get_drvdata(pdev);

	mutex_lock(&iommu_ctx_list_mutex);
	list_del(&ctx->list);
	mutex_unlock(&iommu_ctx_list_mutex);

	return 0;
}

static struct platform_driver iommu_context_dev_driver = {
	.probe = iommu_context_dev_probe,
	.remove = __exit_p(iommu_context_dev_remove),
	.driver = {
		.owner = THIS_MODULE,
		.name = "iommu_context_dev",
#ifdef CONFIG_OF
		.of_match_table = tegra_iommu_context_dev_of_match,
#endif
	},
};

static int __init iommu_context_dev_init(void)
{
	return platform_driver_register(&iommu_context_dev_driver);
}

static void __exit iommu_context_dev_exit(void)
{
	platform_driver_unregister(&iommu_context_dev_driver);
}

module_init(iommu_context_dev_init);
module_exit(iommu_context_dev_exit);

