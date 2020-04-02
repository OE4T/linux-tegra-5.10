/*
 * Copyright (c) 2020, NVIDIA CORPORATION.  All rights reserved.
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

#include <linux/device.h>
#include <dt-bindings/interconnect/tegra_icc_id.h>
#include <linux/interconnect.h>
#include <linux/interconnect-provider.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include "tegra_icc.h"

DEFINE_TNODE(icc_master, TEGRA_ICC_MASTER, TEGRA_ICC_NONE);
DEFINE_TNODE(debug, TEGRA_ICC_DEBUG, TEGRA_ICC_NISO);
DEFINE_TNODE(display, TEGRA_ICC_DISPLAY, TEGRA_ICC_ISO_DISPLAY);
DEFINE_TNODE(vi, TEGRA_ICC_VI, TEGRA_ICC_ISO_VI);
DEFINE_TNODE(eqos, TEGRA_ICC_EQOS, TEGRA_ICC_ISO_OTHER);
DEFINE_TNODE(cpu_cluster0, TEGRA_ICC_CPU_CLUSTER0, TEGRA_ICC_NISO);
DEFINE_TNODE(cpu_cluster1, TEGRA_ICC_CPU_CLUSTER1, TEGRA_ICC_NISO);
DEFINE_TNODE(cpu_cluster2, TEGRA_ICC_CPU_CLUSTER2, TEGRA_ICC_NISO);

static struct tegra_icc_node *tegra_icc_nodes[] = {
	[TEGRA_ICC_MASTER] = &icc_master,
	[TEGRA_ICC_DEBUG] = &debug,
	[TEGRA_ICC_DISPLAY] = &display,
	[TEGRA_ICC_VI] = &vi,
	[TEGRA_ICC_EQOS] = &eqos,
	[TEGRA_ICC_CPU_CLUSTER0] = &cpu_cluster0,
	[TEGRA_ICC_CPU_CLUSTER1] = &cpu_cluster1,
	[TEGRA_ICC_CPU_CLUSTER2] = &cpu_cluster2,
};

static int tegra_icc_probe(struct platform_device *pdev)
{
	const struct tegra_icc_ops *ops;
	struct icc_onecell_data *data;
	struct icc_provider *provider;
	struct tegra_icc_node **tnodes;
	struct tegra_icc_provider *tp;
	struct icc_node *node;
	size_t num_nodes, i;
	int ret;

	ops = of_device_get_match_data(&pdev->dev);
	if (!ops)
		return -EINVAL;

	tnodes = tegra_icc_nodes;
	num_nodes = ARRAY_SIZE(tegra_icc_nodes);

	tp = devm_kzalloc(&pdev->dev, sizeof(*tp), GFP_KERNEL);
	if (!tp)
		return -ENOMEM;

	data = devm_kcalloc(&pdev->dev, num_nodes, sizeof(*node), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	provider = &tp->provider;
	provider->dev = &pdev->dev;
	provider->set = ops->plat_icc_set;
	provider->aggregate = ops->plat_icc_aggregate;
	provider->xlate = of_icc_xlate_onecell;
	INIT_LIST_HEAD(&provider->nodes);
	provider->data = data;

	tp->dev = &pdev->dev;

	ret = icc_provider_add(provider);
	if (ret) {
		dev_err(&pdev->dev, "error adding interconnect provider\n");
		return ret;
	}

	for (i = 0; i < num_nodes; i++) {
		node = icc_node_create(tnodes[i]->id);
		if (IS_ERR(node)) {
			ret = PTR_ERR(node);
			goto err;
		}

		node->name = tnodes[i]->name;
		node->data = tnodes[i];
		icc_node_add(node, provider);

		dev_dbg(&pdev->dev, "registered node %p %s %d\n", node,
			tnodes[i]->name, node->id);
printk("registered node %p %s %d\n", node,
	tnodes[i]->name, node->id);

		/* populate links */
		icc_link_create(node, TEGRA_ICC_MASTER);

		data->nodes[i] = node;
	}
	data->num_nodes = num_nodes;

	platform_set_drvdata(pdev, tp);

	dev_dbg(&pdev->dev, "Registered TEGRA ICC\n");

	return ret;
err:
	list_for_each_entry(node, &provider->nodes, node_list) {
		icc_node_del(node);
		icc_node_destroy(node->id);
	}

	icc_provider_del(provider);
	return ret;
}

static int tegra_icc_remove(struct platform_device *pdev)
{
	struct tegra_icc_provider *tp = platform_get_drvdata(pdev);
	struct icc_provider *provider = &tp->provider;
	struct icc_node *n;

	list_for_each_entry(n, &provider->nodes, node_list) {
		icc_node_del(n);
		icc_node_destroy(n->id);
	}

	return icc_provider_del(provider);
}

static const struct of_device_id tegra_icc_of_match[] = {
	{ .compatible = "nvidia,tegra23x-icc", .data = &tegra23x_icc_ops },
	{ },
};
MODULE_DEVICE_TABLE(of, tegra_icc_of_match);

static struct platform_driver tegra_icc_driver = {
	.probe = tegra_icc_probe,
	.remove = tegra_icc_remove,
	.driver = {
		.name = "tegra-icc",
		.of_match_table = tegra_icc_of_match,
	},
};

static int __init tegra_icc_init(void)
{
	return platform_driver_register(&tegra_icc_driver);
}
core_initcall(tegra_icc_init);

MODULE_AUTHOR("Sanjay Chandrashekara <sanjayc@nvidia.com>");
MODULE_DESCRIPTION("Tegra ICC driver");
MODULE_LICENSE("GPL v2");
