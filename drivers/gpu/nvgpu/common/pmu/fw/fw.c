/*
 * Copyright (c) 2017-2020, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/pmu.h>
#include <nvgpu/dma.h>
#include <nvgpu/log.h>
#include <nvgpu/bug.h>
#include <nvgpu/firmware.h>
#include <nvgpu/enabled.h>
#include <nvgpu/utils.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/nvgpu_init.h>
#include <nvgpu/pmu/allocator.h>
#include <nvgpu/pmu/fw.h>
#include <nvgpu/pmu/pmu_pg.h>

/* PMU NS UCODE IMG */
#define NVGPU_PMU_NS_UCODE_IMAGE	"gpmu_ucode.bin"
/* PMU SECURE UCODE IMG */
#define NVGPU_PMU_UCODE_IMAGE "gpmu_ucode_image.bin"
#define NVGPU_PMU_UCODE_DESC "gpmu_ucode_desc.bin"
#define NVGPU_PMU_UCODE_SIG "pmu_sig.bin"

void nvgpu_pmu_fw_get_cmd_line_args_offset(struct gk20a *g,
	u32 *args_offset)
{
	struct nvgpu_pmu *pmu = g->pmu;
	u32 dmem_size = 0;
	int err = 0;

	err = nvgpu_falcon_get_mem_size(pmu->flcn, MEM_DMEM, &dmem_size);
	if (err != 0) {
		nvgpu_err(g, "dmem size request failed");
		*args_offset = 0;
		return;
	}

	*args_offset = dmem_size - pmu->fw->ops.get_cmd_line_args_size(pmu);
}

void nvgpu_pmu_fw_state_change(struct gk20a *g, struct nvgpu_pmu *pmu,
	u32 pmu_state, bool post_change_event)
{
	nvgpu_pmu_dbg(g, "pmu_state - %d", pmu_state);

	nvgpu_smp_wmb();
	pmu->fw->state = pmu_state;

	/* Set a sticky flag to indicate PMU state exit */
	if (pmu_state == PMU_FW_STATE_EXIT) {
		pmu->pg->pg_init.state_destroy = true;
	}
	if (post_change_event) {
		if (g->can_elpg) {
			pmu->pg->pg_init.state_change = true;
			nvgpu_cond_signal_interruptible(&pmu->pg->pg_init.wq);
		}
	}
}

u32 nvgpu_pmu_get_fw_state(struct gk20a *g, struct nvgpu_pmu *pmu)
{
	u32 state = pmu->fw->state;
	nvgpu_smp_rmb();

	return state;
}

void nvgpu_pmu_set_fw_ready(struct gk20a *g, struct nvgpu_pmu *pmu,
	bool status)
{
	nvgpu_smp_wmb();
	pmu->fw->ready = status;
}

bool nvgpu_pmu_get_fw_ready(struct gk20a *g, struct nvgpu_pmu *pmu)
{
	bool state = pmu->fw->ready;
	nvgpu_smp_rmb();

	return state;
}

int nvgpu_pmu_wait_fw_ack_status(struct gk20a *g, struct nvgpu_pmu *pmu,
	u32 timeout_ms, void *var, u8 val)
{
	struct nvgpu_timeout timeout;
	int err;
	unsigned int delay = POLL_DELAY_MIN_US;

	err = nvgpu_timeout_init(g, &timeout, timeout_ms,
		NVGPU_TIMER_CPU_TIMER);
	if (err != 0) {
		nvgpu_err(g, "PMU wait timeout init failed.");
		return err;
	}

	do {
		nvgpu_rmb();

		if (nvgpu_can_busy(g) == 0) {
			/*
			 * Since the system is shutting down so we don't
			 * wait for the ACK from PMU.
			 * Set ACK received so that state machine is maintained
			 * properly and falcon stats are not dumped due to
			 * PMU command failure
			 */

			*(volatile u8 *)var = val;
			return 0;
		}

		if (g->ops.pmu.pmu_is_interrupted(pmu)) {
			g->ops.pmu.pmu_isr(g);
		}

		nvgpu_usleep_range(delay, delay * 2U);
		delay = min_t(u32, delay << 1, POLL_DELAY_MAX_US);

		/* Confirm ACK from PMU before timeout check */
		if (*(volatile u8 *)var == val) {
			return 0;
		}

	} while (nvgpu_timeout_expired(&timeout) == 0);

	return -ETIMEDOUT;
}

int nvgpu_pmu_wait_fw_ready(struct gk20a *g, struct nvgpu_pmu *pmu)
{
	int status = 0;

	status = nvgpu_pmu_wait_fw_ack_status(g, pmu,
				nvgpu_get_poll_timeout(g),
				&pmu->fw->ready, (u8)true);
	if (status != 0) {
		nvgpu_err(g, "PMU is not ready yet");
	}

	return status;
}

static void pmu_fw_release(struct gk20a *g, struct pmu_rtos_fw *rtos_fw)
{
	struct mm_gk20a *mm = &g->mm;
	struct vm_gk20a *vm = mm->pmu.vm;

	nvgpu_log_fn(g, " ");

	if (rtos_fw->fw_sig != NULL) {
		nvgpu_release_firmware(g, rtos_fw->fw_sig);
	}

	if (rtos_fw->fw_desc != NULL) {
		nvgpu_release_firmware(g, rtos_fw->fw_desc);
	}

	if (rtos_fw->fw_image != NULL) {
		nvgpu_release_firmware(g, rtos_fw->fw_image);
	}

	if (nvgpu_mem_is_valid(&rtos_fw->ucode)) {
		nvgpu_dma_unmap_free(vm, &rtos_fw->ucode);
	}
}

struct nvgpu_firmware *nvgpu_pmu_fw_sig_desc(struct gk20a *g,
	struct nvgpu_pmu *pmu)
{
	return pmu->fw->fw_sig;
}

struct nvgpu_firmware *nvgpu_pmu_fw_desc_desc(struct gk20a *g,
	struct nvgpu_pmu *pmu)
{
	return pmu->fw->fw_desc;
}

struct nvgpu_firmware *nvgpu_pmu_fw_image_desc(struct gk20a *g,
	struct nvgpu_pmu *pmu)
{
	return pmu->fw->fw_image;
}

static int pmu_fw_read_and_init_ops(struct gk20a *g, struct nvgpu_pmu *pmu,
	struct pmu_rtos_fw *rtos_fw)
{
	struct pmu_ucode_desc *desc;
	int err = 0;

	nvgpu_log_fn(g, " ");

	if (!nvgpu_is_enabled(g, NVGPU_SEC_PRIVSECURITY)) {
		/* non-secure PMU boot uocde */
		rtos_fw->fw_image = nvgpu_request_firmware(g,
				NVGPU_PMU_NS_UCODE_IMAGE, 0);
		if (rtos_fw->fw_image == NULL) {
			nvgpu_err(g,
				"failed to load non-secure pmu ucode!!");
			goto exit;
		}

		desc = (struct pmu_ucode_desc *)
			(void *)rtos_fw->fw_image->data;
	} else {
		/* secure boot ucodes's */
		nvgpu_pmu_dbg(g, "requesting PMU ucode image");
		rtos_fw->fw_image =
			nvgpu_request_firmware(g,
				NVGPU_PMU_UCODE_IMAGE, 0);
		if (rtos_fw->fw_image == NULL) {
			nvgpu_err(g, "failed to load pmu ucode!!");
			err = -ENOENT;
			goto exit;
		}

		nvgpu_pmu_dbg(g, "requesting PMU ucode desc");
		rtos_fw->fw_desc =
			nvgpu_request_firmware(g,
				NVGPU_PMU_UCODE_DESC, 0);
		if (rtos_fw->fw_desc == NULL) {
			nvgpu_err(g, "failed to load pmu ucode desc!!");
			err = -ENOENT;
			goto release_img_fw;
		}

		nvgpu_pmu_dbg(g, "requesting PMU ucode sign");
		rtos_fw->fw_sig =
			nvgpu_request_firmware(g,
				NVGPU_PMU_UCODE_SIG, 0);
		if (rtos_fw->fw_sig == NULL) {
			nvgpu_err(g, "failed to load pmu sig!!");
			err = -ENOENT;
			goto release_desc;
		}

		desc = (struct pmu_ucode_desc *)(void *)
					rtos_fw->fw_desc->data;
	}

	err = nvgpu_pmu_init_fw_ver_ops(g, pmu, desc->app_version);
	if (err != 0) {
		nvgpu_err(g, "failed to set function pointers");
		goto release_sig;
	}

	goto exit;

release_sig:
	nvgpu_release_firmware(g, rtos_fw->fw_sig);
release_desc:
	nvgpu_release_firmware(g, rtos_fw->fw_desc);
release_img_fw:
	nvgpu_release_firmware(g, rtos_fw->fw_image);
exit:
	return err;
}

int nvgpu_pmu_init_pmu_fw(struct gk20a *g, struct nvgpu_pmu *pmu,
	struct pmu_rtos_fw **rtos_fw_p)
{
	struct pmu_rtos_fw *rtos_fw = NULL;
	int err;

	if (*rtos_fw_p != NULL) {
		/* skip alloc/reinit for unrailgate sequence */
		nvgpu_pmu_dbg(g, "skip fw init for unrailgate sequence");
		return 0;
	}

	rtos_fw = (struct pmu_rtos_fw *)
		nvgpu_kzalloc(g, sizeof(struct pmu_rtos_fw));
	if (rtos_fw == NULL) {
		err = -ENOMEM;
		goto exit;
	}

	*rtos_fw_p = rtos_fw;

	err = pmu_fw_read_and_init_ops(g, pmu, rtos_fw);

exit:
	return err;
}

void nvgpu_pmu_fw_deinit(struct gk20a *g, struct nvgpu_pmu *pmu,
	struct pmu_rtos_fw *rtos_fw)
{
	nvgpu_log_fn(g, " ");

	if (rtos_fw == NULL) {
		return;
	}

	pmu_fw_release(g, rtos_fw);

	nvgpu_kfree(g, rtos_fw);
}
