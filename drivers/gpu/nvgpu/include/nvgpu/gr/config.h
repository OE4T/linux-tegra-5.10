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

#define GK20A_GR_MAX_PES_PER_GPC 3U

struct gk20a;

struct nvgpu_gr_config {
	u32 max_gpc_count;
	u32 max_tpc_per_gpc_count;
	u32 max_zcull_per_gpc_count;
	u32 max_tpc_count;

	u32 gpc_count;
	u32 tpc_count;
	u32 ppc_count;
	u32 zcb_count;

	u32 pe_count_per_gpc;

	u32 *gpc_ppc_count;
	u32 *gpc_tpc_count;
	u32 *gpc_zcb_count;
	u32 *pes_tpc_count[GK20A_GR_MAX_PES_PER_GPC];

	u32 gpc_mask;
	u32 *gpc_tpc_mask;
	u32 *pes_tpc_mask[GK20A_GR_MAX_PES_PER_GPC];
	u32 *gpc_skip_mask;

	u8 *map_tiles;
	u32 map_tile_count;
	u32 map_row_offset;

	u32 no_of_sm;
};

struct nvgpu_gr_config *nvgpu_gr_config_init(struct gk20a *g);
void nvgpu_gr_config_deinit(struct gk20a *g, struct nvgpu_gr_config *config);
int nvgpu_gr_config_init_map_tiles(struct gk20a *g,
	struct nvgpu_gr_config *config);

u32 nvgpu_gr_config_get_map_tile_count(struct nvgpu_gr_config *config,
	u32 index);
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

u32 nvgpu_gr_config_get_gpc_ppc_count(struct nvgpu_gr_config *config,
	u32 gpc_index);
u32 nvgpu_gr_config_get_gpc_tpc_count(struct nvgpu_gr_config *config,
	u32 gpc_index);
u32 nvgpu_gr_config_get_gpc_zcb_count(struct nvgpu_gr_config *config,
	u32 gpc_index);
u32 nvgpu_gr_config_get_pes_tpc_count(struct nvgpu_gr_config *config,
	u32 gpc_index, u32 pes_index);

u32 nvgpu_gr_config_get_gpc_tpc_mask(struct nvgpu_gr_config *config,
	u32 gpc_index);
u32 nvgpu_gr_config_get_gpc_skip_mask(struct nvgpu_gr_config *config,
	u32 gpc_index);
u32 nvgpu_gr_config_get_pes_tpc_mask(struct nvgpu_gr_config *config,
	u32 gpc_index, u32 pes_index);
u32 nvgpu_gr_config_get_gpc_mask(struct nvgpu_gr_config *config);
u32 nvgpu_gr_config_get_no_of_sm(struct nvgpu_gr_config *config);

#endif /* NVGPU_GR_CONFIG_H */
