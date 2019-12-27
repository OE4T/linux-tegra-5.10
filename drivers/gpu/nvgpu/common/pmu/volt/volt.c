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

#include <nvgpu/pmu/volt.h>
#include <nvgpu/gk20a.h>

#include "volt_rail.h"
#include "volt_dev.h"
#include "volt_policy.h"
#include "volt_pmu.h"

int nvgpu_pmu_volt_sw_setup(struct gk20a *g)
{
	int err;
	nvgpu_log_fn(g, " ");

	err = nvgpu_volt_rail_sw_setup(g);
	if (err != 0) {
		return err;
	}

	err = nvgpu_volt_dev_sw_setup(g);
	if (err != 0) {
		return err;
	}

	err = nvgpu_volt_policy_sw_setup(g);
	if (err != 0) {
		return err;
	}

	return 0;
}

int nvgpu_pmu_volt_init(struct gk20a *g)
{
	int err = 0;

	nvgpu_log_fn(g, " ");

	/* If already allocated, do not re-allocate */
	if (g->pmu->volt != NULL) {
		return 0;
	}

	g->pmu->volt = (struct nvgpu_pmu_volt *) nvgpu_kzalloc(g,
			sizeof(struct nvgpu_pmu_volt));
	if (g->pmu->volt == NULL) {
		err = -ENOMEM;
		return err;
	}

	return err;
}

void nvgpu_pmu_volt_deinit(struct gk20a *g)
{
	if ((g->pmu != NULL) && (g->pmu->volt != NULL)) {
		nvgpu_kfree(g, g->pmu->volt);
		g->pmu->volt = NULL;
	}
}

int nvgpu_pmu_volt_pmu_setup(struct gk20a *g)
{
	int err;
	nvgpu_log_fn(g, " ");

	err = nvgpu_volt_rail_pmu_setup(g);
	if (err != 0) {
		return err;
	}

	err = nvgpu_volt_dev_pmu_setup(g);
	if (err != 0) {
		return err;
	}

	err = nvgpu_volt_policy_pmu_setup(g);
	if (err != 0) {
		return err;
	}

	err = nvgpu_volt_send_load_cmd_to_pmu(g);
	if (err != 0) {
		nvgpu_err(g,
			"Failed to send VOLT LOAD CMD to PMU: status = 0x%08x.",
			err);
		return err;
	}

	return 0;
}
