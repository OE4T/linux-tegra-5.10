/*
 * Copyright (c) 2019-2021, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/soc.h>

#include "acr_bootstrap.h"
#include "acr_priv.h"

int nvgpu_acr_wait_for_completion(struct gk20a *g, struct hs_acr *acr_desc,
	u32 timeout)
{
	u32 flcn_id;
#ifdef CONFIG_NVGPU_FALCON_NON_FUSA
	u32 sctl, cpuctl;
#endif
	int completion = 0;
	u32 data = 0;
	u32 bar0_status = 0;
	u32 error_type;

	nvgpu_log_fn(g, " ");

	flcn_id = nvgpu_falcon_get_id(acr_desc->acr_flcn);

	completion = nvgpu_falcon_wait_for_halt(acr_desc->acr_flcn, timeout);
	if (completion != 0) {
		nvgpu_err(g, "flcn-%d: HS ucode boot timed out, limit: %d ms",
				flcn_id, timeout);
		error_type = ACR_BOOT_TIMEDOUT;
		goto exit;
	}

	if (acr_desc->acr_engine_bus_err_status != NULL) {
		completion = acr_desc->acr_engine_bus_err_status(g,
			&bar0_status, &error_type);
		if (completion != 0) {
			nvgpu_err(g, "flcn-%d: ACR engine bus error", flcn_id);
			goto exit;
		}
	}

	data = nvgpu_falcon_mailbox_read(acr_desc->acr_flcn, FALCON_MAILBOX_0);
	if (data != 0U) {
		nvgpu_err(g, "flcn-%d: HS ucode boot failed, err %x", flcn_id,
				data);
		nvgpu_err(g, "flcn-%d: Mailbox-1 : 0x%x", flcn_id,
				nvgpu_falcon_mailbox_read(acr_desc->acr_flcn,
				FALCON_MAILBOX_1));
		completion = -EAGAIN;
		error_type = ACR_BOOT_FAILED;
		goto exit;
	}

	/*
	 * When engine-falcon is used for ACR bootstrap, validate the integrity
	 * of falcon IMEM and DMEM.
	 */
	if (acr_desc->acr_validate_mem_integrity != NULL) {
		if (!acr_desc->acr_validate_mem_integrity(g)) {
			nvgpu_err(g, "flcn-%d: memcheck failed", flcn_id);
			completion = -EAGAIN;
			error_type = ACR_BOOT_FAILED;
		}
	}

exit:

#ifdef CONFIG_NVGPU_FALCON_NON_FUSA
	nvgpu_falcon_get_ctls(acr_desc->acr_flcn, &sctl, &cpuctl);

	nvgpu_info(g, "flcn-%d: sctl reg %x cpuctl reg %x",
			flcn_id, sctl, cpuctl);
#endif

	if (completion != 0) {
#ifdef CONFIG_NVGPU_FALCON_DEBUG
		nvgpu_falcon_dump_stats(acr_desc->acr_flcn);
#endif
		if (acr_desc->report_acr_engine_bus_err_status != NULL) {
			acr_desc->report_acr_engine_bus_err_status(g,
				bar0_status, error_type);
		}
	}

	return completion;
}

/*
 * Patch signatures into ucode image
 */
static void acr_ucode_patch_sig(struct gk20a *g,
	unsigned int *p_img, unsigned int *p_prod_sig,
	unsigned int *p_dbg_sig, unsigned int *p_patch_loc,
	unsigned int *p_patch_ind, u32 sig_size)
{
#if defined(CONFIG_NVGPU_NEXT)
	struct nvgpu_acr *acr = g->acr;
#endif
	unsigned int i, j, *p_sig;
	const u32 dmem_word_size = 4U;
	nvgpu_acr_dbg(g, " ");

	if (!g->ops.pmu.is_debug_mode_enabled(g)) {
		p_sig = p_prod_sig;
		nvgpu_acr_dbg(g, "PRODUCTION MODE\n");
	} else {
		p_sig = p_dbg_sig;
		nvgpu_info(g, "DEBUG MODE\n");
	}

#if defined(CONFIG_NVGPU_NEXT)
	if (acr->get_versioned_sig != NULL) {
		p_sig = acr->get_versioned_sig(g, acr, p_sig, &sig_size);
	}
#endif

	/* Patching logic:*/
	sig_size = sig_size / dmem_word_size;
	for (i = 0U; i < (sizeof(*p_patch_loc) / dmem_word_size); i++) {
		for (j = 0U; j < sig_size; j++) {
			p_img[nvgpu_safe_add_u32(
				(p_patch_loc[i] / dmem_word_size), j)] =
				p_sig[nvgpu_safe_add_u32(
					(p_patch_ind[i] * dmem_word_size), j)];
		}
	}
}

/*
 * Loads ACR bin to SYSMEM/FB and bootstraps ACR with bootloader code
 * start and end are addresses of ucode blob in non-WPR region
 */
int nvgpu_acr_bootstrap_hs_ucode(struct gk20a *g, struct nvgpu_acr *acr,
	struct hs_acr *acr_desc)
{
	struct nvgpu_firmware *acr_fw = acr_desc->acr_fw;
	struct bin_hdr *hs_bin_hdr = NULL;
	struct acr_fw_header *fw_hdr = NULL;
	u32 *ucode_header = NULL;
	u32 *ucode = NULL;
	u32 timeout = 0;
	int err = 0;

	nvgpu_acr_dbg(g, "ACR TYPE %x ", acr_desc->acr_type);

	if (acr_fw != NULL) {
		err = acr->patch_wpr_info_to_ucode(g, acr, acr_desc, true);
		if (err != 0) {
			nvgpu_err(g, "Falcon ucode patch wpr info failed");
			return err;
                }
	} else {
		/* Firmware is stored in soc specific path in FMODEL
		 * Hence NVGPU_REQUEST_FIRMWARE_NO_WARN is used instead
		 * of NVGPU_REQUEST_FIRMWARE_NO_SOC
		 */
#ifdef CONFIG_NVGPU_SIM
		if (nvgpu_is_enabled(g, NVGPU_IS_FMODEL)) {
			acr_fw = nvgpu_request_firmware(g,
					acr_desc->acr_fw_name,
					NVGPU_REQUEST_FIRMWARE_NO_WARN);
		} else
#endif
		{
			acr_fw = nvgpu_request_firmware(g,
					acr_desc->acr_fw_name,
					NVGPU_REQUEST_FIRMWARE_NO_SOC);
		}
		if (acr_fw == NULL) {
			nvgpu_err(g, "%s ucode get fail for %s",
				acr_desc->acr_fw_name, g->name);
			return -ENOENT;
		}

		acr_desc->acr_fw = acr_fw;

		err = acr->patch_wpr_info_to_ucode(g, acr, acr_desc, false);
		if (err != 0) {
			nvgpu_err(g, "Falcon ucode patch wpr info failed");
			goto err_free_ucode;
                }
	}


	hs_bin_hdr = (struct bin_hdr *)(void *)acr_fw->data;
	fw_hdr = (struct acr_fw_header *)(void *)(acr_fw->data +
			hs_bin_hdr->header_offset);
	ucode_header = (u32 *)(void *)(acr_fw->data + fw_hdr->hdr_offset);
	ucode = (u32 *)(void *)(acr_fw->data + hs_bin_hdr->data_offset);

	/* Patch Ucode signatures */
	acr_ucode_patch_sig(g, ucode,
		(u32 *)(void *)(acr_fw->data + fw_hdr->sig_prod_offset),
		(u32 *)(void *)(acr_fw->data + fw_hdr->sig_dbg_offset),
		(u32 *)(void *)(acr_fw->data + fw_hdr->patch_loc),
		(u32 *)(void *)(acr_fw->data + fw_hdr->patch_sig),
		fw_hdr->sig_dbg_size);

	err = nvgpu_falcon_hs_ucode_load_bootstrap(acr_desc->acr_flcn,
			ucode, ucode_header);
	if (err != 0) {
		nvgpu_err(g, "HS ucode load & bootstrap failed");
		goto err_free_ucode;
	}

	/* wait for complete & halt */
	if (nvgpu_platform_is_silicon(g)) {
		timeout = ACR_COMPLETION_TIMEOUT_SILICON_MS;
	} else {
		timeout = ACR_COMPLETION_TIMEOUT_NON_SILICON_MS;
	}
	err = nvgpu_acr_wait_for_completion(g, acr_desc, timeout);

	if (err != 0) {
		nvgpu_err(g, "HS ucode completion err %d", err);
		goto err_free_ucode;
	}

	return 0;

err_free_ucode:
	nvgpu_release_firmware(g, acr_fw);
	acr_desc->acr_fw = NULL;
	return err;
}
