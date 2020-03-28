/*
 * Copyright (c) 2015-2020, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/device.h>
#include <nvgpu/engines.h>
#include <nvgpu/log.h>
#include <nvgpu/errno.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/static_analysis.h>

#include <nvgpu/hw/gp10b/hw_fifo_gp10b.h>

#include "engines_gp10b.h"

int gp10b_engine_init_ce_info(struct nvgpu_fifo *f)
{
	struct gk20a *g = f->g;
	u32 i;
	enum nvgpu_fifo_engine engine_enum;
	u32 gr_runlist_id;
	u32 pbdma_id = U32_MAX;
	bool found_pbdma_for_runlist = false;
	u32 lce_num_entries = 0;

	gr_runlist_id = nvgpu_engine_get_gr_runlist_id(g);
	nvgpu_log_info(g, "gr_runlist_id: %d", gr_runlist_id);

	lce_num_entries = nvgpu_device_count(g, NVGPU_DEVTYPE_LCE);
	nvgpu_log_info(g, "lce_num_entries: %d", lce_num_entries);

	for (i = 0; i < lce_num_entries; i++) {
		const struct nvgpu_device *dev;
		struct nvgpu_engine_info *info;

		dev = nvgpu_device_get(g, NVGPU_DEVTYPE_LCE, i);
		if (dev == NULL) {
			nvgpu_err(g, "Failed to get LCE device %u", i);
			return -EINVAL;
		}

		found_pbdma_for_runlist =
				g->ops.pbdma.find_for_runlist(g,
						dev->runlist_id,
						&pbdma_id);
		if (!found_pbdma_for_runlist) {
			nvgpu_err(g, "busted pbdma map");
			return -EINVAL;
		}

		info = &g->fifo.engine_info[dev->engine_id];

		engine_enum = nvgpu_engine_enum_from_dev(g, dev);
		/* GR and GR_COPY shares same runlist_id */
		if ((engine_enum == NVGPU_ENGINE_ASYNC_CE) &&
		    (gr_runlist_id == dev->runlist_id)) {
			engine_enum = NVGPU_ENGINE_GRCE;
		}
		info->engine_enum = engine_enum;

		info->fault_id = dev->fault_id;
		info->intr_mask |= BIT32(dev->intr_id);
		info->reset_mask |= BIT32(dev->reset_id);
		info->runlist_id = dev->runlist_id;
		info->pbdma_id = pbdma_id;
		info->inst_id  = dev->inst_id;
		info->pri_base = dev->pri_base;
		info->engine_id = dev->engine_id;

		/* engine_id starts from 0 to NV_HOST_NUM_ENGINES */
		f->active_engines_list[f->num_engines] = dev->engine_id;
		f->num_engines = nvgpu_safe_add_u32(f->num_engines, 1U);
		nvgpu_log_info(g, "gr info: engine_id %d runlist_id %d "
			"intr_id %d reset_id %d engine_type %d "
			"engine_enum %d inst_id %d",
			dev->engine_id,
			dev->runlist_id,
			dev->intr_id,
			dev->reset_id,
			dev->type,
			engine_enum,
			dev->inst_id);
	}
	return 0;
}
