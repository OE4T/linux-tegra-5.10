/*
 * Copyright (c) 2020-2021, NVIDIA CORPORATION.  All rights reserved.
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

#include <linux/cpumask.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/tegra-mce.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0)
#include <soc/tegra/chip-id.h>
#else
#include <soc/tegra/fuse.h>
#endif
#include <linux/platform_device.h>
#include <linux/t23x_ari.h>
#include <linux/debugfs.h>
#include <asm/cputype.h>
#include <asm/smp_plat.h>

#define MAX_CPUS			12U
#define MAX_CORES_PER_CLUSTER		4U
#define ARI_TIMEOUT_MAX			2000U /* msec */

/* Register offsets for ARI request/results*/
#define ARI_REQUEST			0x0U
#define ARI_REQUEST_EVENT_MASK		0x8U
#define ARI_STATUS			0x10U
#define ARI_REQUEST_DATA_LO		0x18U
#define ARI_REQUEST_DATA_HI		0x20U
#define ARI_RESPONSE_DATA_LO		0x28U
#define ARI_RESPONSE_DATA_HI		0x30U

/* Status values for the current request */
#define ARI_REQ_PENDING			1U
#define ARI_REQ_ONGOING			3U
#define ARI_REQUEST_VALID_BIT		(1U << 8U)
#define ARI_REQUEST_NS_BIT		(1U << 31U)

/* Write Enable bit */
#define CACHE_WAYS_WRITE_EN_BIT		(1U << 15U)

static void __iomem *ari_bar_array[MAX_CPUS];

static inline void ari_mmio_write_32(void __iomem *ari_base,
	u32 val, u32 reg)
{
	writel(val, ari_base + reg);
}

static inline u32 ari_mmio_read_32(void __iomem *ari_base, u32 reg)
{
	return readl(ari_base + reg);
}

static inline u32 ari_get_response_low(void __iomem *ari_base)
{
	return ari_mmio_read_32(ari_base, ARI_RESPONSE_DATA_LO);
}

static inline u32 ari_get_response_high(void __iomem *ari_base)
{
	return ari_mmio_read_32(ari_base, ARI_RESPONSE_DATA_HI);
}

static inline void ari_clobber_response(void __iomem *ari_base)
{
	ari_mmio_write_32(ari_base, 0, ARI_RESPONSE_DATA_LO);
	ari_mmio_write_32(ari_base, 0, ARI_RESPONSE_DATA_HI);
}

static int32_t ari_send_request(void __iomem *ari_base, u32 evt_mask,
	u32 req, u32 lo, u32 hi)
{
	uint32_t timeout = ARI_TIMEOUT_MAX;
	uint32_t status;
	int32_t ret = 0;

	/* clobber response */
	ari_mmio_write_32(ari_base, 0, ARI_RESPONSE_DATA_LO);
	ari_mmio_write_32(ari_base, 0, ARI_RESPONSE_DATA_HI);

	/* send request */
	ari_mmio_write_32(ari_base, lo, ARI_REQUEST_DATA_LO);
	ari_mmio_write_32(ari_base, hi, ARI_REQUEST_DATA_HI);
	ari_mmio_write_32(ari_base, evt_mask, ARI_REQUEST_EVENT_MASK);
	ari_mmio_write_32(ari_base,
		req | ARI_REQUEST_VALID_BIT | ARI_REQUEST_NS_BIT, ARI_REQUEST);

	while (timeout) {
		status = ari_mmio_read_32(ari_base, ARI_STATUS);
		if (!(status & (ARI_REQ_ONGOING | ARI_REQ_PENDING)))
			break;
		mdelay(1);
		timeout--;
	}

	if (!timeout)
		ret = -ETIMEDOUT;

	return ret;
}

static int get_ari_address_index(void)
{
	uint64_t mpidr;
	uint32_t core_id, cluster_id;

	mpidr = read_cpuid_mpidr();
	cluster_id = MPIDR_AFFINITY_LEVEL(mpidr, 2);
	core_id = MPIDR_AFFINITY_LEVEL(mpidr, 1);

	return ((cluster_id * MAX_CORES_PER_CLUSTER) + core_id);
}

static int tegra23x_mce_read_versions(u32 *major, u32 *minor)
{
	int cpu_idx;
	int32_t ret = 0;

	if (IS_ERR_OR_NULL(major) || IS_ERR_OR_NULL(minor))
		return -EINVAL;

	preempt_disable();
	cpu_idx = get_ari_address_index();
	ret = ari_send_request(ari_bar_array[cpu_idx], 0U,
			(u32)TEGRA_ARI_VERSION, 0U, 0U);
	if (ret)
		return ret;
	*major = ari_get_response_low(ari_bar_array[cpu_idx]);
	*minor = ari_get_response_high(ari_bar_array[cpu_idx]);
	preempt_enable();

	return 0;
}

/*
 * echo copies data from req_low to resp_low and
 * data from req_high to resp_high.
 */
static int tegra23x_mce_echo_data(u64 data, u64 *matched)
{
	int cpu_idx;
	u32 input1 = (u32)(data & 0xFFFFFFFF);
	u32 input2 = (u32)(data >> 32);
	u64 out1, out2;
	int32_t ret = 0;

	if (IS_ERR_OR_NULL(matched))
		return -EINVAL;

	preempt_disable();
	cpu_idx = get_ari_address_index();
	ret = ari_send_request(ari_bar_array[cpu_idx], 0U,
			(u32)TEGRA_ARI_ECHO, input1, input2);
	if (ret)
		return ret;
	out1 = (u64)ari_get_response_low(ari_bar_array[cpu_idx]);
	out2 = (u64)ari_get_response_high(ari_bar_array[cpu_idx]);
	*matched = ((out2 << 32) | out1);
	preempt_enable();

	if (data == *matched)
		return 0;
	else
		return -ENOMSG;
}

static int tegra23x_mce_read_l4_cache_ways(u64 *value)
{
	int cpu_idx;
	u64 out;
	int32_t ret = 0;

	preempt_disable();
	cpu_idx = get_ari_address_index();
	ret = ari_send_request(ari_bar_array[cpu_idx], 0U,
                               (u32)TEGRA_ARI_CCPLEX_CACHE_CONTROL, 0U, 0U);
	if (ret)
		return ret;

	out = (u64)ari_get_response_low(ari_bar_array[cpu_idx]);

	*value = out;
	preempt_enable();

	return 0;
}

static int tegra23x_mce_write_l4_cache_ways(u64 data, u64 *value)
{
	int cpu_idx;
	u32 input = (u32)(data & 0x00001F1F);
	u64 out;
	int32_t ret = 0;

	if (IS_ERR_OR_NULL(value))
		return -EINVAL;

	preempt_disable();
	cpu_idx = get_ari_address_index();
	input |= CACHE_WAYS_WRITE_EN_BIT;
	ret = ari_send_request(ari_bar_array[cpu_idx], 0U,
			(u32)TEGRA_ARI_CCPLEX_CACHE_CONTROL, input, 0U);
	if (ret)
		return ret;
	out = (u64)ari_get_response_low(ari_bar_array[cpu_idx]);
	*value = out;
	preempt_enable();

	return 0;
}

static int tegra23x_mce_read_uncore_perfmon(u32 req, u32 *data)
{
	int cpu_idx;
	u32 out_lo, out_hi;
	int32_t ret = 0;

	if (IS_ERR_OR_NULL(data))
		return -EINVAL;

	preempt_disable();

	cpu_idx = get_ari_address_index();
	ret = ari_send_request(ari_bar_array[cpu_idx], 0U,
			(u32)TEGRA_ARI_PERFMON, 0U, req);

	if (ret)
		return ret;

	out_lo = ari_get_response_low(ari_bar_array[cpu_idx]);
	out_hi = ari_get_response_high(ari_bar_array[cpu_idx]);

	pr_debug("%s: read status = %u\n", __func__, out_hi);

	if (out_hi != 0)
		return -out_hi;

	*data = out_lo;

	preempt_enable();

	return 0;
}

static int tegra23x_mce_write_uncore_perfmon(u32 req, u32 data)
{
	int cpu_idx;
	u32 out_lo, out_hi;
	int32_t ret = 0;

	preempt_disable();

	cpu_idx = get_ari_address_index();
	ret = ari_send_request(ari_bar_array[cpu_idx], 0U,
			(u32)TEGRA_ARI_PERFMON, data, req);

	if (ret)
		return ret;

	out_lo = ari_get_response_low(ari_bar_array[cpu_idx]);
	out_hi = ari_get_response_high(ari_bar_array[cpu_idx]);

	pr_debug("%s: write status = %u\n", __func__, out_hi);

	if (out_hi != 0)
		return -out_hi;

	preempt_enable();

	return 0;
}

#ifdef CONFIG_DEBUG_FS

static struct dentry *mce_debugfs;

static int tegra23x_mce_versions_get(void *data, u64 *val)
{
	u32 major = 0;
	u32 minor = 0;
	u64 version = 0;
	int ret;

	*val = 0;
	ret = tegra_mce_read_versions(&major, &minor);
	if (!ret) {
		version = (u64)major;
		*val = (version << 32) | minor;
	}
	return ret;
}

static int tegra23x_mce_echo_set(void *data, u64 val)
{
	u64 matched = 0;
	int ret;

	ret = tegra_mce_echo_data(val, &matched);
	if (ret)
		return ret;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(tegra23x_mce_versions_fops, tegra23x_mce_versions_get,
			NULL, "%llx\n");
DEFINE_SIMPLE_ATTRIBUTE(tegra23x_mce_echo_fops, NULL,
			tegra23x_mce_echo_set, "%llx\n");

struct debugfs_entry {
	const char *name;
	const struct file_operations *fops;
	mode_t mode;
};

/* Make sure to put an NULL entry at the end of each group */
static struct debugfs_entry tegra23x_mce_attrs[] = {
	{ "versions", &tegra23x_mce_versions_fops, 0444 },
	{ "echo", &tegra23x_mce_echo_fops, 0200 },
	{ NULL, NULL, 0 }
};

static struct debugfs_entry *tegra_mce_attrs = tegra23x_mce_attrs;

static __init int tegra23x_mce_init(void)
{
	struct debugfs_entry *fent;
	struct dentry *dent;
	int ret;

	if (tegra_get_chip_id() != TEGRA234)
		return 0;

	mce_debugfs = debugfs_create_dir("tegra_mce", NULL);
	if (!mce_debugfs)
		return -ENOMEM;

	for (fent = tegra_mce_attrs; fent->name; fent++) {
		dent = debugfs_create_file(fent->name, fent->mode,
					   mce_debugfs, NULL, fent->fops);
		if (IS_ERR_OR_NULL(dent)) {
			ret = dent ? PTR_ERR(dent) : -EINVAL;
			pr_err("%s: failed to create debugfs (%s): %d\n",
			       __func__, fent->name, ret);
			goto err;
		}
	}

	pr_debug("%s: init finished\n", __func__);

	return 0;

err:
	debugfs_remove_recursive(mce_debugfs);

	return ret;
}

static void __exit tegra23x_mce_exit(void)
{
	if (tegra_get_chip_id() == TEGRA234)
		debugfs_remove_recursive(mce_debugfs);
}
module_init(tegra23x_mce_init);
module_exit(tegra23x_mce_exit);
#endif /* CONFIG_DEBUG_FS */

static struct tegra_mce_ops t23x_mce_ops = {
	.read_versions = tegra23x_mce_read_versions,
	.read_l3_cache_ways = tegra23x_mce_read_l4_cache_ways,
	.write_l3_cache_ways = tegra23x_mce_write_l4_cache_ways,
	.echo_data = tegra23x_mce_echo_data,
	.read_uncore_perfmon = tegra23x_mce_read_uncore_perfmon,
	.write_uncore_perfmon = tegra23x_mce_write_uncore_perfmon,
};

static int t23x_mce_probe(struct platform_device *pdev)
{
	int cpu;
	struct resource *res;

	/* this ARI NS mapping applies to Split, Lock-step and FS */
	for_each_possible_cpu(cpu) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, cpu);
		ari_bar_array[cpu] = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(ari_bar_array[cpu])) {
			dev_err(&pdev->dev, "mapping ARI failed for %d\n",
						cpu);
			return PTR_ERR(ari_bar_array[cpu]);
		}
	}

	return 0;
}

static int t23x_mce_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id t23x_mce_of_match[] = {
	{ .compatible = "nvidia,t23x-mce", .data = NULL },
	{ },
};
MODULE_DEVICE_TABLE(of, t23x_mce_of_match);

static struct platform_driver t23x_mce_driver = {
	.probe = t23x_mce_probe,
	.remove = t23x_mce_remove,
	.driver = {
		.owner  = THIS_MODULE,
		.name = "t23x-mce",
		.of_match_table = of_match_ptr(t23x_mce_of_match),
	},
};

static int __init tegra23x_mce_early_init(void)
{
	if (tegra_get_chip_id() == TEGRA234) {
		tegra_mce_set_ops(&t23x_mce_ops);
		platform_driver_register(&t23x_mce_driver);
	}

	return 0;
}
pure_initcall(tegra23x_mce_early_init);

MODULE_DESCRIPTION("NVIDIA Tegra23x MCE driver");
MODULE_AUTHOR("NVIDIA Corporation");
MODULE_LICENSE("GPL v2");
