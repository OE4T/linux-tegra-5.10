// SPDX-License-Identifier: GPL-2.0-only
// Nvidia ARM SMMU v2 implementation quirks
// Copyright (C) 2019 NVIDIA CORPORATION.  All rights reserved.

#define pr_fmt(fmt) "nvidia-smmu: " fmt

#include <linux/bitfield.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include "arm-smmu.h"

#ifdef CONFIG_ARM_SMMU_DEBUG
#include <linux/arm-smmu-debug.h>
#endif

/* Tegra194 has three ARM MMU-500 Instances.
 * Two of them are used together for Interleaved IOVA accesses and
 * used by Non-Isochronous Hw devices for SMMU translations.
 * Third one is used for SMMU translations from Isochronous HW devices.
 * It is possible to use this Implementation to program either
 * all three or two of the instances identically as desired through
 * DT node.
 *
 * Programming all the three instances identically comes with redundant tlb
 * invalidations as all three never need to be tlb invalidated for a HW device.
 *
 * When Linux Kernel supports multiple SMMU devices, The SMMU device used for
 * Isochornous HW devices should be added as a separate ARM MMU-500 device
 * in DT and be programmed independently for efficient tlb invalidates.
 *
 */
#define MAX_SMMU_INSTANCES 3

#define TLB_LOOP_TIMEOUT		1000000	/* 1s! */
#define TLB_SPIN_COUNT			10

struct nvidia_smmu {
	struct arm_smmu_device	smmu;
	unsigned int		num_inst;
	void __iomem		*bases[MAX_SMMU_INSTANCES];
};

#define to_nvidia_smmu(s) container_of(s, struct nvidia_smmu, smmu)

#define nsmmu_page(smmu, inst, page) \
	(((inst) ? to_nvidia_smmu(smmu)->bases[(inst)] : smmu->base) + \
	((page) << smmu->pgshift))

static u32 nsmmu_read_reg(struct arm_smmu_device *smmu,
			      int page, int offset)
{
	return readl_relaxed(nsmmu_page(smmu, 0, page) + offset);
}

static void nsmmu_write_reg(struct arm_smmu_device *smmu,
			    int page, int offset, u32 val)
{
	unsigned int i;

	for (i = 0; i < to_nvidia_smmu(smmu)->num_inst; i++)
		writel_relaxed(val, nsmmu_page(smmu, i, page) + offset);
}

static u64 nsmmu_read_reg64(struct arm_smmu_device *smmu,
				int page, int offset)
{
	return readq_relaxed(nsmmu_page(smmu, 0, page) + offset);
}

static void nsmmu_write_reg64(struct arm_smmu_device *smmu,
				  int page, int offset, u64 val)
{
	unsigned int i;

	for (i = 0; i < to_nvidia_smmu(smmu)->num_inst; i++)
		writeq_relaxed(val, nsmmu_page(smmu, i, page) + offset);
}

static void nsmmu_tlb_sync(struct arm_smmu_device *smmu, int page,
			   int sync, int status)
{
	u32 reg;
	unsigned int i;
	unsigned int spin_cnt, delay;

	arm_smmu_writel(smmu, page, sync, 0);

	for (delay = 1; delay < TLB_LOOP_TIMEOUT; delay *= 2) {
		for (spin_cnt = TLB_SPIN_COUNT; spin_cnt > 0; spin_cnt--) {
			reg = 0;
			for (i = 0; i < to_nvidia_smmu(smmu)->num_inst; i++) {
				reg |= readl_relaxed(
					nsmmu_page(smmu, i, page) + status);
			}
			if (!(reg & ARM_SMMU_sTLBGSTATUS_GSACTIVE))
				return;
			cpu_relax();
		}
		udelay(delay);
	}
	dev_err_ratelimited(smmu->dev,
			    "TLB sync timed out -- SMMU may be deadlocked\n");
}

static int nsmmu_reset(struct arm_smmu_device *smmu)
{
	u32 reg;
	unsigned int i;

	for (i = 0; i < to_nvidia_smmu(smmu)->num_inst; i++) {
		/* clear global FSR */
		reg = readl_relaxed(nsmmu_page(smmu, i, ARM_SMMU_GR0) +
				    ARM_SMMU_GR0_sGFSR);
		writel_relaxed(reg, nsmmu_page(smmu, i, ARM_SMMU_GR0) +
				    ARM_SMMU_GR0_sGFSR);
	}

	return 0;
}

static struct arm_smmu_domain *to_smmu_domain(struct iommu_domain *dom)
{
	return container_of(dom, struct arm_smmu_domain, domain);
}

static irqreturn_t nsmmu_global_fault_inst(int irq,
					       struct arm_smmu_device *smmu,
					       int inst)
{
	u32 gfsr, gfsynr0, gfsynr1, gfsynr2;

	gfsr = readl_relaxed(nsmmu_page(smmu, inst, 0) + ARM_SMMU_GR0_sGFSR);
	gfsynr0 = readl_relaxed(nsmmu_page(smmu, inst, 0) +
				ARM_SMMU_GR0_sGFSYNR0);
	gfsynr1 = readl_relaxed(nsmmu_page(smmu, inst, 0) +
				ARM_SMMU_GR0_sGFSYNR1);
	gfsynr2 = readl_relaxed(nsmmu_page(smmu, inst, 0) +
				ARM_SMMU_GR0_sGFSYNR2);

	if (!gfsr)
		return IRQ_NONE;

	dev_err_ratelimited(smmu->dev,
		"Unexpected global fault, this could be serious\n");
	dev_err_ratelimited(smmu->dev,
		"\tGFSR 0x%08x, GFSYNR0 0x%08x, GFSYNR1 0x%08x, GFSYNR2 0x%08x\n",
		gfsr, gfsynr0, gfsynr1, gfsynr2);

	writel_relaxed(gfsr, nsmmu_page(smmu, inst, 0) + ARM_SMMU_GR0_sGFSR);
	return IRQ_HANDLED;
}

static irqreturn_t nsmmu_global_fault(int irq, void *dev)
{
	int inst;
	irqreturn_t irq_ret = IRQ_NONE;
	struct arm_smmu_device *smmu = dev;

	for (inst = 0; inst < to_nvidia_smmu(smmu)->num_inst; inst++) {
		irq_ret = nsmmu_global_fault_inst(irq, smmu, inst);
		if (irq_ret == IRQ_HANDLED)
			return irq_ret;
	}

	return irq_ret;
}

static irqreturn_t nsmmu_context_fault_bank(int irq,
					    struct arm_smmu_device *smmu,
					    int idx, int inst)
{
	u32 fsr, fsynr, cbfrsynra;
	unsigned long iova;

	fsr = arm_smmu_cb_read(smmu, idx, ARM_SMMU_CB_FSR);
	if (!(fsr & ARM_SMMU_FSR_FAULT))
		return IRQ_NONE;

	fsynr = readl_relaxed(nsmmu_page(smmu, inst, smmu->numpage + idx) +
			      ARM_SMMU_CB_FSYNR0);
	iova = readq_relaxed(nsmmu_page(smmu, inst, smmu->numpage + idx) +
			     ARM_SMMU_CB_FAR);
	cbfrsynra = readl_relaxed(nsmmu_page(smmu, inst, 1) +
				  ARM_SMMU_GR1_CBFRSYNRA(idx));

	dev_err_ratelimited(smmu->dev,
	"Unhandled context fault: fsr=0x%x, iova=0x%08lx, fsynr=0x%x, cbfrsynra=0x%x, cb=%d\n",
			    fsr, iova, fsynr, cbfrsynra, idx);

	writel_relaxed(fsr, nsmmu_page(smmu, inst, smmu->numpage + idx) +
			    ARM_SMMU_CB_FSR);
	return IRQ_HANDLED;
}

static irqreturn_t nsmmu_context_fault(int irq, void *dev)
{
	int inst, idx;
	irqreturn_t irq_ret = IRQ_NONE;
	struct iommu_domain *domain = dev;
	struct arm_smmu_domain *smmu_domain = to_smmu_domain(domain);
	struct arm_smmu_device *smmu = smmu_domain->smmu;

	for (inst = 0; inst < to_nvidia_smmu(smmu)->num_inst; inst++) {
		/* Interrupt line shared between all context faults.
		 * Check for faults across all contexts.
		 */
		for (idx = 0; idx < smmu->num_context_banks; idx++) {
			irq_ret = nsmmu_context_fault_bank(irq, smmu,
							   idx, inst);

			if (irq_ret == IRQ_HANDLED)
				return irq_ret;
		}
	}

	return irq_ret;
}

static const struct arm_smmu_impl nvidia_smmu_impl = {
	.read_reg = nsmmu_read_reg,
	.write_reg = nsmmu_write_reg,
	.read_reg64 = nsmmu_read_reg64,
	.write_reg64 = nsmmu_write_reg64,
	.reset = nsmmu_reset,
	.tlb_sync = nsmmu_tlb_sync,
	.global_fault = nsmmu_global_fault,
	.context_fault = nsmmu_context_fault,
};

struct arm_smmu_device *nvidia_smmu_impl_init(struct arm_smmu_device *smmu)
{
	unsigned int i;
	struct nvidia_smmu *nsmmu;
	struct resource *res;
	struct device *dev = smmu->dev;
	struct platform_device *pdev = to_platform_device(smmu->dev);

	nsmmu = devm_kzalloc(smmu->dev, sizeof(*nsmmu), GFP_KERNEL);
	if (!nsmmu)
		return ERR_PTR(-ENOMEM);

	nsmmu->smmu = *smmu;
	/* Instance 0 is ioremapped by arm-smmu.c */
	nsmmu->num_inst = 1;

	for (i = 1; i < MAX_SMMU_INSTANCES; i++) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!res)
			break;
		nsmmu->bases[i] = devm_ioremap_resource(dev, res);
		if (IS_ERR(nsmmu->bases[i]))
			return (struct arm_smmu_device *)nsmmu->bases[i];
		nsmmu->num_inst++;
	}

	nsmmu->smmu.impl = &nvidia_smmu_impl;
	devm_kfree(smmu->dev, smmu);
	pr_info("NVIDIA ARM SMMU Implementation, Instances=%d\n",
		nsmmu->num_inst);

#ifdef CONFIG_ARM_SMMU_DEBUG
	arm_smmu_debugfs_setup_bases(&nsmmu->smmu, nsmmu->num_inst, nsmmu->bases);
#endif

	return &nsmmu->smmu;
}
