/*
 * Copyright (c) 2020, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/riscv.h>
#include <nvgpu/pmu.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/acr.h>
#include <nvgpu/bug.h>
#include <nvgpu/soc.h>
#include <nvgpu/io.h>

#include "common/acr/acr_bootstrap.h"
#include "common/acr/acr_priv.h"

#define RISCV_BR_COMPLETION_TIMEOUT_NON_SILICON_MS   10000 /*in msec */
#define RISCV_BR_COMPLETION_TIMEOUT_SILICON_MS       100   /*in msec */

static void ga10b_riscv_release_firmware(struct gk20a *g, struct nvgpu_acr *acr)
{
        nvgpu_release_firmware(g, acr->acr_asc.manifest_fw);
        nvgpu_release_firmware(g, acr->acr_asc.code_fw);
        nvgpu_release_firmware(g, acr->acr_asc.data_fw);
}

static int ga10b_load_riscv_acr_ucodes(struct gk20a *g, struct hs_acr *acr)
{
	int err = 0;

	acr->manifest_fw = nvgpu_request_firmware(g,
					acr->acr_manifest_name,
					NVGPU_REQUEST_FIRMWARE_NO_WARN);
	if (acr->manifest_fw == NULL) {
		nvgpu_err(g, "%s ucode get fail for %s",
			acr->acr_manifest_name, g->name);
		return -ENOENT;
	}

	acr->code_fw = nvgpu_request_firmware(g,
					acr->acr_code_name,
					NVGPU_REQUEST_FIRMWARE_NO_WARN);
	if (acr->code_fw == NULL) {
		nvgpu_err(g, "%s ucode get fail for %s",
			acr->acr_code_name, g->name);
		nvgpu_release_firmware(g, acr->manifest_fw);
		return -ENOENT;
	}

	acr->data_fw = nvgpu_request_firmware(g,
					acr->acr_data_name,
					NVGPU_REQUEST_FIRMWARE_NO_WARN);
	if (acr->data_fw == NULL) {
		nvgpu_err(g, "%s ucode get fail for %s",
			acr->acr_data_name, g->name);
		nvgpu_release_firmware(g, acr->manifest_fw);
		nvgpu_release_firmware(g, acr->code_fw);
		return -ENOENT;
	}

	return err;
}

static bool nvgpu_acr_wait_for_riscv_brom_completion(struct nvgpu_falcon *flcn,
						signed int timeoutms)
{
	u32 reg = 0;

	do {
		reg = flcn->g->ops.falcon.get_brom_retcode(flcn);
		if (flcn->g->ops.falcon.check_brom_passed(reg)) {
			break;
		}

		if (timeoutms <= 0) {
			return false;
		}

		nvgpu_msleep(10);
                timeoutms -= 10;

	} while (true);

    return true;
}

int nvgpu_acr_bootstrap_hs_ucode_riscv(struct gk20a *g, struct nvgpu_acr *acr)
{
	int err = 0;
	bool brom_complete = false;
	u32 timeout = 0;
	u64 acr_sysmem_desc_addr = 0LL;

	err = ga10b_load_riscv_acr_ucodes(g, &acr->acr_asc);
	if (err !=0) {
		nvgpu_err(g, "RISCV ucode loading failed");
		return -EINVAL;
	}

	err = acr->patch_wpr_info_to_ucode(g, acr, &acr->acr_asc, false);
	if (err != 0) {
		nvgpu_err(g, "RISCV ucode patch wpr info failed");
		return err;
	}

	acr_sysmem_desc_addr = nvgpu_mem_get_addr(g,
				&acr->acr_asc.acr_falcon2_sysmem_desc);

	nvgpu_riscv_dump_brom_stats(acr->acr_asc.acr_flcn);

	nvgpu_riscv_hs_ucode_load_bootstrap(acr->acr_asc.acr_flcn,
						acr->acr_asc.manifest_fw,
						acr->acr_asc.code_fw,
						acr->acr_asc.data_fw,
						acr_sysmem_desc_addr);

	if (nvgpu_platform_is_silicon(g)) {
		timeout = RISCV_BR_COMPLETION_TIMEOUT_SILICON_MS;
	} else {
		timeout = RISCV_BR_COMPLETION_TIMEOUT_NON_SILICON_MS;
	}
	brom_complete = nvgpu_acr_wait_for_riscv_brom_completion(
			acr->acr_asc.acr_flcn, timeout);

	nvgpu_riscv_dump_brom_stats(acr->acr_asc.acr_flcn);

	if (brom_complete == false) {
		nvgpu_err(g, "RISCV BROM timed out, limit: %d ms", timeout);
		err = -ETIMEDOUT;
	} else {
		nvgpu_info(g, "RISCV BROM passed");
	}

	/* wait for complete & halt */
	if (nvgpu_platform_is_silicon(g)) {
		timeout = ACR_COMPLETION_TIMEOUT_SILICON_MS;
	} else {
		timeout = ACR_COMPLETION_TIMEOUT_NON_SILICON_MS;
	}
	err = nvgpu_acr_wait_for_completion(g, &acr->acr_asc, timeout);

	ga10b_riscv_release_firmware(g, acr);

	return err;
}
