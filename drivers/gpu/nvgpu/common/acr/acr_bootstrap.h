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

#ifndef ACR_BOOTSTRAP_H
#define ACR_BOOTSTRAP_H

#include "acr_falcon_bl.h"

struct gk20a;
struct nvgpu_acr;

/*
 * Supporting maximum of 2 regions.
 * This is needed to pre-allocate space in DMEM
 */
#define NVGPU_FLCN_ACR_MAX_REGIONS                (2U)
#define LSF_BOOTSTRAP_OWNER_RESERVED_DMEM_SIZE    (0x200U)

struct flcn_acr_region_prop {
	u32 start_addr;
	u32 end_addr;
	u32 region_id;
	u32 read_mask;
	u32 write_mask;
	u32 client_mask;
};

struct flcn_acr_regions {
	u32 no_regions;
	struct flcn_acr_region_prop region_props[NVGPU_FLCN_ACR_MAX_REGIONS];
};

struct flcn_acr_desc {
	union {
		u32 reserved_dmem[(LSF_BOOTSTRAP_OWNER_RESERVED_DMEM_SIZE/4)];
		u32 signatures[4];
	} ucode_reserved_space;
	/*Always 1st*/
	u32 wpr_region_id;
	u32 wpr_offset;
	u32 mmu_mem_range;
	struct flcn_acr_regions regions;
	u32 nonwpr_ucode_blob_size;
	u64 nonwpr_ucode_blob_start;
};

/*
 * start_addr     - Starting address of region
 * end_addr       - Ending address of region
 * region_id      - Region ID
 * read_mask      - Read Mask
 * write_mask     - WriteMask
 * client_mask    - Bit map of all clients currently using this region
 */
struct flcn_acr_region_prop_v1 {
	u32 start_addr;
	u32 end_addr;
	u32 region_id;
	u32 read_mask;
	u32 write_mask;
	u32 client_mask;
	u32 shadowmMem_startaddress;
};

/*
 * no_regions   - Number of regions used.
 * region_props - Region properties
 */
struct flcn_acr_regions_v1 {
	u32 no_regions;
	struct flcn_acr_region_prop_v1 region_props[NVGPU_FLCN_ACR_MAX_REGIONS];
};

/*
 * reserved_dmem-When the bootstrap owner has done bootstrapping other falcons,
 *                and need to switch into LS mode, it needs to have its own
 *                actual DMEM image copied into DMEM as part of LS setup. If
 *                ACR desc is at location 0, it will definitely get overwritten
 *                causing data corruption. Hence we are reserving 0x200 bytes
 *                to give room for any loading data. NOTE: This has to be the
 *                first member always
 * signature     - Signature of ACR ucode.
 * wpr_region_id - Region ID holding the WPR header and its details
 * wpr_offset    - Offset from the WPR region holding the wpr header
 * regions       - Region descriptors
 * nonwpr_ucode_blob_start -stores non-WPR start where kernel stores ucode blob
 * nonwpr_ucode_blob_end   -stores non-WPR end where kernel stores ucode blob
 */
struct flcn_acr_desc_v1 {
	union {
		u32 reserved_dmem[(LSF_BOOTSTRAP_OWNER_RESERVED_DMEM_SIZE/4)];
	} ucode_reserved_space;
	u32 signatures[4];
	/*Always 1st*/
	u32 wpr_region_id;
	u32 wpr_offset;
	u32 mmu_mem_range;
	struct flcn_acr_regions_v1 regions;
	u32 nonwpr_ucode_blob_size;
	u64 nonwpr_ucode_blob_start;
	u32 dummy[4];  /* ACR_BSI_VPR_DESC */
};

struct bin_hdr {
	/* 0x10de */
	u32 bin_magic;
	/* versioning of bin format */
	u32 bin_ver;
	/* Entire image size including this header */
	u32 bin_size;
	/*
	 * Header offset of executable binary metadata,
	 * start @ offset- 0x100 *
	 */
	u32 header_offset;
	/*
	 * Start of executable binary data, start @
	 * offset- 0x200
	 */
	u32 data_offset;
	/* Size of executable binary */
	u32 data_size;
};

struct hs_flcn_bl {
	const char *bl_fw_name;
	struct nvgpu_firmware *hs_bl_fw;
	struct hsflcn_bl_desc *hs_bl_desc;
	struct bin_hdr *hs_bl_bin_hdr;
	struct nvgpu_mem hs_bl_ucode;
};

struct acr_fw_header {
	u32 sig_dbg_offset;
	u32 sig_dbg_size;
	u32 sig_prod_offset;
	u32 sig_prod_size;
	u32 patch_loc;
	u32 patch_sig;
	u32 hdr_offset; /* This header points to acr_ucode_header_t210_load */
	u32 hdr_size; /* Size of above header */
};

/* ACR Falcon descriptor's */
struct hs_acr {
#define ACR_DEFAULT	0U
#define ACR_AHESASC	1U
#define ACR_ASB		2U
	u32 acr_type;

	/* HS bootloader to validate & load ACR ucode */
	struct hs_flcn_bl acr_hs_bl;

	/* ACR ucode */
	const char *acr_fw_name;
	struct nvgpu_firmware *acr_fw;
	struct nvgpu_mem acr_ucode;

	union {
		struct flcn_bl_dmem_desc bl_dmem_desc;
		struct flcn_bl_dmem_desc_v1 bl_dmem_desc_v1;
	};

	void *ptr_bl_dmem_desc;
	u32 bl_dmem_desc_size;

	union{
		struct flcn_acr_desc *acr_dmem_desc;
		struct flcn_acr_desc_v1 *acr_dmem_desc_v1;
	};

	/* Falcon used to execute ACR ucode */
	struct nvgpu_falcon *acr_flcn;

	void (*acr_flcn_setup_boot_config)(struct gk20a *g);
	void (*report_acr_engine_bus_err_status)(struct gk20a *g,
		u32 bar0_status, u32 error_type);
	int (*acr_engine_bus_err_status)(struct gk20a *g, u32 *bar0_status,
		u32 *error_type);
	bool (*acr_validate_mem_integrity)(struct gk20a *g);
};

int nvgpu_acr_bootstrap_hs_ucode(struct gk20a *g, struct nvgpu_acr *acr,
	struct hs_acr *acr_desc);

#endif /* ACR_BOOTSTRAP_H */
