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

#ifndef NVGPU_PMU_VOLT_H
#define NVGPU_PMU_VOLT_H

#include <nvgpu/types.h>
#include <nvgpu/boardobjgrp_e32.h>

struct gk20a;

/*!
 * metadata of voltage rail functionality.
 */
struct voltage_rail_metadata {
	u8 volt_domain_hal;
	u8 pct_delta;
	u32 ext_rel_delta_uv[CTRL_VOLT_RAIL_VOLT_DELTA_MAX_ENTRIES];
	u8 logic_rail_idx;
	u8 sram_rail_idx;
	struct boardobjgrp_e32 volt_rails;
};

struct voltage_device_metadata {
	struct boardobjgrp_e32 volt_devices;
};

struct voltage_policy_metadata {
	u8 perf_core_vf_seq_policy_idx;
	struct boardobjgrp_e32 volt_policies;
};

struct obj_volt {
	struct voltage_rail_metadata volt_rail_metadata;
	struct voltage_device_metadata volt_dev_metadata;
	struct voltage_policy_metadata volt_policy_metadata;
};

int nvgpu_volt_set_voltage_gp10x(struct gk20a *g, u32 logic_voltage_uv,
	u32 sram_voltage_uv);
int nvgpu_volt_rail_get_voltage_gp10x(struct gk20a *g,
	u8 volt_domain, u32 *pvoltage_uv);
int nvgpu_volt_send_load_cmd_to_pmu_gp10x(struct gk20a *g);

int nvgpu_volt_set_voltage_gv10x(struct gk20a *g, u32 logic_voltage_uv,
	u32 sram_voltage_uv);
int volt_set_voltage(struct gk20a *g, u32 logic_voltage_uv,
		u32 sram_voltage_uv);
int nvgpu_volt_rail_get_voltage_gv10x(struct gk20a *g,
	u8 volt_domain, u32 *pvoltage_uv);
int nvgpu_volt_send_load_cmd_to_pmu_gv10x(struct gk20a *g);

int volt_get_voltage(struct gk20a *g, u32 volt_domain, u32 *voltage_uv);

int volt_dev_sw_setup(struct gk20a *g);
int volt_dev_pmu_setup(struct gk20a *g);

int volt_rail_sw_setup(struct gk20a *g);
int volt_rail_pmu_setup(struct gk20a *g);
u8 volt_rail_volt_domain_convert_to_idx(struct gk20a *g, u8 volt_domain);

int volt_policy_sw_setup(struct gk20a *g);
int volt_policy_pmu_setup(struct gk20a *g);

#endif /* NVGPU_PMU_VOLT_H */
