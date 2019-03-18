/*
 * GV11B fifo
 *
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

#include <nvgpu/bug.h>
#include <nvgpu/semaphore.h>
#include <nvgpu/timers.h>
#include <nvgpu/log.h>
#include <nvgpu/dma.h>
#include <nvgpu/nvgpu_mem.h>
#include <nvgpu/gmmu.h>
#include <nvgpu/soc.h>
#include <nvgpu/debug.h>
#include <nvgpu/nvhost.h>
#include <nvgpu/barrier.h>
#include <nvgpu/mm.h>
#include <nvgpu/log2.h>
#include <nvgpu/io_usermode.h>
#include <nvgpu/ptimer.h>
#include <nvgpu/io.h>
#include <nvgpu/utils.h>
#include <nvgpu/fifo.h>
#include <nvgpu/runlist.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/channel.h>
#include <nvgpu/unit.h>
#include <nvgpu/nvgpu_err.h>
#include <nvgpu/pbdma_status.h>
#include <nvgpu/engine_status.h>
#include <nvgpu/power_features/cg.h>
#include <nvgpu/power_features/pg.h>
#include <nvgpu/power_features/power_features.h>
#include <nvgpu/gr/fecs_trace.h>

#include "gk20a/fifo_gk20a.h"

#include "gp10b/fifo_gp10b.h"

#include <nvgpu/hw/gv11b/hw_pbdma_gv11b.h>
#include <nvgpu/hw/gv11b/hw_fifo_gv11b.h>
#include <nvgpu/hw/gv11b/hw_ram_gv11b.h>
#include <nvgpu/hw/gv11b/hw_usermode_gv11b.h>
#include <nvgpu/hw/gv11b/hw_top_gv11b.h>
#include <nvgpu/hw/gv11b/hw_gmmu_gv11b.h>
#include <nvgpu/hw/gv11b/hw_gr_gv11b.h>

#include "fifo_gv11b.h"
#include "subctx_gv11b.h"
#include "gr_gv11b.h"

u64 gv11b_fifo_usermode_base(struct gk20a *g)
{
	return usermode_cfg0_r();
}

u32 gv11b_fifo_doorbell_token(struct channel_gk20a *c)
{
	struct gk20a *g = c->g;
	struct fifo_gk20a *f = &g->fifo;

	return f->channel_base + c->chid;
}

void gv11b_ring_channel_doorbell(struct channel_gk20a *c)
{
	struct gk20a *g = c->g;
	struct fifo_gk20a *f = &g->fifo;
	u32 hw_chid = f->channel_base + c->chid;

	nvgpu_log_info(g, "channel ring door bell %d\n", c->chid);

	nvgpu_usermode_writel(c->g, usermode_notify_channel_pending_r(),
		usermode_notify_channel_pending_id_f(hw_chid));
}

void gv11b_dump_channel_status_ramfc(struct gk20a *g,
				     struct gk20a_debug_output *o,
				     struct nvgpu_channel_dump_info *info)
{
	gk20a_debug_output(o, "%d-%s, TSG: %u, pid %d, refs: %d%s: ",
			info->chid,
			g->name,
			info->tsgid,
			info->pid,
			info->refs,
			info->deterministic ? ", deterministic" : "");
	gk20a_debug_output(o, "channel status: %s in use %s %s\n",
			info->hw_state.enabled ? "" : "not",
			info->hw_state.status_string,
			info->hw_state.busy ? "busy" : "not busy");
	gk20a_debug_output(o,
			"RAMFC : TOP: %016llx PUT: %016llx GET: %016llx "
			"FETCH: %016llx\n"
			"HEADER: %08x COUNT: %08x\n"
			"SEMAPHORE: addr %016llx\n"
			"payload %016llx execute %08x\n",
			info->inst.pb_top_level_get,
			info->inst.pb_put,
			info->inst.pb_get,
			info->inst.pb_fetch,
			info->inst.pb_header,
			info->inst.pb_count,
			info->inst.sem_addr,
			info->inst.sem_payload,
			info->inst.sem_execute);

	if (info->sema.addr != 0ULL) {
		gk20a_debug_output(o, "SEMA STATE: value: 0x%08x "
				   "next_val: 0x%08x addr: 0x%010llx\n",
				  info->sema.value,
				  info->sema.next,
				  info->sema.addr);
	}

	gk20a_debug_output(o, "\n");
}

u32 gv11b_fifo_get_preempt_timeout(struct gk20a *g)
{
	/* using gr_idle_timeout for polling pdma/eng/runlist
	 * might kick in timeout handler in the cases where
	 * preempt is stuck. Use ctxsw_timeout_period_ms
	 * for preempt polling */

	return g->ctxsw_timeout_period_ms;
}

static int gv11b_fifo_poll_pbdma_chan_status(struct gk20a *g, u32 id,
				 u32 pbdma_id)
{
	struct nvgpu_timeout timeout;
	unsigned long delay = GR_IDLE_CHECK_DEFAULT; /* in micro seconds */
	int ret;
	unsigned int loop_count = 0;
	struct nvgpu_pbdma_status_info pbdma_status;

	/* timeout in milli seconds */
	ret = nvgpu_timeout_init(g, &timeout,
			g->ops.fifo.get_preempt_timeout(g),
			NVGPU_TIMER_CPU_TIMER);
	if (ret != 0) {
		nvgpu_err(g, "timeout_init failed: %d", ret);
		return ret;
	}

	/* Default return value */
	ret = -EBUSY;

	nvgpu_log(g, gpu_dbg_info, "wait preempt pbdma %d", pbdma_id);
	/* Verify that ch/tsg is no longer on the pbdma */
	do {
		if (!nvgpu_platform_is_silicon(g)) {
			if (loop_count >= MAX_PRE_SI_RETRIES) {
				nvgpu_err(g, "preempt pbdma retries: %u",
					loop_count);
				break;
			}
			loop_count++;
		}
		/*
		 * If the PBDMA has a stalling interrupt and receives a NACK,
		 * the PBDMA won't save out until the STALLING interrupt is
		 * cleared. Stalling interrupt need not be directly addressed,
		 * as simply clearing of the interrupt bit will be sufficient
		 * to allow the PBDMA to save out. If the stalling interrupt
		 * was due to a SW method or another deterministic failure,
		 * the PBDMA will assert it when the channel is reloaded
		 * or resumed. Note that the fault will still be
		 * reported to SW.
		 */

		/* Ignore un-needed return value "handled" */
		(void) gk20a_fifo_handle_pbdma_intr(g, &g->fifo, pbdma_id,
				RC_NO);

		g->ops.pbdma_status.read_pbdma_status_info(g, pbdma_id,
			&pbdma_status);

		if (nvgpu_pbdma_status_is_chsw_valid(&pbdma_status) ||
			nvgpu_pbdma_status_is_chsw_save(&pbdma_status)) {

			if (id != pbdma_status.id) {
				ret = 0;
				break;
			}

		} else if (nvgpu_pbdma_status_is_chsw_load(&pbdma_status)) {

			if (id != pbdma_status.next_id) {
				ret = 0;
				break;
			}

		} else if (nvgpu_pbdma_status_is_chsw_switch(&pbdma_status)) {

			if ((id != pbdma_status.next_id) &&
				 (id != pbdma_status.id)) {
				ret = 0;
				break;
			}
		} else {
			/* pbdma status is invalid i.e. it is not loaded */
			ret = 0;
			break;
		}

		nvgpu_usleep_range(delay, delay * 2UL);
		delay = min_t(unsigned long,
				delay << 1, GR_IDLE_CHECK_MAX);
	} while (nvgpu_timeout_expired(&timeout) == 0);

	if (ret != 0) {
		nvgpu_err(g, "preempt timeout pbdma: %u pbdma_stat: %u "
				"tsgid: %u", pbdma_id,
				pbdma_status.pbdma_reg_status, id);
	}
	return ret;
}

static int gv11b_fifo_poll_eng_ctx_status(struct gk20a *g, u32 id,
			 u32 act_eng_id, u32 *reset_eng_bitmask)
{
	struct nvgpu_timeout timeout;
	unsigned long delay = GR_IDLE_CHECK_DEFAULT; /* in micro seconds */
	u32 eng_stat;
	u32 ctx_stat;
	int ret;
	unsigned int loop_count = 0;
	u32 eng_intr_pending;

	/* timeout in milli seconds */
	ret = nvgpu_timeout_init(g, &timeout,
			g->ops.fifo.get_preempt_timeout(g),
			NVGPU_TIMER_CPU_TIMER);
	if (ret != 0) {
		nvgpu_err(g, "timeout_init failed: %d", ret);
		return ret;
	}

	/* Default return value */
	ret = -EBUSY;

	nvgpu_log(g, gpu_dbg_info, "wait preempt act engine id: %u",
			act_eng_id);
	/* Check if ch/tsg has saved off the engine or if ctxsw is hung */
	do {
		if (!nvgpu_platform_is_silicon(g)) {
			if (loop_count >= MAX_PRE_SI_RETRIES) {
				nvgpu_err(g, "preempt eng retries: %u",
					loop_count);
				break;
			}
			loop_count++;
		}
		eng_stat = gk20a_readl(g, fifo_engine_status_r(act_eng_id));
		ctx_stat  = fifo_engine_status_ctx_status_v(eng_stat);

		if (g->ops.mc.is_stall_and_eng_intr_pending(g, act_eng_id,
					&eng_intr_pending)) {
		/* From h/w team
		 * Engine save can be blocked by eng  stalling interrupts.
		 * FIFO interrupts shouldn’t block an engine save from
		 * finishing, but could block FIFO from reporting preempt done.
		 * No immediate reason to reset the engine if FIFO interrupt is
		 * pending.
		 * The hub, priv_ring, and ltc interrupts could block context
		 * switch (or memory), but doesn’t necessarily have to.
		 * For Hub interrupts they just report access counters and page
		 * faults. Neither of these necessarily block context switch
		 * or preemption, but they could.
		 * For example a page fault for graphics would prevent graphics
		 * from saving out. An access counter interrupt is a
		 * notification and has no effect.
		 * SW should handle page faults though for preempt to complete.
		 * PRI interrupt (due to a failed PRI transaction) will result
		 * in ctxsw failure reported to HOST.
		 * LTC interrupts are generally ECC related and if so,
		 * certainly don’t block preemption/ctxsw but they could.
		 * Bus interrupts shouldn’t have anything to do with preemption
		 * state as they are part of the Host EXT pipe, though they may
		 * exhibit a symptom that indicates that GPU is in a bad state.
		 * To be completely fair, when an engine is preempting SW
		 * really should just handle other interrupts as they come in.
		 * It’s generally bad to just poll and wait on a preempt
		 * to complete since there are many things in the GPU which may
		 * cause a system to hang/stop responding.
		 */
			nvgpu_log(g, gpu_dbg_info | gpu_dbg_intr,
					"stall intr set, "
					"preemption might not finish");
		}
		if (ctx_stat ==
			 fifo_engine_status_ctx_status_ctxsw_switch_v()) {
			/* Eng save hasn't started yet. Continue polling */
			if (eng_intr_pending != 0U) {
				/* if eng intr, stop polling */
				*reset_eng_bitmask |= BIT32(act_eng_id);
				ret = 0;
				break;
			}

		} else if (ctx_stat ==
			 fifo_engine_status_ctx_status_valid_v() ||
				ctx_stat ==
			 fifo_engine_status_ctx_status_ctxsw_save_v()) {

			if (id == fifo_engine_status_id_v(eng_stat)) {
				if (eng_intr_pending != 0U) {
					/* preemption will not finish */
					*reset_eng_bitmask |= BIT32(act_eng_id);
					ret = 0;
					break;
				}
			} else {
				/* context is not running on the engine */
				ret = 0;
				break;
			}

		} else if (ctx_stat ==
			 fifo_engine_status_ctx_status_ctxsw_load_v()) {

			if (id == fifo_engine_status_next_id_v(eng_stat)) {
				if (eng_intr_pending != 0U) {
					/* preemption will not finish */
					*reset_eng_bitmask |= BIT32(act_eng_id);
					ret = 0;
					break;
				}
			} else {
				/* context is not running on the engine */
				ret = 0;
				break;
			}

		} else {
			/* Preempt should be finished */
			ret = 0;
			break;
		}
		nvgpu_usleep_range(delay, delay * 2UL);
		delay = min_t(unsigned long,
				delay << 1, GR_IDLE_CHECK_MAX);
	} while (nvgpu_timeout_expired(&timeout) == 0);

	if (ret != 0) {
		/*
		* The reasons a preempt can fail are:
		* 1.Some other stalling interrupt is asserted preventing
		*   channel or context save.
		* 2.The memory system hangs.
		* 3.The engine hangs during CTXSW.
		*/
		nvgpu_err(g, "preempt timeout eng: %u ctx_stat: %u tsgid: %u",
			act_eng_id, ctx_stat, id);
		*reset_eng_bitmask |= BIT32(act_eng_id);
	}

	return ret;
}

static void gv11b_reset_faulted_tsg(struct tsg_gk20a *tsg, bool eng, bool pbdma)
{
	struct gk20a *g = tsg->g;
	struct channel_gk20a *ch;

	nvgpu_rwsem_down_read(&tsg->ch_list_lock);
	nvgpu_list_for_each_entry(ch, &tsg->ch_list, channel_gk20a, ch_entry) {
		g->ops.channel.reset_faulted(g, ch, eng, pbdma);
	}
	nvgpu_rwsem_up_read(&tsg->ch_list_lock);
}

void gv11b_fifo_reset_pbdma_and_eng_faulted(struct gk20a *g,
			struct tsg_gk20a *tsg,
			u32 faulted_pbdma, u32 faulted_engine)
{
	if (tsg == NULL) {
		return;
	}

	nvgpu_log(g, gpu_dbg_intr, "reset faulted pbdma:0x%x eng:0x%x",
				faulted_pbdma, faulted_engine);

	gv11b_reset_faulted_tsg(tsg,
			faulted_engine != FIFO_INVAL_ENGINE_ID,
			faulted_pbdma != FIFO_INVAL_PBDMA_ID);
}

static u32 gv11b_fifo_get_runlists_mask(struct gk20a *g, u32 act_eng_bitmask,
			u32 id, unsigned int id_type, unsigned int rc_type,
			 struct mmu_fault_info *mmfault)
{
	u32 runlists_mask = 0;
	struct fifo_gk20a *f = &g->fifo;
	struct fifo_runlist_info_gk20a *runlist;
	u32 i, pbdma_bitmask = 0;

	if (id_type != ID_TYPE_UNKNOWN) {
		if (id_type == ID_TYPE_TSG) {
			runlists_mask |= BIT32(f->tsg[id].runlist_id);
		} else {
			runlists_mask |= BIT32(f->channel[id].runlist_id);
		}
	}

	if ((rc_type == RC_TYPE_MMU_FAULT) && (mmfault != NULL)) {
		if (mmfault->faulted_pbdma != FIFO_INVAL_PBDMA_ID) {
			pbdma_bitmask = BIT32(mmfault->faulted_pbdma);
		}

		for (i = 0U; i < f->num_runlists; i++) {
			runlist = &f->active_runlist_info[i];

			if ((runlist->eng_bitmask & act_eng_bitmask) != 0U) {
				runlists_mask |= BIT32(runlist->runlist_id);
			}

			if ((runlist->pbdma_bitmask & pbdma_bitmask) != 0U) {
				runlists_mask |= BIT32(runlist->runlist_id);
			}
		}
	}

	if (id_type == ID_TYPE_UNKNOWN) {
		for (i = 0U; i < f->num_runlists; i++) {
			runlist = &f->active_runlist_info[i];

			if (act_eng_bitmask != 0U) {
				/* eng ids are known */
				if ((runlist->eng_bitmask & act_eng_bitmask) != 0U) {
					runlists_mask |= BIT32(runlist->runlist_id);
				}
			} else {
				runlists_mask |= BIT32(runlist->runlist_id);
			}
		}
	}
	nvgpu_log(g, gpu_dbg_info, "runlists_mask = 0x%08x", runlists_mask);
	return runlists_mask;
}

static void gv11b_fifo_issue_runlist_preempt(struct gk20a *g,
					 u32 runlists_mask)
{
	u32 reg_val;

	/* issue runlist preempt */
	reg_val = gk20a_readl(g, fifo_runlist_preempt_r());
	reg_val |= runlists_mask;
	gk20a_writel(g, fifo_runlist_preempt_r(), reg_val);
}

int gv11b_fifo_is_preempt_pending(struct gk20a *g, u32 id,
		 unsigned int id_type)
{
	struct fifo_gk20a *f = &g->fifo;
	unsigned long runlist_served_pbdmas;
	unsigned long runlist_served_engines;
	unsigned long pbdma_id;
	unsigned long act_eng_id;
	u32 runlist_id;
	int ret = 0;
	u32 tsgid;

	if (id_type == ID_TYPE_TSG) {
		runlist_id = f->tsg[id].runlist_id;
		tsgid = id;
	} else {
		runlist_id = f->channel[id].runlist_id;
		tsgid = f->channel[id].tsgid;
	}

	nvgpu_log_info(g, "Check preempt pending for tsgid = %u", tsgid);

	runlist_served_pbdmas = f->runlist_info[runlist_id]->pbdma_bitmask;
	runlist_served_engines = f->runlist_info[runlist_id]->eng_bitmask;

	for_each_set_bit(pbdma_id, &runlist_served_pbdmas, f->num_pbdma) {
		ret |= gv11b_fifo_poll_pbdma_chan_status(g, tsgid, pbdma_id);
	}

	f->runlist_info[runlist_id]->reset_eng_bitmask = 0;

	for_each_set_bit(act_eng_id, &runlist_served_engines, f->max_engines) {
		ret |= gv11b_fifo_poll_eng_ctx_status(g, tsgid, act_eng_id,
				&f->runlist_info[runlist_id]->reset_eng_bitmask);
	}
	return ret;
}

int gv11b_fifo_preempt_channel(struct gk20a *g, struct channel_gk20a *ch)
{
	struct tsg_gk20a *tsg = NULL;

	tsg = tsg_gk20a_from_ch(ch);

	if (tsg == NULL) {
		return 0;
	}

	nvgpu_log_info(g, "chid:%d tsgid:%d", ch->chid, tsg->tsgid);

	/* Preempt tsg. Channel preempt is NOOP */
	return g->ops.fifo.preempt_tsg(g, tsg);
}

/* TSG enable sequence applicable for Volta and onwards */
int gv11b_fifo_enable_tsg(struct tsg_gk20a *tsg)
{
	struct gk20a *g = tsg->g;
	struct channel_gk20a *ch;
	struct channel_gk20a *last_ch = NULL;

	nvgpu_rwsem_down_read(&tsg->ch_list_lock);
	nvgpu_list_for_each_entry(ch, &tsg->ch_list, channel_gk20a, ch_entry) {
		g->ops.channel.enable(ch);
		last_ch = ch;
	}
	nvgpu_rwsem_up_read(&tsg->ch_list_lock);

	if (last_ch != NULL) {
		g->ops.fifo.ring_channel_doorbell(last_ch);
	}

	return 0;
}

int gv11b_fifo_preempt_tsg(struct gk20a *g, struct tsg_gk20a *tsg)
{
	struct fifo_gk20a *f = &g->fifo;
	int ret = 0;
	u32 token = PMU_INVALID_MUTEX_OWNER_ID;
	int mutex_ret = 0;
	u32 runlist_id;

	nvgpu_log_fn(g, "tsgid: %d", tsg->tsgid);

	runlist_id = tsg->runlist_id;
	nvgpu_log_fn(g, "runlist_id: %d", runlist_id);
	if (runlist_id == FIFO_INVAL_RUNLIST_ID) {
		return 0;
	}

	nvgpu_mutex_acquire(&f->runlist_info[runlist_id]->runlist_lock);

	/* WAR for Bug 2065990 */
	gk20a_tsg_disable_sched(g, tsg);

	mutex_ret = nvgpu_pmu_mutex_acquire(&g->pmu,
						PMU_MUTEX_ID_FIFO, &token);

	ret = __locked_fifo_preempt(g, tsg->tsgid, true);

	if (mutex_ret == 0) {
		int err = nvgpu_pmu_mutex_release(&g->pmu, PMU_MUTEX_ID_FIFO,
				&token);
		if (err != 0) {
			nvgpu_err(g, "PMU_MUTEX_ID_FIFO not released err=%d",
					err);
		}
	}

	/* WAR for Bug 2065990 */
	gk20a_tsg_enable_sched(g, tsg);

	nvgpu_mutex_release(&f->runlist_info[runlist_id]->runlist_lock);

	if (ret != 0) {
		if (nvgpu_platform_is_silicon(g)) {
			nvgpu_err(g, "preempt timed out for tsgid: %u, "
			"ctxsw timeout will trigger recovery if needed", tsg->tsgid);
		} else {
			gk20a_fifo_preempt_timeout_rc_tsg(g, tsg);
		}
	}

	return ret;
}

static void gv11b_fifo_locked_preempt_runlists_rc(struct gk20a *g,
						u32 runlists_mask)
{
	struct fifo_gk20a *f = &g->fifo;
	struct fifo_runlist_info_gk20a *runlist;
	u32 token = PMU_INVALID_MUTEX_OWNER_ID;
	int mutex_ret = 0;
	u32 i;

	/* runlist_lock are locked by teardown and sched are disabled too */
	nvgpu_log_fn(g, "preempt runlists_mask:0x%08x", runlists_mask);

	mutex_ret = nvgpu_pmu_mutex_acquire(&g->pmu,
			PMU_MUTEX_ID_FIFO, &token);

	/* issue runlist preempt */
	gv11b_fifo_issue_runlist_preempt(g, runlists_mask);

	/*
	 * Preemption will never complete in RC due to some fatal condition.
	 * Do not poll for preemption to complete. Reset engines served by
	 * runlists.
	 */
	for (i = 0U; i < f->num_runlists; i++) {
		runlist = &f->active_runlist_info[i];

		if ((fifo_runlist_preempt_runlist_m(runlist->runlist_id) &
				runlists_mask) != 0U) {
			runlist->reset_eng_bitmask = runlist->eng_bitmask;
		}
	}

	if (mutex_ret == 0) {
		int err = nvgpu_pmu_mutex_release(&g->pmu, PMU_MUTEX_ID_FIFO,
				&token);
		if (err != 0) {
			nvgpu_err(g, "PMU_MUTEX_ID_FIFO not released err=%d",
					err);
		}
	}
}

static void gv11b_fifo_locked_abort_runlist_active_tsgs(struct gk20a *g,
			unsigned int rc_type,
			u32 runlists_mask)
{
	struct fifo_gk20a *f = &g->fifo;
	struct tsg_gk20a *tsg = NULL;
	unsigned long tsgid;
	struct fifo_runlist_info_gk20a *runlist = NULL;
	u32 token = PMU_INVALID_MUTEX_OWNER_ID;
	int mutex_ret = 0;
	int err;
	u32 i;

	nvgpu_err(g, "runlist id unknown, abort active tsgs in runlists");

	/* runlist_lock  are locked by teardown */
	mutex_ret = nvgpu_pmu_mutex_acquire(&g->pmu,
			PMU_MUTEX_ID_FIFO, &token);

	for (i = 0U; i < f->num_runlists; i++) {
		runlist = &f->active_runlist_info[i];

		if ((runlists_mask & BIT32(runlist->runlist_id)) == 0U) {
			continue;
		}
		nvgpu_log(g, gpu_dbg_info, "abort runlist id %d",
				runlist->runlist_id);

		for_each_set_bit(tsgid, runlist->active_tsgs,
			g->fifo.num_channels) {
			tsg = &g->fifo.tsg[tsgid];

			if (!tsg->abortable) {
				nvgpu_log(g, gpu_dbg_info,
					  "tsg %lu is not abortable, skipping",
					  tsgid);
				continue;
			}
			nvgpu_log(g, gpu_dbg_info, "abort tsg id %lu", tsgid);

			gk20a_disable_tsg(tsg);

			/* assume all pbdma and eng faulted are set */
			nvgpu_log(g, gpu_dbg_info, "reset pbdma and eng faulted");
			gv11b_reset_faulted_tsg(tsg, true, true);

#ifdef CONFIG_GK20A_CTXSW_TRACE
			nvgpu_gr_fecs_trace_add_tsg_reset(g, tsg);
#endif
			if (!g->fifo.deferred_reset_pending) {
				if (rc_type == RC_TYPE_MMU_FAULT) {
					nvgpu_tsg_set_ctx_mmu_error(g, tsg);
					/*
					 * Mark error (returned verbose flag is
					 * ignored since it is not needed here)
					 */
					(void) nvgpu_tsg_mark_error(g, tsg);
				}
			}

			/*
			 * remove all entries from this runlist; don't wait for
			 * the update to finish on hw.
			 */
			err = gk20a_runlist_update_locked(g, runlist->runlist_id,
					NULL, false, false);
			if (err != 0) {
				nvgpu_err(g, "runlist id %d is not cleaned up",
					runlist->runlist_id);
			}

			gk20a_fifo_abort_tsg(g, tsg, false);

			nvgpu_log(g, gpu_dbg_info, "aborted tsg id %lu", tsgid);
		}
	}
	if (mutex_ret == 0) {
		err = nvgpu_pmu_mutex_release(&g->pmu, PMU_MUTEX_ID_FIFO,
				&token);
		if (err != 0) {
			nvgpu_err(g, "PMU_MUTEX_ID_FIFO not released err=%d",
					err);
		}
	}
}

void gv11b_fifo_teardown_mask_intr(struct gk20a *g)
{
	u32 val;

	/*
	 * ctxsw timeout error prevents recovery, and ctxsw error will retrigger
	 * every 100ms. Disable ctxsw timeout error to allow recovery.
	 */
	val = gk20a_readl(g, fifo_intr_en_0_r());
	val &= ~ fifo_intr_0_ctxsw_timeout_pending_f();
	gk20a_writel(g, fifo_intr_en_0_r(), val);
	gk20a_writel(g, fifo_intr_ctxsw_timeout_r(),
			gk20a_readl(g, fifo_intr_ctxsw_timeout_r()));

}

void gv11b_fifo_teardown_unmask_intr(struct gk20a *g)
{
	u32 val;

	/* enable ctxsw timeout interrupt */
	val = gk20a_readl(g, fifo_intr_en_0_r());
	val |= fifo_intr_0_ctxsw_timeout_pending_f();
	gk20a_writel(g, fifo_intr_en_0_r(), val);
}


void gv11b_fifo_teardown_ch_tsg(struct gk20a *g, u32 act_eng_bitmask,
			u32 id, unsigned int id_type, unsigned int rc_type,
			 struct mmu_fault_info *mmfault)
{
	struct tsg_gk20a *tsg = NULL;
	u32 runlists_mask, rlid, i;
	unsigned long pbdma_id;
	struct fifo_runlist_info_gk20a *runlist = NULL;
	unsigned long engine_id;
	u32 client_type = ~U32(0U);
	struct fifo_gk20a *f = &g->fifo;
	u32 runlist_id = FIFO_INVAL_RUNLIST_ID;
	u32 num_runlists = 0U;
	unsigned long runlist_served_pbdmas;
	bool deferred_reset_pending = false;

	nvgpu_log_info(g, "acquire engines_reset_mutex");
	nvgpu_mutex_acquire(&g->fifo.engines_reset_mutex);

	nvgpu_fifo_lock_active_runlists(g);

	g->ops.fifo.teardown_mask_intr(g);

	/* get runlist id and tsg */
	if (id_type == ID_TYPE_TSG) {
		if (id != FIFO_INVAL_TSG_ID) {
			tsg = &g->fifo.tsg[id];
			runlist_id = tsg->runlist_id;
			if (runlist_id != FIFO_INVAL_RUNLIST_ID) {
				num_runlists++;
			} else {
				nvgpu_log_fn(g, "tsg runlist id is invalid");
			}
		} else {
			nvgpu_log_fn(g, "id type is tsg but tsg id is inval");
		}
	} else {
		/*
		 * id type is unknown, get runlist_id if eng mask is such that
		 * it corresponds to single runlist id. If eng mask corresponds
		 * to multiple runlists, then abort all runlists
		 */
		for (i = 0U; i < f->num_runlists; i++) {
			runlist = &f->active_runlist_info[i];

			if ((runlist->eng_bitmask & act_eng_bitmask) != 0U) {
				runlist_id = runlist->runlist_id;
				num_runlists++;
			}
		}
		if (num_runlists > 1U) {
			/* abort all runlists */
			runlist_id = FIFO_INVAL_RUNLIST_ID;
		}
	}

	/* if runlist_id is valid and there is only single runlist to be
	 * aborted, release runlist lock that are not
	 * needed for this recovery
	 */
	if (runlist_id != FIFO_INVAL_RUNLIST_ID && num_runlists == 1U) {
		for (i = 0U; i < f->num_runlists; i++) {
			runlist = &f->active_runlist_info[i];

			if (runlist->runlist_id != runlist_id) {
				nvgpu_log_fn(g, "release runlist_lock for "
						"unused runlist id: %d",
						runlist->runlist_id);
				nvgpu_mutex_release(&runlist->runlist_lock);
			}
		}
	}

	nvgpu_log(g, gpu_dbg_info, "id = %d, id_type = %d, rc_type = %d, "
			"act_eng_bitmask = 0x%x, mmfault ptr = 0x%p",
			 id, id_type, rc_type, act_eng_bitmask, mmfault);

	runlists_mask =  gv11b_fifo_get_runlists_mask(g, act_eng_bitmask, id,
					 id_type, rc_type, mmfault);

	/* Disable runlist scheduler */
	gk20a_fifo_set_runlist_state(g, runlists_mask, RUNLIST_DISABLED);

	if (nvgpu_cg_pg_disable(g) != 0) {
		nvgpu_warn(g, "fail to disable power mgmt");
	}

	if (rc_type == RC_TYPE_MMU_FAULT) {
		gk20a_debug_dump(g);
		client_type = mmfault->client_type;
		gv11b_fifo_reset_pbdma_and_eng_faulted(g, tsg,
				mmfault->faulted_pbdma,
				mmfault->faulted_engine);
	}

	if (tsg != NULL) {
		gk20a_disable_tsg(tsg);
	}

	/*
	 * Even though TSG preempt timed out, the RC sequence would by design
	 * require s/w to issue another preempt.
	 * If recovery includes an ENGINE_RESET, to not have race conditions,
	 * use RUNLIST_PREEMPT to kick all work off, and cancel any context
	 * load which may be pending. This is also needed to make sure
	 * that all PBDMAs serving the engine are not loaded when engine is
	 * reset.
	 */
	gv11b_fifo_locked_preempt_runlists_rc(g, runlists_mask);
	/*
	 * For each PBDMA which serves the runlist, poll to verify the TSG is no
	 * longer on the PBDMA and the engine phase of the preempt has started.
	 */
	if (tsg != NULL) {
		rlid = f->tsg[id].runlist_id;
		runlist_served_pbdmas = f->runlist_info[rlid]->pbdma_bitmask;
		for_each_set_bit(pbdma_id, &runlist_served_pbdmas,
			f->num_pbdma) {
			/*
			 * If pbdma preempt fails the only option is to reset
			 * GPU. Any sort of hang indicates the entire GPU’s
			 * memory system would be blocked.
			 */
			if (gv11b_fifo_poll_pbdma_chan_status(g, id,
					pbdma_id) != 0) {
				nvgpu_report_host_error(g, 0,
						GPU_HOST_PBDMA_PREEMPT_ERROR,
						pbdma_id);
				nvgpu_err(g, "PBDMA preempt failed");
			}
		}
	}

	nvgpu_mutex_acquire(&f->deferred_reset_mutex);
	g->fifo.deferred_reset_pending = false;
	nvgpu_mutex_release(&f->deferred_reset_mutex);

	/* check if engine reset should be deferred */
	for (i = 0U; i < f->num_runlists; i++) {
		runlist = &f->active_runlist_info[i];

		if (((runlists_mask & BIT32(runlist->runlist_id)) != 0U) &&
		    (runlist->reset_eng_bitmask != 0U)) {

			unsigned long __reset_eng_bitmask =
				 runlist->reset_eng_bitmask;

			for_each_set_bit(engine_id, &__reset_eng_bitmask,
							g->fifo.max_engines) {
				if ((tsg != NULL) &&
					 gk20a_fifo_should_defer_engine_reset(g,
					engine_id, client_type, false)) {

					g->fifo.deferred_fault_engines |=
							 BIT64(engine_id);

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
		}
	}

#ifdef CONFIG_GK20A_CTXSW_TRACE
	if (tsg != NULL)
		nvgpu_gr_fecs_trace_add_tsg_reset(g, tsg);
#endif
	if (tsg != NULL) {
		if (deferred_reset_pending) {
			gk20a_disable_tsg(tsg);
		} else {
			if (rc_type == RC_TYPE_MMU_FAULT) {
				nvgpu_tsg_set_ctx_mmu_error(g, tsg);
			}
			(void)nvgpu_tsg_mark_error(g, tsg);
			gk20a_fifo_abort_tsg(g, tsg, false);
		}
	} else {
		gv11b_fifo_locked_abort_runlist_active_tsgs(g, rc_type,
			runlists_mask);
	}

	gk20a_fifo_set_runlist_state(g, runlists_mask, RUNLIST_ENABLED);

	if (nvgpu_cg_pg_enable(g) != 0) {
		nvgpu_warn(g, "fail to enable power mgmt");
	}

	g->ops.fifo.teardown_unmask_intr(g);

	/* release runlist_lock */
	if (runlist_id != FIFO_INVAL_RUNLIST_ID) {
		nvgpu_log_fn(g, "release runlist_lock runlist_id = %d",
				runlist_id);
		runlist = f->runlist_info[runlist_id];
		nvgpu_mutex_release(&runlist->runlist_lock);
	} else {
		nvgpu_fifo_unlock_active_runlists(g);
	}

	nvgpu_log_info(g, "release engines_reset_mutex");
	nvgpu_mutex_release(&g->fifo.engines_reset_mutex);
}

int gv11b_init_fifo_reset_enable_hw(struct gk20a *g)
{
	u32 timeout;
	unsigned int i;
	u32 host_num_pbdma = nvgpu_get_litter_value(g, GPU_LIT_HOST_NUM_PBDMA);

	nvgpu_log_fn(g, " ");

	/* enable pmc pfifo */
	g->ops.mc.reset(g, g->ops.mc.reset_mask(g, NVGPU_UNIT_FIFO));

	nvgpu_cg_slcg_ce2_load_enable(g);

	nvgpu_cg_slcg_fifo_load_enable(g);

	nvgpu_cg_blcg_fifo_load_enable(g);

	timeout = gk20a_readl(g, fifo_fb_timeout_r());
	nvgpu_log_info(g, "fifo_fb_timeout reg val = 0x%08x", timeout);
	if (!nvgpu_platform_is_silicon(g)) {
		timeout = set_field(timeout, fifo_fb_timeout_period_m(),
					fifo_fb_timeout_period_max_f());
		timeout = set_field(timeout, fifo_fb_timeout_detection_m(),
					fifo_fb_timeout_detection_disabled_f());
		nvgpu_log_info(g, "new fifo_fb_timeout reg val = 0x%08x",
					timeout);
		gk20a_writel(g, fifo_fb_timeout_r(), timeout);
	}

	for (i = 0; i < host_num_pbdma; i++) {
		timeout = gk20a_readl(g, pbdma_timeout_r(i));
		nvgpu_log_info(g, "pbdma_timeout reg val = 0x%08x",
						 timeout);
		if (!nvgpu_platform_is_silicon(g)) {
			timeout = set_field(timeout, pbdma_timeout_period_m(),
					pbdma_timeout_period_max_f());
			nvgpu_log_info(g, "new pbdma_timeout reg val = 0x%08x",
						 timeout);
			gk20a_writel(g, pbdma_timeout_r(i), timeout);
		}
	}

	g->ops.fifo.intr_0_enable(g, true);
	g->ops.fifo.intr_1_enable(g, true);

	nvgpu_log_fn(g, "done");

	return 0;
}

void gv11b_fifo_init_ramfc_eng_method_buffer(struct gk20a *g,
			struct channel_gk20a *ch, struct nvgpu_mem *mem)
{
	struct tsg_gk20a *tsg;
	struct nvgpu_mem *method_buffer_per_runque;

	tsg = tsg_gk20a_from_ch(ch);
	if (tsg == NULL) {
		nvgpu_err(g, "channel is not part of tsg");
		return;
	}
	if (tsg->eng_method_buffers == NULL) {
		nvgpu_log_info(g, "eng method buffer NULL");
		return;
	}
	if (tsg->runlist_id == gk20a_fifo_get_fast_ce_runlist_id(g)) {
		method_buffer_per_runque =
			&tsg->eng_method_buffers[ASYNC_CE_RUNQUE];
	} else {
		method_buffer_per_runque =
			&tsg->eng_method_buffers[GR_RUNQUE];
	}

	nvgpu_mem_wr32(g, mem, ram_in_eng_method_buffer_addr_lo_w(),
			u64_lo32(method_buffer_per_runque->gpu_va));
	nvgpu_mem_wr32(g, mem, ram_in_eng_method_buffer_addr_hi_w(),
			u64_hi32(method_buffer_per_runque->gpu_va));

	nvgpu_log_info(g, "init ramfc with method buffer");
}

static unsigned int gv11b_fifo_get_eng_method_buffer_size(struct gk20a *g)
{
	unsigned int buffer_size;

	buffer_size =  ((9U + 1U + 3U) * g->ops.ce2.get_num_pce(g)) + 2U;
	buffer_size = (27U * 5U * buffer_size);
	buffer_size = roundup(buffer_size, PAGE_SIZE);
	nvgpu_log_info(g, "method buffer size in bytes %d", buffer_size);

	return buffer_size;
}

void gv11b_fifo_init_eng_method_buffers(struct gk20a *g,
					struct tsg_gk20a *tsg)
{
	struct vm_gk20a *vm = g->mm.bar2.vm;
	int err = 0;
	int i;
	unsigned int runque, method_buffer_size;
	unsigned int num_pbdma = g->fifo.num_pbdma;

	if (tsg->eng_method_buffers != NULL) {
		return;
	}

	method_buffer_size = gv11b_fifo_get_eng_method_buffer_size(g);
	if (method_buffer_size == 0U) {
		nvgpu_info(g, "ce will hit MTHD_BUFFER_FAULT");
		return;
	}

	tsg->eng_method_buffers = nvgpu_kzalloc(g,
					num_pbdma * sizeof(struct nvgpu_mem));

	for (runque = 0; runque < num_pbdma; runque++) {
		err = nvgpu_dma_alloc_map_sys(vm, method_buffer_size,
					&tsg->eng_method_buffers[runque]);
		if (err != 0) {
			break;
		}
	}
	if (err != 0) {
		for (i = ((int)runque - 1); i >= 0; i--) {
			nvgpu_dma_unmap_free(vm,
				 &tsg->eng_method_buffers[i]);
		}

		nvgpu_kfree(g, tsg->eng_method_buffers);
		tsg->eng_method_buffers = NULL;
		nvgpu_err(g, "could not alloc eng method buffers");
		return;
	}
	nvgpu_log_info(g, "eng method buffers allocated");

}

void gv11b_fifo_deinit_eng_method_buffers(struct gk20a *g,
					struct tsg_gk20a *tsg)
{
	struct vm_gk20a *vm = g->mm.bar2.vm;
	unsigned int runque;

	if (tsg->eng_method_buffers == NULL) {
		return;
	}

	for (runque = 0; runque < g->fifo.num_pbdma; runque++) {
		nvgpu_dma_unmap_free(vm, &tsg->eng_method_buffers[runque]);
	}

	nvgpu_kfree(g, tsg->eng_method_buffers);
	tsg->eng_method_buffers = NULL;

	nvgpu_log_info(g, "eng method buffers de-allocated");
}

int gv11b_init_fifo_setup_hw(struct gk20a *g)
{
	struct fifo_gk20a *f = &g->fifo;

	f->max_subctx_count = gr_pri_fe_chip_def_info_max_veid_count_init_v();

	/* configure userd writeback timer */
	nvgpu_writel(g, fifo_userd_writeback_r(),
		fifo_userd_writeback_timer_f(
			fifo_userd_writeback_timer_100us_v()));

	return 0;
}

static u32 gv11b_mmu_fault_id_to_gr_veid(struct gk20a *g, u32 gr_eng_fault_id,
				 u32 mmu_fault_id)
{
	struct fifo_gk20a *f = &g->fifo;
	u32 num_subctx;
	u32 veid = FIFO_INVAL_VEID;

	num_subctx = f->max_subctx_count;

	if (mmu_fault_id >= gr_eng_fault_id &&
			mmu_fault_id < (gr_eng_fault_id + num_subctx)) {
		veid = mmu_fault_id - gr_eng_fault_id;
	}

	return veid;
}

static u32 gv11b_mmu_fault_id_to_eng_id_and_veid(struct gk20a *g,
			 u32 mmu_fault_id, u32 *veid)
{
	u32 engine_id;
	u32 active_engine_id;
	struct fifo_engine_info_gk20a *engine_info;
	struct fifo_gk20a *f = &g->fifo;


	for (engine_id = 0; engine_id < f->num_engines; engine_id++) {
		active_engine_id = f->active_engines_list[engine_id];
		engine_info = &g->fifo.engine_info[active_engine_id];

		if (active_engine_id == NVGPU_ENGINE_GR_GK20A) {
			/* get faulted subctx id */
			*veid = gv11b_mmu_fault_id_to_gr_veid(g,
					engine_info->fault_id, mmu_fault_id);
			if (*veid != FIFO_INVAL_VEID) {
				break;
                        }
		} else {
			if (engine_info->fault_id == mmu_fault_id) {
				break;
			}
		}

		active_engine_id = FIFO_INVAL_ENGINE_ID;
	}
	return active_engine_id;
}

static u32 gv11b_mmu_fault_id_to_pbdma_id(struct gk20a *g, u32 mmu_fault_id)
{
	u32 num_pbdma, reg_val, fault_id_pbdma0;

	reg_val = gk20a_readl(g, fifo_cfg0_r());
	num_pbdma = fifo_cfg0_num_pbdma_v(reg_val);
	fault_id_pbdma0 = fifo_cfg0_pbdma_fault_id_v(reg_val);

	if (mmu_fault_id >= fault_id_pbdma0 &&
			mmu_fault_id <= fault_id_pbdma0 + num_pbdma - 1U) {
		return mmu_fault_id - fault_id_pbdma0;
	}

	return FIFO_INVAL_PBDMA_ID;
}

void gv11b_mmu_fault_id_to_eng_pbdma_id_and_veid(struct gk20a *g,
	u32 mmu_fault_id, u32 *active_engine_id, u32 *veid, u32 *pbdma_id)
{
	*active_engine_id = gv11b_mmu_fault_id_to_eng_id_and_veid(g,
				 mmu_fault_id, veid);

	if (*active_engine_id == FIFO_INVAL_ENGINE_ID) {
		*pbdma_id = gv11b_mmu_fault_id_to_pbdma_id(g, mmu_fault_id);
	} else {
		*pbdma_id = FIFO_INVAL_PBDMA_ID;
	}
}

void gv11b_fifo_tsg_verify_status_faulted(struct channel_gk20a *ch)
{
	struct gk20a *g = ch->g;
	struct tsg_gk20a *tsg = &g->fifo.tsg[ch->tsgid];
	struct nvgpu_channel_hw_state hw_state;

	g->ops.channel.read_state(g, ch, &hw_state);
	/*
	 * If channel has FAULTED set, clear the CE method buffer
	 * if saved out channel is same as faulted channel
	 */
	if (!hw_state.eng_faulted) {
		return;
	}

	if (tsg->eng_method_buffers == NULL) {
		return;
	}

	/*
	 * CE method buffer format :
	 * DWord0 = method count
	 * DWord1 = channel id
	 *
	 * It is sufficient to write 0 to method count to invalidate
	 */
	if ((u32)ch->chid ==
	    nvgpu_mem_rd32(g, &tsg->eng_method_buffers[ASYNC_CE_RUNQUE], 1)) {
		nvgpu_mem_wr32(g, &tsg->eng_method_buffers[ASYNC_CE_RUNQUE], 0, 0);
	}
}
