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

#include <nvgpu/firmware.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/bug.h>

#include "acr_blob_construct_v1.h"
#include "acr_priv.h"
#include "acr_gv100.h"

#include "gp106/sec2_gp106.h"


static void flcn64_set_dma(struct falc_u64 *dma_addr, u64 value)
{
	dma_addr->lo |= u64_lo32(value);
	dma_addr->hi |= u64_hi32(value);
}

static int gv100_acr_patch_wpr_info_to_ucode(struct gk20a *g, struct nvgpu_acr *acr,
		struct hs_acr *acr_desc, bool is_recovery)
{
	struct nvgpu_firmware *acr_fw = acr_desc->acr_fw;
	struct acr_fw_header *acr_fw_hdr = NULL;
	struct bin_hdr *acr_fw_bin_hdr = NULL;
	struct flcn_acr_desc_v1 *acr_dmem_desc;
	struct wpr_carveout_info wpr_inf;
	u32 *acr_ucode_header = NULL;
	u32 *acr_ucode_data = NULL;

	nvgpu_log_fn(g, " ");

	acr_fw_bin_hdr = (struct bin_hdr *)acr_fw->data;
	acr_fw_hdr = (struct acr_fw_header *)
		(acr_fw->data + acr_fw_bin_hdr->header_offset);

	acr_ucode_data = (u32 *)(acr_fw->data + acr_fw_bin_hdr->data_offset);
	acr_ucode_header = (u32 *)(acr_fw->data + acr_fw_hdr->hdr_offset);

	acr->get_wpr_info(g, &wpr_inf);

	acr_dmem_desc = (struct flcn_acr_desc_v1 *)
		&(((u8 *)acr_ucode_data)[acr_ucode_header[2U]]);

	acr_dmem_desc->nonwpr_ucode_blob_start = wpr_inf.nonwpr_base;
	nvgpu_assert(wpr_inf.size <= U32_MAX);
	acr_dmem_desc->nonwpr_ucode_blob_size = (u32)wpr_inf.size;
	acr_dmem_desc->regions.no_regions = 1U;
	acr_dmem_desc->wpr_offset = 0U;

	acr_dmem_desc->wpr_region_id = 1U;
	acr_dmem_desc->regions.region_props[0U].region_id = 1U;
	acr_dmem_desc->regions.region_props[0U].start_addr =
		(wpr_inf.wpr_base) >> 8U;
	acr_dmem_desc->regions.region_props[0U].end_addr =
		((wpr_inf.wpr_base) + wpr_inf.size) >> 8U;
	acr_dmem_desc->regions.region_props[0U].shadowmMem_startaddress =
		wpr_inf.nonwpr_base >> 8U;

	return 0;
}

int gv100_acr_fill_bl_dmem_desc(struct gk20a *g,
	struct nvgpu_acr *acr, struct hs_acr *acr_desc,
	u32 *acr_ucode_header)
{
	struct nvgpu_mem *acr_ucode_mem = &acr_desc->acr_ucode;
	struct flcn_bl_dmem_desc_v1 *bl_dmem_desc =
		&acr_desc->bl_dmem_desc_v1;

	nvgpu_log_fn(g, " ");

	(void) memset(bl_dmem_desc, 0U, sizeof(struct flcn_bl_dmem_desc_v1));

	bl_dmem_desc->signature[0] = 0U;
	bl_dmem_desc->signature[1] = 0U;
	bl_dmem_desc->signature[2] = 0U;
	bl_dmem_desc->signature[3] = 0U;
	bl_dmem_desc->ctx_dma = GK20A_PMU_DMAIDX_VIRT;

	flcn64_set_dma(&bl_dmem_desc->code_dma_base,
		acr_ucode_mem->gpu_va);

	bl_dmem_desc->non_sec_code_off  = acr_ucode_header[0U];
	bl_dmem_desc->non_sec_code_size = acr_ucode_header[1U];
	bl_dmem_desc->sec_code_off = acr_ucode_header[5U];
	bl_dmem_desc->sec_code_size = acr_ucode_header[6U];
	bl_dmem_desc->code_entry_point = 0U;

	flcn64_set_dma(&bl_dmem_desc->data_dma_base,
		acr_ucode_mem->gpu_va + acr_ucode_header[2U]);

	bl_dmem_desc->data_size = acr_ucode_header[3U];

	return 0;
}

/* LSF init */
static u32 gv100_acr_lsf_pmu(struct gk20a *g,
		struct acr_lsf_config *lsf)
{
	/* PMU LS falcon info */
	lsf->falcon_id = FALCON_ID_PMU;
	lsf->falcon_dma_idx = GK20A_PMU_DMAIDX_UCODE;
	lsf->is_lazy_bootstrap = false;
	lsf->is_priv_load = false;
	lsf->get_lsf_ucode_details = nvgpu_acr_lsf_pmu_ucode_details_v1;
	lsf->get_cmd_line_args_offset = nvgpu_pmu_get_cmd_line_args_offset;

	return BIT32(lsf->falcon_id);
}

static u32 gv100_acr_lsf_fecs(struct gk20a *g,
		struct acr_lsf_config *lsf)
{
	/* FECS LS falcon info */
	lsf->falcon_id = FALCON_ID_FECS;
	lsf->falcon_dma_idx = GK20A_PMU_DMAIDX_UCODE;
	lsf->is_lazy_bootstrap = true;
	lsf->is_priv_load = true;
	lsf->get_lsf_ucode_details = nvgpu_acr_lsf_fecs_ucode_details_v1;
	lsf->get_cmd_line_args_offset = NULL;

	return BIT32(lsf->falcon_id);
}

static u32 gv100_acr_lsf_gpccs(struct gk20a *g,
		struct acr_lsf_config *lsf)
{
	/* FECS LS falcon info */
	lsf->falcon_id = FALCON_ID_GPCCS;
	lsf->falcon_dma_idx = GK20A_PMU_DMAIDX_UCODE;
	lsf->is_lazy_bootstrap = true;
	lsf->is_priv_load = true;
	lsf->get_lsf_ucode_details = nvgpu_acr_lsf_gpccs_ucode_details_v1;
	lsf->get_cmd_line_args_offset = NULL;

	return BIT32(lsf->falcon_id);
}

static u32 gv100_acr_lsf_conifg(struct gk20a *g,
	struct nvgpu_acr *acr)
{
	u32 lsf_enable_mask = 0;

	lsf_enable_mask |= gv100_acr_lsf_pmu(g, &acr->lsf[FALCON_ID_PMU]);
	lsf_enable_mask |= gv100_acr_lsf_fecs(g, &acr->lsf[FALCON_ID_FECS]);
	lsf_enable_mask |= gv100_acr_lsf_gpccs(g, &acr->lsf[FALCON_ID_GPCCS]);

	return lsf_enable_mask;
}

static void nvgpu_gv100_acr_default_sw_init(struct gk20a *g, struct hs_acr *hs_acr)
{
	struct hs_flcn_bl *hs_bl = &hs_acr->acr_hs_bl;

	nvgpu_log_fn(g, " ");

	hs_bl->bl_fw_name = HSBIN_ACR_BL_UCODE_IMAGE;

	hs_acr->acr_type = ACR_DEFAULT;
	hs_acr->acr_fw_name = HSBIN_ACR_UCODE_IMAGE;

	hs_acr->ptr_bl_dmem_desc = &hs_acr->bl_dmem_desc_v1;
	hs_acr->bl_dmem_desc_size = (u32)sizeof(struct flcn_bl_dmem_desc_v1);

	hs_acr->acr_flcn = &g->sec2.flcn;
	hs_acr->acr_flcn_setup_boot_config = gp106_sec2_flcn_setup_boot_config;
}

void nvgpu_gv100_acr_sw_init(struct gk20a *g, struct nvgpu_acr *acr)
{
	nvgpu_log_fn(g, " ");

	acr->g = g;

	acr->bootstrap_owner = FALCON_ID_SEC2;

	acr->lsf_enable_mask = gv100_acr_lsf_conifg(g, acr);

	nvgpu_gv100_acr_default_sw_init(g, &acr->acr);

	acr->prepare_ucode_blob = nvgpu_acr_prepare_ucode_blob_v1;
	acr->get_wpr_info = nvgpu_acr_wpr_info_vid;
	acr->alloc_blob_space = nvgpu_acr_alloc_blob_space_vid;
	acr->bootstrap_hs_acr = nvgpu_acr_bootstrap_hs_ucode;
	acr->patch_wpr_info_to_ucode =
		gv100_acr_patch_wpr_info_to_ucode;
	acr->acr_fill_bl_dmem_desc = gv100_acr_fill_bl_dmem_desc;
}
