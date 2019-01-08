/*
 * GV11B PMU
 *
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
#include <nvgpu/enabled.h>
#include <nvgpu/mm.h>
#include <nvgpu/io.h>
#include <nvgpu/utils.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/nvgpu_err.h>

#include "pmu_gp10b.h"
#include "pmu_gp106.h"
#include "pmu_gv11b.h"
#include "acr_gv11b.h"

#include <nvgpu/hw/gv11b/hw_pwr_gv11b.h>

#define gv11b_dbg_pmu(g, fmt, arg...) \
	nvgpu_log(g, gpu_dbg_pmu, fmt, ##arg)

#define ALIGN_4KB     12

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

static void gv11b_pmu_report_ecc_error(struct gk20a *g, u32 inst,
		u32 err_type, u64 err_addr, u64 err_cnt)
{
	int ret = 0;

	if (g->ops.pmu.err_ops.report_ecc_parity_err == NULL) {
		return ;
	}
	ret = g->ops.pmu.err_ops.report_ecc_parity_err(g,
			NVGPU_ERR_MODULE_PWR, inst, err_type, err_addr,
			err_cnt);
	if (ret != 0) {
		nvgpu_err(g, "Failed to report PMU error: inst=%u, \
				err_type=%u, err_addr=%llu, err_cnt=%llu",
				inst, err_type, err_addr, err_cnt);
	}
}

int gv11b_pmu_setup_elpg(struct gk20a *g)
{
	int ret = 0;
	u32 reg_writes;
	u32 index;

	nvgpu_log_fn(g, " ");

	if (g->elpg_enabled) {
		reg_writes = ((sizeof(_pginitseq_gv11b) /
				sizeof((_pginitseq_gv11b)[0])));
		/* Initialize registers with production values*/
		for (index = 0; index < reg_writes; index++) {
			gk20a_writel(g, _pginitseq_gv11b[index].regaddr,
				_pginitseq_gv11b[index].writeval);
		}
	}

	nvgpu_log_fn(g, "done");
	return ret;
}

bool gv11b_is_pmu_supported(struct gk20a *g)
{
	return true;
}

bool gv11b_is_lazy_bootstrap(u32 falcon_id)
{
	bool enable_status = false;

	switch (falcon_id) {
	case FALCON_ID_FECS:
		enable_status = true;
		break;
	case FALCON_ID_GPCCS:
		enable_status = true;
		break;
	default:
		break;
	}

	return enable_status;
}

bool gv11b_is_priv_load(u32 falcon_id)
{
	bool enable_status = false;

	switch (falcon_id) {
	case FALCON_ID_FECS:
		enable_status = false;
		break;
	case FALCON_ID_GPCCS:
		enable_status = true;
		break;
	default:
		break;
	}

	return enable_status;
}

int gv11b_pmu_bootstrap(struct nvgpu_pmu *pmu)
{
	struct gk20a *g = gk20a_from_pmu(pmu);
	struct mm_gk20a *mm = &g->mm;
	struct pmu_ucode_desc *desc = pmu->desc;
	u64 addr_code_lo, addr_data_lo, addr_load_lo;
	u64 addr_code_hi, addr_data_hi;
	u32 i, blocks, addr_args;

	nvgpu_log_fn(g, " ");

	gk20a_writel(g, pwr_falcon_itfen_r(),
		gk20a_readl(g, pwr_falcon_itfen_r()) |
		pwr_falcon_itfen_ctxen_enable_f());

	gk20a_writel(g, pwr_pmu_new_instblk_r(),
		pwr_pmu_new_instblk_ptr_f(
		nvgpu_inst_block_addr(g, &mm->pmu.inst_block) >> ALIGN_4KB) |
		     pwr_pmu_new_instblk_valid_f(1) |
		     (nvgpu_is_enabled(g, NVGPU_USE_COHERENT_SYSMEM) ?
		      pwr_pmu_new_instblk_target_sys_coh_f() :
		      pwr_pmu_new_instblk_target_sys_ncoh_f()));

	/* TBD: load all other surfaces */
	g->ops.pmu_ver.set_pmu_cmdline_args_trace_size(
		pmu, GK20A_PMU_TRACE_BUFSIZE);
	g->ops.pmu_ver.set_pmu_cmdline_args_trace_dma_base(pmu);
	g->ops.pmu_ver.set_pmu_cmdline_args_trace_dma_idx(
		pmu, GK20A_PMU_DMAIDX_VIRT);

	g->ops.pmu_ver.set_pmu_cmdline_args_cpu_freq(pmu,
		g->ops.clk.get_rate(g, CTRL_CLK_DOMAIN_PWRCLK));

	addr_args = (pwr_falcon_hwcfg_dmem_size_v(
		gk20a_readl(g, pwr_falcon_hwcfg_r()))
			<< GK20A_PMU_DMEM_BLKSIZE2) -
		g->ops.pmu_ver.get_pmu_cmdline_args_size(pmu);

	nvgpu_falcon_copy_to_dmem(pmu->flcn, addr_args,
			(u8 *)(g->ops.pmu_ver.get_pmu_cmdline_args_ptr(pmu)),
			g->ops.pmu_ver.get_pmu_cmdline_args_size(pmu), 0);

	gk20a_writel(g, pwr_falcon_dmemc_r(0),
		pwr_falcon_dmemc_offs_f(0) |
		pwr_falcon_dmemc_blk_f(0)  |
		pwr_falcon_dmemc_aincw_f(1));

	addr_code_lo = u64_lo32((pmu->ucode.gpu_va +
			desc->app_start_offset +
			desc->app_resident_code_offset) >> 8);

	addr_code_hi = u64_hi32((pmu->ucode.gpu_va +
			desc->app_start_offset +
			desc->app_resident_code_offset) >> 8);
	addr_data_lo = u64_lo32((pmu->ucode.gpu_va +
			desc->app_start_offset +
			desc->app_resident_data_offset) >> 8);
	addr_data_hi = u64_hi32((pmu->ucode.gpu_va +
			desc->app_start_offset +
			desc->app_resident_data_offset) >> 8);
	addr_load_lo = u64_lo32((pmu->ucode.gpu_va +
			desc->bootloader_start_offset) >> 8);

	gk20a_writel(g, pwr_falcon_dmemd_r(0), 0x0U);
	gk20a_writel(g, pwr_falcon_dmemd_r(0), 0x0U);
	gk20a_writel(g, pwr_falcon_dmemd_r(0), 0x0U);
	gk20a_writel(g, pwr_falcon_dmemd_r(0), 0x0U);
	gk20a_writel(g, pwr_falcon_dmemd_r(0), 0x0U);
	gk20a_writel(g, pwr_falcon_dmemd_r(0), 0x0U);
	gk20a_writel(g, pwr_falcon_dmemd_r(0), 0x0U);
	gk20a_writel(g, pwr_falcon_dmemd_r(0), 0x0U);
	gk20a_writel(g, pwr_falcon_dmemd_r(0), GK20A_PMU_DMAIDX_UCODE);
	gk20a_writel(g, pwr_falcon_dmemd_r(0), addr_code_lo << 8);
	gk20a_writel(g, pwr_falcon_dmemd_r(0), addr_code_hi);
	gk20a_writel(g, pwr_falcon_dmemd_r(0), desc->app_resident_code_offset);
	gk20a_writel(g, pwr_falcon_dmemd_r(0), desc->app_resident_code_size);
	gk20a_writel(g, pwr_falcon_dmemd_r(0), 0x0U);
	gk20a_writel(g, pwr_falcon_dmemd_r(0), 0x0U);
	gk20a_writel(g, pwr_falcon_dmemd_r(0), desc->app_imem_entry);
	gk20a_writel(g, pwr_falcon_dmemd_r(0), addr_data_lo << 8);
	gk20a_writel(g, pwr_falcon_dmemd_r(0), addr_data_hi);
	gk20a_writel(g, pwr_falcon_dmemd_r(0), desc->app_resident_data_size);
	gk20a_writel(g, pwr_falcon_dmemd_r(0), 0x1U);
	gk20a_writel(g, pwr_falcon_dmemd_r(0), addr_args);

	g->ops.pmu.write_dmatrfbase(g,
				U32(addr_load_lo) -
				(desc->bootloader_imem_offset >> U32(8)));

	blocks = ((desc->bootloader_size + 0xFFU) & ~0xFFU) >> 8U;

	for (i = 0; i < blocks; i++) {
		gk20a_writel(g, pwr_falcon_dmatrfmoffs_r(),
			desc->bootloader_imem_offset + (i << 8));
		gk20a_writel(g, pwr_falcon_dmatrffboffs_r(),
			desc->bootloader_imem_offset + (i << 8));
		gk20a_writel(g, pwr_falcon_dmatrfcmd_r(),
			pwr_falcon_dmatrfcmd_imem_f(1)  |
			pwr_falcon_dmatrfcmd_write_f(0) |
			pwr_falcon_dmatrfcmd_size_f(6)  |
			pwr_falcon_dmatrfcmd_ctxdma_f(GK20A_PMU_DMAIDX_UCODE));
	}

	nvgpu_falcon_bootstrap(pmu->flcn, desc->bootloader_entry_point);

	gk20a_writel(g, pwr_falcon_os_r(), desc->app_version);

	return 0;
}

void gv11b_pmu_handle_ext_irq(struct gk20a *g, u32 intr0)
{
	u32 intr1;
	u32 ecc_status, ecc_addr, corrected_cnt, uncorrected_cnt;
	u32 corrected_delta, uncorrected_delta;
	u32 corrected_overflow, uncorrected_overflow;

	/*
	 * handle the ECC interrupt
	 */
	if ((intr0 & pwr_falcon_irqstat_ext_ecc_parity_true_f()) != 0U) {
		intr1 = gk20a_readl(g, pwr_pmu_ecc_intr_status_r());
		if ((intr1 &
			(pwr_pmu_ecc_intr_status_corrected_m() |
			 pwr_pmu_ecc_intr_status_uncorrected_m())) != 0U) {

			ecc_status = gk20a_readl(g,
				pwr_pmu_falcon_ecc_status_r());
			ecc_addr = gk20a_readl(g,
				pwr_pmu_falcon_ecc_address_r());
			corrected_cnt = gk20a_readl(g,
				pwr_pmu_falcon_ecc_corrected_err_count_r());
			uncorrected_cnt = gk20a_readl(g,
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
				gk20a_writel(g, pwr_pmu_falcon_ecc_corrected_err_count_r(), 0);
			}
			if (((intr1 & pwr_pmu_ecc_intr_status_uncorrected_m()) != 0U) ||
							(uncorrected_overflow != 0U)) {
				gk20a_writel(g,
					pwr_pmu_falcon_ecc_uncorrected_err_count_r(), 0);
			}

			gk20a_writel(g, pwr_pmu_falcon_ecc_status_r(),
				pwr_pmu_falcon_ecc_status_reset_task_f());

			/* update counters per slice */
			if (corrected_overflow != 0U) {
				corrected_delta += (0x1UL << pwr_pmu_falcon_ecc_corrected_err_count_total_s());
			}
			if (uncorrected_overflow != 0U) {
				uncorrected_delta += (0x1UL << pwr_pmu_falcon_ecc_uncorrected_err_count_total_s());
			}

			g->ecc.pmu.pmu_ecc_corrected_err_count[0].counter += corrected_delta;
			g->ecc.pmu.pmu_ecc_uncorrected_err_count[0].counter += uncorrected_delta;

			nvgpu_log(g, gpu_dbg_intr,
				"pmu ecc interrupt intr1: 0x%x", intr1);

			if ((ecc_status & pwr_pmu_falcon_ecc_status_corrected_err_imem_m()) != 0U) {
				gv11b_pmu_report_ecc_error(g, 0,
						GPU_PMU_FALCON_IMEM_ECC_CORRECTED,
						ecc_addr,
						g->ecc.pmu.pmu_ecc_corrected_err_count[0].counter);
				nvgpu_log(g, gpu_dbg_intr,
					"imem ecc error corrected");
			}
			if ((ecc_status & pwr_pmu_falcon_ecc_status_uncorrected_err_imem_m()) != 0U) {
				gv11b_pmu_report_ecc_error(g, 0,
						GPU_PMU_FALCON_IMEM_ECC_UNCORRECTED,
						ecc_addr,
						g->ecc.pmu.pmu_ecc_uncorrected_err_count[0].counter);
				nvgpu_log(g, gpu_dbg_intr,
					"imem ecc error uncorrected");
			}
			if ((ecc_status & pwr_pmu_falcon_ecc_status_corrected_err_dmem_m()) != 0U) {
				gv11b_pmu_report_ecc_error(g, 0,
						GPU_PMU_FALCON_DMEM_ECC_CORRECTED,
						ecc_addr,
						g->ecc.pmu.pmu_ecc_corrected_err_count[0].counter);
				nvgpu_log(g, gpu_dbg_intr,
					"dmem ecc error corrected");
			}
			if ((ecc_status & pwr_pmu_falcon_ecc_status_uncorrected_err_dmem_m()) != 0U) {
				gv11b_pmu_report_ecc_error(g, 0,
						GPU_PMU_FALCON_DMEM_ECC_UNCORRECTED,
						ecc_addr,
						g->ecc.pmu.pmu_ecc_uncorrected_err_count[0].counter);
				nvgpu_log(g, gpu_dbg_intr,
					"dmem ecc error uncorrected");
			}

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

static void pmu_handle_pg_sub_feature_msg(struct gk20a *g, struct pmu_msg *msg,
			void *param, u32 handle, u32 status)
{
	nvgpu_log_fn(g, " ");

	if (status != 0U) {
		nvgpu_err(g, "Sub-feature mask update cmd aborted\n");
		return;
	}

	gv11b_dbg_pmu(g, "sub-feature mask update is acknowledged from PMU %x\n",
							msg->msg.pg.msg_type);
}

static void pmu_handle_pg_param_msg(struct gk20a *g, struct pmu_msg *msg,
			void *param, u32 handle, u32 status)
{
	nvgpu_log_fn(g, " ");

	if (status != 0U) {
		nvgpu_err(g, "GR PARAM cmd aborted\n");
		return;
	}

	gv11b_dbg_pmu(g, "GR PARAM is acknowledged from PMU %x\n",
							msg->msg.pg.msg_type);
}

int gv11b_pg_gr_init(struct gk20a *g, u32 pg_engine_id)
{
	struct nvgpu_pmu *pmu = &g->pmu;
	struct pmu_cmd cmd;
	u32 seq;

	if (pg_engine_id == PMU_PG_ELPG_ENGINE_ID_GRAPHICS) {
		(void) memset(&cmd, 0, sizeof(struct pmu_cmd));
		cmd.hdr.unit_id = PMU_UNIT_PG;
		cmd.hdr.size = PMU_CMD_HDR_SIZE +
				sizeof(struct pmu_pg_cmd_gr_init_param_v1);
		cmd.cmd.pg.gr_init_param_v1.cmd_type =
				PMU_PG_CMD_ID_PG_PARAM;
		cmd.cmd.pg.gr_init_param_v1.sub_cmd_id =
				PMU_PG_PARAM_CMD_GR_INIT_PARAM;
		cmd.cmd.pg.gr_init_param_v1.featuremask =
				NVGPU_PMU_GR_FEATURE_MASK_ALL;

		gv11b_dbg_pmu(g, "cmd post PMU_PG_CMD_ID_PG_PARAM_INIT\n");
		nvgpu_pmu_cmd_post(g, &cmd, NULL, NULL, PMU_COMMAND_QUEUE_HPQ,
					pmu_handle_pg_param_msg, pmu, &seq);

	} else {
		return -EINVAL;
	}

	return 0;
}

int gv11b_pg_set_subfeature_mask(struct gk20a *g, u32 pg_engine_id)
{
	struct nvgpu_pmu *pmu = &g->pmu;
	struct pmu_cmd cmd;
	u32 seq;

	if (pg_engine_id == PMU_PG_ELPG_ENGINE_ID_GRAPHICS) {
		(void) memset(&cmd, 0, sizeof(struct pmu_cmd));
		cmd.hdr.unit_id = PMU_UNIT_PG;
		cmd.hdr.size = PMU_CMD_HDR_SIZE +
			sizeof(struct pmu_pg_cmd_sub_feature_mask_update);
		cmd.cmd.pg.sf_mask_update.cmd_type =
				PMU_PG_CMD_ID_PG_PARAM;
		cmd.cmd.pg.sf_mask_update.sub_cmd_id =
				PMU_PG_PARAM_CMD_SUB_FEATURE_MASK_UPDATE;
		cmd.cmd.pg.sf_mask_update.ctrl_id =
				PMU_PG_ELPG_ENGINE_ID_GRAPHICS;
		cmd.cmd.pg.sf_mask_update.enabled_mask =
				NVGPU_PMU_GR_FEATURE_MASK_POWER_GATING |
				NVGPU_PMU_GR_FEATURE_MASK_PRIV_RING |
				NVGPU_PMU_GR_FEATURE_MASK_UNBIND |
				NVGPU_PMU_GR_FEATURE_MASK_SAVE_GLOBAL_STATE |
				NVGPU_PMU_GR_FEATURE_MASK_RESET_ENTRY |
				NVGPU_PMU_GR_FEATURE_MASK_HW_SEQUENCE |
				NVGPU_PMU_GR_FEATURE_MASK_ELPG_SRAM |
				NVGPU_PMU_GR_FEATURE_MASK_ELPG_LOGIC |
				NVGPU_PMU_GR_FEATURE_MASK_ELPG_L2RPPG;

		gv11b_dbg_pmu(g, "cmd post PMU_PG_CMD_SUB_FEATURE_MASK_UPDATE\n");
		nvgpu_pmu_cmd_post(g, &cmd, NULL, NULL, PMU_COMMAND_QUEUE_HPQ,
				pmu_handle_pg_sub_feature_msg, pmu, &seq);
	} else {
		return -EINVAL;
	}

	return 0;
}
