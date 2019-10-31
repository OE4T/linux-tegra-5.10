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
			if (!(reg & sTLBGSTATUS_GSACTIVE))
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

static const struct arm_smmu_impl nvidia_smmu_impl = {
	.read_reg = nsmmu_read_reg,
	.write_reg = nsmmu_write_reg,
	.read_reg64 = nsmmu_read_reg64,
	.write_reg64 = nsmmu_write_reg64,
	.reset = nsmmu_reset,
	.tlb_sync = nsmmu_tlb_sync,
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

	return &nsmmu->smmu;
}
