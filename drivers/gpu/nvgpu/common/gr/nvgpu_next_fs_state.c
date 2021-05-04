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

#include <nvgpu/gk20a.h>
#include <nvgpu/static_analysis.h>

#include <nvgpu/gr/config.h>
#include <nvgpu/gr/nvgpu_next_fs_state.h>

int nvgpu_gr_init_sm_id_early_config(struct gk20a *g, struct nvgpu_gr_config *config)
{
	u32 tpc_index, gpc_index;
	u32 sm_id = 0;
	u32 num_sm;
	int err = 0;

	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gr, " ");

	err = g->ops.gr.config.init_sm_id_table(g, config);
	if (err != 0) {
		return err;
	}

	num_sm = nvgpu_gr_config_get_no_of_sm(config);
	nvgpu_assert(num_sm > 0U);

	for (sm_id = 0; sm_id < num_sm; sm_id++) {
		struct nvgpu_sm_info *sm_info =
			nvgpu_gr_config_get_sm_info(config, sm_id);
		tpc_index = nvgpu_gr_config_get_sm_info_tpc_index(sm_info);
		gpc_index = nvgpu_gr_config_get_sm_info_gpc_index(sm_info);

		g->ops.gr.init.sm_id_numbering(g, gpc_index, tpc_index, sm_id,
					       config, NULL, false);
	}

	return err;
}
