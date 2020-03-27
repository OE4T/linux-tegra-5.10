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
#include "top_gp10b.h"

#include <nvgpu/hw/gp10b/hw_top_gp10b.h>

u32 gp10b_get_num_engine_type_entries(struct gk20a *g, u32 engine_type)
{
	u32 i = 0;
	u32 max_info_entries = top_device_info__size_1_v();
	u32 num_entries = 0;
	u32 table_entry;
	u32 entry;

	for (i = 0; i < max_info_entries; i++) {
		table_entry = nvgpu_readl(g, top_device_info_r(i));
		entry = top_device_info_entry_v(table_entry);

		if (entry == top_device_info_entry_engine_type_v()) {
			nvgpu_log_info(g, "table_entry: 0x%x engine type: 0x%x",
				table_entry,
				top_device_info_type_enum_v(table_entry));
			if (top_device_info_type_enum_v(table_entry) ==
								engine_type) {
				num_entries = nvgpu_safe_add_u32(num_entries,
								 1U);
			}
		}
	}
	return num_entries;
}

static int gp10b_check_device_match(struct gk20a *g,
				    struct nvgpu_device_info *dev_info,
				    u32 entry_engine, u32 engine_type,
				    u32 entry_data, u32 inst_id, u32 entry_enum)
{
	int ret;

	if ((top_device_info_type_enum_v(entry_engine) == engine_type)
		&& (top_device_info_data_inst_id_v(entry_data) ==
							inst_id)) {
		dev_info->engine_type = engine_type;
		g->ops.top.device_info_parse_enum(g,
						entry_enum,
						&dev_info->engine_id,
						&dev_info->runlist_id,
						&dev_info->intr_id,
						&dev_info->reset_id);
		ret = g->ops.top.device_info_parse_data(g,
						entry_data,
						&dev_info->inst_id,
						&dev_info->pri_base,
						&dev_info->fault_id);
		if (ret != 0) {
			nvgpu_err(g, "Error parsing Data Entry 0x%x",
								entry_data);
			return ret;
		}
	}

	return 0;
}

int gp10b_get_device_info(struct gk20a *g, struct nvgpu_device_info *dev_info,
						u32 engine_type, u32 inst_id)
{
	int ret = 0;
	u32 i = 0;
	u32 table_entry;
	u32 entry;
	u32 entry_engine = 0;
	u32 entry_enum = 0;
	u32 entry_data = 0;
	u32 max_info_entries = top_device_info__size_1_v();

	if (dev_info == NULL) {
		nvgpu_err(g, "Null device_info pointer passed.");
		return -EINVAL;
	}

	if ((g->ops.top.device_info_parse_enum == NULL) ||
				(g->ops.top.device_info_parse_data == NULL)) {
		nvgpu_err(g, "Dev_info parsing functions ptrs not set.");
		return -EINVAL;
	}

	for (i = 0; i < max_info_entries; i++) {
		table_entry = nvgpu_readl(g, top_device_info_r(i));
		entry = top_device_info_entry_v(table_entry);

		if (entry == top_device_info_entry_not_valid_v()) {
			continue;
		} else if (entry == top_device_info_entry_enum_v()) {
			entry_enum = table_entry;
		} else if (entry == top_device_info_entry_data_v()) {
			entry_data = table_entry;
		} else {
			entry_engine = table_entry;
		}

		if (top_device_info_chain_v(table_entry) ==
					top_device_info_chain_enable_v()) {
			continue;
		}

		ret = gp10b_check_device_match(g, dev_info, entry_engine,
				engine_type, entry_data, inst_id, entry_enum);
		if (ret != 0) {
			return ret;
		}
	}
	return ret;
}

bool gp10b_is_engine_ce(struct gk20a *g, u32 engine_type)
{
	return (engine_type == top_device_info_type_enum_lce_v());
}
