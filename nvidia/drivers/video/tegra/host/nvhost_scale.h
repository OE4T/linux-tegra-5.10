/*
 * drivers/video/tegra/host/gr3d/scale_emc.h
 *
 * Tegra Graphics Host 3D Clock Scaling
 *
 * Copyright (c) 2010-2015, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVHOST_SCALE_H
#define NVHOST_SCALE_H

#include <linux/nvhost.h>
#include <linux/devfreq.h>

struct platform_device;
struct host1x_actmon;
struct clk;

/*
 * profile_rec - Device specific power management variables
 */

struct nvhost_device_profile {
	struct platform_device		*pdev;

	struct host1x_actmon		**actmon;

	struct clk			*clk;

	bool				busy;
	ktime_t				last_event_time;

	struct devfreq_dev_profile	devfreq_profile;
	struct devfreq_dev_status	dev_stat;

	void				*private_data;
	struct notifier_block		qos_notify_block;
	int				num_actmons;
};

#if defined(CONFIG_TEGRA_GRHOST_SCALE)
/* Initialization and de-initialization for module */
void nvhost_scale_init(struct platform_device *);
void nvhost_scale_deinit(struct platform_device *);

/*
 * call when performing submit to notify scaling mechanism that the module is
 * in use
 */
void nvhost_scale_notify_busy(struct platform_device *);
void nvhost_scale_notify_idle(struct platform_device *);

int nvhost_scale_hw_init(struct platform_device *);
void nvhost_scale_hw_deinit(struct platform_device *);

void nvhost_actmon_debug_init(struct host1x_actmon *actmon,
				     struct dentry *de);
#else
static inline void nvhost_scale_init(struct platform_device *d) { }
static inline void nvhost_scale_deinit(struct platform_device *d) { }
static inline void nvhost_scale_notify_busy(struct platform_device *d) { }
static inline void nvhost_scale_notify_idle(struct platform_device *d) { }
static inline int nvhost_scale_hw_init(struct platform_device *d)
{
	return 0;
}
static inline void nvhost_scale_hw_deinit(struct platform_device *d) { }

#endif
#endif
