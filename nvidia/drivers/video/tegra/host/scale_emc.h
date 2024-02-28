/*
 * drivers/video/tegra/host/gr3d/scale_emc.h
 *
 * Tegra Graphics Host 3D Clock Scaling
 *
 * Copyright (c) 2010-2015, NVIDIA Corporation. All rights reserved.
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

#ifndef NVHOST_T30_SCALE3D_H
#define NVHOST_T30_SCALE3D_H

struct nvhost_device_profile;
struct platform_device;
struct device;
struct dentry;
struct clk;

struct nvhost_emc_params {
	long				emc_slope;
	long				emc_offset;
	long				emc_dip_slope;
	long				emc_dip_offset;
	long				emc_xmid;
	bool				linear;
};

#if defined(CONFIG_TEGRA_GRHOST_SCALE)
/* Suspend is called when powering down module */
void nvhost_scale_emc_suspend(struct device *);

/* Initialization and de-initialization for module */
void nvhost_scale_emc_init(struct platform_device *pdev);
void nvhost_scale_emc_deinit(struct platform_device *pdev);

/* Callback for generic profile. The callback handles setting
 * the frequencies of EMC and the second 3d unit (if available) */
void nvhost_scale_emc_callback(struct nvhost_device_profile *profile,
			     unsigned long freq);

void nvhost_scale_emc_calibrate_emc(struct nvhost_emc_params *emc_params,
				  struct clk *clk_3d, struct clk *clk_3d_emc,
				  bool linear_emc);
long nvhost_scale_emc_get_emc_rate(struct nvhost_emc_params *emc_params,
				 long freq);

#else
static inline void nvhost_scale_emc_suspend(struct device *d) { }
static inline void nvhost_scale_emc_init(struct platform_device *pdev) { }
static inline void nvhost_scale_emc_deinit(struct platform_device *pdev) { }
static inline void nvhost_scale_emc_callback(struct nvhost_device_profile *profile,
					     unsigned long freq) { }

static inline void nvhost_scale_emc_calibrate_emc(struct nvhost_emc_params *emc_params,
						  struct clk *clk_3d,
						  struct clk *clk_3d_emc,
						  bool linear_emc) { }

static inline long nvhost_scale_emc_get_emc_rate(struct nvhost_emc_params *emc_params,
						 long freq)
{
	return 0;
}
#endif
#endif
