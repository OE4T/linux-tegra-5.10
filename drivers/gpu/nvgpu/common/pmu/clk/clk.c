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
#include <nvgpu/pmu/pmuif/ctrlperf.h>
#include <nvgpu/pmu/volt.h>
#include <nvgpu/pmu/perf.h>
#include <nvgpu/pmu/clk/clk.h>
#include <nvgpu/pmu/cmd.h>
#include <nvgpu/timers.h>
#include <nvgpu/pmu/pmu_pstate.h>
#include <nvgpu/pmu/perf_pstate.h>
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

int nvgpu_clk_get_fll_clks(struct gk20a *g,
		struct nvgpu_set_fll_clk *setfllclk)
{
	return g->pmu->clk_pmu->get_fll(g, setfllclk);
}

int nvgpu_clk_set_boot_fll_clk_tu10x(struct gk20a *g)
{
	return g->pmu->clk_pmu->set_boot_fll(g);
}

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

int nvgpu_clk_set_req_fll_clk_ps35(struct gk20a *g,
		struct nvgpu_clk_slave_freq *vf_point)
{
	struct nvgpu_pmu *pmu = g->pmu;
	struct nv_pmu_rpc_perf_change_seq_queue_change rpc;
	struct ctrl_perf_change_seq_change_input change_input;
	struct change_seq_pmu *change_seq_pmu = &g->perf_pmu->changeseq_pmu;
	int status = 0;
	u8 gpcclk_domain = 0U;
	u32 gpcclk_voltuv = 0U, gpcclk_clkmhz = 0U;
	u32 vmin_uv = 0U, vmax_uv = 0U;
	u32 vmargin_uv = 0U, fmargin_mhz = 0U;

	(void) memset(&change_input, 0,
		sizeof(struct ctrl_perf_change_seq_change_input));

	g->pmu->clk_pmu->set_p0_clks(g, &gpcclk_domain, &gpcclk_clkmhz,
			vf_point, &change_input);

	change_input.pstate_index =
			nvgpu_get_pstate_entry_idx(g, CTRL_PERF_PSTATE_P0);
	change_input.flags = (u32)CTRL_PERF_CHANGE_SEQ_CHANGE_FORCE;
	change_input.vf_points_cache_counter = 0xFFFFFFFFU;

	status = nvgpu_vfe_get_freq_margin_limit(g, &fmargin_mhz);
	if (status != 0) {
		nvgpu_err(g, "Failed to fetch Fmargin status=0x%x", status);
		return status;
	}

	gpcclk_clkmhz += fmargin_mhz;
	status = nvgpu_clk_domain_freq_to_volt(g, gpcclk_domain,
	&gpcclk_clkmhz, &gpcclk_voltuv, CTRL_VOLT_DOMAIN_LOGIC);

	status = nvgpu_vfe_get_volt_margin_limit(g, &vmargin_uv);
	if (status != 0) {
		nvgpu_err(g, "Failed to fetch Vmargin status=0x%x", status);
		return status;
	}

	gpcclk_voltuv += vmargin_uv;
	status = nvgpu_volt_get_vmin_vmax_ps35(g, &vmin_uv, &vmax_uv);
	if (status != 0) {
		nvgpu_pmu_dbg(g, "Get vmin,vmax failed, proceeding with "
			"freq_to_volt value");
	}
	if ((status == 0) && (vmin_uv > gpcclk_voltuv)) {
		gpcclk_voltuv = vmin_uv;
		nvgpu_log_fn(g, "Vmin is higher than evaluated Volt");
	}

	if (gpcclk_voltuv > vmax_uv) {
		nvgpu_err(g, "Error: Requested voltage is more than chip max");
		return -EINVAL;
	}

	change_input.volt[0].voltage_uv = gpcclk_voltuv;
	change_input.volt[0].voltage_min_noise_unaware_uv = gpcclk_voltuv;
	change_input.volt_rails_mask.super.data[0] = 1U;

	/* RPC to PMU to queue to execute change sequence request*/
	(void) memset(&rpc, 0,
			sizeof(struct nv_pmu_rpc_perf_change_seq_queue_change));
	rpc.change = change_input;
	rpc.change.pstate_index =
			nvgpu_get_pstate_entry_idx(g, CTRL_PERF_PSTATE_P0);
	change_seq_pmu->change_state = 0U;
	change_seq_pmu->start_time = nvgpu_current_time_us();
	PMU_RPC_EXECUTE_CPB(status, pmu, PERF,
			CHANGE_SEQ_QUEUE_CHANGE, &rpc, 0);
	if (status != 0) {
		nvgpu_err(g, "Failed to execute Change Seq RPC status=0x%x",
			status);
	}

	/* Wait for sync change to complete. */
	if ((rpc.change.flags & CTRL_PERF_CHANGE_SEQ_CHANGE_ASYNC) == 0U) {
		pmu_wait_message_cond(g->pmu,
			nvgpu_get_poll_timeout(g),
			&change_seq_pmu->change_state, 1U);
	}
	change_seq_pmu->stop_time = nvgpu_current_time_us();
	return status;
}

