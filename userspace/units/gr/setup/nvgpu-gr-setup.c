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
#include <unistd.h>

#include <unit/unit.h>
#include <unit/io.h>

#include <nvgpu/posix/io.h>
#include <nvgpu/types.h>
#include <nvgpu/gk20a.h>

#include <nvgpu/channel.h>
#include <nvgpu/runlist.h>
#include <nvgpu/tsg.h>
#include <nvgpu/class.h>
#include <nvgpu/falcon.h>

#include <nvgpu/gr/gr.h>
#include <nvgpu/gr/ctx.h>

#include <nvgpu/hw/gv11b/hw_gr_gv11b.h>

#include "common/gr/gr_priv.h"
#include "common/gr/obj_ctx_priv.h"

#include "../nvgpu-gr.h"
#include "nvgpu-gr-setup.h"

static struct nvgpu_channel *gr_setup_ch;
static struct nvgpu_tsg *gr_setup_tsg;

static u32 stub_channel_count(struct gk20a *g)
{
	return 4;
}

static int stub_runlist_update_for_channel(struct gk20a *g, u32 runlist_id,
					   struct nvgpu_channel *ch,
					   bool add, bool wait_for_finish)
{
	return 0;
}

static int stub_mm_l2_flush(struct gk20a *g, bool invalidate)
{
	return 0;
}

static int stub_gr_init_fe_pwr_mode(struct gk20a *g, bool force_on)
{
	return 0;
}

static int stub_gr_init_wait_idle(struct gk20a *g)
{
	return 0;
}

static int stub_gr_falcon_ctrl_ctxsw(struct gk20a *g, u32 fecs_method,
				     u32 data, u32 *ret_val)
{
	return 0;
}

static int gr_test_setup_unbind_tsg(struct unit_module *m, struct gk20a *g)
{
	int err = 0;

	if ((gr_setup_ch == NULL) || (gr_setup_tsg == NULL)) {
		goto unbind_tsg;
	}

	err = nvgpu_tsg_unbind_channel(gr_setup_tsg, gr_setup_ch);
	if (err != 0) {
		unit_err(m, "failed tsg channel unbind\n");
	}

unbind_tsg:
	return (err == 0) ? UNIT_SUCCESS: UNIT_FAIL;
}

static void gr_test_setup_cleanup_ch_tsg(struct unit_module *m,
					 struct gk20a *g)
{
	if (gr_setup_ch != NULL) {
		nvgpu_channel_close(gr_setup_ch);
	}

	if (gr_setup_tsg != NULL) {
		nvgpu_ref_put(&gr_setup_tsg->refcount, nvgpu_tsg_release);
	}

	gr_setup_tsg = NULL;
	gr_setup_ch = NULL;
}

static int gr_test_setup_allocate_ch_tsg(struct unit_module *m,
					 struct gk20a *g)
{
	u32 tsgid = getpid();
	struct nvgpu_channel *ch = NULL;
	struct nvgpu_tsg *tsg = NULL;
        struct gk20a_as_share *as_share = NULL;
	int err;

	err = nvgpu_channel_setup_sw(g);
	if (err != 0) {
		unit_return_fail(m, "failed channel setup\n");
	}

	err = nvgpu_tsg_setup_sw(g);
	if (err != 0) {
		unit_return_fail(m, "failed tsg setup\n");
	}

	tsg = nvgpu_tsg_open(g, tsgid);
	if (tsg == NULL) {
		unit_return_fail(m, "failed tsg open\n");
	}

	ch = nvgpu_channel_open_new(g, NVGPU_INVALID_RUNLIST_ID,
				false, tsgid, tsgid);
	if (ch == NULL) {
		unit_err(m, "failed channel open\n");
		goto ch_cleanup;
	}

	err = nvgpu_tsg_bind_channel(tsg, ch);
	if (err != 0) {
		unit_err(m, "failed tsg channel bind\n");
		goto ch_cleanup;
	}

	err = gk20a_as_alloc_share(g, 0, 0, &as_share);
	if (err != 0) {
		unit_err(m, "failed vm memory alloc\n");
		goto tsg_unbind;
	}

	err = g->ops.mm.vm_bind_channel(as_share->vm, ch);
	if (err != 0) {
		unit_err(m, "failed vm binding to ch\n");
		goto tsg_unbind;
	}

	gr_setup_ch = ch;
	gr_setup_tsg = tsg;

	goto ch_alloc_end;

tsg_unbind:
	gr_test_setup_unbind_tsg(m, g);

ch_cleanup:
	gr_test_setup_cleanup_ch_tsg(m, g);

ch_alloc_end:
	return (err == 0) ? UNIT_SUCCESS: UNIT_FAIL;
}

int test_gr_setup_set_preemption_mode(struct unit_module *m,
				      struct gk20a *g, void *args)
{
	int err;

	if (gr_setup_ch == NULL) {
		unit_return_fail(m, "failed setup with valid channel\n");
	}

	err = g->ops.gr.setup.set_preemption_mode(gr_setup_ch, 0,
		NVGPU_PREEMPTION_MODE_COMPUTE_CTA);
	if (err != 0) {
		unit_return_fail(m, "setup preemption_mode failed\n");
	}

	return UNIT_SUCCESS;
}

int test_gr_setup_free_obj_ctx(struct unit_module *m,
			       struct gk20a *g, void *args)
{
	int err = 0;

	err = gr_test_setup_unbind_tsg(m, g);

	gr_test_setup_cleanup_ch_tsg(m, g);

	return (err == 0) ? UNIT_SUCCESS: UNIT_FAIL;
}

int test_gr_setup_alloc_obj_ctx(struct unit_module *m,
				struct gk20a *g, void *args)
{
	u32 tsgid = getpid();
	int err;
	struct nvgpu_fifo *f = &g->fifo;

	nvgpu_posix_io_writel_reg_space(g, gr_fecs_current_ctx_r(),
							tsgid);

	g->ops.channel.count = stub_channel_count;
	g->ops.runlist.update_for_channel = stub_runlist_update_for_channel;

	/* Disable those function which need register update in timeout loop */
	g->ops.mm.cache.l2_flush = stub_mm_l2_flush;
	g->ops.gr.init.fe_pwr_mode_force_on = stub_gr_init_fe_pwr_mode;
	g->ops.gr.init.wait_idle = stub_gr_init_wait_idle;
	g->ops.gr.falcon.ctrl_ctxsw = stub_gr_falcon_ctrl_ctxsw;

	if (f != NULL) {
		f->g = g;
	}

	/* Set a default size for golden image */
	g->gr->golden_image->size = 0x800;

	/* Test with channel and tsg */
	err = gr_test_setup_allocate_ch_tsg(m, g);
	if (err != 0) {
		unit_return_fail(m, "setup channel allocation failed\n");
	}

	err = g->ops.gr.setup.alloc_obj_ctx(gr_setup_ch, VOLTA_COMPUTE_A, 0);
	if (err != 0) {
		unit_return_fail(m, "setup alloc ob as current_ctx\n");
	}

	return UNIT_SUCCESS;
}

struct unit_module_test nvgpu_gr_setup_tests[] = {
	UNIT_TEST(gr_setup_setup, test_gr_init_setup_ready, NULL, 0),
	UNIT_TEST(gr_setup_alloc_obj_ctx, test_gr_setup_alloc_obj_ctx, NULL, 0),
	UNIT_TEST(gr_setup_set_preemption_mode, test_gr_setup_set_preemption_mode, NULL, 0),
	UNIT_TEST(gr_setup_free_obj_ctx, test_gr_setup_free_obj_ctx, NULL, 0),
	UNIT_TEST(gr_setup_cleanup, test_gr_init_setup_cleanup, NULL, 0),
};

UNIT_MODULE(nvgpu_gr_setup, nvgpu_gr_setup_tests, UNIT_PRIO_NVGPU_TEST);
