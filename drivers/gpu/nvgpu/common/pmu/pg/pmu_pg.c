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
#include <nvgpu/log.h>
#include <nvgpu/pmuif/nvgpu_gpmu_cmdif.h>
#include <nvgpu/barrier.h>
#include <nvgpu/bug.h>
#include <nvgpu/utils.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/string.h>
#include <nvgpu/engines.h>
#include <nvgpu/gr/gr.h>

/* state transition :
 * OFF => [OFF_ON_PENDING optional] => ON_PENDING => ON => OFF
 * ON => OFF is always synchronized
 */
/* elpg is off */
#define PMU_ELPG_STAT_OFF	0U
/* elpg is on */
#define PMU_ELPG_STAT_ON	1U
/* elpg is off, ALLOW cmd has been sent, wait for ack */
#define PMU_ELPG_STAT_ON_PENDING	2U
/* elpg is on, DISALLOW cmd has been sent, wait for ack */
#define PMU_ELPG_STAT_OFF_PENDING	3U
/* elpg is off, caller has requested on, but ALLOW
 * cmd hasn't been sent due to ENABLE_ALLOW delay
 */
#define PMU_ELPG_STAT_OFF_ON_PENDING	4U

#define PMU_PGENG_GR_BUFFER_IDX_INIT	(0)
#define PMU_PGENG_GR_BUFFER_IDX_ZBC		(1)
#define PMU_PGENG_GR_BUFFER_IDX_FECS	(2)

static void pmu_setup_hw_enable_elpg(struct gk20a *g)
{
	struct nvgpu_pmu *pmu = &g->pmu;

	nvgpu_log_fn(g, " ");

	pmu->pmu_pg.initialized = true;
	nvgpu_pmu_state_change(g, PMU_STATE_STARTED, false);

	if (nvgpu_is_enabled(g, NVGPU_PMU_ZBC_SAVE)) {
		/* Save zbc table after PMU is initialized. */
		pmu->pmu_pg.zbc_ready = true;
		g->ops.pmu.save_zbc(g, 0xf);
	}

	if (g->elpg_enabled) {
		/* Init reg with prod values*/
		if (g->ops.pmu.pmu_setup_elpg != NULL) {
			g->ops.pmu.pmu_setup_elpg(g);
		}
		nvgpu_pmu_enable_elpg(g);
	}

	nvgpu_udelay(50);

	/* Enable AELPG */
	if (g->aelpg_enabled) {
		nvgpu_aelpg_init(g);
		nvgpu_aelpg_init_and_enable(g, PMU_AP_CTRL_ID_GRAPHICS);
	}
}

static void pmu_handle_pg_elpg_msg(struct gk20a *g, struct pmu_msg *msg,
			void *param, u32 handle, u32 status)
{
	struct nvgpu_pmu *pmu = param;
	struct pmu_pg_msg_elpg_msg *elpg_msg = &msg->msg.pg.elpg_msg;

	nvgpu_log_fn(g, " ");

	if (status != 0U) {
		nvgpu_err(g, "ELPG cmd aborted");
		return;
	}

	switch (elpg_msg->msg) {
	case PMU_PG_ELPG_MSG_INIT_ACK:
		nvgpu_pmu_dbg(g, "INIT_PG is ack from PMU, eng - %d",
			elpg_msg->engine_id);
		break;
	case PMU_PG_ELPG_MSG_ALLOW_ACK:
		nvgpu_pmu_dbg(g, "ALLOW is ack from PMU, eng - %d",
			elpg_msg->engine_id);
		if (elpg_msg->engine_id == PMU_PG_ELPG_ENGINE_ID_MS) {
			pmu->mscg_transition_state = PMU_ELPG_STAT_ON;
		} else {
			pmu->pmu_pg.elpg_stat = PMU_ELPG_STAT_ON;
		}
		break;
	case PMU_PG_ELPG_MSG_DISALLOW_ACK:
		nvgpu_pmu_dbg(g, "DISALLOW is ack from PMU, eng - %d",
			elpg_msg->engine_id);

		if (elpg_msg->engine_id == PMU_PG_ELPG_ENGINE_ID_MS) {
			pmu->mscg_transition_state = PMU_ELPG_STAT_OFF;
		} else {
			pmu->pmu_pg.elpg_stat = PMU_ELPG_STAT_OFF;
		}

		if (pmu->pmu_state == PMU_STATE_ELPG_BOOTING) {
			if (g->ops.pmu.pmu_pg_engines_feature_list != NULL &&
				g->ops.pmu.pmu_pg_engines_feature_list(g,
					PMU_PG_ELPG_ENGINE_ID_GRAPHICS) !=
				NVGPU_PMU_GR_FEATURE_MASK_POWER_GATING) {
				pmu->pmu_pg.initialized = true;
				nvgpu_pmu_state_change(g, PMU_STATE_STARTED,
					true);
				WRITE_ONCE(pmu->mscg_stat, PMU_MSCG_DISABLED);
				/* make status visible */
				nvgpu_smp_mb();
			} else {
				nvgpu_pmu_state_change(g, PMU_STATE_ELPG_BOOTED,
					true);
			}
		}
		break;
	default:
		nvgpu_err(g,
			"unsupported ELPG message : 0x%04x", elpg_msg->msg);
		break;
	}
}

/* PG enable/disable */
int nvgpu_pmu_pg_global_enable(struct gk20a *g, bool enable_pg)
{
	int status = 0;

	if (!g->support_ls_pmu) {
		return status;
	}

	if (enable_pg) {
		if (g->ops.pmu.pmu_pg_engines_feature_list != NULL &&
			g->ops.pmu.pmu_pg_engines_feature_list(g,
				PMU_PG_ELPG_ENGINE_ID_GRAPHICS) !=
			NVGPU_PMU_GR_FEATURE_MASK_POWER_GATING) {
			if (g->ops.pmu.pmu_lpwr_enable_pg != NULL) {
				status = g->ops.pmu.pmu_lpwr_enable_pg(g,
						true);
			}
		} else if (g->can_elpg) {
			status = nvgpu_pmu_enable_elpg(g);
		}
	} else {
		if (g->ops.pmu.pmu_pg_engines_feature_list != NULL &&
			g->ops.pmu.pmu_pg_engines_feature_list(g,
				PMU_PG_ELPG_ENGINE_ID_GRAPHICS) !=
			NVGPU_PMU_GR_FEATURE_MASK_POWER_GATING) {
			if (g->ops.pmu.pmu_lpwr_disable_pg != NULL) {
				status = g->ops.pmu.pmu_lpwr_disable_pg(g,
						true);
			}
		} else if (g->can_elpg) {
			status = nvgpu_pmu_disable_elpg(g);
		}
	}

	return status;
}

static int pmu_enable_elpg_locked(struct gk20a *g, u8 pg_engine_id)
{
	struct nvgpu_pmu *pmu = &g->pmu;
	struct pmu_cmd cmd;
	u32 seq;
	int status;
	u64 tmp;

	nvgpu_log_fn(g, " ");

	(void) memset(&cmd, 0, sizeof(struct pmu_cmd));
	cmd.hdr.unit_id = PMU_UNIT_PG;
	tmp = PMU_CMD_HDR_SIZE + sizeof(struct pmu_pg_cmd_elpg_cmd);
	nvgpu_assert(tmp <= U8_MAX);
	cmd.hdr.size = (u8)tmp;
	cmd.cmd.pg.elpg_cmd.cmd_type = PMU_PG_CMD_ID_ELPG_CMD;
	cmd.cmd.pg.elpg_cmd.engine_id = pg_engine_id;
	cmd.cmd.pg.elpg_cmd.cmd = PMU_PG_ELPG_CMD_ALLOW;

	/* no need to wait ack for ELPG enable but set
	* pending to sync with follow up ELPG disable
	*/
	if (pg_engine_id == PMU_PG_ELPG_ENGINE_ID_GRAPHICS) {
		pmu->pmu_pg.elpg_stat = PMU_ELPG_STAT_ON_PENDING;
	} else if (pg_engine_id == PMU_PG_ELPG_ENGINE_ID_MS) {
		pmu->mscg_transition_state = PMU_ELPG_STAT_ON_PENDING;
	}

	nvgpu_pmu_dbg(g, "cmd post PMU_PG_ELPG_CMD_ALLOW");
	status = nvgpu_pmu_cmd_post(g, &cmd, NULL, NULL,
			PMU_COMMAND_QUEUE_HPQ, pmu_handle_pg_elpg_msg,
			pmu, &seq);

	if (status != 0) {
		nvgpu_log_fn(g, "pmu_enable_elpg_locked FAILED err=%d",
			status);
	} else {
		nvgpu_log_fn(g, "done");
	}

	return status;
}

int nvgpu_pmu_enable_elpg(struct gk20a *g)
{
	struct nvgpu_pmu *pmu = &g->pmu;
	struct gr_gk20a *gr = &g->gr;
	u8 pg_engine_id;
	u32 pg_engine_id_list = 0;

	int ret = 0;

	nvgpu_log_fn(g, " ");

	if (!g->support_ls_pmu) {
		return ret;
	}

	nvgpu_mutex_acquire(&pmu->pmu_pg.elpg_mutex);

	pmu->pmu_pg.elpg_refcnt++;
	if (pmu->pmu_pg.elpg_refcnt <= 0) {
		goto exit_unlock;
	}

	/* something is not right if we end up in following code path */
	if (unlikely(pmu->pmu_pg.elpg_refcnt > 1)) {
		nvgpu_warn(g,
			"%s(): possible elpg refcnt mismatch. elpg refcnt=%d",
			__func__, pmu->pmu_pg.elpg_refcnt);
		WARN_ON(true);
	}

	/* do NOT enable elpg until golden ctx is created,
	 * which is related with the ctx that ELPG save and restore.
	*/
	if (unlikely(!gr->ctx_vars.golden_image_initialized)) {
		goto exit_unlock;
	}

	/* return if ELPG is already on or on_pending or off_on_pending */
	if (pmu->pmu_pg.elpg_stat != PMU_ELPG_STAT_OFF) {
		goto exit_unlock;
	}

	if (g->ops.pmu.pmu_pg_supported_engines_list != NULL) {
		pg_engine_id_list = g->ops.pmu.pmu_pg_supported_engines_list(g);
	}

	for (pg_engine_id = PMU_PG_ELPG_ENGINE_ID_GRAPHICS;
		pg_engine_id < PMU_PG_ELPG_ENGINE_ID_INVALID_ENGINE;
		pg_engine_id++) {

		if (pg_engine_id == PMU_PG_ELPG_ENGINE_ID_MS &&
			pmu->mscg_stat == PMU_MSCG_DISABLED) {
			continue;
		}

		if ((BIT32(pg_engine_id) & pg_engine_id_list) != 0U) {
			ret = pmu_enable_elpg_locked(g, pg_engine_id);
		}
	}

exit_unlock:
	nvgpu_mutex_release(&pmu->pmu_pg.elpg_mutex);
	nvgpu_log_fn(g, "done");
	return ret;
}

int nvgpu_pmu_disable_elpg(struct gk20a *g)
{
	struct nvgpu_pmu *pmu = &g->pmu;
	struct pmu_cmd cmd;
	u32 seq;
	int ret = 0;
	u8 pg_engine_id;
	u32 pg_engine_id_list = 0;
	u32 *ptr = NULL;
	u64 tmp;

	nvgpu_log_fn(g, " ");

	if (!g->support_ls_pmu) {
		return ret;
	}

	if (g->ops.pmu.pmu_pg_supported_engines_list != NULL) {
		pg_engine_id_list = g->ops.pmu.pmu_pg_supported_engines_list(g);
	}

	nvgpu_mutex_acquire(&pmu->pmu_pg.elpg_mutex);

	pmu->pmu_pg.elpg_refcnt--;
	if (pmu->pmu_pg.elpg_refcnt > 0) {
		nvgpu_warn(g,
			"%s(): possible elpg refcnt mismatch. elpg refcnt=%d",
			__func__, pmu->pmu_pg.elpg_refcnt);
		WARN_ON(true);
		ret = 0;
		goto exit_unlock;
	}

	/* cancel off_on_pending and return */
	if (pmu->pmu_pg.elpg_stat == PMU_ELPG_STAT_OFF_ON_PENDING) {
		pmu->pmu_pg.elpg_stat = PMU_ELPG_STAT_OFF;
		ret = 0;
		goto exit_reschedule;
	}
	/* wait if on_pending */
	else if (pmu->pmu_pg.elpg_stat == PMU_ELPG_STAT_ON_PENDING) {

		pmu_wait_message_cond(pmu, gk20a_get_gr_idle_timeout(g),
				      &pmu->pmu_pg.elpg_stat, PMU_ELPG_STAT_ON);

		if (pmu->pmu_pg.elpg_stat != PMU_ELPG_STAT_ON) {
			nvgpu_err(g, "ELPG_ALLOW_ACK failed, elpg_stat=%d",
				pmu->pmu_pg.elpg_stat);
			nvgpu_pmu_dump_elpg_stats(pmu);
			nvgpu_pmu_dump_falcon_stats(pmu);
			ret = -EBUSY;
			goto exit_unlock;
		}
	}
	/* return if ELPG is already off */
	else if (pmu->pmu_pg.elpg_stat != PMU_ELPG_STAT_ON) {
		ret = 0;
		goto exit_reschedule;
	}

	for (pg_engine_id = PMU_PG_ELPG_ENGINE_ID_GRAPHICS;
		pg_engine_id < PMU_PG_ELPG_ENGINE_ID_INVALID_ENGINE;
		pg_engine_id++) {

		if (pg_engine_id == PMU_PG_ELPG_ENGINE_ID_MS &&
			pmu->mscg_stat == PMU_MSCG_DISABLED) {
			continue;
		}

		if ((BIT32(pg_engine_id) & pg_engine_id_list) != 0U) {
			(void) memset(&cmd, 0, sizeof(struct pmu_cmd));
			cmd.hdr.unit_id = PMU_UNIT_PG;
			tmp = PMU_CMD_HDR_SIZE +
				sizeof(struct pmu_pg_cmd_elpg_cmd);
			nvgpu_assert(tmp <= U8_MAX);
			cmd.hdr.size = (u8)tmp;
			cmd.cmd.pg.elpg_cmd.cmd_type = PMU_PG_CMD_ID_ELPG_CMD;
			cmd.cmd.pg.elpg_cmd.engine_id = pg_engine_id;
			cmd.cmd.pg.elpg_cmd.cmd = PMU_PG_ELPG_CMD_DISALLOW;

			if (pg_engine_id == PMU_PG_ELPG_ENGINE_ID_GRAPHICS) {
				pmu->pmu_pg.elpg_stat = PMU_ELPG_STAT_OFF_PENDING;
			} else if (pg_engine_id == PMU_PG_ELPG_ENGINE_ID_MS) {
				pmu->mscg_transition_state =
					PMU_ELPG_STAT_OFF_PENDING;
			}
			if (pg_engine_id == PMU_PG_ELPG_ENGINE_ID_GRAPHICS) {
				ptr = &pmu->pmu_pg.elpg_stat;
			} else if (pg_engine_id == PMU_PG_ELPG_ENGINE_ID_MS) {
				ptr = &pmu->mscg_transition_state;
			}

			nvgpu_pmu_dbg(g, "cmd post PMU_PG_ELPG_CMD_DISALLOW");
			ret = nvgpu_pmu_cmd_post(g, &cmd, NULL, NULL,
				PMU_COMMAND_QUEUE_HPQ, pmu_handle_pg_elpg_msg,
				pmu, &seq);
			if (ret != 0) {
				nvgpu_err(g, "PMU_PG_ELPG_CMD_DISALLOW \
					cmd post failed");
				goto exit_unlock;
			}

			pmu_wait_message_cond(pmu,
				gk20a_get_gr_idle_timeout(g),
				ptr, PMU_ELPG_STAT_OFF);
			if (*ptr != PMU_ELPG_STAT_OFF) {
				nvgpu_err(g, "ELPG_DISALLOW_ACK failed");
					nvgpu_pmu_dump_elpg_stats(pmu);
					nvgpu_pmu_dump_falcon_stats(pmu);
				ret = -EBUSY;
				goto exit_unlock;
			}
		}
	}

exit_reschedule:
exit_unlock:
	nvgpu_mutex_release(&pmu->pmu_pg.elpg_mutex);
	nvgpu_log_fn(g, "done");
	return ret;
}

/* PG init */
static void pmu_handle_pg_stat_msg(struct gk20a *g, struct pmu_msg *msg,
			void *param, u32 handle, u32 status)
{
	struct nvgpu_pmu *pmu = param;

	nvgpu_log_fn(g, " ");

	if (status != 0U) {
		nvgpu_err(g, "ELPG cmd aborted");
		return;
	}

	switch (msg->msg.pg.stat.sub_msg_id) {
	case PMU_PG_STAT_MSG_RESP_DMEM_OFFSET:
		nvgpu_pmu_dbg(g, "ALLOC_DMEM_OFFSET is acknowledged from PMU");
		pmu->pmu_pg.stat_dmem_offset[msg->msg.pg.stat.engine_id] =
			msg->msg.pg.stat.data;
		break;
	default:
		nvgpu_err(g, "Invalid msg id:%u",
			msg->msg.pg.stat.sub_msg_id);
		break;
	}
}

static int pmu_pg_init_send(struct gk20a *g, u8 pg_engine_id)
{
	struct nvgpu_pmu *pmu = &g->pmu;
	struct pmu_cmd cmd;
	u32 seq;
	int err = 0;
	u64 tmp;

	nvgpu_log_fn(g, " ");

	g->ops.pmu.pmu_pg_idle_counter_config(g, pg_engine_id);

	if (g->ops.pmu.pmu_pg_init_param != NULL) {
		g->ops.pmu.pmu_pg_init_param(g, pg_engine_id);
	}

	/* init ELPG */
	(void) memset(&cmd, 0, sizeof(struct pmu_cmd));
	cmd.hdr.unit_id = PMU_UNIT_PG;
	tmp = PMU_CMD_HDR_SIZE + sizeof(struct pmu_pg_cmd_elpg_cmd);
	nvgpu_assert(tmp <= U8_MAX);
	cmd.hdr.size = (u8)tmp;
	cmd.cmd.pg.elpg_cmd.cmd_type = PMU_PG_CMD_ID_ELPG_CMD;
	cmd.cmd.pg.elpg_cmd.engine_id = (u8)pg_engine_id;
	cmd.cmd.pg.elpg_cmd.cmd = PMU_PG_ELPG_CMD_INIT;

	nvgpu_pmu_dbg(g, "cmd post PMU_PG_ELPG_CMD_INIT");
	err = nvgpu_pmu_cmd_post(g, &cmd, NULL, NULL, PMU_COMMAND_QUEUE_HPQ,
			pmu_handle_pg_elpg_msg, pmu, &seq);
	if (err != 0) {
		nvgpu_err(g, "PMU_PG_ELPG_CMD_INIT cmd failed\n");
	}

	/* alloc dmem for powergating state log */
	pmu->pmu_pg.stat_dmem_offset[pg_engine_id] = 0;
	(void) memset(&cmd, 0, sizeof(struct pmu_cmd));
	cmd.hdr.unit_id = PMU_UNIT_PG;
	tmp = PMU_CMD_HDR_SIZE + sizeof(struct pmu_pg_cmd_stat);
	nvgpu_assert(tmp <= U8_MAX);
	cmd.hdr.size = (u8)tmp;
	cmd.cmd.pg.stat.cmd_type = PMU_PG_CMD_ID_PG_STAT;
	cmd.cmd.pg.stat.engine_id = pg_engine_id;
	cmd.cmd.pg.stat.sub_cmd_id = PMU_PG_STAT_CMD_ALLOC_DMEM;
	cmd.cmd.pg.stat.data = 0;

	nvgpu_pmu_dbg(g, "cmd post PMU_PG_STAT_CMD_ALLOC_DMEM");
	err = nvgpu_pmu_cmd_post(g, &cmd, NULL, NULL, PMU_COMMAND_QUEUE_LPQ,
			pmu_handle_pg_stat_msg, pmu, &seq);
	if (err != 0) {
		nvgpu_err(g, "PMU_PG_STAT_CMD_ALLOC_DMEM cmd failed\n");
	}

	/* disallow ELPG initially
	 * PMU ucode requires a disallow cmd before allow cmd
	*/
	/* set for wait_event PMU_ELPG_STAT_OFF */
	if (pg_engine_id == PMU_PG_ELPG_ENGINE_ID_GRAPHICS) {
		pmu->pmu_pg.elpg_stat = PMU_ELPG_STAT_OFF;
	} else if (pg_engine_id == PMU_PG_ELPG_ENGINE_ID_MS) {
		pmu->mscg_transition_state = PMU_ELPG_STAT_OFF;
	}
	(void) memset(&cmd, 0, sizeof(struct pmu_cmd));
	cmd.hdr.unit_id = PMU_UNIT_PG;
	tmp = PMU_CMD_HDR_SIZE + sizeof(struct pmu_pg_cmd_elpg_cmd);
	nvgpu_assert(tmp <= U8_MAX);
	cmd.hdr.size = (u8)tmp;
	cmd.cmd.pg.elpg_cmd.cmd_type = PMU_PG_CMD_ID_ELPG_CMD;
	cmd.cmd.pg.elpg_cmd.engine_id = pg_engine_id;
	cmd.cmd.pg.elpg_cmd.cmd = PMU_PG_ELPG_CMD_DISALLOW;

	nvgpu_pmu_dbg(g, "cmd post PMU_PG_ELPG_CMD_DISALLOW");
	err = nvgpu_pmu_cmd_post(g, &cmd, NULL, NULL, PMU_COMMAND_QUEUE_HPQ,
		pmu_handle_pg_elpg_msg, pmu, &seq);
	if (err != 0) {
		nvgpu_err(g, "PMU_PG_ELPG_CMD_DISALLOW cmd failed\n");
	}

	if (g->ops.pmu.pmu_pg_set_sub_feature_mask != NULL) {
		g->ops.pmu.pmu_pg_set_sub_feature_mask(g, pg_engine_id);
	}

	return 0;
}

int nvgpu_pmu_init_powergating(struct gk20a *g)
{
	u8 pg_engine_id;
	u32 pg_engine_id_list = 0;
	struct nvgpu_pmu *pmu = &g->pmu;

	nvgpu_log_fn(g, " ");

	if (g->ops.pmu.pmu_pg_supported_engines_list != NULL) {
		pg_engine_id_list = g->ops.pmu.pmu_pg_supported_engines_list(g);
	}

	nvgpu_gr_wait_initialized(g);

	for (pg_engine_id = PMU_PG_ELPG_ENGINE_ID_GRAPHICS;
		pg_engine_id < PMU_PG_ELPG_ENGINE_ID_INVALID_ENGINE;
			pg_engine_id++) {

		if ((BIT32(pg_engine_id) & pg_engine_id_list) != 0U) {
			if (pmu != NULL &&
			    pmu->pmu_state == PMU_STATE_INIT_RECEIVED) {
				nvgpu_pmu_state_change(g,
					PMU_STATE_ELPG_BOOTING, false);
			}
			pmu_pg_init_send(g, pg_engine_id);
		}
	}

	if (g->ops.pmu.pmu_pg_param_post_init != NULL) {
		g->ops.pmu.pmu_pg_param_post_init(g);
	}

	return 0;
}

static void pmu_handle_pg_buf_config_msg(struct gk20a *g, struct pmu_msg *msg,
			void *param, u32 handle, u32 status)
{
	struct nvgpu_pmu *pmu = param;
	struct pmu_pg_msg_eng_buf_stat *eng_buf_stat =
		&msg->msg.pg.eng_buf_stat;

	nvgpu_log_fn(g, " ");

	nvgpu_pmu_dbg(g,
		"reply PMU_PG_CMD_ID_ENG_BUF_LOAD PMU_PGENG_GR_BUFFER_IDX_FECS");
	if (status != 0U) {
		nvgpu_err(g, "PGENG cmd aborted");
		return;
	}

	pmu->pmu_pg.buf_loaded = (eng_buf_stat->status == PMU_PG_MSG_ENG_BUF_LOADED);
	if ((!pmu->pmu_pg.buf_loaded) &&
		(pmu->pmu_state == PMU_STATE_LOADING_PG_BUF)) {
		nvgpu_err(g, "failed to load PGENG buffer");
	} else {
		nvgpu_pmu_state_change(g, pmu->pmu_state, true);
	}
}

int nvgpu_pmu_init_bind_fecs(struct gk20a *g)
{
	struct nvgpu_pmu *pmu = &g->pmu;
	struct pmu_cmd cmd;
	u32 desc;
	int err = 0;
	u32 gr_engine_id;

	nvgpu_log_fn(g, " ");

	gr_engine_id = nvgpu_engine_get_gr_id(g);

	(void) memset(&cmd, 0, sizeof(struct pmu_cmd));
	cmd.hdr.unit_id = PMU_UNIT_PG;
	cmd.hdr.size = PMU_CMD_HDR_SIZE +
			g->ops.pmu_ver.pg_cmd_eng_buf_load_size(&cmd.cmd.pg);
	g->ops.pmu_ver.pg_cmd_eng_buf_load_set_cmd_type(&cmd.cmd.pg,
			PMU_PG_CMD_ID_ENG_BUF_LOAD);
	g->ops.pmu_ver.pg_cmd_eng_buf_load_set_engine_id(&cmd.cmd.pg,
			gr_engine_id);
	g->ops.pmu_ver.pg_cmd_eng_buf_load_set_buf_idx(&cmd.cmd.pg,
			PMU_PGENG_GR_BUFFER_IDX_FECS);
	g->ops.pmu_ver.pg_cmd_eng_buf_load_set_buf_size(&cmd.cmd.pg,
			pmu->pmu_pg.pg_buf.size);
	g->ops.pmu_ver.pg_cmd_eng_buf_load_set_dma_base(&cmd.cmd.pg,
			u64_lo32(pmu->pmu_pg.pg_buf.gpu_va));
	g->ops.pmu_ver.pg_cmd_eng_buf_load_set_dma_offset(&cmd.cmd.pg,
			(u8)(pmu->pmu_pg.pg_buf.gpu_va & 0xFFU));
	g->ops.pmu_ver.pg_cmd_eng_buf_load_set_dma_idx(&cmd.cmd.pg,
			PMU_DMAIDX_VIRT);

	pmu->pmu_pg.buf_loaded = false;
	nvgpu_pmu_dbg(g, "cmd post PMU_PG_CMD_ID_ENG_BUF_LOAD PMU_PGENG_GR_BUFFER_IDX_FECS");
	nvgpu_pmu_state_change(g, PMU_STATE_LOADING_PG_BUF, false);
	err = nvgpu_pmu_cmd_post(g, &cmd, NULL, NULL, PMU_COMMAND_QUEUE_LPQ,
			pmu_handle_pg_buf_config_msg, pmu, &desc);
	if (err != 0) {
		nvgpu_err(g, "cmd LOAD PMU_PGENG_GR_BUFFER_IDX_FECS failed\n");
	}

	return err;
}

void nvgpu_pmu_setup_hw_load_zbc(struct gk20a *g)
{
	struct nvgpu_pmu *pmu = &g->pmu;
	struct pmu_cmd cmd;
	u32 desc;
	u32 gr_engine_id;
	int err = 0;

	gr_engine_id = nvgpu_engine_get_gr_id(g);

	(void) memset(&cmd, 0, sizeof(struct pmu_cmd));
	cmd.hdr.unit_id = PMU_UNIT_PG;
	cmd.hdr.size = PMU_CMD_HDR_SIZE +
			g->ops.pmu_ver.pg_cmd_eng_buf_load_size(&cmd.cmd.pg);
	g->ops.pmu_ver.pg_cmd_eng_buf_load_set_cmd_type(&cmd.cmd.pg,
			PMU_PG_CMD_ID_ENG_BUF_LOAD);
	g->ops.pmu_ver.pg_cmd_eng_buf_load_set_engine_id(&cmd.cmd.pg,
			gr_engine_id);
	g->ops.pmu_ver.pg_cmd_eng_buf_load_set_buf_idx(&cmd.cmd.pg,
			PMU_PGENG_GR_BUFFER_IDX_ZBC);
	g->ops.pmu_ver.pg_cmd_eng_buf_load_set_buf_size(&cmd.cmd.pg,
			pmu->seq_buf.size);
	g->ops.pmu_ver.pg_cmd_eng_buf_load_set_dma_base(&cmd.cmd.pg,
			u64_lo32(pmu->seq_buf.gpu_va));
	g->ops.pmu_ver.pg_cmd_eng_buf_load_set_dma_offset(&cmd.cmd.pg,
			(u8)(pmu->seq_buf.gpu_va & 0xFFU));
	g->ops.pmu_ver.pg_cmd_eng_buf_load_set_dma_idx(&cmd.cmd.pg,
			PMU_DMAIDX_VIRT);

	pmu->pmu_pg.buf_loaded = false;
	nvgpu_pmu_dbg(g, "cmd post PMU_PG_CMD_ID_ENG_BUF_LOAD PMU_PGENG_GR_BUFFER_IDX_ZBC");
	nvgpu_pmu_state_change(g, PMU_STATE_LOADING_ZBC, false);
	err = nvgpu_pmu_cmd_post(g, &cmd, NULL, NULL, PMU_COMMAND_QUEUE_LPQ,
			pmu_handle_pg_buf_config_msg, pmu, &desc);
	if (err != 0) {
		nvgpu_err(g, "CMD LOAD PMU_PGENG_GR_BUFFER_IDX_ZBC failed\n");
	}
}

/* stats */
int nvgpu_pmu_get_pg_stats(struct gk20a *g, u32 pg_engine_id,
		struct pmu_pg_stats_data *pg_stat_data)
{
	struct nvgpu_pmu *pmu = &g->pmu;
	u32 pg_engine_id_list = 0;
	int err = 0;

	if (!pmu->pmu_pg.initialized) {
		pg_stat_data->ingating_time = 0;
		pg_stat_data->ungating_time = 0;
		pg_stat_data->gating_cnt = 0;
		return 0;
	}

	if (g->ops.pmu.pmu_pg_supported_engines_list != NULL) {
		pg_engine_id_list = g->ops.pmu.pmu_pg_supported_engines_list(g);
	}

	if ((BIT32(pg_engine_id) & pg_engine_id_list) != 0U) {
		err = g->ops.pmu.pmu_elpg_statistics(g, pg_engine_id,
			pg_stat_data);
	}

	return err;
}

int nvgpu_init_task_pg_init(struct gk20a *g)
{
	struct nvgpu_pmu *pmu = &g->pmu;
	char thread_name[64];
	int err = 0;

	nvgpu_log_fn(g, " ");

	nvgpu_cond_init(&pmu->pmu_pg.pg_init.wq);

	(void) snprintf(thread_name, sizeof(thread_name),
				"nvgpu_pg_init_%s", g->name);

	err = nvgpu_thread_create(&pmu->pmu_pg.pg_init.state_task, g,
			nvgpu_pg_init_task, thread_name);
	if (err != 0) {
		nvgpu_err(g, "failed to start nvgpu_pg_init thread");
	}
	return err;
}

void nvgpu_kill_task_pg_init(struct gk20a *g)
{
	struct nvgpu_pmu *pmu = &g->pmu;
	struct nvgpu_timeout timeout;

	/* make sure the pending operations are finished before we continue */
	if (nvgpu_thread_is_running(&pmu->pmu_pg.pg_init.state_task)) {

		/* post PMU_STATE_EXIT to exit PMU state machine loop */
		nvgpu_pmu_state_change(g, PMU_STATE_EXIT, true);

		/* Make thread stop*/
		nvgpu_thread_stop(&pmu->pmu_pg.pg_init.state_task);

		/* wait to confirm thread stopped */
		nvgpu_timeout_init(g, &timeout, 1000, NVGPU_TIMER_RETRY_TIMER);
		do {
			if (!nvgpu_thread_is_running(&pmu->pmu_pg.pg_init.state_task)) {
				break;
			}
			nvgpu_udelay(2);
		} while (nvgpu_timeout_expired_msg(&timeout,
			"timeout - waiting PMU state machine thread stop") == 0);
	} else {
		nvgpu_thread_join(&pmu->pmu_pg.pg_init.state_task);
	}
}

int nvgpu_pg_init_task(void *arg)
{
	struct gk20a *g = (struct gk20a *)arg;
	struct nvgpu_pmu *pmu = &g->pmu;
	struct nvgpu_pg_init *pg_init = &pmu->pmu_pg.pg_init;
	u32 pmu_state = 0;

	nvgpu_log_fn(g, "thread start");

	while (true) {

		NVGPU_COND_WAIT_INTERRUPTIBLE(&pg_init->wq,
			(pg_init->state_change == true), 0U);

		pmu->pmu_pg.pg_init.state_change = false;
		pmu_state = NV_ACCESS_ONCE(pmu->pmu_state);

		if (pmu_state == PMU_STATE_EXIT) {
			nvgpu_pmu_dbg(g, "pmu state exit");
			break;
		}

		switch (pmu_state) {
		case PMU_STATE_INIT_RECEIVED:
			nvgpu_pmu_dbg(g, "pmu starting");
			if (g->can_elpg) {
				nvgpu_pmu_init_powergating(g);
			}
			break;
		case PMU_STATE_ELPG_BOOTED:
			nvgpu_pmu_dbg(g, "elpg booted");
			nvgpu_pmu_init_bind_fecs(g);
			break;
		case PMU_STATE_LOADING_PG_BUF:
			nvgpu_pmu_dbg(g, "loaded pg buf");
			nvgpu_pmu_setup_hw_load_zbc(g);
			break;
		case PMU_STATE_LOADING_ZBC:
			nvgpu_pmu_dbg(g, "loaded zbc");
			pmu_setup_hw_enable_elpg(g);
			nvgpu_pmu_dbg(g, "PMU booted, thread exiting");
			return 0;
		default:
			nvgpu_pmu_dbg(g, "invalid state");
			break;
		}

	}

	while (!nvgpu_thread_should_stop(&pg_init->state_task)) {
		nvgpu_usleep_range(5000, 5100);
	}

	nvgpu_log_fn(g, "thread exit");

	return 0;
}
