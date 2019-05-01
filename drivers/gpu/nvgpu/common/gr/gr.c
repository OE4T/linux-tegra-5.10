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
#include <nvgpu/io.h>
#include <nvgpu/unit.h>
#include <nvgpu/gr/gr.h>
#include <nvgpu/gr/config.h>
#include <nvgpu/gr/gr_intr.h>
#include <nvgpu/gr/zbc.h>
#include <nvgpu/gr/zcull.h>
#include <nvgpu/netlist.h>
#include <nvgpu/gr/gr_falcon.h>
#include <nvgpu/gr/ctx.h>
#include <nvgpu/gr/hwpm_map.h>
#include <nvgpu/gr/obj_ctx.h>
#include <nvgpu/gr/fs_state.h>
#include <nvgpu/gr/fecs_trace.h>
#include <nvgpu/power_features/cg.h>
#include <nvgpu/power_features/pg.h>

#include "gr_priv.h"

static int gr_alloc_global_ctx_buffers(struct gk20a *g)
{
	struct nvgpu_gr *gr = g->gr;
	int err;
	u32 size;

	nvgpu_log_fn(g, " ");

	size = g->ops.gr.init.get_global_ctx_cb_buffer_size(g);
	nvgpu_log_info(g, "cb_buffer_size : %d", size);

	nvgpu_gr_global_ctx_set_size(gr->global_ctx_buffer,
		NVGPU_GR_GLOBAL_CTX_CIRCULAR, size);
	nvgpu_gr_global_ctx_set_size(gr->global_ctx_buffer,
		NVGPU_GR_GLOBAL_CTX_CIRCULAR_VPR, size);

	size = g->ops.gr.init.get_global_ctx_pagepool_buffer_size(g);
	nvgpu_log_info(g, "pagepool_buffer_size : %d", size);

	nvgpu_gr_global_ctx_set_size(gr->global_ctx_buffer,
		NVGPU_GR_GLOBAL_CTX_PAGEPOOL, size);
	nvgpu_gr_global_ctx_set_size(gr->global_ctx_buffer,
		NVGPU_GR_GLOBAL_CTX_PAGEPOOL_VPR, size);

	size = g->ops.gr.init.get_global_attr_cb_size(g,
			nvgpu_gr_config_get_tpc_count(g->gr->config),
			nvgpu_gr_config_get_max_tpc_count(g->gr->config));
	nvgpu_log_info(g, "attr_buffer_size : %u", size);

	nvgpu_gr_global_ctx_set_size(gr->global_ctx_buffer,
		NVGPU_GR_GLOBAL_CTX_ATTRIBUTE, size);
	nvgpu_gr_global_ctx_set_size(gr->global_ctx_buffer,
		NVGPU_GR_GLOBAL_CTX_ATTRIBUTE_VPR, size);

	size = NVGPU_GR_GLOBAL_CTX_PRIV_ACCESS_MAP_SIZE;
	nvgpu_log_info(g, "priv_access_map_size : %d", size);

	nvgpu_gr_global_ctx_set_size(gr->global_ctx_buffer,
		NVGPU_GR_GLOBAL_CTX_PRIV_ACCESS_MAP, size);

#ifdef CONFIG_GK20A_CTXSW_TRACE
	size = nvgpu_gr_fecs_trace_buffer_size(g);
	nvgpu_log_info(g, "fecs_trace_buffer_size : %d", size);

	nvgpu_gr_global_ctx_set_size(gr->global_ctx_buffer,
		NVGPU_GR_GLOBAL_CTX_FECS_TRACE_BUFFER, size);
#endif

	if (g->ops.gr.init.get_rtv_cb_size != NULL) {
		size = g->ops.gr.init.get_rtv_cb_size(g);
		nvgpu_log_info(g, "rtv_circular_buffer_size : %u", size);

		nvgpu_gr_global_ctx_set_size(gr->global_ctx_buffer,
			NVGPU_GR_GLOBAL_CTX_RTV_CIRCULAR_BUFFER, size);
	}

	err = nvgpu_gr_global_ctx_buffer_alloc(g, gr->global_ctx_buffer);
	if (err != 0) {
		return err;
	}

	nvgpu_log_fn(g, "done");
	return 0;
}

u32 nvgpu_gr_get_no_of_sm(struct gk20a *g)
{
	return nvgpu_gr_config_get_no_of_sm(g->gr->config);
}

u32 nvgpu_gr_gpc_offset(struct gk20a *g, u32 gpc)
{
	u32 gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_GPC_STRIDE);
	u32 gpc_offset = gpc_stride * gpc;

	return gpc_offset;
}

u32 nvgpu_gr_tpc_offset(struct gk20a *g, u32 tpc)
{
	u32 tpc_in_gpc_stride = nvgpu_get_litter_value(g,
					GPU_LIT_TPC_IN_GPC_STRIDE);
	u32 tpc_offset = tpc_in_gpc_stride * tpc;

	return tpc_offset;
}

void nvgpu_gr_init(struct gk20a *g)
{
	nvgpu_cond_init(&g->gr->init_wq);
}

int nvgpu_gr_suspend(struct gk20a *g)
{
	int ret = 0;

	nvgpu_log_fn(g, " ");

	ret = g->ops.gr.init.wait_empty(g);
	if (ret != 0) {
		return ret;
	}

	/* Disable fifo access */
	g->ops.gr.init.fifo_access(g, false);

	/* disable gr intr */
	g->ops.gr.intr.enable_interrupts(g, false);

	/* disable all exceptions */
	g->ops.gr.intr.enable_exceptions(g, g->gr->config, false);

	g->ops.gr.intr.flush_channel_tlb(g);

	g->gr->initialized = false;

	nvgpu_log_fn(g, "done");
	return ret;
}


static int gr_init_setup_hw(struct gk20a *g)
{
	struct nvgpu_gr *gr = g->gr;
	int err;

	nvgpu_log_fn(g, " ");

	if (g->ops.gr.init.gpc_mmu != NULL) {
		g->ops.gr.init.gpc_mmu(g);
	}

	/* load gr floorsweeping registers */
	g->ops.gr.init.pes_vsc_stream(g);

	err = nvgpu_gr_zcull_init_hw(g, gr->zcull, gr->config);
	if (err != 0) {
		goto out;
	}

	if (g->ops.priv_ring.set_ppriv_timeout_settings != NULL) {
		g->ops.priv_ring.set_ppriv_timeout_settings(g);
	}

	/* enable fifo access */
	g->ops.gr.init.fifo_access(g, true);

	/* TBD: reload gr ucode when needed */

	/* enable interrupts */
	g->ops.gr.intr.enable_interrupts(g, true);

	/* enable fecs error interrupts */
	g->ops.gr.falcon.fecs_host_int_enable(g);

	g->ops.gr.intr.enable_hww_exceptions(g);
	g->ops.gr.set_hww_esr_report_mask(g);

	/* enable TPC exceptions per GPC */
	if (g->ops.gr.intr.enable_gpc_exceptions != NULL) {
		g->ops.gr.intr.enable_gpc_exceptions(g, gr->config);
	}

	/* enable ECC for L1/SM */
	if (g->ops.gr.init.ecc_scrub_reg != NULL) {
		g->ops.gr.init.ecc_scrub_reg(g, gr->config);
	}

	/* TBD: enable per BE exceptions */

	/* reset and enable exceptions */
	g->ops.gr.intr.enable_exceptions(g, gr->config, true);

	err = nvgpu_gr_zbc_load_table(g, gr->zbc);
	if (err != 0) {
		goto out;
	}

	/*
	 * Disable both surface and LG coalesce.
	 */
	if (g->ops.gr.init.su_coalesce != NULL) {
		g->ops.gr.init.su_coalesce(g, 0);
	}

	if (g->ops.gr.init.lg_coalesce != NULL) {
		g->ops.gr.init.lg_coalesce(g, 0);
	}

	if (g->ops.gr.init.preemption_state != NULL) {
		err = g->ops.gr.init.preemption_state(g);
		if (err != 0) {
			goto out;
		}
	}

	/* floorsweep anything left */
	err = nvgpu_gr_fs_state_init(g, gr->config);
	if (err != 0) {
		goto out;
	}

	err = g->ops.gr.init.wait_idle(g);
out:
	nvgpu_log_fn(g, "done");
	return err;
}

static void gr_remove_support(struct gk20a *g)
{
	struct nvgpu_gr *gr = g->gr;

	nvgpu_log_fn(g, " ");

	nvgpu_gr_global_ctx_buffer_free(g, gr->global_ctx_buffer);
	nvgpu_gr_global_ctx_desc_free(g, gr->global_ctx_buffer);

	nvgpu_gr_ctx_desc_free(g, gr->gr_ctx_desc);

	nvgpu_gr_config_deinit(g, gr->config);

	nvgpu_kfree(g, gr->fbp_rop_l2_en_mask);
	gr->fbp_rop_l2_en_mask = NULL;

	nvgpu_netlist_deinit_ctx_vars(g);

	nvgpu_gr_hwpm_map_deinit(g, gr->hwpm_map);

	nvgpu_gr_falcon_remove_support(g, gr->falcon);
	gr->falcon = NULL;

	nvgpu_gr_intr_remove_support(g, gr->intr);
	gr->intr = NULL;

	nvgpu_gr_zbc_deinit(g, gr->zbc);
	nvgpu_gr_zcull_deinit(g, gr->zcull);
	nvgpu_gr_obj_ctx_deinit(g, gr->golden_image);
}

static int gr_init_access_map(struct gk20a *g, struct nvgpu_gr *gr)
{
	struct nvgpu_mem *mem;
	u32 nr_pages =
		DIV_ROUND_UP(NVGPU_GR_GLOBAL_CTX_PRIV_ACCESS_MAP_SIZE,
			     PAGE_SIZE);
	u32 *whitelist = NULL;
	int w, num_entries = 0;

	mem = nvgpu_gr_global_ctx_buffer_get_mem(gr->global_ctx_buffer,
			NVGPU_GR_GLOBAL_CTX_PRIV_ACCESS_MAP);
	if (mem == NULL) {
		return -EINVAL;
	}

	nvgpu_memset(g, mem, 0, 0, PAGE_SIZE * nr_pages);

	g->ops.gr.init.get_access_map(g, &whitelist, &num_entries);

	for (w = 0; w < num_entries; w++) {
		u32 map_bit, map_byte, map_shift, x;
		map_bit = whitelist[w] >> 2;
		map_byte = map_bit >> 3;
		map_shift = map_bit & 0x7U; /* i.e. 0-7 */
		nvgpu_log_info(g, "access map addr:0x%x byte:0x%x bit:%d",
			       whitelist[w], map_byte, map_shift);
		x = nvgpu_mem_rd32(g, mem, map_byte / (u32)sizeof(u32));
		x |= BIT32(
			   (map_byte % sizeof(u32) * BITS_PER_BYTE)
			  + map_shift);
		nvgpu_mem_wr32(g, mem, map_byte / (u32)sizeof(u32), x);
	}

	return 0;
}

static int gr_init_config(struct gk20a *g, struct nvgpu_gr *gr)
{
	gr->config = nvgpu_gr_config_init(g);
	if (gr->config == NULL) {
		return -ENOMEM;
	}

	gr->num_fbps = g->ops.priv_ring.get_fbp_count(g);
	gr->max_fbps_count = g->ops.top.get_max_fbps_count(g);

	gr->fbp_en_mask = g->ops.gr.init.get_fbp_en_mask(g);

	if (gr->fbp_rop_l2_en_mask == NULL) {
		gr->fbp_rop_l2_en_mask =
			nvgpu_kzalloc(g, gr->max_fbps_count * sizeof(u32));
		if (gr->fbp_rop_l2_en_mask == NULL) {
			goto clean_up;
		}
	} else {
		(void) memset(gr->fbp_rop_l2_en_mask, 0, gr->max_fbps_count *
				sizeof(u32));
	}

	nvgpu_log_info(g, "fbps: %d", gr->num_fbps);
	nvgpu_log_info(g, "max_fbps_count: %d", gr->max_fbps_count);

	nvgpu_log_info(g, "bundle_cb_default_size: %d",
		g->ops.gr.init.get_bundle_cb_default_size(g));
	nvgpu_log_info(g, "min_gpm_fifo_depth: %d",
		g->ops.gr.init.get_min_gpm_fifo_depth(g));
	nvgpu_log_info(g, "bundle_cb_token_limit: %d",
		g->ops.gr.init.get_bundle_cb_token_limit(g));
	nvgpu_log_info(g, "attrib_cb_default_size: %d",
		g->ops.gr.init.get_attrib_cb_default_size(g));
	nvgpu_log_info(g, "attrib_cb_size: %d",
		g->ops.gr.init.get_attrib_cb_size(g,
			nvgpu_gr_config_get_tpc_count(gr->config)));
	nvgpu_log_info(g, "alpha_cb_default_size: %d",
		g->ops.gr.init.get_alpha_cb_default_size(g));
	nvgpu_log_info(g, "alpha_cb_size: %d",
		g->ops.gr.init.get_alpha_cb_size(g,
			nvgpu_gr_config_get_tpc_count(gr->config)));

	return 0;

clean_up:
	return -ENOMEM;
}

static int nvgpu_gr_init_ctx_state(struct gk20a *g)
{
	if (g->gr->golden_image != NULL &&
			nvgpu_gr_obj_ctx_is_golden_image_ready(g->gr->golden_image)) {
		return 0;
	}

	return nvgpu_gr_falcon_init_ctx_state(g, g->gr->falcon);
}

static int gr_init_setup_sw(struct gk20a *g)
{
	struct nvgpu_gr *gr = g->gr;
	int err = 0;

	nvgpu_log_fn(g, " ");

	if (gr->sw_ready) {
		nvgpu_log_fn(g, "skip init");
		return 0;
	}

	gr->g = g;

	err = nvgpu_mutex_init(&gr->ctxsw_disable_mutex);
	if (err != 0) {
		nvgpu_err(g, "Error in ctxsw_disable_mutex init");
		return err;
	}
	gr->ctxsw_disable_count = 0;

	err = nvgpu_gr_obj_ctx_init(g, &gr->golden_image,
			nvgpu_gr_falcon_get_golden_image_size(g->gr->falcon));
	if (err != 0) {
		goto clean_up;
	}

	err = gr_init_config(g, gr);
	if (err != 0) {
		goto clean_up;
	}

	err = nvgpu_gr_hwpm_map_init(g, &g->gr->hwpm_map,
			nvgpu_gr_falcon_get_pm_ctxsw_image_size(g->gr->falcon));
	if (err != 0) {
		nvgpu_err(g, "hwpm_map init failed");
		goto clean_up;
	}

	err = nvgpu_gr_config_init_map_tiles(g, gr->config);
	if (err != 0) {
		goto clean_up;
	}

	err = nvgpu_gr_zcull_init(g, &gr->zcull,
			nvgpu_gr_falcon_get_zcull_image_size(g->gr->falcon),
			g->gr->config);
	if (err != 0) {
		goto clean_up;
	}

	gr->gr_ctx_desc = nvgpu_gr_ctx_desc_alloc(g);
	if (gr->gr_ctx_desc == NULL) {
		goto clean_up;
	}

	nvgpu_gr_ctx_set_size(g->gr->gr_ctx_desc, NVGPU_GR_CTX_PREEMPT_CTXSW,
			nvgpu_gr_falcon_get_preempt_image_size(g->gr->falcon));

	gr->global_ctx_buffer = nvgpu_gr_global_ctx_desc_alloc(g);
	if (gr->global_ctx_buffer == NULL) {
		goto clean_up;
	}

	err = gr_alloc_global_ctx_buffers(g);
	if (err != 0) {
		goto clean_up;
	}

	err = gr_init_access_map(g, gr);
	if (err != 0) {
		goto clean_up;
	}

	err = nvgpu_gr_zbc_init(g, &gr->zbc);
	if (err != 0) {
		goto clean_up;
	}

	gr->intr = nvgpu_gr_intr_init_support(g);
	if (gr->intr == NULL) {
		err = -ENOMEM;
		goto clean_up;
	}

	gr->remove_support = gr_remove_support;
	gr->sw_ready = true;

	nvgpu_log_fn(g, "done");
	return 0;

clean_up:
	nvgpu_err(g, "fail");
	gr_remove_support(g);
	return err;
}

static int gr_init_reset_enable_hw(struct gk20a *g)
{
	struct netlist_av_list *sw_non_ctx_load =
		nvgpu_netlist_get_sw_non_ctx_load_av_list(g);
	u32 i;
	int err = 0;

	nvgpu_log_fn(g, " ");

	/* enable interrupts */
	g->ops.gr.intr.enable_interrupts(g, true);

	/* load non_ctx init */
	for (i = 0; i < sw_non_ctx_load->count; i++) {
		nvgpu_writel(g, sw_non_ctx_load->l[i].addr,
			sw_non_ctx_load->l[i].value);
	}

	err = g->ops.gr.falcon.wait_mem_scrubbing(g);
	if (err != 0) {
		goto out;
	}

	err = g->ops.gr.init.wait_idle(g);
	if (err != 0) {
		goto out;
	}

out:
	if (err != 0) {
		nvgpu_err(g, "fail");
	} else {
		nvgpu_log_fn(g, "done");
	}

	return 0;
}

int nvgpu_gr_prepare_sw(struct gk20a *g)
{
	struct nvgpu_gr *gr = g->gr;
	int err = 0;

	nvgpu_log_fn(g, " ");

	err = nvgpu_netlist_init_ctx_vars(g);
	if (err != 0) {
		nvgpu_err(g, "failed to parse netlist");
		return err;
	}

	if (gr->falcon == NULL) {
		gr->falcon = nvgpu_gr_falcon_init_support(g);
		if (gr->falcon == NULL) {
			nvgpu_err(g, "failed to init gr falcon");
			err = -ENOMEM;
		}
	}
	return err;
}

static void gr_init_prepare_hw(struct gk20a *g)
{
	/* reset gr engine */
	g->ops.mc.reset(g, g->ops.mc.reset_mask(g, NVGPU_UNIT_GRAPH) |
			g->ops.mc.reset_mask(g, NVGPU_UNIT_BLG) |
			g->ops.mc.reset_mask(g, NVGPU_UNIT_PERFMON));

	nvgpu_cg_init_gr_load_gating_prod(g);

	/* Disable elcg until it gets enabled later in the init*/
	nvgpu_cg_elcg_disable_no_wait(g);

	/* enable fifo access */
	g->ops.gr.init.fifo_access(g, true);
}

int nvgpu_gr_enable_hw(struct gk20a *g)
{
	int err;

	nvgpu_log_fn(g, " ");

	gr_init_prepare_hw(g);

	err = gr_init_reset_enable_hw(g);
	if (err != 0) {
		return err;
	}

	nvgpu_log_fn(g, "done");

	return 0;
}

int nvgpu_gr_reset(struct gk20a *g)
{
	int err;
	struct nvgpu_mutex *fecs_mutex =
		nvgpu_gr_falcon_get_fecs_mutex(g->gr->falcon);

	g->gr->initialized = false;

	nvgpu_mutex_acquire(fecs_mutex);

	err = nvgpu_gr_enable_hw(g);
	if (err != 0) {
		nvgpu_mutex_release(fecs_mutex);
		return err;
	}

	err = gr_init_setup_hw(g);
	if (err != 0) {
		nvgpu_mutex_release(fecs_mutex);
		return err;
	}

	err = nvgpu_gr_falcon_init_ctxsw(g, g->gr->falcon);
	if (err != 0) {
		nvgpu_mutex_release(fecs_mutex);
		return err;
	}

	nvgpu_mutex_release(fecs_mutex);

	/* this appears query for sw states but fecs actually init
	   ramchain, etc so this is hw init */
	err = nvgpu_gr_init_ctx_state(g);
	if (err != 0) {
		return err;
	}

	if (g->can_elpg) {
		err = nvgpu_gr_falcon_bind_fecs_elpg(g);
		if (err != 0) {
			return err;
		}
	}

	nvgpu_cg_init_gr_load_gating_prod(g);

	nvgpu_cg_elcg_enable_no_wait(g);

	/* GR is inialized, signal possible waiters */
	g->gr->initialized = true;
	nvgpu_cond_signal(&g->gr->init_wq);
	return err;
}

int nvgpu_gr_init_support(struct gk20a *g)
{
	int err = 0;

	nvgpu_log_fn(g, " ");

	g->gr->initialized = false;

	err = nvgpu_gr_falcon_init_ctxsw(g, g->gr->falcon);
	if (err != 0) {
		return err;
	}

	/* this appears query for sw states but fecs actually init
	   ramchain, etc so this is hw init */
	err = nvgpu_gr_init_ctx_state(g);
	if (err != 0) {
		return err;
	}

	if (g->can_elpg) {
		err = nvgpu_gr_falcon_bind_fecs_elpg(g);
		if (err != 0) {
			return err;
		}
	}

	err = gr_init_setup_sw(g);
	if (err != 0) {
		return err;
	}

	err = gr_init_setup_hw(g);
	if (err != 0) {
		return err;
	}

	nvgpu_cg_elcg_enable_no_wait(g);

	/* GR is inialized, signal possible waiters */
	g->gr->initialized = true;
	nvgpu_cond_signal(&g->gr->init_wq);

	return 0;
}

/* Wait until GR is initialized */
void nvgpu_gr_wait_initialized(struct gk20a *g)
{
	NVGPU_COND_WAIT(&g->gr->init_wq, g->gr->initialized, 0U);
}

int nvgpu_gr_alloc(struct gk20a *g)
{
	struct nvgpu_gr *gr = NULL;

	/* if gr exists return */
	if ((g != NULL) && (g->gr != NULL)) {
		return 0;
	}

	/* Allocate memory for gr struct */
	gr = nvgpu_kzalloc(g, sizeof(*gr));
	if (gr == NULL) {
		return -ENOMEM;
	}
	g->gr = gr;

	return 0;
}

void nvgpu_gr_free(struct gk20a *g)
{
	/*Delete gr memory */
	if (g->gr != NULL) {
		nvgpu_kfree(g, g->gr);
	}
	g->gr = NULL;
}

/**
 * Stop processing (stall) context switches at FECS:-
 * If fecs is sent stop_ctxsw method, elpg entry/exit cannot happen
 * and may timeout. It could manifest as different error signatures
 * depending on when stop_ctxsw fecs method gets sent with respect
 * to pmu elpg sequence. It could come as pmu halt or abort or
 * maybe ext error too.
 */
int nvgpu_gr_disable_ctxsw(struct gk20a *g)
{
	struct nvgpu_gr *gr = g->gr;
	int err = 0;

	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg, " ");

	nvgpu_mutex_acquire(&gr->ctxsw_disable_mutex);
	gr->ctxsw_disable_count++;
	if (gr->ctxsw_disable_count == 1) {
		err = nvgpu_pg_elpg_disable(g);
		if (err != 0) {
			nvgpu_err(g,
				"failed to disable elpg for stop_ctxsw");
			/* stop ctxsw command is not sent */
			gr->ctxsw_disable_count--;
		} else {
			err = g->ops.gr.falcon.ctrl_ctxsw(g,
				NVGPU_GR_FALCON_METHOD_CTXSW_STOP, 0U, NULL);
			if (err != 0) {
				nvgpu_err(g, "failed to stop fecs ctxsw");
				/* stop ctxsw failed */
				gr->ctxsw_disable_count--;
			}
		}
	} else {
		nvgpu_log_info(g, "ctxsw disabled, ctxsw_disable_count: %d",
			gr->ctxsw_disable_count);
	}
	nvgpu_mutex_release(&gr->ctxsw_disable_mutex);

	return err;
}

/* Start processing (continue) context switches at FECS */
int nvgpu_gr_enable_ctxsw(struct gk20a *g)
{
	struct nvgpu_gr *gr = g->gr;
	int err = 0;

	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg, " ");

	nvgpu_mutex_acquire(&gr->ctxsw_disable_mutex);
	if (gr->ctxsw_disable_count == 0) {
		goto ctxsw_already_enabled;
	}
	gr->ctxsw_disable_count--;
	WARN_ON(gr->ctxsw_disable_count < 0);
	if (gr->ctxsw_disable_count == 0) {
		err = g->ops.gr.falcon.ctrl_ctxsw(g,
				NVGPU_GR_FALCON_METHOD_CTXSW_START, 0U, NULL);
		if (err != 0) {
			nvgpu_err(g, "failed to start fecs ctxsw");
		} else {
			if (nvgpu_pg_elpg_enable(g) != 0) {
				nvgpu_err(g,
					"failed to enable elpg for start_ctxsw");
			}
		}
	} else {
		nvgpu_log_info(g, "ctxsw_disable_count: %d is not 0 yet",
			gr->ctxsw_disable_count);
	}
ctxsw_already_enabled:
	nvgpu_mutex_release(&gr->ctxsw_disable_mutex);

	return err;
}

int nvgpu_gr_halt_pipe(struct gk20a *g)
{
	return g->ops.gr.falcon.ctrl_ctxsw(g,
				NVGPU_GR_FALCON_METHOD_HALT_PIPELINE, 0U, NULL);
}

void nvgpu_gr_remove_support(struct gk20a *g)
{
	if (g->gr->remove_support != NULL) {
		g->gr->remove_support(g);
	}
}

void nvgpu_gr_sw_ready(struct gk20a *g, bool enable)
{
	g->gr->sw_ready = enable;
}

void nvgpu_gr_override_ecc_val(struct gk20a *g, u32 ecc_val)
{
	g->gr->fecs_feature_override_ecc_val = ecc_val;
}
