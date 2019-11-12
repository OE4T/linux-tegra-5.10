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
#include <sys/types.h>
#include <unistd.h>

#include <unit/io.h>
#include <unit/unit.h>

#include <nvgpu/channel.h>
#include <nvgpu/channel_sync.h>
#include <nvgpu/dma.h>
#include <nvgpu/engines.h>
#include <nvgpu/tsg.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/runlist.h>

#include "hal/init/hal_gv11b.h"

#include <nvgpu/posix/posix-fault-injection.h>

#include "nvgpu/hw/gv11b/hw_top_gv11b.h"

#include "../nvgpu-fifo-common.h"
#include "../nvgpu-fifo-gv11b.h"
#include "nvgpu-engine.h"

#ifdef ENGINE_UNIT_DEBUG
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

struct unit_ctx {
	u32 branches;
	u32 ce_mask;
	u32 eng_mask;
};

struct unit_ctx unit_ctx;

static void subtest_setup(u32 branches)
{
	unit_ctx.branches = branches;
}

#define subtest_pruned	test_fifo_subtest_pruned
#define branches_str	test_fifo_flags_str

#define assert(cond)	unit_assert(cond, goto done)

#define F_ENGINE_SETUP_SW_ENGINE_INFO_ENOMEM	BIT(0)
#define F_ENGINE_SETUP_SW_ENGINE_LIST_ENOMEM	BIT(1)
#define F_ENGINE_SETUP_SW_INIT_INFO_FAIL	BIT(2)
#define F_ENGINE_SETUP_SW_LAST			BIT(3)


static int stub_engine_init_info_EINVAL(struct nvgpu_fifo *f)
{
	return -EINVAL;
}

static int stub_engine_init_info(struct nvgpu_fifo *f)
{
	return 0;
}

int test_engine_setup_sw(struct unit_module *m,
		struct gk20a *g, void *args)
{
	struct gpu_ops gops = g->ops;
	struct nvgpu_fifo *f = &g->fifo;
	struct nvgpu_posix_fault_inj *kmem_fi = NULL;
	u32 branches = 0;
	int ret = UNIT_FAIL;
	int err;
	u32 fail = F_ENGINE_SETUP_SW_ENGINE_INFO_ENOMEM |
		F_ENGINE_SETUP_SW_ENGINE_LIST_ENOMEM |
		F_ENGINE_SETUP_SW_INIT_INFO_FAIL;
	const char *labels[] = {
		"engine_info_nomem",
		"engine_list_nomem",
		"init_info_fail",
	};
	u32 prune = fail;

	err = test_fifo_setup_gv11b_reg_space(m, g);
	assert(err == 0);

	gv11b_init_hal(g);

	kmem_fi = nvgpu_kmem_get_fault_injection();

	for (branches = 0U; branches < F_ENGINE_SETUP_SW_LAST; branches++) {

		if (subtest_pruned(branches, prune)) {
			unit_verbose(m, "%s branches=%s (pruned)\n",
				__func__, branches_str(branches, labels));
			continue;
		}
		subtest_setup(branches);
		unit_verbose(m, "%s branches=%s\n", __func__,
			branches_str(branches, labels));

		nvgpu_posix_enable_fault_injection(kmem_fi, false, 0);

		if (branches & F_ENGINE_SETUP_SW_ENGINE_INFO_ENOMEM) {
			nvgpu_posix_enable_fault_injection(kmem_fi, true, 0);
		}

		if (branches & F_ENGINE_SETUP_SW_ENGINE_LIST_ENOMEM) {
			nvgpu_posix_enable_fault_injection(kmem_fi, true, 1);
		}

		g->ops.engine.init_info =
			branches & F_ENGINE_SETUP_SW_INIT_INFO_FAIL ?
			stub_engine_init_info_EINVAL : stub_engine_init_info;

		err = nvgpu_engine_setup_sw(g);

		if (branches & fail) {
			assert(err != 0);
			assert(f->active_engines_list == NULL);
			assert(f->engine_info == NULL);
		} else {
			assert(err == 0);
			assert(f->active_engines_list != NULL);
			assert(f->engine_info != NULL);
			nvgpu_engine_cleanup_sw(g);
		}
	}

	ret = UNIT_SUCCESS;
done:
	nvgpu_posix_enable_fault_injection(kmem_fi, false, 0);
	if (ret != UNIT_SUCCESS) {
		unit_err(m, "%s branches=%s\n", __func__,
			branches_str(branches, labels));
	}
	g->ops = gops;
	return ret;
}

#define F_ENGINE_INIT_INFO_GET_DEV_INFO_NULL	BIT(0)
#define F_ENGINE_INIT_INFO_GET_DEV_INFO_FAIL	BIT(1)
#define F_ENGINE_INIT_INFO_PBDMA_FIND_FAIL	BIT(2)
#define F_ENGINE_INIT_INFO_INIT_CE_FAIL		BIT(3)
#define F_ENGINE_INIT_INFO_LAST			BIT(4)

static int stub_top_get_device_info_EINVAL(struct gk20a *g,
		struct nvgpu_device_info *dev_info,
		u32 engine_type, u32 inst_id)
{
	return -EINVAL;
}

static bool stub_pbdma_find_for_runlist_none(struct gk20a *g,
			u32 runlist_id, u32 *pbdma_id)
{
	return false;
}

static int stub_engine_init_ce_info_EINVAL(struct nvgpu_fifo *f)
{
	return -EINVAL;
}

int test_engine_init_info(struct unit_module *m,
		struct gk20a *g, void *args)
{
	struct gpu_ops gops = g->ops;
	struct nvgpu_fifo *f = &g->fifo;
	struct nvgpu_fifo fifo = g->fifo;
	u32 branches;
	int ret = UNIT_FAIL;
	int err;
	u32 fail =
		F_ENGINE_INIT_INFO_GET_DEV_INFO_NULL |
		F_ENGINE_INIT_INFO_GET_DEV_INFO_FAIL |
		F_ENGINE_INIT_INFO_PBDMA_FIND_FAIL |
		F_ENGINE_INIT_INFO_INIT_CE_FAIL;
	const char *labels[] = {
		"get_dev_info_null",
		"get_dev_info_fail",
		"pbdma_find_fail",
		"init_ce_fail",
	};
	u32 prune = fail;

	for (branches = 0U; branches < F_ENGINE_INIT_INFO_LAST; branches++) {

		if (subtest_pruned(branches, prune)) {
			unit_verbose(m, "%s branches=%s (pruned)\n",
				__func__, branches_str(branches, labels));
			continue;
		}
		subtest_setup(branches);
		unit_verbose(m, "%s branches=%s\n", __func__,
			branches_str(branches, labels));

		if (branches & F_ENGINE_INIT_INFO_GET_DEV_INFO_NULL) {
			g->ops.top.get_device_info = NULL;
		} else {
			g->ops.top.get_device_info =
				branches & F_ENGINE_INIT_INFO_GET_DEV_INFO_FAIL ?
					stub_top_get_device_info_EINVAL :
					gops.top.get_device_info;
		}

		g->ops.pbdma.find_for_runlist =
			branches & F_ENGINE_INIT_INFO_PBDMA_FIND_FAIL ?
				stub_pbdma_find_for_runlist_none :
				gops.pbdma.find_for_runlist;

		g->ops.engine.init_ce_info =
			branches & F_ENGINE_INIT_INFO_INIT_CE_FAIL ?
				stub_engine_init_ce_info_EINVAL :
				gops.engine.init_ce_info;

		err = nvgpu_engine_init_info(f);

		if (branches & fail) {
			assert(err != 0);
		} else {
			assert(err == 0);
			assert(f->num_engines > 0);
		}
	}

	ret = UNIT_SUCCESS;
done:
	if (ret != UNIT_SUCCESS) {
		unit_err(m, "%s branches=%s\n", __func__,
			branches_str(branches, labels));
	}
	g->ops = gops;
	g->fifo = fifo;
	return ret;
}

#define MAX_ENGINE_IDS	8

int test_engine_ids(struct unit_module *m,
		struct gk20a *g, void *args)
{
	int ret = UNIT_FAIL;
	enum nvgpu_fifo_engine e;
	u32 engine_ids[MAX_ENGINE_IDS];
	u32 n, i;
	u32 engine_id;

	unit_ctx.ce_mask = 0;
	unit_ctx.eng_mask = 0;

	assert(nvgpu_engine_check_valid_id(g, U32_MAX) == false);

	assert(nvgpu_engine_get_ids(g, &engine_id, 1, NVGPU_ENGINE_INVAL) == 0);

	for (e = NVGPU_ENGINE_GR; e < NVGPU_ENGINE_INVAL; e++) {

		n = nvgpu_engine_get_ids(g, engine_ids, MAX_ENGINE_IDS, e);
		assert(n > 0);
		for (i = 0; i < n; i++) {
			engine_id = engine_ids[i];

			assert(nvgpu_engine_check_valid_id(g, engine_id) == true);
			unit_ctx.eng_mask |= BIT(engine_id);
			if (e == NVGPU_ENGINE_ASYNC_CE || e == NVGPU_ENGINE_GRCE) {
				unit_ctx.ce_mask |= BIT(engine_id);
			}
		}
	}

	assert(nvgpu_engine_get_ids(g, &engine_id, 1, NVGPU_ENGINE_GR) == 1);
	assert(engine_id == nvgpu_engine_get_gr_id(g));
	assert(unit_ctx.eng_mask != 0);
	assert(unit_ctx.ce_mask != 0);

	ret = UNIT_SUCCESS;
done:
	return ret;
}

int test_engine_get_active_eng_info(struct unit_module *m,
		struct gk20a *g, void *args)
{
	int ret = UNIT_FAIL;
	u32 engine_id;
	struct nvgpu_engine_info *info;
	u32 eng_mask = 0;
	struct nvgpu_fifo *f = &g->fifo;

	for (engine_id = 0; engine_id < f->max_engines; engine_id++) {

		unit_verbose(m, "engine_id=%u\n", engine_id);
		info = nvgpu_engine_get_active_eng_info(g, engine_id);
		if (nvgpu_engine_check_valid_id(g, engine_id)) {
			assert(info != NULL);
			assert(info->engine_id == engine_id);
			eng_mask |= BIT(engine_id);
		} else {
			assert(info == NULL);
		}
	}
	unit_verbose(m, "eng_mask=%x\n", eng_mask);
	unit_verbose(m, "unit_ctx.eng_mask=%x\n", unit_ctx.eng_mask);
	assert(eng_mask == unit_ctx.eng_mask);

	ret = UNIT_SUCCESS;
done:
	return ret;
}

int test_engine_enum_from_type(struct unit_module *m,
		struct gk20a *g, void *args)
{
	int ret = UNIT_FAIL;
	int engine_enum;

	engine_enum = nvgpu_engine_enum_from_type(g,
		top_device_info_type_enum_graphics_v());
	assert(engine_enum == NVGPU_ENGINE_GR);

	engine_enum = nvgpu_engine_enum_from_type(g,
		top_device_info_type_enum_lce_v());
	assert(engine_enum == NVGPU_ENGINE_ASYNC_CE);

	engine_enum = nvgpu_engine_enum_from_type(g, 0xff);
	assert(engine_enum == NVGPU_ENGINE_INVAL);

	ret = UNIT_SUCCESS;
done:
	return ret;
}

int test_engine_interrupt_mask(struct unit_module *m,
		struct gk20a *g, void *args)
{
	int ret = UNIT_FAIL;
	u32 intr_mask =
		nvgpu_gr_engine_interrupt_mask(g) |
		nvgpu_ce_engine_interrupt_mask(g);
	u32 all_mask = 0U;
	u32 ce_reset_mask;
	u32 mask;
	u32 engine_id;
	struct nvgpu_fifo *f = &g->fifo;

	assert(intr_mask != 0U);
	for (engine_id = 0; engine_id < f->max_engines; engine_id++) {
		unit_verbose(m, "engine_id=%u\n", engine_id);
		mask = nvgpu_engine_act_interrupt_mask(g, engine_id);
		if (nvgpu_engine_check_valid_id(g, engine_id)) {
			assert(mask != 0);
			assert((mask & intr_mask) == mask);
			all_mask |= mask;
		} else {
			assert(mask == 0);
		}
	}
	assert(intr_mask == all_mask);

	ce_reset_mask = nvgpu_engine_get_all_ce_reset_mask(g);
	assert(ce_reset_mask != 0);;

	ret = UNIT_SUCCESS;
done:
	return ret;
}

struct unit_module_test nvgpu_engine_tests[] = {
	UNIT_TEST(setup_sw, test_engine_setup_sw, &unit_ctx, 0),
	UNIT_TEST(init_support, test_fifo_init_support, &unit_ctx, 0),
	UNIT_TEST(init_info, test_engine_init_info, &unit_ctx, 0),
	UNIT_TEST(ids, test_engine_ids, &unit_ctx, 0),
	UNIT_TEST(get_active_eng_info, test_engine_get_active_eng_info, &unit_ctx, 0),
	UNIT_TEST(enum_from_type, test_engine_enum_from_type, &unit_ctx, 0),
	UNIT_TEST(interrupt_mask, test_engine_interrupt_mask, &unit_ctx, 0),
	UNIT_TEST(remove_support, test_fifo_remove_support, &unit_ctx, 0),
};

UNIT_MODULE(nvgpu_engine, nvgpu_engine_tests, UNIT_PRIO_NVGPU_TEST);
