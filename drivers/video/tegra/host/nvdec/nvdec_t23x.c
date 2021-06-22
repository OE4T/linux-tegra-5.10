/*
 * Tegra NVDEC Module Support on T23x
 *
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
 */

#include <linux/slab.h>         /* for kzalloc */
#include <linux/iopoll.h>
#include <linux/dma-mapping.h>
#include <linux/platform/tegra/tegra_mc.h>
#if defined(CONFIG_TRUSTED_LITTLE_KERNEL) || defined(CONFIG_TRUSTY)
#include <linux/ote_protocol.h>
#endif
#include <soc/tegra/kfuse.h>

#include "dev.h"
#include "bus_client.h"
#include "riscv/riscv.h"
#include "nvhost_acm.h"
#include "platform.h"

#include "nvdec/nvdec.h"
#include "nvdec_t23x.h"
#include "nvdec/hw_nvdec_t23x.h"

#define NVDEC_DEBUGINFO_DUMMY		(0xabcd1234)
#define NVDEC_DEBUGINFO_CLEAR		(0x0)

static int nvdec_read_riscv_bin(struct platform_device *dev,
				const char *desc_bin_name)
{
	const struct firmware *desc_bin;
	struct nvhost_device_data *pdata = platform_get_drvdata(dev);
	struct riscv_data *m = (struct riscv_data *)pdata->riscv_data;

	if (!m) {
		dev_err(&dev->dev, "riscv data is NULL");
		return -ENODATA;
	}

	desc_bin = nvhost_client_request_firmware(dev, desc_bin_name, true);
	if (!desc_bin) {
		dev_err(&dev->dev, "failed to get desc binary");
		return -ENOENT;
	}

	/* Parse the desc binary for offsets */
	riscv_compute_ucode_offsets_2stage(dev, m, desc_bin);

	m->valid = true;
	release_firmware(desc_bin);

	return 0;
}

static int nvhost_nvdec_riscv_init_sw(struct platform_device *pdev)
{
	int err = 0;
	struct nvhost_device_data *pdata = platform_get_drvdata(pdev);
	struct riscv_data *m = (struct riscv_data *)pdata->riscv_data;

	if (m)
		return 0;

	m = kzalloc(sizeof(*m), GFP_KERNEL);
	if (!m) {
		dev_err(&pdev->dev, "Couldn't allocate memory for riscv data");
		return -ENOMEM;
	}
	pdata->riscv_data = m;

	err = nvdec_read_riscv_bin(pdev, pdata->riscv_desc_bin);
	if (err || !m->valid) {
		dev_err(&pdev->dev, "binary not valid");
		goto clean_up;
	}

	return 0;

clean_up:
	kfree(m);
	pdata->riscv_data = NULL;
	return err;
}

static void nvhost_nvdec_riscv_deinit_sw(struct platform_device *dev)
{
	struct nvhost_device_data *pdata = platform_get_drvdata(dev);
	struct riscv_data *m = (struct riscv_data *)pdata->riscv_data;

	kfree(m);
	pdata->riscv_data = NULL;
}

static int load_ucode(struct platform_device *dev, u64 base,
			struct riscv_image_desc desc)
{
	struct nvhost_device_data *pdata = platform_get_drvdata(dev);
	void __iomem *retcode_addr;
	u64 addr;
	int err = 0;
	u32 val;

	/* Load transcfg configuration if defined */
	if (pdata->transcfg_addr)
		host1x_writel(dev, pdata->transcfg_addr, pdata->transcfg_val);

	/* Select RISC-V core for nvdec */
	host1x_writel(dev, nvdec_riscv_bcr_ctrl_r(),
			nvdec_riscv_bcr_ctrl_core_select_riscv_f());

	/* Program manifest start address */
	addr = (base + desc.manifest_offset) >> 8;
	host1x_writel(dev, nvdec_riscv_bcr_dmaaddr_pkcparam_lo_r(),
			lower_32_bits(addr));
	host1x_writel(dev, nvdec_riscv_bcr_dmaaddr_pkcparam_hi_r(),
			upper_32_bits(addr));

	/* Program FMC code start address */
	addr = (base + desc.code_offset) >> 8;
	host1x_writel(dev, nvdec_riscv_bcr_dmaaddr_fmccode_lo_r(),
			lower_32_bits(addr));
	host1x_writel(dev, nvdec_riscv_bcr_dmaaddr_fmccode_hi_r(),
			upper_32_bits(addr));

	/* Program FMC data start address */
	addr = (base + desc.data_offset) >> 8;
	host1x_writel(dev, nvdec_riscv_bcr_dmaaddr_fmcdata_lo_r(),
			lower_32_bits(addr));
	host1x_writel(dev, nvdec_riscv_bcr_dmaaddr_fmcdata_hi_r(),
			upper_32_bits(addr));

	/* Program DMA config registers. GSC ID = 0x1 for CARVEOUT1 */
	host1x_writel(dev, nvdec_riscv_bcr_dmacfg_sec_r(),
			nvdec_riscv_bcr_dmacfg_sec_gscid_f(0x1));
	host1x_writel(dev, nvdec_riscv_bcr_dmacfg_r(),
			nvdec_riscv_bcr_dmacfg_target_local_fb_f() |
			nvdec_riscv_bcr_dmacfg_lock_locked_f());

	/* Write a knwon pattern into DEBUGINFO register */
	host1x_writel(dev, nvdec_debuginfo_r(), NVDEC_DEBUGINFO_DUMMY);

	/* Kick start RISC-V and let BR take over */
	host1x_writel(dev, nvdec_riscv_cpuctl_r(),
			nvdec_riscv_cpuctl_startcpu_true_f());

	/* Check BR return code */
	retcode_addr = get_aperture(dev, 0) + nvdec_riscv_br_retcode_r();
	err  = readl_poll_timeout(retcode_addr, val,
				(nvdec_riscv_br_retcode_result_v(val) ==
					nvdec_riscv_br_retcode_result_pass_v()),
				RISCV_IDLE_CHECK_PERIOD,
				RISCV_IDLE_TIMEOUT_DEFAULT);
	if (err) {
		dev_err(&dev->dev, "BR return code timeout! val=0x%x", val);
		return err;
	}

	return 0;
}

int nvhost_nvdec_riscv_finalize_poweron(struct platform_device *dev)
{
	struct riscv_data *m;
	struct mc_carveout_info inf;
	void __iomem *debuginfo_addr;
	u32 val;
	int err = 0;
	struct nvhost_device_data *pdata = platform_get_drvdata(dev);

	err = nvhost_nvdec_riscv_init_sw(dev);
	if (err)
		return err;

	m = (struct riscv_data *)pdata->riscv_data;

	/* Get GSC carvout info */
	err = mc_get_carveout_info(&inf, NULL, MC_SECURITY_CARVEOUT1);
	if (err || !inf.base) {
		dev_err(&dev->dev, "Carveout memory allocation failed");
		err = -ENOMEM;
		goto clean_up;
	}

	dev_dbg(&dev->dev, "CARVEOUT1 base=0x%llx size=0x%llx",
		inf.base, inf.size);

	/* Load BL ucode in stage-1*/
	err = load_ucode(dev, inf.base, m->bl);
	if (err) {
		dev_err(&dev->dev, "RISC-V stage-1 boot failed, err=0x%x", err);
		goto clean_up;
	}

	/* Check nvdec has reached a proper initialized state */
	debuginfo_addr = get_aperture(dev, 0) + nvdec_debuginfo_r();
	err  = readl_poll_timeout(debuginfo_addr, val,
				(val == NVDEC_DEBUGINFO_CLEAR),
				RISCV_IDLE_CHECK_PERIOD_LONG,
				RISCV_IDLE_TIMEOUT_LONG);
	if (err) {
		dev_err(&dev->dev,
			"RISC-V couldn't reach init state, timeout! val=0x%x",
			val);
		goto clean_up;
	}

	/* Reset NVDEC before stage-2 boot */
	nvhost_module_reset_for_stage2(dev);

	/* Load LS ucode in stage-2 */
	err = load_ucode(dev, inf.base, m->os);
	if (err) {
		dev_err(&dev->dev, "RISC-V stage-2 boot failed = 0x%x", err);
		goto clean_up;
	}

#if defined(CONFIG_TRUSTED_LITTLE_KERNEL)
	tlk_restore_keyslots();
#endif
#if defined(CONFIG_TRUSTY)
	trusty_restore_keyslots();
#endif
	dev_info(&dev->dev, "RISCV boot success");
	return 0;

clean_up:
	dev_err(&dev->dev, "RISCV boot failed");
	nvhost_nvdec_riscv_deinit_sw(dev);
	return err;

}

int nvhost_nvdec_finalize_poweron_t23x(struct platform_device *dev)
{
	int err;
	struct nvhost_device_data *pdata = platform_get_drvdata(dev);

	if (!pdata) {
		dev_info(&dev->dev, "no platform data");
		return -ENODATA;
	}

	if (!tegra_platform_is_vdk()) {
		err = tegra_kfuse_enable_sensing();
		if (err)
			return err;
	}

	flcn_enable_thi_sec(dev);

	if (pdata->enable_riscv_boot) {
		err = nvhost_nvdec_riscv_finalize_poweron(dev);
	} else {
		err = nvhost_nvdec_finalize_poweron(dev);
	}

	if (err && !tegra_platform_is_vdk()) {
		tegra_kfuse_disable_sensing();
	}

	return err;
}

int nvhost_nvdec_prepare_poweroff_t23x(struct platform_device *dev)
{
	if (!tegra_platform_is_vdk()) {
		tegra_kfuse_disable_sensing();
	}

	return 0;
}

