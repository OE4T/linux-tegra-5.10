/*
 * GK20A Graphics FIFO (gr host)
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

#include <nvgpu/mm.h>
#include <nvgpu/dma.h>
#include <nvgpu/timers.h>
#include <nvgpu/enabled.h>
#include <nvgpu/semaphore.h>
#include <nvgpu/kmem.h>
#include <nvgpu/log.h>
#include <nvgpu/soc.h>
#include <nvgpu/atomic.h>
#include <nvgpu/bug.h>
#include <nvgpu/log2.h>
#include <nvgpu/debug.h>
#include <nvgpu/nvhost.h>
#include <nvgpu/barrier.h>
#include <nvgpu/error_notifier.h>
#include <nvgpu/ptimer.h>
#include <nvgpu/io.h>
#include <nvgpu/utils.h>
#include <nvgpu/fifo.h>
#include <nvgpu/runlist.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/channel.h>
#include <nvgpu/unit.h>
#include <nvgpu/types.h>
#include <nvgpu/vm_area.h>
#include <nvgpu/top.h>
#include <nvgpu/nvgpu_err.h>
#include <nvgpu/pbdma_status.h>
#include <nvgpu/engine_status.h>
#include <nvgpu/engines.h>
#include <nvgpu/power_features/cg.h>
#include <nvgpu/power_features/pg.h>
#include <nvgpu/power_features/power_features.h>
#include <nvgpu/gr/fecs_trace.h>

#include "mm_gk20a.h"

#include <hal/fifo/mmu_fault_gk20a.h>

#include <nvgpu/hw/gk20a/hw_fifo_gk20a.h>
#include <nvgpu/hw/gk20a/hw_pbdma_gk20a.h>
#include <nvgpu/hw/gk20a/hw_ram_gk20a.h>
#include <nvgpu/hw/gk20a/hw_gr_gk20a.h>

#define FECS_METHOD_WFI_RESTORE 0x80000U

void nvgpu_report_host_error(struct gk20a *g, u32 inst,
		u32 err_id, u32 intr_info)
{
	int ret;

	if (g->ops.fifo.err_ops.report_host_err == NULL) {
		return ;
	}
	ret = g->ops.fifo.err_ops.report_host_err(g,
			NVGPU_ERR_MODULE_HOST, inst, err_id, intr_info);
	if (ret != 0) {
		nvgpu_err(g, "Failed to report HOST error: \
				inst=%u, err_id=%u, intr_info=%u, ret=%d",
				inst, err_id, intr_info, ret);
	}
}

int gk20a_init_fifo_reset_enable_hw(struct gk20a *g)
{
	u32 timeout;
	unsigned int i;
	u32 host_num_pbdma = nvgpu_get_litter_value(g, GPU_LIT_HOST_NUM_PBDMA);

	nvgpu_log_fn(g, " ");

	/* enable pmc pfifo */
	g->ops.mc.reset(g, g->ops.mc.reset_mask(g, NVGPU_UNIT_FIFO));

	nvgpu_cg_slcg_fifo_load_enable(g);

	nvgpu_cg_blcg_fifo_load_enable(g);

	timeout = gk20a_readl(g, fifo_fb_timeout_r());
	timeout = set_field(timeout, fifo_fb_timeout_period_m(),
			fifo_fb_timeout_period_max_f());
	nvgpu_log_info(g, "fifo_fb_timeout reg val = 0x%08x", timeout);
	gk20a_writel(g, fifo_fb_timeout_r(), timeout);

	/* write pbdma timeout value */
	for (i = 0; i < host_num_pbdma; i++) {
		timeout = gk20a_readl(g, pbdma_timeout_r(i));
		timeout = set_field(timeout, pbdma_timeout_period_m(),
				    pbdma_timeout_period_max_f());
		nvgpu_log_info(g, "pbdma_timeout reg val = 0x%08x", timeout);
		gk20a_writel(g, pbdma_timeout_r(i), timeout);
	}
	if (g->ops.fifo.apply_pb_timeout != NULL) {
		g->ops.fifo.apply_pb_timeout(g);
	}

	g->ops.fifo.intr_0_enable(g, true);
	g->ops.fifo.intr_1_enable(g, true);

	nvgpu_log_fn(g, "done");

	return 0;
}

int gk20a_init_fifo_setup_hw(struct gk20a *g)
{
	struct fifo_gk20a *f = &g->fifo;
	u64 shifted_addr;

	nvgpu_log_fn(g, " ");

	/* set the base for the userd region now */
	shifted_addr = f->userd_gpu_va >> 12;
	if ((shifted_addr >> 32) != 0U) {
		nvgpu_err(g, "GPU VA > 32 bits %016llx\n", f->userd_gpu_va);
		return -EFAULT;
	}
	gk20a_writel(g, fifo_bar1_base_r(),
			fifo_bar1_base_ptr_f(u64_lo32(shifted_addr)) |
			fifo_bar1_base_valid_true_f());

	nvgpu_log_fn(g, "done");

	return 0;
}

bool gk20a_fifo_should_defer_engine_reset(struct gk20a *g, u32 engine_id,
		u32 engine_subid, bool fake_fault)
{
	enum nvgpu_fifo_engine engine_enum = NVGPU_ENGINE_INVAL_GK20A;
	struct fifo_engine_info_gk20a *engine_info;

	if (g == NULL) {
		return false;
	}

	engine_info = nvgpu_engine_get_active_eng_info(g, engine_id);

	if (engine_info != NULL) {
		engine_enum = engine_info->engine_enum;
	}

	if (engine_enum == NVGPU_ENGINE_INVAL_GK20A) {
		return false;
	}

	/* channel recovery is only deferred if an sm debugger
	   is attached and has MMU debug mode is enabled */
	if (!g->ops.gr.sm_debugger_attached(g) ||
	    !g->ops.fb.is_debug_mode_enabled(g)) {
		return false;
	}

	/* if this fault is fake (due to RC recovery), don't defer recovery */
	if (fake_fault) {
		return false;
	}

	if (engine_enum != NVGPU_ENGINE_GR_GK20A) {
		return false;
	}

	return g->ops.engine.is_fault_engine_subid_gpc(g, engine_subid);
}

void gk20a_fifo_abort_tsg(struct gk20a *g, struct tsg_gk20a *tsg, bool preempt)
{
	struct channel_gk20a *ch = NULL;

	nvgpu_log_fn(g, " ");

	WARN_ON(tsg->abortable == false);

	g->ops.tsg.disable(tsg);

	if (preempt) {
		g->ops.fifo.preempt_tsg(g, tsg);
	}

	nvgpu_rwsem_down_read(&tsg->ch_list_lock);
	nvgpu_list_for_each_entry(ch, &tsg->ch_list, channel_gk20a, ch_entry) {
		if (gk20a_channel_get(ch) != NULL) {
			gk20a_channel_set_unserviceable(ch);
			if (ch->g->ops.fifo.ch_abort_clean_up != NULL) {
				ch->g->ops.fifo.ch_abort_clean_up(ch);
			}
			gk20a_channel_put(ch);
		}
	}
	nvgpu_rwsem_up_read(&tsg->ch_list_lock);
}

int gk20a_fifo_deferred_reset(struct gk20a *g, struct channel_gk20a *ch)
{
	unsigned long engine_id, engines = 0U;
	struct tsg_gk20a *tsg;
	bool deferred_reset_pending;
	struct fifo_gk20a *f = &g->fifo;
	int err = 0;

	nvgpu_mutex_acquire(&g->dbg_sessions_lock);

	nvgpu_mutex_acquire(&f->deferred_reset_mutex);
	deferred_reset_pending = g->fifo.deferred_reset_pending;
	nvgpu_mutex_release(&f->deferred_reset_mutex);

	if (!deferred_reset_pending) {
		nvgpu_mutex_release(&g->dbg_sessions_lock);
		return 0;
	}

	err = g->ops.gr.falcon.disable_ctxsw(g);
	if (err != 0) {
		nvgpu_err(g, "failed to disable ctxsw");
		goto fail;
	}

	tsg = tsg_gk20a_from_ch(ch);
	if (tsg != NULL) {
		engines = g->ops.engine.get_mask_on_id(g,
				tsg->tsgid, true);
	} else {
		nvgpu_err(g, "chid: %d is not bound to tsg", ch->chid);
	}

	if (engines == 0U) {
		goto clean_up;
	}

	/*
	 * If deferred reset is set for an engine, and channel is running
	 * on that engine, reset it
	 */

	for_each_set_bit(engine_id, &g->fifo.deferred_fault_engines, 32UL) {
		if ((BIT64(engine_id) & engines) != 0ULL) {
			nvgpu_engine_reset(g, (u32)engine_id);
		}
	}

	nvgpu_mutex_acquire(&f->deferred_reset_mutex);
	g->fifo.deferred_fault_engines = 0;
	g->fifo.deferred_reset_pending = false;
	nvgpu_mutex_release(&f->deferred_reset_mutex);

clean_up:
	err = g->ops.gr.falcon.enable_ctxsw(g);
	if (err != 0) {
		nvgpu_err(g, "failed to enable ctxsw");
	}
fail:
	nvgpu_mutex_release(&g->dbg_sessions_lock);

	return err;
}

static bool gk20a_fifo_handle_mmu_fault_locked(
	struct gk20a *g,
	u32 mmu_fault_engines, /* queried from HW if 0 */
	u32 hw_id, /* queried from HW if ~(u32)0 OR mmu_fault_engines == 0*/
	bool id_is_tsg)
{
	bool fake_fault;
	unsigned long fault_id;
	unsigned long engine_mmu_fault_id;
	bool verbose = true;
	struct nvgpu_engine_status_info engine_status;
	bool deferred_reset_pending = false;
	struct fifo_gk20a *f = &g->fifo;

	nvgpu_log_fn(g, " ");

	if (nvgpu_cg_pg_disable(g) != 0) {
		nvgpu_warn(g, "fail to disable power mgmt");
	}

	/* Disable fifo access */
	g->ops.gr.init.fifo_access(g, false);

	if (mmu_fault_engines != 0U) {
		fault_id = mmu_fault_engines;
		fake_fault = true;
	} else {
		fault_id = gk20a_readl(g, fifo_intr_mmu_fault_id_r());
		fake_fault = false;
	}
	nvgpu_mutex_acquire(&f->deferred_reset_mutex);
	g->fifo.deferred_reset_pending = false;
	nvgpu_mutex_release(&f->deferred_reset_mutex);

	/* go through all faulted engines */
	for_each_set_bit(engine_mmu_fault_id, &fault_id, 32U) {
		/* bits in fifo_intr_mmu_fault_id_r do not correspond 1:1 to
		 * engines. Convert engine_mmu_id to engine_id */
		u32 engine_id = nvgpu_engine_mmu_fault_id_to_engine_id(g,
					(u32)engine_mmu_fault_id);
		struct mmu_fault_info mmfault_info;
		struct channel_gk20a *ch = NULL;
		struct tsg_gk20a *tsg = NULL;
		struct channel_gk20a *refch = NULL;
		bool ctxsw;
		/* read and parse engine status */
		g->ops.engine_status.read_engine_status_info(g, engine_id,
			&engine_status);

		ctxsw = nvgpu_engine_status_is_ctxsw(&engine_status);

		gk20a_fifo_mmu_fault_info_dump(g, engine_id,
				(u32)engine_mmu_fault_id,
				fake_fault, &mmfault_info);

		if (ctxsw) {
			g->ops.gr.falcon.dump_stats(g);
			nvgpu_err(g, "  gr_status_r: 0x%x",
				  gk20a_readl(g, gr_status_r()));
		}

		/* get the channel/TSG */
		if (fake_fault) {
			/* use next_id if context load is failing */
			u32 id, type;

			if (hw_id == ~(u32)0) {
				if (nvgpu_engine_status_is_ctxsw_load(
					&engine_status)) {
					nvgpu_engine_status_get_next_ctx_id_type(
						&engine_status, &id, &type);
				} else {
					nvgpu_engine_status_get_ctx_id_type(
						&engine_status, &id, &type);
				}
			} else {
				id = hw_id;
				type = id_is_tsg ?
					ENGINE_STATUS_CTX_ID_TYPE_TSGID :
					ENGINE_STATUS_CTX_ID_TYPE_CHID;
			}

			if (type == ENGINE_STATUS_CTX_ID_TYPE_TSGID) {
				tsg = &g->fifo.tsg[id];
			} else if (type == ENGINE_STATUS_CTX_ID_TYPE_CHID) {
				ch = &g->fifo.channel[id];
				refch = gk20a_channel_get(ch);
				if (refch != NULL) {
					tsg = tsg_gk20a_from_ch(refch);
				}
			}
		} else {
			/* Look up channel from the inst block pointer. */
			ch = nvgpu_channel_refch_from_inst_ptr(g,
					mmfault_info.inst_ptr);
			refch = ch;
			if (refch != NULL) {
				tsg = tsg_gk20a_from_ch(refch);
			}
		}

		/* check if engine reset should be deferred */
		if (engine_id != FIFO_INVAL_ENGINE_ID) {
			bool defer = gk20a_fifo_should_defer_engine_reset(g,
					engine_id, mmfault_info.client_type,
					fake_fault);
			if (((ch != NULL) || (tsg != NULL)) && defer) {
				g->fifo.deferred_fault_engines |= BIT(engine_id);

				/* handled during channel free */
				nvgpu_mutex_acquire(&f->deferred_reset_mutex);
				g->fifo.deferred_reset_pending = true;
				nvgpu_mutex_release(&f->deferred_reset_mutex);

				deferred_reset_pending = true;

				nvgpu_log(g, gpu_dbg_intr | gpu_dbg_gpu_dbg,
					   "sm debugger attached,"
					   " deferring channel recovery to channel free");
			} else {
				nvgpu_engine_reset(g, engine_id);
			}
		}

#ifdef CONFIG_GK20A_CTXSW_TRACE
		if (tsg != NULL) {
			nvgpu_gr_fecs_trace_add_tsg_reset(g, tsg);
		}
#endif
		/*
		 * Disable the channel/TSG from hw and increment syncpoints.
		 */
		if (tsg != NULL) {
			if (deferred_reset_pending) {
				g->ops.tsg.disable(tsg);
			} else {
				if (!fake_fault) {
					nvgpu_tsg_set_ctx_mmu_error(g, tsg);
				}
				verbose = nvgpu_tsg_mark_error(g, tsg);
				gk20a_fifo_abort_tsg(g, tsg, false);
			}

			/* put back the ref taken early above */
			if (refch != NULL) {
				gk20a_channel_put(ch);
			}
		} else if (refch != NULL) {
			nvgpu_err(g, "mmu error in unbound channel %d",
					  ch->chid);
			gk20a_channel_put(ch);
		} else if (mmfault_info.inst_ptr ==
				nvgpu_inst_block_addr(g, &g->mm.bar1.inst_block)) {
			nvgpu_err(g, "mmu fault from bar1");
		} else if (mmfault_info.inst_ptr ==
				nvgpu_inst_block_addr(g, &g->mm.pmu.inst_block)) {
			nvgpu_err(g, "mmu fault from pmu");
		} else {
			nvgpu_err(g, "couldn't locate channel for mmu fault");
		}
	}

	if (!fake_fault) {
		gk20a_debug_dump(g);
	}

	/* clear interrupt */
	gk20a_writel(g, fifo_intr_mmu_fault_id_r(), (u32)fault_id);

	/* resume scheduler */
	gk20a_writel(g, fifo_error_sched_disable_r(),
		     gk20a_readl(g, fifo_error_sched_disable_r()));

	/* Re-enable fifo access */
	g->ops.gr.init.fifo_access(g, true);

	if (nvgpu_cg_pg_enable(g) != 0) {
		nvgpu_warn(g, "fail to enable power mgmt");
	}
	return verbose;
}

bool gk20a_fifo_handle_mmu_fault(
	struct gk20a *g,
	u32 mmu_fault_engines, /* queried from HW if 0 */
	u32 hw_id, /* queried from HW if ~(u32)0 OR mmu_fault_engines == 0*/
	bool id_is_tsg)
{
	bool verbose;

	nvgpu_log_fn(g, " ");

	nvgpu_log_info(g, "acquire engines_reset_mutex");
	nvgpu_mutex_acquire(&g->fifo.engines_reset_mutex);

	nvgpu_fifo_lock_active_runlists(g);

	verbose = gk20a_fifo_handle_mmu_fault_locked(g, mmu_fault_engines,
			hw_id, id_is_tsg);

	nvgpu_fifo_unlock_active_runlists(g);

	nvgpu_log_info(g, "release engines_reset_mutex");
	nvgpu_mutex_release(&g->fifo.engines_reset_mutex);

	return verbose;
}

static void gk20a_fifo_get_faulty_id_type(struct gk20a *g, u32 engine_id,
					  u32 *id, u32 *type)
{
	struct nvgpu_engine_status_info engine_status;

	g->ops.engine_status.read_engine_status_info(g, engine_id, &engine_status);

	/* use next_id if context load is failing */
	if (nvgpu_engine_status_is_ctxsw_load(
		&engine_status)) {
		nvgpu_engine_status_get_next_ctx_id_type(
			&engine_status, id, type);
	} else {
		nvgpu_engine_status_get_ctx_id_type(
			&engine_status, id, type);
	}
}

void gk20a_fifo_teardown_mask_intr(struct gk20a *g)
{
	u32 val;

	val = gk20a_readl(g, fifo_intr_en_0_r());
	val &= ~(fifo_intr_en_0_sched_error_m() |
		fifo_intr_en_0_mmu_fault_m());
	gk20a_writel(g, fifo_intr_en_0_r(), val);
	gk20a_writel(g, fifo_intr_0_r(), fifo_intr_0_sched_error_reset_f());
}

void gk20a_fifo_teardown_unmask_intr(struct gk20a *g)
{
	u32 val;

	val = gk20a_readl(g, fifo_intr_en_0_r());
	val |= fifo_intr_en_0_mmu_fault_f(1) | fifo_intr_en_0_sched_error_f(1);
	gk20a_writel(g, fifo_intr_en_0_r(), val);

}

void gk20a_fifo_teardown_ch_tsg(struct gk20a *g, u32 __engine_ids,
			u32 hw_id, unsigned int id_type, unsigned int rc_type,
			 struct mmu_fault_info *mmfault)
{
	unsigned long engine_id, i;
	unsigned long _engine_ids = __engine_ids;
	unsigned long engine_ids = 0;
	u32 mmu_fault_engines = 0;
	u32 ref_type;
	u32 ref_id;
	bool ref_id_is_tsg = false;
	bool id_is_known = (id_type != ID_TYPE_UNKNOWN) ? true : false;
	bool id_is_tsg = (id_type == ID_TYPE_TSG) ? true : false;

	nvgpu_log_info(g, "acquire engines_reset_mutex");
	nvgpu_mutex_acquire(&g->fifo.engines_reset_mutex);

	nvgpu_fifo_lock_active_runlists(g);

	if (id_is_known) {
		engine_ids = g->ops.engine.get_mask_on_id(g,
				hw_id, id_is_tsg);
		ref_id = hw_id;
		ref_type = id_is_tsg ?
			fifo_engine_status_id_type_tsgid_v() :
			fifo_engine_status_id_type_chid_v();
		ref_id_is_tsg = id_is_tsg;
		/* atleast one engine will get passed during sched err*/
		engine_ids |= __engine_ids;
		for_each_set_bit(engine_id, &engine_ids, 32U) {
			u32 mmu_id = nvgpu_engine_id_to_mmu_fault_id(g,
							(u32)engine_id);

			if (mmu_id != FIFO_INVAL_ENGINE_ID) {
				mmu_fault_engines |= BIT(mmu_id);
			}
		}
	} else {
		/* store faulted engines in advance */
		for_each_set_bit(engine_id, &_engine_ids, 32U) {
			gk20a_fifo_get_faulty_id_type(g, (u32)engine_id,
						      &ref_id, &ref_type);
			if (ref_type == fifo_engine_status_id_type_tsgid_v()) {
				ref_id_is_tsg = true;
			} else {
				ref_id_is_tsg = false;
			}
			/* Reset *all* engines that use the
			 * same channel as faulty engine */
			for (i = 0; i < g->fifo.num_engines; i++) {
				u32 active_engine_id = g->fifo.active_engines_list[i];
				u32 type;
				u32 id;

				gk20a_fifo_get_faulty_id_type(g,
					active_engine_id, &id, &type);
				if (ref_type == type && ref_id == id) {
					u32 mmu_id = nvgpu_engine_id_to_mmu_fault_id(g,
							active_engine_id);

					engine_ids |= BIT(active_engine_id);
					if (mmu_id != FIFO_INVAL_ENGINE_ID) {
						mmu_fault_engines |= BIT(mmu_id);
					}
				}
			}
		}
	}

	if (mmu_fault_engines != 0U) {
		g->ops.fifo.teardown_mask_intr(g);
		g->ops.fifo.trigger_mmu_fault(g, engine_ids);
		gk20a_fifo_handle_mmu_fault_locked(g, mmu_fault_engines, ref_id,
				ref_id_is_tsg);

		g->ops.fifo.teardown_unmask_intr(g);
	}

	nvgpu_fifo_unlock_active_runlists(g);

	nvgpu_log_info(g, "release engines_reset_mutex");
	nvgpu_mutex_release(&g->fifo.engines_reset_mutex);
}

void gk20a_fifo_recover(struct gk20a *g, u32 engine_ids,
			u32 hw_id, bool id_is_tsg,
			bool id_is_known, bool verbose, u32 rc_type)
{
	unsigned int id_type;

	if (verbose) {
		gk20a_debug_dump(g);
	}

	if (g->ops.ltc.flush != NULL) {
		g->ops.ltc.flush(g);
	}

	if (id_is_known) {
		id_type = id_is_tsg ? ID_TYPE_TSG : ID_TYPE_CHANNEL;
	} else {
		id_type = ID_TYPE_UNKNOWN;
	}

	g->ops.fifo.teardown_ch_tsg(g, engine_ids, hw_id, id_type,
					 rc_type, NULL);
}

/* force reset channel and tsg */
int gk20a_fifo_force_reset_ch(struct channel_gk20a *ch,
				u32 err_code, bool verbose)
{
	struct channel_gk20a *ch_tsg = NULL;
	struct gk20a *g = ch->g;

	struct tsg_gk20a *tsg = tsg_gk20a_from_ch(ch);

	if (tsg != NULL) {
		nvgpu_rwsem_down_read(&tsg->ch_list_lock);

		nvgpu_list_for_each_entry(ch_tsg, &tsg->ch_list,
				channel_gk20a, ch_entry) {
			if (gk20a_channel_get(ch_tsg) != NULL) {
				g->ops.fifo.set_error_notifier(ch_tsg,
								err_code);
				gk20a_channel_put(ch_tsg);
			}
		}

		nvgpu_rwsem_up_read(&tsg->ch_list_lock);
		nvgpu_tsg_recover(g, tsg, verbose, RC_TYPE_FORCE_RESET);
	} else {
		nvgpu_err(g, "chid: %d is not bound to tsg", ch->chid);
	}

	return 0;
}

int gk20a_fifo_tsg_unbind_channel_verify_status(struct channel_gk20a *ch)
{
	struct gk20a *g = ch->g;
	struct nvgpu_channel_hw_state hw_state;

	g->ops.channel.read_state(g, ch, &hw_state);

	if (hw_state.next) {
		nvgpu_err(g, "Channel %d to be removed from TSG %d has NEXT set!",
			ch->chid, ch->tsgid);
		return -EINVAL;
	}

	if (g->ops.fifo.tsg_verify_status_ctx_reload != NULL) {
		g->ops.fifo.tsg_verify_status_ctx_reload(ch);
	}

	if (g->ops.fifo.tsg_verify_status_faulted != NULL) {
		g->ops.fifo.tsg_verify_status_faulted(ch);
	}

	return 0;
}

int gk20a_fifo_tsg_unbind_channel(struct channel_gk20a *ch)
{
	struct gk20a *g = ch->g;
        struct tsg_gk20a *tsg = tsg_gk20a_from_ch(ch);
	int err;
	bool tsg_timedout = false;

	if (tsg == NULL) {
		nvgpu_err(g, "chid: %d is not bound to tsg", ch->chid);
		return 0;
	}

	/* If one channel in TSG times out, we disable all channels */
	nvgpu_rwsem_down_write(&tsg->ch_list_lock);
	tsg_timedout = gk20a_channel_check_unserviceable(ch);
	nvgpu_rwsem_up_write(&tsg->ch_list_lock);

	/* Disable TSG and examine status before unbinding channel */
	g->ops.tsg.disable(tsg);

	err = g->ops.fifo.preempt_tsg(g, tsg);
	if (err != 0) {
		goto fail_enable_tsg;
	}

	if ((g->ops.fifo.tsg_verify_channel_status != NULL) && !tsg_timedout) {
		err = g->ops.fifo.tsg_verify_channel_status(ch);
		if (err != 0) {
			goto fail_enable_tsg;
		}
	}

	/* Channel should be seen as TSG channel while updating runlist */
	err = channel_gk20a_update_runlist(ch, false);
	if (err != 0) {
		goto fail_enable_tsg;
	}

	/* Remove channel from TSG and re-enable rest of the channels */
	nvgpu_rwsem_down_write(&tsg->ch_list_lock);
	nvgpu_list_del(&ch->ch_entry);
	ch->tsgid = NVGPU_INVALID_TSG_ID;

	/* another thread could have re-enabled the channel because it was
	 * still on the list at that time, so make sure it's truly disabled
	 */
	g->ops.channel.disable(ch);
	nvgpu_rwsem_up_write(&tsg->ch_list_lock);

	/*
	 * Don't re-enable all channels if TSG has timed out already
	 *
	 * Note that we can skip disabling and preempting TSG too in case of
	 * time out, but we keep that to ensure TSG is kicked out
	 */
	if (!tsg_timedout) {
		g->ops.tsg.enable(tsg);
	}

	if (ch->g->ops.fifo.ch_abort_clean_up != NULL) {
		ch->g->ops.fifo.ch_abort_clean_up(ch);
	}

	return 0;

fail_enable_tsg:
	if (!tsg_timedout) {
		g->ops.tsg.enable(tsg);
	}
	return err;
}

u32 gk20a_fifo_get_failing_engine_data(struct gk20a *g,
			u32 *__id, bool *__is_tsg)
{
	u32 engine_id;
	u32 id = U32_MAX;
	bool is_tsg = false;
	u32 mailbox2;
	u32 active_engine_id = FIFO_INVAL_ENGINE_ID;
	struct nvgpu_engine_status_info engine_status;

	for (engine_id = 0; engine_id < g->fifo.num_engines; engine_id++) {
		bool failing_engine;

		active_engine_id = g->fifo.active_engines_list[engine_id];
		g->ops.engine_status.read_engine_status_info(g, active_engine_id,
			&engine_status);

		/* we are interested in busy engines */
		failing_engine = engine_status.is_busy;

		/* ..that are doing context switch */
		failing_engine = failing_engine &&
			nvgpu_engine_status_is_ctxsw(&engine_status);

		if (!failing_engine) {
		    active_engine_id = FIFO_INVAL_ENGINE_ID;
			continue;
		}

		if (nvgpu_engine_status_is_ctxsw_load(&engine_status)) {
			id = engine_status.ctx_next_id;
			is_tsg = nvgpu_engine_status_is_next_ctx_type_tsg(
					&engine_status);
		} else if (nvgpu_engine_status_is_ctxsw_switch(&engine_status)) {
			mailbox2 = gk20a_readl(g, gr_fecs_ctxsw_mailbox_r(2));
			if ((mailbox2 & FECS_METHOD_WFI_RESTORE) != 0U) {
				id = engine_status.ctx_next_id;
				is_tsg = nvgpu_engine_status_is_next_ctx_type_tsg(
						&engine_status);
			} else {
				id = engine_status.ctx_id;
				is_tsg = nvgpu_engine_status_is_ctx_type_tsg(
						&engine_status);
			}
		} else {
			id = engine_status.ctx_id;
			is_tsg = nvgpu_engine_status_is_ctx_type_tsg(
					&engine_status);
		}
		break;
	}

	*__id = id;
	*__is_tsg = is_tsg;

	return active_engine_id;
}

void gk20a_fifo_issue_preempt(struct gk20a *g, u32 id, bool is_tsg)
{
	if (is_tsg) {
		gk20a_writel(g, fifo_preempt_r(),
			fifo_preempt_id_f(id) |
			fifo_preempt_type_tsg_f());
	} else {
		gk20a_writel(g, fifo_preempt_r(),
			fifo_preempt_chid_f(id) |
			fifo_preempt_type_channel_f());
	}
}

static u32 gk20a_fifo_get_preempt_timeout(struct gk20a *g)
{
	/* Use fifo_eng_timeout converted to ms for preempt
	 * polling. gr_idle_timeout i.e 3000 ms is and not appropriate
	 * for polling preempt done as context switch timeout gets
	 * triggered every ctxsw_timeout_period_ms.
	 */

	return g->ctxsw_timeout_period_ms;
}

int gk20a_fifo_is_preempt_pending(struct gk20a *g, u32 id,
		unsigned int id_type)
{
	struct nvgpu_timeout timeout;
	u32 delay = POLL_DELAY_MIN_US;
	int ret = -EBUSY;

	nvgpu_timeout_init(g, &timeout, gk20a_fifo_get_preempt_timeout(g),
			   NVGPU_TIMER_CPU_TIMER);
	do {
		if ((gk20a_readl(g, fifo_preempt_r()) &
				fifo_preempt_pending_true_f()) == 0U) {
			ret = 0;
			break;
		}

		nvgpu_usleep_range(delay, delay * 2U);
		delay = min_t(u32, delay << 1, POLL_DELAY_MAX_US);
	} while (nvgpu_timeout_expired(&timeout) == 0);

	if (ret != 0) {
		nvgpu_err(g, "preempt timeout: id: %u id_type: %d ",
			id, id_type);
	}
	return ret;
}

void gk20a_fifo_preempt_timeout_rc_tsg(struct gk20a *g, struct tsg_gk20a *tsg)
{
	struct channel_gk20a *ch = NULL;

	nvgpu_err(g, "preempt TSG %d timeout", tsg->tsgid);

	nvgpu_rwsem_down_read(&tsg->ch_list_lock);
	nvgpu_list_for_each_entry(ch, &tsg->ch_list,
			channel_gk20a, ch_entry) {
		if (gk20a_channel_get(ch) == NULL) {
			continue;
		}
		g->ops.fifo.set_error_notifier(ch,
			NVGPU_ERR_NOTIFIER_FIFO_ERROR_IDLE_TIMEOUT);
		gk20a_channel_put(ch);
	}
	nvgpu_rwsem_up_read(&tsg->ch_list_lock);
	nvgpu_tsg_recover(g, tsg, true, RC_TYPE_PREEMPT_TIMEOUT);
}

void gk20a_fifo_preempt_timeout_rc(struct gk20a *g, struct channel_gk20a *ch)
{
	nvgpu_err(g, "preempt channel %d timeout", ch->chid);

	g->ops.fifo.set_error_notifier(ch,
				NVGPU_ERR_NOTIFIER_FIFO_ERROR_IDLE_TIMEOUT);
	nvgpu_channel_recover(g, ch, true, RC_TYPE_PREEMPT_TIMEOUT);
}

int __locked_fifo_preempt(struct gk20a *g, u32 id, bool is_tsg)
{
	int ret;
	unsigned int id_type;

	nvgpu_log_fn(g, "id: %d is_tsg: %d", id, is_tsg);

	/* issue preempt */
	gk20a_fifo_issue_preempt(g, id, is_tsg);

	id_type = is_tsg ? ID_TYPE_TSG : ID_TYPE_CHANNEL;

	/* wait for preempt */
	ret = g->ops.fifo.is_preempt_pending(g, id, id_type);

	return ret;
}

int gk20a_fifo_preempt_channel(struct gk20a *g, struct channel_gk20a *ch)
{
	int ret = 0;
	u32 token = PMU_INVALID_MUTEX_OWNER_ID;
	int mutex_ret = 0;

	nvgpu_log_fn(g, "chid: %d", ch->chid);

	/* we have no idea which runlist we are using. lock all */
	nvgpu_fifo_lock_active_runlists(g);

	mutex_ret = nvgpu_pmu_mutex_acquire(&g->pmu,
		PMU_MUTEX_ID_FIFO, &token);

	ret = __locked_fifo_preempt(g, ch->chid, false);

	if (mutex_ret == 0) {
		nvgpu_pmu_mutex_release(&g->pmu, PMU_MUTEX_ID_FIFO, &token);
	}

	nvgpu_fifo_unlock_active_runlists(g);

	if (ret != 0) {
		if (nvgpu_platform_is_silicon(g)) {
			nvgpu_err(g, "preempt timed out for chid: %u, "
			"ctxsw timeout will trigger recovery if needed",
			ch->chid);
		} else {
			gk20a_fifo_preempt_timeout_rc(g, ch);
		}
	}



	return ret;
}

int gk20a_fifo_preempt_tsg(struct gk20a *g, struct tsg_gk20a *tsg)
{
	int ret = 0;
	u32 token = PMU_INVALID_MUTEX_OWNER_ID;
	int mutex_ret = 0;

	nvgpu_log_fn(g, "tsgid: %d", tsg->tsgid);

	/* we have no idea which runlist we are using. lock all */
	nvgpu_fifo_lock_active_runlists(g);

	mutex_ret = nvgpu_pmu_mutex_acquire(&g->pmu,
			PMU_MUTEX_ID_FIFO, &token);

	ret = __locked_fifo_preempt(g, tsg->tsgid, true);

	if (mutex_ret == 0) {
		nvgpu_pmu_mutex_release(&g->pmu, PMU_MUTEX_ID_FIFO, &token);
	}

	nvgpu_fifo_unlock_active_runlists(g);

	if (ret != 0) {
		if (nvgpu_platform_is_silicon(g)) {
			nvgpu_err(g, "preempt timed out for tsgid: %u, "
			"ctxsw timeout will trigger recovery if needed",
			tsg->tsgid);
		} else {
			gk20a_fifo_preempt_timeout_rc_tsg(g, tsg);
		}
	}

	return ret;
}

int gk20a_fifo_preempt(struct gk20a *g, struct channel_gk20a *ch)
{
	int err;
	struct tsg_gk20a *tsg = tsg_gk20a_from_ch(ch);

	if (tsg != NULL) {
		err = g->ops.fifo.preempt_tsg(ch->g, tsg);
	} else {
		err = g->ops.fifo.preempt_channel(ch->g, ch);
	}

	return err;
}

u32 gk20a_fifo_runlist_busy_engines(struct gk20a *g, u32 runlist_id)
{
	struct fifo_gk20a *f = &g->fifo;
	u32 engines = 0;
	unsigned int i;
	struct nvgpu_engine_status_info engine_status;

	for (i = 0; i < f->num_engines; i++) {
		u32 active_engine_id = f->active_engines_list[i];
		u32 engine_runlist = f->engine_info[active_engine_id].runlist_id;
		bool engine_busy;
		g->ops.engine_status.read_engine_status_info(g, active_engine_id,
			&engine_status);
		engine_busy = engine_status.is_busy;

		if (engine_busy && engine_runlist == runlist_id) {
			engines |= BIT(active_engine_id);
		}
	}

	return engines;
}

u32 gk20a_fifo_default_timeslice_us(struct gk20a *g)
{
	u64 slice = (((u64)(NVGPU_FIFO_DEFAULT_TIMESLICE_TIMEOUT <<
				NVGPU_FIFO_DEFAULT_TIMESLICE_SCALE) *
			(u64)g->ptimer_src_freq) /
			(u64)PTIMER_REF_FREQ_HZ);

	BUG_ON(slice > U64(U32_MAX));

	return (u32)slice;
}

int gk20a_fifo_tsg_set_timeslice(struct tsg_gk20a *tsg, u32 timeslice)
{
	struct gk20a *g = tsg->g;

	if (timeslice < g->min_timeslice_us ||
		timeslice > g->max_timeslice_us) {
		return -EINVAL;
	}

	gk20a_channel_get_timescale_from_timeslice(g, timeslice,
			&tsg->timeslice_timeout, &tsg->timeslice_scale);

	tsg->timeslice_us = timeslice;

	return g->ops.runlist.reload(g, tsg->runlist_id, true, true);
}

int gk20a_fifo_suspend(struct gk20a *g)
{
	nvgpu_log_fn(g, " ");

	/* stop bar1 snooping */
	if (g->ops.mm.is_bar1_supported(g)) {
		gk20a_writel(g, fifo_bar1_base_r(),
			fifo_bar1_base_valid_false_f());
	}

	/* disable fifo intr */
	g->ops.fifo.intr_0_enable(g, false);
	g->ops.fifo.intr_1_enable(g, false);

	nvgpu_log_fn(g, "done");
	return 0;
}

bool gk20a_fifo_mmu_fault_pending(struct gk20a *g)
{
	if ((gk20a_readl(g, fifo_intr_0_r()) &
	     fifo_intr_0_mmu_fault_pending_f()) != 0U) {
		return true;
	} else {
		return false;
	}
}

static const char * const pbdma_chan_eng_ctx_status_str[] = {
	"invalid",
	"valid",
	"NA",
	"NA",
	"NA",
	"load",
	"save",
	"switch",
};

static const char * const not_found_str[] = {
	"NOT FOUND"
};

const char *gk20a_decode_pbdma_chan_eng_ctx_status(u32 index)
{
	if (index >= ARRAY_SIZE(pbdma_chan_eng_ctx_status_str)) {
		return not_found_str[0];
	} else {
		return pbdma_chan_eng_ctx_status_str[index];
	}
}

void gk20a_dump_channel_status_ramfc(struct gk20a *g,
				     struct gk20a_debug_output *o,
				     struct nvgpu_channel_dump_info *info)
{
	u32 syncpointa, syncpointb;

	syncpointa = info->inst.syncpointa;
	syncpointb = info->inst.syncpointb;

	gk20a_debug_output(o, "Channel ID: %d, TSG ID: %u, pid %d, refs %d; deterministic = %s",
			   info->chid,
			   info->tsgid,
			   info->pid,
			   info->refs,
			   info->deterministic ? "yes" : "no");
	gk20a_debug_output(o, "  In use: %-3s  busy: %-3s  status: %s",
			   info->hw_state.enabled ? "yes" : "no",
			   info->hw_state.busy ? "yes" : "no",
			   info->hw_state.status_string);
	gk20a_debug_output(o,
			   "  TOP       %016llx"
			   "  PUT       %016llx  GET %016llx",
			   info->inst.pb_top_level_get,
			   info->inst.pb_put,
			   info->inst.pb_get);
	gk20a_debug_output(o,
			   "  FETCH     %016llx"
			   "  HEADER    %08x          COUNT %08x",
			   info->inst.pb_fetch,
			   info->inst.pb_header,
			   info->inst.pb_count);
	gk20a_debug_output(o,
			   "  SYNCPOINT %08x %08x "
			   "SEMAPHORE %08x %08x %08x %08x",
			   syncpointa,
			   syncpointb,
			   info->inst.semaphorea,
			   info->inst.semaphoreb,
			   info->inst.semaphorec,
			   info->inst.semaphored);

	if (info->sema.addr == 0ULL) {
		gk20a_debug_output(o,
			"  SEMA STATE: val: %u next_val: %u addr: 0x%010llx",
			info->sema.value,
			info->sema.next,
			info->sema.addr);
	}

#ifdef CONFIG_TEGRA_GK20A_NVHOST
	if ((pbdma_syncpointb_op_v(syncpointb) == pbdma_syncpointb_op_wait_v())
		&& (pbdma_syncpointb_wait_switch_v(syncpointb) ==
			pbdma_syncpointb_wait_switch_en_v())) {
		gk20a_debug_output(o, "%s on syncpt %u (%s) val %u",
			info->hw_state.pending_acquire ? "Waiting" : "Waited",
			pbdma_syncpointb_syncpt_index_v(syncpointb),
			nvgpu_nvhost_syncpt_get_name(g->nvhost_dev,
				pbdma_syncpointb_syncpt_index_v(syncpointb)),
			pbdma_syncpointa_payload_v(syncpointa));
	}
#endif

	gk20a_debug_output(o, " ");
}

void gk20a_debug_dump_all_channel_status_ramfc(struct gk20a *g,
		 struct gk20a_debug_output *o)
{
	struct fifo_gk20a *f = &g->fifo;
	u32 chid;
	struct nvgpu_channel_dump_info **infos;

	infos = nvgpu_kzalloc(g, sizeof(*infos) * f->num_channels);
	if (infos == NULL) {
		gk20a_debug_output(o, "cannot alloc memory for channels\n");
		return;
	}

	for (chid = 0; chid < f->num_channels; chid++) {
		struct channel_gk20a *ch = gk20a_channel_from_id(g, chid);

		if (ch != NULL) {
			struct nvgpu_channel_dump_info *info;

			info = nvgpu_kzalloc(g, sizeof(*info));

			/* ref taken stays to below loop with
			 * successful allocs */
			if (info == NULL) {
				gk20a_channel_put(ch);
			} else {
				infos[chid] = info;
			}
		}
	}

	for (chid = 0; chid < f->num_channels; chid++) {
		struct channel_gk20a *ch = &f->channel[chid];
		struct nvgpu_channel_dump_info *info = infos[chid];
		struct nvgpu_hw_semaphore *hw_sema = ch->hw_sema;

		/* if this info exists, the above loop took a channel ref */
		if (info == NULL) {
			continue;
		}

		info->chid = ch->chid;
		info->tsgid = ch->tsgid;
		info->pid = ch->pid;
		info->refs = nvgpu_atomic_read(&ch->ref_count);
		info->deterministic = ch->deterministic;

		if (hw_sema != NULL) {
			info->sema.value = nvgpu_hw_semaphore_read(hw_sema);
			info->sema.next =
				(u32)nvgpu_hw_semaphore_read_next(hw_sema);
			info->sema.addr = nvgpu_hw_semaphore_addr(hw_sema);
		}

		g->ops.channel.read_state(g, ch, &info->hw_state);
		g->ops.ramfc.capture_ram_dump(g, ch, info);

		gk20a_channel_put(ch);
	}

	gk20a_debug_output(o, "Channel Status - chip %-5s", g->name);
	gk20a_debug_output(o, "---------------------------");
	for (chid = 0; chid < f->num_channels; chid++) {
		struct nvgpu_channel_dump_info *info = infos[chid];

		if (info != NULL) {
			g->ops.fifo.dump_channel_status_ramfc(g, o, info);
			nvgpu_kfree(g, info);
		}
	}
	gk20a_debug_output(o, " ");

	nvgpu_kfree(g, infos);
}

int gk20a_fifo_alloc_inst(struct gk20a *g, struct channel_gk20a *ch)
{
	int err;

	nvgpu_log_fn(g, " ");

	err = g->ops.mm.alloc_inst_block(g, &ch->inst_block);
	if (err != 0) {
		return err;
	}

	nvgpu_log_info(g, "channel %d inst block physical addr: 0x%16llx",
		ch->chid, nvgpu_inst_block_addr(g, &ch->inst_block));

	nvgpu_log_fn(g, "done");
	return 0;
}

void gk20a_fifo_free_inst(struct gk20a *g, struct channel_gk20a *ch)
{
	nvgpu_free_inst_block(g, &ch->inst_block);
}

static void nvgpu_fifo_pbdma_init_intr_descs(struct fifo_gk20a *f)
{
	struct gk20a *g = f->g;

	if (g->ops.pbdma.device_fatal_0_intr_descs != NULL) {
		f->intr.pbdma.device_fatal_0 =
			g->ops.pbdma.device_fatal_0_intr_descs();
	}

	if (g->ops.pbdma.device_fatal_0_intr_descs != NULL) {
		f->intr.pbdma.channel_fatal_0 =
			g->ops.pbdma.channel_fatal_0_intr_descs();
	}
	if (g->ops.pbdma.restartable_0_intr_descs != NULL) {
		f->intr.pbdma.restartable_0 =
			g->ops.pbdma.restartable_0_intr_descs();
	}
}

bool gk20a_fifo_find_pbdma_for_runlist(struct fifo_gk20a *f, u32 runlist_id,
								u32 *pbdma_id)
{
	struct gk20a *g = f->g;
	bool found_pbdma_for_runlist = false;
	u32 runlist_bit;
	u32 id;

	runlist_bit = BIT32(runlist_id);
	for (id = 0; id < f->num_pbdma; id++) {
		if ((f->pbdma_map[id] & runlist_bit) != 0U) {
			nvgpu_log_info(g, "gr info: pbdma_map[%d]=%d", id,
							f->pbdma_map[id]);
			found_pbdma_for_runlist = true;
			break;
		}
	}
	*pbdma_id = id;
	return found_pbdma_for_runlist;
}

int gk20a_fifo_init_pbdma_info(struct fifo_gk20a *f)
{
	struct gk20a *g = f->g;
	u32 id;

	f->num_pbdma = nvgpu_get_litter_value(g, GPU_LIT_HOST_NUM_PBDMA);

	f->pbdma_map = nvgpu_kzalloc(g, f->num_pbdma * sizeof(*f->pbdma_map));
	if (f->pbdma_map == NULL) {
		return -ENOMEM;
	}

	for (id = 0; id < f->num_pbdma; ++id) {
		f->pbdma_map[id] = gk20a_readl(g, fifo_pbdma_map_r(id));
	}

	nvgpu_fifo_pbdma_init_intr_descs(f);

	return 0;
}
