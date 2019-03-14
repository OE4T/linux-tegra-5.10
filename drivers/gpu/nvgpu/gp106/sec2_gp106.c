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
#include <nvgpu/mm.h>
#include <nvgpu/io.h>
#include <nvgpu/timers.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/bug.h>

#include "sec2_gp106.h"

#include <nvgpu/hw/gp106/hw_pwr_gp106.h>
#include <nvgpu/hw/gp106/hw_psec_gp106.h>

int gp106_sec2_reset(struct gk20a *g)
{
	nvgpu_log_fn(g, " ");

	gk20a_writel(g, psec_falcon_engine_r(),
			pwr_falcon_engine_reset_true_f());
	nvgpu_udelay(10);
	gk20a_writel(g, psec_falcon_engine_r(),
			pwr_falcon_engine_reset_false_f());

	nvgpu_log_fn(g, "done");
	return 0;
}

void gp106_sec2_flcn_setup_boot_config(struct gk20a *g)
{
	struct mm_gk20a *mm = &g->mm;
	u64 tmp_addr;
	u32 data = 0U;

	nvgpu_log_fn(g, " ");

	data = gk20a_readl(g, psec_fbif_ctl_r());
	data |= psec_fbif_ctl_allow_phys_no_ctx_allow_f();
	gk20a_writel(g, psec_fbif_ctl_r(), data);

	/* setup apertures - virtual */
	gk20a_writel(g, psec_fbif_transcfg_r(GK20A_PMU_DMAIDX_UCODE),
			psec_fbif_transcfg_mem_type_physical_f() |
			psec_fbif_transcfg_target_local_fb_f());
	gk20a_writel(g, psec_fbif_transcfg_r(GK20A_PMU_DMAIDX_VIRT),
			psec_fbif_transcfg_mem_type_virtual_f());
	/* setup apertures - physical */
	gk20a_writel(g, psec_fbif_transcfg_r(GK20A_PMU_DMAIDX_PHYS_VID),
			psec_fbif_transcfg_mem_type_physical_f() |
			psec_fbif_transcfg_target_local_fb_f());
	gk20a_writel(g, psec_fbif_transcfg_r(GK20A_PMU_DMAIDX_PHYS_SYS_COH),
			psec_fbif_transcfg_mem_type_physical_f() |
			psec_fbif_transcfg_target_coherent_sysmem_f());
	gk20a_writel(g, psec_fbif_transcfg_r(GK20A_PMU_DMAIDX_PHYS_SYS_NCOH),
			psec_fbif_transcfg_mem_type_physical_f() |
			psec_fbif_transcfg_target_noncoherent_sysmem_f());

	/* enable the context interface */
	gk20a_writel(g, psec_falcon_itfen_r(),
		gk20a_readl(g, psec_falcon_itfen_r()) |
		psec_falcon_itfen_ctxen_enable_f());

	/*
	 * The instance block address to write is the lower 32-bits of the 4K-
	 * aligned physical instance block address.
	 */
	tmp_addr = nvgpu_inst_block_addr(g, &mm->pmu.inst_block) >> 12U;
		nvgpu_assert(u64_hi32(tmp_addr) == 0U);

	gk20a_writel(g, psec_falcon_nxtctx_r(),
		pwr_pmu_new_instblk_ptr_f((u32)tmp_addr) |
		pwr_pmu_new_instblk_valid_f(1U) |
		nvgpu_aperture_mask(g, &mm->pmu.inst_block,
			pwr_pmu_new_instblk_target_sys_ncoh_f(),
			pwr_pmu_new_instblk_target_sys_coh_f(),
			pwr_pmu_new_instblk_target_fb_f()));

	data = gk20a_readl(g, psec_falcon_debug1_r());
	data |= psec_falcon_debug1_ctxsw_mode_m();
	gk20a_writel(g, psec_falcon_debug1_r(), data);

	/* Trigger context switch */
	data = gk20a_readl(g, psec_falcon_engctl_r());
	data |= BIT32(3);
	gk20a_writel(g, psec_falcon_engctl_r(), data);
}

u32 gp106_sec2_falcon_base_addr(void)
{
	return psec_falcon_irqsset_r();
}
