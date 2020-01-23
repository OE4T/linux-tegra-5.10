/*
 * Copyright (c) 2016-2020, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/boardobjgrp_e255.h>
#include <nvgpu/pmu/boardobjgrp_classes.h>
#include <nvgpu/boardobjgrpmask.h>
#include <nvgpu/pmu/pmuif/ctrlclk.h>
#include <nvgpu/pmu/clk/clk_vf_point.h>
#include <nvgpu/pmu/perf.h>
#include <nvgpu/string.h>
#include <nvgpu/pmu/pmuif/ctrlvolt.h>
#include <nvgpu/pmu/volt.h>
#include <nvgpu/pmu/clk/clk.h>
#include <nvgpu/pmu/cmd.h>

#include "clk_domain.h"
#include "clk_prog.h"

static struct nvgpu_clk_domain *construct_clk_domain(struct gk20a *g,
		void *pargs);

static int devinit_get_clocks_table(struct gk20a *g,
	struct nvgpu_clk_domains *pclkdomainobjs);

static int clk_domain_pmudatainit_super(struct gk20a *g, struct boardobj
	*board_obj_ptr,	struct nv_pmu_boardobj *ppmudata);

struct vbios_clocks_table_1x_hal_clock_entry {
	u32 domain;
	bool b_noise_aware_capable;
	u8 clk_vf_curve_count;
};

static struct vbios_clocks_table_1x_hal_clock_entry
		vbiosclktbl1xhalentry_gv[] = {
	{ CLKWHICH_GPCCLK,     true,    1, },
	{ CLKWHICH_XBARCLK,    true,    1, },
	{ CLKWHICH_MCLK,       false,   1, },
	{ CLKWHICH_SYSCLK,     true,    1, },
	{ CLKWHICH_HUBCLK,     false,   1, },
	{ CLKWHICH_NVDCLK,     true,    1, },
	{ CLKWHICH_PWRCLK,     false,   1, },
	{ CLKWHICH_DISPCLK,    false,   1, },
	{ CLKWHICH_PCIEGENCLK, false,   1, },
	{ CLKWHICH_HOSTCLK,    true,    1, }
};

static u32 clktranslatehalmumsettoapinumset(u32 clkhaldomains)
{
	u32   clkapidomains = 0;

	if ((clkhaldomains & BIT32(CLKWHICH_GPCCLK)) != 0U) {
		clkapidomains |= CTRL_CLK_DOMAIN_GPCCLK;
	}
	if ((clkhaldomains & BIT32(CLKWHICH_XBARCLK)) != 0U) {
		clkapidomains |= CTRL_CLK_DOMAIN_XBARCLK;
	}
	if ((clkhaldomains & BIT32(CLKWHICH_SYSCLK)) != 0U) {
		clkapidomains |= CTRL_CLK_DOMAIN_SYSCLK;
	}
	if ((clkhaldomains & BIT32(CLKWHICH_HUBCLK)) != 0U) {
		clkapidomains |= CTRL_CLK_DOMAIN_HUBCLK;
	}
	if ((clkhaldomains & BIT32(CLKWHICH_HOSTCLK)) != 0U) {
		clkapidomains |= CTRL_CLK_DOMAIN_HOSTCLK;
	}
	if ((clkhaldomains & BIT32(CLKWHICH_GPC2CLK)) != 0U) {
		clkapidomains |= CTRL_CLK_DOMAIN_GPC2CLK;
	}
	if ((clkhaldomains & BIT32(CLKWHICH_XBAR2CLK)) != 0U) {
		clkapidomains |= CTRL_CLK_DOMAIN_XBAR2CLK;
	}
	if ((clkhaldomains & BIT32(CLKWHICH_SYS2CLK)) != 0U) {
		clkapidomains |= CTRL_CLK_DOMAIN_SYS2CLK;
	}
	if ((clkhaldomains & BIT32(CLKWHICH_HUB2CLK)) != 0U) {
		clkapidomains |= CTRL_CLK_DOMAIN_HUB2CLK;
	}
	if ((clkhaldomains & BIT32(CLKWHICH_PWRCLK)) != 0U) {
		clkapidomains |= CTRL_CLK_DOMAIN_PWRCLK;
	}
	if ((clkhaldomains & BIT32(CLKWHICH_PCIEGENCLK)) != 0U) {
		clkapidomains |= CTRL_CLK_DOMAIN_PCIEGENCLK;
	}
	if ((clkhaldomains & BIT32(CLKWHICH_MCLK)) != 0U) {
		clkapidomains |= CTRL_CLK_DOMAIN_MCLK;
	}
	if ((clkhaldomains & BIT32(CLKWHICH_NVDCLK)) != 0U) {
		clkapidomains |= CTRL_CLK_DOMAIN_NVDCLK;
	}
	if ((clkhaldomains & BIT32(CLKWHICH_DISPCLK)) != 0U) {
		clkapidomains |= CTRL_CLK_DOMAIN_DISPCLK;
	}

	return clkapidomains;
}

static struct nvgpu_clk_domain *clk_get_clk_domain_from_index(
		struct nvgpu_clk_pmupstate *pclk, u8 idx)
{
	return (struct nvgpu_clk_domain *)(void *)BOARDOBJGRP_OBJ_GET_BY_IDX(
			&(pclk->clk_domainobjs->super.super), idx);
}

static int _clk_domains_pmudatainit_3x(struct gk20a *g,
		struct boardobjgrp *pboardobjgrp,
		struct nv_pmu_boardobjgrp_super *pboardobjgrppmu)
{
	struct nv_pmu_clk_clk_domain_boardobjgrp_set_header *pset =
		(struct nv_pmu_clk_clk_domain_boardobjgrp_set_header *)
		(void *)pboardobjgrppmu;
	struct nvgpu_clk_domains *pdomains =
		(struct nvgpu_clk_domains *)(void *)pboardobjgrp;
	int status = 0;

	status = boardobjgrp_pmudatainit_e32(g, pboardobjgrp, pboardobjgrppmu);
	if (status != 0) {
		nvgpu_err(g,
			  "error updating pmu boardobjgrp for clk domain 0x%x",
			  status);
		goto done;
	}

	pset->vbios_domains = pdomains->vbios_domains;
	pset->cntr_sampling_periodms = pdomains->cntr_sampling_periodms;
	pset->clkmon_refwin_usec = pdomains->clkmon_refwin_usec;
	pset->version = pdomains->version;
	pset->b_override_o_v_o_c = false;
	pset->b_debug_mode = false;
	pset->b_enforce_vf_monotonicity = pdomains->b_enforce_vf_monotonicity;
	pset->b_enforce_vf_smoothening = pdomains->b_enforce_vf_smoothening;
	if (g->ops.clk.split_rail_support) {
		pset->volt_rails_max = 2;
	} else {
		pset->volt_rails_max = 1;
	}
	status = nvgpu_boardobjgrpmask_export(
				&pdomains->master_domains_mask.super,
				pdomains->master_domains_mask.super.bitcount,
				&pset->master_domains_mask.super);
	if (status != 0) {
		nvgpu_err(g, "Error exporting Clk master domains masks");
		return status;
	}

	status = nvgpu_boardobjgrpmask_export(
				&pdomains->prog_domains_mask.super,
				pdomains->prog_domains_mask.super.bitcount,
				&pset->prog_domains_mask.super);
	if (status != 0) {
		nvgpu_err(g, "Error exporting Clk prog domains masks");
		return status;
	}

	status = nvgpu_boardobjgrpmask_export(
				&pdomains->clkmon_domains_mask.super,
				pdomains->clkmon_domains_mask.super.bitcount,
				&pset->clkmon_domains_mask.super);
	if (status != 0) {
		nvgpu_err(g, "Error exporting Clk monitor domains masks");
		return status;
	}
	nvgpu_memcpy((u8 *)&pset->deltas, (u8 *)&pdomains->deltas,
		(sizeof(struct ctrl_clk_clk_delta)));

done:
	return status;
}

static int _clk_domains_pmudata_instget(struct gk20a *g,
		struct nv_pmu_boardobjgrp *pmuboardobjgrp,
		struct nv_pmu_boardobj **ppboardobjpmudata, u8 idx)
{
	struct nv_pmu_clk_clk_domain_boardobj_grp_set  *pgrp_set =
		(struct nv_pmu_clk_clk_domain_boardobj_grp_set *)(void *)
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

int nvgpu_clk_domain_sw_setup(struct gk20a *g)
{
	int status;
	struct boardobjgrp *pboardobjgrp = NULL;
	struct nvgpu_clk_domains *pclkdomainobjs;
	struct nvgpu_clk_domain *pdomain;
	struct clk_domain_35_master *pdomain_master_35;
	struct clk_domain_35_slave *pdomain_slave_35;
	struct clk_domain_35_prog *pdomain_prog_35;
	u8 i;

	nvgpu_log_info(g, " ");

	status = nvgpu_boardobjgrp_construct_e32(g,
			&g->pmu->clk_pmu->clk_domainobjs->super);
	if (status != 0) {
		nvgpu_err(g,
		     "error creating boardobjgrp for clk domain, status - 0x%x",
		     status);
		goto done;
	}

	pboardobjgrp = &g->pmu->clk_pmu->clk_domainobjs->super.super;
	pclkdomainobjs = g->pmu->clk_pmu->clk_domainobjs;

	BOARDOBJGRP_PMU_CONSTRUCT(pboardobjgrp, CLK, CLK_DOMAIN);

	status = BOARDOBJGRP_PMU_CMD_GRP_SET_CONSTRUCT(g, pboardobjgrp,
			clk, CLK, clk_domain, CLK_DOMAIN);
	if (status != 0) {
		nvgpu_err(g,
		 "error constructing PMU_BOARDOBJ_CMD_GRP_SET interface - 0x%x",
		 status);
		goto done;
	}

	pboardobjgrp->pmudatainit  = _clk_domains_pmudatainit_3x;
	pboardobjgrp->pmudatainstget  = _clk_domains_pmudata_instget;

	/* Initialize mask to zero.*/
	status = boardobjgrpmask_e32_init(&pclkdomainobjs->prog_domains_mask,
			NULL);
	if (status != 0) {
		nvgpu_err(g, "boardobjgrpmask_e32_init(prog) failed err=%d",
			status);
		goto done;
	}
	status = boardobjgrpmask_e32_init(&pclkdomainobjs->master_domains_mask,
			NULL);
	if (status != 0) {
		nvgpu_err(g, "boardobjgrpmask_e32_init(master) failed err=%d",
			status);
		goto done;
	}
	status = boardobjgrpmask_e32_init(&pclkdomainobjs->clkmon_domains_mask,
			NULL);
	if (status != 0) {
		nvgpu_err(g, "boardobjgrpmask_e32_init(clkmon) failed err=%d",
			status);
		goto done;
	}

	pclkdomainobjs->b_enforce_vf_monotonicity = true;
	pclkdomainobjs->b_enforce_vf_smoothening = true;

	(void) memset(&pclkdomainobjs->ordered_noise_aware_list, 0,
		sizeof(pclkdomainobjs->ordered_noise_aware_list));

	(void) memset(&pclkdomainobjs->ordered_noise_unaware_list, 0,
		sizeof(pclkdomainobjs->ordered_noise_unaware_list));

	(void) memset(&pclkdomainobjs->deltas, 0,
		sizeof(struct ctrl_clk_clk_delta));

	status = devinit_get_clocks_table(g, pclkdomainobjs);
	if (status != 0) {
		goto done;
	}

	BOARDOBJGRP_FOR_EACH(&(pclkdomainobjs->super.super),
			     struct nvgpu_clk_domain *, pdomain, i) {
		pdomain_master_35 = NULL;

		if (pdomain->super.implements(g, &pdomain->super,
				CTRL_CLK_CLK_DOMAIN_TYPE_35_PROG)) {
			pdomain_prog_35 =
				(struct clk_domain_35_prog *)(void *)pdomain;
			status = nvgpu_boardobjgrpmask_bit_set(
				&pclkdomainobjs->prog_domains_mask.super, i);
			if (status != 0) {
				goto done;
			}

			/* Create the mask of clk monitors that are supported */
			if ((pdomain_prog_35->
					clkmon_info.high_threshold_vfe_idx !=
						CLK_CLKMON_VFE_INDEX_INVALID) ||
				(pdomain_prog_35->
					clkmon_info.low_threshold_vfe_idx !=
						CLK_CLKMON_VFE_INDEX_INVALID)) {
				status = nvgpu_boardobjgrpmask_bit_set(
						&pclkdomainobjs->
						clkmon_domains_mask.super, i);
				if (status != 0) {
					nvgpu_err(g,
					"Error setting Clk monitor masks");
					return status;
				}
			}
		}

		if (pdomain->super.implements(g, &pdomain->super,
				CTRL_CLK_CLK_DOMAIN_TYPE_35_MASTER)) {
			status = nvgpu_boardobjgrpmask_bit_set(
				&pclkdomainobjs->master_domains_mask.super, i);
			if (status != 0) {
				goto done;
			}
			pdomain_master_35 =
				(struct clk_domain_35_master *)(void *)pdomain;
			status = nvgpu_boardobjgrpmask_bit_set(
					&pdomain_master_35->
					master_slave_domains_grp_mask.super, i);
			if (status != 0) {
				goto done;
			}
		}

		if (pdomain->super.implements(g, &pdomain->super,
				CTRL_CLK_CLK_DOMAIN_TYPE_35_SLAVE)) {
			pdomain_slave_35 =
				(struct clk_domain_35_slave *)(void *)pdomain;
			pdomain_master_35 = (struct clk_domain_35_master *)
				(void *)
				(g->pmu->clk_pmu->clk_get_clk_domain(
				(g->pmu->clk_pmu),
				pdomain_slave_35->slave.master_idx));
			pdomain_master_35->master.slave_idxs_mask |= BIT32(i);
			pdomain_slave_35->super.clk_pos =
				nvgpu_boardobjgrpmask_bit_set_count(
				&pdomain_master_35->
				master_slave_domains_grp_mask.super);
			status = nvgpu_boardobjgrpmask_bit_set(
				&pdomain_master_35->
				master_slave_domains_grp_mask.super, i);
			if (status != 0) {
				goto done;
			}
		}

	}

done:
	nvgpu_log_info(g, " done status %x", status);
	return status;
}

int nvgpu_clk_domain_pmu_setup(struct gk20a *g)
{
	int status;
	struct boardobjgrp *pboardobjgrp = NULL;

	nvgpu_log_info(g, " ");

	pboardobjgrp = &g->pmu->clk_pmu->clk_domainobjs->super.super;

	if (!pboardobjgrp->bconstructed) {
		return -EINVAL;
	}

	status = pboardobjgrp->pmuinithandle(g, pboardobjgrp);

	nvgpu_log_info(g, "Done");
	return status;
}

static int devinit_get_clocks_table_35(struct gk20a *g,
		struct nvgpu_clk_domains *pclkdomainobjs, u8 *clocks_table_ptr)
{
	int status = 0;
	struct vbios_clocks_table_35_header clocks_table_header = { 0 };
	struct vbios_clocks_table_35_entry clocks_table_entry = { 0 };
	struct vbios_clocks_table_1x_hal_clock_entry *vbiosclktbl1xhalentry;
	u8 *clocks_tbl_entry_ptr = NULL;
	u32 index = 0;
	bool done = false;
	struct nvgpu_clk_domain *pclkdomain_dev;
	union {
		struct boardobj board_obj;
		struct nvgpu_clk_domain clk_domain;
		struct clk_domain_3x v3x;
		struct clk_domain_3x_fixed v3x_fixed;
		struct clk_domain_35_prog v35_prog;
		struct clk_domain_35_master v35_master;
		struct clk_domain_35_slave v35_slave;
	} clk_domain_data;

	nvgpu_log_info(g, " ");
	pclkdomainobjs->version = CLK_DOMAIN_BOARDOBJGRP_VERSION_35;

	nvgpu_memcpy((u8 *)&clocks_table_header, clocks_table_ptr,
			VBIOS_CLOCKS_TABLE_35_HEADER_SIZE_09);
	if (clocks_table_header.header_size <
			(u8) VBIOS_CLOCKS_TABLE_35_HEADER_SIZE_09) {
		status = -EINVAL;
		goto done;
	}

	if (clocks_table_header.entry_size <
			(u8) VBIOS_CLOCKS_TABLE_35_ENTRY_SIZE_11) {
		status = -EINVAL;
		goto done;
	}

	switch (clocks_table_header.clocks_hal) {
	case CLK_TABLE_HAL_ENTRY_GV:
		vbiosclktbl1xhalentry = vbiosclktbl1xhalentry_gv;
		break;
	default:
		status = -EINVAL;
		break;
	}

	if (status == -EINVAL) {
		goto done;
	}

	pclkdomainobjs->cntr_sampling_periodms =
		(u16)clocks_table_header.cntr_sampling_periodms;
	pclkdomainobjs->clkmon_refwin_usec =
		(u16)clocks_table_header.reference_window;

	/* Read table entries*/
	clocks_tbl_entry_ptr = clocks_table_ptr +
			clocks_table_header.header_size;
	for (index = 0; index < clocks_table_header.entry_count; index++) {
		nvgpu_memcpy((u8 *)&clocks_table_entry,
			clocks_tbl_entry_ptr, clocks_table_header.entry_size);
		clk_domain_data.clk_domain.domain =
				(u8) vbiosclktbl1xhalentry[index].domain;
		clk_domain_data.clk_domain.api_domain =
				clktranslatehalmumsettoapinumset(
				(u32) BIT(clk_domain_data.clk_domain.domain));
		clk_domain_data.v3x.b_noise_aware_capable =
			vbiosclktbl1xhalentry[index].b_noise_aware_capable;

		switch (BIOS_GET_FIELD(u32, clocks_table_entry.flags0,
				NV_VBIOS_CLOCKS_TABLE_1X_ENTRY_FLAGS0_USAGE)) {
		case  NV_VBIOS_CLOCKS_TABLE_1X_ENTRY_FLAGS0_USAGE_FIXED:
		{
			clk_domain_data.board_obj.type =
				CTRL_CLK_CLK_DOMAIN_TYPE_3X_FIXED;
			clk_domain_data.v3x_fixed.freq_mhz = BIOS_GET_FIELD(u16,
				clocks_table_entry.param1,
				NV_VBIOS_CLOCKS_TABLE_1X_ENTRY_PARAM1_FIXED_FREQUENCY_MHZ);
			break;
		}

		case  NV_VBIOS_CLOCKS_TABLE_1X_ENTRY_FLAGS0_USAGE_MASTER:
		{
			clk_domain_data.board_obj.type =
				CTRL_CLK_CLK_DOMAIN_TYPE_35_MASTER;
			clk_domain_data.v35_prog.super.clk_prog_idx_first =
				BIOS_GET_FIELD(u8, clocks_table_entry.param0,
				NV_VBIOS_CLOCKS_TABLE_1X_ENTRY_PARAM0_PROG_CLK_PROG_IDX_FIRST);
			clk_domain_data.v35_prog.super.clk_prog_idx_last =
				BIOS_GET_FIELD(u8, clocks_table_entry.param0,
				NV_VBIOS_CLOCKS_TABLE_1X_ENTRY_PARAM0_PROG_CLK_PROG_IDX_LAST);
			clk_domain_data.v35_prog.super.noise_unaware_ordering_index =
				BIOS_GET_FIELD(u8, clocks_table_entry.param2,
				NV_VBIOS_CLOCKS_TABLE_1X_ENTRY_PARAM2_PROG_NOISE_UNAWARE_ORDERING_IDX);
			if (clk_domain_data.v3x.b_noise_aware_capable) {
				clk_domain_data.v35_prog.super.b_force_noise_unaware_ordering =
				BIOS_GET_FIELD(bool, clocks_table_entry.param2,
				NV_VBIOS_CLOCKS_TABLE_1X_ENTRY_PARAM2_PROG_FORCE_NOISE_UNAWARE_ORDERING);

			} else {
				clk_domain_data.v35_prog.super.noise_aware_ordering_index =
					CTRL_CLK_CLK_DOMAIN_3X_PROG_ORDERING_INDEX_INVALID;
				clk_domain_data.v35_prog.super.b_force_noise_unaware_ordering =
						false;
			}
			clk_domain_data.v35_prog.pre_volt_ordering_index =
				BIOS_GET_FIELD(u8, clocks_table_entry.param2,
				NV_VBIOS_CLOCKS_TABLE_35_ENTRY_PARAM2_PROG_PRE_VOLT_ORDERING_IDX);

			clk_domain_data.v35_prog.post_volt_ordering_index =
				BIOS_GET_FIELD(u8, clocks_table_entry.param2,
				NV_VBIOS_CLOCKS_TABLE_35_ENTRY_PARAM2_PROG_POST_VOLT_ORDERING_IDX);

			clk_domain_data.v35_prog.super.factory_delta.data.delta_khz = 0;
			clk_domain_data.v35_prog.super.factory_delta.type = 0;

			clk_domain_data.v35_prog.super.freq_delta_min_mhz =
				BIOS_GET_FIELD(s16, clocks_table_entry.param1,
				NV_VBIOS_CLOCKS_TABLE_1X_ENTRY_PARAM1_MASTER_FREQ_OC_DELTA_MIN_MHZ);

			clk_domain_data.v35_prog.super.freq_delta_max_mhz =
				BIOS_GET_FIELD(s16, clocks_table_entry.param1,
				NV_VBIOS_CLOCKS_TABLE_1X_ENTRY_PARAM1_MASTER_FREQ_OC_DELTA_MAX_MHZ);
			clk_domain_data.v35_prog.clk_vf_curve_count =
				vbiosclktbl1xhalentry[index].clk_vf_curve_count;

			clk_domain_data.v35_prog.clkmon_info.low_threshold_vfe_idx =
				BIOS_GET_FIELD(u8, clocks_table_entry.param3,
				NV_VBIOS_CLOCKS_TABLE_35_ENTRY_PARAM3_CLK_MONITOR_THRESHOLD_MIN);
			clk_domain_data.v35_prog.clkmon_info.high_threshold_vfe_idx =
				BIOS_GET_FIELD(u8, clocks_table_entry.param3,
				NV_VBIOS_CLOCKS_TABLE_35_ENTRY_PARAM3_CLK_MONITOR_THRESHOLD_MAX);
			break;
		}

		case  NV_VBIOS_CLOCKS_TABLE_1X_ENTRY_FLAGS0_USAGE_SLAVE:
		{
			clk_domain_data.board_obj.type =
				CTRL_CLK_CLK_DOMAIN_TYPE_35_SLAVE;
			clk_domain_data.v35_prog.super.clk_prog_idx_first =
				BIOS_GET_FIELD(u8, clocks_table_entry.param0,
				NV_VBIOS_CLOCKS_TABLE_1X_ENTRY_PARAM0_PROG_CLK_PROG_IDX_FIRST);
			clk_domain_data.v35_prog.super.clk_prog_idx_last =
				BIOS_GET_FIELD(u8, clocks_table_entry.param0,
				NV_VBIOS_CLOCKS_TABLE_1X_ENTRY_PARAM0_PROG_CLK_PROG_IDX_LAST);
			clk_domain_data.v35_prog.super.noise_unaware_ordering_index =
				BIOS_GET_FIELD(u8, clocks_table_entry.param2,
				NV_VBIOS_CLOCKS_TABLE_1X_ENTRY_PARAM2_PROG_NOISE_UNAWARE_ORDERING_IDX);

			if (clk_domain_data.v3x.b_noise_aware_capable) {
				clk_domain_data.v35_prog.super.b_force_noise_unaware_ordering =
				BIOS_GET_FIELD(bool, clocks_table_entry.param2,
				NV_VBIOS_CLOCKS_TABLE_1X_ENTRY_PARAM2_PROG_FORCE_NOISE_UNAWARE_ORDERING);

			} else {
				clk_domain_data.v35_prog.super.noise_aware_ordering_index =
					CTRL_CLK_CLK_DOMAIN_3X_PROG_ORDERING_INDEX_INVALID;
				clk_domain_data.v35_prog.super.b_force_noise_unaware_ordering =
						false;
			}
			clk_domain_data.v35_prog.pre_volt_ordering_index =
				BIOS_GET_FIELD(u8, clocks_table_entry.param2,
				NV_VBIOS_CLOCKS_TABLE_35_ENTRY_PARAM2_PROG_PRE_VOLT_ORDERING_IDX);

			clk_domain_data.v35_prog.post_volt_ordering_index =
				BIOS_GET_FIELD(u8, clocks_table_entry.param2,
				NV_VBIOS_CLOCKS_TABLE_35_ENTRY_PARAM2_PROG_POST_VOLT_ORDERING_IDX);

			clk_domain_data.v35_prog.super.factory_delta.data.delta_khz = 0;
			clk_domain_data.v35_prog.super.factory_delta.type = 0;
			clk_domain_data.v35_prog.super.freq_delta_min_mhz = 0;
			clk_domain_data.v35_prog.super.freq_delta_max_mhz = 0;
			clk_domain_data.v35_slave.slave.master_idx =
				BIOS_GET_FIELD(u8, clocks_table_entry.param1,
				NV_VBIOS_CLOCKS_TABLE_1X_ENTRY_PARAM1_SLAVE_MASTER_DOMAIN);

			clk_domain_data.v35_prog.clkmon_info.low_threshold_vfe_idx =
				BIOS_GET_FIELD(u8, clocks_table_entry.param3,
				NV_VBIOS_CLOCKS_TABLE_35_ENTRY_PARAM3_CLK_MONITOR_THRESHOLD_MIN);
			clk_domain_data.v35_prog.clkmon_info.high_threshold_vfe_idx =
				BIOS_GET_FIELD(u8, clocks_table_entry.param3,
				NV_VBIOS_CLOCKS_TABLE_35_ENTRY_PARAM3_CLK_MONITOR_THRESHOLD_MAX);
			break;
		}

		default:
		{
			nvgpu_err(g,
				  "error reading clock domain entry %d", index);
			status = -EINVAL;
			done = true;
			break;
		}

		}
		/*
		 * Previously we were doing "goto done" from the default case of
		 * the switch-case block above. MISRA however, gets upset about
		 * this because it wants a break statement in the default case.
		 * That's why we had to move the goto statement outside of the
		 * switch-case block.
		 */
		if(done) {
			goto done;
		}

		pclkdomain_dev = construct_clk_domain(g,
				(void *)&clk_domain_data);
		if (pclkdomain_dev == NULL) {
			nvgpu_err(g,
			"unable to construct clock domain boardobj for %d",
			index);
			status = -EINVAL;
			goto done;
		}
		status = boardobjgrp_objinsert(&pclkdomainobjs->super.super,
				(struct boardobj *)(void *)
				pclkdomain_dev, index);
		if (status != 0) {
			nvgpu_err(g,
			"unable to insert clock domain boardobj for %d", index);
			status = -EINVAL;
			goto done;
		}
		clocks_tbl_entry_ptr += clocks_table_header.entry_size;
	}

done:
	nvgpu_log_info(g, " done status %x", status);
	return status;
}

static int devinit_get_clocks_table(struct gk20a *g,
	    struct nvgpu_clk_domains *pclkdomainobjs)
{
	int status = 0;
	u8 *clocks_table_ptr = NULL;
	struct vbios_clocks_table_35_header clocks_table_header = { 0 };
	nvgpu_log_info(g, " ");

	clocks_table_ptr = (u8 *)nvgpu_bios_get_perf_table_ptrs(g,
			nvgpu_bios_get_bit_token(g, NVGPU_BIOS_CLOCK_TOKEN),
							CLOCKS_TABLE);
	if (clocks_table_ptr == NULL) {
		status = -EINVAL;
		goto done;
	}
	nvgpu_memcpy((u8 *)&clocks_table_header, clocks_table_ptr,
			VBIOS_CLOCKS_TABLE_35_HEADER_SIZE_09);

	status = devinit_get_clocks_table_35(g, pclkdomainobjs,
			clocks_table_ptr);

done:
	return status;

}

static int clk_domain_construct_super(struct gk20a *g,
				      struct boardobj **ppboardobj,
				      size_t size, void *pargs)
{
	struct nvgpu_clk_domain *pdomain;
	struct nvgpu_clk_domain *ptmpdomain = (struct nvgpu_clk_domain *)pargs;
	int status = 0;

	status = nvgpu_boardobj_construct_super(g, ppboardobj,
		(u16)size, pargs);

	if (status != 0) {
		return -EINVAL;
	}

	pdomain = (struct nvgpu_clk_domain *)(void *)*ppboardobj;

	pdomain->super.pmudatainit =
			clk_domain_pmudatainit_super;

	pdomain->api_domain = ptmpdomain->api_domain;
	pdomain->domain = ptmpdomain->domain;
	pdomain->perf_domain_grp_idx =
		ptmpdomain->perf_domain_grp_idx;

	return status;
}

static int _clk_domain_pmudatainit_3x(struct gk20a *g,
				      struct boardobj *board_obj_ptr,
				      struct nv_pmu_boardobj *ppmudata)
{
	int status = 0;
	struct clk_domain_3x *pclk_domain_3x;
	struct nv_pmu_clk_clk_domain_3x_boardobj_set *pset;

	nvgpu_log_info(g, " ");

	status = clk_domain_pmudatainit_super(g, board_obj_ptr, ppmudata);
	if (status != 0) {
		return status;
	}

	pclk_domain_3x = (struct clk_domain_3x *)(void *)board_obj_ptr;

	pset = (struct nv_pmu_clk_clk_domain_3x_boardobj_set *)(void *)ppmudata;

	pset->b_noise_aware_capable = pclk_domain_3x->b_noise_aware_capable;

	return status;
}

static int clk_domain_construct_3x(struct gk20a *g,
				   struct boardobj **ppboardobj,
				   size_t size, void *pargs)
{
	struct boardobj *ptmpobj = (struct boardobj *)pargs;
	struct clk_domain_3x *pdomain;
	struct clk_domain_3x *ptmpdomain =
			(struct clk_domain_3x *)pargs;
	int status = 0;

	ptmpobj->type_mask = BIT32(CTRL_CLK_CLK_DOMAIN_TYPE_3X);
	status = clk_domain_construct_super(g, ppboardobj,
					size, pargs);
	if (status != 0) {
		return -EINVAL;
	}

	pdomain = (struct clk_domain_3x *)(void *)*ppboardobj;

	pdomain->super.super.pmudatainit =
			_clk_domain_pmudatainit_3x;

	pdomain->b_noise_aware_capable = ptmpdomain->b_noise_aware_capable;

	return status;
}

static int clkdomainclkproglink_3x_prog(struct gk20a *g,
					struct nvgpu_clk_pmupstate *pclk,
					struct nvgpu_clk_domain *pdomain)
{
	int status = 0;
	struct clk_domain_3x_prog *p3xprog =
		(struct clk_domain_3x_prog *)(void *)pdomain;
	struct clk_prog *pprog = NULL;
	u8 i;

	nvgpu_log_info(g, " ");

	for (i = p3xprog->clk_prog_idx_first;
	     i <= p3xprog->clk_prog_idx_last;
	     i++) {
		pprog = CLK_CLK_PROG_GET(pclk, i);
		if (pprog == NULL) {
			status = -EINVAL;
		}
	}
	return status;
}

static int clkdomaingetslaveclk(struct gk20a *g,
				struct nvgpu_clk_pmupstate *pclk,
				struct nvgpu_clk_domain *pdomain,
				u16 *pclkmhz,
				u16 masterclkmhz)
{
	int status = 0;
	struct clk_prog *pprog = NULL;
	struct clk_prog_1x_master *pprog1xmaster = NULL;
	u8 slaveidx;
	struct clk_domain_35_master *p35master;
	nvgpu_log_info(g, " ");

	if (pclkmhz == NULL) {
		return -EINVAL;
	}
	if (masterclkmhz == 0U) {
		return -EINVAL;
	}
	slaveidx = BOARDOBJ_GET_IDX(pdomain);
	p35master = (struct clk_domain_35_master *)(void *)
		g->pmu->clk_pmu->clk_get_clk_domain(pclk,
		((struct clk_domain_35_slave *)
		(void *)pdomain)->slave.master_idx);
	pprog = CLK_CLK_PROG_GET(pclk, p35master->
			master.super.clk_prog_idx_first);
	pprog1xmaster = (struct clk_prog_1x_master *)(void *)pprog;

	status = pprog1xmaster->getslaveclk(g, pclk, pprog1xmaster,
			slaveidx, pclkmhz, masterclkmhz, &pdomain->ratio_domain);

	return status;
}

static int clkdomainvfsearch(struct gk20a *g,
				struct nvgpu_clk_pmupstate *pclk,
				struct nvgpu_clk_domain *pdomain,
				u16 *pclkmhz,
				u32 *pvoltuv,
				u8 rail)
{
	int status = 0;
	struct clk_domain_3x_master *p3xmaster  =
		(struct clk_domain_3x_master *)(void *)pdomain;
	struct clk_prog *pprog = NULL;
	struct clk_prog_1x_master *pprog1xmaster = NULL;
	u8 i;
	u8 *pslaveidx = NULL;
	u8 slaveidx;
	u16 clkmhz;
	u32 voltuv;
	u16 bestclkmhz;
	u32 bestvoltuv;

	nvgpu_log_info(g, " ");

	if ((pclkmhz == NULL) || (pvoltuv == NULL)) {
		return -EINVAL;
	}

	if ((*pclkmhz != 0U) && (*pvoltuv != 0U)) {
		return -EINVAL;
	}

	bestclkmhz = *pclkmhz;
	bestvoltuv = *pvoltuv;

	if (pdomain->super.implements(g, &pdomain->super,
			CTRL_CLK_CLK_DOMAIN_TYPE_3X_SLAVE)) {
		slaveidx = BOARDOBJ_GET_IDX(pdomain);
		pslaveidx = &slaveidx;
		p3xmaster = (struct clk_domain_3x_master *)(void *)
				g->pmu->clk_pmu->clk_get_clk_domain(pclk,
				((struct clk_domain_3x_slave *)(void *)
					pdomain)->master_idx);
	}
	/* Iterate over the set of CLK_PROGs pointed at by this domain.*/
	for (i = p3xmaster->super.clk_prog_idx_first;
	     i <= p3xmaster->super.clk_prog_idx_last;
	     i++) {
		clkmhz = *pclkmhz;
		voltuv = *pvoltuv;
		pprog = CLK_CLK_PROG_GET(pclk, i);

		/* MASTER CLK_DOMAINs must point to MASTER CLK_PROGs.*/
		if (!pprog->super.implements(g, &pprog->super,
				CTRL_CLK_CLK_PROG_TYPE_1X_MASTER)) {
			status = -EINVAL;
			goto done;
		}

		pprog1xmaster = (struct clk_prog_1x_master *)(void *)pprog;
		status = pprog1xmaster->vflookup(g, pclk, pprog1xmaster,
				pslaveidx, &clkmhz, &voltuv, rail);
		/* if look up has found the V or F value matching to other
		 exit */
		if (status == 0) {
			if (*pclkmhz == 0U) {
				bestclkmhz = clkmhz;
			} else {
				bestvoltuv = voltuv;
				break;
			}
		}
	}
	/* clk and volt sent as zero to print vf table */
	if ((*pclkmhz == 0U) && (*pvoltuv == 0U)) {
		status = 0;
		goto done;
	}
	/* atleast one search found a matching value? */
	if ((bestvoltuv != 0U) && (bestclkmhz != 0U)) {
		*pclkmhz = bestclkmhz;
		*pvoltuv = bestvoltuv;
		status = 0;
		goto done;
	}
done:
	nvgpu_log_info(g, "done status %x", status);
	return status;
}

static int clkdomaingetfpoints
(
	struct gk20a *g,
	struct nvgpu_clk_pmupstate *pclk,
	struct nvgpu_clk_domain *pdomain,
	u32 *pfpointscount,
	u16 *pfreqpointsinmhz,
	u8 rail
)
{
	int status = 0;
	struct clk_domain_3x_master *p3xmaster  =
		(struct clk_domain_3x_master *)(void *)pdomain;
	struct clk_prog *pprog = NULL;
	struct clk_prog_1x_master *pprog1xmaster = NULL;
	u32 fpointscount = 0;
	u32 remainingcount;
	u32 totalcount;
	u16 *freqpointsdata;
	u8 i;

	nvgpu_log_info(g, " ");

	if (pfpointscount == NULL) {
		return -EINVAL;
	}

	if ((pfreqpointsinmhz == NULL) && (*pfpointscount != 0U)) {
		return -EINVAL;
	}

	if (pdomain->super.implements(g, &pdomain->super,
			CTRL_CLK_CLK_DOMAIN_TYPE_3X_SLAVE)) {
		return -EINVAL;
	}

	freqpointsdata = pfreqpointsinmhz;
	totalcount = 0;
	fpointscount = *pfpointscount;
	remainingcount = fpointscount;
	/* Iterate over the set of CLK_PROGs pointed at by this domain.*/
	for (i = p3xmaster->super.clk_prog_idx_first;
	     i <= p3xmaster->super.clk_prog_idx_last;
	     i++) {
		pprog = CLK_CLK_PROG_GET(pclk, i);
		pprog1xmaster = (struct clk_prog_1x_master *)(void *)pprog;
		status = pprog1xmaster->getfpoints(g, pclk, pprog1xmaster,
				&fpointscount, &freqpointsdata, rail);
		if (status != 0) {
			*pfpointscount = 0;
			goto done;
		}
		totalcount += fpointscount;
		if (*pfpointscount != 0U) {
			remainingcount -= fpointscount;
			fpointscount = remainingcount;
		} else {
			fpointscount = 0;
		}

	}

	*pfpointscount = totalcount;
done:
	nvgpu_log_info(g, "done status %x", status);
	return status;
}

static int clk_domain_pmudatainit_35_prog(struct gk20a *g,
					   struct boardobj *board_obj_ptr,
					   struct nv_pmu_boardobj *ppmudata)
{
	int status = 0;
	struct clk_domain_35_prog *pclk_domain_35_prog;
	struct clk_domain_3x_prog *pclk_domain_3x_prog;
	struct nv_pmu_clk_clk_domain_35_prog_boardobj_set *pset;
	struct nvgpu_clk_domains *pdomains = g->pmu->clk_pmu->clk_domainobjs;

	nvgpu_log_info(g, " ");

	status = _clk_domain_pmudatainit_3x(g, board_obj_ptr, ppmudata);
	if (status != 0) {
		return status;
	}

	pclk_domain_35_prog = (struct clk_domain_35_prog *)(void*)board_obj_ptr;
	pclk_domain_3x_prog = &pclk_domain_35_prog->super;

	pset = (struct nv_pmu_clk_clk_domain_35_prog_boardobj_set *)
		(void*) ppmudata;

	pset->super.clk_prog_idx_first =
			pclk_domain_3x_prog->clk_prog_idx_first;
	pset->super.clk_prog_idx_last = pclk_domain_3x_prog->clk_prog_idx_last;
	pset->super.b_force_noise_unaware_ordering =
		pclk_domain_3x_prog->b_force_noise_unaware_ordering;
	pset->super.factory_delta = pclk_domain_3x_prog->factory_delta;
	pset->super.freq_delta_min_mhz =
			pclk_domain_3x_prog->freq_delta_min_mhz;
	pset->super.freq_delta_max_mhz =
			pclk_domain_3x_prog->freq_delta_max_mhz;
	nvgpu_memcpy((u8 *)&pset->super.deltas, (u8 *)&pdomains->deltas,
		(sizeof(struct ctrl_clk_clk_delta)));
	pset->pre_volt_ordering_index =
			pclk_domain_35_prog->pre_volt_ordering_index;
	pset->post_volt_ordering_index =
			pclk_domain_35_prog->post_volt_ordering_index;
	pset->clk_pos = pclk_domain_35_prog->clk_pos;
	pset->clk_vf_curve_count = pclk_domain_35_prog->clk_vf_curve_count;
	pset->clkmon_info.high_threshold_vfe_idx = pclk_domain_35_prog->
			clkmon_info.high_threshold_vfe_idx;
	pset->clkmon_info.low_threshold_vfe_idx = pclk_domain_35_prog->
			clkmon_info.low_threshold_vfe_idx;
	pset->clkmon_ctrl.high_threshold_override = pclk_domain_35_prog->
			clkmon_ctrl.high_threshold_override;
	pset->clkmon_ctrl.low_threshold_override = pclk_domain_35_prog->
			clkmon_ctrl.low_threshold_override;

	return status;
}

static int clk_domain_construct_35_prog(struct gk20a *g,
					struct boardobj **ppboardobj,
					size_t size, void *pargs)
{
	struct boardobj *ptmpobj = (struct boardobj *)pargs;
	struct clk_domain_35_prog *pdomain;
	struct clk_domain_35_prog *ptmpdomain =
			(struct clk_domain_35_prog *)pargs;
	int status = 0;

	ptmpobj->type_mask |= BIT32(CTRL_CLK_CLK_DOMAIN_TYPE_35_PROG);
	status = clk_domain_construct_3x(g, ppboardobj, size, pargs);
	if (status != 0)
	{
		return -EINVAL;
	}

	pdomain = (struct clk_domain_35_prog *)(void*) *ppboardobj;

	pdomain->super.super.super.super.type_mask |=
		BIT32(CTRL_CLK_CLK_DOMAIN_TYPE_35_PROG);

	pdomain->super.super.super.super.pmudatainit =
				clk_domain_pmudatainit_35_prog;

	pdomain->super.super.super.clkdomainclkproglink =
				clkdomainclkproglink_3x_prog;

	pdomain->super.super.super.clkdomainclkvfsearch =
				clkdomainvfsearch;

	pdomain->super.super.super.clkdomainclkgetfpoints =
				clkdomaingetfpoints;

	pdomain->super.clk_prog_idx_first =
			ptmpdomain->super.clk_prog_idx_first;
	pdomain->super.clk_prog_idx_last = ptmpdomain->super.clk_prog_idx_last;
	pdomain->super.noise_unaware_ordering_index =
		ptmpdomain->super.noise_unaware_ordering_index;
	pdomain->super.noise_aware_ordering_index =
		ptmpdomain->super.noise_aware_ordering_index;
	pdomain->super.b_force_noise_unaware_ordering =
		ptmpdomain->super.b_force_noise_unaware_ordering;
	pdomain->super.factory_delta = ptmpdomain->super.factory_delta;
	pdomain->super.freq_delta_min_mhz =
			ptmpdomain->super.freq_delta_min_mhz;
	pdomain->super.freq_delta_max_mhz =
			ptmpdomain->super.freq_delta_max_mhz;
	pdomain->pre_volt_ordering_index = ptmpdomain->pre_volt_ordering_index;
	pdomain->post_volt_ordering_index =
			ptmpdomain->post_volt_ordering_index;
	pdomain->clk_pos = ptmpdomain->clk_pos;
	pdomain->clk_vf_curve_count = ptmpdomain->clk_vf_curve_count;
	pdomain->clkmon_info.high_threshold_vfe_idx = ptmpdomain->
			clkmon_info.high_threshold_vfe_idx;
	pdomain->clkmon_info.low_threshold_vfe_idx = ptmpdomain->
			clkmon_info.low_threshold_vfe_idx;

	return status;
}

static int _clk_domain_pmudatainit_35_slave(struct gk20a *g,
					    struct boardobj *board_obj_ptr,
					    struct nv_pmu_boardobj *ppmudata)
{
	int status = 0;
	struct clk_domain_35_slave *pclk_domain_35_slave;
	struct nv_pmu_clk_clk_domain_35_slave_boardobj_set *pset;

	nvgpu_log_info(g, " ");

	status = clk_domain_pmudatainit_35_prog(g, board_obj_ptr, ppmudata);
	if (status != 0) {
		return status;
	}

	pclk_domain_35_slave = (struct clk_domain_35_slave *)
			       (void *)board_obj_ptr;

	pset = (struct nv_pmu_clk_clk_domain_35_slave_boardobj_set *)
		(void*) ppmudata;

	pset->slave.master_idx = pclk_domain_35_slave->slave.master_idx;

	return status;
}

static int clk_domain_construct_35_slave(struct gk20a *g,
					 struct boardobj **ppboardobj,
					 size_t size, void *pargs)
{
	struct boardobj *ptmpobj = (struct boardobj *)pargs;
	struct clk_domain_35_slave *pdomain;
	struct clk_domain_35_slave *ptmpdomain =
			(struct clk_domain_35_slave *)pargs;
	int status = 0;

	if (BOARDOBJ_GET_TYPE(pargs) !=
			(u8) CTRL_CLK_CLK_DOMAIN_TYPE_35_SLAVE) {
		return -EINVAL;
	}

	ptmpobj->type_mask |= BIT32(CTRL_CLK_CLK_DOMAIN_TYPE_35_SLAVE);
	status = clk_domain_construct_35_prog(g, ppboardobj, size, pargs);
	if (status != 0) {
		return -EINVAL;
	}

	pdomain = (struct clk_domain_35_slave *)(void*)*ppboardobj;

	pdomain->super.super.super.super.super.pmudatainit =
			_clk_domain_pmudatainit_35_slave;

	pdomain->slave.master_idx = ptmpdomain->slave.master_idx;

	pdomain->slave.clkdomainclkgetslaveclk =
				clkdomaingetslaveclk;

	return status;
}

static int clkdomainclkproglink_3x_master(struct gk20a *g,
					  struct nvgpu_clk_pmupstate *pclk,
					  struct nvgpu_clk_domain *pdomain)
{
	int status = 0;
	struct clk_domain_3x_master *p3xmaster  =
		(struct clk_domain_3x_master *)(void *)pdomain;
	struct clk_prog *pprog = NULL;
	struct clk_prog_1x_master *pprog1xmaster = NULL;
	u16 freq_max_last_mhz = 0;
	u8 i;

	nvgpu_log_info(g, " ");

	status = clkdomainclkproglink_3x_prog(g, pclk, pdomain);
	if (status != 0) {
		goto done;
	}

	/* Iterate over the set of CLK_PROGs pointed at by this domain.*/
	for (i = p3xmaster->super.clk_prog_idx_first;
	     i <= p3xmaster->super.clk_prog_idx_last;
	     i++) {
		pprog = CLK_CLK_PROG_GET(pclk, i);

		/* MASTER CLK_DOMAINs must point to MASTER CLK_PROGs.*/
		if (!pprog->super.implements(g, &pprog->super,
				CTRL_CLK_CLK_PROG_TYPE_1X_MASTER)) {
			status = -EINVAL;
			goto done;
		}

		pprog1xmaster = (struct clk_prog_1x_master *)(void *)pprog;
		status = pprog1xmaster->vfflatten(g, pclk, pprog1xmaster,
			BOARDOBJ_GET_IDX(p3xmaster), &freq_max_last_mhz);
		if (status != 0) {
			goto done;
		}
	}
done:
	nvgpu_log_info(g, "done status %x", status);
	return status;
}

static int clk_domain_pmudatainit_35_master(struct gk20a *g,
					     struct boardobj *board_obj_ptr,
					     struct nv_pmu_boardobj *ppmudata)
{
	int status = 0;
	struct clk_domain_35_master *pclk_domain_35_master;
	struct nv_pmu_clk_clk_domain_35_master_boardobj_set *pset;

	nvgpu_log_info(g, " ");

	status = clk_domain_pmudatainit_35_prog(g, board_obj_ptr, ppmudata);
	if (status != 0) {
		return status;
	}

	pclk_domain_35_master = (struct clk_domain_35_master *)
		(void*) board_obj_ptr;

	pset = (struct nv_pmu_clk_clk_domain_35_master_boardobj_set *)
		(void*) ppmudata;

	pset->master.slave_idxs_mask =
			pclk_domain_35_master->master.slave_idxs_mask;

	status = nvgpu_boardobjgrpmask_export(
		&pclk_domain_35_master->master_slave_domains_grp_mask.super,
		pclk_domain_35_master->
			master_slave_domains_grp_mask.super.bitcount,
		&pset->master_slave_domains_grp_mask.super);

	return status;
}

static int clk_domain_construct_35_master(struct gk20a *g,
					  struct boardobj **ppboardobj,
					  size_t size, void *pargs)
{
	struct boardobj *ptmpobj = (struct boardobj *)pargs;
	struct clk_domain_35_master *pdomain;
	int status = 0;

	if (BOARDOBJ_GET_TYPE(pargs) !=
			(u8) CTRL_CLK_CLK_DOMAIN_TYPE_35_MASTER) {
		return -EINVAL;
	}

	ptmpobj->type_mask |= BIT32(CTRL_CLK_CLK_DOMAIN_TYPE_35_MASTER);
	status = clk_domain_construct_35_prog(g, ppboardobj, size, pargs);
	if (status != 0) {
		return -EINVAL;
	}

	pdomain = (struct clk_domain_35_master *)(void*) *ppboardobj;

	pdomain->super.super.super.super.super.pmudatainit =
			clk_domain_pmudatainit_35_master;
	pdomain->super.super.super.super.clkdomainclkproglink =
				clkdomainclkproglink_3x_master;

	pdomain->master.slave_idxs_mask = 0;
	pdomain->super.clk_pos = 0;

	status = boardobjgrpmask_e32_init(
			&pdomain->master_slave_domains_grp_mask, NULL);

	return status;
}

static int clkdomainclkproglink_fixed(struct gk20a *g,
				      struct nvgpu_clk_pmupstate *pclk,
				      struct nvgpu_clk_domain *pdomain)
{
	nvgpu_log_info(g, " ");
	return 0;
}

static int _clk_domain_pmudatainit_3x_fixed(struct gk20a *g,
					    struct boardobj *board_obj_ptr,
					    struct nv_pmu_boardobj *ppmudata)
{
	int status = 0;
	struct clk_domain_3x_fixed *pclk_domain_3x_fixed;
	struct nv_pmu_clk_clk_domain_3x_fixed_boardobj_set *pset;

	nvgpu_log_info(g, " ");

	status = _clk_domain_pmudatainit_3x(g, board_obj_ptr, ppmudata);
	if (status != 0) {
		return status;
	}

	pclk_domain_3x_fixed = (struct clk_domain_3x_fixed *)
					(void *)board_obj_ptr;

	pset = (struct nv_pmu_clk_clk_domain_3x_fixed_boardobj_set *)
		(void *)ppmudata;

	pset->freq_mhz = pclk_domain_3x_fixed->freq_mhz;

	return status;
}

static int clk_domain_construct_3x_fixed(struct gk20a *g,
					 struct boardobj **ppboardobj,
					 size_t size, void *pargs)
{
	struct boardobj *ptmpobj = (struct boardobj *)pargs;
	struct clk_domain_3x_fixed *pdomain;
	struct clk_domain_3x_fixed *ptmpdomain =
			(struct clk_domain_3x_fixed *)pargs;
	int status = 0;

	if (BOARDOBJ_GET_TYPE(pargs) != CTRL_CLK_CLK_DOMAIN_TYPE_3X_FIXED) {
		return -EINVAL;
	}

	ptmpobj->type_mask |= BIT32(CTRL_CLK_CLK_DOMAIN_TYPE_3X_FIXED);
	status = clk_domain_construct_3x(g, ppboardobj, size, pargs);
	if (status != 0) {
		return -EINVAL;
	}

	pdomain = (struct clk_domain_3x_fixed *)(void *)*ppboardobj;

	pdomain->super.super.super.pmudatainit =
			_clk_domain_pmudatainit_3x_fixed;

	pdomain->super.super.clkdomainclkproglink =
			clkdomainclkproglink_fixed;

	pdomain->freq_mhz = ptmpdomain->freq_mhz;

	return status;
}

static struct nvgpu_clk_domain *construct_clk_domain(struct gk20a *g,
		void *pargs)
{
	struct boardobj *board_obj_ptr = NULL;
	int status;

	nvgpu_log_info(g, " %d", (BOARDOBJ_GET_TYPE(pargs)));
	switch (BOARDOBJ_GET_TYPE(pargs)) {
	case CTRL_CLK_CLK_DOMAIN_TYPE_3X_FIXED:
		status = clk_domain_construct_3x_fixed(g, &board_obj_ptr,
			sizeof(struct clk_domain_3x_fixed), pargs);
		break;

	case CTRL_CLK_CLK_DOMAIN_TYPE_35_MASTER:
		status = clk_domain_construct_35_master(g, &board_obj_ptr,
			sizeof(struct clk_domain_35_master), pargs);
		break;

	case CTRL_CLK_CLK_DOMAIN_TYPE_35_SLAVE:
		status = clk_domain_construct_35_slave(g, &board_obj_ptr,
			sizeof(struct clk_domain_35_slave), pargs);
		break;

	default:
		nvgpu_err(g, "Unsupported Clk domain type");
		status = -EINVAL;
		break;
	}

	if (status != 0) {
		return NULL;
	}

	nvgpu_log_info(g, " Done");

	return (struct nvgpu_clk_domain *)(void *)board_obj_ptr;
}

static int clk_domain_pmudatainit_super(struct gk20a *g,
					struct boardobj *board_obj_ptr,
					struct nv_pmu_boardobj *ppmudata)
{
	int status = 0;
	struct nvgpu_clk_domain *pclk_domain;
	struct nv_pmu_clk_clk_domain_boardobj_set *pset;

	nvgpu_log_info(g, " ");

	status = nvgpu_boardobj_pmu_data_init_super(g, board_obj_ptr, ppmudata);
	if (status != 0) {
		return status;
	}

	pclk_domain = (struct nvgpu_clk_domain *)(void *)board_obj_ptr;

	pset = (struct nv_pmu_clk_clk_domain_boardobj_set *)(void *)ppmudata;

	pset->domain = pclk_domain->domain;
	pset->api_domain = pclk_domain->api_domain;
	pset->perf_domain_grp_idx = pclk_domain->perf_domain_grp_idx;

	return status;
}

static int clk_domain_clk_prog_link(struct gk20a *g,
		struct nvgpu_clk_pmupstate *pclk)
{
	int status = 0;
	struct nvgpu_clk_domain *pdomain;
	u8 i;

	/* Iterate over all CLK_DOMAINs and flatten their VF curves.*/
	BOARDOBJGRP_FOR_EACH(&(pclk->clk_domainobjs->super.super),
			struct nvgpu_clk_domain *, pdomain, i) {
		status = pdomain->clkdomainclkproglink(g, pclk, pdomain);
		if (status != 0) {
			nvgpu_err(g,
				  "error flattening VF for CLK DOMAIN - 0x%x",
				  pdomain->domain);
			goto done;
		}
	}

done:
	return status;
}

int nvgpu_clk_pmu_clk_domains_load(struct gk20a *g)
{
	int status;
	struct nvgpu_pmu *pmu = g->pmu;
	struct nv_pmu_rpc_struct_clk_load clk_load_rpc;

	(void) memset(&clk_load_rpc, 0,
			sizeof(struct nv_pmu_rpc_struct_clk_load));

	clk_load_rpc.clk_load.feature = NV_NV_PMU_CLK_LOAD_FEATURE_CLK_DOMAIN;

	/* Continue with PMU setup, assume FB map is done  */
	PMU_RPC_EXECUTE_CPB(status, pmu, CLK, LOAD, &clk_load_rpc, 0);
	if (status != 0) {
		nvgpu_err(g,
			"Failed to execute Clock domain Load RPC status=0x%x",
			status);
	}

	return status;
}

#ifdef CONFIG_NVGPU_CLK_ARB
static int clk_get_fll_clks_per_clk_domain(struct gk20a *g,
		struct nvgpu_set_fll_clk *setfllclk)
{
	int status = -EINVAL;
	struct nvgpu_clk_domain *pdomain;
	u8 i;
	struct nvgpu_clk_pmupstate *pclk = g->pmu->clk_pmu;
	unsigned long bit;
	u16 clkmhz = 0;
	struct clk_domain_35_master *p35master;
	struct clk_domain_35_slave *p35slave;
	unsigned long slaveidxmask;

	if (setfllclk->gpc2clkmhz == 0U) {
		return -EINVAL;
	}

	BOARDOBJGRP_FOR_EACH(&(pclk->clk_domainobjs->super.super),
			struct nvgpu_clk_domain *, pdomain, i) {

		if (pdomain->api_domain == CTRL_CLK_DOMAIN_GPCCLK) {
			if (!pdomain->super.implements(g, &pdomain->super,
				CTRL_CLK_CLK_DOMAIN_TYPE_35_MASTER)) {
				status = -EINVAL;
				goto done;
			}
			p35master = (struct clk_domain_35_master *)
				(void *)pdomain;
			slaveidxmask = p35master->master.slave_idxs_mask;
			for_each_set_bit(bit, &slaveidxmask, 32U) {
				i = (u8)bit;
				p35slave = (struct clk_domain_35_slave *)
					(void *)
					g->pmu->clk_pmu->clk_get_clk_domain(pclk, i);

				clkmhz = 0;
				status = p35slave->
					slave.clkdomainclkgetslaveclk(g,
					pclk, (struct nvgpu_clk_domain *)
					(void *)p35slave,
					&clkmhz, setfllclk->gpc2clkmhz);
				if (status != 0) {
					status = -EINVAL;
					goto done;
				}
				if (p35slave->super.super.super.super.
					api_domain == CTRL_CLK_DOMAIN_XBARCLK) {
					setfllclk->xbar2clkmhz = clkmhz;
				}
				if (p35slave->super.super.super.super.
					api_domain == CTRL_CLK_DOMAIN_SYSCLK) {
					setfllclk->sys2clkmhz = clkmhz;
				}
				if (p35slave->super.super.super.super.
					api_domain == CTRL_CLK_DOMAIN_NVDCLK) {
					setfllclk->nvdclkmhz = clkmhz;
				}
				if (p35slave->super.super.super.super.
					api_domain == CTRL_CLK_DOMAIN_HOSTCLK) {
					setfllclk->hostclkmhz = clkmhz;
				}
			}
		}
	}
done:
	return status;
}

static void clk_set_p0_clk_per_domain(struct gk20a *g, u8 *gpcclk_domain,
		u32 *gpcclk_clkmhz,
		struct nvgpu_clk_slave_freq *vf_point,
		struct ctrl_perf_change_seq_change_input *change_input)
{
	struct nvgpu_clk_domain *pclk_domain;
	struct clk_set_info *p0_info;
	u32 max_clkmhz;
	u16 max_ratio;
	u8 i = 0;

	BOARDOBJGRP_FOR_EACH(&(g->pmu->clk_pmu->clk_domainobjs->super.super),
			struct nvgpu_clk_domain *, pclk_domain, i) {

		switch (pclk_domain->api_domain) {
		case CTRL_CLK_DOMAIN_GPCCLK:
			*gpcclk_domain = i;
			*gpcclk_clkmhz = vf_point->gpc_mhz;

			p0_info = nvgpu_pmu_perf_pstate_get_clk_set_info(g,
					CTRL_PERF_PSTATE_P0, CLKWHICH_GPCCLK);
			if (p0_info == NULL) {
				nvgpu_err(g, "failed to get GPCCLK P0 info");
				break;
			}
			if (vf_point->gpc_mhz < p0_info->min_mhz) {
				vf_point->gpc_mhz = p0_info->min_mhz;
			}
			if (vf_point->gpc_mhz > p0_info->max_mhz) {
				vf_point->gpc_mhz = p0_info->max_mhz;
			}
			change_input->clk[i].clk_freq_khz =
					(u32)vf_point->gpc_mhz * 1000U;
			change_input->clk_domains_mask.super.data[0] |=
					(u32) BIT(i);
			break;
		case CTRL_CLK_DOMAIN_XBARCLK:
			p0_info = nvgpu_pmu_perf_pstate_get_clk_set_info(g,
					CTRL_PERF_PSTATE_P0, CLKWHICH_XBARCLK);
			if (p0_info == NULL) {
				nvgpu_err(g, "failed to get XBARCLK P0 info");
				break;
			}
			max_ratio = pclk_domain->ratio_domain;

			if (vf_point->xbar_mhz < p0_info->min_mhz) {
				vf_point->xbar_mhz = p0_info->min_mhz;
			}
			if (vf_point->xbar_mhz > p0_info->max_mhz) {
				vf_point->xbar_mhz = p0_info->max_mhz;
			}
			change_input->clk[i].clk_freq_khz =
					(u32)vf_point->xbar_mhz * 1000U;
			change_input->clk_domains_mask.super.data[0] |=
					(u32) BIT(i);
			if (vf_point->gpc_mhz < vf_point->xbar_mhz) {
				max_clkmhz = (((u32)vf_point->xbar_mhz * 100U) /
						(u32)max_ratio);
				if (*gpcclk_clkmhz < max_clkmhz) {
					*gpcclk_clkmhz = max_clkmhz;
				}
			}
			break;
		case CTRL_CLK_DOMAIN_SYSCLK:
			p0_info = nvgpu_pmu_perf_pstate_get_clk_set_info(g,
					CTRL_PERF_PSTATE_P0, CLKWHICH_SYSCLK);
			if (p0_info == NULL) {
				nvgpu_err(g, "failed to get SYSCLK P0 info");
				break;
			}
			max_ratio = pclk_domain->ratio_domain;
			if (vf_point->sys_mhz < p0_info->min_mhz) {
				vf_point->sys_mhz = p0_info->min_mhz;
			}
			if (vf_point->sys_mhz > p0_info->max_mhz) {
				vf_point->sys_mhz = p0_info->max_mhz;
			}
			change_input->clk[i].clk_freq_khz =
					(u32)vf_point->sys_mhz * 1000U;
			change_input->clk_domains_mask.super.data[0] |=
					(u32) BIT(i);
			if (vf_point->gpc_mhz < vf_point->sys_mhz) {
				max_clkmhz = (((u32)vf_point->sys_mhz * 100U) /
						(u32)max_ratio);
				if (*gpcclk_clkmhz < max_clkmhz) {
					*gpcclk_clkmhz = max_clkmhz;
				}
			}
			break;
		case CTRL_CLK_DOMAIN_NVDCLK:
			p0_info = nvgpu_pmu_perf_pstate_get_clk_set_info(g,
					CTRL_PERF_PSTATE_P0, CLKWHICH_NVDCLK);
			if (p0_info == NULL) {
				nvgpu_err(g, "failed to get NVDCLK P0 info");
				break;
			}
			max_ratio = pclk_domain->ratio_domain;
			if (vf_point->nvd_mhz < p0_info->min_mhz) {
				vf_point->nvd_mhz = p0_info->min_mhz;
			}
			if (vf_point->nvd_mhz > p0_info->max_mhz) {
				vf_point->nvd_mhz = p0_info->max_mhz;
			}
			change_input->clk[i].clk_freq_khz =
					(u32)vf_point->nvd_mhz * 1000U;
			change_input->clk_domains_mask.super.data[0] |=
					(u32) BIT(i);
			if (vf_point->gpc_mhz < vf_point->nvd_mhz) {
				max_clkmhz = (((u32)vf_point->nvd_mhz * 100U) /
						(u32)max_ratio);
				if (*gpcclk_clkmhz < max_clkmhz) {
					*gpcclk_clkmhz = max_clkmhz;
				}
			}
			break;
		case CTRL_CLK_DOMAIN_HOSTCLK:
			p0_info = nvgpu_pmu_perf_pstate_get_clk_set_info(g,
					CTRL_PERF_PSTATE_P0, CLKWHICH_HOSTCLK);
			if (p0_info == NULL) {
				nvgpu_err(g, "failed to get HOSTCLK P0 info");
				break;
			}
			max_ratio = pclk_domain->ratio_domain;
			if (vf_point->host_mhz < p0_info->min_mhz) {
				vf_point->host_mhz = p0_info->min_mhz;
			}
			if (vf_point->host_mhz > p0_info->max_mhz) {
				vf_point->host_mhz = p0_info->max_mhz;
			}
			change_input->clk[i].clk_freq_khz =
					(u32)vf_point->host_mhz * 1000U;
			change_input->clk_domains_mask.super.data[0] |=
					(u32) BIT(i);
			if (vf_point->gpc_mhz < vf_point->host_mhz) {
				max_clkmhz = (((u32)vf_point->host_mhz * 100U) /
						(u32)max_ratio);
				if (*gpcclk_clkmhz < max_clkmhz) {
					*gpcclk_clkmhz = max_clkmhz;
				}
			}
			break;
		default:
			nvgpu_pmu_dbg(g, "Fixed clock domain");
			break;
		}
	}
}
#endif

int nvgpu_clk_domain_init_pmupstate(struct gk20a *g)
{
	/* If already allocated, do not re-allocate */
	if (g->pmu->clk_pmu->clk_domainobjs != NULL) {
		return 0;
	}

	g->pmu->clk_pmu->clk_domainobjs = nvgpu_kzalloc(g,
			sizeof(*g->pmu->clk_pmu->clk_domainobjs));
	if (g->pmu->clk_pmu->clk_domainobjs == NULL) {
		return -ENOMEM;
	}

#ifdef CONFIG_NVGPU_CLK_ARB
	g->pmu->clk_pmu->get_fll =
			clk_get_fll_clks_per_clk_domain;
	g->pmu->clk_pmu->set_p0_clks =
			clk_set_p0_clk_per_domain;
#endif
	g->pmu->clk_pmu->clk_get_clk_domain =
			clk_get_clk_domain_from_index;
	g->pmu->clk_pmu->clk_domain_clk_prog_link =
			clk_domain_clk_prog_link;

	return 0;
}

void nvgpu_clk_domain_free_pmupstate(struct gk20a *g)
{
	nvgpu_kfree(g, g->pmu->clk_pmu->clk_domainobjs);
	g->pmu->clk_pmu->clk_domainobjs = NULL;
}

int nvgpu_clk_domain_get_from_index(struct gk20a *g, u32 *domain, u32 index)
{
	struct nvgpu_clk_domain *clk_domain;

	clk_domain = (struct nvgpu_clk_domain *) BOARDOBJGRP_OBJ_GET_BY_IDX(
		&g->pmu->clk_pmu->clk_domainobjs->super.super, index);
	if (clk_domain == NULL) {
		return -EINVAL;
	}

	*domain = clk_domain->domain;
	return 0;
}
