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
	unit_assert(err == 0, goto done);

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
			unit_assert(err != 0, goto done);
			unit_assert(f->active_engines_list == NULL, goto done);
			unit_assert(f->engine_info == NULL, goto done);
		} else {
			unit_assert(err == 0, goto done);
			unit_assert(f->active_engines_list != NULL, goto done);
			unit_assert(f->engine_info != NULL, goto done);
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
			unit_assert(err != 0, goto done);
		} else {
			unit_assert(err == 0, goto done);
			unit_assert(f->num_engines > 0, goto done);
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

	unit_assert(nvgpu_engine_check_valid_id(g, U32_MAX) == false,
				goto done);

	unit_assert(nvgpu_engine_get_ids(g, &engine_id, 1,
				NVGPU_ENGINE_INVAL) == 0, goto done);

	for (e = NVGPU_ENGINE_GR; e < NVGPU_ENGINE_INVAL; e++) {

		n = nvgpu_engine_get_ids(g, engine_ids, MAX_ENGINE_IDS, e);
		unit_assert(n > 0, goto done);
		for (i = 0; i < n; i++) {
			engine_id = engine_ids[i];

			unit_assert(nvgpu_engine_check_valid_id(g, engine_id) ==
							true, goto done);
			unit_ctx.eng_mask |= BIT(engine_id);
			if (e == NVGPU_ENGINE_ASYNC_CE || e == NVGPU_ENGINE_GRCE) {
				unit_ctx.ce_mask |= BIT(engine_id);
			}
		}
	}

	unit_assert(nvgpu_engine_get_ids(g, &engine_id, 1,
					NVGPU_ENGINE_GR) == 1, goto done);
	unit_assert(engine_id == nvgpu_engine_get_gr_id(g), goto done);
	unit_assert(unit_ctx.eng_mask != 0, goto done);
	unit_assert(unit_ctx.ce_mask != 0, goto done);

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
			unit_assert(info != NULL, goto done);
			unit_assert(info->engine_id == engine_id, goto done);
			eng_mask |= BIT(engine_id);
		} else {
			unit_assert(info == NULL, goto done);
		}
	}
	unit_verbose(m, "eng_mask=%x\n", eng_mask);
	unit_verbose(m, "unit_ctx.eng_mask=%x\n", unit_ctx.eng_mask);
	unit_assert(eng_mask == unit_ctx.eng_mask, goto done);

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
	unit_assert(engine_enum == NVGPU_ENGINE_GR, goto done);

	engine_enum = nvgpu_engine_enum_from_type(g,
		top_device_info_type_enum_lce_v());
	unit_assert(engine_enum == NVGPU_ENGINE_ASYNC_CE, goto done);

	engine_enum = nvgpu_engine_enum_from_type(g, 0xff);
	unit_assert(engine_enum == NVGPU_ENGINE_INVAL, goto done);

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

	unit_assert(intr_mask != 0U, goto done);
	for (engine_id = 0; engine_id < f->max_engines; engine_id++) {
		unit_verbose(m, "engine_id=%u\n", engine_id);
		mask = nvgpu_engine_act_interrupt_mask(g, engine_id);
		if (nvgpu_engine_check_valid_id(g, engine_id)) {
			unit_assert(mask != 0, goto done);
			unit_assert((mask & intr_mask) == mask, goto done);
			all_mask |= mask;
		} else {
			unit_assert(mask == 0, goto done);
		}
	}
	unit_assert(intr_mask == all_mask, goto done);

	ce_reset_mask = nvgpu_engine_get_all_ce_reset_mask(g);
	unit_assert(ce_reset_mask != 0, goto done);;

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
