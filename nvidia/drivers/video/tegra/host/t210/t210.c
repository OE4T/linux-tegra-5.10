/*
 * Tegra Graphics Init for T210 Architecture Chips
 *
 * Copyright (c) 2011-2021, NVIDIA Corporation.  All rights reserved.
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
#include <linux/slab.h>
#include <linux/io.h>

#include <linux/version.h>

#if defined(CONFIG_TEGRA_BWMGR)
#include <linux/platform/tegra/emc_bwmgr.h>
#endif

#include <media/vi.h>

#include "dev.h"
#include "nvhost_job.h"
#include "class_ids.h"
#include "scale_emc.h"

#include "t210.h"
#include "host1x/host1x.h"
#include "host1x/host1x04_hardware.h"
#include "flcn/flcn.h"
#include "nvdec/nvdec.h"
#include "tsec/tsec.h"
#include "isp/isp.h"
#include "isp/isp_isr_v1.h"

#include "../../../../arch/arm/mach-tegra/iomap.h"

#include "chip_support.h"
#include "nvhost_scale.h"
#include "vhost/vhost.h"

#include "cg_regs.c"

#define HOST_EMC_FLOOR 204000000
#define HOST_NVDEC_EMC_FLOOR 102000000
#define TSEC_AUTOSUSPEND_DELAY 500

#define BIT64(nr) (1ULL << (nr))

static struct host1x_device_info host1x04_info = {
	.nb_channels	= T210_NVHOST_NUMCHANNELS,
	.ch_base	= 0,
	.ch_limit	= T210_NVHOST_NUMCHANNELS,
	.nb_mlocks	= NV_HOST1X_NB_MLOCKS,
	.initialize_chip_support = nvhost_init_t210_support,
	.nb_hw_pts	= NV_HOST1X_SYNCPT_NB_PTS,
	.nb_pts		= NV_HOST1X_SYNCPT_NB_PTS,
	.pts_base	= 0,
	.pts_limit	= NV_HOST1X_SYNCPT_NB_PTS,
	.nb_syncpt_irqs	= 1,
	.syncpt_policy	= SYNCPT_PER_CHANNEL,
	.nb_actmons	= 1,
};

struct nvhost_device_data t21_host1x_info = {
	.clocks			= {{"host1x", 81000000},
				   {"actmon", UINT_MAX}, {} },
	.can_powergate		= true,
	.autosuspend_delay	= 50,
	.private_data		= &host1x04_info,
	.finalize_poweron = nvhost_host1x_finalize_poweron,
	.prepare_poweroff = nvhost_host1x_prepare_poweroff,
};

#ifdef CONFIG_TEGRA_GRHOST_ISP
struct nvhost_device_data t21_isp_info = {
	.num_channels		= 1,
	.moduleid		= NVHOST_MODULE_ISP,
	.devfs_name		= "isp",
	.class			= NV_VIDEO_STREAMING_ISP_CLASS_ID,
	.modulemutexes		= {NVMODMUTEX_ISP_0},
	.exclusive		= true,
	/* HACK: Mark as keepalive until 1188795 is fixed */
	.keepalive		= true,
	.autosuspend_delay	= 500,
	.can_powergate		= true,
	.clocks			= {
#ifdef CONFIG_COMMON_CLK
		{ "ispa", UINT_MAX, 0 },
#else
		{ "isp", UINT_MAX, 0 },
#endif
#if !defined(CONFIG_TEGRA_BWMGR)
		{"emc", 0, NVHOST_MODULE_ID_EXTERNAL_MEMORY_CONTROLLER, }
#endif
	 },
	.finalize_poweron	= nvhost_isp_finalize_poweron,
	.prepare_poweroff	= nvhost_isp_prepare_poweroff,
	.hw_init		= nvhost_isp_register_isr_v1,
	.ctrl_ops		= &tegra_isp_ctrl_ops,
};

struct nvhost_device_data t21_ispb_info = {
	.num_channels		= 1,
	.moduleid		= NVHOST_MODULE_ISPB,
	.devfs_name		= "isp.1",
	.class			= NV_VIDEO_STREAMING_ISPB_CLASS_ID,
	.modulemutexes		= {NVMODMUTEX_ISP_1},
	.exclusive		= true,
	/* HACK: Mark as keepalive until 1188795 is fixed */
	.keepalive		= true,
	.autosuspend_delay	= 500,
	.can_powergate		= true,
	.clocks			= {
#ifdef CONFIG_COMMON_CLK
		{ "ispb", UINT_MAX, 0 },
#else
		{ "isp", UINT_MAX, 0 },
#endif
#if !defined(CONFIG_TEGRA_BWMGR)
		{"emc", 0, NVHOST_MODULE_ID_EXTERNAL_MEMORY_CONTROLLER, }
#endif
	 },
	.finalize_poweron	= nvhost_isp_finalize_poweron,
	.prepare_poweroff	= nvhost_isp_prepare_poweroff,
	.hw_init		= nvhost_isp_register_isr_v1,
	.ctrl_ops		= &tegra_isp_ctrl_ops,
};
#endif

#if IS_ENABLED(CONFIG_VIDEO_TEGRA_VI) || IS_ENABLED(CONFIG_VIDEO_TEGRA_VI_MODULE)
struct nvhost_device_data t21_vi_info = {
	.modulemutexes		= {NVMODMUTEX_VI_0},
	.devfs_name		= "vi",
	.exclusive		= true,
	.class			= NV_VIDEO_STREAMING_VI_CLASS_ID,
	/* HACK: Mark as keepalive until 1188795 is fixed */
	.keepalive		= true,
	.autosuspend_delay	= 500,
	.can_powergate		= true,
	.moduleid		= NVHOST_MODULE_VI,
	.clocks = {
		/*
		 * for kernel-3.10, we need vi_bypass clock
		 * for kernel-4.4, we need vi clock
		 */
#ifdef CONFIG_COMMON_CLK
		{"vi", UINT_MAX},
#else
		{"vi_bypass", UINT_MAX},
#endif
		{"csi", 0},
		{"cilab", 102000000},
		{"cilcd", 102000000},
		{"cile", 102000000},
		{"vii2c", 86400000},
		{"i2cslow", 1000000},
#if !defined(CONFIG_TEGRA_BWMGR)
		{"emc", 0, NVHOST_MODULE_ID_EXTERNAL_MEMORY_CONTROLLER, }
#endif
	 },
	.ctrl_ops		= &tegra_vi_ctrl_ops,
	.num_channels		= 6,
	.slcg_notifier_enable	= true,
	.prepare_poweroff = nvhost_vi_prepare_poweroff,
	.finalize_poweron = nvhost_vi_finalize_poweron,
};
EXPORT_SYMBOL(t21_vi_info);
#endif

#if IS_ENABLED(CONFIG_TEGRA_GRHOST_NVENC)
struct nvhost_device_data t21_msenc_info = {
	.version		= NVHOST_ENCODE_FLCN_VER(5, 0),
	.class			= NV_VIDEO_ENCODE_NVENC_CLASS_ID,
	.modulemutexes		= {NVMODMUTEX_MSENC},
	.devfs_name		= "msenc",
	.autosuspend_delay	= 500,
	.can_powergate		= true,
	.clocks			= {{"msenc", UINT_MAX, 0},
				   {"emc", HOST_EMC_FLOOR,
				NVHOST_MODULE_ID_EXTERNAL_MEMORY_CONTROLLER,
#if defined(CONFIG_TEGRA_BWMGR)
				TEGRA_SET_EMC_SHARED_BW} },
#else
				} },
#endif
	.engine_cg_regs		= t21x_nvenc_gating_registers,
	.engine_can_cg		= true,
	.poweron_reset		= true,
	.finalize_poweron	= nvhost_flcn_finalize_poweron,
	.moduleid		= NVHOST_MODULE_MSENC,
	.num_channels		= 1,
	.scaling_init		= nvhost_scale_init,
	.scaling_deinit		= nvhost_scale_deinit,
	.actmon_regs		= HOST1X_CHANNEL_ACTMON1_REG_BASE,
	.mamask_addr		= 0x0000184c,
	.mamask_val		= 0x3d,
	.borps_addr		= 0x00001850,
	.borps_val		= 0x2008,
	.actmon_enabled		= true,
	.firmware_name		= "nvhost_nvenc050.fw",
	.resource_policy	= RESOURCE_PER_DEVICE,
	.serialize		= true,
#if defined(CONFIG_TEGRA_BWMGR)
	.bwmgr_client_id	= TEGRA_BWMGR_CLIENT_MSENC,
#endif
};
#endif

#if IS_ENABLED(CONFIG_TEGRA_GRHOST_NVDEC)
struct nvhost_device_data t21_nvdec_info = {
	.version		= NVHOST_ENCODE_NVDEC_VER(2, 0),
	.class			= NV_NVDEC_CLASS_ID,
	.modulemutexes		= {NVMODMUTEX_NVDEC},
	.devfs_name		= "nvdec",
	.autosuspend_delay	= 500,
	.can_powergate		= true,
	.clocks			= {{"nvdec", 0, 0},
				   {"emc", HOST_NVDEC_EMC_FLOOR,
				NVHOST_MODULE_ID_EXTERNAL_MEMORY_CONTROLLER} },
	.engine_cg_regs		= t21x_nvdec_gating_registers,
	.engine_can_cg		= true,
	.poweron_reset		= true,
	.finalize_poweron	= nvhost_nvdec_finalize_poweron,
	.moduleid		= NVHOST_MODULE_NVDEC,
	.ctrl_ops		= &tegra_nvdec_ctrl_ops,
	.num_channels		= 1,
	.scaling_init		= nvhost_scale_init,
	.scaling_deinit		= nvhost_scale_deinit,
	.actmon_regs		= HOST1X_CHANNEL_ACTMON3_REG_BASE,
	.mamask_addr		= 0x0000164c,
	.mamask_val		= 0x3d,
	.borps_addr		= 0x00001650,
	.borps_val		= 0x2008,
	.actmon_enabled		= true,
	.resource_policy	= RESOURCE_PER_DEVICE,
	.serialize		= true,
#if defined(CONFIG_TEGRA_BWMGR)
	.bwmgr_client_id	= TEGRA_BWMGR_CLIENT_NVDEC,
#endif
};
#endif

#if IS_ENABLED(CONFIG_TEGRA_GRHOST_NVJPG)
struct nvhost_device_data t21_nvjpg_info = {
	.version		= NVHOST_ENCODE_FLCN_VER(1, 0),
	.class			= NV_NVJPG_CLASS_ID,
	.modulemutexes		= {NVMODMUTEX_NVJPG},
	.devfs_name		= "nvjpg",
	.autosuspend_delay	= 500,
	.can_powergate		= true,
	.clocks			= { {"nvjpg", UINT_MAX, 0},
				    {"emc", HOST_EMC_FLOOR,
				NVHOST_MODULE_ID_EXTERNAL_MEMORY_CONTROLLER,
#if defined(CONFIG_TEGRA_BWMGR)
				TEGRA_SET_EMC_SHARED_BW} },
#else
				} },
#endif
	.engine_cg_regs		= t21x_nvjpg_gating_registers,
	.engine_can_cg		= true,
	.poweron_reset		= true,
	.finalize_poweron	= nvhost_flcn_finalize_poweron,
	.moduleid		= NVHOST_MODULE_NVJPG,
	.num_channels		= 1,
	.scaling_init		= nvhost_scale_init,
	.scaling_deinit		= nvhost_scale_deinit,
	.actmon_regs		= HOST1X_CHANNEL_ACTMON4_REG_BASE,
	.mamask_addr		= 0x0000144c,
	.mamask_val		= 0x3d,
	.borps_addr		= 0x00001450,
	.borps_val		= 0x2008,
	.actmon_enabled		= true,
	.resource_policy	= RESOURCE_PER_DEVICE,
	.serialize		= true,
	.firmware_name		= "nvhost_nvjpg010.fw",
#if defined(CONFIG_TEGRA_BWMGR)
	.bwmgr_client_id	= TEGRA_BWMGR_CLIENT_NVJPG,
#endif
};
#endif


#if IS_ENABLED(CONFIG_TEGRA_GRHOST_TSEC)
struct nvhost_device_data t21_tsec_info = {
	.num_channels		= 1,
	.modulemutexes		= {NVMODMUTEX_TSECA},
	.devfs_name		= "tsec",
	.version		= NVHOST_ENCODE_TSEC_VER(1, 0),
	.class			= NV_TSEC_CLASS_ID,
	.exclusive		= false,
	.clocks			= {{"tsec", UINT_MAX, 0},
				   {"emc", HOST_EMC_FLOOR,
				NVHOST_MODULE_ID_EXTERNAL_MEMORY_CONTROLLER} },
	.can_powergate		= true,
	.autosuspend_delay	= TSEC_AUTOSUSPEND_DELAY,
	.keepalive		= true,
	.moduleid		= NVHOST_MODULE_TSEC,
	.engine_can_cg		= true,
	.engine_cg_regs		= t21x_tsec_gating_registers,
	.poweron_reset		= true,
	.finalize_poweron	= nvhost_tsec_finalize_poweron,
	.prepare_poweroff	= nvhost_tsec_prepare_poweroff,
	.resource_policy	= RESOURCE_PER_CHANNEL_INSTANCE,
	.serialize		= true,
#if defined(CONFIG_TEGRA_BWMGR)
	.bwmgr_client_id	= TEGRA_BWMGR_CLIENT_TSEC,
#endif
};

struct nvhost_device_data t21_tsecb_info = {
	.num_channels		= 1,
	.modulemutexes		= {NVMODMUTEX_TSECB},
	.devfs_name		= "tsecb",
	.version		= NVHOST_ENCODE_TSEC_VER(1, 0),
	.class			= NV_TSECB_CLASS_ID,
	.exclusive		= false,
	.clocks			= {{"tsecb", UINT_MAX, 0},
				   {"emc", HOST_EMC_FLOOR,
				NVHOST_MODULE_ID_EXTERNAL_MEMORY_CONTROLLER} },
	.can_powergate		= true,
	.autosuspend_delay	= TSEC_AUTOSUSPEND_DELAY,
	.keepalive		= true,
	.moduleid		= NVHOST_MODULE_TSECB,
	.engine_can_cg		= true,
	.engine_cg_regs		= t21x_tsec_gating_registers,
	.poweron_reset		= true,
	.finalize_poweron	= nvhost_tsec_finalize_poweron,
	.prepare_poweroff	= nvhost_tsec_prepare_poweroff,
	.resource_policy	= RESOURCE_PER_CHANNEL_INSTANCE,
	.serialize		= true,
#if defined(CONFIG_TEGRA_BWMGR)
	.bwmgr_client_id	= TEGRA_BWMGR_CLIENT_TSECB,
#endif
};
#endif

#ifdef CONFIG_TEGRA_GRHOST_VIC
struct nvhost_device_data t21_vic_info = {
	.num_channels		= 1,
	.modulemutexes		= {NVMODMUTEX_VIC},
	.devfs_name		= "vic",
	.clocks			= {{"vic03", 140800000, 0},
				   {"emc", HOST_EMC_FLOOR,
				   NVHOST_MODULE_ID_EXTERNAL_MEMORY_CONTROLLER},
				   {"vic_floor", 0,
				   NVHOST_MODULE_ID_CBUS_FLOOR},
				   {"emc_shared", 0,
				   NVHOST_MODULE_ID_EMC_SHARED,
#if defined(CONFIG_TEGRA_BWMGR)
				   TEGRA_SET_EMC_SHARED_BW}, {} },
#else
				   }, {} },
#endif
	.version		= NVHOST_ENCODE_FLCN_VER(4, 0),
	.can_powergate		= true,
	.autosuspend_delay	= 500,
	.moduleid		= NVHOST_MODULE_VIC,
	.class			= NV_GRAPHICS_VIC_CLASS_ID,
	.engine_cg_regs		= t21x_vic_gating_registers,
	.engine_can_cg		= true,
	.poweron_toggle_slcg	= true,
	.finalize_poweron	= nvhost_vic_finalize_poweron,
	.prepare_poweroff	= nvhost_flcn_prepare_poweroff,
	.flcn_isr               = nvhost_flcn_common_isr,
	.module_irq             = 1,
	.scaling_init           = nvhost_scale_emc_init,
	.scaling_deinit         = nvhost_scale_emc_deinit,
	.scaling_post_cb	= &nvhost_scale_emc_callback,
	.actmon_regs            = HOST1X_CHANNEL_ACTMON2_REG_BASE,
	.linear_emc		= true,
	.actmon_enabled         = true,
	.actmon_irq		= 13,
	.devfreq_governor	= "wmark_active",
	.serialize		= true,
	.push_work_done		= true,
	.firmware_name		= "vic04_ucode.bin",
	.aggregate_constraints	= nvhost_vic_aggregate_constraints,
	.resource_policy	= RESOURCE_PER_DEVICE,
	.num_ppc		= 8,
#if defined(CONFIG_TEGRA_BWMGR)
	.bwmgr_client_id	= TEGRA_BWMGR_CLIENT_VIC,
#endif
};
#endif

#include "host1x/host1x_channel.c"

static void t210_set_nvhost_chanops(struct nvhost_channel *ch)
{
	if (ch)
		ch->ops = host1x_channel_ops;
}

static int nvhost_init_t210_channel_support(struct nvhost_master *host,
       struct nvhost_chip_support *op)
{
	op->nvhost_dev.set_nvhost_chanops = t210_set_nvhost_chanops;

	return 0;
}

static void t210_remove_support(struct nvhost_chip_support *op)
{
	kfree(op->priv);
	op->priv = NULL;
}

#include "host1x/host1x_cdma.c"
#include "host1x/host1x_syncpt.c"
#include "host1x/host1x_intr.c"
#define NVHOST_T210_ACTMON
#if IS_ENABLED(CONFIG_TEGRA_GRHOST_SCALE)
#include "host1x/host1x04_actmon.c"
#endif
#include "host1x/host1x_debug.c"

int nvhost_init_t210_support(struct nvhost_master *host,
       struct nvhost_chip_support *op)
{
	struct nvhost_device_data *data = platform_get_drvdata(host->dev);
	int err;

	op->soc_name = "tegra21x";

	/* don't worry about cleaning up on failure... "remove" does it. */
	err = nvhost_init_t210_channel_support(host, op);
	if (err)
		return err;

	op->cdma = host1x_cdma_ops;
	op->push_buffer = host1x_pushbuffer_ops;
	op->debug = host1x_debug_ops;
	host->sync_aperture = host->aperture + HOST1X_CHANNEL_SYNC_REG_BASE;
	op->syncpt = host1x_syncpt_ops;
	op->intr = host1x_intr_ops;
#if IS_ENABLED(CONFIG_TEGRA_GRHOST_SCALE)
	op->actmon = host1x_actmon_ops;
#endif

	if (nvhost_dev_is_virtual(host->dev)) {
		data->can_powergate = false;
		vhost_init_host1x_debug_ops(&op->debug);
	}

	op->remove_support = t210_remove_support;

	return 0;

}
