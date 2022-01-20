/*
 * GoS support
 *
 * Copyright (c) 2016-2022, NVIDIA Corporation.  All rights reserved.
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

#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/nvhost.h>
#include <linux/errno.h>
#include <linux/iommu.h>
#include <linux/io.h>
#include <linux/nvhost_t194.h>
#include <linux/slab.h>

#include "bus_client_t194.h"
#include "nvhost_syncpt_unit_interface.h"

struct syncpt_gos_backing {
	struct rb_node syncpt_gos_backing_entry; /* backing entry */

	u32 syncpt_id;	/* syncpoint id */

	u32 gos_id;	/* GoS id corresponding to syncpt (0..11) */
	u32 gos_offset;	/* Word-offset of syncpt within GoS (0..63) */
};

/**
 * nvhost_syncpt_get_cv_dev_address_table() - Get details of CV devices address table
 *
 * @engine_pdev:	Pointer to a host1x engine
 * @count:		Pointer for storing count value
 * @table:		Pointer to address table
 *
 * Returns:		0 on success, a negative error code otherwise.
 *
 * This function will return details of all CV devices' addresses.
 * Count is the number of entries in the table with index of array
 * as GoS ID
 * Entries in the table store base IOVA address for each GoS
 */
int nvhost_syncpt_get_cv_dev_address_table(struct platform_device *engine_pdev,
				   int *count,
				   dma_addr_t **table)
{
	return -EOPNOTSUPP;
}

/**
 * nvhost_syncpt_find_gos_backing() - Find GoS backing in a global list
 *
 * @host:		Pointer to nvhost_master
 * @syncpt_id:		Syncpoint id to be searched
 *
 * Returns:		0 on success, a negative error code otherwise.
 *
 * This function will find and return syncpoint backing of a syncpoint
 * in global list.
 * Return NULL if no backing is found
 */
static struct syncpt_gos_backing *
nvhost_syncpt_find_gos_backing(struct nvhost_master *host, u32 syncpt_id)
{
	struct rb_root *root = &host->syncpt_backing_head;
	struct rb_node *node = root->rb_node;
	struct syncpt_gos_backing *syncpt_gos_backing;

	while (node) {
		syncpt_gos_backing =
			container_of(node, struct syncpt_gos_backing,
			syncpt_gos_backing_entry);

		if (syncpt_gos_backing->syncpt_id > syncpt_id)
			node = node->rb_left;
		else if (syncpt_gos_backing->syncpt_id != syncpt_id)
			node = node->rb_right;
		else
			return syncpt_gos_backing;
	}

	return NULL;
}

/**
 * nvhost_syncpt_get_gos() - Get GoS data corresponding to a syncpoint
 *
 * @engine_pdev:	Pointer to a host1x engine
 * @syncpt_id:		Syncpoint id to be checked
 * @gos_id:		Pointer for storing the GoS identifier
 * @gos_offset:		Pointer for storing the word offset within GoS
 *
 * Returns:		0 on success, a negative error code otherwise.
 *
 * This function returns GoS ID and offset of semaphore in syncpoint
 * backing in GoS for a corresponding syncpoint id
 * GoS ID and offset should be referred only if return value is 0
 */
int nvhost_syncpt_get_gos(struct platform_device *engine_pdev,
			      u32 syncpt_id,
			      u32 *gos_id,
			      u32 *gos_offset)
{
	struct nvhost_master *host = nvhost_get_host(engine_pdev);
	struct syncpt_gos_backing *syncpt_gos_backing;

	syncpt_gos_backing = nvhost_syncpt_find_gos_backing(host, syncpt_id);
	if (!syncpt_gos_backing) {
		/*
		 * It is absolutely valid for some dev syncpoints to not to
		 * have GoS backing support. So it is up to the clients to
		 * consider this as real error or not.
		 * Keeping this error message verbose, increases CPU load when
		 * this is called frequently in some usecases.
		 */
		dev_dbg(&engine_pdev->dev, "failed to find gos backing");
		return -EINVAL;
	}

	*gos_id = syncpt_gos_backing->gos_id;
	*gos_offset = syncpt_gos_backing->gos_offset;

	return 0;
}

/**
 * nvhost_syncpt_gos_address() - Get GoS address corresponding to syncpoint id
 *
 * @engine_pdev:	Pointer to a host1x engine
 * @syncpt_id:		syncpoint id
 *
 * Return:	IOVA address of syncpoint in GoS
 *
 * This function returns IOVA address of the syncpoint backing
 * in GoS for corresponding syncpoint id
 * This function returns 0 for all error cases
 */
dma_addr_t nvhost_syncpt_gos_address(struct platform_device *engine_pdev,
				     u32 syncpt_id)
{
	return 0;
}

/**
 * nvhost_syncpt_alloc_gos_backing() - Create GoS backing for a syncpoint
 *
 * @engine_pdev:	Pointer to a host1x engine
 * @syncpt_id:		syncpoint id
 *
 * Return:	0 on success, a negative error code otherwise
 *
 * This function creates a GoS backing for a give syncpoint id.
 * GoS backing is then inserted into a global list for
 * future reference/lookup.
 * A backing will only be created for engines supporting GoS
 * and skipped otherwise.
 */
int nvhost_syncpt_alloc_gos_backing(struct platform_device *engine_pdev,
				     u32 syncpt_id)
{
	return -EOPNOTSUPP;
}

/**
 * nvhost_syncpt_release_gos_backing() - Release GoS backing for a syncpoint
 *
 * @sp:		Pointer to nvhost_syncpt
 * @syncpt_id:	syncpoint id
 *
 * Return:	0 on success, a negative error code otherwise
 *
 * This function finds the GoS backing in global list, removes
 * it from list and then releases the backing
 */
int nvhost_syncpt_release_gos_backing(struct nvhost_syncpt *sp,
				      u32 syncpt_id)
{
	return -EOPNOTSUPP;
}
