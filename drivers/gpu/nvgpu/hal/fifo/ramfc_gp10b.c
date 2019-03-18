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

#include <nvgpu/log.h>
#include <nvgpu/log2.h>
#include <nvgpu/nvgpu_mem.h>
#include <nvgpu/io.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/channel.h>

#include "gk20a/fifo_gk20a.h"

#include "hal/fifo/ramfc_gk20a.h"
#include "hal/fifo/ramfc_gp10b.h"

#include <nvgpu/hw/gp10b/hw_pbdma_gp10b.h>
#include <nvgpu/hw/gp10b/hw_ram_gp10b.h>

int gp10b_ramfc_commit_userd(struct channel_gk20a *ch)
{
	u32 addr_lo;
	u32 addr_hi;
	struct gk20a *g = ch->g;

	nvgpu_log_fn(g, " ");

	addr_lo = u64_lo32(ch->userd_iova >> ram_userd_base_shift_v());
	addr_hi = u64_hi32(ch->userd_iova);

	nvgpu_log_info(g, "channel %d : set ramfc userd 0x%16llx",
		ch->chid, (u64)ch->userd_iova);

	nvgpu_mem_wr32(g, &ch->inst_block,
		ram_in_ramfc_w() + ram_fc_userd_w(),
		nvgpu_aperture_mask(g, ch->userd_mem,
			pbdma_userd_target_sys_mem_ncoh_f(),
			pbdma_userd_target_sys_mem_coh_f(),
			pbdma_userd_target_vid_mem_f()) |
		pbdma_userd_addr_f(addr_lo));

	nvgpu_mem_wr32(g, &ch->inst_block,
		ram_in_ramfc_w() + ram_fc_userd_hi_w(),
		pbdma_userd_hi_addr_f(addr_hi));

	return 0;
}

int gp10b_ramfc_setup(struct channel_gk20a *ch, u64 gpfifo_base,
		u32 gpfifo_entries, u64 pbdma_acquire_timeout, u32 flags)
{
	struct gk20a *g = ch->g;
	struct nvgpu_mem *mem = &ch->inst_block;

	nvgpu_log_fn(g, " ");

	nvgpu_memset(g, mem, 0, 0, ram_fc_size_val_v());

	nvgpu_mem_wr32(g, mem, ram_fc_gp_base_w(),
		pbdma_gp_base_offset_f(
			u64_lo32(gpfifo_base >> pbdma_gp_base_rsvd_s())));

	nvgpu_mem_wr32(g, mem, ram_fc_gp_base_hi_w(),
		pbdma_gp_base_hi_offset_f(u64_hi32(gpfifo_base)) |
		pbdma_gp_base_hi_limit2_f((u32)ilog2(gpfifo_entries)));

	nvgpu_mem_wr32(g, mem, ram_fc_signature_w(),
		ch->g->ops.pbdma.get_pbdma_signature(ch->g));

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
		g->ops.pbdma.pbdma_acquire_val(pbdma_acquire_timeout));

	nvgpu_mem_wr32(g, mem, ram_fc_runlist_timeslice_w(),
		pbdma_runlist_timeslice_timeout_128_f() |
		pbdma_runlist_timeslice_timescale_3_f() |
		pbdma_runlist_timeslice_enable_true_f());

	nvgpu_mem_wr32(g, mem, ram_fc_chid_w(), ram_fc_chid_id_f(ch->chid));

	if (ch->is_privileged_channel) {
		/* Set privilege level for channel */
		nvgpu_mem_wr32(g, mem, ram_fc_config_w(),
			pbdma_config_auth_level_privileged_f());

		/* Enable HCE priv mode for phys mode transfer */
		nvgpu_mem_wr32(g, mem, ram_fc_hce_ctrl_w(),
			pbdma_hce_ctrl_hce_priv_mode_yes_f());
	}

	return g->ops.ramfc.commit_userd(ch);
}

u32 gp10b_ramfc_get_syncpt(struct channel_gk20a *ch)
{
	struct gk20a *g = ch->g;
	u32 v, syncpt;

	v = nvgpu_mem_rd32(g, &ch->inst_block, ram_fc_allowed_syncpoints_w());
	syncpt = pbdma_allowed_syncpoints_0_index_v(v);

	return syncpt;
}

void gp10b_ramfc_set_syncpt(struct channel_gk20a *ch, u32 syncpt)
{
	struct gk20a *g = ch->g;
	u32 v = pbdma_allowed_syncpoints_0_valid_f(1) |
		pbdma_allowed_syncpoints_0_index_f(syncpt);

	nvgpu_log_info(g, "Channel %d, syncpt id %d\n", ch->chid, syncpt);

	nvgpu_mem_wr32(g, &ch->inst_block, ram_fc_allowed_syncpoints_w(), v);
}

