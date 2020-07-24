// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018, NVIDIA CORPORATION.
 */

#include <linux/genalloc.h>
#include <linux/io.h>
#include <linux/mailbox_client.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>

#include <soc/tegra/bpmp.h>
#include <soc/tegra/bpmp-abi.h>
#include <soc/tegra/ivc.h>

#include "bpmp-private.h"

enum tegra_bpmp_mem_type { TEGRA_SRAM, TEGRA_RMEM };

struct tegra186_bpmp {
	struct tegra_bpmp *parent;

	struct {
		dma_addr_t phys;
		void *virt;
	} tx, rx;

	struct {
		struct mbox_client client;
		struct mbox_chan *channel;
	} mbox;

	struct {
		struct gen_pool *tx, *rx;
	} sram;

	enum tegra_bpmp_mem_type type;
};

static inline struct tegra_bpmp *
mbox_client_to_bpmp(struct mbox_client *client)
{
	struct tegra186_bpmp *priv;

	priv = container_of(client, struct tegra186_bpmp, mbox.client);

	return priv->parent;
}

static bool tegra186_bpmp_is_message_ready(struct tegra_bpmp_channel *channel)
{
	void *frame;

	frame = tegra_ivc_read_get_next_frame(channel->ivc);
	if (IS_ERR(frame)) {
		channel->ib = NULL;
		return false;
	}

	channel->ib = frame;

	return true;
}

static bool tegra186_bpmp_is_channel_free(struct tegra_bpmp_channel *channel)
{
	void *frame;

	frame = tegra_ivc_write_get_next_frame(channel->ivc);
	if (IS_ERR(frame)) {
		channel->ob = NULL;
		return false;
	}

	channel->ob = frame;

	return true;
}

static int tegra186_bpmp_ack_message(struct tegra_bpmp_channel *channel)
{
	return tegra_ivc_read_advance(channel->ivc);
}

static int tegra186_bpmp_post_message(struct tegra_bpmp_channel *channel)
{
	return tegra_ivc_write_advance(channel->ivc);
}

static int tegra186_bpmp_ring_doorbell(struct tegra_bpmp *bpmp)
{
	struct tegra186_bpmp *priv = bpmp->priv;
	int err;

	err = mbox_send_message(priv->mbox.channel, NULL);
	if (err < 0)
		return err;

	mbox_client_txdone(priv->mbox.channel, 0);

	return 0;
}

static void tegra186_bpmp_ivc_notify(struct tegra_ivc *ivc, void *data)
{
	struct tegra_bpmp *bpmp = data;
	struct tegra186_bpmp *priv = bpmp->priv;

	if (WARN_ON(priv->mbox.channel == NULL))
		return;

	tegra186_bpmp_ring_doorbell(bpmp);
}

static int tegra186_bpmp_channel_init(struct tegra_bpmp_channel *channel,
				      struct tegra_bpmp *bpmp,
				      unsigned int index)
{
	struct tegra186_bpmp *priv = bpmp->priv;
	size_t message_size, queue_size;
	unsigned int offset;
	int err;

	channel->ivc = devm_kzalloc(bpmp->dev, sizeof(*channel->ivc),
				    GFP_KERNEL);
	if (!channel->ivc)
		return -ENOMEM;

	message_size = tegra_ivc_align(MSG_MIN_SZ);
	queue_size = tegra_ivc_total_queue_size(message_size);
	offset = queue_size * index;

	err = tegra_ivc_init(channel->ivc, NULL,
			     priv->rx.virt + offset, priv->rx.phys + offset,
			     priv->tx.virt + offset, priv->tx.phys + offset,
			     1, message_size, tegra186_bpmp_ivc_notify,
			     bpmp);
	if (err < 0) {
		dev_err(bpmp->dev, "failed to setup IVC for channel %u: %d\n",
			index, err);
		return err;
	}

	init_completion(&channel->completion);
	channel->bpmp = bpmp;

	return 0;
}

static void tegra186_bpmp_channel_reset(struct tegra_bpmp_channel *channel)
{
	/* reset the channel state */
	tegra_ivc_reset(channel->ivc);

	/* sync the channel state with BPMP */
	while (tegra_ivc_notified(channel->ivc))
		;
}

static void tegra186_bpmp_channel_cleanup(struct tegra_bpmp_channel *channel)
{
	tegra_ivc_cleanup(channel->ivc);
}

static void mbox_handle_rx(struct mbox_client *client, void *data)
{
	struct tegra_bpmp *bpmp = mbox_client_to_bpmp(client);

	tegra_bpmp_handle_rx(bpmp);
}

static void tegra186_bpmp_channel_deinit(struct tegra_bpmp *bpmp)
{
	int i;
	struct tegra186_bpmp *priv = bpmp->priv;

	for (i = 0; i < bpmp->threaded.count; i++) {
		if (!bpmp->threaded_channels[i].bpmp)
			continue;

		tegra186_bpmp_channel_cleanup(&bpmp->threaded_channels[i]);
	}

	tegra186_bpmp_channel_cleanup(bpmp->rx_channel);
	tegra186_bpmp_channel_cleanup(bpmp->tx_channel);

	/* rmem gets cleaned up as part of the rmem device shutdown so no
	 * need to do anything here.
	 */
	if (priv->type == TEGRA_SRAM) {
		gen_pool_free(priv->sram.tx, (unsigned long)priv->tx.virt, 4096);
		gen_pool_free(priv->sram.rx, (unsigned long)priv->rx.virt, 4096);
	}
}

static int tegra186_bpmp_channel_setup(struct tegra_bpmp *bpmp)
{
	unsigned int i;
	int err;

	err = tegra186_bpmp_channel_init(bpmp->tx_channel, bpmp,
					 bpmp->soc->channels.cpu_tx.offset);
	if (err < 0)
		return err;

	err = tegra186_bpmp_channel_init(bpmp->rx_channel, bpmp,
					 bpmp->soc->channels.cpu_rx.offset);
	if (err < 0) {
		tegra186_bpmp_channel_cleanup(bpmp->tx_channel);
		return err;
	}

	for (i = 0; i < bpmp->threaded.count; i++) {
		unsigned int index = bpmp->soc->channels.thread.offset + i;

		err = tegra186_bpmp_channel_init(&bpmp->threaded_channels[i],
						 bpmp, index);
		if (err < 0)
			break;
	}

	if (err < 0)
		tegra186_bpmp_channel_deinit(bpmp);

	return err;
}

static void tegra186_bpmp_reset_channels(struct tegra_bpmp *bpmp)
{
	unsigned int i;

	tegra186_bpmp_channel_reset(bpmp->tx_channel);
	tegra186_bpmp_channel_reset(bpmp->rx_channel);

	for (i = 0; i < bpmp->threaded.count; i++)
		tegra186_bpmp_channel_reset(&bpmp->threaded_channels[i]);
}

static int tegra186_bpmp_sram_init(struct tegra_bpmp *bpmp)
{
	int err;
	struct tegra186_bpmp *priv = bpmp->priv;

	priv->sram.tx = of_gen_pool_get(bpmp->dev->of_node, "shmem", 0);
	if (!priv->sram.tx) {
		dev_err(bpmp->dev, "TX shmem pool not found\n");
		return -ENOMEM;
	}

	priv->tx.virt = gen_pool_dma_alloc(priv->sram.tx, 4096, &priv->tx.phys);
	if (!priv->tx.virt) {
		dev_err(bpmp->dev, "failed to allocate from TX pool\n");
		return -ENOMEM;
	}

	priv->sram.rx = of_gen_pool_get(bpmp->dev->of_node, "shmem", 1);
	if (!priv->sram.rx) {
		dev_err(bpmp->dev, "RX shmem pool not found\n");
		err = -ENOMEM;
		goto free_tx;
	}

	priv->rx.virt = gen_pool_dma_alloc(priv->sram.rx, 4096, &priv->rx.phys);
	if (!priv->rx.virt) {
		dev_err(bpmp->dev, "failed to allocate from RX pool\n");
		err = -ENOMEM;
		goto free_tx;
	}

	priv->type = TEGRA_SRAM;

	return 0;

free_tx:
	gen_pool_free(priv->sram.tx, (unsigned long)priv->tx.virt, 4096);

	return err;
}

static int tegra186_bpmp_init(struct tegra_bpmp *bpmp)
{
	struct tegra186_bpmp *priv;
	int err;

	priv = devm_kzalloc(bpmp->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	bpmp->priv = priv;
	priv->parent = bpmp;

	if (of_reserved_mem_device_init(priv->parent->dev) < 0) {
		err = tegra186_bpmp_sram_init(bpmp);
		if (err < 0)
			return err;
	}

	err = tegra186_bpmp_channel_setup(bpmp);
	if (err < 0)
		return err;

	/* mbox registration */
	priv->mbox.client.dev = bpmp->dev;
	priv->mbox.client.rx_callback = mbox_handle_rx;
	priv->mbox.client.tx_block = false;
	priv->mbox.client.knows_txdone = false;

	priv->mbox.channel = mbox_request_channel(&priv->mbox.client, 0);
	if (IS_ERR(priv->mbox.channel)) {
		err = PTR_ERR(priv->mbox.channel);
		dev_err(bpmp->dev, "failed to get HSP mailbox: %d\n", err);
		tegra186_bpmp_channel_deinit(bpmp);
		return err;
	}

	tegra186_bpmp_reset_channels(bpmp);

	return 0;
}

static void tegra186_bpmp_deinit(struct tegra_bpmp *bpmp)
{
	struct tegra186_bpmp *priv = bpmp->priv;

	mbox_free_channel(priv->mbox.channel);

	tegra186_bpmp_channel_deinit(bpmp);
}

static int tegra186_bpmp_resume(struct tegra_bpmp *bpmp)
{
	unsigned int i;

	/* reset message channels */
	tegra186_bpmp_channel_reset(bpmp->tx_channel);
	tegra186_bpmp_channel_reset(bpmp->rx_channel);

	for (i = 0; i < bpmp->threaded.count; i++)
		tegra186_bpmp_channel_reset(&bpmp->threaded_channels[i]);

	return 0;
}

const struct tegra_bpmp_ops tegra186_bpmp_ops = {
	.init = tegra186_bpmp_init,
	.deinit = tegra186_bpmp_deinit,
	.is_response_ready = tegra186_bpmp_is_message_ready,
	.is_request_ready = tegra186_bpmp_is_message_ready,
	.ack_response = tegra186_bpmp_ack_message,
	.ack_request = tegra186_bpmp_ack_message,
	.is_response_channel_free = tegra186_bpmp_is_channel_free,
	.is_request_channel_free = tegra186_bpmp_is_channel_free,
	.post_response = tegra186_bpmp_post_message,
	.post_request = tegra186_bpmp_post_message,
	.ring_doorbell = tegra186_bpmp_ring_doorbell,
	.resume = tegra186_bpmp_resume,
};

static int tegra_bpmp_rmem_device_init(struct reserved_mem *rmem,
				       struct device *dev)
{
	struct tegra_bpmp *bpmp = dev_get_drvdata(dev);
	struct tegra186_bpmp *priv = bpmp->priv;

	if (rmem->size < 0x2000)
		return -ENOMEM;

	priv->tx.phys = rmem->base;
	priv->rx.phys = rmem->base + 0x1000;

	priv->tx.virt = memremap(priv->tx.phys, rmem->size, MEMREMAP_WC);
	if (priv->tx.virt == NULL)
		return -ENOMEM;
	priv->rx.virt = priv->tx.virt + 0x1000;

	priv->type = TEGRA_RMEM;

	return 0;
}

static void tegra_bpmp_rmem_device_release(struct reserved_mem *rmem,
					   struct device *dev)
{
	struct tegra_bpmp *bpmp = dev_get_drvdata(dev);
	struct tegra186_bpmp *priv = bpmp->priv;

	memunmap(priv->tx.virt);
}


static const struct reserved_mem_ops tegra_bpmp_rmem_ops = {
	.device_init = tegra_bpmp_rmem_device_init,
	.device_release = tegra_bpmp_rmem_device_release,
};

static int tegra_bpmp_rmem_init(struct reserved_mem *rmem)
{
	pr_debug("Tegra BPMP message buffer at %pa, size %lu bytes\n", &rmem->base, (unsigned long)rmem->size);

	rmem->ops = &tegra_bpmp_rmem_ops;

	return 0;
}

RESERVEDMEM_OF_DECLARE(tegra_bpmp, "nvidia,tegra234-bpmp-shmem", tegra_bpmp_rmem_init);
