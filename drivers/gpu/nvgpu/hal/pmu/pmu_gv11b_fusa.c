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

static struct nvgpu_hw_err_inject_info pmu_ecc_err_desc[] = {
	NVGPU_ECC_ERR("falcon_imem_ecc_corrected",
			gv11b_pmu_inject_ecc_error,
			pwr_pmu_falcon_ecc_control_r,
			pwr_pmu_falcon_ecc_control_inject_corrected_err_f),
	NVGPU_ECC_ERR("falcon_imem_ecc_uncorrected",
			gv11b_pmu_inject_ecc_error,
			pwr_pmu_falcon_ecc_control_r,
			pwr_pmu_falcon_ecc_control_inject_uncorrected_err_f),
};

static struct nvgpu_hw_err_inject_info_desc pmu_err_desc;

struct nvgpu_hw_err_inject_info_desc *
gv11b_pmu_intr_get_err_desc(struct gk20a *g)
{
	pmu_err_desc.info_ptr = pmu_ecc_err_desc;
	pmu_err_desc.info_size = nvgpu_safe_cast_u64_to_u32(
			sizeof(pmu_ecc_err_desc) /
			sizeof(struct nvgpu_hw_err_inject_info));

	return &pmu_err_desc;
}

/* error handler */
void gv11b_clear_pmu_bar0_host_err_status(struct gk20a *g)
{
	u32 status;

	status = nvgpu_readl(g, pwr_pmu_bar0_host_error_r());
	nvgpu_writel(g, pwr_pmu_bar0_host_error_r(), status);
}

static u32 pmu_bar0_host_tout_etype(u32 val)
{
	return (val != 0U) ? PMU_BAR0_HOST_WRITE_TOUT : PMU_BAR0_HOST_READ_TOUT;
}

static u32 pmu_bar0_fecs_tout_etype(u32 val)
{
	return (val != 0U) ? PMU_BAR0_FECS_WRITE_TOUT : PMU_BAR0_FECS_READ_TOUT;
}

static u32 pmu_bar0_cmd_hwerr_etype(u32 val)
{
	return (val != 0U) ? PMU_BAR0_CMD_WRITE_HWERR : PMU_BAR0_CMD_READ_HWERR;
}

static u32 pmu_bar0_fecserr_etype(u32 val)
{
	return (val != 0U) ? PMU_BAR0_WRITE_FECSERR : PMU_BAR0_READ_FECSERR;
}

static u32 pmu_bar0_hosterr_etype(u32 val)
{
	return (val != 0U) ? PMU_BAR0_WRITE_HOSTERR : PMU_BAR0_READ_HOSTERR;
}

int gv11b_pmu_bar0_error_status(struct gk20a *g, u32 *bar0_status,
	u32 *etype)
{
	u32 val = 0;
	u32 err_status = 0;
	u32 err_cmd = 0;

	val = nvgpu_readl(g, pwr_pmu_bar0_error_status_r());
	*bar0_status = val;
	if (val == 0U) {
		return 0;
	}

	err_cmd = val & pwr_pmu_bar0_error_status_err_cmd_m();

	if ((val & pwr_pmu_bar0_error_status_timeout_host_m()) != 0U) {
		*etype = pmu_bar0_host_tout_etype(err_cmd);
	} else if ((val & pwr_pmu_bar0_error_status_timeout_fecs_m()) != 0U) {
		*etype = pmu_bar0_fecs_tout_etype(err_cmd);
	} else if ((val & pwr_pmu_bar0_error_status_cmd_hwerr_m()) != 0U) {
		*etype = pmu_bar0_cmd_hwerr_etype(err_cmd);
	} else if ((val & pwr_pmu_bar0_error_status_fecserr_m()) != 0U) {
		*etype = pmu_bar0_fecserr_etype(err_cmd);
		err_status = nvgpu_readl(g, pwr_pmu_bar0_fecs_error_r());
		/*
		 * BAR0_FECS_ERROR would only record the first error code if
		 * multiple FECS error happen. Once BAR0_FECS_ERROR is cleared,
		 * BAR0_FECS_ERROR can record the error code from FECS again.
		 * Writing status regiter to clear the FECS Hardware state.
		 */
		nvgpu_writel(g, pwr_pmu_bar0_fecs_error_r(), err_status);
	} else if ((val & pwr_pmu_bar0_error_status_hosterr_m()) != 0U) {
		*etype = pmu_bar0_hosterr_etype(err_cmd);
		/*
		 * BAR0_HOST_ERROR would only record the first error code if
		 * multiple HOST error happen. Once BAR0_HOST_ERROR is cleared,
		 * BAR0_HOST_ERROR can record the error code from HOST again.
		 * Writing status regiter to clear the FECS Hardware state.
		 *
		 * Defining clear ops for host err as gk20a does not have
		 * status register for this.
		 */
		if (g->ops.pmu.pmu_clear_bar0_host_err_status != NULL) {
			g->ops.pmu.pmu_clear_bar0_host_err_status(g);
		}
	} else {
		nvgpu_err(g, "PMU bar0 status type is not found");
	}

	/* Writing Bar0 status regiter to clear the Hardware state */
	nvgpu_writel(g, pwr_pmu_bar0_error_status_r(), val);
	return (-EIO);
}

int gv11b_pmu_correct_ecc(struct gk20a *g, u32 ecc_status, u32 ecc_addr)
{
	int ret = 0;

	if ((ecc_status &
		pwr_pmu_falcon_ecc_status_corrected_err_imem_m()) != 0U) {
		(void) nvgpu_report_ecc_err(g, NVGPU_ERR_MODULE_PMU, 0,
			GPU_PMU_FALCON_IMEM_ECC_CORRECTED,
			ecc_addr,
			g->ecc.pmu.pmu_ecc_corrected_err_count[0].counter);
		nvgpu_log(g, gpu_dbg_intr, "imem ecc error corrected");
	}
	if ((ecc_status &
		pwr_pmu_falcon_ecc_status_uncorrected_err_imem_m()) != 0U) {
		(void) nvgpu_report_ecc_err(g, NVGPU_ERR_MODULE_PMU, 0,
			GPU_PMU_FALCON_IMEM_ECC_UNCORRECTED,
			ecc_addr,
			g->ecc.pmu.pmu_ecc_uncorrected_err_count[0].counter);
		nvgpu_log(g, gpu_dbg_intr, "imem ecc error uncorrected");
		ret = -EFAULT;
	}
	if ((ecc_status &
		pwr_pmu_falcon_ecc_status_corrected_err_dmem_m()) != 0U) {
		(void) nvgpu_report_ecc_err(g, NVGPU_ERR_MODULE_PMU, 0,
			GPU_PMU_FALCON_DMEM_ECC_CORRECTED,
			ecc_addr,
			g->ecc.pmu.pmu_ecc_corrected_err_count[0].counter);
		nvgpu_log(g, gpu_dbg_intr, "dmem ecc error corrected");
	}
	if ((ecc_status &
		pwr_pmu_falcon_ecc_status_uncorrected_err_dmem_m()) != 0U) {
		(void) nvgpu_report_ecc_err(g, NVGPU_ERR_MODULE_PMU, 0,
			GPU_PMU_FALCON_DMEM_ECC_UNCORRECTED,
			ecc_addr,
			g->ecc.pmu.pmu_ecc_uncorrected_err_count[0].counter);
		nvgpu_log(g, gpu_dbg_intr, "dmem ecc error uncorrected");
		ret = -EFAULT;
	}

	return ret;
}

bool gv11b_pmu_validate_mem_integrity(struct gk20a *g)
{
	u32 ecc_status, ecc_addr;

	ecc_status = nvgpu_readl(g, pwr_pmu_falcon_ecc_status_r());
	ecc_addr = nvgpu_readl(g, pwr_pmu_falcon_ecc_address_r());

	return ((gv11b_pmu_correct_ecc(g, ecc_status, ecc_addr) == 0) ? true :
			false);
}

bool gv11b_pmu_is_debug_mode_en(struct gk20a *g)
{
	u32 ctl_stat =  nvgpu_readl(g, pwr_pmu_scpctl_stat_r());

	return pwr_pmu_scpctl_stat_debug_mode_v(ctl_stat) != 0U;
}

void gv11b_pmu_flcn_setup_boot_config(struct gk20a *g)
{
	struct mm_gk20a *mm = &g->mm;
	u32 inst_block_ptr;

	nvgpu_log_fn(g, " ");

	/* setup apertures */
	if (g->ops.pmu.setup_apertures != NULL) {
		g->ops.pmu.setup_apertures(g);
	}

	/* Clearing mailbox register used to reflect capabilities */
	nvgpu_writel(g, pwr_falcon_mailbox1_r(), 0);

	/* enable the context interface */
	nvgpu_writel(g, pwr_falcon_itfen_r(),
		nvgpu_readl(g, pwr_falcon_itfen_r()) |
		pwr_falcon_itfen_ctxen_enable_f());

	/*
	 * The instance block address to write is the lower 32-bits of the 4K-
	 * aligned physical instance block address.
	 */
	inst_block_ptr = nvgpu_inst_block_ptr(g, &mm->pmu.inst_block);

	nvgpu_writel(g, pwr_pmu_new_instblk_r(),
		pwr_pmu_new_instblk_ptr_f(inst_block_ptr) |
		pwr_pmu_new_instblk_valid_f(1U) |
		(nvgpu_is_enabled(g, NVGPU_USE_COHERENT_SYSMEM) ?
		pwr_pmu_new_instblk_target_sys_coh_f() :
		pwr_pmu_new_instblk_target_sys_ncoh_f()));
}

void gv11b_setup_apertures(struct gk20a *g)
{
	struct mm_gk20a *mm = &g->mm;
	struct nvgpu_mem *inst_block = &mm->pmu.inst_block;

	nvgpu_log_fn(g, " ");

	/* setup apertures - virtual */
	nvgpu_writel(g, pwr_fbif_transcfg_r(GK20A_PMU_DMAIDX_UCODE),
		pwr_fbif_transcfg_mem_type_physical_f() |
		nvgpu_aperture_mask(g, inst_block,
		pwr_fbif_transcfg_target_noncoherent_sysmem_f(),
		pwr_fbif_transcfg_target_coherent_sysmem_f(),
		pwr_fbif_transcfg_target_local_fb_f()));
	nvgpu_writel(g, pwr_fbif_transcfg_r(GK20A_PMU_DMAIDX_VIRT),
		pwr_fbif_transcfg_mem_type_virtual_f());
	/* setup apertures - physical */
	nvgpu_writel(g, pwr_fbif_transcfg_r(GK20A_PMU_DMAIDX_PHYS_VID),
		pwr_fbif_transcfg_mem_type_physical_f() |
		nvgpu_aperture_mask(g, inst_block,
		pwr_fbif_transcfg_target_noncoherent_sysmem_f(),
		pwr_fbif_transcfg_target_coherent_sysmem_f(),
		pwr_fbif_transcfg_target_local_fb_f()));
	nvgpu_writel(g, pwr_fbif_transcfg_r(GK20A_PMU_DMAIDX_PHYS_SYS_COH),
		pwr_fbif_transcfg_mem_type_physical_f() |
		pwr_fbif_transcfg_target_coherent_sysmem_f());
	nvgpu_writel(g, pwr_fbif_transcfg_r(GK20A_PMU_DMAIDX_PHYS_SYS_NCOH),
		pwr_fbif_transcfg_mem_type_physical_f() |
		pwr_fbif_transcfg_target_noncoherent_sysmem_f());
}

void gv11b_write_dmatrfbase(struct gk20a *g, u32 addr)
{
	nvgpu_writel(g, pwr_falcon_dmatrfbase_r(),
				addr);
	nvgpu_writel(g, pwr_falcon_dmatrfbase1_r(),
				0x0U);
}

u32 gv11b_pmu_falcon_base_addr(void)
{
	return pwr_falcon_irqsset_r();
}

void gv11b_secured_pmu_start(struct gk20a *g)
{
	nvgpu_writel(g, pwr_falcon_cpuctl_alias_r(),
		pwr_falcon_cpuctl_startcpu_f(1));
}

bool gv11b_is_pmu_supported(struct gk20a *g)
{
#ifdef CONFIG_NVGPU_LS_PMU
	return true;
#else
	/* set to false to disable LS PMU ucode support */
	return false;
#endif
}
