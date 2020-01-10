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

#include <nvgpu/pmu.h>
#include <nvgpu/pmu/pmuif/nvgpu_cmdif.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/pmu/pmuif/ctrlclk.h>
#include <nvgpu/pmu/pmuif/ctrlvolt.h>
#include <nvgpu/bug.h>
#include <nvgpu/pmu/volt.h>
#include <nvgpu/pmu/perf.h>
#include <nvgpu/pmu/clk/clk.h>
#include <nvgpu/pmu/cmd.h>
#include <nvgpu/timers.h>
#include <nvgpu/pmu/pmu_pstate.h>
#include <nvgpu/pmu/perf.h>
#include <nvgpu/pmu/clk/clk_vf_point.h>

int nvgpu_clk_domain_freq_to_volt(struct gk20a *g, u8 clkdomain_idx,
	u32 *pclkmhz, u32 *pvoltuv, u8 railidx)
{

	struct nvgpu_clk_vf_points *pclk_vf_points;
	struct boardobjgrp *pboardobjgrp;
	struct boardobj *pboardobj = NULL;
	int status = -EINVAL;
	struct clk_vf_point *pclk_vf_point;
	u8 index;

	nvgpu_log_info(g, " ");
	pclk_vf_points = g->pmu->clk_pmu->clk_vf_pointobjs;
	pboardobjgrp = &pclk_vf_points->super.super;

	BOARDOBJGRP_FOR_EACH(pboardobjgrp, struct boardobj*, pboardobj, index) {
		pclk_vf_point = (struct clk_vf_point *)(void *)pboardobj;
		if((*pclkmhz) <= pclk_vf_point->pair.freq_mhz) {
			*pvoltuv = pclk_vf_point->pair.voltage_uv;
			return 0;
		}
	}
	return status;
}

#ifdef CONFIG_NVGPU_CLK_ARB
int nvgpu_clk_get_fll_clks(struct gk20a *g,
		struct nvgpu_set_fll_clk *setfllclk)
{
	return g->pmu->clk_pmu->get_fll(g, setfllclk);
}

int nvgpu_clk_set_boot_fll_clk_tu10x(struct gk20a *g)
{
	return g->pmu->clk_pmu->set_boot_fll(g);
}
#endif

int nvgpu_clk_init_pmupstate(struct gk20a *g)
{
	/* If already allocated, do not re-allocate */
	if (g->pmu->clk_pmu != NULL) {
		return 0;
	}

	g->pmu->clk_pmu = nvgpu_kzalloc(g, sizeof(*g->pmu->clk_pmu));
	if (g->pmu->clk_pmu == NULL) {
		return -ENOMEM;
	}

	return 0;
}

void nvgpu_clk_free_pmupstate(struct gk20a *g)
{
	nvgpu_kfree(g, g->pmu->clk_pmu);
	g->pmu->clk_pmu = NULL;
}

u32 nvgpu_clk_mon_init_domains(struct gk20a *g)
{
	u32 domain_mask;

	domain_mask = (CTRL_CLK_DOMAIN_MCLK |
			CTRL_CLK_DOMAIN_XBARCLK 	|
			CTRL_CLK_DOMAIN_SYSCLK		|
			CTRL_CLK_DOMAIN_HUBCLK		|
			CTRL_CLK_DOMAIN_GPCCLK		|
			CTRL_CLK_DOMAIN_HOSTCLK		|
			CTRL_CLK_DOMAIN_UTILSCLK	|
			CTRL_CLK_DOMAIN_PWRCLK		|
			CTRL_CLK_DOMAIN_NVDCLK		|
			CTRL_CLK_DOMAIN_XCLK		|
			CTRL_CLK_DOMAIN_NVL_COMMON	|
			CTRL_CLK_DOMAIN_PEX_REFCLK	);
	return domain_mask;
}
