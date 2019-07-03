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

#include <nvgpu/pmu.h>
#include <nvgpu/io.h>
#include <nvgpu/gk20a.h>

#include "pmu_gk20a.h"
#include "pmu_gp106.h"

#include <nvgpu/hw/gp106/hw_pwr_gp106.h>

bool gp106_pmu_is_engine_in_reset(struct gk20a *g)
{
	u32 reg_reset;
	bool status = false;

	reg_reset = gk20a_readl(g, pwr_falcon_engine_r());
	if (reg_reset == pwr_falcon_engine_reset_true_f()) {
		status = true;
	}

	return status;
}

void gp106_pmu_engine_reset(struct gk20a *g, bool do_reset)
{
	/*
	* From GP10X onwards, we are using PPWR_FALCON_ENGINE for reset. And as
	* it may come into same behavior, reading NV_PPWR_FALCON_ENGINE again
	* after Reset.
	*/
	if (do_reset) {
		gk20a_writel(g, pwr_falcon_engine_r(),
			pwr_falcon_engine_reset_false_f());
		(void) gk20a_readl(g, pwr_falcon_engine_r());
	} else {
		gk20a_writel(g, pwr_falcon_engine_r(),
			pwr_falcon_engine_reset_true_f());
		(void) gk20a_readl(g, pwr_falcon_engine_r());
	}
}
