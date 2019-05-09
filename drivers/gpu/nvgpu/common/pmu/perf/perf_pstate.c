/*
 * general p state infrastructure
 *
 * Copyright (c) 2019, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/pmu.h>
#include <nvgpu/boardobj.h>
#include <nvgpu/boardobjgrp.h>
#include <nvgpu/boardobjgrp_e32.h>
#include <nvgpu/boardobjgrp_e255.h>
#include <nvgpu/pmu/perf.h>
#include <nvgpu/pmu/perf_pstate.h>
#include <nvgpu/pmu/clk/clk.h>
#include <nvgpu/pmu/clk/clk_domain.h>
#include <nvgpu/pmu/pmuif/perfpstate.h>

#include "perf_pstate.h"

static int pstate_init_pmudata_super(struct gk20a *g,
		struct boardobj *board_obj_ptr,
		struct nv_pmu_boardobj *ppmudata)
{
	return nvgpu_boardobj_pmu_data_init_super(g, board_obj_ptr, ppmudata);
}

static int pstate_init_pmudata(struct gk20a *g,
		struct boardobj *board_obj_ptr,
		struct nv_pmu_boardobj *ppmudata)
{
	int status = 0;
	u32 clkidx;
	struct pstate *pstate;
	struct nv_pmu_perf_pstate_35 *pstate_pmu_data;

	status = pstate_init_pmudata_super(g, board_obj_ptr, ppmudata);
	if (status != 0) {
		return status;
	}

	pstate = (struct pstate *)board_obj_ptr;
	pstate_pmu_data = (struct nv_pmu_perf_pstate_35 *)ppmudata;

	pstate_pmu_data->super.super.lpwrEntryIdx = pstate->lpwr_entry_idx;
	pstate_pmu_data->super.super.flags = pstate->flags;
	pstate_pmu_data->nvlinkIdx = pstate->nvlink_idx;
	pstate_pmu_data->pcieIdx = pstate->pcie_idx;

	for (clkidx = 0; clkidx < pstate->clklist.num_info; clkidx++) {
		pstate_pmu_data->clkEntries[clkidx].max.baseFreqKhz =
			pstate->clklist.clksetinfo[clkidx].max_mhz*1000;
		pstate_pmu_data->clkEntries[clkidx].max.freqKz =
			pstate->clklist.clksetinfo[clkidx].max_mhz*1000;
		pstate_pmu_data->clkEntries[clkidx].max.origFreqKhz =
			pstate->clklist.clksetinfo[clkidx].max_mhz*1000;
		pstate_pmu_data->clkEntries[clkidx].max.porFreqKhz =
			pstate->clklist.clksetinfo[clkidx].max_mhz*1000;

		pstate_pmu_data->clkEntries[clkidx].min.baseFreqKhz =
			pstate->clklist.clksetinfo[clkidx].min_mhz*1000;
		pstate_pmu_data->clkEntries[clkidx].min.freqKz =
			pstate->clklist.clksetinfo[clkidx].min_mhz*1000;
		pstate_pmu_data->clkEntries[clkidx].min.origFreqKhz =
			pstate->clklist.clksetinfo[clkidx].min_mhz*1000;
		pstate_pmu_data->clkEntries[clkidx].min.porFreqKhz =
			pstate->clklist.clksetinfo[clkidx].min_mhz*1000;

		pstate_pmu_data->clkEntries[clkidx].nom.baseFreqKhz =
			pstate->clklist.clksetinfo[clkidx].nominal_mhz*1000;
		pstate_pmu_data->clkEntries[clkidx].nom.freqKz =
			pstate->clklist.clksetinfo[clkidx].nominal_mhz*1000;
		pstate_pmu_data->clkEntries[clkidx].nom.origFreqKhz =
			pstate->clklist.clksetinfo[clkidx].nominal_mhz*1000;
		pstate_pmu_data->clkEntries[clkidx].nom.porFreqKhz =
			pstate->clklist.clksetinfo[clkidx].nominal_mhz*1000;
	}

	return status;
}

static int pstate_construct_super(struct gk20a *g, struct boardobj **ppboardobj,
				size_t size, void *args)
{
	return nvgpu_boardobj_construct_super(g, ppboardobj, size, args);

}

static int pstate_construct_35(struct gk20a *g, struct boardobj **ppboardobj,
				u16 size, void *args)
{
	struct boardobj  *ptmpobj = (struct boardobj *)args;

	ptmpobj->type_mask |= BIT32(CTRL_PERF_PSTATE_TYPE_35);
	return pstate_construct_super(g, ppboardobj, size, args);
}

static struct pstate *pstate_construct(struct gk20a *g, void *args)
{
	struct pstate *pstate = NULL;
	struct pstate *ptmppstate = (struct pstate *)args;
	int status;
	u32 clkidx;

	status = pstate_construct_35(g, (struct boardobj **)&pstate,
			(u16)sizeof(struct pstate), args);
	if (status != 0) {
		nvgpu_err(g,
			"error constructing pstate num=%u", ptmppstate->num);
		return NULL;
	}

	pstate->super.pmudatainit = pstate_init_pmudata;
	pstate->num = ptmppstate->num;
	pstate->flags = ptmppstate->flags;
	pstate->lpwr_entry_idx = ptmppstate->lpwr_entry_idx;
	pstate->pcie_idx = ptmppstate->pcie_idx;
	pstate->nvlink_idx = ptmppstate->nvlink_idx;
	pstate->clklist.num_info = ptmppstate->clklist.num_info;

	for (clkidx = 0; clkidx < ptmppstate->clklist.num_info; clkidx++) {
		pstate->clklist.clksetinfo[clkidx].clkwhich =
			ptmppstate->clklist.clksetinfo[clkidx].clkwhich;
		pstate->clklist.clksetinfo[clkidx].max_mhz =
			ptmppstate->clklist.clksetinfo[clkidx].max_mhz;
		pstate->clklist.clksetinfo[clkidx].min_mhz =
			ptmppstate->clklist.clksetinfo[clkidx].min_mhz;
		pstate->clklist.clksetinfo[clkidx].nominal_mhz =
			ptmppstate->clklist.clksetinfo[clkidx].nominal_mhz;
	}

	return pstate;
}

static int pstate_insert(struct gk20a *g, struct pstate *pstate, u8 index)
{
	struct pstates *pstates = &(g->perf_pmu->pstatesobjs);
	int err;

	err = boardobjgrp_objinsert(&pstates->super.super,
			(struct boardobj *)pstate, index);
	if (err != 0) {
		nvgpu_err(g,
			  "error adding pstate boardobj %d", index);
		return err;
	}

	pstates->num_clk_domains++;

	return err;
}

static int parse_pstate_entry_6x(struct gk20a *g,
		struct vbios_pstate_header_6x *hdr,
		struct vbios_pstate_entry_6x *entry,
		struct pstate *pstate)
{
	u8 *p = (u8 *)entry;
	u32 clkidx;

	p += hdr->base_entry_size;
	(void) memset(pstate, 0, sizeof(struct pstate));
	pstate->super.type = CTRL_PERF_PSTATE_TYPE_35;
	pstate->num = 0x0FU - U32(entry->pstate_level);
	pstate->clklist.num_info = hdr->clock_entry_count;
	pstate->lpwr_entry_idx = entry->lpwr_entry_idx;
	pstate->flags = entry->flags0;
	pstate->nvlink_idx = entry->nvlink_idx;
	pstate->pcie_idx = entry->pcie_idx;

	for (clkidx = 0; clkidx < hdr->clock_entry_count; clkidx++) {
		struct clk_set_info *pclksetinfo;
		struct vbios_pstate_entry_clock_6x *clk_entry;
		struct nvgpu_clk_domain *clk_domain;

		clk_domain = (struct nvgpu_clk_domain *)
			BOARDOBJGRP_OBJ_GET_BY_IDX(
			&g->pmu->clk_pmu->clk_domainobjs->super.super, clkidx);

		pclksetinfo = &pstate->clklist.clksetinfo[clkidx];
		clk_entry = (struct vbios_pstate_entry_clock_6x *)p;

		pclksetinfo->clkwhich = clk_domain->domain;
		pclksetinfo->nominal_mhz =
			BIOS_GET_FIELD(u32, clk_entry->param0,
				VBIOS_PSTATE_6X_CLOCK_PROG_PARAM0_NOM_FREQ_MHZ);
		pclksetinfo->min_mhz =
			BIOS_GET_FIELD(u16, clk_entry->param1,
				VBIOS_PSTATE_6X_CLOCK_PROG_PARAM1_MIN_FREQ_MHZ);
		pclksetinfo->max_mhz =
			BIOS_GET_FIELD(u16, clk_entry->param1,
				VBIOS_PSTATE_6X_CLOCK_PROG_PARAM1_MAX_FREQ_MHZ);

		nvgpu_log_info(g,
			"clk_domain=%u nominal_mhz=%u min_mhz=%u max_mhz=%u",
			pclksetinfo->clkwhich, pclksetinfo->nominal_mhz,
			pclksetinfo->min_mhz, pclksetinfo->max_mhz);

		p += hdr->clock_entry_size;
	}

	return 0;
}

static int parse_pstate_table_6x(struct gk20a *g,
		struct vbios_pstate_header_6x *hdr)
{
	struct pstate _pstate, *pstate;
	struct vbios_pstate_entry_6x *entry;
	u32 entry_size;
	u8 i;
	u8 *p = (u8 *)hdr;
	int err = 0;

	if ((hdr->header_size != VBIOS_PSTATE_HEADER_6X_SIZE_10) ||
		(hdr->base_entry_count == 0U) ||
		(hdr->clock_entry_size != VBIOS_PSTATE_CLOCK_ENTRY_6X_SIZE_6) ||
		(hdr->clock_entry_count > CLK_SET_INFO_MAX_SIZE)) {
		return -EINVAL;
	}

	p += hdr->header_size;

	entry_size = U32(hdr->base_entry_size) +
			U32(hdr->clock_entry_count) *
			U32(hdr->clock_entry_size);

	for (i = 0; i < hdr->base_entry_count; i++) {
		entry = (struct vbios_pstate_entry_6x *)p;

		if (entry->pstate_level == VBIOS_PERFLEVEL_SKIP_ENTRY) {
			p += entry_size;
			continue;
		}

		err = parse_pstate_entry_6x(g, hdr, entry, &_pstate);
		if (err != 0) {
			goto done;
		}

		pstate = pstate_construct(g, &_pstate);
		if (pstate == NULL) {
			goto done;
		}

		err = pstate_insert(g, pstate, i);
		if (err != 0) {
			goto done;
		}
		p += entry_size;
	}

done:
	return err;
}

static int devinit_get_pstate_table(struct gk20a *g)
{
	struct vbios_pstate_header_6x *hdr = NULL;
	int err = 0;

	hdr = (struct vbios_pstate_header_6x *)
			nvgpu_bios_get_perf_table_ptrs(g,
			nvgpu_bios_get_bit_token(g, NVGPU_BIOS_PERF_TOKEN),
						PERFORMANCE_TABLE);

	if (hdr == NULL) {
		nvgpu_err(g, "performance table not found");
		err = -EINVAL;
		goto done;
	}

	if (hdr->version != VBIOS_PSTATE_TABLE_VERSION_6X) {
		nvgpu_err(g, "unknown/unsupported clocks table version=0x%02x",
				hdr->version);
		err = -EINVAL;
		goto done;
	}

	err = parse_pstate_table_6x(g, hdr);
done:
	return err;
}

static int perf_pstate_pmudatainit(struct gk20a *g,
		struct boardobjgrp *pboardobjgrp,
		struct nv_pmu_boardobjgrp_super *pboardobjgrppmu)
{
	int status = 0;
	struct nv_pmu_perf_pstate_boardobjgrp_set_header *pset =
			(struct nv_pmu_perf_pstate_boardobjgrp_set_header *)
			(void *)pboardobjgrppmu;
	struct pstates *pprogs = (struct pstates *)(void *)pboardobjgrp;

	status = boardobjgrp_pmudatainit_e32(g, pboardobjgrp, pboardobjgrppmu);
	if (status != 0) {
		nvgpu_err(g, "error updating pmu boardobjgrp for vfe equ 0x%x",
			  status);
		goto done;
	}

	pset->numClkDomains = pprogs->num_clk_domains;

done:
	return status;
}

static int perf_pstate_pmudata_instget(struct gk20a *g,
		struct nv_pmu_boardobjgrp *pmuboardobjgrp,
		struct nv_pmu_boardobj **ppboardobjpmudata, u8 idx)
{
	struct nv_pmu_perf_pstate_boardobj_grp_set  *pgrp_set =
		(struct nv_pmu_perf_pstate_boardobj_grp_set *)
		(void *)pmuboardobjgrp;

	/* check whether pmuboardobjgrp has a valid boardobj in index */
	if (idx >= CTRL_BOARDOBJGRP_E32_MAX_OBJECTS) {
		return -EINVAL;
	}

	*ppboardobjpmudata = (struct nv_pmu_boardobj *)
		&pgrp_set->objects[idx].data.boardObj;

	return 0;
}

static int perf_pstate_pmustatus_instget(struct gk20a *g,
		void *pboardobjgrppmu,
		struct nv_pmu_boardobj_query **ppboardobjpmustatus, u8 idx)
{
	struct nv_pmu_perf_pstate_boardobj_grp_get_status *pgrp_get_status =
		(struct nv_pmu_perf_pstate_boardobj_grp_get_status *)(void *)
		pboardobjgrppmu;

	/*Check for valid pmuboardobjgrp index*/
	if ((BIT32(idx) &
		pgrp_get_status->hdr.data.super.obj_mask.super.data[0]) == 0U) {
		return -EINVAL;
	}

	*ppboardobjpmustatus = (struct nv_pmu_boardobj_query *)
			&pgrp_get_status->objects[idx].data.board_obj;
	return 0;
}

int nvgpu_pmu_perf_pstate_sw_setup(struct gk20a *g)
{
	int status;
	struct boardobjgrp *pboardobjgrp = NULL;

	status = nvgpu_boardobjgrp_construct_e32(g,
			&g->perf_pmu->pstatesobjs.super);
	if (status != 0) {
		nvgpu_err(g,
			"error creating boardobjgrp for pstate, status - 0x%x",
			status);
		goto done;
	}

	pboardobjgrp = &g->perf_pmu->pstatesobjs.super.super;

	BOARDOBJGRP_PMU_CONSTRUCT(pboardobjgrp, PERF, PSTATE);

	status = BOARDOBJGRP_PMU_CMD_GRP_SET_CONSTRUCT(g, pboardobjgrp,
			perf, PERF, pstate, PSTATE);
	if (status != 0) {
		nvgpu_err(g,
			"error constructing PSTATE_SET interface - 0x%x",
			status);
		goto done;
	}

	g->perf_pmu->pstatesobjs.num_clk_domains =
			VBIOS_PSTATE_CLOCK_ENTRY_6X_COUNT;

	status = BOARDOBJGRP_PMU_CMD_GRP_GET_STATUS_CONSTRUCT(g, pboardobjgrp,
			perf, PERF, pstate, PSTATE);
	if (status != 0) {
		nvgpu_err(g,
			"error constructing PSTATE_GET_STATUS interface - 0x%x",
			status);
		goto done;
	}

	pboardobjgrp->pmudatainit = perf_pstate_pmudatainit;
	pboardobjgrp->pmudatainstget  = perf_pstate_pmudata_instget;
	pboardobjgrp->pmustatusinstget  = perf_pstate_pmustatus_instget;

	status = devinit_get_pstate_table(g);
	if (status != 0) {
		nvgpu_err(g, "Error parsing the performance Vbios tables");
		goto done;
	}

done:
	return status;
}

int nvgpu_pmu_perf_pstate_pmu_setup(struct gk20a *g)
{
	int status;
	struct boardobjgrp *pboardobjgrp = NULL;

	pboardobjgrp = &g->perf_pmu->pstatesobjs.super.super;
	if (!pboardobjgrp->bconstructed) {
		return -EINVAL;
	}

	status = pboardobjgrp->pmuinithandle(g, pboardobjgrp);

	return status;
}


struct pstate *nvgpu_pmu_perf_pstate_find(struct gk20a *g, u32 num)
{
	struct pstates *pstates = &(g->perf_pmu->pstatesobjs);
	struct pstate *pstate;
	u8 i;

	BOARDOBJGRP_FOR_EACH(&pstates->super.super,
			struct pstate *, pstate, i) {
		nvgpu_log_info(g, "pstate=%p num=%u (looking for num=%u)",
				pstate, pstate->num, num);
		if (pstate->num == num) {
			return pstate;
		}
	}
	return NULL;
}

struct clk_set_info *nvgpu_pmu_perf_pstate_get_clk_set_info(struct gk20a *g,
		u32 pstate_num, u32 clkwhich)
{
	struct pstate *pstate = nvgpu_pmu_perf_pstate_find(g, pstate_num);
	struct clk_set_info *info;
	u32 clkidx;

	if (pstate == NULL) {
		return NULL;
	}

	for (clkidx = 0; clkidx < pstate->clklist.num_info; clkidx++) {
		info = &pstate->clklist.clksetinfo[clkidx];
		if (info->clkwhich == clkwhich) {
			return info;
		}
	}
	return NULL;
}

