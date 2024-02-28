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

static int _pwr_device_pmudata_instget(struct gk20a *g,
			struct nv_pmu_boardobjgrp *pmuboardobjgrp,
			struct nv_pmu_boardobj **pmu_obj,
			u8 idx)
{
	struct nv_pmu_pmgr_pwr_device_desc_table *ppmgrdevice =
		(struct nv_pmu_pmgr_pwr_device_desc_table *)pmuboardobjgrp;

	nvgpu_log_info(g, " ");

	/*check whether pmuboardobjgrp has a valid boardobj in index*/
	if (((u32)BIT(idx) &
		ppmgrdevice->hdr.data.super.obj_mask.super.data[0]) == 0U) {
		return -EINVAL;
	}

	*pmu_obj = (struct nv_pmu_boardobj *)
		&ppmgrdevice->devices[idx].data.obj;

	nvgpu_log_info(g, " Done");

	return 0;
}

static int _pwr_domains_pmudatainit_ina3221(struct gk20a *g,
			struct pmu_board_obj *obj,
			struct nv_pmu_boardobj *pmu_obj)
{
	struct nv_pmu_pmgr_pwr_device_desc_ina3221 *ina3221_desc;
	struct pwr_device_ina3221 *ina3221;
	int status = 0;
	u32 indx;

	status = pmu_board_obj_pmu_data_init_super(g, obj, pmu_obj);
	if (status != 0) {
		nvgpu_err(g,
			  "error updating pmu boardobjgrp for pwr domain 0x%x",
			  status);
		goto done;
	}

	ina3221 = (struct pwr_device_ina3221 *)(void *)obj;
	ina3221_desc = (struct nv_pmu_pmgr_pwr_device_desc_ina3221 *)
			(void *) pmu_obj;

	ina3221_desc->super.power_corr_factor = ina3221->super.power_corr_factor;
	ina3221_desc->i2c_dev_idx = ina3221->super.i2c_dev_idx;
	ina3221_desc->configuration = ina3221->configuration;
	ina3221_desc->mask_enable = ina3221->mask_enable;
	/* configure NV_PMU_THERM_EVENT_EXT_OVERT */
	ina3221_desc->event_mask = BIT32(0);
	ina3221_desc->curr_correct_m  = ina3221->curr_correct_m;
	ina3221_desc->curr_correct_b  = ina3221->curr_correct_b;

	for (indx = 0; indx < NV_PMU_PMGR_PWR_DEVICE_INA3221_CH_NUM; indx++) {
		ina3221_desc->r_shuntm_ohm[indx] = ina3221->r_shuntm_ohm[indx];
	}

done:
	return status;
}

static struct pmu_board_obj *construct_pwr_device(struct gk20a *g,
			void *pargs, size_t pargs_size, u8 type)
{
	struct pmu_board_obj *obj = NULL;
	int status;
	u32 indx;
	struct pwr_device_ina3221 *pwrdev;
	struct pwr_device_ina3221 *ina3221 = (struct pwr_device_ina3221*)pargs;

	(void)type;

	pwrdev = nvgpu_kzalloc(g, pargs_size);
	if (pwrdev == NULL) {
		return NULL;
	}
	obj = (struct pmu_board_obj *)(void *)pwrdev;

	status = pmu_board_obj_construct_super(g, obj, pargs);
	if (status != 0) {
		return NULL;
	}

	obj = (struct pmu_board_obj *)(void *)pwrdev;
	/* Set Super class interfaces */
	obj->pmudatainit = _pwr_domains_pmudatainit_ina3221;

	pwrdev = (struct pwr_device_ina3221 *)(void *)obj;

	pwrdev->super.power_rail          = ina3221->super.power_rail;
	pwrdev->super.i2c_dev_idx       = ina3221->super.i2c_dev_idx;
	pwrdev->super.power_corr_factor = BIT32(12);
	pwrdev->super.bIs_inforom_config = false;

	/* Set INA3221-specific information */
	pwrdev->configuration   = ina3221->configuration;
	pwrdev->mask_enable      = ina3221->mask_enable;
	pwrdev->gpio_function    = ina3221->gpio_function;
	pwrdev->curr_correct_m    = ina3221->curr_correct_m;
	pwrdev->curr_correct_b    = ina3221->curr_correct_b;

	for (indx = 0; indx < NV_PMU_PMGR_PWR_DEVICE_INA3221_CH_NUM; indx++) {
		pwrdev->r_shuntm_ohm[indx] = ina3221->r_shuntm_ohm[indx];
	}

	nvgpu_log_info(g, " Done");

	return obj;
}

static int devinit_get_pwr_device_table(struct gk20a *g,
			struct pwr_devices *ppwrdeviceobjs)
{
	int status = 0;
	u8 *pwr_device_table_ptr = NULL;
	u8 *curr_pwr_device_table_ptr = NULL;
	struct pmu_board_obj *obj_tmp;
	struct pwr_sensors_2x_header pwr_sensor_table_header = { 0 };
	struct pwr_sensors_2x_entry pwr_sensor_table_entry = { 0 };
	u32 index;
	u32 obj_index = 0;
	size_t pwr_device_size;
	union {
		struct pmu_board_obj obj;
		struct pwr_device pwrdev;
		struct pwr_device_ina3221 ina3221;
	} pwr_device_data;

	nvgpu_log_info(g, " ");

	pwr_device_table_ptr = (u8 *)nvgpu_bios_get_perf_table_ptrs(g,
			nvgpu_bios_get_bit_token(g, NVGPU_BIOS_PERF_TOKEN),
						POWER_SENSORS_TABLE);
	if (pwr_device_table_ptr == NULL) {
		status = -EINVAL;
		goto done;
	}

	nvgpu_memcpy((u8 *)&pwr_sensor_table_header, pwr_device_table_ptr,
		VBIOS_POWER_SENSORS_2X_HEADER_SIZE_08);

	if (pwr_sensor_table_header.version !=
			VBIOS_POWER_SENSORS_VERSION_2X) {
		status = -EINVAL;
		goto done;
	}

	if (pwr_sensor_table_header.header_size <
			VBIOS_POWER_SENSORS_2X_HEADER_SIZE_08) {
		status = -EINVAL;
		goto done;
	}

	if (pwr_sensor_table_header.table_entry_size !=
			VBIOS_POWER_SENSORS_2X_ENTRY_SIZE_15) {
		status = -EINVAL;
		goto done;
	}

	curr_pwr_device_table_ptr = (pwr_device_table_ptr +
		VBIOS_POWER_SENSORS_2X_HEADER_SIZE_08);

	for (index = 0; index < pwr_sensor_table_header.num_table_entries; index++) {
		bool use_fxp8_8 = false;
		u8 i2c_dev_idx;
		u8 device_type;

		curr_pwr_device_table_ptr += (pwr_sensor_table_header.table_entry_size * index);

		pwr_sensor_table_entry.flags0 = *curr_pwr_device_table_ptr;

		nvgpu_memcpy((u8 *)&pwr_sensor_table_entry.class_param0,
			(curr_pwr_device_table_ptr + 1),
			(VBIOS_POWER_SENSORS_2X_ENTRY_SIZE_15 - 1U));

		device_type = BIOS_GET_FIELD(u8, pwr_sensor_table_entry.flags0,
				NV_VBIOS_POWER_SENSORS_2X_ENTRY_FLAGS0_CLASS);

		if (device_type == NV_VBIOS_POWER_SENSORS_2X_ENTRY_FLAGS0_CLASS_I2C) {
			i2c_dev_idx = BIOS_GET_FIELD(u8,
				pwr_sensor_table_entry.class_param0,
				NV_VBIOS_POWER_SENSORS_2X_ENTRY_CLASS_PARAM0_I2C_INDEX);
			use_fxp8_8 = BIOS_GET_FIELD(bool,
				pwr_sensor_table_entry.class_param0,
				NV_VBIOS_POWER_SENSORS_2X_ENTRY_CLASS_PARAM0_I2C_USE_FXP8_8);

			pwr_device_data.ina3221.super.i2c_dev_idx = i2c_dev_idx;
			pwr_device_data.ina3221.r_shuntm_ohm[0].use_fxp8_8 = use_fxp8_8;
			pwr_device_data.ina3221.r_shuntm_ohm[1].use_fxp8_8 = use_fxp8_8;
			pwr_device_data.ina3221.r_shuntm_ohm[2].use_fxp8_8 = use_fxp8_8;
			pwr_device_data.ina3221.r_shuntm_ohm[0].rshunt_value =
				BIOS_GET_FIELD(u16,
				pwr_sensor_table_entry.sensor_param0,
				NV_VBIOS_POWER_SENSORS_2X_ENTRY_SENSOR_PARAM0_INA3221_RSHUNT0_MOHM);

			pwr_device_data.ina3221.r_shuntm_ohm[1].rshunt_value =
				BIOS_GET_FIELD(u16,
				pwr_sensor_table_entry.sensor_param0,
				NV_VBIOS_POWER_SENSORS_2X_ENTRY_SENSOR_PARAM0_INA3221_RSHUNT1_MOHM);

			pwr_device_data.ina3221.r_shuntm_ohm[2].rshunt_value =
				BIOS_GET_FIELD(u16,
				pwr_sensor_table_entry.sensor_param1,
				NV_VBIOS_POWER_SENSORS_2X_ENTRY_SENSOR_PARAM1_INA3221_RSHUNT2_MOHM);

			pwr_device_data.ina3221.configuration =
				BIOS_GET_FIELD(u16,
				pwr_sensor_table_entry.sensor_param1,
				NV_VBIOS_POWER_SENSORS_2X_ENTRY_SENSOR_PARAM1_INA3221_CONFIGURATION);

			pwr_device_data.ina3221.mask_enable =
				BIOS_GET_FIELD(u16,
				pwr_sensor_table_entry.sensor_param2,
				NV_VBIOS_POWER_SENSORS_2X_ENTRY_SENSOR_PARAM2_INA3221_MASKENABLE);

			pwr_device_data.ina3221.gpio_function =
				BIOS_GET_FIELD(u8,
				pwr_sensor_table_entry.sensor_param2,
				NV_VBIOS_POWER_SENSORS_2X_ENTRY_SENSOR_PARAM2_INA3221_GPIOFUNCTION);

			pwr_device_data.ina3221.curr_correct_m =
				BIOS_GET_FIELD(u16,
				pwr_sensor_table_entry.sensor_param3,
				NV_VBIOS_POWER_SENSORS_2X_ENTRY_SENSOR_PARAM3_INA3221_CURR_CORRECT_M);

			pwr_device_data.ina3221.curr_correct_b =
				BIOS_GET_FIELD(s16,
				pwr_sensor_table_entry.sensor_param3,
				NV_VBIOS_POWER_SENSORS_2X_ENTRY_SENSOR_PARAM3_INA3221_CURR_CORRECT_B);

			if (pwr_device_data.ina3221.curr_correct_m == 0U) {
				pwr_device_data.ina3221.curr_correct_m = BIT16(12);
			}
			pwr_device_size = sizeof(struct pwr_device_ina3221);
		} else {
			continue;
		}

		pwr_device_data.obj.type = CTRL_PMGR_PWR_DEVICE_TYPE_INA3221;
		pwr_device_data.pwrdev.power_rail = (u8)0;

		obj_tmp = construct_pwr_device(g, &pwr_device_data,
					pwr_device_size, pwr_device_data.obj.type);

		if (obj_tmp == NULL) {
			nvgpu_err(g,
			"unable to create pwr device for %d type %d", index,
			pwr_device_data.obj.type);
			status = -EINVAL;
			goto done;
		}

		status = boardobjgrp_objinsert(&ppwrdeviceobjs->super.super,
				obj_tmp, (u8)obj_index);

		if (status != 0) {
			nvgpu_err(g,
			"unable to insert pwr device boardobj for %d", index);
			status = -EINVAL;
			goto done;
		}

		++obj_index;
	}

done:
	nvgpu_log_info(g, " done status %x", status);
	return status;
}

int pmgr_device_sw_setup(struct gk20a *g)
{
	int status;
	struct boardobjgrp *pboardobjgrp = NULL;
	struct pwr_devices *ppwrdeviceobjs;

	/* Construct the Super Class and override the Interfaces */
	status = nvgpu_boardobjgrp_construct_e32(g,
			&g->pmgr_pmu->pmgr_deviceobjs.super);
	if (status != 0) {
		nvgpu_err(g,
			"error creating boardobjgrp for pmgr devices, "
			"status - 0x%x", status);
		goto done;
	}

	pboardobjgrp = &g->pmgr_pmu->pmgr_deviceobjs.super.super;
	ppwrdeviceobjs = &(g->pmgr_pmu->pmgr_deviceobjs);

	/* Override the Interfaces */
	pboardobjgrp->pmudatainstget = _pwr_device_pmudata_instget;

	status = devinit_get_pwr_device_table(g, ppwrdeviceobjs);
	if (status != 0) {
		goto done;
	}

done:
	nvgpu_log_info(g, " done status %x", status);
	return status;
}
