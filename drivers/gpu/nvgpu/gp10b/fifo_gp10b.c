/*
 * GP10B fifo
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

#include <nvgpu/dma.h>
#include <nvgpu/bug.h>
#include <nvgpu/log2.h>
#include <nvgpu/enabled.h>
#include <nvgpu/io.h>
#include <nvgpu/utils.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/channel.h>
#include <nvgpu/channel_sync.h>
#include <nvgpu/channel_sync_syncpt.h>
#include <nvgpu/engines.h>
#include <nvgpu/top.h>

#include "fifo_gp10b.h"

#include "gk20a/fifo_gk20a.h"
#include "gm20b/fifo_gm20b.h"

#include <nvgpu/hw/gp10b/hw_pbdma_gp10b.h>
#include <nvgpu/hw/gp10b/hw_fifo_gp10b.h>
#include <nvgpu/hw/gp10b/hw_ram_gp10b.h>

int gp10b_fifo_resetup_ramfc(struct channel_gk20a *c)
{
	u32 new_syncpt = 0, old_syncpt;
	u32 v;
	struct gk20a *g = c->g;
	struct nvgpu_channel_sync_syncpt *sync_syncpt = NULL;

	nvgpu_log_fn(g, " ");

	v = nvgpu_mem_rd32(c->g, &c->inst_block,
			ram_fc_allowed_syncpoints_w());
	old_syncpt = pbdma_allowed_syncpoints_0_index_v(v);
	if (c->sync != NULL) {
		sync_syncpt = nvgpu_channel_sync_to_syncpt(c->sync);
		if (sync_syncpt != NULL) {
			new_syncpt =
			    nvgpu_channel_sync_get_syncpt_id(sync_syncpt);
		} else {
			new_syncpt = FIFO_INVAL_SYNCPT_ID;
		}
	}
	if ((new_syncpt != 0U) && (new_syncpt != old_syncpt)) {
		/* disable channel */
		gk20a_disable_channel_tsg(c->g, c);

		/* preempt the channel */
		WARN_ON(gk20a_fifo_preempt(c->g, c) != 0);

		v = pbdma_allowed_syncpoints_0_valid_f(1);

		nvgpu_log_info(g, "Channel %d, syncpt id %d\n",
				c->chid, new_syncpt);

		v |= pbdma_allowed_syncpoints_0_index_f(new_syncpt);

		nvgpu_mem_wr32(c->g, &c->inst_block,
				ram_fc_allowed_syncpoints_w(), v);
	}

	/* enable channel */
	gk20a_enable_channel_tsg(c->g, c);

	nvgpu_log_fn(g, "done");

	return 0;
}

int gp10b_fifo_init_ce_engine_info(struct fifo_gk20a *f)
{
	struct gk20a *g = f->g;
	int ret = 0;
	u32 i;
	enum nvgpu_fifo_engine engine_enum;
	u32 gr_runlist_id;
	u32 pbdma_id = U32_MAX;
	bool found_pbdma_for_runlist = false;
	u32 lce_num_entries = 0;

	gr_runlist_id = gk20a_fifo_get_gr_runlist_id(g);
	nvgpu_log_info(g, "gr_runlist_id: %d", gr_runlist_id);

	if (g->ops.top.get_num_engine_type_entries != NULL) {
		lce_num_entries = g->ops.top.get_num_engine_type_entries(g,
							NVGPU_ENGINE_LCE);
		nvgpu_log_info(g, "lce_num_entries: %d", lce_num_entries);
	}

	for (i = 0; i < lce_num_entries; i++) {
		struct nvgpu_device_info dev_info;
		struct fifo_engine_info_gk20a *info;

		ret = g->ops.top.get_device_info(g, &dev_info,
						NVGPU_ENGINE_LCE, i);
		if (ret != 0) {
			nvgpu_err(g,
				"Failed to parse dev_info for engine%d",
				NVGPU_ENGINE_LCE);
			return -EINVAL;
		}

		found_pbdma_for_runlist =
				g->ops.fifo.find_pbdma_for_runlist(f,
						dev_info.runlist_id,
						&pbdma_id);
		if (!found_pbdma_for_runlist) {
			nvgpu_err(g, "busted pbdma map");
			return -EINVAL;
		}

		info = &g->fifo.engine_info[dev_info.engine_id];

		engine_enum = nvgpu_engine_enum_from_type(
					g,
					dev_info.engine_type);
		/* GR and GR_COPY shares same runlist_id */
		if ((engine_enum == NVGPU_ENGINE_ASYNC_CE_GK20A) &&
			(gr_runlist_id == dev_info.runlist_id)) {
				engine_enum = NVGPU_ENGINE_GRCE_GK20A;
		}
		info->engine_enum = engine_enum;

		if (g->ops.top.get_ce_inst_id != NULL) {
			dev_info.inst_id = g->ops.top.get_ce_inst_id(g,
						dev_info.engine_type);
		}

		if ((dev_info.fault_id == 0U) &&
				(engine_enum == NVGPU_ENGINE_GRCE_GK20A)) {
			dev_info.fault_id = 0x1b;
		}
		info->fault_id = dev_info.fault_id;


		info->intr_mask |= BIT32(dev_info.intr_id);
		info->reset_mask |= BIT32(dev_info.reset_id);
		info->runlist_id = dev_info.runlist_id;
		info->pbdma_id = pbdma_id;
		info->inst_id  = dev_info.inst_id;
		info->pri_base = dev_info.pri_base;


		/* engine_id starts from 0 to NV_HOST_NUM_ENGINES */
		f->active_engines_list[f->num_engines] =
						dev_info.engine_id;
		++f->num_engines;
		nvgpu_log_info(g, "gr info: engine_id %d runlist_id %d "
			"intr_id %d reset_id %d engine_type %d "
			"engine_enum %d inst_id %d",
			dev_info.engine_id,
			dev_info.runlist_id,
			dev_info.intr_id,
			dev_info.reset_id,
			dev_info.engine_type,
			engine_enum,
			dev_info.inst_id);
	}
	return 0;
}

void gp10b_fifo_get_mmu_fault_info(struct gk20a *g, u32 mmu_fault_id,
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
		fifo_intr_mmu_fault_info_access_type_v(fault_info);
	mmfault->client_type =
		fifo_intr_mmu_fault_info_client_type_v(fault_info);
	mmfault->client_id =
		fifo_intr_mmu_fault_info_client_v(fault_info);

	addr_lo = gk20a_readl(g, fifo_intr_mmu_fault_lo_r(mmu_fault_id));
	addr_hi = gk20a_readl(g, fifo_intr_mmu_fault_hi_r(mmu_fault_id));
	mmfault->fault_addr = hi32_lo32_to_u64(addr_hi, addr_lo);
	/* note:ignoring aperture */
	mmfault->inst_ptr = fifo_intr_mmu_fault_inst_ptr_v(
		 gk20a_readl(g, fifo_intr_mmu_fault_inst_r(mmu_fault_id)));
	/* note: inst_ptr is a 40b phys addr.  */
	mmfault->inst_ptr <<= fifo_intr_mmu_fault_inst_ptr_align_shift_v();
}
/* fault info/descriptions */
static const char * const gp10b_fault_type_descs[] = {
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
	 "atomic violation",
};

static const char * const gp10b_hub_client_descs[] = {
	"vip", "ce0", "ce1", "dniso", "fe", "fecs", "host", "host cpu",
	"host cpu nb", "iso", "mmu", "mspdec", "msppp", "msvld",
	"niso", "p2p", "pd", "perf", "pmu", "raster twod", "scc",
	"scc nb", "sec", "ssync", "gr copy", "xv", "mmu nb",
	"msenc", "d falcon", "sked", "a falcon", "n/a",
	"hsce0", "hsce1", "hsce2", "hsce3", "hsce4", "hsce5",
	"hsce6", "hsce7", "hsce8", "hsce9", "hshub",
	"ptp x0", "ptp x1", "ptp x2", "ptp x3", "ptp x4",
	"ptp x5", "ptp x6", "ptp x7", "vpr scrubber0", "vpr scrubber1",
};

/* fill in mmu fault desc */
void gp10b_fifo_get_mmu_fault_desc(struct mmu_fault_info *mmfault)
{
	if (mmfault->fault_type >= ARRAY_SIZE(gp10b_fault_type_descs)) {
		WARN_ON(mmfault->fault_type >=
				ARRAY_SIZE(gp10b_fault_type_descs));
	} else {
		mmfault->fault_type_desc =
			 gp10b_fault_type_descs[mmfault->fault_type];
	}
}

/* fill in mmu fault client description */
void gp10b_fifo_get_mmu_fault_client_desc(struct mmu_fault_info *mmfault)
{
	if (mmfault->client_id >= ARRAY_SIZE(gp10b_hub_client_descs)) {
		WARN_ON(mmfault->client_id >=
				ARRAY_SIZE(gp10b_hub_client_descs));
	} else {
		mmfault->client_id_desc =
			 gp10b_hub_client_descs[mmfault->client_id];
	}
}
