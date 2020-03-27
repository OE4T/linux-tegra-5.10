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

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <unit/io.h>
#include <unit/unit.h>

#include <nvgpu/gk20a.h>
#include <nvgpu/io.h>
#include <nvgpu/device.h>
#include <nvgpu/engines.h>
#include <nvgpu/engine_status.h>

#include <nvgpu/posix/posix-fault-injection.h>

#include "hal/fifo/engines_gp10b.h"

#include <nvgpu/hw/gp10b/hw_fifo_gp10b.h>
#include <nvgpu/hw/gp10b/hw_top_gp10b.h>

#include "../../nvgpu-fifo-common.h"
#include "nvgpu-engine-gp10b.h"

#ifdef ENGINE_GP10B_UNIT_DEBUG
#undef unit_verbose
#define unit_verbose	unit_info
#else
#define unit_verbose(unit, msg, ...) \
	do { \
		if (0) { \
			unit_info(unit, msg, ##__VA_ARGS__); \
		} \
	} while (0)
#endif

#define branches_str test_fifo_flags_str
#define pruned test_fifo_subtest_pruned

struct unit_ctx {
	struct unit_module *m;
	u32 branches;
	struct gpu_ops gops;
};

static struct unit_ctx u;

static inline void subtest_setup(struct unit_module *m, u32 branches)
{
	u.branches = branches;
}

#define F_ENGINE_INIT_CE_INFO_GET_NUM_ENGINES_NULL	BIT(0)
#define F_ENGINE_INIT_CE_INFO_NO_LCE			BIT(1)
#define F_ENGINE_INIT_CE_INFO_GET_DEV_INFO_FAIL		BIT(2)
#define F_ENGINE_INIT_CE_INFO_PBDMA_FIND_FAIL		BIT(3)
#define F_ENGINE_INIT_CE_INFO_ASYNC_CE			BIT(4)
#define F_ENGINE_INIT_CE_INFO_GRCE			BIT(5)
#define F_ENGINE_INIT_CE_INFO_FAULT_ID_0		BIT(6)
#define F_ENGINE_INIT_CE_INFO_GET_INST_NULL		BIT(7)
#define F_ENGINE_INIT_CE_INFO_INVAL_ENUM		BIT(8)
#define F_ENGINE_INIT_CE_INFO_LAST			BIT(9)

static u32 wrap_top_get_num_engine_type_entries(struct gk20a *g,
		u32 engine_type)
{
	u32 branches = u.branches;

	if (engine_type != NVGPU_ENGINE_LCE)
		goto done;

	if (branches & F_ENGINE_INIT_CE_INFO_NO_LCE) {
		return 0;
	}

	if ((branches & F_ENGINE_INIT_CE_INFO_GRCE) ||
	    (branches & F_ENGINE_INIT_CE_INFO_ASYNC_CE)) {
		return 1;
	}

done:
	return u.gops.top.get_num_engine_type_entries(g, engine_type);
}

static int wrap_top_get_device_info(struct gk20a *g,
		struct nvgpu_device_info *dev_info,
		u32 engine_type, u32 inst_id)
{
	u32 branches = u.branches;

	if (engine_type != NVGPU_ENGINE_LCE)
		goto done;

	if (branches & F_ENGINE_INIT_CE_INFO_GET_DEV_INFO_FAIL) {
		return -EINVAL;
	}

	if (branches & F_ENGINE_INIT_CE_INFO_FAULT_ID_0) {
		dev_info->fault_id = 0;
	} else {
		dev_info->fault_id = 1;
	}

	if (branches & F_ENGINE_INIT_CE_INFO_GRCE) {
		dev_info->runlist_id = nvgpu_engine_get_gr_runlist_id(g);
		dev_info->engine_id = 1;
		dev_info->engine_type = top_device_info_type_enum_lce_v();
		return 0;
	}

	if (branches & F_ENGINE_INIT_CE_INFO_ASYNC_CE) {
		dev_info->runlist_id = 1;
		dev_info->engine_id = 1;
		dev_info->engine_type = top_device_info_type_enum_lce_v();
		return 0;
	}

	if (branches & F_ENGINE_INIT_CE_INFO_INVAL_ENUM) {
		dev_info->runlist_id = 1;
		dev_info->engine_id = 1;
		dev_info->engine_type = 5;
		return 0;
	}

done:
	return u.gops.top.get_device_info(g, dev_info, engine_type, inst_id);

}

static bool wrap_pbdma_find_for_runlist(struct gk20a *g,
			u32 runlist_id, u32 *pbdma_id)
{
	if (u.branches & F_ENGINE_INIT_CE_INFO_PBDMA_FIND_FAIL)
		return false;

	return u.gops.pbdma.find_for_runlist(g, runlist_id, pbdma_id);
}

static u32 wrap_top_get_ce_inst_id(struct gk20a *g, u32 engine_type)
{
	if (u.gops.top.get_ce_inst_id != NULL) {
		return u.gops.top.get_ce_inst_id(g, engine_type);
	}

	return 0;
}

int test_gp10b_engine_init_ce_info(struct unit_module *m,
		struct gk20a *g, void *args)
{
	int ret = UNIT_FAIL;
	struct nvgpu_fifo *f = &g->fifo;
	int err;
	u32 fail =
		F_ENGINE_INIT_CE_INFO_GET_DEV_INFO_FAIL |
		F_ENGINE_INIT_CE_INFO_PBDMA_FIND_FAIL;
	const char *labels[] = {
		"get_num_engines_null",
		"no_lce",
		"get_dev_info_fail",
		"pbdma_find_fail",
		"async_ce",
		"grce",
		"fault_id_0",
		"get_inst_null",
		"inval_enum"
	};
	u32 prune =
		F_ENGINE_INIT_CE_INFO_GET_NUM_ENGINES_NULL |
		F_ENGINE_INIT_CE_INFO_NO_LCE |
		F_ENGINE_INIT_CE_INFO_INVAL_ENUM | fail;
	u32 branches = 0;
	u32 num_lce;

	u.m = m;
	u.gops = g->ops;

	unit_assert(f->num_engines > 0, goto done);
	unit_assert(f->engine_info[0].engine_enum == NVGPU_ENGINE_GR,
			goto done);

	g->ops.top.get_device_info = wrap_top_get_device_info;
	g->ops.pbdma.find_for_runlist = wrap_pbdma_find_for_runlist;

	for (branches = 0U; branches < F_ENGINE_INIT_CE_INFO_LAST; branches++) {

		if (pruned(branches, prune)) {
			unit_verbose(m, "%s branches=%s (pruned)\n",
				__func__, branches_str(branches, labels));
			continue;
		}
		subtest_setup(m, branches);
		unit_verbose(m, "%s branches=%s\n", __func__,
			branches_str(branches, labels));

		g->ops.top.get_num_engine_type_entries =
			branches & F_ENGINE_INIT_CE_INFO_GET_NUM_ENGINES_NULL ?
			NULL : wrap_top_get_num_engine_type_entries;

		g->ops.top.get_ce_inst_id =
			branches & F_ENGINE_INIT_CE_INFO_GET_INST_NULL ?
			NULL : wrap_top_get_ce_inst_id;

		/* keep only GR engine */
		f->num_engines = 1;

		err = gp10b_engine_init_ce_info(f);

		if ((branches & F_ENGINE_INIT_CE_INFO_GET_NUM_ENGINES_NULL) ||
		    (branches & F_ENGINE_INIT_CE_INFO_NO_LCE) ||
		    (branches & F_ENGINE_INIT_CE_INFO_INVAL_ENUM)) {
			num_lce = 0;
		} else {
			num_lce = g->ops.top.get_num_engine_type_entries(g, NVGPU_ENGINE_LCE);
		}

		if (branches & fail) {
			unit_assert(err != 0, goto done);
			unit_assert(f->num_engines < (1 + num_lce), goto done);
		} else {
			unit_assert(err == 0, goto done);
			unit_assert(f->num_engines = (1 + num_lce), goto done);
		}
	}

	ret = UNIT_SUCCESS;
done:
	if (ret != UNIT_SUCCESS) {
		unit_err(m, "%s branches=%s\n", __func__,
			branches_str(branches, labels));
	}

	g->ops = u.gops;
	return ret;
}

struct unit_module_test nvgpu_engine_gp10b_tests[] = {
	UNIT_TEST(init_support, test_fifo_init_support, NULL, 0),
	UNIT_TEST(engine_init_ce_info, test_gp10b_engine_init_ce_info, NULL, 0),
	UNIT_TEST(remove_support, test_fifo_remove_support, NULL, 0),
};

UNIT_MODULE(nvgpu_engine_gp10b, nvgpu_engine_gp10b_tests, UNIT_PRIO_NVGPU_TEST);
