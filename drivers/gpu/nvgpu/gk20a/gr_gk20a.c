/*
 * GK20A Graphics
 *
 * Copyright (c) 2011-2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/dma.h>
#include <nvgpu/kmem.h>
#include <nvgpu/gmmu.h>
#include <nvgpu/timers.h>
#include <nvgpu/nvgpu_common.h>
#include <nvgpu/log.h>
#include <nvgpu/bug.h>
#include <nvgpu/firmware.h>
#include <nvgpu/enabled.h>
#include <nvgpu/debug.h>
#include <nvgpu/barrier.h>
#include <nvgpu/mm.h>
#include <nvgpu/debugger.h>
#include <nvgpu/netlist.h>
#include <nvgpu/error_notifier.h>
#include <nvgpu/ecc.h>
#include <nvgpu/io.h>
#include <nvgpu/utils.h>
#include <nvgpu/fifo.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/channel.h>
#include <nvgpu/unit.h>
#include <nvgpu/string.h>
#include <nvgpu/regops.h>
#include <nvgpu/gr/global_ctx.h>
#include <nvgpu/gr/subctx.h>
#include <nvgpu/gr/ctx.h>
#include <nvgpu/gr/zbc.h>
#include <nvgpu/gr/gr.h>
#include <nvgpu/gr/gr_falcon.h>
#include <nvgpu/gr/zcull.h>
#include <nvgpu/gr/config.h>
#include <nvgpu/gr/fecs_trace.h>
#include <nvgpu/gr/hwpm_map.h>
#include <nvgpu/engines.h>
#include <nvgpu/engine_status.h>
#include <nvgpu/nvgpu_err.h>
#include <nvgpu/power_features/cg.h>
#include <nvgpu/power_features/pg.h>

#include "gr_gk20a.h"
#include "gr_pri_gk20a.h"

#include <nvgpu/hw/gk20a/hw_fifo_gk20a.h>
#include <nvgpu/hw/gk20a/hw_gr_gk20a.h>
#include <nvgpu/hw/gk20a/hw_ram_gk20a.h>

static struct channel_gk20a *gk20a_gr_get_channel_from_ctx(
	struct gk20a *g, u32 curr_ctx, u32 *curr_tsgid);

static int gk20a_init_gr_bind_fecs_elpg(struct gk20a *g);

void nvgpu_report_gr_exception(struct gk20a *g, u32 inst,
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
	curr_ctx = gk20a_readl(g, gr_fecs_current_ctx_r());
	ch = gk20a_gr_get_channel_from_ctx(g, curr_ctx, &tsgid);
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

static void nvgpu_report_gr_sm_exception(struct gk20a *g, u32 gpc, u32 tpc,
		u32 sm, u32 hww_warp_esr_status, u64 hww_warp_esr_pc)
{
	int ret;
	struct gr_sm_mcerr_info err_info;
	struct channel_gk20a *ch;
	struct gr_err_info info;
	u32 tsgid, chid, curr_ctx, inst = 0;

	if (g->ops.gr.err_ops.report_gr_err == NULL) {
		return;
	}

	tsgid = NVGPU_INVALID_TSG_ID;
	curr_ctx = gk20a_readl(g, gr_fecs_current_ctx_r());
	ch = gk20a_gr_get_channel_from_ctx(g, curr_ctx, &tsgid);
	chid = ch != NULL ? ch->chid : FIFO_INVAL_CHANNEL_ID;
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

static void gr_report_ctxsw_error(struct gk20a *g, u32 err_type, u32 chid,
		u32 mailbox_value)
{
	int ret = 0;
	struct ctxsw_err_info err_info;

	err_info.curr_ctx = gk20a_readl(g, gr_fecs_current_ctx_r());
	err_info.ctxsw_status0 = gk20a_readl(g, gr_fecs_ctxsw_status_fe_0_r());
	err_info.ctxsw_status1 = gk20a_readl(g, gr_fecs_ctxsw_status_1_r());
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
int gr_gk20a_ctx_wait_ucode(struct gk20a *g, u32 mailbox_id,
			    u32 *mailbox_ret, u32 opc_success,
			    u32 mailbox_ok, u32 opc_fail,
			    u32 mailbox_fail, bool sleepduringwait)
{
	struct nvgpu_timeout timeout;
	u32 delay = GR_FECS_POLL_INTERVAL;
	enum wait_ucode_status check = WAIT_UCODE_LOOP;
	u32 reg;

	nvgpu_log_fn(g, " ");

	if (sleepduringwait) {
		delay = POLL_DELAY_MIN_US;
	}

	nvgpu_timeout_init(g, &timeout, nvgpu_get_poll_timeout(g),
			   NVGPU_TIMER_CPU_TIMER);

	while (check == WAIT_UCODE_LOOP) {
		if (nvgpu_timeout_expired(&timeout) != 0) {
			check = WAIT_UCODE_TIMEOUT;
		}

		reg = gk20a_readl(g, gr_fecs_ctxsw_mailbox_r(mailbox_id));

		if (mailbox_ret != NULL) {
			*mailbox_ret = reg;
		}

		switch (opc_success) {
		case GR_IS_UCODE_OP_EQUAL:
			if (reg == mailbox_ok) {
				check = WAIT_UCODE_OK;
			}
			break;
		case GR_IS_UCODE_OP_NOT_EQUAL:
			if (reg != mailbox_ok) {
				check = WAIT_UCODE_OK;
			}
			break;
		case GR_IS_UCODE_OP_AND:
			if ((reg & mailbox_ok) != 0U) {
				check = WAIT_UCODE_OK;
			}
			break;
		case GR_IS_UCODE_OP_LESSER:
			if (reg < mailbox_ok) {
				check = WAIT_UCODE_OK;
			}
			break;
		case GR_IS_UCODE_OP_LESSER_EQUAL:
			if (reg <= mailbox_ok) {
				check = WAIT_UCODE_OK;
			}
			break;
		case GR_IS_UCODE_OP_SKIP:
			/* do no success check */
			break;
		default:
			nvgpu_err(g,
				   "invalid success opcode 0x%x", opc_success);

			check = WAIT_UCODE_ERROR;
			break;
		}

		switch (opc_fail) {
		case GR_IS_UCODE_OP_EQUAL:
			if (reg == mailbox_fail) {
				check = WAIT_UCODE_ERROR;
			}
			break;
		case GR_IS_UCODE_OP_NOT_EQUAL:
			if (reg != mailbox_fail) {
				check = WAIT_UCODE_ERROR;
			}
			break;
		case GR_IS_UCODE_OP_AND:
			if ((reg & mailbox_fail) != 0U) {
				check = WAIT_UCODE_ERROR;
			}
			break;
		case GR_IS_UCODE_OP_LESSER:
			if (reg < mailbox_fail) {
				check = WAIT_UCODE_ERROR;
			}
			break;
		case GR_IS_UCODE_OP_LESSER_EQUAL:
			if (reg <= mailbox_fail) {
				check = WAIT_UCODE_ERROR;
			}
			break;
		case GR_IS_UCODE_OP_SKIP:
			/* do no check on fail*/
			break;
		default:
			nvgpu_err(g,
				   "invalid fail opcode 0x%x", opc_fail);
			check = WAIT_UCODE_ERROR;
			break;
		}

		if (sleepduringwait) {
			nvgpu_usleep_range(delay, delay * 2U);
			delay = min_t(u32, delay << 1, POLL_DELAY_MAX_US);
		} else {
			nvgpu_udelay(delay);
		}
	}

	if (check == WAIT_UCODE_TIMEOUT) {
		nvgpu_err(g,
			   "timeout waiting on mailbox=%d value=0x%08x",
			   mailbox_id, reg);
		g->ops.gr.falcon.dump_stats(g);
		gk20a_gr_debug_dump(g);
		return -1;
	} else if (check == WAIT_UCODE_ERROR) {
		nvgpu_err(g,
			   "ucode method failed on mailbox=%d value=0x%08x",
			   mailbox_id, reg);
		g->ops.gr.falcon.dump_stats(g);
		return -1;
	}

	nvgpu_log_fn(g, "done");
	return 0;
}

/* The following is a less brittle way to call gr_gk20a_submit_fecs_method(...)
 * We should replace most, if not all, fecs method calls to this instead. */
int gr_gk20a_submit_fecs_method_op(struct gk20a *g,
				   struct fecs_method_op_gk20a op,
				   bool sleepduringwait)
{
	struct gr_gk20a *gr = &g->gr;
	int ret;

	nvgpu_mutex_acquire(&gr->fecs_mutex);

	if (op.mailbox.id != 0U) {
		gk20a_writel(g, gr_fecs_ctxsw_mailbox_r(op.mailbox.id),
			     op.mailbox.data);
	}

	gk20a_writel(g, gr_fecs_ctxsw_mailbox_clear_r(0),
		gr_fecs_ctxsw_mailbox_clear_value_f(op.mailbox.clr));

	gk20a_writel(g, gr_fecs_method_data_r(), op.method.data);
	gk20a_writel(g, gr_fecs_method_push_r(),
		gr_fecs_method_push_adr_f(op.method.addr));

	/* op.mailbox.id == 4 cases require waiting for completion on
	 * for op.mailbox.id == 0 */
	if (op.mailbox.id == 4U) {
		op.mailbox.id = 0;
	}

	ret = gr_gk20a_ctx_wait_ucode(g, op.mailbox.id, op.mailbox.ret,
				      op.cond.ok, op.mailbox.ok,
				      op.cond.fail, op.mailbox.fail,
				      sleepduringwait);
	if (ret != 0) {
		nvgpu_err(g,"fecs method: data=0x%08x push adr=0x%08x",
			op.method.data, op.method.addr);
	}

	nvgpu_mutex_release(&gr->fecs_mutex);

	return ret;
}

/* Sideband mailbox writes are done a bit differently */
int gr_gk20a_submit_fecs_sideband_method_op(struct gk20a *g,
		struct fecs_method_op_gk20a op)
{
	struct gr_gk20a *gr = &g->gr;
	int ret;

	nvgpu_mutex_acquire(&gr->fecs_mutex);

	gk20a_writel(g, gr_fecs_ctxsw_mailbox_clear_r(op.mailbox.id),
		gr_fecs_ctxsw_mailbox_clear_value_f(op.mailbox.clr));

	gk20a_writel(g, gr_fecs_method_data_r(), op.method.data);
	gk20a_writel(g, gr_fecs_method_push_r(),
		gr_fecs_method_push_adr_f(op.method.addr));

	ret = gr_gk20a_ctx_wait_ucode(g, op.mailbox.id, op.mailbox.ret,
				      op.cond.ok, op.mailbox.ok,
				      op.cond.fail, op.mailbox.fail,
				      false);
	if (ret != 0) {
		nvgpu_err(g,"fecs method: data=0x%08x push adr=0x%08x",
			op.method.data, op.method.addr);
	}

	nvgpu_mutex_release(&gr->fecs_mutex);

	return ret;
}

static int gr_gk20a_ctrl_ctxsw(struct gk20a *g, u32 fecs_method, u32 *ret)
{
	return gr_gk20a_submit_fecs_method_op(g,
	      (struct fecs_method_op_gk20a) {
		      .method.addr = fecs_method,
		      .method.data = ~U32(0U),
		      .mailbox = { .id   = 1U, /*sideband?*/
				   .data = ~U32(0U), .clr = ~U32(0U), .ret = ret,
				   .ok   = gr_fecs_ctxsw_mailbox_value_pass_v(),
				   .fail = gr_fecs_ctxsw_mailbox_value_fail_v(), },
		      .cond.ok = GR_IS_UCODE_OP_EQUAL,
		      .cond.fail = GR_IS_UCODE_OP_EQUAL }, true);
}

/**
 * Stop processing (stall) context switches at FECS:-
 * If fecs is sent stop_ctxsw method, elpg entry/exit cannot happen
 * and may timeout. It could manifest as different error signatures
 * depending on when stop_ctxsw fecs method gets sent with respect
 * to pmu elpg sequence. It could come as pmu halt or abort or
 * maybe ext error too.
*/
int gr_gk20a_disable_ctxsw(struct gk20a *g)
{
	int err = 0;

	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg, " ");

	nvgpu_mutex_acquire(&g->ctxsw_disable_lock);
	g->ctxsw_disable_count++;
	if (g->ctxsw_disable_count == 1) {
		err = nvgpu_pg_elpg_disable(g);
		if (err != 0) {
			nvgpu_err(g, "failed to disable elpg. not safe to "
					"stop_ctxsw");
			/* stop ctxsw command is not sent */
			g->ctxsw_disable_count--;
		} else {
			err = gr_gk20a_ctrl_ctxsw(g,
				gr_fecs_method_push_adr_stop_ctxsw_v(), NULL);
			if (err != 0) {
				nvgpu_err(g, "failed to stop fecs ctxsw");
				/* stop ctxsw failed */
				g->ctxsw_disable_count--;
			}
		}
	} else {
		nvgpu_log_info(g, "ctxsw disabled, ctxsw_disable_count: %d",
			g->ctxsw_disable_count);
	}
	nvgpu_mutex_release(&g->ctxsw_disable_lock);

	return err;
}

/* Start processing (continue) context switches at FECS */
int gr_gk20a_enable_ctxsw(struct gk20a *g)
{
	int err = 0;

	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg, " ");

	nvgpu_mutex_acquire(&g->ctxsw_disable_lock);
	if (g->ctxsw_disable_count == 0) {
		goto ctxsw_already_enabled;
	}
	g->ctxsw_disable_count--;
	WARN_ON(g->ctxsw_disable_count < 0);
	if (g->ctxsw_disable_count == 0) {
		err = gr_gk20a_ctrl_ctxsw(g,
				gr_fecs_method_push_adr_start_ctxsw_v(), NULL);
		if (err != 0) {
			nvgpu_err(g, "failed to start fecs ctxsw");
		} else {
			if (nvgpu_pg_elpg_enable(g) != 0) {
				nvgpu_err(g, "failed to enable elpg "
					"after start_ctxsw");
			}
		}
	} else {
		nvgpu_log_info(g, "ctxsw_disable_count: %d is not 0 yet",
			g->ctxsw_disable_count);
	}
ctxsw_already_enabled:
	nvgpu_mutex_release(&g->ctxsw_disable_lock);

	return err;
}

int gr_gk20a_halt_pipe(struct gk20a *g)
{
	return gr_gk20a_submit_fecs_method_op(g,
	      (struct fecs_method_op_gk20a) {
		      .method.addr =
				gr_fecs_method_push_adr_halt_pipeline_v(),
		      .method.data = ~U32(0U),
		      .mailbox = { .id   = 1U, /*sideband?*/
				.data = ~U32(0U), .clr = ~U32(0U), .ret = NULL,
				.ok   = gr_fecs_ctxsw_mailbox_value_pass_v(),
				.fail = gr_fecs_ctxsw_mailbox_value_fail_v(), },
		      .cond.ok = GR_IS_UCODE_OP_EQUAL,
		      .cond.fail = GR_IS_UCODE_OP_EQUAL }, false);
}


int gr_gk20a_commit_inst(struct channel_gk20a *c, u64 gpu_va)
{
	u32 addr_lo;
	u32 addr_hi;

	nvgpu_log_fn(c->g, " ");

	addr_lo = u64_lo32(gpu_va) >> 12;
	addr_hi = u64_hi32(gpu_va);

	nvgpu_mem_wr32(c->g, &c->inst_block, ram_in_gr_wfi_target_w(),
		 ram_in_gr_cs_wfi_f() | ram_in_gr_wfi_mode_virtual_f() |
		 ram_in_gr_wfi_ptr_lo_f(addr_lo));

	nvgpu_mem_wr32(c->g, &c->inst_block, ram_in_gr_wfi_ptr_hi_w(),
		 ram_in_gr_wfi_ptr_hi_f(addr_hi));

	return 0;
}

static u32 fecs_current_ctx_data(struct gk20a *g, struct nvgpu_mem *inst_block)
{
	u64 ptr = nvgpu_inst_block_addr(g, inst_block) >>
		ram_in_base_shift_v();
	u32 aperture = nvgpu_aperture_mask(g, inst_block,
				gr_fecs_current_ctx_target_sys_mem_ncoh_f(),
				gr_fecs_current_ctx_target_sys_mem_coh_f(),
				gr_fecs_current_ctx_target_vid_mem_f());

	return gr_fecs_current_ctx_ptr_f(u64_lo32(ptr)) | aperture |
		gr_fecs_current_ctx_valid_f(1);
}

int gr_gk20a_fecs_ctx_bind_channel(struct gk20a *g,
					struct channel_gk20a *c)
{
	u32 inst_base_ptr = u64_lo32(nvgpu_inst_block_addr(g, &c->inst_block)
				     >> ram_in_base_shift_v());
	u32 data = fecs_current_ctx_data(g, &c->inst_block);
	int ret;

	nvgpu_log_info(g, "bind channel %d inst ptr 0x%08x",
		   c->chid, inst_base_ptr);

	ret = gr_gk20a_submit_fecs_method_op(g,
		     (struct fecs_method_op_gk20a) {
		     .method.addr = gr_fecs_method_push_adr_bind_pointer_v(),
		     .method.data = data,
		     .mailbox = { .id = 0, .data = 0,
				  .clr = 0x30,
				  .ret = NULL,
				  .ok = 0x10,
				  .fail = 0x20, },
		     .cond.ok = GR_IS_UCODE_OP_AND,
		     .cond.fail = GR_IS_UCODE_OP_AND}, true);
	if (ret != 0) {
		nvgpu_err(g,
			"bind channel instance failed");
	}

	return ret;
}

int gr_gk20a_commit_global_ctx_buffers(struct gk20a *g,
			struct nvgpu_gr_ctx *gr_ctx, bool patch)
{
	struct gr_gk20a *gr = &g->gr;
	u64 addr;
	u32 size;

	nvgpu_log_fn(g, " ");

	if (patch) {
		int err;
		err = nvgpu_gr_ctx_patch_write_begin(g, gr_ctx, false);
		if (err != 0) {
			return err;
		}
	}

	/* global pagepool buffer */
	addr = nvgpu_gr_ctx_get_global_ctx_va(gr_ctx, NVGPU_GR_CTX_PAGEPOOL_VA);
	size = (u32)nvgpu_gr_global_ctx_get_size(gr->global_ctx_buffer,
			NVGPU_GR_GLOBAL_CTX_PAGEPOOL);

	g->ops.gr.init.commit_global_pagepool(g, gr_ctx, addr, size, patch,
		true);

	/* global bundle cb */
	addr = nvgpu_gr_ctx_get_global_ctx_va(gr_ctx, NVGPU_GR_CTX_CIRCULAR_VA);
	size = g->ops.gr.init.get_bundle_cb_default_size(g);

	g->ops.gr.init.commit_global_bundle_cb(g, gr_ctx, addr, size, patch);

	/* global attrib cb */
	addr = nvgpu_gr_ctx_get_global_ctx_va(gr_ctx,
			NVGPU_GR_CTX_ATTRIBUTE_VA);

	g->ops.gr.init.commit_global_attrib_cb(g, gr_ctx,
		nvgpu_gr_config_get_tpc_count(g->gr.config),
		nvgpu_gr_config_get_max_tpc_count(g->gr.config), addr, patch);

	g->ops.gr.init.commit_global_cb_manager(g, g->gr.config, gr_ctx, patch);

	if (g->ops.gr.init.commit_rtv_cb != NULL) {
		/* RTV circular buffer */
		addr = nvgpu_gr_ctx_get_global_ctx_va(gr_ctx,
			NVGPU_GR_CTX_RTV_CIRCULAR_BUFFER_VA);

		g->ops.gr.init.commit_rtv_cb(g, addr, gr_ctx, patch);
	}

	if (patch) {
		nvgpu_gr_ctx_patch_write_end(g, gr_ctx, false);
	}

	return 0;
}

int gr_gk20a_fecs_ctx_image_save(struct channel_gk20a *c, u32 save_type)
{
	struct gk20a *g = c->g;
	int ret;

	nvgpu_log_fn(g, " ");

	ret = gr_gk20a_submit_fecs_method_op(g,
		(struct fecs_method_op_gk20a) {
		.method.addr = save_type,
		.method.data = fecs_current_ctx_data(g, &c->inst_block),
		.mailbox = {.id = 0, .data = 0, .clr = 3, .ret = NULL,
			.ok = 1, .fail = 2,
		},
		.cond.ok = GR_IS_UCODE_OP_AND,
		.cond.fail = GR_IS_UCODE_OP_AND,
		 }, true);

	if (ret != 0) {
		nvgpu_err(g, "save context image failed");
	}

	return ret;
}

int gk20a_init_sw_bundle(struct gk20a *g)
{
	struct netlist_av_list *sw_bundle_init =
			&g->netlist_vars->sw_bundle_init;
	struct netlist_av_list *sw_veid_bundle_init =
			&g->netlist_vars->sw_veid_bundle_init;
	struct netlist_av64_list *sw_bundle64_init =
			&g->netlist_vars->sw_bundle64_init;
	int err = 0;

	/* enable pipe mode override */
	g->ops.gr.init.pipe_mode_override(g, true);

	/* load bundle init */
	err = g->ops.gr.init.load_sw_bundle_init(g, sw_bundle_init);
	if (err != 0) {
		goto error;
	}

	if (g->ops.gr.init.load_sw_veid_bundle != NULL) {
		err = g->ops.gr.init.load_sw_veid_bundle(g,
				sw_veid_bundle_init);
		if (err != 0) {
			goto error;
		}
	}

	if (g->ops.gr.init.load_sw_bundle64 != NULL) {
		err = g->ops.gr.init.load_sw_bundle64(g, sw_bundle64_init);
		if (err != 0) {
			goto error;
		}
	}

	/* disable pipe mode override */
	g->ops.gr.init.pipe_mode_override(g, false);

	err = g->ops.gr.init.wait_idle(g);

	return err;

error:
	/* in case of error skip waiting for GR idle - just restore state */
	g->ops.gr.init.pipe_mode_override(g, false);

	return err;
}

/* init global golden image from a fresh gr_ctx in channel ctx.
   save a copy in local_golden_image in ctx_vars */
int gr_gk20a_init_golden_ctx_image(struct gk20a *g,
					  struct channel_gk20a *c,
					  struct nvgpu_gr_ctx *gr_ctx)
{
	struct gr_gk20a *gr = &g->gr;
	u32 i;
	struct nvgpu_mem *gr_mem;
	int err = 0;
	struct netlist_aiv_list *sw_ctx_load = &g->netlist_vars->sw_ctx_load;
	struct netlist_av_list *sw_method_init = &g->netlist_vars->sw_method_init;

	nvgpu_log_fn(g, " ");

	gr_mem = &gr_ctx->mem;

	/* golden ctx is global to all channels. Although only the first
	   channel initializes golden image, driver needs to prevent multiple
	   channels from initializing golden ctx at the same time */
	nvgpu_mutex_acquire(&gr->ctx_mutex);

	if (gr->ctx_vars.golden_image_initialized) {
		goto clean_up;
	}

	err = g->ops.gr.init.fe_pwr_mode_force_on(g, true);
	if (err != 0) {
		goto clean_up;
	}

	g->ops.gr.init.override_context_reset(g);

	err = g->ops.gr.init.fe_pwr_mode_force_on(g, false);
	if (err != 0) {
		goto clean_up;
	}

	err = gr_gk20a_fecs_ctx_bind_channel(g, c);
	if (err != 0) {
		goto clean_up;
	}

	err = g->ops.gr.init.wait_idle(g);

	/* load ctx init */
	for (i = 0; i < sw_ctx_load->count; i++) {
		gk20a_writel(g, sw_ctx_load->l[i].addr,
			     sw_ctx_load->l[i].value);
	}

	if (g->ops.gr.init.preemption_state != NULL) {
		err = g->ops.gr.init.preemption_state(g,
			gr->gfxp_wfi_timeout_count,
			gr->gfxp_wfi_timeout_unit_usec);
		if (err != 0) {
			goto clean_up;
		}
	}

	nvgpu_cg_blcg_gr_load_enable(g);

	err = g->ops.gr.init.wait_idle(g);
	if (err != 0) {
		goto clean_up;
	}

	/* disable fe_go_idle */
	g->ops.gr.init.fe_go_idle_timeout(g, false);

	err = g->ops.gr.commit_global_ctx_buffers(g, gr_ctx, false);
	if (err != 0) {
		goto clean_up;
	}

	/* override a few ctx state registers */
	g->ops.gr.init.commit_global_timeslice(g);

	/* floorsweep anything left */
	err = nvgpu_gr_init_fs_state(g);
	if (err != 0) {
		goto clean_up;
	}

	err = g->ops.gr.init.wait_idle(g);
	if (err != 0) {
		goto restore_fe_go_idle;
	}

	err = gk20a_init_sw_bundle(g);
	if (err != 0) {
		goto clean_up;
	}

restore_fe_go_idle:
	/* restore fe_go_idle */
	g->ops.gr.init.fe_go_idle_timeout(g, true);

	if ((err != 0) || (g->ops.gr.init.wait_idle(g) != 0)) {
		goto clean_up;
	}

	/* load method init */
	g->ops.gr.init.load_method_init(g, sw_method_init);

	err = g->ops.gr.init.wait_idle(g);
	if (err != 0) {
		goto clean_up;
	}

	err = nvgpu_gr_ctx_init_zcull(g, gr_ctx);
	if (err != 0) {
		goto clean_up;
	}

	gr_gk20a_fecs_ctx_image_save(c, gr_fecs_method_push_adr_wfi_golden_save_v());

	gr->local_golden_image =
		nvgpu_gr_global_ctx_init_local_golden_image(g, gr_mem,
			gr->ctx_vars.golden_image_size);
	if (gr->local_golden_image == NULL) {
		err = -ENOMEM;
		goto clean_up;
	}

	gr->ctx_vars.golden_image_initialized = true;

	g->ops.gr.falcon.set_current_ctx_invalid(g);

clean_up:
	if (err != 0) {
		nvgpu_err(g, "fail");
	} else {
		nvgpu_log_fn(g, "done");
	}

	nvgpu_mutex_release(&gr->ctx_mutex);
	return err;
}

int gr_gk20a_update_smpc_ctxsw_mode(struct gk20a *g,
				    struct channel_gk20a *c,
				    bool enable_smpc_ctxsw)
{
	struct tsg_gk20a *tsg;
	int ret;

	nvgpu_log_fn(g, " ");

	tsg = tsg_gk20a_from_ch(c);
	if (tsg == NULL) {
		return -EINVAL;
	}

	ret = gk20a_disable_channel_tsg(g, c);
	if (ret != 0) {
		nvgpu_err(g, "failed to disable channel/TSG");
		goto out;
	}
	ret = gk20a_fifo_preempt(g, c);
	if (ret != 0) {
		gk20a_enable_channel_tsg(g, c);
		nvgpu_err(g, "failed to preempt channel/TSG");
		goto out;
	}

	ret = nvgpu_gr_ctx_set_smpc_mode(g, tsg->gr_ctx, enable_smpc_ctxsw);

out:
	gk20a_enable_channel_tsg(g, c);
	return ret;
}

int gr_gk20a_update_hwpm_ctxsw_mode(struct gk20a *g,
				  struct channel_gk20a *c,
				  u64 gpu_va,
				  u32 mode)
{
	struct tsg_gk20a *tsg;
	struct nvgpu_gr_ctx *gr_ctx;
	bool skip_update = false;
	int ret;

	nvgpu_log_fn(g, " ");

	tsg = tsg_gk20a_from_ch(c);
	if (tsg == NULL) {
		return -EINVAL;
	}

	gr_ctx = tsg->gr_ctx;

	if (mode != NVGPU_GR_CTX_HWPM_CTXSW_MODE_NO_CTXSW) {
		nvgpu_gr_ctx_set_size(g->gr.gr_ctx_desc,
			NVGPU_GR_CTX_PM_CTX,
			g->gr.ctx_vars.pm_ctxsw_image_size);

		ret = nvgpu_gr_ctx_alloc_pm_ctx(g, gr_ctx,
			g->gr.gr_ctx_desc, c->vm,
			gpu_va);
		if (ret != 0) {
			nvgpu_err(g,
				"failed to allocate pm ctxt buffer");
			return ret;
		}

		if ((mode == NVGPU_GR_CTX_HWPM_CTXSW_MODE_STREAM_OUT_CTXSW) &&
			(g->ops.gr.init_hwpm_pmm_register != NULL)) {
			g->ops.gr.init_hwpm_pmm_register(g);
		}
	}

	ret = nvgpu_gr_ctx_prepare_hwpm_mode(g, gr_ctx, mode, &skip_update);
	if (ret != 0) {
		return ret;
	}
	if (skip_update) {
		return 0;
	}

	ret = gk20a_disable_channel_tsg(g, c);
	if (ret != 0) {
		nvgpu_err(g, "failed to disable channel/TSG");
		return ret;
	}

	ret = gk20a_fifo_preempt(g, c);
	if (ret != 0) {
		gk20a_enable_channel_tsg(g, c);
		nvgpu_err(g, "failed to preempt channel/TSG");
		return ret;
	}

	if (c->subctx != NULL) {
		struct channel_gk20a *ch;

		nvgpu_rwsem_down_read(&tsg->ch_list_lock);
		nvgpu_list_for_each_entry(ch, &tsg->ch_list, channel_gk20a, ch_entry) {
			ret = nvgpu_gr_ctx_set_hwpm_mode(g, gr_ctx, false);
			if (ret == 0) {
				nvgpu_gr_subctx_set_hwpm_mode(g, ch->subctx,
					gr_ctx);
			}
		}
		nvgpu_rwsem_up_read(&tsg->ch_list_lock);
	} else {
		ret = nvgpu_gr_ctx_set_hwpm_mode(g, gr_ctx, true);
	}

	/* enable channel */
	gk20a_enable_channel_tsg(g, c);

	return ret;
}

static int gr_gk20a_wait_ctxsw_ready(struct gk20a *g)
{
	int ret;

	nvgpu_log_fn(g, " ");

	ret = gr_gk20a_ctx_wait_ucode(g, 0, NULL,
				      GR_IS_UCODE_OP_EQUAL,
				      eUcodeHandshakeInitComplete,
				      GR_IS_UCODE_OP_SKIP, 0, false);
	if (ret != 0) {
		nvgpu_err(g, "falcon ucode init timeout");
		return ret;
	}

	if (nvgpu_is_enabled(g, NVGPU_GR_USE_DMA_FOR_FW_BOOTSTRAP) ||
		nvgpu_is_enabled(g, NVGPU_SEC_SECUREGPCCS)) {
		gk20a_writel(g, gr_fecs_current_ctx_r(),
			gr_fecs_current_ctx_valid_false_f());
	}

	gk20a_writel(g, gr_fecs_ctxsw_mailbox_clear_r(0), 0xffffffffU);
	gk20a_writel(g, gr_fecs_method_data_r(), 0x7fffffff);
	gk20a_writel(g, gr_fecs_method_push_r(),
		     gr_fecs_method_push_adr_set_watchdog_timeout_f());

	nvgpu_log_fn(g, "done");
	return 0;
}

int gr_gk20a_init_ctx_state(struct gk20a *g)
{
	int ret;
	struct fecs_method_op_gk20a op = {
		.mailbox = { .id = 0U, .data = 0U,
			     .clr = ~U32(0U), .ok = 0U, .fail = 0U},
		.method.data = 0U,
		.cond.ok = GR_IS_UCODE_OP_NOT_EQUAL,
		.cond.fail = GR_IS_UCODE_OP_SKIP,
		};

	nvgpu_log_fn(g, " ");
	/* query ctxsw image sizes, if golden context is not created */
	if (!g->gr.ctx_vars.golden_image_initialized) {
		op.method.addr =
			gr_fecs_method_push_adr_discover_image_size_v();
		op.mailbox.ret = &g->gr.ctx_vars.golden_image_size;
		ret = gr_gk20a_submit_fecs_method_op(g, op, false);
		if (ret != 0) {
			nvgpu_err(g,
				   "query golden image size failed");
			return ret;
		}

		op.method.addr =
			gr_fecs_method_push_adr_discover_pm_image_size_v();
		op.mailbox.ret = &g->gr.ctx_vars.pm_ctxsw_image_size;
		ret = gr_gk20a_submit_fecs_method_op(g, op, false);
		if (ret != 0) {
			nvgpu_err(g,
				   "query pm ctx image size failed");
			return ret;
		}

		if (g->gr.ctx_vars.pm_ctxsw_image_size != 0U) {
			ret = nvgpu_gr_hwpm_map_init(g, &g->gr.hwpm_map,
				g->gr.ctx_vars.pm_ctxsw_image_size);
			if (ret != 0) {
				nvgpu_err(g,
				   "hwpm_map init failed");
				return ret;
			}
		}

		g->gr.ctx_vars.priv_access_map_size = 512 * 1024;
#ifdef CONFIG_GK20A_CTXSW_TRACE
		g->gr.ctx_vars.fecs_trace_buffer_size =
			nvgpu_gr_fecs_trace_buffer_size(g);
#endif
	}

	nvgpu_log_fn(g, "done");
	return 0;
}

int gr_gk20a_alloc_global_ctx_buffers(struct gk20a *g)
{
	struct gr_gk20a *gr = &g->gr;
	int err;
	u32 size;

	nvgpu_log_fn(g, " ");

	size = g->ops.gr.init.get_global_ctx_cb_buffer_size(g);
	nvgpu_log_info(g, "cb_buffer_size : %d", size);

	nvgpu_gr_global_ctx_set_size(gr->global_ctx_buffer,
		NVGPU_GR_GLOBAL_CTX_CIRCULAR, size);
	nvgpu_gr_global_ctx_set_size(gr->global_ctx_buffer,
		NVGPU_GR_GLOBAL_CTX_CIRCULAR_VPR, size);

	size = g->ops.gr.init.get_global_ctx_pagepool_buffer_size(g);
	nvgpu_log_info(g, "pagepool_buffer_size : %d", size);

	nvgpu_gr_global_ctx_set_size(gr->global_ctx_buffer,
		NVGPU_GR_GLOBAL_CTX_PAGEPOOL, size);
	nvgpu_gr_global_ctx_set_size(gr->global_ctx_buffer,
		NVGPU_GR_GLOBAL_CTX_PAGEPOOL_VPR, size);

	size = g->ops.gr.init.get_global_attr_cb_size(g,
			nvgpu_gr_config_get_tpc_count(g->gr.config),
			nvgpu_gr_config_get_max_tpc_count(g->gr.config));
	nvgpu_log_info(g, "attr_buffer_size : %u", size);

	nvgpu_gr_global_ctx_set_size(gr->global_ctx_buffer,
		NVGPU_GR_GLOBAL_CTX_ATTRIBUTE, size);
	nvgpu_gr_global_ctx_set_size(gr->global_ctx_buffer,
		NVGPU_GR_GLOBAL_CTX_ATTRIBUTE_VPR, size);

	nvgpu_log_info(g, "priv_access_map_size : %d",
		   gr->ctx_vars.priv_access_map_size);

	nvgpu_gr_global_ctx_set_size(gr->global_ctx_buffer,
		NVGPU_GR_GLOBAL_CTX_PRIV_ACCESS_MAP,
		gr->ctx_vars.priv_access_map_size);

#ifdef CONFIG_GK20A_CTXSW_TRACE
	nvgpu_log_info(g, "fecs_trace_buffer_size : %d",
		   gr->ctx_vars.fecs_trace_buffer_size);

	nvgpu_gr_global_ctx_set_size(gr->global_ctx_buffer,
		NVGPU_GR_GLOBAL_CTX_FECS_TRACE_BUFFER,
		gr->ctx_vars.fecs_trace_buffer_size);
#endif

	if (g->ops.gr.init.get_rtv_cb_size != NULL) {
		size = g->ops.gr.init.get_rtv_cb_size(g);
		nvgpu_log_info(g, "rtv_circular_buffer_size : %u", size);

		nvgpu_gr_global_ctx_set_size(gr->global_ctx_buffer,
			NVGPU_GR_GLOBAL_CTX_RTV_CIRCULAR_BUFFER, size);
	}

	err = nvgpu_gr_global_ctx_buffer_alloc(g, gr->global_ctx_buffer);
	if (err != 0) {
		return err;
	}

	nvgpu_log_fn(g, "done");
	return 0;
}

static int gr_gk20a_alloc_gr_ctx(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx, struct vm_gk20a *vm)
{
	struct gr_gk20a *gr = &g->gr;
	int err = 0;

	nvgpu_log_fn(g, " ");

	nvgpu_gr_ctx_set_size(gr->gr_ctx_desc, NVGPU_GR_CTX_CTX,
		gr->ctx_vars.golden_image_size);

	err = nvgpu_gr_ctx_alloc(g, gr_ctx, gr->gr_ctx_desc, vm);
	if (err != 0) {
		return err;
	}

	return 0;
}

void gr_gk20a_free_gr_ctx(struct gk20a *g,
			  struct vm_gk20a *vm, struct nvgpu_gr_ctx *gr_ctx)
{
	nvgpu_log_fn(g, " ");

	if (gr_ctx != NULL) {
		if ((g->ops.gr.ctxsw_prog.dump_ctxsw_stats != NULL) &&
		     g->gr.ctx_vars.dump_ctxsw_stats_on_channel_close) {
			g->ops.gr.ctxsw_prog.dump_ctxsw_stats(g, &gr_ctx->mem);
		}

		nvgpu_gr_ctx_free(g, gr_ctx, g->gr.global_ctx_buffer, vm);
	}
}

void gr_gk20a_free_tsg_gr_ctx(struct tsg_gk20a *tsg)
{
	struct gk20a *g = tsg->g;

	if (tsg->vm == NULL) {
		nvgpu_err(g, "No address space bound");
		return;
	}
	tsg->g->ops.gr.free_gr_ctx(g, tsg->vm, tsg->gr_ctx);
}

u32 gr_gk20a_get_patch_slots(struct gk20a *g)
{
	return PATCH_CTX_SLOTS_PER_PAGE;
}

int gk20a_alloc_obj_ctx(struct channel_gk20a  *c, u32 class_num, u32 flags)
{
	struct gk20a *g = c->g;
	struct nvgpu_gr_ctx *gr_ctx;
	struct tsg_gk20a *tsg = NULL;
	int err = 0;

	nvgpu_log_fn(g, " ");

	/* an address space needs to have been bound at this point.*/
	if (!gk20a_channel_as_bound(c) && (c->vm == NULL)) {
		nvgpu_err(g,
			   "not bound to address space at time"
			   " of grctx allocation");
		return -EINVAL;
	}

	if (!g->ops.gr.is_valid_class(g, class_num)) {
		nvgpu_err(g,
			   "invalid obj class 0x%x", class_num);
		err = -EINVAL;
		goto out;
	}
	c->obj_class = class_num;

	tsg = tsg_gk20a_from_ch(c);
	if (tsg == NULL) {
		return -EINVAL;
	}

	gr_ctx = tsg->gr_ctx;

	if (!nvgpu_mem_is_valid(&gr_ctx->mem)) {
		tsg->vm = c->vm;
		nvgpu_vm_get(tsg->vm);
		err = gr_gk20a_alloc_gr_ctx(g, gr_ctx, tsg->vm);
		if (err != 0) {
			nvgpu_err(g,
				"fail to allocate TSG gr ctx buffer");
			nvgpu_vm_put(tsg->vm);
			tsg->vm = NULL;
			goto out;
		}

		gr_ctx->tsgid = tsg->tsgid;

		/* allocate patch buffer */
		if (!nvgpu_mem_is_valid(&gr_ctx->patch_ctx.mem)) {
			gr_ctx->patch_ctx.data_count = 0;

			nvgpu_gr_ctx_set_size(g->gr.gr_ctx_desc,
				NVGPU_GR_CTX_PATCH_CTX,
				g->ops.gr.get_patch_slots(g) *
					PATCH_CTX_SLOTS_REQUIRED_PER_ENTRY);

			err = nvgpu_gr_ctx_alloc_patch_ctx(g, gr_ctx,
				g->gr.gr_ctx_desc, c->vm);
			if (err != 0) {
				nvgpu_err(g,
					"fail to allocate patch buffer");
				goto out;
			}
		}

		g->ops.gr.init_ctxsw_preemption_mode(g, gr_ctx, tsg->vm,
			class_num, flags);

		/* map global buffer to channel gpu_va and commit */
		err = nvgpu_gr_ctx_map_global_ctx_buffers(g, gr_ctx,
				g->gr.global_ctx_buffer, tsg->vm, c->vpr);
		if (err != 0) {
			nvgpu_err(g,
				"fail to map global ctx buffer");
			goto out;
		}
		g->ops.gr.commit_global_ctx_buffers(g, gr_ctx, true);

		/* commit gr ctx buffer */
		err = g->ops.gr.commit_inst(c, gr_ctx->mem.gpu_va);
		if (err != 0) {
			nvgpu_err(g,
				"fail to commit gr ctx buffer");
			goto out;
		}

		/* init golden image, ELPG enabled after this is done */
		err = gr_gk20a_init_golden_ctx_image(g, c, gr_ctx);
		if (err != 0) {
			nvgpu_err(g,
				"fail to init golden ctx image");
			goto out;
		}

		/* load golden image */
		nvgpu_gr_ctx_load_golden_ctx_image(g, gr_ctx,
			g->gr.local_golden_image, c->cde);
		if (err != 0) {
			nvgpu_err(g,
				"fail to load golden ctx image");
			goto out;
		}

		if (g->ops.gr.update_ctxsw_preemption_mode != NULL) {
			g->ops.gr.update_ctxsw_preemption_mode(g, gr_ctx,
				c->subctx);
		}

#ifdef CONFIG_GK20A_CTXSW_TRACE
		if (g->ops.gr.fecs_trace.bind_channel && !c->vpr) {
			err = g->ops.gr.fecs_trace.bind_channel(g, &c->inst_block,
				c->subctx, gr_ctx, tsg->tgid, 0);
			if (err != 0) {
				nvgpu_warn(g,
					"fail to bind channel for ctxsw trace");
			}
		}
#endif
	} else {
		/* commit gr ctx buffer */
		err = g->ops.gr.commit_inst(c, gr_ctx->mem.gpu_va);
		if (err != 0) {
			nvgpu_err(g,
				"fail to commit gr ctx buffer");
			goto out;
		}
#ifdef CONFIG_GK20A_CTXSW_TRACE
		if (g->ops.gr.fecs_trace.bind_channel && !c->vpr) {
			err = g->ops.gr.fecs_trace.bind_channel(g, &c->inst_block,
				c->subctx, gr_ctx, tsg->tgid, 0);
			if (err != 0) {
				nvgpu_warn(g,
					"fail to bind channel for ctxsw trace");
			}
		}
#endif
	}

	nvgpu_log_fn(g, "done");
	return 0;
out:
	/* 1. gr_ctx, patch_ctx and global ctx buffer mapping
	   can be reused so no need to release them.
	   2. golden image init and load is a one time thing so if
	   they pass, no need to undo. */
	nvgpu_err(g, "fail");
	return err;
}

static void gk20a_remove_gr_support(struct gr_gk20a *gr)
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

	if (gr->local_golden_image != NULL) {
		nvgpu_gr_global_ctx_deinit_local_golden_image(g,
			gr->local_golden_image);
		gr->local_golden_image = NULL;
		gr->ctx_vars.golden_image_initialized = false;
	}

	nvgpu_gr_hwpm_map_deinit(g, gr->hwpm_map);

	nvgpu_ecc_remove_support(g);
	nvgpu_gr_zbc_deinit(g, gr->zbc);
	nvgpu_gr_zcull_deinit(g, gr->zcull);
}

static int gr_gk20a_init_gr_config(struct gk20a *g, struct gr_gk20a *gr)
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

void gr_gk20a_fecs_host_int_enable(struct gk20a *g)
{
	gk20a_writel(g, gr_fecs_host_int_enable_r(),
		     gr_fecs_host_int_enable_ctxsw_intr1_enable_f() |
		     gr_fecs_host_int_enable_fault_during_ctxsw_enable_f() |
		     gr_fecs_host_int_enable_umimp_firmware_method_enable_f() |
		     gr_fecs_host_int_enable_umimp_illegal_method_enable_f() |
		     gr_fecs_host_int_enable_watchdog_enable_f());
}

static int gk20a_init_gr_setup_hw(struct gk20a *g)
{
	struct gr_gk20a *gr = &g->gr;
	int err;

	nvgpu_log_fn(g, " ");

	if (g->ops.gr.init.gpc_mmu != NULL) {
		g->ops.gr.init.gpc_mmu(g);
	}

	/* load gr floorsweeping registers */
	g->ops.gr.init.pes_vsc_stream(g);

	nvgpu_gr_zcull_init_hw(g, gr->zcull, gr->config);

	if (g->ops.priv_ring.set_ppriv_timeout_settings != NULL) {
		g->ops.priv_ring.set_ppriv_timeout_settings(g);
	}

	/* enable fifo access */
	g->ops.gr.init.fifo_access(g, true);

	/* TBD: reload gr ucode when needed */

	/* enable interrupts */
	g->ops.gr.intr.enable_interrupts(g, true);

	/* enable fecs error interrupts */
	g->ops.gr.fecs_host_int_enable(g);

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
	err = nvgpu_gr_init_fs_state(g);
	if (err != 0) {
		goto out;
	}

	err = g->ops.gr.init.wait_idle(g);
out:
	nvgpu_log_fn(g, "done");
	return err;
}

static void gk20a_init_gr_prepare(struct gk20a *g)
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

static int gr_gk20a_init_ctxsw(struct gk20a *g)
{
	int err = 0;

	err = g->ops.gr.falcon.load_ctxsw_ucode(g);
	if (err != 0) {
		goto out;
	}

	err = gr_gk20a_wait_ctxsw_ready(g);
	if (err != 0) {
		goto out;
	}

out:
	if (err != 0) {
		nvgpu_err(g, "fail");
	} else {
		nvgpu_log_fn(g, "done");
	}

	return err;
}

static int gk20a_init_gr_reset_enable_hw(struct gk20a *g)
{
	struct netlist_av_list *sw_non_ctx_load = &g->netlist_vars->sw_non_ctx_load;
	u32 i;
	int err = 0;

	nvgpu_log_fn(g, " ");

	/* enable interrupts */
	g->ops.gr.intr.enable_interrupts(g, true);

	/* load non_ctx init */
	for (i = 0; i < sw_non_ctx_load->count; i++) {
		gk20a_writel(g, sw_non_ctx_load->l[i].addr,
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

static int gr_gk20a_init_access_map(struct gk20a *g)
{
	struct gr_gk20a *gr = &g->gr;
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

static int gk20a_init_gr_setup_sw(struct gk20a *g)
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

	err = gr_gk20a_init_gr_config(g, gr);
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

	err = gr_gk20a_init_access_map(g);
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

	gr->remove_support = gk20a_remove_gr_support;
	gr->sw_ready = true;

	err = nvgpu_ecc_init_support(g);
	if (err != 0) {
		goto clean_up;
	}

	nvgpu_log_fn(g, "done");
	return 0;

clean_up:
	nvgpu_err(g, "fail");
	gk20a_remove_gr_support(gr);
	return err;
}

static int gk20a_init_gr_bind_fecs_elpg(struct gk20a *g)
{
	struct nvgpu_pmu *pmu = &g->pmu;
	struct mm_gk20a *mm = &g->mm;
	struct vm_gk20a *vm = mm->pmu.vm;
	int err = 0;

	u32 size;

	nvgpu_log_fn(g, " ");

	size = 0;

	err = gr_gk20a_fecs_get_reglist_img_size(g, &size);
	if (err != 0) {
		nvgpu_err(g,
			"fail to query fecs pg buffer size");
		return err;
	}

	if (pmu->pmu_pg.pg_buf.cpu_va == NULL) {
		err = nvgpu_dma_alloc_map_sys(vm, size, &pmu->pmu_pg.pg_buf);
		if (err != 0) {
			nvgpu_err(g, "failed to allocate memory");
			return -ENOMEM;
		}
	}


	err = gr_gk20a_fecs_set_reglist_bind_inst(g, &mm->pmu.inst_block);
	if (err != 0) {
		nvgpu_err(g,
			"fail to bind pmu inst to gr");
		return err;
	}

	err = gr_gk20a_fecs_set_reglist_virtual_addr(g, pmu->pmu_pg.pg_buf.gpu_va);
	if (err != 0) {
		nvgpu_err(g,
			"fail to set pg buffer pmu va");
		return err;
	}

	return err;
}

int gk20a_init_gr_support(struct gk20a *g)
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

	err = gr_gk20a_init_ctxsw(g);
	if (err != 0) {
		return err;
	}

	/* this appears query for sw states but fecs actually init
	   ramchain, etc so this is hw init */
	err = g->ops.gr.init_ctx_state(g);
	if (err != 0) {
		return err;
	}

	if (g->can_elpg) {
		err = gk20a_init_gr_bind_fecs_elpg(g);
		if (err != 0) {
			return err;
		}
	}

	err = gk20a_init_gr_setup_sw(g);
	if (err != 0) {
		return err;
	}

	err = gk20a_init_gr_setup_hw(g);
	if (err != 0) {
		return err;
	}

	nvgpu_cg_elcg_enable_no_wait(g);

	/* GR is inialized, signal possible waiters */
	g->gr.initialized = true;
	nvgpu_cond_signal(&g->gr.init_wq);

	return 0;
}

#define NVA297_SET_ALPHA_CIRCULAR_BUFFER_SIZE	0x02dcU
#define NVA297_SET_CIRCULAR_BUFFER_SIZE		0x1280U
#define NVA297_SET_SHADER_EXCEPTIONS		0x1528U
#define NVA0C0_SET_SHADER_EXCEPTIONS		0x1528U

#define NVA297_SET_SHADER_EXCEPTIONS_ENABLE_FALSE	U32(0)

void gk20a_gr_set_shader_exceptions(struct gk20a *g, u32 data)
{
	nvgpu_log_fn(g, " ");

	if (data == NVA297_SET_SHADER_EXCEPTIONS_ENABLE_FALSE) {
		gk20a_writel(g,
			gr_gpcs_tpcs_sm_hww_warp_esr_report_mask_r(), 0);
		gk20a_writel(g,
			gr_gpcs_tpcs_sm_hww_global_esr_report_mask_r(), 0);
	} else {
		/* setup sm warp esr report masks */
		gk20a_writel(g, gr_gpcs_tpcs_sm_hww_warp_esr_report_mask_r(),
			gr_gpcs_tpcs_sm_hww_warp_esr_report_mask_stack_error_report_f()	|
			gr_gpcs_tpcs_sm_hww_warp_esr_report_mask_api_stack_error_report_f() |
			gr_gpcs_tpcs_sm_hww_warp_esr_report_mask_ret_empty_stack_error_report_f() |
			gr_gpcs_tpcs_sm_hww_warp_esr_report_mask_pc_wrap_report_f() |
			gr_gpcs_tpcs_sm_hww_warp_esr_report_mask_misaligned_pc_report_f() |
			gr_gpcs_tpcs_sm_hww_warp_esr_report_mask_pc_overflow_report_f() |
			gr_gpcs_tpcs_sm_hww_warp_esr_report_mask_misaligned_immc_addr_report_f() |
			gr_gpcs_tpcs_sm_hww_warp_esr_report_mask_misaligned_reg_report_f() |
			gr_gpcs_tpcs_sm_hww_warp_esr_report_mask_illegal_instr_encoding_report_f() |
			gr_gpcs_tpcs_sm_hww_warp_esr_report_mask_illegal_sph_instr_combo_report_f() |
			gr_gpcs_tpcs_sm_hww_warp_esr_report_mask_illegal_instr_param_report_f() |
			gr_gpcs_tpcs_sm_hww_warp_esr_report_mask_invalid_const_addr_report_f() |
			gr_gpcs_tpcs_sm_hww_warp_esr_report_mask_oor_reg_report_f() |
			gr_gpcs_tpcs_sm_hww_warp_esr_report_mask_oor_addr_report_f() |
			gr_gpcs_tpcs_sm_hww_warp_esr_report_mask_misaligned_addr_report_f() |
			gr_gpcs_tpcs_sm_hww_warp_esr_report_mask_invalid_addr_space_report_f() |
			gr_gpcs_tpcs_sm_hww_warp_esr_report_mask_illegal_instr_param2_report_f() |
			gr_gpcs_tpcs_sm_hww_warp_esr_report_mask_invalid_const_addr_ldc_report_f() |
			gr_gpcs_tpcs_sm_hww_warp_esr_report_mask_geometry_sm_error_report_f() |
			gr_gpcs_tpcs_sm_hww_warp_esr_report_mask_divergent_report_f());

		/* setup sm global esr report mask */
		gk20a_writel(g, gr_gpcs_tpcs_sm_hww_global_esr_report_mask_r(),
			gr_gpcs_tpcs_sm_hww_global_esr_report_mask_sm_to_sm_fault_report_f() |
			gr_gpcs_tpcs_sm_hww_global_esr_report_mask_l1_error_report_f() |
			gr_gpcs_tpcs_sm_hww_global_esr_report_mask_multiple_warp_errors_report_f() |
			gr_gpcs_tpcs_sm_hww_global_esr_report_mask_physical_stack_overflow_error_report_f() |
			gr_gpcs_tpcs_sm_hww_global_esr_report_mask_bpt_int_report_f() |
			gr_gpcs_tpcs_sm_hww_global_esr_report_mask_bpt_pause_report_f() |
			gr_gpcs_tpcs_sm_hww_global_esr_report_mask_single_step_complete_report_f());
	}
}

int gk20a_enable_gr_hw(struct gk20a *g)
{
	int err;

	nvgpu_log_fn(g, " ");

	gk20a_init_gr_prepare(g);

	err = nvgpu_netlist_init_ctx_vars(g);
	if (err != 0) {
		nvgpu_err(g, "failed to parse netlist");
		return err;
	}

	err = gk20a_init_gr_reset_enable_hw(g);
	if (err != 0) {
		return err;
	}

	nvgpu_log_fn(g, "done");

	return 0;
}

int gk20a_gr_reset(struct gk20a *g)
{
	int err;

	g->gr.initialized = false;

	nvgpu_mutex_acquire(&g->gr.fecs_mutex);

	err = gk20a_enable_gr_hw(g);
	if (err != 0) {
		nvgpu_mutex_release(&g->gr.fecs_mutex);
		return err;
	}

	err = gk20a_init_gr_setup_hw(g);
	if (err != 0) {
		nvgpu_mutex_release(&g->gr.fecs_mutex);
		return err;
	}

	err = gr_gk20a_init_ctxsw(g);
	if (err != 0) {
		nvgpu_mutex_release(&g->gr.fecs_mutex);
		return err;
	}

	nvgpu_mutex_release(&g->gr.fecs_mutex);

	/* this appears query for sw states but fecs actually init
	   ramchain, etc so this is hw init */
	err = g->ops.gr.init_ctx_state(g);
	if (err != 0) {
		return err;
	}

	if (g->can_elpg) {
		err = gk20a_init_gr_bind_fecs_elpg(g);
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

static void gk20a_gr_set_error_notifier(struct gk20a *g,
		  struct gr_gk20a_isr_data *isr_data, u32 error_notifier)
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

static int gk20a_gr_handle_semaphore_timeout_pending(struct gk20a *g,
		  struct gr_gk20a_isr_data *isr_data)
{
	nvgpu_log_fn(g, " ");
	gk20a_gr_set_error_notifier(g, isr_data,
			 NVGPU_ERR_NOTIFIER_GR_SEMAPHORE_TIMEOUT);
	nvgpu_err(g,
		   "gr semaphore timeout");
	return -EINVAL;
}

static int gk20a_gr_intr_illegal_notify_pending(struct gk20a *g,
		  struct gr_gk20a_isr_data *isr_data)
{
	nvgpu_log_fn(g, " ");
	gk20a_gr_set_error_notifier(g, isr_data,
			 NVGPU_ERR_NOTIFIER_GR_ILLEGAL_NOTIFY);
	/* This is an unrecoverable error, reset is needed */
	nvgpu_err(g,
		   "gr semaphore timeout");
	return -EINVAL;
}

static int gk20a_gr_handle_illegal_method(struct gk20a *g,
					  struct gr_gk20a_isr_data *isr_data)
{
	int ret = g->ops.gr.handle_sw_method(g, isr_data->addr,
			isr_data->class_num, isr_data->offset,
			isr_data->data_lo);
	if (ret != 0) {
		gk20a_gr_set_error_notifier(g, isr_data,
			 NVGPU_ERR_NOTIFIER_GR_ILLEGAL_NOTIFY);
		nvgpu_err(g, "invalid method class 0x%08x"
			", offset 0x%08x address 0x%08x",
			isr_data->class_num, isr_data->offset, isr_data->addr);
	}
	return ret;
}

static int gk20a_gr_handle_illegal_class(struct gk20a *g,
					  struct gr_gk20a_isr_data *isr_data)
{
	nvgpu_log_fn(g, " ");
	gk20a_gr_set_error_notifier(g, isr_data,
			 NVGPU_ERR_NOTIFIER_GR_ERROR_SW_NOTIFY);
	nvgpu_err(g,
		   "invalid class 0x%08x, offset 0x%08x",
		   isr_data->class_num, isr_data->offset);
	return -EINVAL;
}

int gk20a_gr_handle_fecs_error(struct gk20a *g, struct channel_gk20a *ch,
					  struct gr_gk20a_isr_data *isr_data)
{
	u32 gr_fecs_intr = gk20a_readl(g, gr_fecs_host_int_status_r());
	int ret = 0;
	u32 chid = isr_data->ch != NULL ?
		isr_data->ch->chid : FIFO_INVAL_CHANNEL_ID;

	if (gr_fecs_intr == 0U) {
		return 0;
	}

	if ((gr_fecs_intr &
	     gr_fecs_host_int_status_umimp_firmware_method_f(1)) != 0U) {
		gk20a_gr_set_error_notifier(g, isr_data,
			 NVGPU_ERR_NOTIFIER_FECS_ERR_UNIMP_FIRMWARE_METHOD);
		nvgpu_err(g,
			  "firmware method error 0x%08x for offset 0x%04x",
			  gk20a_readl(g, gr_fecs_ctxsw_mailbox_r(6)),
			  isr_data->data_lo);
		ret = -1;
	} else if ((gr_fecs_intr &
			gr_fecs_host_int_status_watchdog_active_f()) != 0U) {
		gr_report_ctxsw_error(g, GPU_FECS_CTXSW_WATCHDOG_TIMEOUT,
				chid, 0);
		/* currently, recovery is not initiated */
		nvgpu_err(g, "fecs watchdog triggered for channel %u, "
				"cannot ctxsw anymore !!", chid);
		g->ops.gr.falcon.dump_stats(g);
	} else if ((gr_fecs_intr &
		    gr_fecs_host_int_status_ctxsw_intr_f(CTXSW_INTR0)) != 0U) {
		u32 mailbox_value = gk20a_readl(g, gr_fecs_ctxsw_mailbox_r(6));

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
		if (g->ops.gr.get_ctxsw_checksum_mismatch_mailbox_val
				!= NULL && mailbox_value ==
				g->ops.gr.get_ctxsw_checksum_mismatch_mailbox_val()) {

			gr_report_ctxsw_error(g, GPU_FECS_CTXSW_CRC_MISMATCH,
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
			gr_report_ctxsw_error(g, GPU_FECS_FAULT_DURING_CTXSW,
					chid, mailbox_value);
			nvgpu_err(g,
				 "ctxsw intr0 set by ucode, error_code: 0x%08x",
				 mailbox_value);
			ret = -1;
		}
	} else if ((gr_fecs_intr &
			gr_fecs_host_int_status_fault_during_ctxsw_f(1)) != 0U) {
		gr_report_ctxsw_error(g, GPU_FECS_FAULT_DURING_CTXSW,
				chid, 0);
		nvgpu_err(g, "fecs fault during ctxsw for channel %u", chid);
		ret = -1;
	} else {
		nvgpu_err(g,
			"unhandled fecs error interrupt 0x%08x for channel %u",
			gr_fecs_intr, chid);
		g->ops.gr.falcon.dump_stats(g);
	}

	gk20a_writel(g, gr_fecs_host_int_clear_r(), gr_fecs_intr);
	return ret;
}

static int gk20a_gr_handle_class_error(struct gk20a *g,
				       struct gr_gk20a_isr_data *isr_data)
{
	u32 gr_class_error;
	u32 chid = isr_data->ch != NULL ?
		isr_data->ch->chid : FIFO_INVAL_CHANNEL_ID;

	nvgpu_log_fn(g, " ");

	gr_class_error =
		gr_class_error_code_v(gk20a_readl(g, gr_class_error_r()));
	gk20a_gr_set_error_notifier(g, isr_data,
			 NVGPU_ERR_NOTIFIER_GR_ERROR_SW_NOTIFY);
	nvgpu_err(g, "class error 0x%08x, offset 0x%08x,"
		"sub channel 0x%08x mme generated %d,"
		" mme pc 0x%08xdata high %d priv status %d"
		" unhandled intr 0x%08x for channel %u",
		isr_data->class_num, (isr_data->offset << 2),
		gr_trapped_addr_subch_v(isr_data->addr),
		gr_trapped_addr_mme_generated_v(isr_data->addr),
		gr_trapped_data_mme_pc_v(
			gk20a_readl(g, gr_trapped_data_mme_r())),
		gr_trapped_addr_datahigh_v(isr_data->addr),
		gr_trapped_addr_priv_v(isr_data->addr),
		gr_class_error, chid);

	nvgpu_err(g, "trapped data low 0x%08x",
		gk20a_readl(g, gr_trapped_data_lo_r()));
	if (gr_trapped_addr_datahigh_v(isr_data->addr) != 0U) {
		nvgpu_err(g, "trapped data high 0x%08x",
		gk20a_readl(g, gr_trapped_data_hi_r()));
	}

	return -EINVAL;
}

static int gk20a_gr_handle_firmware_method(struct gk20a *g,
					   struct gr_gk20a_isr_data *isr_data)
{
	u32 chid = isr_data->ch != NULL ?
		isr_data->ch->chid : FIFO_INVAL_CHANNEL_ID;

	nvgpu_log_fn(g, " ");

	gk20a_gr_set_error_notifier(g, isr_data,
			 NVGPU_ERR_NOTIFIER_GR_ERROR_SW_NOTIFY);
	nvgpu_err(g,
		   "firmware method 0x%08x, offset 0x%08x for channel %u",
		   isr_data->class_num, isr_data->offset,
		   chid);
	return -EINVAL;
}

int gk20a_gr_handle_semaphore_pending(struct gk20a *g,
				     struct gr_gk20a_isr_data *isr_data)
{
	struct channel_gk20a *ch = isr_data->ch;
	struct tsg_gk20a *tsg;

	if (ch == NULL) {
		return 0;
	}

	tsg = tsg_gk20a_from_ch(ch);
	if (tsg != NULL) {
		g->ops.fifo.post_event_id(tsg,
			NVGPU_EVENT_ID_GR_SEMAPHORE_WRITE_AWAKEN);

		nvgpu_cond_broadcast(&ch->semaphore_wq);
	} else {
		nvgpu_err(g, "chid: %d is not bound to tsg", ch->chid);
	}

	return 0;
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
	valid = valid && gk20a_io_valid_reg(g, offset);
	return valid;
}
#endif

int gk20a_gr_handle_notify_pending(struct gk20a *g,
					  struct gr_gk20a_isr_data *isr_data)
{
	struct channel_gk20a *ch = isr_data->ch;

#if defined(CONFIG_GK20A_CYCLE_STATS)
	void *virtual_address;
	u32 buffer_size;
	u32 offset;
	bool exit;
#endif
	if (ch == NULL || tsg_gk20a_from_ch(ch) == NULL) {
		return 0;
	}

#if defined(CONFIG_GK20A_CYCLE_STATS)
	/* GL will never use payload 0 for cycle state */
	if ((ch->cyclestate.cyclestate_buffer == NULL) ||
	    (isr_data->data_lo == 0)) {
		return 0;
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
			(sh_hdr->operation == OP_END ?
			 sizeof(struct share_buffer_head) :
			 sizeof(struct gk20a_cyclestate_buffer_elem));

		/* validate sh_hdr->size */
		if (sh_hdr->size < min_element_size ||
		    offset + sh_hdr->size > buffer_size ||
		    offset + sh_hdr->size < offset) {
			nvgpu_err(g,
				  "bad cyclestate buffer header size at offset 0x%x",
				  offset);
			sh_hdr->failed = true;
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

				sh_hdr->failed = exit = true;
				break;
			}


			mask_orig =
				((1ULL <<
				  (op_elem->last_bit + 1))
				 -1)&~((1ULL <<
					op_elem->first_bit)-1);

			raw_reg =
				gk20a_readl(g,
					    op_elem->offset_bar0);

			switch (sh_hdr->operation) {
			case BAR0_READ32:
				op_elem->data =
					(raw_reg & mask_orig)
					>> op_elem->first_bit;
				break;

			case BAR0_WRITE32:
				v = 0;
				if ((unsigned int)mask_orig !=
							~((unsigned int)0)) {
					v = (unsigned int)
						(raw_reg & ~mask_orig);
				}

				v |= ((op_elem->data
				       << op_elem->first_bit)
				      & mask_orig);

				gk20a_writel(g,
					     op_elem->offset_bar0,
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
		sh_hdr->completed = true;
		offset += sh_hdr->size;
	}
	nvgpu_mutex_release(&ch->cyclestate.cyclestate_buffer_mutex);
#endif
	nvgpu_log_fn(g, " ");
	nvgpu_cond_broadcast_interruptible(&ch->notifier_wq);
	return 0;
}

/* Used by sw interrupt thread to translate current ctx to chid.
 * Also used by regops to translate current ctx to chid and tsgid.
 * For performance, we don't want to go through 128 channels every time.
 * curr_ctx should be the value read from gr_fecs_current_ctx_r().
 * A small tlb is used here to cache translation.
 *
 * Returned channel must be freed with gk20a_channel_put() */
static struct channel_gk20a *gk20a_gr_get_channel_from_ctx(
	struct gk20a *g, u32 curr_ctx, u32 *curr_tsgid)
{
	struct fifo_gk20a *f = &g->fifo;
	struct gr_gk20a *gr = &g->gr;
	u32 chid;
	u32 tsgid = NVGPU_INVALID_TSG_ID;
	u32 i;
	struct channel_gk20a *ret = NULL;

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
			ret = gk20a_channel_from_id(g, chid);
			goto unlock;
		}
	}

	/* slow path */
	for (chid = 0; chid < f->num_channels; chid++) {
		struct channel_gk20a *ch = gk20a_channel_from_id(g, chid);

		if (ch == NULL) {
			continue;
		}

		if ((u32)(nvgpu_inst_block_addr(g, &ch->inst_block) >>
					ram_in_base_shift_v()) ==
				gr_fecs_current_ctx_ptr_v(curr_ctx)) {
			tsgid = ch->tsgid;
			/* found it */
			ret = ch;
			break;
		}
		gk20a_channel_put(ch);
	}

	if (ret == NULL) {
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
	return ret;
}

int gk20a_gr_lock_down_sm(struct gk20a *g,
			 u32 gpc, u32 tpc, u32 sm, u32 global_esr_mask,
			 bool check_errors)
{
	u32 offset = nvgpu_gr_gpc_offset(g, gpc) + nvgpu_gr_tpc_offset(g, tpc);
	u32 dbgr_control0;

	nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg,
			"GPC%d TPC%d SM%d: assert stop trigger", gpc, tpc, sm);

	/* assert stop trigger */
	dbgr_control0 =
		gk20a_readl(g, gr_gpc0_tpc0_sm_dbgr_control0_r() + offset);
	dbgr_control0 |= gr_gpc0_tpc0_sm_dbgr_control0_stop_trigger_enable_f();
	gk20a_writel(g,
		gr_gpc0_tpc0_sm_dbgr_control0_r() + offset, dbgr_control0);

	return g->ops.gr.wait_for_sm_lock_down(g, gpc, tpc, sm, global_esr_mask,
			check_errors);
}

bool gk20a_gr_sm_debugger_attached(struct gk20a *g)
{
	u32 dbgr_control0 = gk20a_readl(g, gr_gpc0_tpc0_sm_dbgr_control0_r());

	/* check if an sm debugger is attached.
	 * assumption: all SMs will have debug mode enabled/disabled
	 * uniformly. */
	if (gr_gpc0_tpc0_sm_dbgr_control0_debugger_mode_v(dbgr_control0) ==
			gr_gpc0_tpc0_sm_dbgr_control0_debugger_mode_on_v()) {
		return true;
	}

	return false;
}

int gr_gk20a_handle_sm_exception(struct gk20a *g, u32 gpc, u32 tpc, u32 sm,
		bool *post_event, struct channel_gk20a *fault_ch,
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
		nvgpu_report_gr_sm_exception(g, gpc, tpc, sm, warp_esr,
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
	if ((global_esr == gr_gpc0_tpc0_sm_hww_global_esr_bpt_int_pending_f())
			&& (warp_esr == 0U)) {
		disable_sm_exceptions = false;
	}

	if (!ignore_debugger && disable_sm_exceptions) {
		u32 tpc_exception_en = gk20a_readl(g,
				gr_gpc0_tpc0_tpccs_tpc_exception_en_r() +
				offset);
		tpc_exception_en &= ~gr_gpc0_tpc0_tpccs_tpc_exception_en_sm_enabled_f();
		gk20a_writel(g,
			     gr_gpc0_tpc0_tpccs_tpc_exception_en_r() + offset,
			     tpc_exception_en);
		nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg, "SM Exceptions disabled");
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

void gk20a_gr_get_esr_sm_sel(struct gk20a *g, u32 gpc, u32 tpc,
				u32 *esr_sm_sel)
{
	*esr_sm_sel = 1;
}

static int gk20a_gr_handle_tpc_exception(struct gk20a *g, u32 gpc, u32 tpc,
		bool *post_event, struct channel_gk20a *fault_ch,
		u32 *hww_global_esr)
{
	int tmp_ret, ret = 0;
	u32 offset = nvgpu_gr_gpc_offset(g, gpc) + nvgpu_gr_tpc_offset(g, tpc);
	u32 tpc_exception = gk20a_readl(g, gr_gpc0_tpc0_tpccs_tpc_exception_r()
			+ offset);
	u32 sm_per_tpc = nvgpu_get_litter_value(g, GPU_LIT_NUM_SM_PER_TPC);

	nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg,
			"GPC%d TPC%d: pending exception 0x%x",
			gpc, tpc, tpc_exception);

	/* check if an sm exeption is pending */
	if (gr_gpc0_tpc0_tpccs_tpc_exception_sm_v(tpc_exception) ==
			gr_gpc0_tpc0_tpccs_tpc_exception_sm_pending_v()) {
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
	if (gr_gpc0_tpc0_tpccs_tpc_exception_tex_v(tpc_exception) ==
			gr_gpc0_tpc0_tpccs_tpc_exception_tex_pending_v()) {
		nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg,
				"GPC%d TPC%d: TEX exception pending", gpc, tpc);
		if (g->ops.gr.intr.handle_tex_exception != NULL) {
			g->ops.gr.intr.handle_tex_exception(g, gpc, tpc);
		}
	}

	if (g->ops.gr.intr.handle_tpc_mpc_exception != NULL) {
		g->ops.gr.intr.handle_tpc_mpc_exception(g, gpc, tpc);
	}

	return ret;
}

static int gk20a_gr_handle_gpc_exception(struct gk20a *g, bool *post_event,
		struct channel_gk20a *fault_ch, u32 *hww_global_esr)
{
	int tmp_ret, ret = 0;
	u32 gpc_offset, gpc, tpc;
	struct gr_gk20a *gr = &g->gr;
	u32 exception1 = gk20a_readl(g, gr_exception1_r());
	u32 gpc_exception;

	nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg, " ");

	for (gpc = 0; gpc < nvgpu_gr_config_get_gpc_count(gr->config); gpc++) {
		if ((exception1 & BIT32(gpc)) == 0U) {
			continue;
		}

		nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg,
				"GPC%d exception pending", gpc);

		gpc_offset = nvgpu_gr_gpc_offset(g, gpc);

		gpc_exception = gk20a_readl(g, gr_gpc0_gpccs_gpc_exception_r()
				+ gpc_offset);

		/* check if any tpc has an exception */
		for (tpc = 0; tpc < nvgpu_gr_config_get_gpc_tpc_count(gr->config, gpc); tpc++) {
			if ((gr_gpc0_gpccs_gpc_exception_tpc_v(gpc_exception) &
				BIT32(tpc)) == 0U) {
				continue;
			}

			nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg,
				  "GPC%d: TPC%d exception pending", gpc, tpc);

			tmp_ret = gk20a_gr_handle_tpc_exception(g, gpc, tpc,
					post_event, fault_ch, hww_global_esr);
			ret = (ret != 0) ? ret : tmp_ret;
		}

		/* Handle GCC exception */
		if ((gr_gpc0_gpccs_gpc_exception_gcc_v(gpc_exception) != 0U) &&
				(g->ops.gr.handle_gcc_exception != NULL)) {
			tmp_ret = g->ops.gr.handle_gcc_exception(g, gpc, tpc,
				post_event, fault_ch, hww_global_esr);
			ret = (ret != 0) ? ret : tmp_ret;
		}

		/* Handle GPCCS exceptions */
		if (g->ops.gr.handle_gpc_gpccs_exception != NULL) {
			tmp_ret = g->ops.gr.handle_gpc_gpccs_exception(g, gpc,
								gpc_exception);
			ret = (ret != 0) ? ret : tmp_ret;
		}

		/* Handle GPCMMU exceptions */
		if (g->ops.gr.handle_gpc_gpcmmu_exception != NULL) {
			tmp_ret = g->ops.gr.handle_gpc_gpcmmu_exception(g, gpc,
								gpc_exception);
			ret = (ret != 0) ? ret : tmp_ret;
		}

	}

	return ret;
}

static int gk20a_gr_post_bpt_events(struct gk20a *g, struct tsg_gk20a *tsg,
				    u32 global_esr)
{
	if ((global_esr &
	     gr_gpc0_tpc0_sm_hww_global_esr_bpt_int_pending_f()) != 0U) {
		g->ops.fifo.post_event_id(tsg, NVGPU_EVENT_ID_BPT_INT);
	}

	if ((global_esr &
	     gr_gpc0_tpc0_sm_hww_global_esr_bpt_pause_pending_f()) != 0U) {
		g->ops.fifo.post_event_id(tsg, NVGPU_EVENT_ID_BPT_PAUSE);
	}

	return 0;
}

int gk20a_gr_isr(struct gk20a *g)
{
	struct gr_gk20a_isr_data isr_data;
	u32 obj_table;
	bool need_reset = false;
	u32 gr_intr = gk20a_readl(g, gr_intr_r());
	struct channel_gk20a *ch = NULL;
	struct channel_gk20a *fault_ch = NULL;
	u32 tsgid = NVGPU_INVALID_TSG_ID;
	struct tsg_gk20a *tsg = NULL;
	u32 gr_engine_id;
	u32 global_esr = 0;
	u32 chid;

	nvgpu_log_fn(g, " ");
	nvgpu_log(g, gpu_dbg_intr, "pgraph intr 0x%08x", gr_intr);

	if (gr_intr == 0U) {
		return 0;
	}

	gr_engine_id = nvgpu_engine_get_gr_id(g);
	if (gr_engine_id != FIFO_INVAL_ENGINE_ID) {
		gr_engine_id = BIT32(gr_engine_id);
	}

	/* Disable fifo access */
	g->ops.gr.init.fifo_access(g, false);

	isr_data.addr = gk20a_readl(g, gr_trapped_addr_r());
	isr_data.data_lo = gk20a_readl(g, gr_trapped_data_lo_r());
	isr_data.data_hi = gk20a_readl(g, gr_trapped_data_hi_r());
	isr_data.curr_ctx = gk20a_readl(g, gr_fecs_current_ctx_r());
	isr_data.offset = gr_trapped_addr_mthd_v(isr_data.addr);
	isr_data.sub_chan = gr_trapped_addr_subch_v(isr_data.addr);
	obj_table = (isr_data.sub_chan < 4U) ? gk20a_readl(g,
		gr_fe_object_table_r(isr_data.sub_chan)) : 0U;
	isr_data.class_num = gr_fe_object_table_nvclass_v(obj_table);

	ch = gk20a_gr_get_channel_from_ctx(g, isr_data.curr_ctx, &tsgid);
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

	if ((gr_intr & gr_intr_notify_pending_f()) != 0U) {
		g->ops.gr.handle_notify_pending(g, &isr_data);
		gk20a_writel(g, gr_intr_r(),
			gr_intr_notify_reset_f());
		gr_intr &= ~gr_intr_notify_pending_f();
	}

	if ((gr_intr & gr_intr_semaphore_pending_f()) != 0U) {
		g->ops.gr.handle_semaphore_pending(g, &isr_data);
		gk20a_writel(g, gr_intr_r(),
			gr_intr_semaphore_reset_f());
		gr_intr &= ~gr_intr_semaphore_pending_f();
	}

	if ((gr_intr & gr_intr_semaphore_timeout_pending_f()) != 0U) {
		if (gk20a_gr_handle_semaphore_timeout_pending(g,
				&isr_data) != 0) {
			need_reset = true;
		}
		gk20a_writel(g, gr_intr_r(),
			gr_intr_semaphore_reset_f());
		gr_intr &= ~gr_intr_semaphore_pending_f();
	}

	if ((gr_intr & gr_intr_illegal_notify_pending_f()) != 0U) {
		if (gk20a_gr_intr_illegal_notify_pending(g,
				&isr_data) != 0) {
			need_reset = true;
		}
		gk20a_writel(g, gr_intr_r(),
			gr_intr_illegal_notify_reset_f());
		gr_intr &= ~gr_intr_illegal_notify_pending_f();
	}

	if ((gr_intr & gr_intr_illegal_method_pending_f()) != 0U) {
		if (gk20a_gr_handle_illegal_method(g, &isr_data) != 0) {
			need_reset = true;
		}
		gk20a_writel(g, gr_intr_r(),
			gr_intr_illegal_method_reset_f());
		gr_intr &= ~gr_intr_illegal_method_pending_f();
	}

	if ((gr_intr & gr_intr_illegal_class_pending_f()) != 0U) {
		if (gk20a_gr_handle_illegal_class(g, &isr_data) != 0) {
			need_reset = true;
		}
		gk20a_writel(g, gr_intr_r(),
			gr_intr_illegal_class_reset_f());
		gr_intr &= ~gr_intr_illegal_class_pending_f();
	}

	if ((gr_intr & gr_intr_fecs_error_pending_f()) != 0U) {
		if (g->ops.gr.handle_fecs_error(g, ch, &isr_data) != 0) {
			need_reset = true;
		}
		gk20a_writel(g, gr_intr_r(),
			gr_intr_fecs_error_reset_f());
		gr_intr &= ~gr_intr_fecs_error_pending_f();
	}

	if ((gr_intr & gr_intr_class_error_pending_f()) != 0U) {
		if (gk20a_gr_handle_class_error(g, &isr_data) != 0) {
			need_reset = true;
		}
		gk20a_writel(g, gr_intr_r(),
			gr_intr_class_error_reset_f());
		gr_intr &= ~gr_intr_class_error_pending_f();
	}

	/* this one happens if someone tries to hit a non-whitelisted
	 * register using set_falcon[4] */
	if ((gr_intr & gr_intr_firmware_method_pending_f()) != 0U) {
		if (gk20a_gr_handle_firmware_method(g, &isr_data) != 0) {
			need_reset = true;
		}
		nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg, "firmware method intr pending\n");
		gk20a_writel(g, gr_intr_r(),
			gr_intr_firmware_method_reset_f());
		gr_intr &= ~gr_intr_firmware_method_pending_f();
	}

	if ((gr_intr & gr_intr_exception_pending_f()) != 0U) {
		u32 exception = gk20a_readl(g, gr_exception_r());

		nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg, "exception %08x\n", exception);

		if ((exception & gr_exception_fe_m()) != 0U) {
			u32 fe = gk20a_readl(g, gr_fe_hww_esr_r());
			u32 info = gk20a_readl(g, gr_fe_hww_esr_info_r());

			nvgpu_report_gr_exception(g, 0,
					GPU_PGRAPH_FE_EXCEPTION,
					fe);
			nvgpu_err(g, "fe exception: esr 0x%08x, info 0x%08x",
					fe, info);
			gk20a_writel(g, gr_fe_hww_esr_r(),
				gr_fe_hww_esr_reset_active_f());
			need_reset = true;
		}

		if ((exception & gr_exception_memfmt_m()) != 0U) {
			u32 memfmt = gk20a_readl(g, gr_memfmt_hww_esr_r());

			nvgpu_report_gr_exception(g, 0,
					GPU_PGRAPH_MEMFMT_EXCEPTION,
					memfmt);
			nvgpu_err(g, "memfmt exception: esr %08x", memfmt);
			gk20a_writel(g, gr_memfmt_hww_esr_r(),
					gr_memfmt_hww_esr_reset_active_f());
			need_reset = true;
		}

		if ((exception & gr_exception_pd_m()) != 0U) {
			u32 pd = gk20a_readl(g, gr_pd_hww_esr_r());

			nvgpu_report_gr_exception(g, 0,
					GPU_PGRAPH_PD_EXCEPTION,
					pd);
			nvgpu_err(g, "pd exception: esr 0x%08x", pd);
			gk20a_writel(g, gr_pd_hww_esr_r(),
					gr_pd_hww_esr_reset_active_f());
			need_reset = true;
		}

		if ((exception & gr_exception_scc_m()) != 0U) {
			u32 scc = gk20a_readl(g, gr_scc_hww_esr_r());

			nvgpu_report_gr_exception(g, 0,
					GPU_PGRAPH_SCC_EXCEPTION,
					scc);
			nvgpu_err(g, "scc exception: esr 0x%08x", scc);
			gk20a_writel(g, gr_scc_hww_esr_r(),
					gr_scc_hww_esr_reset_active_f());
			need_reset = true;
		}

		if ((exception & gr_exception_ds_m()) != 0U) {
			u32 ds = gk20a_readl(g, gr_ds_hww_esr_r());

			nvgpu_report_gr_exception(g, 0,
					GPU_PGRAPH_DS_EXCEPTION,
					ds);
			nvgpu_err(g, "ds exception: esr: 0x%08x", ds);
			gk20a_writel(g, gr_ds_hww_esr_r(),
					 gr_ds_hww_esr_reset_task_f());
			need_reset = true;
		}

		if ((exception & gr_exception_ssync_m()) != 0U) {
			u32 ssync_esr = 0;

			if (g->ops.gr.handle_ssync_hww != NULL) {
				if (g->ops.gr.handle_ssync_hww(g, &ssync_esr)
						!= 0) {
					need_reset = true;
				}
			} else {
				nvgpu_err(g, "unhandled ssync exception");
			}
			nvgpu_report_gr_exception(g, 0,
					GPU_PGRAPH_SSYNC_EXCEPTION,
					ssync_esr);
		}

		if ((exception & gr_exception_mme_m()) != 0U) {
			u32 mme = gk20a_readl(g, gr_mme_hww_esr_r());
			u32 info = gk20a_readl(g, gr_mme_hww_esr_info_r());

			nvgpu_report_gr_exception(g, 0,
					GPU_PGRAPH_MME_EXCEPTION,
					mme);
			nvgpu_err(g, "mme exception: esr 0x%08x info:0x%08x",
					mme, info);
			if (g->ops.gr.log_mme_exception != NULL) {
				g->ops.gr.log_mme_exception(g);
			}

			gk20a_writel(g, gr_mme_hww_esr_r(),
				gr_mme_hww_esr_reset_active_f());
			need_reset = true;
		}

		if ((exception & gr_exception_sked_m()) != 0U) {
			u32 sked = gk20a_readl(g, gr_sked_hww_esr_r());

			nvgpu_report_gr_exception(g, 0,
					GPU_PGRAPH_SKED_EXCEPTION,
					sked);
			nvgpu_err(g, "sked exception: esr 0x%08x", sked);
			gk20a_writel(g, gr_sked_hww_esr_r(),
				gr_sked_hww_esr_reset_active_f());
			need_reset = true;
		}

		/* check if a gpc exception has occurred */
		if (((exception & gr_exception_gpc_m()) != 0U) &&
						!need_reset) {
			bool post_event = false;

			nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg,
					 "GPC exception pending");

			if (tsg != NULL) {
				fault_ch = isr_data.ch;
			}

			/* fault_ch can be NULL */
			/* check if any gpc has an exception */
			if (gk20a_gr_handle_gpc_exception(g, &post_event,
					fault_ch, &global_esr) != 0) {
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

		gk20a_writel(g, gr_intr_r(), gr_intr_exception_reset_f());
		gr_intr &= ~gr_intr_exception_pending_f();

		if (need_reset) {
			nvgpu_err(g, "set gr exception notifier");
			gk20a_gr_set_error_notifier(g, &isr_data,
					 NVGPU_ERR_NOTIFIER_GR_EXCEPTION);
		}
	}

	if (need_reset) {
		if (tsg != NULL) {
			gk20a_fifo_recover(g, gr_engine_id,
					   tsgid, true, true, true,
						RC_TYPE_GR_FAULT);
		} else {
			if (ch != NULL) {
				nvgpu_err(g, "chid: %d referenceable but not "
					"bound to tsg", chid);
			}
			gk20a_fifo_recover(g, gr_engine_id,
					   0, false, false, true,
						RC_TYPE_GR_FAULT);
		}
	}

	if (gr_intr != 0U) {
		/* clear unhandled interrupts */
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
		gk20a_writel(g, gr_intr_r(), gr_intr);
	}

	/* Enable fifo access */
	g->ops.gr.init.fifo_access(g, true);

	/* Posting of BPT events should be the last thing in this function */
	if ((global_esr != 0U) && (tsg != NULL) && (need_reset == false)) {
		gk20a_gr_post_bpt_events(g, tsg, global_esr);
	}

	if (ch != NULL) {
		gk20a_channel_put(ch);
	}

	return 0;
}

u32 gk20a_gr_nonstall_isr(struct gk20a *g)
{
	u32 ops = 0;
	u32 gr_intr = gk20a_readl(g, gr_intr_nonstall_r());

	nvgpu_log(g, gpu_dbg_intr, "pgraph nonstall intr %08x", gr_intr);

	if ((gr_intr & gr_intr_nonstall_trap_pending_f()) != 0U) {
		/* Clear the interrupt */
		gk20a_writel(g, gr_intr_nonstall_r(),
			gr_intr_nonstall_trap_pending_f());
		ops |= (GK20A_NONSTALL_OPS_WAKEUP_SEMAPHORE |
			GK20A_NONSTALL_OPS_POST_EVENTS);
	}
	return ops;
}

int gr_gk20a_fecs_get_reglist_img_size(struct gk20a *g, u32 *size)
{
	BUG_ON(size == NULL);
	return gr_gk20a_submit_fecs_method_op(g,
		   (struct fecs_method_op_gk20a) {
			   .mailbox.id = 0U,
			   .mailbox.data = 0U,
			   .mailbox.clr = ~U32(0U),
			   .method.data = 1U,
			   .method.addr = gr_fecs_method_push_adr_discover_reglist_image_size_v(),
			   .mailbox.ret = size,
			   .cond.ok = GR_IS_UCODE_OP_NOT_EQUAL,
			   .mailbox.ok = 0U,
			   .cond.fail = GR_IS_UCODE_OP_SKIP,
			   .mailbox.fail = 0U}, false);
}

int gr_gk20a_fecs_set_reglist_bind_inst(struct gk20a *g,
		struct nvgpu_mem *inst_block)
{
	u32 data = fecs_current_ctx_data(g, inst_block);

	return gr_gk20a_submit_fecs_method_op(g,
		   (struct fecs_method_op_gk20a){
			   .mailbox.id = 4U,
			   .mailbox.data = data,
			   .mailbox.clr = ~U32(0U),
			   .method.data = 1U,
			   .method.addr = gr_fecs_method_push_adr_set_reglist_bind_instance_v(),
			   .mailbox.ret = NULL,
			   .cond.ok = GR_IS_UCODE_OP_EQUAL,
			   .mailbox.ok = 1U,
			   .cond.fail = GR_IS_UCODE_OP_SKIP,
			   .mailbox.fail = 0U}, false);
}

int gr_gk20a_fecs_set_reglist_virtual_addr(struct gk20a *g, u64 pmu_va)
{
	return gr_gk20a_submit_fecs_method_op(g,
		   (struct fecs_method_op_gk20a) {
			   .mailbox.id = 4U,
			   .mailbox.data = u64_lo32(pmu_va >> 8),
			   .mailbox.clr = ~U32(0U),
			   .method.data = 1U,
			   .method.addr = gr_fecs_method_push_adr_set_reglist_virtual_address_v(),
			   .mailbox.ret = NULL,
			   .cond.ok = GR_IS_UCODE_OP_EQUAL,
			   .mailbox.ok = 1U,
			   .cond.fail = GR_IS_UCODE_OP_SKIP,
			   .mailbox.fail = 0U}, false);
}

static int gr_gk20a_find_priv_offset_in_buffer(struct gk20a *g,
					       u32 addr,
					       bool is_quad, u32 quad,
					       u32 *context_buffer,
					       u32 context_buffer_size,
					       u32 *priv_offset);

/* This function will decode a priv address and return the partition type and numbers. */
int gr_gk20a_decode_priv_addr(struct gk20a *g, u32 addr,
			      enum ctxsw_addr_type *addr_type,
			      u32 *gpc_num, u32 *tpc_num, u32 *ppc_num, u32 *be_num,
			      u32 *broadcast_flags)
{
	u32 gpc_addr;

	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg, "addr=0x%x", addr);

	/* setup defaults */
	*addr_type = CTXSW_ADDR_TYPE_SYS;
	*broadcast_flags = PRI_BROADCAST_FLAGS_NONE;
	*gpc_num = 0;
	*tpc_num = 0;
	*ppc_num = 0;
	*be_num  = 0;

	if (pri_is_gpc_addr(g, addr)) {
		*addr_type = CTXSW_ADDR_TYPE_GPC;
		gpc_addr = pri_gpccs_addr_mask(addr);
		if (pri_is_gpc_addr_shared(g, addr)) {
			*addr_type = CTXSW_ADDR_TYPE_GPC;
			*broadcast_flags |= PRI_BROADCAST_FLAGS_GPC;
		} else {
			*gpc_num = pri_get_gpc_num(g, addr);
		}

		if (pri_is_ppc_addr(g, gpc_addr)) {
			*addr_type = CTXSW_ADDR_TYPE_PPC;
			if (pri_is_ppc_addr_shared(g, gpc_addr)) {
				*broadcast_flags |= PRI_BROADCAST_FLAGS_PPC;
				return 0;
			}
		}
		if (g->ops.gr.is_tpc_addr(g, gpc_addr)) {
			*addr_type = CTXSW_ADDR_TYPE_TPC;
			if (pri_is_tpc_addr_shared(g, gpc_addr)) {
				*broadcast_flags |= PRI_BROADCAST_FLAGS_TPC;
				return 0;
			}
			*tpc_num = g->ops.gr.get_tpc_num(g, gpc_addr);
		}
		return 0;
	} else if (pri_is_be_addr(g, addr)) {
		*addr_type = CTXSW_ADDR_TYPE_BE;
		if (pri_is_be_addr_shared(g, addr)) {
			*broadcast_flags |= PRI_BROADCAST_FLAGS_BE;
			return 0;
		}
		*be_num = pri_get_be_num(g, addr);
		return 0;
	} else if (g->ops.ltc.pri_is_ltc_addr(g, addr)) {
		*addr_type = CTXSW_ADDR_TYPE_LTCS;
		if (g->ops.ltc.is_ltcs_ltss_addr(g, addr)) {
			*broadcast_flags |= PRI_BROADCAST_FLAGS_LTCS;
		} else if (g->ops.ltc.is_ltcn_ltss_addr(g, addr)) {
			*broadcast_flags |= PRI_BROADCAST_FLAGS_LTSS;
		}
		return 0;
	} else if (pri_is_fbpa_addr(g, addr)) {
		*addr_type = CTXSW_ADDR_TYPE_FBPA;
		if (pri_is_fbpa_addr_shared(g, addr)) {
			*broadcast_flags |= PRI_BROADCAST_FLAGS_FBPA;
			return 0;
		}
		return 0;
	} else if ((g->ops.gr.is_egpc_addr != NULL) &&
		   g->ops.gr.is_egpc_addr(g, addr)) {
			return g->ops.gr.decode_egpc_addr(g,
					addr, addr_type, gpc_num,
					tpc_num, broadcast_flags);
	} else {
		*addr_type = CTXSW_ADDR_TYPE_SYS;
		return 0;
	}
	/* PPC!?!?!?! */

	/*NOTREACHED*/
	return -EINVAL;
}

void gr_gk20a_split_fbpa_broadcast_addr(struct gk20a *g, u32 addr,
				      u32 num_fbpas,
				      u32 *priv_addr_table, u32 *t)
{
	u32 fbpa_id;

	for (fbpa_id = 0; fbpa_id < num_fbpas; fbpa_id++) {
		priv_addr_table[(*t)++] = pri_fbpa_addr(g,
				pri_fbpa_addr_mask(g, addr), fbpa_id);
	}
}

int gr_gk20a_split_ppc_broadcast_addr(struct gk20a *g, u32 addr,
				      u32 gpc_num,
				      u32 *priv_addr_table, u32 *t)
{
	u32 ppc_num;

	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg, "addr=0x%x", addr);

	for (ppc_num = 0;
	     ppc_num < nvgpu_gr_config_get_gpc_ppc_count(g->gr.config, gpc_num);
	     ppc_num++) {
		priv_addr_table[(*t)++] = pri_ppc_addr(g, pri_ppccs_addr_mask(addr),
		gpc_num, ppc_num);
	}

	return 0;
}

/*
 * The context buffer is indexed using BE broadcast addresses and GPC/TPC
 * unicast addresses. This function will convert a BE unicast address to a BE
 * broadcast address and split a GPC/TPC broadcast address into a table of
 * GPC/TPC addresses.  The addresses generated by this function can be
 * successfully processed by gr_gk20a_find_priv_offset_in_buffer
 */
int gr_gk20a_create_priv_addr_table(struct gk20a *g,
					   u32 addr,
					   u32 *priv_addr_table,
					   u32 *num_registers)
{
	enum ctxsw_addr_type addr_type;
	u32 gpc_num, tpc_num, ppc_num, be_num;
	u32 priv_addr, gpc_addr;
	u32 broadcast_flags;
	u32 t;
	int err;

	t = 0;
	*num_registers = 0;

	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg, "addr=0x%x", addr);

	err = g->ops.gr.decode_priv_addr(g, addr, &addr_type,
					&gpc_num, &tpc_num, &ppc_num, &be_num,
					&broadcast_flags);
	nvgpu_log(g, gpu_dbg_gpu_dbg, "addr_type = %d", addr_type);
	if (err != 0) {
		return err;
	}

	if ((addr_type == CTXSW_ADDR_TYPE_SYS) ||
	    (addr_type == CTXSW_ADDR_TYPE_BE)) {
		/* The BE broadcast registers are included in the compressed PRI
		 * table. Convert a BE unicast address to a broadcast address
		 * so that we can look up the offset. */
		if ((addr_type == CTXSW_ADDR_TYPE_BE) &&
		    ((broadcast_flags & PRI_BROADCAST_FLAGS_BE) == 0U)) {
			priv_addr_table[t++] = pri_be_shared_addr(g, addr);
		} else {
			priv_addr_table[t++] = addr;
		}

		*num_registers = t;
		return 0;
	}

	/* The GPC/TPC unicast registers are included in the compressed PRI
	 * tables. Convert a GPC/TPC broadcast address to unicast addresses so
	 * that we can look up the offsets. */
	if ((broadcast_flags & PRI_BROADCAST_FLAGS_GPC) != 0U) {
		for (gpc_num = 0;
		     gpc_num < nvgpu_gr_config_get_gpc_count(g->gr.config);
		     gpc_num++) {

			if ((broadcast_flags & PRI_BROADCAST_FLAGS_TPC) != 0U) {
				for (tpc_num = 0;
				     tpc_num < nvgpu_gr_config_get_gpc_tpc_count(g->gr.config, gpc_num);
				     tpc_num++) {
					priv_addr_table[t++] =
						pri_tpc_addr(g, pri_tpccs_addr_mask(addr),
							     gpc_num, tpc_num);
				}

			} else if ((broadcast_flags & PRI_BROADCAST_FLAGS_PPC) != 0U) {
				err = gr_gk20a_split_ppc_broadcast_addr(g, addr, gpc_num,
							       priv_addr_table, &t);
				if (err != 0) {
					return err;
				}
			} else {
				priv_addr = pri_gpc_addr(g,
						pri_gpccs_addr_mask(addr),
						gpc_num);

				gpc_addr = pri_gpccs_addr_mask(priv_addr);
				tpc_num = g->ops.gr.get_tpc_num(g, gpc_addr);
				if (tpc_num >= nvgpu_gr_config_get_gpc_tpc_count(g->gr.config, gpc_num)) {
					continue;
				}

				priv_addr_table[t++] = priv_addr;
			}
		}
	} else if (((addr_type == CTXSW_ADDR_TYPE_EGPC) ||
			(addr_type == CTXSW_ADDR_TYPE_ETPC)) &&
			(g->ops.gr.egpc_etpc_priv_addr_table != NULL)) {
		nvgpu_log(g, gpu_dbg_gpu_dbg, "addr_type : EGPC/ETPC");
		g->ops.gr.egpc_etpc_priv_addr_table(g, addr, gpc_num, tpc_num,
				broadcast_flags, priv_addr_table, &t);
	} else if ((broadcast_flags & PRI_BROADCAST_FLAGS_LTSS) != 0U) {
		g->ops.ltc.split_lts_broadcast_addr(g, addr,
							priv_addr_table, &t);
	} else if ((broadcast_flags & PRI_BROADCAST_FLAGS_LTCS) != 0U) {
		g->ops.ltc.split_ltc_broadcast_addr(g, addr,
							priv_addr_table, &t);
	} else if ((broadcast_flags & PRI_BROADCAST_FLAGS_FBPA) != 0U) {
		g->ops.gr.split_fbpa_broadcast_addr(g, addr,
				nvgpu_get_litter_value(g, GPU_LIT_NUM_FBPAS),
				priv_addr_table, &t);
	} else if ((broadcast_flags & PRI_BROADCAST_FLAGS_GPC) == 0U) {
		if ((broadcast_flags & PRI_BROADCAST_FLAGS_TPC) != 0U) {
			for (tpc_num = 0;
			     tpc_num < nvgpu_gr_config_get_gpc_tpc_count(g->gr.config, gpc_num);
			     tpc_num++) {
				priv_addr_table[t++] =
					pri_tpc_addr(g, pri_tpccs_addr_mask(addr),
						     gpc_num, tpc_num);
			}
		} else if ((broadcast_flags & PRI_BROADCAST_FLAGS_PPC) != 0U) {
			err = gr_gk20a_split_ppc_broadcast_addr(g,
					addr, gpc_num, priv_addr_table, &t);
		} else {
			priv_addr_table[t++] = addr;
		}
	}

	*num_registers = t;
	return 0;
}

int gr_gk20a_get_ctx_buffer_offsets(struct gk20a *g,
				    u32 addr,
				    u32 max_offsets,
				    u32 *offsets, u32 *offset_addrs,
				    u32 *num_offsets,
				    bool is_quad, u32 quad)
{
	u32 i;
	u32 priv_offset = 0;
	u32 *priv_registers;
	u32 num_registers = 0;
	int err = 0;
	struct gr_gk20a *gr = &g->gr;
	u32 sm_per_tpc = nvgpu_get_litter_value(g, GPU_LIT_NUM_SM_PER_TPC);
	u32 potential_offsets = nvgpu_gr_config_get_max_gpc_count(gr->config) *
		nvgpu_gr_config_get_max_tpc_per_gpc_count(gr->config) *
		sm_per_tpc;

	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg, "addr=0x%x", addr);

	/* implementation is crossed-up if either of these happen */
	if (max_offsets > potential_offsets) {
		nvgpu_log_fn(g, "max_offsets > potential_offsets");
		return -EINVAL;
	}

	if (!g->gr.ctx_vars.golden_image_initialized) {
		return -ENODEV;
	}

	priv_registers = nvgpu_kzalloc(g, sizeof(u32) * potential_offsets);
	if (priv_registers == NULL) {
		nvgpu_log_fn(g, "failed alloc for potential_offsets=%d", potential_offsets);
		err = -ENOMEM;
		goto cleanup;
	}
	(void) memset(offsets,      0, sizeof(u32) * max_offsets);
	(void) memset(offset_addrs, 0, sizeof(u32) * max_offsets);
	*num_offsets = 0;

	g->ops.gr.create_priv_addr_table(g, addr, &priv_registers[0],
			&num_registers);

	if ((max_offsets > 1U) && (num_registers > max_offsets)) {
		nvgpu_log_fn(g, "max_offsets = %d, num_registers = %d",
				max_offsets, num_registers);
		err = -EINVAL;
		goto cleanup;
	}

	if ((max_offsets == 1U) && (num_registers > 1U)) {
		num_registers = 1;
	}

	if (!g->gr.ctx_vars.golden_image_initialized) {
		nvgpu_log_fn(g, "no context switch header info to work with");
		err = -EINVAL;
		goto cleanup;
	}

	for (i = 0; i < num_registers; i++) {
		err = gr_gk20a_find_priv_offset_in_buffer(g,
			  priv_registers[i],
			  is_quad, quad,
			  nvgpu_gr_global_ctx_get_local_golden_image_ptr(
				g->gr.local_golden_image),
			  g->gr.ctx_vars.golden_image_size,
			  &priv_offset);
		if (err != 0) {
			nvgpu_log_fn(g, "Could not determine priv_offset for addr:0x%x",
				      addr); /*, grPriRegStr(addr)));*/
			goto cleanup;
		}

		offsets[i] = priv_offset;
		offset_addrs[i] = priv_registers[i];
	}

	*num_offsets = num_registers;
cleanup:
	if (!IS_ERR_OR_NULL(priv_registers)) {
		nvgpu_kfree(g, priv_registers);
	}

	return err;
}

int gr_gk20a_get_pm_ctx_buffer_offsets(struct gk20a *g,
				       u32 addr,
				       u32 max_offsets,
				       u32 *offsets, u32 *offset_addrs,
				       u32 *num_offsets)
{
	u32 i;
	u32 priv_offset = 0;
	u32 *priv_registers;
	u32 num_registers = 0;
	int err = 0;
	struct gr_gk20a *gr = &g->gr;
	u32 sm_per_tpc = nvgpu_get_litter_value(g, GPU_LIT_NUM_SM_PER_TPC);
	u32 potential_offsets = nvgpu_gr_config_get_max_gpc_count(gr->config) *
		nvgpu_gr_config_get_max_tpc_per_gpc_count(gr->config) *
		sm_per_tpc;

	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg, "addr=0x%x", addr);

	/* implementation is crossed-up if either of these happen */
	if (max_offsets > potential_offsets) {
		return -EINVAL;
	}

	if (!g->gr.ctx_vars.golden_image_initialized) {
		return -ENODEV;
	}

	priv_registers = nvgpu_kzalloc(g, sizeof(u32) * potential_offsets);
	if (priv_registers == NULL) {
		nvgpu_log_fn(g, "failed alloc for potential_offsets=%d", potential_offsets);
		return -ENOMEM;
	}
	(void) memset(offsets,      0, sizeof(u32) * max_offsets);
	(void) memset(offset_addrs, 0, sizeof(u32) * max_offsets);
	*num_offsets = 0;

	g->ops.gr.create_priv_addr_table(g, addr, priv_registers,
			&num_registers);

	if ((max_offsets > 1U) && (num_registers > max_offsets)) {
		err = -EINVAL;
		goto cleanup;
	}

	if ((max_offsets == 1U) && (num_registers > 1U)) {
		num_registers = 1;
	}

	if (!g->gr.ctx_vars.golden_image_initialized) {
		nvgpu_log_fn(g, "no context switch header info to work with");
		err = -EINVAL;
		goto cleanup;
	}

	for (i = 0; i < num_registers; i++) {
		err = nvgpu_gr_hwmp_map_find_priv_offset(g, g->gr.hwpm_map,
						  priv_registers[i],
						  &priv_offset);
		if (err != 0) {
			nvgpu_log_fn(g, "Could not determine priv_offset for addr:0x%x",
				      addr); /*, grPriRegStr(addr)));*/
			goto cleanup;
		}

		offsets[i] = priv_offset;
		offset_addrs[i] = priv_registers[i];
	}

	*num_offsets = num_registers;
cleanup:
	nvgpu_kfree(g, priv_registers);

	return err;
}

/* Setup some register tables.  This looks hacky; our
 * register/offset functions are just that, functions.
 * So they can't be used as initializers... TBD: fix to
 * generate consts at least on an as-needed basis.
 */
static const u32 _num_ovr_perf_regs = 17;
static u32 _ovr_perf_regs[17] = { 0, };
/* Following are the blocks of registers that the ucode
 stores in the extended region.*/

void gk20a_gr_init_ovr_sm_dsm_perf(void)
{
	if (_ovr_perf_regs[0] != 0U) {
		return;
	}

	_ovr_perf_regs[0] = gr_pri_gpc0_tpc0_sm_dsm_perf_counter_control_sel0_r();
	_ovr_perf_regs[1] = gr_pri_gpc0_tpc0_sm_dsm_perf_counter_control_sel1_r();
	_ovr_perf_regs[2] = gr_pri_gpc0_tpc0_sm_dsm_perf_counter_control0_r();
	_ovr_perf_regs[3] = gr_pri_gpc0_tpc0_sm_dsm_perf_counter_control5_r();
	_ovr_perf_regs[4] = gr_pri_gpc0_tpc0_sm_dsm_perf_counter_status1_r();
	_ovr_perf_regs[5] = gr_pri_gpc0_tpc0_sm_dsm_perf_counter0_control_r();
	_ovr_perf_regs[6] = gr_pri_gpc0_tpc0_sm_dsm_perf_counter1_control_r();
	_ovr_perf_regs[7] = gr_pri_gpc0_tpc0_sm_dsm_perf_counter2_control_r();
	_ovr_perf_regs[8] = gr_pri_gpc0_tpc0_sm_dsm_perf_counter3_control_r();
	_ovr_perf_regs[9] = gr_pri_gpc0_tpc0_sm_dsm_perf_counter4_control_r();
	_ovr_perf_regs[10] = gr_pri_gpc0_tpc0_sm_dsm_perf_counter5_control_r();
	_ovr_perf_regs[11] = gr_pri_gpc0_tpc0_sm_dsm_perf_counter6_control_r();
	_ovr_perf_regs[12] = gr_pri_gpc0_tpc0_sm_dsm_perf_counter7_control_r();
	_ovr_perf_regs[13] = gr_pri_gpc0_tpc0_sm_dsm_perf_counter4_r();
	_ovr_perf_regs[14] = gr_pri_gpc0_tpc0_sm_dsm_perf_counter5_r();
	_ovr_perf_regs[15] = gr_pri_gpc0_tpc0_sm_dsm_perf_counter6_r();
	_ovr_perf_regs[16] = gr_pri_gpc0_tpc0_sm_dsm_perf_counter7_r();

}

/* TBD: would like to handle this elsewhere, at a higher level.
 * these are currently constructed in a "test-then-write" style
 * which makes it impossible to know externally whether a ctx
 * write will actually occur. so later we should put a lazy,
 *  map-and-hold system in the patch write state */
static int gr_gk20a_ctx_patch_smpc(struct gk20a *g,
			    struct channel_gk20a *ch,
			    u32 addr, u32 data,
			    struct nvgpu_gr_ctx *gr_ctx)
{
	u32 num_gpc = nvgpu_gr_config_get_gpc_count(g->gr.config);
	u32 num_tpc;
	u32 tpc, gpc, reg;
	u32 chk_addr;
	u32 num_ovr_perf_regs = 0;
	u32 *ovr_perf_regs = NULL;
	u32 gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_GPC_STRIDE);
	u32 tpc_in_gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_TPC_IN_GPC_STRIDE);

	g->ops.gr.init_ovr_sm_dsm_perf();
	g->ops.gr.init_sm_dsm_reg_info();
	g->ops.gr.get_ovr_perf_regs(g, &num_ovr_perf_regs, &ovr_perf_regs);

	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg, "addr=0x%x", addr);

	for (reg = 0; reg < num_ovr_perf_regs; reg++) {
		for (gpc = 0; gpc < num_gpc; gpc++)  {
			num_tpc = nvgpu_gr_config_get_gpc_tpc_count(g->gr.config, gpc);
			for (tpc = 0; tpc < num_tpc; tpc++) {
				chk_addr = ((gpc_stride * gpc) +
					    (tpc_in_gpc_stride * tpc) +
					    ovr_perf_regs[reg]);
				if (chk_addr != addr) {
					continue;
				}
				/* reset the patch count from previous
				   runs,if ucode has already processed
				   it */
				nvgpu_gr_ctx_reset_patch_count(g, gr_ctx);

				nvgpu_gr_ctx_patch_write(g, gr_ctx,
							 addr, data, true);

				if (ch->subctx != NULL) {
					nvgpu_gr_ctx_set_patch_ctx(g, gr_ctx,
						false);
					nvgpu_gr_subctx_set_patch_ctx(g,
						ch->subctx, gr_ctx);
				} else {
					nvgpu_gr_ctx_set_patch_ctx(g, gr_ctx,
						true);
				}

				/* we're not caching these on cpu side,
				   but later watch for it */
				return 0;
			}
		}
	}

	return 0;
}

#define ILLEGAL_ID	~U32(0U)

void gk20a_gr_get_ovr_perf_regs(struct gk20a *g, u32 *num_ovr_perf_regs,
					       u32 **ovr_perf_regs)
{
	*num_ovr_perf_regs = _num_ovr_perf_regs;
	*ovr_perf_regs = _ovr_perf_regs;
}

static int gr_gk20a_find_priv_offset_in_ext_buffer(struct gk20a *g,
						   u32 addr,
						   bool is_quad, u32 quad,
						   u32 *context_buffer,
						   u32 context_buffer_size,
						   u32 *priv_offset)
{
	u32 i;
	u32 gpc_num, tpc_num;
	u32 num_gpcs;
	u32 chk_addr;
	u32 ext_priv_offset, ext_priv_size;
	u8 *context;
	u32 offset_to_segment, offset_to_segment_end;
	u32 sm_dsm_perf_reg_id = ILLEGAL_ID;
	u32 sm_dsm_perf_ctrl_reg_id = ILLEGAL_ID;
	u32 num_ext_gpccs_ext_buffer_segments;
	u32 inter_seg_offset;
	u32 max_tpc_count;
	u32 *sm_dsm_perf_ctrl_regs = NULL;
	u32 num_sm_dsm_perf_ctrl_regs = 0;
	u32 *sm_dsm_perf_regs = NULL;
	u32 num_sm_dsm_perf_regs = 0;
	u32 buffer_segments_size = 0;
	u32 marker_size = 0;
	u32 control_register_stride = 0;
	u32 perf_register_stride = 0;
	struct gr_gk20a *gr = &g->gr;
	u32 gpc_base = nvgpu_get_litter_value(g, GPU_LIT_GPC_BASE);
	u32 gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_GPC_STRIDE);
	u32 tpc_in_gpc_base = nvgpu_get_litter_value(g, GPU_LIT_TPC_IN_GPC_BASE);
	u32 tpc_in_gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_TPC_IN_GPC_STRIDE);
	u32 tpc_gpc_mask = (tpc_in_gpc_stride - 1U);

	/* Only have TPC registers in extended region, so if not a TPC reg,
	   then return error so caller can look elsewhere. */
	if (pri_is_gpc_addr(g, addr))   {
		u32 gpc_addr = 0;
		gpc_num = pri_get_gpc_num(g, addr);
		gpc_addr = pri_gpccs_addr_mask(addr);
		if (g->ops.gr.is_tpc_addr(g, gpc_addr)) {
			tpc_num = g->ops.gr.get_tpc_num(g, gpc_addr);
		} else {
			return -EINVAL;
		}

		nvgpu_log_info(g, " gpc = %d tpc = %d",
				gpc_num, tpc_num);
	} else if ((g->ops.gr.is_etpc_addr != NULL) &&
				g->ops.gr.is_etpc_addr(g, addr)) {
			g->ops.gr.get_egpc_etpc_num(g, addr, &gpc_num, &tpc_num);
			gpc_base = g->ops.gr.get_egpc_base(g);
	} else {
		nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg,
				"does not exist in extended region");
		return -EINVAL;
	}

	buffer_segments_size = g->ops.gr.ctxsw_prog.hw_get_extended_buffer_segments_size_in_bytes();
	/* note below is in words/num_registers */
	marker_size = g->ops.gr.ctxsw_prog.hw_extended_marker_size_in_bytes() >> 2;

	context = (u8 *)context_buffer;
	/* sanity check main header */
	if (!g->ops.gr.ctxsw_prog.check_main_image_header_magic(context)) {
		nvgpu_err(g,
			   "Invalid main header: magic value");
		return -EINVAL;
	}
	num_gpcs = g->ops.gr.ctxsw_prog.get_num_gpcs(context);
	if (gpc_num >= num_gpcs) {
		nvgpu_err(g,
		   "GPC 0x%08x is greater than total count 0x%08x!",
			   gpc_num, num_gpcs);
		return -EINVAL;
	}

	g->ops.gr.ctxsw_prog.get_extended_buffer_size_offset(context,
		&ext_priv_size, &ext_priv_offset);
	if (0U == ext_priv_size) {
		nvgpu_log_info(g, " No extended memory in context buffer");
		return -EINVAL;
	}

	offset_to_segment = ext_priv_offset * 256U;
	offset_to_segment_end = offset_to_segment +
		(ext_priv_size * buffer_segments_size);

	/* check local header magic */
	context += g->ops.gr.ctxsw_prog.hw_get_fecs_header_size();
	if (!g->ops.gr.ctxsw_prog.check_local_header_magic(context)) {
		nvgpu_err(g,
			   "Invalid local header: magic value");
		return -EINVAL;
	}

	/*
	 * See if the incoming register address is in the first table of
	 * registers. We check this by decoding only the TPC addr portion.
	 * If we get a hit on the TPC bit, we then double check the address
	 * by computing it from the base gpc/tpc strides.  Then make sure
	 * it is a real match.
	 */
	g->ops.gr.get_sm_dsm_perf_regs(g, &num_sm_dsm_perf_regs,
				       &sm_dsm_perf_regs,
				       &perf_register_stride);

	g->ops.gr.init_sm_dsm_reg_info();

	for (i = 0; i < num_sm_dsm_perf_regs; i++) {
		if ((addr & tpc_gpc_mask) == (sm_dsm_perf_regs[i] & tpc_gpc_mask)) {
			sm_dsm_perf_reg_id = i;

			nvgpu_log_info(g, "register match: 0x%08x",
					sm_dsm_perf_regs[i]);

			chk_addr = (gpc_base + gpc_stride * gpc_num) +
				   tpc_in_gpc_base +
				   (tpc_in_gpc_stride * tpc_num) +
				   (sm_dsm_perf_regs[sm_dsm_perf_reg_id] & tpc_gpc_mask);

			if (chk_addr != addr) {
				nvgpu_err(g,
				   "Oops addr miss-match! : 0x%08x != 0x%08x",
					   addr, chk_addr);
				return -EINVAL;
			}
			break;
		}
	}

	/* Didn't find reg in supported group 1.
	 *  so try the second group now */
	g->ops.gr.get_sm_dsm_perf_ctrl_regs(g, &num_sm_dsm_perf_ctrl_regs,
				       &sm_dsm_perf_ctrl_regs,
				       &control_register_stride);

	if (ILLEGAL_ID == sm_dsm_perf_reg_id) {
		for (i = 0; i < num_sm_dsm_perf_ctrl_regs; i++) {
			if ((addr & tpc_gpc_mask) ==
			    (sm_dsm_perf_ctrl_regs[i] & tpc_gpc_mask)) {
				sm_dsm_perf_ctrl_reg_id = i;

				nvgpu_log_info(g, "register match: 0x%08x",
						sm_dsm_perf_ctrl_regs[i]);

				chk_addr = (gpc_base + gpc_stride * gpc_num) +
					   tpc_in_gpc_base +
					   tpc_in_gpc_stride * tpc_num +
					   (sm_dsm_perf_ctrl_regs[sm_dsm_perf_ctrl_reg_id] &
					    tpc_gpc_mask);

				if (chk_addr != addr) {
					nvgpu_err(g,
						   "Oops addr miss-match! : 0x%08x != 0x%08x",
						   addr, chk_addr);
					return -EINVAL;

				}

				break;
			}
		}
	}

	if ((ILLEGAL_ID == sm_dsm_perf_ctrl_reg_id) &&
	    (ILLEGAL_ID == sm_dsm_perf_reg_id)) {
		return -EINVAL;
	}

	/* Skip the FECS extended header, nothing there for us now. */
	offset_to_segment += buffer_segments_size;

	/* skip through the GPCCS extended headers until we get to the data for
	 * our GPC.  The size of each gpc extended segment is enough to hold the
	 * max tpc count for the gpcs,in 256b chunks.
	 */

	max_tpc_count = nvgpu_gr_config_get_max_tpc_per_gpc_count(gr->config);

	num_ext_gpccs_ext_buffer_segments = (u32)((max_tpc_count + 1U) / 2U);

	offset_to_segment += (num_ext_gpccs_ext_buffer_segments *
			      buffer_segments_size * gpc_num);

	/* skip the head marker to start with */
	inter_seg_offset = marker_size;

	if (ILLEGAL_ID != sm_dsm_perf_ctrl_reg_id) {
		/* skip over control regs of TPC's before the one we want.
		 *  then skip to the register in this tpc */
		inter_seg_offset = inter_seg_offset +
			(tpc_num * control_register_stride) +
			sm_dsm_perf_ctrl_reg_id;
	} else {
		return -EINVAL;
	}

	/* set the offset to the segment offset plus the inter segment offset to
	 *  our register */
	offset_to_segment += (inter_seg_offset * 4U);

	/* last sanity check: did we somehow compute an offset outside the
	 * extended buffer? */
	if (offset_to_segment > offset_to_segment_end) {
		nvgpu_err(g,
			   "Overflow ctxsw buffer! 0x%08x > 0x%08x",
			   offset_to_segment, offset_to_segment_end);
		return -EINVAL;
	}

	*priv_offset = offset_to_segment;

	return 0;
}


static int
gr_gk20a_process_context_buffer_priv_segment(struct gk20a *g,
					     enum ctxsw_addr_type addr_type,
					     u32 pri_addr,
					     u32 gpc_num, u32 num_tpcs,
					     u32 num_ppcs, u32 ppc_mask,
					     u32 *priv_offset)
{
	u32 i;
	u32 address, base_address;
	u32 sys_offset, gpc_offset, tpc_offset, ppc_offset;
	u32 ppc_num, tpc_num, tpc_addr, gpc_addr, ppc_addr;
	struct netlist_aiv *reg;
	u32 gpc_base = nvgpu_get_litter_value(g, GPU_LIT_GPC_BASE);
	u32 gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_GPC_STRIDE);
	u32 ppc_in_gpc_base = nvgpu_get_litter_value(g, GPU_LIT_PPC_IN_GPC_BASE);
	u32 ppc_in_gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_PPC_IN_GPC_STRIDE);
	u32 tpc_in_gpc_base = nvgpu_get_litter_value(g, GPU_LIT_TPC_IN_GPC_BASE);
	u32 tpc_in_gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_TPC_IN_GPC_STRIDE);

	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg, "pri_addr=0x%x", pri_addr);

	if (!g->netlist_valid) {
		return -EINVAL;
	}

	/* Process the SYS/BE segment. */
	if ((addr_type == CTXSW_ADDR_TYPE_SYS) ||
	    (addr_type == CTXSW_ADDR_TYPE_BE)) {
		for (i = 0; i < g->netlist_vars->ctxsw_regs.sys.count; i++) {
			reg = &g->netlist_vars->ctxsw_regs.sys.l[i];
			address    = reg->addr;
			sys_offset = reg->index;

			if (pri_addr == address) {
				*priv_offset = sys_offset;
				return 0;
			}
		}
	}

	/* Process the TPC segment. */
	if (addr_type == CTXSW_ADDR_TYPE_TPC) {
		for (tpc_num = 0; tpc_num < num_tpcs; tpc_num++) {
			for (i = 0; i < g->netlist_vars->ctxsw_regs.tpc.count; i++) {
				reg = &g->netlist_vars->ctxsw_regs.tpc.l[i];
				address = reg->addr;
				tpc_addr = pri_tpccs_addr_mask(address);
				base_address = gpc_base +
					(gpc_num * gpc_stride) +
					tpc_in_gpc_base +
					(tpc_num * tpc_in_gpc_stride);
				address = base_address + tpc_addr;
				/*
				 * The data for the TPCs is interleaved in the context buffer.
				 * Example with num_tpcs = 2
				 * 0    1    2    3    4    5    6    7    8    9    10   11 ...
				 * 0-0  1-0  0-1  1-1  0-2  1-2  0-3  1-3  0-4  1-4  0-5  1-5 ...
				 */
				tpc_offset = (reg->index * num_tpcs) + (tpc_num * 4U);

				if (pri_addr == address) {
					*priv_offset = tpc_offset;
					return 0;
				}
			}
		}
	} else if ((addr_type == CTXSW_ADDR_TYPE_EGPC) ||
		(addr_type == CTXSW_ADDR_TYPE_ETPC)) {
		if (g->ops.gr.get_egpc_base == NULL) {
			return -EINVAL;
		}

		for (tpc_num = 0; tpc_num < num_tpcs; tpc_num++) {
			for (i = 0; i < g->netlist_vars->ctxsw_regs.etpc.count; i++) {
				reg = &g->netlist_vars->ctxsw_regs.etpc.l[i];
				address = reg->addr;
				tpc_addr = pri_tpccs_addr_mask(address);
				base_address = g->ops.gr.get_egpc_base(g) +
					(gpc_num * gpc_stride) +
					tpc_in_gpc_base +
					(tpc_num * tpc_in_gpc_stride);
				address = base_address + tpc_addr;
				/*
				 * The data for the TPCs is interleaved in the context buffer.
				 * Example with num_tpcs = 2
				 * 0    1    2    3    4    5    6    7    8    9    10   11 ...
				 * 0-0  1-0  0-1  1-1  0-2  1-2  0-3  1-3  0-4  1-4  0-5  1-5 ...
				 */
				tpc_offset = (reg->index * num_tpcs) + (tpc_num * 4U);

				if (pri_addr == address) {
					*priv_offset = tpc_offset;
					nvgpu_log(g,
						gpu_dbg_fn | gpu_dbg_gpu_dbg,
						"egpc/etpc priv_offset=0x%#08x",
						*priv_offset);
					return 0;
				}
			}
		}
	}


	/* Process the PPC segment. */
	if (addr_type == CTXSW_ADDR_TYPE_PPC) {
		for (ppc_num = 0; ppc_num < num_ppcs; ppc_num++) {
			for (i = 0; i < g->netlist_vars->ctxsw_regs.ppc.count; i++) {
				reg = &g->netlist_vars->ctxsw_regs.ppc.l[i];
				address = reg->addr;
				ppc_addr = pri_ppccs_addr_mask(address);
				base_address = gpc_base +
					(gpc_num * gpc_stride) +
					ppc_in_gpc_base +
					(ppc_num * ppc_in_gpc_stride);
				address = base_address + ppc_addr;
				/*
				 * The data for the PPCs is interleaved in the context buffer.
				 * Example with numPpcs = 2
				 * 0    1    2    3    4    5    6    7    8    9    10   11 ...
				 * 0-0  1-0  0-1  1-1  0-2  1-2  0-3  1-3  0-4  1-4  0-5  1-5 ...
				 */
				ppc_offset = (reg->index * num_ppcs) + (ppc_num * 4U);

				if (pri_addr == address)  {
					*priv_offset = ppc_offset;
					return 0;
				}
			}
		}
	}


	/* Process the GPC segment. */
	if (addr_type == CTXSW_ADDR_TYPE_GPC) {
		for (i = 0; i < g->netlist_vars->ctxsw_regs.gpc.count; i++) {
			reg = &g->netlist_vars->ctxsw_regs.gpc.l[i];

			address = reg->addr;
			gpc_addr = pri_gpccs_addr_mask(address);
			gpc_offset = reg->index;

			base_address = gpc_base + (gpc_num * gpc_stride);
			address = base_address + gpc_addr;

			if (pri_addr == address) {
				*priv_offset = gpc_offset;
				return 0;
			}
		}
	}
	return -EINVAL;
}

static int gr_gk20a_determine_ppc_configuration(struct gk20a *g,
					       u8 *context,
					       u32 *num_ppcs, u32 *ppc_mask,
					       u32 *reg_ppc_count)
{
	u32 num_pes_per_gpc = nvgpu_get_litter_value(g, GPU_LIT_NUM_PES_PER_GPC);

	/*
	 * if there is only 1 PES_PER_GPC, then we put the PES registers
	 * in the GPC reglist, so we can't error out if ppc.count == 0
	 */
	if ((!g->netlist_valid) ||
	    ((g->netlist_vars->ctxsw_regs.ppc.count == 0U) &&
	     (num_pes_per_gpc > 1U))) {
		return -EINVAL;
	}

	g->ops.gr.ctxsw_prog.get_ppc_info(context, num_ppcs, ppc_mask);
	*reg_ppc_count = g->netlist_vars->ctxsw_regs.ppc.count;

	return 0;
}

int gr_gk20a_get_offset_in_gpccs_segment(struct gk20a *g,
					enum ctxsw_addr_type addr_type,
					u32 num_tpcs,
					u32 num_ppcs,
					u32 reg_list_ppc_count,
					u32 *__offset_in_segment)
{
	u32 offset_in_segment = 0;

	if (addr_type == CTXSW_ADDR_TYPE_TPC) {
		/*
		 * reg = g->netlist_vars->ctxsw_regs.tpc.l;
		 * offset_in_segment = 0;
		 */
	} else if ((addr_type == CTXSW_ADDR_TYPE_EGPC) ||
			(addr_type == CTXSW_ADDR_TYPE_ETPC)) {
		offset_in_segment =
			((g->netlist_vars->ctxsw_regs.tpc.count *
				num_tpcs) << 2);

		nvgpu_log(g, gpu_dbg_info | gpu_dbg_gpu_dbg,
			"egpc etpc offset_in_segment 0x%#08x",
			offset_in_segment);
	} else if (addr_type == CTXSW_ADDR_TYPE_PPC) {
		/*
		 * The ucode stores TPC data before PPC data.
		 * Advance offset past TPC data to PPC data.
		 */
		offset_in_segment =
			(((g->netlist_vars->ctxsw_regs.tpc.count +
				g->netlist_vars->ctxsw_regs.etpc.count) *
			  num_tpcs) << 2);
	} else if (addr_type == CTXSW_ADDR_TYPE_GPC) {
		/*
		 * The ucode stores TPC/PPC data before GPC data.
		 * Advance offset past TPC/PPC data to GPC data.
		 *
		 * Note 1 PES_PER_GPC case
		 */
		u32 num_pes_per_gpc = nvgpu_get_litter_value(g,
				GPU_LIT_NUM_PES_PER_GPC);
		if (num_pes_per_gpc > 1U) {
			offset_in_segment =
				((((g->netlist_vars->ctxsw_regs.tpc.count +
					g->netlist_vars->ctxsw_regs.etpc.count) *
					num_tpcs) << 2) +
					((reg_list_ppc_count * num_ppcs) << 2));
		} else {
			offset_in_segment =
				(((g->netlist_vars->ctxsw_regs.tpc.count +
					g->netlist_vars->ctxsw_regs.etpc.count) *
					num_tpcs) << 2);
		}
	} else {
		nvgpu_log_fn(g, "Unknown address type.");
		return -EINVAL;
	}

	*__offset_in_segment = offset_in_segment;
	return 0;
}

/*
 *  This function will return the 32 bit offset for a priv register if it is
 *  present in the context buffer. The context buffer is in CPU memory.
 */
static int gr_gk20a_find_priv_offset_in_buffer(struct gk20a *g,
					       u32 addr,
					       bool is_quad, u32 quad,
					       u32 *context_buffer,
					       u32 context_buffer_size,
					       u32 *priv_offset)
{
	u32 i;
	int err;
	enum ctxsw_addr_type addr_type;
	u32 broadcast_flags;
	u32 gpc_num, tpc_num, ppc_num, be_num;
	u32 num_gpcs, num_tpcs, num_ppcs;
	u32 offset;
	u32 sys_priv_offset, gpc_priv_offset;
	u32 ppc_mask, reg_list_ppc_count;
	u8 *context;
	u32 offset_to_segment, offset_in_segment = 0;

	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg, "addr=0x%x", addr);

	err = g->ops.gr.decode_priv_addr(g, addr, &addr_type,
					&gpc_num, &tpc_num, &ppc_num, &be_num,
					&broadcast_flags);
	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg,
			"addr_type = %d, broadcast_flags: %08x",
			addr_type, broadcast_flags);
	if (err != 0) {
		return err;
	}

	context = (u8 *)context_buffer;
	if (!g->ops.gr.ctxsw_prog.check_main_image_header_magic(context)) {
		nvgpu_err(g,
			   "Invalid main header: magic value");
		return -EINVAL;
	}
	num_gpcs = g->ops.gr.ctxsw_prog.get_num_gpcs(context);

	/* Parse the FECS local header. */
	context += g->ops.gr.ctxsw_prog.hw_get_fecs_header_size();
	if (!g->ops.gr.ctxsw_prog.check_local_header_magic(context)) {
		nvgpu_err(g,
			   "Invalid FECS local header: magic value");
		return -EINVAL;
	}

	sys_priv_offset =
	       g->ops.gr.ctxsw_prog.get_local_priv_register_ctl_offset(context);
	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg, "sys_priv_offset=0x%x", sys_priv_offset);

	/* If found in Ext buffer, ok.
	 * If it failed and we expected to find it there (quad offset)
	 * then return the error.  Otherwise continue on.
	 */
	err = gr_gk20a_find_priv_offset_in_ext_buffer(g,
				      addr, is_quad, quad, context_buffer,
				      context_buffer_size, priv_offset);
	if ((err == 0) || ((err != 0) && is_quad)) {
		nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg,
				"err = %d, is_quad = %s",
				err, is_quad ? "true" : "false");
		return err;
	}

	if ((addr_type == CTXSW_ADDR_TYPE_SYS) ||
	    (addr_type == CTXSW_ADDR_TYPE_BE)) {
		/* Find the offset in the FECS segment. */
		offset_to_segment = sys_priv_offset * 256U;

		err = gr_gk20a_process_context_buffer_priv_segment(g,
					   addr_type, addr,
					   0, 0, 0, 0,
					   &offset);
		if (err != 0) {
			return err;
		}

		*priv_offset = (offset_to_segment + offset);
		return 0;
	}

	if ((gpc_num + 1U) > num_gpcs)  {
		nvgpu_err(g,
			   "GPC %d not in this context buffer.",
			   gpc_num);
		return -EINVAL;
	}

	/* Parse the GPCCS local header(s).*/
	for (i = 0; i < num_gpcs; i++) {
		context += g->ops.gr.ctxsw_prog.hw_get_gpccs_header_size();
		if (!g->ops.gr.ctxsw_prog.check_local_header_magic(context)) {
			nvgpu_err(g,
				   "Invalid GPCCS local header: magic value");
			return -EINVAL;

		}
		gpc_priv_offset = g->ops.gr.ctxsw_prog.get_local_priv_register_ctl_offset(context);

		err = gr_gk20a_determine_ppc_configuration(g, context,
							   &num_ppcs, &ppc_mask,
							   &reg_list_ppc_count);
		if (err != 0) {
			nvgpu_err(g, "determine ppc configuration failed");
			return err;
		}


		num_tpcs = g->ops.gr.ctxsw_prog.get_num_tpcs(context);

		if ((i == gpc_num) && ((tpc_num + 1U) > num_tpcs)) {
			nvgpu_err(g,
			   "GPC %d TPC %d not in this context buffer.",
				   gpc_num, tpc_num);
			return -EINVAL;
		}

		/* Find the offset in the GPCCS segment.*/
		if (i == gpc_num) {
			nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg,
					"gpc_priv_offset 0x%#08x",
					gpc_priv_offset);
			offset_to_segment = gpc_priv_offset * 256U;

			err = g->ops.gr.get_offset_in_gpccs_segment(g,
					addr_type,
					num_tpcs, num_ppcs, reg_list_ppc_count,
					&offset_in_segment);
			if (err != 0) {
				return -EINVAL;
			}

			offset_to_segment += offset_in_segment;
			nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg,
				"offset_to_segment 0x%#08x",
				offset_to_segment);

			err = gr_gk20a_process_context_buffer_priv_segment(g,
							   addr_type, addr,
							   i, num_tpcs,
							   num_ppcs, ppc_mask,
							   &offset);
			if (err != 0) {
				return -EINVAL;
			}

			*priv_offset = offset_to_segment + offset;
			return 0;
		}
	}

	return -EINVAL;
}

bool gk20a_is_channel_ctx_resident(struct channel_gk20a *ch)
{
	u32 curr_gr_ctx;
	u32 curr_gr_tsgid;
	struct gk20a *g = ch->g;
	struct channel_gk20a *curr_ch;
	bool ret = false;
	struct tsg_gk20a *tsg;

	curr_gr_ctx  = gk20a_readl(g, gr_fecs_current_ctx_r());

	/* when contexts are unloaded from GR, the valid bit is reset
	 * but the instance pointer information remains intact. So the
	 * valid bit must be checked to be absolutely certain that a
	 * valid context is currently resident.
	 */
	if (gr_fecs_current_ctx_valid_v(curr_gr_ctx) == 0U) {
		return false;
	}

	curr_ch = gk20a_gr_get_channel_from_ctx(g, curr_gr_ctx,
					      &curr_gr_tsgid);

	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg,
		  "curr_gr_chid=%d curr_tsgid=%d, ch->tsgid=%d"
		  " ch->chid=%d",
		  (curr_ch != NULL) ? curr_ch->chid : U32_MAX,
		  curr_gr_tsgid,
		  ch->tsgid,
		  ch->chid);

	if (curr_ch == NULL) {
		return false;
	}

	if (ch->chid == curr_ch->chid) {
		ret = true;
	}

	tsg = tsg_gk20a_from_ch(ch);
	if ((tsg != NULL) && (tsg->tsgid == curr_gr_tsgid)) {
		ret = true;
	}

	gk20a_channel_put(curr_ch);
	return ret;
}

int __gr_gk20a_exec_ctx_ops(struct channel_gk20a *ch,
			    struct nvgpu_dbg_reg_op *ctx_ops, u32 num_ops,
			    u32 num_ctx_wr_ops, u32 num_ctx_rd_ops,
			    bool ch_is_curr_ctx)
{
	struct gk20a *g = ch->g;
	struct tsg_gk20a *tsg;
	struct nvgpu_gr_ctx *gr_ctx;
	bool gr_ctx_ready = false;
	bool pm_ctx_ready = false;
	struct nvgpu_mem *current_mem = NULL;
	u32 i, j, offset, v;
	struct gr_gk20a *gr = &g->gr;
	u32 sm_per_tpc = nvgpu_get_litter_value(g, GPU_LIT_NUM_SM_PER_TPC);
	u32 max_offsets = nvgpu_gr_config_get_max_gpc_count(gr->config) *
		nvgpu_gr_config_get_max_tpc_per_gpc_count(gr->config) *
		sm_per_tpc;
	u32 *offsets = NULL;
	u32 *offset_addrs = NULL;
	u32 ctx_op_nr, num_ctx_ops[2] = {num_ctx_wr_ops, num_ctx_rd_ops};
	int err = 0, pass;

	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg, "wr_ops=%d rd_ops=%d",
		   num_ctx_wr_ops, num_ctx_rd_ops);

	tsg = tsg_gk20a_from_ch(ch);
	if (tsg == NULL) {
		return -EINVAL;
	}

	gr_ctx = tsg->gr_ctx;

	if (ch_is_curr_ctx) {
		for (pass = 0; pass < 2; pass++) {
			ctx_op_nr = 0;
			for (i = 0; (ctx_op_nr < num_ctx_ops[pass]) && (i < num_ops); ++i) {
				/* only do ctx ops and only on the right pass */
				if ((ctx_ops[i].type == REGOP(TYPE_GLOBAL)) ||
				    (((pass == 0) && reg_op_is_read(ctx_ops[i].op)) ||
				     ((pass == 1) && !reg_op_is_read(ctx_ops[i].op)))) {
					continue;
				}

				/* if this is a quad access, setup for special access*/
				if ((ctx_ops[i].type == REGOP(TYPE_GR_CTX_QUAD))
					&& (g->ops.gr.access_smpc_reg != NULL)) {
					g->ops.gr.access_smpc_reg(g,
							ctx_ops[i].quad,
							ctx_ops[i].offset);
				}
				offset = ctx_ops[i].offset;

				if (pass == 0) { /* write pass */
					v = gk20a_readl(g, offset);
					v &= ~ctx_ops[i].and_n_mask_lo;
					v |= ctx_ops[i].value_lo;
					gk20a_writel(g, offset, v);

					nvgpu_log(g, gpu_dbg_gpu_dbg,
						   "direct wr: offset=0x%x v=0x%x",
						   offset, v);

					if (ctx_ops[i].op == REGOP(WRITE_64)) {
						v = gk20a_readl(g, offset + 4U);
						v &= ~ctx_ops[i].and_n_mask_hi;
						v |= ctx_ops[i].value_hi;
						gk20a_writel(g, offset + 4U, v);

						nvgpu_log(g, gpu_dbg_gpu_dbg,
							   "direct wr: offset=0x%x v=0x%x",
							   offset + 4U, v);
					}

				} else { /* read pass */
					ctx_ops[i].value_lo =
						gk20a_readl(g, offset);

					nvgpu_log(g, gpu_dbg_gpu_dbg,
						   "direct rd: offset=0x%x v=0x%x",
						   offset, ctx_ops[i].value_lo);

					if (ctx_ops[i].op == REGOP(READ_64)) {
						ctx_ops[i].value_hi =
							gk20a_readl(g, offset + 4U);

						nvgpu_log(g, gpu_dbg_gpu_dbg,
							   "direct rd: offset=0x%x v=0x%x",
							   offset, ctx_ops[i].value_lo);
					} else {
						ctx_ops[i].value_hi = 0;
					}
				}
				ctx_op_nr++;
			}
		}
		goto cleanup;
	}

	/* they're the same size, so just use one alloc for both */
	offsets = nvgpu_kzalloc(g, 2U * sizeof(u32) * max_offsets);
	if (offsets == NULL) {
		err = -ENOMEM;
		goto cleanup;
	}
	offset_addrs = offsets + max_offsets;

	err = nvgpu_gr_ctx_patch_write_begin(g, gr_ctx, false);
	if (err != 0) {
		goto cleanup;
	}

	err = g->ops.mm.l2_flush(g, true);
	if (err != 0) {
		nvgpu_err(g, "l2_flush failed");
		goto cleanup;
	}

	/* write to appropriate place in context image,
	 * first have to figure out where that really is */

	/* first pass is writes, second reads */
	for (pass = 0; pass < 2; pass++) {
		ctx_op_nr = 0;
		for (i = 0; (ctx_op_nr < num_ctx_ops[pass]) && (i < num_ops); ++i) {
			u32 num_offsets;

			/* only do ctx ops and only on the right pass */
			if ((ctx_ops[i].type == REGOP(TYPE_GLOBAL)) ||
			    (((pass == 0) && reg_op_is_read(ctx_ops[i].op)) ||
			     ((pass == 1) && !reg_op_is_read(ctx_ops[i].op)))) {
				continue;
			}

			err = gr_gk20a_get_ctx_buffer_offsets(g,
						ctx_ops[i].offset,
						max_offsets,
						offsets, offset_addrs,
						&num_offsets,
						ctx_ops[i].type == REGOP(TYPE_GR_CTX_QUAD),
						ctx_ops[i].quad);
			if (err == 0) {
				if (!gr_ctx_ready) {
					gr_ctx_ready = true;
				}
				current_mem = &gr_ctx->mem;
			} else {
				err = gr_gk20a_get_pm_ctx_buffer_offsets(g,
							ctx_ops[i].offset,
							max_offsets,
							offsets, offset_addrs,
							&num_offsets);
				if (err != 0) {
					nvgpu_log(g, gpu_dbg_gpu_dbg,
					   "ctx op invalid offset: offset=0x%x",
					   ctx_ops[i].offset);
					ctx_ops[i].status =
						REGOP(STATUS_INVALID_OFFSET);
					continue;
				}
				if (!pm_ctx_ready) {
					/* Make sure ctx buffer was initialized */
					if (!nvgpu_mem_is_valid(&gr_ctx->pm_ctx.mem)) {
						nvgpu_err(g,
							"Invalid ctx buffer");
						err = -EINVAL;
						goto cleanup;
					}
					pm_ctx_ready = true;
				}
				current_mem = &gr_ctx->pm_ctx.mem;
			}

			/* if this is a quad access, setup for special access*/
			if ((ctx_ops[i].type == REGOP(TYPE_GR_CTX_QUAD)) &&
				(g->ops.gr.access_smpc_reg != NULL)) {
				g->ops.gr.access_smpc_reg(g, ctx_ops[i].quad,
							 ctx_ops[i].offset);
			}

			for (j = 0; j < num_offsets; j++) {
				/* sanity check gr ctxt offsets,
				 * don't write outside, worst case
				 */
				if ((current_mem == &gr_ctx->mem) &&
					(offsets[j] >= g->gr.ctx_vars.golden_image_size)) {
					continue;
				}
				if (pass == 0) { /* write pass */
					v = nvgpu_mem_rd(g, current_mem, offsets[j]);
					v &= ~ctx_ops[i].and_n_mask_lo;
					v |= ctx_ops[i].value_lo;
					nvgpu_mem_wr(g, current_mem, offsets[j], v);

					nvgpu_log(g, gpu_dbg_gpu_dbg,
						   "context wr: offset=0x%x v=0x%x",
						   offsets[j], v);

					if (ctx_ops[i].op == REGOP(WRITE_64)) {
						v = nvgpu_mem_rd(g, current_mem, offsets[j] + 4U);
						v &= ~ctx_ops[i].and_n_mask_hi;
						v |= ctx_ops[i].value_hi;
						nvgpu_mem_wr(g, current_mem, offsets[j] + 4U, v);

						nvgpu_log(g, gpu_dbg_gpu_dbg,
							   "context wr: offset=0x%x v=0x%x",
							   offsets[j] + 4U, v);
					}

					if (current_mem == &gr_ctx->mem) {
						/* check to see if we need to add a special WAR
						   for some of the SMPC perf regs */
						gr_gk20a_ctx_patch_smpc(g, ch,
							offset_addrs[j],
							v, gr_ctx);
					}
				} else { /* read pass */
					ctx_ops[i].value_lo =
						nvgpu_mem_rd(g, current_mem, offsets[0]);

					nvgpu_log(g, gpu_dbg_gpu_dbg, "context rd: offset=0x%x v=0x%x",
						   offsets[0], ctx_ops[i].value_lo);

					if (ctx_ops[i].op == REGOP(READ_64)) {
						ctx_ops[i].value_hi =
							nvgpu_mem_rd(g, current_mem, offsets[0] + 4U);

						nvgpu_log(g, gpu_dbg_gpu_dbg,
							   "context rd: offset=0x%x v=0x%x",
							   offsets[0] + 4U, ctx_ops[i].value_hi);
					} else {
						ctx_ops[i].value_hi = 0;
					}
				}
			}
			ctx_op_nr++;
		}
	}

 cleanup:
	if (offsets != NULL) {
		nvgpu_kfree(g, offsets);
	}

	if (gr_ctx->patch_ctx.mem.cpu_va != NULL) {
		nvgpu_gr_ctx_patch_write_end(g, gr_ctx, gr_ctx_ready);
	}

	return err;
}

int gr_gk20a_exec_ctx_ops(struct channel_gk20a *ch,
			  struct nvgpu_dbg_reg_op *ctx_ops, u32 num_ops,
			  u32 num_ctx_wr_ops, u32 num_ctx_rd_ops,
			  bool *is_curr_ctx)
{
	struct gk20a *g = ch->g;
	int err, tmp_err;
	bool ch_is_curr_ctx;

	/* disable channel switching.
	 * at that point the hardware state can be inspected to
	 * determine if the context we're interested in is current.
	 */
	err = gr_gk20a_disable_ctxsw(g);
	if (err != 0) {
		nvgpu_err(g, "unable to stop gr ctxsw");
		/* this should probably be ctx-fatal... */
		return err;
	}

	ch_is_curr_ctx = gk20a_is_channel_ctx_resident(ch);
	if (is_curr_ctx != NULL) {
		*is_curr_ctx = ch_is_curr_ctx;
	}
	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg, "is curr ctx=%d",
		  ch_is_curr_ctx);

	err = __gr_gk20a_exec_ctx_ops(ch, ctx_ops, num_ops, num_ctx_wr_ops,
				      num_ctx_rd_ops, ch_is_curr_ctx);

	tmp_err = gr_gk20a_enable_ctxsw(g);
	if (tmp_err != 0) {
		nvgpu_err(g, "unable to restart ctxsw!");
		err = tmp_err;
	}

	return err;
}

void gk20a_init_gr(struct gk20a *g)
{
	nvgpu_cond_init(&g->gr.init_wq);
}

int gk20a_gr_wait_for_sm_lock_down(struct gk20a *g, u32 gpc, u32 tpc, u32 sm,
		u32 global_esr_mask, bool check_errors)
{
	bool locked_down;
	bool no_error_pending;
	u32 delay = POLL_DELAY_MIN_US;
	bool mmu_debug_mode_enabled = g->ops.fb.is_debug_mode_enabled(g);
	u32 offset = nvgpu_gr_gpc_offset(g, gpc) + nvgpu_gr_tpc_offset(g, tpc);
	u32 dbgr_status0 = 0, dbgr_control0 = 0;
	u64 warps_valid = 0, warps_paused = 0, warps_trapped = 0;
	struct nvgpu_timeout timeout;
	u32 warp_esr;

	nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg,
		"GPC%d TPC%d SM%d: locking down SM", gpc, tpc, sm);

	nvgpu_timeout_init(g, &timeout, nvgpu_get_poll_timeout(g),
			   NVGPU_TIMER_CPU_TIMER);

	/* wait for the sm to lock down */
	do {
		u32 global_esr = g->ops.gr.get_sm_hww_global_esr(g,
						gpc, tpc, sm);
		dbgr_status0 = gk20a_readl(g,
				gr_gpc0_tpc0_sm_dbgr_status0_r() + offset);

		warp_esr = g->ops.gr.get_sm_hww_warp_esr(g, gpc, tpc, sm);

		locked_down =
		    (gr_gpc0_tpc0_sm_dbgr_status0_locked_down_v(dbgr_status0) ==
		     gr_gpc0_tpc0_sm_dbgr_status0_locked_down_true_v());
		no_error_pending =
			check_errors &&
			(gr_gpc0_tpc0_sm_hww_warp_esr_error_v(warp_esr) ==
			 gr_gpc0_tpc0_sm_hww_warp_esr_error_none_v()) &&
			((global_esr & ~global_esr_mask) == 0U);

		if (locked_down || no_error_pending) {
			nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg,
				  "GPC%d TPC%d SM%d: locked down SM",
					gpc, tpc, sm);
			return 0;
		}

		/* if an mmu fault is pending and mmu debug mode is not
		 * enabled, the sm will never lock down. */
		if (!mmu_debug_mode_enabled &&
		     (g->ops.mm.mmu_fault_pending(g))) {
			nvgpu_err(g,
				"GPC%d TPC%d: mmu fault pending,"
				" SM%d will never lock down!", gpc, tpc, sm);
			return -EFAULT;
		}

		nvgpu_usleep_range(delay, delay * 2U);
		delay = min_t(u32, delay << 1, POLL_DELAY_MAX_US);
	} while (nvgpu_timeout_expired(&timeout) == 0);

	dbgr_control0 = gk20a_readl(g,
				gr_gpc0_tpc0_sm_dbgr_control0_r() + offset);

	/* 64 bit read */
	warps_valid = (u64)gk20a_readl(g, gr_gpc0_tpc0_sm_warp_valid_mask_1_r() + offset) << 32;
	warps_valid |= gk20a_readl(g, gr_gpc0_tpc0_sm_warp_valid_mask_r() + offset);

	/* 64 bit read */
	warps_paused = (u64)gk20a_readl(g, gr_gpc0_tpc0_sm_dbgr_bpt_pause_mask_1_r() + offset) << 32;
	warps_paused |= gk20a_readl(g, gr_gpc0_tpc0_sm_dbgr_bpt_pause_mask_r() + offset);

	/* 64 bit read */
	warps_trapped = (u64)gk20a_readl(g, gr_gpc0_tpc0_sm_dbgr_bpt_trap_mask_1_r() + offset) << 32;
	warps_trapped |= gk20a_readl(g, gr_gpc0_tpc0_sm_dbgr_bpt_trap_mask_r() + offset);

	nvgpu_err(g,
		"GPC%d TPC%d: timed out while trying to lock down SM", gpc, tpc);
	nvgpu_err(g,
		"STATUS0(0x%x)=0x%x CONTROL0=0x%x VALID_MASK=0x%llx PAUSE_MASK=0x%llx TRAP_MASK=0x%llx",
		gr_gpc0_tpc0_sm_dbgr_status0_r() + offset, dbgr_status0, dbgr_control0,
		warps_valid, warps_paused, warps_trapped);

	return -ETIMEDOUT;
}

void gk20a_gr_suspend_single_sm(struct gk20a *g,
		u32 gpc, u32 tpc, u32 sm,
		u32 global_esr_mask, bool check_errors)
{
	int err;
	u32 dbgr_control0;
	u32 offset = nvgpu_gr_gpc_offset(g, gpc) + nvgpu_gr_tpc_offset(g, tpc);

	/* if an SM debugger isn't attached, skip suspend */
	if (!g->ops.gr.sm_debugger_attached(g)) {
		nvgpu_err(g,
			"SM debugger not attached, skipping suspend!");
		return;
	}

	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg,
		"suspending gpc:%d, tpc:%d, sm%d", gpc, tpc, sm);

	/* assert stop trigger. */
	dbgr_control0 = gk20a_readl(g,
				gr_gpc0_tpc0_sm_dbgr_control0_r() + offset);
	dbgr_control0 |= gr_gpcs_tpcs_sm_dbgr_control0_stop_trigger_enable_f();
	gk20a_writel(g, gr_gpc0_tpc0_sm_dbgr_control0_r() + offset,
			dbgr_control0);

	err = g->ops.gr.wait_for_sm_lock_down(g, gpc, tpc, sm,
			global_esr_mask, check_errors);
	if (err != 0) {
		nvgpu_err(g,
			"SuspendSm failed");
		return;
	}
}

void gk20a_gr_suspend_all_sms(struct gk20a *g,
		u32 global_esr_mask, bool check_errors)
{
	struct gr_gk20a *gr = &g->gr;
	u32 gpc, tpc, sm;
	int err;
	u32 dbgr_control0;
	u32 sm_per_tpc = nvgpu_get_litter_value(g, GPU_LIT_NUM_SM_PER_TPC);

	/* if an SM debugger isn't attached, skip suspend */
	if (!g->ops.gr.sm_debugger_attached(g)) {
		nvgpu_err(g,
			"SM debugger not attached, skipping suspend!");
		return;
	}

	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg, "suspending all sms");
	/* assert stop trigger. uniformity assumption: all SMs will have
	 * the same state in dbg_control0.
	 */
	dbgr_control0 =
		gk20a_readl(g, gr_gpc0_tpc0_sm_dbgr_control0_r());
	dbgr_control0 |= gr_gpcs_tpcs_sm_dbgr_control0_stop_trigger_enable_f();

	/* broadcast write */
	gk20a_writel(g,
		gr_gpcs_tpcs_sm_dbgr_control0_r(), dbgr_control0);

	for (gpc = 0; gpc < nvgpu_gr_config_get_gpc_count(gr->config); gpc++) {
		for (tpc = 0;
		     tpc < nvgpu_gr_config_get_gpc_tpc_count(gr->config, gpc);
		     tpc++) {
			for (sm = 0; sm < sm_per_tpc; sm++) {
				err = g->ops.gr.wait_for_sm_lock_down(g,
					gpc, tpc, sm,
					global_esr_mask, check_errors);
				if (err != 0) {
					nvgpu_err(g, "SuspendAllSms failed");
					return;
				}
			}
		}
	}
}

void gk20a_gr_resume_single_sm(struct gk20a *g,
		u32 gpc, u32 tpc, u32 sm)
{
	u32 dbgr_control0;
	u32 offset;
	/*
	 * The following requires some clarification. Despite the fact that both
	 * RUN_TRIGGER and STOP_TRIGGER have the word "TRIGGER" in their
	 *  names, only one is actually a trigger, and that is the STOP_TRIGGER.
	 * Merely writing a 1(_TASK) to the RUN_TRIGGER is not sufficient to
	 * resume the gpu - the _STOP_TRIGGER must explicitly be set to 0
	 * (_DISABLE) as well.

	* Advice from the arch group:  Disable the stop trigger first, as a
	* separate operation, in order to ensure that the trigger has taken
	* effect, before enabling the run trigger.
	*/

	offset = nvgpu_gr_gpc_offset(g, gpc) + nvgpu_gr_tpc_offset(g, tpc);

	/*De-assert stop trigger */
	dbgr_control0 =
		gk20a_readl(g, gr_gpc0_tpc0_sm_dbgr_control0_r() + offset);
	dbgr_control0 = set_field(dbgr_control0,
			gr_gpcs_tpcs_sm_dbgr_control0_stop_trigger_m(),
			gr_gpcs_tpcs_sm_dbgr_control0_stop_trigger_disable_f());
	gk20a_writel(g,
		gr_gpc0_tpc0_sm_dbgr_control0_r() + offset, dbgr_control0);

	/* Run trigger */
	dbgr_control0 |= gr_gpcs_tpcs_sm_dbgr_control0_run_trigger_task_f();
	gk20a_writel(g,
		gr_gpc0_tpc0_sm_dbgr_control0_r() + offset, dbgr_control0);
}

void gk20a_gr_resume_all_sms(struct gk20a *g)
{
	u32 dbgr_control0;
	/*
	 * The following requires some clarification. Despite the fact that both
	 * RUN_TRIGGER and STOP_TRIGGER have the word "TRIGGER" in their
	 *  names, only one is actually a trigger, and that is the STOP_TRIGGER.
	 * Merely writing a 1(_TASK) to the RUN_TRIGGER is not sufficient to
	 * resume the gpu - the _STOP_TRIGGER must explicitly be set to 0
	 * (_DISABLE) as well.

	* Advice from the arch group:  Disable the stop trigger first, as a
	* separate operation, in order to ensure that the trigger has taken
	* effect, before enabling the run trigger.
	*/

	/*De-assert stop trigger */
	dbgr_control0 =
		gk20a_readl(g, gr_gpcs_tpcs_sm_dbgr_control0_r());
	dbgr_control0 &= ~gr_gpcs_tpcs_sm_dbgr_control0_stop_trigger_enable_f();
	gk20a_writel(g,
		gr_gpcs_tpcs_sm_dbgr_control0_r(), dbgr_control0);

	/* Run trigger */
	dbgr_control0 |= gr_gpcs_tpcs_sm_dbgr_control0_run_trigger_task_f();
	gk20a_writel(g,
		gr_gpcs_tpcs_sm_dbgr_control0_r(), dbgr_control0);
}

int gr_gk20a_set_sm_debug_mode(struct gk20a *g,
	struct channel_gk20a *ch, u64 sms, bool enable)
{
	struct nvgpu_dbg_reg_op *ops;
	unsigned int i = 0, sm_id;
	int err;
	u32 gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_GPC_STRIDE);
	u32 tpc_in_gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_TPC_IN_GPC_STRIDE);
	u32 no_of_sm = nvgpu_gr_config_get_no_of_sm(g->gr.config);

	ops = nvgpu_kcalloc(g, no_of_sm, sizeof(*ops));
	if (ops == NULL) {
		return -ENOMEM;
	}

	for (sm_id = 0; sm_id < no_of_sm; sm_id++) {
		u32 gpc, tpc;
		u32 tpc_offset, gpc_offset, reg_offset, reg_mask, reg_val;
		struct sm_info *sm_info;

		if ((sms & BIT64(sm_id)) == 0ULL) {
			continue;
		}
		sm_info = nvgpu_gr_config_get_sm_info(g->gr.config, sm_id);
		gpc = sm_info->gpc_index;
		tpc = sm_info->tpc_index;

		tpc_offset = tpc_in_gpc_stride * tpc;
		gpc_offset = gpc_stride * gpc;
		reg_offset = tpc_offset + gpc_offset;

		ops[i].op = REGOP(WRITE_32);
		ops[i].type = REGOP(TYPE_GR_CTX);
		ops[i].offset  = gr_gpc0_tpc0_sm_dbgr_control0_r() + reg_offset;

		reg_mask = 0;
		reg_val = 0;
		if (enable) {
			reg_mask |= gr_gpc0_tpc0_sm_dbgr_control0_debugger_mode_m();
			reg_val |= gr_gpc0_tpc0_sm_dbgr_control0_debugger_mode_on_f();
			reg_mask |= gr_gpc0_tpc0_sm_dbgr_control0_stop_on_any_warp_m();
			reg_val |= gr_gpc0_tpc0_sm_dbgr_control0_stop_on_any_warp_disable_f();
			reg_mask |= gr_gpc0_tpc0_sm_dbgr_control0_stop_on_any_sm_m();
			reg_val |= gr_gpc0_tpc0_sm_dbgr_control0_stop_on_any_sm_disable_f();
		} else {
			reg_mask |= gr_gpc0_tpc0_sm_dbgr_control0_debugger_mode_m();
			reg_val |= gr_gpc0_tpc0_sm_dbgr_control0_debugger_mode_off_f();
		}

		ops[i].and_n_mask_lo = reg_mask;
		ops[i].value_lo = reg_val;
		i++;
	}

	err = gr_gk20a_exec_ctx_ops(ch, ops, i, i, 0, NULL);
	if (err != 0) {
		nvgpu_err(g, "Failed to access register");
	}
	nvgpu_kfree(g, ops);
	return err;
}

/*
 * gr_gk20a_suspend_context()
 * This API should be called with dbg_session lock held
 * and ctxsw disabled
 * Returns bool value indicating if context was resident
 * or not
 */
bool gr_gk20a_suspend_context(struct channel_gk20a *ch)
{
	struct gk20a *g = ch->g;
	bool ctx_resident = false;

	if (gk20a_is_channel_ctx_resident(ch)) {
		g->ops.gr.suspend_all_sms(g, 0, false);
		ctx_resident = true;
	} else {
		gk20a_disable_channel_tsg(g, ch);
	}

	return ctx_resident;
}

bool gr_gk20a_resume_context(struct channel_gk20a *ch)
{
	struct gk20a *g = ch->g;
	bool ctx_resident = false;

	if (gk20a_is_channel_ctx_resident(ch)) {
		g->ops.gr.resume_all_sms(g);
		ctx_resident = true;
	} else {
		gk20a_enable_channel_tsg(g, ch);
	}

	return ctx_resident;
}

int gr_gk20a_suspend_contexts(struct gk20a *g,
			      struct dbg_session_gk20a *dbg_s,
			      int *ctx_resident_ch_fd)
{
	int local_ctx_resident_ch_fd = -1;
	bool ctx_resident;
	struct channel_gk20a *ch;
	struct dbg_session_channel_data *ch_data;
	int err = 0;

	nvgpu_mutex_acquire(&g->dbg_sessions_lock);

	err = gr_gk20a_disable_ctxsw(g);
	if (err != 0) {
		nvgpu_err(g, "unable to stop gr ctxsw");
		goto clean_up;
	}

	nvgpu_mutex_acquire(&dbg_s->ch_list_lock);

	nvgpu_list_for_each_entry(ch_data, &dbg_s->ch_list,
			dbg_session_channel_data, ch_entry) {
		ch = g->fifo.channel + ch_data->chid;

		ctx_resident = gr_gk20a_suspend_context(ch);
		if (ctx_resident) {
			local_ctx_resident_ch_fd = ch_data->channel_fd;
		}
	}

	nvgpu_mutex_release(&dbg_s->ch_list_lock);

	err = gr_gk20a_enable_ctxsw(g);
	if (err != 0) {
		nvgpu_err(g, "unable to restart ctxsw!");
	}

	*ctx_resident_ch_fd = local_ctx_resident_ch_fd;

clean_up:
	nvgpu_mutex_release(&g->dbg_sessions_lock);

	return err;
}

int gr_gk20a_resume_contexts(struct gk20a *g,
			      struct dbg_session_gk20a *dbg_s,
			      int *ctx_resident_ch_fd)
{
	int local_ctx_resident_ch_fd = -1;
	bool ctx_resident;
	struct channel_gk20a *ch;
	int err = 0;
	struct dbg_session_channel_data *ch_data;

	nvgpu_mutex_acquire(&g->dbg_sessions_lock);

	err = gr_gk20a_disable_ctxsw(g);
	if (err != 0) {
		nvgpu_err(g, "unable to stop gr ctxsw");
		goto clean_up;
	}

	nvgpu_list_for_each_entry(ch_data, &dbg_s->ch_list,
			dbg_session_channel_data, ch_entry) {
		ch = g->fifo.channel + ch_data->chid;

		ctx_resident = gr_gk20a_resume_context(ch);
		if (ctx_resident) {
			local_ctx_resident_ch_fd = ch_data->channel_fd;
		}
	}

	err = gr_gk20a_enable_ctxsw(g);
	if (err != 0) {
		nvgpu_err(g, "unable to restart ctxsw!");
	}

	*ctx_resident_ch_fd = local_ctx_resident_ch_fd;

clean_up:
	nvgpu_mutex_release(&g->dbg_sessions_lock);

	return err;
}

int gr_gk20a_trigger_suspend(struct gk20a *g)
{
	int err = 0;
	u32 dbgr_control0;

	/* assert stop trigger. uniformity assumption: all SMs will have
	 * the same state in dbg_control0. */
	dbgr_control0 =
		gk20a_readl(g, gr_gpc0_tpc0_sm_dbgr_control0_r());
	dbgr_control0 |= gr_gpcs_tpcs_sm_dbgr_control0_stop_trigger_enable_f();

	/* broadcast write */
	gk20a_writel(g,
		gr_gpcs_tpcs_sm_dbgr_control0_r(), dbgr_control0);

	return err;
}

int gr_gk20a_wait_for_pause(struct gk20a *g, struct nvgpu_warpstate *w_state)
{
	int err = 0;
	struct gr_gk20a *gr = &g->gr;
	u32 gpc, tpc, sm, sm_id;
	u32 global_mask;
	u32 no_of_sm = nvgpu_gr_config_get_no_of_sm(gr->config);

	/* Wait for the SMs to reach full stop. This condition is:
	 * 1) All SMs with valid warps must be in the trap handler (SM_IN_TRAP_MODE)
	 * 2) All SMs in the trap handler must have equivalent VALID and PAUSED warp
	 *    masks.
	*/
	global_mask = g->ops.gr.get_sm_no_lock_down_hww_global_esr_mask(g);

	/* Lock down all SMs */
	for (sm_id = 0; sm_id < no_of_sm; sm_id++) {
		struct sm_info *sm_info =
			nvgpu_gr_config_get_sm_info(g->gr.config, sm_id);
		gpc = sm_info->gpc_index;
		tpc = sm_info->tpc_index;
		sm = sm_info->sm_index;

		err = g->ops.gr.lock_down_sm(g, gpc, tpc, sm,
				global_mask, false);
		if (err != 0) {
			nvgpu_err(g, "sm did not lock down!");
			return err;
		}
	}

	/* Read the warp status */
	g->ops.gr.bpt_reg_info(g, w_state);

	return 0;
}

int gr_gk20a_resume_from_pause(struct gk20a *g)
{
	int err = 0;
	u32 reg_val;

	/* Clear the pause mask to tell the GPU we want to resume everyone */
	gk20a_writel(g,
		gr_gpcs_tpcs_sm_dbgr_bpt_pause_mask_r(), 0);

	/* explicitly re-enable forwarding of SM interrupts upon any resume */
	reg_val = gk20a_readl(g, gr_gpc0_tpc0_tpccs_tpc_exception_en_r());
	reg_val |= gr_gpc0_tpc0_tpccs_tpc_exception_en_sm_enabled_f();
	gk20a_writel(g, gr_gpcs_tpcs_tpccs_tpc_exception_en_r(), reg_val);

	/* Now resume all sms, write a 0 to the stop trigger
	 * then a 1 to the run trigger */
	g->ops.gr.resume_all_sms(g);

	return err;
}

int gr_gk20a_clear_sm_errors(struct gk20a *g)
{
	int ret = 0;
	u32 gpc, tpc, sm;
	struct gr_gk20a *gr = &g->gr;
	u32 global_esr;
	u32 sm_per_tpc = nvgpu_get_litter_value(g, GPU_LIT_NUM_SM_PER_TPC);

	for (gpc = 0; gpc < nvgpu_gr_config_get_gpc_count(gr->config); gpc++) {

		/* check if any tpc has an exception */
		for (tpc = 0;
		     tpc < nvgpu_gr_config_get_gpc_tpc_count(gr->config, gpc);
		     tpc++) {

			for (sm = 0; sm < sm_per_tpc; sm++) {
				global_esr = g->ops.gr.get_sm_hww_global_esr(g,
							 gpc, tpc, sm);

				/* clearing hwws, also causes tpc and gpc
				 * exceptions to be cleared
				 */
				g->ops.gr.clear_sm_hww(g,
					gpc, tpc, sm, global_esr);
			}
		}
	}

	return ret;
}

u32 gr_gk20a_tpc_enabled_exceptions(struct gk20a *g)
{
	struct gr_gk20a *gr = &g->gr;
	u32 sm_id, tpc_exception_en = 0;
	u32 offset, regval, tpc_offset, gpc_offset;
	u32 gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_GPC_STRIDE);
	u32 tpc_in_gpc_stride = nvgpu_get_litter_value(g, GPU_LIT_TPC_IN_GPC_STRIDE);
	u32 no_of_sm = nvgpu_gr_config_get_no_of_sm(gr->config);

	for (sm_id = 0; sm_id < no_of_sm; sm_id++) {
		struct sm_info *sm_info =
			nvgpu_gr_config_get_sm_info(g->gr.config, sm_id);
		tpc_offset = tpc_in_gpc_stride * sm_info->tpc_index;
		gpc_offset = gpc_stride * sm_info->gpc_index;
		offset = tpc_offset + gpc_offset;

		regval = gk20a_readl(g,	gr_gpc0_tpc0_tpccs_tpc_exception_en_r() +
								offset);
		/* Each bit represents corresponding enablement state, bit 0 corrsponds to SM0 */
		tpc_exception_en |= gr_gpc0_tpc0_tpccs_tpc_exception_en_sm_v(regval) << sm_id;
	}

	return tpc_exception_en;
}

u32 gk20a_gr_get_sm_hww_warp_esr(struct gk20a *g, u32 gpc, u32 tpc, u32 sm)
{
	u32 offset = nvgpu_gr_gpc_offset(g, gpc) + nvgpu_gr_tpc_offset(g, tpc);
	u32 hww_warp_esr = gk20a_readl(g,
			 gr_gpc0_tpc0_sm_hww_warp_esr_r() + offset);
	return hww_warp_esr;
}

u32 gk20a_gr_get_sm_hww_global_esr(struct gk20a *g, u32 gpc, u32 tpc, u32 sm)
{
	u32 offset = nvgpu_gr_gpc_offset(g, gpc) + nvgpu_gr_tpc_offset(g, tpc);

	u32 hww_global_esr = gk20a_readl(g,
				 gr_gpc0_tpc0_sm_hww_global_esr_r() + offset);

	return hww_global_esr;
}

u32 gk20a_gr_get_sm_no_lock_down_hww_global_esr_mask(struct gk20a *g)
{
	/*
	 * These three interrupts don't require locking down the SM. They can
	 * be handled by usermode clients as they aren't fatal. Additionally,
	 * usermode clients may wish to allow some warps to execute while others
	 * are at breakpoints, as opposed to fatal errors where all warps should
	 * halt.
	 */
	u32 global_esr_mask =
		gr_gpc0_tpc0_sm_hww_global_esr_bpt_int_pending_f() |
		gr_gpc0_tpc0_sm_hww_global_esr_bpt_pause_pending_f() |
		gr_gpc0_tpc0_sm_hww_global_esr_single_step_complete_pending_f();

	return global_esr_mask;
}
