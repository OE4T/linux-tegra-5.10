/*
 * PVA uCode Self Test
 *
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <linux/kernel.h>
#include <linux/nvhost.h>
#include <linux/delay.h>
#include <linux/iommu.h>
#include <linux/iova.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>

#include "dev.h"
#include "pva.h"

/* Defines for self test-mode in ucode */
#define PVA_MBOX_VAL_TESTS_DONE		0x57800000
#define PVA_SELF_TESTMODE_START_ADDR	0x90000000
#define PVA_SELF_TESTMODE_ADDR_SIZE	0x00800000

/**
 * Allocate a dma buffer and map it to a specified iova
 * Return valid cpu virtual address on success or NULL on failure
 */
static void *pva_dma_alloc_and_map_at(struct device *dev,
				      struct iova_domain *iovad, size_t size,
				      dma_addr_t iova, gfp_t flags,
				      unsigned long attrs)
{
	struct iommu_domain *domain = iommu_get_domain_for_dev(dev);
	unsigned long shift = __ffs(domain->pgsize_bitmap);
	unsigned long pg_size = 1UL << shift;
	unsigned long mp_size = pg_size;
	dma_addr_t tmp_iova, offset;
	phys_addr_t pa, pa_new;
	struct iova *iova_pfn;
	void *cpu_va;
	int ret;

	/* Reserve iova range */
	iova_pfn = alloc_iova(iovad, size >> shift,
			      (iova + size - pg_size) >> shift, false);
	if (!iova_pfn || (iova_pfn->pfn_lo << shift) != iova) {
		dev_err(dev, "failed to reserve iova at 0x%llx size 0x%lx\n",
			iova, size);
		return NULL;
	}

	/* Allocate a memory first and get a tmp_iova */
	cpu_va = dma_alloc_attrs(dev, size, &tmp_iova, flags, attrs);
	if (!cpu_va)
		goto fail_dma_alloc;

	/* Luckily hitting the target iova, no need to remap */
	if (tmp_iova == iova)
		return cpu_va;

	/* Use tmp_iova to remap non-contiguous pages to the desired iova */
	for (offset = 0; offset < size; offset += mp_size) {
		dma_addr_t cur_iova = tmp_iova + offset;

		mp_size = pg_size;
		pa = iommu_iova_to_phys(domain, cur_iova);
		/* Checking if next physical addresses are contiguous */
		for ( ; offset + mp_size < size; mp_size += pg_size) {
			pa_new = iommu_iova_to_phys(domain, cur_iova + mp_size);
			if (pa + mp_size != pa_new)
				break;
		}

		/* Remap the contiguous physical addresses together */
		ret = iommu_map(domain, iova + offset, pa, mp_size,
				IOMMU_READ | IOMMU_WRITE);
		if (ret) {
			dev_err(dev, "failed to map pa %llx va %llx size %lx\n",
				pa, iova + offset, mp_size);
			goto fail_map;
		}

		/* Verify if the new iova is correctly mapped */
		if (pa != iommu_iova_to_phys(domain, iova + offset)) {
			dev_err(dev, "mismatched pa 0x%llx <-> 0x%llx\n",
				pa, iommu_iova_to_phys(domain, iova + offset));
			goto fail_map;
		}
	}

	/* Unmap the tmp_iova since target iova is linked */
	iommu_unmap(domain, tmp_iova, size);

	return cpu_va;

fail_map:
	iommu_unmap(domain, iova, offset);
	dma_free_attrs(dev, size, cpu_va, tmp_iova, attrs);
fail_dma_alloc:
	__free_iova(iovad, iova_pfn);

	return NULL;
}

int pva_run_ucode_selftest(struct platform_device *pdev)
{
	struct nvhost_device_data *pdata = platform_get_drvdata(pdev);
	struct pva *pva = pdata->private_data;
	struct iova_domain *iovad;
	int err = 0;
	u32 reg_status;
	u32 ucode_mode;
	void *selftest_cpuaddr;
	dma_addr_t base_iova = PVA_SELF_TESTMODE_START_ADDR;
	size_t base_size = PVA_SELF_TESTMODE_ADDR_SIZE;

	/* Map static memory for self test mode */
	nvhost_dbg_info("uCode TESTMODE Enabled");

	iovad = kzalloc(sizeof(*iovad), GFP_KERNEL);
	if (!iovad)
		return -ENOMEM;

	init_iova_domain(iovad, PAGE_SIZE, base_iova >> PAGE_SHIFT,
			 (base_iova + base_size) >> PAGE_SHIFT);
	selftest_cpuaddr = pva_dma_alloc_and_map_at(&pdev->dev, iovad,
			base_size, base_iova, GFP_KERNEL | __GFP_ZERO,
			DMA_ATTR_SKIP_CPU_SYNC | DMA_ATTR_SKIP_IOVA_GAP);

	if (!selftest_cpuaddr) {
		dev_warn(&pdev->dev, "Failed to get Selftest Static memory\n");
		err = -ENOMEM;
		goto err_selftest;
	}

	pva->mailbox_status = PVA_MBOX_STATUS_WFI;
	host1x_writel(pdev, hsp_ss0_set_r(), PVA_TEST_RUN);

	/* Wait till we get a AISR_ABORT interrupt */
	err = pva_mailbox_wait_event(pva, 60000);
	if (err)
		goto wait_timeout;

	ucode_mode = host1x_readl(pdev, hsp_ss0_state_r());

	/*Check whether ucode halted */
	if ((ucode_mode & PVA_HALTED) == 0) {
		nvhost_dbg_info("uCode SELFTEST Failed to Halt");
		err = -EINVAL;
		goto err_selftest;
	}

	reg_status = pva_read_mailbox(pdev, PVA_MBOX_ISR);

	/* check test passed bit set and test status done*/
	if ((ucode_mode & PVA_TESTS_PASSED) &&
		(reg_status == PVA_MBOX_VAL_TESTS_DONE))
		nvhost_dbg_info("uCode SELFTEST Passed");
	else if (ucode_mode & PVA_TESTS_FAILED)
		nvhost_dbg_info("uCode SELFTEST Failed");
	else
		nvhost_dbg_info("uCode SELFTEST UnKnown State");

	/* Get CCQ8 register value */
	reg_status = host1x_readl(pdev, cfg_ccq_status8_r());
	nvhost_dbg_info("Major 0x%x, Minor 0x%x, Flags 0x%x, Trace Sequence 0x%x \n",
			(reg_status & 0xFF000000) >> 24,
			(reg_status & 0x00FF0000) >> 16,
			(reg_status & 0xFF00) >> 8,
			(reg_status & 0xFF));

wait_timeout:
err_selftest:
	if (selftest_cpuaddr) {
		dma_free_attrs(&pdev->dev,
			PVA_SELF_TESTMODE_ADDR_SIZE, selftest_cpuaddr,
			PVA_SELF_TESTMODE_START_ADDR,
			DMA_ATTR_SKIP_CPU_SYNC | DMA_ATTR_SKIP_IOVA_GAP);
		free_iova(iovad, iova_pfn(iovad, base_iova));
	}
	put_iova_domain(iovad);
	kfree(iovad);

	return err;

}
