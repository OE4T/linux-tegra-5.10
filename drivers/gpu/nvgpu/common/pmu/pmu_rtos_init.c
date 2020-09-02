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

#include <nvgpu/gk20a.h>
#include <nvgpu/log.h>
#include <nvgpu/enabled.h>
#include <nvgpu/bug.h>
#include <nvgpu/utils.h>
#include <nvgpu/power_features/cg.h>
#include <nvgpu/nvgpu_err.h>
#include <nvgpu/boardobjgrp.h>
#include <nvgpu/pmu.h>

#include <nvgpu/pmu/mutex.h>
#include <nvgpu/pmu/seq.h>
#include <nvgpu/pmu/lsfm.h>
#include <nvgpu/pmu/super_surface.h>
#include <nvgpu/pmu/pmu_perfmon.h>
#include <nvgpu/pmu/fw.h>
#include <nvgpu/pmu/debug.h>
#include <nvgpu/pmu/pmu_pstate.h>

#include "boardobj/boardobj.h"

#ifdef CONFIG_NVGPU_POWER_PG
#include <nvgpu/pmu/pmu_pg.h>
#endif

#ifdef CONFIG_NVGPU_DGPU
#include <nvgpu/sec2/lsfm.h>
#endif

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

#ifdef CONFIG_NVGPU_POWER_PG
	if (!pmu->pg->initialized) {
		return -EINVAL;
	}
#endif

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

#ifdef CONFIG_PMU_POWER_PG
	if (!pmu->pg->initialized) {
		return -EINVAL;
	}
#endif

	return nvgpu_pmu_mutex_release(g, pmu->mutexes, id, token);
}

/* PMU RTOS init/setup functions */
int nvgpu_pmu_destroy(struct gk20a *g, struct nvgpu_pmu *pmu)
{
	nvgpu_log_fn(g, " ");

#ifdef CONFIG_NVGPU_POWER_PG
	if (g->can_elpg) {
		nvgpu_pmu_pg_destroy(g, pmu, pmu->pg);
	}
#endif

	nvgpu_pmu_queues_free(g, &pmu->queues);

	nvgpu_pmu_fw_state_change(g, pmu, PMU_FW_STATE_OFF, false);
	nvgpu_pmu_set_fw_ready(g, pmu, false);
	nvgpu_pmu_lsfm_clean(g, pmu, pmu->lsfm);
	pmu->pmu_perfmon->perfmon_ready = false;


	nvgpu_log_fn(g, "done");
	return 0;
}

static void remove_pmu_support(struct nvgpu_pmu *pmu)
{
	struct gk20a *g = pmu->g;
	struct pmu_board_obj *obj, *obj_tmp;
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

		nvgpu_list_for_each_entry_safe(obj, obj_tmp,
			&g->boardobj_head, boardobj, node) {
				obj->destruct(obj);
		}
	}

	if (nvgpu_is_enabled(g, NVGPU_SUPPORT_PMU_SUPER_SURFACE)) {
		nvgpu_pmu_super_surface_deinit(g, pmu, pmu->super_surface);
	}

	if (nvgpu_is_enabled(g, NVGPU_PMU_PSTATE)) {
		nvgpu_pmu_pstate_deinit(g);
	}

	nvgpu_pmu_debug_deinit(g, pmu);
	nvgpu_pmu_lsfm_deinit(g, pmu, pmu->lsfm);
#ifdef CONFIG_PMU_POWER_PG
	nvgpu_pmu_pg_deinit(g, pmu, pmu->pg);
#endif
	nvgpu_pmu_sequences_deinit(g, pmu, pmu->sequences);
	nvgpu_pmu_mutexe_deinit(g, pmu, pmu->mutexes);
	nvgpu_pmu_fw_deinit(g, pmu, pmu->fw);
	nvgpu_pmu_deinitialize_perfmon(g, pmu);
}

static int pmu_sw_setup(struct gk20a *g, struct nvgpu_pmu *pmu )
{
	int err = 0;

	nvgpu_log_fn(g, " ");

	/* set default value to mutexes */
	nvgpu_pmu_mutex_sw_setup(g, pmu, pmu->mutexes);

	/* set default value to sequences */
	nvgpu_pmu_sequences_sw_setup(g, pmu, pmu->sequences);

#ifdef CONFIG_NVGPU_POWER_PG
	if (g->can_elpg) {
		err = nvgpu_pmu_pg_sw_setup(g, pmu, pmu->pg);
		if (err != 0){
			goto exit;
		}
	}
#endif

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

int nvgpu_pmu_rtos_init(struct gk20a *g)
{
	int err = 0;

	nvgpu_log_fn(g, " ");

	if (!g->support_ls_pmu || (g->pmu == NULL)) {
		goto exit;
	}

	err = pmu_sw_setup(g, g->pmu);
	if (err != 0) {
		goto exit;
	}

	if (nvgpu_is_enabled(g, NVGPU_SEC_PRIVSECURITY)) {
#ifdef CONFIG_NVGPU_DGPU
		if (nvgpu_is_enabled(g, NVGPU_SUPPORT_SEC2_RTOS)) {
			/* Reset PMU engine */
			err = nvgpu_falcon_reset(g->pmu->flcn);

			/* Bootstrap PMU from SEC2 RTOS*/
			err = nvgpu_sec2_bootstrap_ls_falcons(g, &g->sec2,
				FALCON_ID_PMU);
			if (err != 0) {
				goto exit;
			}
		}
#endif
		/*
		 * clear halt interrupt to avoid PMU-RTOS ucode
		 * hitting breakpoint due to PMU halt
		 */
		err = nvgpu_falcon_clear_halt_intr_status(g->pmu->flcn,
			nvgpu_get_poll_timeout(g));
		if (err != 0) {
			goto exit;
		}

		if (g->ops.pmu.setup_apertures != NULL) {
			g->ops.pmu.setup_apertures(g);
		}

		err = nvgpu_pmu_lsfm_ls_pmu_cmdline_args_copy(g, g->pmu,
			g->pmu->lsfm);
		if (err != 0) {
			goto exit;
		}

		nvgpu_pmu_enable_irq(g, true);

		/*Once in LS mode, cpuctl_alias is only accessible*/
		if (g->ops.pmu.secured_pmu_start != NULL) {
			g->ops.pmu.secured_pmu_start(g);
		}
	} else {
		/* non-secure boot */
		err = nvgpu_pmu_ns_fw_bootstrap(g, g->pmu);
		if (err != 0) {
			goto exit;
		}
	}

	nvgpu_pmu_fw_state_change(g, g->pmu, PMU_FW_STATE_STARTING, false);

exit:
	return err;
}

int nvgpu_pmu_rtos_early_init(struct gk20a *g, struct nvgpu_pmu *pmu)
{
	int err = 0;

	nvgpu_log_fn(g, " ");

	/* Allocate memory for pmu_perfmon */
	err = nvgpu_pmu_initialize_perfmon(g, pmu, &pmu->pmu_perfmon);
	if (err != 0) {
		goto exit;
	}

	err = nvgpu_pmu_init_pmu_fw(g, pmu, &pmu->fw);
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

#ifdef CONFIG_NVGPU_POWER_PG
	if (g->can_elpg) {
		err = nvgpu_pmu_pg_init(g, pmu, &pmu->pg);
		if (err != 0) {
			goto init_failed;
		}
	}
#endif

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
