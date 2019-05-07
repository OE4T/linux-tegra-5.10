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
#include <nvgpu/mm.h>
#include <nvgpu/pmu/pmu_pg.h>
#include <nvgpu/gr/ctx.h>
#include <nvgpu/gr/subctx.h>
#include <nvgpu/gr/global_ctx.h>
#include <nvgpu/gr/obj_ctx.h>
#include <nvgpu/gr/config.h>
#include <nvgpu/netlist.h>
#include <nvgpu/gr/gr_falcon.h>
#include <nvgpu/gr/fs_state.h>
#include <nvgpu/power_features/cg.h>

#include "obj_ctx_priv.h"
#include "gr_priv.h"

void nvgpu_gr_obj_ctx_commit_inst_gpu_va(struct gk20a *g,
	struct nvgpu_mem *inst_block, u64 gpu_va)
{
	g->ops.ramin.set_gr_ptr(g, inst_block, gpu_va);
}

void nvgpu_gr_obj_ctx_commit_inst(struct gk20a *g, struct nvgpu_mem *inst_block,
	struct nvgpu_gr_ctx *gr_ctx, struct nvgpu_gr_subctx *subctx, u64 gpu_va)
{
	struct nvgpu_mem *ctxheader;

	nvgpu_log_fn(g, " ");

	if (nvgpu_is_enabled(g, NVGPU_SUPPORT_TSG_SUBCONTEXTS)) {
		nvgpu_gr_subctx_load_ctx_header(g, subctx, gr_ctx, gpu_va);

		ctxheader = nvgpu_gr_subctx_get_ctx_header(g, subctx);
		nvgpu_gr_obj_ctx_commit_inst_gpu_va(g, inst_block,
			ctxheader->gpu_va);
	} else {
		nvgpu_gr_obj_ctx_commit_inst_gpu_va(g, inst_block, gpu_va);
	}
}

static int nvgpu_gr_obj_ctx_init_ctxsw_preemption_mode(struct gk20a *g,
	struct nvgpu_gr_config *config, struct nvgpu_gr_ctx_desc *gr_ctx_desc,
	struct nvgpu_gr_ctx *gr_ctx, struct vm_gk20a *vm,
	u32 class_num, u32 flags)
{
	int err;
	u32 graphics_preempt_mode = 0;
	u32 compute_preempt_mode = 0;

	nvgpu_log_fn(g, " ");

	if (!nvgpu_is_enabled(g, NVGPU_SUPPORT_PREEMPTION_GFXP)) {
		if (g->ops.gpu_class.is_valid_compute(class_num)) {
			nvgpu_gr_ctx_init_compute_preemption_mode(gr_ctx,
				NVGPU_PREEMPTION_MODE_COMPUTE_CTA);
		}

		return 0;
	}

	if ((flags & NVGPU_OBJ_CTX_FLAGS_SUPPORT_GFXP) != 0U) {
		graphics_preempt_mode = NVGPU_PREEMPTION_MODE_GRAPHICS_GFXP;
	}
	if ((flags & NVGPU_OBJ_CTX_FLAGS_SUPPORT_CILP) != 0U) {
		compute_preempt_mode = NVGPU_PREEMPTION_MODE_COMPUTE_CILP;
	}

	if ((graphics_preempt_mode != 0U) || (compute_preempt_mode != 0U)) {
		err = nvgpu_gr_obj_ctx_set_ctxsw_preemption_mode(g, config,
			gr_ctx_desc, gr_ctx, vm, class_num, graphics_preempt_mode,
			compute_preempt_mode);
		if (err != 0) {
			nvgpu_err(g, "set_ctxsw_preemption_mode failed");
			return err;
		}
	}

	nvgpu_log_fn(g, "done");

	return 0;
}

int nvgpu_gr_obj_ctx_set_ctxsw_preemption_mode(struct gk20a *g,
	struct nvgpu_gr_config *config, struct nvgpu_gr_ctx_desc *gr_ctx_desc,
	struct nvgpu_gr_ctx *gr_ctx, struct vm_gk20a *vm, u32 class_num,
	u32 graphics_preempt_mode, u32 compute_preempt_mode)
{
	int err = 0;

	if (!nvgpu_is_enabled(g, NVGPU_SUPPORT_PREEMPTION_GFXP)) {
		return 0;
	}

	if (g->ops.gpu_class.is_valid_gfx(class_num) &&
			nvgpu_gr_ctx_desc_force_preemption_gfxp(gr_ctx_desc)) {
		graphics_preempt_mode = NVGPU_PREEMPTION_MODE_GRAPHICS_GFXP;
	}

	if (g->ops.gpu_class.is_valid_compute(class_num) &&
			nvgpu_gr_ctx_desc_force_preemption_cilp(gr_ctx_desc)) {
		compute_preempt_mode = NVGPU_PREEMPTION_MODE_COMPUTE_CILP;
	}

	/* check for invalid combinations */
	if (nvgpu_gr_ctx_check_valid_preemption_mode(gr_ctx,
			graphics_preempt_mode, compute_preempt_mode) == false) {
		return -EINVAL;
	}

	/* set preemption modes */
	switch (graphics_preempt_mode) {
	case NVGPU_PREEMPTION_MODE_GRAPHICS_GFXP:
		{
		u32 rtv_cb_size;
		u32 spill_size = g->ops.gr.init.get_ctx_spill_size(g);
		u32 pagepool_size = g->ops.gr.init.get_ctx_pagepool_size(g);
		u32 betacb_size = g->ops.gr.init.get_ctx_betacb_size(g);
		u32 attrib_cb_size =
			g->ops.gr.init.get_ctx_attrib_cb_size(g, betacb_size,
				nvgpu_gr_config_get_tpc_count(config),
				nvgpu_gr_config_get_max_tpc_count(config));

		nvgpu_log_info(g, "gfxp context spill_size=%d", spill_size);
		nvgpu_log_info(g, "gfxp context pagepool_size=%d", pagepool_size);
		nvgpu_log_info(g, "gfxp context attrib_cb_size=%d",
				attrib_cb_size);

		nvgpu_gr_ctx_set_size(gr_ctx_desc,
			NVGPU_GR_CTX_SPILL_CTXSW, spill_size);
		nvgpu_gr_ctx_set_size(gr_ctx_desc,
			NVGPU_GR_CTX_BETACB_CTXSW, attrib_cb_size);
		nvgpu_gr_ctx_set_size(gr_ctx_desc,
			NVGPU_GR_CTX_PAGEPOOL_CTXSW, pagepool_size);

		if (g->ops.gr.init.get_gfxp_rtv_cb_size != NULL) {
			rtv_cb_size = g->ops.gr.init.get_gfxp_rtv_cb_size(g);
			nvgpu_gr_ctx_set_size(gr_ctx_desc,
				NVGPU_GR_CTX_GFXP_RTVCB_CTXSW, rtv_cb_size);
		}

		err = nvgpu_gr_ctx_alloc_ctxsw_buffers(g, gr_ctx,
			gr_ctx_desc, vm);
		if (err != 0) {
			nvgpu_err(g, "cannot allocate ctxsw buffers");
			goto fail;
		}

		nvgpu_gr_ctx_init_graphics_preemption_mode(gr_ctx,
			graphics_preempt_mode);
		break;
		}

	case NVGPU_PREEMPTION_MODE_GRAPHICS_WFI:
		nvgpu_gr_ctx_init_graphics_preemption_mode(gr_ctx,
			graphics_preempt_mode);
		break;

	default:
		nvgpu_log_info(g, "graphics_preempt_mode=%u",
			graphics_preempt_mode);
		break;
	}

	if (g->ops.gpu_class.is_valid_compute(class_num) ||
			g->ops.gpu_class.is_valid_gfx(class_num)) {
		switch (compute_preempt_mode) {
		case NVGPU_PREEMPTION_MODE_COMPUTE_WFI:
		case NVGPU_PREEMPTION_MODE_COMPUTE_CTA:
		case NVGPU_PREEMPTION_MODE_COMPUTE_CILP:
			nvgpu_gr_ctx_init_compute_preemption_mode(gr_ctx,
				compute_preempt_mode);
			break;
		default:
			nvgpu_log_info(g, "compute_preempt_mode=%u",
				compute_preempt_mode);
			break;
		}
	}

	return 0;

fail:
	return err;
}

void nvgpu_gr_obj_ctx_update_ctxsw_preemption_mode(struct gk20a *g,
	struct nvgpu_gr_config *config,
	struct nvgpu_gr_ctx *gr_ctx, struct nvgpu_gr_subctx *subctx)
{
	int err;
	u64 addr;
	u32 size;
	struct nvgpu_mem *mem;

	nvgpu_log_fn(g, " ");

	nvgpu_gr_ctx_set_preemption_modes(g, gr_ctx);

	if (!nvgpu_is_enabled(g, NVGPU_SUPPORT_PREEMPTION_GFXP)) {
		return;
	}

	if (!nvgpu_mem_is_valid(
			nvgpu_gr_ctx_get_preempt_ctxsw_buffer(gr_ctx))) {
		return;
	}

	if (subctx != NULL) {
		nvgpu_gr_subctx_set_preemption_buffer_va(g, subctx,
			gr_ctx);
	} else {
		nvgpu_gr_ctx_set_preemption_buffer_va(g, gr_ctx);
	}

	err = nvgpu_gr_ctx_patch_write_begin(g, gr_ctx, true);
	if (err != 0) {
		nvgpu_err(g, "can't map patch context");
		goto out;
	}

	addr = nvgpu_gr_ctx_get_betacb_ctxsw_buffer(gr_ctx)->gpu_va;
	g->ops.gr.init.commit_global_attrib_cb(g, gr_ctx,
		nvgpu_gr_config_get_tpc_count(config),
		nvgpu_gr_config_get_max_tpc_count(config), addr,
		true);

	mem = nvgpu_gr_ctx_get_pagepool_ctxsw_buffer(gr_ctx);
	addr = mem->gpu_va;
	nvgpu_assert(mem->size <= U32_MAX);
	size = (u32)mem->size;

	g->ops.gr.init.commit_global_pagepool(g, gr_ctx, addr, size,
		true, false);

	mem = nvgpu_gr_ctx_get_spill_ctxsw_buffer(gr_ctx);
	addr = mem->gpu_va;
	nvgpu_assert(mem->size <= U32_MAX);
	size = (u32)mem->size;

	g->ops.gr.init.commit_ctxsw_spill(g, gr_ctx, addr, size, true);

	g->ops.gr.init.commit_cbes_reserve(g, gr_ctx, true);

	if (g->ops.gr.init.gfxp_wfi_timeout != NULL) {
		g->ops.gr.init.gfxp_wfi_timeout(g, gr_ctx, true);
	}

	if (g->ops.gr.init.commit_gfxp_rtv_cb != NULL) {
		g->ops.gr.init.commit_gfxp_rtv_cb(g, gr_ctx, true);
	}

	nvgpu_gr_ctx_patch_write_end(g, gr_ctx, true);

out:
	nvgpu_log_fn(g, "done");
}

int nvgpu_gr_obj_ctx_commit_global_ctx_buffers(struct gk20a *g,
	struct nvgpu_gr_global_ctx_buffer_desc *global_ctx_buffer,
	struct nvgpu_gr_config *config,	struct nvgpu_gr_ctx *gr_ctx, bool patch)
{
	u64 addr;
	size_t size;

	nvgpu_log_fn(g, " ");

	if (patch) {
		int err;
		err = nvgpu_gr_ctx_patch_write_begin(g, gr_ctx, false);
		if (err != 0) {
			return err;
		}
	}

	/* global pagepool buffer */
	addr = nvgpu_gr_ctx_get_global_ctx_va(gr_ctx, NVGPU_GR_CTX_PAGEPOOL_VA);
	size = nvgpu_gr_global_ctx_get_size(global_ctx_buffer,
			NVGPU_GR_GLOBAL_CTX_PAGEPOOL);

	g->ops.gr.init.commit_global_pagepool(g, gr_ctx, addr, size, patch,
		true);

	/* global bundle cb */
	addr = nvgpu_gr_ctx_get_global_ctx_va(gr_ctx, NVGPU_GR_CTX_CIRCULAR_VA);
	size = g->ops.gr.init.get_bundle_cb_default_size(g);

	g->ops.gr.init.commit_global_bundle_cb(g, gr_ctx, addr, size, patch);

	/* global attrib cb */
	addr = nvgpu_gr_ctx_get_global_ctx_va(gr_ctx,
			NVGPU_GR_CTX_ATTRIBUTE_VA);

	g->ops.gr.init.commit_global_attrib_cb(g, gr_ctx,
		nvgpu_gr_config_get_tpc_count(config),
		nvgpu_gr_config_get_max_tpc_count(config), addr, patch);

	g->ops.gr.init.commit_global_cb_manager(g, config, gr_ctx, patch);

	if (g->ops.gr.init.commit_rtv_cb != NULL) {
		/* RTV circular buffer */
		addr = nvgpu_gr_ctx_get_global_ctx_va(gr_ctx,
			NVGPU_GR_CTX_RTV_CIRCULAR_BUFFER_VA);

		g->ops.gr.init.commit_rtv_cb(g, addr, gr_ctx, patch);
	}

	if (patch) {
		nvgpu_gr_ctx_patch_write_end(g, gr_ctx, false);
	}

	return 0;
}

static int nvgpu_gr_obj_ctx_alloc_sw_bundle(struct gk20a *g)
{
	struct netlist_av_list *sw_bundle_init =
			nvgpu_netlist_get_sw_bundle_init_av_list(g);
	struct netlist_av_list *sw_veid_bundle_init =
			nvgpu_netlist_get_sw_veid_bundle_init_av_list(g);
	struct netlist_av64_list *sw_bundle64_init =
			nvgpu_netlist_get_sw_bundle64_init_av64_list(g);
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

/*
 * init global golden image from a fresh gr_ctx in channel ctx.
 * save a copy in local_golden_image in ctx_vars
 */
int nvgpu_gr_obj_ctx_alloc_golden_ctx_image(struct gk20a *g,
	struct nvgpu_gr_obj_ctx_golden_image *golden_image,
	struct nvgpu_gr_global_ctx_buffer_desc *global_ctx_buffer,
	struct nvgpu_gr_config *config,
	struct nvgpu_gr_ctx *gr_ctx,
	struct nvgpu_mem *inst_block)
{
	u32 i;
	u64 size;
	struct nvgpu_mem *gr_mem;
	int err = 0;
	struct netlist_aiv_list *sw_ctx_load =
				nvgpu_netlist_get_sw_ctx_load_aiv_list(g);
	struct netlist_av_list *sw_method_init =
				nvgpu_netlist_get_sw_method_init_av_list(g);
	u32 data;

	nvgpu_log_fn(g, " ");

	gr_mem = nvgpu_gr_ctx_get_ctx_mem(gr_ctx);

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

	data = g->ops.gr.falcon.get_fecs_current_ctx_data(g, inst_block);
	err = g->ops.gr.falcon.ctrl_ctxsw(g,
			NVGPU_GR_FALCON_METHOD_ADDRESS_BIND_PTR, data, NULL);
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
		err = g->ops.gr.init.preemption_state(g);
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

	err = nvgpu_gr_obj_ctx_commit_global_ctx_buffers(g, global_ctx_buffer,
		config, gr_ctx, false);
	if (err != 0) {
		goto clean_up;
	}

	/* override a few ctx state registers */
	g->ops.gr.init.commit_global_timeslice(g);

	/* floorsweep anything left */
	err = nvgpu_gr_fs_state_init(g, config);
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

	data = g->ops.gr.falcon.get_fecs_current_ctx_data(g, inst_block);
	err = g->ops.gr.falcon.ctrl_ctxsw(g,
			NVGPU_GR_FALCON_METHOD_GOLDEN_IMAGE_SAVE, data, NULL);
	if (err != 0) {
		goto clean_up;
	}

	size = nvgpu_gr_obj_ctx_get_golden_image_size(golden_image);

	golden_image->local_golden_image =
		nvgpu_gr_global_ctx_init_local_golden_image(g, gr_mem, size);
	if (golden_image->local_golden_image == NULL) {
		err = -ENOMEM;
		goto clean_up;
	}

	golden_image->ready = true;

	nvgpu_pmu_set_golden_image_initialized(g, true);
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
	struct nvgpu_gr_obj_ctx_golden_image *golden_image,
	struct nvgpu_gr_ctx_desc *gr_ctx_desc, struct nvgpu_gr_ctx *gr_ctx,
	struct vm_gk20a *vm)
{
	u64 size;
	int err = 0;

	nvgpu_log_fn(g, " ");

	size = nvgpu_gr_obj_ctx_get_golden_image_size(golden_image);
	nvgpu_assert(size <= U64(U32_MAX));
	nvgpu_gr_ctx_set_size(gr_ctx_desc, NVGPU_GR_CTX_CTX, U32(size));

	err = nvgpu_gr_ctx_alloc(g, gr_ctx, gr_ctx_desc, vm);
	if (err != 0) {
		return err;
	}

	return 0;
}

int nvgpu_gr_obj_ctx_alloc(struct gk20a *g,
	struct nvgpu_gr_obj_ctx_golden_image *golden_image,
	struct nvgpu_gr_global_ctx_buffer_desc *global_ctx_buffer,
	struct nvgpu_gr_ctx_desc *gr_ctx_desc,
	struct nvgpu_gr_config *config,
	struct nvgpu_gr_ctx *gr_ctx,
	struct nvgpu_gr_subctx *subctx,
	struct vm_gk20a *vm,
	struct nvgpu_mem *inst_block,
	u32 class_num, u32 flags,
	bool cde, bool vpr)
{
	int err = 0;

	nvgpu_log_fn(g, " ");

	err = nvgpu_gr_obj_ctx_gr_ctx_alloc(g, golden_image, gr_ctx_desc,
		gr_ctx, vm);
	if (err != 0) {
		nvgpu_err(g, "fail to allocate TSG gr ctx buffer");
		goto out;
	}

	/* allocate patch buffer */
	if (!nvgpu_mem_is_valid(nvgpu_gr_ctx_get_patch_ctx_mem(gr_ctx))) {
		nvgpu_gr_ctx_set_patch_ctx_data_count(gr_ctx, 0);

		nvgpu_gr_ctx_set_size(gr_ctx_desc,
			NVGPU_GR_CTX_PATCH_CTX,
			g->ops.gr.init.get_patch_slots(g, config) *
				PATCH_CTX_SLOTS_REQUIRED_PER_ENTRY);

		err = nvgpu_gr_ctx_alloc_patch_ctx(g, gr_ctx, gr_ctx_desc, vm);
		if (err != 0) {
			nvgpu_err(g, "fail to allocate patch buffer");
			goto out;
		}
	}

	err = nvgpu_gr_obj_ctx_init_ctxsw_preemption_mode(g, config,
		gr_ctx_desc, gr_ctx, vm, class_num, flags);
	if (err != 0) {
		nvgpu_err(g, "fail to init preemption mode");
		goto out;
	}

	/* map global buffer to channel gpu_va and commit */
	err = nvgpu_gr_ctx_map_global_ctx_buffers(g, gr_ctx,
			global_ctx_buffer, vm, vpr);
	if (err != 0) {
		nvgpu_err(g, "fail to map global ctx buffer");
		goto out;
	}

	err = nvgpu_gr_obj_ctx_commit_global_ctx_buffers(g, global_ctx_buffer,
			config, gr_ctx, true);
	if (err != 0) {
		nvgpu_err(g, "fail to commit global ctx buffer");
		goto out;
	}

	/* commit gr ctx buffer */
	nvgpu_gr_obj_ctx_commit_inst(g, inst_block, gr_ctx, subctx,
			nvgpu_gr_ctx_get_ctx_mem(gr_ctx)->gpu_va);

	/* init golden image, ELPG enabled after this is done */
	err = nvgpu_gr_obj_ctx_alloc_golden_ctx_image(g, golden_image,
		global_ctx_buffer, config, gr_ctx, inst_block);
	if (err != 0) {
		nvgpu_err(g, "fail to init golden ctx image");
		goto out;
	}

	/* load golden image */
	err = nvgpu_gr_ctx_load_golden_ctx_image(g, gr_ctx,
		golden_image->local_golden_image, cde);
	if (err != 0) {
		nvgpu_err(g, "fail to load golden ctx image");
		goto out;
	}

	nvgpu_gr_obj_ctx_update_ctxsw_preemption_mode(g, config, gr_ctx,
		subctx);

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

bool nvgpu_gr_obj_ctx_is_golden_image_ready(
	struct nvgpu_gr_obj_ctx_golden_image *golden_image)
{
	bool ready;

	nvgpu_mutex_acquire(&golden_image->ctx_mutex);
	ready = golden_image->ready;
	nvgpu_mutex_release(&golden_image->ctx_mutex);

	return ready;
}

int nvgpu_gr_obj_ctx_init(struct gk20a *g,
	struct nvgpu_gr_obj_ctx_golden_image **gr_golden_image, u32 size)
{
	struct nvgpu_gr_obj_ctx_golden_image *golden_image;
	int err;

	golden_image = nvgpu_kzalloc(g, sizeof(*golden_image));
	if (golden_image == NULL) {
		return -ENOMEM;
	}

	nvgpu_gr_obj_ctx_set_golden_image_size(golden_image, size);

	err = nvgpu_mutex_init(&golden_image->ctx_mutex);
	if (err != 0) {
		nvgpu_kfree(g, golden_image);
		return err;
	}

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

	nvgpu_pmu_set_golden_image_initialized(g, false);
	golden_image->ready = false;
	nvgpu_kfree(g, golden_image);
}

