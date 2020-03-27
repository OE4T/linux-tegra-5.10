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

int gm20b_device_info_parse_data(struct gk20a *g, u32 table_entry, u32 *inst_id,
						u32 *pri_base, u32 *fault_id)
{
	if (top_device_info_entry_v(table_entry) !=
					top_device_info_entry_data_v()) {
		nvgpu_err(g, "Invalid device_info_data %u",
				top_device_info_entry_v(table_entry));
		return -EINVAL;
	}

	if (top_device_info_data_type_v(table_entry) !=
					top_device_info_data_type_enum2_v()) {
		nvgpu_err(g, "Unknown device_info_data_type %u",
				top_device_info_data_type_v(table_entry));
		return -EINVAL;
	}

	nvgpu_log_info(g, "Entry_data to be parsed 0x%x", table_entry);

	*pri_base = (top_device_info_data_pri_base_v(table_entry) <<
				top_device_info_data_pri_base_align_v());
	nvgpu_log_info(g, "Pri Base addr: 0x%x", *pri_base);

	if (top_device_info_data_fault_id_v(table_entry) ==
				top_device_info_data_fault_id_valid_v()) {
		*fault_id = top_device_info_data_fault_id_enum_v(table_entry);
	} else {
		*fault_id = U32_MAX;
	}
	nvgpu_log_info(g, "Fault_id: %u", *fault_id);

	/* In Maxwell days, instance id was not relevant as each instance of
	 * an engine would be assigned new engine_type.
	 */
	*inst_id = 0;
	nvgpu_log_info(g, "Inst_id: %u", *inst_id);

	return 0;
}

int gm20b_get_device_info(struct gk20a *g, struct nvgpu_device_info *dev_info,
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

	for (i = 0; i < max_info_entries; i++) {
		table_entry = nvgpu_readl(g, top_device_info_r(i));
		entry = top_device_info_entry_v(table_entry);

		if (entry == top_device_info_entry_not_valid_v()) {
			continue;
		} else if (entry == top_device_info_entry_enum_v()) {
			entry_enum = table_entry;
		} else if (entry == top_device_info_entry_data_v()) {
			entry_data = table_entry;
		} else if (entry == top_device_info_entry_engine_type_v()) {
			entry_engine = table_entry;
		} else {
			nvgpu_err(g, "Invalid entry type in device_info table");
			return -EINVAL;
		}

		if (top_device_info_chain_v(table_entry) ==
					top_device_info_chain_enable_v()) {
			continue;
		}

		if (top_device_info_type_enum_v(entry_engine) == engine_type) {
			dev_info->engine_type = engine_type;
			if (g->ops.top.device_info_parse_enum != NULL) {
				g->ops.top.device_info_parse_enum(g,
							entry_enum,
							&dev_info->engine_id,
							&dev_info->runlist_id,
							&dev_info->intr_id,
							&dev_info->reset_id);
			}
			if (g->ops.top.device_info_parse_data != NULL) {
				ret = g->ops.top.device_info_parse_data(g,
							entry_data,
							&dev_info->inst_id,
							&dev_info->pri_base,
							&dev_info->fault_id);
				if (ret != 0) {
					nvgpu_err(g,
						"Error parsing Data Entry 0x%x",
						entry_data);
					return ret;
				}
			}
		}
	}
	return ret;
}

bool gm20b_is_engine_ce(struct gk20a *g, u32 engine_type)
{
	return ((engine_type >= top_device_info_type_enum_copy0_v()) &&
			(engine_type <= top_device_info_type_enum_copy2_v()));
}
u32 gm20b_get_ce_inst_id(struct gk20a *g, u32 engine_type)
{
	/* inst_id starts from CE0 to CE2 */
	return (engine_type - NVGPU_ENGINE_COPY0);
}
