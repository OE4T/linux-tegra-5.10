/*
 * Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/falcon.h>
#include <nvgpu/gk20a.h>

#include "falcon_gk20a.h"
#include "falcon_gv100.h"
#include "falcon_tu104.h"
#include "tu104/sec2_tu104.h"

#include <nvgpu/hw/tu104/hw_psec_tu104.h>
#include <nvgpu/hw/tu104/hw_pnvdec_tu104.h>

static void tu104_falcon_engine_dependency_ops(struct nvgpu_falcon *flcn)
{
	struct nvgpu_falcon_engine_dependency_ops *flcn_eng_dep_ops =
			&flcn->flcn_engine_dep_ops;

	switch (flcn->flcn_id) {
	case FALCON_ID_SEC2:
		flcn_eng_dep_ops->reset_eng = tu104_sec2_reset;
		flcn_eng_dep_ops->copy_to_emem = tu104_sec2_flcn_copy_to_emem;
		flcn_eng_dep_ops->copy_from_emem =
						tu104_sec2_flcn_copy_from_emem;
		flcn_eng_dep_ops->queue_head = tu104_sec2_queue_head;
		flcn_eng_dep_ops->queue_tail = tu104_sec2_queue_tail;
		break;
	default:
		flcn_eng_dep_ops->reset_eng = NULL;
		break;
	}
}

static void tu104_falcon_ops(struct nvgpu_falcon *flcn)
{
	gk20a_falcon_ops(flcn);
	tu104_falcon_engine_dependency_ops(flcn);
}

int tu104_falcon_hal_sw_init(struct nvgpu_falcon *flcn)
{
	struct gk20a *g = flcn->g;
	int err = 0;

	switch (flcn->flcn_id) {
	case FALCON_ID_SEC2:
		flcn->flcn_base = psec_falcon_irqsset_r();
		flcn->is_falcon_supported = true;
		flcn->is_interrupt_enabled = true;
		break;
	case FALCON_ID_NVDEC:
		flcn->flcn_base = pnvdec_falcon_irqsset_r(0);
		flcn->is_falcon_supported = true;
		flcn->is_interrupt_enabled = true;
		break;
	default:
		/*
		 * set false to inherit falcon support
		 * from previous chips HAL
		 */
		flcn->is_falcon_supported = false;
		break;
	}

	if (flcn->is_falcon_supported) {
		err = nvgpu_mutex_init(&flcn->copy_lock);
		if (err != 0) {
			nvgpu_err(g, "Error in flcn.copy_lock mutex initialization");
		} else {
			tu104_falcon_ops(flcn);
		}
	} else {
		/*
		 * Forward call to previous chips HAL
		 * to fetch info for requested
		 * falcon as no changes between
		 * current & previous chips.
		 */
		err = gv100_falcon_hal_sw_init(flcn);
	}

	return err;
}
