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
#include <nvgpu/io.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/bug.h>

#include "common/pmu/pmu_gp10b.h"
#include "pmu_gk20a.h"

#include <nvgpu/hw/gp10b/hw_pwr_gp10b.h>

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
	size_t tmp_size;

	nvgpu_log_fn(g, " ");

	nvgpu_pmu_dbg(g, "wprinit status = %x", g->pmu_lsf_pmu_wpr_init_done);
	if (g->pmu_lsf_pmu_wpr_init_done) {
		/* send message to load FECS falcon */
		(void) memset(&cmd, 0, sizeof(struct pmu_cmd));
		cmd.hdr.unit_id = PMU_UNIT_ACR;
		tmp_size = PMU_CMD_HDR_SIZE +
			sizeof(struct pmu_acr_cmd_bootstrap_multiple_falcons);
		nvgpu_assert(tmp_size <= (size_t)U8_MAX);
		cmd.hdr.size = (u8)tmp_size;
		cmd.cmd.acr.boot_falcons.cmd_type =
		  PMU_ACR_CMD_ID_BOOTSTRAP_MULTIPLE_FALCONS;
		cmd.cmd.acr.boot_falcons.flags = flags;
		cmd.cmd.acr.boot_falcons.falconidmask =
				falconidmask;
		cmd.cmd.acr.boot_falcons.usevamask = 0;
		cmd.cmd.acr.boot_falcons.wprvirtualbase.lo = 0x0U;
		cmd.cmd.acr.boot_falcons.wprvirtualbase.hi = 0x0U;
		nvgpu_pmu_dbg(g, "PMU_ACR_CMD_ID_BOOTSTRAP_MULTIPLE_FALCONS:%x",
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
				nvgpu_get_poll_timeout(g),
				&g->pmu_lsf_pmu_wpr_init_done, 1);
		/* check again if it still not ready indicate an error */
		if (!g->pmu_lsf_pmu_wpr_init_done) {
			nvgpu_err(g, "PMU not ready to load LSF");
			return -ETIMEDOUT;
		}
	}
	/* load falcon(s) */
	gp10b_pmu_load_multiple_falcons(g, falconidmask, flags);
	nvgpu_assert(falconidmask <= U8_MAX);
	pmu_wait_message_cond(&g->pmu,
			nvgpu_get_poll_timeout(g),
			&g->pmu_lsf_loaded_falcon_id, (u8)falconidmask);
	if (g->pmu_lsf_loaded_falcon_id != falconidmask) {
		return -ETIMEDOUT;
	}
	return 0;
}

int gp10b_pmu_setup_elpg(struct gk20a *g)
{
	int ret = 0;
	size_t reg_writes;
	size_t index;

	nvgpu_log_fn(g, " ");

	if (g->can_elpg && g->elpg_enabled) {
		reg_writes = ARRAY_SIZE(_pginitseq_gp10b);
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

bool gp10b_is_pmu_supported(struct gk20a *g)
{
	return true;
}
