/*
 * GR MANAGER
 *
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
#include <nvgpu/enabled.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/grmgr.h>
#include <nvgpu/engines.h>
#include <nvgpu/device.h>

int nvgpu_init_gr_manager(struct gk20a *g)
{
	u32 gpc_id;
	struct nvgpu_gpu_instance *gpu_instance = &g->mig.gpu_instance[0];
	struct nvgpu_gr_syspipe *gr_syspipe = &gpu_instance->gr_syspipe;

	/* Number of gpu instance is 1 for legacy mode */
	g->mig.gpc_count = g->ops.priv_ring.get_gpc_count(g);
	g->mig.num_gpu_instances = 1U;
	g->mig.current_gpu_instance_config_id = 0U;
	g->mig.is_nongr_engine_sharable = false;

	gpu_instance->gpu_instance_id = 0U;
	gpu_instance->is_memory_partition_supported = false;

	gr_syspipe->gr_instance_id = 0U;
	gr_syspipe->gr_syspipe_id = 0U;
	gr_syspipe->num_gpc = g->mig.gpc_count;
	gr_syspipe->gr_dev = nvgpu_device_get(g, NVGPU_DEVTYPE_GRAPHICS, 0U);
	nvgpu_assert(gr_syspipe->gr_dev != NULL);

	g->mig.gpcgrp_gpc_count[0] = gr_syspipe->num_gpc;
	if (g->ops.gr.config.get_gpc_mask != NULL) {
		gr_syspipe->gpc_mask = g->ops.gr.config.get_gpc_mask(g);
	} else {
		gr_syspipe->gpc_mask = nvgpu_safe_sub_u32(
			BIT32(gr_syspipe->num_gpc),
			1U);
	}

	/* In Legacy mode, Local GPC Id = physical GPC Id = Logical GPC Id */
	for (gpc_id = 0U; gpc_id < gr_syspipe->num_gpc; gpc_id++) {
		gr_syspipe->gpcs[gpc_id].logical_id =
			gr_syspipe->gpcs[gpc_id].physical_id = gpc_id;
		gr_syspipe->gpcs[gpc_id].gpcgrp_id = 0U;
	}
	gr_syspipe->max_veid_count_per_tsg = g->fifo.max_subctx_count;
	gr_syspipe->veid_start_offset = 0U;

	gpu_instance->num_lce = nvgpu_device_get_copies(g, gpu_instance->lce_devs,
							NVGPU_MIG_MAX_ENGINES);
	nvgpu_assert(gpu_instance->num_lce > 0U);

	g->mig.max_gr_sys_pipes_supported = 1U;
	g->mig.gr_syspipe_en_mask = 1U;
	g->mig.num_gr_sys_pipes_enabled = 1U;

	g->mig.current_gr_syspipe_id = NVGPU_MIG_INVALID_GR_SYSPIPE_ID;

	nvgpu_log(g, gpu_dbg_mig,
		"[non MIG boot] gpu_instance_id[%u] gr_instance_id[%u] "
			"gr_syspipe_id[%u] num_gpc[%u] gr_engine_id[%u] "
			"max_veid_count_per_tsg[%u] veid_start_offset[%u] "
			"is_memory_partition_support[%d] num_lce[%u] ",
		gpu_instance->gpu_instance_id,
		gr_syspipe->gr_instance_id,
		gr_syspipe->gr_syspipe_id,
		gr_syspipe->num_gpc,
		gr_syspipe->gr_dev->engine_id,
		gr_syspipe->max_veid_count_per_tsg,
		gr_syspipe->veid_start_offset,
		gpu_instance->is_memory_partition_supported,
		gpu_instance->num_lce);

	return 0;
}

int nvgpu_grmgr_config_gr_remap_window(struct gk20a *g,
		u32 gr_syspipe_id, bool enable)
{
	int err = 0;
#if defined(CONFIG_NVGPU_NEXT) && defined(CONFIG_NVGPU_MIG)
	if (nvgpu_is_enabled(g, NVGPU_SUPPORT_MIG)) {
		/*
		 * GR remap window enable/disable sequence for a GR
		 * SYSPIPE PGRAPH programming:
		 * 1) Config_gr_remap_window (syspipe_index, enable).
		 * 2) Acquire gr_syspipe_lock.
		 * 3) HW write to enable the gr syspipe programming.
		 * 4) Return success.
		 * 5) Do GR programming belong to particular gr syspipe.
		 * 6) Config_gr_remap_window (syspipe_index, disable).
		 * 7) HW write to disable the gr syspipe programming.
		 * 8) Release the gr_syspipe_lock.
		 *
		 * GR remap window disable/enable request for legacy
		 * GR PGRAPH programming:
		 * 1) Config_gr_remap_window (invalid_syspipe_index, disable).
		 * 2) Acquire gr_syspipe_lock.
		 * 3) HW write to enable the legacy gr syspipe programming.
		 * 4) Return success.
		 * 5) Do legacy GR PGRAPH programming.
		 * 6) Config_gr_remap_window (invalid_syspipe_index, enable).
		 * 7) HW write to disable the legacy gr syspipe programming.
		 * 8) Release the gr_syspipe_lock.
		 */

		if ((gr_syspipe_id !=
				NVGPU_MIG_INVALID_GR_SYSPIPE_ID) &&
				((g->mig.usable_gr_syspipe_mask & BIT32(
					gr_syspipe_id)) == 0U)) {
			nvgpu_err(g, "Invalid param syspipe_id[%x] en_mask[%x]",
				gr_syspipe_id,
				g->mig.usable_gr_syspipe_mask);
			return -EINVAL;
		}

		if (enable && (g->mig.current_gr_syspipe_id ==
				NVGPU_MIG_INVALID_GR_SYSPIPE_ID) &&
				(gr_syspipe_id ==
				NVGPU_MIG_INVALID_GR_SYSPIPE_ID)) {
			nvgpu_warn(g,
				"Legacy GR PGRAPH window enable called before "
					"disable sequence call ");
			return -EPERM;
		}

		if (!enable && (gr_syspipe_id !=
				NVGPU_MIG_INVALID_GR_SYSPIPE_ID) &&
				(g->mig.current_gr_syspipe_id ==
				NVGPU_MIG_INVALID_GR_SYSPIPE_ID)) {
			nvgpu_warn(g,
				"Repeated GR remap window disable call[%x %x] ",
				gr_syspipe_id,
				g->mig.current_gr_syspipe_id);
			return -EPERM;
		}

		if (enable) {
			if (gr_syspipe_id !=
					NVGPU_MIG_INVALID_GR_SYSPIPE_ID) {
				nvgpu_mutex_acquire(&g->mig.gr_syspipe_lock);
			}
		} else {
			if (gr_syspipe_id ==
					NVGPU_MIG_INVALID_GR_SYSPIPE_ID) {
				nvgpu_mutex_acquire(&g->mig.gr_syspipe_lock);
			} else {
				gr_syspipe_id = 0U;
			}
		}

		nvgpu_log(g, gpu_dbg_mig,
			"current_gr_syspipe_id[%u] "
				"requested_gr_syspipe_id[%u] enable[%d] ",
			g->mig.current_gr_syspipe_id,
			gr_syspipe_id,
			enable);

		if (gr_syspipe_id != NVGPU_MIG_INVALID_GR_SYSPIPE_ID) {
			if ((g->mig.current_gr_syspipe_id ==
					NVGPU_MIG_INVALID_GR_SYSPIPE_ID) ||
					(!enable)) {
				err = g->ops.priv_ring.config_gr_remap_window(g,
					gr_syspipe_id, enable);
			} else {
				nvgpu_warn(g,
					"Gr remap window enable/disable call "
						"from the same thread "
						"requsted gr_syspipe_id[%u] "
						"current_gr_syspipe_id[%u] ",
					gr_syspipe_id,
					g->mig.current_gr_syspipe_id);
				err = -EPERM;
			}
		} else {
			nvgpu_log(g, gpu_dbg_mig,
				"Legacy GR PGRAPH window enable[%d] ",
				enable);
		}

		if (err != 0) {
			nvgpu_mutex_release(&g->mig.gr_syspipe_lock);
			nvgpu_err(g, "Failed [%d]", err);
			return err;
		}

		if (enable) {
			if ((gr_syspipe_id ==
					NVGPU_MIG_INVALID_GR_SYSPIPE_ID) &&
					(g->mig.current_gr_syspipe_id == 0U)) {
				g->mig.current_gr_syspipe_id =
					NVGPU_MIG_INVALID_GR_SYSPIPE_ID;
				nvgpu_mutex_release(
					&g->mig.gr_syspipe_lock);
			} else {
				g->mig.current_gr_syspipe_id = gr_syspipe_id;
			}
		} else {
			if (g->mig.current_gr_syspipe_id !=
					NVGPU_MIG_INVALID_GR_SYSPIPE_ID) {
				g->mig.current_gr_syspipe_id =
					NVGPU_MIG_INVALID_GR_SYSPIPE_ID;
				nvgpu_mutex_release(
					&g->mig.gr_syspipe_lock);
			} else {
				if (g->mig.current_gr_syspipe_id ==
					NVGPU_MIG_INVALID_GR_SYSPIPE_ID) {
					g->mig.current_gr_syspipe_id = 0U;
				} else {
					nvgpu_warn(g,
						"Repeated Legacy GR remap "
							"window disable call "
							"from same thread ");
				}
			}
		}
	}
#endif
	return err;
}

u32 nvgpu_grmgr_get_num_gr_instances(struct gk20a *g)
{
	/*
	 * There is only one gr engine instance per syspipe.
	 * Hence just return number of syspipes here.
	 */
	return g->mig.num_gr_sys_pipes_enabled;
}

u32 nvgpu_grmgr_get_gr_syspipe_id(struct gk20a *g, u32 gr_instance_id)
{
	struct nvgpu_gpu_instance *gpu_instance;
	struct nvgpu_gr_syspipe *gr_syspipe;

	if (gr_instance_id < g->mig.num_gpu_instances) {
		gpu_instance = &g->mig.gpu_instance[gr_instance_id];
		gr_syspipe = &gpu_instance->gr_syspipe;

		return gr_syspipe->gr_syspipe_id;
	}

	return U32_MAX;
}
