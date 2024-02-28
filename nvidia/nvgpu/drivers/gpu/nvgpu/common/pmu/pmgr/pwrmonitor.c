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
#include <nvgpu/string.h>

#include "pwrdev.h"
#include "pmgr.h"

static int _pwr_channel_pmudata_instget(struct gk20a *g,
			struct nv_pmu_boardobjgrp *pmuboardobjgrp,
			struct nv_pmu_boardobj **pmu_obj,
			u8 idx)
{
	struct nv_pmu_pmgr_pwr_channel_desc *ppmgrchannel =
		(struct nv_pmu_pmgr_pwr_channel_desc *)pmuboardobjgrp;

	nvgpu_log_info(g, " ");

	/*check whether pmuboardobjgrp has a valid boardobj in index*/
	if (((u32)BIT(idx) &
		ppmgrchannel->hdr.data.super.obj_mask.super.data[0]) == 0U) {
		return -EINVAL;
	}

	*pmu_obj = (struct nv_pmu_boardobj *)
		&ppmgrchannel->channels[idx].data.obj;

	/* handle Global/common data here as we need index */
	ppmgrchannel->channels[idx].data.channel.ch_idx = idx;

	nvgpu_log_info(g, " Done");

	return 0;
}

static int _pwr_channel_rels_pmudata_instget(struct gk20a *g,
			struct nv_pmu_boardobjgrp *pmuboardobjgrp,
			struct nv_pmu_boardobj **pmu_obj,
			u8 idx)
{
	struct nv_pmu_pmgr_pwr_chrelationship_desc *ppmgrchrels =
		(struct nv_pmu_pmgr_pwr_chrelationship_desc *)pmuboardobjgrp;

	nvgpu_log_info(g, " ");

	/*check whether pmuboardobjgrp has a valid boardobj in index*/
	if (((u32)BIT(idx) &
		ppmgrchrels->hdr.data.super.obj_mask.super.data[0]) == 0U) {
		return -EINVAL;
	}

	*pmu_obj = (struct nv_pmu_boardobj *)
		&ppmgrchrels->ch_rels[idx].data.obj;

	nvgpu_log_info(g, " Done");

	return 0;
}

static int _pwr_channel_state_init(struct gk20a *g)
{
	u8 indx = 0;
	struct pwr_channel *pchannel;
	u32 objmask =
		g->pmgr_pmu->pmgr_monitorobjs.pwr_channels.super.objmask;

	/* Initialize each PWR_CHANNEL's dependent channel mask */
	BOARDOBJGRP_FOR_EACH_INDEX_IN_MASK(32, indx, objmask) {
		pchannel = PMGR_PWR_MONITOR_GET_PWR_CHANNEL(g, indx);
		if (pchannel == NULL) {
			nvgpu_err(g,
				"PMGR_PWR_MONITOR_GET_PWR_CHANNEL-failed %d", indx);
			return -EINVAL;
		}
		pchannel->dependent_ch_mask =0;
	}
	BOARDOBJGRP_FOR_EACH_INDEX_IN_MASK_END

	return 0;
}

static bool _pwr_channel_implements(struct pwr_channel *pchannel,
			u8 type)
{
	return (type == pmu_board_obj_get_type((struct pmu_board_obj *)
			(void *)pchannel));
}

static int _pwr_domains_pmudatainit_sensor(struct gk20a *g,
					struct pmu_board_obj *obj,
					struct nv_pmu_boardobj *pmu_obj)
{
	struct nv_pmu_pmgr_pwr_channel_sensor *pmu_sensor_data;
	struct pwr_channel_sensor *sensor;
	int status = 0;

	status = pmu_board_obj_pmu_data_init_super(g, obj, pmu_obj);
	if (status != 0) {
		nvgpu_err(g,
			  "error updating pmu boardobjgrp for pwr sensor 0x%x",
			  status);
		goto done;
	}

	sensor = (struct pwr_channel_sensor *)(void *)obj;
	pmu_sensor_data = (struct nv_pmu_pmgr_pwr_channel_sensor *)
			(void *) pmu_obj;

	pmu_sensor_data->super.pwr_rail = sensor->super.pwr_rail;
	pmu_sensor_data->super.volt_fixedu_v = sensor->super.volt_fixed_uv;
	pmu_sensor_data->super.pwr_corr_slope = sensor->super.pwr_corr_slope;
	pmu_sensor_data->super.pwr_corr_offsetm_w = sensor->super.pwr_corr_offset_mw;
	pmu_sensor_data->super.curr_corr_slope = sensor->super.curr_corr_slope;
	pmu_sensor_data->super.curr_corr_offsetm_a = sensor->super.curr_corr_offset_ma;
	pmu_sensor_data->super.dependent_ch_mask = sensor->super.dependent_ch_mask;
	pmu_sensor_data->super.ch_idx = 0;

	pmu_sensor_data->pwr_dev_idx = sensor->pwr_dev_idx;
	pmu_sensor_data->pwr_dev_prov_idx = sensor->pwr_dev_prov_idx;

done:
	return status;
}

static struct pmu_board_obj *construct_pwr_topology(struct gk20a *g,
				void *pargs, size_t pargs_size, u8 type)
{
	struct pmu_board_obj *obj = NULL;
	int status;
	struct pwr_channel_sensor *pwrchannel;
	struct pwr_channel_sensor *sensor = (struct pwr_channel_sensor*)pargs;

	(void)type;

	pwrchannel = nvgpu_kzalloc(g, pargs_size);
	if (pwrchannel == NULL) {
		return NULL;
	}
	obj = (struct pmu_board_obj *)(void *)pwrchannel;

	status = pmu_board_obj_construct_super(g, obj, pargs);
	if (status != 0) {
		return NULL;
	}

	pwrchannel = (struct pwr_channel_sensor *)(void *)obj;

	/* Set Super class interfaces */
	obj->pmudatainit = _pwr_domains_pmudatainit_sensor;

	pwrchannel->super.pwr_rail = sensor->super.pwr_rail;
	pwrchannel->super.volt_fixed_uv = sensor->super.volt_fixed_uv;
	pwrchannel->super.pwr_corr_slope = sensor->super.pwr_corr_slope;
	pwrchannel->super.pwr_corr_offset_mw = sensor->super.pwr_corr_offset_mw;
	pwrchannel->super.curr_corr_slope = sensor->super.curr_corr_slope;
	pwrchannel->super.curr_corr_offset_ma = sensor->super.curr_corr_offset_ma;
	pwrchannel->super.dependent_ch_mask = 0;

	pwrchannel->pwr_dev_idx = sensor->pwr_dev_idx;
	pwrchannel->pwr_dev_prov_idx = sensor->pwr_dev_prov_idx;

	nvgpu_log_info(g, " Done");

	return obj;
}

static int devinit_get_pwr_topology_table(struct gk20a *g,
				struct pmgr_pwr_monitor *ppwrmonitorobjs)
{
	int status = 0;
	u8 *pwr_topology_table_ptr = NULL;
	u8 *curr_pwr_topology_table_ptr = NULL;
	struct pmu_board_obj *obj_tmp;
	struct pwr_topology_2x_header pwr_topology_table_header;
	struct pwr_topology_2x_entry pwr_topology_table_entry;
	u32 index;
	u32 obj_index = 0;
	size_t pwr_topology_size;
	union {
		struct pmu_board_obj obj;
		struct pwr_channel pwrchannel;
		struct pwr_channel_sensor sensor;
	} pwr_topology_data;

	(void) memset(&pwr_topology_table_header, 0U,
					sizeof(struct pwr_topology_2x_header));
	(void) memset(&pwr_topology_table_entry, 0U,
					sizeof(struct pwr_topology_2x_entry));

	nvgpu_log_info(g, " ");

	pwr_topology_table_ptr = (u8 *)nvgpu_bios_get_perf_table_ptrs(g,
			nvgpu_bios_get_bit_token(g, NVGPU_BIOS_PERF_TOKEN),
						POWER_TOPOLOGY_TABLE);
	if (pwr_topology_table_ptr == NULL) {
		status = -EINVAL;
		goto done;
	}

	nvgpu_memcpy((u8 *)&pwr_topology_table_header, pwr_topology_table_ptr,
		VBIOS_POWER_TOPOLOGY_2X_HEADER_SIZE_06);

	if (pwr_topology_table_header.version !=
			VBIOS_POWER_TOPOLOGY_VERSION_2X) {
		status = -EINVAL;
		goto done;
	}

	g->pmgr_pmu->pmgr_monitorobjs.b_is_topology_tbl_ver_1x = false;

	if (pwr_topology_table_header.header_size <
			VBIOS_POWER_TOPOLOGY_2X_HEADER_SIZE_06) {
		status = -EINVAL;
		goto done;
	}

	if (pwr_topology_table_header.table_entry_size !=
			VBIOS_POWER_TOPOLOGY_2X_ENTRY_SIZE_16) {
		status = -EINVAL;
		goto done;
	}

	curr_pwr_topology_table_ptr = (pwr_topology_table_ptr +
		VBIOS_POWER_TOPOLOGY_2X_HEADER_SIZE_06);

	for (index = 0; index < pwr_topology_table_header.num_table_entries;
		index++) {
		u8 class_type;

		curr_pwr_topology_table_ptr += (pwr_topology_table_header.table_entry_size * index);

		pwr_topology_table_entry.flags0 = *curr_pwr_topology_table_ptr;
		pwr_topology_table_entry.pwr_rail = *(curr_pwr_topology_table_ptr + 1);

		nvgpu_memcpy((u8 *)&pwr_topology_table_entry.param0,
			(curr_pwr_topology_table_ptr + 2),
			(VBIOS_POWER_TOPOLOGY_2X_ENTRY_SIZE_16 - 2U));

		class_type = BIOS_GET_FIELD(u8, pwr_topology_table_entry.flags0,
				NV_VBIOS_POWER_TOPOLOGY_2X_ENTRY_FLAGS0_CLASS);

		if (class_type == NV_VBIOS_POWER_TOPOLOGY_2X_ENTRY_FLAGS0_CLASS_SENSOR) {
			pwr_topology_data.sensor.pwr_dev_idx =
				BIOS_GET_FIELD(u8,
				pwr_topology_table_entry.param1,
				NV_VBIOS_POWER_TOPOLOGY_2X_ENTRY_PARAM1_SENSOR_INDEX);
			pwr_topology_data.sensor.pwr_dev_prov_idx =
				BIOS_GET_FIELD(u8,
				pwr_topology_table_entry.param1,
				NV_VBIOS_POWER_TOPOLOGY_2X_ENTRY_PARAM1_SENSOR_PROVIDER_INDEX);

			pwr_topology_size = sizeof(struct pwr_channel_sensor);
		} else {
			continue;
		}

		/* Initialize data for the parent class */
		pwr_topology_data.obj.type = CTRL_PMGR_PWR_CHANNEL_TYPE_SENSOR;
		pwr_topology_data.pwrchannel.pwr_rail = (u8)pwr_topology_table_entry.pwr_rail;
		pwr_topology_data.pwrchannel.volt_fixed_uv = pwr_topology_table_entry.param0;
		pwr_topology_data.pwrchannel.pwr_corr_slope = BIT32(12);
		pwr_topology_data.pwrchannel.pwr_corr_offset_mw = 0;
		pwr_topology_data.pwrchannel.curr_corr_slope  =
			(u32)pwr_topology_table_entry.curr_corr_slope;
		pwr_topology_data.pwrchannel.curr_corr_offset_ma =
			(s32)pwr_topology_table_entry.curr_corr_offset;

		obj_tmp = construct_pwr_topology(g, &pwr_topology_data,
					pwr_topology_size, pwr_topology_data.obj.type);

		if (obj_tmp == NULL) {
			nvgpu_err(g,
				"unable to create pwr topology for %d type %d",
				index, pwr_topology_data.obj.type);
			status = -EINVAL;
			goto done;
		}

		status = boardobjgrp_objinsert(&ppwrmonitorobjs->pwr_channels.super,
				obj_tmp, (u8)obj_index);

		if (status != 0) {
			nvgpu_err(g,
				"unable to insert pwr topology boardobj for %d", index);
			status = -EINVAL;
			goto done;
		}

		++obj_index;
	}

done:
	nvgpu_log_info(g, " done status %x", status);
	return status;
}

int pmgr_monitor_sw_setup(struct gk20a *g)
{
	int status;
	struct boardobjgrp *pboardobjgrp = NULL;
	struct pwr_channel *pchannel;
	struct pmgr_pwr_monitor *ppwrmonitorobjs;
	u8 indx = 0;

	/* Construct the Super Class and override the Interfaces */
	status = nvgpu_boardobjgrp_construct_e32(g,
			&g->pmgr_pmu->pmgr_monitorobjs.pwr_channels);
	if (status != 0) {
		nvgpu_err(g,
			"error creating boardobjgrp for pmgr channel, "
			"status - 0x%x", status);
		goto done;
	}

	pboardobjgrp = &(g->pmgr_pmu->pmgr_monitorobjs.pwr_channels.super);

	/* Override the Interfaces */
	pboardobjgrp->pmudatainstget = _pwr_channel_pmudata_instget;

	/* Construct the Super Class and override the Interfaces */
	status = nvgpu_boardobjgrp_construct_e32(g,
			&g->pmgr_pmu->pmgr_monitorobjs.pwr_ch_rels);
	if (status != 0) {
		nvgpu_err(g,
			"error creating boardobjgrp for pmgr channel "
			"relationship, status - 0x%x", status);
		goto done;
	}

	pboardobjgrp = &(g->pmgr_pmu->pmgr_monitorobjs.pwr_ch_rels.super);

	/* Override the Interfaces */
	pboardobjgrp->pmudatainstget = _pwr_channel_rels_pmudata_instget;

	/* Initialize the Total GPU Power Channel Mask to 0 */
	g->pmgr_pmu->pmgr_monitorobjs.pmu_data.channels.hdr.data.total_gpu_power_channel_mask = 0;
	g->pmgr_pmu->pmgr_monitorobjs.total_gpu_channel_idx =
			CTRL_PMGR_PWR_CHANNEL_INDEX_INVALID;

	/* Supported topology table version 1.0 */
	g->pmgr_pmu->pmgr_monitorobjs.b_is_topology_tbl_ver_1x = true;

	ppwrmonitorobjs = &(g->pmgr_pmu->pmgr_monitorobjs);

	status = devinit_get_pwr_topology_table(g, ppwrmonitorobjs);
	if (status != 0) {
		goto done;
	}

	status = _pwr_channel_state_init(g);
	if (status != 0) {
		goto done;
	}

	/* Initialise physicalChannelMask */
	g->pmgr_pmu->pmgr_monitorobjs.physical_channel_mask = 0;

	pboardobjgrp = &g->pmgr_pmu->pmgr_monitorobjs.pwr_channels.super;

	BOARDOBJGRP_FOR_EACH(pboardobjgrp, struct pwr_channel *, pchannel, indx) {
		if (_pwr_channel_implements(pchannel,
				CTRL_PMGR_PWR_CHANNEL_TYPE_SENSOR)) {
			g->pmgr_pmu->pmgr_monitorobjs.physical_channel_mask  |=
								BIT32(indx);
		}
	}

done:
	nvgpu_log_info(g, " done status %x", status);
	return status;
}
