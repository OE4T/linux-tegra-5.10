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

#include <unit/unit.h>
#include <unit/io.h>

#include <nvgpu/posix/io.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/hal_init.h>
#include <nvgpu/device.h>
#include <nvgpu/gr/gr.h>
#include <nvgpu/gr/gr_falcon.h>

#include "common/gr/gr_falcon_priv.h"

#include "hal/init/hal_gv11b.h"

#include "nvgpu-gr.h"
#include "nvgpu-gr-gv11b.h"

int test_gr_init_setup(struct unit_module *m, struct gk20a *g, void *args)
{
	int err;

	err = test_gr_setup_gv11b_reg_space(m, g);
	if (err != 0) {
		goto fail;
	}

	nvgpu_device_init(g);

	/*
	 * Allocate gr unit
	 */
	err = nvgpu_gr_alloc(g);
	if (err != 0) {
		unit_err(m, "Gr allocation failed\n");
		return -ENOMEM;
	}

	return UNIT_SUCCESS;

fail:
	return UNIT_FAIL;
}

static int test_gr_falcon_load_ctxsw_ucode(struct gk20a *g,
				struct nvgpu_gr_falcon *falcon)
{
	int err = 0;
	err = nvgpu_gr_falcon_init_ctxsw_ucode(g, falcon);
	if (err == 0) {
		falcon->skip_ucode_init = true;
	}

	return err;
}

int test_gr_init_prepare(struct unit_module *m, struct gk20a *g, void *args)
{
	int err;

	err = g->ops.ecc.ecc_init_support(g);
	if (err != 0) {
		unit_return_fail(m, "ecc init failed\n");
	}

	err = nvgpu_gr_prepare_sw(g);
	if (err != 0) {
		unit_return_fail(m, "nvgpu_gr_prepare_sw returned fail\n");
	}

	err = nvgpu_gr_enable_hw(g);
	if (err != 0) {
		unit_return_fail(m, "nvgpu_gr_enable_hw returned fail\n");
	}

	return UNIT_SUCCESS;
}

int test_gr_init_support(struct unit_module *m, struct gk20a *g, void *args)
{
	int err;

	g->ops.ecc.ecc_init_support(g);
	g->ops.ltc.init_ltc_support(g);
	g->ops.mm.init_mm_support(g);

	/* over-ride the falcon load_ctxsw_ucode */
	g->ops.gr.falcon.load_ctxsw_ucode = test_gr_falcon_load_ctxsw_ucode;

	/* init gpu characteristics */
	g->ops.chip_init_gpu_characteristics(g);

	err = nvgpu_gr_init_support(g);
	if (err != 0) {
		unit_return_fail(m, "nvgpu_gr_init_support returned fail\n");
	}

	g->ops.ecc.ecc_finalize_support(g);

	return UNIT_SUCCESS;
}

int test_gr_suspend(struct unit_module *m, struct gk20a *g, void *args)
{
	if (nvgpu_gr_suspend(g) != 0) {
		unit_return_fail(m, "nvgpu_gr_suspend returned fail\n");
	}

	return UNIT_SUCCESS;
}

int test_gr_init_setup_ready(struct unit_module *m,
		struct gk20a *g, void *args)
{
	int err = 0;

	g->fifo.g = g;
	nvgpu_device_init(g);
	nvgpu_fifo_setup_sw(g);

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

	nvgpu_ref_init(&g->refcount);
	nvgpu_gr_sw_ready(g, true);

	return UNIT_SUCCESS;
}

int test_gr_remove_support(struct unit_module *m, struct gk20a *g, void *args)
{
	if (g->ops.ecc.ecc_remove_support != NULL) {
		g->ops.ecc.ecc_remove_support(g);
	}

	nvgpu_gr_remove_support(g);

	return UNIT_SUCCESS;
}

int test_gr_remove_setup(struct unit_module *m,
		struct gk20a *g, void *args)
{
	test_gr_cleanup_gv11b_reg_space(m, g);
	nvgpu_gr_free(g);

	return UNIT_SUCCESS;
}

int test_gr_init_setup_cleanup(struct unit_module *m,
		struct gk20a *g, void *args)
{
	int err = 0;

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
