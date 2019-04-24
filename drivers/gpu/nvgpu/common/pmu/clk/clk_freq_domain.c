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

#include <nvgpu/bios.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/boardobjgrp.h>
#include <nvgpu/boardobjgrp_e32.h>
#include <nvgpu/pmu/pmuif/ctrlclk.h>
#include <nvgpu/bug.h>
#include <nvgpu/pmu/pmuif/ctrlboardobj.h>
#include <nvgpu/pmu/pmuif/nvgpu_cmdif.h>
#include <nvgpu/pmu/clk/clk_freq_domain.h>
#include <nvgpu/pmu/clk/clk.h>

struct domain_type {
	u8 type;
	u32 clk_domain;
};

static struct domain_type clk_freq_domain_type[] = {
	{
		CTRL_CLK_FREQ_DOMAIN_SCHEMA_MULTI_FLL,
		CTRL_CLK_DOMAIN_GPCCLK,
	},
	{
		CTRL_CLK_FREQ_DOMAIN_SCHEMA_SINGLE_FLL,
		CTRL_CLK_DOMAIN_XBARCLK,
	},
	{
		CTRL_CLK_FREQ_DOMAIN_SCHEMA_SINGLE_FLL,
		CTRL_CLK_DOMAIN_SYSCLK,
	},
	{
		CTRL_CLK_FREQ_DOMAIN_SCHEMA_SINGLE_FLL,
		CTRL_CLK_DOMAIN_NVDCLK,
	},
	{
		CTRL_CLK_FREQ_DOMAIN_SCHEMA_SINGLE_FLL,
		CTRL_CLK_DOMAIN_HOSTCLK,
	},
};

static int clk_freq_domain_grp_pmudatainit(struct gk20a *g,
	struct boardobjgrp *pboardobjgrp,
	struct nv_pmu_boardobjgrp_super *pboardobjgrppmu)
{
	struct nv_pmu_clk_clk_freq_domain_boardobjgrp_set_header *pset =
		(struct nv_pmu_clk_clk_freq_domain_boardobjgrp_set_header *)
		pboardobjgrppmu;
	struct nvgpu_clk_freq_domain_grp *pfreq_domain_grp =
		(struct nvgpu_clk_freq_domain_grp *)pboardobjgrp;
	int status = 0;

	status = boardobjgrp_pmudatainit_e32(g, pboardobjgrp, pboardobjgrppmu);
	if (status != 0) {
		nvgpu_err(g,
			"error updating pmu boardobjgrp for clk freq domain 0x%x",
			status);
		goto exit;
	}

	pset->init_flags = pfreq_domain_grp->init_flags;

exit:
	return status;
}

static int clk_freq_domain_grp_pmudata_instget(struct gk20a *g,
	struct nv_pmu_boardobjgrp *pmuboardobjgrp,
	struct nv_pmu_boardobj **ppboardobjpmudata,
	u8 idx)
{
	struct nv_pmu_clk_clk_freq_domain_boardobj_grp_set  *pgrp_set =
		(struct nv_pmu_clk_clk_freq_domain_boardobj_grp_set *)
		pmuboardobjgrp;

	nvgpu_log_fn(g, " ");

	/*check whether pmuboardobjgrp has a valid boardobj in index*/
	if (((u32)BIT(idx) &
		pgrp_set->hdr.data.super.obj_mask.super.data[0]) == 0U) {
		nvgpu_err(g, "bit(idx)==0");
		return -EINVAL;
	}

	*ppboardobjpmudata = (struct nv_pmu_boardobj *)
		&pgrp_set->objects[idx].data.super;

	return 0;
}


static int clk_freq_domain_pmudatainit(struct gk20a *g,
	struct boardobj *board_obj_ptr,
	struct nv_pmu_boardobj *ppmudata)
{
	struct nv_pmu_clk_clk_freq_domain_boardobj_set *pset = NULL;
	struct nvgpu_clk_freq_domain *freq_domain = NULL;
	int status = 0;

	nvgpu_log_fn(g, " ");

	status = boardobj_pmudatainit_super(g, board_obj_ptr, ppmudata);
	if(status != 0) {
		nvgpu_err(g, "Failed pmudatainit freq_domain");
		goto exit;
	}

	freq_domain = (struct nvgpu_clk_freq_domain *)(void*)board_obj_ptr;
	pset = (struct nv_pmu_clk_clk_freq_domain_boardobj_set *)
			(void*)ppmudata;
	pset->clk_domain = freq_domain->clk_domain;

exit:
	return status;
}

int nvgpu_clk_freq_domain_sw_setup(struct gk20a *g)
{
	struct boardobjgrp *pboardobjgrp = NULL;
	struct boardobj *pboardobj = NULL;
	struct nvgpu_clk_freq_domain *pfreq_domain = NULL;
	struct nvgpu_clk_freq_domain_grp *pfreq_domain_grp = NULL;
	size_t tmp_num_of_domains = sizeof(clk_freq_domain_type) /
		sizeof(struct domain_type);
	u8 num_of_domains;
	int status = 0;
	u8 idx = 0;

	union {
		struct boardobj super;
		struct nvgpu_clk_freq_domain freq_domain;
	}freq_domain_data;

	nvgpu_assert(tmp_num_of_domains <= U8_MAX);
	num_of_domains = (u8)tmp_num_of_domains;

	pboardobjgrp = &g->pmu.clk_pmu->freq_domain_grp_objs->super.super;
	pfreq_domain_grp = g->pmu.clk_pmu->freq_domain_grp_objs;

	status = boardobjgrpconstruct_e32(g,
			&g->pmu.clk_pmu->freq_domain_grp_objs->super);
	if (status != 0) {
		nvgpu_err(g,
			"error creating boardobjgrp for clk freq domain, status - 0x%x",
			status);
		goto exit;
	}

	pfreq_domain_grp->super.super.pmudatainit =
		clk_freq_domain_grp_pmudatainit;
	pfreq_domain_grp->super.super.pmudatainstget =
		clk_freq_domain_grp_pmudata_instget;

	/* No need to report */
	pfreq_domain_grp->init_flags = 0U;

	BOARDOBJGRP_PMU_CONSTRUCT(pboardobjgrp, CLK, CLK_FREQ_DOMAIN);

	status = BOARDOBJGRP_PMU_CMD_GRP_SET_CONSTRUCT(g, pboardobjgrp,
			clk, CLK, clk_freq_domain, CLK_FREQ_DOMAIN);
	if (status != 0) {
		nvgpu_err(g,
			"error constructing PMU_BOARDOBJ_CMD_GRP_SET interface - 0x%x",
			status);
		goto exit;
	}

	for (idx = 0; idx < num_of_domains; idx++) {
		memset(&freq_domain_data, 0, sizeof(freq_domain_data));

		freq_domain_data.super.type = clk_freq_domain_type[idx].type;
		freq_domain_data.freq_domain.clk_domain =
			clk_freq_domain_type[idx].clk_domain;

		pboardobj = NULL;
		status = boardobj_construct_super(g,&pboardobj,
			sizeof(struct nvgpu_clk_freq_domain),
			(void*)&freq_domain_data);
		if(status != 0) {
			nvgpu_err(g, "Failed to construct nvgpu_clk_freq_domain Board obj");
			goto exit;
		}

		pfreq_domain = (struct nvgpu_clk_freq_domain*)(void*) pboardobj;
		pfreq_domain->super.pmudatainit = clk_freq_domain_pmudatainit;
		pfreq_domain->clk_domain = freq_domain_data.freq_domain.clk_domain;

		status = boardobjgrp_objinsert(&pfreq_domain_grp->super.super,
				&pfreq_domain->super, idx);
		if (status != 0) {
			nvgpu_err(g,
			"unable to insert clock freq domain boardobj for %d", idx);
			goto exit;
		}
	}

exit:
	return status;
}

int nvgpu_clk_freq_domain_pmu_setup(struct gk20a *g)
{
	int status = 0;
	struct boardobjgrp *pboardobjgrp = NULL;

	nvgpu_log_fn(g, " ");

	pboardobjgrp = &g->pmu.clk_pmu->freq_domain_grp_objs->super.super;

	if (!pboardobjgrp->bconstructed) {
		return -EINVAL;
	}

	status = pboardobjgrp->pmuinithandle(g, pboardobjgrp);

	return status;

}

int nvgpu_clk_freq_domain_init_pmupstate(struct gk20a *g)
{
	/* If already allocated, do not re-allocate */
	if (g->pmu.clk_pmu->freq_domain_grp_objs != NULL) {
		return 0;
	}

	g->pmu.clk_pmu->freq_domain_grp_objs = nvgpu_kzalloc(g,
			sizeof(*g->pmu.clk_pmu->freq_domain_grp_objs));
	if (g->pmu.clk_pmu->freq_domain_grp_objs == NULL) {
		return -ENOMEM;
	}

	return 0;
}

void nvgpu_clk_freq_domain_free_pmupstate(struct gk20a *g)
{
	nvgpu_kfree(g, g->pmu.clk_pmu->freq_domain_grp_objs);
	g->pmu.clk_pmu->freq_domain_grp_objs = NULL;
}
