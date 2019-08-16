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


#include <stdlib.h>

#include <unit/unit.h>
#include <unit/io.h>

#include <nvgpu/posix/io.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/gr/config.h>

#include "common/gr/gr_config_priv.h"

#include "../nvgpu-gr.h"

static struct nvgpu_gr_config *unit_gr_config;

static int test_gr_config_init(struct unit_module *m,
		struct gk20a *g, void *args)
{
	unit_gr_config = nvgpu_gr_config_init(g);
	if (unit_gr_config == NULL) {
		return UNIT_FAIL;
	}

	return UNIT_SUCCESS;
}

static int test_gr_config_deinit(struct unit_module *m,
		struct gk20a *g, void *args)
{
	if (unit_gr_config != NULL) {
		nvgpu_gr_config_deinit(g, unit_gr_config);
		return UNIT_SUCCESS;
	}

	return UNIT_FAIL;
}

static int test_gr_config_count(struct unit_module *m,
		struct gk20a *g, void *args)
{
	u32 val = 0U;
	u32 *reg_base = NULL;
	u32 gindex = 0U, pindex = 0U;
	u32 pes_tpc_val = 0U;
	u32 pes_tpc_count[GK20A_GR_MAX_PES_PER_GPC] = {0x2, 0x2, 0x0};
	u32 pes_tpc_mask[GK20A_GR_MAX_PES_PER_GPC] = {0x5, 0xa, 0x0};
	u32 gpc_tpc_mask = 0xf;
	u32 gpc_skip_mask = 0x0;
	u32 gpc_tpc_count = 0x4;
	u32 gpc_ppc_count = 0x2;

	struct nvgpu_gr_config gv11b_gr_config = {
		.g = NULL,
		.max_gpc_count = 0x1,
		.max_tpc_per_gpc_count = 0x4,
		.max_tpc_count = 0x4,
		.gpc_count = 0x1,
		.tpc_count = 0x4,
		.ppc_count = 0x2,
		.pe_count_per_gpc = 0x2,
		.sm_count_per_tpc = 0x2,
		.gpc_ppc_count = &gpc_ppc_count,
		.gpc_tpc_count = &gpc_tpc_count,
		.pes_tpc_count = {NULL, NULL, NULL},
		.gpc_mask = 0x1,
		.gpc_tpc_mask = &gpc_tpc_mask,
		.pes_tpc_mask = {NULL, NULL, NULL},
		.gpc_skip_mask = &gpc_skip_mask,
		.no_of_sm = 0x0,
		.sm_to_cluster = NULL,
	};

	gv11b_gr_config.pes_tpc_mask[0] = &pes_tpc_mask[0];
	gv11b_gr_config.pes_tpc_mask[1] = &pes_tpc_mask[1];
	gv11b_gr_config.pes_tpc_count[0] = &pes_tpc_count[0];
	gv11b_gr_config.pes_tpc_count[1] = &pes_tpc_count[1];

	/*
	 * Compare the config registers value against
	 * gv11b silicon following poweron
	 */

	val = nvgpu_gr_config_get_max_gpc_count(unit_gr_config);
	if (val != gv11b_gr_config.max_gpc_count) {
		unit_err(m, " mismatch in max_gpc_count\n");
		goto init_fail;
	}

	val = nvgpu_gr_config_get_max_tpc_count(unit_gr_config);
	if (val != gv11b_gr_config.max_tpc_count) {
		unit_err(m, " mismatch in max_tpc_count\n");
		goto init_fail;
	}

	val = nvgpu_gr_config_get_max_tpc_per_gpc_count(unit_gr_config);
	if (val != gv11b_gr_config.max_tpc_per_gpc_count) {
		unit_err(m, " mismatch in max_tpc_per_gpc_count\n");
		goto init_fail;
	}

	val = nvgpu_gr_config_get_gpc_count(unit_gr_config);
	if (val != gv11b_gr_config.gpc_count) {
		unit_err(m, " mismatch in gpc_count\n");
		goto init_fail;
	}

	val = nvgpu_gr_config_get_tpc_count(unit_gr_config);
	if (val != gv11b_gr_config.tpc_count) {
		unit_err(m, " mismatch in tpc_count\n");
		goto init_fail;
	}

	val = nvgpu_gr_config_get_ppc_count(unit_gr_config);
	if (val != gv11b_gr_config.ppc_count) {
		unit_err(m, " mismatch in ppc_count\n");
		goto init_fail;
	}

	val = nvgpu_gr_config_get_pe_count_per_gpc(unit_gr_config);
	if (val != gv11b_gr_config.pe_count_per_gpc) {
		unit_err(m, " mismatch in pe_count_per_gpc\n");
		goto init_fail;
	}

	val = nvgpu_gr_config_get_sm_count_per_tpc(unit_gr_config);
	if (val != gv11b_gr_config.sm_count_per_tpc) {
		unit_err(m, " mismatch in sm_count_per_tpc\n");
		goto init_fail;
	}

	val = nvgpu_gr_config_get_gpc_mask(unit_gr_config);
	if (val != gv11b_gr_config.gpc_mask) {
		unit_err(m, " mismatch in gpc_mask\n");
		goto init_fail;
	}

	for (gindex = 0U; gindex < gv11b_gr_config.gpc_count;
							gindex++) {
		val = nvgpu_gr_config_get_gpc_ppc_count(unit_gr_config,
							gindex);
		if (val != gv11b_gr_config.gpc_ppc_count[gindex]) {
			unit_err(m, " mismatch in gpc_ppc_count\n");
			goto init_fail;
		}

		val = nvgpu_gr_config_get_gpc_skip_mask(unit_gr_config,
							gindex);
		if (val != gv11b_gr_config.gpc_skip_mask[gindex]) {
			unit_err(m, " mismatch in gpc_skip_mask\n");
			goto init_fail;
		}

		val = nvgpu_gr_config_get_gpc_tpc_count(unit_gr_config,
							gindex);
		if (val != gv11b_gr_config.gpc_tpc_count[gindex]) {
			unit_err(m, " mismatch in gpc_tpc_count\n");
			goto init_fail;
		}

		for (pindex = 0U; pindex < gv11b_gr_config.gpc_count;
							pindex++) {
			pes_tpc_val =
				gv11b_gr_config.pes_tpc_count[pindex][gindex];
			val = nvgpu_gr_config_get_pes_tpc_count(
					unit_gr_config, gindex, pindex);
			if (val != pes_tpc_val) {
				unit_err(m, " mismatch in pes_tpc_count\n");
				goto init_fail;
			}

			pes_tpc_val =
				gv11b_gr_config.pes_tpc_mask[pindex][gindex];
			val = nvgpu_gr_config_get_pes_tpc_mask(
					unit_gr_config, gindex, pindex);
			if (val != pes_tpc_val) {
				unit_err(m, " mismatch in pes_tpc_count\n");
				goto init_fail;
			}
		}
	}

	/*
	 * Check for valid memory
	 */
	reg_base = nvgpu_gr_config_get_gpc_tpc_mask_base(unit_gr_config);
	if (reg_base == NULL) {
		unit_err(m, " Invalid gpc_tpc_mask_base\n");
		goto init_fail;
	}

	reg_base = nvgpu_gr_config_get_gpc_tpc_count_base(unit_gr_config);
	if (reg_base == NULL) {
		unit_err(m, " Invalid gpc_tpc_count_base\n");
		goto init_fail;
	}

	return UNIT_SUCCESS;

init_fail:
	return UNIT_FAIL;
}

static int test_gr_config_set_get(struct unit_module *m,
		struct gk20a *g, void *args)
{
	u32 gindex = 0U;
	u32 val = 0U;
	struct nvgpu_sm_info *sm_info;

	srand(0);

	/*
	 * Set random value and read back
	 */
	val = (u32)rand();
	nvgpu_gr_config_set_no_of_sm(unit_gr_config, val);
	if (val != nvgpu_gr_config_get_no_of_sm(unit_gr_config)) {
		unit_err(m, " mismatch in no_of_sm\n");
		goto set_get_fail;
	}

	sm_info = nvgpu_gr_config_get_sm_info(unit_gr_config, 0);
	val = (u32)rand();
	nvgpu_gr_config_set_sm_info_gpc_index(sm_info, val);
	if (val != nvgpu_gr_config_get_sm_info_gpc_index(sm_info)) {
		unit_err(m, " mismatch in sm_info_gindex\n");
		goto set_get_fail;
	}

	val = (u32)rand();
	nvgpu_gr_config_set_sm_info_tpc_index(sm_info, val);
	if (val != nvgpu_gr_config_get_sm_info_tpc_index(sm_info)) {
		unit_err(m, " mismatch in sm_info_tpc_index\n");
		goto set_get_fail;
	}

	val = (u32)rand();
	nvgpu_gr_config_set_sm_info_global_tpc_index(sm_info, val);
	if (val !=
		nvgpu_gr_config_get_sm_info_global_tpc_index(sm_info)) {
		unit_err(m, " mismatch in sm_info_global_tpc_index\n");
		goto set_get_fail;
	}

	val = (u32)rand();
	nvgpu_gr_config_set_sm_info_sm_index(sm_info, val);
	if (val != nvgpu_gr_config_get_sm_info_sm_index(sm_info)) {
		unit_err(m, " mismatch in sm_info_sm_index\n");
		goto set_get_fail;
	}

	for (gindex = 0U; gindex < unit_gr_config->gpc_count;
							gindex++) {
		val = (u32)rand();
		nvgpu_gr_config_set_gpc_tpc_mask(unit_gr_config, gindex, val);
		if (val !=
		    nvgpu_gr_config_get_gpc_tpc_mask(unit_gr_config, gindex)) {
			unit_err(m, " mismatch in gpc_tpc_mask\n");
			goto set_get_fail;
		}
	}

	return UNIT_SUCCESS;

set_get_fail:
	return UNIT_FAIL;
}

struct unit_module_test nvgpu_gr_config_tests[] = {
	UNIT_TEST(init_support, test_gr_init_support, NULL, 0),
	UNIT_TEST(config_init, test_gr_config_init, NULL, 0),
	UNIT_TEST(config_check_init, test_gr_config_count, NULL, 0),
	UNIT_TEST(config_check_set_get, test_gr_config_set_get, NULL, 0),
	UNIT_TEST(config_deinit, test_gr_config_deinit, NULL, 0),
	UNIT_TEST(remove_support, test_gr_remove_support, NULL, 0),
};

UNIT_MODULE(nvgpu_gr_config, nvgpu_gr_config_tests, UNIT_PRIO_NVGPU_TEST);
