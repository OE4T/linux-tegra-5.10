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
#include <nvgpu/timers.h>
#include <nvgpu/enabled.h>
#include <nvgpu/engine_status.h>
#include <nvgpu/netlist.h>

#include <nvgpu/gr/gr.h>
#include <nvgpu/gr/config.h>

#include "gr_init_gm20b.h"

#include <nvgpu/hw/gm20b/hw_gr_gm20b.h>

#define FE_PWR_MODE_TIMEOUT_MAX_US 2000U
#define FE_PWR_MODE_TIMEOUT_DEFAULT_US 10U
#define FECS_CTXSW_RESET_DELAY_US 10U

int gm20b_gr_init_fs_state(struct gk20a *g)
{
	int err = 0;

	nvgpu_log_fn(g, " ");

	nvgpu_writel(g, gr_bes_zrop_settings_r(),
		     gr_bes_zrop_settings_num_active_ltcs_f(g->ltc_count));
	nvgpu_writel(g, gr_bes_crop_settings_r(),
		     gr_bes_crop_settings_num_active_ltcs_f(g->ltc_count));

	nvgpu_writel(g, gr_bes_crop_debug3_r(),
		     gk20a_readl(g, gr_be0_crop_debug3_r()) |
		     gr_bes_crop_debug3_comp_vdc_4to2_disable_m());

	return err;
}

void gm20b_gr_init_pd_tpc_per_gpc(struct gk20a *g)
{
	u32 reg_index;
	u32 tpc_per_gpc;
	u32 gpc_id = 0;
	struct nvgpu_gr_config *gr_config = g->gr.config;

	for (reg_index = 0U, gpc_id = 0U;
	     reg_index < gr_pd_num_tpc_per_gpc__size_1_v();
	     reg_index++, gpc_id += 8U) {

		tpc_per_gpc =
		 gr_pd_num_tpc_per_gpc_count0_f(
		  nvgpu_gr_config_get_gpc_tpc_count(gr_config, gpc_id + 0U)) |
		 gr_pd_num_tpc_per_gpc_count1_f(
		  nvgpu_gr_config_get_gpc_tpc_count(gr_config, gpc_id + 1U)) |
		 gr_pd_num_tpc_per_gpc_count2_f(
		  nvgpu_gr_config_get_gpc_tpc_count(gr_config, gpc_id + 2U)) |
		 gr_pd_num_tpc_per_gpc_count3_f(
		  nvgpu_gr_config_get_gpc_tpc_count(gr_config, gpc_id + 3U)) |
		 gr_pd_num_tpc_per_gpc_count4_f(
		  nvgpu_gr_config_get_gpc_tpc_count(gr_config, gpc_id + 4U)) |
		 gr_pd_num_tpc_per_gpc_count5_f(
		  nvgpu_gr_config_get_gpc_tpc_count(gr_config, gpc_id + 5U)) |
		 gr_pd_num_tpc_per_gpc_count6_f(
		  nvgpu_gr_config_get_gpc_tpc_count(gr_config, gpc_id + 6U)) |
		 gr_pd_num_tpc_per_gpc_count7_f(
		  nvgpu_gr_config_get_gpc_tpc_count(gr_config, gpc_id + 7U));

		nvgpu_writel(g, gr_pd_num_tpc_per_gpc_r(reg_index), tpc_per_gpc);
		nvgpu_writel(g, gr_ds_num_tpc_per_gpc_r(reg_index), tpc_per_gpc);
	}
}

void gm20b_gr_init_pd_skip_table_gpc(struct gk20a *g)
{
	u32 gpc_index;
	bool skip_mask;
	struct nvgpu_gr_config *gr_config = g->gr.config;

	for (gpc_index = 0;
	     gpc_index < gr_pd_dist_skip_table__size_1_v() * 4U;
	     gpc_index += 4U) {
		skip_mask =
		 (gr_pd_dist_skip_table_gpc_4n0_mask_f(
		   nvgpu_gr_config_get_gpc_skip_mask(gr_config,
						     gpc_index)) != 0U) ||
		 (gr_pd_dist_skip_table_gpc_4n1_mask_f(
		   nvgpu_gr_config_get_gpc_skip_mask(gr_config,
						     gpc_index + 1U)) != 0U) ||
		 (gr_pd_dist_skip_table_gpc_4n2_mask_f(
		   nvgpu_gr_config_get_gpc_skip_mask(gr_config,
						     gpc_index + 2U)) != 0U) ||
		 (gr_pd_dist_skip_table_gpc_4n3_mask_f(
		   nvgpu_gr_config_get_gpc_skip_mask(gr_config,
						     gpc_index + 3U)) != 0U);

		nvgpu_writel(g, gr_pd_dist_skip_table_r(gpc_index/4U),
			     (u32)skip_mask);
	}
}

void gm20b_gr_init_cwd_gpcs_tpcs_num(struct gk20a *g,
				     u32 gpc_count, u32 tpc_count)
{
	nvgpu_writel(g, gr_cwd_fs_r(),
		     gr_cwd_fs_num_gpcs_f(gpc_count) |
		     gr_cwd_fs_num_tpcs_f(tpc_count));
}

int gm20b_gr_init_wait_idle(struct gk20a *g)
{
	u32 delay = NVGPU_GR_IDLE_CHECK_DEFAULT_US;
	u32 gr_engine_id;
	int err = 0;
	bool ctxsw_active;
	bool gr_busy;
	bool ctx_status_invalid;
	struct nvgpu_engine_status_info engine_status;
	struct nvgpu_timeout timeout;

	nvgpu_log_fn(g, " ");

	gr_engine_id = nvgpu_engine_get_gr_id(g);

	err = nvgpu_timeout_init(g, &timeout, nvgpu_gr_get_idle_timeout(g),
				 NVGPU_TIMER_CPU_TIMER);
	if (err != 0) {
		return err;
	}

	do {
		/*
		 * fmodel: host gets fifo_engine_status(gr) from gr
		 * only when gr_status is read
		 */
		(void) nvgpu_readl(g, gr_status_r());

		g->ops.engine_status.read_engine_status_info(g, gr_engine_id,
							     &engine_status);

		ctxsw_active = engine_status.ctxsw_in_progress;

		ctx_status_invalid = nvgpu_engine_status_is_ctxsw_invalid(
							&engine_status);

		gr_busy = (nvgpu_readl(g, gr_engine_status_r()) &
				gr_engine_status_value_busy_f()) != 0U;

		if (ctx_status_invalid || (!gr_busy && !ctxsw_active)) {
			nvgpu_log_fn(g, "done");
			return 0;
		}

		nvgpu_usleep_range(delay, delay * 2U);
		delay = min_t(u32, delay << 1, NVGPU_GR_IDLE_CHECK_MAX_US);

	} while (nvgpu_timeout_expired(&timeout) == 0);

	nvgpu_err(g, "timeout, ctxsw busy : %d, gr busy : %d",
		  ctxsw_active, gr_busy);

	return -EAGAIN;
}

int gm20b_gr_init_wait_fe_idle(struct gk20a *g)
{
	u32 val;
	u32 delay = NVGPU_GR_IDLE_CHECK_DEFAULT_US;
	struct nvgpu_timeout timeout;
	int err = 0;

	if (nvgpu_is_enabled(g, NVGPU_IS_FMODEL)) {
		return 0;
	}

	nvgpu_log_fn(g, " ");

	err = nvgpu_timeout_init(g, &timeout, nvgpu_gr_get_idle_timeout(g),
			         NVGPU_TIMER_CPU_TIMER);
	if (err != 0) {
		return err;
	}

	do {
		val = nvgpu_readl(g, gr_status_r());

		if (gr_status_fe_method_lower_v(val) == 0U) {
			nvgpu_log_fn(g, "done");
			return 0;
		}

		nvgpu_usleep_range(delay, delay * 2U);
		delay = min_t(u32, delay << 1, NVGPU_GR_IDLE_CHECK_MAX_US);
	} while (nvgpu_timeout_expired(&timeout) == 0);

	nvgpu_err(g, "timeout, fe busy : %x", val);

	return -EAGAIN;
}

int gm20b_gr_init_fe_pwr_mode_force_on(struct gk20a *g, bool force_on)
{
	struct nvgpu_timeout timeout;
	int ret = 0;
	u32 reg_val;

	if (nvgpu_is_enabled(g, NVGPU_IS_FMODEL)) {
		return 0;
	}

	if (force_on) {
		reg_val = gr_fe_pwr_mode_req_send_f() |
			  gr_fe_pwr_mode_mode_force_on_f();
	} else {
		reg_val = gr_fe_pwr_mode_req_send_f() |
			  gr_fe_pwr_mode_mode_auto_f();
	}

	ret = nvgpu_timeout_init(g, &timeout,
			   FE_PWR_MODE_TIMEOUT_MAX_US /
				FE_PWR_MODE_TIMEOUT_DEFAULT_US,
			   NVGPU_TIMER_RETRY_TIMER);
	if (ret != 0) {
		return ret;
	}

	nvgpu_writel(g, gr_fe_pwr_mode_r(), reg_val);

	ret = -ETIMEDOUT;

	do {
		u32 req = gr_fe_pwr_mode_req_v(
				nvgpu_readl(g, gr_fe_pwr_mode_r()));
		if (req == gr_fe_pwr_mode_req_done_v()) {
			ret = 0;
			break;
		}

		nvgpu_udelay(FE_PWR_MODE_TIMEOUT_DEFAULT_US);
	} while (nvgpu_timeout_expired_msg(&timeout,
				"timeout setting FE mode %u", force_on) == 0);

	return ret;
}

void gm20b_gr_init_override_context_reset(struct gk20a *g)
{
	nvgpu_writel(g, gr_fecs_ctxsw_reset_ctl_r(),
			gr_fecs_ctxsw_reset_ctl_sys_halt_disabled_f() |
			gr_fecs_ctxsw_reset_ctl_gpc_halt_disabled_f() |
			gr_fecs_ctxsw_reset_ctl_be_halt_disabled_f() |
			gr_fecs_ctxsw_reset_ctl_sys_engine_reset_disabled_f() |
			gr_fecs_ctxsw_reset_ctl_gpc_engine_reset_disabled_f() |
			gr_fecs_ctxsw_reset_ctl_be_engine_reset_disabled_f() |
			gr_fecs_ctxsw_reset_ctl_sys_context_reset_enabled_f() |
			gr_fecs_ctxsw_reset_ctl_gpc_context_reset_enabled_f() |
			gr_fecs_ctxsw_reset_ctl_be_context_reset_enabled_f());

	nvgpu_udelay(FECS_CTXSW_RESET_DELAY_US);
	(void) nvgpu_readl(g, gr_fecs_ctxsw_reset_ctl_r());

	/* Deassert reset */
	nvgpu_writel(g, gr_fecs_ctxsw_reset_ctl_r(),
			gr_fecs_ctxsw_reset_ctl_sys_halt_disabled_f() |
			gr_fecs_ctxsw_reset_ctl_gpc_halt_disabled_f() |
			gr_fecs_ctxsw_reset_ctl_be_halt_disabled_f() |
			gr_fecs_ctxsw_reset_ctl_sys_engine_reset_disabled_f() |
			gr_fecs_ctxsw_reset_ctl_gpc_engine_reset_disabled_f() |
			gr_fecs_ctxsw_reset_ctl_be_engine_reset_disabled_f() |
			gr_fecs_ctxsw_reset_ctl_sys_context_reset_disabled_f() |
			gr_fecs_ctxsw_reset_ctl_gpc_context_reset_disabled_f() |
			gr_fecs_ctxsw_reset_ctl_be_context_reset_disabled_f());

	nvgpu_udelay(FECS_CTXSW_RESET_DELAY_US);
	(void) nvgpu_readl(g, gr_fecs_ctxsw_reset_ctl_r());
}

void gm20b_gr_init_fe_go_idle_timeout(struct gk20a *g, bool enable)
{
	if (enable) {
		nvgpu_writel(g, gr_fe_go_idle_timeout_r(),
			gr_fe_go_idle_timeout_count_prod_f());
	} else {
		nvgpu_writel(g, gr_fe_go_idle_timeout_r(),
			gr_fe_go_idle_timeout_count_disabled_f());
	}
}

void gm20b_gr_init_load_method_init(struct gk20a *g,
		struct netlist_av_list *sw_method_init)
{
	u32 i;
	u32 last_method_data = 0U;

	if (sw_method_init->count != 0U) {
		nvgpu_writel(g, gr_pri_mme_shadow_raw_data_r(),
			     sw_method_init->l[0U].value);
		nvgpu_writel(g, gr_pri_mme_shadow_raw_index_r(),
			     gr_pri_mme_shadow_raw_index_write_trigger_f() |
			     sw_method_init->l[0U].addr);
		last_method_data = sw_method_init->l[0U].value;
	}

	for (i = 1U; i < sw_method_init->count; i++) {
		if (sw_method_init->l[i].value != last_method_data) {
			nvgpu_writel(g, gr_pri_mme_shadow_raw_data_r(),
				sw_method_init->l[i].value);
			last_method_data = sw_method_init->l[i].value;
		}
		nvgpu_writel(g, gr_pri_mme_shadow_raw_index_r(),
			gr_pri_mme_shadow_raw_index_write_trigger_f() |
			sw_method_init->l[i].addr);
	}
}
