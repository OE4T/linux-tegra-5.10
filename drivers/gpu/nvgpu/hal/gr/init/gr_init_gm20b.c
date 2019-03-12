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

#include "gr_init_gm20b.h"

#include <nvgpu/hw/gm20b/hw_gr_gm20b.h>

#define FE_PWR_MODE_TIMEOUT_MAX_US 2000U
#define FE_PWR_MODE_TIMEOUT_DEFAULT_US 10U

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
