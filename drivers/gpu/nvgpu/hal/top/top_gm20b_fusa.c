/*
 * Copyright (c) 2018-2020, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/device.h>
#include <nvgpu/types.h>
#include <nvgpu/io.h>
#include <nvgpu/gk20a.h>
#include "top_gm20b.h"

#include <nvgpu/hw/gm20b/hw_top_gm20b.h>

void gm20b_device_info_parse_enum(struct gk20a *g, u32 table_entry,
				u32 *engine_id, u32 *runlist_id,
				u32 *intr_id, u32 *reset_id)
{
	nvgpu_log_info(g, "Entry_enum to be parsed 0x%x", table_entry);

	if (top_device_info_engine_v(table_entry) ==
					top_device_info_engine_valid_v()) {
		*engine_id = top_device_info_engine_enum_v(table_entry);
	} else {
		*engine_id = U32_MAX;
	}
	nvgpu_log_info(g, "Engine_id: %u", *engine_id);

	if (top_device_info_runlist_v(table_entry) ==
					top_device_info_runlist_valid_v()) {
		*runlist_id = top_device_info_runlist_enum_v(table_entry);
	} else {
		*runlist_id = U32_MAX;
	}
	nvgpu_log_info(g, "Runlist_id: %u", *runlist_id);

	if (top_device_info_intr_v(table_entry) ==
					top_device_info_intr_valid_v()) {
		*intr_id = top_device_info_intr_enum_v(table_entry);
	} else {
		*intr_id = U32_MAX;
	}
	nvgpu_log_info(g, "Intr_id: %u", *intr_id);

	if (top_device_info_reset_v(table_entry) ==
					top_device_info_reset_valid_v()) {
		*reset_id = top_device_info_reset_enum_v(table_entry);
	} else {
		*reset_id = U32_MAX;
	}
	nvgpu_log_info(g, "Reset_id: %u", *reset_id);

}

bool gm20b_is_engine_gr(struct gk20a *g, u32 engine_type)
{
	return (engine_type == top_device_info_type_enum_graphics_v());
}

u32 gm20b_top_get_max_gpc_count(struct gk20a *g)
{
	u32 tmp;

	tmp = nvgpu_readl(g, top_num_gpcs_r());
	return top_num_gpcs_value_v(tmp);
}

u32 gm20b_top_get_max_tpc_per_gpc_count(struct gk20a *g)
{
	u32 tmp;

	tmp = nvgpu_readl(g, top_tpc_per_gpc_r());
	return top_tpc_per_gpc_value_v(tmp);
}

u32 gm20b_top_get_max_fbps_count(struct gk20a *g)
{
	u32 tmp;

	tmp = nvgpu_readl(g, top_num_fbps_r());
	return top_num_fbps_value_v(tmp);
}

u32 gm20b_top_get_max_ltc_per_fbp(struct gk20a *g)
{
	u32 tmp;

	tmp = nvgpu_readl(g,  top_ltc_per_fbp_r());
	return top_ltc_per_fbp_value_v(tmp);
}

u32 gm20b_top_get_max_lts_per_ltc(struct gk20a *g)
{
	u32 tmp;

	tmp = nvgpu_readl(g,  top_slices_per_ltc_r());
	return top_slices_per_ltc_value_v(tmp);
}

u32 gm20b_top_get_num_ltcs(struct gk20a *g)
{
	u32 tmp;

	tmp = nvgpu_readl(g, top_num_ltcs_r());
	return top_num_ltcs_value_v(tmp);
}
