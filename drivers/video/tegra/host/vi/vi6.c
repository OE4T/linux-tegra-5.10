/*
 * VI6 driver for T234
 *
 * Copyright (c) 2019, NVIDIA Corporation.  All rights reserved.
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

#include <asm/ioctls.h>
#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/export.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_graph.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <media/capture_vi_channel.h>
#include <soc/tegra/camrtc-capture.h>
#include <soc/tegra/chip-id.h>

#include "vi6.h"
#include "dev.h"
#include "bus_client.h"
#include "nvhost_acm.h"
#include "capture/capture-support.h"

#include "t23x/t23x.h"

#include <media/vi.h>
#include <media/mc_common.h>
#include <media/tegra_camera_platform.h>
#include "camera/vi/vi5_fops.h"
#include <uapi/linux/nvhost_vi_ioctl.h>
#include <linux/platform/tegra/latency_allowance.h>

/* HW capability, pixels per clock */
#define NUM_PPC		8
/* 15% bus protocol overhead */
/* + 5% SW overhead */
#define VI_OVERHEAD	20

struct host_vi6 {
	struct platform_device *pdev;
	struct platform_device *vi_thi;
	struct vi vi_common;

	/* RCE RM area */
	struct sg_table rm_sgt;
};

int nvhost_vi6_aggregate_constraints(struct platform_device *dev,
				int clk_index,
				unsigned long floor_rate,
				unsigned long pixelrate,
				unsigned long bw_constraint)
{
	struct nvhost_device_data *pdata = nvhost_get_devdata(dev);

	if (!pdata) {
		dev_err(&dev->dev,
			"No platform data, fall back to default policy\n");
		return 0;
	}
	if (!pixelrate || clk_index != 0)
		return 0;
	/* SCF send request using NVHOST_CLK, which is calculated
	 * in floor_rate, so we need to aggregate its request
	 * with V4L2 pixelrate request
	 */
	return floor_rate + (pixelrate / pdata->num_ppc);
}

int vi6_priv_early_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct nvhost_device_data *info;
	struct device_node *thi_np;
	struct platform_device *thi = NULL;
	struct host_vi6 *vi6;
	int err = 0;

	info = (void *)of_device_get_match_data(dev);
	if (unlikely(info == NULL)) {
		dev_WARN(dev, "no platform data\n");
		return -ENODATA;
	}

	thi_np = of_parse_phandle(dev->of_node, "nvidia,vi-falcon-device", 0);
	if (thi_np == NULL) {
		dev_WARN(dev, "missing %s handle\n", "nvidia,vi-falcon-device");
		return -ENODEV;
	}

	thi = of_find_device_by_node(thi_np);
	of_node_put(thi_np);

	if (thi == NULL)
		return -ENODEV;

	if (thi->dev.driver == NULL) {
		err = -EPROBE_DEFER;
		goto put_vi;
	}

	vi6 = (struct host_vi6 *) devm_kzalloc(dev, sizeof(*vi6), GFP_KERNEL);
	if (!vi6) {
		err = -ENOMEM;
		goto put_vi;
	}

	vi6->vi_thi = thi;
	vi6->pdev = pdev;
	info->pdev = pdev;
	mutex_init(&info->lock);
	platform_set_drvdata(pdev, info);
	info->private_data = vi6;

	(void) dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(40));

	return 0;

put_vi:
	platform_device_put(thi);
	if (err != -EPROBE_DEFER)
		dev_err(&pdev->dev, "probe failed: %d\n", err);

	info->private_data = NULL;

	return err;
}

int vi6_priv_late_probe(struct platform_device *pdev)
{
	struct tegra_camera_dev_info vi_info;
	struct nvhost_device_data *info = platform_get_drvdata(pdev);
	struct host_vi6 *vi6 = info->private_data;
	int err;

	memset(&vi_info, 0, sizeof(vi_info));
	vi_info.pdev = pdev;
	vi_info.hw_type = HWTYPE_VI;
	vi_info.ppc = NUM_PPC;
	vi_info.overhead = VI_OVERHEAD;
	err = tegra_camera_device_register(&vi_info, vi6);
	if (err)
		goto device_release;

	vi6->vi_common.mc_vi.vi = &vi6->vi_common;

	/* vi6_fops are not available yet. */
	vi6->vi_common.mc_vi.fops = &vi5_fops;
	err = tegra_vi_media_controller_init(&vi6->vi_common.mc_vi, pdev);
	if (err) {
		dev_warn(&pdev->dev, "media controller init failed\n");
		err = 0;
	}

	return 0;

device_release:
	nvhost_client_device_release(pdev);

	return err;
}

static int vi6_probe(struct platform_device *pdev)
{
	int err;
	struct nvhost_device_data *pdata;
	struct host_vi6 *vi6;

	err = vi6_priv_early_probe(pdev);
	if (err)
		goto error;

	pdata = platform_get_drvdata(pdev);
	vi6 = pdata->private_data;

	err = nvhost_client_device_get_resources(pdev);
	if (err)
		goto put_vi;

	err = nvhost_module_init(pdev);
	if (err)
		goto put_vi;

	err = nvhost_client_device_init(pdev);
	if (err)
		goto deinit;

	vi6_priv_late_probe(pdev);

	return 0;

deinit:
	nvhost_module_deinit(pdev);
put_vi:
	platform_device_put(vi6->vi_thi);
	if (err != -EPROBE_DEFER)
		dev_err(&pdev->dev, "probe failed: %d\n", err);
error:
	return err;
}

static int vi6_remove(struct platform_device *pdev)
{
	struct nvhost_device_data *pdata = platform_get_drvdata(pdev);
	struct host_vi6 *vi6 = pdata->private_data;

	tegra_camera_device_unregister(vi6);

	tegra_vi_media_controller_cleanup(&vi6->vi_common.mc_vi);

	if (vi6->rm_sgt.sgl) {
		dma_unmap_sg(&pdev->dev, vi6->rm_sgt.sgl,
			vi6->rm_sgt.orig_nents, DMA_FROM_DEVICE);
		sg_free_table(&vi6->rm_sgt);
	}

	platform_device_put(vi6->vi_thi);

	return 0;
}

static const struct of_device_id tegra_vi6_of_match[] = {
	{
		.compatible = "nvidia,tegra234-vi",
		.data = &t23x_vi6_info,
	},
	{ },
};

static struct platform_driver vi6_driver = {
	.probe = vi6_probe,
	.remove = vi6_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = "tegra234-vi6",
#ifdef CONFIG_OF
		.of_match_table = tegra_vi6_of_match,
#endif
#ifdef CONFIG_PM
		.pm = &nvhost_module_pm_ops,
#endif
	},
};

module_platform_driver(vi6_driver);