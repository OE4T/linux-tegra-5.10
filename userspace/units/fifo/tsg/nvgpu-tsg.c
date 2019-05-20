/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/tsg.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/fifo/userd.h>

#include "hal/fifo/tsg_gk20a.h"

#include "hal/init/hal_gv11b.h"

#include "nvgpu-fifo-gv11b.h"

struct test_tsg_args {
	bool init_done;
	struct nvgpu_tsg *tsg;
};

struct test_tsg_args test_args = {
	.init_done = false,
};

/* test implementations of some hals */
static u32 test_gv11b_gr_init_get_no_of_sm(struct gk20a *g)
{
	return 8;
}

#ifdef NVGPU_USERD
static int stub_userd_setup_sw(struct gk20a *g)
{
	struct nvgpu_fifo *f = &g->fifo;
	int err;

	f->userd_entry_size = g->ops.userd.entry_size(g);

	err = nvgpu_userd_init_slabs(g);
	if (err != 0) {
		nvgpu_err(g, "failed to init userd support");
		return err;
	}

	return 0;
}
#endif

static int test_fifo_init_support(struct unit_module *m,
		struct gk20a *g, void *args)
{
	struct test_tsg_args *t = args;
	int err;

	if (t->init_done) {
		unit_return_fail(m, "init already done");
	}

	err = test_fifo_setup_gv11b_reg_space(m, g);
	if (err != 0) {
		goto fail;
	}

	/*
	 * set IS_FMODEL to avoid reading fuses
	 * TODO: add fuses reg space to avoid this
	 */
	nvgpu_set_enabled(g, NVGPU_IS_FMODEL, true);
	gv11b_init_hal(g);
	g->ops.fifo.init_fifo_setup_hw = NULL;
	nvgpu_set_enabled(g, NVGPU_IS_FMODEL, false);

#ifdef NVGPU_USERD
	/*
	 * Regular USERD init requires bar1.vm to be initialized
	 * Use a stub in unit tests, since it will be disabled in
	 * safety build anyway.
	 */
	g->ops.userd.setup_sw = stub_userd_setup_sw;
#endif

	err = nvgpu_fifo_init_support(g);
	if (err != 0) {
		test_fifo_cleanup_gv11b_reg_space(m, g);
		goto fail;
	}

	t->init_done = true;

	return UNIT_SUCCESS;

fail:
	return UNIT_FAIL;
}

static int test_tsg_open(struct unit_module *m,
		struct gk20a *g, void *args)
{
	struct test_tsg_args *t = args;

	g->ops.gr.init.get_no_of_sm = test_gv11b_gr_init_get_no_of_sm;
	g->ops.tsg.init_eng_method_buffers = NULL;

	t->tsg = nvgpu_tsg_open(g, getpid());
	if (!t->tsg) {
		unit_return_fail(m, "nvgpu_tsg_open failed");
	}

	return UNIT_SUCCESS;
}

static int test_tsg_release(struct unit_module *m,
		struct gk20a *g, void *args)
{
	struct test_tsg_args *t = args;

	if (t->tsg == NULL) {
		unit_return_fail(m, "tsg in NULL");
	}

	nvgpu_ref_put(&t->tsg->refcount, nvgpu_tsg_release);
	return UNIT_SUCCESS;
}

static int test_fifo_remove_support(struct unit_module *m,
		struct gk20a *g, void *args)
{
	struct test_tsg_args *t = args;

	if (!t->init_done) {
		unit_return_fail(m, "missing init support");
	}

	if (g->fifo.remove_support) {
		g->fifo.remove_support(&g->fifo);
	}

	return UNIT_SUCCESS;
}

struct unit_module_test nvgpu_tsg_tests[] = {
	UNIT_TEST(init_support, test_fifo_init_support, &test_args, 0),
	UNIT_TEST(open, test_tsg_open, &test_args, 0),
	UNIT_TEST(release, test_tsg_release, &test_args, 0),
	UNIT_TEST(remove_support, test_fifo_remove_support, &test_args, 0),
};

UNIT_MODULE(nvgpu_tsg, nvgpu_tsg_tests, UNIT_PRIO_NVGPU_TEST);
