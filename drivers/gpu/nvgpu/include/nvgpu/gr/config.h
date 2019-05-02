/*
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

#ifndef NVGPU_GR_CONFIG_H
#define NVGPU_GR_CONFIG_H

#include <nvgpu/types.h>

struct gk20a;
struct nvgpu_sm_info;
struct nvgpu_gr_config;

struct nvgpu_gr_config *nvgpu_gr_config_init(struct gk20a *g);
void nvgpu_gr_config_deinit(struct gk20a *g, struct nvgpu_gr_config *config);
int nvgpu_gr_config_init_map_tiles(struct gk20a *g,
	struct nvgpu_gr_config *config);

u32 nvgpu_gr_config_get_map_tile_count(struct nvgpu_gr_config *config,
	u32 index);
u8 *nvgpu_gr_config_get_map_tiles(struct nvgpu_gr_config *config);
u32 nvgpu_gr_config_get_map_row_offset(struct nvgpu_gr_config *config);

u32 nvgpu_gr_config_get_max_gpc_count(struct nvgpu_gr_config *config);
u32 nvgpu_gr_config_get_max_tpc_per_gpc_count(struct nvgpu_gr_config *config);
u32 nvgpu_gr_config_get_max_zcull_per_gpc_count(struct nvgpu_gr_config *config);
u32 nvgpu_gr_config_get_max_tpc_count(struct nvgpu_gr_config *config);

u32 nvgpu_gr_config_get_gpc_count(struct nvgpu_gr_config *config);
u32 nvgpu_gr_config_get_tpc_count(struct nvgpu_gr_config *config);
u32 nvgpu_gr_config_get_ppc_count(struct nvgpu_gr_config *config);
u32 nvgpu_gr_config_get_zcb_count(struct nvgpu_gr_config *config);

u32 nvgpu_gr_config_get_pe_count_per_gpc(struct nvgpu_gr_config *config);
u32 nvgpu_gr_config_get_sm_count_per_tpc(struct nvgpu_gr_config *config);

u32 nvgpu_gr_config_get_gpc_ppc_count(struct nvgpu_gr_config *config,
	u32 gpc_index);
u32 *nvgpu_gr_config_get_gpc_tpc_count_base(struct nvgpu_gr_config *config);
u32 nvgpu_gr_config_get_gpc_tpc_count(struct nvgpu_gr_config *config,
	u32 gpc_index);
u32 nvgpu_gr_config_get_gpc_zcb_count(struct nvgpu_gr_config *config,
	u32 gpc_index);
u32 nvgpu_gr_config_get_pes_tpc_count(struct nvgpu_gr_config *config,
	u32 gpc_index, u32 pes_index);

u32 *nvgpu_gr_config_get_gpc_tpc_mask_base(struct nvgpu_gr_config *config);
u32 nvgpu_gr_config_get_gpc_tpc_mask(struct nvgpu_gr_config *config,
	u32 gpc_index);
void nvgpu_gr_config_set_gpc_tpc_mask(struct nvgpu_gr_config *config,
	u32 gpc_index, u32 val);
u32 nvgpu_gr_config_get_gpc_skip_mask(struct nvgpu_gr_config *config,
	u32 gpc_index);
u32 nvgpu_gr_config_get_pes_tpc_mask(struct nvgpu_gr_config *config,
	u32 gpc_index, u32 pes_index);
u32 nvgpu_gr_config_get_gpc_mask(struct nvgpu_gr_config *config);
u32 nvgpu_gr_config_get_no_of_sm(struct nvgpu_gr_config *config);
void nvgpu_gr_config_set_no_of_sm(struct nvgpu_gr_config *config, u32 no_of_sm);
struct nvgpu_sm_info *nvgpu_gr_config_get_sm_info(struct nvgpu_gr_config *config,
	u32 sm_id);
u32 nvgpu_gr_config_get_sm_info_gpc_index(struct nvgpu_sm_info *sm_info);
void nvgpu_gr_config_set_sm_info_gpc_index(struct nvgpu_sm_info *sm_info,
	u32 gpc_index);
u32 nvgpu_gr_config_get_sm_info_tpc_index(struct nvgpu_sm_info *sm_info);
void nvgpu_gr_config_set_sm_info_tpc_index(struct nvgpu_sm_info *sm_info,
	u32 tpc_index);
u32 nvgpu_gr_config_get_sm_info_global_tpc_index(struct nvgpu_sm_info *sm_info);
void nvgpu_gr_config_set_sm_info_global_tpc_index(struct nvgpu_sm_info *sm_info,
	u32 global_tpc_index);
u32 nvgpu_gr_config_get_sm_info_sm_index(struct nvgpu_sm_info *sm_info);
void nvgpu_gr_config_set_sm_info_sm_index(struct nvgpu_sm_info *sm_info,
	u32 sm_index);

#endif /* NVGPU_GR_CONFIG_H */
