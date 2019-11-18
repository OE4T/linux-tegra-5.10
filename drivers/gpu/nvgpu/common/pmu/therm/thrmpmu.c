/*
 * Copyright (c) 2016-2019, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/boardobjgrp.h>
#include <nvgpu/boardobjgrp_e32.h>
#include <nvgpu/pmu/boardobjgrp_classes.h>
#include <nvgpu/pmu/therm.h>
#include <nvgpu/pmu/pmuif/nvgpu_cmdif.h>
#include <nvgpu/pmu/cmd.h>

#include "thrmpmu.h"
#include "thrm.h"

int therm_send_pmgr_tables_to_pmu(struct gk20a *g)
{
	int status = 0;
	struct boardobjgrp *pboardobjgrp = NULL;

	if (!BOARDOBJGRP_IS_EMPTY(&g->pmu->therm_pmu->therm_deviceobjs.super.super)) {
		pboardobjgrp = &g->pmu->therm_pmu->therm_deviceobjs.super.super;
		status = pboardobjgrp->pmuinithandle(g, pboardobjgrp);
		if (status != 0) {
			nvgpu_err(g,
				"therm_send_pmgr_tables_to_pmu - therm_device failed %x",
				status);
			goto exit;
		}
	}

	if (!BOARDOBJGRP_IS_EMPTY(
			&g->pmu->therm_pmu->therm_channelobjs.super.super)) {
		pboardobjgrp = &g->pmu->therm_pmu->therm_channelobjs.super.super;
		status = pboardobjgrp->pmuinithandle(g, pboardobjgrp);
		if (status != 0) {
			nvgpu_err(g,
				"therm_send_pmgr_tables_to_pmu - therm_channel failed %x",
				status);
			goto exit;
		}
	}

exit:
	return status;
}

void nvgpu_pmu_therm_rpc_handler(struct gk20a *g, struct nvgpu_pmu *pmu,
		struct nv_pmu_rpc_header *rpc)
{
	switch (rpc->function) {
	case NV_PMU_RPC_ID_THERM_BOARD_OBJ_GRP_CMD:
		nvgpu_pmu_dbg(g,
			"reply NV_PMU_RPC_ID_THERM_BOARD_OBJ_GRP_CMD");
		break;
	default:
		nvgpu_pmu_dbg(g, "reply PMU_UNIT_THERM");
		break;
	}
}
