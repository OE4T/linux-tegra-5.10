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
#include <nvgpu/gr/gr.h>

#include "gr_init_gm20b.h"

#include <nvgpu/hw/gm20b/hw_gr_gm20b.h>

#define FE_PWR_MODE_TIMEOUT_MAX_US 2000U
#define FE_PWR_MODE_TIMEOUT_DEFAULT_US 10U
#define FECS_CTXSW_RESET_DELAY_US 10U

int gm20b_gr_init_wait_idle(struct gk20a *g)
{
	u32 delay = NVGPU_GR_IDLE_CHECK_DEFAULT_US;
	u32 gr_engine_id;
	int err = -EAGAIN;
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

	return err;
}

int gm20b_gr_init_wait_fe_idle(struct gk20a *g)
{
	u32 val;
	u32 delay = NVGPU_GR_IDLE_CHECK_DEFAULT_US;
	struct nvgpu_timeout timeout;
	int err = -EAGAIN;

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

	return err;
}

int gm20b_gr_init_fe_pwr_mode_force_on(struct gk20a *g, bool force_on)
{
	struct nvgpu_timeout timeout;
	int ret = -ETIMEDOUT;
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

	nvgpu_timeout_init(g, &timeout,
			   FE_PWR_MODE_TIMEOUT_MAX_US /
				FE_PWR_MODE_TIMEOUT_DEFAULT_US,
			   NVGPU_TIMER_RETRY_TIMER);

	nvgpu_writel(g, gr_fe_pwr_mode_r(), reg_val);

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
