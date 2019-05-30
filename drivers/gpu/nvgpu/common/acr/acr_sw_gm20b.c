/*
 * Copyright (c) 2015-2019, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/types.h>
#include <nvgpu/firmware.h>
#include <nvgpu/falcon.h>
#include <nvgpu/bug.h>
#include <nvgpu/pmu/fw.h>

#include "acr_wpr.h"
#include "acr_priv.h"
#include "acr_sw_gm20b.h"
#include "acr_blob_alloc.h"
#include "acr_bootstrap.h"
#include "acr_blob_construct_v0.h"

static void gm20b_acr_patch_wpr_info_to_ucode(struct gk20a *g,
	struct nvgpu_acr *acr, struct hs_acr *acr_desc, bool is_recovery)
{
	struct nvgpu_firmware *acr_fw = acr_desc->acr_fw;
	struct acr_fw_header *acr_fw_hdr = NULL;
	struct bin_hdr *acr_fw_bin_hdr = NULL;
	struct flcn_acr_desc *acr_dmem_desc;
	u32 *acr_ucode_header = NULL;
	u32 *acr_ucode_data = NULL;

	nvgpu_log_fn(g, " ");

	if (is_recovery) {
		acr_desc->acr_dmem_desc->nonwpr_ucode_blob_size = 0U;
	} else {
		acr_fw_bin_hdr = (struct bin_hdr *)acr_fw->data;
		acr_fw_hdr = (struct acr_fw_header *)
			(acr_fw->data + acr_fw_bin_hdr->header_offset);

		acr_ucode_data = (u32 *)(acr_fw->data +
			acr_fw_bin_hdr->data_offset);

		acr_ucode_header = (u32 *)(acr_fw->data +
			acr_fw_hdr->hdr_offset);

		/* During recovery need to update blob size as 0x0*/
		acr_desc->acr_dmem_desc = (struct flcn_acr_desc *)((u8 *)(
			acr_desc->acr_ucode.cpu_va) + acr_ucode_header[2U]);

		/* Patch WPR info to ucode */
		acr_dmem_desc = (struct flcn_acr_desc *)
			&(((u8 *)acr_ucode_data)[acr_ucode_header[2U]]);

		acr_dmem_desc->nonwpr_ucode_blob_start =
			nvgpu_mem_get_addr(g, &g->acr->ucode_blob);
		nvgpu_assert(g->acr->ucode_blob.size <= U32_MAX);
		acr_dmem_desc->nonwpr_ucode_blob_size =
			(u32)g->acr->ucode_blob.size;
		acr_dmem_desc->regions.no_regions = 1U;
		acr_dmem_desc->wpr_offset = 0U;
	}
}

static void gm20b_acr_fill_bl_dmem_desc(struct gk20a *g,
	struct nvgpu_acr *acr, struct hs_acr *acr_desc,
	u32 *acr_ucode_header)
{
	struct flcn_bl_dmem_desc *bl_dmem_desc = &acr_desc->bl_dmem_desc;

	nvgpu_log_fn(g, " ");

	(void) memset(bl_dmem_desc, 0, sizeof(struct flcn_bl_dmem_desc));

	bl_dmem_desc->signature[0] = 0U;
	bl_dmem_desc->signature[1] = 0U;
	bl_dmem_desc->signature[2] = 0U;
	bl_dmem_desc->signature[3] = 0U;
	bl_dmem_desc->ctx_dma = GK20A_PMU_DMAIDX_VIRT;
	bl_dmem_desc->code_dma_base =
		(unsigned int)(((u64)acr_desc->acr_ucode.gpu_va >> 8U));
	bl_dmem_desc->code_dma_base1 = 0x0U;
	bl_dmem_desc->non_sec_code_off  = acr_ucode_header[0U];
	bl_dmem_desc->non_sec_code_size = acr_ucode_header[1U];
	bl_dmem_desc->sec_code_off = acr_ucode_header[5U];
	bl_dmem_desc->sec_code_size = acr_ucode_header[6U];
	bl_dmem_desc->code_entry_point = 0U; /* Start at 0th offset */
	bl_dmem_desc->data_dma_base =
		bl_dmem_desc->code_dma_base +
		((acr_ucode_header[2U]) >> 8U);
	bl_dmem_desc->data_dma_base1 = 0x0U;
	bl_dmem_desc->data_size = acr_ucode_header[3U];
}

/* LSF static config functions */
static u32 gm20b_acr_lsf_pmu(struct gk20a *g,
		struct acr_lsf_config *lsf)
{
	/* PMU LS falcon info */
	lsf->falcon_id = FALCON_ID_PMU;
	lsf->falcon_dma_idx = GK20A_PMU_DMAIDX_UCODE;
	lsf->is_lazy_bootstrap = false;
	lsf->is_priv_load = false;
#ifdef NVGPU_FEATURE_LS_PMU
	lsf->get_lsf_ucode_details = nvgpu_acr_lsf_pmu_ucode_details_v0;
	lsf->get_cmd_line_args_offset = nvgpu_pmu_fw_get_cmd_line_args_offset;
#endif
	return BIT32(lsf->falcon_id);
}

static u32 gm20b_acr_lsf_fecs(struct gk20a *g,
		struct acr_lsf_config *lsf)
{
	/* FECS LS falcon info */
	lsf->falcon_id = FALCON_ID_FECS;
	lsf->falcon_dma_idx = GK20A_PMU_DMAIDX_UCODE;
	lsf->is_lazy_bootstrap = false;
	lsf->is_priv_load = false;
	lsf->get_lsf_ucode_details = nvgpu_acr_lsf_fecs_ucode_details_v0;
	lsf->get_cmd_line_args_offset = NULL;

	return BIT32(lsf->falcon_id);
}

static u32 gm20b_acr_lsf_conifg(struct gk20a *g,
	struct nvgpu_acr *acr)
{
	u32 lsf_enable_mask = 0;

	lsf_enable_mask |= gm20b_acr_lsf_pmu(g, &acr->lsf[FALCON_ID_PMU]);
	lsf_enable_mask |= gm20b_acr_lsf_fecs(g, &acr->lsf[FALCON_ID_FECS]);

	return lsf_enable_mask;
}

static void gm20b_acr_default_sw_init(struct gk20a *g, struct hs_acr *hs_acr)
{
	struct hs_flcn_bl *hs_bl = &hs_acr->acr_hs_bl;

	nvgpu_log_fn(g, " ");

	/* ACR HS bootloader ucode name */
	hs_bl->bl_fw_name = HSBIN_ACR_BL_UCODE_IMAGE;

	/* ACR HS ucode type & f/w name*/
	hs_acr->acr_type = ACR_DEFAULT;
	hs_acr->acr_fw_name = HSBIN_ACR_UCODE_IMAGE;

	/* bootlader interface used by ACR HS bootloader*/
	hs_acr->ptr_bl_dmem_desc = &hs_acr->bl_dmem_desc;
	hs_acr->bl_dmem_desc_size = (u32)sizeof(struct flcn_bl_dmem_desc);

	/* set on which falcon ACR need to execute*/
	hs_acr->acr_flcn = g->pmu->flcn;
	hs_acr->acr_flcn_setup_boot_config =
		g->ops.pmu.flcn_setup_boot_config;
	hs_acr->acr_engine_bus_err_status =
		g->ops.pmu.bar0_error_status;
}

void nvgpu_gm20b_acr_sw_init(struct gk20a *g, struct nvgpu_acr *acr)
{
	nvgpu_log_fn(g, " ");

	acr->g = g;

	acr->bootstrap_owner = FALCON_ID_PMU;

	acr->lsf_enable_mask = gm20b_acr_lsf_conifg(g, acr);

	gm20b_acr_default_sw_init(g, &acr->acr);

	acr->prepare_ucode_blob = nvgpu_acr_prepare_ucode_blob_v0;
	acr->get_wpr_info = nvgpu_acr_wpr_info_sys;
	acr->alloc_blob_space = nvgpu_acr_alloc_blob_space_sys;
	acr->bootstrap_hs_acr = nvgpu_acr_bootstrap_hs_ucode;
	acr->patch_wpr_info_to_ucode =
		gm20b_acr_patch_wpr_info_to_ucode;
	acr->acr_fill_bl_dmem_desc =
		gm20b_acr_fill_bl_dmem_desc;
}
