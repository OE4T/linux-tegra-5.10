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

#include <nvgpu/gr/gr.h>
#include <nvgpu/gr/config.h>

u32 nvgpu_gr_get_idle_timeout(struct gk20a *g)
{
	return nvgpu_is_timeouts_enabled(g) ?
		g->gr_idle_timeout_default : UINT_MAX;
}

int nvgpu_gr_init_fs_state(struct gk20a *g)
{
	u32 tpc_index, gpc_index;
	u32 sm_id = 0;
	u32 fuse_tpc_mask;
	u32 gpc_cnt, tpc_cnt, max_tpc_cnt;
	int err = 0;
	struct nvgpu_gr_config *gr_config = g->gr.config;

	nvgpu_log_fn(g, " ");

	err = g->ops.gr.init.fs_state(g);
	if (err != 0) {
		return err;
	}

	if (g->ops.gr.init_sm_id_table != NULL) {
		err = g->ops.gr.init_sm_id_table(g);
		if (err != 0) {
			return err;
		}

		/* Is table empty ? */
		if (g->gr.no_of_sm == 0U) {
			return -EINVAL;
		}
	}

	for (sm_id = 0; sm_id < g->gr.no_of_sm; sm_id++) {
		tpc_index = g->gr.sm_to_cluster[sm_id].tpc_index;
		gpc_index = g->gr.sm_to_cluster[sm_id].gpc_index;

		g->ops.gr.program_sm_id_numbering(g, gpc_index, tpc_index, sm_id);
	}

	g->ops.gr.init.pd_tpc_per_gpc(g);

	/* gr__setup_pd_mapping */
	g->ops.gr.setup_rop_mapping(g, &g->gr);

	g->ops.gr.init.pd_skip_table_gpc(g);

	fuse_tpc_mask = g->ops.gr.config.get_gpc_tpc_mask(g, gr_config, 0);
	gpc_cnt = nvgpu_gr_config_get_gpc_count(gr_config);
	tpc_cnt = nvgpu_gr_config_get_tpc_count(gr_config);
	max_tpc_cnt = nvgpu_gr_config_get_max_tpc_count(gr_config);

	if ((g->tpc_fs_mask_user != 0U) &&
		(fuse_tpc_mask == BIT32(max_tpc_cnt) - 1U)) {
		u32 val = g->tpc_fs_mask_user;
		val &= BIT32(max_tpc_cnt) - U32(1);
		tpc_cnt = (u32)hweight32(val);
	}
	g->ops.gr.init.cwd_gpcs_tpcs_num(g, gpc_cnt, tpc_cnt);

	g->ops.gr.load_tpc_mask(g);

	err = g->ops.gr.load_smid_config(g);
	if (err != 0) {
		nvgpu_err(g, "load_smid_config failed err=%d", err);
	}

	return err;
}
