/*
 * Copyright (c) 2019, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef ACR_BLOB_CONSTRUCT_V1_H
#define ACR_BLOB_CONSTRUCT_V1_H

#include <nvgpu/falcon.h>
#include <nvgpu/flcnif_cmn.h>

/*
 * Light Secure WPR Content Alignments
 */
#define LSF_WPR_HEADER_ALIGNMENT        (256U)
#define LSF_SUB_WPR_HEADER_ALIGNMENT    (256U)
#define LSF_LSB_HEADER_ALIGNMENT        (256U)
#define LSF_BL_DATA_ALIGNMENT           (256U)
#define LSF_BL_DATA_SIZE_ALIGNMENT      (256U)
#define LSF_BL_CODE_SIZE_ALIGNMENT      (256U)
#define LSF_DATA_SIZE_ALIGNMENT         (256U)
#define LSF_CODE_SIZE_ALIGNMENT         (256U)

/* MMU excepts sub_wpr sizes in units of 4K */
#define SUB_WPR_SIZE_ALIGNMENT	(4096U)

/*
 * Maximum WPR Header size
 */
#define LSF_WPR_HEADERS_TOTAL_SIZE_MAX	\
	(ALIGN_UP(((u32)sizeof(struct lsf_wpr_header_v1) * FALCON_ID_END), \
		LSF_WPR_HEADER_ALIGNMENT))
#define LSF_LSB_HEADER_TOTAL_SIZE_MAX	(\
	ALIGN_UP(sizeof(struct lsf_lsb_header_v1), LSF_LSB_HEADER_ALIGNMENT))

/* Maximum SUB WPR header size */
#define LSF_SUB_WPR_HEADERS_TOTAL_SIZE_MAX	(ALIGN_UP( \
	(sizeof(struct lsf_shared_sub_wpr_header) * \
	LSF_SHARED_DATA_SUB_WPR_USE_CASE_ID_MAX), \
	LSF_SUB_WPR_HEADER_ALIGNMENT))


#define LSF_UCODE_DATA_ALIGNMENT 4096U


/* Defined for 1MB alignment */
#define SHIFT_1MB	(20U)
#define SHIFT_4KB	(12U)

/*
 * Maximum WPR Header size
 */
#define LSF_WPR_HEADERS_TOTAL_SIZE_MAX	\
	(ALIGN_UP(((u32)sizeof(struct lsf_wpr_header_v1) * FALCON_ID_END), \
		LSF_WPR_HEADER_ALIGNMENT))

/* shared sub_wpr use case IDs */
enum {
	LSF_SHARED_DATA_SUB_WPR_USE_CASE_ID_FRTS_VBIOS_TABLES	= 1,
	LSF_SHARED_DATA_SUB_WPR_USE_CASE_ID_PLAYREADY_SHARED_DATA = 2
};

#define LSF_SHARED_DATA_SUB_WPR_USE_CASE_ID_MAX \
	LSF_SHARED_DATA_SUB_WPR_USE_CASE_ID_PLAYREADY_SHARED_DATA

#define LSF_SHARED_DATA_SUB_WPR_USE_CASE_ID_INVALID	(0xFFFFFFFFU)

#define MAX_SUPPORTED_SHARED_SUB_WPR_USE_CASES	\
	LSF_SHARED_DATA_SUB_WPR_USE_CASE_ID_MAX

/* Static sizes of shared subWPRs */
/* Minimum granularity supported is 4K */
/* 1MB in 4K */
#define LSF_SHARED_DATA_SUB_WPR_FRTS_VBIOS_TABLES_SIZE_IN_4K	(0x100U)
/* 4K */
#define LSF_SHARED_DATA_SUB_WPR_PLAYREADY_SHARED_DATA_SIZE_IN_4K	(0x1U)

/*Light Secure Bootstrap header related defines*/
#define NV_FLCN_ACR_LSF_FLAG_LOAD_CODE_AT_0_FALSE       0U
#define NV_FLCN_ACR_LSF_FLAG_LOAD_CODE_AT_0_TRUE        BIT32(0)
#define NV_FLCN_ACR_LSF_FLAG_DMACTL_REQ_CTX_FALSE       0U
#define NV_FLCN_ACR_LSF_FLAG_DMACTL_REQ_CTX_TRUE        BIT32(2)
#define NV_FLCN_ACR_LSF_FLAG_FORCE_PRIV_LOAD_TRUE       BIT32(3)
#define NV_FLCN_ACR_LSF_FLAG_FORCE_PRIV_LOAD_FALSE      0U

/*
 * Image Status Defines
 */
#define LSF_IMAGE_STATUS_NONE                           (0U)
#define LSF_IMAGE_STATUS_COPY                           (1U)
#define LSF_IMAGE_STATUS_VALIDATION_CODE_FAILED         (2U)
#define LSF_IMAGE_STATUS_VALIDATION_DATA_FAILED         (3U)
#define LSF_IMAGE_STATUS_VALIDATION_DONE                (4U)
#define LSF_IMAGE_STATUS_VALIDATION_SKIPPED             (5U)
#define LSF_IMAGE_STATUS_BOOTSTRAP_READY                (6U)

struct lsf_wpr_header_v1 {
	u32 falcon_id;
	u32 lsb_offset;
	u32 bootstrap_owner;
	u32 lazy_bootstrap;
	u32 bin_version;
	u32 status;
};

struct lsf_ucode_desc_v1 {
	u8  prd_keys[2][16];
	u8  dbg_keys[2][16];
	u32 b_prd_present;
	u32 b_dbg_present;
	u32 falcon_id;
	u32 bsupports_versioning;
	u32 version;
	u32 dep_map_count;
	u8  dep_map[FALCON_ID_END * 2 * 4];
	u8  kdf[16];
};

struct lsf_lsb_header_v1 {
	struct lsf_ucode_desc_v1 signature;
	u32 ucode_off;
	u32 ucode_size;
	u32 data_size;
	u32 bl_code_size;
	u32 bl_imem_off;
	u32 bl_data_off;
	u32 bl_data_size;
	u32 app_code_off;
	u32 app_code_size;
	u32 app_data_off;
	u32 app_data_size;
	u32 flags;
};

struct flcn_bl_dmem_desc_v1 {
	u32    reserved[4];        /*Should be the first element..*/
	u32    signature[4];        /*Should be the first element..*/
	u32    ctx_dma;
	struct falc_u64 code_dma_base;
	u32    non_sec_code_off;
	u32    non_sec_code_size;
	u32    sec_code_off;
	u32    sec_code_size;
	u32    code_entry_point;
	struct falc_u64 data_dma_base;
	u32    data_size;
	u32 argc;
	u32 argv;
};

#define UCODE_NB_MAX_DATE_LENGTH  64U
struct ls_falcon_ucode_desc {
	u32 descriptor_size;
	u32 image_size;
	u32 tools_version;
	u32 app_version;
	char date[UCODE_NB_MAX_DATE_LENGTH];
	u32 bootloader_start_offset;
	u32 bootloader_size;
	u32 bootloader_imem_offset;
	u32 bootloader_entry_point;
	u32 app_start_offset;
	u32 app_size;
	u32 app_imem_offset;
	u32 app_imem_entry;
	u32 app_dmem_offset;
	u32 app_resident_code_offset;
	u32 app_resident_code_size;
	u32 app_resident_data_offset;
	u32 app_resident_data_size;
	u32 nb_imem_overlays;
	u32 nb_dmem_overlays;
	struct {u32 start; u32 size; } load_ovl[64];
	u32 compressed;
};

struct flcn_ucode_img_v1 {
	u32 *header;
	u32 *data;
	struct ls_falcon_ucode_desc *desc;
	u32 data_size;
	void *fw_ver;
	u8 load_entire_os_data;
	struct lsf_ucode_desc_v1 *lsf_desc;
	u8 free_res_allocs;
	u32 flcn_inst;
};

struct lsfm_managed_ucode_img_v2 {
	struct lsfm_managed_ucode_img_v2 *next;
	struct lsf_wpr_header_v1 wpr_header;
	struct lsf_lsb_header_v1 lsb_header;
	struct flcn_bl_dmem_desc_v1 bl_gen_desc;
	u32 bl_gen_desc_size;
	u32 full_ucode_size;
	struct flcn_ucode_img_v1 ucode_img;
};

/*
 * LSF shared SubWpr Header
 *
 * use_case_id - Shared SubWpr use case ID (updated by nvgpu)
 * start_addr  - start address of subWpr (updated by nvgpu)
 * size_4K     - size of subWpr in 4K (updated by nvgpu)
 */
struct lsf_shared_sub_wpr_header {
	u32 use_case_id;
	u32 start_addr;
	u32 size_4K;
};

/*
 * LSFM SUB WPRs struct
 * pnext          : Next entry in the list, NULL if last
 * sub_wpr_header : SubWpr Header struct
 */
struct lsfm_sub_wpr {
	struct lsfm_sub_wpr *pnext;
	struct lsf_shared_sub_wpr_header sub_wpr_header;
};

struct ls_flcn_mgr_v1 {
	u16 managed_flcn_cnt;
	u32 wpr_size;
	struct lsfm_managed_ucode_img_v2 *ucode_img_list;
	u16 managed_sub_wpr_count;
	struct lsfm_sub_wpr *psub_wpr_list;
};

int nvgpu_acr_prepare_ucode_blob_v1(struct gk20a *g);

int nvgpu_acr_lsf_pmu_ucode_details_v1(struct gk20a *g, void *lsf_ucode_img);
int nvgpu_acr_lsf_fecs_ucode_details_v1(struct gk20a *g, void *lsf_ucode_img);
int nvgpu_acr_lsf_gpccs_ucode_details_v1(struct gk20a *g, void *lsf_ucode_img);
int nvgpu_acr_lsf_sec2_ucode_details_v1(struct gk20a *g, void *lsf_ucode_img);

#endif /* ACR_BLOB_CONSTRUCT_V1_H */
