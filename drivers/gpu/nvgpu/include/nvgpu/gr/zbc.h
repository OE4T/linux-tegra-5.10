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

#ifndef NVGPU_GR_ZBC_H
#define NVGPU_GR_ZBC_H

#include <nvgpu/types.h>


#define NVGPU_GR_ZBC_COLOR_VALUE_SIZE	4U  /* RGBA */

/* index zero reserved to indicate "not ZBCd" */
#define NVGPU_GR_ZBC_STARTOF_TABLE	1U
/* match ltcs_ltss_dstg_zbc_index_address width (4) */
#define NVGPU_GR_ZBC_SIZEOF_TABLE	16U
#define NVGPU_GR_ZBC_TABLE_SIZE		(16U - 1U)

#define NVGPU_GR_ZBC_TYPE_INVALID	0U
#define NVGPU_GR_ZBC_TYPE_COLOR		1U
#define NVGPU_GR_ZBC_TYPE_DEPTH		2U
#define NVGPU_GR_ZBC_TYPE_STENCIL	3U

struct gk20a;
struct zbc_color_table;
struct zbc_depth_table;
struct zbc_s_table;

struct nvgpu_gr_zbc_entry {
	u32 color_ds[NVGPU_GR_ZBC_COLOR_VALUE_SIZE];
	u32 color_l2[NVGPU_GR_ZBC_COLOR_VALUE_SIZE];
	u32 depth;
	u32 type;	/* color or depth */
	u32 format;
};

struct nvgpu_gr_zbc_query_params {
	u32 color_ds[NVGPU_GR_ZBC_COLOR_VALUE_SIZE];
	u32 color_l2[NVGPU_GR_ZBC_COLOR_VALUE_SIZE];
	u32 depth;
	u32 ref_cnt;
	u32 format;
	u32 type;	/* color or depth */
	u32 index_size;	/* [out] size, [in] index */
};

struct nvgpu_gr_zbc {
	struct nvgpu_mutex zbc_lock;
	struct zbc_color_table *zbc_col_tbl;
	struct zbc_depth_table *zbc_dep_tbl;
	struct zbc_s_table *zbc_s_tbl;
	s32 max_default_color_index;
	s32 max_default_depth_index;
	s32 max_default_s_index;
	u32 max_used_color_index;
	u32 max_used_depth_index;
	u32 max_used_s_index;
};

int nvgpu_gr_zbc_init(struct gk20a *g, struct nvgpu_gr_zbc **zbc);
void nvgpu_gr_zbc_deinit(struct gk20a *g, struct nvgpu_gr_zbc *zbc);
int nvgpu_gr_zbc_load_table(struct gk20a *g, struct nvgpu_gr_zbc *zbc);
int nvgpu_gr_zbc_add_depth(struct gk20a *g, struct nvgpu_gr_zbc *zbc,
			   struct nvgpu_gr_zbc_entry *depth_val, u32 index);
int nvgpu_gr_zbc_add_color(struct gk20a *g, struct nvgpu_gr_zbc *zbc,
			   struct nvgpu_gr_zbc_entry *color_val, u32 index);
int nvgpu_gr_zbc_query_table(struct gk20a *g, struct nvgpu_gr_zbc *zbc,
			     struct nvgpu_gr_zbc_query_params *query_params);
int nvgpu_gr_zbc_set_table(struct gk20a *g, struct nvgpu_gr_zbc *zbc,
			   struct nvgpu_gr_zbc_entry *zbc_val);
int nvgpu_gr_zbc_stencil_query_table(struct gk20a *g, struct nvgpu_gr_zbc *zbc,
				struct nvgpu_gr_zbc_query_params *query_params);
bool nvgpu_gr_zbc_add_type_stencil(struct gk20a *g, struct nvgpu_gr_zbc *zbc,
				   struct nvgpu_gr_zbc_entry *zbc_val,
				   int *ret_val);
int nvgpu_gr_zbc_load_stencil_default_tbl(struct gk20a *g,
					  struct nvgpu_gr_zbc *zbc);
int nvgpu_gr_zbc_load_stencil_tbl(struct gk20a *g, struct nvgpu_gr_zbc *zbc);
#endif /* NVGPU_GR_ZBC_H */
