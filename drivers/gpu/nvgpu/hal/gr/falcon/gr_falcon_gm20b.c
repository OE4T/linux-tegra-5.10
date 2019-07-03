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
#include <nvgpu/gr/gr_falcon.h>
#include <nvgpu/io.h>
#include <nvgpu/debug.h>
#include <nvgpu/power_features/pg.h>
#include <nvgpu/soc.h>
#include <nvgpu/safe_ops.h>
#include <nvgpu/gr/gr_utils.h>
#include <nvgpu/pmu/pmuif/ctrlclk.h>

#include "gr_falcon_gm20b.h"
#include "common/gr/gr_falcon_priv.h"
#include <nvgpu/gr/gr_utils.h>

#include <nvgpu/hw/gm20b/hw_gr_gm20b.h>

#define GR_FECS_POLL_INTERVAL	5U /* usec */

#define FECS_ARB_CMD_TIMEOUT_MAX_US 40U
#define FECS_ARB_CMD_TIMEOUT_DEFAULT_US 2U
#define CTXSW_MEM_SCRUBBING_TIMEOUT_MAX_US 1000U
#define CTXSW_MEM_SCRUBBING_TIMEOUT_DEFAULT_US 10U

#define CTXSW_WDT_DEFAULT_VALUE 0x7FFFFFFFU
#define CTXSW_INTR0 BIT32(0)
#define CTXSW_INTR1 BIT32(1)

#ifdef CONFIG_NVGPU_SIM
void gm20b_gr_falcon_configure_fmodel(struct gk20a *g)
{
	nvgpu_log_fn(g, " ");

	nvgpu_writel(g, gr_fecs_ctxsw_mailbox_r(7),
		gr_fecs_ctxsw_mailbox_value_f(0xc0de7777U));
	nvgpu_writel(g, gr_gpccs_ctxsw_mailbox_r(7),
		gr_gpccs_ctxsw_mailbox_value_f(0xc0de7777U));

}
#endif

/* The following is a less brittle way to call gr_gk20a_submit_fecs_method(...)
 * We should replace most, if not all, fecs method calls to this instead.
 */

/* Sideband mailbox writes are done a bit differently */

void gm20b_gr_falcon_fecs_host_int_enable(struct gk20a *g)
{
	nvgpu_writel(g, gr_fecs_host_int_enable_r(),
		     gr_fecs_host_int_enable_ctxsw_intr1_enable_f() |
		     gr_fecs_host_int_enable_fault_during_ctxsw_enable_f() |
		     gr_fecs_host_int_enable_umimp_firmware_method_enable_f() |
		     gr_fecs_host_int_enable_umimp_illegal_method_enable_f() |
		     gr_fecs_host_int_enable_watchdog_enable_f());
}
