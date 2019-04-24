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

#include <nvgpu/bios.h>
#include <nvgpu/bug.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/boardobjgrp.h>
#include <nvgpu/boardobjgrp_e32.h>
#include <nvgpu/boardobjgrpmask.h>
#include <nvgpu/boardobj.h>
#include <nvgpu/string.h>
#include <nvgpu/pmu/pmuif/ctrlclk.h>
#include <nvgpu/pmu/pmuif/ctrlvolt.h>
#include <nvgpu/pmu/clk/clk.h>
#include <nvgpu/pmu/clk/clk_fll.h>
#include <nvgpu/pmu/clk/clk_domain.h>
#include <nvgpu/pmu/clk/clk_freq_controller.h>
#include <nvgpu/pmu/cmd.h>

#include "clk_freq_controller.h"

struct clk_freq_ctlr_rpc_pmucmdhandler_params {
	struct nv_pmu_clk_rpc *prpccall;
	u32 success;
};

static void clk_freq_ctlr_rpc_pmucmdhandler(struct gk20a *g,
		struct pmu_msg *msg, void *param, u32 status)
{
	struct clk_freq_ctlr_rpc_pmucmdhandler_params *phandlerparams =
		(struct clk_freq_ctlr_rpc_pmucmdhandler_params *)param;

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

static int clk_freq_controller_pmudatainit_super(struct gk20a *g,
	struct boardobj *board_obj_ptr,
	struct nv_pmu_boardobj *ppmudata)
{
	struct nv_pmu_clk_clk_freq_controller_boardobj_set *pfreq_cntlr_set;
	struct clk_freq_controller *pfreq_cntlr;
	int status = 0;

	status = boardobj_pmudatainit_super(g, board_obj_ptr, ppmudata);
	if (status != 0) {
		return status;
	}

	pfreq_cntlr_set =
		(struct nv_pmu_clk_clk_freq_controller_boardobj_set *)ppmudata;
	pfreq_cntlr    = (struct clk_freq_controller *)board_obj_ptr;

	pfreq_cntlr_set->controller_id = pfreq_cntlr->controller_id;
	pfreq_cntlr_set->clk_domain = pfreq_cntlr->clk_domain;
	pfreq_cntlr_set->parts_freq_mode = pfreq_cntlr->parts_freq_mode;
	pfreq_cntlr_set->bdisable = pfreq_cntlr->bdisable;
	pfreq_cntlr_set->freq_cap_noise_unaware_vmin_above =
		pfreq_cntlr->freq_cap_noise_unaware_vmin_above;
	pfreq_cntlr_set->freq_cap_noise_unaware_vmin_below =
		pfreq_cntlr->freq_cap_noise_unaware_vmin_below;
	pfreq_cntlr_set->freq_hyst_pos_mhz = pfreq_cntlr->freq_hyst_pos_mhz;
	pfreq_cntlr_set->freq_hyst_neg_mhz = pfreq_cntlr->freq_hyst_neg_mhz;

	return status;
}

static int clk_freq_controller_pmudatainit_pi(struct gk20a *g,
	struct boardobj *board_obj_ptr,
	struct nv_pmu_boardobj *ppmudata)
{
	struct nv_pmu_clk_clk_freq_controller_pi_boardobj_set
		*pfreq_cntlr_pi_set;
	struct clk_freq_controller_pi *pfreq_cntlr_pi;
	int status = 0;

	status = clk_freq_controller_pmudatainit_super(g,
		board_obj_ptr, ppmudata);
	if (status != 0) {
		return -1;
	}

	pfreq_cntlr_pi_set =
		(struct nv_pmu_clk_clk_freq_controller_pi_boardobj_set *)
		ppmudata;
	pfreq_cntlr_pi = (struct clk_freq_controller_pi *)board_obj_ptr;

	pfreq_cntlr_pi_set->prop_gain = pfreq_cntlr_pi->prop_gain;
	pfreq_cntlr_pi_set->integ_gain = pfreq_cntlr_pi->integ_gain;
	pfreq_cntlr_pi_set->integ_decay = pfreq_cntlr_pi->integ_decay;
	pfreq_cntlr_pi_set->volt_delta_min = pfreq_cntlr_pi->volt_delta_min;
	pfreq_cntlr_pi_set->volt_delta_max = pfreq_cntlr_pi->volt_delta_max;
	pfreq_cntlr_pi_set->slowdown_pct_min = pfreq_cntlr_pi->slowdown_pct_min;
	pfreq_cntlr_pi_set->bpoison = pfreq_cntlr_pi->bpoison;

	return status;
}

static int clk_freq_controller_construct_super(struct gk20a *g,
	struct boardobj **ppboardobj,
	size_t size, void *pargs)
{
	struct clk_freq_controller *pfreq_cntlr = NULL;
	struct clk_freq_controller *pfreq_cntlr_tmp = NULL;
	int status = 0;

	status = boardobj_construct_super(g, ppboardobj, size, pargs);
	if (status != 0) {
		return -EINVAL;
	}

	pfreq_cntlr_tmp = (struct clk_freq_controller *)pargs;
	pfreq_cntlr = (struct clk_freq_controller *)*ppboardobj;

	pfreq_cntlr->super.pmudatainit = clk_freq_controller_pmudatainit_super;

	pfreq_cntlr->controller_id   = pfreq_cntlr_tmp->controller_id;
	pfreq_cntlr->clk_domain      = pfreq_cntlr_tmp->clk_domain;
	pfreq_cntlr->parts_freq_mode  = pfreq_cntlr_tmp->parts_freq_mode;
	pfreq_cntlr->freq_cap_noise_unaware_vmin_above  =
		pfreq_cntlr_tmp->freq_cap_noise_unaware_vmin_above;
	pfreq_cntlr->freq_cap_noise_unaware_vmin_below  =
		pfreq_cntlr_tmp->freq_cap_noise_unaware_vmin_below;
	pfreq_cntlr->freq_hyst_pos_mhz = pfreq_cntlr_tmp->freq_hyst_pos_mhz;
	pfreq_cntlr->freq_hyst_neg_mhz = pfreq_cntlr_tmp->freq_hyst_neg_mhz;

	return status;
}

static int clk_freq_controller_construct_pi(struct gk20a *g,
	struct boardobj **ppboardobj,
	size_t size, void *pargs)
{
	struct clk_freq_controller_pi *pfreq_cntlr_pi = NULL;
	struct clk_freq_controller_pi *pfreq_cntlr_pi_tmp = NULL;
	int status = 0;

	status = clk_freq_controller_construct_super(g, ppboardobj,
			size, pargs);
	if (status != 0) {
		return -EINVAL;
	}

	pfreq_cntlr_pi = (struct clk_freq_controller_pi *)*ppboardobj;
	pfreq_cntlr_pi_tmp = (struct clk_freq_controller_pi *)pargs;

	pfreq_cntlr_pi->super.super.pmudatainit =
		clk_freq_controller_pmudatainit_pi;

	pfreq_cntlr_pi->prop_gain = pfreq_cntlr_pi_tmp->prop_gain;
	pfreq_cntlr_pi->integ_gain = pfreq_cntlr_pi_tmp->integ_gain;
	pfreq_cntlr_pi->integ_decay = pfreq_cntlr_pi_tmp->integ_decay;
	pfreq_cntlr_pi->volt_delta_min = pfreq_cntlr_pi_tmp->volt_delta_min;
	pfreq_cntlr_pi->volt_delta_max = pfreq_cntlr_pi_tmp->volt_delta_max;
	pfreq_cntlr_pi->slowdown_pct_min = pfreq_cntlr_pi_tmp->slowdown_pct_min;
	pfreq_cntlr_pi->bpoison = pfreq_cntlr_pi_tmp->bpoison;

	return status;
}

static struct clk_freq_controller *clk_clk_freq_controller_construct(
	struct gk20a *g,
	void *pargs)
{
	struct boardobj *board_obj_ptr = NULL;
	int status = 0;

	if (BOARDOBJ_GET_TYPE(pargs) != CTRL_CLK_CLK_FREQ_CONTROLLER_TYPE_PI) {
		return NULL;
	}

	status = clk_freq_controller_construct_pi(g, &board_obj_ptr,
			sizeof(struct clk_freq_controller_pi), pargs);
	if (status != 0) {
		return NULL;
	}

	return (struct clk_freq_controller *)board_obj_ptr;
}


static int clk_get_freq_controller_table(struct gk20a *g,
		struct nvgpu_clk_freq_controllers *pclk_freq_controllers)
{
	int status = 0;
	u8 *pfreq_controller_table_ptr = NULL;
	struct vbios_fct_1x_header header = { 0 };
	struct vbios_fct_1x_entry entry  = { 0 };
	u8 entry_idx;
	u8 *entry_offset;
	struct clk_freq_controller *pclk_freq_cntr = NULL;
	struct clk_freq_controller *ptmp_freq_cntr = NULL;
	struct clk_freq_controller_pi *ptmp_freq_cntr_pi = NULL;
	struct nvgpu_clk_domain *pclk_domain;

	struct freq_controller_data_type {
		union {
			struct boardobj board_obj;
			struct clk_freq_controller freq_controller;
			struct clk_freq_controller_pi  freq_controller_pi;
		};
	} freq_controller_data;

	pfreq_controller_table_ptr =
		(u8 *)nvgpu_bios_get_perf_table_ptrs(g,
			g->bios.clock_token,
			FREQUENCY_CONTROLLER_TABLE);
	if (pfreq_controller_table_ptr == NULL) {
		status = -EINVAL;
		goto done;
	}

	nvgpu_memcpy((u8 *)&header, pfreq_controller_table_ptr,
			sizeof(struct vbios_fct_1x_header));

	pclk_freq_controllers->sampling_period_ms = header.sampling_period_ms;
	pclk_freq_controllers->volt_policy_idx = 0;

	/* Read in the entries. */
	for (entry_idx = 0; entry_idx < header.entry_count; entry_idx++) {
		entry_offset = (pfreq_controller_table_ptr +
			header.header_size + (entry_idx * header.entry_size));

		(void) memset(&freq_controller_data, 0x0,
				sizeof(struct freq_controller_data_type));
		ptmp_freq_cntr = &freq_controller_data.freq_controller;
		ptmp_freq_cntr_pi = &freq_controller_data.freq_controller_pi;

		nvgpu_memcpy((u8 *)&entry, entry_offset,
			sizeof(struct vbios_fct_1x_entry));

		if (!BIOS_GET_FIELD(bool, entry.flags0,
				NV_VBIOS_FCT_1X_ENTRY_FLAGS0_TYPE)) {
			continue;
		}

		freq_controller_data.board_obj.type = BIOS_GET_FIELD(u8,
			entry.flags0, NV_VBIOS_FCT_1X_ENTRY_FLAGS0_TYPE);

		ptmp_freq_cntr->controller_id =
			BIOS_GET_FIELD(u8, entry.param0,
				NV_VBIOS_FCT_1X_ENTRY_PARAM0_ID);

		pclk_domain = g->pmu.clk_pmu->clk_get_clk_domain(
				(g->pmu.clk_pmu),
				(u32)entry.clk_domain_idx);
		freq_controller_data.freq_controller.clk_domain =
			pclk_domain->api_domain;

		ptmp_freq_cntr->parts_freq_mode =
			BIOS_GET_FIELD(u8, entry.param0,
				NV_VBIOS_FCT_1X_ENTRY_PARAM0_FREQ_MODE);

		/* Populate PI specific data */
		ptmp_freq_cntr_pi->slowdown_pct_min =
			BIOS_GET_FIELD(u8, entry.param1,
				NV_VBIOS_FCT_1X_ENTRY_PARAM1_SLOWDOWN_PCT_MIN);

		ptmp_freq_cntr_pi->bpoison =
			BIOS_GET_FIELD(bool, entry.param1,
				NV_VBIOS_FCT_1X_ENTRY_PARAM1_POISON);

		ptmp_freq_cntr_pi->prop_gain =
			BIOS_GET_FIELD(s32, entry.param2,
				NV_VBIOS_FCT_1X_ENTRY_PARAM2_PROP_GAIN);

		ptmp_freq_cntr_pi->integ_gain =
			BIOS_GET_FIELD(s32, entry.param3,
				NV_VBIOS_FCT_1X_ENTRY_PARAM3_INTEG_GAIN);

		ptmp_freq_cntr_pi->integ_decay =
			BIOS_GET_FIELD(s32, entry.param4,
				NV_VBIOS_FCT_1X_ENTRY_PARAM4_INTEG_DECAY);

		ptmp_freq_cntr_pi->volt_delta_min =
			BIOS_GET_FIELD(s32, entry.param5,
				NV_VBIOS_FCT_1X_ENTRY_PARAM5_VOLT_DELTA_MIN);

		ptmp_freq_cntr_pi->volt_delta_max =
			BIOS_GET_FIELD(s32, entry.param6,
				NV_VBIOS_FCT_1X_ENTRY_PARAM6_VOLT_DELTA_MAX);

		ptmp_freq_cntr->freq_cap_noise_unaware_vmin_above =
			BIOS_GET_FIELD(s16, entry.param7,
				NV_VBIOS_FCT_1X_ENTRY_PARAM7_FREQ_CAP_VF);

		ptmp_freq_cntr->freq_cap_noise_unaware_vmin_below =
			BIOS_GET_FIELD(s16, entry.param7,
				NV_VBIOS_FCT_1X_ENTRY_PARAM7_FREQ_CAP_VMIN);

		ptmp_freq_cntr->freq_hyst_pos_mhz =
			BIOS_GET_FIELD(s16, entry.param8,
				NV_VBIOS_FCT_1X_ENTRY_PARAM8_FREQ_HYST_POS);
		ptmp_freq_cntr->freq_hyst_neg_mhz =
			BIOS_GET_FIELD(s16, entry.param8,
				NV_VBIOS_FCT_1X_ENTRY_PARAM8_FREQ_HYST_NEG);

		if (ptmp_freq_cntr_pi->volt_delta_max <
			ptmp_freq_cntr_pi->volt_delta_min) {
			goto done;
		}

		pclk_freq_cntr = clk_clk_freq_controller_construct(g,
						(void *)&freq_controller_data);

		if (pclk_freq_cntr == NULL) {
			nvgpu_err(g,
				"unable to construct clock freq cntlr boardobj for %d",
				entry_idx);
			status = -EINVAL;
			goto done;
		}

		status = boardobjgrp_objinsert(
				&pclk_freq_controllers->super.super,
				(struct boardobj *)pclk_freq_cntr, entry_idx);
		if (status != 0) {
			nvgpu_err(g,
			"unable to insert clock freq cntlr boardobj for");
			status = -EINVAL;
			goto done;
		}

	}

done:
	return status;
}

int nvgpu_clk_freq_controller_pmu_setup(struct gk20a *g)
{
	int status;
	struct boardobjgrp *pboardobjgrp = NULL;

	nvgpu_log_info(g, " ");

	pboardobjgrp = &g->pmu.clk_pmu->clk_freq_controllers->super.super;

	if (!pboardobjgrp->bconstructed) {
		return -EINVAL;
	}

	status = pboardobjgrp->pmuinithandle(g, pboardobjgrp);

	nvgpu_log_info(g, "Done");
	return status;
}

static int _clk_freq_controller_devgrp_pmudata_instget(struct gk20a *g,
		struct nv_pmu_boardobjgrp *pmuboardobjgrp,
		struct nv_pmu_boardobj **ppboardobjpmudata, u8 idx)
{
	struct nv_pmu_clk_clk_freq_controller_boardobj_grp_set *pgrp_set =
		(struct nv_pmu_clk_clk_freq_controller_boardobj_grp_set *)
		pmuboardobjgrp;

	nvgpu_log_info(g, " ");

	/*check whether pmuboardobjgrp has a valid boardobj in index*/
	if (((u32)BIT(idx) &
		pgrp_set->hdr.data.super.obj_mask.super.data[0]) == 0U) {
		return -EINVAL;
	}

	*ppboardobjpmudata = (struct nv_pmu_boardobj *)
		&pgrp_set->objects[idx].data.board_obj;
	nvgpu_log_info(g, " Done");
	return 0;
}

static int _clk_freq_controllers_pmudatainit(struct gk20a *g,
		struct boardobjgrp *pboardobjgrp,
		struct nv_pmu_boardobjgrp_super *pboardobjgrppmu)
{
	struct nv_pmu_clk_clk_freq_controller_boardobjgrp_set_header *pset =
		(struct nv_pmu_clk_clk_freq_controller_boardobjgrp_set_header *)
		pboardobjgrppmu;
	struct nvgpu_clk_freq_controllers *pcntrs =
		(struct nvgpu_clk_freq_controllers *)pboardobjgrp;
	int status = 0;

	status = boardobjgrp_pmudatainit_e32(g, pboardobjgrp, pboardobjgrppmu);
	if (status != 0) {
		nvgpu_err(g,
			"error updating pmu boardobjgrp for clk freq ctrs 0x%x",
			 status);
		goto done;
	}
	pset->sampling_period_ms = pcntrs->sampling_period_ms;
	pset->volt_policy_idx = pcntrs->volt_policy_idx;

done:
	return status;
}

int nvgpu_clk_freq_controller_sw_setup(struct gk20a *g)
{
	int status = 0;
	struct boardobjgrp *pboardobjgrp = NULL;
	struct nvgpu_clk_freq_controllers *pclk_freq_controllers;
	struct nvgpu_avfsfllobjs *pfllobjs = g->pmu.clk_pmu->avfs_fllobjs;
	struct fll_device *pfll;
	struct clk_freq_controller *pclkfreqctrl;
	u8 i;
	u8 j;

	nvgpu_log_info(g, " ");

	pclk_freq_controllers = g->pmu.clk_pmu->clk_freq_controllers;
	status = boardobjgrpconstruct_e32(g, &pclk_freq_controllers->super);
	if (status != 0) {
		nvgpu_err(g,
			"error creating boardobjgrp for clk FCT, status - 0x%x",
			status);
		goto done;
	}

	pboardobjgrp = &g->pmu.clk_pmu->clk_freq_controllers->super.super;

	pboardobjgrp->pmudatainit  = _clk_freq_controllers_pmudatainit;
	pboardobjgrp->pmudatainstget  =
			_clk_freq_controller_devgrp_pmudata_instget;
	pboardobjgrp->pmustatusinstget  = NULL;

	/* Initialize mask to zero.*/
	boardobjgrpmask_e32_init(&pclk_freq_controllers->freq_ctrl_load_mask,
		NULL);

	BOARDOBJGRP_PMU_CONSTRUCT(pboardobjgrp, CLK, CLK_FREQ_CONTROLLER);

	status = BOARDOBJGRP_PMU_CMD_GRP_SET_CONSTRUCT(g, pboardobjgrp,
			clk, CLK, clk_freq_controller, CLK_FREQ_CONTROLLER);
	if (status != 0) {
		nvgpu_err(g,
			"error constructing PMU_BOARDOBJ_CMD_GRP_SET interface - 0x%x",
			status);
		goto done;
	}

	status = clk_get_freq_controller_table(g, pclk_freq_controllers);
	if (status != 0) {
		nvgpu_err(g, "error reading freq controller table - 0x%x",
			status);
		goto done;
	}

	BOARDOBJGRP_FOR_EACH(&(pclk_freq_controllers->super.super),
			     struct clk_freq_controller *, pclkfreqctrl, i) {
		pfll = NULL;
		j = 0;
		BOARDOBJGRP_FOR_EACH(&(pfllobjs->super.super),
			     struct fll_device *, pfll, j) {
			if (pclkfreqctrl->controller_id == pfll->id) {
				pfll->freq_ctrl_idx = i;
				break;
			}
		}
		boardobjgrpmask_bitset(&pclk_freq_controllers->
			freq_ctrl_load_mask.super, i);
	}
done:
		nvgpu_log_info(g, " done status %x", status);
		return status;
}

int nvgpu_clk_pmu_freq_controller_load(struct gk20a *g, bool bload, u8 bit_idx)
{
	struct pmu_cmd cmd;
	struct pmu_payload payload;
	int status;
	struct nv_pmu_clk_rpc rpccall;
	struct clk_freq_ctlr_rpc_pmucmdhandler_params handler;
	struct nv_pmu_clk_load *clkload;
	struct nvgpu_clk_freq_controllers *pclk_freq_controllers;
	struct ctrl_boardobjgrp_mask_e32 *load_mask;
	struct boardobjgrpmask_e32 isolate_cfc_mask;

	(void) memset(&payload, 0, sizeof(struct pmu_payload));
	(void) memset(&rpccall, 0, sizeof(struct nv_pmu_clk_rpc));
	(void) memset(&handler, 0, sizeof(
			struct clk_freq_ctlr_rpc_pmucmdhandler_params));

	pclk_freq_controllers = g->pmu.clk_pmu->clk_freq_controllers;
	rpccall.function = NV_PMU_CLK_RPC_ID_LOAD;
	clkload = &rpccall.params.clk_load;
	clkload->feature = NV_PMU_CLK_LOAD_FEATURE_FREQ_CONTROLLER;
	clkload->action_mask = bload ?
		NV_PMU_CLK_LOAD_ACTION_MASK_FREQ_CONTROLLER_CALLBACK_YES :
		NV_PMU_CLK_LOAD_ACTION_MASK_FREQ_CONTROLLER_CALLBACK_NO;

	load_mask = &rpccall.params.clk_load.payload.freq_controllers.load_mask;

	status = boardobjgrpmask_e32_init(&isolate_cfc_mask, NULL);

	if (bit_idx == CTRL_CLK_CLK_FREQ_CONTROLLER_ID_ALL) {
		status = boardobjgrpmask_export(
				&pclk_freq_controllers->
					freq_ctrl_load_mask.super,
				pclk_freq_controllers->
					freq_ctrl_load_mask.super.bitcount,
				&load_mask->super);


	} else {
		status = boardobjgrpmask_bitset(&isolate_cfc_mask.super,
						bit_idx);
		status = boardobjgrpmask_export(&isolate_cfc_mask.super,
					isolate_cfc_mask.super.bitcount,
					&load_mask->super);
		if (bload) {
			status = boardobjgrpmask_bitset(
					&pclk_freq_controllers->
						freq_ctrl_load_mask.super,
					bit_idx);
		} else {
			status = boardobjgrpmask_bitclr(
					&pclk_freq_controllers->
						freq_ctrl_load_mask.super,
					bit_idx);
		}
	}

	if (status != 0) {
		nvgpu_err(g, "Error in generating mask used to select CFC");
		goto done;
	}

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
	nvgpu_assert(NV_PMU_CLK_MSG_RPC_ALLOC_OFFSET < U64(U32_MAX));
	payload.out.offset = (u32)NV_PMU_CLK_MSG_RPC_ALLOC_OFFSET;

	handler.prpccall = &rpccall;
	handler.success = 0;
	status = nvgpu_pmu_cmd_post(g, &cmd, &payload,
			PMU_COMMAND_QUEUE_LPQ,
			clk_freq_ctlr_rpc_pmucmdhandler, (void *)&handler);

	if (status != 0) {
		nvgpu_err(g, "unable to post clk RPC cmd %x",
			cmd.cmd.clk.cmd_type);
		goto done;
	}

	pmu_wait_message_cond(&g->pmu, nvgpu_get_poll_timeout(g),
			&handler.success, 1);

	if (handler.success == 0U) {
		nvgpu_err(g, "rpc call to load freq cntlr cal failed");
		status = -EINVAL;
	}

done:
	return status;
}

int nvgpu_clk_freq_controller_init_pmupstate(struct gk20a *g)
{
	/* If already allocated, do not re-allocate */
	if (g->pmu.clk_pmu->clk_freq_controllers != NULL) {
		return 0;
	}

	g->pmu.clk_pmu->clk_freq_controllers = nvgpu_kzalloc(g,
			sizeof(*g->pmu.clk_pmu->clk_freq_controllers));
	if (g->pmu.clk_pmu->clk_freq_controllers == NULL) {
		return -ENOMEM;
	}

	return 0;
}

void nvgpu_clk_freq_controller_free_pmupstate(struct gk20a *g)
{
	nvgpu_kfree(g, g->pmu.clk_pmu->clk_freq_controllers);
	g->pmu.clk_pmu->clk_freq_controllers = NULL;
}
