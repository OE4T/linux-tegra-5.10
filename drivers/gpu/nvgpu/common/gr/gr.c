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
#include <nvgpu/gr/gr.h>
#include <nvgpu/gr/config.h>
#include <nvgpu/gr/zbc.h>
#include <nvgpu/gr/zcull.h>
#include <nvgpu/gr/gr_falcon.h>
#include <nvgpu/gr/ctx.h>
#include <nvgpu/unit.h>
#include <nvgpu/gr/hwpm_map.h>
#include <nvgpu/gr/obj_ctx.h>
#include <nvgpu/gr/fs_state.h>
#include <nvgpu/power_features/cg.h>

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
	g->ops.gr.intr.enable_exceptions(g, g->gr.config, false);

	nvgpu_gr_flush_channel_tlb(g);

	g->gr.initialized = false;

	nvgpu_log_fn(g, "done");
	return ret;
}

/* invalidate channel lookup tlb */
void nvgpu_gr_flush_channel_tlb(struct gk20a *g)
{
	nvgpu_spinlock_acquire(&g->gr.ch_tlb_lock);
	(void) memset(g->gr.chid_tlb, 0,
		sizeof(struct gr_channel_map_tlb_entry) *
		GR_CHANNEL_MAP_TLB_SIZE);
	nvgpu_spinlock_release(&g->gr.ch_tlb_lock);
}

static int gr_init_setup_hw(struct gk20a *g)
{
	struct gr_gk20a *gr = &g->gr;
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
		err = g->ops.gr.init.preemption_state(g, gr->gfxp_wfi_timeout_count,
			gr->gfxp_wfi_timeout_unit_usec);
		if (err != 0) {
			goto out;
		}
	}

	/* floorsweep anything left */
	err = nvgpu_gr_fs_state_init(g);
	if (err != 0) {
		goto out;
	}

	err = g->ops.gr.init.wait_idle(g);
out:
	nvgpu_log_fn(g, "done");
	return err;
}

static void gr_remove_support(struct gr_gk20a *gr)
{
	struct gk20a *g = gr->g;

	nvgpu_log_fn(g, " ");

	gr_gk20a_free_cyclestats_snapshot_data(g);

	nvgpu_gr_global_ctx_buffer_free(g, gr->global_ctx_buffer);
	nvgpu_gr_global_ctx_desc_free(g, gr->global_ctx_buffer);

	nvgpu_gr_ctx_desc_free(g, gr->gr_ctx_desc);

	nvgpu_gr_config_deinit(g, gr->config);

	nvgpu_kfree(g, gr->fbp_rop_l2_en_mask);
	gr->fbp_rop_l2_en_mask = NULL;

	nvgpu_netlist_deinit_ctx_vars(g);

	nvgpu_gr_hwpm_map_deinit(g, gr->hwpm_map);

	nvgpu_ecc_remove_support(g);
	nvgpu_gr_zbc_deinit(g, gr->zbc);
	nvgpu_gr_zcull_deinit(g, gr->zcull);
	nvgpu_gr_obj_ctx_deinit(g, gr->golden_image);
	gr->ctx_vars.golden_image_initialized = false;
}

static int gr_init_access_map(struct gk20a *g, struct gr_gk20a *gr)
{
	struct nvgpu_mem *mem;
	u32 nr_pages =
		DIV_ROUND_UP(gr->ctx_vars.priv_access_map_size,
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

static int gr_init_config(struct gk20a *g, struct gr_gk20a *gr)
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

static int gr_init_setup_sw(struct gk20a *g)
{
	struct gr_gk20a *gr = &g->gr;
	int err = 0;

	nvgpu_log_fn(g, " ");

	if (gr->sw_ready) {
		nvgpu_log_fn(g, "skip init");
		return 0;
	}

	gr->g = g;

#if defined(CONFIG_GK20A_CYCLE_STATS)
	err = nvgpu_mutex_init(&g->gr.cs_lock);
	if (err != 0) {
		nvgpu_err(g, "Error in gr.cs_lock mutex initialization");
		return err;
	}
#endif

	err = nvgpu_gr_obj_ctx_init(g, &gr->golden_image,
			g->gr.ctx_vars.golden_image_size);
	if (err != 0) {
		goto clean_up;
	}

	err = gr_init_config(g, gr);
	if (err != 0) {
		goto clean_up;
	}

	err = nvgpu_gr_config_init_map_tiles(g, gr->config);
	if (err != 0) {
		goto clean_up;
	}

	err = nvgpu_gr_zcull_init(g, &gr->zcull);
	if (err != 0) {
		goto clean_up;
	}

	gr->gr_ctx_desc = nvgpu_gr_ctx_desc_alloc(g);
	if (gr->gr_ctx_desc == NULL) {
		goto clean_up;
	}

	gr->global_ctx_buffer = nvgpu_gr_global_ctx_desc_alloc(g);
	if (gr->global_ctx_buffer == NULL) {
		goto clean_up;
	}

	err = g->ops.gr.alloc_global_ctx_buffers(g);
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

	if (g->ops.gr.init_gfxp_wfi_timeout_count != NULL) {
		g->ops.gr.init_gfxp_wfi_timeout_count(g);
	}

	err = nvgpu_mutex_init(&gr->ctx_mutex);
	if (err != 0) {
		nvgpu_err(g, "Error in gr.ctx_mutex initialization");
		goto clean_up;
	}

	nvgpu_spinlock_init(&gr->ch_tlb_lock);

	gr->remove_support = gr_remove_support;
	gr->sw_ready = true;

	err = nvgpu_ecc_init_support(g);
	if (err != 0) {
		goto clean_up;
	}

	nvgpu_log_fn(g, "done");
	return 0;

clean_up:
	nvgpu_err(g, "fail");
	gr_remove_support(gr);
	return err;
}

static int gr_init_reset_enable_hw(struct gk20a *g)
{
	struct netlist_av_list *sw_non_ctx_load =
			&g->netlist_vars->sw_non_ctx_load;
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

static void gr_init_prepare(struct gk20a *g)
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

	gr_init_prepare(g);

	err = nvgpu_netlist_init_ctx_vars(g);
	if (err != 0) {
		nvgpu_err(g, "failed to parse netlist");
		return err;
	}

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

	g->gr.initialized = false;

	nvgpu_mutex_acquire(&g->gr.fecs_mutex);

	err = nvgpu_gr_enable_hw(g);
	if (err != 0) {
		nvgpu_mutex_release(&g->gr.fecs_mutex);
		return err;
	}

	err = gr_init_setup_hw(g);
	if (err != 0) {
		nvgpu_mutex_release(&g->gr.fecs_mutex);
		return err;
	}

	err = nvgpu_gr_falcon_init_ctxsw(g);
	if (err != 0) {
		nvgpu_mutex_release(&g->gr.fecs_mutex);
		return err;
	}

	nvgpu_mutex_release(&g->gr.fecs_mutex);

	/* this appears query for sw states but fecs actually init
	   ramchain, etc so this is hw init */
	err = nvgpu_gr_falcon_init_ctx_state(g);
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
	g->gr.initialized = true;
	nvgpu_cond_signal(&g->gr.init_wq);
	return err;
}

int nvgpu_gr_init_support(struct gk20a *g)
{
	int err = 0;

	nvgpu_log_fn(g, " ");

	g->gr.initialized = false;

	/* this is required before gr_gk20a_init_ctx_state */
	err = nvgpu_mutex_init(&g->gr.fecs_mutex);
	if (err != 0) {
		nvgpu_err(g, "Error in gr.fecs_mutex initialization");
		return err;
	}

	err = nvgpu_gr_falcon_init_ctxsw(g);
	if (err != 0) {
		return err;
	}

	/* this appears query for sw states but fecs actually init
	   ramchain, etc so this is hw init */
	err = nvgpu_gr_falcon_init_ctx_state(g);
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
	g->gr.initialized = true;
	nvgpu_cond_signal(&g->gr.init_wq);

	return 0;
}

/* Wait until GR is initialized */
void nvgpu_gr_wait_initialized(struct gk20a *g)
{
	NVGPU_COND_WAIT(&g->gr.init_wq, g->gr.initialized, 0U);
}
