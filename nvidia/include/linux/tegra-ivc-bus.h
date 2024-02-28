/*
 * Copyright (c) 2015-2017 NVIDIA CORPORATION.  All rights reserved.
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

#ifndef _LINUX_TEGRA_IVC_BUS_H
#define _LINUX_TEGRA_IVC_BUS_H

#include <linux/tegra-ivc-instance.h>
#include <linux/types.h>

extern struct bus_type tegra_ivc_bus_type;
extern struct device_type tegra_ivc_bus_dev_type;
struct tegra_ivc_bus;
struct tegra_ivc_rpc_data;

struct tegra_ivc_bus *tegra_ivc_bus_create(struct device *);

void tegra_ivc_bus_ready(struct tegra_ivc_bus *bus, bool online);
void tegra_ivc_bus_destroy(struct tegra_ivc_bus *bus);
int tegra_ivc_bus_boot_sync(struct tegra_ivc_bus *bus);
void tegra_ivc_bus_notify(struct tegra_ivc_bus *bus, u16 group);

struct tegra_ivc_driver {
	struct device_driver driver;
	struct device_type *dev_type;
	union {
		const struct tegra_ivc_channel_ops *channel;
	} ops;
};

static inline struct tegra_ivc_driver *to_tegra_ivc_driver(
						struct device_driver *drv)
{
	if (drv == NULL)
		return NULL;
	return container_of(drv, struct tegra_ivc_driver, driver);
}

int tegra_ivc_driver_register(struct tegra_ivc_driver *drv);
void tegra_ivc_driver_unregister(struct tegra_ivc_driver *drv);
#define tegra_ivc_module_driver(drv) \
	module_driver(drv, tegra_ivc_driver_register, \
			tegra_ivc_driver_unregister)

#define tegra_ivc_subsys_driver(__driver, __register, __unregister, ...) \
static int __init __driver##_init(void) \
{ \
	return __register(&(__driver) , ##__VA_ARGS__); \
} \
subsys_initcall_sync(__driver##_init);

#define tegra_ivc_subsys_driver_default(__driver) \
tegra_ivc_subsys_driver(__driver, \
						tegra_ivc_driver_register, \
						tegra_ivc_driver_unregister)

/* IVC channel driver support */
extern struct device_type tegra_ivc_channel_type;

struct tegra_ivc_channel {
	struct ivc ivc;
	struct device dev;
	const struct tegra_ivc_channel_ops __rcu *ops;
	struct tegra_ivc_channel *next;
	struct mutex ivc_wr_lock;
	struct tegra_ivc_rpc_data *rpc_priv;
	atomic_t bus_resets;
	u16 group;
	bool is_ready;
};

static inline bool tegra_ivc_channel_online_check(
		struct tegra_ivc_channel *chan)
{
	atomic_set(&chan->bus_resets, 0);

	smp_wmb();
	smp_rmb();

	return chan->is_ready;
}

static inline bool tegra_ivc_channel_has_been_reset(
		struct tegra_ivc_channel *chan)
{
	smp_rmb();
	return atomic_read(&chan->bus_resets) != 0;
}

static inline void *tegra_ivc_channel_get_drvdata(
		struct tegra_ivc_channel *chan)
{
	return dev_get_drvdata(&chan->dev);
}

static inline void tegra_ivc_channel_set_drvdata(
		struct tegra_ivc_channel *chan, void *data)
{
	dev_set_drvdata(&chan->dev, data);
}

static inline struct tegra_ivc_channel *to_tegra_ivc_channel(
		struct device *dev)
{
	return container_of(dev, struct tegra_ivc_channel, dev);
}

static inline struct device *tegra_ivc_channel_to_camrtc_dev(
	struct tegra_ivc_channel *ch)
{
	if (unlikely(ch == NULL))
		return NULL;

	BUG_ON(ch->dev.parent == NULL);
	BUG_ON(ch->dev.parent->parent == NULL);

	return ch->dev.parent->parent;
}

int tegra_ivc_channel_runtime_get(struct tegra_ivc_channel *chan);
void tegra_ivc_channel_runtime_put(struct tegra_ivc_channel *chan);

struct tegra_ivc_channel_ops {
	int (*probe)(struct tegra_ivc_channel *);
	void (*ready)(struct tegra_ivc_channel *, bool online);
	void (*remove)(struct tegra_ivc_channel *);
	void (*notify)(struct tegra_ivc_channel *);
};

/* Legacy mailbox support */
struct tegra_ivc_mbox_msg {
	int length;
	void *data;
};

#endif
