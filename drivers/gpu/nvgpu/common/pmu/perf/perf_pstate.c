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

#include "perf_pstate.h"

static int pstate_construct_super(struct gk20a *g, struct boardobj **ppboardobj,
				size_t size, void *args)
{
	struct pstate *ptmppstate = (struct pstate *)args;
	struct pstate *pstate;
	int err;

	err = boardobj_construct_super(g, ppboardobj, size, args);
	if (err != 0) {
		return err;
	}

	pstate = (struct pstate *)*ppboardobj;

	pstate->num = ptmppstate->num;
	pstate->clklist = ptmppstate->clklist;
	pstate->lpwr_entry_idx = ptmppstate->lpwr_entry_idx;

	return 0;
}

static int pstate_construct_3x(struct gk20a *g, struct boardobj **ppboardobj,
				size_t size, void *args)
{
	struct boardobj  *ptmpobj = (struct boardobj *)args;

	ptmpobj->type_mask |= BIT32(CTRL_PERF_PSTATE_TYPE_3X);
	return pstate_construct_super(g, ppboardobj, size, args);
}

static struct pstate *pstate_construct(struct gk20a *g, void *args)
{
	struct pstate *pstate = NULL;
	struct pstate *tmp = (struct pstate *)args;

	if ((tmp->super.type != CTRL_PERF_PSTATE_TYPE_3X) ||
	    (pstate_construct_3x(g, (struct boardobj **)&pstate,
			    sizeof(struct pstate), args) != 0)) {
		nvgpu_err(g,
			"error constructing pstate num=%u", tmp->num);
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

	pstates->num_levels++;

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
	pstate->super.type = CTRL_PERF_PSTATE_TYPE_3X;
	pstate->num = 0x0FU - U32(entry->pstate_level);
	pstate->clklist.num_info = hdr->clock_entry_count;
	pstate->lpwr_entry_idx = entry->lpwr_entry_idx;

	nvgpu_log_info(g, "pstate P%u", pstate->num);

	for (clkidx = 0; clkidx < hdr->clock_entry_count; clkidx++) {
		struct clk_set_info *pclksetinfo;
		struct vbios_pstate_entry_clock_6x *clk_entry;
		struct nvgpu_clk_domain *clk_domain;

		clk_domain = (struct nvgpu_clk_domain *)
			BOARDOBJGRP_OBJ_GET_BY_IDX(
			&g->pmu.clk_pmu->clk_domainobjs->super.super, clkidx);

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

	for (i = 0; i < hdr->base_entry_count; i++, p += entry_size) {
		entry = (struct vbios_pstate_entry_6x *)p;

		if (entry->pstate_level == VBIOS_PERFLEVEL_SKIP_ENTRY) {
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
	}

done:
	return err;
}

int nvgpu_pmu_perf_pstate_sw_setup(struct gk20a *g)
{
	struct vbios_pstate_header_6x *hdr = NULL;
	int err = 0;

	nvgpu_log_fn(g, " ");

	nvgpu_cond_init(&g->perf_pmu->pstatesobjs.pstate_notifier_wq);

	err = nvgpu_mutex_init(&g->perf_pmu->pstatesobjs.pstate_mutex);
	if (err != 0) {
		return err;
	}

	err = boardobjgrpconstruct_e32(g, &g->perf_pmu->pstatesobjs.super);
	if (err != 0) {
		nvgpu_err(g,
			  "error creating boardobjgrp for pstates, err=%d",
			  err);
		goto done;
	}

	hdr = (struct vbios_pstate_header_6x *)
			nvgpu_bios_get_perf_table_ptrs(g,
			g->bios.perf_token, PERFORMANCE_TABLE);

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
	if (err != 0) {
		nvgpu_mutex_destroy(&g->perf_pmu->pstatesobjs.pstate_mutex);
	}
	return err;
}

struct pstate *nvgpu_pmu_perf_pstate_find(struct gk20a *g, u32 num)
{
	struct pstates *pstates = &(g->perf_pmu->pstatesobjs);
	struct pstate *pstate;
	u8 i;

	nvgpu_log_info(g, "pstates = %p", pstates);

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

	nvgpu_log_info(g, "pstate = %p", pstate);

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

