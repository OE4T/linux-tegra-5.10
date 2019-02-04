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

#include <nvgpu/engines.h>
#include <nvgpu/gk20a.h>

enum nvgpu_fifo_engine nvgpu_engine_enum_from_type(struct gk20a *g,
					u32 engine_type)
{
	enum nvgpu_fifo_engine ret = NVGPU_ENGINE_INVAL_GK20A;

	if ((g->ops.top.is_engine_gr != NULL) &&
					(g->ops.top.is_engine_ce != NULL)) {
		if (g->ops.top.is_engine_gr(g, engine_type)) {
			ret = NVGPU_ENGINE_GR_GK20A;
		} else if (g->ops.top.is_engine_ce(g, engine_type)) {
			/* Lets consider all the CE engine have separate
			 * runlist at this point. We can identify the
			 * NVGPU_ENGINE_GRCE_GK20A type CE using runlist_id
			 * comparsion logic with GR runlist_id in
			 * init_engine_info()
			 */
			ret = NVGPU_ENGINE_ASYNC_CE_GK20A;
		} else {
			ret = NVGPU_ENGINE_INVAL_GK20A;
		}
	}

	return ret;
}

struct fifo_engine_info_gk20a *nvgpu_engine_get_active_eng_info(
	struct gk20a *g, u32 engine_id)
{
	struct fifo_gk20a *f = NULL;
	u32 engine_id_idx;
	struct fifo_engine_info_gk20a *info = NULL;

	if (g == NULL) {
		return info;
	}

	f = &g->fifo;

	if (engine_id < f->max_engines) {
		for (engine_id_idx = 0; engine_id_idx < f->num_engines;
				++engine_id_idx) {
			if (engine_id ==
					f->active_engines_list[engine_id_idx]) {
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
		u32 engine_id[], u32 engine_id_sz,
		enum nvgpu_fifo_engine engine_enum)
{
	struct fifo_gk20a *f = NULL;
	u32 instance_cnt = 0;
	u32 engine_id_idx;
	u32 active_engine_id = 0;
	struct fifo_engine_info_gk20a *info = NULL;

	if ((g == NULL) || (engine_id_sz == 0U) ||
			(engine_enum == NVGPU_ENGINE_INVAL_GK20A)) {
		return instance_cnt;
	}

	f = &g->fifo;
	for (engine_id_idx = 0; engine_id_idx < f->num_engines;
			 ++engine_id_idx) {
		active_engine_id = f->active_engines_list[engine_id_idx];
		info = &f->engine_info[active_engine_id];

		if (info->engine_enum == engine_enum) {
			if (instance_cnt < engine_id_sz) {
				engine_id[instance_cnt] = active_engine_id;
				++instance_cnt;
			} else {
				nvgpu_log_info(g, "warning engine_id table sz is small %d",
					engine_id_sz);
			}
		}
	}
	return instance_cnt;
}

bool nvgpu_engine_check_valid_eng_id(struct gk20a *g, u32 engine_id)
{
	struct fifo_gk20a *f = NULL;
	u32 engine_id_idx;
	bool valid = false;

	if (g == NULL) {
		return valid;
	}

	f = &g->fifo;

	if (engine_id < f->max_engines) {
		for (engine_id_idx = 0; engine_id_idx < f->num_engines;
				++engine_id_idx) {
			if (engine_id == f->active_engines_list[engine_id_idx]) {
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

u32 nvgpu_engine_get_gr_eng_id(struct gk20a *g)
{
	u32 gr_engine_cnt = 0;
	u32 gr_engine_id = FIFO_INVAL_ENGINE_ID;

	/* Consider 1st available GR engine */
	gr_engine_cnt = nvgpu_engine_get_ids(g, &gr_engine_id,
			1, NVGPU_ENGINE_GR_GK20A);

	if (gr_engine_cnt == 0U) {
		nvgpu_err(g, "No GR engine available on this device!");
	}

	return gr_engine_id;
}

u32 nvgpu_engine_act_interrupt_mask(struct gk20a *g, u32 act_eng_id)
{
	struct fifo_engine_info_gk20a *engine_info = NULL;

	engine_info = nvgpu_engine_get_active_eng_info(g, act_eng_id);
	if (engine_info != NULL) {
		return engine_info->intr_mask;
	}

	return 0;
}

u32 nvgpu_engine_interrupt_mask(struct gk20a *g)
{
	u32 eng_intr_mask = 0;
	unsigned int i;
	u32 active_engine_id = 0;
	enum nvgpu_fifo_engine engine_enum = NVGPU_ENGINE_INVAL_GK20A;

	for (i = 0; i < g->fifo.num_engines; i++) {
		u32 intr_mask;
		active_engine_id = g->fifo.active_engines_list[i];
		intr_mask = g->fifo.engine_info[active_engine_id].intr_mask;
		engine_enum = g->fifo.engine_info[active_engine_id].engine_enum;
		if (((engine_enum == NVGPU_ENGINE_GRCE_GK20A) ||
		     (engine_enum == NVGPU_ENGINE_ASYNC_CE_GK20A)) &&
		    ((g->ops.ce2.isr_stall == NULL) ||
		     (g->ops.ce2.isr_nonstall == NULL))) {
				continue;
		}

		eng_intr_mask |= intr_mask;
	}

	return eng_intr_mask;
}

u32 nvgpu_engine_get_all_ce_eng_reset_mask(struct gk20a *g)
{
	u32 reset_mask = 0;
	enum nvgpu_fifo_engine engine_enum = NVGPU_ENGINE_INVAL_GK20A;
	struct fifo_gk20a *f = NULL;
	u32 engine_id_idx;
	struct fifo_engine_info_gk20a *engine_info;
	u32 active_engine_id = 0;

	if (g == NULL) {
		return reset_mask;
	}

	f = &g->fifo;

	for (engine_id_idx = 0; engine_id_idx < f->num_engines;
			++engine_id_idx) {
		active_engine_id = f->active_engines_list[engine_id_idx];
		engine_info = &f->engine_info[active_engine_id];
		engine_enum = engine_info->engine_enum;

		if ((engine_enum == NVGPU_ENGINE_GRCE_GK20A) ||
			(engine_enum == NVGPU_ENGINE_ASYNC_CE_GK20A)) {
				reset_mask |= engine_info->reset_mask;
		}
	}

	return reset_mask;
}