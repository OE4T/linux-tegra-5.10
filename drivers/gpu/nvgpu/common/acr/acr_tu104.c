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

#include <nvgpu/gk20a.h>
#include <nvgpu/firmware.h>
#include <nvgpu/sec2if/sec2_if_cmn.h>

#include "acr_wpr.h"
#include "acr_priv.h"
#include "acr_blob_alloc.h"
#include "acr_bootstrap.h"
#include "acr_blob_construct_v1.h"
#include "acr_gv100.h"
#include "acr_tu104.h"

#include "tu104/sec2_tu104.h"

static int tu104_bootstrap_hs_acr(struct gk20a *g, struct nvgpu_acr *acr,
	struct hs_acr *acr_type)
{
	int err = 0;

	nvgpu_log_fn(g, " ");

	err = nvgpu_acr_bootstrap_hs_ucode(g, g->acr, &g->acr->acr_ahesasc);
	if (err != 0) {
		nvgpu_err(g, "ACR AHESASC bootstrap failed");
		goto exit;
	}
	err = nvgpu_acr_bootstrap_hs_ucode(g, g->acr, &g->acr->acr_asb);
	if (err != 0) {
		nvgpu_err(g, "ACR ASB bootstrap failed");
		goto exit;
	}

exit:
	return err;
}

/* LSF init */
static u32 tu104_acr_lsf_sec2(struct gk20a *g,
		struct acr_lsf_config *lsf)
{
	/* SEC2 LS falcon info */
	lsf->falcon_id = FALCON_ID_SEC2;
	lsf->falcon_dma_idx = NV_SEC2_DMAIDX_UCODE;
	lsf->is_lazy_bootstrap = false;
	lsf->is_priv_load = false;
	lsf->get_lsf_ucode_details = nvgpu_acr_lsf_sec2_ucode_details_v1;
	lsf->get_cmd_line_args_offset = NULL;

	return BIT32(lsf->falcon_id);
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

	acr_ahesasc->acr_flcn = &g->sec2.flcn;
	acr_ahesasc->acr_flcn_setup_boot_config =
		tu104_sec2_flcn_setup_boot_config;
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
	acr_asb->acr_flcn_setup_boot_config = g->ops.gsp.falcon_setup_boot_config;
}

void nvgpu_tu104_acr_sw_init(struct gk20a *g, struct nvgpu_acr *acr)
{
	nvgpu_log_fn(g, " ");

	/* Inherit settings from older chip */
	nvgpu_gv100_acr_sw_init(g, acr);

	acr->lsf_enable_mask |= tu104_acr_lsf_sec2(g,
		&acr->lsf[FALCON_ID_SEC2]);

	acr->prepare_ucode_blob = nvgpu_acr_prepare_ucode_blob_v1;
	acr->bootstrap_owner = FALCON_ID_GSPLITE;
	acr->bootstrap_hs_acr = tu104_bootstrap_hs_acr;

	/* Init ACR-AHESASC */
	nvgpu_tu104_acr_ahesasc_sw_init(g, &acr->acr_ahesasc);

	/* Init ACR-ASB*/
	nvgpu_tu104_acr_asb_sw_init(g, &acr->acr_asb);
}
