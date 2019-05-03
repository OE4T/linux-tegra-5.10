/*
 * Copyright (c) 2017-2019, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/pmu/pmu_pg.h>
#include <nvgpu/dma.h>
#include <nvgpu/pmu/mutex.h>
#include <nvgpu/pmu/seq.h>
#include <nvgpu/log.h>
#include <nvgpu/pmu/pmuif/nvgpu_cmdif.h>
#include <nvgpu/enabled.h>
#include <nvgpu/engine_queue.h>
#include <nvgpu/barrier.h>
#include <nvgpu/timers.h>
#include <nvgpu/bug.h>
#include <nvgpu/utils.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/string.h>
#include <nvgpu/power_features/cg.h>
#include <nvgpu/nvgpu_err.h>
#include <nvgpu/pmu/lsfm.h>
#include <nvgpu/sec2/lsfm.h>
#include <nvgpu/pmu/super_surface.h>
#include <nvgpu/pmu/pmu_perfmon.h>
#include <nvgpu/pmu/pmu_pg.h>
#include <nvgpu/pmu/fw.h>
#include <nvgpu/firmware.h>
#include <nvgpu/pmu/debug.h>
#include <nvgpu/boardobj.h>
#include <nvgpu/boardobjgrp.h>

/* PMU locks used to sync with PMU-RTOS */
int nvgpu_pmu_lock_acquire(struct gk20a *g, struct nvgpu_pmu *pmu,
			u32 id, u32 *token)
{
	if (!g->support_ls_pmu) {
		return 0;
	}

	if (!g->can_elpg) {
		return 0;
	}

	if (!pmu->pg->initialized) {
		return -EINVAL;
	}

	return nvgpu_pmu_mutex_acquire(g, pmu->mutexes, id, token);
}

int nvgpu_pmu_lock_release(struct gk20a *g, struct nvgpu_pmu *pmu,
			u32 id, u32 *token)
{
	if (!g->support_ls_pmu) {
		return 0;
	}

	if (!g->can_elpg) {
		return 0;
	}

	if (!pmu->pg->initialized) {
		return -EINVAL;
	}

	return nvgpu_pmu_mutex_release(g, pmu->mutexes, id, token);
}

/* PMU RTOS init/setup functions */
int nvgpu_pmu_destroy(struct gk20a *g, struct nvgpu_pmu *pmu)
{
	nvgpu_log_fn(g, " ");

	if (!g->support_ls_pmu) {
		return 0;
	}

	if (g->can_elpg) {
		nvgpu_pmu_pg_destroy(g, pmu, pmu->pg);
	}

	nvgpu_mutex_acquire(&pmu->isr_mutex);
	g->ops.pmu.pmu_enable_irq(pmu, false);
	pmu->isr_enabled = false;
	nvgpu_mutex_release(&pmu->isr_mutex);

	nvgpu_pmu_queues_free(g, &pmu->queues);

	nvgpu_pmu_fw_state_change(g, pmu, PMU_FW_STATE_OFF, false);
	nvgpu_pmu_set_fw_ready(g, pmu, false);
	pmu->pmu_perfmon->perfmon_ready = false;

	nvgpu_set_enabled(g, NVGPU_PMU_FECS_BOOTSTRAP_DONE, false);

	nvgpu_log_fn(g, "done");
	return 0;
}

static void remove_pmu_support(struct nvgpu_pmu *pmu)
{
	struct gk20a *g = pmu->g;
	struct boardobj *pboardobj, *pboardobj_tmp;
	struct boardobjgrp *pboardobjgrp, *pboardobjgrp_tmp;
	int err = 0;

	nvgpu_log_fn(g, " ");

	if (nvgpu_alloc_initialized(&pmu->dmem)) {
		nvgpu_alloc_destroy(&pmu->dmem);
	}

	if (nvgpu_is_enabled(g, NVGPU_PMU_PSTATE)) {
		nvgpu_list_for_each_entry_safe(pboardobjgrp,
			pboardobjgrp_tmp, &g->boardobjgrp_head,
			boardobjgrp, node) {
				err = pboardobjgrp->destruct(pboardobjgrp);
				if (err != 0) {
					nvgpu_err(g, "pboardobjgrp destruct failed");
				}
		}

		nvgpu_list_for_each_entry_safe(pboardobj, pboardobj_tmp,
			&g->boardobj_head, boardobj, node) {
				pboardobj->destruct(pboardobj);
		}
	}

	if (nvgpu_is_enabled(g, NVGPU_SUPPORT_PMU_SUPER_SURFACE)) {
		nvgpu_pmu_super_surface_deinit(g, pmu, pmu->super_surface);
	}

	nvgpu_pmu_debug_deinit(g, pmu);
	nvgpu_pmu_lsfm_deinit(g, pmu, pmu->lsfm);
	nvgpu_pmu_pg_deinit(g, pmu, pmu->pg);
	nvgpu_pmu_sequences_deinit(g, pmu, pmu->sequences);
	nvgpu_pmu_mutexe_deinit(g, pmu, pmu->mutexes);
	nvgpu_pmu_fw_release(g, pmu);
	nvgpu_pmu_deinitialize_perfmon(g, pmu);
	nvgpu_mutex_destroy(&pmu->isr_mutex);
}

static int pmu_sw_setup(struct gk20a *g, struct nvgpu_pmu *pmu )
{
	int err = 0;

	nvgpu_log_fn(g, " ");

	/* set default value to mutexes */
	nvgpu_pmu_mutex_sw_setup(g, pmu, pmu->mutexes);

	/* set default value to sequences */
	nvgpu_pmu_sequences_sw_setup(g, pmu, pmu->sequences);

	if (g->can_elpg) {
		err = nvgpu_pmu_pg_sw_setup(g, pmu, pmu->pg);
		if (err != 0){
			goto exit;
		}
	}

	if (pmu->sw_ready) {
		nvgpu_log_fn(g, "skip PMU-RTOS shared buffer realloc");
		goto exit;
	}

	/* alloc shared buffer to read PMU-RTOS debug message */
	err = nvgpu_pmu_debug_init(g, pmu);
	if (err != 0) {
		goto exit;
	}

	/* alloc shared buffer super buffer to communicate with PMU-RTOS */
	if (nvgpu_is_enabled(g, NVGPU_SUPPORT_PMU_SUPER_SURFACE)) {
		err = nvgpu_pmu_super_surface_buf_alloc(g,
				pmu, pmu->super_surface);
		if (err != 0) {
			goto exit;
		}
	}

	pmu->sw_ready = true;
exit:
	if (err != 0) {
		remove_pmu_support(pmu);
	}

	return err;
}

int nvgpu_pmu_init(struct gk20a *g, struct nvgpu_pmu *pmu)
{
	int err = 0;

	nvgpu_log_fn(g, " ");

	if (!g->support_ls_pmu) {
		goto exit;
	}

	err = pmu_sw_setup(g, pmu);
	if (err != 0) {
		goto exit;
	}

	if (nvgpu_is_enabled(g, NVGPU_SEC_PRIVSECURITY)) {

		if (nvgpu_is_enabled(g, NVGPU_SUPPORT_SEC2_RTOS)) {
			/* Reset PMU engine */
			err = nvgpu_falcon_reset(&g->pmu.flcn);

			/* Bootstrap PMU from SEC2 RTOS*/
			err = nvgpu_sec2_bootstrap_ls_falcons(g, &g->sec2,
				FALCON_ID_PMU);
			if (err != 0) {
				goto exit;
			}
		}

		/*
		 * clear halt interrupt to avoid PMU-RTOS ucode
		 * hitting breakpoint due to PMU halt
		 */
		err = nvgpu_falcon_clear_halt_intr_status(&g->pmu.flcn,
			nvgpu_get_poll_timeout(g));
		if (err != 0) {
			goto exit;
		}

		if (g->ops.pmu.setup_apertures != NULL) {
			g->ops.pmu.setup_apertures(g);
		}

		err = nvgpu_pmu_lsfm_ls_pmu_cmdline_args_copy(g, pmu,
			pmu->lsfm);
		if (err != 0) {
			goto exit;
		}

		if (g->ops.pmu.pmu_enable_irq != NULL) {
			nvgpu_mutex_acquire(&g->pmu.isr_mutex);
			g->ops.pmu.pmu_enable_irq(&g->pmu, true);
			g->pmu.isr_enabled = true;
			nvgpu_mutex_release(&g->pmu.isr_mutex);
		}

		/*Once in LS mode, cpuctl_alias is only accessible*/
		if (g->ops.pmu.secured_pmu_start != NULL) {
			g->ops.pmu.secured_pmu_start(g);
		}
	} else {
		/* non-secure boot */
		err = nvgpu_pmu_ns_fw_bootstrap(g, pmu);
		if (err != 0) {
			goto exit;
		}
	}

	nvgpu_pmu_fw_state_change(g, pmu, PMU_FW_STATE_STARTING, false);

exit:
	return err;
}

int nvgpu_pmu_early_init(struct gk20a *g, struct nvgpu_pmu *pmu)
{
	int err = 0;

	nvgpu_log_fn(g, " ");

	pmu->g = g;

	if (!g->support_ls_pmu) {
		goto exit;
	}

	if (!g->ops.pmu.is_pmu_supported(g)) {
		g->support_ls_pmu = false;

		/* Disable LS PMU global checkers */
		g->can_elpg = false;
		g->elpg_enabled = false;
		g->aelpg_enabled = false;
		nvgpu_set_enabled(g, NVGPU_PMU_PERFMON, false);
		goto exit;
	}

	err = nvgpu_mutex_init(&pmu->isr_mutex);
	if (err != 0) {
		goto init_failed;
	}

	/* Allocate memory for pmu_perfmon */
	err = nvgpu_pmu_initialize_perfmon(g, pmu, &pmu->pmu_perfmon);
	if (err != 0) {
		goto exit;
	}

	err = nvgpu_pmu_init_pmu_fw(g, pmu);
	if (err != 0) {
		goto init_failed;
	}

	err = nvgpu_pmu_init_mutexe(g, pmu, &pmu->mutexes);
	if (err != 0) {
		goto init_failed;
	}

	err = nvgpu_pmu_sequences_init(g, pmu, &pmu->sequences);
	if (err != 0) {
		goto init_failed;
	}

	if (g->can_elpg) {
		err = nvgpu_pmu_pg_init(g, pmu, &pmu->pg);
		if (err != 0) {
			goto init_failed;
		}
	}

	err = nvgpu_pmu_lsfm_init(g, &pmu->lsfm);
	if (err != 0) {
		goto init_failed;
	}

	if (nvgpu_is_enabled(g, NVGPU_SUPPORT_PMU_SUPER_SURFACE)) {
		err = nvgpu_pmu_super_surface_init(g, pmu,
				&pmu->super_surface);
		if (err != 0) {
			goto init_failed;
		}
	}

	pmu->remove_support = remove_pmu_support;

	goto exit;

init_failed:
	remove_pmu_support(pmu);
exit:
	return err;
}

/* PMU H/W error functions */
static void pmu_report_error(struct gk20a *g, u32 err_type,
		u32 status, u32 pmu_err_type)
{
	int ret = 0;

	if (g->ops.pmu.err_ops.report_pmu_err != NULL) {
		ret = g->ops.pmu.err_ops.report_pmu_err(g,
			NVGPU_ERR_MODULE_PWR, err_type, status, pmu_err_type);
		if (ret != 0) {
			nvgpu_err(g, "Failed to report PMU error: %d",
					err_type);
		}
	}
}

void nvgpu_pmu_report_bar0_pri_err_status(struct gk20a *g, u32 bar0_status,
	u32 error_type)
{
	pmu_report_error(g,
		GPU_PMU_BAR0_ERROR_TIMEOUT, bar0_status, error_type);
	return;
}

/* PMU engine reset functions */
static int pmu_enable_hw(struct nvgpu_pmu *pmu, bool enable)
{
	struct gk20a *g = pmu->g;
	int err = 0;

	nvgpu_log_fn(g, " %s ", g->name);

	if (enable) {
		/* bring PMU falcon/engine out of reset */
		g->ops.pmu.reset_engine(g, true);

		nvgpu_cg_slcg_pmu_load_enable(g);

		nvgpu_cg_blcg_pmu_load_enable(g);

		if (nvgpu_falcon_mem_scrub_wait(&pmu->flcn) != 0) {
			/* keep PMU falcon/engine in reset
			 * if IMEM/DMEM scrubbing fails
			 */
			g->ops.pmu.reset_engine(g, false);
			nvgpu_err(g, "Falcon mem scrubbing timeout");
			err = -ETIMEDOUT;
		}
	} else {
		/* keep PMU falcon/engine in reset */
		g->ops.pmu.reset_engine(g, false);
	}

	nvgpu_log_fn(g, "%s Done, status - %d ", g->name, err);
	return err;
}

static int pmu_enable(struct nvgpu_pmu *pmu, bool enable)
{
	struct gk20a *g = pmu->g;
	int err = 0;

	nvgpu_log_fn(g, " ");

	if (!enable) {
		if (!g->ops.pmu.is_engine_in_reset(g)) {
			g->ops.pmu.pmu_enable_irq(pmu, false);
			pmu_enable_hw(pmu, false);
		}
	} else {
		err = pmu_enable_hw(pmu, true);
		if (err != 0) {
			goto exit;
		}

		err = nvgpu_falcon_wait_idle(&pmu->flcn);
		if (err != 0) {
			goto exit;
		}
	}

exit:
	nvgpu_log_fn(g, "Done, status - %d ", err);
	return err;
}

int nvgpu_pmu_reset(struct gk20a *g)
{
	struct nvgpu_pmu *pmu = &g->pmu;
	int err = 0;

	nvgpu_log_fn(g, " %s ", g->name);

	err = nvgpu_falcon_wait_idle(&pmu->flcn);
	if (err != 0) {
		goto exit;
	}

	err = pmu_enable(pmu, false);
	if (err != 0) {
		goto exit;
	}

	err = pmu_enable(pmu, true);

exit:
	nvgpu_log_fn(g, " %s Done, status - %d ", g->name, err);
	return err;
}

