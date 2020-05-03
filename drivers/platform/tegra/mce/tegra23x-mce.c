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

#include <linux/debugfs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/tegra-mce.h>

#include <asm/smp_plat.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0)
#include <soc/tegra/chip-id.h>
#else
#include <soc/tegra/fuse.h>
#endif
#include <asm/cacheflush.h>
#include <linux/platform/tegra/tegra19x_cache.h>

static int tegra23x_mce_read_versions(u32 *major, u32 *minor)
{
	pr_info("Stub supported: tegra23x_mce_read_versions\n");

	return 0;
}

static int tegra23x_mce_read_l4_cache_ways(u64 *value)
{
	pr_info("Stub supported: tegra23x_mce_read_l4_cache_ways\n");

	return 0;
}

static int tegra23x_mce_write_l4_cache_ways(u64 data, u64 *value)
{
	pr_info("Stub supported: tegra23x_mce_write_l4_cache_ways\n");

	return 0;
}

static int tegra_23x_flush_cache_all(void)
{
	int ret = 0;

	ret = tegra19x_flush_cache_all();
	/* Fallback to VA flush cache all if not support or failed */
	if (ret)
		flush_cache_all();

	/* CRITICAL: failed to flush all cache */
	WARN_ON(ret && ret != -ENOTSUPP);

	return ret;
}

static int tegra_23x_flush_dcache_all(void *__maybe_unused unused)
{
	int ret = 0;

	ret = tegra19x_flush_dcache_all();
	/* Fallback to VA flush dcache if not support or failed */
	if (ret)
		__flush_dcache_all(unused);

	/* CRITICAL: failed to flush dcache */
	WARN_ON(ret && ret != -ENOTSUPP);

	return ret;
}

static int tegra_23x_clean_dcache_all(void *__maybe_unused unused)
{
	int ret = 0;

	ret = tegra19x_clean_dcache_all();
	/* Fallback to VA clean if not support or failed */
	if (ret)
		__clean_dcache_all(unused);

	/* CRITICAL: failed to clean dcache */
	WARN_ON(ret && ret != -ENOTSUPP);

	return ret;
}

#ifdef CONFIG_DEBUG_FS
static struct dentry *mce_debugfs;

static int tegra23x_mce_versions_get(void *data, u64 *val)
{
	u32 major, minor;
	int ret;

	ret = tegra_mce_read_versions(&major, &minor);
	if (!ret)
		*val = ((u64)major << 32) | minor;
	return ret;
}

DEFINE_SIMPLE_ATTRIBUTE(tegra23x_mce_versions_fops, tegra23x_mce_versions_get,
			NULL, "%llu\n");

struct debugfs_entry {
	const char *name;
	const struct file_operations *fops;
	mode_t mode;
};

static struct debugfs_entry tegra23x_mce_attrs[] = {
	{ "versions", &tegra23x_mce_versions_fops, 0444 },
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
	.flush_cache_all = tegra_23x_flush_cache_all,
	.flush_dcache_all = tegra_23x_flush_dcache_all,
	.clean_dcache_all = tegra_23x_clean_dcache_all,
};

static int __init tegra23x_mce_early_init(void)
{
	if (tegra_get_chip_id() == TEGRA234)
		tegra_mce_set_ops(&t23x_mce_ops);

	return 0;
}
early_initcall(tegra23x_mce_early_init);

MODULE_DESCRIPTION("NVIDIA Tegra23x MCE driver");
MODULE_AUTHOR("Sandipan Patra <spatra@nvidia.com>");
MODULE_LICENSE("GPL v2");
