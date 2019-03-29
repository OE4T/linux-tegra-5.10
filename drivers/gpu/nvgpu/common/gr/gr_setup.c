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

#include <nvgpu/log.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/gr/ctx.h>
#include <nvgpu/gr/zcull.h>
#include <nvgpu/gr/setup.h>
#include <nvgpu/channel.h>

static int nvgpu_gr_setup_zcull(struct gk20a *g, struct channel_gk20a *c,
				struct nvgpu_gr_ctx *gr_ctx)
{
	int ret = 0;

	nvgpu_log_fn(g, " ");

	ret = gk20a_disable_channel_tsg(g, c);
	if (ret != 0) {
		nvgpu_err(g, "failed to disable channel/TSG");
		return ret;
	}

	ret = gk20a_fifo_preempt(g, c);
	if (ret != 0) {
		if (gk20a_enable_channel_tsg(g, c) != 0) {
			nvgpu_err(g, "failed to re-enable channel/TSG");
		}
		nvgpu_err(g, "failed to preempt channel/TSG");
		return ret;
	}

	ret = nvgpu_gr_zcull_ctx_setup(g, c->subctx, gr_ctx);
	if (ret != 0) {
		nvgpu_err(g, "failed to setup zcull");
	}

	ret = gk20a_enable_channel_tsg(g, c);
	if (ret != 0) {
		nvgpu_err(g, "failed to enable channel/TSG");
	}

	return ret;
}

int nvgpu_gr_setup_bind_ctxsw_zcull(struct gk20a *g, struct channel_gk20a *c,
			u64 zcull_va, u32 mode)
{
	struct tsg_gk20a *tsg;
	struct nvgpu_gr_ctx *gr_ctx;

	tsg = tsg_gk20a_from_ch(c);
	if (tsg == NULL) {
		return -EINVAL;
	}

	gr_ctx = tsg->gr_ctx;
	nvgpu_gr_ctx_set_zcull_ctx(g, gr_ctx, mode, zcull_va);

	return nvgpu_gr_setup_zcull(g, c, gr_ctx);
}
