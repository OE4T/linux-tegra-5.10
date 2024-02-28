/*
 * amc.c
 *
 * AMC and ARAM handling
 *
 * Copyright (C) 2014-2021, NVIDIA Corporation. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/tegra_nvadsp.h>
#include <linux/irqchip/tegra-agic.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#if KERNEL_VERSION(4, 15, 0) > LINUX_VERSION_CODE
#include <soc/tegra/chip-id.h>
#else
#include <soc/tegra/fuse.h>
#endif

#include "dev.h"
#include "amc.h"

static struct platform_device *nvadsp_pdev;
static struct nvadsp_drv_data *nvadsp_drv_data;

static inline u32 amc_readl(u32 reg)
{
	return readl(nvadsp_drv_data->base_regs[AMC] + reg);
}

static inline void amc_writel(u32 val, u32 reg)
{
	writel(val, nvadsp_drv_data->base_regs[AMC] + reg);
}

static void wmemcpy_to_aram(u32 to_aram, const u32 *from_mem, size_t wlen)
{
	u32 base, offset;

	base = to_aram & AMC_ARAM_APERTURE_DATA_LEN;
	amc_writel(base, AMC_ARAM_APERTURE_BASE);

	offset = to_aram % AMC_ARAM_APERTURE_DATA_LEN;

	while (wlen--) {
		if (offset == AMC_ARAM_APERTURE_DATA_LEN) {
			base += AMC_ARAM_APERTURE_DATA_LEN;
			amc_writel(base, AMC_ARAM_APERTURE_BASE);
			offset = 0;
		}

		amc_writel(*from_mem, AMC_ARAM_APERTURE_DATA_START + offset);
		from_mem++;
		offset += 4;
	}
}

static void wmemcpy_from_aram(u32 *to_mem, const u32 from_aram, size_t wlen)
{
	u32 base, offset;

	base = from_aram & AMC_ARAM_APERTURE_DATA_LEN;
	amc_writel(base, AMC_ARAM_APERTURE_BASE);

	offset = from_aram % AMC_ARAM_APERTURE_DATA_LEN;

	while (wlen--) {
		if (offset == AMC_ARAM_APERTURE_DATA_LEN) {
			base += AMC_ARAM_APERTURE_DATA_LEN;
			amc_writel(base, AMC_ARAM_APERTURE_BASE);
			offset = 0;
		}

		*to_mem = amc_readl(AMC_ARAM_APERTURE_DATA_START + offset);
		to_mem++;
		offset += 4;
	}
}

int nvadsp_aram_save(struct platform_device *pdev)
{
	struct nvadsp_drv_data *d = platform_get_drvdata(pdev);

	wmemcpy_from_aram(d->state.aram, AMC_ARAM_START, AMC_ARAM_WSIZE);
	return 0;
}

int nvadsp_aram_restore(struct platform_device *pdev)
{
	struct nvadsp_drv_data *ndd = platform_get_drvdata(pdev);

	wmemcpy_to_aram(AMC_ARAM_START, ndd->state.aram, AMC_ARAM_WSIZE);
	return 0;
}

int nvadsp_amc_save(struct platform_device *pdev)
{
	struct nvadsp_drv_data *d = platform_get_drvdata(pdev);
	u32 val, offset = 0;
	int i = 0;

	offset = 0x0;
	val = readl(d->base_regs[AMC] + offset);
	d->state.amc_regs[i++] = val;

	offset = 0x8;
	val = readl(d->base_regs[AMC] + offset);
	d->state.amc_regs[i++] = val;

	return 0;
}

int nvadsp_amc_restore(struct platform_device *pdev)
{
	struct nvadsp_drv_data *d = platform_get_drvdata(pdev);
	u32 val, offset = 0;
	int i = 0;

	offset = 0x0;
	val = d->state.amc_regs[i++];
	writel(val, d->base_regs[AMC] + offset);

	offset = 0x8;
	val = d->state.amc_regs[i++];
	writel(val, d->base_regs[AMC] + offset);

	return 0;
}

static irqreturn_t nvadsp_amc_error_int_handler(int irq, void *devid)
{
	u32 val, addr, status, intr = 0;

	status = amc_readl(AMC_INT_STATUS);
	addr = amc_readl(AMC_ERROR_ADDR);

	if (status & AMC_INT_STATUS_ARAM) {
		/*
		 * Ignore addresses lesser than AMC_ERROR_ADDR_IGNORE (4k)
		 * as those are spurious ones due a hardware issue.
		 */
		if (!(nvadsp_drv_data->chip_data->amc_err_war) ||
				(addr > AMC_ERROR_ADDR_IGNORE))
			pr_info("nvadsp: invalid ARAM access. address: 0x%x\n",
				addr);

		intr |= AMC_INT_INVALID_ARAM_ACCESS;
	}

	if (status & AMC_INT_STATUS_REG) {
		pr_info("nvadsp: invalid AMC reg access. address: 0x%x\n",
			addr);
		intr |= AMC_INT_INVALID_REG_ACCESS;
	}

	val = amc_readl(AMC_INT_CLR);
	val |= intr;
	amc_writel(val, AMC_INT_CLR);

	return IRQ_HANDLED;
}

void nvadsp_free_amc_interrupts(struct platform_device *pdev)
{
	struct nvadsp_drv_data *drv = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;
	struct device_node *node;

	node = dev->of_node;

	if (!is_tegra_hypervisor_mode())
		devm_free_irq(dev, drv->agic_irqs[AMC_ERR_VIRQ], pdev);
}

int nvadsp_setup_amc_interrupts(struct platform_device *pdev)
{
	struct nvadsp_drv_data *drv = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;
	struct device_node *node;
	int ret = 0;

	node = dev->of_node;
	nvadsp_pdev = pdev;
	nvadsp_drv_data = drv;

	if (!is_tegra_hypervisor_mode())
		ret = devm_request_irq(dev, drv->agic_irqs[AMC_ERR_VIRQ],
			nvadsp_amc_error_int_handler, 0,
			"AMC error int", pdev);

	return ret;
}
