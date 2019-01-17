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
#include <nvgpu/string.h>
#include <nvgpu/pmuif/ctrlclk.h>
#include <nvgpu/pmuif/ctrlvolt.h>
#include <nvgpu/timers.h>
#include <nvgpu/clk_arb.h>
#include <nvgpu/pmu/volt.h>

#include "clk.h"
#include "clk_vf_point.h"

static int _clk_vf_point_pmudatainit_super(struct gk20a *g, struct boardobj
	*board_obj_ptr,	struct nv_pmu_boardobj *ppmudata);

static int _clk_vf_points_pmudatainit(struct gk20a *g,
				      struct boardobjgrp *pboardobjgrp,
				      struct nv_pmu_boardobjgrp_super *pboardobjgrppmu)
{
	int status = 0;

	status = boardobjgrp_pmudatainit_e32(g, pboardobjgrp, pboardobjgrppmu);
	if (status != 0) {
		nvgpu_err(g,
			  "error updating pmu boardobjgrp for clk vfpoint 0x%x",
			  status);
		goto done;
	}

done:
	return status;
}

static int _clk_vf_points_pmudata_instget(struct gk20a *g,
					  struct nv_pmu_boardobjgrp *pmuboardobjgrp,
					  struct nv_pmu_boardobj **ppboardobjpmudata,
					  u8 idx)
{
	struct nv_pmu_clk_clk_vf_point_boardobj_grp_set  *pgrp_set =
		(struct nv_pmu_clk_clk_vf_point_boardobj_grp_set *)
		pmuboardobjgrp;

	nvgpu_log_info(g, " ");

	/*check whether pmuboardobjgrp has a valid boardobj in index*/
	if (idx >= CTRL_BOARDOBJGRP_E255_MAX_OBJECTS) {
		return -EINVAL;
	}

	*ppboardobjpmudata = (struct nv_pmu_boardobj *)
		&pgrp_set->objects[idx].data.board_obj;
	nvgpu_log_info(g, " Done");
	return 0;
}

static int _clk_vf_points_pmustatus_instget(struct gk20a *g,
					    void *pboardobjgrppmu,
					    struct nv_pmu_boardobj_query **ppboardobjpmustatus,
					    u8 idx)
{
	struct nv_pmu_clk_clk_vf_point_boardobj_grp_get_status *pgrp_get_status =
		(struct nv_pmu_clk_clk_vf_point_boardobj_grp_get_status *)
		pboardobjgrppmu;

	/*check whether pmuboardobjgrp has a valid boardobj in index*/
	if (idx >= CTRL_BOARDOBJGRP_E255_MAX_OBJECTS) {
		return -EINVAL;
	}

	*ppboardobjpmustatus = (struct nv_pmu_boardobj_query *)
			&pgrp_get_status->objects[idx].data.board_obj;
	return 0;
}

int clk_vf_point_sw_setup(struct gk20a *g)
{
	int status;
	struct boardobjgrp *pboardobjgrp = NULL;
	u32 ver = g->params.gpu_arch + g->params.gpu_impl;

	nvgpu_log_info(g, " ");

	status = boardobjgrpconstruct_e255(g, &g->clk_pmu->clk_vf_pointobjs.super);
	if (status != 0) {
		nvgpu_err(g,
		"error creating boardobjgrp for clk vfpoint, status - 0x%x",
		status);
		goto done;
	}

	pboardobjgrp = &g->clk_pmu->clk_vf_pointobjs.super.super;

	BOARDOBJGRP_PMU_CONSTRUCT(pboardobjgrp, CLK, CLK_VF_POINT);

	if (ver == NVGPU_GPUID_TU104) {
		status = BOARDOBJGRP_PMU_CMD_GRP_SET_CONSTRUCT_35(g, pboardobjgrp,
				clk, CLK, clk_vf_point, CLK_VF_POINT);
		if (status != 0) {
			nvgpu_err(g,
				"error constructing PMU_BOARDOBJ_CMD_GRP_SET interface - 0x%x",
				status);
			goto done;
		}

		status = BOARDOBJGRP_PMU_CMD_GRP_GET_STATUS_CONSTRUCT_35(g,
					&g->clk_pmu->clk_vf_pointobjs.super.super,
					clk, CLK, clk_vf_point, CLK_VF_POINT);
		if (status != 0) {
			nvgpu_err(g,
				"error constructing PMU_BOARDOBJ_CMD_GRP_SET interface - 0x%x",
				status);
			goto done;
		}
	}
	else {
		status = BOARDOBJGRP_PMU_CMD_GRP_SET_CONSTRUCT(g, pboardobjgrp,
				clk, CLK, clk_vf_point, CLK_VF_POINT);
		if (status != 0) {
			nvgpu_err(g,
				"error constructing PMU_BOARDOBJ_CMD_GRP_SET interface - 0x%x",
				status);
			goto done;
		}

		nvgpu_err(g,"GV100 vf_point ss_offset %x", pboardobjgrp->pmu.set.super_surface_offset);

		status = BOARDOBJGRP_PMU_CMD_GRP_GET_STATUS_CONSTRUCT(g,
					&g->clk_pmu->clk_vf_pointobjs.super.super,
					clk, CLK, clk_vf_point, CLK_VF_POINT);
		if (status != 0) {
			nvgpu_err(g,
				"error constructing PMU_BOARDOBJ_CMD_GRP_SET interface - 0x%x",
				status);
			goto done;
		}
	}
	pboardobjgrp->pmudatainit = _clk_vf_points_pmudatainit;
	pboardobjgrp->pmudatainstget  = _clk_vf_points_pmudata_instget;
	pboardobjgrp->pmustatusinstget  = _clk_vf_points_pmustatus_instget;

done:
	nvgpu_log_info(g, " done status %x", status);
	return status;
}

int clk_vf_point_pmu_setup(struct gk20a *g)
{
	int status;
	struct boardobjgrp *pboardobjgrp = NULL;

	nvgpu_log_info(g, " ");

	pboardobjgrp = &g->clk_pmu->clk_vf_pointobjs.super.super;

	if (!pboardobjgrp->bconstructed) {
		return -EINVAL;
	}

	status = pboardobjgrp->pmuinithandle(g, pboardobjgrp);

	nvgpu_log_info(g, "Done");
	return status;
}

static int clk_vf_point_construct_super(struct gk20a *g,
					struct boardobj **ppboardobj,
					u16 size, void *pargs)
{
	struct clk_vf_point *pclkvfpoint;
	struct clk_vf_point *ptmpvfpoint =
			(struct clk_vf_point *)pargs;
	int status = 0;

	status = boardobj_construct_super(g, ppboardobj,
		size, pargs);
	if (status != 0) {
		return -EINVAL;
	}

	pclkvfpoint = (struct clk_vf_point *)*ppboardobj;

	pclkvfpoint->super.pmudatainit =
			_clk_vf_point_pmudatainit_super;

	pclkvfpoint->vfe_equ_idx = ptmpvfpoint->vfe_equ_idx;
	pclkvfpoint->volt_rail_idx = ptmpvfpoint->volt_rail_idx;

	return status;
}

static int _clk_vf_point_pmudatainit_volt(struct gk20a *g,
					  struct boardobj *board_obj_ptr,
					  struct nv_pmu_boardobj *ppmudata)
{
	int status = 0;
	struct clk_vf_point_volt *pclk_vf_point_volt;
	struct nv_pmu_clk_clk_vf_point_volt_boardobj_set *pset;

	nvgpu_log_info(g, " ");

	status = _clk_vf_point_pmudatainit_super(g, board_obj_ptr, ppmudata);
	if (status != 0) {
		return status;
	}

	pclk_vf_point_volt =
		(struct clk_vf_point_volt *)board_obj_ptr;

	pset = (struct nv_pmu_clk_clk_vf_point_volt_boardobj_set *)
		ppmudata;

	pset->source_voltage_uv = pclk_vf_point_volt->source_voltage_uv;
	pset->freq_delta.data = pclk_vf_point_volt->freq_delta.data;
	pset->freq_delta.type = pclk_vf_point_volt->freq_delta.type;

	return status;
}

static int _clk_vf_point_pmudatainit_freq(struct gk20a *g,
					  struct boardobj *board_obj_ptr,
					  struct nv_pmu_boardobj *ppmudata)
{
	int status = 0;
	struct clk_vf_point_freq *pclk_vf_point_freq;
	struct nv_pmu_clk_clk_vf_point_freq_boardobj_set *pset;

	nvgpu_log_info(g, " ");

	status = _clk_vf_point_pmudatainit_super(g, board_obj_ptr, ppmudata);
	if (status != 0) {
		return status;
	}

	pclk_vf_point_freq =
		(struct clk_vf_point_freq *)board_obj_ptr;

	pset = (struct nv_pmu_clk_clk_vf_point_freq_boardobj_set *)
		ppmudata;

	pset->freq_mhz =
		clkvfpointfreqmhzget(g, &pclk_vf_point_freq->super);

	pset->volt_delta_uv = pclk_vf_point_freq->volt_delta_uv;

	return status;
}

static int clk_vf_point_construct_volt(struct gk20a *g,
				       struct boardobj **ppboardobj,
				       u16 size, void *pargs)
{
	struct boardobj *ptmpobj = (struct boardobj *)pargs;
	struct clk_vf_point_volt *pclkvfpoint;
	struct clk_vf_point_volt *ptmpvfpoint =
			(struct clk_vf_point_volt *)pargs;
	int status = 0;

	if (BOARDOBJ_GET_TYPE(pargs) != CTRL_CLK_CLK_VF_POINT_TYPE_VOLT) {
		return -EINVAL;
	}

	ptmpobj->type_mask = BIT(CTRL_CLK_CLK_VF_POINT_TYPE_VOLT);
	status = clk_vf_point_construct_super(g, ppboardobj, size, pargs);
	if (status != 0) {
		return -EINVAL;
	}

	pclkvfpoint = (struct clk_vf_point_volt *)*ppboardobj;

	pclkvfpoint->super.super.pmudatainit =
			_clk_vf_point_pmudatainit_volt;

	pclkvfpoint->source_voltage_uv = ptmpvfpoint->source_voltage_uv;
	pclkvfpoint->freq_delta = ptmpvfpoint->freq_delta;

	return status;
}

static int clk_vf_point_construct_freq(struct gk20a *g,
				       struct boardobj **ppboardobj,
				       u16 size, void *pargs)
{
	struct boardobj *ptmpobj = (struct boardobj *)pargs;
	struct clk_vf_point_freq *pclkvfpoint;
	struct clk_vf_point_freq *ptmpvfpoint =
			(struct clk_vf_point_freq *)pargs;
	int status = 0;

	if (BOARDOBJ_GET_TYPE(pargs) != CTRL_CLK_CLK_VF_POINT_TYPE_FREQ) {
		return -EINVAL;
	}

	ptmpobj->type_mask = BIT(CTRL_CLK_CLK_VF_POINT_TYPE_FREQ);
	status = clk_vf_point_construct_super(g, ppboardobj, size, pargs);
	if (status != 0) {
		return -EINVAL;
	}

	pclkvfpoint = (struct clk_vf_point_freq *)*ppboardobj;

	pclkvfpoint->super.super.pmudatainit =
			_clk_vf_point_pmudatainit_freq;

	clkvfpointfreqmhzset(g, &pclkvfpoint->super,
		clkvfpointfreqmhzget(g, &ptmpvfpoint->super));

	return status;
}

static int clk_vf_point_construct_volt_35(struct gk20a *g,
				       struct boardobj **ppboardobj,
				       u16 size, void *pargs)
{
	struct boardobj *ptmpobj = (struct boardobj *)pargs;
	struct clk_vf_point_volt *pclkvfpoint;
	struct clk_vf_point_volt *ptmpvfpoint =
			(struct clk_vf_point_volt *)pargs;
	int status = 0;

	if (BOARDOBJ_GET_TYPE(pargs) != CTRL_CLK_CLK_VF_POINT_TYPE_35_VOLT) {
		return -EINVAL;
	}

	ptmpobj->type_mask = (u32) BIT(CTRL_CLK_CLK_VF_POINT_TYPE_35_VOLT);
	status = clk_vf_point_construct_super(g, ppboardobj, size, pargs);
	if (status != 0) {
		return -EINVAL;
	}

	pclkvfpoint = (struct clk_vf_point_volt *) (void *) *ppboardobj;

	pclkvfpoint->super.super.pmudatainit =
			_clk_vf_point_pmudatainit_volt;

	pclkvfpoint->source_voltage_uv = ptmpvfpoint->source_voltage_uv;
	pclkvfpoint->freq_delta = ptmpvfpoint->freq_delta;

	return status;
}

static int clk_vf_point_construct_freq_35(struct gk20a *g,
				       struct boardobj **ppboardobj,
				       u16 size, void *pargs)
{
	struct boardobj *ptmpobj = (struct boardobj *)pargs;
	struct clk_vf_point_freq *pclkvfpoint;
	struct clk_vf_point_freq *ptmpvfpoint =
			(struct clk_vf_point_freq *)pargs;
	int status = 0;

	if (BOARDOBJ_GET_TYPE(pargs) != CTRL_CLK_CLK_VF_POINT_TYPE_35_FREQ) {
		return -EINVAL;
	}

	ptmpobj->type_mask = (u32) BIT(CTRL_CLK_CLK_VF_POINT_TYPE_35_FREQ);
	status = clk_vf_point_construct_super(g, ppboardobj, size, pargs);
	if (status != 0) {
		return -EINVAL;
	}

	pclkvfpoint = (struct clk_vf_point_freq *)(void*) *ppboardobj;

	pclkvfpoint->super.super.pmudatainit =
			_clk_vf_point_pmudatainit_freq;

	clkvfpointfreqmhzset(g, &pclkvfpoint->super,
		clkvfpointfreqmhzget(g, &ptmpvfpoint->super));

	return status;
}

struct clk_vf_point *construct_clk_vf_point(struct gk20a *g, void *pargs)
{
	struct boardobj *board_obj_ptr = NULL;
	int status;

	nvgpu_log_info(g, " ");
	switch (BOARDOBJ_GET_TYPE(pargs)) {
	case CTRL_CLK_CLK_VF_POINT_TYPE_FREQ:
		status = clk_vf_point_construct_freq(g, &board_obj_ptr,
			sizeof(struct clk_vf_point_freq), pargs);
		break;

	case CTRL_CLK_CLK_VF_POINT_TYPE_VOLT:
		status = clk_vf_point_construct_volt(g, &board_obj_ptr,
			sizeof(struct clk_vf_point_volt), pargs);
		break;

	case CTRL_CLK_CLK_VF_POINT_TYPE_35_FREQ:
		status = clk_vf_point_construct_freq_35(g, &board_obj_ptr,
			sizeof(struct clk_vf_point_freq), pargs);
		break;

	case CTRL_CLK_CLK_VF_POINT_TYPE_35_VOLT:
		status = clk_vf_point_construct_volt_35(g, &board_obj_ptr,
			sizeof(struct clk_vf_point_volt), pargs);
		break;

	default:
		return NULL;
	}

	if (status != 0) {
		return NULL;
	}

	nvgpu_log_info(g, " Done");

	return (struct clk_vf_point *)board_obj_ptr;
}

static int _clk_vf_point_pmudatainit_super(struct gk20a *g,
					   struct boardobj *board_obj_ptr,
					   struct nv_pmu_boardobj *ppmudata)
{
	int status = 0;
	struct clk_vf_point *pclk_vf_point;
	struct nv_pmu_clk_clk_vf_point_boardobj_set *pset;

	nvgpu_log_info(g, " ");

	status = boardobj_pmudatainit_super(g, board_obj_ptr, ppmudata);
	if (status != 0) {
		return status;
	}

	pclk_vf_point =
		(struct clk_vf_point *)board_obj_ptr;

	pset = (struct nv_pmu_clk_clk_vf_point_boardobj_set *)
		ppmudata;


	pset->vfe_equ_idx = pclk_vf_point->vfe_equ_idx;
	pset->volt_rail_idx = pclk_vf_point->volt_rail_idx;
	return status;
}


static int clk_vf_point_update(struct gk20a *g,
				struct boardobj *board_obj_ptr,
				struct nv_pmu_boardobj *ppmudata)
{
	struct clk_vf_point *pclk_vf_point;
	struct nv_pmu_clk_clk_vf_point_boardobj_get_status *pstatus;

	nvgpu_log_info(g, " ");


	pclk_vf_point =
		(struct clk_vf_point *)board_obj_ptr;

	pstatus = (struct nv_pmu_clk_clk_vf_point_boardobj_get_status *)
		ppmudata;

	if (pstatus->super.type != pclk_vf_point->super.type) {
		nvgpu_err(g,
			"pmu data and boardobj type not matching");
		return -EINVAL;
	}
	/* now copy VF pair */
	nvgpu_memcpy((u8 *)&pclk_vf_point->pair, (u8 *)&pstatus->pair,
		sizeof(struct ctrl_clk_vf_pair));
	return 0;
}

int nvgpu_clk_set_req_fll_clk_ps35(struct gk20a *g, struct nvgpu_clk_slave_freq *vf_point)
{
	struct nvgpu_pmu *pmu = &g->pmu;
	struct nv_pmu_rpc_perf_change_seq_queue_change rpc;
	struct ctrl_perf_change_seq_change_input change_input;
	struct clk_domain *pclk_domain;
	int status = 0;
	u8 i = 0, gpcclk_domain=0;
	u32 gpcclk_voltuv=0,gpcclk_clkmhz=0;
	u32 max_clkmhz;
	u16 max_ratio;
	struct clk_set_info *p0_info;
	u32 vmin_uv = 0;

	(void) memset(&change_input, 0,
		sizeof(struct ctrl_perf_change_seq_change_input));
	BOARDOBJGRP_FOR_EACH(&(g->clk_pmu->clk_domainobjs.super.super),
		struct clk_domain *, pclk_domain, i) {

		switch (pclk_domain->api_domain) {
		case CTRL_CLK_DOMAIN_GPCCLK:
			gpcclk_domain = i;
			gpcclk_clkmhz = vf_point->gpc_mhz;

			p0_info = pstate_get_clk_set_info(g,
					CTRL_PERF_PSTATE_P0, CLKWHICH_GPCCLK);
			if(p0_info == NULL){
				nvgpu_err(g, "failed to get GPCCLK P0 info");
				break;
			}
			if ( vf_point->gpc_mhz < p0_info->min_mhz ) {
				vf_point->gpc_mhz = p0_info->min_mhz;
			}
			if (vf_point->gpc_mhz > p0_info->max_mhz) {
				vf_point->gpc_mhz = p0_info->max_mhz;
			}
			change_input.clk[i].clk_freq_khz = (u32)vf_point->gpc_mhz * 1000U;
			change_input.clk_domains_mask.super.data[0] |= (u32) BIT(i);
			break;
		case CTRL_CLK_DOMAIN_XBARCLK:
			p0_info = pstate_get_clk_set_info(g,
					CTRL_PERF_PSTATE_P0, CLKWHICH_XBARCLK);
			if(p0_info == NULL){
				nvgpu_err(g, "failed to get XBARCLK P0 info");
				break;
			}
			max_ratio = (vf_point->xbar_mhz*100U)/vf_point->gpc_mhz;
			if ( vf_point->xbar_mhz < p0_info->min_mhz ) {
				vf_point->xbar_mhz = p0_info->min_mhz;
			}
			if (vf_point->xbar_mhz > p0_info->max_mhz) {
				vf_point->xbar_mhz = p0_info->max_mhz;
			}
			change_input.clk[i].clk_freq_khz = (u32)vf_point->xbar_mhz * 1000U;
			change_input.clk_domains_mask.super.data[0] |= (u32) BIT(i);
			if (vf_point->gpc_mhz < vf_point->xbar_mhz) {
				max_clkmhz = (((u32)vf_point->xbar_mhz * 100U) / (u32)max_ratio);
				if (gpcclk_clkmhz < max_clkmhz) {
					gpcclk_clkmhz = max_clkmhz;
				}
			}
			break;
		case CTRL_CLK_DOMAIN_SYSCLK:
			p0_info = pstate_get_clk_set_info(g,
					CTRL_PERF_PSTATE_P0, CLKWHICH_SYSCLK);
			if(p0_info == NULL){
				nvgpu_err(g, "failed to get SYSCLK P0 info");
				break;
			}
			max_ratio = (vf_point->sys_mhz*100U)/vf_point->gpc_mhz;
			if ( vf_point->sys_mhz < p0_info->min_mhz ) {
				vf_point->sys_mhz = p0_info->min_mhz;
			}
			if (vf_point->sys_mhz > p0_info->max_mhz) {
				vf_point->sys_mhz = p0_info->max_mhz;
			}
			change_input.clk[i].clk_freq_khz = (u32)vf_point->sys_mhz * 1000U;
			change_input.clk_domains_mask.super.data[0] |= (u32) BIT(i);
			if (vf_point->gpc_mhz < vf_point->sys_mhz) {
				max_clkmhz = (((u32)vf_point->sys_mhz * 100U) / (u32)max_ratio);
				if (gpcclk_clkmhz < max_clkmhz) {
					gpcclk_clkmhz = max_clkmhz;
				}
			}
			break;
		case CTRL_CLK_DOMAIN_NVDCLK:
			p0_info = pstate_get_clk_set_info(g,
					CTRL_PERF_PSTATE_P0, CLKWHICH_NVDCLK);
			if(p0_info == NULL){
				nvgpu_err(g, "failed to get NVDCLK P0 info");
				break;
			}
			max_ratio = (vf_point->nvd_mhz*100U)/vf_point->gpc_mhz;
			if ( vf_point->nvd_mhz < p0_info->min_mhz ) {
				vf_point->nvd_mhz = p0_info->min_mhz;
			}
			if (vf_point->nvd_mhz > p0_info->max_mhz) {
				vf_point->nvd_mhz = p0_info->max_mhz;
			}
			change_input.clk[i].clk_freq_khz = (u32)vf_point->nvd_mhz * 1000U;
			change_input.clk_domains_mask.super.data[0] |= (u32) BIT(i);
			if (vf_point->gpc_mhz < vf_point->nvd_mhz) {
				max_clkmhz = (((u32)vf_point->nvd_mhz * 100U) / (u32)max_ratio);
				if (gpcclk_clkmhz < max_clkmhz) {
					gpcclk_clkmhz = max_clkmhz;
				}
			}
			break;
		case CTRL_CLK_DOMAIN_HOSTCLK:
			p0_info = pstate_get_clk_set_info(g,
					CTRL_PERF_PSTATE_P0, CLKWHICH_HOSTCLK);
			if(p0_info == NULL){
				nvgpu_err(g, "failed to get HOSTCLK P0 info");
				break;
			}
			max_ratio = (vf_point->host_mhz*100U)/vf_point->gpc_mhz;
			if ( vf_point->host_mhz < p0_info->min_mhz ) {
				vf_point->host_mhz = p0_info->min_mhz;
			}
			if (vf_point->host_mhz > p0_info->max_mhz) {
				vf_point->host_mhz = p0_info->max_mhz;
			}
			change_input.clk[i].clk_freq_khz = (u32)vf_point->host_mhz * 1000U;
			change_input.clk_domains_mask.super.data[0] |= (u32) BIT(i);
			if (vf_point->gpc_mhz < vf_point->host_mhz) {
				max_clkmhz = (((u32)vf_point->host_mhz * 100U) / (u32)max_ratio);
				if (gpcclk_clkmhz < max_clkmhz) {
					gpcclk_clkmhz = max_clkmhz;
				}
			}
			break;
		default:
			nvgpu_pmu_dbg(g, "Fixed clock domain");
			break;
		}
	}

	change_input.pstate_index = 0U;
	change_input.flags = (u32)CTRL_PERF_CHANGE_SEQ_CHANGE_FORCE;
	change_input.vf_points_cache_counter = 0xFFFFFFFFU;

	status = clk_domain_freq_to_volt(g, gpcclk_domain,
	&gpcclk_clkmhz, &gpcclk_voltuv, CTRL_VOLT_DOMAIN_LOGIC);

	status = g->ops.pmu_ver.volt.volt_get_vmin(g, &vmin_uv);
	if (status != 0) {
		nvgpu_err(g, "Failed to execute Vmin get_status status=0x%x",
			status);
	}
	if ((status == 0) && (vmin_uv > gpcclk_voltuv)) {
		gpcclk_voltuv = vmin_uv;
		nvgpu_log_fn(g, "Vmin is higher than evaluated Volt");
	}

	change_input.volt[0].voltage_uv = gpcclk_voltuv;
	change_input.volt[0].voltage_min_noise_unaware_uv = gpcclk_voltuv;
	change_input.volt_rails_mask.super.data[0] = 1U;

	/* RPC to PMU to queue to execute change sequence request*/
	(void) memset(&rpc, 0, sizeof(struct nv_pmu_rpc_perf_change_seq_queue_change ));
	rpc.change = change_input;
	rpc.change.pstate_index =  0;
	PMU_RPC_EXECUTE_CPB(status, pmu, PERF, CHANGE_SEQ_QUEUE_CHANGE, &rpc, 0);
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

 int nvgpu_clk_arb_find_slave_points(struct nvgpu_clk_arb *arb,
		struct nvgpu_clk_slave_freq *vf_point)
{

	u16 gpc2clk_target;
	struct nvgpu_clk_vf_table *table;
	u32 index;
	int status = 0;
	do {
		gpc2clk_target = vf_point->gpc_mhz;

		table = NV_ACCESS_ONCE(arb->current_vf_table);
		/* pointer to table can be updated by callback */
		nvgpu_smp_rmb();

		if (table == NULL) {
			continue;
		}
		if ((table->gpc2clk_num_points == 0U)) {
			nvgpu_err(arb->g, "found empty table");
			status = -EINVAL; ;
		}

		/* round up the freq requests */
		for (index = 0; index < table->gpc2clk_num_points; index++) {
			if ((table->gpc2clk_points[index].gpc_mhz >=
							gpc2clk_target)) {
				gpc2clk_target =
					table->gpc2clk_points[index].gpc_mhz;
				vf_point->sys_mhz =
					table->gpc2clk_points[index].sys_mhz;
				vf_point->xbar_mhz =
					table->gpc2clk_points[index].xbar_mhz;
				vf_point->nvd_mhz =
					table->gpc2clk_points[index].nvd_mhz;
				vf_point->host_mhz =
					table->gpc2clk_points[index].host_mhz;
				break;
			}
		}
		vf_point->gpc_mhz = gpc2clk_target < vf_point->gpc_mhz ? gpc2clk_target : vf_point->gpc_mhz;
	} while ((table == NULL) ||
		(NV_ACCESS_ONCE(arb->current_vf_table) != table));

	return status;

}

/*get latest vf point data from PMU */
int clk_vf_point_cache(struct gk20a *g)
{

	struct clk_vf_points *pclk_vf_points;
	struct boardobjgrp *pboardobjgrp;
	struct boardobjgrpmask *pboardobjgrpmask;
	struct nv_pmu_boardobjgrp_super *pboardobjgrppmu;
	struct boardobj *pboardobj = NULL;
	struct nv_pmu_boardobj_query *pboardobjpmustatus = NULL;
	int status;
	struct clk_vf_point *pclk_vf_point;
	u8 index;
	u32 voltage_min_uv,voltage_step_size_uv;
	u32 gpcclk_clkmhz=0, gpcclk_voltuv=0;
	u32 ver = g->params.gpu_arch + g->params.gpu_impl;

	nvgpu_log_info(g, " ");
	pclk_vf_points = &g->clk_pmu->clk_vf_pointobjs;
	pboardobjgrp = &pclk_vf_points->super.super;
	pboardobjgrpmask = &pclk_vf_points->super.mask.super;

	if (ver == NVGPU_GPUID_GV100) {
		status = pboardobjgrp->pmugetstatus(g, pboardobjgrp, pboardobjgrpmask);
		if (status != 0) {
			nvgpu_err(g, "err getting boardobjs from pmu");
			return status;
		}
		pboardobjgrppmu = pboardobjgrp->pmu.getstatus.buf;

		BOARDOBJGRP_FOR_EACH(pboardobjgrp, struct boardobj*, pboardobj, index) {
			status = pboardobjgrp->pmustatusinstget(g,
					(struct nv_pmu_boardobjgrp *)pboardobjgrppmu,
					&pboardobjpmustatus, index);
			if (status != 0) {
				nvgpu_err(g, "could not get status object instance");
				return status;
			}
			status = clk_vf_point_update(g, pboardobj,
				(struct nv_pmu_boardobj *)pboardobjpmustatus);
			if (status != 0) {
				nvgpu_err(g, "invalid data from pmu at %d", index);
				return status;
			}

		}
	} else {
		voltage_min_uv = g->clk_pmu->avfs_fllobjs.lut_min_voltage_uv;
		voltage_step_size_uv = g->clk_pmu->avfs_fllobjs.lut_step_size_uv * 2U;
		BOARDOBJGRP_FOR_EACH(pboardobjgrp, struct boardobj*, pboardobj, index) {
			pclk_vf_point = (struct clk_vf_point *)(void *)pboardobj;
			gpcclk_voltuv =
					voltage_min_uv + index * voltage_step_size_uv;
			status = clk_domain_volt_to_freq(g, 0,
					&gpcclk_clkmhz, &gpcclk_voltuv, CTRL_VOLT_DOMAIN_LOGIC);
			if (status != 0) {
				nvgpu_err(g, "Failed to get freq for requested voltage");
				return status;
			}
		pclk_vf_point->pair.freq_mhz = (u16)gpcclk_clkmhz;
		pclk_vf_point->pair.voltage_uv = gpcclk_voltuv;
		}
	}
	return status;
}
