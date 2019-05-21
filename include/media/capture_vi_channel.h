/*
 * include/linux/media/capture_vi_channel.h
 *
 * VI channel character device driver header
 *
 * Copyright (c) 2017-2019 NVIDIA Corporation.  All rights reserved.
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

#ifndef __VI_CHANNEL_H__
#define __VI_CHANNEL_H__

#include <linux/of_platform.h>

struct vi_channel_drv;

/**
 * @brief VI fops for Host1x syncpt/gos allocations
 *
 * This fops is a HAL for chip/IP generations, see the respective VI platform
 * drivers for the implementations.
 */
struct vi_channel_drv_ops {
	/**
	 * Request a syncpt allocation from Host1x
	 *
	 * @param[in]	pdev		VI platform_device
	 * @param[in]	name		syncpt name
	 * @param[out]	syncpt_id	assigned syncpt id
	 * @returns	0 (success), errno (error)
	 */
	int (*alloc_syncpt)(struct platform_device *pdev, const char *name,
			uint32_t *syncpt_id);

	/**
	 * Release a syncpt to Host1x
	 *
	 * @param[in]	pdev	VI platform_device
	 * @param[in]	id	syncpt id to free
	 */
	void (*release_syncpt)(struct platform_device *pdev, uint32_t id);

	/**
	 * Retrieve the GoS table allocated in the VI-THI carveout
	 *
	 * @param[in]	pdev	VI platform_device
	 * @param[out]	count	No. of carveout devices
	 * @param[out]	table	GoS table pointer
	 */
	void (*get_gos_table)(struct platform_device *pdev, int *count,
			const dma_addr_t **table);

	/**
	 * Get a syncpt's GoS backing in the VI-THI carveout
	 *
	 * @param[in]	pdev		VI platform_device
	 * @param[in]	id		syncpt id
	 * @param[out]	gos_index	GoS id
	 * @param[out]	gos_offset	Word offset of syncpt within GoS
	 * @returns	0 (success), errno (error)
	 */
	int (*get_syncpt_gos_backing)(struct platform_device *pdev, uint32_t id,
			dma_addr_t *syncpt_addr, uint32_t *gos_index,
			uint32_t *gos_offset);
};

/**
 * @brief VI channel context (character device)
 */
struct tegra_vi_channel {
	struct device *dev; /**< VI device */
	struct platform_device *ndev; /**< VI platform_device */
	struct vi_channel_drv *drv; /**< VI channels driver context */
	struct rcu_head rcu; /**< VI channel rcu */
	struct vi_capture *capture_data; /**< VI channel capture context */
	const struct vi_channel_drv_ops *ops; /**< VI syncpt/gos fops */
	struct device *rtcpu_dev; /**< rtcpu device */
};

/**
 * @brief Create the VI channels driver contexts, and instantiate
 * MAX_VI_CHANNELS many channel character device nodes.
 *
 * VI channel nodes appear in the filesystem as:
 * /dev/capture-vi-channel{0..MAX_VI_CHANNELS-1}
 *
 * @param[in]	ndev	VI platform_device context
 * @param[in]	ops	vi_channel_drv_ops fops
 * @returns	0 (success), errno (error)
 */
int vi_channel_drv_register(struct platform_device *ndev,
	const struct vi_channel_drv_ops *ops);

/**
 * @brief Destroy the VI channels driver and all character device nodes.
 *
 * The VI channels driver and associated channel contexts in memory are freed,
 * rendering the VI platform driver unusable until re-initialized.
 *
 * @param[in]	dev	VI device context
 */
void vi_channel_drv_unregister(struct device *dev);

/**
 * @brief Unpin and free the list of pinned capture_mapping's associated with a
 * capture request.
 *
 * @param[in]	chan		VI channel context
 * @param[in]	buffer_index	Capture descriptor queue index
 */
void vi_capture_request_unpin(struct tegra_vi_channel *chan,
	uint32_t buffer_index);

/**
 * Internal APIs for V4L2 driver (aka. VI mode)
 */

/**
 * @brief Open a VI channel for streaming
 *
 * @param[in]	channel		VI channel no.
 * @param[in]	is_mem_pinned	Whether capture request memory is pinned
 * @returns	tegra_vi_channel pointer (success), ERR_PTR (error)
 */
struct tegra_vi_channel *vi_channel_open_ex(unsigned channel,
	bool is_mem_pinned);

/**
 * @brief Close an opened VI channel
 *
 * @param[in]	channel	VI channel no.
 * @param[in]	chan	Corresponding tegra_vi_channel
 * @returns	0
 */
int vi_channel_close_ex(unsigned channel, struct tegra_vi_channel *chan);

#endif /* __VI_CHANNEL_H__ */
