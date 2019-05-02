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

void nvgpu_clkrpc_pmucmdhandler(struct gk20a *g, struct pmu_msg *msg,
		void *param, u32 status)
{
	struct nvgpu_clkrpc_pmucmdhandler_params *phandlerparams =
		(struct nvgpu_clkrpc_pmucmdhandler_params *)param;

	nvgpu_log_info(g, " ");

	if (msg->msg.clk.msg_type != NV_PMU_CLK_MSG_ID_RPC) {
		nvgpu_err(g, "unsupported msg for CLK LOAD RPC %x",
			msg->msg.clk.msg_type);
		return;
	}

	if (phandlerparams->prpccall->b_supported) {
		phandlerparams->success = 1;
	}
}

int nvgpu_clk_domain_freq_to_volt(struct gk20a *g, u8 clkdomain_idx,
	u32 *pclkmhz, u32 *pvoltuv, u8 railidx)
{
	struct nv_pmu_rpc_clk_domain_35_prog_freq_to_volt  rpc;
	struct nvgpu_pmu *pmu = &g->pmu;
	int status = -EINVAL;

	(void)memset(&rpc, 0,
		sizeof(struct nv_pmu_rpc_clk_domain_35_prog_freq_to_volt));
	rpc.volt_rail_idx =
			nvgpu_volt_rail_volt_domain_convert_to_idx(g, railidx);
	rpc.clk_domain_idx = clkdomain_idx;
	rpc.voltage_type = CTRL_VOLT_DOMAIN_LOGIC;
	rpc.input.value = *pclkmhz;
	PMU_RPC_EXECUTE_CPB(status, pmu, CLK,
		CLK_DOMAIN_35_PROG_FREQ_TO_VOLT, &rpc, 0);
	if (status != 0) {
		nvgpu_err(g, "Failed to execute Freq to Volt RPC status=0x%x",
			status);
	}
	*pvoltuv = rpc.output.value;
	return status;
}

static void nvgpu_clk_vf_change_inject_data_fill(struct gk20a *g,
	struct nv_pmu_clk_rpc *rpccall,
	struct nvgpu_set_fll_clk *setfllclk)
{
	struct nv_pmu_clk_vf_change_inject_v1 *vfchange;

	vfchange = &rpccall->params.clk_vf_change_inject_v1;
	vfchange->flags = 0;
	vfchange->clk_list.num_domains = 4;
	vfchange->clk_list.clk_domains[0].clk_domain = CTRL_CLK_DOMAIN_GPCCLK;
	vfchange->clk_list.clk_domains[0].clk_freq_khz =
		(u32)setfllclk->gpc2clkmhz * 1000U;

	vfchange->clk_list.clk_domains[1].clk_domain = CTRL_CLK_DOMAIN_XBARCLK;
	vfchange->clk_list.clk_domains[1].clk_freq_khz =
		(u32)setfllclk->xbar2clkmhz * 1000U;

	vfchange->clk_list.clk_domains[2].clk_domain = CTRL_CLK_DOMAIN_SYSCLK;
	vfchange->clk_list.clk_domains[2].clk_freq_khz =
		(u32)setfllclk->sys2clkmhz * 1000U;

	vfchange->clk_list.clk_domains[3].clk_domain = CTRL_CLK_DOMAIN_NVDCLK;
	vfchange->clk_list.clk_domains[3].clk_freq_khz = 855 * 1000;

	vfchange->volt_list.num_rails = 1;
	vfchange->volt_list.rails[0].rail_idx = 0;
	vfchange->volt_list.rails[0].voltage_uv = setfllclk->voltuv;
	vfchange->volt_list.rails[0].voltage_min_noise_unaware_uv =
		setfllclk->voltuv;
}

static int clk_pmu_vf_inject(struct gk20a *g,
		struct nvgpu_set_fll_clk *setfllclk)
{
	struct pmu_cmd cmd;
	struct pmu_payload payload;
	int status;
	struct nv_pmu_clk_rpc rpccall;
	struct nvgpu_clkrpc_pmucmdhandler_params handler;

	(void) memset(&payload, 0, sizeof(struct pmu_payload));
	(void) memset(&rpccall, 0, sizeof(struct nv_pmu_clk_rpc));
	(void) memset(&handler, 0,
		sizeof(struct nvgpu_clkrpc_pmucmdhandler_params));
	(void) memset(&cmd, 0, sizeof(struct pmu_cmd));

	if ((setfllclk->gpc2clkmhz == 0U) || (setfllclk->xbar2clkmhz == 0U) ||
		(setfllclk->sys2clkmhz == 0U) || (setfllclk->voltuv == 0U)) {
		return -EINVAL;
	}

	if ((setfllclk->target_regime_id_gpc > CTRL_CLK_FLL_REGIME_ID_FR) ||
		(setfllclk->target_regime_id_sys > CTRL_CLK_FLL_REGIME_ID_FR) ||
		(setfllclk->target_regime_id_xbar > CTRL_CLK_FLL_REGIME_ID_FR)) {
		return -EINVAL;
	}

	rpccall.function = NV_PMU_CLK_RPC_ID_CLK_VF_CHANGE_INJECT;

	nvgpu_clk_vf_change_inject_data_fill(g, &rpccall, setfllclk);

	cmd.hdr.unit_id = PMU_UNIT_CLK;
	cmd.hdr.size =  (u32)sizeof(struct nv_pmu_clk_cmd) +
		(u32)sizeof(struct pmu_hdr);

	cmd.cmd.clk.cmd_type = NV_PMU_CLK_CMD_ID_RPC;

	payload.in.buf = (u8 *)&rpccall;
	payload.in.size = (u32)sizeof(struct nv_pmu_clk_rpc);
	payload.in.fb_size = PMU_CMD_SUBMIT_PAYLOAD_PARAMS_FB_SIZE_UNUSED;
	nvgpu_assert(NV_PMU_CLK_CMD_RPC_ALLOC_OFFSET < U64(U32_MAX));
	payload.in.offset = (u32)NV_PMU_CLK_CMD_RPC_ALLOC_OFFSET;

	payload.out.buf = (u8 *)&rpccall;
	payload.out.size = (u32)sizeof(struct nv_pmu_clk_rpc);
	payload.out.fb_size = PMU_CMD_SUBMIT_PAYLOAD_PARAMS_FB_SIZE_UNUSED;
	payload.out.offset = NV_PMU_CLK_MSG_RPC_ALLOC_OFFSET;

	handler.prpccall = &rpccall;
	handler.success = 0;

	status = nvgpu_pmu_cmd_post(g, &cmd, &payload,
		PMU_COMMAND_QUEUE_LPQ, nvgpu_clkrpc_pmucmdhandler,
		(void *)&handler);

	if (status != 0) {
		nvgpu_err(g, "unable to post clk RPC cmd %x",
			cmd.cmd.clk.cmd_type);
		goto done;
	}

	pmu_wait_message_cond(&g->pmu,
			nvgpu_get_poll_timeout(g),
			&handler.success, 1);

	if (handler.success == 0U) {
		nvgpu_err(g, "rpc call to inject clock failed");
		status = -EINVAL;
	}
done:
	return status;
}

int nvgpu_clk_set_fll_clks(struct gk20a *g,
		struct nvgpu_set_fll_clk *setfllclk)
{
	int status = -EINVAL;

	/*set regime ids */
	status = g->pmu.clk_pmu->get_regime_id(g, CTRL_CLK_DOMAIN_GPCCLK,
			&setfllclk->current_regime_id_gpc);
	if (status != 0) {
		goto done;
	}

	setfllclk->target_regime_id_gpc = g->pmu.clk_pmu->find_regime_id(g,
			CTRL_CLK_DOMAIN_GPCCLK, setfllclk->gpc2clkmhz);

	status = g->pmu.clk_pmu->get_regime_id(g, CTRL_CLK_DOMAIN_SYSCLK,
			&setfllclk->current_regime_id_sys);
	if (status != 0) {
		goto done;
	}

	setfllclk->target_regime_id_sys = g->pmu.clk_pmu->find_regime_id(g,
			CTRL_CLK_DOMAIN_SYSCLK, setfllclk->sys2clkmhz);

	status = g->pmu.clk_pmu->get_regime_id(g, CTRL_CLK_DOMAIN_XBARCLK,
			&setfllclk->current_regime_id_xbar);
	if (status != 0) {
		goto done;
	}

	setfllclk->target_regime_id_xbar = g->pmu.clk_pmu->find_regime_id(g,
			CTRL_CLK_DOMAIN_XBARCLK, setfllclk->xbar2clkmhz);

	status = clk_pmu_vf_inject(g, setfllclk);

	if (status != 0) {
		nvgpu_err(g, "vf inject to change clk failed");
	}

	/* save regime ids */
	status = g->pmu.clk_pmu->set_regime_id(g, CTRL_CLK_DOMAIN_XBARCLK,
			setfllclk->target_regime_id_xbar);
	if (status != 0) {
		goto done;
	}

	status = g->pmu.clk_pmu->set_regime_id(g, CTRL_CLK_DOMAIN_GPCCLK,
			setfllclk->target_regime_id_gpc);
	if (status != 0) {
		goto done;
	}

	status = g->pmu.clk_pmu->set_regime_id(g, CTRL_CLK_DOMAIN_SYSCLK,
			setfllclk->target_regime_id_sys);
	if (status != 0) {
		goto done;
	}
done:
	return status;
}

int nvgpu_clk_get_fll_clks(struct gk20a *g,
		struct nvgpu_set_fll_clk *setfllclk)
{
	return g->pmu.clk_pmu->get_fll(g, setfllclk);
}

int nvgpu_clk_set_boot_fll_clk_tu10x(struct gk20a *g)
{
	return g->pmu.clk_pmu->set_boot_fll(g);
}

int nvgpu_clk_init_pmupstate(struct gk20a *g)
{
	/* If already allocated, do not re-allocate */
	if (g->pmu.clk_pmu != NULL) {
		return 0;
	}

	g->pmu.clk_pmu = nvgpu_kzalloc(g, sizeof(*g->pmu.clk_pmu));
	if (g->pmu.clk_pmu == NULL) {
		return -ENOMEM;
	}

	return 0;
}

void nvgpu_clk_free_pmupstate(struct gk20a *g)
{
	nvgpu_kfree(g, g->pmu.clk_pmu);
	g->pmu.clk_pmu = NULL;
}

int nvgpu_clk_set_req_fll_clk_ps35(struct gk20a *g,
		struct nvgpu_clk_slave_freq *vf_point)
{
	struct nvgpu_pmu *pmu = &g->pmu;
	struct nv_pmu_rpc_perf_change_seq_queue_change rpc;
	struct ctrl_perf_change_seq_change_input change_input;
	int status = 0;
	u8 gpcclk_domain = 0U;
	u32 gpcclk_voltuv = 0U, gpcclk_clkmhz = 0U;
	u32 vmin_uv = 0, vmargin_uv = 0U, fmargin_mhz = 0U;

	(void) memset(&change_input, 0,
		sizeof(struct ctrl_perf_change_seq_change_input));

	g->pmu.clk_pmu->set_p0_clks(g, &gpcclk_domain, &gpcclk_clkmhz,
			vf_point, &change_input);

	change_input.pstate_index = 0U;
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

	status = nvgpu_volt_get_vmin_ps35(g, &vmin_uv);
	if (status != 0) {
		nvgpu_pmu_dbg(g,
			"Get vmin failed, proceeding with freq_to_volt value");
	}
	if ((status == 0) && (vmin_uv > gpcclk_voltuv)) {
		gpcclk_voltuv = vmin_uv;
		nvgpu_log_fn(g, "Vmin is higher than evaluated Volt");
	}

	change_input.volt[0].voltage_uv = gpcclk_voltuv;
	change_input.volt[0].voltage_min_noise_unaware_uv = gpcclk_voltuv;
	change_input.volt_rails_mask.super.data[0] = 1U;

	/* RPC to PMU to queue to execute change sequence request*/
	(void) memset(&rpc, 0,
			sizeof(struct nv_pmu_rpc_perf_change_seq_queue_change));
	rpc.change = change_input;
	rpc.change.pstate_index =  0;
	PMU_RPC_EXECUTE_CPB(status, pmu, PERF,
			CHANGE_SEQ_QUEUE_CHANGE, &rpc, 0);
	if (status != 0) {
		nvgpu_err(g, "Failed to execute Change Seq RPC status=0x%x",
			status);
	}

	/* Wait for sync change to complete. */
	if ((rpc.change.flags & CTRL_PERF_CHANGE_SEQ_CHANGE_ASYNC) == 0U) {
		nvgpu_msleep(20);
	}
	return status;
}

