/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/falcon.h>

#include "falcon_sw_gk20a.h"
#include "falcon_sw_gp106.h"
#include "falcon_sw_gv100.h"

void gv100_falcon_engine_dependency_ops(struct nvgpu_falcon *flcn)
{
	struct nvgpu_falcon_engine_dependency_ops *flcn_eng_dep_ops =
			&flcn->flcn_engine_dep_ops;
	struct gk20a *g = flcn->g;

	gk20a_falcon_engine_dependency_ops(flcn);

	switch (flcn->flcn_id) {
	case FALCON_ID_GSPLITE:
		flcn_eng_dep_ops->reset_eng = g->ops.gsp.gsp_reset;
	break;
	default:
		flcn_eng_dep_ops->reset_eng = NULL;
		break;
	}
}

void gv100_falcon_sw_init(struct nvgpu_falcon *flcn)
{
	struct gk20a *g = flcn->g;

	switch (flcn->flcn_id) {
	case FALCON_ID_MINION:
		flcn->flcn_base = g->ops.nvlink.minion.base_addr(g);
		flcn->is_falcon_supported = true;
		flcn->is_interrupt_enabled = true;
		break;
	case FALCON_ID_GSPLITE:
		flcn->flcn_base = g->ops.gsp.falcon_base_addr();
		flcn->is_falcon_supported = true;
		flcn->is_interrupt_enabled = false;
		break;
	default:
		flcn->is_falcon_supported = false;
		break;
	}

	if (flcn->is_falcon_supported) {
		gv100_falcon_engine_dependency_ops(flcn);
	} else {
		/*
		 * Forward call to previous chip's SW init
		 * to fetch info for requested
		 * falcon as no changes between
		 * current & previous chips.
		 */
		gp106_falcon_sw_init(flcn);
	}
}
