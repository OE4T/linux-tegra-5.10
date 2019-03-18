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

#include <nvgpu/hw/gv11b/hw_pbdma_gv11b.h>
#include <nvgpu/hw/gv11b/hw_ram_gv11b.h>

#include "hal/fifo/ramfc_gv11b.h"

#include "gv11b/fifo_gv11b.h"
#include "gv11b/subctx_gv11b.h"

int gv11b_ramfc_setup(struct channel_gk20a *ch, u64 gpfifo_base,
		u32 gpfifo_entries, u64 pbdma_acquire_timeout, u32 flags)
{
	struct gk20a *g = ch->g;
	struct nvgpu_mem *mem = &ch->inst_block;
	u32 data;
	bool replayable = false;

	nvgpu_log_fn(g, " ");

	nvgpu_memset(g, mem, 0, 0, ram_fc_size_val_v());

	if ((flags & NVGPU_SETUP_BIND_FLAGS_REPLAYABLE_FAULTS_ENABLE) != 0U) {
		replayable = true;
	}
	gv11b_init_subcontext_pdb(ch->vm, mem, replayable);

	nvgpu_mem_wr32(g, mem, ram_fc_gp_base_w(),
		pbdma_gp_base_offset_f(
			u64_lo32(gpfifo_base >> pbdma_gp_base_rsvd_s())));

	nvgpu_mem_wr32(g, mem, ram_fc_gp_base_hi_w(),
		pbdma_gp_base_hi_offset_f(u64_hi32(gpfifo_base)) |
		pbdma_gp_base_hi_limit2_f(ilog2(gpfifo_entries)));

	nvgpu_mem_wr32(g, mem, ram_fc_signature_w(),
		ch->g->ops.pbdma.get_pbdma_signature(ch->g));

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
		g->ops.pbdma.pbdma_acquire_val(pbdma_acquire_timeout));

	nvgpu_mem_wr32(g, mem, ram_fc_runlist_timeslice_w(),
		pbdma_runlist_timeslice_timeout_128_f() |
		pbdma_runlist_timeslice_timescale_3_f() |
		pbdma_runlist_timeslice_enable_true_f());

	nvgpu_mem_wr32(g, mem, ram_fc_chid_w(), ram_fc_chid_id_f(ch->chid));

	nvgpu_mem_wr32(g, mem, ram_fc_set_channel_info_w(),
		pbdma_set_channel_info_veid_f(ch->subctx_id));

	nvgpu_mem_wr32(g, mem, ram_in_engine_wfi_veid_w(),
		ram_in_engine_wfi_veid_f(ch->subctx_id));

	gv11b_fifo_init_ramfc_eng_method_buffer(g, ch, mem);

	if (ch->is_privileged_channel) {
		/* Set privilege level for channel */
		nvgpu_mem_wr32(g, mem, ram_fc_config_w(),
			pbdma_config_auth_level_privileged_f());

		/* Enable HCE priv mode for phys mode transfer */
		nvgpu_mem_wr32(g, mem, ram_fc_hce_ctrl_w(),
			pbdma_hce_ctrl_hce_priv_mode_yes_f());
	}

	/* Enable userd writeback */
	data = nvgpu_mem_rd32(g, mem, ram_fc_config_w());
	data = data | pbdma_config_userd_writeback_enable_f();
	nvgpu_mem_wr32(g, mem, ram_fc_config_w(), data);

	return g->ops.ramfc.commit_userd(ch);
}

void gv11b_ramfc_capture_ram_dump(struct gk20a *g, struct channel_gk20a *ch,
		struct nvgpu_channel_dump_info *info)
{
	struct nvgpu_mem *mem = &ch->inst_block;

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
	info->inst.sem_addr = nvgpu_mem_rd32_pair(g, mem,
			ram_fc_sem_addr_lo_w(),
			ram_fc_sem_addr_hi_w());
	info->inst.sem_payload = nvgpu_mem_rd32_pair(g, mem,
			ram_fc_sem_payload_lo_w(),
			ram_fc_sem_payload_hi_w());
	info->inst.sem_execute = nvgpu_mem_rd32(g, mem,
			ram_fc_sem_execute_w());
}
