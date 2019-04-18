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
#include <nvgpu/regops.h>
#include <nvgpu/rc.h>
#include <nvgpu/error_notifier.h>

#include <nvgpu/gr/gr.h>
#include <nvgpu/gr/gr_intr.h>
#include <nvgpu/gr/config.h>

static int gr_intr_handle_tpc_exception(struct gk20a *g, u32 gpc, u32 tpc,
		bool *post_event, struct channel_gk20a *fault_ch,
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

			tmp_ret = g->ops.gr.handle_sm_exception(g,
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

static void gr_intr_post_bpt_events(struct gk20a *g, struct tsg_gk20a *tsg,
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
	int ret = g->ops.gr.handle_sw_method(g, isr_data->addr,
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
		isr_data->ch->chid : FIFO_INVAL_CHANNEL_ID;

	nvgpu_log_fn(g, " ");

	g->ops.gr.intr.handle_class_error(g, chid, isr_data);

	nvgpu_gr_intr_set_error_notifier(g, isr_data,
			 NVGPU_ERR_NOTIFIER_GR_ERROR_SW_NOTIFY);

	return -EINVAL;
}

/* Used by sw interrupt thread to translate current ctx to chid.
 * Also used by regops to translate current ctx to chid and tsgid.
 * For performance, we don't want to go through 128 channels every time.
 * curr_ctx should be the value read from gr falcon get_current_ctx op
 * A small tlb is used here to cache translation.
 *
 * Returned channel must be freed with gk20a_channel_put() */
struct channel_gk20a *nvgpu_gr_intr_get_channel_from_ctx(struct gk20a *g,
			u32 curr_ctx, u32 *curr_tsgid)
{
	struct fifo_gk20a *f = &g->fifo;
	struct nvgpu_gr *gr = g->gr;
	u32 chid;
	u32 tsgid = NVGPU_INVALID_TSG_ID;
	u32 i;
	struct channel_gk20a *ret_ch = NULL;

	/* when contexts are unloaded from GR, the valid bit is reset
	 * but the instance pointer information remains intact.
	 * This might be called from gr_isr where contexts might be
	 * unloaded. No need to check ctx_valid bit
	 */

	nvgpu_spinlock_acquire(&gr->ch_tlb_lock);

	/* check cache first */
	for (i = 0; i < GR_CHANNEL_MAP_TLB_SIZE; i++) {
		if (gr->chid_tlb[i].curr_ctx == curr_ctx) {
			chid = gr->chid_tlb[i].chid;
			tsgid = gr->chid_tlb[i].tsgid;
			ret_ch = gk20a_channel_from_id(g, chid);
			goto unlock;
		}
	}

	/* slow path */
	for (chid = 0; chid < f->num_channels; chid++) {
		struct channel_gk20a *ch = gk20a_channel_from_id(g, chid);

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
		if (gr->chid_tlb[i].curr_ctx == 0U) {
			gr->chid_tlb[i].curr_ctx = curr_ctx;
			gr->chid_tlb[i].chid = chid;
			gr->chid_tlb[i].tsgid = tsgid;
			goto unlock;
		}
	}

	/* no free entry, flush one */
	gr->chid_tlb[gr->channel_tlb_flush_index].curr_ctx = curr_ctx;
	gr->chid_tlb[gr->channel_tlb_flush_index].chid = chid;
	gr->chid_tlb[gr->channel_tlb_flush_index].tsgid = tsgid;

	gr->channel_tlb_flush_index =
		(gr->channel_tlb_flush_index + 1U) &
		(GR_CHANNEL_MAP_TLB_SIZE - 1U);

unlock:
	nvgpu_spinlock_release(&gr->ch_tlb_lock);
	if (curr_tsgid != NULL) {
		*curr_tsgid = tsgid;
	}
	return ret_ch;
}

void nvgpu_gr_intr_report_exception(struct gk20a *g, u32 inst,
		u32 err_type, u32 status)
{
	int ret = 0;
	struct channel_gk20a *ch;
	struct gr_exception_info err_info;
	struct gr_err_info info;
	u32 tsgid, chid, curr_ctx;

	if (g->ops.gr.err_ops.report_gr_err == NULL) {
		return;
	}

	tsgid = NVGPU_INVALID_TSG_ID;
	curr_ctx = g->ops.gr.falcon.get_current_ctx(g);
	ch = nvgpu_gr_intr_get_channel_from_ctx(g, curr_ctx, &tsgid);
	chid = ch != NULL ? ch->chid : FIFO_INVAL_CHANNEL_ID;
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
	struct channel_gk20a *ch;
	struct tsg_gk20a *tsg;

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

int nvgpu_gr_intr_handle_gpc_exception(struct gk20a *g, bool *post_event,
	struct nvgpu_gr_config *gr_config, struct channel_gk20a *fault_ch,
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

#if defined(CONFIG_GK20A_CYCLE_STATS)
static inline bool is_valid_cyclestats_bar0_offset_gk20a(struct gk20a *g,
							 u32 offset)
{
	/* support only 24-bit 4-byte aligned offsets */
	bool valid = !(offset & 0xFF000003U);

	if (g->allow_all) {
		return true;
	}

	/* whitelist check */
	valid = valid &&
		is_bar0_global_offset_whitelisted_gk20a(g, offset);
	/* resource size check in case there was a problem
	 * with allocating the assumed size of bar0 */
	valid = valid && nvgpu_io_valid_reg(g, offset);
	return valid;
}
#endif

void nvgpu_gr_intr_handle_notify_pending(struct gk20a *g,
					struct nvgpu_gr_isr_data *isr_data)
{
	struct channel_gk20a *ch = isr_data->ch;

#if defined(CONFIG_GK20A_CYCLE_STATS)
	void *virtual_address;
	u32 buffer_size;
	u32 offset;
	bool exit;
#endif
	if (ch == NULL || tsg_gk20a_from_ch(ch) == NULL) {
		return;
	}

#if defined(CONFIG_GK20A_CYCLE_STATS)
	/* GL will never use payload 0 for cycle state */
	if ((ch->cyclestate.cyclestate_buffer == NULL) ||
	    (isr_data->data_lo == 0)) {
		return;
	}

	nvgpu_mutex_acquire(&ch->cyclestate.cyclestate_buffer_mutex);

	virtual_address = ch->cyclestate.cyclestate_buffer;
	buffer_size = ch->cyclestate.cyclestate_buffer_size;
	offset = isr_data->data_lo;
	exit = false;
	while (!exit) {
		struct share_buffer_head *sh_hdr;
		u32 min_element_size;

		/* validate offset */
		if (offset + sizeof(struct share_buffer_head) > buffer_size ||
		    offset + sizeof(struct share_buffer_head) < offset) {
			nvgpu_err(g,
				  "cyclestats buffer overrun at offset 0x%x",
				  offset);
			break;
		}

		sh_hdr = (struct share_buffer_head *)
			 ((char *)virtual_address + offset);

		min_element_size =
			U32(sh_hdr->operation == OP_END ?
			 sizeof(struct share_buffer_head) :
			 sizeof(struct gk20a_cyclestate_buffer_elem));

		/* validate sh_hdr->size */
		if (sh_hdr->size < min_element_size ||
		    offset + sh_hdr->size > buffer_size ||
		    offset + sh_hdr->size < offset) {
			nvgpu_err(g,
				  "bad cyclestate buffer header size at offset 0x%x",
				  offset);
			sh_hdr->failed = U32(true);
			break;
		}

		switch (sh_hdr->operation) {
		case OP_END:
			exit = true;
			break;

		case BAR0_READ32:
		case BAR0_WRITE32:
		{
			struct gk20a_cyclestate_buffer_elem *op_elem =
				(struct gk20a_cyclestate_buffer_elem *)sh_hdr;
			bool valid = is_valid_cyclestats_bar0_offset_gk20a(
						g, op_elem->offset_bar0);
			u32 raw_reg;
			u64 mask_orig;
			u64 v;

			if (!valid) {
				nvgpu_err(g,
					   "invalid cycletstats op offset: 0x%x",
					   op_elem->offset_bar0);

				exit = true;
				sh_hdr->failed = U32(exit);
				break;
			}

			mask_orig =
				((1ULL << (op_elem->last_bit + 1)) - 1) &
				~((1ULL << op_elem->first_bit) - 1);

			raw_reg = nvgpu_readl(g, op_elem->offset_bar0);

			switch (sh_hdr->operation) {
			case BAR0_READ32:
				op_elem->data =	((raw_reg & mask_orig)
							>> op_elem->first_bit);
				break;

			case BAR0_WRITE32:
				v = 0;
				if ((unsigned int)mask_orig !=
							~((unsigned int)0)) {
					v = (unsigned int)
						(raw_reg & ~mask_orig);
				}

				v |= ((op_elem->data << op_elem->first_bit)
							& mask_orig);
				nvgpu_writel(g,op_elem->offset_bar0,
					     (unsigned int)v);
				break;
			default:
				/* nop ok?*/
				break;
			}
		}
		break;

		default:
			/* no operation content case */
			exit = true;
			break;
		}
		sh_hdr->completed = U32(true);
		offset += sh_hdr->size;
	}
	nvgpu_mutex_release(&ch->cyclestate.cyclestate_buffer_mutex);
#endif
	nvgpu_log_fn(g, " ");
	nvgpu_cond_broadcast_interruptible(&ch->notifier_wq);
}

void nvgpu_gr_intr_handle_semaphore_pending(struct gk20a *g,
					   struct nvgpu_gr_isr_data *isr_data)
{
	struct channel_gk20a *ch = isr_data->ch;
	struct tsg_gk20a *tsg;

	if (ch == NULL) {
		return;
	}

	tsg = tsg_gk20a_from_ch(ch);
	if (tsg != NULL) {
		g->ops.tsg.post_event_id(tsg,
			NVGPU_EVENT_ID_GR_SEMAPHORE_WRITE_AWAKEN);

		nvgpu_cond_broadcast(&ch->semaphore_wq);
	} else {
		nvgpu_err(g, "chid: %d is not bound to tsg", ch->chid);
	}
}

int nvgpu_gr_intr_stall_isr(struct gk20a *g)
{
	struct nvgpu_gr_isr_data isr_data;
	struct nvgpu_gr_intr_info intr_info;
	bool need_reset = false;
	struct channel_gk20a *ch = NULL;
	struct channel_gk20a *fault_ch = NULL;
	u32 tsgid = NVGPU_INVALID_TSG_ID;
	struct tsg_gk20a *tsg = NULL;
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
	chid = ch != NULL ? ch->chid : FIFO_INVAL_CHANNEL_ID;

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
		if (g->ops.gr.handle_fecs_error(g, ch, &isr_data) != 0) {
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
			isr_data.ch->chid : FIFO_INVAL_CHANNEL_ID;
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
