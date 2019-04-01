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

#include <nvgpu/gk20a.h>
#include <nvgpu/log.h>
#include <nvgpu/io.h>
#include <nvgpu/gr/ctx.h>
#include <nvgpu/gr/global_ctx.h>
#include <nvgpu/gr/obj_ctx.h>
#include <nvgpu/power_features/cg.h>

#include "obj_ctx_priv.h"

/*
 * TODO: needed for nvgpu_gr_init_fs_state() and introduces cyclic dependency
 * with common.gr.gr unit. Remove this in follow up
 */
#include <nvgpu/gr/gr.h>

/*
 * TODO: remove these when nvgpu_gr_obj_ctx_bind_channel() and
 * nvgpu_gr_obj_ctx_image_save() are moved to appropriate units
 */
#include <nvgpu/hw/gk20a/hw_gr_gk20a.h>
#include <nvgpu/hw/gk20a/hw_ram_gk20a.h>

static int nvgpu_gr_obj_ctx_alloc_sw_bundle(struct gk20a *g)
{
	struct netlist_av_list *sw_bundle_init =
			&g->netlist_vars->sw_bundle_init;
	struct netlist_av_list *sw_veid_bundle_init =
			&g->netlist_vars->sw_veid_bundle_init;
	struct netlist_av64_list *sw_bundle64_init =
			&g->netlist_vars->sw_bundle64_init;
	int err = 0;

	/* enable pipe mode override */
	g->ops.gr.init.pipe_mode_override(g, true);

	/* load bundle init */
	err = g->ops.gr.init.load_sw_bundle_init(g, sw_bundle_init);
	if (err != 0) {
		goto error;
	}

	if (g->ops.gr.init.load_sw_veid_bundle != NULL) {
		err = g->ops.gr.init.load_sw_veid_bundle(g,
				sw_veid_bundle_init);
		if (err != 0) {
			goto error;
		}
	}

	if (g->ops.gr.init.load_sw_bundle64 != NULL) {
		err = g->ops.gr.init.load_sw_bundle64(g, sw_bundle64_init);
		if (err != 0) {
			goto error;
		}
	}

	/* disable pipe mode override */
	g->ops.gr.init.pipe_mode_override(g, false);

	err = g->ops.gr.init.wait_idle(g);

	return err;

error:
	/* in case of error skip waiting for GR idle - just restore state */
	g->ops.gr.init.pipe_mode_override(g, false);

	return err;
}

static int nvgpu_gr_obj_ctx_bind_channel(struct gk20a *g,
		struct nvgpu_mem *inst_block)
{
	u32 inst_base_ptr = u64_lo32(nvgpu_inst_block_addr(g, inst_block)
				     >> ram_in_base_shift_v());
	u32 data = fecs_current_ctx_data(g, inst_block);
	int ret;

	nvgpu_log_info(g, "bind inst ptr 0x%08x", inst_base_ptr);

	ret = g->ops.gr.falcon.submit_fecs_method_op(g,
		     (struct fecs_method_op_gk20a) {
		     .method.addr = gr_fecs_method_push_adr_bind_pointer_v(),
		     .method.data = data,
		     .mailbox = { .id = 0, .data = 0,
				  .clr = 0x30,
				  .ret = NULL,
				  .ok = 0x10,
				  .fail = 0x20, },
		     .cond.ok = GR_IS_UCODE_OP_AND,
		     .cond.fail = GR_IS_UCODE_OP_AND}, true);
	if (ret != 0) {
		nvgpu_err(g,
			"bind channel instance failed");
	}

	return ret;
}

static int nvgpu_gr_obj_ctx_image_save(struct gk20a *g,
		struct nvgpu_mem *inst_block)
{
	int ret;

	nvgpu_log_fn(g, " ");

	ret = g->ops.gr.falcon.submit_fecs_method_op(g,
		(struct fecs_method_op_gk20a) {
		.method.addr = gr_fecs_method_push_adr_wfi_golden_save_v(),
		.method.data = fecs_current_ctx_data(g, inst_block),
		.mailbox = {.id = 0, .data = 0, .clr = 3, .ret = NULL,
			.ok = 1, .fail = 2,
		},
		.cond.ok = GR_IS_UCODE_OP_AND,
		.cond.fail = GR_IS_UCODE_OP_AND,
		 }, true);

	if (ret != 0) {
		nvgpu_err(g, "save context image failed");
	}

	return ret;
}

/*
 * init global golden image from a fresh gr_ctx in channel ctx.
 * save a copy in local_golden_image in ctx_vars
 */
int nvgpu_gr_obj_ctx_alloc_golden_ctx_image(struct gk20a *g,
	struct nvgpu_gr_obj_ctx_golden_image *golden_image,
	struct nvgpu_gr_ctx *gr_ctx,
	struct nvgpu_mem *inst_block)
{
	u32 i;
	struct nvgpu_mem *gr_mem;
	int err = 0;
	struct netlist_aiv_list *sw_ctx_load = &g->netlist_vars->sw_ctx_load;
	struct netlist_av_list *sw_method_init = &g->netlist_vars->sw_method_init;

	nvgpu_log_fn(g, " ");

	gr_mem = &gr_ctx->mem;

	/*
	 * golden ctx is global to all channels. Although only the first
	 * channel initializes golden image, driver needs to prevent multiple
	 * channels from initializing golden ctx at the same time
	 */
	nvgpu_mutex_acquire(&golden_image->ctx_mutex);

	if (golden_image->ready) {
		goto clean_up;
	}

	err = g->ops.gr.init.fe_pwr_mode_force_on(g, true);
	if (err != 0) {
		goto clean_up;
	}

	g->ops.gr.init.override_context_reset(g);

	err = g->ops.gr.init.fe_pwr_mode_force_on(g, false);
	if (err != 0) {
		goto clean_up;
	}

	err = nvgpu_gr_obj_ctx_bind_channel(g, inst_block);
	if (err != 0) {
		goto clean_up;
	}

	err = g->ops.gr.init.wait_idle(g);

	/* load ctx init */
	for (i = 0U; i < sw_ctx_load->count; i++) {
		nvgpu_writel(g, sw_ctx_load->l[i].addr,
			     sw_ctx_load->l[i].value);
	}

	if (g->ops.gr.init.preemption_state != NULL) {
		err = g->ops.gr.init.preemption_state(g,
			g->gr.gfxp_wfi_timeout_count,
			g->gr.gfxp_wfi_timeout_unit_usec);
		if (err != 0) {
			goto clean_up;
		}
	}

	nvgpu_cg_blcg_gr_load_enable(g);

	err = g->ops.gr.init.wait_idle(g);
	if (err != 0) {
		goto clean_up;
	}

	/* disable fe_go_idle */
	g->ops.gr.init.fe_go_idle_timeout(g, false);

	err = g->ops.gr.commit_global_ctx_buffers(g, gr_ctx, false);
	if (err != 0) {
		goto clean_up;
	}

	/* override a few ctx state registers */
	g->ops.gr.init.commit_global_timeslice(g);

	/* floorsweep anything left */
	err = nvgpu_gr_init_fs_state(g);
	if (err != 0) {
		goto clean_up;
	}

	err = g->ops.gr.init.wait_idle(g);
	if (err != 0) {
		goto restore_fe_go_idle;
	}

	err = nvgpu_gr_obj_ctx_alloc_sw_bundle(g);
	if (err != 0) {
		goto clean_up;
	}

restore_fe_go_idle:
	/* restore fe_go_idle */
	g->ops.gr.init.fe_go_idle_timeout(g, true);

	if ((err != 0) || (g->ops.gr.init.wait_idle(g) != 0)) {
		goto clean_up;
	}

	/* load method init */
	g->ops.gr.init.load_method_init(g, sw_method_init);

	err = g->ops.gr.init.wait_idle(g);
	if (err != 0) {
		goto clean_up;
	}

	err = nvgpu_gr_ctx_init_zcull(g, gr_ctx);
	if (err != 0) {
		goto clean_up;
	}

	nvgpu_gr_obj_ctx_image_save(g, inst_block);

	golden_image->local_golden_image =
		nvgpu_gr_global_ctx_init_local_golden_image(g, gr_mem,
			g->gr.ctx_vars.golden_image_size);
	if (golden_image->local_golden_image == NULL) {
		err = -ENOMEM;
		goto clean_up;
	}

	golden_image->ready = true;
	g->gr.ctx_vars.golden_image_initialized = true;

	g->ops.gr.falcon.set_current_ctx_invalid(g);

clean_up:
	if (err != 0) {
		nvgpu_err(g, "fail");
	} else {
		nvgpu_log_fn(g, "done");
	}

	nvgpu_mutex_release(&golden_image->ctx_mutex);
	return err;
}

static int nvgpu_gr_obj_ctx_gr_ctx_alloc(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx, struct vm_gk20a *vm)
{
	struct gr_gk20a *gr = &g->gr;
	u32 size;
	int err = 0;

	nvgpu_log_fn(g, " ");

	size = nvgpu_gr_obj_ctx_get_golden_image_size(g->gr.golden_image);
	nvgpu_gr_ctx_set_size(gr->gr_ctx_desc, NVGPU_GR_CTX_CTX, size);

	err = nvgpu_gr_ctx_alloc(g, gr_ctx, gr->gr_ctx_desc, vm);
	if (err != 0) {
		return err;
	}

	return 0;
}

int nvgpu_gr_obj_ctx_alloc(struct gk20a *g,
	struct nvgpu_gr_obj_ctx_golden_image *golden_image,
	struct nvgpu_gr_global_ctx_buffer_desc *global_ctx_buffer,
	struct nvgpu_gr_ctx *gr_ctx,
	struct nvgpu_gr_subctx *subctx,
	struct channel_gk20a *c,
	struct vm_gk20a *vm,
	struct nvgpu_mem *inst_block,
	u32 class_num, u32 flags,
	bool cde, bool vpr)
{
	int err = 0;

	nvgpu_log_fn(g, " ");

	err = nvgpu_gr_obj_ctx_gr_ctx_alloc(g, gr_ctx, vm);
	if (err != 0) {
		nvgpu_err(g,
			"fail to allocate TSG gr ctx buffer");
		goto out;
	}

	/* allocate patch buffer */
	if (!nvgpu_mem_is_valid(&gr_ctx->patch_ctx.mem)) {
		gr_ctx->patch_ctx.data_count = 0;

		nvgpu_gr_ctx_set_size(g->gr.gr_ctx_desc,
			NVGPU_GR_CTX_PATCH_CTX,
			g->ops.gr.get_patch_slots(g) *
				PATCH_CTX_SLOTS_REQUIRED_PER_ENTRY);

		err = nvgpu_gr_ctx_alloc_patch_ctx(g, gr_ctx,
			g->gr.gr_ctx_desc, vm);
		if (err != 0) {
			nvgpu_err(g,
				"fail to allocate patch buffer");
			goto out;
		}
	}

	g->ops.gr.init_ctxsw_preemption_mode(g, gr_ctx, vm, class_num, flags);

	/* map global buffer to channel gpu_va and commit */
	err = nvgpu_gr_ctx_map_global_ctx_buffers(g, gr_ctx,
			global_ctx_buffer, vm, vpr);
	if (err != 0) {
		nvgpu_err(g,
			"fail to map global ctx buffer");
		goto out;
	}

	g->ops.gr.commit_global_ctx_buffers(g, gr_ctx, true);

	/* commit gr ctx buffer */
	err = g->ops.gr.commit_inst(c, gr_ctx->mem.gpu_va);
	if (err != 0) {
		nvgpu_err(g,
			"fail to commit gr ctx buffer");
		goto out;
	}

	/* init golden image, ELPG enabled after this is done */
	err = nvgpu_gr_obj_ctx_alloc_golden_ctx_image(g, golden_image, gr_ctx,
		inst_block);
	if (err != 0) {
		nvgpu_err(g,
			"fail to init golden ctx image");
		goto out;
	}

	/* load golden image */
	nvgpu_gr_ctx_load_golden_ctx_image(g, gr_ctx,
		golden_image->local_golden_image, cde);
	if (err != 0) {
		nvgpu_err(g,
			"fail to load golden ctx image");
		goto out;
	}

	if (g->ops.gr.update_ctxsw_preemption_mode != NULL) {
		g->ops.gr.update_ctxsw_preemption_mode(g, gr_ctx,
			subctx);
	}

	nvgpu_log_fn(g, "done");
	return 0;
out:
	/*
	 * 1. gr_ctx, patch_ctx and global ctx buffer mapping
	 * can be reused so no need to release them.
	 * 2. golden image init and load is a one time thing so if
	 * they pass, no need to undo.
	 */
	nvgpu_err(g, "fail");
	return err;
}

void nvgpu_gr_obj_ctx_set_golden_image_size(
		struct nvgpu_gr_obj_ctx_golden_image *golden_image,
		size_t size)
{
	golden_image->size = size;
}

size_t nvgpu_gr_obj_ctx_get_golden_image_size(
		struct nvgpu_gr_obj_ctx_golden_image *golden_image)
{
	return golden_image->size;
}

u32 *nvgpu_gr_obj_ctx_get_local_golden_image_ptr(
	struct nvgpu_gr_obj_ctx_golden_image *golden_image)
{
	return nvgpu_gr_global_ctx_get_local_golden_image_ptr(
			golden_image->local_golden_image);
}

int nvgpu_gr_obj_ctx_init(struct gk20a *g,
	struct nvgpu_gr_obj_ctx_golden_image **gr_golden_image, u32 size)
{
	struct nvgpu_gr_obj_ctx_golden_image *golden_image;

	golden_image = nvgpu_kzalloc(g, sizeof(*golden_image));
	if (golden_image == NULL) {
		return -ENOMEM;
	}

	nvgpu_gr_obj_ctx_set_golden_image_size(golden_image, size);
	nvgpu_mutex_init(&golden_image->ctx_mutex);

	*gr_golden_image = golden_image;

	return 0;
}

void nvgpu_gr_obj_ctx_deinit(struct gk20a *g,
	struct nvgpu_gr_obj_ctx_golden_image *golden_image)
{
	if (golden_image->local_golden_image != NULL) {
		nvgpu_gr_global_ctx_deinit_local_golden_image(g,
			golden_image->local_golden_image);
		golden_image->local_golden_image = NULL;
	}

	golden_image->ready = false;
	nvgpu_kfree(g, golden_image);
}

