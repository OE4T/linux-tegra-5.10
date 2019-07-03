/*
 * Copyright (c) 2016-2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <nvgpu/pmu.h>
#include <nvgpu/falcon.h>
#include <nvgpu/io.h>
#include <nvgpu/utils.h>
#include <nvgpu/mm.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/nvgpu_err.h>
#include <nvgpu/firmware.h>
#include <nvgpu/bug.h>
#ifdef CONFIG_NVGPU_LS_PMU
#include <nvgpu/pmu/cmd.h>
#endif

#include "pmu_gv11b.h"

#include <nvgpu/hw/gv11b/hw_pwr_gv11b.h>

#define ALIGN_4KB     12

int gv11b_pmu_inject_ecc_error(struct gk20a *g,
		struct nvgpu_hw_err_inject_info *err, u32 error_info)
{
	nvgpu_info(g, "Injecting PMU fault %s", err->name);
	nvgpu_writel(g, err->get_reg_addr(), err->get_reg_val(1U));

	return 0;
}

#ifdef CONFIG_NVGPU_LS_PMU
/* PROD settings for ELPG sequencing registers*/
static struct pg_init_sequence_list _pginitseq_gv11b[] = {
	{0x0010e0a8U, 0x00000000U} ,
	{0x0010e0acU, 0x00000000U} ,
	{0x0010e198U, 0x00000200U} ,
	{0x0010e19cU, 0x00000000U} ,
	{0x0010e19cU, 0x00000000U} ,
	{0x0010e19cU, 0x00000000U} ,
	{0x0010e19cU, 0x00000000U} ,
	{0x0010aba8U, 0x00000200U} ,
	{0x0010abacU, 0x00000000U} ,
	{0x0010abacU, 0x00000000U} ,
	{0x0010abacU, 0x00000000U} ,
	{0x0010e09cU, 0x00000731U} ,
	{0x0010e18cU, 0x00000731U} ,
	{0x0010ab9cU, 0x00000731U} ,
	{0x0010e0a0U, 0x00000200U} ,
	{0x0010e0a4U, 0x00000004U} ,
	{0x0010e0a4U, 0x80000000U} ,
	{0x0010e0a4U, 0x80000009U} ,
	{0x0010e0a4U, 0x8000001AU} ,
	{0x0010e0a4U, 0x8000001EU} ,
	{0x0010e0a4U, 0x8000002AU} ,
	{0x0010e0a4U, 0x8000002EU} ,
	{0x0010e0a4U, 0x80000016U} ,
	{0x0010e0a4U, 0x80000022U} ,
	{0x0010e0a4U, 0x80000026U} ,
	{0x0010e0a4U, 0x00000005U} ,
	{0x0010e0a4U, 0x80000001U} ,
	{0x0010e0a4U, 0x8000000AU} ,
	{0x0010e0a4U, 0x8000001BU} ,
	{0x0010e0a4U, 0x8000001FU} ,
	{0x0010e0a4U, 0x8000002BU} ,
	{0x0010e0a4U, 0x8000002FU} ,
	{0x0010e0a4U, 0x80000017U} ,
	{0x0010e0a4U, 0x80000023U} ,
	{0x0010e0a4U, 0x80000027U} ,
	{0x0010e0a4U, 0x00000006U} ,
	{0x0010e0a4U, 0x80000002U} ,
	{0x0010e0a4U, 0x8000000BU} ,
	{0x0010e0a4U, 0x8000001CU} ,
	{0x0010e0a4U, 0x80000020U} ,
	{0x0010e0a4U, 0x8000002CU} ,
	{0x0010e0a4U, 0x80000030U} ,
	{0x0010e0a4U, 0x80000018U} ,
	{0x0010e0a4U, 0x80000024U} ,
	{0x0010e0a4U, 0x80000028U} ,
	{0x0010e0a4U, 0x00000007U} ,
	{0x0010e0a4U, 0x80000003U} ,
	{0x0010e0a4U, 0x8000000CU} ,
	{0x0010e0a4U, 0x8000001DU} ,
	{0x0010e0a4U, 0x80000021U} ,
	{0x0010e0a4U, 0x8000002DU} ,
	{0x0010e0a4U, 0x80000031U} ,
	{0x0010e0a4U, 0x80000019U} ,
	{0x0010e0a4U, 0x80000025U} ,
	{0x0010e0a4U, 0x80000029U} ,
	{0x0010e0a4U, 0x80000012U} ,
	{0x0010e0a4U, 0x80000010U} ,
	{0x0010e0a4U, 0x00000013U} ,
	{0x0010e0a4U, 0x80000011U} ,
	{0x0010e0a4U, 0x80000008U} ,
	{0x0010e0a4U, 0x8000000DU} ,
	{0x0010e190U, 0x00000200U} ,
	{0x0010e194U, 0x80000015U} ,
	{0x0010e194U, 0x80000014U} ,
	{0x0010aba0U, 0x00000200U} ,
	{0x0010aba4U, 0x8000000EU} ,
	{0x0010aba4U, 0x0000000FU} ,
	{0x0010ab34U, 0x00000001U} ,
	{0x00020004U, 0x00000000U} ,
};

void gv11b_pmu_setup_elpg(struct gk20a *g)
{
	size_t reg_writes;
	size_t index;

	nvgpu_log_fn(g, " ");

	if (g->can_elpg && g->elpg_enabled) {
		reg_writes = ARRAY_SIZE(_pginitseq_gv11b);
		/* Initialize registers with production values*/
		for (index = 0; index < reg_writes; index++) {
			nvgpu_writel(g, _pginitseq_gv11b[index].regaddr,
				_pginitseq_gv11b[index].writeval);
		}
	}

	nvgpu_log_fn(g, "done");
}

int gv11b_pmu_bootstrap(struct gk20a *g, struct nvgpu_pmu *pmu,
	u32 args_offset)
{
	struct mm_gk20a *mm = &g->mm;
	struct nvgpu_firmware *fw = NULL;
	struct pmu_ucode_desc *desc = NULL;
	u32 addr_code_lo, addr_data_lo, addr_load_lo;
	u32 addr_code_hi, addr_data_hi;
	u32 i, blocks;
	int err;
	u32 inst_block_ptr;

	nvgpu_log_fn(g, " ");

	fw = nvgpu_pmu_fw_image_desc(g, pmu);
	desc = (struct pmu_ucode_desc *)(void *)fw->data;

	nvgpu_writel(g, pwr_falcon_itfen_r(),
		nvgpu_readl(g, pwr_falcon_itfen_r()) |
		pwr_falcon_itfen_ctxen_enable_f());

	inst_block_ptr = nvgpu_inst_block_ptr(g, &mm->pmu.inst_block);
	nvgpu_writel(g, pwr_pmu_new_instblk_r(),
		pwr_pmu_new_instblk_ptr_f(inst_block_ptr) |
		pwr_pmu_new_instblk_valid_f(1) |
		(nvgpu_is_enabled(g, NVGPU_USE_COHERENT_SYSMEM) ?
		pwr_pmu_new_instblk_target_sys_coh_f() :
		pwr_pmu_new_instblk_target_sys_ncoh_f()));

	nvgpu_writel(g, pwr_falcon_dmemc_r(0),
		pwr_falcon_dmemc_offs_f(0) |
		pwr_falcon_dmemc_blk_f(0)  |
		pwr_falcon_dmemc_aincw_f(1));

	addr_code_lo = u64_lo32((pmu->fw->ucode.gpu_va +
			desc->app_start_offset +
			desc->app_resident_code_offset) >> 8);

	addr_code_hi = u64_hi32((pmu->fw->ucode.gpu_va +
			desc->app_start_offset +
			desc->app_resident_code_offset) >> 8);
	addr_data_lo = u64_lo32((pmu->fw->ucode.gpu_va +
			desc->app_start_offset +
			desc->app_resident_data_offset) >> 8);
	addr_data_hi = u64_hi32((pmu->fw->ucode.gpu_va +
			desc->app_start_offset +
			desc->app_resident_data_offset) >> 8);
	addr_load_lo = u64_lo32((pmu->fw->ucode.gpu_va +
			desc->bootloader_start_offset) >> 8);

	nvgpu_writel(g, pwr_falcon_dmemd_r(0), 0x0U);
	nvgpu_writel(g, pwr_falcon_dmemd_r(0), 0x0U);
	nvgpu_writel(g, pwr_falcon_dmemd_r(0), 0x0U);
	nvgpu_writel(g, pwr_falcon_dmemd_r(0), 0x0U);
	nvgpu_writel(g, pwr_falcon_dmemd_r(0), 0x0U);
	nvgpu_writel(g, pwr_falcon_dmemd_r(0), 0x0U);
	nvgpu_writel(g, pwr_falcon_dmemd_r(0), 0x0U);
	nvgpu_writel(g, pwr_falcon_dmemd_r(0), 0x0U);
	nvgpu_writel(g, pwr_falcon_dmemd_r(0), GK20A_PMU_DMAIDX_UCODE);
	nvgpu_writel(g, pwr_falcon_dmemd_r(0), addr_code_lo << 8);
	nvgpu_writel(g, pwr_falcon_dmemd_r(0), addr_code_hi);
	nvgpu_writel(g, pwr_falcon_dmemd_r(0), desc->app_resident_code_offset);
	nvgpu_writel(g, pwr_falcon_dmemd_r(0), desc->app_resident_code_size);
	nvgpu_writel(g, pwr_falcon_dmemd_r(0), 0x0U);
	nvgpu_writel(g, pwr_falcon_dmemd_r(0), 0x0U);
	nvgpu_writel(g, pwr_falcon_dmemd_r(0), desc->app_imem_entry);
	nvgpu_writel(g, pwr_falcon_dmemd_r(0), addr_data_lo << 8);
	nvgpu_writel(g, pwr_falcon_dmemd_r(0), addr_data_hi);
	nvgpu_writel(g, pwr_falcon_dmemd_r(0), desc->app_resident_data_size);
	nvgpu_writel(g, pwr_falcon_dmemd_r(0), 0x1U);
	nvgpu_writel(g, pwr_falcon_dmemd_r(0), args_offset);

	g->ops.pmu.write_dmatrfbase(g,
				addr_load_lo -
				(desc->bootloader_imem_offset >> U32(8)));

	blocks = ((desc->bootloader_size + 0xFFU) & ~0xFFU) >> 8U;

	for (i = 0; i < blocks; i++) {
		nvgpu_writel(g, pwr_falcon_dmatrfmoffs_r(),
			desc->bootloader_imem_offset + (i << 8));
		nvgpu_writel(g, pwr_falcon_dmatrffboffs_r(),
			desc->bootloader_imem_offset + (i << 8));
		nvgpu_writel(g, pwr_falcon_dmatrfcmd_r(),
			pwr_falcon_dmatrfcmd_imem_f(1)  |
			pwr_falcon_dmatrfcmd_write_f(0) |
			pwr_falcon_dmatrfcmd_size_f(6)  |
			pwr_falcon_dmatrfcmd_ctxdma_f(GK20A_PMU_DMAIDX_UCODE));
	}

	err = nvgpu_falcon_bootstrap(pmu->flcn, desc->bootloader_entry_point);

	nvgpu_writel(g, pwr_falcon_os_r(), desc->app_version);

	return err;
}

static void gv11b_pmu_handle_ecc_irq(struct gk20a *g)
{
	u32 intr1;
	u32 ecc_status, ecc_addr, corrected_cnt, uncorrected_cnt;
	u32 corrected_delta, uncorrected_delta;
	u32 corrected_overflow, uncorrected_overflow;

	intr1 = nvgpu_readl(g, pwr_pmu_ecc_intr_status_r());
	if ((intr1 &
		(pwr_pmu_ecc_intr_status_corrected_m() |
		 pwr_pmu_ecc_intr_status_uncorrected_m())) == 0U) {
		return;
	}

	ecc_status = nvgpu_readl(g,
		pwr_pmu_falcon_ecc_status_r());
	ecc_addr = nvgpu_readl(g,
		pwr_pmu_falcon_ecc_address_r());
	corrected_cnt = nvgpu_readl(g,
		pwr_pmu_falcon_ecc_corrected_err_count_r());
	uncorrected_cnt = nvgpu_readl(g,
		pwr_pmu_falcon_ecc_uncorrected_err_count_r());

	corrected_delta =
	  pwr_pmu_falcon_ecc_corrected_err_count_total_v(corrected_cnt);
	uncorrected_delta =
	  pwr_pmu_falcon_ecc_uncorrected_err_count_total_v(uncorrected_cnt);
	corrected_overflow = ecc_status &
	  pwr_pmu_falcon_ecc_status_corrected_err_total_counter_overflow_m();

	uncorrected_overflow = ecc_status &
	  pwr_pmu_falcon_ecc_status_uncorrected_err_total_counter_overflow_m();
	corrected_overflow = ecc_status &
	  pwr_pmu_falcon_ecc_status_corrected_err_total_counter_overflow_m();

	/* clear the interrupt */
	if (((intr1 & pwr_pmu_ecc_intr_status_corrected_m()) != 0U) ||
					(corrected_overflow != 0U)) {
		nvgpu_writel(g, pwr_pmu_falcon_ecc_corrected_err_count_r(), 0);
	}
	if (((intr1 & pwr_pmu_ecc_intr_status_uncorrected_m()) != 0U) ||
					(uncorrected_overflow != 0U)) {
		nvgpu_writel(g,
			pwr_pmu_falcon_ecc_uncorrected_err_count_r(), 0);
	}

	nvgpu_writel(g, pwr_pmu_falcon_ecc_status_r(),
		pwr_pmu_falcon_ecc_status_reset_task_f());

	/* update counters per slice */
	if (corrected_overflow != 0U) {
		corrected_delta +=
			BIT32(pwr_pmu_falcon_ecc_corrected_err_count_total_s());
	}
	if (uncorrected_overflow != 0U) {
		uncorrected_delta +=
		  BIT32(pwr_pmu_falcon_ecc_uncorrected_err_count_total_s());
	}

	g->ecc.pmu.pmu_ecc_corrected_err_count[0].counter += corrected_delta;
	g->ecc.pmu.pmu_ecc_uncorrected_err_count[0].counter +=
							uncorrected_delta;

	nvgpu_log(g, gpu_dbg_intr,
		"pmu ecc interrupt intr1: 0x%x", intr1);

	(void)gv11b_pmu_correct_ecc(g, ecc_status, ecc_addr);

	if ((corrected_overflow != 0U) || (uncorrected_overflow != 0U)) {
		nvgpu_info(g, "ecc counter overflow!");
	}

	nvgpu_log(g, gpu_dbg_intr,
		"ecc error row address: 0x%x",
		pwr_pmu_falcon_ecc_address_row_address_v(ecc_addr));

	nvgpu_log(g, gpu_dbg_intr,
		"ecc error count corrected: %d, uncorrected %d",
		g->ecc.pmu.pmu_ecc_corrected_err_count[0].counter,
		g->ecc.pmu.pmu_ecc_uncorrected_err_count[0].counter);
}

void gv11b_pmu_handle_ext_irq(struct gk20a *g, u32 intr0)
{
	/*
	 * handle the ECC interrupt
	 */
	if ((intr0 & pwr_falcon_irqstat_ext_ecc_parity_true_f()) != 0U) {
		gv11b_pmu_handle_ecc_irq(g);
	}
}

u32 gv11b_pmu_get_irqdest(struct gk20a *g)
{
	u32 intr_dest;

	/* dest 0=falcon, 1=host; level 0=irq0, 1=irq1 */
	intr_dest = pwr_falcon_irqdest_host_gptmr_f(0)      |
		pwr_falcon_irqdest_host_wdtmr_f(1)          |
		pwr_falcon_irqdest_host_mthd_f(0)           |
		pwr_falcon_irqdest_host_ctxsw_f(0)          |
		pwr_falcon_irqdest_host_halt_f(1)           |
		pwr_falcon_irqdest_host_exterr_f(0)         |
		pwr_falcon_irqdest_host_swgen0_f(1)         |
		pwr_falcon_irqdest_host_swgen1_f(0)         |
		pwr_falcon_irqdest_host_ext_ecc_parity_f(1) |
		pwr_falcon_irqdest_target_gptmr_f(1)        |
		pwr_falcon_irqdest_target_wdtmr_f(0)        |
		pwr_falcon_irqdest_target_mthd_f(0)         |
		pwr_falcon_irqdest_target_ctxsw_f(0)        |
		pwr_falcon_irqdest_target_halt_f(0)         |
		pwr_falcon_irqdest_target_exterr_f(0)       |
		pwr_falcon_irqdest_target_swgen0_f(0)       |
		pwr_falcon_irqdest_target_swgen1_f(0)       |
		pwr_falcon_irqdest_target_ext_ecc_parity_f(0);

	return intr_dest;
}
#endif

/* error handler */
