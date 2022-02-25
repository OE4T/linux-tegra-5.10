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

#include <tegra_hwpm_log.h>
#include <tegra_hwpm.h>
#include <tegra_hwpm_common.h>

int tegra_soc_hwpm_get_allowlist_size(struct tegra_soc_hwpm *hwpm)
{
	int ret = 0;

	hwpm->full_alist_size = 0ULL;

	tegra_hwpm_fn(hwpm, " ");

	if (hwpm->active_chip->get_alist_size == NULL) {
		tegra_hwpm_err(hwpm, "get_alist_size uninitialized");
		return -ENODEV;
	}
	ret = hwpm->active_chip->get_alist_size(hwpm);
	if (ret != 0) {
		tegra_hwpm_err(hwpm, "get_alist_size failed");
		return ret;
	}

	return 0;
}

int tegra_soc_hwpm_update_allowlist(struct tegra_soc_hwpm *hwpm,
	void *ioctl_struct)
{
	int err = 0;
	long pinned_pages = 0;
	long page_idx = 0;
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

	if (hwpm->full_alist_size < 0ULL) {
		tegra_hwpm_err(hwpm, "Invalid allowlist size");
		return -EINVAL;
	}
	if (hwpm->active_chip->get_alist_buf_size == NULL) {
		tegra_hwpm_err(hwpm, "alist_buf_size uninitialized");
		return -ENODEV;
	}
	alist_buf_size = hwpm->full_alist_size *
		hwpm->active_chip->get_alist_buf_size(hwpm);

	/* Memory map user buffer into kernel address space */
	num_pages = DIV_ROUND_UP(offset + alist_buf_size, PAGE_SIZE);
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

	if (hwpm->active_chip->combine_alist == NULL) {
		tegra_hwpm_err(hwpm, "combine_alist uninitialized");
		return -ENODEV;
	}
	err = hwpm->active_chip->combine_alist(hwpm, full_alist_u64);
	if (err != 0) {
		goto alist_unmap;
	}

	query_allowlist->allowlist_size = hwpm->full_alist_size;
	return 0;

alist_unmap:
	if (full_alist)
		vunmap(full_alist);
	if (pinned_pages > 0) {
		for (page_idx = 0; page_idx < pinned_pages; page_idx++) {
			set_page_dirty(pages[page_idx]);
			put_page(pages[page_idx]);
		}
	}
	if (pages) {
		kfree(pages);
	}

	return err;
}