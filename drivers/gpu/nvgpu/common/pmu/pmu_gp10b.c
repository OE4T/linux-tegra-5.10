/*
 * GP10B PMU
 *
 * Copyright (c) 2015-2019, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/log.h>
#include <nvgpu/fuse.h>
#include <nvgpu/enabled.h>
#include <nvgpu/io.h>
#include <nvgpu/gk20a.h>

#include "acr_gm20b.h"
#include "pmu_gk20a.h"
#include "pmu_gm20b.h"
#include "pmu_gp10b.h"

#include <nvgpu/hw/gp10b/hw_pwr_gp10b.h>

#define gp10b_dbg_pmu(g, fmt, arg...) \
	nvgpu_log(g, gpu_dbg_pmu, fmt, ##arg)

/* PROD settings for ELPG sequencing registers*/
static struct pg_init_sequence_list _pginitseq_gp10b[] = {
		{0x0010ab10U, 0x0000868BU} ,
		{0x0010e118U, 0x8590848FU} ,
		{0x0010e000U, 0x0U} ,
		{0x0010e06cU, 0x000000A3U} ,
		{0x0010e06cU, 0x000000A0U} ,
		{0x0010e06cU, 0x00000095U} ,
		{0x0010e06cU, 0x000000A6U} ,
		{0x0010e06cU, 0x0000008CU} ,
		{0x0010e06cU, 0x00000080U} ,
		{0x0010e06cU, 0x00000081U} ,
		{0x0010e06cU, 0x00000087U} ,
		{0x0010e06cU, 0x00000088U} ,
		{0x0010e06cU, 0x0000008DU} ,
		{0x0010e06cU, 0x00000082U} ,
		{0x0010e06cU, 0x00000083U} ,
		{0x0010e06cU, 0x00000089U} ,
		{0x0010e06cU, 0x0000008AU} ,
		{0x0010e06cU, 0x000000A2U} ,
		{0x0010e06cU, 0x00000097U} ,
		{0x0010e06cU, 0x00000092U} ,
		{0x0010e06cU, 0x00000099U} ,
		{0x0010e06cU, 0x0000009BU} ,
		{0x0010e06cU, 0x0000009DU} ,
		{0x0010e06cU, 0x0000009FU} ,
		{0x0010e06cU, 0x000000A1U} ,
		{0x0010e06cU, 0x00000096U} ,
		{0x0010e06cU, 0x00000091U} ,
		{0x0010e06cU, 0x00000098U} ,
		{0x0010e06cU, 0x0000009AU} ,
		{0x0010e06cU, 0x0000009CU} ,
		{0x0010e06cU, 0x0000009EU} ,
		{0x0010ab14U, 0x00000000U} ,
		{0x0010e024U, 0x00000000U} ,
		{0x0010e028U, 0x00000000U} ,
		{0x0010e11cU, 0x00000000U} ,
		{0x0010ab1cU, 0x140B0BFFU} ,
		{0x0010e020U, 0x0E2626FFU} ,
		{0x0010e124U, 0x251010FFU} ,
		{0x0010ab20U, 0x89abcdefU} ,
		{0x0010ab24U, 0x00000000U} ,
		{0x0010e02cU, 0x89abcdefU} ,
		{0x0010e030U, 0x00000000U} ,
		{0x0010e128U, 0x89abcdefU} ,
		{0x0010e12cU, 0x00000000U} ,
		{0x0010ab28U, 0x7FFFFFFFU} ,
		{0x0010ab2cU, 0x70000000U} ,
		{0x0010e034U, 0x7FFFFFFFU} ,
		{0x0010e038U, 0x70000000U} ,
		{0x0010e130U, 0x7FFFFFFFU} ,
		{0x0010e134U, 0x70000000U} ,
		{0x0010ab30U, 0x00000000U} ,
		{0x0010ab34U, 0x00000001U} ,
		{0x00020004U, 0x00000000U} ,
		{0x0010e138U, 0x00000000U} ,
		{0x0010e040U, 0x00000000U} ,
		{0x0010e168U, 0x00000000U} ,
		{0x0010e114U, 0x0000A5A4U} ,
		{0x0010e110U, 0x00000000U} ,
		{0x0010e10cU, 0x8590848FU} ,
		{0x0010e05cU, 0x00000000U} ,
		{0x0010e044U, 0x00000000U} ,
		{0x0010a644U, 0x0000868BU} ,
		{0x0010a648U, 0x00000000U} ,
		{0x0010a64cU, 0x00829493U} ,
		{0x0010a650U, 0x00000000U} ,
		{0x0010e000U, 0x0U} ,
		{0x0010e068U, 0x000000A3U} ,
		{0x0010e068U, 0x000000A0U} ,
		{0x0010e068U, 0x00000095U} ,
		{0x0010e068U, 0x000000A6U} ,
		{0x0010e068U, 0x0000008CU} ,
		{0x0010e068U, 0x00000080U} ,
		{0x0010e068U, 0x00000081U} ,
		{0x0010e068U, 0x00000087U} ,
		{0x0010e068U, 0x00000088U} ,
		{0x0010e068U, 0x0000008DU} ,
		{0x0010e068U, 0x00000082U} ,
		{0x0010e068U, 0x00000083U} ,
		{0x0010e068U, 0x00000089U} ,
		{0x0010e068U, 0x0000008AU} ,
		{0x0010e068U, 0x000000A2U} ,
		{0x0010e068U, 0x00000097U} ,
		{0x0010e068U, 0x00000092U} ,
		{0x0010e068U, 0x00000099U} ,
		{0x0010e068U, 0x0000009BU} ,
		{0x0010e068U, 0x0000009DU} ,
		{0x0010e068U, 0x0000009FU} ,
		{0x0010e068U, 0x000000A1U} ,
		{0x0010e068U, 0x00000096U} ,
		{0x0010e068U, 0x00000091U} ,
		{0x0010e068U, 0x00000098U} ,
		{0x0010e068U, 0x0000009AU} ,
		{0x0010e068U, 0x0000009CU} ,
		{0x0010e068U, 0x0000009EU} ,
		{0x0010e000U, 0x0U} ,
		{0x0010e004U, 0x0000008EU},
};

static void gp10b_pmu_load_multiple_falcons(struct gk20a *g, u32 falconidmask,
					 u32 flags)
{
	struct nvgpu_pmu *pmu = &g->pmu;
	struct pmu_cmd cmd;
	u32 seq;

	nvgpu_log_fn(g, " ");

	gp10b_dbg_pmu(g, "wprinit status = %x\n", g->pmu_lsf_pmu_wpr_init_done);
	if (g->pmu_lsf_pmu_wpr_init_done) {
		/* send message to load FECS falcon */
		(void) memset(&cmd, 0, sizeof(struct pmu_cmd));
		cmd.hdr.unit_id = PMU_UNIT_ACR;
		cmd.hdr.size = PMU_CMD_HDR_SIZE +
		  sizeof(struct pmu_acr_cmd_bootstrap_multiple_falcons);
		cmd.cmd.acr.boot_falcons.cmd_type =
		  PMU_ACR_CMD_ID_BOOTSTRAP_MULTIPLE_FALCONS;
		cmd.cmd.acr.boot_falcons.flags = flags;
		cmd.cmd.acr.boot_falcons.falconidmask =
				falconidmask;
		cmd.cmd.acr.boot_falcons.usevamask = 0;
		cmd.cmd.acr.boot_falcons.wprvirtualbase.lo = 0x0U;
		cmd.cmd.acr.boot_falcons.wprvirtualbase.hi = 0x0U;
		gp10b_dbg_pmu(g, "PMU_ACR_CMD_ID_BOOTSTRAP_MULTIPLE_FALCONS:%x\n",
				falconidmask);
		nvgpu_pmu_cmd_post(g, &cmd, NULL, NULL, PMU_COMMAND_QUEUE_HPQ,
				pmu_handle_fecs_boot_acr_msg, pmu, &seq);
	}

	nvgpu_log_fn(g, "done");
	return;
}

int gp10b_load_falcon_ucode(struct gk20a *g, u32 falconidmask)
{
	u32 flags = PMU_ACR_CMD_BOOTSTRAP_FALCON_FLAGS_RESET_YES;

	/* GM20B PMU supports loading FECS and GPCCS only */
	if (falconidmask == 0U) {
		return -EINVAL;
	}
	if ((falconidmask &
		~(BIT32(FALCON_ID_FECS) |
		  BIT32(FALCON_ID_GPCCS))) != 0U) {
		return -EINVAL;
	}
	g->pmu_lsf_loaded_falcon_id = 0;
	/* check whether pmu is ready to bootstrap lsf if not wait for it */
	if (!g->pmu_lsf_pmu_wpr_init_done) {
		pmu_wait_message_cond(&g->pmu,
				gk20a_get_gr_idle_timeout(g),
				&g->pmu_lsf_pmu_wpr_init_done, 1);
		/* check again if it still not ready indicate an error */
		if (!g->pmu_lsf_pmu_wpr_init_done) {
			nvgpu_err(g, "PMU not ready to load LSF");
			return -ETIMEDOUT;
		}
	}
	/* load falcon(s) */
	gp10b_pmu_load_multiple_falcons(g, falconidmask, flags);
	pmu_wait_message_cond(&g->pmu,
			gk20a_get_gr_idle_timeout(g),
			&g->pmu_lsf_loaded_falcon_id, falconidmask);
	if (g->pmu_lsf_loaded_falcon_id != falconidmask) {
		return -ETIMEDOUT;
	}
	return 0;
}

static void pmu_handle_gr_param_msg(struct gk20a *g, struct pmu_msg *msg,
			void *param, u32 handle, u32 status)
{
	nvgpu_log_fn(g, " ");

	if (status != 0U) {
		nvgpu_err(g, "GR PARAM cmd aborted");
		/* TBD: disable ELPG */
		return;
	}

	gp10b_dbg_pmu(g, "GR PARAM is acknowledged from PMU %x \n",
			msg->msg.pg.msg_type);

	return;
}

int gp10b_pg_gr_init(struct gk20a *g, u32 pg_engine_id)
{
	struct nvgpu_pmu *pmu = &g->pmu;
	struct pmu_cmd cmd;
	u32 seq;

	if (pg_engine_id == PMU_PG_ELPG_ENGINE_ID_GRAPHICS) {
		(void) memset(&cmd, 0, sizeof(struct pmu_cmd));
		cmd.hdr.unit_id = PMU_UNIT_PG;
		cmd.hdr.size = PMU_CMD_HDR_SIZE +
				sizeof(struct pmu_pg_cmd_gr_init_param_v2);
		cmd.cmd.pg.gr_init_param_v2.cmd_type =
				PMU_PG_CMD_ID_PG_PARAM;
		cmd.cmd.pg.gr_init_param_v2.sub_cmd_id =
				PMU_PG_PARAM_CMD_GR_INIT_PARAM;
		cmd.cmd.pg.gr_init_param_v2.featuremask =
				NVGPU_PMU_GR_FEATURE_MASK_POWER_GATING;
		cmd.cmd.pg.gr_init_param_v2.ldiv_slowdown_factor =
				g->ldiv_slowdown_factor;

		gp10b_dbg_pmu(g, "cmd post PMU_PG_CMD_ID_PG_PARAM ");
		nvgpu_pmu_cmd_post(g, &cmd, NULL, NULL, PMU_COMMAND_QUEUE_HPQ,
				pmu_handle_gr_param_msg, pmu, &seq);

	} else {
		return -EINVAL;
	}

	return 0;
}

void gp10b_pmu_elpg_statistics(struct gk20a *g, u32 pg_engine_id,
		struct pmu_pg_stats_data *pg_stat_data)
{
	struct nvgpu_pmu *pmu = &g->pmu;
	struct pmu_pg_stats_v1 stats;

	nvgpu_falcon_copy_from_dmem(pmu->flcn,
		pmu->stat_dmem_offset[pg_engine_id],
		(u8 *)&stats, (u32)sizeof(struct pmu_pg_stats_v1), 0);

	pg_stat_data->ingating_time = stats.total_sleep_timeus;
	pg_stat_data->ungating_time = stats.total_nonsleep_timeus;
	pg_stat_data->gating_cnt = stats.entry_count;
	pg_stat_data->avg_entry_latency_us = stats.entrylatency_avgus;
	pg_stat_data->avg_exit_latency_us = stats.exitlatency_avgus;
}

int gp10b_pmu_setup_elpg(struct gk20a *g)
{
	int ret = 0;
	u32 reg_writes;
	u32 index;

	nvgpu_log_fn(g, " ");

	if (g->elpg_enabled) {
		reg_writes = ((sizeof(_pginitseq_gp10b) /
				sizeof((_pginitseq_gp10b)[0])));
		/* Initialize registers with production values*/
		for (index = 0; index < reg_writes; index++) {
			gk20a_writel(g, _pginitseq_gp10b[index].regaddr,
				_pginitseq_gp10b[index].writeval);
		}
	}

	nvgpu_log_fn(g, "done");
	return ret;
}

void gp10b_write_dmatrfbase(struct gk20a *g, u32 addr)
{
	gk20a_writel(g, pwr_falcon_dmatrfbase_r(),
				addr);
	gk20a_writel(g, pwr_falcon_dmatrfbase1_r(),
				0x0U);
}

bool gp10b_is_lazy_bootstrap(u32 falcon_id)
{
	bool enable_status = false;

	switch (falcon_id) {
	case FALCON_ID_FECS:
		enable_status = false;
		break;
	case FALCON_ID_GPCCS:
		enable_status = true;
		break;
	default:
		break;
	}

	return enable_status;
}

bool gp10b_is_priv_load(u32 falcon_id)
{
	bool enable_status = false;

	switch (falcon_id) {
	case FALCON_ID_FECS:
		enable_status = false;
		break;
	case FALCON_ID_GPCCS:
		enable_status = true;
		break;
	default:
		break;
	}

	return enable_status;
}

bool gp10b_is_pmu_supported(struct gk20a *g)
{
	return true;
}
