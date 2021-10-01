/*
 * Tegra TSEC Module Support on t23x
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
#include <linux/delay.h>	/* for udelay */
#include <linux/iommu.h>
#include <linux/iopoll.h>
#include <linux/dma-mapping.h>
#include <linux/platform/tegra/tegra_mc.h>

#include "dev.h"
#include "bus_client.h"
#include "tsec/tsec.h"
#include "tsec_t23x.h"
#include "hw_tsec_t23x.h"
#include "flcn/flcn.h"
#include "riscv/riscv.h"


#define TSEC_RISCV_INIT_SUCCESS		(0xa5a5a5a5)
#define NV_RISCV_AMAP_FBGPA_START	0x0000040000000000ULL
#define NV_RISCV_AMAP_SMMU_IDX		BIT_ULL(40)

/* 'N' << 24 | 'V' << 16 | 'R' << 8 | 'M' */
#define RM_RISCV_BOOTLDR_BOOT_TYPE_RM	0x4e56524d

/* Version of bootloader struct, increment on struct changes (while on prod) */
#define RM_RISCV_BOOTLDR_VERSION	1

/* Configuration for bootloader */
typedef struct {
	/*
	 *                   *** WARNING ***
	 * First 3 fields must be frozen like that always. Should never
	 * be reordered or changed.
	 */
	u32 bootType;        // Set to 'NVRM' if booting from RM.
	u16 size;            // Size of boot params.
	u8  version;         // Version of boot params.
	/*
	 * You can reorder or change below this point but update version.
	 */
} NV_RISCV_BOOTLDR_PARAMS;


static int tsec_read_riscv_bin(struct platform_device *dev,
			const char *desc_name,
			const char *image_name)
{
	int err, w;
	const struct firmware *riscv_desc, *riscv_image;
	struct nvhost_device_data *pdata = platform_get_drvdata(dev);
	struct riscv_data *m = (struct riscv_data *)pdata->riscv_data;

	if (!m) {
		dev_err(&dev->dev, "riscv data is NULL\n");
		return -ENODATA;
	}

	m->dma_addr = 0;
	m->mapped = NULL;
	riscv_desc = nvhost_client_request_firmware(dev, desc_name, true);
	if (!riscv_desc) {
		dev_err(&dev->dev, "failed to get tsec desc binary\n");
		return -ENOENT;
	}

	riscv_image = nvhost_client_request_firmware(dev, image_name, true);
	if (!riscv_image) {
		dev_err(&dev->dev, "failed to get tsec image binary\n");
		release_firmware(riscv_desc);
		return -ENOENT;
	}

	m->size = riscv_image->size;
	m->mapped = dma_alloc_attrs(&dev->dev, m->size, &m->dma_addr,
				GFP_KERNEL,
				DMA_ATTR_READ_ONLY | DMA_ATTR_FORCE_CONTIGUOUS);
	if (!m->mapped) {
		dev_err(&dev->dev, "dma memory allocation failed");
		err = -ENOMEM;
		goto clean_up;
	}

	/* Copy the whole image taking endianness into account */
	for (w = 0; w < riscv_image->size/sizeof(u32); w++)
		m->mapped[w] = le32_to_cpu(((__le32 *)riscv_image->data)[w]);

	/* Read the offsets from desc binary */
	err = riscv_compute_ucode_offsets(dev, m, riscv_desc);
	if (err) {
		dev_err(&dev->dev, "failed to parse desc binary\n");
		goto clean_up;
	}

	m->valid = true;
	release_firmware(riscv_desc);
	release_firmware(riscv_image);

	return 0;

clean_up:
	if (m->mapped) {
		dma_free_attrs(&dev->dev, m->size, m->mapped, m->dma_addr,
				DMA_ATTR_READ_ONLY | DMA_ATTR_FORCE_CONTIGUOUS);
		m->mapped = NULL;
		m->dma_addr = 0;
	}
	release_firmware(riscv_desc);
	release_firmware(riscv_image);
	return err;
}

static int nvhost_tsec_riscv_init_sw(struct platform_device *dev)
{
	int err = 0;
	NV_RISCV_BOOTLDR_PARAMS *bl_args;
	struct nvhost_device_data *pdata = platform_get_drvdata(dev);
	struct riscv_data *m = (struct riscv_data *)pdata->riscv_data;

	if (m)
		return 0;

	m = kzalloc(sizeof(*m), GFP_KERNEL);
	if (!m) {
		dev_err(&dev->dev, "Couldn't allocate for riscv info struct");
		return -ENOMEM;
	}
	pdata->riscv_data = m;

	err = tsec_read_riscv_bin(dev, pdata->riscv_desc_bin,
				pdata->riscv_image_bin);

	if (err || !m->valid) {
		dev_err(&dev->dev, "ucode not valid");
		goto clean_up;
	}

	/*
	 * TSEC firmware expects BL arguments in struct RM_GSP_BOOT_PARAMS.
	 * But, we only populate the first few fields of it. ie.
	 * NV_RISCV_BOOTLDR_PARAMS is located at offset 0 of RM_GSP_BOOT_PARAMS.
	 * actual sizeof(RM_GSP_BOOT_PARAMS) = 152
	 */
	m->bl_args_size = 152;
	m->mapped_bl_args = dma_alloc_attrs(&dev->dev, m->bl_args_size,
				&m->dma_addr_bl_args, GFP_KERNEL, 0);
	if (!m->mapped_bl_args) {
		dev_err(&dev->dev, "dma memory allocation for BL args failed");
		err = -ENOMEM;
		goto clean_up;
	}
	bl_args = (NV_RISCV_BOOTLDR_PARAMS *) m->mapped_bl_args;
	bl_args->bootType = RM_RISCV_BOOTLDR_BOOT_TYPE_RM;
	bl_args->size = m->bl_args_size;
	bl_args->version = RM_RISCV_BOOTLDR_VERSION;

	return 0;

clean_up:
	dev_err(&dev->dev, "RISC-V init sw failed: err=%d", err);
	kfree(m);
	pdata->riscv_data = NULL;
	return err;
}

static int nvhost_tsec_riscv_deinit_sw(struct platform_device *dev)
{
	struct nvhost_device_data *pdata = platform_get_drvdata(dev);
	struct riscv_data *m = (struct riscv_data *)pdata->riscv_data;

	if (!m)
		return 0;

	if (m->mapped) {
		dma_free_attrs(&dev->dev, m->size, m->mapped, m->dma_addr,
				DMA_ATTR_READ_ONLY | DMA_ATTR_FORCE_CONTIGUOUS);
		m->mapped = NULL;
		m->dma_addr = 0;
	}
	if (m->mapped_bl_args) {
		dma_free_attrs(&dev->dev, m->bl_args_size, m->mapped_bl_args,
				m->dma_addr_bl_args, 0);
		m->mapped_bl_args = NULL;
		m->dma_addr_bl_args = 0;
	}
	kfree(m);
	pdata->riscv_data = NULL;
	return 0;
}

static int nvhost_tsec_riscv_poweron(struct platform_device *dev)
{
	int err = 0;
	struct riscv_data *m;
	u32 val;
	phys_addr_t dma_pa, pa;
	struct iommu_domain *domain;
	void __iomem *cpuctl_addr, *retcode_addr, *mailbox0_addr;
	struct mc_carveout_info inf;
	unsigned int gscid = 0x0;
	dma_addr_t bl_args_iova;
	struct nvhost_device_data *pdata = platform_get_drvdata(dev);

	err = nvhost_tsec_riscv_init_sw(dev);
	if (err)
		return err;

	m = (struct riscv_data *)pdata->riscv_data;

	/* Select RISC-V core */
	host1x_writel(dev, tsec_riscv_bcr_ctrl_r(),
			tsec_riscv_bcr_ctrl_core_select_riscv_f());

	/* Get the physical address of corresponding dma address */
	domain = iommu_get_domain_for_dev(&dev->dev);

	/* Get GSC carvout info */
	err = mc_get_carveout_info(&inf, NULL, MC_SECURITY_CARVEOUT4);
	if (err) {
		dev_err(&dev->dev, "Carveout memory allocation failed");
		err = -ENOMEM;
		goto clean_up;
	}

	dev_dbg(&dev->dev, "CARVEOUT4 base=0x%llx size=0x%llx\n",
		inf.base, inf.size);
	if (inf.base) {
		dma_pa = inf.base;
		gscid = 0x4;
		dev_info(&dev->dev, "RISC-V booting from GSC\n");
	} else {
		/* For non-secure boot only. It can be depricated later */
		dma_pa = iommu_iova_to_phys(domain, m->dma_addr);
		dev_info(&dev->dev, "RISC-V boot using kernel allocated Mem\n");
	}

	/* Program manifest start address */
	pa = (dma_pa + m->os.manifest_offset) >> 8;
	host1x_writel(dev, tsec_riscv_bcr_dmaaddr_pkcparam_lo_r(),
			lower_32_bits(pa));
	host1x_writel(dev, tsec_riscv_bcr_dmaaddr_pkcparam_hi_r(),
			upper_32_bits(pa));

	/* Program FMC code start address */
	pa = (dma_pa + m->os.code_offset) >> 8;
	host1x_writel(dev, tsec_riscv_bcr_dmaaddr_fmccode_lo_r(),
			lower_32_bits(pa));
	host1x_writel(dev, tsec_riscv_bcr_dmaaddr_fmccode_hi_r(),
			upper_32_bits(pa));

	/* Program FMC data start address */
	pa = (dma_pa + m->os.data_offset) >> 8;
	host1x_writel(dev, tsec_riscv_bcr_dmaaddr_fmcdata_lo_r(),
			lower_32_bits(pa));
	host1x_writel(dev, tsec_riscv_bcr_dmaaddr_fmcdata_hi_r(),
			upper_32_bits(pa));

	/* Program DMA config registers */
	host1x_writel(dev, tsec_riscv_bcr_dmacfg_sec_r(),
			tsec_riscv_bcr_dmacfg_sec_gscid_f(gscid));
	host1x_writel(dev, tsec_riscv_bcr_dmacfg_r(),
			tsec_riscv_bcr_dmacfg_target_local_fb_f() |
			tsec_riscv_bcr_dmacfg_lock_locked_f());

	/* Pass the address of BL argument struct via mailbox registers */
	bl_args_iova = m->dma_addr_bl_args + NV_RISCV_AMAP_FBGPA_START;
	bl_args_iova |= NV_RISCV_AMAP_SMMU_IDX;
	host1x_writel(dev, tsec_falcon_mailbox0_r(),
			lower_32_bits((unsigned long long)bl_args_iova));
	host1x_writel(dev, tsec_falcon_mailbox1_r(),
			upper_32_bits((unsigned long long)bl_args_iova));

	/* Kick start RISC-V and let BR take over */
	host1x_writel(dev, tsec_riscv_cpuctl_r(),
			tsec_riscv_cpuctl_startcpu_true_f());

	cpuctl_addr = get_aperture(dev, 0) + tsec_riscv_cpuctl_r();
	retcode_addr = get_aperture(dev, 0) + tsec_riscv_br_retcode_r();
	mailbox0_addr = get_aperture(dev, 0) + tsec_falcon_mailbox0_r();

	/* Check BR return code */
	err  = readl_poll_timeout(retcode_addr, val,
				(tsec_riscv_br_retcode_result_v(val) ==
				 tsec_riscv_br_retcode_result_pass_v()),
				RISCV_IDLE_CHECK_PERIOD,
				RISCV_IDLE_TIMEOUT_DEFAULT);
	if (err) {
		dev_err(&dev->dev, "BR return code timeout! val=0x%x\n", val);
		goto clean_up;
	}

	/* Check cpuctl active state */
	err  = readl_poll_timeout(cpuctl_addr, val,
				(tsec_riscv_cpuctl_active_stat_v(val) ==
				 tsec_riscv_cpuctl_active_stat_active_v()),
				RISCV_IDLE_CHECK_PERIOD,
				RISCV_IDLE_TIMEOUT_DEFAULT);
	if (err) {
		dev_err(&dev->dev, "cpuctl active state timeout! val=0x%x\n",
			val);
		goto clean_up;
	}

	/* Check tsec has reached a proper initialized state */
	err  = readl_poll_timeout(mailbox0_addr, val,
				(val == TSEC_RISCV_INIT_SUCCESS),
				RISCV_IDLE_CHECK_PERIOD_LONG,
				RISCV_IDLE_TIMEOUT_LONG);
	if (err) {
		dev_err(&dev->dev,
			"not reached initialized state, timeout! val=0x%x\n",
			val);
		goto clean_up;
	}

	/* Booted-up successfully */
	dev_info(&dev->dev, "RISC-V boot success\n");
	return err;

clean_up:
	nvhost_tsec_riscv_deinit_sw(dev);
	return err;
}

int nvhost_tsec_finalize_poweron_t23x(struct platform_device *dev)
{
	struct nvhost_device_data *pdata = platform_get_drvdata(dev);

	if (!pdata) {
		dev_err(&dev->dev, "no platform data\n");
		return -ENODATA;
	}

	flcn_enable_thi_sec(dev);
	if (pdata->enable_riscv_boot) {
		return nvhost_tsec_riscv_poweron(dev);
	} else {
		dev_err(&dev->dev,
			"Falcon boot is not supported from t23x tsec driver\n");
		return -ENOTSUPP;
	}
}

int nvhost_tsec_prepare_poweroff_t23x(struct platform_device *dev)
{
	/*
	 * Below call is redundant, but there are something statically declared
	 * in $(srctree.nvidia)/drivers/video/tegra/host/tsec/tsec.c,
	 * which needs to be reset.
	 */
	nvhost_tsec_prepare_poweroff(dev);
	return 0;
}

