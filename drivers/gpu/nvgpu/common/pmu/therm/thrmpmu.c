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
#include <nvgpu/gk20a.h>
#include <nvgpu/boardobjgrp.h>
#include <nvgpu/boardobjgrp_e32.h>
#include <nvgpu/pmu/therm.h>
#include <nvgpu/pmu/pmuif/nvgpu_cmdif.h>
#include <nvgpu/pmu/cmd.h>

#include "thrmpmu.h"
#include "thrm.h"

struct therm_pmucmdhandler_params {
	struct nv_pmu_therm_rpc *prpccall;
	u32 success;
};

static void therm_pmucmdhandler(struct gk20a *g, struct pmu_msg *msg,
			void *param, u32 status)
{
	struct therm_pmucmdhandler_params *phandlerparams =
		(struct therm_pmucmdhandler_params *)param;

	if (msg->msg.therm.msg_type != NV_PMU_THERM_MSG_ID_RPC) {
		nvgpu_err(g, "unknow msg %x",
			msg->msg.pmgr.msg_type);
		return;
	}

	if (!phandlerparams->prpccall->b_supported) {
		nvgpu_err(g, "RPC msg %x failed",
			msg->msg.pmgr.msg_type);
	} else {
		phandlerparams->success = 1;
	}
}

int therm_send_pmgr_tables_to_pmu(struct gk20a *g)
{
	int status = 0;
	struct boardobjgrp *pboardobjgrp = NULL;

	if (!BOARDOBJGRP_IS_EMPTY(&g->pmu.therm_pmu->therm_deviceobjs.super.super)) {
		pboardobjgrp = &g->pmu.therm_pmu->therm_deviceobjs.super.super;
		status = pboardobjgrp->pmuinithandle(g, pboardobjgrp);
		if (status != 0) {
			nvgpu_err(g,
				"therm_send_pmgr_tables_to_pmu - therm_device failed %x",
				status);
			goto exit;
		}
	}

	if (!BOARDOBJGRP_IS_EMPTY(
			&g->pmu.therm_pmu->therm_channelobjs.super.super)) {
		pboardobjgrp = &g->pmu.therm_pmu->therm_channelobjs.super.super;
		status = pboardobjgrp->pmuinithandle(g, pboardobjgrp);
		if (status != 0) {
			nvgpu_err(g,
				"therm_send_pmgr_tables_to_pmu - therm_channel failed %x",
				status);
			goto exit;
		}
	}

exit:
	return status;
}

static int therm_pmu_cmd_post(struct gk20a *g, struct pmu_cmd *cmd,
		struct pmu_payload *payload,
		u32 queue_id, pmu_callback callback, void *cb_param)
{
	int status;
	struct therm_pmucmdhandler_params *handlerparams = NULL;

	status = nvgpu_pmu_cmd_post(g, cmd, payload,
				queue_id,
				callback,
				cb_param);
	if (status != 0) {
		nvgpu_err(g,
			"unable to post therm cmd for unit %x cmd id %x size %x",
			cmd->hdr.unit_id, cmd->cmd.therm.cmd_type, cmd->hdr.size);
		goto exit;
	}

	if (cb_param != NULL) {
		handlerparams = (struct therm_pmucmdhandler_params*)cb_param;

		pmu_wait_message_cond(&g->pmu,
				nvgpu_get_poll_timeout(g),
				&handlerparams->success, 1);

		if (handlerparams->success == 0U) {
			nvgpu_err(g, "could not process cmd");
			status = -ETIMEDOUT;
			goto exit;
		}
	}

exit:
	return status;
}

static int therm_set_warn_temp_limit(struct gk20a *g)
{
	struct pmu_cmd cmd;
	struct pmu_payload payload;
	struct nv_pmu_therm_rpc rpccall;
	struct therm_pmucmdhandler_params handlerparams;

	(void) memset(&payload, 0, sizeof(struct pmu_payload));
	(void) memset(&cmd, 0, sizeof(struct pmu_cmd));
	(void) memset(&rpccall, 0, sizeof(struct nv_pmu_therm_rpc));
	(void) memset(&handlerparams, 0,
		sizeof(struct therm_pmucmdhandler_params));

	rpccall.function = NV_PMU_THERM_RPC_ID_SLCT_EVENT_TEMP_TH_SET;
	rpccall.params.slct_event_temp_th_set.event_id =
		NV_PMU_THERM_EVENT_THERMAL_1;
	rpccall.params.slct_event_temp_th_set.temp_threshold = g->curr_warn_temp;
	rpccall.b_supported = false;

	cmd.hdr.unit_id = PMU_UNIT_THERM;
	cmd.hdr.size = ((u32)sizeof(struct nv_pmu_therm_cmd_rpc) +
			(u32)sizeof(struct pmu_hdr));
	cmd.cmd.therm.cmd_type = NV_PMU_THERM_CMD_ID_RPC;

	payload.in.buf = (u8 *)&rpccall;
	payload.in.size = (u32)sizeof(struct nv_pmu_therm_rpc);
	payload.in.fb_size = PMU_CMD_SUBMIT_PAYLOAD_PARAMS_FB_SIZE_UNUSED;
	payload.in.offset = NV_PMU_THERM_CMD_RPC_ALLOC_OFFSET;

	payload.out.buf = (u8 *)&rpccall;
	payload.out.size = (u32)sizeof(struct nv_pmu_therm_rpc);
	payload.out.fb_size = PMU_CMD_SUBMIT_PAYLOAD_PARAMS_FB_SIZE_UNUSED;
	payload.out.offset = NV_PMU_CLK_MSG_RPC_ALLOC_OFFSET;

	/* Setup the handler params to communicate back results.*/
	handlerparams.success = 0;
	handlerparams.prpccall = &rpccall;

	return therm_pmu_cmd_post(g, &cmd, &payload,
				PMU_COMMAND_QUEUE_LPQ,
				therm_pmucmdhandler,
				(void *)&handlerparams);
}

static int therm_enable_slct_notification_request(struct gk20a *g)
{
	struct pmu_cmd cmd = { {0} };

	cmd.hdr.unit_id = PMU_UNIT_THERM;
	cmd.hdr.size = ((u32)sizeof(struct nv_pmu_therm_cmd_hw_slowdown_notification) +
		(u32)sizeof(struct pmu_hdr));

	cmd.cmd.therm.cmd_type = NV_PMU_THERM_CMD_ID_HW_SLOWDOWN_NOTIFICATION;
	cmd.cmd.therm.hw_slct_notification.request =
		NV_RM_PMU_THERM_HW_SLOWDOWN_NOTIFICATION_REQUEST_ENABLE;

	return therm_pmu_cmd_post(g, &cmd, NULL,
				PMU_COMMAND_QUEUE_LPQ,
				NULL,
				NULL);
}

static int therm_send_slct_configuration_to_pmu(struct gk20a *g)
{
	struct pmu_cmd cmd;
	struct pmu_payload payload;
	struct nv_pmu_therm_rpc rpccall;
	struct therm_pmucmdhandler_params handlerparams;

	(void) memset(&payload, 0, sizeof(struct pmu_payload));
	(void) memset(&cmd, 0, sizeof(struct pmu_cmd));
	(void) memset(&rpccall, 0, sizeof(struct nv_pmu_therm_rpc));
	(void) memset(&handlerparams, 0,
		sizeof(struct therm_pmucmdhandler_params));

	rpccall.function = NV_PMU_THERM_RPC_ID_SLCT;
	rpccall.params.slct.mask_enabled =
		BIT32(NV_PMU_THERM_EVENT_THERMAL_1);
	rpccall.b_supported = false;

	cmd.hdr.unit_id = PMU_UNIT_THERM;
	cmd.hdr.size = ((u32)sizeof(struct nv_pmu_therm_cmd_rpc) +
			(u32)sizeof(struct pmu_hdr));
	cmd.cmd.therm.cmd_type = NV_PMU_THERM_CMD_ID_RPC;

	payload.in.buf = (u8 *)&rpccall;
	payload.in.size = (u32)sizeof(struct nv_pmu_therm_rpc);
	payload.in.fb_size = PMU_CMD_SUBMIT_PAYLOAD_PARAMS_FB_SIZE_UNUSED;
	payload.in.offset = NV_PMU_THERM_CMD_RPC_ALLOC_OFFSET;

	payload.out.buf = (u8 *)&rpccall;
	payload.out.size = (u32)sizeof(struct nv_pmu_therm_rpc);
	payload.out.fb_size = PMU_CMD_SUBMIT_PAYLOAD_PARAMS_FB_SIZE_UNUSED;
	payload.out.offset = NV_PMU_CLK_MSG_RPC_ALLOC_OFFSET;

	/* Setup the handler params to communicate back results.*/
	handlerparams.success = 0;
	handlerparams.prpccall = &rpccall;

	return therm_pmu_cmd_post(g, &cmd, &payload,
				PMU_COMMAND_QUEUE_LPQ,
				therm_pmucmdhandler,
				(void *)&handlerparams);
}

int nvgpu_therm_configure_therm_alert(struct gk20a *g, struct nvgpu_pmu *pmu)
{
	int status;

	status = therm_enable_slct_notification_request(g);
	if (status != 0) {
		nvgpu_err(g,
			"therm_enable_slct_notification_request-failed %d",
			status);
		goto exit;
	}

	status = therm_send_slct_configuration_to_pmu(g);
	if (status != 0) {
		nvgpu_err(g,
			"therm_send_slct_configuration_to_pmu-failed %d",
			status);
		goto exit;
	}

	status = therm_set_warn_temp_limit(g);
	if (status != 0) {
		nvgpu_err(g,
			"therm_set_warn_temp_limit-failed %d",
			status);
		goto exit;
	}
exit:
	return status;
}

void nvgpu_pmu_therm_rpc_handler(struct gk20a *g, struct nvgpu_pmu *pmu,
		struct nv_pmu_rpc_header *rpc)
{
	switch (rpc->function) {
	case NV_PMU_RPC_ID_THERM_BOARD_OBJ_GRP_CMD:
		nvgpu_pmu_dbg(g,
			"reply NV_PMU_RPC_ID_THERM_BOARD_OBJ_GRP_CMD");
		break;
	default:
		nvgpu_pmu_dbg(g, "reply PMU_UNIT_THERM");
		break;
	}
}
