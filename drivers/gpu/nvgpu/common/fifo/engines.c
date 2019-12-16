/*
 * Copyright (c) 2019-2020, NVIDIA CORPORATION.  All rights reserved.
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


#include <nvgpu/log.h>
#include <nvgpu/errno.h>
#include <nvgpu/timers.h>
#include <nvgpu/bitops.h>
#ifdef CONFIG_NVGPU_LS_PMU
#include <nvgpu/pmu.h>
#include <nvgpu/pmu/mutex.h>
#endif
#include <nvgpu/runlist.h>
#include <nvgpu/engines.h>
#include <nvgpu/engine_status.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/pbdma_status.h>
#include <nvgpu/power_features/pg.h>
#include <nvgpu/channel.h>
#include <nvgpu/soc.h>
#include <nvgpu/top.h>
#include <nvgpu/gr/gr_falcon.h>
#include <nvgpu/gr/gr.h>
#include <nvgpu/fifo.h>
#include <nvgpu/static_analysis.h>
#include <nvgpu/gops_mc.h>

#define FECS_METHOD_WFI_RESTORE	0x80000U

enum nvgpu_fifo_engine nvgpu_engine_enum_from_type(struct gk20a *g,
					u32 engine_type)
{
	enum nvgpu_fifo_engine ret = NVGPU_ENGINE_INVAL;

	if ((g->ops.top.is_engine_gr != NULL) &&
					(g->ops.top.is_engine_ce != NULL)) {
		if (g->ops.top.is_engine_gr(g, engine_type)) {
			ret = NVGPU_ENGINE_GR;
		} else if (g->ops.top.is_engine_ce(g, engine_type)) {
			/* Lets consider all the CE engine have separate
			 * runlist at this point. We can identify the
			 * NVGPU_ENGINE_GRCE type CE using runlist_id
			 * comparsion logic with GR runlist_id in
			 * init_info()
			 */
			ret = NVGPU_ENGINE_ASYNC_CE;
		} else {
			ret = NVGPU_ENGINE_INVAL;
		}
	}

	return ret;
}

struct nvgpu_engine_info *nvgpu_engine_get_active_eng_info(
	struct gk20a *g, u32 engine_id)
{
	struct nvgpu_fifo *f = NULL;
	u32 i;
	struct nvgpu_engine_info *info = NULL;

	if (g == NULL) {
		return info;
	}

	f = &g->fifo;

	if (engine_id < f->max_engines) {
		for (i = 0U; i < f->num_engines; i++) {
			if (engine_id == f->active_engines_list[i]) {
				info = &f->engine_info[engine_id];
				break;
			}
		}
	}

	if (info == NULL) {
		nvgpu_err(g, "engine_id is not in active list/invalid %d",
			engine_id);
	}

	return info;
}

u32 nvgpu_engine_get_ids(struct gk20a *g,
		u32 *engine_ids, u32 engine_id_sz,
		enum nvgpu_fifo_engine engine_enum)
{
	struct nvgpu_fifo *f = NULL;
	u32 instance_cnt = 0;
	u32 i;
	u32 engine_id = 0;
	struct nvgpu_engine_info *info = NULL;

	if ((g == NULL) || (engine_id_sz == 0U) ||
			(engine_enum == NVGPU_ENGINE_INVAL)) {
		return instance_cnt;
	}

	f = &g->fifo;
	for (i = 0U; i < f->num_engines; i++) {
		engine_id = f->active_engines_list[i];
		info = &f->engine_info[engine_id];

		if (info->engine_enum == engine_enum) {
			if (instance_cnt < engine_id_sz) {
				engine_ids[instance_cnt] = engine_id;
				++instance_cnt;
			} else {
				nvgpu_log_info(g, "warning engine_id table sz is small %d",
					engine_id_sz);
			}
		}
	}
	return instance_cnt;
}

bool nvgpu_engine_check_valid_id(struct gk20a *g, u32 engine_id)
{
	struct nvgpu_fifo *f = NULL;
	u32 i;
	bool valid = false;

	if (g == NULL) {
		return valid;
	}

	f = &g->fifo;

	if (engine_id < f->max_engines) {
		for (i = 0U; i < f->num_engines; i++) {
			if (engine_id == f->active_engines_list[i]) {
				valid = true;
				break;
			}
		}
	}

	if (!valid) {
		nvgpu_err(g, "engine_id is not in active list/invalid %d",
			engine_id);
	}

	return valid;
}

u32 nvgpu_engine_get_gr_id(struct gk20a *g)
{
	u32 gr_engine_cnt = 0;
	u32 gr_engine_id = NVGPU_INVALID_ENG_ID;

	/* Consider 1st available GR engine */
	gr_engine_cnt = nvgpu_engine_get_ids(g, &gr_engine_id,
			1, NVGPU_ENGINE_GR);

	if (gr_engine_cnt == 0U) {
		nvgpu_err(g, "No GR engine available on this device!");
	}

	return gr_engine_id;
}

u32 nvgpu_engine_act_interrupt_mask(struct gk20a *g, u32 engine_id)
{
	struct nvgpu_engine_info *engine_info = NULL;

	engine_info = nvgpu_engine_get_active_eng_info(g, engine_id);
	if (engine_info != NULL) {
		return engine_info->intr_mask;
	}

	return 0;
}

u32 nvgpu_gr_engine_interrupt_mask(struct gk20a *g)
{
	u32 eng_intr_mask = 0;
	unsigned int i;
	u32 engine_id = 0;
	enum nvgpu_fifo_engine engine_enum;

	for (i = 0; i < g->fifo.num_engines; i++) {
		u32 intr_mask;

		engine_id = g->fifo.active_engines_list[i];
		intr_mask = g->fifo.engine_info[engine_id].intr_mask;
		engine_enum = g->fifo.engine_info[engine_id].engine_enum;

		if (engine_enum != NVGPU_ENGINE_GR) {
			continue;
		}

		eng_intr_mask |= intr_mask;
	}

	return eng_intr_mask;
}

u32 nvgpu_ce_engine_interrupt_mask(struct gk20a *g)
{
	u32 eng_intr_mask = 0;
	unsigned int i;
	u32 engine_id = 0;
	enum nvgpu_fifo_engine engine_enum;

	if ((g->ops.ce.isr_stall == NULL) ||
	    (g->ops.ce.isr_nonstall == NULL)) {
		return 0U;
	}

	for (i = 0; i < g->fifo.num_engines; i++) {
		u32 intr_mask;

		engine_id = g->fifo.active_engines_list[i];
		intr_mask = g->fifo.engine_info[engine_id].intr_mask;
		engine_enum = g->fifo.engine_info[engine_id].engine_enum;

		if ((engine_enum == NVGPU_ENGINE_GRCE) ||
		    (engine_enum == NVGPU_ENGINE_ASYNC_CE)) {
			eng_intr_mask |= intr_mask;
		}
	}

	return eng_intr_mask;
}

u32 nvgpu_engine_get_all_ce_reset_mask(struct gk20a *g)
{
	u32 reset_mask = 0;
	enum nvgpu_fifo_engine engine_enum;
	struct nvgpu_fifo *f = NULL;
	u32 i;
	struct nvgpu_engine_info *engine_info;
	u32 engine_id = 0;

	if (g == NULL) {
		return reset_mask;
	}

	f = &g->fifo;

	for (i = 0U; i < f->num_engines; i++) {
		engine_id = f->active_engines_list[i];
		engine_info = &f->engine_info[engine_id];
		engine_enum = engine_info->engine_enum;

		if ((engine_enum == NVGPU_ENGINE_GRCE) ||
			(engine_enum == NVGPU_ENGINE_ASYNC_CE)) {
				reset_mask |= engine_info->reset_mask;
		}
	}

	return reset_mask;
}

#ifdef CONFIG_NVGPU_FIFO_ENGINE_ACTIVITY

int nvgpu_engine_enable_activity(struct gk20a *g,
				struct nvgpu_engine_info *eng_info)
{
	nvgpu_log(g, gpu_dbg_info, "start");

	nvgpu_runlist_set_state(g, BIT32(eng_info->runlist_id),
			RUNLIST_ENABLED);
	return 0;
}

int nvgpu_engine_enable_activity_all(struct gk20a *g)
{
	unsigned int i;
	int err = 0, ret = 0;

	for (i = 0; i < g->fifo.num_engines; i++) {
		u32 engine_id = g->fifo.active_engines_list[i];
		err = nvgpu_engine_enable_activity(g,
				&g->fifo.engine_info[engine_id]);
		if (err != 0) {
			nvgpu_err(g,
				"failed to enable engine %d activity", engine_id);
			ret = err;
		}
	}

	return ret;
}

int nvgpu_engine_disable_activity(struct gk20a *g,
				struct nvgpu_engine_info *eng_info,
				bool wait_for_idle)
{
	u32 pbdma_chid = NVGPU_INVALID_CHANNEL_ID;
	u32 engine_chid = NVGPU_INVALID_CHANNEL_ID;
#ifdef CONFIG_NVGPU_LS_PMU
	u32 token = PMU_INVALID_MUTEX_OWNER_ID;
	int mutex_ret = -EINVAL;
#endif
	struct nvgpu_channel *ch = NULL;
	int err = 0;
	struct nvgpu_engine_status_info engine_status;
	struct nvgpu_pbdma_status_info pbdma_status;

	nvgpu_log_fn(g, " ");

	g->ops.engine_status.read_engine_status_info(g, eng_info->engine_id,
		 &engine_status);
	if (engine_status.is_busy && !wait_for_idle) {
		return -EBUSY;
	}

#ifdef CONFIG_NVGPU_LS_PMU
	if (g->ops.pmu.is_pmu_supported(g)) {
		mutex_ret = nvgpu_pmu_lock_acquire(g, g->pmu,
						PMU_MUTEX_ID_FIFO, &token);
	}
#endif

	nvgpu_runlist_set_state(g, BIT32(eng_info->runlist_id),
			RUNLIST_DISABLED);

	/* chid from pbdma status */
	g->ops.pbdma_status.read_pbdma_status_info(g, eng_info->pbdma_id,
		&pbdma_status);
	if (nvgpu_pbdma_status_is_chsw_valid(&pbdma_status) ||
			nvgpu_pbdma_status_is_chsw_save(&pbdma_status)) {
		pbdma_chid = pbdma_status.id;
	} else if (nvgpu_pbdma_status_is_chsw_load(&pbdma_status) ||
			nvgpu_pbdma_status_is_chsw_switch(&pbdma_status)) {
		pbdma_chid = pbdma_status.next_id;
	} else {
		/* Nothing to do here */
	}

	if (pbdma_chid != NVGPU_INVALID_CHANNEL_ID) {
		ch = nvgpu_channel_from_id(g, pbdma_chid);
		if (ch != NULL) {
			err = g->ops.fifo.preempt_channel(g, ch);
			nvgpu_channel_put(ch);
		}
		if (err != 0) {
			goto clean_up;
		}
	}

	/* chid from engine status */
	g->ops.engine_status.read_engine_status_info(g, eng_info->engine_id,
		 &engine_status);
	if (nvgpu_engine_status_is_ctxsw_valid(&engine_status) ||
	    nvgpu_engine_status_is_ctxsw_save(&engine_status)) {
		engine_chid = engine_status.ctx_id;
	} else if (nvgpu_engine_status_is_ctxsw_switch(&engine_status) ||
	    nvgpu_engine_status_is_ctxsw_load(&engine_status)) {
		engine_chid = engine_status.ctx_next_id;
	} else {
		/* Nothing to do here */
	}

	if (engine_chid != NVGPU_INVALID_ENG_ID && engine_chid != pbdma_chid) {
		ch = nvgpu_channel_from_id(g, engine_chid);
		if (ch != NULL) {
			err = g->ops.fifo.preempt_channel(g, ch);
			nvgpu_channel_put(ch);
		}
		if (err != 0) {
			goto clean_up;
		}
	}

clean_up:
#ifdef CONFIG_NVGPU_LS_PMU
	if (mutex_ret == 0) {
		if (nvgpu_pmu_lock_release(g, g->pmu,
			PMU_MUTEX_ID_FIFO, &token) != 0){
			nvgpu_err(g, "failed to release PMU lock");
		}
	}
#endif
	if (err != 0) {
		nvgpu_log_fn(g, "failed");
		if (nvgpu_engine_enable_activity(g, eng_info) != 0) {
			nvgpu_err(g,
				"failed to enable gr engine activity");
		}
	} else {
		nvgpu_log_fn(g, "done");
	}
	return err;
}

int nvgpu_engine_disable_activity_all(struct gk20a *g,
					   bool wait_for_idle)
{
	unsigned int i;
	int err = 0, ret = 0;
	u32 engine_id;

	for (i = 0; i < g->fifo.num_engines; i++) {
		engine_id = g->fifo.active_engines_list[i];
		err = nvgpu_engine_disable_activity(g,
				&g->fifo.engine_info[engine_id],
				wait_for_idle);
		if (err != 0) {
			nvgpu_err(g, "failed to disable engine %d activity",
				engine_id);
			ret = err;
			break;
		}
	}

	if (err != 0) {
		while (i-- != 0U) {
			engine_id = g->fifo.active_engines_list[i];
			err = nvgpu_engine_enable_activity(g,
					&g->fifo.engine_info[engine_id]);
			if (err != 0) {
				nvgpu_err(g,
					"failed to re-enable engine %d activity",
					engine_id);
			}
		}
	}

	return ret;
}

int nvgpu_engine_wait_for_idle(struct gk20a *g)
{
	struct nvgpu_timeout timeout;
	u32 delay = POLL_DELAY_MIN_US;
	int ret = 0, err = 0;
	u32 i, host_num_engines;
	struct nvgpu_engine_status_info engine_status;

	nvgpu_log_fn(g, " ");

	host_num_engines =
		 nvgpu_get_litter_value(g, GPU_LIT_HOST_NUM_ENGINES);

	err = nvgpu_timeout_init(g, &timeout, nvgpu_get_poll_timeout(g),
			   NVGPU_TIMER_CPU_TIMER);
	if (err != 0) {
		return -EINVAL;
	}

	for (i = 0; i < host_num_engines; i++) {
		if (!nvgpu_engine_check_valid_id(g, i)) {
			continue;
		}

		ret = -ETIMEDOUT;
		do {
			g->ops.engine_status.read_engine_status_info(g, i,
				&engine_status);
			if (!engine_status.is_busy) {
				ret = 0;
				break;
			}

			nvgpu_usleep_range(delay, delay * 2U);
			delay = min_t(u32,
					delay << 1, POLL_DELAY_MAX_US);
		} while (nvgpu_timeout_expired(&timeout) == 0);

		if (ret != 0) {
			/* possible causes:
			 * check register settings programmed in hal set by
			 * elcg_init_idle_filters and init_therm_setup_hw
			 */
			nvgpu_err(g, "cannot idle engine: %u "
					"engine_status: 0x%08x", i,
					engine_status.reg_data);
			break;
		}
	}

	nvgpu_log_fn(g, "done");

	return ret;
}

#endif /* CONFIG_NVGPU_FIFO_ENGINE_ACTIVITY */

int nvgpu_engine_setup_sw(struct gk20a *g)
{
	struct nvgpu_fifo *f = &g->fifo;
	int err = 0;
	size_t size;

	f->max_engines = nvgpu_get_litter_value(g, GPU_LIT_HOST_NUM_ENGINES);
	size = nvgpu_safe_mult_u64(f->max_engines, sizeof(*f->engine_info));
	f->engine_info = nvgpu_kzalloc(g, size);
	if (f->engine_info == NULL) {
		nvgpu_err(g, "no mem for engine info");
		return -ENOMEM;
	}

	size = nvgpu_safe_mult_u64(f->max_engines, sizeof(u32));
	f->active_engines_list = nvgpu_kzalloc(g, size);
	if (f->active_engines_list == NULL) {
		nvgpu_err(g, "no mem for active engine list");
		err = -ENOMEM;
		goto clean_up_engine_info;
	}
	(void) memset(f->active_engines_list, 0xff, size);

	err = g->ops.engine.init_info(f);
	if (err != 0) {
		nvgpu_err(g, "init engine info failed");
		goto clean_up;
	}

	return 0;

clean_up:
	nvgpu_kfree(g, f->active_engines_list);
	f->active_engines_list = NULL;

clean_up_engine_info:
	nvgpu_kfree(g, f->engine_info);
	f->engine_info = NULL;

	return err;
}

void nvgpu_engine_cleanup_sw(struct gk20a *g)
{
	struct nvgpu_fifo *f = &g->fifo;

	f->num_engines = 0;
	nvgpu_kfree(g, f->engine_info);
	f->engine_info = NULL;
	nvgpu_kfree(g, f->active_engines_list);
	f->active_engines_list = NULL;
}

#ifdef CONFIG_NVGPU_ENGINE_RESET
void nvgpu_engine_reset(struct gk20a *g, u32 engine_id)
{
	enum nvgpu_fifo_engine engine_enum = NVGPU_ENGINE_INVAL;
	struct nvgpu_engine_info *engine_info;

	nvgpu_log_fn(g, " ");

	if (g == NULL) {
		return;
	}

	engine_info = nvgpu_engine_get_active_eng_info(g, engine_id);

	if (engine_info != NULL) {
		engine_enum = engine_info->engine_enum;
	}

	if (engine_enum == NVGPU_ENGINE_INVAL) {
		nvgpu_err(g, "unsupported engine_id %d", engine_id);
	}

	if (engine_enum == NVGPU_ENGINE_GR) {
#ifdef CONFIG_NVGPU_POWER_PG
		if (nvgpu_pg_elpg_disable(g) != 0 ) {
			nvgpu_err(g, "failed to set disable elpg");
		}
#endif

#ifdef CONFIG_NVGPU_FECS_TRACE
		/*
		 * Resetting engine will alter read/write index. Need to flush
		 * circular buffer before re-enabling FECS.
		 */
		if (g->ops.gr.fecs_trace.reset != NULL) {
			if (g->ops.gr.fecs_trace.reset(g) != 0) {
				nvgpu_warn(g, "failed to reset fecs traces");
			}
		}
#endif
		if (!nvgpu_platform_is_simulation(g)) {
			int err = 0;

			/*HALT_PIPELINE method, halt GR engine*/
			err = g->ops.gr.falcon.ctrl_ctxsw(g,
				NVGPU_GR_FALCON_METHOD_HALT_PIPELINE, 0U, NULL);
			if (err != 0) {
				nvgpu_err(g, "failed to halt gr pipe");
			}

			/*
			 * resetting engine using mc_enable_r() is not
			 * enough, we do full init sequence
			 */
			nvgpu_log(g, gpu_dbg_info, "resetting gr engine");

			err = nvgpu_gr_reset(g);
			if (err != 0) {
				nvgpu_err(g, "failed to reset gr engine");
			}
		} else {
			nvgpu_log(g, gpu_dbg_info,
				"HALT gr pipe not supported and "
				"gr cannot be reset without halting gr pipe");
		}

#ifdef CONFIG_NVGPU_POWER_PG
		if (nvgpu_pg_elpg_enable(g) != 0 ) {
			nvgpu_err(g, "failed to set enable elpg");
		}
#endif
	}

	if ((engine_enum == NVGPU_ENGINE_GRCE) ||
		(engine_enum == NVGPU_ENGINE_ASYNC_CE)) {
			g->ops.mc.reset(g, engine_info->reset_mask);
	}
}
#endif

u32 nvgpu_engine_get_fast_ce_runlist_id(struct gk20a *g)
{
	u32 ce_runlist_id = nvgpu_engine_get_gr_runlist_id(g);
	enum nvgpu_fifo_engine engine_enum;
	struct nvgpu_fifo *f = NULL;
	u32 i;
	struct nvgpu_engine_info *engine_info;
	u32 engine_id = 0U;

	if (g == NULL) {
		return ce_runlist_id;
	}

	f = &g->fifo;

	for (i = 0U; i < f->num_engines; i++) {
		engine_id = f->active_engines_list[i];
		engine_info = &f->engine_info[engine_id];
		engine_enum = engine_info->engine_enum;

		/* select last available ASYNC_CE if available */
		if (engine_enum == NVGPU_ENGINE_ASYNC_CE) {
			ce_runlist_id = engine_info->runlist_id;
		}
	}

	return ce_runlist_id;
}

u32 nvgpu_engine_get_gr_runlist_id(struct gk20a *g)
{
	struct nvgpu_fifo *f = &g->fifo;
	u32 gr_engine_cnt = 0;
	u32 gr_engine_id = NVGPU_INVALID_ENG_ID;
	struct nvgpu_engine_info *engine_info;
	u32 gr_runlist_id = U32_MAX;

	/* Consider 1st available GR engine */
	gr_engine_cnt = nvgpu_engine_get_ids(g, &gr_engine_id,
			1, NVGPU_ENGINE_GR);

	if (gr_engine_cnt == 0U) {
		nvgpu_err(g,
			"No GR engine available on this device!");
		goto end;
	}

	engine_info = &f->engine_info[gr_engine_id];
	gr_runlist_id = engine_info->runlist_id;

end:
	return gr_runlist_id;
}

bool nvgpu_engine_is_valid_runlist_id(struct gk20a *g, u32 runlist_id)
{
	struct nvgpu_fifo *f = NULL;
	u32 i;
	u32 engine_id;
	struct nvgpu_engine_info *engine_info;

	if (g == NULL) {
		return false;
	}

	f = &g->fifo;

	for (i = 0U; i < f->num_engines; i++) {
		engine_id = f->active_engines_list[i];
		engine_info = &f->engine_info[engine_id];
		if (engine_info->runlist_id == runlist_id) {
			return true;
		}
	}

	return false;
}

/*
 * Link engine IDs to MMU IDs and vice versa.
 */
u32 nvgpu_engine_id_to_mmu_fault_id(struct gk20a *g, u32 engine_id)
{
	u32 fault_id = NVGPU_INVALID_ENG_ID;
	struct nvgpu_engine_info *engine_info;

	engine_info = nvgpu_engine_get_active_eng_info(g, engine_id);

	if (engine_info != NULL) {
		fault_id = engine_info->fault_id;
	} else {
		nvgpu_err(g, "engine_id: %d is not in active list/invalid",
			engine_id);
	}
	return fault_id;
}

u32 nvgpu_engine_mmu_fault_id_to_engine_id(struct gk20a *g, u32 fault_id)
{
	u32 i;
	u32 engine_id;
	struct nvgpu_engine_info *engine_info;
	struct nvgpu_fifo *f = &g->fifo;

	for (i = 0U; i < f->num_engines; i++) {
		engine_id = f->active_engines_list[i];
		engine_info = &g->fifo.engine_info[engine_id];

		if (engine_info->fault_id == fault_id) {
			break;
		}
		engine_id = NVGPU_INVALID_ENG_ID;
	}
	return engine_id;
}

u32 nvgpu_engine_get_mask_on_id(struct gk20a *g, u32 id, bool is_tsg)
{
	unsigned int i;
	u32 engines = 0;
	struct nvgpu_engine_status_info engine_status;
	u32 ctx_id;
	u32 type;
	bool busy;

	for (i = 0; i < g->fifo.num_engines; i++) {
		u32 engine_id = g->fifo.active_engines_list[i];

		g->ops.engine_status.read_engine_status_info(g,
			engine_id, &engine_status);

		if (nvgpu_engine_status_is_ctxsw_load(
			&engine_status)) {
			nvgpu_engine_status_get_next_ctx_id_type(
				&engine_status, &ctx_id, &type);
		} else {
			nvgpu_engine_status_get_ctx_id_type(
				&engine_status, &ctx_id, &type);
		}

		busy = engine_status.is_busy;

		if (busy && (ctx_id == id)) {
			if ((is_tsg && (type ==
					ENGINE_STATUS_CTX_ID_TYPE_TSGID)) ||
				(!is_tsg && (type ==
					ENGINE_STATUS_CTX_ID_TYPE_CHID))) {
				engines |= BIT32(engine_id);
			}
		}
	}

	return engines;
}

int nvgpu_engine_init_info(struct nvgpu_fifo *f)
{
	struct gk20a *g = f->g;
	int ret = 0;
	enum nvgpu_fifo_engine engine_enum;
	u32 pbdma_id = U32_MAX;
	bool found_pbdma_for_runlist = false;
	struct nvgpu_device_info dev_info;
	struct nvgpu_engine_info *info;

	f->num_engines = 0;
	if (g->ops.top.get_device_info == NULL) {
		nvgpu_err(g, "unable to parse dev_info table");
		return -EINVAL;
	}

	ret = g->ops.top.get_device_info(g, &dev_info,
			NVGPU_ENGINE_GRAPHICS, 0);
	if (ret != 0) {
		nvgpu_err(g,
			"Failed to parse dev_info table for engine %d",
			NVGPU_ENGINE_GRAPHICS);
		return -EINVAL;
	}

	found_pbdma_for_runlist = g->ops.pbdma.find_for_runlist(g,
						dev_info.runlist_id,
						&pbdma_id);
	if (!found_pbdma_for_runlist) {
		nvgpu_err(g, "busted pbdma map");
		return -EINVAL;
	}

	engine_enum = nvgpu_engine_enum_from_type(g, dev_info.engine_type);

	info = &g->fifo.engine_info[dev_info.engine_id];

	info->intr_mask |= BIT32(dev_info.intr_id);
	info->reset_mask |= BIT32(dev_info.reset_id);
	info->runlist_id = dev_info.runlist_id;
	info->pbdma_id = pbdma_id;
	info->inst_id  = dev_info.inst_id;
	info->pri_base = dev_info.pri_base;
	info->engine_enum = engine_enum;
	info->fault_id = dev_info.fault_id;

	/* engine_id starts from 0 to NV_HOST_NUM_ENGINES */
	f->active_engines_list[f->num_engines] = dev_info.engine_id;
	++f->num_engines;
	nvgpu_log_info(g,
		"gr info: engine_id %d runlist_id %d intr_id %d "
		"reset_id %d engine_type %d engine_enum %d inst_id %d",
		dev_info.engine_id,
		dev_info.runlist_id,
		dev_info.intr_id,
		dev_info.reset_id,
		dev_info.engine_type,
		engine_enum,
		dev_info.inst_id);

	ret = g->ops.engine.init_ce_info(f);

	return ret;
}

void nvgpu_engine_get_id_and_type(struct gk20a *g, u32 engine_id,
					  u32 *id, u32 *type)
{
	struct nvgpu_engine_status_info engine_status;

	g->ops.engine_status.read_engine_status_info(g, engine_id,
		&engine_status);

	/* use next_id if context load is failing */
	if (nvgpu_engine_status_is_ctxsw_load(
		&engine_status)) {
		nvgpu_engine_status_get_next_ctx_id_type(
			&engine_status, id, type);
	} else {
		nvgpu_engine_status_get_ctx_id_type(
			&engine_status, id, type);
	}
}

u32 nvgpu_engine_find_busy_doing_ctxsw(struct gk20a *g,
			u32 *id_ptr, bool *is_tsg_ptr)
{
	u32 i;
	u32 id = U32_MAX;
	bool is_tsg = false;
	u32 mailbox2;
	u32 engine_id = NVGPU_INVALID_ENG_ID;
	struct nvgpu_engine_status_info engine_status;

	for (i = 0U; i < g->fifo.num_engines; i++) {
		bool failing_engine;

		engine_id = g->fifo.active_engines_list[i];
		g->ops.engine_status.read_engine_status_info(g, engine_id,
			&engine_status);

		/* we are interested in busy engines */
		failing_engine = engine_status.is_busy;

		/* ..that are doing context switch */
		failing_engine = failing_engine &&
			nvgpu_engine_status_is_ctxsw(&engine_status);

		if (!failing_engine) {
			engine_id = NVGPU_INVALID_ENG_ID;
			continue;
		}

		if (nvgpu_engine_status_is_ctxsw_load(&engine_status)) {
			id = engine_status.ctx_next_id;
			is_tsg = nvgpu_engine_status_is_next_ctx_type_tsg(
					&engine_status);
		} else if (nvgpu_engine_status_is_ctxsw_switch(&engine_status)) {
			mailbox2 = g->ops.gr.falcon.read_fecs_ctxsw_mailbox(g,
					NVGPU_GR_FALCON_FECS_CTXSW_MAILBOX2);
			if ((mailbox2 & FECS_METHOD_WFI_RESTORE) != 0U) {
				id = engine_status.ctx_next_id;
				is_tsg = nvgpu_engine_status_is_next_ctx_type_tsg(
						&engine_status);
			} else {
				id = engine_status.ctx_id;
				is_tsg = nvgpu_engine_status_is_ctx_type_tsg(
						&engine_status);
			}
		} else {
			id = engine_status.ctx_id;
			is_tsg = nvgpu_engine_status_is_ctx_type_tsg(
					&engine_status);
		}
		break;
	}

	*id_ptr = id;
	*is_tsg_ptr = is_tsg;

	return engine_id;
}

u32 nvgpu_engine_get_runlist_busy_engines(struct gk20a *g, u32 runlist_id)
{
	struct nvgpu_fifo *f = &g->fifo;
	u32 i, eng_bitmask = 0U;
	struct nvgpu_engine_status_info engine_status;

	for (i = 0U; i < f->num_engines; i++) {
		u32 engine_id = f->active_engines_list[i];
		u32 engine_runlist = f->engine_info[engine_id].runlist_id;
		bool engine_busy;

		g->ops.engine_status.read_engine_status_info(g, engine_id,
			&engine_status);
		engine_busy = engine_status.is_busy;

		if (engine_busy && (engine_runlist == runlist_id)) {
			eng_bitmask |= BIT32(engine_id);
		}
	}

	return eng_bitmask;
}

#ifdef CONFIG_NVGPU_DEBUGGER
bool nvgpu_engine_should_defer_reset(struct gk20a *g, u32 engine_id,
		u32 engine_subid, bool fake_fault)
{
	enum nvgpu_fifo_engine engine_enum = NVGPU_ENGINE_INVAL;
	struct nvgpu_engine_info *engine_info;

	if (g == NULL) {
		return false;
	}

	engine_info = nvgpu_engine_get_active_eng_info(g, engine_id);

	if (engine_info != NULL) {
		engine_enum = engine_info->engine_enum;
	}

	if (engine_enum == NVGPU_ENGINE_INVAL) {
		return false;
	}

	/*
	 * channel recovery is only deferred if an sm debugger
	 * is attached and has MMU debug mode is enabled
	 */
	if (!g->ops.gr.sm_debugger_attached(g) ||
	    !g->ops.fb.is_debug_mode_enabled(g)) {
		return false;
	}

	/* if this fault is fake (due to RC recovery), don't defer recovery */
	if (fake_fault) {
		return false;
	}

	if (engine_enum != NVGPU_ENGINE_GR) {
		return false;
	}

	return g->ops.engine.is_fault_engine_subid_gpc(g, engine_subid);
}
#endif

u32 nvgpu_engine_mmu_fault_id_to_veid(struct gk20a *g, u32 mmu_fault_id,
			u32 gr_eng_fault_id)
{
	struct nvgpu_fifo *f = &g->fifo;
	u32 num_subctx;
	u32 veid = INVAL_ID;

	num_subctx = f->max_subctx_count;

	if ((mmu_fault_id >= gr_eng_fault_id) &&
		(mmu_fault_id < nvgpu_safe_add_u32(gr_eng_fault_id,
						num_subctx))) {
		veid = mmu_fault_id - gr_eng_fault_id;
	}

	return veid;
}

u32 nvgpu_engine_mmu_fault_id_to_eng_id_and_veid(struct gk20a *g,
			 u32 mmu_fault_id, u32 *veid)
{
	u32 i;
	u32 engine_id = INVAL_ID;
	struct nvgpu_engine_info *engine_info;
	struct nvgpu_fifo *f = &g->fifo;


	for (i = 0U; i < f->num_engines; i++) {
		engine_id = f->active_engines_list[i];
		engine_info = &g->fifo.engine_info[engine_id];

		if (engine_info->engine_enum == NVGPU_ENGINE_GR) {
			*veid = nvgpu_engine_mmu_fault_id_to_veid(g,
					mmu_fault_id, engine_info->fault_id);
			if (*veid != INVAL_ID) {
				break;
			}
		} else {
			if (engine_info->fault_id == mmu_fault_id) {
				*veid = INVAL_ID;
				break;
			}
		}
		engine_id = INVAL_ID;
	}
	return engine_id;
}

void nvgpu_engine_mmu_fault_id_to_eng_ve_pbdma_id(struct gk20a *g,
	u32 mmu_fault_id, u32 *engine_id, u32 *veid, u32 *pbdma_id)
{
	*engine_id = nvgpu_engine_mmu_fault_id_to_eng_id_and_veid(g,
				 mmu_fault_id, veid);

	if (*engine_id == INVAL_ID) {
		*pbdma_id = g->ops.fifo.mmu_fault_id_to_pbdma_id(g,
				mmu_fault_id);
	} else {
		*pbdma_id = INVAL_ID;
	}
}
