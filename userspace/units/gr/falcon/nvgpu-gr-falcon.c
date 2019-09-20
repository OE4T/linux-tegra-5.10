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
#include <nvgpu/gr/gr.h>
#include <nvgpu/gr/gr_falcon.h>

#include "common/gr/gr_priv.h"
#include "common/gr/gr_falcon_priv.h"
#include "hal/gr/falcon/gr_falcon_gm20b.h"

#include "../nvgpu-gr.h"

struct nvgpu_gr_falcon *unit_gr_falcon;

static void test_gr_falcon_bind_instblk(struct gk20a *g,
				struct nvgpu_mem *mem, u64 inst_ptr)
{
	/* Do nothing */
}

static void test_gr_falcon_load_ctxsw_ucode_header(struct gk20a *g,
	u32 reg_offset, u32 boot_signature, u32 addr_code32,
	u32 addr_data32, u32 code_size, u32 data_size)
{
	/* Do nothing */

}

static int test_gr_falcon_init(struct unit_module *m,
				struct gk20a *g, void *args)
{
	int err = 0;

	/* Allocate and Initialize GR */
	err = test_gr_init_setup(m, g, args);
	if (err != 0) {
		unit_return_fail(m, "gr init setup failed\n");
	}

	err = test_gr_init_prepare(m, g, args);
	if (err != 0) {
		unit_return_fail(m, "gr init prepare failed\n");
	}

	err = test_gr_init_support(m, g, args);
	if (err != 0) {
		unit_return_fail(m, "gr init support failed\n");
	}

	/* set up test specific HALs */
	g->ops.gr.falcon.load_ctxsw_ucode =
				nvgpu_gr_falcon_load_secure_ctxsw_ucode;
	g->ops.gr.falcon.load_ctxsw_ucode_header =
			test_gr_falcon_load_ctxsw_ucode_header;
	g->ops.gr.falcon.bind_instblk = test_gr_falcon_bind_instblk;

	unit_gr_falcon = nvgpu_gr_falcon_init_support(g);
	if (unit_gr_falcon == NULL) {
		unit_return_fail(m, "nvgpu_gr_falcon_init_support failed\n");
	}

	return UNIT_SUCCESS;
}

static int test_gr_falcon_init_ctxsw(struct unit_module *m,
				struct gk20a *g, void *args)
{
	int err = 0;

	err = nvgpu_gr_falcon_init_ctxsw(g, unit_gr_falcon);
	if (err) {
		unit_return_fail(m, "nvgpu_gr_falcon_init_ctxsw failed\n");
	}

	return UNIT_SUCCESS;
}

static int test_gr_falcon_nonsecure_gpccs_init_ctxsw(struct unit_module *m,
				struct gk20a *g, void *args)
{
	int err = 0;

	nvgpu_set_enabled(g, NVGPU_SEC_SECUREGPCCS, false);
	err = nvgpu_gr_falcon_init_ctxsw(g, unit_gr_falcon);
	if (err) {
		unit_return_fail(m, "nvgpu_gr_falcon_init_ctxsw failed\n");
	}
	nvgpu_set_enabled(g, NVGPU_SEC_SECUREGPCCS, true);

	return UNIT_SUCCESS;
}

static int test_gr_falcon_recovery_ctxsw(struct unit_module *m,
				struct gk20a *g, void *args)
{
	int err = 0;

	err = nvgpu_gr_falcon_init_ctxsw(g, unit_gr_falcon);
	/* Recovey expected to fail */
	if (err == 0) {
		unit_return_fail(m,
			"test_gr_falcon_init_recovery_ctxsw failed\n");
	}

	return UNIT_SUCCESS;
}

static int test_gr_falcon_nonsecure_gpccs_recovery_ctxsw(struct unit_module *m,
				struct gk20a *g, void *args)
{
	int err = 0;

	nvgpu_set_enabled(g, NVGPU_SEC_SECUREGPCCS, false);

	err = nvgpu_gr_falcon_init_ctxsw(g, unit_gr_falcon);
	if (err) {
		unit_return_fail(m, "nvgpu_gr_falcon_init_ctxsw failed\n");
	}
	nvgpu_set_enabled(g, NVGPU_SEC_SECUREGPCCS, true);

	return UNIT_SUCCESS;
}

static int test_gr_falcon_query_test(struct unit_module *m,
				struct gk20a *g, void *args)
{

	struct nvgpu_mutex *fecs_mutex =
		nvgpu_gr_falcon_get_fecs_mutex(unit_gr_falcon);
	struct nvgpu_ctxsw_ucode_segments *fecs =
		nvgpu_gr_falcon_get_fecs_ucode_segments(unit_gr_falcon);
	struct nvgpu_ctxsw_ucode_segments *gpccs =
		nvgpu_gr_falcon_get_gpccs_ucode_segments(unit_gr_falcon);
	void *cpu_va = nvgpu_gr_falcon_get_surface_desc_cpu_va(unit_gr_falcon);

	if (fecs_mutex == NULL) {
		unit_return_fail(m, "nvgpu_gr_falcon_get_fecs_mutex failed\n");
	}

	if (fecs == NULL) {
		unit_return_fail(m,
			"nvgpu_gr_falcon_get_fecs_ucode_segments failed\n");
	}

	if (gpccs == NULL) {
		unit_return_fail(m,
			"nvgpu_gr_falcon_get_gpccs_ucode_segments failed\n");
	}

	unit_info(m, "nvgpu_gr_falcon_get_surface_desc_cpu_va %p\n",
								cpu_va);

	return UNIT_SUCCESS;

}

static int test_gr_falcon_init_ctx_state(struct unit_module *m,
				struct gk20a *g, void *args)
{
	int err = 0;

	err = nvgpu_gr_falcon_init_ctx_state(g, unit_gr_falcon);
	if (err) {
		unit_return_fail(m, "nvgpu_gr_falcon_init_ctx_state failed\n");
	}

	return UNIT_SUCCESS;
}

static int test_gr_falcon_deinit(struct unit_module *m,
				struct gk20a *g, void *args)
{
	int err = 0;

	if (unit_gr_falcon != NULL) {
		nvgpu_gr_falcon_remove_support(g, unit_gr_falcon);
	}

	/* Cleanup GR */
	err = test_gr_remove_support(m, g, args);
	if (err != 0) {
		unit_return_fail(m, "gr remove support failed\n");
	}

	err = test_gr_remove_setup(m, g, args);
	if (err != 0) {
		unit_return_fail(m, "gr remove setup failed\n");
	}

	return UNIT_SUCCESS;
}

struct unit_module_test nvgpu_gr_falcon_tests[] = {
	UNIT_TEST(gr_falcon_init, test_gr_falcon_init, NULL, 0),
	UNIT_TEST(gr_falcon_init_ctxsw, test_gr_falcon_init_ctxsw, NULL, 0),
	UNIT_TEST(gr_falcon_nonsecure_gpccs_init_ctxsw,
		test_gr_falcon_nonsecure_gpccs_init_ctxsw, NULL, 0),
	UNIT_TEST(gr_falcon_recovery_ctxsw,
				test_gr_falcon_recovery_ctxsw, NULL, 0),
	UNIT_TEST(gr_falcon_nonsecure_gpccs_recovery_ctxsw,
			test_gr_falcon_nonsecure_gpccs_recovery_ctxsw, NULL, 0),
	UNIT_TEST(gr_falcon_query_test,
			test_gr_falcon_query_test, NULL, 0),
	UNIT_TEST(gr_falcon_init_ctx_state,
			test_gr_falcon_init_ctx_state, NULL, 0),
	UNIT_TEST(gr_falcon_deinit, test_gr_falcon_deinit, NULL, 0),
};


UNIT_MODULE(nvgpu_gr_falcon, nvgpu_gr_falcon_tests, UNIT_PRIO_NVGPU_TEST);
