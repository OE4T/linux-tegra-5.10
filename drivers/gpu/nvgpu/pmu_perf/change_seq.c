/*
 * Copyright (c) 2016-2018, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/pmuif/nvgpu_gpmu_cmdif.h>
#include <nvgpu/pmuif/gpmu_super_surf_if.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/timers.h>
#include <nvgpu/pmuif/ctrlclk.h>

#include "clk/clk_domain.h"
#include "pstate/pstate.h"
#include "pmu_perf.h"

#include "change_seq.h"

static int perf_change_seq_sw_setup_super(struct gk20a *g,
		struct change_seq *p_change_seq)
{
	int status = 0;

	nvgpu_log_fn(g, " ");

	/* Initialize parameters */
	p_change_seq->client_lock_mask = 0;

	p_change_seq->version = CTRL_PERF_CHANGE_SEQ_VERSION_35;

	status = boardobjgrpmask_init(
		&p_change_seq->clk_domains_exclusion_mask.super,
		32U, ((void*)0));
	if (status != 0) {
		nvgpu_err(g, "clk_domains_exclusion_mask failed to init %d",
			status);
		goto perf_change_seq_sw_setup_super_exit;
	}

	status = boardobjgrpmask_init(
		&p_change_seq->clk_domains_inclusion_mask.super,
		32U, ((void*)0));
	if (status != 0) {
		nvgpu_err(g, "clk_domains_inclusion_mask failed to init %d",
			status);
		goto perf_change_seq_sw_setup_super_exit;
	}

perf_change_seq_sw_setup_super_exit:
	return status;
}

int nvgpu_perf_change_seq_sw_setup(struct gk20a *g)
{
	struct change_seq_pmu *perf_change_seq_pmu =
		&(g->perf_pmu->changeseq_pmu);
	int status = 0;

	nvgpu_log_fn(g, " ");

	(void) memset(perf_change_seq_pmu, 0,
			sizeof(struct change_seq_pmu));

	status = perf_change_seq_sw_setup_super(g, &perf_change_seq_pmu->super);
	if (status != 0) {
		goto exit;
	}

	perf_change_seq_pmu->super.b_enabled_pmu_support = true;
	/*exclude MCLK, may not be needed as MCLK is already fixed */
	perf_change_seq_pmu->super.clk_domains_exclusion_mask.super.data[0]
		= 0x04U;
	perf_change_seq_pmu->b_vf_point_check_ignore = true;
	perf_change_seq_pmu->b_lock = false;
	perf_change_seq_pmu->cpu_step_id_mask = 0;
	perf_change_seq_pmu->cpu_adverised_step_id_mask = 0;

exit:
	return status;
}

static void build_change_seq_boot (struct gk20a *g)
{
	struct nvgpu_pmu *pmu = &g->pmu;
	struct change_seq_pmu *perf_change_seq_pmu =
		&(g->perf_pmu->changeseq_pmu);
	struct clk_domain *pdomain;
	struct clk_set_info *p0_info;
	struct change_seq_pmu_script *script_last =
		&perf_change_seq_pmu->script_last;
	u8 i = 0;

	nvgpu_log_fn(g, " ");

	script_last->super_surface_offset =
		(u32) offsetof(struct nv_pmu_super_surface,
				change_seq.script_last);

	nvgpu_mem_rd_n(g, &pmu->super_surface_buf,
		script_last->super_surface_offset,
		&script_last->buf,
		(u32) sizeof(struct perf_change_seq_pmu_script));

	script_last->buf.change.data.flags = CTRL_PERF_CHANGE_SEQ_CHANGE_NONE;

	BOARDOBJGRP_FOR_EACH(&(g->clk_pmu->clk_domainobjs.super.super),
		struct clk_domain *, pdomain, i) {

		p0_info = pstate_get_clk_set_info(g, CTRL_PERF_PSTATE_P0,
			pdomain->domain);

		script_last->buf.change.data.clk_list.clk_domains[i].clk_domain =
			pdomain->api_domain;

		script_last->buf.change.data.clk_list.clk_domains[i].clk_freq_khz =
			p0_info->nominal_mhz * 1000U;

		/* VBIOS always boots with FFR*/
		script_last->buf.change.data.clk_list.clk_domains[i].regime_id =
			CTRL_CLK_FLL_REGIME_ID_FFR;

		script_last->buf.change.data.clk_list.num_domains++;

		nvgpu_pmu_dbg(g, "Domain %x, Nom Freq = %d Max Freq =%d, regime %d",
			pdomain->api_domain,p0_info->nominal_mhz, p0_info->max_mhz,
			CTRL_CLK_FLL_REGIME_ID_FFR);
	}

	nvgpu_pmu_dbg(g,"Total domains = %d\n",
		script_last->buf.change.data.clk_list.num_domains);

	/* Assume everything is P0 - Need to find the index for P0  */
	script_last->buf.change.data.pstate_index = 0;

	nvgpu_mem_wr_n(g, &pmu->super_surface_buf,
		script_last->super_surface_offset,
		&script_last->buf,
		(u32) sizeof(struct perf_change_seq_pmu_script));

	return;
}

static int perf_pmu_load(struct gk20a *g)
{
	int status = 0;
	struct nv_pmu_rpc_struct_perf_load rpc;
	struct nvgpu_pmu *pmu = &g->pmu;

	(void) memset(&rpc, 0, sizeof(struct nv_pmu_rpc_struct_perf_load));
	PMU_RPC_EXECUTE_CPB(status, pmu, PERF, LOAD, &rpc, 0);
	if (status != 0) {
		nvgpu_err(g, "Failed to execute RPC status=0x%x",
				status);
	}
	return status;
}

int nvgpu_perf_change_seq_pmu_setup(struct gk20a *g)
{
	struct nv_pmu_rpc_perf_change_seq_info_get info_get;
	struct nv_pmu_rpc_perf_change_seq_info_set info_set;
	struct nvgpu_pmu *pmu = &g->pmu;
	struct change_seq_pmu *perf_change_seq_pmu =
		&(g->perf_pmu->changeseq_pmu);
	int status;

	/* Do this  till we enable performance table */
	build_change_seq_boot(g);

	(void) memset(&info_get, 0,
			sizeof(struct nv_pmu_rpc_perf_change_seq_info_get));
	(void) memset(&info_set, 0,
			sizeof(struct nv_pmu_rpc_perf_change_seq_info_set));

	PMU_RPC_EXECUTE_CPB(status, pmu, PERF, CHANGE_SEQ_INFO_GET, &info_get, 0);
	if (status != 0) {
		nvgpu_err(g,
			"Failed to execute Change Seq GET RPC status=0x%x",
			status);
		goto perf_change_seq_pmu_setup_exit;
	}

	info_set.info_set.super.version = perf_change_seq_pmu->super.version;

	status = boardobjgrpmask_export(
		&perf_change_seq_pmu->super.clk_domains_exclusion_mask.super,
		perf_change_seq_pmu->super.clk_domains_exclusion_mask.super.bitcount,
		&info_set.info_set.super.clk_domains_exclusion_mask.super);
	if ( status != 0 ) {
		nvgpu_err(g, "Could not export clkdomains exclusion mask");
		goto perf_change_seq_pmu_setup_exit;
	}

	status = boardobjgrpmask_export(
		&perf_change_seq_pmu->super.clk_domains_inclusion_mask.super,
		perf_change_seq_pmu->super.clk_domains_inclusion_mask.super.bitcount,
		&info_set.info_set.super.clk_domains_inclusion_mask.super);
	if ( status != 0 ) {
		nvgpu_err(g, "Could not export clkdomains inclusion mask");
		goto perf_change_seq_pmu_setup_exit;
	}

	info_set.info_set.b_vf_point_check_ignore =
		perf_change_seq_pmu->b_vf_point_check_ignore;
	info_set.info_set.cpu_step_id_mask =
		perf_change_seq_pmu->cpu_step_id_mask;
	info_set.info_set.b_lock =
		perf_change_seq_pmu->b_lock;

	perf_change_seq_pmu->script_last.super_surface_offset =
		(u32) offsetof(struct nv_pmu_super_surface,
				change_seq.script_last);

	nvgpu_mem_rd_n(g, &pmu->super_surface_buf,
		perf_change_seq_pmu->script_last.super_surface_offset,
		&perf_change_seq_pmu->script_last.buf,
		(u32) sizeof(struct perf_change_seq_pmu_script));

	/* Assume everything is P0 - Need to find the index for P0  */
	perf_change_seq_pmu->script_last.buf.change.data.pstate_index = 0;

	nvgpu_mem_wr_n(g, &pmu->super_surface_buf,
		perf_change_seq_pmu->script_last.super_surface_offset,
		&perf_change_seq_pmu->script_last.buf,
		(u32) sizeof(struct perf_change_seq_pmu_script));

	/* Continue with PMU setup, assume FB map is done  */
	PMU_RPC_EXECUTE_CPB(status, pmu, PERF, CHANGE_SEQ_INFO_SET, &info_set, 0);
	if (status != 0) {
		nvgpu_err(g,
			"Failed to execute Change Seq SET RPC status=0x%x",
			status);
		goto perf_change_seq_pmu_setup_exit;
	}

	/* Perf Load*/
	status = perf_pmu_load(g);
	if (status != 0) {
		nvgpu_err(g, "Failed to Load Perf");
	}

perf_change_seq_pmu_setup_exit:
	return status;
}
