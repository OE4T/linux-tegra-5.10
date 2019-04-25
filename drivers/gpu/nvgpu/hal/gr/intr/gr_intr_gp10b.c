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
#include <nvgpu/class.h>
#include <nvgpu/channel.h>

#include <nvgpu/gr/config.h>
#include <nvgpu/gr/gr.h>
#include <nvgpu/gr/ctx.h>
#include <nvgpu/gr/gr_falcon.h>
#include <nvgpu/gr/gr_intr.h>

#include "common/gr/gr_priv.h"

#include "gr_intr_gp10b.h"

#include <nvgpu/hw/gp10b/hw_gr_gp10b.h>

static int gp10b_gr_intr_clear_cilp_preempt_pending(struct gk20a *g,
					       struct channel_gk20a *fault_ch)
{
	struct tsg_gk20a *tsg;
	struct nvgpu_gr_ctx *gr_ctx;

	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg | gpu_dbg_intr, " ");

	tsg = tsg_gk20a_from_ch(fault_ch);
	if (tsg == NULL) {
		return -EINVAL;
	}

	gr_ctx = tsg->gr_ctx;

	/* The ucode is self-clearing, so all we need to do here is
	   to clear cilp_preempt_pending. */
	if (!nvgpu_gr_ctx_get_cilp_preempt_pending(gr_ctx)) {
		nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg | gpu_dbg_intr,
				"CILP is already cleared for chid %d\n",
				fault_ch->chid);
		return 0;
	}

	nvgpu_gr_ctx_set_cilp_preempt_pending(gr_ctx, false);
	g->gr->cilp_preempt_pending_chid = NVGPU_INVALID_CHANNEL_ID;

	return 0;
}

static int gp10b_gr_intr_get_cilp_preempt_pending_chid(struct gk20a *g,
					u32 *__chid)
{
	struct nvgpu_gr_ctx *gr_ctx;
	struct channel_gk20a *ch;
	struct tsg_gk20a *tsg;
	u32 chid;
	int ret = -EINVAL;

	chid = g->gr->cilp_preempt_pending_chid;
	if (chid == NVGPU_INVALID_CHANNEL_ID) {
		return ret;
	}

	ch = gk20a_channel_from_id(g, chid);
	if (ch == NULL) {
		return ret;
	}

	tsg = tsg_gk20a_from_ch(ch);
	if (tsg == NULL) {
		gk20a_channel_put(ch);
		return -EINVAL;
	}

	gr_ctx = tsg->gr_ctx;

	if (nvgpu_gr_ctx_get_cilp_preempt_pending(gr_ctx)) {
		*__chid = chid;
		ret = 0;
	}

	gk20a_channel_put(ch);

	return ret;
}

int gp10b_gr_intr_handle_fecs_error(struct gk20a *g,
				struct channel_gk20a *__ch,
				struct nvgpu_gr_isr_data *isr_data)
{
	struct channel_gk20a *ch;
	u32 chid = NVGPU_INVALID_CHANNEL_ID;
	int ret = 0;
	struct tsg_gk20a *tsg;
	struct nvgpu_fecs_host_intr_status fecs_host_intr;
	u32 gr_fecs_intr = g->ops.gr.falcon.fecs_host_intr_status(g,
						&fecs_host_intr);


	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg | gpu_dbg_intr, " ");

	if (gr_fecs_intr == 0U) {
		return 0;
	}

	/*
	 * INTR1 (bit 1 of the HOST_INT_STATUS_CTXSW_INTR)
	 * indicates that a CILP ctxsw save has finished
	 */
	if (fecs_host_intr.ctxsw_intr1 != 0U) {
		nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg | gpu_dbg_intr,
				"CILP: ctxsw save completed!\n");

		/* now clear the interrupt */
		g->ops.gr.falcon.fecs_host_clear_intr(g,
					fecs_host_intr.ctxsw_intr1);

		ret = gp10b_gr_intr_get_cilp_preempt_pending_chid(g, &chid);
		if ((ret != 0) || (chid == NVGPU_INVALID_CHANNEL_ID)) {
			goto clean_up;
		}

		ch = gk20a_channel_from_id(g, chid);
		if (ch == NULL) {
			goto clean_up;
		}


		/* set preempt_pending to false */
		ret = gp10b_gr_intr_clear_cilp_preempt_pending(g, ch);
		if (ret != 0) {
			nvgpu_err(g, "CILP: error while unsetting CILP preempt pending!");
			gk20a_channel_put(ch);
			goto clean_up;
		}

#ifdef NVGPU_DEBUGGER
		/* Post events to UMD */
		g->ops.debugger.post_events(ch);
#endif

		tsg = &g->fifo.tsg[ch->tsgid];

		g->ops.tsg.post_event_id(tsg,
				NVGPU_EVENT_ID_CILP_PREEMPTION_COMPLETE);

		gk20a_channel_put(ch);
	}

clean_up:
	/* handle any remaining interrupts */
	return nvgpu_gr_intr_handle_fecs_error(g, __ch, isr_data);
}

void gp10b_gr_intr_set_go_idle_timeout(struct gk20a *g, u32 data)
{
	nvgpu_writel(g, gr_fe_go_idle_timeout_r(), data);
}

void gp10b_gr_intr_set_coalesce_buffer_size(struct gk20a *g, u32 data)
{
	u32 val;

	nvgpu_log_fn(g, " ");

	val = nvgpu_readl(g, gr_gpcs_tc_debug0_r());
	val = set_field(val, gr_gpcs_tc_debug0_limit_coalesce_buffer_size_m(),
			gr_gpcs_tc_debug0_limit_coalesce_buffer_size_f(data));
	nvgpu_writel(g, gr_gpcs_tc_debug0_r(), val);

	nvgpu_log_fn(g, "done");
}

int gp10b_gr_intr_handle_sw_method(struct gk20a *g, u32 addr,
				     u32 class_num, u32 offset, u32 data)
{
	nvgpu_log_fn(g, " ");

	if (class_num == PASCAL_COMPUTE_A) {
		switch (offset << 2) {
		case NVC0C0_SET_SHADER_EXCEPTIONS:
			g->ops.gr.intr.set_shader_exceptions(g, data);
			break;
		case NVC0C0_SET_RD_COALESCE:
			g->ops.gr.init.lg_coalesce(g, data);
			break;
		default:
			goto fail;
		}
	}

	if (class_num == PASCAL_A) {
		switch (offset << 2) {
		case NVC097_SET_SHADER_EXCEPTIONS:
			g->ops.gr.intr.set_shader_exceptions(g, data);
			break;
		case NVC097_SET_CIRCULAR_BUFFER_SIZE:
			g->ops.gr.set_circular_buffer_size(g, data);
			break;
		case NVC097_SET_ALPHA_CIRCULAR_BUFFER_SIZE:
			g->ops.gr.set_alpha_circular_buffer_size(g, data);
			break;
		case NVC097_SET_GO_IDLE_TIMEOUT:
			gp10b_gr_intr_set_go_idle_timeout(g, data);
			break;
		case NVC097_SET_COALESCE_BUFFER_SIZE:
			gp10b_gr_intr_set_coalesce_buffer_size(g, data);
			break;
		case NVC097_SET_RD_COALESCE:
			g->ops.gr.init.lg_coalesce(g, data);
			break;
		case NVC097_SET_BES_CROP_DEBUG3:
			g->ops.gr.set_bes_crop_debug3(g, data);
			break;
		case NVC097_SET_BES_CROP_DEBUG4:
			g->ops.gr.set_bes_crop_debug4(g, data);
			break;
		default:
			goto fail;
		}
	}
	return 0;

fail:
	return -EINVAL;
}

static void gr_gp10b_sm_lrf_ecc_overcount_war(bool single_err,
						u32 sed_status,
						u32 ded_status,
						u32 *count_to_adjust,
						u32 opposite_count)
{
	u32 over_count = 0;

	sed_status >>= gr_pri_gpc0_tpc0_sm_lrf_ecc_status_single_err_detected_qrfdp0_b();
	ded_status >>= gr_pri_gpc0_tpc0_sm_lrf_ecc_status_double_err_detected_qrfdp0_b();

	/* One overcount for each partition on which a SBE occurred but not a
	   DBE (or vice-versa) */
	if (single_err) {
		over_count = (u32)hweight32(sed_status & ~ded_status);
	} else {
		over_count = (u32)hweight32(ded_status & ~sed_status);
	}

	/* If both a SBE and a DBE occur on the same partition, then we have an
	   overcount for the subpartition if the opposite error counts are
	   zero. */
	if (((sed_status & ded_status) != 0U) && (opposite_count == 0U)) {
		over_count += (u32)hweight32(sed_status & ded_status);
	}

	if (*count_to_adjust > over_count) {
		*count_to_adjust -= over_count;
	} else {
		*count_to_adjust = 0;
	}
}

int gp10b_gr_intr_handle_sm_exception(struct gk20a *g,
			u32 gpc, u32 tpc, u32 sm,
			bool *post_event, struct channel_gk20a *fault_ch,
			u32 *hww_global_esr)
{
	int ret = 0;
	u32 offset = nvgpu_gr_gpc_offset(g, gpc) + nvgpu_gr_tpc_offset(g, tpc);
	u32 lrf_ecc_status, lrf_ecc_sed_status, lrf_ecc_ded_status;
	u32 lrf_single_count_delta, lrf_double_count_delta;
	u32 shm_ecc_status;

	ret = nvgpu_gr_intr_handle_sm_exception(g,
		gpc, tpc, sm, post_event, fault_ch, hww_global_esr);

	/* Check for LRF ECC errors. */
        lrf_ecc_status = nvgpu_readl(g,
			gr_pri_gpc0_tpc0_sm_lrf_ecc_status_r() + offset);
	lrf_ecc_sed_status =
		lrf_ecc_status &
		(gr_pri_gpc0_tpc0_sm_lrf_ecc_status_single_err_detected_qrfdp0_pending_f() |
		 gr_pri_gpc0_tpc0_sm_lrf_ecc_status_single_err_detected_qrfdp1_pending_f() |
		 gr_pri_gpc0_tpc0_sm_lrf_ecc_status_single_err_detected_qrfdp2_pending_f() |
		 gr_pri_gpc0_tpc0_sm_lrf_ecc_status_single_err_detected_qrfdp3_pending_f());
	lrf_ecc_ded_status =
		lrf_ecc_status &
		(gr_pri_gpc0_tpc0_sm_lrf_ecc_status_double_err_detected_qrfdp0_pending_f() |
		 gr_pri_gpc0_tpc0_sm_lrf_ecc_status_double_err_detected_qrfdp1_pending_f() |
		 gr_pri_gpc0_tpc0_sm_lrf_ecc_status_double_err_detected_qrfdp2_pending_f() |
		 gr_pri_gpc0_tpc0_sm_lrf_ecc_status_double_err_detected_qrfdp3_pending_f());
	lrf_single_count_delta =
		nvgpu_readl(g,
			gr_pri_gpc0_tpc0_sm_lrf_ecc_single_err_count_r() +
			offset);
	lrf_double_count_delta =
		nvgpu_readl(g,
			gr_pri_gpc0_tpc0_sm_lrf_ecc_double_err_count_r() +
			offset);
	nvgpu_writel(g,
		gr_pri_gpc0_tpc0_sm_lrf_ecc_single_err_count_r() + offset, 0);
	nvgpu_writel(g,
		gr_pri_gpc0_tpc0_sm_lrf_ecc_double_err_count_r() + offset, 0);
	if (lrf_ecc_sed_status != 0U) {
		nvgpu_log(g, gpu_dbg_fn | gpu_dbg_intr,
			"Single bit error detected in SM LRF!");

		gr_gp10b_sm_lrf_ecc_overcount_war(true,
						lrf_ecc_sed_status,
						lrf_ecc_ded_status,
						&lrf_single_count_delta,
						lrf_double_count_delta);
		g->ecc.gr.sm_lrf_ecc_single_err_count[gpc][tpc].counter +=
							lrf_single_count_delta;
	}
	if (lrf_ecc_ded_status != 0U) {
		nvgpu_log(g, gpu_dbg_fn | gpu_dbg_intr,
			"Double bit error detected in SM LRF!");

		gr_gp10b_sm_lrf_ecc_overcount_war(false,
						lrf_ecc_sed_status,
						lrf_ecc_ded_status,
						&lrf_double_count_delta,
						lrf_single_count_delta);
		g->ecc.gr.sm_lrf_ecc_double_err_count[gpc][tpc].counter +=
							lrf_double_count_delta;
	}
	nvgpu_writel(g, gr_pri_gpc0_tpc0_sm_lrf_ecc_status_r() + offset,
			lrf_ecc_status);

	/* Check for SHM ECC errors. */
        shm_ecc_status = nvgpu_readl(g,
			gr_pri_gpc0_tpc0_sm_shm_ecc_status_r() + offset);
	if ((shm_ecc_status &
		gr_pri_gpc0_tpc0_sm_shm_ecc_status_single_err_corrected_shm0_pending_f()) != 0U ||
		(shm_ecc_status &
		gr_pri_gpc0_tpc0_sm_shm_ecc_status_single_err_corrected_shm1_pending_f()) != 0U ||
		(shm_ecc_status &
		gr_pri_gpc0_tpc0_sm_shm_ecc_status_single_err_detected_shm0_pending_f()) != 0U ||
		(shm_ecc_status &
		gr_pri_gpc0_tpc0_sm_shm_ecc_status_single_err_detected_shm1_pending_f()) != 0U ) {
		u32 ecc_stats_reg_val;

		nvgpu_log(g, gpu_dbg_fn | gpu_dbg_intr,
			"Single bit error detected in SM SHM!");

		ecc_stats_reg_val =
			nvgpu_readl(g,
				gr_pri_gpc0_tpc0_sm_shm_ecc_err_count_r() + offset);
		g->ecc.gr.sm_shm_ecc_sec_count[gpc][tpc].counter +=
			gr_pri_gpc0_tpc0_sm_shm_ecc_err_count_single_corrected_v(ecc_stats_reg_val);
		g->ecc.gr.sm_shm_ecc_sed_count[gpc][tpc].counter +=
			gr_pri_gpc0_tpc0_sm_shm_ecc_err_count_single_detected_v(ecc_stats_reg_val);
		ecc_stats_reg_val &= ~(gr_pri_gpc0_tpc0_sm_shm_ecc_err_count_single_corrected_m() |
					gr_pri_gpc0_tpc0_sm_shm_ecc_err_count_single_detected_m());
		nvgpu_writel(g,
			gr_pri_gpc0_tpc0_sm_shm_ecc_err_count_r() + offset,
			ecc_stats_reg_val);
	}
	if ((shm_ecc_status &
		gr_pri_gpc0_tpc0_sm_shm_ecc_status_double_err_detected_shm0_pending_f()) != 0U ||
		(shm_ecc_status &
		gr_pri_gpc0_tpc0_sm_shm_ecc_status_double_err_detected_shm1_pending_f()) != 0U) {
		u32 ecc_stats_reg_val;

		nvgpu_log(g, gpu_dbg_fn | gpu_dbg_intr,
			"Double bit error detected in SM SHM!");

		ecc_stats_reg_val =
			nvgpu_readl(g,
				gr_pri_gpc0_tpc0_sm_shm_ecc_err_count_r() + offset);
		g->ecc.gr.sm_shm_ecc_ded_count[gpc][tpc].counter +=
			gr_pri_gpc0_tpc0_sm_shm_ecc_err_count_double_detected_v(ecc_stats_reg_val);
		ecc_stats_reg_val &= ~(gr_pri_gpc0_tpc0_sm_shm_ecc_err_count_double_detected_m());
		nvgpu_writel(g,
			gr_pri_gpc0_tpc0_sm_shm_ecc_err_count_r() + offset,
			ecc_stats_reg_val);
	}
	nvgpu_writel(g, gr_pri_gpc0_tpc0_sm_shm_ecc_status_r() + offset,
			shm_ecc_status);


	return ret;
}

void gp10b_gr_intr_handle_tex_exception(struct gk20a *g, u32 gpc, u32 tpc)
{
	u32 offset = nvgpu_gr_gpc_offset(g, gpc) + nvgpu_gr_tpc_offset(g, tpc);
	u32 esr;
	u32 ecc_stats_reg_val;

	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg, " ");

	esr = nvgpu_readl(g,
			 gr_gpc0_tpc0_tex_m_hww_esr_r() + offset);
	nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg, "0x%08x", esr);

	if ((esr & gr_gpc0_tpc0_tex_m_hww_esr_ecc_sec_pending_f()) != 0U) {
		nvgpu_log(g, gpu_dbg_fn | gpu_dbg_intr,
			"Single bit error detected in TEX!");

		/* Pipe 0 counters */
		nvgpu_writel(g,
			gr_pri_gpc0_tpc0_tex_m_routing_r() + offset,
			gr_pri_gpc0_tpc0_tex_m_routing_sel_pipe0_f());

		ecc_stats_reg_val = nvgpu_readl(g,
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_total_r() + offset);
		g->ecc.gr.tex_ecc_total_sec_pipe0_count[gpc][tpc].counter +=
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_total_sec_v(
							ecc_stats_reg_val);
		ecc_stats_reg_val &=
			~gr_pri_gpc0_tpc0_tex_m_ecc_cnt_total_sec_m();
		nvgpu_writel(g,
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_total_r() + offset,
			ecc_stats_reg_val);

		ecc_stats_reg_val = nvgpu_readl(g,
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_unique_r() + offset);
		g->ecc.gr.tex_unique_ecc_sec_pipe0_count[gpc][tpc].counter +=
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_unique_sec_v(
							ecc_stats_reg_val);
		ecc_stats_reg_val &=
			~gr_pri_gpc0_tpc0_tex_m_ecc_cnt_unique_sec_m();
		nvgpu_writel(g,
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_unique_r() + offset,
			ecc_stats_reg_val);


		/* Pipe 1 counters */
		nvgpu_writel(g,
			gr_pri_gpc0_tpc0_tex_m_routing_r() + offset,
			gr_pri_gpc0_tpc0_tex_m_routing_sel_pipe1_f());

		ecc_stats_reg_val = nvgpu_readl(g,
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_total_r() + offset);
		g->ecc.gr.tex_ecc_total_sec_pipe1_count[gpc][tpc].counter +=
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_total_sec_v(
							ecc_stats_reg_val);
		ecc_stats_reg_val &=
			~gr_pri_gpc0_tpc0_tex_m_ecc_cnt_total_sec_m();
		nvgpu_writel(g,
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_total_r() + offset,
			ecc_stats_reg_val);

		ecc_stats_reg_val = nvgpu_readl(g,
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_unique_r() + offset);
		g->ecc.gr.tex_unique_ecc_sec_pipe1_count[gpc][tpc].counter +=
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_unique_sec_v(
							ecc_stats_reg_val);
		ecc_stats_reg_val &=
			~gr_pri_gpc0_tpc0_tex_m_ecc_cnt_unique_sec_m();
		nvgpu_writel(g,
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_unique_r() + offset,
			ecc_stats_reg_val);


		nvgpu_writel(g,
			gr_pri_gpc0_tpc0_tex_m_routing_r() + offset,
			gr_pri_gpc0_tpc0_tex_m_routing_sel_default_f());
	}

	if ((esr & gr_gpc0_tpc0_tex_m_hww_esr_ecc_ded_pending_f()) != 0U) {
		nvgpu_log(g, gpu_dbg_fn | gpu_dbg_intr,
			"Double bit error detected in TEX!");

		/* Pipe 0 counters */
		nvgpu_writel(g,
			gr_pri_gpc0_tpc0_tex_m_routing_r() + offset,
			gr_pri_gpc0_tpc0_tex_m_routing_sel_pipe0_f());

		ecc_stats_reg_val = nvgpu_readl(g,
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_total_r() + offset);
		g->ecc.gr.tex_ecc_total_ded_pipe0_count[gpc][tpc].counter +=
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_total_ded_v(
							ecc_stats_reg_val);
		ecc_stats_reg_val &=
			~gr_pri_gpc0_tpc0_tex_m_ecc_cnt_total_ded_m();
		nvgpu_writel(g,
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_total_r() + offset,
			ecc_stats_reg_val);

		ecc_stats_reg_val = nvgpu_readl(g,
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_unique_r() + offset);
		g->ecc.gr.tex_unique_ecc_ded_pipe0_count[gpc][tpc].counter +=
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_unique_ded_v(
							ecc_stats_reg_val);
		ecc_stats_reg_val &=
			~gr_pri_gpc0_tpc0_tex_m_ecc_cnt_unique_ded_m();
		nvgpu_writel(g,
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_unique_r() + offset,
			ecc_stats_reg_val);


		/* Pipe 1 counters */
		nvgpu_writel(g,
			gr_pri_gpc0_tpc0_tex_m_routing_r() + offset,
			gr_pri_gpc0_tpc0_tex_m_routing_sel_pipe1_f());

		ecc_stats_reg_val = nvgpu_readl(g,
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_total_r() + offset);
		g->ecc.gr.tex_ecc_total_ded_pipe1_count[gpc][tpc].counter +=
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_total_ded_v(
							ecc_stats_reg_val);
		ecc_stats_reg_val &=
			~gr_pri_gpc0_tpc0_tex_m_ecc_cnt_total_ded_m();
		nvgpu_writel(g,
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_total_r() + offset,
			ecc_stats_reg_val);

		ecc_stats_reg_val = nvgpu_readl(g,
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_unique_r() + offset);
		g->ecc.gr.tex_unique_ecc_ded_pipe1_count[gpc][tpc].counter +=
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_unique_ded_v(
							ecc_stats_reg_val);
		ecc_stats_reg_val &=
			~gr_pri_gpc0_tpc0_tex_m_ecc_cnt_unique_ded_m();
		nvgpu_writel(g,
			gr_pri_gpc0_tpc0_tex_m_ecc_cnt_unique_r() + offset,
			ecc_stats_reg_val);


		nvgpu_writel(g,
			gr_pri_gpc0_tpc0_tex_m_routing_r() + offset,
			gr_pri_gpc0_tpc0_tex_m_routing_sel_default_f());
	}

	nvgpu_writel(g,
		     gr_gpc0_tpc0_tex_m_hww_esr_r() + offset,
		     esr | gr_gpc0_tpc0_tex_m_hww_esr_reset_active_f());

}
