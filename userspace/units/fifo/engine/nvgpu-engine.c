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
#include <nvgpu/engine_status.h>
#include <nvgpu/tsg.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/runlist.h>

#include "hal/init/hal_gv11b.h"

#include <nvgpu/posix/posix-fault-injection.h>

#include "nvgpu/hw/gv11b/hw_top_gv11b.h"

#include "../nvgpu-fifo-common.h"
#include "../nvgpu-fifo-gv11b.h"
#include "nvgpu-engine.h"
#include "nvgpu-engine-status.h"

#define ENGINE_UNIT_DEBUG
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
	u32 id;
	u32 is_tsg;
};

static struct unit_ctx u;

static void subtest_setup(u32 branches)
{
	u.branches = branches;

	/* do NOT clean u.eng_mask */
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
#define F_ENGINE_INIT_INFO_INIT_CE_FAIL		BIT(2)
#define F_ENGINE_INIT_INFO_LAST			BIT(3)

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
		F_ENGINE_INIT_INFO_INIT_CE_FAIL;
	const char *labels[] = {
		"get_dev_info_null",
		"get_dev_info_fail",
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

	u.ce_mask = 0;
	u.eng_mask = 0;

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
			u.eng_mask |= BIT(engine_id);
			if (e == NVGPU_ENGINE_ASYNC_CE || e == NVGPU_ENGINE_GRCE) {
				u.ce_mask |= BIT(engine_id);
			}
		}
	}

	unit_assert(nvgpu_engine_get_ids(g, &engine_id,
		1, NVGPU_ENGINE_GR) == 1, goto done);
	unit_assert(engine_id == nvgpu_engine_get_gr_id(g), goto done);
	unit_assert(u.eng_mask != 0, goto done);
	unit_assert(u.ce_mask != 0, goto done);

	unit_assert(nvgpu_engine_get_ids(g, &engine_id,
		0, NVGPU_ENGINE_GR) == 0, goto done);
	unit_assert(nvgpu_engine_get_ids(g, &engine_id,
		1, NVGPU_ENGINE_GRCE) == 1, goto done);

	ret = UNIT_SUCCESS;
done:
	return ret;
}

int test_engine_is_valid_runlist_id(struct unit_module *m,
		struct gk20a *g, void *args)
{
	int ret = UNIT_FAIL;
	u32 i;
	u32 engine_id;
	u32 runlist_id = 0;
	struct nvgpu_engine_info *engine_info;
	struct nvgpu_fifo *f = &g->fifo;

	for (i = 0; i < f->num_engines; i++) {
		engine_id = f->active_engines_list[i];
		engine_info = &f->engine_info[engine_id];
		unit_assert(nvgpu_engine_is_valid_runlist_id(g,
			engine_info->runlist_id), goto done);
	}

	unit_assert(!nvgpu_engine_is_valid_runlist_id(NULL,
		runlist_id), goto done);
	unit_assert(!nvgpu_engine_is_valid_runlist_id(g,
		NVGPU_INVALID_RUNLIST_ID), goto done);

	ret = UNIT_SUCCESS;
done:
	return ret;
}



int test_engine_get_fast_ce_runlist_id(struct unit_module *m,
		struct gk20a *g, void *args)
{
	u32 runlist_id;
	int ret = UNIT_FAIL;

	runlist_id = nvgpu_engine_get_fast_ce_runlist_id(g);
	unit_assert(runlist_id != NVGPU_INVALID_RUNLIST_ID, goto done);

	unit_assert(nvgpu_engine_get_fast_ce_runlist_id(NULL) ==
		NVGPU_INVALID_ENG_ID, goto done);

	ret = UNIT_SUCCESS;
done:
	return ret;
}

int test_engine_get_gr_runlist_id(struct unit_module *m,
		struct gk20a *g, void *args)
{
	struct nvgpu_fifo *f = &g->fifo;
	struct nvgpu_fifo fifo = g->fifo;
	u32 runlist_id;
	int ret = UNIT_FAIL;
	struct nvgpu_engine_info engine_info[2];
	u32 active_engines_list;

	runlist_id = nvgpu_engine_get_gr_runlist_id(g);
	unit_assert(runlist_id != NVGPU_INVALID_RUNLIST_ID, goto done);

	f->num_engines = 1;
	f->max_engines = 1;

	f->active_engines_list = &active_engines_list;
	active_engines_list = 0;

	f->engine_info = engine_info;
	engine_info[0].engine_id = 0;
	engine_info[0].runlist_id = 1;

	/* NVGPU_ENGINE_GR not found */
	engine_info[0].engine_enum = NVGPU_ENGINE_GRCE;
	runlist_id = nvgpu_engine_get_gr_runlist_id(g);
	unit_assert(runlist_id == NVGPU_INVALID_RUNLIST_ID, goto done);

	/* valid entry */
	engine_info[0].engine_enum = NVGPU_ENGINE_GR;
	runlist_id = nvgpu_engine_get_gr_runlist_id(g);
	unit_assert(runlist_id != NVGPU_INVALID_RUNLIST_ID, goto done);

	ret = UNIT_SUCCESS;

done:
	g->fifo = fifo;
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
	struct nvgpu_fifo fifo = g->fifo;

	unit_assert(nvgpu_engine_get_active_eng_info(NULL, 0) == NULL,
		goto done);

	for (engine_id = 0; engine_id <= f->max_engines; engine_id++) {

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
	unit_verbose(m, "u.eng_mask=%x\n", u.eng_mask);
	unit_assert(eng_mask == u.eng_mask, goto done);

	f->num_engines = 0;
	unit_assert(nvgpu_engine_get_active_eng_info(g, 0) == NULL,
		goto done);

	ret = UNIT_SUCCESS;
done:
	g->fifo = fifo;
	return ret;
}

int test_engine_interrupt_mask(struct unit_module *m,
		struct gk20a *g, void *args)
{
	int ret = UNIT_FAIL;
	struct gpu_ops gops = g->ops;
	u32 intr_mask =
		nvgpu_gr_engine_interrupt_mask(g) |
		nvgpu_ce_engine_interrupt_mask(g);
	u32 all_mask = 0U;
	u32 ce_reset_mask;
	u32 mask;
	u32 engine_id;
	struct nvgpu_fifo *f = &g->fifo;
	struct nvgpu_fifo fifo = g->fifo;

	unit_assert(nvgpu_engine_check_valid_id(NULL, 0) == false, goto done);

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

	unit_assert(nvgpu_engine_act_interrupt_mask(NULL, 0) == 0, goto done);

	g->ops.ce.isr_stall = NULL;
	unit_assert(nvgpu_ce_engine_interrupt_mask(g) == 0, goto done);

	g->ops = gops;
	g->ops.ce.isr_nonstall = NULL;
	unit_assert(nvgpu_ce_engine_interrupt_mask(g) == 0, goto done);

	ce_reset_mask = nvgpu_engine_get_all_ce_reset_mask(g);
	unit_assert(ce_reset_mask != 0, goto done);

	unit_assert(nvgpu_engine_get_all_ce_reset_mask(NULL) == 0, goto done);

	f->num_engines = 0;
	unit_assert(nvgpu_engine_check_valid_id(g, 0) == false, goto done);

	ret = UNIT_SUCCESS;
done:
	g->fifo = fifo;
	g->ops = gops;
	return ret;
}

int test_engine_mmu_fault_id(struct unit_module *m,
		struct gk20a *g, void *args)
{
	struct nvgpu_fifo *f = &g->fifo;
	int ret = UNIT_FAIL;
	struct nvgpu_engine_info *engine_info;
	u32 fault_id;
	u32 engine_id;
	u32 id;

	for (engine_id = 0;
		engine_id <= f->max_engines;
		engine_id++) {

		engine_info = nvgpu_engine_get_active_eng_info(g, engine_id);
		unit_assert((engine_info == NULL) ==
			!nvgpu_engine_check_valid_id(g, engine_id), goto done);

		fault_id = nvgpu_engine_id_to_mmu_fault_id(g, engine_id);
		unit_assert((fault_id == NVGPU_INVALID_ENG_ID) ==
			!nvgpu_engine_check_valid_id(g, engine_id), goto done);
		unit_assert(!engine_info ||
			(engine_info->fault_id == fault_id), goto done);
		id = nvgpu_engine_mmu_fault_id_to_engine_id(g, fault_id);
		unit_assert((id == NVGPU_INVALID_ENG_ID) ==
			!nvgpu_engine_check_valid_id(g, engine_id), goto done);
		unit_assert(!engine_info ||
			(engine_info->engine_id == id), goto done);
	}

	ret = UNIT_SUCCESS;
done:
	return ret;
}

int test_engine_mmu_fault_id_veid(struct unit_module *m,
		struct gk20a *g, void *args)
{
	struct nvgpu_fifo *f = &g->fifo;
	struct nvgpu_fifo fifo = g->fifo;
	int ret = UNIT_FAIL;
	struct nvgpu_engine_info *engine_info;
	u32 fault_id;
	u32 engine_id;
	u32 veid;
	u32 gr_eng_fault_id;
	u32 pbdma_id;
	u32 id;
	u32 n;
	u32 i;

	for (engine_id = 0;
		engine_id <= f->max_engines;
		engine_id++) {

		engine_info = nvgpu_engine_get_active_eng_info(g, engine_id);
		unit_assert((engine_info == NULL) ==
			!nvgpu_engine_check_valid_id(g, engine_id), goto done);

		fault_id = nvgpu_engine_id_to_mmu_fault_id(g, engine_id);
		unit_assert((fault_id == NVGPU_INVALID_ENG_ID) ==
			!nvgpu_engine_check_valid_id(g, engine_id), goto done);
		unit_assert(!engine_info ||
			(engine_info->fault_id == fault_id), goto done);

		id = nvgpu_engine_mmu_fault_id_to_eng_id_and_veid(g,
			fault_id, &veid);
		unit_assert(!engine_info || (id == engine_id), goto done);
	}

	/* fault_id in GR MMU fault id range */
	engine_id = nvgpu_engine_get_gr_id(g);
	engine_info = nvgpu_engine_get_active_eng_info(g, engine_id);
	unit_assert(engine_info->engine_enum == NVGPU_ENGINE_GR,
		goto done);
	gr_eng_fault_id = engine_info->fault_id;
	for (i = 0; i < f->max_subctx_count; i++) {
		fault_id = gr_eng_fault_id + i;
		veid = nvgpu_engine_mmu_fault_id_to_veid(g,
			fault_id, gr_eng_fault_id);
		unit_assert(veid == i, goto done);

		id = nvgpu_engine_mmu_fault_id_to_eng_id_and_veid(g,
			fault_id, &veid);
		unit_assert(veid == i, goto done);
		unit_assert(id == engine_id, goto done);

		nvgpu_engine_mmu_fault_id_to_eng_ve_pbdma_id(g,
			fault_id, &id, &veid, &pbdma_id);
		unit_assert(id == engine_id, goto done);
		unit_assert(pbdma_id == INVAL_ID, goto done);
	}

	/* fault_id in CE range */
	n = nvgpu_engine_get_ids(g, &engine_id, 1, NVGPU_ENGINE_ASYNC_CE);
	unit_assert(n == 1, goto done);

	engine_info = nvgpu_engine_get_active_eng_info(g, engine_id);
	unit_assert(engine_info != NULL, goto done);

	veid = 0xcafe;
	fault_id = engine_info->fault_id;
	id = nvgpu_engine_mmu_fault_id_to_eng_id_and_veid(g,
		fault_id, &veid);
	unit_assert(id == engine_id, goto done);
	unit_assert(veid == INVAL_ID, goto done);

	/* valid CE MMU fault id */
	fault_id = engine_info->fault_id;
	nvgpu_engine_mmu_fault_id_to_eng_ve_pbdma_id(g,
			fault_id, &id, &veid, &pbdma_id);
	unit_assert(id == engine_id, goto done);
	unit_assert(veid == INVAL_ID, goto done);
	unit_assert(pbdma_id == INVAL_ID, goto done);

	/* valid PBDMA MMU fault id */
	fault_id = 33;
	nvgpu_engine_mmu_fault_id_to_eng_ve_pbdma_id(g,
			fault_id, &id, &veid, &pbdma_id);
	unit_assert(id == NVGPU_INVALID_ENG_ID, goto done);
	unit_assert(veid == INVAL_ID, goto done);
	unit_assert(pbdma_id != INVAL_ID, goto done);

	/* invalid engine and pbdma MMU fault id */
	pbdma_id = 0xcafe;
	nvgpu_engine_mmu_fault_id_to_eng_ve_pbdma_id(g,
			INVAL_ID, &id, &veid, &pbdma_id);
	unit_assert(id == NVGPU_INVALID_ENG_ID, goto done);
	unit_assert(veid == INVAL_ID, goto done);
	unit_assert(pbdma_id == INVAL_ID, goto done);

	ret = UNIT_SUCCESS;
done:
	g->fifo = fifo;
	return ret;
}

#define F_GET_MASK_IS_TSG	BIT(0)
#define F_GET_MASK_LOAD		BIT(1)
#define F_GET_MASK_BUSY 	BIT(2)
#define F_GET_MASK_SAME_ID 	BIT(3)
#define F_GET_MASK_SAME_TYPE	BIT(4)
#define F_GET_MASK_LAST 	BIT(5)

#define FECS_METHOD_WFI_RESTORE	0x80000U

static void stub_engine_read_engine_status_info(struct gk20a *g,
		u32 engine_id, struct nvgpu_engine_status_info *status)
{
	status->ctxsw_status = u.branches & F_GET_MASK_LOAD ?
		NVGPU_CTX_STATUS_CTXSW_LOAD :
		NVGPU_CTX_STATUS_VALID;

	status->is_busy = ((u.branches & F_GET_MASK_BUSY) != 0);

	status->ctx_id_type = ENGINE_STATUS_CTX_ID_TYPE_INVALID;
	status->ctx_next_id_type = ENGINE_STATUS_CTX_NEXT_ID_TYPE_INVALID;

	if (u.branches & F_GET_MASK_SAME_TYPE) {
		status->ctx_id_type =
			u.branches & F_GET_MASK_IS_TSG ?
				ENGINE_STATUS_CTX_ID_TYPE_TSGID :
				ENGINE_STATUS_CTX_ID_TYPE_CHID;
		status->ctx_next_id_type =
			u.branches & F_GET_MASK_IS_TSG ?
				ENGINE_STATUS_CTX_NEXT_ID_TYPE_TSGID :
				ENGINE_STATUS_CTX_NEXT_ID_TYPE_CHID;
	}

	if (u.branches & F_GET_MASK_SAME_ID) {
		status->ctx_id = u.id;
		status->ctx_next_id = u.id;
	} else {
		status->ctx_id = ~0;
		status->ctx_next_id = ~0;
	}
}

int test_engine_get_mask_on_id(struct unit_module *m,
		struct gk20a *g, void *args)
{
	struct gpu_ops gops = g->ops;
	int ret = UNIT_FAIL;
	u32 mask;
	u32 branches;
	u32 engine_id = nvgpu_engine_get_gr_id(g);
	const char *labels[] = {
		"is_tsg",
		"load",
		"busy",
		"same_id",
		"same_type"
	};

	g->ops.engine_status.read_engine_status_info =
		stub_engine_read_engine_status_info;

	u.id = 0x0100;

	for (branches = 0U; branches < F_GET_MASK_LAST; branches++) {

		u32 id;
		u32 type;
		u32 expected_type;

		subtest_setup(branches);
		unit_verbose(m, "%s branches=%s\n", __func__,
			branches_str(branches, labels));

		u.is_tsg = ((branches & F_GET_MASK_IS_TSG) != 0);
		u.id++;

		expected_type = ENGINE_STATUS_CTX_ID_TYPE_INVALID;
		if (branches & F_GET_MASK_SAME_TYPE) {
			expected_type = branches & F_GET_MASK_IS_TSG ?
					ENGINE_STATUS_CTX_ID_TYPE_TSGID :
					ENGINE_STATUS_CTX_ID_TYPE_CHID;
		}
		nvgpu_engine_get_id_and_type(g, engine_id, &id, &type);
		unit_assert((id == u.id) ==
			((branches & F_GET_MASK_SAME_ID) != 0), goto done);
		unit_assert(type == expected_type, goto done);

		mask = nvgpu_engine_get_mask_on_id(g, u.id, u.is_tsg);

		if ((branches & F_GET_MASK_BUSY) &&
		    (branches & F_GET_MASK_SAME_ID) &&
		    (branches & F_GET_MASK_SAME_TYPE)) {
			unit_assert(mask = u.eng_mask, goto done);
		} else {
			unit_assert(mask == 0, goto done);
		}
	}

	ret = UNIT_SUCCESS;
done:
	if (ret != UNIT_SUCCESS) {
		unit_err(m, "%s branches=%s\n", __func__,
			branches_str(branches, labels));
	}
	g->ops = gops;
	return ret;
}

#define F_FIND_BUSY_CTXSW_IDLE 				BIT(0)
#define F_FIND_BUSY_CTXSW_LOAD				BIT(1)
#define F_FIND_BUSY_CTXSW_SWITCH_FECS_WFI_RESTORE	BIT(2)
#define F_FIND_BUSY_CTXSW_SWITCH_FECS_OTHER		BIT(3)
#define F_FIND_BUSY_CTXSW_SAVE				BIT(4)
#define F_FIND_BUSY_CTXSW_LAST				BIT(5)

static u32 stub_gr_falcon_read_fecs_ctxsw_mailbox(struct gk20a *g,
		u32 reg_index)
{
	if (u.branches & F_FIND_BUSY_CTXSW_SWITCH_FECS_WFI_RESTORE) {
		return FECS_METHOD_WFI_RESTORE;
	}
	return 0;
}

static void stub_engine_read_engine_status_info2(struct gk20a *g,
		u32 engine_id, struct nvgpu_engine_status_info *status)
{
	status->is_busy = ((u.branches & F_FIND_BUSY_CTXSW_IDLE) == 0);

	status->ctx_id = ENGINE_STATUS_CTX_ID_INVALID;
	status->ctx_id_type = ENGINE_STATUS_CTX_ID_TYPE_INVALID;

	status->ctx_next_id = ENGINE_STATUS_CTX_NEXT_ID_INVALID;
	status->ctx_next_id_type = ENGINE_STATUS_CTX_NEXT_ID_TYPE_INVALID;

	status->ctxsw_status = NVGPU_CTX_STATUS_VALID;

	if (u.branches & F_FIND_BUSY_CTXSW_LOAD) {
		status->ctxsw_status = NVGPU_CTX_STATUS_CTXSW_LOAD;
		status->ctx_next_id = u.id;
		status->ctx_next_id_type = ENGINE_STATUS_CTX_ID_TYPE_TSGID;
	}

	if (u.branches & F_FIND_BUSY_CTXSW_SWITCH_FECS_WFI_RESTORE) {
		status->ctxsw_status = NVGPU_CTX_STATUS_CTXSW_SWITCH;
		status->ctx_next_id = u.id;
		status->ctx_next_id_type = ENGINE_STATUS_CTX_ID_TYPE_TSGID;
	}

	if (u.branches & F_FIND_BUSY_CTXSW_SWITCH_FECS_OTHER) {
		status->ctxsw_status = NVGPU_CTX_STATUS_CTXSW_SWITCH;
		status->ctx_id = u.id;
		status->ctx_id_type = ENGINE_STATUS_CTX_ID_TYPE_TSGID;
	}

	if (u.branches & F_FIND_BUSY_CTXSW_SAVE) {
		status->ctxsw_status = NVGPU_CTX_STATUS_CTXSW_SAVE;
		status->ctx_id = u.id;
		status->ctx_id_type = ENGINE_STATUS_CTX_ID_TYPE_TSGID;
	}
}

int test_engine_find_busy_doing_ctxsw(struct unit_module *m,
		struct gk20a *g, void *args)
{
	struct gpu_ops gops = g->ops;
	struct nvgpu_fifo fifo = g->fifo;
	struct nvgpu_fifo *f = &g->fifo;
	int ret = UNIT_FAIL;
	u32 branches;
	u32 engine_id;
	const char *labels[] = {
		"idle",
		"load",
		"switch_fecs_restore",
		"switch_fecs_other",
		"save",
	};

	g->ops.gr.falcon.read_fecs_ctxsw_mailbox =
		stub_gr_falcon_read_fecs_ctxsw_mailbox;
	g->ops.engine_status.read_engine_status_info =
		stub_engine_read_engine_status_info2;
	f->num_engines = 1;

	u.id = 0x0100;

	for (branches = 0U; branches < F_FIND_BUSY_CTXSW_LAST; branches++) {

		u32 id;
		bool is_tsg;
		u32 count;

		count = __builtin_popcount(branches &
			(F_FIND_BUSY_CTXSW_LOAD |
			 F_FIND_BUSY_CTXSW_SWITCH_FECS_WFI_RESTORE |
			 F_FIND_BUSY_CTXSW_SWITCH_FECS_OTHER |
			 F_FIND_BUSY_CTXSW_SAVE));
		if (count > 1) {
			goto pruned;
		}

		if ((branches & F_FIND_BUSY_CTXSW_IDLE) &&
		    (branches & ~F_FIND_BUSY_CTXSW_IDLE)) {
pruned:
			unit_verbose(m, "%s branches=%s (pruned)\n",
				__func__, branches_str(branches, labels));
			continue;
		}

		subtest_setup(branches);
		unit_verbose(m, "%s branches=%s\n", __func__,
			branches_str(branches, labels));

		u.id++;

		is_tsg = false;
		engine_id = nvgpu_engine_find_busy_doing_ctxsw(g, &id, &is_tsg);

		if ((branches & F_FIND_BUSY_CTXSW_IDLE) || (count == 0)) {
			unit_assert(engine_id == NVGPU_INVALID_ENG_ID,
				goto done);
			unit_assert(id == NVGPU_INVALID_TSG_ID, goto done);
			unit_assert(!is_tsg, goto done);
		} else {
			unit_assert(engine_id != NVGPU_INVALID_ENG_ID,
				goto done);
			unit_assert(id == u.id, goto done);
			unit_assert(is_tsg, goto done);
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

static void stub_engine_read_engine_status_info_busy(struct gk20a *g,
		u32 engine_id, struct nvgpu_engine_status_info *status)
{
	status->is_busy = true;
}

static void stub_engine_read_engine_status_info_idle(struct gk20a *g,
		u32 engine_id, struct nvgpu_engine_status_info *status)
{
	status->is_busy = false;
}

int test_engine_get_runlist_busy_engines(struct unit_module *m,
		struct gk20a *g, void *args)
{
	struct gpu_ops gops = g->ops;
	struct nvgpu_fifo fifo = g->fifo;
	struct nvgpu_fifo *f = &g->fifo;
	int ret = UNIT_FAIL;
	struct nvgpu_engine_info engine_info;
	u32 engine_id = 0;
	u32 eng_mask;

	f->num_engines = 1;
	f->engine_info = &engine_info;
	f->active_engines_list = &engine_id;
	engine_info.engine_id = 1;
	engine_info.runlist_id = 3;
	g->ops.engine_status.read_engine_status_info =
		stub_engine_read_engine_status_info_busy;

	/* busy and same runlist_id (match found) */
	eng_mask = nvgpu_engine_get_runlist_busy_engines(g,
			engine_info.runlist_id);
	unit_assert(eng_mask == BIT32(engine_id), goto done);

	/* no entry with matching runlist_id */
	eng_mask = nvgpu_engine_get_runlist_busy_engines(g, 1);
	unit_assert(eng_mask == 0, goto done);

	/* no busy entry found */
	g->ops.engine_status.read_engine_status_info =
		stub_engine_read_engine_status_info_idle;
	eng_mask = nvgpu_engine_get_runlist_busy_engines(g,
			engine_info.runlist_id);
	unit_assert(eng_mask == 0, goto done);

	/* no entry at all */
	f->num_engines = 0;
	eng_mask = nvgpu_engine_get_runlist_busy_engines(g,
			engine_info.runlist_id);
	unit_assert(eng_mask == 0, goto done);

	ret = UNIT_SUCCESS;
done:
	g->ops = gops;
	g->fifo = fifo;
	return ret;
}

struct unit_module_test nvgpu_engine_tests[] = {
	UNIT_TEST(setup_sw, test_engine_setup_sw, &u, 2),
	UNIT_TEST(init_support, test_fifo_init_support, &u, 2),
	UNIT_TEST(init_info, test_engine_init_info, &u, 2),
	UNIT_TEST(ids, test_engine_ids, &u, 2),
	UNIT_TEST(get_active_eng_info, test_engine_get_active_eng_info, &u, 2),
	UNIT_TEST(interrupt_mask, test_engine_interrupt_mask, &u, 2),
	UNIT_TEST(get_fast_ce_runlist_id,
		test_engine_get_fast_ce_runlist_id, &u, 2),
	UNIT_TEST(get_gr_runlist_id,
		test_engine_get_gr_runlist_id, &u, 2),
	UNIT_TEST(is_valid_runlist_id,
		test_engine_is_valid_runlist_id, &u, 2),
	UNIT_TEST(mmu_fault_id, test_engine_mmu_fault_id, &u, 2),
	UNIT_TEST(mmu_fault_id_veid, test_engine_mmu_fault_id_veid, &u, 2),
	UNIT_TEST(get_mask_on_id, test_engine_get_mask_on_id, &u, 2),
	UNIT_TEST(status, test_engine_status, &u, 2),
	UNIT_TEST(find_busy_doing_ctxsw,
		test_engine_find_busy_doing_ctxsw, &u, 2),
	UNIT_TEST(get_runlist_busy_engines,
		test_engine_get_runlist_busy_engines, &u, 2),
	UNIT_TEST(remove_support, test_fifo_remove_support, &u, 2),
};

UNIT_MODULE(nvgpu_engine, nvgpu_engine_tests, UNIT_PRIO_NVGPU_TEST);
