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
#include <nvgpu/channel.h>
#include <nvgpu/rc.h>
#include <nvgpu/error_notifier.h>
#include <nvgpu/power_features/pg.h>
#if defined(CONFIG_GK20A_CYCLE_STATS)
#include <nvgpu/cyclestats.h>
#endif

#include <nvgpu/gr/gr.h>
#include <nvgpu/gr/gr_intr.h>
#include <nvgpu/gr/config.h>
#include <nvgpu/gr/gr_falcon.h>
#include <nvgpu/gr/fecs_trace.h>

#include "gr_priv.h"
#include "gr_intr_priv.h"

static void gr_intr_report_ctxsw_error(struct gk20a *g, u32 err_type, u32 chid,
		u32 mailbox_value)
{
	int ret = 0;
	struct ctxsw_err_info err_info;

	err_info.curr_ctx = g->ops.gr.falcon.get_current_ctx(g);
	err_info.ctxsw_status0 = g->ops.gr.falcon.read_fecs_ctxsw_status0(g);
	err_info.ctxsw_status1 = g->ops.gr.falcon.read_fecs_ctxsw_status1(g);
	err_info.mailbox_value = mailbox_value;
	err_info.chid = chid;

	if (g->ops.gr.err_ops.report_ctxsw_err != NULL) {
		ret = g->ops.gr.err_ops.report_ctxsw_err(g,
				NVGPU_ERR_MODULE_FECS,
				err_type, (void *)&err_info);
		if (ret != 0) {
			nvgpu_err(g, "Failed to report FECS CTXSW error: %d",
					err_type);
		}
	}
}

static int gr_intr_handle_tpc_exception(struct gk20a *g, u32 gpc, u32 tpc,
		bool *post_event, struct nvgpu_channel *fault_ch,
		u32 *hww_global_esr)
{
	int tmp_ret, ret = 0;
	struct nvgpu_gr_tpc_exception pending_tpc;
	u32 offset = nvgpu_gr_gpc_offset(g, gpc) + nvgpu_gr_tpc_offset(g, tpc);
	u32 tpc_exception = g->ops.gr.intr.get_tpc_exception(g, offset,
							&pending_tpc);
	u32 sm_per_tpc = nvgpu_get_litter_value(g, GPU_LIT_NUM_SM_PER_TPC);

	nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg,
			"GPC%d TPC%d: pending exception 0x%x",
			gpc, tpc, tpc_exception);

	/* check if an sm exeption is pending */
	if (pending_tpc.sm_exception) {
		u32 esr_sm_sel, sm;

		nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg,
				"GPC%d TPC%d: SM exception pending", gpc, tpc);

		if (g->ops.gr.handle_tpc_sm_ecc_exception != NULL) {
			g->ops.gr.handle_tpc_sm_ecc_exception(g, gpc, tpc,
				post_event, fault_ch, hww_global_esr);
		}

		g->ops.gr.get_esr_sm_sel(g, gpc, tpc, &esr_sm_sel);

		for (sm = 0; sm < sm_per_tpc; sm++) {

			if ((esr_sm_sel & BIT32(sm)) == 0U) {
				continue;
			}

			nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg,
				"GPC%d TPC%d: SM%d exception pending",
				 gpc, tpc, sm);

			tmp_ret = g->ops.gr.intr.handle_sm_exception(g,
					gpc, tpc, sm, post_event, fault_ch,
					hww_global_esr);
			ret = (ret != 0) ? ret : tmp_ret;

			/* clear the hwws, also causes tpc and gpc
			 * exceptions to be cleared. Should be cleared
			 * only if SM is locked down or empty.
			 */
			g->ops.gr.clear_sm_hww(g,
				gpc, tpc, sm, *hww_global_esr);

		}
	}

	/* check if a tex exception is pending */
	if (pending_tpc.tex_exception) {
		nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg,
			  "GPC%d TPC%d: TEX exception pending", gpc, tpc);
		if (g->ops.gr.intr.handle_tex_exception != NULL) {
			g->ops.gr.intr.handle_tex_exception(g, gpc, tpc);
		}
	}

	/* check if a mpc exception is pending */
	if (pending_tpc.mpc_exception) {
		nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg,
			  "GPC%d TPC%d: MPC exception pending", gpc, tpc);
		if (g->ops.gr.intr.handle_tpc_mpc_exception != NULL) {
			g->ops.gr.intr.handle_tpc_mpc_exception(g, gpc, tpc);
		}
	}

	return ret;
}

static void gr_intr_post_bpt_events(struct gk20a *g, struct nvgpu_tsg *tsg,
				    u32 global_esr)
{
	if (g->ops.gr.esr_bpt_pending_events(global_esr,
						NVGPU_EVENT_ID_BPT_INT)) {
		g->ops.tsg.post_event_id(tsg, NVGPU_EVENT_ID_BPT_INT);
	}

	if (g->ops.gr.esr_bpt_pending_events(global_esr,
						NVGPU_EVENT_ID_BPT_PAUSE)) {
		g->ops.tsg.post_event_id(tsg, NVGPU_EVENT_ID_BPT_PAUSE);
	}
}

static int gr_intr_handle_illegal_method(struct gk20a *g,
					  struct nvgpu_gr_isr_data *isr_data)
{
	int ret = g->ops.gr.intr.handle_sw_method(g, isr_data->addr,
			isr_data->class_num, isr_data->offset,
			isr_data->data_lo);
	if (ret != 0) {
		nvgpu_gr_intr_set_error_notifier(g, isr_data,
			 NVGPU_ERR_NOTIFIER_GR_ILLEGAL_NOTIFY);
		nvgpu_err(g, "invalid method class 0x%08x"
			", offset 0x%08x address 0x%08x",
			isr_data->class_num, isr_data->offset, isr_data->addr);
	}
	return ret;
}

static int gr_intr_handle_class_error(struct gk20a *g,
				       struct nvgpu_gr_isr_data *isr_data)
{
	u32 chid = isr_data->ch != NULL ?
		isr_data->ch->chid : NVGPU_INVALID_CHANNEL_ID;

	nvgpu_log_fn(g, " ");

	g->ops.gr.intr.handle_class_error(g, chid, isr_data);

	nvgpu_gr_intr_set_error_notifier(g, isr_data,
			 NVGPU_ERR_NOTIFIER_GR_ERROR_SW_NOTIFY);

	return -EINVAL;
}

static void gr_intr_report_sm_exception(struct gk20a *g, u32 gpc, u32 tpc,
		u32 sm, u32 hww_warp_esr_status, u64 hww_warp_esr_pc)
{
	int ret;
	struct gr_sm_mcerr_info err_info;
	struct nvgpu_channel *ch;
	struct gr_err_info info;
	u32 tsgid, chid, curr_ctx, inst = 0;

	if (g->ops.gr.err_ops.report_gr_err == NULL) {
		return;
	}

	tsgid = NVGPU_INVALID_TSG_ID;
	curr_ctx = g->ops.gr.falcon.get_current_ctx(g);
	ch = nvgpu_gr_intr_get_channel_from_ctx(g, curr_ctx, &tsgid);
	chid = ch != NULL ? ch->chid : NVGPU_INVALID_CHANNEL_ID;
	if (ch != NULL) {
		gk20a_channel_put(ch);
	}

	(void) memset(&err_info, 0, sizeof(err_info));
	(void) memset(&info, 0, sizeof(info));
	err_info.curr_ctx = curr_ctx;
	err_info.chid = chid;
	err_info.tsgid = tsgid;
	err_info.hww_warp_esr_pc = hww_warp_esr_pc;
	err_info.hww_warp_esr_status = hww_warp_esr_status;
	err_info.gpc = gpc;
	err_info.tpc = tpc;
	err_info.sm = sm;
	info.sm_mcerr_info = &err_info;
	ret = g->ops.gr.err_ops.report_gr_err(g,
			NVGPU_ERR_MODULE_SM, inst, GPU_SM_MACHINE_CHECK_ERROR,
			&info);
	if (ret != 0) {
		nvgpu_err(g, "failed to report SM_EXCEPTION "
				"gpc=%u, tpc=%u, sm=%u, esr_status=%x",
				gpc, tpc, sm, hww_warp_esr_status);
	}
}

/* Used by sw interrupt thread to translate current ctx to chid.
 * Also used by regops to translate current ctx to chid and tsgid.
 * For performance, we don't want to go through 128 channels every time.
 * curr_ctx should be the value read from gr falcon get_current_ctx op
 * A small tlb is used here to cache translation.
 *
 * Returned channel must be freed with gk20a_channel_put() */
struct nvgpu_channel *nvgpu_gr_intr_get_channel_from_ctx(struct gk20a *g,
			u32 curr_ctx, u32 *curr_tsgid)
{
	struct nvgpu_fifo *f = &g->fifo;
	struct nvgpu_gr_intr *intr = g->gr->intr;
	u32 chid;
	u32 tsgid = NVGPU_INVALID_TSG_ID;
	u32 i;
	struct nvgpu_channel *ret_ch = NULL;

	/* when contexts are unloaded from GR, the valid bit is reset
	 * but the instance pointer information remains intact.
	 * This might be called from gr_isr where contexts might be
	 * unloaded. No need to check ctx_valid bit
	 */

	nvgpu_spinlock_acquire(&intr->ch_tlb_lock);

	/* check cache first */
	for (i = 0; i < GR_CHANNEL_MAP_TLB_SIZE; i++) {
		if (intr->chid_tlb[i].curr_ctx == curr_ctx) {
			chid = intr->chid_tlb[i].chid;
			tsgid = intr->chid_tlb[i].tsgid;
			ret_ch = nvgpu_channel_from_id(g, chid);
			goto unlock;
		}
	}

	/* slow path */
	for (chid = 0; chid < f->num_channels; chid++) {
		struct nvgpu_channel *ch = nvgpu_channel_from_id(g, chid);

		if (ch == NULL) {
			continue;
		}

		if (nvgpu_inst_block_ptr(g, &ch->inst_block) ==
				g->ops.gr.falcon.get_ctx_ptr(curr_ctx)) {
			tsgid = ch->tsgid;
			/* found it */
			ret_ch = ch;
			break;
		}
		gk20a_channel_put(ch);
	}

	if (ret_ch == NULL) {
		goto unlock;
	}

	/* add to free tlb entry */
	for (i = 0; i < GR_CHANNEL_MAP_TLB_SIZE; i++) {
		if (intr->chid_tlb[i].curr_ctx == 0U) {
			intr->chid_tlb[i].curr_ctx = curr_ctx;
			intr->chid_tlb[i].chid = chid;
			intr->chid_tlb[i].tsgid = tsgid;
			goto unlock;
		}
	}

	/* no free entry, flush one */
	intr->chid_tlb[intr->channel_tlb_flush_index].curr_ctx = curr_ctx;
	intr->chid_tlb[intr->channel_tlb_flush_index].chid = chid;
	intr->chid_tlb[intr->channel_tlb_flush_index].tsgid = tsgid;

	intr->channel_tlb_flush_index =
		(intr->channel_tlb_flush_index + 1U) &
		(GR_CHANNEL_MAP_TLB_SIZE - 1U);

unlock:
	nvgpu_spinlock_release(&intr->ch_tlb_lock);
	if (curr_tsgid != NULL) {
		*curr_tsgid = tsgid;
	}
	return ret_ch;
}

void nvgpu_gr_intr_report_exception(struct gk20a *g, u32 inst,
		u32 err_type, u32 status)
{
	int ret = 0;
	struct nvgpu_channel *ch;
	struct gr_exception_info err_info;
	struct gr_err_info info;
	u32 tsgid, chid, curr_ctx;

	if (g->ops.gr.err_ops.report_gr_err == NULL) {
		return;
	}

	tsgid = NVGPU_INVALID_TSG_ID;
	curr_ctx = g->ops.gr.falcon.get_current_ctx(g);
	ch = nvgpu_gr_intr_get_channel_from_ctx(g, curr_ctx, &tsgid);
	chid = ch != NULL ? ch->chid : NVGPU_INVALID_CHANNEL_ID;
	if (ch != NULL) {
		gk20a_channel_put(ch);
	}

	(void) memset(&err_info, 0, sizeof(err_info));
	(void) memset(&info, 0, sizeof(info));
	err_info.curr_ctx = curr_ctx;
	err_info.chid = chid;
	err_info.tsgid = tsgid;
	err_info.status = status;
	info.exception_info = &err_info;
	ret = g->ops.gr.err_ops.report_gr_err(g,
			NVGPU_ERR_MODULE_PGRAPH, inst, err_type,
			&info);
	if (ret != 0) {
		nvgpu_err(g, "Failed to report PGRAPH exception: "
				"inst=%u, err_type=%u, status=%u",
				inst, err_type, status);
	}
}

void nvgpu_gr_intr_set_error_notifier(struct gk20a *g,
		  struct nvgpu_gr_isr_data *isr_data, u32 error_notifier)
{
	struct nvgpu_channel *ch;
	struct nvgpu_tsg *tsg;

	ch = isr_data->ch;

	if (ch == NULL) {
		return;
	}

	tsg = tsg_gk20a_from_ch(ch);
	if (tsg != NULL) {
		nvgpu_tsg_set_error_notifier(g, tsg, error_notifier);
	} else {
		nvgpu_err(g, "chid: %d is not bound to tsg", ch->chid);
	}
}

int nvgpu_gr_intr_handle_sm_exception(struct gk20a *g, u32 gpc, u32 tpc, u32 sm,
		bool *post_event, struct nvgpu_channel *fault_ch,
		u32 *hww_global_esr)
{
	int ret = 0;
	bool do_warp_sync = false, early_exit = false, ignore_debugger = false;
	bool disable_sm_exceptions = true;
	u32 offset = nvgpu_gr_gpc_offset(g, gpc) + nvgpu_gr_tpc_offset(g, tpc);
	bool sm_debugger_attached;
	u32 global_esr, warp_esr, global_mask;
	u64 hww_warp_esr_pc = 0;

	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg, " ");

	sm_debugger_attached = g->ops.gr.sm_debugger_attached(g);

	global_esr = g->ops.gr.get_sm_hww_global_esr(g, gpc, tpc, sm);
	*hww_global_esr = global_esr;
	warp_esr = g->ops.gr.get_sm_hww_warp_esr(g, gpc, tpc, sm);
	global_mask = g->ops.gr.get_sm_no_lock_down_hww_global_esr_mask(g);

	if (!sm_debugger_attached) {
		nvgpu_err(g, "sm hww global 0x%08x warp 0x%08x",
			  global_esr, warp_esr);
		return -EFAULT;
	}

	nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg,
		  "sm hww global 0x%08x warp 0x%08x", global_esr, warp_esr);

	/*
	 * Check and report any fatal wrap errors.
	 */
	if ((global_esr & ~global_mask) != 0U) {
		if (g->ops.gr.get_sm_hww_warp_esr_pc != NULL) {
			hww_warp_esr_pc = g->ops.gr.get_sm_hww_warp_esr_pc(g,
					offset);
		}
		gr_intr_report_sm_exception(g, gpc, tpc, sm, warp_esr,
				hww_warp_esr_pc);
	}
	nvgpu_pg_elpg_protected_call(g,
		g->ops.gr.record_sm_error_state(g, gpc, tpc, sm, fault_ch));

	if (g->ops.gr.pre_process_sm_exception != NULL) {
		ret = g->ops.gr.pre_process_sm_exception(g, gpc, tpc, sm,
				global_esr, warp_esr,
				sm_debugger_attached,
				fault_ch,
				&early_exit,
				&ignore_debugger);
		if (ret != 0) {
			nvgpu_err(g, "could not pre-process sm error!");
			return ret;
		}
	}

	if (early_exit) {
		nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg,
				"returning early");
		return ret;
	}

	/*
	 * Disable forwarding of tpc exceptions,
	 * the debugger will reenable exceptions after servicing them.
	 *
	 * Do not disable exceptions if the only SM exception is BPT_INT
	 */
	if ((g->ops.gr.esr_bpt_pending_events(global_esr,
			NVGPU_EVENT_ID_BPT_INT)) && (warp_esr == 0U)) {
		disable_sm_exceptions = false;
	}

	if (!ignore_debugger && disable_sm_exceptions) {
		g->ops.gr.intr.tpc_exception_sm_disable(g, offset);
		nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg,
			  "SM Exceptions disabled");
	}

	/* if a debugger is present and an error has occurred, do a warp sync */
	if (!ignore_debugger &&
	    ((warp_esr != 0U) || ((global_esr & ~global_mask) != 0U))) {
		nvgpu_log(g, gpu_dbg_intr, "warp sync needed");
		do_warp_sync = true;
	}

	if (do_warp_sync) {
		ret = g->ops.gr.lock_down_sm(g, gpc, tpc, sm,
				 global_mask, true);
		if (ret != 0) {
			nvgpu_err(g, "sm did not lock down!");
			return ret;
		}
	}

	if (ignore_debugger) {
		nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg,
			"ignore_debugger set, skipping event posting");
	} else {
		*post_event = true;
	}

	return ret;
}

int nvgpu_gr_intr_handle_fecs_error(struct gk20a *g, struct nvgpu_channel *ch,
					struct nvgpu_gr_isr_data *isr_data)
{
	u32 gr_fecs_intr, mailbox_value;
	int ret = 0;
	struct nvgpu_fecs_host_intr_status fecs_host_intr;
	u32 chid = isr_data->ch != NULL ?
		isr_data->ch->chid : NVGPU_INVALID_CHANNEL_ID;
	u32 mailbox_id = NVGPU_GR_FALCON_FECS_CTXSW_MAILBOX6;

	gr_fecs_intr = g->ops.gr.falcon.fecs_host_intr_status(g,
						&fecs_host_intr);
	if (gr_fecs_intr == 0U) {
		return 0;
	}

	if (fecs_host_intr.unimp_fw_method_active) {
		mailbox_value = g->ops.gr.falcon.read_fecs_ctxsw_mailbox(g,
								mailbox_id);
		nvgpu_gr_intr_set_error_notifier(g, isr_data,
			 NVGPU_ERR_NOTIFIER_FECS_ERR_UNIMP_FIRMWARE_METHOD);
		nvgpu_err(g,
			  "firmware method error 0x%08x for offset 0x%04x",
			  mailbox_value,
			  isr_data->data_lo);
		ret = -1;
	} else if (fecs_host_intr.watchdog_active) {
		gr_intr_report_ctxsw_error(g,
				GPU_FECS_CTXSW_WATCHDOG_TIMEOUT,
				chid, 0);
		/* currently, recovery is not initiated */
		nvgpu_err(g, "fecs watchdog triggered for channel %u, "
				"cannot ctxsw anymore !!", chid);
		g->ops.gr.falcon.dump_stats(g);
	} else if (fecs_host_intr.ctxsw_intr0 != 0U) {
		mailbox_value = g->ops.gr.falcon.read_fecs_ctxsw_mailbox(g,
								mailbox_id);
#ifdef CONFIG_GK20A_CTXSW_TRACE
		if (mailbox_value ==
			g->ops.gr.fecs_trace.get_buffer_full_mailbox_val()) {
			nvgpu_info(g, "ctxsw intr0 set by ucode, "
					"timestamp buffer full");
			nvgpu_gr_fecs_trace_reset_buffer(g);
		} else
#endif
		/*
		 * The mailbox values may vary across chips hence keeping it
		 * as a HAL.
		 */
		if ((g->ops.gr.get_ctxsw_checksum_mismatch_mailbox_val != NULL)
			&& (mailbox_value ==
			g->ops.gr.get_ctxsw_checksum_mismatch_mailbox_val())) {

			gr_intr_report_ctxsw_error(g,
					GPU_FECS_CTXSW_CRC_MISMATCH,
					chid, mailbox_value);
			nvgpu_err(g, "ctxsw intr0 set by ucode, "
					"ctxsw checksum mismatch");
			ret = -1;
		} else {
			/*
			 * Other errors are also treated as fatal and channel
			 * recovery is initiated and error is reported to
			 * 3LSS.
			 */
			gr_intr_report_ctxsw_error(g,
					GPU_FECS_FAULT_DURING_CTXSW,
					chid, mailbox_value);
			nvgpu_err(g,
				 "ctxsw intr0 set by ucode, error_code: 0x%08x",
				 mailbox_value);
			ret = -1;
		}
	} else if (fecs_host_intr.fault_during_ctxsw_active) {
		gr_intr_report_ctxsw_error(g,
				GPU_FECS_FAULT_DURING_CTXSW,
				chid, 0);
		nvgpu_err(g, "fecs fault during ctxsw for channel %u", chid);
		ret = -1;
	} else {
		nvgpu_err(g,
			"unhandled fecs error interrupt 0x%08x for channel %u",
			gr_fecs_intr, chid);
		g->ops.gr.falcon.dump_stats(g);
	}

	g->ops.gr.falcon.fecs_host_clear_intr(g, gr_fecs_intr);

	return ret;
}

int nvgpu_gr_intr_handle_gpc_exception(struct gk20a *g, bool *post_event,
	struct nvgpu_gr_config *gr_config, struct nvgpu_channel *fault_ch,
	u32 *hww_global_esr)
{
	int tmp_ret, ret = 0;
	u32 gpc, tpc;
	u32 exception1 = g->ops.gr.intr.read_exception1(g);
	u32 gpc_exception, tpc_exception;

	nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg, " ");

	for (gpc = 0; gpc < nvgpu_gr_config_get_gpc_count(gr_config); gpc++) {
		if ((exception1 & BIT32(gpc)) == 0U) {
			continue;
		}

		nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg,
				"GPC%d exception pending", gpc);
		gpc_exception = g->ops.gr.intr.read_gpc_exception(g, gpc);
		tpc_exception = g->ops.gr.intr.read_gpc_tpc_exception(
							gpc_exception);

		/* check if any tpc has an exception */
		for (tpc = 0;
		     tpc < nvgpu_gr_config_get_gpc_tpc_count(gr_config, gpc);
		     tpc++) {
			if ((tpc_exception & BIT32(tpc)) == 0U) {
				continue;
			}

			nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg,
				  "GPC%d: TPC%d exception pending", gpc, tpc);

			tmp_ret = gr_intr_handle_tpc_exception(g, gpc, tpc,
					post_event, fault_ch, hww_global_esr);
			ret = (ret != 0) ? ret : tmp_ret;
		}

		/* Handle GCC exception */
		if (g->ops.gr.intr.handle_gcc_exception != NULL) {
			g->ops.gr.intr.handle_gcc_exception(g, gpc,
				tpc, gpc_exception,
				&g->ecc.gr.gcc_l15_ecc_corrected_err_count[gpc].counter,
				&g->ecc.gr.gcc_l15_ecc_uncorrected_err_count[gpc].counter);
		}

		/* Handle GPCCS exceptions */
		if (g->ops.gr.intr.handle_gpc_gpccs_exception != NULL) {
			g->ops.gr.intr.handle_gpc_gpccs_exception(g, gpc,
				gpc_exception,
				&g->ecc.gr.gpccs_ecc_corrected_err_count[gpc].counter,
				&g->ecc.gr.gpccs_ecc_uncorrected_err_count[gpc].counter);
		}

		/* Handle GPCMMU exceptions */
		if (g->ops.gr.intr.handle_gpc_gpcmmu_exception != NULL) {
			 g->ops.gr.intr.handle_gpc_gpcmmu_exception(g, gpc,
				gpc_exception,
				&g->ecc.gr.mmu_l1tlb_ecc_corrected_err_count[gpc].counter,
				&g->ecc.gr.mmu_l1tlb_ecc_uncorrected_err_count[gpc].counter);
		}

	}

	return ret;
}

void nvgpu_gr_intr_handle_notify_pending(struct gk20a *g,
					struct nvgpu_gr_isr_data *isr_data)
{
	struct nvgpu_channel *ch = isr_data->ch;
	int err;

	if (ch == NULL) {
		return;
	}

	if (tsg_gk20a_from_ch(ch) == NULL) {
		return;
	}

	nvgpu_log_fn(g, " ");

#if defined(CONFIG_GK20A_CYCLE_STATS)
	nvgpu_cyclestats_exec(g, ch, isr_data->data_lo);
#endif

	err = nvgpu_cond_broadcast_interruptible(&ch->notifier_wq);
	if (err != 0) {
		nvgpu_log(g, gpu_dbg_intr, "failed to broadcast");
	}
}

void nvgpu_gr_intr_handle_semaphore_pending(struct gk20a *g,
					   struct nvgpu_gr_isr_data *isr_data)
{
	struct nvgpu_channel *ch = isr_data->ch;
	struct nvgpu_tsg *tsg;

	if (ch == NULL) {
		return;
	}

	tsg = tsg_gk20a_from_ch(ch);
	if (tsg != NULL) {
		int err;

		g->ops.tsg.post_event_id(tsg,
			NVGPU_EVENT_ID_GR_SEMAPHORE_WRITE_AWAKEN);

		err = nvgpu_cond_broadcast(&ch->semaphore_wq);
		if (err != 0) {
			nvgpu_log(g, gpu_dbg_intr, "failed to broadcast");
		}
	} else {
		nvgpu_err(g, "chid: %d is not bound to tsg", ch->chid);
	}
}

int nvgpu_gr_intr_stall_isr(struct gk20a *g)
{
	struct nvgpu_gr_isr_data isr_data;
	struct nvgpu_gr_intr_info intr_info;
	bool need_reset = false;
	struct nvgpu_channel *ch = NULL;
	struct nvgpu_channel *fault_ch = NULL;
	u32 tsgid = NVGPU_INVALID_TSG_ID;
	struct nvgpu_tsg *tsg = NULL;
	u32 global_esr = 0;
	u32 chid;
	struct nvgpu_gr_config *gr_config = g->gr->config;
	u32 gr_intr = g->ops.gr.intr.read_pending_interrupts(g, &intr_info);
	u32 clear_intr = gr_intr;

	nvgpu_log_fn(g, " ");
	nvgpu_log(g, gpu_dbg_intr, "pgraph intr 0x%08x", gr_intr);

	if (gr_intr == 0U) {
		return 0;
	}

	/* Disable fifo access */
	g->ops.gr.init.fifo_access(g, false);

	g->ops.gr.intr.trapped_method_info(g, &isr_data);

	ch = nvgpu_gr_intr_get_channel_from_ctx(g, isr_data.curr_ctx, &tsgid);
	isr_data.ch = ch;
	chid = ch != NULL ? ch->chid : NVGPU_INVALID_CHANNEL_ID;

	if (ch == NULL) {
		nvgpu_err(g, "pgraph intr: 0x%08x, chid: INVALID", gr_intr);
	} else {
		tsg = tsg_gk20a_from_ch(ch);
		if (tsg == NULL) {
			nvgpu_err(g, "pgraph intr: 0x%08x, chid: %d "
				"not bound to tsg", gr_intr, chid);
		}
	}

	nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg,
		"channel %d: addr 0x%08x, "
		"data 0x%08x 0x%08x,"
		"ctx 0x%08x, offset 0x%08x, "
		"subchannel 0x%08x, class 0x%08x",
		chid, isr_data.addr,
		isr_data.data_hi, isr_data.data_lo,
		isr_data.curr_ctx, isr_data.offset,
		isr_data.sub_chan, isr_data.class_num);

	if (intr_info.notify != 0U) {
		g->ops.gr.intr.handle_notify_pending(g, &isr_data);
		clear_intr &= ~intr_info.notify;
	}

	if (intr_info.semaphore != 0U) {
		g->ops.gr.intr.handle_semaphore_pending(g, &isr_data);
		clear_intr &= ~intr_info.semaphore;
	}

	if (intr_info.illegal_notify != 0U) {
		nvgpu_err(g, "illegal notify pending");

		nvgpu_gr_intr_set_error_notifier(g, &isr_data,
				NVGPU_ERR_NOTIFIER_GR_ILLEGAL_NOTIFY);
		need_reset = true;
		clear_intr &= ~intr_info.illegal_notify;
	}

	if (intr_info.illegal_method != 0U) {
		if (gr_intr_handle_illegal_method(g, &isr_data) != 0) {
			need_reset = true;
		}
		clear_intr &= ~intr_info.illegal_method;
	}

	if (intr_info.illegal_class != 0U) {
		nvgpu_err(g, "invalid class 0x%08x, offset 0x%08x",
			  isr_data.class_num, isr_data.offset);

		nvgpu_gr_intr_set_error_notifier(g, &isr_data,
				NVGPU_ERR_NOTIFIER_GR_ERROR_SW_NOTIFY);
		need_reset = true;
		clear_intr &= ~intr_info.illegal_class;
	}

	if (intr_info.fecs_error != 0U) {
		if (g->ops.gr.intr.handle_fecs_error(g, ch, &isr_data) != 0) {
			need_reset = true;
		}
		clear_intr &= ~intr_info.fecs_error;
	}

	if (intr_info.class_error != 0U) {
		if (gr_intr_handle_class_error(g, &isr_data) != 0) {
			need_reset = true;
		}
		clear_intr &= ~intr_info.class_error;
	}

	/* this one happens if someone tries to hit a non-whitelisted
	 * register using set_falcon[4] */
	if (intr_info.fw_method != 0U) {
		u32 ch_id = isr_data.ch != NULL ?
			isr_data.ch->chid : NVGPU_INVALID_CHANNEL_ID;
		nvgpu_err(g,
		   "firmware method 0x%08x, offset 0x%08x for channel %u",
		   isr_data.class_num, isr_data.offset,
		   ch_id);

		nvgpu_gr_intr_set_error_notifier(g, &isr_data,
			 NVGPU_ERR_NOTIFIER_GR_ERROR_SW_NOTIFY);
		need_reset = true;
		clear_intr &= ~intr_info.fw_method;
	}

	if (intr_info.exception != 0U) {
		bool is_gpc_exception = false;

		need_reset = g->ops.gr.intr.handle_exceptions(g,
							&is_gpc_exception);

		/* check if a gpc exception has occurred */
		if (is_gpc_exception &&	!need_reset) {
			bool post_event = false;

			nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg,
					 "GPC exception pending");

			if (tsg != NULL) {
				fault_ch = isr_data.ch;
			}

			/* fault_ch can be NULL */
			/* check if any gpc has an exception */
			if (nvgpu_gr_intr_handle_gpc_exception(g, &post_event,
				gr_config, fault_ch, &global_esr) != 0) {
				need_reset = true;
			}

#ifdef NVGPU_DEBUGGER
			/* signal clients waiting on an event */
			if (g->ops.gr.sm_debugger_attached(g) &&
				post_event && (fault_ch != NULL)) {
				g->ops.debugger.post_events(fault_ch);
			}
#endif
		}
		clear_intr &= ~intr_info.exception;

		if (need_reset) {
			nvgpu_err(g, "set gr exception notifier");
			nvgpu_gr_intr_set_error_notifier(g, &isr_data,
					 NVGPU_ERR_NOTIFIER_GR_EXCEPTION);
		}
	}

	if (need_reset) {
		nvgpu_rc_gr_fault(g, tsg, ch);
	}

	if (clear_intr != 0U) {
		if (ch == NULL) {
			/*
			 * This is probably an interrupt during
			 * gk20a_free_channel()
			 */
			nvgpu_err(g, "unhandled gr intr 0x%08x for "
				"unreferenceable channel, clearing",
				gr_intr);
		} else {
			nvgpu_err(g, "unhandled gr intr 0x%08x for chid: %d",
				gr_intr, chid);
		}
	}

	/* clear handled and unhandled interrupts */
	g->ops.gr.intr.clear_pending_interrupts(g, gr_intr);

	/* Enable fifo access */
	g->ops.gr.init.fifo_access(g, true);

	/* Posting of BPT events should be the last thing in this function */
	if ((global_esr != 0U) && (tsg != NULL) && (need_reset == false)) {
		gr_intr_post_bpt_events(g, tsg, global_esr);
	}

	if (ch != NULL) {
		gk20a_channel_put(ch);
	}

	return 0;
}

/* invalidate channel lookup tlb */
void nvgpu_gr_intr_flush_channel_tlb(struct gk20a *g)
{
	struct nvgpu_gr_intr *intr = g->gr->intr;

	nvgpu_spinlock_acquire(&intr->ch_tlb_lock);
	(void) memset(intr->chid_tlb, 0,
		sizeof(struct gr_channel_map_tlb_entry) *
		GR_CHANNEL_MAP_TLB_SIZE);
	nvgpu_spinlock_release(&intr->ch_tlb_lock);
}

struct nvgpu_gr_intr *nvgpu_gr_intr_init_support(struct gk20a *g)
{
	struct nvgpu_gr_intr *intr;

	nvgpu_log_fn(g, " ");

	intr = nvgpu_kzalloc(g, sizeof(*intr));
	if (intr == NULL) {
		return intr;
	}

	nvgpu_spinlock_init(&intr->ch_tlb_lock);

	return intr;
}

void nvgpu_gr_intr_remove_support(struct gk20a *g, struct nvgpu_gr_intr *intr)
{
	nvgpu_log_fn(g, " ");

	if (intr == NULL) {
		return;
	}
	nvgpu_kfree(g, intr);
}
