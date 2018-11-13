/*
 * Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/types.h>
#include <nvgpu/timers.h>
#include <nvgpu/io.h>
#include <nvgpu/utils.h>
#include <nvgpu/log2.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/channel.h>

#include "gk20a/fifo_gk20a.h"

#include "gp10b/fifo_gp10b.h"

#include "gv11b/fifo_gv11b.h"

#include "tu104/fifo_tu104.h"
#include "tu104/func_tu104.h"

#include <nvgpu/hw/tu104/hw_fifo_tu104.h>
#include <nvgpu/hw/tu104/hw_pbdma_tu104.h>
#include <nvgpu/hw/tu104/hw_ram_tu104.h>
#include <nvgpu/hw/tu104/hw_func_tu104.h>
#include <nvgpu/hw/tu104/hw_ctrl_tu104.h>

int channel_tu104_setup_ramfc(struct channel_gk20a *c,
                u64 gpfifo_base, u32 gpfifo_entries,
                unsigned long acquire_timeout, u32 flags)
{
	struct gk20a *g = c->g;
	struct nvgpu_mem *mem = &c->inst_block;
	u32 data;

	nvgpu_log_fn(g, " ");

	nvgpu_memset(g, mem, 0, 0, ram_fc_size_val_v());

	nvgpu_mem_wr32(g, mem, ram_fc_gp_base_w(),
		pbdma_gp_base_offset_f(
		u64_lo32(gpfifo_base >> pbdma_gp_base_rsvd_s())));

	nvgpu_mem_wr32(g, mem, ram_fc_gp_base_hi_w(),
		pbdma_gp_base_hi_offset_f(u64_hi32(gpfifo_base)) |
		pbdma_gp_base_hi_limit2_f(ilog2(gpfifo_entries)));

	nvgpu_mem_wr32(g, mem, ram_fc_signature_w(),
		c->g->ops.fifo.get_pbdma_signature(c->g));

	nvgpu_mem_wr32(g, mem, ram_fc_pb_header_w(),
		pbdma_pb_header_method_zero_f() |
		pbdma_pb_header_subchannel_zero_f() |
		pbdma_pb_header_level_main_f() |
		pbdma_pb_header_first_true_f() |
		pbdma_pb_header_type_inc_f());

	nvgpu_mem_wr32(g, mem, ram_fc_subdevice_w(),
		pbdma_subdevice_id_f(PBDMA_SUBDEVICE_ID) |
		pbdma_subdevice_status_active_f() |
		pbdma_subdevice_channel_dma_enable_f());

	nvgpu_mem_wr32(g, mem, ram_fc_target_w(),
		pbdma_target_eng_ctx_valid_true_f() |
		pbdma_target_ce_ctx_valid_true_f() |
		pbdma_target_engine_sw_f());

	nvgpu_mem_wr32(g, mem, ram_fc_acquire_w(),
		g->ops.fifo.pbdma_acquire_val(acquire_timeout));

	nvgpu_mem_wr32(g, mem, ram_fc_set_channel_info_w(),
		pbdma_set_channel_info_veid_f(c->subctx_id));

	nvgpu_mem_wr32(g, mem, ram_in_engine_wfi_veid_w(),
		ram_in_engine_wfi_veid_f(c->subctx_id));

	gv11b_fifo_init_ramfc_eng_method_buffer(g, c, mem);

	if (c->is_privileged_channel) {
		/* Set privilege level for channel */
		nvgpu_mem_wr32(g, mem, ram_fc_config_w(),
		pbdma_config_auth_level_privileged_f());

		gk20a_fifo_setup_ramfc_for_privileged_channel(c);
	}

	/* Enable userd writeback */
	data = nvgpu_mem_rd32(g, mem, ram_fc_config_w());
	data = data | pbdma_config_userd_writeback_enable_f();
	nvgpu_mem_wr32(g, mem, ram_fc_config_w(),data);

	gv11b_userd_writeback_config(g);

	return channel_gp10b_commit_userd(c);
}

void tu104_fifo_runlist_hw_submit(struct gk20a *g, u32 runlist_id,
	u32 count, u32 buffer_index)
{
	struct fifo_runlist_info_gk20a *runlist = NULL;
	u64 runlist_iova;
	u32 runlist_iova_lo, runlist_iova_hi;

	runlist = &g->fifo.runlist_info[runlist_id];
	runlist_iova = nvgpu_mem_get_addr(g, &runlist->mem[buffer_index]);

	runlist_iova_lo = u64_lo32(runlist_iova) >>
				fifo_runlist_base_lo_ptr_align_shift_v();
	runlist_iova_hi = u64_hi32(runlist_iova);

	if (count != 0) {
		nvgpu_writel(g, fifo_runlist_base_lo_r(runlist_id),
			fifo_runlist_base_lo_ptr_lo_f(runlist_iova_lo) |
			nvgpu_aperture_mask(g, &runlist->mem[buffer_index],
				fifo_runlist_base_lo_target_sys_mem_ncoh_f(),
				fifo_runlist_base_lo_target_sys_mem_coh_f(),
				fifo_runlist_base_lo_target_vid_mem_f()));

		nvgpu_writel(g, fifo_runlist_base_hi_r(runlist_id),
			fifo_runlist_base_hi_ptr_hi_f(runlist_iova_hi));
	}

	nvgpu_writel(g, fifo_runlist_submit_r(runlist_id),
		fifo_runlist_submit_length_f(count));
}

int tu104_fifo_runlist_wait_pending(struct gk20a *g, u32 runlist_id)
{
	struct nvgpu_timeout timeout;
	unsigned long delay = GR_IDLE_CHECK_DEFAULT;
	int ret = -ETIMEDOUT;

	ret = nvgpu_timeout_init(g, &timeout, gk20a_get_gr_idle_timeout(g),
			   NVGPU_TIMER_CPU_TIMER);
	if (ret != 0) {
		return ret;
	}

	ret = -ETIMEDOUT;
	do {
		if ((nvgpu_readl(g, fifo_runlist_submit_info_r(runlist_id)) &
			      fifo_runlist_submit_info_pending_true_f()) == 0) {
			ret = 0;
			break;
		}

		nvgpu_usleep_range(delay, delay * 2);
		delay = min_t(u32, delay << 1, GR_IDLE_CHECK_MAX);
	} while (nvgpu_timeout_expired(&timeout) == 0);

	return ret;
}

int tu104_init_fifo_setup_hw(struct gk20a *g)
{
	u32 val;

	nvgpu_log_fn(g, " ");

	/*
	 * Required settings for tu104_ring_channel_doorbell()
	 */
	val = nvgpu_readl(g, ctrl_virtual_channel_cfg_r(0));
	val |= ctrl_virtual_channel_cfg_pending_enable_true_f();
	nvgpu_writel(g, ctrl_virtual_channel_cfg_r(0), val);

	return gv11b_init_fifo_setup_hw(g);
}

void tu104_ring_channel_doorbell(struct channel_gk20a *c)
{
	struct fifo_gk20a *f = &c->g->fifo;
	u32 hw_chid = f->channel_base + c->chid;

	nvgpu_log_info(c->g, "channel ring door bell %d, runlist %d",
		c->chid, c->runlist_id);

	nvgpu_func_writel(c->g, func_doorbell_r(),
		ctrl_doorbell_vector_f(hw_chid) |
		ctrl_doorbell_runlist_id_f(c->runlist_id));
}

u64 tu104_fifo_usermode_base(struct gk20a *g)
{
	return U64(func_full_phys_offset_v()) + func_cfg0_r();
}

u32 tu104_fifo_doorbell_token(struct channel_gk20a *c)
{
	struct gk20a *g = c->g;
	struct fifo_gk20a *f = &g->fifo;
	u32 hw_chid = f->channel_base + c->chid;

	return ctrl_doorbell_vector_f(hw_chid) |
		ctrl_doorbell_runlist_id_f(c->runlist_id);
}

int tu104_init_pdb_cache_war(struct gk20a *g)
{
	u32 size = PAGE_SIZE * 258U;
	u64 last_bind_pdb_addr;
	u64 pdb_addr;
	u32 pdb_addr_lo, pdb_addr_hi;
	u32 i;
	int err;

	if (nvgpu_mem_is_valid(&g->pdb_cache_war_mem)) {
		return 0;
	}

	/*
	 * Allocate memory for 257 instance block binds +
	 * PDB bound to 257th instance block
	 */
	err = nvgpu_dma_alloc_sys(g, size, &g->pdb_cache_war_mem);
	if (err != 0) {
		return err;
	}

	/*
	 * 257th instance block (i.e. last bind) needs to be bound to
	 * valid memory
	 * First 256 binds can happen to dummy addresses
	 */
	pdb_addr = PAGE_SIZE;
	last_bind_pdb_addr = nvgpu_mem_get_addr(g, &g->pdb_cache_war_mem) +
			     (257U * PAGE_SIZE);

	/* Setup first 256 instance blocks */
	for (i = 0U; i < 256U; i++) {
		pdb_addr_lo = u64_lo32(pdb_addr >> ram_in_base_shift_v());
		pdb_addr_hi = u64_hi32(pdb_addr);

		nvgpu_mem_wr32(g, &g->pdb_cache_war_mem,
			ram_in_page_dir_base_lo_w() + (i * PAGE_SIZE / 4),
			nvgpu_aperture_mask(g, &g->pdb_cache_war_mem,
				ram_in_page_dir_base_target_sys_mem_ncoh_f(),
				ram_in_page_dir_base_target_sys_mem_coh_f(),
				ram_in_page_dir_base_target_vid_mem_f()) |
			ram_in_page_dir_base_vol_true_f() |
			ram_in_big_page_size_64kb_f() |
			ram_in_page_dir_base_lo_f(pdb_addr_lo) |
			ram_in_use_ver2_pt_format_true_f());

		nvgpu_mem_wr32(g, &g->pdb_cache_war_mem,
			ram_in_page_dir_base_hi_w() + (i * PAGE_SIZE / 4),
			ram_in_page_dir_base_hi_f(pdb_addr_hi));

		pdb_addr += PAGE_SIZE;
	}

	/* Setup 257th instance block */
	pdb_addr_lo = u64_lo32(last_bind_pdb_addr >> ram_in_base_shift_v());
	pdb_addr_hi = u64_hi32(last_bind_pdb_addr);

	nvgpu_mem_wr32(g, &g->pdb_cache_war_mem,
		ram_in_page_dir_base_lo_w() + (256U * PAGE_SIZE / 4),
		nvgpu_aperture_mask(g, &g->pdb_cache_war_mem,
			ram_in_page_dir_base_target_sys_mem_ncoh_f(),
			ram_in_page_dir_base_target_sys_mem_coh_f(),
			ram_in_page_dir_base_target_vid_mem_f()) |
		ram_in_page_dir_base_vol_true_f() |
		ram_in_big_page_size_64kb_f() |
		ram_in_page_dir_base_lo_f(pdb_addr_lo) |
		ram_in_use_ver2_pt_format_true_f());

	nvgpu_mem_wr32(g, &g->pdb_cache_war_mem,
		ram_in_page_dir_base_hi_w() + (256U * PAGE_SIZE / 4),
		ram_in_page_dir_base_hi_f(pdb_addr_hi));

	return 0;
}

void tu104_deinit_pdb_cache_war(struct gk20a *g)
{
	if (nvgpu_mem_is_valid(&g->pdb_cache_war_mem)) {
		nvgpu_dma_free(g, &g->pdb_cache_war_mem);
	}
}
