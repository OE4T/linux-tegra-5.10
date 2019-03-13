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

#include <trace/events/gk20a.h>

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

u32 gk20a_fifo_get_fast_ce_runlist_id(struct gk20a *g)
{
	u32 ce_runlist_id = gk20a_fifo_get_gr_runlist_id(g);
	enum nvgpu_fifo_engine engine_enum;
	struct fifo_gk20a *f = NULL;
	u32 engine_id_idx;
	struct fifo_engine_info_gk20a *engine_info;
	u32 active_engine_id = 0;

	if (g == NULL) {
		return ce_runlist_id;
	}

	f = &g->fifo;

	for (engine_id_idx = 0; engine_id_idx < f->num_engines; ++engine_id_idx) {
		active_engine_id = f->active_engines_list[engine_id_idx];
		engine_info = &f->engine_info[active_engine_id];
		engine_enum = engine_info->engine_enum;

		/* selecet last available ASYNC_CE if available */
		if (engine_enum == NVGPU_ENGINE_ASYNC_CE_GK20A) {
			ce_runlist_id = engine_info->runlist_id;
		}
	}

	return ce_runlist_id;
}

u32 gk20a_fifo_get_gr_runlist_id(struct gk20a *g)
{
	u32 gr_engine_cnt = 0;
	u32 gr_engine_id = FIFO_INVAL_ENGINE_ID;
	struct fifo_engine_info_gk20a *engine_info;
	u32 gr_runlist_id = U32_MAX;

	/* Consider 1st available GR engine */
	gr_engine_cnt = nvgpu_engine_get_ids(g, &gr_engine_id,
			1, NVGPU_ENGINE_GR_GK20A);

	if (gr_engine_cnt == 0U) {
		nvgpu_err(g,
			"No GR engine available on this device!");
		goto end;
	}

	engine_info = nvgpu_engine_get_active_eng_info(g, gr_engine_id);

	if (engine_info != NULL) {
		gr_runlist_id = engine_info->runlist_id;
	} else {
		nvgpu_err(g,
			"gr_engine_id is not in active list/invalid %d", gr_engine_id);
	}

end:
	return gr_runlist_id;
}

bool gk20a_fifo_is_valid_runlist_id(struct gk20a *g, u32 runlist_id)
{
	struct fifo_gk20a *f = NULL;
	u32 engine_id_idx;
	u32 active_engine_id;
	struct fifo_engine_info_gk20a *engine_info;

	if (g == NULL) {
		return false;
	}

	f = &g->fifo;

	for (engine_id_idx = 0; engine_id_idx < f->num_engines; ++engine_id_idx) {
		active_engine_id = f->active_engines_list[engine_id_idx];
		engine_info = nvgpu_engine_get_active_eng_info(g, active_engine_id);
		if ((engine_info != NULL) &&
		    (engine_info->runlist_id == runlist_id)) {
			return true;
		}
	}

	return false;
}

/*
 * Link engine IDs to MMU IDs and vice versa.
 */

static inline u32 gk20a_engine_id_to_mmu_id(struct gk20a *g, u32 engine_id)
{
	u32 fault_id = FIFO_INVAL_ENGINE_ID;
	struct fifo_engine_info_gk20a *engine_info;

	engine_info = nvgpu_engine_get_active_eng_info(g, engine_id);

	if (engine_info != NULL) {
		fault_id = engine_info->fault_id;
	} else {
		nvgpu_err(g, "engine_id is not in active list/invalid %d", engine_id);
	}
	return fault_id;
}

static inline u32 gk20a_mmu_id_to_engine_id(struct gk20a *g, u32 fault_id)
{
	u32 engine_id;
	u32 active_engine_id;
	struct fifo_engine_info_gk20a *engine_info;
	struct fifo_gk20a *f = &g->fifo;

	for (engine_id = 0; engine_id < f->num_engines; engine_id++) {
		active_engine_id = f->active_engines_list[engine_id];
		engine_info = &g->fifo.engine_info[active_engine_id];

		if (engine_info->fault_id == fault_id) {
			break;
		}
		active_engine_id = FIFO_INVAL_ENGINE_ID;
	}
	return active_engine_id;
}

u32 gk20a_fifo_intr_0_error_mask(struct gk20a *g)
{
	u32 intr_0_error_mask =
		fifo_intr_0_bind_error_pending_f() |
		fifo_intr_0_sched_error_pending_f() |
		fifo_intr_0_chsw_error_pending_f() |
		fifo_intr_0_fb_flush_timeout_pending_f() |
		fifo_intr_0_dropped_mmu_fault_pending_f() |
		fifo_intr_0_mmu_fault_pending_f() |
		fifo_intr_0_lb_error_pending_f() |
		fifo_intr_0_pio_error_pending_f();

	return intr_0_error_mask;
}

static u32 gk20a_fifo_intr_0_en_mask(struct gk20a *g)
{
	u32 intr_0_en_mask;

	intr_0_en_mask = g->ops.fifo.intr_0_error_mask(g);

	intr_0_en_mask |= fifo_intr_0_runlist_event_pending_f() |
				 fifo_intr_0_pbdma_intr_pending_f();

	return intr_0_en_mask;
}

int gk20a_init_fifo_reset_enable_hw(struct gk20a *g)
{
	u32 intr_stall;
	u32 mask;
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

	if (g->ops.fifo.apply_ctxsw_timeout_intr != NULL) {
		g->ops.fifo.apply_ctxsw_timeout_intr(g);
	} else {
		timeout = g->fifo_eng_timeout_us;
		timeout = scale_ptimer(timeout,
			ptimer_scalingfactor10x(g->ptimer_src_freq));
		timeout |= fifo_eng_timeout_detection_enabled_f();
		gk20a_writel(g, fifo_eng_timeout_r(), timeout);
	}

	/* clear and enable pbdma interrupt */
	for (i = 0; i < host_num_pbdma; i++) {
		gk20a_writel(g, pbdma_intr_0_r(i), 0xFFFFFFFFU);
		gk20a_writel(g, pbdma_intr_1_r(i), 0xFFFFFFFFU);

		intr_stall = gk20a_readl(g, pbdma_intr_stall_r(i));
		intr_stall &= ~pbdma_intr_stall_lbreq_enabled_f();
		gk20a_writel(g, pbdma_intr_stall_r(i), intr_stall);
		nvgpu_log_info(g, "pbdma id:%u, intr_en_0 0x%08x", i, intr_stall);
		gk20a_writel(g, pbdma_intr_en_0_r(i), intr_stall);
		intr_stall = gk20a_readl(g, pbdma_intr_stall_1_r(i));
		/*
		 * For bug 2082123
		 * Mask the unused HCE_RE_ILLEGAL_OP bit from the interrupt.
		 */
		intr_stall &= ~pbdma_intr_stall_1_hce_illegal_op_enabled_f();
		nvgpu_log_info(g, "pbdma id:%u, intr_en_1 0x%08x", i, intr_stall);
		gk20a_writel(g, pbdma_intr_en_1_r(i), intr_stall);
	}

	/* reset runlist interrupts */
	gk20a_writel(g, fifo_intr_runlist_r(), U32_MAX);

	/* clear and enable pfifo interrupt */
	gk20a_writel(g, fifo_intr_0_r(), 0xFFFFFFFFU);
	mask = gk20a_fifo_intr_0_en_mask(g);
	nvgpu_log_info(g, "fifo_intr_en_0 0x%08x", mask);
	gk20a_writel(g, fifo_intr_en_0_r(), mask);
	nvgpu_log_info(g, "fifo_intr_en_1 = 0x80000000");
	gk20a_writel(g, fifo_intr_en_1_r(), 0x80000000U);

	nvgpu_log_fn(g, "done");

	return 0;
}

int gk20a_fifo_init_userd_slabs(struct gk20a *g)
{
	struct fifo_gk20a *f = &g->fifo;
	int err;

	err = nvgpu_mutex_init(&f->userd_mutex);
	if (err != 0) {
		nvgpu_err(g, "failed to init userd_mutex");
		return err;
	}

	f->num_channels_per_slab = PAGE_SIZE /  f->userd_entry_size;
	f->num_userd_slabs =
		DIV_ROUND_UP(f->num_channels, f->num_channels_per_slab);

	f->userd_slabs = nvgpu_big_zalloc(g, f->num_userd_slabs *
				          sizeof(struct nvgpu_mem));
	if (f->userd_slabs == NULL) {
		nvgpu_err(g, "could not allocate userd slabs");
		return -ENOMEM;
	}

	return 0;
}

int gk20a_fifo_init_userd(struct gk20a *g, struct channel_gk20a *c)
{
	struct fifo_gk20a *f = &g->fifo;
	struct nvgpu_mem *mem;
	u32 slab = c->chid / f->num_channels_per_slab;
	int err = 0;

	if (slab > f->num_userd_slabs) {
		nvgpu_err(g, "chid %u, slab %u out of range (max=%u)",
			c->chid, slab,  f->num_userd_slabs);
		return -EINVAL;
	}

	mem = &g->fifo.userd_slabs[slab];

	nvgpu_mutex_acquire(&f->userd_mutex);
	if (!nvgpu_mem_is_valid(mem)) {
		err = nvgpu_dma_alloc_sys(g, PAGE_SIZE, mem);
		if (err != 0) {
			nvgpu_err(g, "userd allocation failed, err=%d", err);
			goto done;
		}

		if (g->ops.mm.is_bar1_supported(g)) {
			mem->gpu_va = g->ops.mm.bar1_map_userd(g, mem,
							 slab * PAGE_SIZE);
		}
	}
	c->userd_mem = mem;
	c->userd_offset = (c->chid % f->num_channels_per_slab) *
				f->userd_entry_size;
	c->userd_iova = gk20a_channel_userd_addr(c);

	nvgpu_log(g, gpu_dbg_info,
		"chid=%u slab=%u mem=%p offset=%u addr=%llx gpu_va=%llx",
		c->chid, slab, mem, c->userd_offset,
		gk20a_channel_userd_addr(c),
		gk20a_channel_userd_gpu_va(c));

done:
	nvgpu_mutex_release(&f->userd_mutex);
	return err;
}

void gk20a_fifo_free_userd_slabs(struct gk20a *g)
{
	struct fifo_gk20a *f = &g->fifo;
	u32 slab;

	if (f->userd_slabs != NULL) {
		for (slab = 0; slab < f->num_userd_slabs; slab++) {
			nvgpu_dma_free(g, &f->userd_slabs[slab]);
		}
		nvgpu_big_free(g, f->userd_slabs);
		f->userd_slabs = NULL;
	}
}

void gk20a_fifo_handle_runlist_event(struct gk20a *g)
{
	u32 runlist_event = gk20a_readl(g, fifo_intr_runlist_r());

	nvgpu_log(g, gpu_dbg_intr, "runlist event %08x",
		  runlist_event);

	gk20a_writel(g, fifo_intr_runlist_r(), runlist_event);
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

/* return with a reference to the channel, caller must put it back */
struct channel_gk20a *
gk20a_refch_from_inst_ptr(struct gk20a *g, u64 inst_ptr)
{
	struct fifo_gk20a *f = &g->fifo;
	unsigned int ci;
	if (unlikely(f->channel == NULL)) {
		return NULL;
	}
	for (ci = 0; ci < f->num_channels; ci++) {
		struct channel_gk20a *ch;
		u64 ch_inst_ptr;

		ch = gk20a_channel_from_id(g, ci);
		/* only alive channels are searched */
		if (ch == NULL) {
			continue;
		}

		ch_inst_ptr = nvgpu_inst_block_addr(g, &ch->inst_block);
		if (inst_ptr == ch_inst_ptr) {
			return ch;
		}

		gk20a_channel_put(ch);
	}
	return NULL;
}

/* fault info/descriptions.
 * tbd: move to setup
 *  */
static const char * const gk20a_fault_type_descs[] = {
	 "pde", /*fifo_intr_mmu_fault_info_type_pde_v() == 0 */
	 "pde size",
	 "pte",
	 "va limit viol",
	 "unbound inst",
	 "priv viol",
	 "ro viol",
	 "wo viol",
	 "pitch mask",
	 "work creation",
	 "bad aperture",
	 "compression failure",
	 "bad kind",
	 "region viol",
	 "dual ptes",
	 "poisoned",
};
/* engine descriptions */
static const char * const engine_subid_descs[] = {
	"gpc",
	"hub",
};

static const char * const gk20a_hub_client_descs[] = {
	"vip", "ce0", "ce1", "dniso", "fe", "fecs", "host", "host cpu",
	"host cpu nb", "iso", "mmu", "mspdec", "msppp", "msvld",
	"niso", "p2p", "pd", "perf", "pmu", "raster twod", "scc",
	"scc nb", "sec", "ssync", "gr copy", "xv", "mmu nb",
	"msenc", "d falcon", "sked", "a falcon", "n/a",
};

static const char * const gk20a_gpc_client_descs[] = {
	"l1 0", "t1 0", "pe 0",
	"l1 1", "t1 1", "pe 1",
	"l1 2", "t1 2", "pe 2",
	"l1 3", "t1 3", "pe 3",
	"rast", "gcc", "gpccs",
	"prop 0", "prop 1", "prop 2", "prop 3",
	"l1 4", "t1 4", "pe 4",
	"l1 5", "t1 5", "pe 5",
	"l1 6", "t1 6", "pe 6",
	"l1 7", "t1 7", "pe 7",
};

static const char * const does_not_exist[] = {
	"does not exist"
};

/* fill in mmu fault desc */
void gk20a_fifo_get_mmu_fault_desc(struct mmu_fault_info *mmfault)
{
	if (mmfault->fault_type >= ARRAY_SIZE(gk20a_fault_type_descs)) {
		WARN_ON(mmfault->fault_type >=
				ARRAY_SIZE(gk20a_fault_type_descs));
	} else {
		mmfault->fault_type_desc =
			 gk20a_fault_type_descs[mmfault->fault_type];
	}
}

/* fill in mmu fault client description */
void gk20a_fifo_get_mmu_fault_client_desc(struct mmu_fault_info *mmfault)
{
	if (mmfault->client_id >= ARRAY_SIZE(gk20a_hub_client_descs)) {
		WARN_ON(mmfault->client_id >=
				ARRAY_SIZE(gk20a_hub_client_descs));
	} else {
		mmfault->client_id_desc =
			 gk20a_hub_client_descs[mmfault->client_id];
	}
}

/* fill in mmu fault gpc description */
void gk20a_fifo_get_mmu_fault_gpc_desc(struct mmu_fault_info *mmfault)
{
	if (mmfault->client_id >= ARRAY_SIZE(gk20a_gpc_client_descs)) {
		WARN_ON(mmfault->client_id >=
				ARRAY_SIZE(gk20a_gpc_client_descs));
	} else {
		mmfault->client_id_desc =
			 gk20a_gpc_client_descs[mmfault->client_id];
	}
}

static void get_exception_mmu_fault_info(struct gk20a *g, u32 mmu_fault_id,
	struct mmu_fault_info *mmfault)
{
	g->ops.fifo.get_mmu_fault_info(g, mmu_fault_id, mmfault);

	/* parse info */
	mmfault->fault_type_desc =  does_not_exist[0];
	if (g->ops.fifo.get_mmu_fault_desc != NULL) {
		g->ops.fifo.get_mmu_fault_desc(mmfault);
	}

	if (mmfault->client_type >= ARRAY_SIZE(engine_subid_descs)) {
		WARN_ON(mmfault->client_type >= ARRAY_SIZE(engine_subid_descs));
		mmfault->client_type_desc = does_not_exist[0];
	} else {
		mmfault->client_type_desc =
				 engine_subid_descs[mmfault->client_type];
	}

	mmfault->client_id_desc = does_not_exist[0];
	if ((mmfault->client_type ==
		fifo_intr_mmu_fault_info_engine_subid_hub_v())
		&& (g->ops.fifo.get_mmu_fault_client_desc != NULL)) {
		g->ops.fifo.get_mmu_fault_client_desc(mmfault);
	} else if ((mmfault->client_type ==
			fifo_intr_mmu_fault_info_engine_subid_gpc_v())
			&& (g->ops.fifo.get_mmu_fault_gpc_desc != NULL)) {
		g->ops.fifo.get_mmu_fault_gpc_desc(mmfault);
	}
}

/* reads info from hardware and fills in mmu fault info record */
void gk20a_fifo_get_mmu_fault_info(struct gk20a *g, u32 mmu_fault_id,
	struct mmu_fault_info *mmfault)
{
	u32 fault_info;
	u32 addr_lo, addr_hi;

	nvgpu_log_fn(g, "mmu_fault_id %d", mmu_fault_id);

	(void) memset(mmfault, 0, sizeof(*mmfault));

	fault_info = gk20a_readl(g,
		fifo_intr_mmu_fault_info_r(mmu_fault_id));
	mmfault->fault_type =
		fifo_intr_mmu_fault_info_type_v(fault_info);
	mmfault->access_type =
		fifo_intr_mmu_fault_info_write_v(fault_info);
	mmfault->client_type =
		fifo_intr_mmu_fault_info_engine_subid_v(fault_info);
	mmfault->client_id =
		fifo_intr_mmu_fault_info_client_v(fault_info);

	addr_lo = gk20a_readl(g, fifo_intr_mmu_fault_lo_r(mmu_fault_id));
	addr_hi = gk20a_readl(g, fifo_intr_mmu_fault_hi_r(mmu_fault_id));
	mmfault->fault_addr = hi32_lo32_to_u64(addr_hi, addr_lo);
	/* note:ignoring aperture on gk20a... */
	mmfault->inst_ptr = fifo_intr_mmu_fault_inst_ptr_v(
		 gk20a_readl(g, fifo_intr_mmu_fault_inst_r(mmu_fault_id)));
	/* note: inst_ptr is a 40b phys addr.  */
	mmfault->inst_ptr <<= fifo_intr_mmu_fault_inst_ptr_align_shift_v();
}

void gk20a_fifo_reset_engine(struct gk20a *g, u32 engine_id)
{
	enum nvgpu_fifo_engine engine_enum = NVGPU_ENGINE_INVAL_GK20A;
	struct fifo_engine_info_gk20a *engine_info;

	nvgpu_log_fn(g, " ");

	if (g == NULL) {
		return;
	}

	engine_info = nvgpu_engine_get_active_eng_info(g, engine_id);

	if (engine_info != NULL) {
		engine_enum = engine_info->engine_enum;
	}

	if (engine_enum == NVGPU_ENGINE_INVAL_GK20A) {
		nvgpu_err(g, "unsupported engine_id %d", engine_id);
	}

	if (engine_enum == NVGPU_ENGINE_GR_GK20A) {
		if (nvgpu_pg_elpg_disable(g) != 0 ) {
			nvgpu_err(g, "failed to set disable elpg");
		}

#ifdef CONFIG_GK20A_CTXSW_TRACE
		/*
		 * Resetting engine will alter read/write index. Need to flush
		 * circular buffer before re-enabling FECS.
		 */
		if (g->ops.gr.fecs_trace.reset)
			g->ops.gr.fecs_trace.reset(g);
#endif
		if (!nvgpu_platform_is_simulation(g)) {
			/*HALT_PIPELINE method, halt GR engine*/
			if (gr_gk20a_halt_pipe(g) != 0) {
				nvgpu_err(g, "failed to HALT gr pipe");
			}
			/*
			 * resetting engine using mc_enable_r() is not
			 * enough, we do full init sequence
			 */
			nvgpu_log(g, gpu_dbg_info, "resetting gr engine");
			gk20a_gr_reset(g);
		} else {
			nvgpu_log(g, gpu_dbg_info,
				"HALT gr pipe not supported and "
				"gr cannot be reset without halting gr pipe");
		}
		if (nvgpu_pg_elpg_enable(g) != 0 ) {
			nvgpu_err(g, "failed to set enable elpg");
		}
	}
	if ((engine_enum == NVGPU_ENGINE_GRCE_GK20A) ||
		(engine_enum == NVGPU_ENGINE_ASYNC_CE_GK20A)) {
			g->ops.mc.reset(g, engine_info->reset_mask);
	}
}

static void gk20a_fifo_handle_chsw_fault(struct gk20a *g)
{
	u32 intr;

	intr = gk20a_readl(g, fifo_intr_chsw_error_r());
	nvgpu_report_host_error(g, 0,
			GPU_HOST_PFIFO_CHSW_ERROR, intr);
	nvgpu_err(g, "chsw: %08x", intr);
	g->ops.gr.dump_gr_falcon_stats(g);
	gk20a_writel(g, fifo_intr_chsw_error_r(), intr);
}

static void gk20a_fifo_handle_dropped_mmu_fault(struct gk20a *g)
{
	u32 fault_id = gk20a_readl(g, fifo_intr_mmu_fault_id_r());
	nvgpu_err(g, "dropped mmu fault (0x%08x)", fault_id);
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

	g->ops.fifo.disable_tsg(tsg);

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

	nvgpu_mutex_acquire(&g->dbg_sessions_lock);

	nvgpu_mutex_acquire(&f->deferred_reset_mutex);
	deferred_reset_pending = g->fifo.deferred_reset_pending;
	nvgpu_mutex_release(&f->deferred_reset_mutex);

	if (!deferred_reset_pending) {
		nvgpu_mutex_release(&g->dbg_sessions_lock);
		return 0;
	}

	gr_gk20a_disable_ctxsw(g);

	tsg = tsg_gk20a_from_ch(ch);
	if (tsg != NULL) {
		engines = g->ops.fifo.get_engines_mask_on_id(g,
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
			gk20a_fifo_reset_engine(g, (u32)engine_id);
		}
	}

	nvgpu_mutex_acquire(&f->deferred_reset_mutex);
	g->fifo.deferred_fault_engines = 0;
	g->fifo.deferred_reset_pending = false;
	nvgpu_mutex_release(&f->deferred_reset_mutex);

clean_up:
	gr_gk20a_enable_ctxsw(g);
	nvgpu_mutex_release(&g->dbg_sessions_lock);

	return 0;
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
	u32 grfifo_ctl;
	struct nvgpu_engine_status_info engine_status;
	bool deferred_reset_pending = false;
	struct fifo_gk20a *f = &g->fifo;

	nvgpu_log_fn(g, " ");

	if (nvgpu_cg_pg_disable(g) != 0) {
		nvgpu_warn(g, "fail to disable power mgmt");
	}

	/* Disable fifo access */
	grfifo_ctl = gk20a_readl(g, gr_gpfifo_ctl_r());
	grfifo_ctl &= ~gr_gpfifo_ctl_semaphore_access_f(1);
	grfifo_ctl &= ~gr_gpfifo_ctl_access_f(1);

	gk20a_writel(g, gr_gpfifo_ctl_r(),
		grfifo_ctl | gr_gpfifo_ctl_access_f(0) |
		gr_gpfifo_ctl_semaphore_access_f(0));

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
		u32 engine_id = gk20a_mmu_id_to_engine_id(g,
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

		get_exception_mmu_fault_info(g, (u32)engine_mmu_fault_id,
						 &mmfault_info);
		trace_gk20a_mmu_fault(mmfault_info.fault_addr,
				      mmfault_info.fault_type,
				      mmfault_info.access_type,
				      mmfault_info.inst_ptr,
				      engine_id,
				      mmfault_info.client_type_desc,
				      mmfault_info.client_id_desc,
				      mmfault_info.fault_type_desc);
		nvgpu_err(g, "MMU fault @ address: 0x%llx %s",
			  mmfault_info.fault_addr,
			  fake_fault ? "[FAKE]" : "");
		nvgpu_err(g, "  Engine: %d  subid: %d (%s)",
			  (int)engine_id,
			  mmfault_info.client_type,
			  mmfault_info.client_type_desc);
		nvgpu_err(g, "  Client %d (%s), ",
			  mmfault_info.client_id,
			  mmfault_info.client_id_desc);
		nvgpu_err(g, "  Type %d (%s); access_type 0x%08x; inst_ptr 0x%llx",
			  mmfault_info.fault_type,
			  mmfault_info.fault_type_desc,
			  mmfault_info.access_type, mmfault_info.inst_ptr);

		if (ctxsw) {
			g->ops.gr.dump_gr_falcon_stats(g);
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
			ch = gk20a_refch_from_inst_ptr(g,
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
				gk20a_fifo_reset_engine(g, engine_id);
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
				gk20a_disable_tsg(tsg);
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
	gk20a_writel(g, gr_gpfifo_ctl_r(),
		     gr_gpfifo_ctl_access_enabled_f() |
		     gr_gpfifo_ctl_semaphore_access_enabled_f());

	if (nvgpu_cg_pg_enable(g) != 0) {
		nvgpu_warn(g, "fail to enable power mgmt");
	}
	return verbose;
}

static bool gk20a_fifo_handle_mmu_fault(
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

u32 gk20a_fifo_engines_on_id(struct gk20a *g, u32 id, bool is_tsg)
{
	unsigned int i;
	u32 engines = 0;
	struct nvgpu_engine_status_info engine_status;
	u32 ctx_id;
	u32 type;
	bool busy;

	for (i = 0; i < g->fifo.num_engines; i++) {
		u32 active_engine_id = g->fifo.active_engines_list[i];
		g->ops.engine_status.read_engine_status_info(g,
			active_engine_id, &engine_status);

		if (nvgpu_engine_status_is_ctxsw_load(
			&engine_status)) {
			nvgpu_engine_status_get_next_ctx_id_type(
				&engine_status, &ctx_id, &type);
		} else {
			nvgpu_engine_status_get_ctx_id_type(
				&engine_status, &ctx_id, &type);
		}

		busy = engine_status.is_busy;

		if (busy && ctx_id == id) {
			if ((is_tsg && type ==
					ENGINE_STATUS_CTX_ID_TYPE_TSGID) ||
				    (!is_tsg && type ==
					ENGINE_STATUS_CTX_ID_TYPE_CHID)) {
				engines |= BIT(active_engine_id);
			}
		}
	}

	return engines;
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
		engine_ids = g->ops.fifo.get_engines_mask_on_id(g,
				hw_id, id_is_tsg);
		ref_id = hw_id;
		ref_type = id_is_tsg ?
			fifo_engine_status_id_type_tsgid_v() :
			fifo_engine_status_id_type_chid_v();
		ref_id_is_tsg = id_is_tsg;
		/* atleast one engine will get passed during sched err*/
		engine_ids |= __engine_ids;
		for_each_set_bit(engine_id, &engine_ids, 32U) {
			u32 mmu_id = gk20a_engine_id_to_mmu_id(g,
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

				gk20a_fifo_get_faulty_id_type(g, active_engine_id, &id, &type);
				if (ref_type == type && ref_id == id) {
					u32 mmu_id = gk20a_engine_id_to_mmu_id(g, active_engine_id);

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
	g->ops.fifo.disable_tsg(tsg);

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
	nvgpu_rwsem_up_write(&tsg->ch_list_lock);

	/*
	 * Don't re-enable all channels if TSG has timed out already
	 *
	 * Note that we can skip disabling and preempting TSG too in case of
	 * time out, but we keep that to ensure TSG is kicked out
	 */
	if (!tsg_timedout) {
		g->ops.fifo.enable_tsg(tsg);
	}

	if (ch->g->ops.fifo.ch_abort_clean_up != NULL) {
		ch->g->ops.fifo.ch_abort_clean_up(ch);
	}

	return 0;

fail_enable_tsg:
	if (!tsg_timedout) {
		g->ops.fifo.enable_tsg(tsg);
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

bool gk20a_fifo_handle_sched_error(struct gk20a *g)
{
	u32 sched_error;
	u32 engine_id;
	u32 id = U32_MAX;
	bool is_tsg = false;
	bool ret = false;
	struct channel_gk20a *ch = NULL;

	/* read the scheduler error register */
	sched_error = gk20a_readl(g, fifo_intr_sched_error_r());

	engine_id = gk20a_fifo_get_failing_engine_data(g, &id, &is_tsg);
	/*
	 * Could not find the engine
	 * Possible Causes:
	 * a)
	 * On hitting engine reset, h/w drops the ctxsw_status to INVALID in
	 * fifo_engine_status register. Also while the engine is held in reset
	 * h/w passes busy/idle straight through. fifo_engine_status registers
	 * are correct in that there is no context switch outstanding
	 * as the CTXSW is aborted when reset is asserted.
	 * This is just a side effect of how gv100 and earlier versions of
	 * ctxsw_timeout behave.
	 * With gv11b and later, h/w snaps the context at the point of error
	 * so that s/w can see the tsg_id which caused the HW timeout.
	 * b)
	 * If engines are not busy and ctxsw state is valid then intr occurred
	 * in the past and if the ctxsw state has moved on to VALID from LOAD
	 * or SAVE, it means that whatever timed out eventually finished
	 * anyways. The problem with this is that s/w cannot conclude which
	 * context caused the problem as maybe more switches occurred before
	 * intr is handled.
	 */
	if (engine_id == FIFO_INVAL_ENGINE_ID) {
		nvgpu_info(g, "fifo sched error: 0x%08x, failed to find engine "
				"that is busy doing ctxsw. "
				"May be ctxsw already happened", sched_error);
		ret = false;
		goto err;
	}

	if (!nvgpu_engine_check_valid_id(g, engine_id)) {
		nvgpu_err(g, "fifo sched error: 0x%08x, engine_id %u not valid",
			sched_error, engine_id);
		ret = false;
		goto err;
	}

	if (fifo_intr_sched_error_code_f(sched_error) ==
			fifo_intr_sched_error_code_ctxsw_timeout_v()) {
		struct fifo_gk20a *f = &g->fifo;
		u32 ms = 0;
		bool verbose = false;

		if (id > f->num_channels) {
			nvgpu_err(g, "fifo sched error : channel id invalid %u",
				  id);
			ret = false;
			goto err;
		}

		if (is_tsg) {
			ret = nvgpu_tsg_check_ctxsw_timeout(
					&f->tsg[id], &verbose, &ms);
		} else {
			ch = gk20a_channel_from_id(g, id);
			if (ch != NULL) {
				ret = g->ops.fifo.check_ch_ctxsw_timeout(
					ch, &verbose, &ms);

				gk20a_channel_put(ch);
			} else {
				/* skip recovery since channel is null */
				ret = false;
			}
		}

		if (ret) {
			nvgpu_err(g,
				"fifo sched ctxsw timeout error: "
				"engine=%u, %s=%d, ms=%u",
				engine_id, is_tsg ? "tsg" : "ch", id, ms);
			/*
			 * Cancel all channels' timeout since SCHED error might
			 * trigger multiple watchdogs at a time
			 */
			gk20a_channel_timeout_restart_all_channels(g);
			gk20a_fifo_recover(g, BIT(engine_id), id,
					is_tsg, true, verbose,
					RC_TYPE_CTXSW_TIMEOUT);
		} else {
			nvgpu_log_info(g,
				"fifo is waiting for ctx switch for %d ms, "
				"%s=%d", ms, is_tsg ? "tsg" : "ch", id);
		}
	} else {
		nvgpu_err(g,
			"fifo sched error : 0x%08x, engine=%u, %s=%d",
			sched_error, engine_id, is_tsg ? "tsg" : "ch", id);
	}

err:
	return ret;
}

static u32 fifo_error_isr(struct gk20a *g, u32 fifo_intr)
{
	u32 handled = 0;

	nvgpu_log_fn(g, "fifo_intr=0x%08x", fifo_intr);

	if ((fifo_intr & fifo_intr_0_pio_error_pending_f()) != 0U) {
		/* pio mode is unused.  this shouldn't happen, ever. */
		/* should we clear it or just leave it pending? */
		nvgpu_err(g, "fifo pio error!");
		BUG();
	}

	if ((fifo_intr & fifo_intr_0_bind_error_pending_f()) != 0U) {
		u32 bind_error = gk20a_readl(g, fifo_intr_bind_error_r());
		nvgpu_report_host_error(g, 0,
				GPU_HOST_PFIFO_BIND_ERROR, bind_error);
		nvgpu_err(g, "fifo bind error: 0x%08x", bind_error);
		handled |= fifo_intr_0_bind_error_pending_f();
	}

	if ((fifo_intr & fifo_intr_0_sched_error_pending_f()) != 0U) {
		(void) g->ops.fifo.handle_sched_error(g);
		handled |= fifo_intr_0_sched_error_pending_f();
	}

	if ((fifo_intr & fifo_intr_0_chsw_error_pending_f()) != 0U) {
		gk20a_fifo_handle_chsw_fault(g);
		handled |= fifo_intr_0_chsw_error_pending_f();
	}

	if ((fifo_intr & fifo_intr_0_mmu_fault_pending_f()) != 0U) {
		(void) gk20a_fifo_handle_mmu_fault(g, 0, ~(u32)0, false);
		handled |= fifo_intr_0_mmu_fault_pending_f();
	}

	if ((fifo_intr & fifo_intr_0_dropped_mmu_fault_pending_f()) != 0U) {
		gk20a_fifo_handle_dropped_mmu_fault(g);
		handled |= fifo_intr_0_dropped_mmu_fault_pending_f();
	}

	return handled;
}

static void gk20a_fifo_pbdma_fault_rc(struct gk20a *g,
			struct fifo_gk20a *f, u32 pbdma_id,
			u32 error_notifier)
{
	u32 id;
	struct nvgpu_pbdma_status_info pbdma_status;

	nvgpu_log(g, gpu_dbg_info, "pbdma id %d error notifier %d",
			pbdma_id, error_notifier);

	g->ops.pbdma_status.read_pbdma_status_info(g, pbdma_id,
		&pbdma_status);
	/* Remove channel from runlist */
	id = pbdma_status.id;
	if (pbdma_status.id_type == PBDMA_STATUS_ID_TYPE_CHID) {
		struct channel_gk20a *ch = gk20a_channel_from_id(g, id);

		if (ch != NULL) {
			g->ops.fifo.set_error_notifier(ch, error_notifier);
			nvgpu_channel_recover(g, ch, true, RC_TYPE_PBDMA_FAULT);
			gk20a_channel_put(ch);
		}
	} else if (pbdma_status.id_type == PBDMA_STATUS_ID_TYPE_TSGID) {
		struct tsg_gk20a *tsg = &f->tsg[id];
		struct channel_gk20a *ch = NULL;

		nvgpu_rwsem_down_read(&tsg->ch_list_lock);
		nvgpu_list_for_each_entry(ch, &tsg->ch_list,
				channel_gk20a, ch_entry) {
			if (gk20a_channel_get(ch) != NULL) {
				g->ops.fifo.set_error_notifier(ch,
					error_notifier);
				gk20a_channel_put(ch);
			}
		}
		nvgpu_rwsem_up_read(&tsg->ch_list_lock);
		nvgpu_tsg_recover(g, tsg, true, RC_TYPE_PBDMA_FAULT);
	} else {
		nvgpu_err(g, "Invalid pbdma_id");
	}
}

u32 gk20a_fifo_handle_pbdma_intr(struct gk20a *g, struct fifo_gk20a *f,
			u32 pbdma_id, unsigned int rc)
{
	u32 pbdma_intr_0 = gk20a_readl(g, pbdma_intr_0_r(pbdma_id));
	u32 pbdma_intr_1 = gk20a_readl(g, pbdma_intr_1_r(pbdma_id));

	u32 handled = 0;
	u32 error_notifier = NVGPU_ERR_NOTIFIER_PBDMA_ERROR;
	unsigned int rc_type = RC_TYPE_NO_RC;

	if (pbdma_intr_0 != 0U) {
		nvgpu_log(g, gpu_dbg_info | gpu_dbg_intr,
			"pbdma id %d intr_0 0x%08x pending",
			pbdma_id, pbdma_intr_0);

		if (g->ops.pbdma.handle_pbdma_intr_0(g, pbdma_id, pbdma_intr_0,
			&handled, &error_notifier) != RC_TYPE_NO_RC) {
			rc_type = RC_TYPE_PBDMA_FAULT;
		}
		gk20a_writel(g, pbdma_intr_0_r(pbdma_id), pbdma_intr_0);
	}

	if (pbdma_intr_1 != 0U) {
		nvgpu_log(g, gpu_dbg_info | gpu_dbg_intr,
			"pbdma id %d intr_1 0x%08x pending",
			pbdma_id, pbdma_intr_1);

		if (g->ops.pbdma.handle_pbdma_intr_1(g, pbdma_id, pbdma_intr_1,
			&handled, &error_notifier) != RC_TYPE_NO_RC) {
			rc_type = RC_TYPE_PBDMA_FAULT;
		}
		gk20a_writel(g, pbdma_intr_1_r(pbdma_id), pbdma_intr_1);
	}

	if (rc == RC_YES && rc_type == RC_TYPE_PBDMA_FAULT) {
		gk20a_fifo_pbdma_fault_rc(g, f, pbdma_id, error_notifier);
	}

	return handled;
}

static u32 fifo_pbdma_isr(struct gk20a *g, u32 fifo_intr)
{
	struct fifo_gk20a *f = &g->fifo;
	u32 clear_intr = 0, i;
	u32 host_num_pbdma = nvgpu_get_litter_value(g, GPU_LIT_HOST_NUM_PBDMA);
	u32 pbdma_pending = gk20a_readl(g, fifo_intr_pbdma_id_r());

	for (i = 0; i < host_num_pbdma; i++) {
		if (fifo_intr_pbdma_id_status_v(pbdma_pending, i) != 0U) {
			nvgpu_log(g, gpu_dbg_intr, "pbdma id %d intr pending", i);
			clear_intr |=
				gk20a_fifo_handle_pbdma_intr(g, f, i, RC_YES);
		}
	}
	return fifo_intr_0_pbdma_intr_pending_f();
}

void gk20a_fifo_isr(struct gk20a *g)
{
	u32 error_intr_mask;
	u32 clear_intr = 0;
	u32 fifo_intr = gk20a_readl(g, fifo_intr_0_r());

	error_intr_mask = g->ops.fifo.intr_0_error_mask(g);

	if (g->fifo.sw_ready) {
		/* note we're not actually in an "isr", but rather
		 * in a threaded interrupt context... */
		nvgpu_mutex_acquire(&g->fifo.intr.isr.mutex);

		nvgpu_log(g, gpu_dbg_intr, "fifo isr %08x\n", fifo_intr);

		/* handle runlist update */
		if ((fifo_intr & fifo_intr_0_runlist_event_pending_f()) != 0U) {
			gk20a_fifo_handle_runlist_event(g);
			clear_intr |= fifo_intr_0_runlist_event_pending_f();
		}
		if ((fifo_intr & fifo_intr_0_pbdma_intr_pending_f()) != 0U) {
			clear_intr |= fifo_pbdma_isr(g, fifo_intr);
		}

		if (g->ops.fifo.handle_ctxsw_timeout != NULL) {
			g->ops.fifo.handle_ctxsw_timeout(g, fifo_intr);
		}

		if (unlikely((fifo_intr & error_intr_mask) != 0U)) {
			clear_intr |= fifo_error_isr(g, fifo_intr);
		}

		nvgpu_mutex_release(&g->fifo.intr.isr.mutex);
	}
	gk20a_writel(g, fifo_intr_0_r(), clear_intr);

	return;
}

u32 gk20a_fifo_nonstall_isr(struct gk20a *g)
{
	u32 fifo_intr = gk20a_readl(g, fifo_intr_0_r());
	u32 clear_intr = 0;

	nvgpu_log(g, gpu_dbg_intr, "fifo nonstall isr %08x\n", fifo_intr);

	if ((fifo_intr & fifo_intr_0_channel_intr_pending_f()) != 0U) {
		clear_intr = fifo_intr_0_channel_intr_pending_f();
	}

	gk20a_writel(g, fifo_intr_0_r(), clear_intr);

	return GK20A_NONSTALL_OPS_WAKEUP_SEMAPHORE;
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
	 * triggered every 100 ms and context switch recovery
	 * happens every 3000 ms */

	return g->fifo_eng_timeout_us / 1000U;
}

int gk20a_fifo_is_preempt_pending(struct gk20a *g, u32 id,
		unsigned int id_type)
{
	struct nvgpu_timeout timeout;
	u32 delay = GR_IDLE_CHECK_DEFAULT;
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
		delay = min_t(u32, delay << 1, GR_IDLE_CHECK_MAX);
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
	gk20a_writel(g, fifo_intr_en_0_r(), 0);
	gk20a_writel(g, fifo_intr_en_1_r(), 0);

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

void gk20a_capture_channel_ram_dump(struct gk20a *g,
		struct channel_gk20a *ch,
		struct nvgpu_channel_dump_info *info)
{
	struct nvgpu_mem *mem = &ch->inst_block;

	g->ops.channel.read_state(g, ch, &info->hw_state);

	info->inst.pb_top_level_get = nvgpu_mem_rd32_pair(g, mem,
			ram_fc_pb_top_level_get_w(),
			ram_fc_pb_top_level_get_hi_w());
	info->inst.pb_put = nvgpu_mem_rd32_pair(g, mem,
			ram_fc_pb_put_w(),
			ram_fc_pb_put_hi_w());
	info->inst.pb_get = nvgpu_mem_rd32_pair(g, mem,
			ram_fc_pb_get_w(),
			ram_fc_pb_get_hi_w());
	info->inst.pb_fetch = nvgpu_mem_rd32_pair(g, mem,
			ram_fc_pb_fetch_w(),
			ram_fc_pb_fetch_hi_w());
	info->inst.pb_header = nvgpu_mem_rd32(g, mem,
			ram_fc_pb_header_w());
	info->inst.pb_count = nvgpu_mem_rd32(g, mem,
			ram_fc_pb_count_w());
	info->inst.syncpointa = nvgpu_mem_rd32(g, mem,
			ram_fc_syncpointa_w());
	info->inst.syncpointb = nvgpu_mem_rd32(g, mem,
			ram_fc_syncpointb_w());
	info->inst.semaphorea = nvgpu_mem_rd32(g, mem,
			ram_fc_semaphorea_w());
	info->inst.semaphoreb = nvgpu_mem_rd32(g, mem,
			ram_fc_semaphoreb_w());
	info->inst.semaphorec = nvgpu_mem_rd32(g, mem,
			ram_fc_semaphorec_w());
	info->inst.semaphored = nvgpu_mem_rd32(g, mem,
			ram_fc_semaphored_w());
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

		g->ops.fifo.capture_channel_ram_dump(g, ch, info);

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

static int gk20a_fifo_commit_userd(struct channel_gk20a *c)
{
	u32 addr_lo;
	u32 addr_hi;
	struct gk20a *g = c->g;

	nvgpu_log_fn(g, " ");

	addr_lo = u64_lo32(c->userd_iova >> ram_userd_base_shift_v());
	addr_hi = u64_hi32(c->userd_iova);

	nvgpu_log_info(g, "channel %d : set ramfc userd 0x%16llx",
		c->chid, (u64)c->userd_iova);

	nvgpu_mem_wr32(g, &c->inst_block,
		       ram_in_ramfc_w() + ram_fc_userd_w(),
		       nvgpu_aperture_mask(g, c->userd_mem,
					   pbdma_userd_target_sys_mem_ncoh_f(),
					   pbdma_userd_target_sys_mem_coh_f(),
					   pbdma_userd_target_vid_mem_f()) |
		       pbdma_userd_addr_f(addr_lo));

	nvgpu_mem_wr32(g, &c->inst_block,
		       ram_in_ramfc_w() + ram_fc_userd_hi_w(),
		       pbdma_userd_hi_addr_f(addr_hi));

	return 0;
}

int gk20a_fifo_setup_ramfc(struct channel_gk20a *c,
			u64 gpfifo_base, u32 gpfifo_entries,
			unsigned long timeout,
			u32 flags)
{
	struct gk20a *g = c->g;
	struct nvgpu_mem *mem = &c->inst_block;
	unsigned long limit2_val;

	nvgpu_log_fn(g, " ");

	nvgpu_memset(g, mem, 0, 0, ram_fc_size_val_v());

	nvgpu_mem_wr32(g, mem, ram_fc_gp_base_w(),
		pbdma_gp_base_offset_f(
		u64_lo32(gpfifo_base >> pbdma_gp_base_rsvd_s())));

	limit2_val = ilog2(gpfifo_entries);
	if (u64_hi32(limit2_val) != 0U) {
		nvgpu_err(g,  "Unable to cast pbdma limit2 value");
		return -EOVERFLOW;
	}
	nvgpu_mem_wr32(g, mem, ram_fc_gp_base_hi_w(),
		pbdma_gp_base_hi_offset_f(u64_hi32(gpfifo_base)) |
		pbdma_gp_base_hi_limit2_f((u32)limit2_val));

	nvgpu_mem_wr32(g, mem, ram_fc_signature_w(),
		 c->g->ops.pbdma.get_pbdma_signature(c->g));

	nvgpu_mem_wr32(g, mem, ram_fc_formats_w(),
		pbdma_formats_gp_fermi0_f() |
		pbdma_formats_pb_fermi1_f() |
		pbdma_formats_mp_fermi0_f());

	nvgpu_mem_wr32(g, mem, ram_fc_pb_header_w(),
		pbdma_pb_header_priv_user_f() |
		pbdma_pb_header_method_zero_f() |
		pbdma_pb_header_subchannel_zero_f() |
		pbdma_pb_header_level_main_f() |
		pbdma_pb_header_first_true_f() |
		pbdma_pb_header_type_inc_f());

	nvgpu_mem_wr32(g, mem, ram_fc_subdevice_w(),
		pbdma_subdevice_id_f(1) |
		pbdma_subdevice_status_active_f() |
		pbdma_subdevice_channel_dma_enable_f());

	nvgpu_mem_wr32(g, mem, ram_fc_target_w(), pbdma_target_engine_sw_f());

	nvgpu_mem_wr32(g, mem, ram_fc_acquire_w(),
		g->ops.pbdma.pbdma_acquire_val(timeout));

	nvgpu_mem_wr32(g, mem, ram_fc_runlist_timeslice_w(),
		fifo_runlist_timeslice_timeout_128_f() |
		fifo_runlist_timeslice_timescale_3_f() |
		fifo_runlist_timeslice_enable_true_f());

	nvgpu_mem_wr32(g, mem, ram_fc_pb_timeslice_w(),
		fifo_pb_timeslice_timeout_16_f() |
		fifo_pb_timeslice_timescale_0_f() |
		fifo_pb_timeslice_enable_true_f());

	nvgpu_mem_wr32(g, mem, ram_fc_chid_w(), ram_fc_chid_id_f(c->chid));

	if (c->is_privileged_channel) {
		gk20a_fifo_setup_ramfc_for_privileged_channel(c);
	}

	return gk20a_fifo_commit_userd(c);
}

void gk20a_fifo_setup_ramfc_for_privileged_channel(struct channel_gk20a *c)
{
	struct gk20a *g = c->g;
	struct nvgpu_mem *mem = &c->inst_block;

	nvgpu_log_info(g, "channel %d : set ramfc privileged_channel", c->chid);

	/* Enable HCE priv mode for phys mode transfer */
	nvgpu_mem_wr32(g, mem, ram_fc_hce_ctrl_w(),
		pbdma_hce_ctrl_hce_priv_mode_yes_f());
}

int gk20a_fifo_setup_userd(struct channel_gk20a *c)
{
	struct gk20a *g = c->g;
	struct nvgpu_mem *mem = c->userd_mem;
	u32 offset = c->userd_offset / U32(sizeof(u32));

	nvgpu_log_fn(g, " ");

	nvgpu_mem_wr32(g, mem, offset + ram_userd_put_w(), 0);
	nvgpu_mem_wr32(g, mem, offset + ram_userd_get_w(), 0);
	nvgpu_mem_wr32(g, mem, offset + ram_userd_ref_w(), 0);
	nvgpu_mem_wr32(g, mem, offset + ram_userd_put_hi_w(), 0);
	nvgpu_mem_wr32(g, mem, offset + ram_userd_gp_top_level_get_w(), 0);
	nvgpu_mem_wr32(g, mem, offset + ram_userd_gp_top_level_get_hi_w(), 0);
	nvgpu_mem_wr32(g, mem, offset + ram_userd_get_hi_w(), 0);
	nvgpu_mem_wr32(g, mem, offset + ram_userd_gp_get_w(), 0);
	nvgpu_mem_wr32(g, mem, offset + ram_userd_gp_put_w(), 0);

	return 0;
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

u32 gk20a_fifo_userd_gp_get(struct gk20a *g, struct channel_gk20a *c)
{
	u64 userd_gpu_va = gk20a_channel_userd_gpu_va(c);
	u64 addr = userd_gpu_va + sizeof(u32) * ram_userd_gp_get_w();

	BUG_ON(u64_hi32(addr) != 0U);

	return gk20a_bar1_readl(g, (u32)addr);
}

u64 gk20a_fifo_userd_pb_get(struct gk20a *g, struct channel_gk20a *c)
{
	u64 userd_gpu_va = gk20a_channel_userd_gpu_va(c);
	u64 lo_addr = userd_gpu_va + sizeof(u32) * ram_userd_get_w();
	u64 hi_addr = userd_gpu_va + sizeof(u32) * ram_userd_get_hi_w();
	u32 lo, hi;

	BUG_ON((u64_hi32(lo_addr) != 0U) || (u64_hi32(hi_addr) != 0U));
	lo = gk20a_bar1_readl(g, (u32)lo_addr);
	hi = gk20a_bar1_readl(g, (u32)hi_addr);

	return ((u64)hi << 32) | lo;
}

void gk20a_fifo_userd_gp_put(struct gk20a *g, struct channel_gk20a *c)
{
	u64 userd_gpu_va = gk20a_channel_userd_gpu_va(c);
	u64 addr = userd_gpu_va + sizeof(u32) * ram_userd_gp_put_w();

	BUG_ON(u64_hi32(addr) != 0U);
	gk20a_bar1_writel(g, (u32)addr, c->gpfifo.put);
}

u32 gk20a_fifo_userd_entry_size(struct gk20a *g)
{
	return BIT32(ram_userd_base_shift_v());
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

	if (g->ops.fifo.init_pbdma_intr_descs != NULL) {
		g->ops.fifo.init_pbdma_intr_descs(f);
	}

	return 0;
}
