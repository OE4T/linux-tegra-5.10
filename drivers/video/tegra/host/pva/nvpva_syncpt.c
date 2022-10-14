/*
 * Copyright (c) 2022, NVIDIA CORPORATION.  All rights reserved.
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

#include <linux/kernel.h>
#include <linux/nvhost.h>
#include <linux/nvhost_t194.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/dma-iommu.h>
#include <linux/dma-mapping.h>
#include "pva.h"

static int nvpva_map_sp(struct device *dev,
			phys_addr_t start,
			size_t size,
			dma_addr_t *sp_start,
			struct scatterlist *sg,
			u32 attr)
{
	int err = 0;

	/* If IOMMU is enabled, map it into the device memory */
	if (iommu_get_domain_for_dev(dev)) {

		/* Initialize the scatterlist to cover the whole range */
		sg_init_table(sg, 1);
		sg_set_page(sg, phys_to_page(start), size, 0);

		err = dma_map_sg_attrs(dev,
				       sg,
				       1,
				       attr,
				       DMA_ATTR_SKIP_CPU_SYNC);

		/* dma_map_sg_attrs returns 0 on errors */
		if (err == 0) {
			err = -ENOMEM;
			goto out;
		}

		*sp_start = sg_dma_address(sg);
		err = 0;
	}
out:
	return err;
}

void nvpva_syncpt_put_ref_ext(struct platform_device *pdev,
			       u32 id)
{
	struct nvhost_device_data *pdata = platform_get_drvdata(pdev);
	struct pva *pva = pdata->private_data;
	int i;

	if (pva->version == PVA_HW_GEN1) {
		nvhost_syncpt_put_ref_ext(pdev, id);
		return;
	}

	for (i = 0; i < MAX_PVA_QUEUE_COUNT; i++) {
		if (pva->syncpts.syncpts_rw[i].id == id) {
			pva->syncpts.syncpts_rw[i].assigned = 0;
			break;
		}
	}
}

u32 nvpva_get_syncpt_client_managed(struct platform_device *pdev,
				    const char *syncpt_name)
{
	struct nvhost_device_data *pdata = platform_get_drvdata(pdev);
	struct pva *pva = pdata->private_data;
	u32 id = 0;
	int i;

	if (pva->version == PVA_HW_GEN1) {
		id = nvhost_get_syncpt_client_managed(pdev, "pva_syncpt");
		goto out;
	}

	for (i = 0; i < MAX_PVA_QUEUE_COUNT; i++) {
		if (pva->syncpts.syncpts_rw[i].assigned == 0) {
			id = pva->syncpts.syncpts_rw[i].id;
			pva->syncpts.syncpts_rw[i].assigned = 1;
			break;
		}
	}
out:
	return id;
}

dma_addr_t
nvpva_syncpt_address(struct platform_device *pdev, u32 id, bool rw)
{
	struct nvhost_device_data *pdata = platform_get_drvdata(pdev);
	struct pva *pva = pdata->private_data;
	struct platform_device *host_pdev = pva->syncpts.host_pdev;
	dma_addr_t addr = 0;
	u32 offset = 0;
	int i;

	if (pva->version == PVA_HW_GEN1) {
		addr =  nvhost_syncpt_address(pdev, id);
		goto out;
	}

	if (!rw) {
		offset = nvhost_syncpt_unit_interface_get_byte_offset_ext(host_pdev, id);
		addr = pva->syncpts.syncpt_start_iova_r + (dma_addr_t)offset;
		goto out;
	}

	for (i = 0; i < MAX_PVA_QUEUE_COUNT; i++) {
		if (pva->syncpts.syncpts_rw[i].id == id) {
			addr = pva->syncpts.syncpts_rw[i].addr;
			break;
		}
	}
out:
	nvpva_dbg_info(pva,
		       "syncpt_addr:  id: %d   addr: %llx offset: %llx\n",
		       id,
		       addr,
		       (u64)offset);

	return addr;
}

void nvpva_syncpt_unit_interface_deinit(struct platform_device *pdev)
{
	struct nvhost_device_data *pdata = platform_get_drvdata(pdev);
	struct pva *pva = pdata->private_data;
	int i;

	if (!pva->syncpts.syncpts_mapped_r)
		goto out;

	if (pva->version == PVA_HW_GEN1) {
		pva->syncpts.syncpts_mapped_rw = false;
		pva->syncpts.syncpts_mapped_r = false;
		goto out;
	}

	dma_unmap_sg_attrs(&pdev->dev,
			   pva->syncpts.sg,
			   1,
			   DMA_TO_DEVICE,
			   DMA_ATTR_SKIP_CPU_SYNC);
	pva->syncpts.syncpts_mapped_r = false;
	kfree(pva->syncpts.sg);
	pva->syncpts.sg = NULL;

	for (i = 0; i < MAX_PVA_QUEUE_COUNT; i++) {
		if (pva->syncpts.syncpts_rw[i].id == 0)
			continue;

		dma_unmap_sg_attrs(&pdev->dev,
				   pva->syncpts.syncpts_rw[i].sg,
				   1,
				   DMA_BIDIRECTIONAL,
				   DMA_ATTR_SKIP_CPU_SYNC);
		kfree(pva->syncpts.syncpts_rw[i].sg);
		pva->syncpts.syncpts_rw[i].sg = NULL;
		pva->syncpts.syncpts_rw[i].assigned = 0;
		nvhost_syncpt_put_ref_ext(pdev,
					  pva->syncpts.syncpts_rw[i].id);
		pva->syncpts.syncpts_rw[i].id = 0;
	}

	pva->syncpts.syncpts_mapped_rw = false;
out:
	return;
}

int nvpva_syncpt_unit_interface_init(struct platform_device *pdev)
{
	struct nvhost_device_data *pdata = platform_get_drvdata(pdev);
	struct pva *pva = pdata->private_data;
	phys_addr_t base;
	size_t size;
	dma_addr_t syncpt_addr_rw;
	u32 syncpt_offset;
	int err = 0;
	int i;
	u32 id = 0;

	if ((pva->syncpts.syncpts_mapped_r)
	||  (pva->syncpts.syncpts_mapped_rw))
		goto out;

	if (pva->version == PVA_HW_GEN1) {
		pva->syncpts.syncpt_start_iova_r = 0;
		pva->syncpts.syncpt_range_r = 0;
		pva->syncpts.page_size = 0;
		pva->syncpts.syncpts_mapped_r = true;
		pva->syncpts.syncpts_mapped_rw = true;
		pva->syncpts.syncpt_start_iova_rw = 0;
		pva->syncpts.syncpt_range_rw = 0;
		goto out;
	}

	syncpt_offset = nvhost_syncpt_unit_interface_get_byte_offset_ext(pdev, 1);
#ifdef TEGRA_OOT_MODULE
	pva->syncpts.host_pdev = pdev;
#else
	pva->syncpts.host_pdev = to_platform_device(pdev->dev.parent);
#endif
	err = nvhost_syncpt_unit_interface_get_aperture(pva->syncpts.host_pdev,
							&base,
							&size);
	if (err) {
		dev_err(&pdev->dev, "failed to get aperture");
		goto out;
	}

	pva->syncpts.sg = kzalloc(sizeof(struct scatterlist), GFP_KERNEL);
	if (pva->syncpts.sg == NULL) {
		dev_err(&pdev->dev, "failed to allocate mem for sg list\n");
		err = -ENOMEM;
		goto out;
	}

	err = nvpva_map_sp(&pdev->dev,
			   base,
			   size,
			   &syncpt_addr_rw,
			   pva->syncpts.sg,
			   DMA_TO_DEVICE);
	if (err)
		goto out;

	pva->syncpts.syncpt_start_iova_r = syncpt_addr_rw;
	pva->syncpts.syncpt_range_r  = size;
	pva->syncpts.page_size = syncpt_offset;
	pva->syncpts.syncpts_mapped_r = true;

	nvpva_dbg_info(pva,
		       "syncpt_start_iova %llx,  size %llx\n",
			pva->syncpts.syncpt_start_iova_rw,
			pva->syncpts.syncpt_range_r);

	for (i = 0; i < MAX_PVA_QUEUE_COUNT; i++) {
		pva->syncpts.syncpts_rw[i].sg = kzalloc(sizeof(struct scatterlist), GFP_KERNEL);
		if (pva->syncpts.syncpts_rw[i].sg == NULL) {
			dev_err(&pdev->dev, "failed to allocate mem for sg list\n");
			err = -ENOMEM;
			goto error_alloc_sg;
		}

		id = nvhost_get_syncpt_client_managed(pdev, "pva_syncpt");
		if (id == 0) {
			dev_err(&pdev->dev, "failed to get syncpt\n");
			err = -ENOMEM;
			goto err_alloc_syncpt;
		}

		syncpt_offset = nvhost_syncpt_unit_interface_get_byte_offset_ext(pdev, id);
		err = nvpva_map_sp(&pdev->dev,
				   (base + syncpt_offset),
				   pva->syncpts.page_size,
				   &syncpt_addr_rw,
				   pva->syncpts.syncpts_rw[i].sg,
				   DMA_BIDIRECTIONAL);
		if (err) {
			dev_err(&pdev->dev, "failed to map syncpt %d\n", id);
			goto err_map_sp;
		}

		pva->syncpts.syncpts_rw[i].addr = syncpt_addr_rw;
		pva->syncpts.syncpts_rw[i].id = id;
		pva->syncpts.syncpts_rw[i].assigned = 0;
		nvpva_dbg_info(pva,
			"syncpt_addr:  id: %d   addr: %llx offset: %llx\n",
			id,
			syncpt_addr_rw,
			0LLU);
	}

	pva->syncpts.syncpts_mapped_rw = true;
	syncpt_addr_rw = pva->syncpts.syncpts_rw[MAX_PVA_QUEUE_COUNT - 1].addr;
	pva->syncpts.syncpt_start_iova_rw = syncpt_addr_rw;
	pva->syncpts.syncpt_range_rw = MAX_PVA_QUEUE_COUNT *
				       (pva->syncpts.syncpts_rw[0].addr -
					pva->syncpts.syncpts_rw[1].addr);

	if (pva->version == PVA_HW_GEN1)
		goto out;

	if (syncpt_addr_rw % (pva->syncpts.syncpt_range_rw) != 0) {
		dev_err(&pdev->dev, "RW sync pts base not aligned to 512k");
		err = -ENOMEM;
		goto err_map_sp;
	}

	syncpt_addr_rw += (MAX_PVA_QUEUE_COUNT - 1) * pva->syncpts.page_size;
	if (syncpt_addr_rw != pva->syncpts.syncpts_rw[0].addr) {
		dev_err(&pdev->dev, "RW sync pts not contiguous");
		err = -ENOMEM;
		goto err_map_sp;
	}

	goto out;

err_map_sp:
err_alloc_syncpt:
error_alloc_sg:
	nvpva_syncpt_unit_interface_deinit(pdev);
out:
	return err;
}
