/*
 * Copyright (c) 2019-2020, NVIDIA CORPORATION.  All rights reserved.
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
 * the driver checks ErrVld status of all three Error Logger's of that NOC.
 * It then prints debug information about failed transaction using ErrLog
 * registers of error logger which has ErrVld set. Currently, SLV, DEC,
 * TMO, SEC, UNS are the only codes which are supported by CBB.
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
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0)
#include <soc/tegra/chip-id.h>
#else
#include <soc/tegra/fuse.h>
#endif
#include <linux/platform/tegra/tegra_cbb.h>
#include <linux/platform/tegra/tegra23x_cbb.h>

#define get_mstr_id(userbits) get_em_el_subfield(errmon->user_bits, 29, 24)

static LIST_HEAD(cbb_errmon_list);
static DEFINE_SPINLOCK(cbb_errmon_lock);


static void tegra234_cbb_errmon_faulten(void __iomem *addr)
{
	writel(0x1FF, addr + FABRIC_EN_CFG_INTERRUPT_ENABLE_0_0);
	dsb(sy);
}

static void tegra234_cbb_errmon_errclr(void __iomem *addr)
{
	writel(0x3F, addr + FABRIC_MN_MASTER_ERR_STATUS_0);
	dsb(sy);
}

static unsigned int tegra234_cbb_errmon_errvld(void __iomem *addr)
{
	unsigned int errvld_status = 0;

	errvld_status = readl(addr + FABRIC_EN_CFG_STATUS_0_0);

	dsb(sy);
	return errvld_status;
}

static void print_errmon_err(struct seq_file *file,
		struct tegra_cbb_errmon_record *errmon,
		unsigned int errmon_err_status,
		unsigned int errmon_overflow_status)
{
	int err_type = 0;

	if (errmon_err_status & (errmon_err_status - 1))
		print_cbb_err(file, "\t  Multiple type of errors reported\n");

	while (errmon_err_status) {
		if (errmon_err_status & 0x1)
			print_cbb_err(file, "\t  Error Code\t\t: %s\n",
				tegra234_errmon_errors[err_type].errcode);
		errmon_err_status >>= 1;
		err_type++;
	}

	err_type = 0;
	while (errmon_overflow_status) {
		if (errmon_overflow_status & 0x1)
			print_cbb_err(file, "\t  Overflow\t\t: Multiple %s\n",
				tegra234_errmon_errors[err_type].errcode);
		errmon_overflow_status >>= 1;
		err_type++;
	}
}

static void print_errlog_err(struct seq_file *file,
		struct tegra_cbb_errmon_record *errmon)
{
	u8 cache_type = 0, prot_type = 0, burst_length = 0;
	u8 beat_size = 0, access_type = 0, access_id = 0;
	u8 mstr_id = 0, grpsec = 0, vqc = 0, falconsec = 0;
	u8 slave_id = 0, fabric_id = 0, burst_type = 0;

	cache_type = get_em_el_subfield(errmon->attr0, 27, 24);
	prot_type = get_em_el_subfield(errmon->attr0, 22, 20);
	burst_length = get_em_el_subfield(errmon->attr0, 19, 12);
	burst_type = get_em_el_subfield(errmon->attr0, 9, 8);
	beat_size = get_em_el_subfield(errmon->attr0, 6, 4);
	access_type = get_em_el_subfield(errmon->attr0, 0, 0);

	access_id = get_em_el_subfield(errmon->attr1, 7, 0);

	fabric_id = get_em_el_subfield(errmon->attr2, 20, 16);
	slave_id = get_em_el_subfield(errmon->attr2, 7, 0);

	mstr_id = get_em_el_subfield(errmon->user_bits, 29, 24);
	grpsec = get_em_el_subfield(errmon->user_bits, 17, 16);
	vqc = get_em_el_subfield(errmon->user_bits, 14, 8);
	falconsec = get_em_el_subfield(errmon->user_bits, 1, 0);

	print_cbb_err(file, "\t  First logged Err Code : %s\n",
			tegra234_errmon_errors[errmon->err_type].errcode);

	print_cbb_err(file, "\t  MASTER_ID\t\t: %s\n",
					errmon->tegra_cbb_master_id[mstr_id]);
	print_cbb_err(file, "\t  Address\t\t: 0x%llx\n",
					(u64)errmon->addr_access);

	print_cache(file, cache_type);
	print_prot(file, prot_type);

	print_cbb_err(file, "\t  Access_Type\t\t: %s",
			(access_type) ? "Write\n" : "Read");
	print_cbb_err(file, "\t  Fabric_Id\t\t: %d\n", fabric_id);
	print_cbb_err(file, "\t  Slave_Id\t\t: %d\n", slave_id);
	print_cbb_err(file, "\t  Burst_length\t\t: %d\n", burst_length);
	print_cbb_err(file, "\t  Burst_type\t\t: %d\n", burst_type);
	print_cbb_err(file, "\t  Beat_size\t\t: %d\n", beat_size);
	print_cbb_err(file, "\t  VQC\t\t\t: %d\n", vqc);
	print_cbb_err(file, "\t  GRPSEC\t\t: %d\n", grpsec);
	print_cbb_err(file, "\t  FALCONSEC\t\t: %d\n", falconsec);

}

static void print_errmonX_info(
		struct seq_file *file,
		struct tegra_cbb_errmon_record *errmon)
{
	unsigned int errmon_err_status = 0;
	unsigned int errlog_err_status = 0;
	unsigned int errmon_overflow_status = 0;
	u64 addr = 0;

	errmon->err_type = 0;

	errmon_err_status = readl(errmon->addr_errmon+
					FABRIC_MN_MASTER_ERR_STATUS_0);
	if (!errmon_err_status) {
		pr_err("Error Notifier received a spurious notification\n");
		BUG();
	}

	/*get overflow flag*/
	errmon_overflow_status = readl(errmon->addr_errmon+
					FABRIC_MN_MASTER_ERR_OVERFLOW_STATUS_0);

	print_errmon_err(file, errmon, errmon_err_status,
					errmon_overflow_status);

	errlog_err_status = readl(errmon->addr_errmon+
					FABRIC_MN_MASTER_LOG_ERR_STATUS_0);
	if (!errlog_err_status) {
		pr_info("Error Monitor doesn't have Error Logger\n");
		return;
	}

	while (errlog_err_status) {
		if (errlog_err_status & 0x1) {
			addr = readl(errmon->addr_errmon+
					FABRIC_MN_MASTER_LOG_ADDR_HIGH_0);
			addr = (addr<<32) | readl(errmon->addr_errmon+
					FABRIC_MN_MASTER_LOG_ADDR_LOW_0);
			errmon->addr_access = (void __iomem *)addr;

			errmon->attr0 = readl(errmon->addr_errmon+
					FABRIC_MN_MASTER_LOG_ATTRIBUTES0_0);
			errmon->attr1 = readl(errmon->addr_errmon+
					FABRIC_MN_MASTER_LOG_ATTRIBUTES1_0);
			errmon->attr2 = readl(errmon->addr_errmon+
					FABRIC_MN_MASTER_LOG_ATTRIBUTES2_0);
			errmon->user_bits = readl(errmon->addr_errmon+
					FABRIC_MN_MASTER_LOG_USER_BITS0_0);

			print_errlog_err(file, errmon);
		}
		errmon->err_type++;
		errlog_err_status >>= 1;
	}
}

static void print_err_notifier(struct seq_file *file,
		struct tegra_cbb_errmon_record *errmon,
		int err_notifier_status)
{
	u64 errmon_phys_addr = 0;
	u64 errmon_addr_offset = 0;
	int errmon_no = 1;

	pr_crit("**************************************\n");
	pr_crit("* For more Internal Decode Help\n");
	pr_crit("*     http://nv/cbberr\n");
	pr_crit("* NVIDIA userID is required to access\n");
	pr_crit("**************************************\n");
	pr_crit("CPU:%d, Error:%s, Errmon:%d\n", smp_processor_id(),
					errmon->name, err_notifier_status);
	while (err_notifier_status) {
		if (err_notifier_status & 0x1) {
			writel(errmon_no, errmon->vaddr+
					errmon->err_notifier_base+
					FABRIC_EN_CFG_ADDR_INDEX_0_0);

			errmon_phys_addr = readl(errmon->vaddr+
						errmon->err_notifier_base+
						FABRIC_EN_CFG_ADDR_HI_0);
			errmon_phys_addr = (errmon_phys_addr<<32) |
						readl(errmon->vaddr+
						errmon->err_notifier_base+
						FABRIC_EN_CFG_ADDR_LOW_0);

			errmon_addr_offset = errmon_phys_addr - errmon->start;
			errmon->addr_errmon = (void __iomem *)(errmon->vaddr+
							errmon_addr_offset);
			errmon->errmon_no = errmon_no;

			print_errmonX_info(file, errmon);
			tegra234_cbb_errmon_errclr(errmon->addr_errmon);
		}
		err_notifier_status >>= 1;
		errmon_no <<= 1;
	}

	print_cbb_err(file, "\t**************************************\n");
}

static int tegra234_cbb_serr_callback(struct pt_regs *regs, int reason,
		unsigned int esr, void *priv)
{
	unsigned int errvld_status = 0;
	struct tegra_cbb_errmon_record *errmon = priv;
	int retval = 1;

	if ((!errmon->is_clk_rst) ||
		(errmon->is_clk_rst && errmon->is_clk_enabled())) {
		errvld_status = tegra_cbb_errvld(errmon->vaddr+
						errmon->err_notifier_base);

		if (errvld_status) {
			print_err_notifier(NULL, errmon, errvld_status);
			retval = 0;
		}
	}
	return retval;
}

#ifdef CONFIG_DEBUG_FS
static DEFINE_MUTEX(cbb_err_mutex);

static int tegra234_cbb_err_show(struct seq_file *file, void *data)
{
	struct tegra_cbb_errmon_record *errmon;
	unsigned int errvld_status = 0;

	mutex_lock(&cbb_err_mutex);

	list_for_each_entry(errmon, &cbb_errmon_list, node) {
		if ((!errmon->is_clk_rst) ||
			(errmon->is_clk_rst && errmon->is_clk_enabled())) {

			errvld_status = tegra_cbb_errvld(errmon->vaddr+
						errmon->err_notifier_base);
			if (errvld_status) {
				print_err_notifier(file, errmon,
						errvld_status);
			}
		}
	}

	mutex_unlock(&cbb_err_mutex);
	return 0;
}
#endif

/*
 * Handler for CBB errors from masters other than CCPLEX
 */
static irqreturn_t tegra234_cbb_error_isr(int irq, void *dev_id)
{
	struct tegra_cbb_errmon_record *errmon;
	unsigned int errvld_status = 0;
	unsigned long flags;
	bool is_inband_err = 0;
	u8 mstr_id = 0;

	spin_lock_irqsave(&cbb_errmon_lock, flags);

	list_for_each_entry(errmon, &cbb_errmon_list, node) {
		if ((!errmon->is_clk_rst) ||
			(errmon->is_clk_rst && errmon->is_clk_enabled())) {
			errvld_status = tegra_cbb_errvld(errmon->vaddr+
						errmon->err_notifier_base);

			if (errvld_status &&
				((irq == errmon->errmon_secure_irq) ||
				(irq == errmon->errmon_nonsecure_irq))) {
				print_cbb_err(NULL, "CPU:%d, Error:%s@0x%llx,"
				"irq=%d\n", smp_processor_id(), errmon->name,
				errmon->start, irq);

				print_err_notifier(NULL, errmon, errvld_status);

				mstr_id = get_mstr_id(errmon->user_bits);
				/* If illegal request is from CCPLEX(id:0x1)
				 * master then call BUG() to crash system.
				 */
				if ((mstr_id == 0x1) &&
						(errmon->erd_mask_inband_err))
					is_inband_err = 1;
			}
		}
	}
	spin_unlock_irqrestore(&cbb_errmon_lock, flags);

	if (is_inband_err)
		BUG();

	return IRQ_HANDLED;
}

/*
 * Register handler for CBB_NONSECURE & CBB_SECURE interrupts due to
 * CBB errors from masters other than CCPLEX
 */
static int tegra234_cbb_enable_interrupt(struct platform_device *pdev,
				int errmon_secure_irq, int errmon_nonsecure_irq)
{
	int err = 0;

	if (errmon_secure_irq) {
		if (request_irq(errmon_secure_irq, tegra234_cbb_error_isr, 0,
					dev_name(&pdev->dev), pdev)) {
			dev_err(&pdev->dev,
				"%s: Unable to register (%d) interrupt\n",
				__func__, errmon_secure_irq);
			goto isr_err_free_sec_irq;
		}
	}
	if (errmon_nonsecure_irq) {
		if (request_irq(errmon_nonsecure_irq, tegra234_cbb_error_isr, 0,
					dev_name(&pdev->dev), pdev)) {
			dev_err(&pdev->dev,
				"%s: Unable to register (%d) interrupt\n",
				__func__, errmon_nonsecure_irq);
			goto isr_err_free_nonsec_irq;
		}
	}
	return 0;

isr_err_free_nonsec_irq:
	if (errmon_nonsecure_irq)
		free_irq(errmon_nonsecure_irq, pdev);
isr_err_free_sec_irq:
	if (errmon_secure_irq)
		free_irq(errmon_secure_irq, pdev);

	return err;
}


static void tegra234_cbb_error_enable(void __iomem *vaddr)
{
	tegra_cbb_faulten(vaddr);
}


static int tegra234_cbb_remove(struct platform_device *pdev)
{
	struct resource *res_base;
	struct tegra_cbb_errmon_record *errmon;
	unsigned long flags;

	res_base = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res_base)
		return -EINVAL;

	spin_lock_irqsave(&cbb_errmon_lock, flags);
	list_for_each_entry(errmon, &cbb_errmon_list, node) {
		if (errmon->start == res_base->start) {
			unregister_serr_hook(errmon->callback);
			list_del(&errmon->node);
			break;
		}
	}
	spin_unlock_irqrestore(&cbb_errmon_lock, flags);

	return 0;
}

static struct tegra_cbberr_ops tegra234_cbb_errmon_ops = {
	.errvld	 = tegra234_cbb_errmon_errvld,
	.errclr	 = tegra234_cbb_errmon_errclr,
	.faulten = tegra234_cbb_errmon_faulten,
	.cbb_error_enable = tegra234_cbb_error_enable,
	.cbb_enable_interrupt = tegra234_cbb_enable_interrupt,
#ifdef CONFIG_DEBUG_FS
	.cbb_err_debugfs_show = tegra234_cbb_err_show
#endif
};

static struct tegra_cbb_noc_data tegra234_aon_en_data = {
	.name   = "AON-EN",
	.is_ax2apb_bridge_connected = 0,
	.is_clk_rst = false,
	.erd_mask_inband_err = false
};

static struct tegra_cbb_noc_data tegra234_bpmp_en_data = {
	.name   = "BPMP-EN",
	.is_ax2apb_bridge_connected = 0,
	.is_clk_rst = false,
	.erd_mask_inband_err = false
};

static struct tegra_cbb_noc_data tegra234_cbb_en_data = {
	.name   = "CBB-EN",
	.is_ax2apb_bridge_connected = 0,
	.is_clk_rst = false,
	.erd_mask_inband_err = false,
	.off_erd_err_config = 0x120c
};

static struct tegra_cbb_noc_data tegra234_dce_en_data = {
	.name   = "DCE-EN",
	.is_ax2apb_bridge_connected = 0,
	.is_clk_rst = false,
	.erd_mask_inband_err = false
};

static struct tegra_cbb_noc_data tegra234_rce_en_data = {
	.name   = "RCE-EN",
	.is_ax2apb_bridge_connected = 0,
	.is_clk_rst = false,
	.erd_mask_inband_err = false
};

static struct tegra_cbb_noc_data tegra234_sce_en_data = {
	.name   = "SCE-EN",
	.is_ax2apb_bridge_connected = 0,
	.is_clk_rst = false,
	.erd_mask_inband_err = false
};

static const struct of_device_id tegra234_cbb_match[] = {
	{.compatible    = "nvidia,tegra234-CBB-EN",
		.data = &tegra234_cbb_en_data},
	{.compatible    = "nvidia,tegra234-AON-EN",
		.data = &tegra234_aon_en_data},
	{.compatible    = "nvidia,tegra234-BPMP-EN",
		.data = &tegra234_bpmp_en_data},
	{.compatible    = "nvidia,tegra234-DCE-EN",
		.data = &tegra234_dce_en_data},
	{.compatible    = "nvidia,tegra234-RCE-EN",
		.data = &tegra234_rce_en_data},
	{.compatible    = "nvidia,tegra234-SCE-EN",
		.data = &tegra234_sce_en_data},
	{},
};
MODULE_DEVICE_TABLE(of, tegra234_cbb_match);

static int tegra234_cbb_errmon_set_data(struct tegra_cbb_errmon_record *errmon)
{
	if (strlen(errmon->name) > 0)
		errmon->tegra_cbb_master_id = t234_master_id;
	else
		return -EINVAL;
	return 0;
}

static void tegra234_cbb_errmon_set_clk_en_ops(
		struct tegra_cbb_errmon_record *errmon,
		const struct tegra_cbb_noc_data *bdata)
{
	if (bdata->is_clk_rst) {
		errmon->is_clk_rst = bdata->is_clk_rst;
		errmon->is_cluster_probed = bdata->is_cluster_probed;
		errmon->is_clk_enabled = bdata->is_clk_enabled;
		errmon->tegra_errmon_en_clk_rpm = bdata->tegra_noc_en_clk_rpm;
		errmon->tegra_errmon_dis_clk_rpm = bdata->tegra_noc_dis_clk_rpm;
		errmon->tegra_errmon_en_clk_no_rpm =
			bdata->tegra_noc_en_clk_no_rpm;
		errmon->tegra_errmon_dis_clk_no_rpm =
			bdata->tegra_noc_dis_clk_no_rpm;
	}
}

static int tegra234_cbb_errmon_init(struct platform_device *pdev,
		struct serr_hook *callback,
		const struct tegra_cbb_noc_data *bdata,
		struct tegra_cbb_init_data *cbb_init_data)
{
	struct tegra_cbb_errmon_record *errmon;
	unsigned long flags;
	struct resource *res_base = cbb_init_data->res_base;
	struct device_node *np;
	int err = 0;

	errmon = devm_kzalloc(&pdev->dev, sizeof(*errmon), GFP_KERNEL);
	if (!errmon)
		return -ENOMEM;

	errmon->start = res_base->start;
	errmon->vaddr = devm_ioremap_resource(&pdev->dev, res_base);
	if (IS_ERR(errmon->vaddr))
		return -EPERM;

	errmon->name      = bdata->name;
	errmon->tegra_cbb_master_id = bdata->tegra_cbb_master_id;
	errmon->is_ax2apb_bridge_connected = bdata->is_ax2apb_bridge_connected;
	errmon->erd_mask_inband_err = bdata->erd_mask_inband_err;

	np = of_node_get(pdev->dev.of_node);
	err = of_property_read_u64(np, "err-notifier-base",
						&errmon->err_notifier_base);
	if (err) {
		dev_err(&pdev->dev, "Can't parse err-notifier-base\n");
		return -ENOENT;
	}

	tegra_cbberr_set_ops(&tegra234_cbb_errmon_ops);
	tegra234_cbb_errmon_set_clk_en_ops(errmon, bdata);
	err = tegra234_cbb_errmon_set_data(errmon);
	if (err) {
		dev_err(&pdev->dev, "Err logger name mismatch\n");
		return -EINVAL;
	}

	if (bdata->is_ax2apb_bridge_connected) {
		err = tegra_cbb_axi2apb_bridge_data(pdev,
				&(errmon->apb_bridge_cnt),
				&(errmon->axi2abp_bases));
		if (err) {
			dev_err(&pdev->dev, "axi2apb bridge read failed\n");
			return -EINVAL;
		}
	}

	err = tegra_cbb_err_getirq(pdev,
				&errmon->errmon_nonsecure_irq,
				&errmon->errmon_secure_irq, &errmon->num_intr);
	if (err)
		return -EINVAL;

	cbb_init_data->secure_irq = errmon->errmon_secure_irq;
	cbb_init_data->nonsecure_irq = errmon->errmon_nonsecure_irq;
	cbb_init_data->vaddr = errmon->vaddr+errmon->err_notifier_base;

	platform_set_drvdata(pdev, errmon);

	if (callback) {
		errmon->callback = callback;
		callback->fn = tegra234_cbb_serr_callback;
		callback->priv = errmon;
	}

	spin_lock_irqsave(&cbb_errmon_lock, flags);
	list_add(&errmon->node, &cbb_errmon_list);
	spin_unlock_irqrestore(&cbb_errmon_lock, flags);

	return err;
};

static int tegra234_cbb_probe(struct platform_device *pdev)
{
	const struct tegra_cbb_noc_data *bdata;
	struct serr_hook *callback;
	struct resource *res_base;
	struct tegra_cbb_init_data cbb_init_data;
	int err = 0;

	if ((tegra_get_chipid() != TEGRA_CHIPID_TEGRA23)
			|| !tegra_cbb_core_probed()) {
		dev_err(&pdev->dev,
		"Wrong SOC or tegra_cbb core driver not initialized\n");
		return -EINVAL;
	}

	bdata = of_device_get_match_data(&pdev->dev);
	if (!bdata) {
		dev_err(&pdev->dev, "No device match found\n");
		return -EINVAL;
	}

	if (bdata->is_clk_rst) {
		if (bdata->is_cluster_probed() && !bdata->is_clk_enabled()) {
			bdata->tegra_noc_en_clk_rpm();
		} else {
			dev_info(&pdev->dev, "defer probe as %s not probed yet",
					bdata->name);
			return -EPROBE_DEFER;
		}
	}

	res_base = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res_base) {
		dev_err(&pdev->dev, "Could not find base address");
		return -ENOENT;
	}

	memset(&cbb_init_data, 0, sizeof(cbb_init_data));
	cbb_init_data.res_base = res_base;

	callback = devm_kzalloc(&pdev->dev, sizeof(*callback), GFP_KERNEL);
	if (callback == NULL)
		return -ENOMEM;

	err = tegra234_cbb_errmon_init(pdev, callback, bdata, &cbb_init_data);
	if (err) {
		dev_err(&pdev->dev, "cbberr init for soc failing\n");
		return -EINVAL;
	}

	err = tegra_cbberr_register_hook_en(pdev, bdata, callback,
							cbb_init_data);
	if (err)
		return err;

	if ((bdata->is_clk_rst) && (bdata->is_cluster_probed())
			&& bdata->is_clk_enabled())
		bdata->tegra_noc_dis_clk_rpm();

	return err;
}

#ifdef CONFIG_PM_SLEEP
static int tegra234_cbb_resume_noirq(struct device *dev)
{
	struct tegra_cbb_errmon_record *errmon = dev_get_drvdata(dev);
	int ret = 0;

	if (errmon->is_clk_rst) {
		if (errmon->is_cluster_probed() && !errmon->is_clk_enabled())
			errmon->tegra_errmon_en_clk_no_rpm();
		else {
			dev_info(dev, "%s not resumed", errmon->name);
			return -EINVAL;
		}
	}

	tegra234_cbb_error_enable(errmon->vaddr+errmon->err_notifier_base);

	if ((errmon->is_clk_rst) && (errmon->is_cluster_probed())
			&& errmon->is_clk_enabled())
		errmon->tegra_errmon_dis_clk_no_rpm();

	dev_info(dev, "%s resumed\n", errmon->name);
	return ret;
}

static int tegra234_cbb_suspend_noirq(struct device *dev)
{
	return 0;
}

static const struct dev_pm_ops tegra234_cbb_pm = {
	SET_NOIRQ_SYSTEM_SLEEP_PM_OPS(tegra234_cbb_suspend_noirq,
			tegra234_cbb_resume_noirq)
};
#endif

static struct platform_driver tegra234_cbb_driver = {
	.probe          = tegra234_cbb_probe,
	.remove         = tegra234_cbb_remove,
	.driver = {
		.owner  = THIS_MODULE,
		.name   = "tegra23x-cbb",
		.of_match_table = of_match_ptr(tegra234_cbb_match),
#ifdef CONFIG_PM_SLEEP
		.pm     = &tegra234_cbb_pm,
#endif
	},
};

static int __init tegra234_cbb_init(void)
{
	return platform_driver_register(&tegra234_cbb_driver);
}

static void __exit tegra234_cbb_exit(void)
{
	platform_driver_unregister(&tegra234_cbb_driver);
}


pure_initcall(tegra234_cbb_init);
module_exit(tegra234_cbb_exit);
