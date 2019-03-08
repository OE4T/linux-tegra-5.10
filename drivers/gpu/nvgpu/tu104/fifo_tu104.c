/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION.  All rights reserved.
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
			ram_in_page_dir_base_lo_w() + (i * PAGE_SIZE / 4U),
			nvgpu_aperture_mask(g, &g->pdb_cache_war_mem,
				ram_in_page_dir_base_target_sys_mem_ncoh_f(),
				ram_in_page_dir_base_target_sys_mem_coh_f(),
				ram_in_page_dir_base_target_vid_mem_f()) |
			ram_in_page_dir_base_vol_true_f() |
			ram_in_big_page_size_64kb_f() |
			ram_in_page_dir_base_lo_f(pdb_addr_lo) |
			ram_in_use_ver2_pt_format_true_f());

		nvgpu_mem_wr32(g, &g->pdb_cache_war_mem,
			ram_in_page_dir_base_hi_w() + (i * PAGE_SIZE / 4U),
			ram_in_page_dir_base_hi_f(pdb_addr_hi));

		pdb_addr += PAGE_SIZE;
	}

	/* Setup 257th instance block */
	pdb_addr_lo = u64_lo32(last_bind_pdb_addr >> ram_in_base_shift_v());
	pdb_addr_hi = u64_hi32(last_bind_pdb_addr);

	nvgpu_mem_wr32(g, &g->pdb_cache_war_mem,
		ram_in_page_dir_base_lo_w() + (256U * PAGE_SIZE / 4U),
		nvgpu_aperture_mask(g, &g->pdb_cache_war_mem,
			ram_in_page_dir_base_target_sys_mem_ncoh_f(),
			ram_in_page_dir_base_target_sys_mem_coh_f(),
			ram_in_page_dir_base_target_vid_mem_f()) |
		ram_in_page_dir_base_vol_true_f() |
		ram_in_big_page_size_64kb_f() |
		ram_in_page_dir_base_lo_f(pdb_addr_lo) |
		ram_in_use_ver2_pt_format_true_f());

	nvgpu_mem_wr32(g, &g->pdb_cache_war_mem,
		ram_in_page_dir_base_hi_w() + (256U * PAGE_SIZE / 4U),
		ram_in_page_dir_base_hi_f(pdb_addr_hi));

	return 0;
}

void tu104_deinit_pdb_cache_war(struct gk20a *g)
{
	if (nvgpu_mem_is_valid(&g->pdb_cache_war_mem)) {
		nvgpu_dma_free(g, &g->pdb_cache_war_mem);
	}
}

