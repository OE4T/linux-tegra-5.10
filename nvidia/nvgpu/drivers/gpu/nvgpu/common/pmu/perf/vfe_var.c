/*
 * Copyright (c) 2016-2022, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/pmu/boardobjgrp_classes.h>
#include <nvgpu/pmu/clk/clk.h>
#include <nvgpu/string.h>
#include <nvgpu/pmu/perf.h>

#include "ucode_perf_vfe_inf.h"
#include "vfe_var.h"
#include "perf.h"

static int vfe_vars_pmudatainit(struct gk20a *g,
				 struct boardobjgrp *pboardobjgrp,
				 struct nv_pmu_boardobjgrp_super *pboardobjgrppmu)
{
	struct nv_pmu_perf_vfe_var_boardobjgrp_set_header *pset =
		(struct nv_pmu_perf_vfe_var_boardobjgrp_set_header *)(void *)
		pboardobjgrppmu;
	struct vfe_vars *pvars = (struct vfe_vars *)(void *)pboardobjgrp;
	int status = 0;

	status = boardobjgrp_pmudatainit_e32(g, pboardobjgrp, pboardobjgrppmu);
	if (status != 0) {
		nvgpu_err(g,
			"error updating pmu boardobjgrp for vfe var 0x%x",
			 status);
		goto done;
	}
	pset->polling_periodms = pvars->polling_periodms;

done:
	return status;
}

static int vfe_vars_pmudata_instget(struct gk20a *g,
				     struct nv_pmu_boardobjgrp *pmuboardobjgrp,
				     struct nv_pmu_boardobj **pmu_obj,
				     u8 idx)
{
	struct nv_pmu_perf_vfe_var_boardobj_grp_set  *pgrp_set =
		(struct nv_pmu_perf_vfe_var_boardobj_grp_set *)(void *)
		pmuboardobjgrp;

	/*check whether pmuboardobjgrp has a valid boardobj in index*/
	if (idx >= CTRL_BOARDOBJGRP_E32_MAX_OBJECTS) {
		return -EINVAL;
	}

	*pmu_obj = (struct nv_pmu_boardobj *)
		&pgrp_set->objects[idx].data.obj;

	nvgpu_log_info(g, " Done");
	return 0;
}

static int vfe_vars_pmustatus_instget(struct gk20a *g, void *pboardobjgrppmu,
	struct nv_pmu_boardobj_query **obj_pmu_status, u8 idx)
{
	struct nv_pmu_perf_vfe_var_boardobj_grp_get_status *pgrp_get_status =
		(struct nv_pmu_perf_vfe_var_boardobj_grp_get_status *)
		pboardobjgrppmu;

	(void)g;

	if (((u32)BIT(idx) &
		pgrp_get_status->hdr.data.super.obj_mask.super.data[0]) == 0U) {
		return -EINVAL;
	}

	*obj_pmu_status = (struct nv_pmu_boardobj_query *)
			&pgrp_get_status->objects[idx].data.obj;
	return 0;
}

static int vfe_var_get_s_param_value(struct gk20a *g,
		struct vfe_var_single_sensed_fuse *fuse_value,
		struct nv_pmu_boardobj *pmu_obj)
{
	struct nv_pmu_perf_vfe_var_single_sensed_fuse_get_status *pstatus;
	pstatus = (struct nv_pmu_perf_vfe_var_single_sensed_fuse_get_status *)
		(void *)pmu_obj;

	if (pstatus->super.obj.type !=
			fuse_value->super.super.super.super.type) {
		nvgpu_err(g, "pmu data and boardobj type not matching");
		return -EINVAL;
	}

	if (pstatus->fuse_value_integer.b_signed) {
		fuse_value->b_fuse_value_signed =
			pstatus->fuse_value_integer.b_signed;
		fuse_value->fuse_value_integer =
			(u32)pstatus->fuse_value_integer.data.signed_value;
		fuse_value->fuse_value_hw_integer =
			(u32)pstatus->fuse_value_hw_integer.data.signed_value;
	} else {
		fuse_value->b_fuse_value_signed =
			pstatus->fuse_value_integer.b_signed;
		fuse_value->fuse_value_integer =
			pstatus->fuse_value_integer.data.unsigned_value;
		fuse_value->fuse_value_hw_integer =
			pstatus->fuse_value_hw_integer.data.unsigned_value;
	}
	return 0;
}

static int vfe_var_dependency_mask_build(struct gk20a *g,
		struct vfe_vars *pvfe_vars)
{
	int status = 0;
	u8 index_1 = 0, index_2 = 0;
	struct vfe_var *tmp_vfe_var_1 = NULL, *tmp_vfe_var_2 = NULL;
	struct pmu_board_obj *obj_tmp_1 = NULL, *obj_tmp_2 = NULL;
	struct boardobjgrp *pboardobjgrp = &(pvfe_vars->super.super);

	/* Initialize mask_depending_vars */
	BOARDOBJGRP_FOR_EACH(pboardobjgrp, struct pmu_board_obj*,
			obj_tmp_1, index_1) {
		tmp_vfe_var_1 = (struct vfe_var *)(void *)obj_tmp_1;
		status = tmp_vfe_var_1->mask_depending_build(g, pboardobjgrp,
				tmp_vfe_var_1);
		if (status != 0) {
			nvgpu_err(g, "failure in calling vfevar[%d].depmskbld",
					index_1);
			return status;
		}
	}
	/* Initialize mask_dependent_vars */
	BOARDOBJGRP_FOR_EACH(pboardobjgrp, struct pmu_board_obj*,
			obj_tmp_1, index_1) {
		tmp_vfe_var_1 = (struct vfe_var *)(void *)obj_tmp_1;
		BOARDOBJGRP_ITERATOR(pboardobjgrp, struct pmu_board_obj*,
				obj_tmp_2, index_2,
				&tmp_vfe_var_1->mask_depending_vars.super) {
			tmp_vfe_var_2 = (struct vfe_var *)(void *)obj_tmp_2;
			status = nvgpu_boardobjgrpmask_bit_set(
				&tmp_vfe_var_2->mask_dependent_vars.super,
				index_1);
			if (status != 0) {
				nvgpu_err(g, "failing boardobjgrpmask_bit_set");
				return status;
			}
		}
	}
	return status;
}

static int dev_init_get_vfield_info(struct gk20a *g,
	struct vfe_var_single_sensed_fuse *pvfevar)
{
	u8 *vfieldtableptr = NULL;
	u32 vfieldheadersize = VFIELD_HEADER_SIZE;
	u8 *vfieldregtableptr = NULL;
	u32 vfieldregheadersize = VFIELD_REG_HEADER_SIZE;
	u32 i;
	u32 oldindex = 0xFFFFFFFFU;
	u32 currindex;
	struct vfield_reg_header vregheader;
	struct vfield_reg_entry vregentry;
	struct vfield_header vheader;
	struct vfield_entry ventry;
	struct ctrl_bios_vfield_register_segment *psegment = NULL;
	u8 *psegmentcount = NULL;
	int status = 0;

	vfieldregtableptr = (u8 *)nvgpu_bios_get_perf_table_ptrs(g,
			nvgpu_bios_get_bit_token(g, NVGPU_BIOS_VIRT_TOKEN),
			VP_FIELD_REGISTER);
	if (vfieldregtableptr == NULL) {
		status = -EINVAL;
		goto done;
	}

	vfieldtableptr = (u8 *)nvgpu_bios_get_perf_table_ptrs(g,
			nvgpu_bios_get_bit_token(g, NVGPU_BIOS_VIRT_TOKEN),
			VP_FIELD_TABLE);
	if (vfieldtableptr == NULL) {
		status = -EINVAL;
		goto done;
	}

	nvgpu_memcpy((u8 *)&vregheader, vfieldregtableptr,
		VFIELD_REG_HEADER_SIZE);

	if (vregheader.version != VBIOS_VFIELD_REG_TABLE_VERSION_1_0) {
		nvgpu_err(g, "invalid vreg header version");
		goto done;
	}

	nvgpu_memcpy((u8 *)&vheader, vfieldtableptr, VFIELD_HEADER_SIZE);

	if (vregheader.version != VBIOS_VFIELD_TABLE_VERSION_1_0) {
		nvgpu_err(g, "invalid vfield header version");
		goto done;
	}

	pvfevar->vfield_info.fuse.segment_count = 0;
	pvfevar->vfield_ver_info.fuse.segment_count = 0;
	for (i = 0; i < (u32)vheader.count; i++) {
		nvgpu_memcpy((u8 *)&ventry, vfieldtableptr + vfieldheadersize +
			(i * vheader.entry_size),
			vheader.entry_size);

		currindex = U32(VFIELD_BIT_REG(ventry));
		if (currindex != oldindex) {

			nvgpu_memcpy((u8 *)&vregentry, vfieldregtableptr +
				vfieldregheadersize +
				(currindex * vregheader.entry_size),
				vregheader.entry_size);
			oldindex = currindex;
		}

		if (pvfevar->vfield_info.v_field_id == ventry.strap_id) {
			psegmentcount =
				&(pvfevar->vfield_info.fuse.segment_count);
			psegment =
				&(pvfevar->vfield_info.fuse.segments[*psegmentcount]);
			if (*psegmentcount > NV_PMU_VFE_VAR_SINGLE_SENSED_FUSE_SEGMENTS_MAX) {
				status = -EINVAL;
				goto done;
			}
		} else if (pvfevar->vfield_ver_info.v_field_id_ver == ventry.strap_id) {
			psegmentcount =
				&(pvfevar->vfield_ver_info.fuse.segment_count);
			psegment =
				&(pvfevar->vfield_ver_info.fuse.segments[*psegmentcount]);
			if (*psegmentcount > NV_PMU_VFE_VAR_SINGLE_SENSED_FUSE_SEGMENTS_MAX) {
				status = -EINVAL;
				goto done;
			}
		} else {
			continue;
		}

		switch (VFIELD_CODE((&vregentry))) {
		case NV_VFIELD_DESC_CODE_REG:
			psegment->type =
				NV_PMU_BIOS_VFIELD_DESC_CODE_REG;
			psegment->data.reg.addr = vregentry.reg;
			psegment->data.reg.super.high_bit = (u8)(VFIELD_BIT_STOP(ventry));
			psegment->data.reg.super.low_bit = (u8)(VFIELD_BIT_START(ventry));
			break;

		case NV_VFIELD_DESC_CODE_INDEX_REG:
			psegment->type =
				NV_PMU_BIOS_VFIELD_DESC_CODE_INDEX_REG;
			psegment->data.index_reg.addr = vregentry.reg;
			psegment->data.index_reg.index = vregentry.index;
			psegment->data.index_reg.reg_index = vregentry.reg_index;
			psegment->data.index_reg.super.high_bit = (u8)(VFIELD_BIT_STOP(ventry));
			psegment->data.index_reg.super.low_bit = (u8)(VFIELD_BIT_START(ventry));
			break;

		default:
			psegment->type =
				NV_PMU_BIOS_VFIELD_DESC_CODE_INVALID;
			status = -EINVAL;
			break;
		}
		if (status != 0) {
			goto done;
		}

		if (VFIELD_SIZE((&vregentry)) != NV_VFIELD_DESC_SIZE_DWORD) {
			psegment->type =
				NV_PMU_BIOS_VFIELD_DESC_CODE_INVALID;
			return -EINVAL;
		}
		(*psegmentcount)++;
	}

done:
	return status;
}

static int vfe_var_pmudatainit_super(struct gk20a *g,
				      struct pmu_board_obj *obj,
				      struct nv_pmu_boardobj *pmu_obj)
{
	int status = 0;
	struct vfe_var *pvfe_var;
	struct nv_pmu_vfe_var *pset;

	nvgpu_log_info(g, " ");

	status = pmu_board_obj_pmu_data_init_super(g, obj, pmu_obj);
	if (status != 0) {
		return status;
	}

	pvfe_var = (struct vfe_var *)(void *)obj;
	pset = (struct nv_pmu_vfe_var *)(void *)pmu_obj;

	pset->out_range_min = pvfe_var->out_range_min;
	pset->out_range_max = pvfe_var->out_range_max;
	status = nvgpu_boardobjgrpmask_export(&pvfe_var->
			mask_dependent_vars.super,
			pvfe_var->mask_dependent_vars.super.bitcount,
			&pset->mask_dependent_vars.super);
	status = nvgpu_boardobjgrpmask_export(&pvfe_var->
			mask_dependent_equs.super,
			pvfe_var->mask_dependent_equs.super.bitcount,
			&pset->mask_dependent_equs.super);
	return status;
}

static int vfe_var_build_depending_mask_null(struct gk20a *g,
		struct boardobjgrp *pboardobjgrp,
		struct vfe_var *pvfe_var)
{
	(void)g;
	(void)pboardobjgrp;
	(void)pvfe_var;
	/* Individual vfe_var members should over_ride this with their */
	/* respective function types */
	return -EINVAL;
}

static int vfe_var_construct_super(struct gk20a *g,
				   struct pmu_board_obj **obj,
				   size_t size, void *pargs)
{
	struct vfe_var *pvfevar;
	struct vfe_var *ptmpvar = (struct vfe_var *)pargs;
	int status;

	pvfevar = nvgpu_kzalloc(g, size);
	if (pvfevar == NULL) {
		return -ENOMEM;
	}

	status = pmu_board_obj_construct_super(g,
			(struct pmu_board_obj *)(void *)pvfevar, pargs);
	if (status != 0) {
		return -EINVAL;
	}

	*obj = (struct pmu_board_obj *)(void *)pvfevar;

	pvfevar->super.pmudatainit =
			vfe_var_pmudatainit_super;

	pvfevar->out_range_min = ptmpvar->out_range_min;
	pvfevar->out_range_max = ptmpvar->out_range_max;
	pvfevar->b_is_dynamic_valid = false;
	pvfevar->mask_depending_build = vfe_var_build_depending_mask_null;

	status = boardobjgrpmask_e32_init(&pvfevar->mask_depending_vars, NULL);
	if (status != 0) {
		return -EINVAL;
	}
	status = boardobjgrpmask_e32_init(&pvfevar->mask_dependent_vars, NULL);
	if (status != 0) {
		return -EINVAL;
	}
	status = boardobjgrpmask_e255_init(&pvfevar->mask_dependent_equs, NULL);
	if (status != 0) {
		return -EINVAL;
	}
	nvgpu_log_info(g, " ");

	return status;
}

static int vfe_var_pmudatainit_derived(struct gk20a *g,
					struct pmu_board_obj *obj,
					struct nv_pmu_boardobj *pmu_obj)
{
	return vfe_var_pmudatainit_super(g, obj, pmu_obj);
}

static int vfe_var_construct_derived(struct gk20a *g,
				     struct pmu_board_obj **obj,
				     size_t size, void *pargs)
{
	struct pmu_board_obj *obj_tmp = (struct pmu_board_obj *)pargs;
	int status = 0;
	struct vfe_var_derived *pvfevar;

	obj_tmp->type_mask |= (u32)BIT(CTRL_PERF_VFE_VAR_TYPE_DERIVED);
	status = vfe_var_construct_super(g, obj, size, pargs);
	if (status != 0) {
		return -EINVAL;
	}

	pvfevar = (struct vfe_var_derived *)(void *)*obj;

	pvfevar->super.super.pmudatainit =
			vfe_var_pmudatainit_derived;

	return status;
}

static int vfe_var_pmudatainit_derived_product(struct gk20a *g,
						struct pmu_board_obj *obj,
						struct nv_pmu_boardobj *pmu_obj)
{
	int status = 0;
	struct vfe_var_derived_product *pvfe_var_derived_product;
	struct nv_pmu_vfe_var_derived_product *pset;

	nvgpu_log_info(g, " ");

	status = vfe_var_pmudatainit_derived(g, obj, pmu_obj);
	if (status != 0) {
		return status;
	}

	pvfe_var_derived_product =
		(struct vfe_var_derived_product *)(void *)obj;
	pset = (struct nv_pmu_vfe_var_derived_product *)(void *)pmu_obj;

	pset->var_idx0 = pvfe_var_derived_product->var_idx0;
	pset->var_idx1 = pvfe_var_derived_product->var_idx1;

	return status;
}

static int vfe_var_build_depending_mask_derived_product(struct gk20a *g,
		struct boardobjgrp *pboardobjgrp,
		struct vfe_var *pvfe_var)
{
	struct vfe_var_derived_product *pvar_dp =
			(struct vfe_var_derived_product *)(void *)pvfe_var;
	int status;
	struct vfe_var *var0, *var1;

	var0 = (struct vfe_var *)(void *)BOARDOBJGRP_OBJ_GET_BY_IDX(
			pboardobjgrp, pvar_dp->var_idx0);
	status = var0->mask_depending_build(g, pboardobjgrp, var0);
	if (status != 0) {
		nvgpu_err(g, " Failed calling vfevar[%d].mask_depending_build",
				pvar_dp->var_idx0);
		return status;
	}

	var1 = (struct vfe_var *)BOARDOBJGRP_OBJ_GET_BY_IDX(
			pboardobjgrp,
			pvar_dp->var_idx1);
	status = var1->mask_depending_build(g, pboardobjgrp, var1);
	if (status != 0) {
		nvgpu_err(g, " Failed calling vfevar[%d].mask_depending_build",
				pvar_dp->var_idx1);
		return status;
	}

	status = nvgpu_boardobjmask_or(&(pvfe_var->mask_depending_vars.super),
			&(var0->mask_depending_vars.super),
			&(var1->mask_depending_vars.super));

	return status;
}

static int vfe_var_construct_derived_product(struct gk20a *g,
					     struct pmu_board_obj **obj,
					     size_t size, void *pargs)
{
	struct pmu_board_obj *obj_tmp = (struct pmu_board_obj *)pargs;
	struct vfe_var_derived_product *pvfevar;
	struct vfe_var_derived_product *ptmpvar =
			(struct vfe_var_derived_product *)pargs;
	int status = 0;

	if (pmu_board_obj_get_type(pargs) != CTRL_PERF_VFE_VAR_TYPE_DERIVED_PRODUCT) {
		return -EINVAL;
	}

	obj_tmp->type_mask |= (u32)BIT(CTRL_PERF_VFE_VAR_TYPE_DERIVED_PRODUCT);
	status = vfe_var_construct_derived(g, obj, size, pargs);
	if (status != 0) {
		return -EINVAL;
	}

	pvfevar = (struct vfe_var_derived_product *)(void *)*obj;
	pvfevar->super.super.mask_depending_build =
			vfe_var_build_depending_mask_derived_product;
	pvfevar->super.super.super.pmudatainit =
			vfe_var_pmudatainit_derived_product;

	pvfevar->var_idx0 = ptmpvar->var_idx0;
	pvfevar->var_idx1 = ptmpvar->var_idx1;


	return status;
}

static int vfe_var_pmudatainit_derived_sum(struct gk20a *g,
					    struct pmu_board_obj *obj,
					    struct nv_pmu_boardobj *pmu_obj)
{
	int status = 0;
	struct vfe_var_derived_sum *pvfe_var_derived_sum;
	struct nv_pmu_vfe_var_derived_sum *pset;

	status = vfe_var_pmudatainit_derived(g, obj, pmu_obj);
	if (status != 0) {
		return status;
	}

	pvfe_var_derived_sum = (struct vfe_var_derived_sum *)
							(void *)obj;
	pset = (struct nv_pmu_vfe_var_derived_sum *)(void *)pmu_obj;

	pset->var_idx0 = pvfe_var_derived_sum->var_idx0;
	pset->var_idx1 = pvfe_var_derived_sum->var_idx1;

	return status;
}

static int vfe_var_build_depending_mask_derived_sum(struct gk20a *g,
		struct boardobjgrp *pboardobjgrp,
		struct vfe_var *pvfe_var)
{
	struct vfe_var_derived_sum *pvar_dsum =
			(struct vfe_var_derived_sum *)(void *)pvfe_var;
	int status;
	struct vfe_var *var0, *var1;

	var0 = (struct vfe_var *)BOARDOBJGRP_OBJ_GET_BY_IDX(
			pboardobjgrp, pvar_dsum->var_idx0);
	status = var0->mask_depending_build(g, pboardobjgrp, var0);
	if (status != 0) {
		nvgpu_err(g, " Failed calling vfevar[%d].mask_depending_build",
				pvar_dsum->var_idx0);
		return status;
	}

	var1 = (struct vfe_var *)(void *)BOARDOBJGRP_OBJ_GET_BY_IDX(
			pboardobjgrp,
			pvar_dsum->var_idx1);
	status = var1->mask_depending_build(g, pboardobjgrp, var1);
	if (status != 0) {
		nvgpu_err(g, " Failed calling vfevar[%d].mask_depending_build",
				pvar_dsum->var_idx1);
		return status;
	}

	status = nvgpu_boardobjmask_or(&(pvfe_var->mask_depending_vars.super),
			&(var0->mask_depending_vars.super),
			&(var1)->mask_depending_vars.super);

	return status;
}

static int vfe_var_construct_derived_sum(struct gk20a *g,
					 struct pmu_board_obj **obj,
					 size_t size, void *pargs)
{
	struct pmu_board_obj *obj_tmp = (struct pmu_board_obj *)pargs;
	struct vfe_var_derived_sum *pvfevar;
	struct vfe_var_derived_sum *ptmpvar =
			(struct vfe_var_derived_sum *)pargs;
	int status = 0;

	if (pmu_board_obj_get_type(pargs) != CTRL_PERF_VFE_VAR_TYPE_DERIVED_SUM) {
		return -EINVAL;
	}

	obj_tmp->type_mask |= (u32)BIT(CTRL_PERF_VFE_VAR_TYPE_DERIVED_SUM);
	status = vfe_var_construct_derived(g, obj, size, pargs);
	if (status != 0) {
		return -EINVAL;
	}

	pvfevar = (struct vfe_var_derived_sum *)(void *)*obj;
	pvfevar->super.super.mask_depending_build =
				vfe_var_build_depending_mask_derived_sum;
	pvfevar->super.super.super.pmudatainit =
			vfe_var_pmudatainit_derived_sum;

	pvfevar->var_idx0 = ptmpvar->var_idx0;
	pvfevar->var_idx1 = ptmpvar->var_idx1;

	return status;
}

static int vfe_var_pmudatainit_single(struct gk20a *g,
				       struct pmu_board_obj *obj,
				       struct nv_pmu_boardobj *pmu_obj)
{
	int status = 0;
	struct vfe_var_single *pvfe_var_single;
	struct nv_pmu_vfe_var_single *pset;

	status = vfe_var_pmudatainit_super(g, obj, pmu_obj);
	if (status != 0) {
		return status;
	}

	pvfe_var_single = (struct vfe_var_single *)(void *)obj;
	pset = (struct nv_pmu_vfe_var_single *)(void *)
		pmu_obj;

	pset->override_type = pvfe_var_single->override_type;
	pset->override_value = pvfe_var_single->override_value;

	return status;
}

static int vfe_var_pmudatainit_single_frequency(struct gk20a *g,
						 struct pmu_board_obj *obj,
						 struct nv_pmu_boardobj *pmu_obj)
{
	int status = 0;
	struct vfe_var_single_frequency *pvfe_var_single_frequency;
	struct nv_pmu_vfe_var_single_frequency *pset;

	status = vfe_var_pmudatainit_single(g, obj, pmu_obj);

	pvfe_var_single_frequency = (struct vfe_var_single_frequency *)
							(void *)obj;
	pset = (struct nv_pmu_vfe_var_single_frequency *)(void *)pmu_obj;

	pset->clk_domain_idx = pvfe_var_single_frequency->clk_domain_idx;

	return status;
}

static int vfe_var_build_depending_mask_single(struct gk20a *g,
		struct boardobjgrp *pboardobjgrp,
		struct vfe_var *pvfe_var)
{
	(void)g;
	(void)pboardobjgrp;
	return nvgpu_boardobjgrpmask_bit_set(
			&pvfe_var->mask_depending_vars.super,
			pvfe_var->super.idx);
}

static int vfe_var_construct_single(struct gk20a *g,
		struct pmu_board_obj **obj, size_t size, void *pargs)
{
	struct pmu_board_obj *obj_tmp = (struct pmu_board_obj *)pargs;
	struct vfe_var_single *pvfevar;
	int status = 0;

	obj_tmp->type_mask |= (u32)BIT(CTRL_PERF_VFE_VAR_TYPE_SINGLE);
	status = vfe_var_construct_super(g, obj, size, pargs);
	if (status != 0) {
		return -EINVAL;
	}

	pvfevar = (struct vfe_var_single *)(void *)*obj;
	pvfevar->super.mask_depending_build =
				vfe_var_build_depending_mask_single;
	pvfevar->super.super.pmudatainit =
			vfe_var_pmudatainit_single;

	pvfevar->override_type =
			(u8)CTRL_PERF_VFE_VAR_SINGLE_OVERRIDE_TYPE_NONE;
	pvfevar->override_value = 0;

	nvgpu_log_info(g, "Done");
	return status;
}

static int vfe_var_construct_single_frequency(struct gk20a *g,
					      struct pmu_board_obj **obj,
					      size_t size, void *pargs)
{
	struct pmu_board_obj *obj_tmp = (struct pmu_board_obj *)pargs;
	struct vfe_var_single_frequency *pvfevar;
	struct vfe_var_single_frequency *ptmpvar =
			(struct vfe_var_single_frequency *)pargs;
	int status = 0;

	if (pmu_board_obj_get_type(pargs) != CTRL_PERF_VFE_VAR_TYPE_SINGLE_FREQUENCY) {
		return -EINVAL;
	}

	obj_tmp->type_mask |= (u32)BIT(CTRL_PERF_VFE_VAR_TYPE_SINGLE_FREQUENCY);
	status = vfe_var_construct_single(g, obj, size, pargs);
	if (status != 0) {
		return -EINVAL;
	}

	pvfevar = (struct vfe_var_single_frequency *)(void *)*obj;
	pvfevar->super.super.mask_depending_build =
				vfe_var_build_depending_mask_single;
	pvfevar->super.super.super.pmudatainit =
			vfe_var_pmudatainit_single_frequency;

	pvfevar->super.super.b_is_dynamic = false;
	pvfevar->super.super.b_is_dynamic_valid = true;
	pvfevar->clk_domain_idx = ptmpvar->clk_domain_idx;

	nvgpu_log_info(g, "Done");
	return status;
}

static int vfe_var_pmudatainit_single_caller_specified(struct gk20a *g,
						 struct pmu_board_obj *obj,
						 struct nv_pmu_boardobj *pmu_obj)
{
	int status = 0;
	struct vfe_var_single_caller_specified
					*pvfe_var_single_caller_specified;
	struct nv_pmu_vfe_var_single_caller_specified *pset;

	status = vfe_var_pmudatainit_single(g, obj, pmu_obj);

	pvfe_var_single_caller_specified =
		(struct vfe_var_single_caller_specified *)(void *)obj;
	pset = (struct nv_pmu_vfe_var_single_caller_specified *)
							(void *)pmu_obj;

	pset->uid = pvfe_var_single_caller_specified->uid;

	return status;
}

static int vfe_var_construct_single_caller_specified(struct gk20a *g,
					      struct pmu_board_obj **obj,
					      size_t size, void *pargs)
{
	struct pmu_board_obj *obj_tmp = (struct pmu_board_obj *)pargs;
	struct vfe_var_single_caller_specified *pvfevar;
	struct vfe_var_single_caller_specified *ptmpvar =
			(struct vfe_var_single_caller_specified *)pargs;
	int status = 0;

	if (pmu_board_obj_get_type(pargs) != CTRL_PERF_VFE_VAR_TYPE_SINGLE_FREQUENCY) {
		return -EINVAL;
	}

	obj_tmp->type_mask |= (u32)BIT(CTRL_PERF_VFE_VAR_TYPE_SINGLE_FREQUENCY);
	status = vfe_var_construct_single(g, obj, size, pargs);
	if (status != 0) {
		return -EINVAL;
	}

	pvfevar = (struct vfe_var_single_caller_specified *)(void *)*obj;

	pvfevar->super.super.super.pmudatainit =
			vfe_var_pmudatainit_single_caller_specified;

	pvfevar->super.super.b_is_dynamic = false;
	pvfevar->super.super.b_is_dynamic_valid = true;
	pvfevar->uid = ptmpvar->uid;

	nvgpu_log_info(g, "Done");
	return status;
}

static int vfe_var_pmudatainit_single_sensed(struct gk20a *g,
					      struct pmu_board_obj *obj,
					      struct nv_pmu_boardobj *pmu_obj)
{
	return vfe_var_pmudatainit_single(g, obj, pmu_obj);
}

static int vfe_var_pmudatainit_single_sensed_fuse(struct gk20a *g,
						   struct pmu_board_obj *obj,
						   struct nv_pmu_boardobj *pmu_obj)
{
	int status = 0;
	struct vfe_var_single_sensed_fuse *pvfe_var_single_sensed_fuse;
	struct nv_pmu_vfe_var_single_sensed_fuse *pset;

	status = vfe_var_pmudatainit_single_sensed(g, obj, pmu_obj);
	if (status != 0) {
		return status;
	}

	pvfe_var_single_sensed_fuse =
		(struct vfe_var_single_sensed_fuse *)(void *)obj;

	pset = (struct nv_pmu_vfe_var_single_sensed_fuse *)(void *)
		pmu_obj;

	nvgpu_memcpy((u8 *)&pset->vfield_info,
		(u8 *)&pvfe_var_single_sensed_fuse->vfield_info,
		sizeof(struct ctrl_perf_vfe_var_single_sensed_fuse_vfield_info));

	nvgpu_memcpy((u8 *)&pset->vfield_ver_info,
		(u8 *)&pvfe_var_single_sensed_fuse->vfield_ver_info,
		sizeof(struct ctrl_perf_vfe_var_single_sensed_fuse_ver_vfield_info));

	nvgpu_memcpy((u8 *)&pset->override_info,
		(u8 *)&pvfe_var_single_sensed_fuse->override_info,
		sizeof(struct ctrl_perf_vfe_var_single_sensed_fuse_override_info));

	pset->b_fuse_value_signed = pvfe_var_single_sensed_fuse->b_fuse_value_signed;
	return status;
}

static int vfe_var_construct_single_sensed(struct gk20a *g,
					   struct pmu_board_obj **obj,
					   size_t size, void *pargs)
{
	struct pmu_board_obj *obj_tmp = (struct pmu_board_obj *)pargs;
	struct vfe_var_single_sensed *pvfevar;
	int status = 0;

	obj_tmp->type_mask |= (u32)BIT(CTRL_PERF_VFE_VAR_TYPE_SINGLE_SENSED);
	status = vfe_var_construct_single(g, obj, size, pargs);
	if (status != 0) {
		return -EINVAL;
	}

	pvfevar = (struct vfe_var_single_sensed *)(void *)*obj;

	pvfevar->super.super.super.pmudatainit =
			vfe_var_pmudatainit_single_sensed;

	nvgpu_log_info(g, "Done");

	return status;
}

static int vfe_var_construct_single_sensed_fuse(struct gk20a *g,
						struct pmu_board_obj **obj,
						size_t size, void *pargs)
{
	struct pmu_board_obj *obj_tmp = (struct pmu_board_obj *)pargs;
	struct vfe_var_single_sensed_fuse *pvfevar;
	struct vfe_var_single_sensed_fuse *ptmpvar =
			(struct vfe_var_single_sensed_fuse *)pargs;
	int status = 0;

	if (pmu_board_obj_get_type(pargs) != CTRL_PERF_VFE_VAR_TYPE_SINGLE_SENSED_FUSE) {
		return -EINVAL;
	}

	obj_tmp->type_mask |= (u32)BIT(CTRL_PERF_VFE_VAR_TYPE_SINGLE_SENSED_FUSE);
	status = vfe_var_construct_single_sensed(g, obj, size, pargs);
	if (status != 0) {
		return -EINVAL;
	}

	pvfevar = (struct vfe_var_single_sensed_fuse *)(void *)*obj;

	pvfevar->super.super.super.super.pmudatainit =
			vfe_var_pmudatainit_single_sensed_fuse;

	pvfevar->vfield_info.v_field_id = ptmpvar->vfield_info.v_field_id;
	pvfevar->vfield_info.fuse_val_default =
		ptmpvar->vfield_info.fuse_val_default;
	pvfevar->vfield_info.hw_correction_scale =
		ptmpvar->vfield_info.hw_correction_scale;
	pvfevar->vfield_info.hw_correction_offset =
		ptmpvar->vfield_info.hw_correction_offset;
	pvfevar->vfield_ver_info.v_field_id_ver =
		ptmpvar->vfield_ver_info.v_field_id_ver;
	pvfevar->vfield_ver_info.ver_expected =
		ptmpvar->vfield_ver_info.ver_expected;
	pvfevar->vfield_ver_info.b_ver_expected_is_mask =
		ptmpvar->vfield_ver_info.b_ver_expected_is_mask;
	pvfevar->vfield_ver_info.b_use_default_on_ver_check_fail =
		ptmpvar->vfield_ver_info.b_use_default_on_ver_check_fail;
	pvfevar->b_version_check_done = false;
	pvfevar->b_fuse_value_signed =
		ptmpvar->b_fuse_value_signed;
	pvfevar->super.super.super.b_is_dynamic = false;
	pvfevar->super.super.super.b_is_dynamic_valid = true;

	status = dev_init_get_vfield_info(g, pvfevar);
	if (status != 0) {
		nvgpu_err(g, "Get vfield table failed");
		goto exit;
	}
	/*check whether fuse segment got initialized*/
	if (pvfevar->vfield_info.fuse.segment_count == 0U) {
		nvgpu_err(g, "unable to get fuse reg info %x",
			pvfevar->vfield_info.v_field_id);
		status = -EINVAL;
		goto exit;
	}
	if (pvfevar->vfield_ver_info.fuse.segment_count == 0U) {
		nvgpu_err(g, "unable to get fuse reg info %x",
			pvfevar->vfield_ver_info.v_field_id_ver);
		status = -EINVAL;
		goto exit;
	}
exit:
	if (status != 0) {
		(*obj)->destruct(*obj);
	}

	return status;
}

static int vfe_var_pmudatainit_single_sensed_temp(struct gk20a *g,
						   struct pmu_board_obj *obj,
						   struct nv_pmu_boardobj *pmu_obj)
{
	int status = 0;
	struct vfe_var_single_sensed_temp *pvfe_var_single_sensed_temp;
	struct nv_pmu_vfe_var_single_sensed_temp *pset;

	status = vfe_var_pmudatainit_single_sensed(g, obj, pmu_obj);
	if (status != 0) {
		return status;
	}

	pvfe_var_single_sensed_temp =
		(struct vfe_var_single_sensed_temp *)(void *)obj;

	pset = (struct nv_pmu_vfe_var_single_sensed_temp *)(void *)
		pmu_obj;
	pset->therm_channel_index =
		 pvfe_var_single_sensed_temp->therm_channel_index;
	pset->temp_hysteresis_positive =
		 pvfe_var_single_sensed_temp->temp_hysteresis_positive;
	pset->temp_hysteresis_negative =
		 pvfe_var_single_sensed_temp->temp_hysteresis_negative;
	pset->temp_default =
		 pvfe_var_single_sensed_temp->temp_default;
	return status;
}

static int vfe_var_construct_single_sensed_temp(struct gk20a *g,
						struct pmu_board_obj **obj,
						size_t size, void *pargs)
{
	struct pmu_board_obj *obj_tmp = (struct pmu_board_obj *)pargs;
	struct vfe_var_single_sensed_temp *pvfevar;
	struct vfe_var_single_sensed_temp *ptmpvar =
			(struct vfe_var_single_sensed_temp *)pargs;
	int status = 0;

	if (pmu_board_obj_get_type(pargs) != CTRL_PERF_VFE_VAR_TYPE_SINGLE_SENSED_TEMP) {
		return -EINVAL;
	}

	obj_tmp->type_mask |= (u32)BIT(CTRL_PERF_VFE_VAR_TYPE_SINGLE_SENSED_TEMP);
	status = vfe_var_construct_single_sensed(g, obj, size, pargs);
	if (status != 0) {
		return -EINVAL;
	}

	pvfevar = (struct vfe_var_single_sensed_temp *)(void *)*obj;

	pvfevar->super.super.super.super.pmudatainit =
			vfe_var_pmudatainit_single_sensed_temp;

	pvfevar->therm_channel_index =
		 ptmpvar->therm_channel_index;
	pvfevar->temp_hysteresis_positive =
		 ptmpvar->temp_hysteresis_positive;
	pvfevar->temp_hysteresis_negative =
		 ptmpvar->temp_hysteresis_negative;
	pvfevar->temp_default =
		 ptmpvar->temp_default;
	pvfevar->super.super.super.b_is_dynamic = false;
	pvfevar->super.super.super.b_is_dynamic_valid = true;

	return status;
}

static int vfe_var_pmudatainit_single_voltage(struct gk20a *g,
					       struct pmu_board_obj *obj,
					       struct nv_pmu_boardobj *pmu_obj)
{
	return vfe_var_pmudatainit_single(g, obj, pmu_obj);
}

static int vfe_var_construct_single_voltage(struct gk20a *g,
					    struct pmu_board_obj **obj,
					    size_t size, void *pargs)
{
	struct pmu_board_obj *obj_tmp = (struct pmu_board_obj *)pargs;
	struct vfe_var_single_voltage *pvfevar;
	int status = 0;

	if (pmu_board_obj_get_type(pargs) != CTRL_PERF_VFE_VAR_TYPE_SINGLE_VOLTAGE) {
		return -EINVAL;
	}

	obj_tmp->type_mask |= (u32)BIT(CTRL_PERF_VFE_VAR_TYPE_SINGLE_VOLTAGE);
	status = vfe_var_construct_super(g, obj, size, pargs);
	if (status != 0) {
		return -EINVAL;
	}

	pvfevar = (struct vfe_var_single_voltage *)(void *)*obj;
	pvfevar->super.super.mask_depending_build =
			vfe_var_build_depending_mask_single;
	pvfevar->super.super.super.pmudatainit =
			vfe_var_pmudatainit_single_voltage;

	pvfevar->super.super.b_is_dynamic = false;
	pvfevar->super.super.b_is_dynamic_valid = true;

	return status;
}

static struct vfe_var *construct_vfe_var(struct gk20a *g, void *pargs)
{
	struct pmu_board_obj *obj = NULL;
	int status;

	switch (pmu_board_obj_get_type(pargs)) {
	case CTRL_PERF_VFE_VAR_TYPE_DERIVED_PRODUCT:
		status = vfe_var_construct_derived_product(g, &obj,
			sizeof(struct vfe_var_derived_product), pargs);
		break;

	case CTRL_PERF_VFE_VAR_TYPE_DERIVED_SUM:
		status = vfe_var_construct_derived_sum(g, &obj,
			sizeof(struct vfe_var_derived_sum), pargs);
		break;

	case CTRL_PERF_VFE_VAR_TYPE_SINGLE_FREQUENCY:
		status = vfe_var_construct_single_frequency(g, &obj,
			sizeof(struct vfe_var_single_frequency), pargs);
		break;

	case CTRL_PERF_VFE_VAR_TYPE_SINGLE_SENSED_FUSE:
		status = vfe_var_construct_single_sensed_fuse(g, &obj,
			sizeof(struct vfe_var_single_sensed_fuse), pargs);
		break;

	case CTRL_PERF_VFE_VAR_TYPE_SINGLE_SENSED_TEMP:
		status = vfe_var_construct_single_sensed_temp(g, &obj,
			sizeof(struct vfe_var_single_sensed_temp), pargs);
		break;

	case CTRL_PERF_VFE_VAR_TYPE_SINGLE_VOLTAGE:
		status = vfe_var_construct_single_voltage(g, &obj,
			sizeof(struct vfe_var_single_voltage), pargs);
		break;

	case CTRL_PERF_VFE_VAR_TYPE_SINGLE_CALLER_SPECIFIED:
		status = vfe_var_construct_single_caller_specified(g, &obj,
			sizeof(struct vfe_var_single_caller_specified), pargs);
		break;

	case CTRL_PERF_VFE_VAR_TYPE_DERIVED:
	case CTRL_PERF_VFE_VAR_TYPE_SINGLE_SENSED:
	case CTRL_PERF_VFE_VAR_TYPE_SINGLE:
	default:
		status = -EINVAL;
		break;
	}

	if (status != 0) {
		return NULL;
	}

	nvgpu_log_info(g, "done");

	return (struct vfe_var *)(void *)obj;
}

static int devinit_get_vfe_var_table(struct gk20a *g,
				     struct vfe_vars *pvfevarobjs)
{
	int status = 0;
	u8 *vfevars_tbl_ptr = NULL;
	struct vbios_vfe_3x_header_struct vfevars_tbl_header = { 0 };
	struct vbios_vfe_3x_var_entry_struct var = { 0 };
	u8 *vfevars_tbl_entry_ptr = NULL;
	u8 *rd_offset_ptr = NULL;
	u32 index = 0;
	struct vfe_var *pvar;
	u8 var_type;
	u32 szfmt, val;
	bool done = false;
	u32 hdrszfmt = 0;
	u8 clk_domain_idx_available =
		VBIOS_VFE_3X_VAR_ENTRY_PAR0_SFREQ_CLK_DOMAIN_IS_AVAILABLE_NO;

	union {
		struct pmu_board_obj obj;
		struct vfe_var super;
		struct vfe_var_derived_product derived_product;
		struct vfe_var_derived_sum derived_sum;
		struct vfe_var_single_sensed_fuse single_sensed_fuse;
		struct vfe_var_single_sensed_temp single_sensed_temp;
		struct vfe_var_single_frequency single_freq;
		struct vfe_var_single_caller_specified single_caller_specified;
	} var_data;

	vfevars_tbl_ptr = (u8 *)nvgpu_bios_get_perf_table_ptrs(g,
			nvgpu_bios_get_bit_token(g, NVGPU_BIOS_PERF_TOKEN),
			CONTINUOUS_VIRTUAL_BINNING_TABLE);
	if (vfevars_tbl_ptr == NULL) {
		status = -EINVAL;
		goto done;
	}

	nvgpu_memcpy((u8 *)&vfevars_tbl_header, vfevars_tbl_ptr,
			VBIOS_VFE_3X_HEADER_SIZE_09);
	if (vfevars_tbl_header.header_size == VBIOS_VFE_3X_HEADER_SIZE_09) {
		hdrszfmt = VBIOS_VFE_3X_HEADER_SIZE_09;
		nvgpu_memcpy((u8 *)&vfevars_tbl_header, vfevars_tbl_ptr, hdrszfmt);
	} else {
		nvgpu_err(g, "Invalid VFE Table Header size\n");
		status = -EINVAL;
		goto done;
	}

	if (vfevars_tbl_header.vfe_var_entry_size ==
			VBIOS_VFE_3X_VAR_ENTRY_SIZE_19) {
		szfmt = VBIOS_VFE_3X_VAR_ENTRY_SIZE_19;
	} else {
		nvgpu_err(g, "Invalid VFE VAR Entry size\n");
		status = -EINVAL;
		goto done;
	}

	/* Read table entries*/
	vfevars_tbl_entry_ptr = vfevars_tbl_ptr + hdrszfmt;
	for (index = 0;
	     index < vfevars_tbl_header.vfe_var_entry_count;
	     index++) {
		rd_offset_ptr = vfevars_tbl_entry_ptr +
				(index * vfevars_tbl_header.vfe_var_entry_size);
		nvgpu_memcpy((u8 *)&var, rd_offset_ptr, szfmt);

		var_data.super.out_range_min = var.out_range_min;
		var_data.super.out_range_max = var.out_range_max;

		switch ((u8)var.type) {
		case VBIOS_VFE_3X_VAR_ENTRY_TYPE_DISABLED:
			continue;
			break;

		case VBIOS_VFE_3X_VAR_ENTRY_TYPE_SINGLE_FREQUENCY:
			var_type = (u8)CTRL_PERF_VFE_VAR_TYPE_SINGLE_FREQUENCY;
			clk_domain_idx_available =
				BIOS_GET_FIELD(u8, var.param0,
					VBIOS_VFE_3X_VAR_ENTRY_PAR0_SFREQ_CLK_DOMAIN_IS_AVAILABLE);
			if (clk_domain_idx_available ==
					VBIOS_VFE_3X_VAR_ENTRY_PAR0_SFREQ_CLK_DOMAIN_IS_AVAILABLE_YES) {
				var_data.single_freq.clk_domain_idx =
					BIOS_GET_FIELD(u8, var.param0,
						VBIOS_VFE_3X_VAR_ENTRY_PAR0_SFREQ_CLK_DOMAIN_IDX);
			};
			break;

		case VBIOS_VFE_3X_VAR_ENTRY_TYPE_SINGLE_VOLTAGE:
			var_type = (u8)CTRL_PERF_VFE_VAR_TYPE_SINGLE_VOLTAGE;
			break;

		case VBIOS_VFE_3X_VAR_ENTRY_TYPE_SINGLE_CALLER_SPECIFIED:
			var_type = (u8)CTRL_PERF_VFE_VAR_TYPE_SINGLE_CALLER_SPECIFIED;
			var_data.single_caller_specified.uid =
				BIOS_GET_FIELD(u8, var.param0,
					VBIOS_VFE_3X_VAR_ENTRY_PAR0_SINGLE_CALLER_SPECIFIED_UID);
			break;

		case VBIOS_VFE_3X_VAR_ENTRY_TYPE_SINGLE_SENSED_TEMP:
			var_type = (u8)CTRL_PERF_VFE_VAR_TYPE_SINGLE_SENSED_TEMP;
			var_data.single_sensed_temp.temp_default = 0x9600;
			var_data.single_sensed_temp.therm_channel_index =
				BIOS_GET_FIELD(u8, var.param0,
				VBIOS_VFE_3X_VAR_ENTRY_PAR0_SSTEMP_TH_CH_IDX);
			val = BIOS_GET_FIELD(u32, var.param0,
			      VBIOS_VFE_3X_VAR_ENTRY_PAR0_SSTEMP_HYS_POS) << 5U;
			var_data.single_sensed_temp.temp_hysteresis_positive =
				(int)val;
			val = BIOS_GET_FIELD(u32, var.param0,
			      VBIOS_VFE_3X_VAR_ENTRY_PAR0_SSTEMP_HYS_NEG) << 5U;
			var_data.single_sensed_temp.temp_hysteresis_negative =
				(int)val;
			break;

		case VBIOS_VFE_3X_VAR_ENTRY_TYPE_SINGLE_SENSED_FUSE:
			var_type = (u8)CTRL_PERF_VFE_VAR_TYPE_SINGLE_SENSED_FUSE;
			var_data.single_sensed_fuse.vfield_info.v_field_id =
				BIOS_GET_FIELD(u8, var.param0,
					VBIOS_VFE_3X_VAR_ENTRY_PAR0_SSFUSE_VFIELD_ID);
			var_data.single_sensed_fuse.vfield_ver_info.v_field_id_ver =
				BIOS_GET_FIELD(u8, var.param0,
					VBIOS_VFE_3X_VAR_ENTRY_PAR0_SSFUSE_VFIELD_ID_VER);
			var_data.single_sensed_fuse.vfield_ver_info.ver_expected =
				BIOS_GET_FIELD(u8, var.param0,
					VBIOS_VFE_3X_VAR_ENTRY_PAR0_SSFUSE_EXPECTED_VER);
			var_data.single_sensed_fuse.vfield_ver_info.b_ver_expected_is_mask =
				BIOS_GET_FIELD(u8, var.param0,
					VBIOS_VFE_3X_VAR_ENTRY_PAR0_SSFUSE_EXPECTED_VER_MODE);
			var_data.single_sensed_fuse.vfield_ver_info.b_use_default_on_ver_check_fail =
				(BIOS_GET_FIELD(bool, var.param0,
					VBIOS_VFE_3X_VAR_ENTRY_PAR0_SSFUSE_USE_DEFAULT_ON_VER_CHECK_FAIL) &&
					(VBIOS_VFE_3X_VAR_ENTRY_PAR0_SSFUSE_USE_DEFAULT_ON_VER_CHECK_FAIL_YES != 0U));
			var_data.single_sensed_fuse.b_fuse_value_signed =
				BIOS_GET_FIELD(bool, var.param0,
					VBIOS_VFE_3X_VAR_ENTRY_PAR0_SSFUSE_VALUE_SIGNED_INTEGER);
			var_data.single_sensed_fuse.vfield_info.fuse_val_default =
				var.param1;
			var_data.single_sensed_fuse.vfield_info.hw_correction_scale  =
				var.param2;
			var_data.single_sensed_fuse.vfield_info.hw_correction_offset =
				(int)var.param3;
			break;

		case VBIOS_VFE_3X_VAR_ENTRY_TYPE_DERIVED_PRODUCT:
			var_type = (u8)CTRL_PERF_VFE_VAR_TYPE_DERIVED_PRODUCT;
			var_data.derived_product.var_idx0 =
				BIOS_GET_FIELD(u8, var.param0,
					VBIOS_VFE_3X_VAR_ENTRY_PAR0_DPROD_VFE_VAR_IDX_0);
			var_data.derived_product.var_idx1 =
				BIOS_GET_FIELD(u8, var.param0,
					VBIOS_VFE_3X_VAR_ENTRY_PAR0_DPROD_VFE_VAR_IDX_1);
			break;

		case VBIOS_VFE_3X_VAR_ENTRY_TYPE_DERIVED_SUM:
			var_type = (u8)CTRL_PERF_VFE_VAR_TYPE_DERIVED_SUM;
			var_data.derived_sum.var_idx0 =
				BIOS_GET_FIELD(u8, var.param0,
					VBIOS_VFE_3X_VAR_ENTRY_PAR0_DSUM_VFE_VAR_IDX_0);
			var_data.derived_sum.var_idx1 =
				BIOS_GET_FIELD(u8, var.param0,
					VBIOS_VFE_3X_VAR_ENTRY_PAR0_DSUM_VFE_VAR_IDX_1);
			break;
		default:
			status = -EINVAL;
			done = true;
			break;
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

		var_data.obj.type = var_type;
		var_data.obj.type_mask = 0;

		pvar = construct_vfe_var(g, &var_data);
		if (pvar == NULL) {
			nvgpu_err(g,
				  "error constructing vfe_var boardobj %d",
				  index);
			status = -EINVAL;
			goto done;
		}

		status = boardobjgrp_objinsert(&pvfevarobjs->super.super,
					       (struct pmu_board_obj *)pvar, (u8)index);
		if (status != 0) {
			nvgpu_err(g, "error adding vfe_var boardobj %d", index);
			status = -EINVAL;
			goto done;
		}
	}
	pvfevarobjs->polling_periodms = vfevars_tbl_header.polling_periodms;
done:
	nvgpu_log_info(g, "done status %x", status);
	return status;
}

static int vfe_var_boardobj_grp_get_status(struct gk20a *g)
{

	struct boardobjgrp *pboardobjgrp;
	struct boardobjgrpmask *pboardobjgrpmask;
	struct nv_pmu_boardobjgrp_super *pboardobjgrppmu;
	struct pmu_board_obj *obj = NULL;
	struct nv_pmu_boardobj_query *pboardobjpmustatus = NULL;
	struct vfe_var_single_sensed_fuse *single_sensed_fuse = NULL;
	int status;
	u8 index;

	pboardobjgrp = &g->pmu->perf_pmu->vfe_varobjs.super.super;
	pboardobjgrpmask = &g->pmu->perf_pmu->vfe_varobjs.super.mask.super;

	status = pboardobjgrp->pmugetstatus(g, pboardobjgrp, pboardobjgrpmask);
	if (status != 0) {
		nvgpu_err(g, "err getting boardobjs from pmu");
		return status;
	}

	pboardobjgrppmu = pboardobjgrp->pmu.getstatus.buf;

	BOARDOBJGRP_FOR_EACH(pboardobjgrp, struct pmu_board_obj*, obj, index) {
		single_sensed_fuse = (struct vfe_var_single_sensed_fuse *)
				(void *)obj;
		status = pboardobjgrp->pmustatusinstget(g,
			(struct nv_pmu_boardobjgrp *)(void *)pboardobjgrppmu,
				&pboardobjpmustatus, index);
		if (status != 0) {
			nvgpu_err(g, "could not get status object instance");
			return status;
		}
		/* At present we are updating only s_param,
		 * in future we can add other fields if required */
		if (single_sensed_fuse->vfield_info.v_field_id ==
				VFIELD_ID_S_PARAM) {
			status = vfe_var_get_s_param_value(g,
					single_sensed_fuse,
					(struct nv_pmu_boardobj *)
					(void *)pboardobjpmustatus);
			if (status != 0) {
				nvgpu_err(g,
				"could not get single sensed fuse value");
				return status;
			}
		break;
		}
	}
	return 0;
}

int perf_vfe_var_sw_setup(struct gk20a *g)
{
	int status;
	struct boardobjgrp *pboardobjgrp = NULL;
	struct vfe_vars *pvfevarobjs;

	status = nvgpu_boardobjgrp_construct_e32(g,
			&g->pmu->perf_pmu->vfe_varobjs.super);
	if (status != 0) {
		nvgpu_err(g,
			  "error creating boardobjgrp for clk domain, "
			  "status - 0x%x", status);
		goto done;
	}

	pboardobjgrp = &g->pmu->perf_pmu->vfe_varobjs.super.super;
	pvfevarobjs = &g->pmu->perf_pmu->vfe_varobjs;

	BOARDOBJGRP_PMU_CONSTRUCT(pboardobjgrp, PERF, VFE_VAR);

	status = BOARDOBJGRP_PMU_CMD_GRP_SET_CONSTRUCT(g, pboardobjgrp,
			perf, PERF, vfe_var, VFE_VAR);
	if (status != 0) {
		nvgpu_err(g, "error constructing GRP_SET interface - 0x%x",
			status);
		goto done;
	}

	pboardobjgrp->pmudatainit  = vfe_vars_pmudatainit;
	pboardobjgrp->pmudatainstget  = vfe_vars_pmudata_instget;
	pboardobjgrp->pmustatusinstget  = vfe_vars_pmustatus_instget;

	status = devinit_get_vfe_var_table(g, pvfevarobjs);
	if (status != 0) {
		goto done;
	}

	status = vfe_var_dependency_mask_build(g, pvfevarobjs);
	if (status != 0) {
		goto done;
	}

	status = BOARDOBJGRP_PMU_CMD_GRP_GET_STATUS_CONSTRUCT(g,
				&g->pmu->perf_pmu->vfe_varobjs.super.super,
				perf, PERF, vfe_var, VFE_VAR);
	if (status != 0) {
		nvgpu_err(g,
			"error constructing GRP_GET_STATUS interface - 0x%x",
			status);
		goto done;
	}

done:
	nvgpu_log_info(g, " done status %x", status);
	return status;
}

int perf_vfe_var_pmu_setup(struct gk20a *g)
{
	int status;
	struct boardobjgrp *pboardobjgrp = NULL;

	pboardobjgrp = &g->pmu->perf_pmu->vfe_varobjs.super.super;

	if (!pboardobjgrp->bconstructed) {
		return -EINVAL;
	}

	status = pboardobjgrp->pmuinithandle(g, pboardobjgrp);

	nvgpu_log_info(g, "Done");
	return status;
}

int nvgpu_pmu_perf_vfe_get_s_param(struct gk20a *g, u64 *s_param)
{
	struct boardobjgrp *pboardobjgrp;
	struct pmu_board_obj *obj = NULL;
	struct vfe_var_single_sensed_fuse *single_sensed_fuse = NULL;
	u8 index;
	int status;

	status = vfe_var_boardobj_grp_get_status(g);
	if (status != 0) {
		nvgpu_err(g, "Vfe_var get status failed");
		return status;
	}

	pboardobjgrp = &g->pmu->perf_pmu->vfe_varobjs.super.super;

	BOARDOBJGRP_FOR_EACH(pboardobjgrp, struct pmu_board_obj*, obj, index) {
		single_sensed_fuse = (struct vfe_var_single_sensed_fuse *)
				(void *)obj;
		if (single_sensed_fuse->vfield_info.v_field_id ==
				VFIELD_ID_S_PARAM) {
			*s_param = single_sensed_fuse->fuse_value_hw_integer;
		}
	}
	return status;
}
