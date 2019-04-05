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
#include <nvgpu/gr/subctx.h>
#include <nvgpu/gr/obj_ctx.h>
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

int nvgpu_gr_setup_alloc_obj_ctx(struct channel_gk20a *c, u32 class_num,
		u32 flags)
{
	struct gk20a *g = c->g;
	struct nvgpu_gr_ctx *gr_ctx;
	struct tsg_gk20a *tsg = NULL;
	int err = 0;

	nvgpu_log_fn(g, " ");

	/* an address space needs to have been bound at this point.*/
	if (!gk20a_channel_as_bound(c) && (c->vm == NULL)) {
		nvgpu_err(g,
			   "not bound to address space at time"
			   " of grctx allocation");
		return -EINVAL;
	}

	if (!g->ops.gr.is_valid_class(g, class_num)) {
		nvgpu_err(g,
			   "invalid obj class 0x%x", class_num);
		err = -EINVAL;
		goto out;
	}
	c->obj_class = class_num;

	tsg = tsg_gk20a_from_ch(c);
	if (tsg == NULL) {
		return -EINVAL;
	}

	gr_ctx = tsg->gr_ctx;

	if (nvgpu_is_enabled(g, NVGPU_SUPPORT_TSG_SUBCONTEXTS)) {
		if (c->subctx == NULL) {
			c->subctx = nvgpu_gr_subctx_alloc(g, c->vm);
			if (c->subctx == NULL) {
				err = -ENOMEM;
				goto out;
			}
		}
	}

	if (!nvgpu_mem_is_valid(nvgpu_gr_ctx_get_ctx_mem(gr_ctx))) {
		tsg->vm = c->vm;
		nvgpu_vm_get(tsg->vm);

		err = nvgpu_gr_obj_ctx_alloc(g, g->gr.golden_image,
				g->gr.global_ctx_buffer, gr_ctx, c->subctx,
				tsg->vm, &c->inst_block, class_num, flags,
				c->cde, c->vpr);
		if (err != 0) {
			nvgpu_err(g,
				"failed to allocate gr ctx buffer");
			nvgpu_vm_put(tsg->vm);
			tsg->vm = NULL;
			goto out;
		}

		nvgpu_gr_ctx_set_tsgid(gr_ctx, tsg->tsgid);
	} else {
		/* commit gr ctx buffer */
		nvgpu_gr_obj_ctx_commit_inst(g, &c->inst_block, gr_ctx,
			c->subctx, nvgpu_gr_ctx_get_ctx_mem(gr_ctx)->gpu_va);
	}

#ifdef CONFIG_GK20A_CTXSW_TRACE
	if (g->ops.gr.fecs_trace.bind_channel && !c->vpr) {
		err = g->ops.gr.fecs_trace.bind_channel(g, &c->inst_block,
			c->subctx, gr_ctx, tsg->tgid, 0);
		if (err != 0) {
			nvgpu_warn(g,
				"fail to bind channel for ctxsw trace");
		}
	}
#endif

	nvgpu_log_fn(g, "done");
	return 0;
out:
	if (c->subctx != NULL) {
		nvgpu_gr_subctx_free(g, c->subctx, c->vm);
	}

	/* 1. gr_ctx, patch_ctx and global ctx buffer mapping
	   can be reused so no need to release them.
	   2. golden image init and load is a one time thing so if
	   they pass, no need to undo. */
	nvgpu_err(g, "fail");
	return err;
}

void nvgpu_gr_setup_free_gr_ctx(struct gk20a *g,
		struct vm_gk20a *vm, struct nvgpu_gr_ctx *gr_ctx)
{
	nvgpu_log_fn(g, " ");

	if (gr_ctx != NULL) {
		if ((g->ops.gr.ctxsw_prog.dump_ctxsw_stats != NULL) &&
		     g->gr.ctx_vars.dump_ctxsw_stats_on_channel_close) {
			g->ops.gr.ctxsw_prog.dump_ctxsw_stats(g,
				 nvgpu_gr_ctx_get_ctx_mem(gr_ctx));
		}

		nvgpu_gr_ctx_free(g, gr_ctx, g->gr.global_ctx_buffer, vm);
	}
}

int nvgpu_gr_setup_set_preemption_mode(struct channel_gk20a *ch,
					u32 graphics_preempt_mode,
					u32 compute_preempt_mode)
{
	struct nvgpu_gr_ctx *gr_ctx;
	struct gk20a *g = ch->g;
	struct tsg_gk20a *tsg;
	struct vm_gk20a *vm;
	u32 class;
	int err = 0;

	class = ch->obj_class;
	if (class == 0U) {
		return -EINVAL;
	}

	tsg = tsg_gk20a_from_ch(ch);
	if (tsg == NULL) {
		return -EINVAL;
	}

	vm = tsg->vm;
	gr_ctx = tsg->gr_ctx;

	/* skip setting anything if both modes are already set */
	if ((graphics_preempt_mode != 0U) &&
		(graphics_preempt_mode ==
			nvgpu_gr_ctx_get_graphics_preemption_mode(gr_ctx))) {
		graphics_preempt_mode = 0;
	}

	if ((compute_preempt_mode != 0U) &&
	    (compute_preempt_mode ==
		    nvgpu_gr_ctx_get_compute_preemption_mode(gr_ctx))) {
		compute_preempt_mode = 0;
	}

	if ((graphics_preempt_mode == 0U) && (compute_preempt_mode == 0U)) {
		return 0;
	}

	nvgpu_log(g, gpu_dbg_sched, "chid=%d tsgid=%d pid=%d "
			"graphics_preempt=%d compute_preempt=%d",
			ch->chid,
			ch->tsgid,
			ch->tgid,
			graphics_preempt_mode,
			compute_preempt_mode);
	err = nvgpu_gr_obj_ctx_set_ctxsw_preemption_mode(g, gr_ctx, vm, class,
					graphics_preempt_mode, compute_preempt_mode);
	if (err != 0) {
		nvgpu_err(g, "set_ctxsw_preemption_mode failed");
		return err;
	}

	err = gk20a_disable_channel_tsg(g, ch);
	if (err != 0) {
		return err;
	}

	err = gk20a_fifo_preempt(g, ch);
	if (err != 0) {
		goto enable_ch;
	}

	nvgpu_gr_obj_ctx_update_ctxsw_preemption_mode(ch->g, gr_ctx, ch->subctx);

	err = nvgpu_gr_ctx_patch_write_begin(g, gr_ctx, true);
	if (err != 0) {
		nvgpu_err(g, "can't map patch context");
		goto enable_ch;
	}
	g->ops.gr.init.commit_global_cb_manager(g, g->gr.config, gr_ctx,
		true);
	nvgpu_gr_ctx_patch_write_end(g, gr_ctx, true);

enable_ch:
	gk20a_enable_channel_tsg(g, ch);
	return err;
}
