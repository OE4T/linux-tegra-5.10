/*
 * Copyright (c) 2017-2019, NVIDIA CORPORATION.  All rights reserved.
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
 * The driver handles Error's from Control Backbone(CBB) generated due to
 * illegal accesses. When an error is reported from a NOC within CBB,
 * the driver prints error type and debug information about failed transaction.
 */

#include <asm/traps.h>
#include <linux/clk.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <soc/tegra/chip-id.h>
#include <linux/platform/tegra/tegra_cbb.h>

static void __iomem **axi2apb_bases;
static struct tegra_cbberr_ops *cbberr_ops;

static char *tegra_axi2apb_errors[] = {
	"SFIFONE - Status FIFO Not Empty interrupt",
	"SFIFOF - Status FIFO Full interrupt",
	"TIM - Timer(Timeout) interrupt",
	"SLV - SLVERR interrupt",
	"NULL",
	"ERBF - Early response buffer Full interrupt",
	"NULL",
	"RDFIFOF - Read Response FIFO Full interrupt",
	"WRFIFOF - Write Response FIFO Full interrupt",
	"CH0DFIFOF - Ch0 Data FIFO Full interrupt",
	"CH1DFIFOF - Ch1 Data FIFO Full interrupt",
	"CH2DFIFOF - Ch2 Data FIFO Full interrupt",
	"UAT - Unsupported alignment type error",
	"UBS - Unsupported burst size error",
	"UBE - Unsupported Byte Enable error",
	"UBT - Unsupported burst type error",
	"BFS - Block Firewall security error",
	"ARFS - Address Range Firewall security error",
	"CH0RFIFOF - Ch0 Request FIFO Full interrupt",
	"CH1RFIFOF - Ch1 Request FIFO Full interrupt",
	"CH2RFIFOF - Ch2 Request FIFO Full interrupt"
};

void print_cbb_err(struct seq_file *file, const char *fmt, ...)
{
	va_list args;
	struct va_format vaf;

	va_start(args, fmt);

	if (file) {
		seq_vprintf(file, fmt, args);
	} else {
		vaf.fmt = fmt;
		vaf.va = &args;
		pr_crit("%pV", &vaf);
	}

	va_end(args);
}

void print_cache(struct seq_file *file, u32 cache)
{
	if ((cache & 0x3) == 0x0) {
		print_cbb_err(file, "\t  Cache\t\t\t: 0x%x -- "
			"Non-cacheable/Non-Bufferable)\n", cache);
		return;
	}
	if ((cache & 0x3) == 0x1) {
		print_cbb_err(file, "\t  Cache\t\t\t: 0x%x -- Device\n", cache);
		return;
	}

	switch (cache) {
	case 0x2:
		print_cbb_err(file,
		"\t  Cache\t\t\t: 0x%x -- Cacheable/Non-Bufferable\n", cache);
		break;

	case 0x3:
		print_cbb_err(file,
		"\t  Cache\t\t\t: 0x%x -- Cacheable/Bufferable\n", cache);
		break;

	default:
		print_cbb_err(file,
		"\t  Cache\t\t\t: 0x%x -- Cacheable\n", cache);
	}
}

void print_prot(struct seq_file *file, u32 prot)
{
	char *data_str;
	char *secure_str;
	char *priv_str;

	data_str = (prot & 0x4) ? "Instruction" : "Data";
	secure_str = (prot & 0x2) ? "Non-Secure" : "Secure";
	priv_str = (prot & 0x1) ? "Privileged" : "Unprivileged";

	print_cbb_err(file, "\t  Protection\t\t: 0x%x -- %s, %s, %s Access\n",
			prot, priv_str, secure_str, data_str);
}


#ifdef CONFIG_DEBUG_FS
static int created_root;

static int cbb_err_show(struct seq_file *file, void *data)
{
	return cbberr_ops->cbb_err_debugfs_show(file, data);
}

static int cbb_err_open(struct inode *inode, struct file *file)
{
	return single_open(file, cbb_err_show, inode->i_private);
}

static const struct file_operations cbb_err_fops = {
	.open = cbb_err_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};
static int cbb_noc_dbgfs_init(void)
{
	struct dentry *d;
	if (!created_root) {
		d = debugfs_create_file("tegra_cbb_err",
				S_IRUGO, NULL, NULL, &cbb_err_fops);
		if (IS_ERR_OR_NULL(d)) {
			pr_err("%s: could not create 'tegra_cbb_err' node\n",
					__func__);
			return PTR_ERR(d);
		}
		created_root = true;
	}
	return 0;
}

#else
static int cbb_noc_dbgfs_init(void) { return 0; }
#endif

void tegra_cbb_stallen(void __iomem *addr)
{
	cbberr_ops->stallen(addr);
}

void tegra_cbb_faulten(void __iomem *addr)
{
	cbberr_ops->faulten(addr);
}

void tegra_cbb_errclr(void __iomem *addr)
{
	cbberr_ops->errclr(addr);
}

unsigned int tegra_cbb_errvld(void __iomem *addr)
{
	return cbberr_ops->errvld(addr);
}

void tegra_cbberr_set_ops(struct tegra_cbberr_ops *tegra_cbb_err_ops)
{
	cbberr_ops = tegra_cbb_err_ops;
}

static const struct of_device_id axi2apb_match[] = {
	{ .compatible = "nvidia,tegra194-AXI2APB-bridge", },
	{},
};

int tegra_cbb_axi2apb_bridge_data(struct platform_device *pdev,
		int *apb_bridge_cnt,
		void __iomem ***bases)
{
	struct device_node *np;
	int ret = 0, i = 0;

	if (axi2apb_bases == NULL) {
		np = of_find_matching_node(NULL, axi2apb_match);
		if (np == NULL) {
			dev_info(&pdev->dev, "No match found for axi2apb\n");
			return -ENOENT;
		}
		*apb_bridge_cnt = (of_property_count_elems_of_size
					(np, "reg", sizeof(u32)))/4;

		axi2apb_bases = devm_kzalloc(&pdev->dev,
				sizeof(void *) * (*apb_bridge_cnt), GFP_KERNEL);
		if (axi2apb_bases == NULL)
			return -ENOMEM;

		for (i = 0; i < *apb_bridge_cnt; i++) {
			void __iomem *base = of_iomap(np, i);

			if (base == NULL) {
				dev_err(&pdev->dev,
					"failed to map axi2apb range\n");
				return -ENOENT;
			}
			axi2apb_bases[i] = base;
		}
	}
	*bases = axi2apb_bases;

	return ret;
}


unsigned int tegra_axi2apb_errstatus(void __iomem *addr)
{
	unsigned int error_status;

	error_status = readl(addr+DMAAPB_X_RAW_INTERRUPT_STATUS);
	writel(0xFFFFFFFF, addr+DMAAPB_X_RAW_INTERRUPT_STATUS);
	return error_status;
}

void tegra_axi2apb_err(struct seq_file *file, int bridge, u32 bus_status)
{
	int max_axi2apb_err = ARRAY_SIZE(tegra_axi2apb_errors);
	int j = 0;

	for (j = 0; j < max_axi2apb_err; j++) {
		if (bus_status & (1<<j))
			print_cbb_err(file, "\t  "
				"AXI2APB_%d bridge error: %s\n"
				, bridge, tegra_axi2apb_errors[j]);
	}
}

int tegra_cbb_err_getirq(struct platform_device *pdev,
		int *nonsecure_irq, int *secure_irq, int *num_intr)
{
	int err = 0;
	*nonsecure_irq = 0;

	*num_intr = platform_irq_count(pdev);
	if (!*num_intr)
		return -EINVAL;

	if (*num_intr == 2) {
		*nonsecure_irq = platform_get_irq(pdev, 0);
		if (*nonsecure_irq <= 0) {
			dev_err(&pdev->dev, "can't get irq (%d)\n",
					*nonsecure_irq);
			return -ENOENT;
		}
	}

	*secure_irq = platform_get_irq(pdev, 1);
	if (*secure_irq <= 0) {
		dev_err(&pdev->dev, "can't get irq (%d)\n", *secure_irq);
		return -ENOENT;
	}

	if (*num_intr == 1)
		dev_info(&pdev->dev, "secure_irq = %d\n", *secure_irq);
	if (*num_intr == 2)
		dev_info(&pdev->dev, "secure_irq = %d, nonsecure_irq = %d>\n",
						*secure_irq, *nonsecure_irq);
	return err;
}

int tegra_cbberr_register_hook_en(struct platform_device *pdev,
			const struct tegra_cbb_noc_data *bdata,
			struct serr_hook *callback,
			struct tegra_cbb_init_data cbb_init_data)
{
	int ret = 0;

	/* register handler for CBB errors due to CCPLEX master*/
	register_serr_hook(callback);

	/* register handler for CBB errors due to masters other than CCPLEX*/
	ret = cbberr_ops->cbb_enable_interrupt(pdev, cbb_init_data.secure_irq,
				cbb_init_data.nonsecure_irq);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to register CBB Interrupt ISR");
		return ret;
	}

	cbberr_ops->cbb_error_enable(cbb_init_data.vaddr);

	dsb(sy);

	return ret;
}

static int __init tegra_cbb_init(void)
{
	int err = 0;

        /*
         * CBB don't exist on the simulator
         */
        if (tegra_cpu_is_asim() &&
		(tegra_get_chipid() != TEGRA_CHIPID_TEGRA19))
		return 0;

	err = cbb_noc_dbgfs_init();
	if (err)
		return err;

	return 0;
}

static void __exit tegra_cbb_exit(void)
{
}

postcore_initcall(tegra_cbb_init);
module_exit(tegra_cbb_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("SError handler for errors within Control Backbone");
