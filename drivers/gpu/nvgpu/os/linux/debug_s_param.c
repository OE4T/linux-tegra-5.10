/*
 * Copyright (c) 2019, NVIDIA Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/debugfs.h>
#include "os_linux.h"
#include "include/nvgpu/bios.h"

#include <common/pmu/perf/vfe_var.h>
#include <nvgpu/pmu/perf.h>

static int get_s_param_info(void *data, u64 *val)
{
	struct gk20a *g = (struct gk20a *)data;
	struct boardobjgrp *pboardobjgrp;
	struct boardobj *pboardobj = NULL;
	struct vfe_var_single_sensed_fuse *single_sensed_fuse = NULL;
	int status;
	u8 index;

	status = nvgpu_vfe_var_boardobj_grp_get_status(g);
	if(status != 0) {
		nvgpu_err(g, "Vfe_var get status failed");
		return status;
	}

	pboardobjgrp = &g->perf_pmu->vfe_varobjs.super.super;

	BOARDOBJGRP_FOR_EACH(pboardobjgrp, struct boardobj*, pboardobj, index) {
		single_sensed_fuse = (struct vfe_var_single_sensed_fuse *)
				(void *)pboardobj;
		if(single_sensed_fuse->vfield_info.v_field_id ==
				VFIELD_ID_S_PARAM) {
			*val = single_sensed_fuse->fuse_value_hw_integer;
		}
	}
	return status;
}
DEFINE_SIMPLE_ATTRIBUTE(s_param_fops, get_s_param_info , NULL, "%llu\n");

int nvgpu_s_param_init_debugfs(struct gk20a *g)
{
	struct nvgpu_os_linux *l = nvgpu_os_linux_from_gk20a(g);
	struct dentry *dbgentry;

	dbgentry = debugfs_create_file(
		"s_param", S_IRUGO, l->debugfs, g, &s_param_fops);
	if (!dbgentry) {
		pr_err("%s: Failed to make debugfs node\n", __func__);
		return -ENOMEM;
	}

	return 0;
}
