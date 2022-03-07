/*
 * Copyright (c) 2021-2022, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#include <soc/tegra/fuse.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/of_address.h>
#include <linux/dma-buf.h>

#include <uapi/linux/tegra-soc-hwpm-uapi.h>

#include <tegra_hwpm.h>
#include <tegra_hwpm_log.h>
#include <tegra_hwpm_common.h>
#include <tegra_hwpm_static_analysis.h>

static int tegra_hwpm_get_alist_size(struct tegra_soc_hwpm *hwpm)
{
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	u32 ip_idx;
	u32 perfmux_idx, perfmon_idx;
	unsigned long inst_idx = 0UL;
	unsigned long floorsweep_info = 0UL;
	struct hwpm_ip *chip_ip = NULL;
	hwpm_ip_perfmux *perfmux = NULL;
	hwpm_ip_perfmon *perfmon = NULL;

	tegra_hwpm_fn(hwpm, " ");

	for (ip_idx = 0U; ip_idx < active_chip->get_ip_max_idx(hwpm);
		ip_idx++) {
		chip_ip = active_chip->chip_ips[ip_idx];

		/* Skip unavailable IPs */
		if (!chip_ip->reserved) {
			continue;
		}

		if (chip_ip->fs_mask == 0U) {
			/* No IP instance is available */
			continue;
		}

		floorsweep_info = (unsigned long)chip_ip->fs_mask;

		for_each_set_bit(inst_idx, &floorsweep_info, 32U) {
			/* Add perfmux alist size to full alist size */
			for (perfmux_idx = 0U;
				perfmux_idx < chip_ip->num_perfmux_slots;
				perfmux_idx++) {
				perfmux = chip_ip->ip_perfmux[perfmux_idx];

				if (perfmux == NULL) {
					continue;
				}

				if (perfmux->hw_inst_mask != BIT(inst_idx)) {
					continue;
				}

				if (perfmux->alist) {
					hwpm->full_alist_size =
						tegra_hwpm_safe_add_u64(
							hwpm->full_alist_size,
							perfmux->alist_size);
				} else {
					tegra_hwpm_err(hwpm, "IP %d"
						" perfmux %d NULL alist",
						ip_idx, perfmux_idx);
				}
			}

			/* Add perfmon alist size to full alist size */
			for (perfmon_idx = 0U;
				perfmon_idx < chip_ip->num_perfmon_slots;
				perfmon_idx++) {
				perfmon = chip_ip->ip_perfmon[perfmon_idx];

				if (perfmon == NULL) {
					continue;
				}

				if (perfmon->hw_inst_mask != BIT(inst_idx)) {
					continue;
				}

				if (perfmon->alist) {
					hwpm->full_alist_size =
						tegra_hwpm_safe_add_u64(
							hwpm->full_alist_size,
							perfmon->alist_size);
				} else {
					tegra_hwpm_err(hwpm, "IP %d"
						" perfmon %d NULL alist",
						ip_idx, perfmon_idx);
				}
			}
		}
	}

	return 0;
}

int tegra_hwpm_get_allowlist_size(struct tegra_soc_hwpm *hwpm)
{
	int ret = 0;

	hwpm->full_alist_size = 0ULL;

	tegra_hwpm_fn(hwpm, " ");

	ret = tegra_hwpm_get_alist_size(hwpm);
	if (ret != 0) {
		tegra_hwpm_err(hwpm, "get_alist_size failed");
		return ret;
	}

	return 0;
}

static int tegra_hwpm_combine_alist(struct tegra_soc_hwpm *hwpm, u64 *alist)
{
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	u32 ip_idx;
	u32 perfmux_idx, perfmon_idx;
	unsigned long inst_idx = 0UL;
	unsigned long floorsweep_info = 0UL;
	struct hwpm_ip *chip_ip = NULL;
	hwpm_ip_perfmux *perfmux = NULL;
	hwpm_ip_perfmon *perfmon = NULL;
	u64 full_alist_idx = 0;
	int err = 0;

	tegra_hwpm_fn(hwpm, " ");

	for (ip_idx = 0U; ip_idx < active_chip->get_ip_max_idx(hwpm);
		ip_idx++) {
		chip_ip = active_chip->chip_ips[ip_idx];

		/* Skip unavailable IPs */
		if (!chip_ip->reserved) {
			continue;
		}

		if (chip_ip->fs_mask == 0U) {
			/* No IP instance is available */
			continue;
		}

		if (hwpm->active_chip->copy_alist == NULL) {
			tegra_hwpm_err(hwpm, "copy_alist uninitialized");
			return -ENODEV;
		}

		floorsweep_info = (unsigned long)chip_ip->fs_mask;

		for_each_set_bit(inst_idx, &floorsweep_info, 32U) {
			/* Copy perfmux alist to full alist array */
			for (perfmux_idx = 0U;
				perfmux_idx < chip_ip->num_perfmux_slots;
				perfmux_idx++) {
				perfmux = chip_ip->ip_perfmux[perfmux_idx];

				if (perfmux == NULL) {
					continue;
				}

				if (perfmux->hw_inst_mask != BIT(inst_idx)) {
					continue;
				}

				err = hwpm->active_chip->copy_alist(hwpm,
					perfmux, alist, &full_alist_idx);
				if (err != 0) {
					tegra_hwpm_err(hwpm, "IP %d"
						" perfmux %d alist copy failed",
						ip_idx, perfmux_idx);
					goto fail;
				}
			}

			/* Copy perfmon alist to full alist array */
			for (perfmon_idx = 0U;
				perfmon_idx < chip_ip->num_perfmon_slots;
				perfmon_idx++) {
				perfmon = chip_ip->ip_perfmon[perfmon_idx];

				if (perfmon == NULL) {
					continue;
				}

				if (perfmon->hw_inst_mask != BIT(inst_idx)) {
					continue;
				}

				err = hwpm->active_chip->copy_alist(hwpm,
					perfmon, alist, &full_alist_idx);
				if (err != 0) {
					tegra_hwpm_err(hwpm, "IP %d"
						" perfmon %d alist copy failed",
						ip_idx, perfmon_idx);
					goto fail;
				}
			}
		}
	}

	/* Check size of full alist with hwpm->full_alist_size*/
	if (full_alist_idx != hwpm->full_alist_size) {
		tegra_hwpm_err(hwpm, "full_alist_size 0x%llx doesn't match "
			"max full_alist_idx 0x%llx",
			hwpm->full_alist_size, full_alist_idx);
		err = -EINVAL;
	}

fail:
	return err;
}

int tegra_hwpm_update_allowlist(struct tegra_soc_hwpm *hwpm,
	void *ioctl_struct)
{
	int err = 0;
	u64 pinned_pages = 0;
	u64 page_idx = 0;
	u64 alist_buf_size = 0;
	u64 num_pages = 0;
	u64 *full_alist_u64 = NULL;
	void *full_alist = NULL;
	struct page **pages = NULL;
	struct tegra_soc_hwpm_query_allowlist *query_allowlist =
			(struct tegra_soc_hwpm_query_allowlist *)ioctl_struct;
	unsigned long user_va = (unsigned long)(query_allowlist->allowlist);
	unsigned long offset = user_va & ~PAGE_MASK;

	tegra_hwpm_fn(hwpm, " ");

	if (hwpm->full_alist_size == 0ULL) {
		tegra_hwpm_err(hwpm, "Invalid allowlist size");
		return -EINVAL;
	}
	if (hwpm->active_chip->get_alist_buf_size == NULL) {
		tegra_hwpm_err(hwpm, "alist_buf_size uninitialized");
		return -ENODEV;
	}
	alist_buf_size = tegra_hwpm_safe_mult_u64(hwpm->full_alist_size,
		hwpm->active_chip->get_alist_buf_size(hwpm));

	/* Memory map user buffer into kernel address space */
	alist_buf_size = tegra_hwpm_safe_add_u64(offset, alist_buf_size);

	/* Round-up and Divide */
	alist_buf_size = tegra_hwpm_safe_sub_u64(
		tegra_hwpm_safe_add_u64(alist_buf_size, PAGE_SIZE), 1ULL);
	num_pages = alist_buf_size / PAGE_SIZE;

	pages = (struct page **)kzalloc(sizeof(*pages) * num_pages, GFP_KERNEL);
	if (!pages) {
		tegra_hwpm_err(hwpm,
			"Couldn't allocate memory for pages array");
		err = -ENOMEM;
		goto alist_unmap;
	}

	pinned_pages = get_user_pages(user_va & PAGE_MASK, num_pages, 0,
				pages, NULL);
	if (pinned_pages != num_pages) {
		tegra_hwpm_err(hwpm, "Requested %llu pages / Got %ld pages",
				num_pages, pinned_pages);
		err = -ENOMEM;
		goto alist_unmap;
	}

	full_alist = vmap(pages, num_pages, VM_MAP, PAGE_KERNEL);
	if (!full_alist) {
		tegra_hwpm_err(hwpm, "Couldn't map allowlist buffer into"
				   " kernel address space");
		err = -ENOMEM;
		goto alist_unmap;
	}
	full_alist_u64 = (u64 *)(full_alist + offset);

	err = tegra_hwpm_combine_alist(hwpm, full_alist_u64);
	if (err != 0) {
		goto alist_unmap;
	}

	query_allowlist->allowlist_size = hwpm->full_alist_size;
	return 0;

alist_unmap:
	if (full_alist)
		vunmap(full_alist);
	if (pinned_pages > 0) {
		for (page_idx = 0ULL; page_idx < pinned_pages; page_idx++) {
			set_page_dirty(pages[page_idx]);
			put_page(pages[page_idx]);
		}
	}
	if (pages) {
		kfree(pages);
	}

	return err;
}