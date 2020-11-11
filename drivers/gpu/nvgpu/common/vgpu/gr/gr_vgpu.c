/*
 * Virtualized GPU Graphics
 *
 * Copyright (c) 2014-2020, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/kmem.h>
#include <nvgpu/bug.h>
#include <nvgpu/dma.h>
#include <nvgpu/error_notifier.h>
#include <nvgpu/dma.h>
#include <nvgpu/debugger.h>
#include <nvgpu/vgpu/vgpu_ivc.h>
#include <nvgpu/vgpu/vgpu.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/channel.h>
#include <nvgpu/tsg.h>
#include <nvgpu/string.h>
#include <nvgpu/gr/global_ctx.h>
#include <nvgpu/gr/gr.h>
#include <nvgpu/gr/ctx.h>
#include <nvgpu/gr/config.h>
#include <nvgpu/gr/gr_intr.h>
#include <nvgpu/gr/gr_falcon.h>
#ifdef CONFIG_NVGPU_GRAPHICS
#include <nvgpu/gr/zbc.h>
#include <nvgpu/gr/zcull.h>
#endif
#include <nvgpu/gr/fecs_trace.h>
#include <nvgpu/gr/hwpm_map.h>
#include <nvgpu/gr/obj_ctx.h>
#include <nvgpu/cyclestats_snapshot.h>
#include <nvgpu/power_features/pg.h>

#include "gr_vgpu.h"
#include "ctx_vgpu.h"
#include "subctx_vgpu.h"

#include "common/vgpu/perf/cyclestats_snapshot_vgpu.h"
#include "common/vgpu/ivc/comm_vgpu.h"

#include "common/gr/gr_config_priv.h"
#include "common/gr/gr_falcon_priv.h"
#include "common/gr/gr_intr_priv.h"
#include "common/gr/ctx_priv.h"
#ifdef CONFIG_NVGPU_GRAPHICS
#include "common/gr/zcull_priv.h"
#include "common/gr/zbc_priv.h"
#endif
#include "common/gr/gr_priv.h"

static int vgpu_gr_set_ctxsw_preemption_mode(struct gk20a *g,
				struct nvgpu_gr_ctx *gr_ctx,
				struct vm_gk20a *vm, u32 class,
				u32 graphics_preempt_mode,
				u32 compute_preempt_mode);
static int vgpu_gr_init_ctxsw_preemption_mode(struct gk20a *g,
				struct nvgpu_gr_ctx *gr_ctx,
				struct vm_gk20a *vm,
				u32 class,
				u32 flags);

void vgpu_gr_detect_sm_arch(struct gk20a *g)
{
	struct vgpu_priv_data *priv = vgpu_get_priv_data(g);

	nvgpu_log_fn(g, " ");

	g->params.sm_arch_sm_version =
			priv->constants.sm_arch_sm_version;
	g->params.sm_arch_spa_version =
			priv->constants.sm_arch_spa_version;
	g->params.sm_arch_warp_count =
			priv->constants.sm_arch_warp_count;
}

static int vgpu_gr_commit_inst(struct nvgpu_channel *c, u64 gpu_va)
{
	struct tegra_vgpu_cmd_msg msg;
	struct tegra_vgpu_ch_ctx_params *p = &msg.params.ch_ctx;
	int err;
	struct gk20a *g = c->g;

	nvgpu_log_fn(g, " ");

	if (nvgpu_is_enabled(g, NVGPU_SUPPORT_TSG_SUBCONTEXTS)) {
		err = vgpu_alloc_subctx_header(g, &c->subctx, c->vm,
					c->virt_ctx);
		if (err != 0) {
			return err;
		}
	}

	msg.cmd = TEGRA_VGPU_CMD_CHANNEL_COMMIT_GR_CTX;
	msg.handle = vgpu_get_handle(c->g);
	p->handle = c->virt_ctx;
	err = vgpu_comm_sendrecv(&msg, sizeof(msg), sizeof(msg));

	if (err || msg.ret) {
		if (nvgpu_is_enabled(g, NVGPU_SUPPORT_TSG_SUBCONTEXTS)) {
			vgpu_free_subctx_header(g, c->subctx, c->vm,
					c->virt_ctx);
		}
		return -1;
	} else {
		return 0;
	}
}

static int vgpu_gr_commit_global_ctx_buffers(struct gk20a *g,
					struct nvgpu_channel *c, bool patch)
{
	struct tegra_vgpu_cmd_msg msg;
	struct tegra_vgpu_ch_ctx_params *p = &msg.params.ch_ctx;
	int err;

	nvgpu_log_fn(g, " ");

	msg.cmd = TEGRA_VGPU_CMD_CHANNEL_COMMIT_GR_GLOBAL_CTX;
	msg.handle = vgpu_get_handle(g);
	p->handle = c->virt_ctx;
	err = vgpu_comm_sendrecv(&msg, sizeof(msg), sizeof(msg));

	return (err || msg.ret) ? -1 : 0;
}

int vgpu_gr_init_ctx_state(struct gk20a *g,
		struct nvgpu_gr_falcon_query_sizes *sizes)
{
	struct vgpu_priv_data *priv = vgpu_get_priv_data(g);

	nvgpu_log_fn(g, " ");

	sizes->golden_image_size = priv->constants.golden_ctx_size;
	sizes->pm_ctxsw_image_size = priv->constants.hwpm_ctx_size;
	if (!sizes->golden_image_size ||
		!sizes->pm_ctxsw_image_size) {
		return -ENXIO;
	}

#ifdef CONFIG_NVGPU_GRAPHICS
	sizes->zcull_image_size = priv->constants.zcull_ctx_size;
	if (sizes->zcull_image_size == 0U) {
		return -ENXIO;
	}
#endif

	sizes->preempt_image_size =
			priv->constants.preempt_ctx_size;
	if (!sizes->preempt_image_size) {
		return -EINVAL;
	}

	return 0;
}

int vgpu_gr_alloc_global_ctx_buffers(struct gk20a *g)
{
	struct nvgpu_gr *gr = g->gr;
	u32 size;

	nvgpu_log_fn(g, " ");

	gr->global_ctx_buffer = nvgpu_gr_global_ctx_desc_alloc(g);
	if (gr->global_ctx_buffer == NULL) {
		return -ENOMEM;
	}

	size = g->ops.gr.init.get_global_ctx_cb_buffer_size(g);
	nvgpu_log_info(g, "cb_buffer_size : %d", size);

	nvgpu_gr_global_ctx_set_size(gr->global_ctx_buffer,
		NVGPU_GR_GLOBAL_CTX_CIRCULAR, size);

	size = g->ops.gr.init.get_global_ctx_pagepool_buffer_size(g);
	nvgpu_log_info(g, "pagepool_buffer_size : %d", size);

	nvgpu_gr_global_ctx_set_size(gr->global_ctx_buffer,
		NVGPU_GR_GLOBAL_CTX_PAGEPOOL, size);

	size = g->ops.gr.init.get_global_attr_cb_size(g,
			nvgpu_gr_config_get_tpc_count(g->gr->config),
			nvgpu_gr_config_get_max_tpc_count(g->gr->config));
	nvgpu_log_info(g, "attr_buffer_size : %u", size);

	nvgpu_gr_global_ctx_set_size(gr->global_ctx_buffer,
		NVGPU_GR_GLOBAL_CTX_ATTRIBUTE, size);

	if (g->ops.gr.init.get_rtv_cb_size != NULL) {
		size = g->ops.gr.init.get_rtv_cb_size(g);
		nvgpu_log_info(g, "rtv_circular_buffer_size : %u", size);

		nvgpu_gr_global_ctx_set_size(gr->global_ctx_buffer,
			NVGPU_GR_GLOBAL_CTX_RTV_CIRCULAR_BUFFER, size);
	}

	size = NVGPU_GR_GLOBAL_CTX_PRIV_ACCESS_MAP_SIZE;
	nvgpu_log_info(g, "priv_access_map_size : %d", size);

	nvgpu_gr_global_ctx_set_size(gr->global_ctx_buffer,
		NVGPU_GR_GLOBAL_CTX_PRIV_ACCESS_MAP, size);

#ifdef CONFIG_NVGPU_FECS_TRACE
	size = nvgpu_gr_fecs_trace_buffer_size(g);
	nvgpu_log_info(g, "fecs_trace_buffer_size : %d", size);

	nvgpu_gr_global_ctx_set_size(gr->global_ctx_buffer,
		NVGPU_GR_GLOBAL_CTX_FECS_TRACE_BUFFER, size);
#endif
	return 0;
}

int vgpu_gr_alloc_obj_ctx(struct nvgpu_channel  *c, u32 class_num, u32 flags)
{
	struct gk20a *g = c->g;
	struct nvgpu_gr_ctx *gr_ctx = NULL;
	struct nvgpu_gr *gr = g->gr;
	struct nvgpu_tsg *tsg = NULL;
	int err = 0;

	nvgpu_log_fn(g, " ");

	/* an address space needs to have been bound at this point.*/
	if (!nvgpu_channel_as_bound(c)) {
		nvgpu_err(g, "not bound to address space at time"
			   " of grctx allocation");
		return -EINVAL;
	}

	if (!g->ops.gpu_class.is_valid(class_num)) {
		nvgpu_err(g, "invalid obj class 0x%x", class_num);
		err = -EINVAL;
		goto out;
	}
	c->obj_class = class_num;

	tsg = nvgpu_tsg_from_ch(c);
	if (tsg == NULL) {
		return -EINVAL;
	}

	gr_ctx = tsg->gr_ctx;

	if (!nvgpu_mem_is_valid(nvgpu_gr_ctx_get_ctx_mem(gr_ctx))) {
		tsg->vm = c->vm;
		nvgpu_vm_get(tsg->vm);
		nvgpu_gr_ctx_set_tsgid(gr_ctx, tsg->tsgid);
		err = vgpu_gr_alloc_gr_ctx(g, gr_ctx, c->vm);
		if (err) {
			nvgpu_err(g,
				"fail to allocate TSG gr ctx buffer, err=%d",
				err);
			nvgpu_vm_put(tsg->vm);
			tsg->vm = NULL;
			goto out;
		}

		/* allocate patch buffer */
		err = vgpu_gr_alloc_patch_ctx(g, gr_ctx, c->vm, c->virt_ctx);
		if (err) {
			nvgpu_err(g, "fail to allocate patch buffer");
			goto out;
		}

		vgpu_gr_init_ctxsw_preemption_mode(g, gr_ctx,
						c->vm,
						class_num,
						flags);

		/* map global buffer to channel gpu_va and commit */
		err = vgpu_gr_map_global_ctx_buffers(g, gr_ctx,
						gr->global_ctx_buffer, c->vm,
						c->virt_ctx);
		if (err) {
			nvgpu_err(g, "fail to map global ctx buffer");
			goto out;
		}

		err = vgpu_gr_commit_global_ctx_buffers(g, c, true);
		if (err) {
			nvgpu_err(g, "fail to commit global ctx buffers");
			goto out;
		}

		/* commit gr ctx buffer */
		err = vgpu_gr_commit_inst(c,
				nvgpu_gr_ctx_get_ctx_mem(gr_ctx)->gpu_va);
		if (err) {
			nvgpu_err(g, "fail to commit gr ctx buffer");
			goto out;
		}

		/* load golden image */
		err = nvgpu_pg_elpg_protected_call(g,
				vgpu_gr_load_golden_ctx_image(g, c->virt_ctx));
		if (err) {
			nvgpu_err(g, "fail to load golden ctx image");
			goto out;
		}
	} else {
		/* commit gr ctx buffer */
		err = vgpu_gr_commit_inst(c,
				nvgpu_gr_ctx_get_ctx_mem(gr_ctx)->gpu_va);
		if (err) {
			nvgpu_err(g, "fail to commit gr ctx buffer");
			goto out;
		}
#ifdef CONFIG_NVGPU_FECS_TRACE
		/* for fecs bind channel */
		err = nvgpu_pg_elpg_protected_call(g,
				vgpu_gr_load_golden_ctx_image(g, c->virt_ctx));
		if (err) {
			nvgpu_err(g, "fail to load golden ctx image");
			goto out;
		}
#endif
	}

	/* PM ctxt switch is off by default */
	nvgpu_gr_ctx_set_pm_ctx_pm_mode(gr_ctx,
		g->ops.gr.ctxsw_prog.hw_get_pm_mode_no_ctxsw());

	nvgpu_log_fn(g, "done");
	return 0;
out:
	/* 1. gr_ctx, patch_ctx and global ctx buffer mapping
	   can be reused so no need to release them.
	   2. golden image load is a one time thing so if
	   they pass, no need to undo. */
	nvgpu_err(g, "fail");
	return err;
}

static int vgpu_gr_init_gr_config(struct gk20a *g, struct nvgpu_gr *gr)
{
	struct vgpu_priv_data *priv = vgpu_get_priv_data(g);
	struct nvgpu_gr_config *config;
	u32 gpc_index;
	u32 sm_per_tpc;
	u32 pes_index;
	int err = -ENOMEM;

	nvgpu_log_fn(g, " ");

	gr->config = nvgpu_kzalloc(g, sizeof(*gr->config));
	if (gr->config == NULL) {
		return -ENOMEM;
	}

	config = gr->config;

	config->g = g;

	config->max_gpc_count = priv->constants.max_gpc_count;
	config->gpc_count = priv->constants.gpc_count;
	config->gpc_mask = priv->constants.gpc_mask;
	config->max_tpc_per_gpc_count = priv->constants.max_tpc_per_gpc_count;

	config->max_tpc_count = config->max_gpc_count * config->max_tpc_per_gpc_count;

	config->gpc_tpc_count = nvgpu_kzalloc(g, config->gpc_count * sizeof(u32));
	if (!config->gpc_tpc_count) {
		goto cleanup;
	}

	config->gpc_tpc_mask = nvgpu_kzalloc(g, config->gpc_count * sizeof(u32));
	if (!config->gpc_tpc_mask) {
		goto cleanup;
	}

	sm_per_tpc = priv->constants.sm_per_tpc;
	gr->config->sm_to_cluster = nvgpu_kzalloc(g, config->gpc_count *
					  config->max_tpc_per_gpc_count *
					  sm_per_tpc *
					  sizeof(struct nvgpu_sm_info));
	if (!gr->config->sm_to_cluster) {
		goto cleanup;
	}

#ifdef CONFIG_NVGPU_SM_DIVERSITY
	if (nvgpu_is_enabled(g, NVGPU_SUPPORT_SM_DIVERSITY)) {
		config->sm_to_cluster_redex_config =
			nvgpu_kzalloc(g, config->gpc_count *
					  config->max_tpc_per_gpc_count *
					  sm_per_tpc *
					  sizeof(struct nvgpu_sm_info));
		if (config->sm_to_cluster_redex_config == NULL) {
			nvgpu_err(g, "sm_to_cluster_redex_config == NULL");
			goto cleanup;
		}
	}
#endif

	config->tpc_count = 0;
	for (gpc_index = 0; gpc_index < config->gpc_count; gpc_index++) {
		config->gpc_tpc_count[gpc_index] =
			priv->constants.gpc_tpc_count[gpc_index];

		config->tpc_count += config->gpc_tpc_count[gpc_index];

		if (g->ops.gr.config.get_gpc_tpc_mask) {
			gr->config->gpc_tpc_mask[gpc_index] =
				g->ops.gr.config.get_gpc_tpc_mask(g,
					g->gr->config, gpc_index);
		}
	}

	config->pe_count_per_gpc =
		nvgpu_get_litter_value(g, GPU_LIT_NUM_PES_PER_GPC);
	if (config->pe_count_per_gpc > GK20A_GR_MAX_PES_PER_GPC) {
		nvgpu_do_assert_print(g, "too many pes per gpc %u\n",
			config->pe_count_per_gpc);
		goto cleanup;
	}
	if (config->pe_count_per_gpc > TEGRA_VGPU_MAX_PES_COUNT_PER_GPC) {
		nvgpu_err(g, "pe_count_per_gpc %d is too big!",
				config->pe_count_per_gpc);
		goto cleanup;
	}

	if (config->gpc_ppc_count == NULL) {
		config->gpc_ppc_count = nvgpu_kzalloc(g, config->gpc_count *
					sizeof(u32));
	} else {
		(void) memset(config->gpc_ppc_count, 0, config->gpc_count *
					sizeof(u32));
	}

	for (gpc_index = 0; gpc_index < config->gpc_count; gpc_index++) {
		config->gpc_ppc_count[gpc_index] =
			priv->constants.gpc_ppc_count[gpc_index];

		for (pes_index = 0u; pes_index < config->pe_count_per_gpc;
				pes_index++) {
			u32 pes_tpc_count, pes_tpc_mask;

			if (config->pes_tpc_count[pes_index] == NULL) {
				config->pes_tpc_count[pes_index] = nvgpu_kzalloc(g,
					config->gpc_count * sizeof(u32));
				config->pes_tpc_mask[pes_index] = nvgpu_kzalloc(g,
					config->gpc_count * sizeof(u32));
				if (config->pes_tpc_count[pes_index] == NULL ||
					config->pes_tpc_mask[pes_index] == NULL) {
					goto cleanup;
				}
			}

			pes_tpc_count = priv->constants.
				pes_tpc_count[TEGRA_VGPU_MAX_PES_COUNT_PER_GPC *
				gpc_index + pes_index];
			pes_tpc_mask = priv->constants.
				pes_tpc_mask[TEGRA_VGPU_MAX_PES_COUNT_PER_GPC *
				gpc_index + pes_index];
			config->pes_tpc_count[pes_index][gpc_index] = pes_tpc_count;
			config->pes_tpc_mask[pes_index][gpc_index] = pes_tpc_mask;
		}
	}

	err = g->ops.gr.config.init_sm_id_table(g, g->gr->config);
	if (err) {
		goto cleanup;
	}
	return 0;
cleanup:
	nvgpu_err(g, "out of memory");

	for (pes_index = 0u; pes_index < config->pe_count_per_gpc; pes_index++) {
		nvgpu_kfree(g, config->pes_tpc_count[pes_index]);
		config->pes_tpc_count[pes_index] = NULL;
		nvgpu_kfree(g, config->pes_tpc_mask[pes_index]);
		config->pes_tpc_mask[pes_index] = NULL;
	}

	nvgpu_kfree(g, config->gpc_ppc_count);
	config->gpc_ppc_count = NULL;

	nvgpu_kfree(g, config->gpc_tpc_count);
	config->gpc_tpc_count = NULL;

	nvgpu_kfree(g, config->gpc_tpc_mask);
	config->gpc_tpc_mask = NULL;

	if (config->sm_to_cluster != NULL) {
		nvgpu_kfree(g, config->sm_to_cluster);
		config->sm_to_cluster = NULL;
	}

#ifdef CONFIG_NVGPU_SM_DIVERSITY
	if (config->sm_to_cluster_redex_config != NULL) {
		nvgpu_kfree(g, config->sm_to_cluster_redex_config);
		config->sm_to_cluster_redex_config = NULL;
	}
#endif

	return err;
}

#ifdef CONFIG_NVGPU_GRAPHICS
static int vgpu_gr_init_gr_zcull(struct gk20a *g, struct nvgpu_gr *gr,
		u32 size)
{
	nvgpu_log_fn(g, " ");

	gr->zcull = nvgpu_kzalloc(g, sizeof(*gr->zcull));
	if (gr->zcull == NULL) {
		return -ENOMEM;
	}

	gr->zcull->zcull_ctxsw_image_size = size;

	return 0;
}
int vgpu_gr_bind_ctxsw_zcull(struct gk20a *g, struct nvgpu_channel *c,
			u64 zcull_va, u32 mode)
{
	struct tegra_vgpu_cmd_msg msg;
	struct tegra_vgpu_zcull_bind_params *p = &msg.params.zcull_bind;
	int err;

	nvgpu_log_fn(g, " ");

	msg.cmd = TEGRA_VGPU_CMD_CHANNEL_BIND_ZCULL;
	msg.handle = vgpu_get_handle(g);
	p->handle = c->virt_ctx;
	p->zcull_va = zcull_va;
	p->mode = mode;
	err = vgpu_comm_sendrecv(&msg, sizeof(msg), sizeof(msg));

	return (err || msg.ret) ? -ENOMEM : 0;
}

int vgpu_gr_get_zcull_info(struct gk20a *g,
			struct nvgpu_gr_config *gr_config,
			struct nvgpu_gr_zcull *zcull,
			struct nvgpu_gr_zcull_info *zcull_params)
{
	struct tegra_vgpu_cmd_msg msg;
	struct tegra_vgpu_zcull_info_params *p = &msg.params.zcull_info;
	int err;

	nvgpu_log_fn(g, " ");

	msg.cmd = TEGRA_VGPU_CMD_GET_ZCULL_INFO;
	msg.handle = vgpu_get_handle(g);
	err = vgpu_comm_sendrecv(&msg, sizeof(msg), sizeof(msg));
	if (err || msg.ret) {
		return -ENOMEM;
	}

	zcull_params->width_align_pixels = p->width_align_pixels;
	zcull_params->height_align_pixels = p->height_align_pixels;
	zcull_params->pixel_squares_by_aliquots = p->pixel_squares_by_aliquots;
	zcull_params->aliquot_total = p->aliquot_total;
	zcull_params->region_byte_multiplier = p->region_byte_multiplier;
	zcull_params->region_header_size = p->region_header_size;
	zcull_params->subregion_header_size = p->subregion_header_size;
	zcull_params->subregion_width_align_pixels =
		p->subregion_width_align_pixels;
	zcull_params->subregion_height_align_pixels =
		p->subregion_height_align_pixels;
	zcull_params->subregion_count = p->subregion_count;

	return 0;
}
#endif

u32 vgpu_gr_get_gpc_tpc_mask(struct gk20a *g, struct nvgpu_gr_config *config,
	u32 gpc_index)
{
	struct vgpu_priv_data *priv = vgpu_get_priv_data(g);

	return priv->constants.gpc_tpc_mask[gpc_index];
}

u32 vgpu_gr_get_max_fbps_count(struct gk20a *g)
{
	struct vgpu_priv_data *priv = vgpu_get_priv_data(g);

	nvgpu_log_fn(g, " ");

	return priv->constants.num_fbps;
}

u32 vgpu_gr_get_max_ltc_per_fbp(struct gk20a *g)
{
	struct vgpu_priv_data *priv = vgpu_get_priv_data(g);

	nvgpu_log_fn(g, " ");

	return priv->constants.ltc_per_fbp;
}

u32 vgpu_gr_get_max_lts_per_ltc(struct gk20a *g)
{
	struct vgpu_priv_data *priv = vgpu_get_priv_data(g);

	nvgpu_log_fn(g, " ");

	return priv->constants.max_lts_per_ltc;
}

#ifdef CONFIG_NVGPU_GRAPHICS
int vgpu_gr_add_zbc(struct gk20a *g, struct nvgpu_gr_zbc *zbc,
			   struct nvgpu_gr_zbc_entry *zbc_val)
{
	struct tegra_vgpu_cmd_msg msg = {0};
	struct tegra_vgpu_zbc_set_table_params *p = &msg.params.zbc_set_table;
	int err;

	nvgpu_log_fn(g, " ");

	msg.cmd = TEGRA_VGPU_CMD_ZBC_SET_TABLE;
	msg.handle = vgpu_get_handle(g);

	p->type = zbc_val->type;
	p->format = zbc_val->format;
	switch (p->type) {
	case NVGPU_GR_ZBC_TYPE_COLOR:
		nvgpu_memcpy((u8 *)p->color_ds, (u8 *)zbc_val->color_ds,
			sizeof(p->color_ds));
		nvgpu_memcpy((u8 *)p->color_l2, (u8 *)zbc_val->color_l2,
			sizeof(p->color_l2));
		break;
	case NVGPU_GR_ZBC_TYPE_DEPTH:
		p->depth = zbc_val->depth;
		break;
	default:
		return -EINVAL;
	}

	err = vgpu_comm_sendrecv(&msg, sizeof(msg), sizeof(msg));

	return (err || msg.ret) ? -ENOMEM : 0;
}

int vgpu_gr_query_zbc(struct gk20a *g, struct nvgpu_gr_zbc *zbc,
			struct nvgpu_gr_zbc_query_params *query_params)
{
	struct tegra_vgpu_cmd_msg msg = {0};
	struct tegra_vgpu_zbc_query_table_params *p =
					&msg.params.zbc_query_table;
	int err;

	nvgpu_log_fn(g, " ");

	msg.cmd = TEGRA_VGPU_CMD_ZBC_QUERY_TABLE;
	msg.handle = vgpu_get_handle(g);

	p->type = query_params->type;
	p->index_size = query_params->index_size;

	err = vgpu_comm_sendrecv(&msg, sizeof(msg), sizeof(msg));
	if (err || msg.ret) {
		return -ENOMEM;
	}

	switch (query_params->type) {
	case NVGPU_GR_ZBC_TYPE_COLOR:
		nvgpu_memcpy((u8 *)query_params->color_ds, (u8 *)p->color_ds,
				sizeof(query_params->color_ds));
		nvgpu_memcpy((u8 *)query_params->color_l2, (u8 *)p->color_l2,
				sizeof(query_params->color_l2));
		break;
	case NVGPU_GR_ZBC_TYPE_DEPTH:
		query_params->depth = p->depth;
		break;
	case NVGPU_GR_ZBC_TYPE_INVALID:
		query_params->index_size = p->index_size;
		break;
	default:
		return -EINVAL;
	}
	query_params->ref_cnt = p->ref_cnt;
	query_params->format = p->format;

	return 0;
}
#endif

static void vgpu_remove_gr_support(struct gk20a *g)
{
	struct nvgpu_gr *gr = g->gr;

	nvgpu_log_fn(gr->g, " ");

	nvgpu_kfree(gr->g, gr->config->sm_to_cluster);
	gr->config->sm_to_cluster = NULL;

#ifdef CONFIG_NVGPU_SM_DIVERSITY
	if (gr->config->sm_to_cluster_redex_config != NULL) {
		nvgpu_kfree(g, gr->config->sm_to_cluster_redex_config);
		gr->config->sm_to_cluster_redex_config = NULL;
	}
#endif

	nvgpu_gr_config_deinit(gr->g, gr->config);

#ifdef CONFIG_NVGPU_GRAPHICS
	nvgpu_gr_zcull_deinit(gr->g, gr->zcull);
#endif

	nvgpu_gr_free(g);
}

static int vgpu_gr_init_gr_setup_sw(struct gk20a *g)
{
	struct nvgpu_gr *gr = g->gr;
	int err;

	nvgpu_log_fn(g, " ");

	if (gr->sw_ready) {
		nvgpu_log_fn(g, "skip init");
		return 0;
	}

	gr->g = g;

	err = g->ops.gr.falcon.init_ctx_state(g, &gr->falcon->sizes);
	if (err) {
		goto clean_up;
	}

	err = vgpu_gr_init_gr_config(g, gr);
	if (err) {
		goto clean_up;
	}

	err = nvgpu_gr_obj_ctx_init(g, &gr->golden_image,
			nvgpu_gr_falcon_get_golden_image_size(g->gr->falcon));
	if (err != 0) {
		goto clean_up;
	}

#ifdef CONFIG_NVGPU_DEBUGGER
	err = nvgpu_gr_hwpm_map_init(g, &g->gr->hwpm_map,
			nvgpu_gr_falcon_get_pm_ctxsw_image_size(g->gr->falcon));
	if (err != 0) {
		nvgpu_err(g, "hwpm_map init failed");
		goto clean_up;
	}
#endif

#ifdef CONFIG_NVGPU_GRAPHICS
	err = vgpu_gr_init_gr_zcull(g, gr,
			nvgpu_gr_falcon_get_zcull_image_size(g->gr->falcon));
	if (err) {
		goto clean_up;
	}
#endif

	err = vgpu_gr_alloc_global_ctx_buffers(g);
	if (err) {
		goto clean_up;
	}

	gr->gr_ctx_desc = nvgpu_gr_ctx_desc_alloc(g);
	if (gr->gr_ctx_desc == NULL) {
		goto clean_up;
	}

#ifdef CONFIG_NVGPU_GRAPHICS
	if (!nvgpu_is_enabled(g, NVGPU_SUPPORT_MIG)) {
		nvgpu_gr_ctx_set_size(gr->gr_ctx_desc,
			NVGPU_GR_CTX_PREEMPT_CTXSW,
			nvgpu_gr_falcon_get_preempt_image_size(g->gr->falcon));
	}
#endif

	nvgpu_spinlock_init(&g->gr->intr->ch_tlb_lock);

	gr->remove_support = vgpu_remove_gr_support;
	gr->sw_ready = true;

	nvgpu_log_fn(g, "done");
	return 0;

clean_up:
	nvgpu_err(g, "fail");
	vgpu_remove_gr_support(g);
	return err;
}

int vgpu_init_gr_support(struct gk20a *g)
{
	nvgpu_log_fn(g, " ");

	return vgpu_gr_init_gr_setup_sw(g);
}

int vgpu_gr_isr(struct gk20a *g, struct tegra_vgpu_gr_intr_info *info)
{
	struct nvgpu_channel *ch = nvgpu_channel_from_id(g, info->chid);

	nvgpu_log_fn(g, " ");

	if (!ch) {
		return 0;
	}

	if (info->type != TEGRA_VGPU_GR_INTR_NOTIFY &&
		info->type != TEGRA_VGPU_GR_INTR_SEMAPHORE) {
		nvgpu_err(g, "gr intr (%d) on ch %u", info->type, info->chid);
	}

	switch (info->type) {
	case TEGRA_VGPU_GR_INTR_NOTIFY:
		nvgpu_cond_broadcast_interruptible(&ch->notifier_wq);
		break;
	case TEGRA_VGPU_GR_INTR_SEMAPHORE:
		nvgpu_cond_broadcast_interruptible(&ch->semaphore_wq);
		break;
	case TEGRA_VGPU_GR_INTR_SEMAPHORE_TIMEOUT:
		g->ops.channel.set_error_notifier(ch,
			NVGPU_ERR_NOTIFIER_GR_SEMAPHORE_TIMEOUT);
		break;
	case TEGRA_VGPU_GR_INTR_ILLEGAL_NOTIFY:
		g->ops.channel.set_error_notifier(ch,
			NVGPU_ERR_NOTIFIER_GR_ILLEGAL_NOTIFY);
	case TEGRA_VGPU_GR_INTR_ILLEGAL_METHOD:
		break;
	case TEGRA_VGPU_GR_INTR_ILLEGAL_CLASS:
		g->ops.channel.set_error_notifier(ch,
			NVGPU_ERR_NOTIFIER_GR_ERROR_SW_NOTIFY);
		break;
	case TEGRA_VGPU_GR_INTR_FECS_ERROR:
		break;
	case TEGRA_VGPU_GR_INTR_CLASS_ERROR:
		g->ops.channel.set_error_notifier(ch,
			NVGPU_ERR_NOTIFIER_GR_ERROR_SW_NOTIFY);
		break;
	case TEGRA_VGPU_GR_INTR_FIRMWARE_METHOD:
		g->ops.channel.set_error_notifier(ch,
			NVGPU_ERR_NOTIFIER_GR_ERROR_SW_NOTIFY);
		break;
	case TEGRA_VGPU_GR_INTR_EXCEPTION:
		g->ops.channel.set_error_notifier(ch,
			NVGPU_ERR_NOTIFIER_GR_ERROR_SW_NOTIFY);
		break;
#ifdef CONFIG_NVGPU_DEBUGGER
	case TEGRA_VGPU_GR_INTR_SM_EXCEPTION:
		g->ops.debugger.post_events(ch);
		break;
#endif
	default:
		WARN_ON(1);
		break;
	}

	nvgpu_channel_put(ch);
	return 0;
}

int vgpu_gr_set_sm_debug_mode(struct gk20a *g,
	struct nvgpu_channel *ch, u64 sms, bool enable)
{
	struct tegra_vgpu_cmd_msg msg;
	struct tegra_vgpu_sm_debug_mode *p = &msg.params.sm_debug_mode;
	int err;

	nvgpu_log_fn(g, " ");

	msg.cmd = TEGRA_VGPU_CMD_SET_SM_DEBUG_MODE;
	msg.handle = vgpu_get_handle(g);
	p->handle = ch->virt_ctx;
	p->sms = sms;
	p->enable = (u32)enable;
	err = vgpu_comm_sendrecv(&msg, sizeof(msg), sizeof(msg));
	WARN_ON(err || msg.ret);

	return err ? err : msg.ret;
}

int vgpu_gr_update_smpc_ctxsw_mode(struct gk20a *g,
	struct nvgpu_tsg *tsg, bool enable)
{
	struct tegra_vgpu_cmd_msg msg;
	struct tegra_vgpu_channel_set_ctxsw_mode *p = &msg.params.set_ctxsw_mode;
	int err;

	nvgpu_log_fn(g, " ");

	msg.cmd = TEGRA_VGPU_CMD_CHANNEL_SET_SMPC_CTXSW_MODE;
	msg.handle = vgpu_get_handle(g);
	p->tsg_id = tsg->tsgid;

	if (enable) {
		p->mode = TEGRA_VGPU_CTXSW_MODE_CTXSW;
	} else {
		p->mode = TEGRA_VGPU_CTXSW_MODE_NO_CTXSW;
	}

	err = vgpu_comm_sendrecv(&msg, sizeof(msg), sizeof(msg));
	WARN_ON(err || msg.ret);

	return err ? err : msg.ret;
}

int vgpu_gr_update_hwpm_ctxsw_mode(struct gk20a *g,
	struct nvgpu_tsg *tsg, u64 gpu_va, u32 mode)
{
	struct nvgpu_gr_ctx *gr_ctx;
	struct tegra_vgpu_cmd_msg msg;
	struct tegra_vgpu_channel_set_ctxsw_mode *p = &msg.params.set_ctxsw_mode;
	int err;

	nvgpu_log_fn(g, " ");

	if (gpu_va) {
		nvgpu_err(g, "gpu_va suppose to be allocated by this function.");
		return -EINVAL;
	}

	gr_ctx = tsg->gr_ctx;

	if (mode == NVGPU_GR_CTX_HWPM_CTXSW_MODE_CTXSW) {
		/*
		 * send command to enable HWPM only once - otherwise server
		 * will return an error due to using the same GPU VA twice.
		 */

		if (nvgpu_gr_ctx_get_pm_ctx_pm_mode(gr_ctx) ==
				g->ops.gr.ctxsw_prog.hw_get_pm_mode_ctxsw()) {
			return 0;
		}
		p->mode = TEGRA_VGPU_CTXSW_MODE_CTXSW;
	} else if (mode == NVGPU_GR_CTX_HWPM_CTXSW_MODE_NO_CTXSW) {
		if (nvgpu_gr_ctx_get_pm_ctx_pm_mode(gr_ctx) ==
				g->ops.gr.ctxsw_prog.hw_get_pm_mode_no_ctxsw()) {
			return 0;
		}
		p->mode = TEGRA_VGPU_CTXSW_MODE_NO_CTXSW;
	} else if ((mode == NVGPU_GR_CTX_HWPM_CTXSW_MODE_STREAM_OUT_CTXSW) &&
			g->ops.gr.ctxsw_prog.hw_get_pm_mode_stream_out_ctxsw()) {
		if (nvgpu_gr_ctx_get_pm_ctx_pm_mode(gr_ctx) ==
				g->ops.gr.ctxsw_prog.hw_get_pm_mode_stream_out_ctxsw()) {
			return 0;
		}
		p->mode = TEGRA_VGPU_CTXSW_MODE_STREAM_OUT_CTXSW;
	} else {
		nvgpu_err(g, "invalid hwpm context switch mode");
		return -EINVAL;
	}

	if (mode != NVGPU_GR_CTX_HWPM_CTXSW_MODE_NO_CTXSW) {
		/* Allocate buffer if necessary */
		err = vgpu_gr_alloc_pm_ctx(g, tsg->gr_ctx, tsg->vm);
		if (err != 0) {
			nvgpu_err(g,
				"failed to allocate pm ctxt buffer");
			return err;
		}
	}

	msg.cmd = TEGRA_VGPU_CMD_CHANNEL_SET_HWPM_CTXSW_MODE;
	msg.handle = vgpu_get_handle(g);
	p->tsg_id = tsg->tsgid;
	p->gpu_va = nvgpu_gr_ctx_get_pm_ctx_mem(gr_ctx)->gpu_va;

	err = vgpu_comm_sendrecv(&msg, sizeof(msg), sizeof(msg));
	WARN_ON(err || msg.ret);
	err = err ? err : msg.ret;
	if (!err) {
		if (mode == NVGPU_GR_CTX_HWPM_CTXSW_MODE_CTXSW) {
			nvgpu_gr_ctx_set_pm_ctx_pm_mode(gr_ctx,
				g->ops.gr.ctxsw_prog.hw_get_pm_mode_ctxsw());
		} else if (mode == NVGPU_GR_CTX_HWPM_CTXSW_MODE_NO_CTXSW) {
			nvgpu_gr_ctx_set_pm_ctx_pm_mode(gr_ctx,
				g->ops.gr.ctxsw_prog.hw_get_pm_mode_no_ctxsw());
		} else {
			nvgpu_gr_ctx_set_pm_ctx_pm_mode(gr_ctx,
				g->ops.gr.ctxsw_prog.hw_get_pm_mode_stream_out_ctxsw());
		}
	}

	return err;
}

int vgpu_gr_clear_sm_error_state(struct gk20a *g,
		struct nvgpu_channel *ch, u32 sm_id)
{
	struct tegra_vgpu_cmd_msg msg;
	struct tegra_vgpu_clear_sm_error_state *p =
			&msg.params.clear_sm_error_state;
	struct nvgpu_tsg *tsg;
	int err;

	tsg = nvgpu_tsg_from_ch(ch);
	if (!tsg) {
		return -EINVAL;
	}

	nvgpu_mutex_acquire(&g->dbg_sessions_lock);
	msg.cmd = TEGRA_VGPU_CMD_CLEAR_SM_ERROR_STATE;
	msg.handle = vgpu_get_handle(g);
	p->handle = ch->virt_ctx;
	p->sm_id = sm_id;

	err = vgpu_comm_sendrecv(&msg, sizeof(msg), sizeof(msg));
	WARN_ON(err || msg.ret);

	(void) memset(&tsg->sm_error_states[sm_id], 0,
		sizeof(*tsg->sm_error_states));
	nvgpu_mutex_release(&g->dbg_sessions_lock);

	return err ? err : msg.ret;


	return 0;
}

static int vgpu_gr_suspend_resume_contexts(struct gk20a *g,
		struct dbg_session_gk20a *dbg_s,
		int *ctx_resident_ch_fd, u32 cmd)
{
	struct dbg_session_channel_data *ch_data;
	struct tegra_vgpu_cmd_msg msg;
	struct tegra_vgpu_suspend_resume_contexts *p;
	size_t n;
	int channel_fd = -1;
	int err = 0;
	void *handle = NULL;
	u16 *oob;
	size_t oob_size;

	nvgpu_mutex_acquire(&g->dbg_sessions_lock);
	nvgpu_mutex_acquire(&dbg_s->ch_list_lock);

	handle = vgpu_ivc_oob_get_ptr(vgpu_ivc_get_server_vmid(),
			TEGRA_VGPU_QUEUE_CMD,
			(void **)&oob, &oob_size);
	if (!handle) {
		err = -EINVAL;
		goto done;
	}

	n = 0;
	nvgpu_list_for_each_entry(ch_data, &dbg_s->ch_list,
			dbg_session_channel_data, ch_entry) {
		n++;
	}

	if (oob_size < n * sizeof(u16)) {
		err = -ENOMEM;
		goto done;
	}

	msg.cmd = cmd;
	msg.handle = vgpu_get_handle(g);
	p = &msg.params.suspend_contexts;
	p->num_channels = n;
	n = 0;
	nvgpu_list_for_each_entry(ch_data, &dbg_s->ch_list,
			dbg_session_channel_data, ch_entry) {
		oob[n++] = (u16)ch_data->chid;
	}

	err = vgpu_comm_sendrecv(&msg, sizeof(msg), sizeof(msg));
	if (err || msg.ret) {
		err = -ENOMEM;
		goto done;
	}

	if (p->resident_chid != (u16)~0) {
		nvgpu_list_for_each_entry(ch_data, &dbg_s->ch_list,
				dbg_session_channel_data, ch_entry) {
			if (ch_data->chid == p->resident_chid) {
				channel_fd = ch_data->channel_fd;
				break;
			}
		}
	}

done:
	if (handle) {
		vgpu_ivc_oob_put_ptr(handle);
	}
	nvgpu_mutex_release(&dbg_s->ch_list_lock);
	nvgpu_mutex_release(&g->dbg_sessions_lock);
	*ctx_resident_ch_fd = channel_fd;
	return err;
}

int vgpu_gr_suspend_contexts(struct gk20a *g,
		struct dbg_session_gk20a *dbg_s,
		int *ctx_resident_ch_fd)
{
	return vgpu_gr_suspend_resume_contexts(g, dbg_s,
			ctx_resident_ch_fd, TEGRA_VGPU_CMD_SUSPEND_CONTEXTS);
}

int vgpu_gr_resume_contexts(struct gk20a *g,
		struct dbg_session_gk20a *dbg_s,
		int *ctx_resident_ch_fd)
{
	return vgpu_gr_suspend_resume_contexts(g, dbg_s,
			ctx_resident_ch_fd, TEGRA_VGPU_CMD_RESUME_CONTEXTS);
}

void vgpu_gr_handle_sm_esr_event(struct gk20a *g,
			struct tegra_vgpu_sm_esr_info *info)
{
	struct nvgpu_tsg_sm_error_state *sm_error_states;
	struct nvgpu_tsg *tsg;
	u32 no_of_sm = g->ops.gr.init.get_no_of_sm(g);

	if (info->sm_id >= no_of_sm) {
		nvgpu_err(g, "invalid smd_id %d / %d", info->sm_id, no_of_sm);
		return;
	}

	if (info->tsg_id >= g->fifo.num_channels) {
		nvgpu_err(g, "invalid tsg_id in sm esr event");
		return;
	}

	tsg = nvgpu_tsg_check_and_get_from_id(g, info->tsg_id);
	if (tsg == NULL) {
		nvgpu_err(g, "invalid tsg");
		return;
	}

	nvgpu_mutex_acquire(&g->dbg_sessions_lock);

	sm_error_states = &tsg->sm_error_states[info->sm_id];

	sm_error_states->hww_global_esr = info->hww_global_esr;
	sm_error_states->hww_warp_esr = info->hww_warp_esr;
	sm_error_states->hww_warp_esr_pc = info->hww_warp_esr_pc;
	sm_error_states->hww_global_esr_report_mask =
				info->hww_global_esr_report_mask;
	sm_error_states->hww_warp_esr_report_mask =
				info->hww_warp_esr_report_mask;

	nvgpu_mutex_release(&g->dbg_sessions_lock);
}

int vgpu_gr_init_sm_id_table(struct gk20a *g, struct nvgpu_gr_config *gr_config)
{
	struct tegra_vgpu_cmd_msg msg = {};
	struct tegra_vgpu_vsms_mapping_params *p = &msg.params.vsms_mapping;
	struct tegra_vgpu_vsms_mapping_entry *entry;
	struct vgpu_priv_data *priv = vgpu_get_priv_data(g);
	struct nvgpu_sm_info *sm_info;
	int err;
	size_t oob_size;
	void *handle = NULL;
	u32 sm_id;
	u32 max_sm;
	u32 sm_config;

	msg.cmd = TEGRA_VGPU_CMD_GET_VSMS_MAPPING;
	msg.handle = vgpu_get_handle(g);
	err = vgpu_comm_sendrecv(&msg, sizeof(msg), sizeof(msg));
	err = err ? err : msg.ret;
	if (err) {
		nvgpu_err(g,
			"get vsms mapping failed err %d", err);
		return err;
	}

	handle = vgpu_ivc_oob_get_ptr(vgpu_ivc_get_server_vmid(),
					   TEGRA_VGPU_QUEUE_CMD,
					   (void **)&entry, &oob_size);
	if (!handle) {
		return -EINVAL;
	}

	max_sm = gr_config->gpc_count *
			gr_config->max_tpc_per_gpc_count *
			priv->constants.sm_per_tpc;
	if (p->num_sm > max_sm) {
		return -EINVAL;
	}

	if ((p->num_sm * sizeof(*entry) *
		priv->constants.max_sm_diversity_config_count) > oob_size) {
		return -EINVAL;
	}

	gr_config->no_of_sm = p->num_sm;
	for (sm_config = NVGPU_DEFAULT_SM_DIVERSITY_CONFIG;
		sm_config < priv->constants.max_sm_diversity_config_count;
		sm_config++) {
		for (sm_id = 0; sm_id < p->num_sm; sm_id++, entry++) {
#ifdef CONFIG_NVGPU_SM_DIVERSITY
			sm_info =
			((sm_config == NVGPU_DEFAULT_SM_DIVERSITY_CONFIG) ?
				nvgpu_gr_config_get_sm_info(gr_config, sm_id) :
				nvgpu_gr_config_get_redex_sm_info(
					gr_config, sm_id));
#else
			sm_info = nvgpu_gr_config_get_sm_info(gr_config, sm_id);
#endif
			sm_info->tpc_index = entry->tpc_index;
			sm_info->gpc_index = entry->gpc_index;
			sm_info->sm_index = entry->sm_index;
			sm_info->global_tpc_index = entry->global_tpc_index;
		}
	}
	vgpu_ivc_oob_put_ptr(handle);

	return 0;
}

int vgpu_gr_update_pc_sampling(struct nvgpu_channel *ch, bool enable)
{
	struct tegra_vgpu_cmd_msg msg;
	struct tegra_vgpu_channel_update_pc_sampling *p =
						&msg.params.update_pc_sampling;
	struct gk20a *g;
	int err = -EINVAL;

	if (!ch->g) {
		return err;
	}
	g = ch->g;
	nvgpu_log_fn(g, " ");

	msg.cmd = TEGRA_VGPU_CMD_UPDATE_PC_SAMPLING;
	msg.handle = vgpu_get_handle(g);
	p->handle = ch->virt_ctx;
	if (enable) {
		p->mode = TEGRA_VGPU_ENABLE_SAMPLING;
	} else {
		p->mode = TEGRA_VGPU_DISABLE_SAMPLING;
	}

	err = vgpu_comm_sendrecv(&msg, sizeof(msg), sizeof(msg));
	WARN_ON(err || msg.ret);

	return err ? err : msg.ret;
}

void vgpu_gr_init_cyclestats(struct gk20a *g)
{
#if defined(CONFIG_NVGPU_CYCLESTATS)
	bool snapshots_supported = true;

	/* cyclestats not supported on vgpu */
	nvgpu_set_enabled(g, NVGPU_SUPPORT_CYCLE_STATS, false);

	if (vgpu_css_init(g) != 0) {
		snapshots_supported = false;
	}

	nvgpu_set_enabled(g, NVGPU_SUPPORT_CYCLE_STATS_SNAPSHOT,
							snapshots_supported);
#endif
}

static int vgpu_gr_init_ctxsw_preemption_mode(struct gk20a *g,
				struct nvgpu_gr_ctx *gr_ctx,
				struct vm_gk20a *vm,
				u32 class,
				u32 flags)
{
	u32 graphics_preempt_mode = 0;
	u32 compute_preempt_mode = 0;
	struct vgpu_priv_data *priv = vgpu_get_priv_data(g);
	int err;

	nvgpu_log_fn(g, " ");

#ifdef CONFIG_NVGPU_GRAPHICS
	if (flags & NVGPU_OBJ_CTX_FLAGS_SUPPORT_GFXP) {
		graphics_preempt_mode = NVGPU_PREEMPTION_MODE_GRAPHICS_GFXP;
	}
#endif
#ifdef CONFIG_NVGPU_CILP
	if (flags & NVGPU_OBJ_CTX_FLAGS_SUPPORT_CILP) {
		compute_preempt_mode = NVGPU_PREEMPTION_MODE_COMPUTE_CILP;
	}
#endif

	if (priv->constants.force_preempt_mode && !graphics_preempt_mode &&
		!compute_preempt_mode) {
#ifdef CONFIG_NVGPU_GRAPHICS
		graphics_preempt_mode = g->ops.gpu_class.is_valid_gfx(class) ?
					NVGPU_PREEMPTION_MODE_GRAPHICS_GFXP : 0;
#endif
		compute_preempt_mode =
			g->ops.gpu_class.is_valid_compute(class) ?
			NVGPU_PREEMPTION_MODE_COMPUTE_CTA : 0;
	}

	if (graphics_preempt_mode || compute_preempt_mode) {
		err = vgpu_gr_set_ctxsw_preemption_mode(g, gr_ctx, vm,
		    class, graphics_preempt_mode, compute_preempt_mode);
		if (err) {
			nvgpu_err(g,
				"set_ctxsw_preemption_mode failed");
			return err;
		}
	}

	nvgpu_log_fn(g, "done");
	return 0;
}

static int vgpu_gr_set_ctxsw_preemption_mode(struct gk20a *g,
				struct nvgpu_gr_ctx *gr_ctx,
				struct vm_gk20a *vm, u32 class,
				u32 graphics_preempt_mode,
				u32 compute_preempt_mode)
{
	struct tegra_vgpu_cmd_msg msg = {};
	struct tegra_vgpu_gr_bind_ctxsw_buffers_params *p =
				&msg.params.gr_bind_ctxsw_buffers;
	int err = 0;

#ifdef CONFIG_NVGPU_GRAPHICS
	if (g->ops.gpu_class.is_valid_gfx(class) &&
			g->gr->gr_ctx_desc->force_preemption_gfxp) {
		graphics_preempt_mode = NVGPU_PREEMPTION_MODE_GRAPHICS_GFXP;
	}
#endif

#ifdef CONFIG_NVGPU_CILP
	if (g->ops.gpu_class.is_valid_compute(class) &&
			g->gr->gr_ctx_desc->force_preemption_cilp) {
		compute_preempt_mode = NVGPU_PREEMPTION_MODE_COMPUTE_CILP;
	}
#endif

	/* check for invalid combinations */
	if ((graphics_preempt_mode == 0) && (compute_preempt_mode == 0)) {
		return -EINVAL;
	}

#if defined(CONFIG_NVGPU_CILP) && defined(CONFIG_NVGPU_GRAPHICS)
	if ((graphics_preempt_mode == NVGPU_PREEMPTION_MODE_GRAPHICS_GFXP) &&
		   (compute_preempt_mode == NVGPU_PREEMPTION_MODE_COMPUTE_CILP)) {
		return -EINVAL;
	}
#endif

	/* set preemption modes */
	switch (graphics_preempt_mode) {
#ifdef CONFIG_NVGPU_GRAPHICS
	case NVGPU_PREEMPTION_MODE_GRAPHICS_GFXP:
	{
		u32 spill_size = g->ops.gr.init.get_ctx_spill_size(g);
		u32 pagepool_size = g->ops.gr.init.get_ctx_pagepool_size(g);
		u32 betacb_size = g->ops.gr.init.get_ctx_betacb_size(g);
		u32 attrib_cb_size =
			g->ops.gr.init.get_ctx_attrib_cb_size(g, betacb_size,
				nvgpu_gr_config_get_tpc_count(g->gr->config),
				nvgpu_gr_config_get_max_tpc_count(g->gr->config));
		struct nvgpu_mem *desc;

		nvgpu_log_info(g, "gfxp context preempt size=%d",
			g->gr->falcon->sizes.preempt_image_size);
		nvgpu_log_info(g, "gfxp context spill size=%d", spill_size);
		nvgpu_log_info(g, "gfxp context pagepool size=%d", pagepool_size);
		nvgpu_log_info(g, "gfxp context attrib cb size=%d",
			attrib_cb_size);

		nvgpu_gr_ctx_set_size(g->gr->gr_ctx_desc,
			NVGPU_GR_CTX_SPILL_CTXSW, spill_size);
		nvgpu_gr_ctx_set_size(g->gr->gr_ctx_desc,
			NVGPU_GR_CTX_BETACB_CTXSW, attrib_cb_size);
		nvgpu_gr_ctx_set_size(g->gr->gr_ctx_desc,
			NVGPU_GR_CTX_PAGEPOOL_CTXSW, pagepool_size);

		err = nvgpu_gr_ctx_alloc_ctxsw_buffers(g, gr_ctx,
			g->gr->gr_ctx_desc, vm);
		if (err != 0) {
			nvgpu_err(g, "cannot allocate ctxsw buffers");
			goto fail;
		}

		desc = nvgpu_gr_ctx_get_preempt_ctxsw_buffer(gr_ctx);
		p->gpu_va[TEGRA_VGPU_GR_BIND_CTXSW_BUFFER_MAIN] = desc->gpu_va;
		p->size[TEGRA_VGPU_GR_BIND_CTXSW_BUFFER_MAIN] = desc->size;

		desc = nvgpu_gr_ctx_get_spill_ctxsw_buffer(gr_ctx);
		p->gpu_va[TEGRA_VGPU_GR_BIND_CTXSW_BUFFER_SPILL] = desc->gpu_va;
		p->size[TEGRA_VGPU_GR_BIND_CTXSW_BUFFER_SPILL] = desc->size;

		desc = nvgpu_gr_ctx_get_pagepool_ctxsw_buffer(gr_ctx);
		p->gpu_va[TEGRA_VGPU_GR_BIND_CTXSW_BUFFER_PAGEPOOL] =
			desc->gpu_va;
		p->size[TEGRA_VGPU_GR_BIND_CTXSW_BUFFER_PAGEPOOL] = desc->size;

		desc = nvgpu_gr_ctx_get_betacb_ctxsw_buffer(gr_ctx);
		p->gpu_va[TEGRA_VGPU_GR_BIND_CTXSW_BUFFER_BETACB] =
			desc->gpu_va;
		p->size[TEGRA_VGPU_GR_BIND_CTXSW_BUFFER_BETACB] = desc->size;

		nvgpu_gr_ctx_init_graphics_preemption_mode(gr_ctx,
			NVGPU_PREEMPTION_MODE_GRAPHICS_GFXP);
		p->mode = TEGRA_VGPU_GR_CTXSW_PREEMPTION_MODE_GFX_GFXP;
		break;
	}
#endif
	case NVGPU_PREEMPTION_MODE_GRAPHICS_WFI:
		nvgpu_gr_ctx_init_graphics_preemption_mode(gr_ctx,
			graphics_preempt_mode);
		break;

	default:
		break;
	}

	if (g->ops.gpu_class.is_valid_compute(class)) {
		switch (compute_preempt_mode) {
		case NVGPU_PREEMPTION_MODE_COMPUTE_WFI:
			nvgpu_gr_ctx_init_compute_preemption_mode(gr_ctx,
				NVGPU_PREEMPTION_MODE_COMPUTE_WFI);
			p->mode = TEGRA_VGPU_GR_CTXSW_PREEMPTION_MODE_WFI;
			break;
		case NVGPU_PREEMPTION_MODE_COMPUTE_CTA:
			nvgpu_gr_ctx_init_compute_preemption_mode(gr_ctx,
				NVGPU_PREEMPTION_MODE_COMPUTE_CTA);
			p->mode =
				TEGRA_VGPU_GR_CTXSW_PREEMPTION_MODE_COMPUTE_CTA;
			break;
#ifdef CONFIG_NVGPU_CILP
		case NVGPU_PREEMPTION_MODE_COMPUTE_CILP:
			nvgpu_gr_ctx_init_compute_preemption_mode(gr_ctx,
				NVGPU_PREEMPTION_MODE_COMPUTE_CILP);
			p->mode =
				TEGRA_VGPU_GR_CTXSW_PREEMPTION_MODE_COMPUTE_CILP;
			break;
#endif
		default:
			break;
		}
	}

	if (
#ifdef CONFIG_NVGPU_GRAPHICS
		(nvgpu_gr_ctx_get_graphics_preemption_mode(gr_ctx) != 0U) ||
#endif
		(nvgpu_gr_ctx_get_compute_preemption_mode(gr_ctx) != 0U)) {
		msg.cmd = TEGRA_VGPU_CMD_BIND_GR_CTXSW_BUFFERS;
		msg.handle = vgpu_get_handle(g);
		p->tsg_id = nvgpu_gr_ctx_get_tsgid(gr_ctx);
		err = vgpu_comm_sendrecv(&msg, sizeof(msg), sizeof(msg));
		if (err || msg.ret) {
			err = -ENOMEM;
			goto fail;
		}
	}

	return err;

fail:
	nvgpu_err(g, "%s failed %d", __func__, err);
	return err;
}

int vgpu_gr_set_preemption_mode(struct nvgpu_channel *ch,
		u32 graphics_preempt_mode, u32 compute_preempt_mode,
		u32 gr_instance_id)
{
	struct nvgpu_gr_ctx *gr_ctx;
	struct gk20a *g = ch->g;
	struct nvgpu_tsg *tsg;
	struct vm_gk20a *vm;
	u32 class;
	int err;

	class = ch->obj_class;
	if (!class) {
		return -EINVAL;
	}

	tsg = nvgpu_tsg_from_ch(ch);
	if (!tsg) {
		return -EINVAL;
	}

	vm = tsg->vm;
	gr_ctx = tsg->gr_ctx;

#ifdef CONFIG_NVGPU_GRAPHICS
	/* skip setting anything if both modes are already set */
	if (graphics_preempt_mode &&
		(graphics_preempt_mode ==
			nvgpu_gr_ctx_get_graphics_preemption_mode(gr_ctx))) {
		graphics_preempt_mode = 0;
	}
#endif

	if (compute_preempt_mode &&
	   (compute_preempt_mode ==
			nvgpu_gr_ctx_get_compute_preemption_mode(gr_ctx))) {
		compute_preempt_mode = 0;
	}

	if (graphics_preempt_mode == 0 && compute_preempt_mode == 0) {
		return 0;
	}

	err = vgpu_gr_set_ctxsw_preemption_mode(g, gr_ctx, vm, class,
					graphics_preempt_mode,
					compute_preempt_mode);
	if (err) {
		nvgpu_err(g, "set_ctxsw_preemption_mode failed");
		return err;
	}

	return err;
}

u32 vgpu_gr_get_gpc_count(struct gk20a *g)
{
	struct vgpu_priv_data *priv = vgpu_get_priv_data(g);

	return priv->constants.gpc_count;
}

u32 vgpu_gr_get_gpc_mask(struct gk20a *g)
{
	struct vgpu_priv_data *priv = vgpu_get_priv_data(g);

	return priv->constants.gpc_mask;
}

#ifdef CONFIG_NVGPU_DEBUGGER

u64 vgpu_gr_gk20a_tpc_enabled_exceptions(struct gk20a *g)
{
	struct tegra_vgpu_cmd_msg msg = {};
	struct tegra_vgpu_get_tpc_exception_en_status_params *p =
				&msg.params.get_tpc_exception_status;
	u64 tpc_exception_en = 0U;
	int err = 0;

	msg.cmd = TEGRA_VGPU_CMD_GET_TPC_EXCEPTION_EN_STATUS;
	msg.handle = vgpu_get_handle(g);
	err = vgpu_comm_sendrecv(&msg, sizeof(msg), sizeof(msg));
	err = err ? err : msg.ret;
	if (err) {
		nvgpu_err(g,
			"get tpc enabled exception failed err %d", err);
		return err;
	}

	tpc_exception_en = p->tpc_exception_en_sm_mask;
	return tpc_exception_en;
}

int vgpu_gr_set_mmu_debug_mode(struct gk20a *g,
		struct nvgpu_channel *ch, bool enable)
{
	struct tegra_vgpu_cmd_msg msg = {};
	struct tegra_vgpu_gr_set_mmu_debug_mode_params *p =
				&msg.params.gr_set_mmu_debug_mode;
	int err;

	msg.cmd = TEGRA_VGPU_CMD_GR_SET_MMU_DEBUG_MODE;
	msg.handle = vgpu_get_handle(g);
	p->ch_handle = ch->virt_ctx;
	p->enable = enable ? 1U : 0U;
	err = vgpu_comm_sendrecv(&msg, sizeof(msg), sizeof(msg));
	err = err != 0 ? err : msg.ret;
	if (err != 0) {
		nvgpu_err(g,
			"gr set mmu debug mode failed err %d", err);
	}

	return err;
}

#endif
