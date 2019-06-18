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

#include <nvgpu/gk20a.h>
#include <nvgpu/engines.h>
#include <nvgpu/fifo.h>

#include <nvgpu/vgpu/vgpu.h>

#include "engines_vgpu.h"

int vgpu_engine_init_info(struct nvgpu_fifo *f)
{
	struct vgpu_priv_data *priv = vgpu_get_priv_data(f->g);
	struct tegra_vgpu_engines_info *engines = &priv->constants.engines_info;
	u32 i;
	struct gk20a *g = f->g;

	nvgpu_log_fn(g, " ");

	if (engines->num_engines > TEGRA_VGPU_MAX_ENGINES) {
		nvgpu_err(f->g, "num_engines %d larger than max %d",
			engines->num_engines, TEGRA_VGPU_MAX_ENGINES);
		return -EINVAL;
	}

	f->num_engines = engines->num_engines;
	for (i = 0; i < f->num_engines; i++) {
		struct nvgpu_engine_info *info =
				&f->engine_info[engines->info[i].engine_id];

		if (engines->info[i].engine_id >= f->max_engines) {
			nvgpu_err(f->g, "engine id %d larger than max %d",
				engines->info[i].engine_id,
				f->max_engines);
			return -EINVAL;
		}

		info->intr_mask = engines->info[i].intr_mask;
		info->reset_mask = engines->info[i].reset_mask;
		info->runlist_id = engines->info[i].runlist_id;
		info->pbdma_id = engines->info[i].pbdma_id;
		info->inst_id = engines->info[i].inst_id;
		info->pri_base = engines->info[i].pri_base;
		info->engine_enum = engines->info[i].engine_enum;
		info->fault_id = engines->info[i].fault_id;
		f->active_engines_list[i] = engines->info[i].engine_id;
	}

	nvgpu_log_fn(g, "done");

	return 0;
}
