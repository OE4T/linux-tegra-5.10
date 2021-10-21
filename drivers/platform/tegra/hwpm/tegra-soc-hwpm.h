/*
 * Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
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
 *
 * tegra-soc-hwpm.h:
 * This is the header for the Tegra SOC HWPM driver.
 */

#ifndef TEGRA_SOC_HWPM_H
#define TEGRA_SOC_HWPM_H

#include <linux/reset.h>
#include <linux/clk.h>
#include <linux/debugfs.h>
#include <linux/dma-buf.h>
#include <linux/scatterlist.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/delay.h>

#include "tegra-soc-hwpm-log.h"
#include "tegra-soc-hwpm-hw.h"
#include <uapi/linux/tegra-soc-hwpm-uapi.h>

/* FIXME: Default timeout is 1 sec. Is this sufficient for pre-si? */
#define HWPM_TIMEOUT(timeout_check, expiry_msg) ({			\
	bool timeout_expired = false;					\
	s32 timeout_msecs = 1000;					\
	u32 sleep_msecs = 100;						\
	while(!(timeout_check)) {					\
		msleep(sleep_msecs);					\
		timeout_msecs -= sleep_msecs;				\
		if (timeout_msecs <= 0) {				\
			tegra_soc_hwpm_err("Timeout expired for %s!",	\
					expiry_msg);			\
			timeout_expired = true;				\
			break;						\
		}							\
	}								\
	timeout_expired;						\
})

/* Driver struct */
struct tegra_soc_hwpm {
	/* Device */
	struct platform_device *pdev;
	struct device *dev;
	struct device_node *np;
	struct class class;
	dev_t dev_t;
	struct cdev cdev;

	/* IP floorsweep info */
	u64 ip_fs_info[TERGA_SOC_HWPM_NUM_IPS];

	/* MMIO apertures in device tree */
	void __iomem *dt_apertures[TEGRA_SOC_HWPM_NUM_DT_APERTURES];

	/* Clocks and resets */
	struct clk *la_clk;
	struct clk *la_parent_clk;
	struct reset_control *la_rst;
	struct reset_control *hwpm_rst;

	struct tegra_soc_hwpm_ip_ops ip_info[TEGRA_SOC_HWPM_NUM_DT_APERTURES];

	/* Memory Management */
	struct dma_buf *stream_dma_buf;
	struct dma_buf_attachment *stream_attach;
	struct sg_table *stream_sgt;
	struct dma_buf *mem_bytes_dma_buf;
	struct dma_buf_attachment *mem_bytes_attach;
	struct sg_table *mem_bytes_sgt;
	void *mem_bytes_kernel;

	/* SW State */
	bool bind_completed;
	s32 full_alist_size;

	/* Debugging */
#ifdef CONFIG_DEBUG_FS
	struct dentry *debugfs_root;
#endif
	bool fake_registers_enabled;
};

extern struct platform_device *tegra_soc_hwpm_pdev;
extern const struct file_operations tegra_soc_hwpm_ops;

#ifdef CONFIG_DEBUG_FS
void tegra_soc_hwpm_debugfs_init(struct tegra_soc_hwpm *hwpm);
void tegra_soc_hwpm_debugfs_deinit(struct tegra_soc_hwpm *hwpm);
#else
static inline void tegra_soc_hwpm_debugfs_init(struct tegra_soc_hwpm *hwpm) {}
static inline void tegra_soc_hwpm_debugfs_deinit(struct tegra_soc_hwpm *hwpm) {}
#endif  /* CONFIG_DEBUG_FS  */

#endif /* TEGRA_SOC_HWPM_H */
