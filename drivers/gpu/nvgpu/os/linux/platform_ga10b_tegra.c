/*
 * GA10B Tegra Platform Interface
 *
 * Copyright (c) 2016-2021, NVIDIA CORPORATION.  All rights reserved.
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

#include <linux/of_platform.h>
#include <linux/debugfs.h>
#include <linux/dma-buf.h>
#include <linux/reset.h>
#include <linux/iommu.h>
#include <linux/hashtable.h>
#include <linux/clk.h>
#ifdef CONFIG_TEGRA_BWMGR
#include <linux/platform/tegra/emc_bwmgr.h>
#endif
#include <linux/pm_runtime.h>

#include <nvgpu/gk20a.h>
#include <nvgpu/nvhost.h>
#include <nvgpu/soc.h>

#ifdef CONFIG_NV_TEGRA_BPMP
#include <soc/tegra/tegra-bpmp-dvfs.h>
#endif /* CONFIG_NV_TEGRA_BPMP */

#include <uapi/linux/nvgpu.h>

#include "os/linux/platform_gk20a.h"
#include "os/linux/clk.h"
#include "os/linux/scale.h"
#include "os/linux/module.h"

#include "os/linux/platform_gp10b.h"

#include "os/linux/os_linux.h"
#include "os/linux/platform_gk20a_tegra.h"

#define EMC3D_GA10B_RATIO 500

#define GPCCLK_INIT_RATE 1000000000UL

/* Run gpc0, gpc1 and sysclk at same rate */
struct gk20a_platform_clk tegra_ga10b_clocks[] = {
	{"sysclk", GPCCLK_INIT_RATE},
	{"gpc0clk", GPCCLK_INIT_RATE},
	{"gpc1clk", GPCCLK_INIT_RATE},
	{"fuse", UINT_MAX}
};

/*
 * This function finds clocks in tegra platform and populates
 * the clock information to platform data.
 */

static int ga10b_tegra_acquire_platform_clocks(struct device *dev,
		struct gk20a_platform_clk *clk_entries,
		unsigned int num_clk_entries)
{
	struct gk20a_platform *platform = dev_get_drvdata(dev);
	struct gk20a *g = platform->g;
	struct device_node *np = nvgpu_get_node(g);
	unsigned int i, num_clks_dt;

	/* Get clocks only on supported platforms(silicon and fpga) */
	if (!nvgpu_platform_is_silicon(g) && !nvgpu_platform_is_fpga(g)) {
		return 0;
	}

	num_clks_dt = of_clk_get_parent_count(np);
	if (num_clks_dt > num_clk_entries) {
		nvgpu_err(g, "maximum number of clocks supported is %d",
			num_clk_entries);
		return -EINVAL;
	} else if (num_clks_dt == 0) {
		nvgpu_err(g, "unable to read clocks from DT");
		return -ENODEV;
	}

	platform->num_clks = 0;

	/* TODO add floorsweep check before requesting clocks */
	for (i = 0; i < num_clks_dt; i++) {
		long rate;
		struct clk *c = of_clk_get_by_name(np, clk_entries[i].name);

		if (IS_ERR(c)) {
			nvgpu_info(g, "cannot get clock %s",
					clk_entries[i].name);
			/* Continue with other clocks enable */
		} else {
			rate = clk_entries[i].default_rate;
			clk_set_rate(c, rate);
			platform->clk[i] = c;
		}
	}

	platform->num_clks = i;

#ifdef CONFIG_NV_TEGRA_BPMP
	if (platform->clk[0]) {
		i = tegra_bpmp_dvfs_get_clk_id(dev->of_node,
					       clk_entries[0].name);
		if (i > 0)
			platform->maxmin_clk_id = i;
	}
#endif

	return 0;
}

static int ga10b_tegra_get_clocks(struct device *dev)
{
	return ga10b_tegra_acquire_platform_clocks(dev, tegra_ga10b_clocks,
			ARRAY_SIZE(tegra_ga10b_clocks));
}

void ga10b_tegra_scale_init(struct device *dev)
{
#ifdef CONFIG_TEGRA_BWMG
	struct gk20a_platform *platform = gk20a_get_platform(dev);
	struct gk20a_scale_profile *profile = platform->g->scale_profile;

	if (!profile)
		return;

	platform->g->emc3d_ratio = EMC3D_GA10B_RATIO;

	gp10b_tegra_scale_init(dev);
#endif
}

static void ga10b_tegra_scale_exit(struct device *dev)
{
#ifdef CONFIG_TEGRA_BWMGR
	struct gk20a_platform *platform = gk20a_get_platform(dev);
	struct gk20a_scale_profile *profile = platform->g->scale_profile;

	if (profile)
		tegra_bwmgr_unregister(
			(struct tegra_bwmgr_client *)profile->private_data);
#endif
}

static int ga10b_tegra_probe(struct device *dev)
{
	struct gk20a_platform *platform = dev_get_drvdata(dev);
	int err;
	bool joint_xpu_rail = false;
	struct gk20a *g = platform->g;
#ifdef CONFIG_OF
	struct device_node *of_chosen;
#endif

	err = nvgpu_nvhost_syncpt_init(platform->g);
	if (err) {
		if (err != -ENOSYS)
			return err;
	}

	err = gk20a_tegra_init_secure_alloc(platform);
	if (err)
		return err;

	platform->disable_bigpage = !iommu_get_domain_for_dev(dev) &&
		(NVGPU_CPU_PAGE_SIZE < SZ_64K);

#ifdef CONFIG_OF
	of_chosen = of_find_node_by_path("/chosen");
	if (!of_chosen)
		return -ENODEV;

	joint_xpu_rail = of_property_read_bool(of_chosen,
				"nvidia,tegra-joint_xpu_rail");
#endif

	if (joint_xpu_rail) {
		nvgpu_log_info(g, "XPU rails are joint\n");
		platform->can_railgate_init = false;
		nvgpu_set_enabled(g, NVGPU_CAN_RAILGATE, false);
	}

	err = ga10b_tegra_get_clocks(dev);
	if (err != 0) {
		return err;
	}
	nvgpu_linux_init_clk_support(platform->g);

	nvgpu_mutex_init(&platform->clk_get_freq_lock);

	return 0;
}

static int ga10b_tegra_late_probe(struct device *dev)
{
	return 0;
}

static int ga10b_tegra_remove(struct device *dev)
{
	struct gk20a_platform *platform = gk20a_get_platform(dev);

	ga10b_tegra_scale_exit(dev);

#ifdef CONFIG_TEGRA_GK20A_NVHOST
	nvgpu_free_nvhost_dev(get_gk20a(dev));
#endif

	nvgpu_mutex_destroy(&platform->clk_get_freq_lock);

	return 0;
}

static bool ga10b_tegra_is_railgated(struct device *dev)
{
	struct gk20a *g = get_gk20a(dev);
	bool ret = false;

	if (pm_runtime_status_suspended(dev)) {
		ret = true;
	}

	nvgpu_log(g, gpu_dbg_info, "railgated? %s", ret ? "yes" : "no");

	return ret;
}

static int ga10b_tegra_railgate(struct device *dev)
{
#ifdef CONFIG_TEGRA_BWMGR
	struct gk20a_platform *platform = gk20a_get_platform(dev);
	struct gk20a_scale_profile *profile = platform->g->scale_profile;

	/* remove emc frequency floor */
	if (profile)
		tegra_bwmgr_set_emc(
			(struct tegra_bwmgr_client *)profile->private_data,
			0, TEGRA_BWMGR_SET_EMC_FLOOR);
#endif /* CONFIG_TEGRA_BWMGR */

	gp10b_tegra_clks_control(dev, false);

	return 0;
}

static int ga10b_tegra_unrailgate(struct device *dev)
{
	int ret = 0;
#ifdef CONFIG_TEGRA_BWMGR
	struct gk20a_platform *platform = gk20a_get_platform(dev);
	struct gk20a_scale_profile *profile = platform->g->scale_profile;
#endif

	gp10b_tegra_clks_control(dev, true);

#ifdef CONFIG_TEGRA_BWMGR
	/* to start with set emc frequency floor to max rate*/
	if (profile)
		tegra_bwmgr_set_emc(
			(struct tegra_bwmgr_client *)profile->private_data,
			tegra_bwmgr_get_max_emc_rate(),
			TEGRA_BWMGR_SET_EMC_FLOOR);
#endif
	return ret;
}

static int ga10b_tegra_suspend(struct device *dev)
{
	return 0;
}

static bool is_tpc_mask_valid(struct gk20a_platform *platform, u32 tpc_pg_mask)
{
	u32 i;
	bool valid = false;

	for (i = 0; i < MAX_TPC_PG_CONFIGS; i++) {
		if (tpc_pg_mask == platform->valid_tpc_mask[i]) {
			valid = true;
			break;
		}
	}
	return valid;
}

static void ga10b_tegra_set_tpc_pg_mask(struct device *dev, u32 tpc_pg_mask)
{
	struct gk20a_platform *platform = gk20a_get_platform(dev);
	struct gk20a *g = get_gk20a(dev);

	if (is_tpc_mask_valid(platform, tpc_pg_mask)) {
		g->tpc_pg_mask = tpc_pg_mask;
	}
}

struct gk20a_platform ga10b_tegra_platform = {
#ifdef CONFIG_TEGRA_GK20A_NVHOST
	.has_syncpoints = true,
#endif

	/* ptimer src frequency in hz*/
	.ptimer_src_freq = 31250000,

	.ch_wdt_init_limit_ms = 5000,

	.probe = ga10b_tegra_probe,
	.late_probe = ga10b_tegra_late_probe,
	.remove = ga10b_tegra_remove,
	.railgate_delay_init    = 500,
	.can_railgate_init      = false,

	/* add tpc powergate JIRA NVGPU-4683 */
	.can_tpc_powergate      = false,

	.set_tpc_pg_mask	= ga10b_tegra_set_tpc_pg_mask,

	.can_slcg               = true,
	.can_blcg               = true,
	.can_elcg               = true,
	.enable_slcg            = false,
	.enable_blcg            = false,
	.enable_elcg            = false,
	.enable_perfmon         = true,

	/* power management configuration  JIRA NVGPU-4683 */
	.enable_elpg            = false,
	.enable_elpg_ms         = true,
	.can_elpg_init          = false,
	.enable_aelpg           = false,

	/* power management callbacks */
	.suspend = ga10b_tegra_suspend,
	.railgate = ga10b_tegra_railgate,
	.unrailgate = ga10b_tegra_unrailgate,
	.is_railgated = ga10b_tegra_is_railgated,

	.busy = gk20a_tegra_busy,
	.idle = gk20a_tegra_idle,

	.clk_round_rate = gp10b_round_clk_rate,
	.get_clk_freqs = gp10b_clk_get_freqs,

	/* frequency scaling configuration */
	.initscale = ga10b_tegra_scale_init,
	.prescale = gp10b_tegra_prescale,
	.postscale = gp10b_tegra_postscale,
	/* Enable ga10b frequency scaling - JIRA NVGPU-4683 */
	/* Disable frequency scaling */
	.devfreq_governor = NULL,
	.qos_notify = NULL,

	.dump_platform_dependencies = gk20a_tegra_debug_dump,

	.platform_chip_id = TEGRA_234,
	.soc_name = "tegra23x",

	.honors_aperture = true,
	.unified_memory = true,

	/*
	 * This specifies the maximum contiguous size of a DMA mapping to Linux
	 * kernel's DMA framework.
	 * The IOMMU is capable of mapping all of physical memory and hence
	 * dma_mask is set to memory size (128GB in this case).
	 * For iGPU, nvgpu executes own dma allocs (e.g. alloc_page()) and
	 * sg_table construction. No IOMMU mapping is required and so dma_mask
	 * value is not important.
	 * However, for dGPU connected over PCIe through an IOMMU, dma_mask is
	 * significant. In this case, IOMMU bit in GPU physical address is not
	 * relevant.
	 */
	.dma_mask = DMA_BIT_MASK(37),

	.reset_assert = gp10b_tegra_reset_assert,
	.reset_deassert = gp10b_tegra_reset_deassert,

	/*
	 * Size includes total size of ctxsw VPR buffers.
	 * The size can vary for different chips as attribute ctx buffer
	 * size depends on max number of tpcs supported on the chip.
	 */
	.secure_buffer_size = 0x400000, /* 4 MB */
};
