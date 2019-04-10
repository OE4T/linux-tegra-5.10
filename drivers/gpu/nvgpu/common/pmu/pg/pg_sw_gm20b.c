/*
 * Copyright (c) 2017-2019, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/gk20a.h>
#include <nvgpu/pmu/lpwr.h>
#include <nvgpu/pmu/cmd.h>
#include <nvgpu/bug.h>

#include "pg_sw_gm20b.h"

u32 gm20b_pmu_pg_engines_list(struct gk20a *g)
{
	return BIT32(PMU_PG_ELPG_ENGINE_ID_GRAPHICS);
}

u32 gm20b_pmu_pg_feature_list(struct gk20a *g, u32 pg_engine_id)
{
	if (pg_engine_id == PMU_PG_ELPG_ENGINE_ID_GRAPHICS) {
		return NVGPU_PMU_GR_FEATURE_MASK_POWER_GATING;
	}

	return 0;
}

static void pmu_handle_zbc_msg(struct gk20a *g, struct pmu_msg *msg,
	void *param, u32 status)
{
	struct nvgpu_pmu *pmu = param;
	nvgpu_pmu_dbg(g, "reply ZBC_TABLE_UPDATE");
	pmu->pg->zbc_save_done = true;
}

void gm20b_pmu_save_zbc(struct gk20a *g, u32 entries)
{
	struct nvgpu_pmu *pmu = &g->pmu;
	struct pmu_cmd cmd;
	size_t tmp_size;
	int err = 0;

	if (!nvgpu_pmu_get_fw_ready(g, pmu) ||
		(entries == 0U) || !pmu->pg->zbc_ready) {
		return;
	}

	(void) memset(&cmd, 0, sizeof(struct pmu_cmd));
	cmd.hdr.unit_id = PMU_UNIT_PG;
	tmp_size = PMU_CMD_HDR_SIZE + sizeof(struct pmu_zbc_cmd);
	nvgpu_assert(tmp_size <= (size_t)U8_MAX);
	cmd.hdr.size = (u8)tmp_size;
	cmd.cmd.zbc.cmd_type = g->pmu_ver_cmd_id_zbc_table_update;
	cmd.cmd.zbc.entry_mask = ZBC_MASK(entries);

	pmu->pg->zbc_save_done = false;

	nvgpu_pmu_dbg(g, "cmd post ZBC_TABLE_UPDATE");
	err = nvgpu_pmu_cmd_post(g, &cmd, NULL, PMU_COMMAND_QUEUE_HPQ,
			pmu_handle_zbc_msg, pmu);
	if (err != 0) {
		nvgpu_err(g, "ZBC_TABLE_UPDATE cmd post failed");
		return;
	}
	pmu_wait_message_cond(pmu, nvgpu_get_poll_timeout(g),
			&pmu->pg->zbc_save_done, 1);
	if (!pmu->pg->zbc_save_done) {
		nvgpu_err(g, "ZBC save timeout");
	}
}

int gm20b_pmu_elpg_statistics(struct gk20a *g, u32 pg_engine_id,
		struct pmu_pg_stats_data *pg_stat_data)
{
	struct nvgpu_pmu *pmu = &g->pmu;
	struct pmu_pg_stats stats;
	int err;

	err = nvgpu_falcon_copy_from_dmem(&pmu->flcn,
		pmu->pg->stat_dmem_offset[pg_engine_id],
		(u8 *)&stats, (u32)sizeof(struct pmu_pg_stats), 0);
	if (err != 0) {
		nvgpu_err(g, "PMU falcon DMEM copy failed");
		return err;
	}

	pg_stat_data->ingating_time = stats.pg_ingating_time_us;
	pg_stat_data->ungating_time = stats.pg_ungating_time_us;
	pg_stat_data->gating_cnt = stats.pg_gating_cnt;
	pg_stat_data->avg_entry_latency_us = stats.pg_avg_entry_time_us;
	pg_stat_data->avg_exit_latency_us = stats.pg_avg_exit_time_us;

	return err;
}
