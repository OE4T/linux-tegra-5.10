/*
 * Copyright (c) 2011-2020, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/hw/gm20b/hw_fifo_gm20b.h>

#include "engines_gm20b.h"

bool gm20b_is_fault_engine_subid_gpc(struct gk20a *g, u32 engine_subid)
{
	return (engine_subid == fifo_intr_mmu_fault_info_engine_subid_gpc_v());
}

int gm20b_engine_init_ce_info(struct nvgpu_fifo *f)
{
	struct gk20a *g = f->g;
	u32 i;
	enum nvgpu_fifo_engine engine_enum;
	u32 pbdma_id = U32_MAX;
	u32 gr_runlist_id;
	bool found_pbdma_for_runlist = false;

	gr_runlist_id = nvgpu_engine_get_gr_runlist_id(g);
	nvgpu_log_info(g, "gr_runlist_id: %d", gr_runlist_id);

	for (i = NVGPU_DEVTYPE_COPY0;  i <= NVGPU_DEVTYPE_COPY2; i++) {
		struct nvgpu_device *dev;
		struct nvgpu_engine_info *info;

		/*
		 * Cast to a non-const version since we have to hack up a few fields for
		 * SW to work.
		 */
		dev = (struct nvgpu_device *)nvgpu_device_get(g, i, 0);
		if (dev == NULL) {
			/*
			 * Not an error condition; gm20b has only 1 CE.
			 */
			continue;
		}

		found_pbdma_for_runlist = g->ops.pbdma.find_for_runlist(g,
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

		if (g->ops.top.get_ce_inst_id != NULL) {
			dev->inst_id = g->ops.top.get_ce_inst_id(g, dev->type);
		}

		if ((dev->fault_id == 0U) &&
		    (engine_enum == NVGPU_ENGINE_GRCE)) {
			dev->fault_id = 0x1b;
		}

		info->fault_id = dev->fault_id;
		info->intr_mask |= BIT32(dev->intr_id);
		info->reset_mask |= BIT32(dev->reset_id);
		info->runlist_id = dev->runlist_id;
		info->pbdma_id = pbdma_id;
		info->inst_id  = dev->inst_id;
		info->pri_base = dev->pri_base;

		/* engine_id starts from 0 to NV_HOST_NUM_ENGINES */
		f->active_engines_list[f->num_engines] = dev->engine_id;
		++f->num_engines;
		nvgpu_log_info(g, "gr info: engine_id %d runlist_id %d "
			       "intr_id %d reset_id %d type %d "
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
