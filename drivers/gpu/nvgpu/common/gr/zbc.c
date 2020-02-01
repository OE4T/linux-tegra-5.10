/*
 * Copyright (c) 2019-2020, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/gk20a.h>
#include <nvgpu/io.h>
#include <nvgpu/bug.h>
#include <nvgpu/string.h>
#include <nvgpu/power_features/pg.h>
#ifdef CONFIG_NVGPU_LS_PMU
#include <nvgpu/pmu/pmu_pg.h>
#endif

#include "zbc_priv.h"

static int nvgpu_gr_zbc_add(struct gk20a *g, struct nvgpu_gr_zbc *zbc,
			    struct nvgpu_gr_zbc_entry *zbc_val)
{
	struct zbc_color_table *c_tbl;
	struct zbc_depth_table *d_tbl;
	u32 i;
	int ret = -ENOSPC;
	bool added = false;
#ifdef CONFIG_NVGPU_LS_PMU
	u32 entries;
#endif

	/* no endian swap ? */

	nvgpu_mutex_acquire(&zbc->zbc_lock);
	nvgpu_speculation_barrier();
	switch (zbc_val->type) {
	case NVGPU_GR_ZBC_TYPE_COLOR:
		/* search existing tables */
		for (i = 0; i < zbc->max_used_color_index; i++) {

			c_tbl = &zbc->zbc_col_tbl[i];

			if ((c_tbl->ref_cnt != 0U) &&
				(c_tbl->format == zbc_val->format) &&
				(nvgpu_memcmp((u8 *)c_tbl->color_ds,
					(u8 *)zbc_val->color_ds,
					sizeof(zbc_val->color_ds)) == 0) &&
				(nvgpu_memcmp((u8 *)c_tbl->color_l2,
					(u8 *)zbc_val->color_l2,
					sizeof(zbc_val->color_l2)) == 0)) {

				added = true;
				c_tbl->ref_cnt++;
				ret = 0;
				break;
			}
		}
		/* add new table */
		if (!added &&
			zbc->max_used_color_index <
					g->ops.ltc.zbc_table_size(g)) {

			c_tbl =
			    &zbc->zbc_col_tbl[zbc->max_used_color_index];
			WARN_ON(c_tbl->ref_cnt != 0U);

			ret = nvgpu_gr_zbc_add_color(g, zbc,
				zbc_val, zbc->max_used_color_index);

			if (ret == 0) {
				zbc->max_used_color_index++;
			}
		}
		break;
	case NVGPU_GR_ZBC_TYPE_DEPTH:
		/* search existing tables */
		for (i = 0; i < zbc->max_used_depth_index; i++) {

			d_tbl = &zbc->zbc_dep_tbl[i];

			if ((d_tbl->ref_cnt != 0U) &&
			    (d_tbl->depth == zbc_val->depth) &&
			    (d_tbl->format == zbc_val->format)) {
				added = true;
				d_tbl->ref_cnt++;
				ret = 0;
				break;
			}
		}
		/* add new table */
		if (!added &&
			zbc->max_used_depth_index <
					g->ops.ltc.zbc_table_size(g)) {

			d_tbl =
			    &zbc->zbc_dep_tbl[zbc->max_used_depth_index];
			WARN_ON(d_tbl->ref_cnt != 0U);

			ret = nvgpu_gr_zbc_add_depth(g, zbc,
				zbc_val, zbc->max_used_depth_index);

			if (ret == 0) {
				zbc->max_used_depth_index++;
			}
		}
		break;
	case NVGPU_GR_ZBC_TYPE_STENCIL:
		if (nvgpu_is_enabled(g, NVGPU_SUPPORT_ZBC_STENCIL)) {
			added =  nvgpu_gr_zbc_add_type_stencil(g, zbc,
								zbc_val, &ret);
		} else {
			nvgpu_err(g,
			"invalid zbc table type %d", zbc_val->type);
			ret = -EINVAL;
			goto err_mutex;
		}
		break;
	default:
		nvgpu_err(g,
			"invalid zbc table type %d", zbc_val->type);
		ret = -EINVAL;
		goto err_mutex;
	}

#ifdef CONFIG_NVGPU_LS_PMU
	if (!added && ret == 0) {
		/* update zbc for elpg only when new entry is added */
		entries = max(zbc->max_used_color_index,
					zbc->max_used_depth_index);
		if (g->elpg_enabled) {
			nvgpu_pmu_save_zbc(g, entries);
		}
	}
#endif

err_mutex:
	nvgpu_mutex_release(&zbc->zbc_lock);
	return ret;
}

int nvgpu_gr_zbc_add_depth(struct gk20a *g, struct nvgpu_gr_zbc *zbc,
			   struct nvgpu_gr_zbc_entry *depth_val, u32 index)
{
	/* update l2 table */
	g->ops.ltc.set_zbc_depth_entry(g, depth_val->depth, index);

	/* update local copy */
	zbc->zbc_dep_tbl[index].depth = depth_val->depth;
	zbc->zbc_dep_tbl[index].format = depth_val->format;
	zbc->zbc_dep_tbl[index].ref_cnt++;

	/* update zbc registers */
	g->ops.gr.zbc.add_depth(g, depth_val, index);

	return 0;
}

int nvgpu_gr_zbc_add_color(struct gk20a *g, struct nvgpu_gr_zbc *zbc,
			   struct nvgpu_gr_zbc_entry *color_val, u32 index)
{
	u32 i;

	/* update l2 table */
	g->ops.ltc.set_zbc_color_entry(g, color_val->color_l2, index);

	/* update local copy */
	for (i = 0; i < NVGPU_GR_ZBC_COLOR_VALUE_SIZE; i++) {
		zbc->zbc_col_tbl[index].color_l2[i] = color_val->color_l2[i];
		zbc->zbc_col_tbl[index].color_ds[i] = color_val->color_ds[i];
	}
	zbc->zbc_col_tbl[index].format = color_val->format;
	zbc->zbc_col_tbl[index].ref_cnt++;

	/* update zbc registers */
	g->ops.gr.zbc.add_color(g, color_val, index);

	return 0;

}

static int nvgpu_gr_zbc_load_default_table(struct gk20a *g,
					   struct nvgpu_gr_zbc *zbc)
{
	struct nvgpu_gr_zbc_entry zbc_val;
	u32 i = 0;
	int err = 0;

	nvgpu_mutex_init(&zbc->zbc_lock);

	/* load default color table */
	zbc_val.type = NVGPU_GR_ZBC_TYPE_COLOR;

	/* Opaque black (i.e. solid black, fmt 0x28 = A8B8G8R8) */
	zbc_val.format = GR_ZBC_SOLID_BLACK_COLOR_FMT;
	for (i = 0; i < NVGPU_GR_ZBC_COLOR_VALUE_SIZE; i++) {
		zbc_val.color_ds[i] = 0U;
		zbc_val.color_l2[i] = 0U;
	}
	zbc_val.color_l2[0] = 0xff000000U;
	zbc_val.color_ds[3] = 0x3f800000U;
	err = nvgpu_gr_zbc_add(g, zbc, &zbc_val);
	if (err != 0) {
		goto color_fail;
	}

	/* Transparent black = (fmt 1 = zero) */
	zbc_val.format = GR_ZBC_TRANSPARENT_BLACK_COLOR_FMT;
	for (i = 0; i < NVGPU_GR_ZBC_COLOR_VALUE_SIZE; i++) {
		zbc_val.color_ds[i] = 0U;
		zbc_val.color_l2[i] = 0U;
	}
	err = nvgpu_gr_zbc_add(g, zbc, &zbc_val);
	if (err != 0) {
		goto color_fail;
	}

	/* Opaque white (i.e. solid white) = (fmt 2 = uniform 1) */
	zbc_val.format = GR_ZBC_SOLID_WHITE_COLOR_FMT;
	for (i = 0; i < NVGPU_GR_ZBC_COLOR_VALUE_SIZE; i++) {
		zbc_val.color_ds[i] = 0x3f800000U;
		zbc_val.color_l2[i] = 0xffffffffU;
	}
	err = nvgpu_gr_zbc_add(g, zbc, &zbc_val);
	if (err != 0) {
		goto color_fail;
	}

	zbc->max_default_color_index = 3;

	/* load default depth table */
	zbc_val.type = NVGPU_GR_ZBC_TYPE_DEPTH;

	zbc_val.format = GR_ZBC_Z_FMT_VAL_FP32;
	zbc_val.depth = 0x3f800000;
	err = nvgpu_gr_zbc_add(g, zbc, &zbc_val);
	if (err != 0) {
		goto depth_fail;
	}

	zbc_val.format = GR_ZBC_Z_FMT_VAL_FP32;
	zbc_val.depth = 0;
	err = nvgpu_gr_zbc_add(g, zbc, &zbc_val);
	if (err != 0) {
		goto depth_fail;
	}

	zbc->max_default_depth_index = 2;

	if (nvgpu_is_enabled(g, NVGPU_SUPPORT_ZBC_STENCIL)) {
		err =  nvgpu_gr_zbc_load_stencil_default_tbl(g, zbc);
		if (err != 0) {
			return err;
		}
	}

	return 0;

color_fail:
	nvgpu_err(g, "fail to load default zbc color table");
	return err;
depth_fail:
	nvgpu_err(g, "fail to load default zbc depth table");
	return err;
}

int nvgpu_gr_zbc_load_table(struct gk20a *g, struct nvgpu_gr_zbc *zbc)
{
	unsigned int i;
	int ret;

	for (i = 0; i < zbc->max_used_color_index; i++) {
		struct zbc_color_table *c_tbl = &zbc->zbc_col_tbl[i];
		struct nvgpu_gr_zbc_entry zbc_val;

		zbc_val.type = NVGPU_GR_ZBC_TYPE_COLOR;
		nvgpu_memcpy((u8 *)zbc_val.color_ds,
			(u8 *)c_tbl->color_ds, sizeof(zbc_val.color_ds));
		nvgpu_memcpy((u8 *)zbc_val.color_l2,
			(u8 *)c_tbl->color_l2, sizeof(zbc_val.color_l2));
		zbc_val.format = c_tbl->format;

		ret = nvgpu_gr_zbc_add_color(g, zbc, &zbc_val, i);

		if (ret != 0) {
			return ret;
		}
	}
	for (i = 0; i < zbc->max_used_depth_index; i++) {
		struct zbc_depth_table *d_tbl = &zbc->zbc_dep_tbl[i];
		struct nvgpu_gr_zbc_entry zbc_val;

		zbc_val.type = NVGPU_GR_ZBC_TYPE_DEPTH;
		zbc_val.depth = d_tbl->depth;
		zbc_val.format = d_tbl->format;

		ret = nvgpu_gr_zbc_add_depth(g, zbc, &zbc_val, i);
		if (ret != 0) {
			return ret;
		}
	}

	if (nvgpu_is_enabled(g, NVGPU_SUPPORT_ZBC_STENCIL)) {
		ret = nvgpu_gr_zbc_load_stencil_tbl(g, zbc);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

int nvgpu_gr_zbc_stencil_query_table(struct gk20a *g, struct nvgpu_gr_zbc *zbc,
				struct nvgpu_gr_zbc_query_params *query_params)
{
	u32 index = query_params->index_size;

	if (index >= g->ops.ltc.zbc_table_size(g)) {
		nvgpu_err(g, "invalid zbc stencil table index");
		return -EINVAL;
	}

	nvgpu_speculation_barrier();
	query_params->depth = zbc->zbc_s_tbl[index].stencil;
	query_params->format = zbc->zbc_s_tbl[index].format;
	query_params->ref_cnt = zbc->zbc_s_tbl[index].ref_cnt;

	return 0;
}

int nvgpu_gr_zbc_load_stencil_default_tbl(struct gk20a *g,
					  struct nvgpu_gr_zbc *zbc)
{
	struct nvgpu_gr_zbc_entry zbc_val;
	int err;

	/* load default stencil table */
	zbc_val.type = NVGPU_GR_ZBC_TYPE_STENCIL;

	zbc_val.depth = 0x0;
	zbc_val.format = GR_ZBC_STENCIL_CLEAR_FMT_U8;
	err = nvgpu_gr_zbc_add(g, zbc, &zbc_val);
	if (err != 0) {
		goto fail;
	}
	zbc_val.depth = 0x1;
	zbc_val.format = GR_ZBC_STENCIL_CLEAR_FMT_U8;
	err = nvgpu_gr_zbc_add(g, zbc, &zbc_val);
	if (err != 0) {
		goto fail;
	}

	zbc_val.depth = 0xff;
	zbc_val.format = GR_ZBC_STENCIL_CLEAR_FMT_U8;
	err = nvgpu_gr_zbc_add(g, zbc, &zbc_val);
	if (err != 0) {
		goto fail;
	}

	zbc->max_default_s_index = 3;

	return 0;

fail:
	nvgpu_err(g, "fail to load default zbc stencil table");
	return err;
}

static int gr_zbc_load_stencil_tbl(struct gk20a *g, struct nvgpu_gr_zbc *zbc,
			     struct nvgpu_gr_zbc_entry *stencil_val, u32 index)
{
	/* update l2 table */
	if (g->ops.ltc.set_zbc_s_entry != NULL) {
		g->ops.ltc.set_zbc_s_entry(g, stencil_val->depth, index);
	}

	/* update local copy */
	zbc->zbc_s_tbl[index].stencil = stencil_val->depth;
	zbc->zbc_s_tbl[index].format = stencil_val->format;
	zbc->zbc_s_tbl[index].ref_cnt++;

	/* update zbc stencil registers */
	return g->ops.gr.zbc.add_stencil(g, stencil_val, index);
}

int nvgpu_gr_zbc_load_stencil_tbl(struct gk20a *g, struct nvgpu_gr_zbc *zbc)
{
	int ret;
	u32 i;

	for (i = 0; i < zbc->max_used_s_index; i++) {
		struct zbc_s_table *s_tbl = &zbc->zbc_s_tbl[i];
		struct nvgpu_gr_zbc_entry zbc_val;

		zbc_val.type = NVGPU_GR_ZBC_TYPE_STENCIL;
		zbc_val.depth = s_tbl->stencil;
		zbc_val.format = s_tbl->format;

		ret = gr_zbc_load_stencil_tbl(g, zbc, &zbc_val, i);
		if (ret != 0) {
			return ret;
		}
	}
	return 0;
}

bool nvgpu_gr_zbc_add_type_stencil(struct gk20a *g, struct nvgpu_gr_zbc *zbc,
				   struct nvgpu_gr_zbc_entry *zbc_val,
				   int *ret_val)
{
	struct zbc_s_table *s_tbl;
	u32 i;
	bool added = false;

	*ret_val = -ENOMEM;

	/* search existing tables */
	for (i = 0; i < zbc->max_used_s_index; i++) {

		s_tbl = &zbc->zbc_s_tbl[i];

		if ((s_tbl->ref_cnt != 0U) &&
		    (s_tbl->stencil == zbc_val->depth) &&
		    (s_tbl->format == zbc_val->format)) {
			added = true;
			s_tbl->ref_cnt++;
			*ret_val = 0;
			break;
		}
	}
	/* add new table */
	if (!added &&
		zbc->max_used_s_index < g->ops.ltc.zbc_table_size(g)) {

		s_tbl = &zbc->zbc_s_tbl[zbc->max_used_s_index];
		WARN_ON(s_tbl->ref_cnt != 0U);

		*ret_val = gr_zbc_load_stencil_tbl(g, zbc,
				zbc_val, zbc->max_used_s_index);

		if ((*ret_val) == 0) {
			zbc->max_used_s_index++;
		}
	}
	return added;
}

int nvgpu_gr_zbc_set_table(struct gk20a *g, struct nvgpu_gr_zbc *zbc,
			   struct nvgpu_gr_zbc_entry *zbc_val)
{
	nvgpu_log_fn(g, " ");

	return nvgpu_pg_elpg_protected_call(g,
		nvgpu_gr_zbc_add(g, zbc, zbc_val));
}

/* get a zbc table entry specified by index
 * return table size when type is invalid */
int nvgpu_gr_zbc_query_table(struct gk20a *g, struct nvgpu_gr_zbc *zbc,
			struct nvgpu_gr_zbc_query_params *query_params)
{
	u32 index = query_params->index_size;
	u32 i;

	nvgpu_speculation_barrier();
	switch (query_params->type) {
	case NVGPU_GR_ZBC_TYPE_INVALID:
		query_params->index_size = g->ops.ltc.zbc_table_size(g);
		break;
	case NVGPU_GR_ZBC_TYPE_COLOR:
		if (index >= g->ops.ltc.zbc_table_size(g)) {
			nvgpu_err(g,
				"invalid zbc color table index");
			return -EINVAL;
		}

		nvgpu_speculation_barrier();
		for (i = 0; i < NVGPU_GR_ZBC_COLOR_VALUE_SIZE; i++) {
			query_params->color_l2[i] =
				zbc->zbc_col_tbl[index].color_l2[i];
			query_params->color_ds[i] =
				zbc->zbc_col_tbl[index].color_ds[i];
		}
		query_params->format = zbc->zbc_col_tbl[index].format;
		query_params->ref_cnt = zbc->zbc_col_tbl[index].ref_cnt;
		break;
	case NVGPU_GR_ZBC_TYPE_DEPTH:
		if (index >= g->ops.ltc.zbc_table_size(g)) {
			nvgpu_err(g,
				"invalid zbc depth table index");
			return -EINVAL;
		}

		nvgpu_speculation_barrier();
		query_params->depth = zbc->zbc_dep_tbl[index].depth;
		query_params->format = zbc->zbc_dep_tbl[index].format;
		query_params->ref_cnt = zbc->zbc_dep_tbl[index].ref_cnt;
		break;
	case NVGPU_GR_ZBC_TYPE_STENCIL:
		if (nvgpu_is_enabled(g, NVGPU_SUPPORT_ZBC_STENCIL)) {
			return nvgpu_gr_zbc_stencil_query_table(g, zbc,
					 query_params);
		} else {
			nvgpu_err(g,
				"invalid zbc table type");
			return -EINVAL;
		}
		break;
	default:
		nvgpu_err(g,
				"invalid zbc table type");
		return -EINVAL;
	}

	return 0;
}

static int gr_zbc_allocate_local_tbls(struct gk20a *g, struct nvgpu_gr_zbc *zbc)
{
	zbc->zbc_col_tbl = nvgpu_kzalloc(g,
			sizeof(struct zbc_color_table) *
			g->ops.ltc.zbc_table_size(g));
	if (zbc->zbc_col_tbl == NULL) {
		goto alloc_col_tbl_err;
	}

	zbc->zbc_dep_tbl = nvgpu_kzalloc(g,
			sizeof(struct zbc_depth_table) *
			g->ops.ltc.zbc_table_size(g));

	if (zbc->zbc_dep_tbl == NULL) {
		goto alloc_dep_tbl_err;
	}

	zbc->zbc_s_tbl = nvgpu_kzalloc(g,
			sizeof(struct zbc_s_table) *
			g->ops.ltc.zbc_table_size(g));
	if (zbc->zbc_s_tbl == NULL) {
		goto alloc_s_tbl_err;
	}

	return 0;

alloc_s_tbl_err:
	nvgpu_kfree(g, zbc->zbc_dep_tbl);
alloc_dep_tbl_err:
	nvgpu_kfree(g, zbc->zbc_col_tbl);
alloc_col_tbl_err:
	return -ENOMEM;
}

/* allocate the struct and load the table */
int nvgpu_gr_zbc_init(struct gk20a *g, struct nvgpu_gr_zbc **zbc)
{
	int ret = -ENOMEM;
	struct nvgpu_gr_zbc *gr_zbc = NULL;

	*zbc = NULL;

	gr_zbc = nvgpu_kzalloc(g, sizeof(*gr_zbc));
	if (gr_zbc == NULL) {
		return ret;
	}

	ret = gr_zbc_allocate_local_tbls(g, gr_zbc);
	if (ret != 0) {
		goto alloc_err;
	}

	ret = nvgpu_gr_zbc_load_default_table(g, gr_zbc);
	if (ret != 0) {
		goto alloc_err;
	}

	*zbc = gr_zbc;
	return ret;

alloc_err:
	nvgpu_kfree(g, gr_zbc);
	return ret;
}

/* deallocate the memory for the struct */
void nvgpu_gr_zbc_deinit(struct gk20a *g, struct nvgpu_gr_zbc *zbc)
{
	nvgpu_kfree(g, zbc->zbc_col_tbl);
	nvgpu_kfree(g, zbc->zbc_dep_tbl);
	nvgpu_kfree(g, zbc->zbc_s_tbl);
	nvgpu_kfree(g, zbc);
}

struct nvgpu_gr_zbc_entry *nvgpu_gr_zbc_entry_alloc(struct gk20a *g)
{
	return nvgpu_kzalloc(g, sizeof(struct nvgpu_gr_zbc_entry));
}
void nvgpu_gr_zbc_entry_free(struct gk20a *g, struct nvgpu_gr_zbc_entry *entry)
{
	nvgpu_kfree(g, entry);
}

u32 nvgpu_gr_zbc_get_entry_color_ds(struct nvgpu_gr_zbc_entry *entry,
		int idx)
{
	return entry->color_ds[idx];
}

void nvgpu_gr_zbc_set_entry_color_ds(struct nvgpu_gr_zbc_entry *entry,
		int idx, u32 ds)
{
	entry->color_ds[idx] = ds;
}

u32 nvgpu_gr_zbc_get_entry_color_l2(struct nvgpu_gr_zbc_entry *entry,
		int idx)
{
	return entry->color_l2[idx];
}

void nvgpu_gr_zbc_set_entry_color_l2(struct nvgpu_gr_zbc_entry *entry,
		int idx, u32 l2)
{
	entry->color_l2[idx] = l2;
}

u32 nvgpu_gr_zbc_get_entry_depth(struct nvgpu_gr_zbc_entry *entry)
{
	return entry->depth;
}

void nvgpu_gr_zbc_set_entry_depth(struct nvgpu_gr_zbc_entry *entry,
		u32 depth)
{
	entry->depth = depth;
}

u32 nvgpu_gr_zbc_get_entry_type(struct nvgpu_gr_zbc_entry *entry)
{
	return entry->type;
}

void nvgpu_gr_zbc_set_entry_type(struct nvgpu_gr_zbc_entry *entry,
		u32 type)
{
	entry->type = type;
}

u32 nvgpu_gr_zbc_get_entry_format(struct nvgpu_gr_zbc_entry *entry)
{
	return entry->format;
}

void nvgpu_gr_zbc_set_entry_format(struct nvgpu_gr_zbc_entry *entry,
		u32 format)
{
	entry->format = format;
}
