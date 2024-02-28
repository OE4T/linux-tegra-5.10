/*
 * Copyright (c) 2020, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_PMU_THERM_INF_H
#define NVGPU_PMU_THERM_INF_H

#define CTRL_THERMAL_THERM_DEVICE_CLASS_GPU                             0x01
#define CTRL_THERMAL_THERM_CHANNEL_CLASS_DEVICE                         0x01

#define NV_PMU_THERM_CMD_ID_BOARDOBJ_GRP_SET                         0x0000000B
#define NV_PMU_THERM_MSG_ID_BOARDOBJ_GRP_SET                         0x00000008
#define NV_PMU_THERM_CMD_ID_BOARDOBJ_GRP_GET_STATUS                  0x00000001U
#define NV_PMU_THERM_MSG_ID_BOARDOBJ_GRP_GET_STATUS                  0x00000001U

struct nv_pmu_therm_therm_device_boardobjgrp_set_header {
	struct nv_pmu_boardobjgrp_e32 super;
};

struct nv_pmu_therm_therm_device_boardobj_set {
	struct nv_pmu_boardobj super;
};

struct nv_pmu_therm_therm_device_gpu_gpc_tsosc_boardobj_set {
	struct nv_pmu_therm_therm_device_boardobj_set super;
	u8 gpc_tsosc_idx;
};

struct nv_pmu_therm_therm_device_gpu_sci_boardobj_set {
	struct nv_pmu_therm_therm_device_boardobj_set super;
};

struct nv_pmu_therm_therm_device_i2c_boardobj_set {
	struct nv_pmu_therm_therm_device_boardobj_set super;
	u8 i2c_dev_idx;
};

struct nv_pmu_therm_therm_device_hbm2_site_boardobj_set {
	struct nv_pmu_therm_therm_device_boardobj_set super;
	u8 site_idx;
};

struct nv_pmu_therm_therm_device_hbm2_combined_boardobj_set {
	struct nv_pmu_therm_therm_device_boardobj_set super;
};

union nv_pmu_therm_therm_device_boardobj_set_union {
	struct nv_pmu_boardobj obj;
	struct nv_pmu_therm_therm_device_boardobj_set device;
	struct nv_pmu_therm_therm_device_gpu_gpc_tsosc_boardobj_set
							gpu_gpc_tsosc;
	struct nv_pmu_therm_therm_device_gpu_sci_boardobj_set gpu_sci;
	struct nv_pmu_therm_therm_device_i2c_boardobj_set i2c;
	struct nv_pmu_therm_therm_device_hbm2_site_boardobj_set hbm2_site;
	struct nv_pmu_therm_therm_device_hbm2_combined_boardobj_set
							hbm2_combined;
};

NV_PMU_BOARDOBJ_GRP_SET_MAKE_E32(therm, therm_device);

struct nv_pmu_therm_therm_channel_boardobjgrp_set_header {
	struct nv_pmu_boardobjgrp_e32 super;
};

struct nv_pmu_therm_therm_channel_boardobj_set {
	struct nv_pmu_boardobj super;
	s16 scaling;
	s16 offset;
	s32 temp_min;
	s32 temp_max;
};

struct nv_pmu_therm_therm_channel_device_boardobj_set {
	struct nv_pmu_therm_therm_channel_boardobj_set super;
	u8 therm_dev_idx;
	u8 therm_dev_prov_idx;
};

union nv_pmu_therm_therm_channel_boardobj_set_union {
	struct nv_pmu_boardobj obj;
	struct nv_pmu_therm_therm_channel_boardobj_set channel;
	struct nv_pmu_therm_therm_channel_device_boardobj_set device;
};

NV_PMU_BOARDOBJ_GRP_SET_MAKE_E32(therm, therm_channel);

struct nv_pmu_therm_therm_channel_boardobjgrp_get_status_header {
	struct nv_pmu_boardobjgrp_e32 super;
};

struct nv_pmu_therm_therm_channel_boardobj_get_status
{
	struct nv_pmu_boardobj_query super;
	u32 current_temp;
};

union nv_pmu_therm_therm_channel_boardobj_get_status_union
{
	struct nv_pmu_boardobj_query obj;
	struct nv_pmu_therm_therm_channel_boardobj_get_status therm_channel;
};

NV_PMU_BOARDOBJ_GRP_GET_STATUS_MAKE_E32(therm, therm_channel);

#endif /* NVGPU_PMU_THERM_INF_H */
