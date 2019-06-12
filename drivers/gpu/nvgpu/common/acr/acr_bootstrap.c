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

#include "acr_falcon_bl.h"
#include "acr_bootstrap.h"
#include "acr_priv.h"

struct vm_gk20a* acr_get_engine_vm(struct gk20a *g, u32 falcon_id);

static int acr_wait_for_completion(struct gk20a *g,
	struct nvgpu_falcon *flcn, unsigned int timeout)
{
	u32 flcn_id = nvgpu_falcon_get_id(flcn);
	u32 sctl, cpuctl;
	int completion = 0;
	u32 data = 0;
	u32 bar0_status = 0;
	u32 error_type;

	nvgpu_log_fn(g, " ");

	completion = nvgpu_falcon_wait_for_halt(flcn, timeout);
	if (completion != 0) {
		nvgpu_err(g, "flcn-%d: HS ucode boot timed out", flcn_id);
		nvgpu_falcon_dump_stats(flcn);
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

	nvgpu_falcon_get_ctls(flcn, &sctl, &cpuctl);

	nvgpu_acr_dbg(g, "flcn-%d: sctl reg %x cpuctl reg %x",
			flcn_id, sctl, cpuctl);

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

struct vm_gk20a* acr_get_engine_vm(struct gk20a *g, u32 falcon_id)
{
	struct vm_gk20a *vm = NULL;

	switch (falcon_id) {
	case FALCON_ID_PMU:
		vm = g->mm.pmu.vm;
		break;
#ifdef CONFIG_NVGPU_DGPU
	case FALCON_ID_SEC2:
		if (nvgpu_is_enabled(g, NVGPU_SUPPORT_SEC2_VM)) {
			vm = g->mm.sec2.vm;
		}
		break;
	case FALCON_ID_GSPLITE:
		if (nvgpu_is_enabled(g, NVGPU_SUPPORT_GSP_VM)) {
			vm = g->mm.gsp.vm;
		}
		break;
#endif
	default:
		vm = NULL;
		break;
	}

	return vm;
}

static int acr_hs_bl_exec(struct gk20a *g, struct nvgpu_acr *acr,
	struct hs_acr *acr_desc, bool b_wait_for_halt)
{
	struct nvgpu_firmware *hs_bl_fw = acr_desc->acr_hs_bl.hs_bl_fw;
	struct hsflcn_bl_desc *hs_bl_desc;
	struct nvgpu_falcon_bl_info bl_info;
	struct hs_flcn_bl *hs_bl = &acr_desc->acr_hs_bl;
	struct vm_gk20a *vm = NULL;
	u32 flcn_id = nvgpu_falcon_get_id(acr_desc->acr_flcn);
	u32 *hs_bl_code = NULL;
	int err = 0;
	u32 bl_sz;

	nvgpu_acr_dbg(g, "Executing ACR HS Bootloader %s on Falcon-ID - %d",
		hs_bl->bl_fw_name, flcn_id);

	vm = acr_get_engine_vm(g, flcn_id);
	if (vm == NULL) {
		nvgpu_err(g, "vm space not allocated for engine falcon - %d", flcn_id);
		return -ENOMEM;
	}

	if (hs_bl_fw == NULL) {
		hs_bl_fw = nvgpu_request_firmware(g, hs_bl->bl_fw_name, 0);
		if (hs_bl_fw == NULL) {
			nvgpu_err(g, "ACR HS BL ucode load fail");
			return -ENOENT;
		}

		hs_bl->hs_bl_fw = hs_bl_fw;
		hs_bl->hs_bl_bin_hdr = (struct bin_hdr *)(void *)hs_bl_fw->data;

		hs_bl->hs_bl_desc = (struct hsflcn_bl_desc *)(void *)
			(hs_bl_fw->data + hs_bl->hs_bl_bin_hdr->header_offset);

		hs_bl_desc = hs_bl->hs_bl_desc;

		hs_bl_code = (u32 *)(void *)(hs_bl_fw->data +
			hs_bl->hs_bl_bin_hdr->data_offset);

		bl_sz = ALIGN(hs_bl_desc->bl_img_hdr.bl_code_size, 256U);

		hs_bl->hs_bl_ucode.size = bl_sz;

		err = nvgpu_dma_alloc_sys(g, bl_sz, &hs_bl->hs_bl_ucode);
		if (err != 0) {
			nvgpu_err(g, "ACR HS BL failed to allocate memory");
			goto err_done;
		}

		hs_bl->hs_bl_ucode.gpu_va = nvgpu_gmmu_map(vm,
			&hs_bl->hs_bl_ucode,
			bl_sz,
			0U, /* flags */
			gk20a_mem_flag_read_only, false,
			hs_bl->hs_bl_ucode.aperture);
		if (hs_bl->hs_bl_ucode.gpu_va == 0U) {
			nvgpu_err(g, "ACR HS BL failed to map ucode memory!!");
			goto err_free_ucode;
		}

		nvgpu_mem_wr_n(g, &hs_bl->hs_bl_ucode, 0U, hs_bl_code, bl_sz);

		nvgpu_acr_dbg(g, "Copied BL ucode to bl_cpuva");
	}

	/* Fill HS BL info */
	bl_info.bl_src = hs_bl->hs_bl_ucode.cpu_va;
	bl_info.bl_desc = acr_desc->ptr_bl_dmem_desc;
	nvgpu_assert(acr_desc->bl_dmem_desc_size <= U32_MAX);
	bl_info.bl_desc_size = (u32)acr_desc->bl_dmem_desc_size;
	nvgpu_assert(hs_bl->hs_bl_ucode.size <= U32_MAX);
	bl_info.bl_size = (u32)hs_bl->hs_bl_ucode.size;
	bl_info.bl_start_tag = hs_bl->hs_bl_desc->bl_start_tag;

	/* Engine falcon reset */
	err = nvgpu_falcon_reset(acr_desc->acr_flcn);
	if (err != 0) {
		goto err_unmap_bl;
	}

	/* setup falcon apertures, boot-config */
	acr_desc->acr_flcn_setup_boot_config(g);

	nvgpu_falcon_mailbox_write(acr_desc->acr_flcn, FALCON_MAILBOX_0,
		0xDEADA5A5U);

	/* bootstrap falcon */
	err = nvgpu_falcon_bl_bootstrap(acr_desc->acr_flcn, &bl_info);
	if (err != 0) {
		goto err_unmap_bl;
	}

	if (b_wait_for_halt) {
		/* wait for ACR halt*/
		err = acr_wait_for_completion(g, acr_desc->acr_flcn,
			ACR_COMPLETION_TIMEOUT_MS);
		if (err != 0) {
			goto err_unmap_bl;
		}
	}

	return 0;
err_unmap_bl:
	nvgpu_gmmu_unmap(vm, &hs_bl->hs_bl_ucode, hs_bl->hs_bl_ucode.gpu_va);
err_free_ucode:
	nvgpu_dma_free(g, &hs_bl->hs_bl_ucode);
err_done:
	nvgpu_release_firmware(g, hs_bl_fw);
	acr_desc->acr_hs_bl.hs_bl_fw = NULL;

	return err;
}

/*
 * Patch signatures into ucode image
 */
static int acr_ucode_patch_sig(struct gk20a *g,
	unsigned int *p_img, unsigned int *p_prod_sig,
	unsigned int *p_dbg_sig, unsigned int *p_patch_loc,
	unsigned int *p_patch_ind)
{
	unsigned int i, *p_sig;
	nvgpu_acr_dbg(g, " ");

	if (!g->ops.pmu.is_debug_mode_enabled(g)) {
		p_sig = p_prod_sig;
		nvgpu_acr_dbg(g, "PRODUCTION MODE\n");
	} else {
		p_sig = p_dbg_sig;
		nvgpu_acr_dbg(g, "DEBUG MODE\n");
	}

	/* Patching logic:*/
	for (i = 0U; i < sizeof(*p_patch_loc)>>2U; i++) {
		p_img[(p_patch_loc[i]>>2U)] = p_sig[(p_patch_ind[i]<<2U)];

		p_img[nvgpu_safe_add_u32((p_patch_loc[i]>>2U), 1U)] =
			p_sig[nvgpu_safe_add_u32((p_patch_ind[i]<<2U), 1U)];

		p_img[nvgpu_safe_add_u32((p_patch_loc[i]>>2U), 2U)] =
			p_sig[nvgpu_safe_add_u32((p_patch_ind[i]<<2U), 2U)];

		p_img[nvgpu_safe_add_u32((p_patch_loc[i]>>2U), 3U)] =
			p_sig[nvgpu_safe_add_u32((p_patch_ind[i]<<2U), 3U)];
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
	struct vm_gk20a *vm = NULL;
	struct nvgpu_firmware *acr_fw = acr_desc->acr_fw;
	struct bin_hdr *acr_fw_bin_hdr = NULL;
	struct acr_fw_header *acr_fw_hdr = NULL;
	struct nvgpu_mem *acr_ucode_mem = &acr_desc->acr_ucode;
	u32 flcn_id = nvgpu_falcon_get_id(acr_desc->acr_flcn);
	u32 img_size_in_bytes = 0;
	u32 *acr_ucode_data;
	u32 *acr_ucode_header;
	int status = 0;

	nvgpu_acr_dbg(g, "ACR TYPE %x ", acr_desc->acr_type);

	vm = acr_get_engine_vm(g, flcn_id);
	if (vm == NULL) {
		nvgpu_err(g, "vm space not allocated for engine falcon - %d",
				flcn_id);
		return -ENOMEM;
	}

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

		acr_fw_bin_hdr = (struct bin_hdr *)(void *)acr_fw->data;

		acr_fw_hdr = (struct acr_fw_header *)(void *)
			(acr_fw->data + acr_fw_bin_hdr->header_offset);

		acr_ucode_header = (u32 *)(void *)(acr_fw->data +
			acr_fw_hdr->hdr_offset);

		acr_ucode_data = (u32 *)(void *)(acr_fw->data +
			acr_fw_bin_hdr->data_offset);


		img_size_in_bytes = ALIGN((acr_fw_bin_hdr->data_size), 256U);

		/* Lets patch the signatures first.. */
		if (acr_ucode_patch_sig(g, acr_ucode_data,
			(u32 *)(void *)(acr_fw->data +
						acr_fw_hdr->sig_prod_offset),
			(u32 *)(void *)(acr_fw->data +
						acr_fw_hdr->sig_dbg_offset),
			(u32 *)(void *)(acr_fw->data +
						acr_fw_hdr->patch_loc),
			(u32 *)(void *)(acr_fw->data +
						acr_fw_hdr->patch_sig)) < 0) {
				nvgpu_err(g, "patch signatures fail");
				status = -1;
				goto err_release_acr_fw;
		}

		status = nvgpu_dma_alloc_map_sys(vm, img_size_in_bytes,
			acr_ucode_mem);
		if (status != 0) {
			status = -ENOMEM;
			goto err_release_acr_fw;
		}

		acr->patch_wpr_info_to_ucode(g, acr, acr_desc, false);

		nvgpu_mem_wr_n(g, acr_ucode_mem, 0U, acr_ucode_data,
			img_size_in_bytes);

		/*
		 * In order to execute this binary, we will be using
		 * a bootloader which will load this image into
		 * FALCON IMEM/DMEM.
		 * Fill up the bootloader descriptor to use..
		 * TODO: Use standard descriptor which the generic bootloader is
		 * checked in.
		 */
		acr->acr_fill_bl_dmem_desc(g, acr, acr_desc, acr_ucode_header);
	}

	status = acr_hs_bl_exec(g, acr, acr_desc, true);
	if (status != 0) {
		goto err_free_ucode_map;
	}

	return 0;
err_free_ucode_map:
	nvgpu_dma_unmap_free(vm, acr_ucode_mem);
err_release_acr_fw:
	nvgpu_release_firmware(g, acr_fw);
	acr_desc->acr_fw = NULL;
	return status;
}
#ifdef CONFIG_NVGPU_DGPU
int nvgpu_acr_self_hs_load_bootstrap(struct gk20a *g, struct nvgpu_falcon *flcn,
	struct nvgpu_firmware *hs_fw, u32 timeout)
{
	struct bin_hdr *bin_hdr = NULL;
	struct acr_fw_header *fw_hdr = NULL;
	u32 *ucode_header = NULL;
	u32 *ucode = NULL;
	u32 sec_imem_dest = 0U;
	int err = 0;

	/* falcon reset */
	err = nvgpu_falcon_reset(flcn);
	if (err != 0) {
		nvgpu_err(g, "nvgpu_falcon_reset() failed err=%d", err);
		return err;
	}

	bin_hdr = (struct bin_hdr *)hs_fw->data;
	fw_hdr = (struct acr_fw_header *)(hs_fw->data + bin_hdr->header_offset);
	ucode_header = (u32 *)(hs_fw->data + fw_hdr->hdr_offset);
	ucode = (u32 *)(hs_fw->data + bin_hdr->data_offset);

	/* Patch Ucode signatures */
	if (acr_ucode_patch_sig(g, ucode,
		(u32 *)(hs_fw->data + fw_hdr->sig_prod_offset),
		(u32 *)(hs_fw->data + fw_hdr->sig_dbg_offset),
		(u32 *)(hs_fw->data + fw_hdr->patch_loc),
		(u32 *)(hs_fw->data + fw_hdr->patch_sig)) < 0) {
		nvgpu_err(g, "HS ucode patch signatures fail");
		err = -EPERM;
		goto exit;
	}

	/* Clear interrupts */
	nvgpu_falcon_set_irq(flcn, false, 0x0U, 0x0U);

	/* Copy Non Secure IMEM code */
	err = nvgpu_falcon_copy_to_imem(flcn, 0U,
		(u8 *)&ucode[ucode_header[OS_CODE_OFFSET] >> 2U],
		ucode_header[OS_CODE_SIZE], 0U, false,
		GET_IMEM_TAG(ucode_header[OS_CODE_OFFSET]));
	if (err != 0) {
		nvgpu_err(g, "HS ucode non-secure code to IMEM failed");
		goto exit;
	}

	/* Put secure code after non-secure block */
	sec_imem_dest = GET_NEXT_BLOCK(ucode_header[OS_CODE_SIZE]);

	err = nvgpu_falcon_copy_to_imem(flcn, sec_imem_dest,
		(u8 *)&ucode[ucode_header[APP_0_CODE_OFFSET] >> 2U],
		ucode_header[APP_0_CODE_SIZE], 0U, true,
		GET_IMEM_TAG(ucode_header[APP_0_CODE_OFFSET]));
	if (err != 0) {
		nvgpu_err(g, "HS ucode secure code to IMEM failed");
		goto exit;
	}

	/* load DMEM: ensure that signatures are patched */
	err = nvgpu_falcon_copy_to_dmem(flcn, 0U, (u8 *)&ucode[
		ucode_header[OS_DATA_OFFSET] >> 2U],
		ucode_header[OS_DATA_SIZE], 0U);
	if (err != 0) {
		nvgpu_err(g, "HS ucode data copy to DMEM failed");
		goto exit;
	}

	/*
	 * Write non-zero value to mailbox register which is updated by
	 * HS bin to denote its return status.
	 */
	nvgpu_falcon_mailbox_write(flcn, FALCON_MAILBOX_0, 0xdeadbeefU);

	/* set BOOTVEC to start of non-secure code */
	err = nvgpu_falcon_bootstrap(flcn, 0U);
	if (err != 0) {
		nvgpu_err(g, "HS ucode bootstrap failed err-%d on falcon-%d", err,
			nvgpu_falcon_get_id(flcn));
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
#endif

