/*
 * Copyright (c) 2016-2018, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/acr/nvgpu_acr.h>
#include <nvgpu/firmware.h>
#include <nvgpu/enabled.h>
#include <nvgpu/utils.h>
#include <nvgpu/debug.h>
#include <nvgpu/kmem.h>
#include <nvgpu/pmu.h>
#include <nvgpu/dma.h>
#include <nvgpu/gk20a.h>

#include "gv100/gsp_gv100.h"
#include "tu104/sec2_tu104.h"

#include "acr_gm20b.h"
#include "acr_gp106.h"
#include "acr_tu104.h"

static int tu104_bootstrap_hs_acr(struct gk20a *g, struct nvgpu_acr *acr,
	struct hs_acr *acr_type)
{
	int err = 0;

	nvgpu_log_fn(g, " ");

	err = gm20b_bootstrap_hs_acr(g, &g->acr, &g->acr.acr_ahesasc);
	if (err != 0) {
		nvgpu_err(g, "ACR AHESASC bootstrap failed");
		goto exit;
	}
	err = gm20b_bootstrap_hs_acr(g, &g->acr, &g->acr.acr_asb);
	if (err != 0) {
		nvgpu_err(g, "ACR ASB bootstrap failed");
		goto exit;
	}

exit:
	return err;
}

/* ACR-AHESASC(ACR hub encryption setter and signature checker) init*/
static void nvgpu_tu104_acr_ahesasc_sw_init(struct gk20a *g,
	struct hs_acr *acr_ahesasc)
{
	struct hs_flcn_bl *hs_bl = &acr_ahesasc->acr_hs_bl;

	hs_bl->bl_fw_name = HSBIN_ACR_BL_UCODE_IMAGE;

	acr_ahesasc->acr_type = ACR_AHESASC;

	if (!g->ops.pmu.is_debug_mode_enabled(g)) {
		acr_ahesasc->acr_fw_name = HSBIN_ACR_AHESASC_PROD_UCODE;
	} else {
		acr_ahesasc->acr_fw_name = HSBIN_ACR_AHESASC_DBG_UCODE;
	}

	acr_ahesasc->ptr_bl_dmem_desc = &acr_ahesasc->bl_dmem_desc_v1;
	acr_ahesasc->bl_dmem_desc_size = (u32)sizeof(struct flcn_bl_dmem_desc_v1);

	acr_ahesasc->acr_flcn = &g->sec2_flcn;
	acr_ahesasc->acr_flcn_setup_hw_and_bl_bootstrap =
		tu104_sec2_setup_hw_and_bl_bootstrap;
}

/* ACR-ASB(ACR SEC2 booter) init*/
static void nvgpu_tu104_acr_asb_sw_init(struct gk20a *g,
	struct hs_acr *acr_asb)
{
	struct hs_flcn_bl *hs_bl = &acr_asb->acr_hs_bl;

	hs_bl->bl_fw_name = HSBIN_ACR_BL_UCODE_IMAGE;

	acr_asb->acr_type = ACR_ASB;

	if (!g->ops.pmu.is_debug_mode_enabled(g)) {
		acr_asb->acr_fw_name = HSBIN_ACR_ASB_PROD_UCODE;
	} else {
		acr_asb->acr_fw_name = HSBIN_ACR_ASB_DBG_UCODE;
	}

	acr_asb->ptr_bl_dmem_desc = &acr_asb->bl_dmem_desc_v1;
	acr_asb->bl_dmem_desc_size = (u32)sizeof(struct flcn_bl_dmem_desc_v1);

	acr_asb->acr_flcn = &g->gsp_flcn;
	acr_asb->acr_flcn_setup_hw_and_bl_bootstrap =
		gv100_gsp_setup_hw_and_bl_bootstrap;
}

static void tu104_free_hs_acr(struct gk20a *g,
	struct hs_acr *acr_type)
{
	struct mm_gk20a *mm = &g->mm;
	struct vm_gk20a *vm = mm->pmu.vm;

	if (acr_type->acr_fw != NULL) {
		nvgpu_release_firmware(g, acr_type->acr_fw);
	}

	if (acr_type->acr_hs_bl.hs_bl_fw != NULL) {
		nvgpu_release_firmware(g, acr_type->acr_hs_bl.hs_bl_fw);
	}

	if (nvgpu_mem_is_valid(&acr_type->acr_ucode)) {
		nvgpu_dma_unmap_free(vm, &acr_type->acr_ucode);
	}
	if (nvgpu_mem_is_valid(&acr_type->acr_hs_bl.hs_bl_ucode)) {
		nvgpu_dma_unmap_free(vm, &acr_type->acr_hs_bl.hs_bl_ucode);
	}
}

static void tu104_remove_acr_support(struct nvgpu_acr *acr)
{
	struct gk20a *g = acr->g;

	tu104_free_hs_acr(g, &acr->acr_ahesasc);

	tu104_free_hs_acr(g, &acr->acr_asb);
}

void nvgpu_tu104_acr_sw_init(struct gk20a *g, struct nvgpu_acr *acr)
{
	nvgpu_log_fn(g, " ");

	/* Inherit settings from older chip */
	nvgpu_gp106_acr_sw_init(g, acr);

	acr->bootstrap_owner = LSF_FALCON_ID_GSPLITE;
	acr->max_supported_lsfm = TU104_MAX_SUPPORTED_LSFM;
	acr->bootstrap_hs_acr = tu104_bootstrap_hs_acr;
	acr->remove_support = tu104_remove_acr_support;

	/* Init ACR-AHESASC */
	nvgpu_tu104_acr_ahesasc_sw_init(g, &acr->acr_ahesasc);

	/* Init ACR-ASB*/
	nvgpu_tu104_acr_asb_sw_init(g, &acr->acr_asb);
}
