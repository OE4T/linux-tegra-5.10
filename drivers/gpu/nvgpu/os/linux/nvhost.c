/*
 * Copyright (c) 2017-2020, NVIDIA CORPORATION.  All rights reserved.
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

#include <linux/nvhost.h>
#include <linux/nvhost_t194.h>
#include <linux/dma-mapping.h>
#include <uapi/linux/nvhost_ioctl.h>
#include <linux/of_platform.h>

#include <nvgpu/gk20a.h>
#include <nvgpu/nvhost.h>
#include <nvgpu/enabled.h>
#include <nvgpu/dma.h>

#include "nvhost_priv.h"

#include "os_linux.h"
#include "module.h"

int nvgpu_get_nvhost_dev(struct gk20a *g)
{
	struct device_node *np = nvgpu_get_node(g);
	struct platform_device *host1x_pdev = NULL;
	const __be32 *host1x_ptr;

	host1x_ptr = of_get_property(np, "nvidia,host1x", NULL);
	if (host1x_ptr) {
		struct device_node *host1x_node =
			of_find_node_by_phandle(be32_to_cpup(host1x_ptr));

		host1x_pdev = of_find_device_by_node(host1x_node);
		if (!host1x_pdev) {
			nvgpu_warn(g, "host1x device not available");
			return -EPROBE_DEFER;
		}

	} else {
		if (nvgpu_has_syncpoints(g)) {
			nvgpu_warn(g, "host1x reference not found. assuming no syncpoints support");
			nvgpu_set_enabled(g, NVGPU_HAS_SYNCPOINTS, false);
		}
		return 0;
	}

	g->nvhost = nvgpu_kzalloc(g, sizeof(struct nvgpu_nvhost_dev));
	if (!g->nvhost)
		return -ENOMEM;

	g->nvhost->host1x_pdev = host1x_pdev;

	return 0;
}

void nvgpu_free_nvhost_dev(struct gk20a *g)
{
	if (nvgpu_iommuable(g) && !nvgpu_is_enabled(g, NVGPU_SUPPORT_NVLINK)) {
		struct device *dev = dev_from_gk20a(g);
		struct nvgpu_mem *mem = &g->syncpt_mem;

		dma_unmap_sg_attrs(dev, mem->priv.sgt->sgl, 1,
				DMA_BIDIRECTIONAL, DMA_ATTR_SKIP_CPU_SYNC);
		sg_free_table(mem->priv.sgt);
		nvgpu_kfree(g, mem->priv.sgt);
	}
	nvgpu_kfree(g, g->nvhost);
}

bool nvgpu_has_syncpoints(struct gk20a *g)
{
	struct nvgpu_os_linux *l = nvgpu_os_linux_from_gk20a(g);

	return nvgpu_is_enabled(g, NVGPU_HAS_SYNCPOINTS) &&
		!l->disable_syncpoints;
}

int nvgpu_nvhost_module_busy_ext(
	struct nvgpu_nvhost_dev *nvhost_dev)
{
	return nvhost_module_busy_ext(nvhost_dev->host1x_pdev);
}

void nvgpu_nvhost_module_idle_ext(
	struct nvgpu_nvhost_dev *nvhost_dev)
{
	nvhost_module_idle_ext(nvhost_dev->host1x_pdev);
}

void nvgpu_nvhost_debug_dump_device(
	struct nvgpu_nvhost_dev *nvhost_dev)
{
	nvhost_debug_dump_device(nvhost_dev->host1x_pdev);
}

const char *nvgpu_nvhost_syncpt_get_name(
	struct nvgpu_nvhost_dev *nvhost_dev, int id)
{
	return nvhost_syncpt_get_name(nvhost_dev->host1x_pdev, id);
}

bool nvgpu_nvhost_syncpt_is_valid_pt_ext(
	struct nvgpu_nvhost_dev *nvhost_dev, u32 id)
{
	return nvhost_syncpt_is_valid_pt_ext(nvhost_dev->host1x_pdev, id);
}

bool nvgpu_nvhost_syncpt_is_expired_ext(
	struct nvgpu_nvhost_dev *nvhost_dev, u32 id, u32 thresh)
{
	return nvhost_syncpt_is_expired_ext(nvhost_dev->host1x_pdev,
			id, thresh);
}

int nvgpu_nvhost_intr_register_notifier(
	struct nvgpu_nvhost_dev *nvhost_dev, u32 id, u32 thresh,
	void (*callback)(void *, int), void *private_data)
{
	return nvhost_intr_register_notifier(nvhost_dev->host1x_pdev,
			id, thresh,
			callback, private_data);
}

void nvgpu_nvhost_syncpt_set_minval(
	struct nvgpu_nvhost_dev *nvhost_dev, u32 id, u32 val)
{
	nvhost_syncpt_set_minval(nvhost_dev->host1x_pdev, id, val);
}

void nvgpu_nvhost_syncpt_put_ref_ext(
	struct nvgpu_nvhost_dev *nvhost_dev, u32 id)
{
	nvhost_syncpt_put_ref_ext(nvhost_dev->host1x_pdev, id);
}

u32 nvgpu_nvhost_get_syncpt_client_managed(
	struct nvgpu_nvhost_dev *nvhost_dev,
	const char *syncpt_name)
{
	return nvhost_get_syncpt_client_managed(nvhost_dev->host1x_pdev,
			syncpt_name);
}

int nvgpu_nvhost_syncpt_wait_timeout_ext(
	struct nvgpu_nvhost_dev *nvhost_dev, u32 id,
	u32 thresh, u32 timeout, u32 waiter_index)
{
	return nvhost_syncpt_wait_timeout_ext(nvhost_dev->host1x_pdev,
		id, thresh, timeout, NULL, NULL);
}

int nvgpu_nvhost_syncpt_read_ext_check(
	struct nvgpu_nvhost_dev *nvhost_dev, u32 id, u32 *val)
{
	return nvhost_syncpt_read_ext_check(nvhost_dev->host1x_pdev, id, val);
}

void nvgpu_nvhost_syncpt_set_safe_state(
	struct nvgpu_nvhost_dev *nvhost_dev, u32 id)
{
	u32 val = 0;
	int err;

	/*
	 * Add large number of increments to current value
	 * so that all waiters on this syncpoint are released
	 *
	 * We don't expect any case where more than 0x10000 increments
	 * are pending
	 */
	err = nvhost_syncpt_read_ext_check(nvhost_dev->host1x_pdev,
			id, &val);
	if (err != 0) {
		pr_err("%s: syncpt id read failed, cannot reset for safe state",
				__func__);
	} else {
		val += 0x10000;
		nvhost_syncpt_set_minval(nvhost_dev->host1x_pdev, id, val);
	}
}

int nvgpu_nvhost_create_symlink(struct gk20a *g)
{
	struct device *dev = dev_from_gk20a(g);
	int err = 0;

	if (g->nvhost &&
			(dev->parent != &g->nvhost->host1x_pdev->dev)) {
		err = sysfs_create_link(&g->nvhost->host1x_pdev->dev.kobj,
				&dev->kobj,
				dev_name(dev));
	}

	return err;
}

void nvgpu_nvhost_remove_symlink(struct gk20a *g)
{
	struct device *dev = dev_from_gk20a(g);

	if (g->nvhost &&
			(dev->parent != &g->nvhost->host1x_pdev->dev)) {
		sysfs_remove_link(&g->nvhost->host1x_pdev->dev.kobj,
				  dev_name(dev));
	}
}

int nvgpu_nvhost_get_syncpt_aperture(
		struct nvgpu_nvhost_dev *nvhost_dev,
		u64 *base, size_t *size)
{
	return nvhost_syncpt_unit_interface_get_aperture(
		nvhost_dev->host1x_pdev, (phys_addr_t *)base, size);
}

u32 nvgpu_nvhost_syncpt_unit_interface_get_byte_offset(u32 syncpt_id)
{
	return nvhost_syncpt_unit_interface_get_byte_offset(syncpt_id);
}

int nvgpu_nvhost_syncpt_init(struct gk20a *g)
{
	int err = 0;
	struct nvgpu_mem *mem = &g->syncpt_mem;

	if (!nvgpu_has_syncpoints(g))
		return -ENOSYS;

	err = nvgpu_get_nvhost_dev(g);
	if (err) {
		nvgpu_err(g, "host1x device not available");
		err = -ENOSYS;
		goto fail_sync;
	}

	err = nvgpu_nvhost_get_syncpt_aperture(
			g->nvhost,
			&g->syncpt_unit_base,
			&g->syncpt_unit_size);
	if (err) {
		nvgpu_err(g, "Failed to get syncpt interface");
		err = -ENOSYS;
		goto fail_sync;
	}

	/*
	 * If IOMMU is enabled, create iova for syncpt region. This iova is then
	 * used to create nvgpu_mem for syncpt by nvgpu_mem_create_from_phys.
	 * For entire syncpt shim read-only mapping full iova range is used and
	 * for a given syncpt read-write mapping only a part of iova range is
	 * used. Instead of creating another variable to store the sgt,
	 * g->syncpt_mem's priv field is used which later on is needed for
	 * freeing the mapping in deinit.
	 */
	if (nvgpu_iommuable(g) && !nvgpu_is_enabled(g, NVGPU_SUPPORT_NVLINK)) {
		struct device *dev = dev_from_gk20a(g);
		struct scatterlist *sg;

		mem->priv.sgt = nvgpu_kzalloc(g, sizeof(struct sg_table));
		if (!mem->priv.sgt) {
			err = -ENOMEM;
			goto fail_sync;
		}

		err = sg_alloc_table(mem->priv.sgt, 1, GFP_KERNEL);
		if (err) {
			err = -ENOMEM;
			goto fail_kfree;
		}
		sg = mem->priv.sgt->sgl;
		sg_set_page(sg, phys_to_page(g->syncpt_unit_base),
				g->syncpt_unit_size, 0);
		err = dma_map_sg_attrs(dev, sg, 1,
				DMA_BIDIRECTIONAL, DMA_ATTR_SKIP_CPU_SYNC);
		/* dma_map_sg_attrs returns 0 on errors */
		if (err == 0) {
			nvgpu_err(g, "iova creation for syncpoint failed");
			err = -ENOMEM;
			goto fail_sgt;
		}
		g->syncpt_unit_base = sg_dma_address(sg);
	}

	g->syncpt_size =
			nvgpu_nvhost_syncpt_unit_interface_get_byte_offset(1);
	nvgpu_info(g, "syncpt_unit_base %llx syncpt_unit_size %zx size %x\n",
			g->syncpt_unit_base, g->syncpt_unit_size,
			g->syncpt_size);
	return 0;
fail_sgt:
	sg_free_table(mem->priv.sgt);
fail_kfree:
	nvgpu_kfree(g, mem->priv.sgt);
fail_sync:
	nvgpu_set_enabled(g, NVGPU_HAS_SYNCPOINTS, false);
	return err;
}
