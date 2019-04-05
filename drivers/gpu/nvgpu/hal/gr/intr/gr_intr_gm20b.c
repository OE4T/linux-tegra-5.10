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

#include <nvgpu/gr/config.h>
#include <nvgpu/gr/gr.h>
#include <nvgpu/gr/gr_intr.h>

#include "gr_intr_gm20b.h"

#include <nvgpu/hw/gm20b/hw_gr_gm20b.h>

bool gm20b_gr_intr_handle_exceptions(struct gk20a *g, bool *is_gpc_exception)
{
	bool gpc_reset = false;
	u32 exception = nvgpu_readl(g, gr_exception_r());

	nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg,
				"exception %08x\n", exception);

	if ((exception & gr_exception_fe_m()) != 0U) {
		u32 fe = nvgpu_readl(g, gr_fe_hww_esr_r());
		u32 info = nvgpu_readl(g, gr_fe_hww_esr_info_r());

		nvgpu_report_gr_exception(g, 0,
				GPU_PGRAPH_FE_EXCEPTION,
				fe);
		nvgpu_err(g, "fe exception: esr 0x%08x, info 0x%08x",
				fe, info);
		nvgpu_writel(g, gr_fe_hww_esr_r(),
			gr_fe_hww_esr_reset_active_f());
		gpc_reset = true;
	}

	if ((exception & gr_exception_memfmt_m()) != 0U) {
		u32 memfmt = nvgpu_readl(g, gr_memfmt_hww_esr_r());

		nvgpu_report_gr_exception(g, 0,
				GPU_PGRAPH_MEMFMT_EXCEPTION,
				memfmt);
		nvgpu_err(g, "memfmt exception: esr %08x", memfmt);
		nvgpu_writel(g, gr_memfmt_hww_esr_r(),
				gr_memfmt_hww_esr_reset_active_f());
		gpc_reset = true;
	}

	if ((exception & gr_exception_pd_m()) != 0U) {
		u32 pd = nvgpu_readl(g, gr_pd_hww_esr_r());

		nvgpu_report_gr_exception(g, 0,
				GPU_PGRAPH_PD_EXCEPTION,
				pd);
		nvgpu_err(g, "pd exception: esr 0x%08x", pd);
		nvgpu_writel(g, gr_pd_hww_esr_r(),
				gr_pd_hww_esr_reset_active_f());
		gpc_reset = true;
	}

	if ((exception & gr_exception_scc_m()) != 0U) {
		u32 scc = nvgpu_readl(g, gr_scc_hww_esr_r());

		nvgpu_report_gr_exception(g, 0,
				GPU_PGRAPH_SCC_EXCEPTION,
				scc);
		nvgpu_err(g, "scc exception: esr 0x%08x", scc);
		nvgpu_writel(g, gr_scc_hww_esr_r(),
				gr_scc_hww_esr_reset_active_f());
		gpc_reset = true;
	}

	if ((exception & gr_exception_ds_m()) != 0U) {
		u32 ds = nvgpu_readl(g, gr_ds_hww_esr_r());

		nvgpu_report_gr_exception(g, 0,
				GPU_PGRAPH_DS_EXCEPTION,
				ds);
		nvgpu_err(g, "ds exception: esr: 0x%08x", ds);
		nvgpu_writel(g, gr_ds_hww_esr_r(),
				 gr_ds_hww_esr_reset_task_f());
		gpc_reset = true;
	}

	if ((exception & gr_exception_ssync_m()) != 0U) {
		u32 ssync_esr = 0;

		if (g->ops.gr.handle_ssync_hww != NULL) {
			if (g->ops.gr.handle_ssync_hww(g, &ssync_esr)
					!= 0) {
				gpc_reset = true;
			}
		} else {
			nvgpu_err(g, "unhandled ssync exception");
		}
		nvgpu_report_gr_exception(g, 0,
				GPU_PGRAPH_SSYNC_EXCEPTION,
				ssync_esr);
	}

	if ((exception & gr_exception_mme_m()) != 0U) {
		u32 mme = nvgpu_readl(g, gr_mme_hww_esr_r());
		u32 info = nvgpu_readl(g, gr_mme_hww_esr_info_r());

		nvgpu_report_gr_exception(g, 0,
				GPU_PGRAPH_MME_EXCEPTION,
				mme);
		nvgpu_err(g, "mme exception: esr 0x%08x info:0x%08x",
				mme, info);
		if (g->ops.gr.log_mme_exception != NULL) {
			g->ops.gr.log_mme_exception(g);
		}

		nvgpu_writel(g, gr_mme_hww_esr_r(),
			gr_mme_hww_esr_reset_active_f());
		gpc_reset = true;
	}

	if ((exception & gr_exception_sked_m()) != 0U) {
		u32 sked = nvgpu_readl(g, gr_sked_hww_esr_r());

		nvgpu_report_gr_exception(g, 0,
				GPU_PGRAPH_SKED_EXCEPTION,
				sked);
		nvgpu_err(g, "sked exception: esr 0x%08x", sked);
		nvgpu_writel(g, gr_sked_hww_esr_r(),
			gr_sked_hww_esr_reset_active_f());
		gpc_reset = true;
	}

	/* check if a gpc exception has occurred */
	if ((exception & gr_exception_gpc_m()) != 0U) {
		*is_gpc_exception = true;
	}

	return gpc_reset;
}

u32 gm20b_gr_intr_read_gpc_tpc_exception(u32 gpc_exception)
{
	return gr_gpc0_gpccs_gpc_exception_tpc_v(gpc_exception);
}

u32 gm20b_gr_intr_read_gpc_exception(struct gk20a *g, u32 gpc)
{
	u32 gpc_offset = nvgpu_gr_gpc_offset(g, gpc);

	return nvgpu_readl(g, gr_gpc0_gpccs_gpc_exception_r() + gpc_offset);
}

u32 gm20b_gr_intr_read_exception1(struct gk20a *g)
{
	return nvgpu_readl(g, gr_exception1_r());
}

void gm20b_gr_intr_get_trapped_method_info(struct gk20a *g,
				    struct nvgpu_gr_isr_data *isr_data)
{
	u32 obj_table;

	isr_data->addr = nvgpu_readl(g, gr_trapped_addr_r());
	isr_data->data_lo = nvgpu_readl(g, gr_trapped_data_lo_r());
	isr_data->data_hi = nvgpu_readl(g, gr_trapped_data_hi_r());
	isr_data->curr_ctx = nvgpu_readl(g, gr_fecs_current_ctx_r());
	isr_data->offset = gr_trapped_addr_mthd_v(isr_data->addr);
	isr_data->sub_chan = gr_trapped_addr_subch_v(isr_data->addr);
	obj_table = (isr_data->sub_chan < 4U) ? nvgpu_readl(g,
		gr_fe_object_table_r(isr_data->sub_chan)) : 0U;
	isr_data->class_num = gr_fe_object_table_nvclass_v(obj_table);
}

u32 gm20b_gr_intr_get_tpc_exception(struct gk20a *g, u32 offset,
				    struct nvgpu_gr_tpc_exception *pending_tpc)
{
	u32 tpc_exception = nvgpu_readl(g,
				gr_gpc0_tpc0_tpccs_tpc_exception_r()
				+ offset);

	(void) memset(pending_tpc, 0, sizeof(struct nvgpu_gr_tpc_exception));

	if (gr_gpc0_tpc0_tpccs_tpc_exception_tex_v(tpc_exception) ==
		gr_gpc0_tpc0_tpccs_tpc_exception_tex_pending_v()) {
			pending_tpc->tex_exception = true;
	}

	if (gr_gpc0_tpc0_tpccs_tpc_exception_sm_v(tpc_exception) ==
		gr_gpc0_tpc0_tpccs_tpc_exception_sm_pending_v()) {
			pending_tpc->sm_exception = true;
	}

	if ((tpc_exception & gr_gpc0_tpc0_tpccs_tpc_exception_mpc_m()) != 0U) {
		pending_tpc->mpc_exception = true;
	}

	return tpc_exception;
}

void gm20b_gr_intr_handle_tex_exception(struct gk20a *g, u32 gpc, u32 tpc)
{
	u32 offset = nvgpu_gr_gpc_offset(g, gpc) + nvgpu_gr_tpc_offset(g, tpc);
	u32 esr;

	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg, " ");

	esr = nvgpu_readl(g,
			  gr_gpc0_tpc0_tex_m_hww_esr_r() + offset);
	nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg, "0x%08x", esr);

	nvgpu_writel(g,
		     gr_gpc0_tpc0_tex_m_hww_esr_r() + offset,
		     esr);
}

void gm20b_gr_intr_enable_hww_exceptions(struct gk20a *g)
{
	/* enable exceptions */
	nvgpu_writel(g, gr_fe_hww_esr_r(),
		     gr_fe_hww_esr_en_enable_f() |
		     gr_fe_hww_esr_reset_active_f());
	nvgpu_writel(g, gr_memfmt_hww_esr_r(),
		     gr_memfmt_hww_esr_en_enable_f() |
		     gr_memfmt_hww_esr_reset_active_f());
}

void gm20b_gr_intr_enable_interrupts(struct gk20a *g, bool enable)
{
	if (enable) {
		nvgpu_writel(g, gr_intr_r(), 0xFFFFFFFFU);
		nvgpu_writel(g, gr_intr_en_r(), 0xFFFFFFFFU);
	} else {
		nvgpu_writel(g, gr_intr_r(), 0);
		nvgpu_writel(g, gr_intr_en_r(), 0);
	}

}
void gm20b_gr_intr_enable_exceptions(struct gk20a *g,
				     struct nvgpu_gr_config *gr_config,
				     bool enable)
{
	u32 reg_value = (enable) ? 0xFFFFFFFFU : 0U;

	nvgpu_writel(g, gr_exception_en_r(), reg_value);
	nvgpu_writel(g, gr_exception1_en_r(), reg_value);
	nvgpu_writel(g, gr_exception2_en_r(), reg_value);
}

void gm20b_gr_intr_enable_gpc_exceptions(struct gk20a *g,
					 struct nvgpu_gr_config *gr_config)
{
	u32 tpc_mask, tpc_mask_calc;

	nvgpu_writel(g, gr_gpcs_tpcs_tpccs_tpc_exception_en_r(),
			gr_gpcs_tpcs_tpccs_tpc_exception_en_tex_enabled_f() |
			gr_gpcs_tpcs_tpccs_tpc_exception_en_sm_enabled_f());

	tpc_mask_calc = (u32)BIT32(
			 nvgpu_gr_config_get_max_tpc_per_gpc_count(gr_config));
	tpc_mask = gr_gpcs_gpccs_gpc_exception_en_tpc_f(tpc_mask_calc - 1U);

	nvgpu_writel(g, gr_gpcs_gpccs_gpc_exception_en_r(), tpc_mask);
}

void gm20ab_gr_intr_tpc_exception_sm_disable(struct gk20a *g, u32 offset)
{
	u32 tpc_exception_en = nvgpu_readl(g,
				gr_gpc0_tpc0_tpccs_tpc_exception_en_r() +
				offset);

	tpc_exception_en &=
			~gr_gpc0_tpc0_tpccs_tpc_exception_en_sm_enabled_f();
	nvgpu_writel(g,
		     gr_gpc0_tpc0_tpccs_tpc_exception_en_r() + offset,
		     tpc_exception_en);
}

void gm20ab_gr_intr_tpc_exception_sm_enable(struct gk20a *g)
{
	u32 tpc_exception_en = nvgpu_readl(g,
				gr_gpc0_tpc0_tpccs_tpc_exception_en_r());

	tpc_exception_en &=
			~gr_gpc0_tpc0_tpccs_tpc_exception_en_sm_enabled_f();
	tpc_exception_en |= gr_gpc0_tpc0_tpccs_tpc_exception_en_sm_enabled_f();
	nvgpu_writel(g,
		     gr_gpcs_tpcs_tpccs_tpc_exception_en_r(),
		     tpc_exception_en);
}

u32 gm20b_gr_intr_nonstall_isr(struct gk20a *g)
{
	u32 ops = 0;
	u32 gr_intr = nvgpu_readl(g, gr_intr_nonstall_r());

	nvgpu_log(g, gpu_dbg_intr, "pgraph nonstall intr %08x", gr_intr);

	if ((gr_intr & gr_intr_nonstall_trap_pending_f()) != 0U) {
		/* Clear the interrupt */
		nvgpu_writel(g, gr_intr_nonstall_r(),
			gr_intr_nonstall_trap_pending_f());
		ops |= (GK20A_NONSTALL_OPS_WAKEUP_SEMAPHORE |
			GK20A_NONSTALL_OPS_POST_EVENTS);
	}
	return ops;
}
