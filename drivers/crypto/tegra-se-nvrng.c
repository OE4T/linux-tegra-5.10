/*
 * drivers/crypto/tegra-se-nvrng.c
 *
 * Support for Tegra NVRNG Engine Error Handling.
 *
 * Copyright (c) 2020, NVIDIA CORPORATION.  All rights reserved.
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

#include <asm/io.h>
#include <linux/acpi.h>
#include <linux/clk.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#define NV_NVRNG_R_IE_0		0x80
#define NV_NVRNG_R_ISTAT_0	0x84
#define NV_NVRNG_R_CTRL1_0	0x90

#define HALTED			0x4
#define STARTUP_DONE		0x2
#define ERROR			0x1

#define HALT			0x10
#define SOFT_RST		0x1

#define CLK_RATE		38400

struct tegra_se_nvrng_dev {
	void __iomem	*base;
	int		irq;
	struct clk	*clk;
};

static unsigned int tegra_se_nvrng_readl(struct tegra_se_nvrng_dev *nvrng_dev,
				 unsigned int offset)
{
	return readl(nvrng_dev->base + offset);
}

static void tegra_se_nvrng_writel(struct tegra_se_nvrng_dev *nvrng_dev,
				  unsigned int offset, unsigned int value)
{
	writel(value, nvrng_dev->base + offset);
}

static irqreturn_t tegra_se_nvrng_isr(int irq, void *dev_id)
{
	int handled = 0;
	unsigned int mask, status;
	struct tegra_se_nvrng_dev *nvrng_dev = dev_id;

	/* Handle the interrupt if issued for an error condition.
	 * Ignore the interrupt otherwise.
	 */
	status = tegra_se_nvrng_readl(nvrng_dev, NV_NVRNG_R_ISTAT_0);
	if (status & ERROR) {
		mask = tegra_se_nvrng_readl(nvrng_dev, NV_NVRNG_R_IE_0);

		/* Disable STARTUP_DONE & ERROR interrupts. */
		mask &= ~(STARTUP_DONE | ERROR);
		tegra_se_nvrng_writel(nvrng_dev, NV_NVRNG_R_IE_0, mask);

		/* Halt NVRNG and enable HALT interrupt. */
		tegra_se_nvrng_writel(nvrng_dev, NV_NVRNG_R_CTRL1_0, HALT);
		tegra_se_nvrng_writel(nvrng_dev, NV_NVRNG_R_IE_0, HALTED);

		handled = 1;
	} else if (status & HALTED) {
		mask = tegra_se_nvrng_readl(nvrng_dev, NV_NVRNG_R_IE_0);

		/* Disable HALT interrupt. */
		mask &= ~HALTED;
		tegra_se_nvrng_writel(nvrng_dev, NV_NVRNG_R_IE_0, mask);

		/* Soft reset NVRNG and enable STARTUP_DONE interrupt. */
		mask |= STARTUP_DONE;
		tegra_se_nvrng_writel(nvrng_dev, NV_NVRNG_R_CTRL1_0, SOFT_RST);
		tegra_se_nvrng_writel(nvrng_dev, NV_NVRNG_R_IE_0, STARTUP_DONE);
		handled = 1;
	} else {
		/* Soft reset complete, enable ERROR interrupt*/
		tegra_se_nvrng_writel(nvrng_dev, NV_NVRNG_R_IE_0, ERROR);
		handled = 1;
	}

	return IRQ_RETVAL(handled);
}

static int tegra_se_nvrng_request_irq(struct tegra_se_nvrng_dev *nvrng_dev)
{
	int ret;
	unsigned int mask;

	ret = request_irq(nvrng_dev->irq, tegra_se_nvrng_isr, 0,
			  "tegra-se-nvrng", nvrng_dev);

	/* Set NV_NVRNG_R_IE_0.ERROR = Enabled.
	 * This will enable interrupts for errors.
	 */
	mask = tegra_se_nvrng_readl(nvrng_dev, NV_NVRNG_R_IE_0);
	tegra_se_nvrng_writel(nvrng_dev, NV_NVRNG_R_IE_0,
			      mask & ERROR);

	return ret;
}

static int tegra_se_nvrng_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct tegra_se_nvrng_dev *nvrng_dev;

	nvrng_dev = devm_kzalloc(&pdev->dev, sizeof(struct tegra_se_nvrng_dev),
				 GFP_KERNEL);
	if (!nvrng_dev)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	nvrng_dev->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(nvrng_dev->base))
		return PTR_ERR(nvrng_dev->base);

	nvrng_dev->irq = platform_get_irq(pdev, 0);
	if (nvrng_dev->irq < 0) {
		if (nvrng_dev->irq != -EPROBE_DEFER)
			dev_err(&pdev->dev, "cannot obtain irq\n");
		return nvrng_dev->irq;
	}

	nvrng_dev->clk = devm_clk_get(&pdev->dev, "se");
	if (IS_ERR(nvrng_dev->clk))
		return PTR_ERR(nvrng_dev->clk);

	clk_prepare_enable(nvrng_dev->clk);
	clk_set_rate(nvrng_dev->clk, CLK_RATE);

	platform_set_drvdata(pdev, nvrng_dev);

	return tegra_se_nvrng_request_irq(nvrng_dev);
}

static int tegra_se_nvrng_remove(struct platform_device *pdev)
{
	struct tegra_se_nvrng_dev *nvrng_dev =
		(struct tegra_se_nvrng_dev *)platform_get_drvdata(pdev);

	free_irq(nvrng_dev->irq, nvrng_dev);
	clk_disable_unprepare(nvrng_dev->clk);
	devm_clk_put(&pdev->dev, nvrng_dev->clk);

	return 0;
}

#ifdef CONFIG_ACPI
static const struct acpi_device_id tegra_se_nvrng_acpi_match[] = {
	{}
};
MODULE_DEVICE_TABLE(acpi, tegra_se_nvrng_acpi_match);
#endif /* CONFIG_ACPI */

static const struct of_device_id tegra_se_nvrng_of_match[] = {
	{ .compatible = "nvidia,tegra234-se-nvrng" },
	{}
};
MODULE_DEVICE_TABLE(of, tegra_se_nvrng_of_match);

static struct platform_driver tegra_se_nvrng_driver = {
	.probe = tegra_se_nvrng_probe,
	.remove = tegra_se_nvrng_remove,
	.driver = {
		.name = "tegra-se-nvrng",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(tegra_se_nvrng_of_match),
#ifdef CONFIG_ACPI
		.acpi_match_table = ACPI_PTR(tegra_se_nvrng_acpi_match),
#endif /* CONFIG_ACPI */
	},
};

static int __init tegra_se_nvrng_module_init(void)
{
	return platform_driver_register(&tegra_se_nvrng_driver);
}

static void __exit tegra_se_nvrng_module_exit(void)
{
	platform_driver_unregister(&tegra_se_nvrng_driver);
}

module_init(tegra_se_nvrng_module_init);
module_exit(tegra_se_nvrng_module_exit);

MODULE_AUTHOR("Kartik <kkartik@nvidia.com>");
MODULE_DESCRIPTION("Tegra Crypto NVRNG error handling support");
MODULE_LICENSE("GPL");
MODULE_ALIAS("tegra-se-nvrng");
