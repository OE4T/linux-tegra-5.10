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

#include <nvgpu/types.h>
#include <nvgpu/dma.h>
#include <nvgpu/timers.h>
#include <nvgpu/nvgpu_mem.h>
#include <nvgpu/firmware.h>
#include <nvgpu/pmu.h>
#include <nvgpu/falcon.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/acr.h>
#include <nvgpu/bug.h>

#include "acr_bootstrap.h"
#include "acr_priv.h"

static int acr_wait_for_completion(struct gk20a *g,
	struct nvgpu_falcon *flcn, unsigned int timeout)
{
	u32 flcn_id = nvgpu_falcon_get_id(flcn);
#ifdef CONFIG_NVGPU_FALCON_NON_FUSA
	u32 sctl, cpuctl;
#endif
	int completion = 0;
	u32 data = 0;
	u32 bar0_status = 0;
	u32 error_type;

	nvgpu_log_fn(g, " ");

	completion = nvgpu_falcon_wait_for_halt(flcn, timeout);
	if (completion != 0) {
		nvgpu_err(g, "flcn-%d: HS ucode boot timed out", flcn_id);
#ifdef CONFIG_NVGPU_FALCON_DEBUG
		nvgpu_falcon_dump_stats(flcn);
#endif
		error_type = ACR_BOOT_TIMEDOUT;
		goto exit;
	}

	if (g->acr->acr.acr_engine_bus_err_status != NULL) {
		completion = g->acr->acr.acr_engine_bus_err_status(g,
			&bar0_status, &error_type);
		if (completion != 0) {
			nvgpu_err(g, "flcn-%d: ACR engine bus error", flcn_id);
			goto exit;
		}
	}

	nvgpu_acr_dbg(g, "flcn-%d: HS ucode capabilities %x", flcn_id,
		nvgpu_falcon_mailbox_read(flcn, FALCON_MAILBOX_1));

	data = nvgpu_falcon_mailbox_read(flcn, FALCON_MAILBOX_0);
	if (data != 0U) {
		nvgpu_err(g, "flcn-%d: HS ucode boot failed, err %x", flcn_id,
			data);
		completion = -EAGAIN;
		error_type = ACR_BOOT_FAILED;
		goto exit;
	}

#ifdef CONFIG_NVGPU_FALCON_NON_FUSA
	nvgpu_falcon_get_ctls(flcn, &sctl, &cpuctl);

	nvgpu_acr_dbg(g, "flcn-%d: sctl reg %x cpuctl reg %x",
			flcn_id, sctl, cpuctl);
#endif

	/*
	 * When engine-falcon is used for ACR bootstrap, validate the integrity
	 * of falcon IMEM and DMEM.
	 */
	if (g->acr->acr.acr_validate_mem_integrity != NULL) {
		if (!g->acr->acr.acr_validate_mem_integrity(g)) {
			nvgpu_err(g, "flcn-%d: memcheck failed", flcn_id);
			completion = -EAGAIN;
			error_type = ACR_BOOT_FAILED;
		}
	}
exit:
	if (completion != 0) {
		if (g->acr->acr.report_acr_engine_bus_err_status != NULL) {
			g->acr->acr.report_acr_engine_bus_err_status(g,
				bar0_status, error_type);
		}
	}
	return completion;
}

/*
 * Patch signatures into ucode image
 */
static int acr_ucode_patch_sig(struct gk20a *g,
	unsigned int *p_img, unsigned int *p_prod_sig,
	unsigned int *p_dbg_sig, unsigned int *p_patch_loc,
	unsigned int *p_patch_ind, u32 sig_size)
{
	unsigned int i, j, *p_sig;
	nvgpu_acr_dbg(g, " ");

	if (!g->ops.pmu.is_debug_mode_enabled(g)) {
		p_sig = p_prod_sig;
		nvgpu_acr_dbg(g, "PRODUCTION MODE\n");
	} else {
		p_sig = p_dbg_sig;
		nvgpu_acr_dbg(g, "DEBUG MODE\n");
	}

	/* Patching logic:*/
	sig_size = sig_size / 4U;
	for (i = 0U; i < sizeof(*p_patch_loc)>>2U; i++) {
		for (j = 0U; j < sig_size; j++) {
			p_img[nvgpu_safe_add_u32((p_patch_loc[i]>>2U), j)] =
				p_sig[nvgpu_safe_add_u32((p_patch_ind[i]<<2U), j)];
		}
	}

	return 0;
}

/*
 * Loads ACR bin to SYSMEM/FB and bootstraps ACR with bootloader code
 * start and end are addresses of ucode blob in non-WPR region
 */
int nvgpu_acr_bootstrap_hs_ucode(struct gk20a *g, struct nvgpu_acr *acr,
	struct hs_acr *acr_desc)
{
	struct nvgpu_firmware *acr_fw = acr_desc->acr_fw;
	int status = 0;

	nvgpu_acr_dbg(g, "ACR TYPE %x ", acr_desc->acr_type);

	if (acr_fw != NULL) {
		acr->patch_wpr_info_to_ucode(g, acr, acr_desc, true);
	} else {
		acr_fw = nvgpu_request_firmware(g, acr_desc->acr_fw_name,
				NVGPU_REQUEST_FIRMWARE_NO_SOC);
		if (acr_fw == NULL) {
			nvgpu_err(g, "%s ucode get fail for %s",
				acr_desc->acr_fw_name, g->name);
			return -ENOENT;
		}

		acr_desc->acr_fw = acr_fw;

		acr->patch_wpr_info_to_ucode(g, acr, acr_desc, false);
	}

	/* Load acr ucode and bootstrap */
	status = nvgpu_acr_self_hs_load_bootstrap(g, acr_desc->acr_flcn, acr_fw,
					ACR_COMPLETION_TIMEOUT_MS);
	if (status != 0) {
		goto err_free_ucode;
	}

	return 0;
err_free_ucode:
	nvgpu_release_firmware(g, acr_fw);
	acr_desc->acr_fw = NULL;
	return status;
}

int nvgpu_acr_self_hs_load_bootstrap(struct gk20a *g, struct nvgpu_falcon *flcn,
		struct nvgpu_firmware *hs_fw, u32 timeout)
{
	struct bin_hdr *hs_bin_hdr = NULL;
	struct acr_fw_header *fw_hdr = NULL;
	u32 *ucode_header = NULL;
	u32 *ucode = NULL;
	int err = 0;

	hs_bin_hdr = (struct bin_hdr *)(void *)hs_fw->data;
	fw_hdr = (struct acr_fw_header *)(void *)(hs_fw->data +
				hs_bin_hdr->header_offset);
	ucode_header = (u32 *)(void *)(hs_fw->data + fw_hdr->hdr_offset);
	ucode = (u32 *)(void *)(hs_fw->data + hs_bin_hdr->data_offset);

	/* Patch Ucode signatures */
	if (acr_ucode_patch_sig(g, ucode,
		(u32 *)(void *)(hs_fw->data + fw_hdr->sig_prod_offset),
		(u32 *)(void *)(hs_fw->data + fw_hdr->sig_dbg_offset),
		(u32 *)(void *)(hs_fw->data + fw_hdr->patch_loc),
		(u32 *)(void *)(hs_fw->data + fw_hdr->patch_sig),
		fw_hdr->sig_dbg_size) < 0) {
		nvgpu_err(g, "HS ucode patch signatures fail");
		err = -EPERM;
		goto exit;
	}

	err = nvgpu_falcon_hs_ucode_load_bootstrap(flcn, ucode, ucode_header);
	if (err != 0) {
		nvgpu_err(g, "HS ucode load & bootstrap failed");
		goto exit;
	}

	/* wait for complete & halt */
	err = acr_wait_for_completion(g, flcn, timeout);
	if (err != 0) {
		nvgpu_err(g, "HS ucode completion err %d", err);
	}

exit:
	return err;
}

