/*
 * Copyright (c) 2015-2019, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/soc.h>
#include <nvgpu/ptimer.h>
#include <nvgpu/channel.h>
#include <nvgpu/tsg.h>
#include <nvgpu/nvgpu_err.h>
#include <nvgpu/error_notifier.h>

#include <hal/fifo/fifo_intr_gk20a.h>
#include <hal/fifo/fifo_intr_gv11b.h>

#include <nvgpu/hw/gv11b/hw_fifo_gv11b.h>
#include <nvgpu/hw/gv11b/hw_pbdma_gv11b.h> /* TODO: remove */

static u32 gv11b_fifo_intr_0_error_mask(struct gk20a *g)
{
	u32 intr_0_error_mask =
		fifo_intr_0_bind_error_pending_f() |
		fifo_intr_0_sched_error_pending_f() |
		fifo_intr_0_chsw_error_pending_f() |
		fifo_intr_0_memop_timeout_pending_f() |
		fifo_intr_0_lb_error_pending_f();

	return intr_0_error_mask;
}

static u32 gv11b_fifo_intr_0_en_mask(struct gk20a *g)
{
	u32 intr_0_en_mask;

	intr_0_en_mask = gv11b_fifo_intr_0_error_mask(g);

	intr_0_en_mask |= fifo_intr_0_pbdma_intr_pending_f() |
				 fifo_intr_0_ctxsw_timeout_pending_f();

	return intr_0_en_mask;
}

void gv11b_fifo_intr_0_enable(struct gk20a *g, bool enable)
{
	unsigned int i;
	u32 intr_stall, timeout, mask;
	u32 host_num_pbdma = nvgpu_get_litter_value(g, GPU_LIT_HOST_NUM_PBDMA);

	if (!enable) {
		nvgpu_writel(g, fifo_intr_en_0_r(), 0);
		return;
	}
	/* clear and enable pbdma interrupt */
	for (i = 0; i < host_num_pbdma; i++) {
		nvgpu_writel(g, pbdma_intr_0_r(i), U32_MAX);
		nvgpu_writel(g, pbdma_intr_1_r(i), U32_MAX);

		intr_stall = nvgpu_readl(g, pbdma_intr_stall_r(i));
		nvgpu_log_info(g, "pbdma id:%u, intr_en_0 0x%08x", i,
				intr_stall);
		nvgpu_writel(g, pbdma_intr_en_0_r(i), intr_stall);

		intr_stall = nvgpu_readl(g, pbdma_intr_stall_1_r(i));
		/*
		 * For bug 2082123
		 * Mask the unused HCE_RE_ILLEGAL_OP bit from the interrupt.
		 */
		intr_stall &= ~pbdma_intr_stall_1_hce_illegal_op_enabled_f();
		nvgpu_log_info(g, "pbdma id:%u, intr_en_1 0x%08x", i,
				intr_stall);
		nvgpu_writel(g, pbdma_intr_en_1_r(i), intr_stall);
	}

	/* clear ctxsw timeout interrupts */
	nvgpu_writel(g, fifo_intr_ctxsw_timeout_r(), ~U32(0U));

	if (nvgpu_platform_is_silicon(g)) {
		/* timeout is in us. Enable ctxsw timeout */
		timeout = g->ctxsw_timeout_period_ms * 1000U;
		timeout = scale_ptimer(timeout,
			ptimer_scalingfactor10x(g->ptimer_src_freq));
		timeout |= fifo_eng_ctxsw_timeout_detection_enabled_f();
		nvgpu_writel(g, fifo_eng_ctxsw_timeout_r(), timeout);
	} else {
		timeout = nvgpu_readl(g, fifo_eng_ctxsw_timeout_r());
		nvgpu_log_info(g,
			"fifo_eng_ctxsw_timeout reg val = 0x%08x",
			 timeout);
		timeout = set_field(timeout,
				fifo_eng_ctxsw_timeout_period_m(),
				fifo_eng_ctxsw_timeout_period_max_f());
		timeout = set_field(timeout,
				fifo_eng_ctxsw_timeout_detection_m(),
				fifo_eng_ctxsw_timeout_detection_disabled_f());
		nvgpu_log_info(g,
			"new fifo_eng_ctxsw_timeout reg val = 0x%08x",
			 timeout);
		nvgpu_writel(g, fifo_eng_ctxsw_timeout_r(), timeout);
	}

	/* clear runlist interrupts */
	nvgpu_writel(g, fifo_intr_runlist_r(), ~U32(0U));

	/* clear and enable pfifo interrupt */
	nvgpu_writel(g, fifo_intr_0_r(), U32_MAX);
	mask = gv11b_fifo_intr_0_en_mask(g);
	nvgpu_log_info(g, "fifo_intr_en_0 0x%08x", mask);
	nvgpu_writel(g, fifo_intr_en_0_r(), mask);
}

static const char *const gv11b_sched_error_str[] = {
	"xxx-0",
	"xxx-1",
	"xxx-2",
	"xxx-3",
	"xxx-4",
	"engine_reset",
	"rl_ack_timeout",
	"rl_ack_extra",
	"rl_rdat_timeout",
	"rl_rdat_extra",
	"eng_ctxsw_timeout",
	"xxx-b",
	"rl_req_timeout",
	"new_runlist",
	"code_config_while_busy",
	"xxx-f",
	"xxx-0x10",
	"xxx-0x11",
	"xxx-0x12",
	"xxx-0x13",
	"xxx-0x14",
	"xxx-0x15",
	"xxx-0x16",
	"xxx-0x17",
	"xxx-0x18",
	"xxx-0x19",
	"xxx-0x1a",
	"xxx-0x1b",
	"xxx-0x1c",
	"xxx-0x1d",
	"xxx-0x1e",
	"xxx-0x1f",
	"bad_tsg",
};

bool gv11b_fifo_handle_sched_error(struct gk20a *g)
{
	u32 sched_error;

	sched_error = nvgpu_readl(g, fifo_intr_sched_error_r());

	if (sched_error < ARRAY_SIZE(gv11b_sched_error_str)) {
		nvgpu_err(g, "fifo sched error :%s",
			gv11b_sched_error_str[sched_error]);
	} else {
		nvgpu_err(g, "fifo sched error code not supported");
	}

	nvgpu_report_host_error(g, 0,
			GPU_HOST_PFIFO_SCHED_ERROR, sched_error);

	if (sched_error == SCHED_ERROR_CODE_BAD_TSG) {
		/* id is unknown, preempt all runlists and do recovery */
		gk20a_fifo_recover(g, 0, 0, false, false, false,
				RC_TYPE_SCHED_ERR);
	}

	return false;
}

static const char * const invalid_str = "invalid";

static const char *const ctxsw_timeout_status_desc[] = {
	"awaiting ack",
	"eng was reset",
	"ack received",
	"dropped timeout"
};

static u32 gv11b_fifo_ctxsw_timeout_info(struct gk20a *g, u32 active_eng_id,
						u32 *info_status)
{
	u32 tsgid = FIFO_INVAL_TSG_ID;
	u32 timeout_info;
	u32 ctx_status;

	timeout_info = nvgpu_readl(g,
			 fifo_intr_ctxsw_timeout_info_r(active_eng_id));

	/*
	 * ctxsw_state and tsgid are snapped at the point of the timeout and
	 * will not change while the corresponding INTR_CTXSW_TIMEOUT_ENGINE bit
	 * is PENDING.
	 */
	ctx_status = fifo_intr_ctxsw_timeout_info_ctxsw_state_v(timeout_info);
	if (ctx_status ==
		fifo_intr_ctxsw_timeout_info_ctxsw_state_load_v()) {

		tsgid = fifo_intr_ctxsw_timeout_info_next_tsgid_v(timeout_info);

	} else if (ctx_status ==
		       fifo_intr_ctxsw_timeout_info_ctxsw_state_switch_v() ||
			ctx_status ==
			fifo_intr_ctxsw_timeout_info_ctxsw_state_save_v()) {

		tsgid = fifo_intr_ctxsw_timeout_info_prev_tsgid_v(timeout_info);
	}
	nvgpu_log_info(g, "ctxsw timeout info: tsgid = %d", tsgid);

	/*
	 * STATUS indicates whether the context request ack was eventually
	 * received and whether a subsequent request timed out.  This field is
	 * updated live while the corresponding INTR_CTXSW_TIMEOUT_ENGINE bit
	 * is PENDING. STATUS starts in AWAITING_ACK, and progresses to
	 * ACK_RECEIVED and finally ends with DROPPED_TIMEOUT.
	 *
	 * AWAITING_ACK - context request ack still not returned from engine.
	 * ENG_WAS_RESET - The engine was reset via a PRI write to NV_PMC_ENABLE
	 * or NV_PMC_ELPG_ENABLE prior to receiving the ack.  Host will not
	 * expect ctx ack to return, but if it is already in flight, STATUS will
	 * transition shortly to ACK_RECEIVED unless the interrupt is cleared
	 * first.  Once the engine is reset, additional context switches can
	 * occur; if one times out, STATUS will transition to DROPPED_TIMEOUT
	 * if the interrupt isn't cleared first.
	 * ACK_RECEIVED - The ack for the timed-out context request was
	 * received between the point of the timeout and this register being
	 * read.  Note this STATUS can be reported during the load stage of the
	 * same context switch that timed out if the timeout occurred during the
	 * save half of a context switch.  Additional context requests may have
	 * completed or may be outstanding, but no further context timeout has
	 * occurred.  This simplifies checking for spurious context switch
	 * timeouts.
	 * DROPPED_TIMEOUT - The originally timed-out context request acked,
	 * but a subsequent context request then timed out.
	 * Information about the subsequent timeout is not stored; in fact, that
	 * context request may also have already been acked by the time SW
	 * SW reads this register.  If not, there is a chance SW can get the
	 * dropped information by clearing the corresponding
	 * INTR_CTXSW_TIMEOUT_ENGINE bit and waiting for the timeout to occur
	 * again. Note, however, that if the engine does time out again,
	 * it may not be from the  original request that caused the
	 * DROPPED_TIMEOUT state, as that request may
	 * be acked in the interim.
	 */
	*info_status = fifo_intr_ctxsw_timeout_info_status_v(timeout_info);
	if (*info_status ==
		 fifo_intr_ctxsw_timeout_info_status_ack_received_v()) {

		nvgpu_log_info(g, "ctxsw timeout info : ack received");
		/* no need to recover */
		tsgid = FIFO_INVAL_TSG_ID;

	} else if (*info_status ==
		fifo_intr_ctxsw_timeout_info_status_dropped_timeout_v()) {

		nvgpu_log_info(g, "ctxsw timeout info : dropped timeout");
		/* no need to recover */
		tsgid = FIFO_INVAL_TSG_ID;

	}
	return tsgid;
}

bool gv11b_fifo_handle_ctxsw_timeout(struct gk20a *g, u32 fifo_intr)
{
	bool ret = false;
	u32 tsgid = FIFO_INVAL_TSG_ID;
	u32 engine_id, active_eng_id;
	u32 timeout_val, ctxsw_timeout_engines;
	u32 info_status;
	const char *info_status_str;


	if ((fifo_intr & fifo_intr_0_ctxsw_timeout_pending_f()) == 0U) {
		return ret;
	}

	/* get ctxsw timedout engines */
	ctxsw_timeout_engines = nvgpu_readl(g, fifo_intr_ctxsw_timeout_r());
	if (ctxsw_timeout_engines == 0U) {
		nvgpu_err(g, "no eng ctxsw timeout pending");
		return ret;
	}

	timeout_val = nvgpu_readl(g, fifo_eng_ctxsw_timeout_r());
	timeout_val = fifo_eng_ctxsw_timeout_period_v(timeout_val);

	nvgpu_log_info(g, "eng ctxsw timeout period = 0x%x", timeout_val);

	for (engine_id = 0; engine_id < g->fifo.num_engines; engine_id++) {
		active_eng_id = g->fifo.active_engines_list[engine_id];

		if ((ctxsw_timeout_engines &
			fifo_intr_ctxsw_timeout_engine_pending_f(
				active_eng_id)) != 0U) {

			struct fifo_gk20a *f = &g->fifo;
			u32 ms = 0;
			bool verbose = false;

			tsgid = gv11b_fifo_ctxsw_timeout_info(g, active_eng_id,
						&info_status);

			if (tsgid == FIFO_INVAL_TSG_ID) {
				continue;
			}

			if (g->ops.tsg.check_ctxsw_timeout(
				&f->tsg[tsgid], &verbose, &ms)) {
				ret = true;

				info_status_str =  invalid_str;
				if (info_status <
					ARRAY_SIZE(ctxsw_timeout_status_desc)) {
					info_status_str =
					ctxsw_timeout_status_desc[info_status];
				}

				nvgpu_err(g, "ctxsw timeout error: "
				"active engine id =%u, %s=%d, info: %s ms=%u",
				active_eng_id, "tsg", tsgid, info_status_str,
				ms);

				/* Cancel all channels' timeout */
				nvgpu_channel_wdt_restart_all_channels(g);
				gk20a_fifo_recover(g, BIT32(active_eng_id),
						tsgid, true, true, verbose,
						RC_TYPE_CTXSW_TIMEOUT);
			} else {
				nvgpu_log_info(g,
					"fifo is waiting for ctx switch: "
					"for %d ms, %s=%d", ms, "tsg", tsgid);
			}
		}
	}
	/* clear interrupt */
	nvgpu_writel(g, fifo_intr_ctxsw_timeout_r(), ctxsw_timeout_engines);
	return ret;
}

static u32 gv11b_fifo_intr_handle_errors(struct gk20a *g, u32 fifo_intr)
{
	u32 handled = 0U;

	nvgpu_log_fn(g, "fifo_intr=0x%08x", fifo_intr);

	if ((fifo_intr & fifo_intr_0_bind_error_pending_f()) != 0U) {
		u32 bind_error = nvgpu_readl(g, fifo_intr_bind_error_r());

		nvgpu_report_host_error(g, 0,
				GPU_HOST_PFIFO_BIND_ERROR, bind_error);
		nvgpu_err(g, "fifo bind error: 0x%08x", bind_error);
		handled |= fifo_intr_0_bind_error_pending_f();
	}

	if ((fifo_intr & fifo_intr_0_chsw_error_pending_f()) != 0U) {
		gk20a_fifo_intr_handle_chsw_error(g);
		handled |= fifo_intr_0_chsw_error_pending_f();
	}

	if ((fifo_intr & fifo_intr_0_memop_timeout_pending_f()) != 0U) {
		nvgpu_err(g, "fifo memop timeout error");
		handled |= fifo_intr_0_memop_timeout_pending_f();
	}

	if ((fifo_intr & fifo_intr_0_lb_error_pending_f()) != 0U) {
		nvgpu_err(g, "fifo lb error");
		handled |= fifo_intr_0_lb_error_pending_f();
	}

	return handled;
}

void gv11b_fifo_intr_0_isr(struct gk20a *g)
{
	u32 clear_intr = 0U;
	u32 fifo_intr = nvgpu_readl(g, fifo_intr_0_r());

	/* TODO: sw_ready is needed only for recovery part */
	if (!g->fifo.sw_ready) {
		nvgpu_err(g, "unhandled fifo intr: 0x%08x", fifo_intr);
		nvgpu_writel(g, fifo_intr_0_r(), fifo_intr);
		return;
	}
	/* note we're not actually in an "isr", but rather
	 * in a threaded interrupt context... */
	nvgpu_mutex_acquire(&g->fifo.intr.isr.mutex);

	nvgpu_log(g, gpu_dbg_intr, "fifo isr %08x", fifo_intr);

	if (unlikely((fifo_intr & gv11b_fifo_intr_0_error_mask(g)) !=
							0U)) {
		clear_intr |= gv11b_fifo_intr_handle_errors(g,
				fifo_intr);
	}

	if ((fifo_intr & fifo_intr_0_runlist_event_pending_f()) != 0U) {
		gk20a_fifo_intr_handle_runlist_event(g);
		clear_intr |= fifo_intr_0_runlist_event_pending_f();
	}

	if ((fifo_intr & fifo_intr_0_pbdma_intr_pending_f()) != 0U) {
		clear_intr |= fifo_pbdma_isr(g, fifo_intr);
	}

	if ((fifo_intr & fifo_intr_0_sched_error_pending_f()) != 0U) {
		(void) g->ops.fifo.handle_sched_error(g);
		clear_intr |= fifo_intr_0_sched_error_pending_f();
	}

	if ((fifo_intr & fifo_intr_0_ctxsw_timeout_pending_f()) != 0U) {
		if (g->ops.fifo.handle_ctxsw_timeout != NULL) {
			g->ops.fifo.handle_ctxsw_timeout(g, fifo_intr);
		} else {
			nvgpu_err(g, "unhandled fifo ctxsw timeout intr");
		}
	}

	nvgpu_mutex_release(&g->fifo.intr.isr.mutex);

	nvgpu_writel(g, fifo_intr_0_r(), clear_intr);
}
