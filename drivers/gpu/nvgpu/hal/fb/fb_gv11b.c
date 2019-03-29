/*
 * GV11B FB
 *
 * Copyright (c) 2016-2019, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/log.h>
#include <nvgpu/enabled.h>
#include <nvgpu/gmmu.h>
#include <nvgpu/barrier.h>
#include <nvgpu/bug.h>
#include <nvgpu/soc.h>
#include <nvgpu/ptimer.h>
#include <nvgpu/io.h>
#include <nvgpu/utils.h>
#include <nvgpu/timers.h>
#include <nvgpu/fifo.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/channel.h>
#include <nvgpu/tsg.h>
#include <nvgpu/nvgpu_err.h>
#include <nvgpu/ltc.h>
#include <nvgpu/rc.h>

#include "gk20a/mm_gk20a.h"

#include "fb_gm20b.h"
#include "fb_gp10b.h"
#include "fb_gv11b.h"

#include <nvgpu/hw/gv11b/hw_fb_gv11b.h>
#include <nvgpu/hw/gv11b/hw_gmmu_gv11b.h>

static int gv11b_fb_fix_page_fault(struct gk20a *g,
		 struct mmu_fault_info *mmfault);

static void gv11b_init_nvlink_soc_credits(struct gk20a *g)
{
	if (nvgpu_is_bpmp_running(g) && (!nvgpu_platform_is_simulation(g))) {
		nvgpu_log(g, gpu_dbg_info, "nvlink soc credits init done by bpmp");
	} else {
#ifndef __NVGPU_POSIX__
		nvgpu_mss_nvlink_init_credits(g);
#endif
	}
}

static void gv11b_fb_set_atomic_mode(struct gk20a *g)
{
	u32 reg_val;

	/*
	 * NV_PFB_PRI_MMU_CTRL_ATOMIC_CAPABILITY_MODE to RMW MODE
	 * NV_PFB_PRI_MMU_CTRL_ATOMIC_CAPABILITY_SYS_NCOH_MODE to L2
	 */
	reg_val = nvgpu_readl(g, fb_mmu_ctrl_r());
	reg_val = set_field(reg_val, fb_mmu_ctrl_atomic_capability_mode_m(),
		fb_mmu_ctrl_atomic_capability_mode_rmw_f());
	reg_val = set_field(reg_val, fb_mmu_ctrl_atomic_capability_sys_ncoh_mode_m(),
		fb_mmu_ctrl_atomic_capability_sys_ncoh_mode_l2_f());
	nvgpu_writel(g, fb_mmu_ctrl_r(), reg_val);

	/* NV_PFB_HSHUB_NUM_ACTIVE_LTCS_HUB_SYS_ATOMIC_MODE to USE_RMW */
	reg_val = nvgpu_readl(g, fb_hshub_num_active_ltcs_r());
	reg_val = set_field(reg_val, fb_hshub_num_active_ltcs_hub_sys_atomic_mode_m(),
		    fb_hshub_num_active_ltcs_hub_sys_atomic_mode_use_rmw_f());
	nvgpu_writel(g, fb_hshub_num_active_ltcs_r(), reg_val);

	nvgpu_log(g,  gpu_dbg_info, "fb_mmu_ctrl_r 0x%x",
					gk20a_readl(g, fb_mmu_ctrl_r()));

	nvgpu_log(g,   gpu_dbg_info, "fb_hshub_num_active_ltcs_r 0x%x",
			gk20a_readl(g, fb_hshub_num_active_ltcs_r()));
}

void gv11b_fb_init_hw(struct gk20a *g)
{
	gm20b_fb_init_hw(g);

	g->ops.fb.intr.enable(g);
}

void gv11b_fb_init_fs_state(struct gk20a *g)
{
	nvgpu_log(g, gpu_dbg_fn, "initialize gv11b fb");

	gv11b_init_nvlink_soc_credits(g);

	gv11b_fb_set_atomic_mode(g);

	nvgpu_log(g, gpu_dbg_info, "fbhub active ltcs %x",
			gk20a_readl(g, fb_fbhub_num_active_ltcs_r()));

	nvgpu_log(g, gpu_dbg_info, "mmu active ltcs %u",
			fb_mmu_num_active_ltcs_count_v(
			gk20a_readl(g, fb_mmu_num_active_ltcs_r())));

	if (!nvgpu_is_enabled(g, NVGPU_SEC_PRIVSECURITY)) {
		/* Bypass MMU check for non-secure boot. For
		 * secure-boot,this register write has no-effect */
		gk20a_writel(g, fb_priv_mmu_phy_secure_r(), 0xffffffffU);
	}
}

void gv11b_fb_cbc_configure(struct gk20a *g, struct nvgpu_cbc *cbc)
{
	u32 compbit_base_post_divide;
	u64 compbit_base_post_multiply64;
	u64 compbit_store_iova;
	u64 compbit_base_post_divide64;

	if (nvgpu_is_enabled(g, NVGPU_IS_FMODEL)) {
		compbit_store_iova = nvgpu_mem_get_phys_addr(g,
						&cbc->compbit_store.mem);
	} else {
		compbit_store_iova = nvgpu_mem_get_addr(g,
						&cbc->compbit_store.mem);
	}
	/* must be aligned to 64 KB */
	compbit_store_iova = roundup(compbit_store_iova, (u64)SZ_64K);

	compbit_base_post_divide64 = compbit_store_iova >>
		fb_mmu_cbc_base_address_alignment_shift_v();

	do_div(compbit_base_post_divide64, nvgpu_ltc_get_ltc_count(g));
	compbit_base_post_divide = u64_lo32(compbit_base_post_divide64);

	compbit_base_post_multiply64 = ((u64)compbit_base_post_divide *
		nvgpu_ltc_get_ltc_count(g))
			<< fb_mmu_cbc_base_address_alignment_shift_v();

	if (compbit_base_post_multiply64 < compbit_store_iova) {
		compbit_base_post_divide++;
	}

	if (g->ops.cbc.fix_config != NULL) {
		compbit_base_post_divide =
			g->ops.cbc.fix_config(g, compbit_base_post_divide);
	}

	gk20a_writel(g, fb_mmu_cbc_base_r(),
		fb_mmu_cbc_base_address_f(compbit_base_post_divide));

	nvgpu_log(g, gpu_dbg_info | gpu_dbg_map_v | gpu_dbg_pte,
		"compbit base.pa: 0x%x,%08x cbc_base:0x%08x\n",
		(u32)(compbit_store_iova >> 32),
		(u32)(compbit_store_iova & 0xffffffffU),
		compbit_base_post_divide);
	nvgpu_log(g, gpu_dbg_fn, "cbc base %x",
		gk20a_readl(g, fb_mmu_cbc_base_r()));

	cbc->compbit_store.base_hw = compbit_base_post_divide;

}

static const char * const invalid_str = "invalid";

static const char *const fault_type_descs_gv11b[] = {
	"invalid pde",
	"invalid pde size",
	"invalid pte",
	"limit violation",
	"unbound inst block",
	"priv violation",
	"write",
	"read",
	"pitch mask violation",
	"work creation",
	"unsupported aperture",
	"compression failure",
	"unsupported kind",
	"region violation",
	"poison",
	"atomic"
};

static const char *const fault_client_type_descs_gv11b[] = {
	"gpc",
	"hub",
};

static const char *const fault_access_type_descs_gv11b[] = {
	"virt read",
	"virt write",
	"virt atomic strong",
	"virt prefetch",
	"virt atomic weak",
	"xxx",
	"xxx",
	"xxx",
	"phys read",
	"phys write",
	"phys atomic",
	"phys prefetch",
};

static const char *const hub_client_descs_gv11b[] = {
	"vip", "ce0", "ce1", "dniso", "fe", "fecs", "host", "host cpu",
	"host cpu nb", "iso", "mmu", "nvdec", "nvenc1", "nvenc2",
	"niso", "p2p", "pd", "perf", "pmu", "raster twod", "scc",
	"scc nb", "sec", "ssync", "gr copy", "xv", "mmu nb",
	"nvenc", "d falcon", "sked", "a falcon", "hsce0", "hsce1",
	"hsce2", "hsce3", "hsce4", "hsce5", "hsce6", "hsce7", "hsce8",
	"hsce9", "hshub", "ptp x0", "ptp x1", "ptp x2", "ptp x3",
	"ptp x4", "ptp x5", "ptp x6", "ptp x7", "vpr scrubber0",
	"vpr scrubber1", "dwbif", "fbfalcon", "ce shim", "gsp",
	"dont care"
};

static const char *const gpc_client_descs_gv11b[] = {
	"t1 0", "t1 1", "t1 2", "t1 3",
	"t1 4", "t1 5", "t1 6", "t1 7",
	"pe 0", "pe 1", "pe 2", "pe 3",
	"pe 4", "pe 5", "pe 6", "pe 7",
	"rast", "gcc", "gpccs",
	"prop 0", "prop 1", "prop 2", "prop 3",
	"gpm",
	"ltp utlb 0", "ltp utlb 1", "ltp utlb 2", "ltp utlb 3",
	"ltp utlb 4", "ltp utlb 5", "ltp utlb 6", "ltp utlb 7",
	"utlb",
	"t1 8", "t1 9", "t1 10", "t1 11",
	"t1 12", "t1 13", "t1 14", "t1 15",
	"tpccs 0", "tpccs 1", "tpccs 2", "tpccs 3",
	"tpccs 4", "tpccs 5", "tpccs 6", "tpccs 7",
	"pe 8", "pe 9", "tpccs 8", "tpccs 9",
	"t1 16", "t1 17", "t1 18", "t1 19",
	"pe 10", "pe 11", "tpccs 10", "tpccs 11",
	"t1 20", "t1 21", "t1 22", "t1 23",
	"pe 12", "pe 13", "tpccs 12", "tpccs 13",
	"t1 24", "t1 25", "t1 26", "t1 27",
	"pe 14", "pe 15", "tpccs 14", "tpccs 15",
	"t1 28", "t1 29", "t1 30", "t1 31",
	"pe 16", "pe 17", "tpccs 16", "tpccs 17",
	"t1 32", "t1 33", "t1 34", "t1 35",
	"pe 18", "pe 19", "tpccs 18", "tpccs 19",
	"t1 36", "t1 37", "t1 38", "t1 39",
};

bool gv11b_fb_is_fault_buf_enabled(struct gk20a *g, u32 index)
{
	u32 reg_val;

	reg_val = g->ops.fb.read_mmu_fault_buffer_size(g, index);
	return fb_mmu_fault_buffer_size_enable_v(reg_val) != 0U;
}

static void gv11b_fb_fault_buffer_get_ptr_update(struct gk20a *g,
				 u32 index, u32 next)
{
	u32 reg_val;

	nvgpu_log(g, gpu_dbg_intr, "updating get index with = %d", next);

	reg_val = g->ops.fb.read_mmu_fault_buffer_get(g, index);
	reg_val = set_field(reg_val, fb_mmu_fault_buffer_get_ptr_m(),
			 fb_mmu_fault_buffer_get_ptr_f(next));

	/* while the fault is being handled it is possible for overflow
	 * to happen,
	 */
	if ((reg_val & fb_mmu_fault_buffer_get_overflow_m()) != 0U) {
		reg_val |= fb_mmu_fault_buffer_get_overflow_clear_f();
	}

	g->ops.fb.write_mmu_fault_buffer_get(g, index, reg_val);

	/* make sure get ptr update is visible to everyone to avoid
	 * reading already read entry
	 */
	nvgpu_mb();
}

static u32 gv11b_fb_fault_buffer_get_index(struct gk20a *g, u32 index)
{
	u32 reg_val;

	reg_val = g->ops.fb.read_mmu_fault_buffer_get(g, index);
	return fb_mmu_fault_buffer_get_ptr_v(reg_val);
}

static u32 gv11b_fb_fault_buffer_put_index(struct gk20a *g, u32 index)
{
	u32 reg_val;

	reg_val = g->ops.fb.read_mmu_fault_buffer_put(g, index);
	return fb_mmu_fault_buffer_put_ptr_v(reg_val);
}

static u32 gv11b_fb_fault_buffer_size_val(struct gk20a *g, u32 index)
{
	u32 reg_val;

	reg_val = g->ops.fb.read_mmu_fault_buffer_size(g, index);
	return fb_mmu_fault_buffer_size_val_v(reg_val);
}

static bool gv11b_fb_is_fault_buffer_empty(struct gk20a *g,
		 u32 index, u32 *get_idx)
{
	u32 put_idx;

	*get_idx = gv11b_fb_fault_buffer_get_index(g, index);
	put_idx = gv11b_fb_fault_buffer_put_index(g, index);

	return *get_idx == put_idx;
}

static bool gv11b_fb_is_fault_buffer_full(struct gk20a *g, u32 index)
{
	u32 get_idx, put_idx, entries;


	get_idx = gv11b_fb_fault_buffer_get_index(g, index);

	put_idx = gv11b_fb_fault_buffer_put_index(g, index);

	entries = gv11b_fb_fault_buffer_size_val(g, index);

	return get_idx == ((put_idx + 1U) % entries);
}

void gv11b_fb_fault_buf_set_state_hw(struct gk20a *g,
		 u32 index, u32 state)
{
	u32 fault_status;
	u32 reg_val;
	int err = 0;

	nvgpu_log_fn(g, " ");

	reg_val = g->ops.fb.read_mmu_fault_buffer_size(g, index);
	if (state == NVGPU_FB_MMU_FAULT_BUF_ENABLED) {
		if (gv11b_fb_is_fault_buf_enabled(g, index)) {
			nvgpu_log_info(g, "fault buffer is already enabled");
		} else {
			reg_val |= fb_mmu_fault_buffer_size_enable_true_f();
			g->ops.fb.write_mmu_fault_buffer_size(g, index,
				reg_val);
		}

	} else {
		struct nvgpu_timeout timeout;
		u32 delay = POLL_DELAY_MIN_US;

		err = nvgpu_timeout_init(g, &timeout, nvgpu_get_poll_timeout(g),
			   NVGPU_TIMER_CPU_TIMER);
		if (err != 0) {
			nvgpu_err(g, "nvgpu_timeout_init failed err=%d", err);
		}

		reg_val &= (~(fb_mmu_fault_buffer_size_enable_m()));
		g->ops.fb.write_mmu_fault_buffer_size(g, index, reg_val);

		fault_status = g->ops.fb.read_mmu_fault_status(g);

		do {
			if ((fault_status &
			     fb_mmu_fault_status_busy_true_f()) == 0U) {
				break;
			}
			/*
			 * Make sure fault buffer is disabled.
			 * This is to avoid accessing fault buffer by hw
			 * during the window BAR2 is being unmapped by s/w
			 */
			nvgpu_log_info(g, "fault status busy set, check again");
			fault_status = g->ops.fb.read_mmu_fault_status(g);

			nvgpu_usleep_range(delay, delay * 2U);
			delay = min_t(u32, delay << 1, POLL_DELAY_MAX_US);
		} while (nvgpu_timeout_expired_msg(&timeout,
				"fault status busy set") == 0);
	}
}

void gv11b_fb_fault_buf_configure_hw(struct gk20a *g, u32 index)
{
	u32 addr_lo;
	u32 addr_hi;

	nvgpu_log_fn(g, " ");

	gv11b_fb_fault_buf_set_state_hw(g, index,
					 NVGPU_FB_MMU_FAULT_BUF_DISABLED);
	addr_lo = u64_lo32(g->mm.hw_fault_buf[index].gpu_va >>
					fb_mmu_fault_buffer_lo_addr_b());
	addr_hi = u64_hi32(g->mm.hw_fault_buf[index].gpu_va);

	g->ops.fb.write_mmu_fault_buffer_lo_hi(g, index,
		fb_mmu_fault_buffer_lo_addr_f(addr_lo),
		fb_mmu_fault_buffer_hi_addr_f(addr_hi));

	g->ops.fb.write_mmu_fault_buffer_size(g, index,
		fb_mmu_fault_buffer_size_val_f(g->ops.channel.count(g)) |
		fb_mmu_fault_buffer_size_overflow_intr_enable_f());

	gv11b_fb_fault_buf_set_state_hw(g, index, NVGPU_FB_MMU_FAULT_BUF_ENABLED);
}

static void gv11b_fb_parse_mmfault(struct mmu_fault_info *mmfault)
{
	if (mmfault->fault_type >= ARRAY_SIZE(fault_type_descs_gv11b)) {
		nvgpu_do_assert();
		mmfault->fault_type_desc =  invalid_str;
	} else {
		mmfault->fault_type_desc =
			 fault_type_descs_gv11b[mmfault->fault_type];
	}

	if (mmfault->client_type >= ARRAY_SIZE(fault_client_type_descs_gv11b)) {
		nvgpu_do_assert();
		mmfault->client_type_desc = invalid_str;
	} else {
		mmfault->client_type_desc =
			 fault_client_type_descs_gv11b[mmfault->client_type];
	}

	mmfault->client_id_desc = invalid_str;
	if (mmfault->client_type == gmmu_fault_client_type_hub_v()) {
		if (!(mmfault->client_id >=
				 ARRAY_SIZE(hub_client_descs_gv11b))) {
			mmfault->client_id_desc =
				 hub_client_descs_gv11b[mmfault->client_id];
		} else {
			nvgpu_do_assert();
		}
	} else if (mmfault->client_type ==
			gmmu_fault_client_type_gpc_v()) {
		if (!(mmfault->client_id >=
				 ARRAY_SIZE(gpc_client_descs_gv11b))) {
			mmfault->client_id_desc =
				 gpc_client_descs_gv11b[mmfault->client_id];
		} else {
			nvgpu_do_assert();
		}
	}

}

static void gv11b_fb_print_fault_info(struct gk20a *g,
			 struct mmu_fault_info *mmfault)
{
	if (mmfault != NULL && mmfault->valid) {
		nvgpu_err(g, "[MMU FAULT] "
			"mmu engine id:  %d, "
			"ch id:  %d, "
			"fault addr: 0x%llx, "
			"fault addr aperture: %d, "
			"fault type: %s, "
			"access type: %s, ",
			mmfault->mmu_engine_id,
			mmfault->chid,
			mmfault->fault_addr,
			mmfault->fault_addr_aperture,
			mmfault->fault_type_desc,
			fault_access_type_descs_gv11b[mmfault->access_type]);
		nvgpu_err(g, "[MMU FAULT] "
			"protected mode: %d, "
			"client type: %s, "
			"client id:  %s, "
			"gpc id if client type is gpc: %d, ",
			mmfault->protected_mode,
			mmfault->client_type_desc,
			mmfault->client_id_desc,
			mmfault->gpc_id);

		nvgpu_log(g, gpu_dbg_intr, "[MMU FAULT] "
			"faulted act eng id if any: 0x%x, "
			"faulted veid if any: 0x%x, "
			"faulted pbdma id if any: 0x%x, ",
			mmfault->faulted_engine,
			mmfault->faulted_subid,
			mmfault->faulted_pbdma);
		nvgpu_log(g, gpu_dbg_intr, "[MMU FAULT] "
			"inst ptr: 0x%llx, "
			"inst ptr aperture: %d, "
			"replayable fault: %d, "
			"replayable fault en:  %d "
			"timestamp hi:lo 0x%08x:0x%08x, ",
			mmfault->inst_ptr,
			mmfault->inst_aperture,
			mmfault->replayable_fault,
			mmfault->replay_fault_en,
			mmfault->timestamp_hi, mmfault->timestamp_lo);
	}
}

/*
 *Fault buffer format
 *
 * 31    28     24 23           16 15            8 7     4       0
 *.-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-.
 *|              inst_lo                  |0 0|apr|0 0 0 0 0 0 0 0|
 *`-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-'
 *|                             inst_hi                           |
 *`-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-'
 *|              addr_31_12               |                   |AP |
 *`-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-'
 *|                            addr_63_32                         |
 *`-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-'
 *|                          timestamp_lo                         |
 *`-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-'
 *|                          timestamp_hi                         |
 *`-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-'
 *|                           (reserved)        |    engine_id    |
 *`-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-'
 *|V|R|P|  gpc_id |0 0 0|t|0|acctp|0|   client    |RF0 0|faulttype|
 */

static void gv11b_fb_copy_from_hw_fault_buf(struct gk20a *g,
	 struct nvgpu_mem *mem, u32 offset, struct mmu_fault_info *mmfault)
{
	u32 rd32_val;
	u32 addr_lo, addr_hi;
	u64 inst_ptr;
	u32 chid = FIFO_INVAL_CHANNEL_ID;
	struct channel_gk20a *refch;

	(void) memset(mmfault, 0, sizeof(*mmfault));

	rd32_val = nvgpu_mem_rd32(g, mem, offset +
			 gmmu_fault_buf_entry_inst_lo_w());
	addr_lo = gmmu_fault_buf_entry_inst_lo_v(rd32_val);
	addr_lo = addr_lo << gmmu_fault_buf_entry_inst_lo_b();

	addr_hi = nvgpu_mem_rd32(g, mem, offset +
				 gmmu_fault_buf_entry_inst_hi_w());
	addr_hi = gmmu_fault_buf_entry_inst_hi_v(addr_hi);

	inst_ptr = hi32_lo32_to_u64(addr_hi, addr_lo);

	/* refch will be put back after fault is handled */
	refch = nvgpu_channel_refch_from_inst_ptr(g, inst_ptr);
	if (refch != NULL) {
		chid = refch->chid;
	}

	/* it is ok to continue even if refch is NULL */
	mmfault->refch = refch;
	mmfault->chid = chid;
	mmfault->inst_ptr = inst_ptr;
	mmfault->inst_aperture = gmmu_fault_buf_entry_inst_aperture_v(rd32_val);

	rd32_val = nvgpu_mem_rd32(g, mem, offset +
			 gmmu_fault_buf_entry_addr_lo_w());

	mmfault->fault_addr_aperture =
		gmmu_fault_buf_entry_addr_phys_aperture_v(rd32_val);
	addr_lo = gmmu_fault_buf_entry_addr_lo_v(rd32_val);
	addr_lo = addr_lo << gmmu_fault_buf_entry_addr_lo_b();

	rd32_val = nvgpu_mem_rd32(g, mem, offset +
			 gmmu_fault_buf_entry_addr_hi_w());
	addr_hi = gmmu_fault_buf_entry_addr_hi_v(rd32_val);
	mmfault->fault_addr = hi32_lo32_to_u64(addr_hi, addr_lo);

	rd32_val = nvgpu_mem_rd32(g, mem, offset +
			 gmmu_fault_buf_entry_timestamp_lo_w());
	mmfault->timestamp_lo =
		 gmmu_fault_buf_entry_timestamp_lo_v(rd32_val);

	rd32_val = nvgpu_mem_rd32(g, mem, offset +
			 gmmu_fault_buf_entry_timestamp_hi_w());
	mmfault->timestamp_hi =
		 gmmu_fault_buf_entry_timestamp_hi_v(rd32_val);

	rd32_val = nvgpu_mem_rd32(g, mem, offset +
			 gmmu_fault_buf_entry_engine_id_w());

	mmfault->mmu_engine_id =
		 gmmu_fault_buf_entry_engine_id_v(rd32_val);
	nvgpu_engine_mmu_fault_id_to_eng_ve_pbdma_id(g, mmfault->mmu_engine_id,
		 &mmfault->faulted_engine, &mmfault->faulted_subid,
		 &mmfault->faulted_pbdma);

	rd32_val = nvgpu_mem_rd32(g, mem, offset +
			gmmu_fault_buf_entry_fault_type_w());
	mmfault->client_id =
		 gmmu_fault_buf_entry_client_v(rd32_val);
	mmfault->replayable_fault =
		(gmmu_fault_buf_entry_replayable_fault_v(rd32_val) ==
			gmmu_fault_buf_entry_replayable_fault_true_v());

	mmfault->fault_type =
		 gmmu_fault_buf_entry_fault_type_v(rd32_val);
	mmfault->access_type =
		 gmmu_fault_buf_entry_access_type_v(rd32_val);

	mmfault->client_type =
		gmmu_fault_buf_entry_mmu_client_type_v(rd32_val);

	mmfault->gpc_id =
		 gmmu_fault_buf_entry_gpc_id_v(rd32_val);
	mmfault->protected_mode =
		gmmu_fault_buf_entry_protected_mode_v(rd32_val);

	mmfault->replay_fault_en =
		gmmu_fault_buf_entry_replayable_fault_en_v(rd32_val);

	mmfault->valid = (gmmu_fault_buf_entry_valid_v(rd32_val) ==
				gmmu_fault_buf_entry_valid_true_v());

	rd32_val = nvgpu_mem_rd32(g, mem, offset +
			gmmu_fault_buf_entry_fault_type_w());
	rd32_val &= ~(gmmu_fault_buf_entry_valid_m());
	nvgpu_mem_wr32(g, mem, offset + gmmu_fault_buf_entry_valid_w(),
					 rd32_val);

	gv11b_fb_parse_mmfault(mmfault);
}

static void gv11b_fb_handle_mmu_fault_common(struct gk20a *g,
		 struct mmu_fault_info *mmfault, u32 *invalidate_replay_val)
{
	unsigned int id_type = ID_TYPE_UNKNOWN;
	u32 num_lce, act_eng_bitmask = 0;
	int err = 0;
	u32 id = FIFO_INVAL_TSG_ID;
	unsigned int rc_type = RC_TYPE_NO_RC;
	struct tsg_gk20a *tsg = NULL;

	if (!mmfault->valid) {
		return;
	}

	gv11b_fb_print_fault_info(g, mmfault);

	num_lce = g->ops.top.get_num_lce(g);
	if ((mmfault->mmu_engine_id >=
			gmmu_fault_mmu_eng_id_ce0_v()) &&
			(mmfault->mmu_engine_id <
			gmmu_fault_mmu_eng_id_ce0_v() + num_lce)) {
		/* CE page faults are not reported as replayable */
		nvgpu_log(g, gpu_dbg_intr, "CE Faulted");
		err = gv11b_fb_fix_page_fault(g, mmfault);
		if ((mmfault->refch != NULL) &&
		    ((u32)mmfault->refch->tsgid != FIFO_INVAL_TSG_ID)) {
			tsg = nvgpu_tsg_get_from_id(g, mmfault->refch->tsgid);
			nvgpu_tsg_reset_faulted_eng_pbdma(g, tsg, true, true);
		}
		if (err == 0) {
			nvgpu_log(g, gpu_dbg_intr, "CE Page Fault Fixed");
			*invalidate_replay_val = 0;
			if (mmfault->refch != NULL) {
				gk20a_channel_put(mmfault->refch);
				mmfault->refch = NULL;
			}
			return;
		}
		/* Do recovery */
		nvgpu_log(g, gpu_dbg_intr, "CE Page Fault Not Fixed");
	}

	if (!mmfault->replayable_fault) {
		if (mmfault->fault_type ==
				gmmu_fault_type_unbound_inst_block_v()) {
		/*
		 * Bug 1847172: When an engine faults due to an unbound
		 * instance block, the fault cannot be isolated to a
		 * single context so we need to reset the entire runlist
		 */
			rc_type = RC_TYPE_MMU_FAULT;

		} else if (mmfault->refch != NULL) {
			if (mmfault->refch->mmu_nack_handled) {
				/* We have already recovered for the same
				 * context, skip doing another recovery.
				 */
				mmfault->refch->mmu_nack_handled = false;
				/*
				 * Recovery path can be entered twice for the
				 * same error in case of mmu nack. If mmu
				 * nack interrupt is handled before mmu fault
				 * then channel reference is increased to avoid
				 * closing the channel by userspace. Decrement
				 * channel reference.
				 */
				gk20a_channel_put(mmfault->refch);
				/* refch in mmfault is assigned at the time
				 * of copying fault info from snap reg or bar2
				 * fault buf.
				 */
				gk20a_channel_put(mmfault->refch);
				return;
			} else {
				/* Indicate recovery is handled if mmu fault is
				 * a result of mmu nack.
				 */
				mmfault->refch->mmu_nack_handled = true;
			}

			tsg = tsg_gk20a_from_ch(mmfault->refch);
			if (tsg != NULL) {
				id = mmfault->refch->tsgid;
				id_type = ID_TYPE_TSG;
				rc_type = RC_TYPE_MMU_FAULT;
			} else {
				nvgpu_err(g, "chid: %d is referenceable but "
						"not bound to tsg",
						mmfault->refch->chid);
				id_type = ID_TYPE_CHANNEL;
				rc_type = RC_TYPE_NO_RC;
			}
		}

		/* engine is faulted */
		if (mmfault->faulted_engine != FIFO_INVAL_ENGINE_ID) {
			act_eng_bitmask = BIT32(mmfault->faulted_engine);
			rc_type = RC_TYPE_MMU_FAULT;
		}

		/* refch in mmfault is assigned at the time of copying
		 * fault info from snap reg or bar2 fault buf
		 */
		if (mmfault->refch != NULL) {
			gk20a_channel_put(mmfault->refch);
			mmfault->refch = NULL;
		}

		if (rc_type != RC_TYPE_NO_RC) {
			g->ops.fifo.recover(g, act_eng_bitmask,
				id, id_type, rc_type, mmfault);
		}
	} else {
		if (mmfault->fault_type == gmmu_fault_type_pte_v()) {
			nvgpu_log(g, gpu_dbg_intr, "invalid pte! try to fix");
			err = gv11b_fb_fix_page_fault(g, mmfault);
			if (err != 0) {
				*invalidate_replay_val |=
					fb_mmu_invalidate_replay_cancel_global_f();
			} else {
				*invalidate_replay_val |=
					fb_mmu_invalidate_replay_start_ack_all_f();
			}
		} else {
			/* cancel faults other than invalid pte */
			*invalidate_replay_val |=
				fb_mmu_invalidate_replay_cancel_global_f();
		}
		/* refch in mmfault is assigned at the time of copying
		 * fault info from snap reg or bar2 fault buf
		 */
		if (mmfault->refch != NULL) {
			gk20a_channel_put(mmfault->refch);
			mmfault->refch = NULL;
		}
	}
}

static int gv11b_fb_replay_or_cancel_faults(struct gk20a *g,
			 u32 invalidate_replay_val)
{
	int err = 0;

	nvgpu_log_fn(g, " ");

	if ((invalidate_replay_val &
	     fb_mmu_invalidate_replay_cancel_global_f()) != 0U) {
		/*
		 * cancel faults so that next time it faults as
		 * replayable faults and channel recovery can be done
		 */
		err = g->ops.fb.mmu_invalidate_replay(g,
			fb_mmu_invalidate_replay_cancel_global_f());
	} else if ((invalidate_replay_val &
		    fb_mmu_invalidate_replay_start_ack_all_f()) != 0U) {
		/* pte valid is fixed. replay faulting request */
		err = g->ops.fb.mmu_invalidate_replay(g,
			fb_mmu_invalidate_replay_start_ack_all_f());
	}

	return err;
}

void gv11b_fb_handle_mmu_nonreplay_replay_fault(struct gk20a *g,
		 u32 fault_status, u32 index)
{
	u32 get_indx, offset, rd32_val, entries;
	struct nvgpu_mem *mem;
	struct mmu_fault_info *mmfault;
	u32 invalidate_replay_val = 0;
	u64 prev_fault_addr =  0ULL;
	u64 next_fault_addr =  0ULL;
	int err = 0;

	if (gv11b_fb_is_fault_buffer_empty(g, index, &get_indx)) {
		nvgpu_log(g, gpu_dbg_intr,
			"SPURIOUS mmu fault: reg index:%d", index);
		return;
	}
	nvgpu_log(g, gpu_dbg_intr, "%s MMU FAULT" ,
			index == NVGPU_FB_MMU_FAULT_REPLAY_REG_INDEX ?
					"REPLAY" : "NON-REPLAY");

	nvgpu_log(g, gpu_dbg_intr, "get ptr = %d", get_indx);

	mem = &g->mm.hw_fault_buf[index];
	mmfault = &g->mm.fault_info[index];

	entries = gv11b_fb_fault_buffer_size_val(g, index);
	nvgpu_log(g, gpu_dbg_intr, "buffer num entries = %d", entries);

	offset = (get_indx * gmmu_fault_buf_size_v()) / U32(sizeof(u32));
	nvgpu_log(g, gpu_dbg_intr, "starting word offset = 0x%x", offset);

	rd32_val = nvgpu_mem_rd32(g, mem,
		 offset + gmmu_fault_buf_entry_valid_w());
	nvgpu_log(g, gpu_dbg_intr, "entry valid offset val = 0x%x", rd32_val);

	while ((rd32_val & gmmu_fault_buf_entry_valid_m()) != 0U) {

		nvgpu_log(g, gpu_dbg_intr, "entry valid = 0x%x", rd32_val);

		gv11b_fb_copy_from_hw_fault_buf(g, mem, offset, mmfault);

		get_indx = (get_indx + 1U) % entries;
		nvgpu_log(g, gpu_dbg_intr, "new get index = %d", get_indx);

		gv11b_fb_fault_buffer_get_ptr_update(g, index, get_indx);

		offset = (get_indx * gmmu_fault_buf_size_v()) /
			 U32(sizeof(u32));
		nvgpu_log(g, gpu_dbg_intr, "next word offset = 0x%x", offset);

		rd32_val = nvgpu_mem_rd32(g, mem,
			 offset + gmmu_fault_buf_entry_valid_w());

		if (index == NVGPU_FB_MMU_FAULT_REPLAY_REG_INDEX &&
		    mmfault->fault_addr != 0ULL) {
			/* fault_addr "0" is not supposed to be fixed ever.
			 * For the first time when prev = 0, next = 0 and
			 * fault addr is also 0 then handle_mmu_fault_common will
			 * not be called. Fix by checking fault_addr not equal to 0
			 */
			prev_fault_addr = next_fault_addr;
			next_fault_addr = mmfault->fault_addr;
			if (prev_fault_addr == next_fault_addr) {
				nvgpu_log(g, gpu_dbg_intr, "pte already scanned");
				if (mmfault->refch != NULL) {
					gk20a_channel_put(mmfault->refch);
					mmfault->refch = NULL;
				}
				continue;
			}
		}

		gv11b_fb_handle_mmu_fault_common(g, mmfault,
				 &invalidate_replay_val);

	}
	if (index == NVGPU_FB_MMU_FAULT_REPLAY_REG_INDEX &&
	    invalidate_replay_val != 0U) {
		err = gv11b_fb_replay_or_cancel_faults(g,
			invalidate_replay_val);
		if (err != 0) {
			nvgpu_err(g, "replay_or_cancel_faults failed err=%d",
				err);
		}
	}
}

static void gv11b_mm_copy_from_fault_snap_reg(struct gk20a *g,
		u32 fault_status, struct mmu_fault_info *mmfault)
{
	u32 reg_val;
	u32 addr_lo, addr_hi;
	u64 inst_ptr;
	u32 chid = FIFO_INVAL_CHANNEL_ID;
	struct channel_gk20a *refch;

	(void) memset(mmfault, 0, sizeof(*mmfault));

	if ((fault_status & fb_mmu_fault_status_valid_set_f()) == 0U) {

		nvgpu_log(g, gpu_dbg_intr, "mmu fault status valid not set");
		return;
	}

	g->ops.fb.read_mmu_fault_inst_lo_hi(g, &reg_val, &addr_hi);

	addr_lo = fb_mmu_fault_inst_lo_addr_v(reg_val);
	addr_lo = addr_lo << fb_mmu_fault_inst_lo_addr_b();

	addr_hi = fb_mmu_fault_inst_hi_addr_v(addr_hi);
	inst_ptr = hi32_lo32_to_u64(addr_hi, addr_lo);

	/* refch will be put back after fault is handled */
	refch = nvgpu_channel_refch_from_inst_ptr(g, inst_ptr);
	if (refch != NULL) {
		chid = refch->chid;
	}

	/* It is still ok to continue if refch is NULL */
	mmfault->refch = refch;
	mmfault->chid = chid;
	mmfault->inst_ptr = inst_ptr;
	mmfault->inst_aperture = fb_mmu_fault_inst_lo_aperture_v(reg_val);
	mmfault->mmu_engine_id = fb_mmu_fault_inst_lo_engine_id_v(reg_val);

	nvgpu_engine_mmu_fault_id_to_eng_ve_pbdma_id(g, mmfault->mmu_engine_id,
		 &mmfault->faulted_engine, &mmfault->faulted_subid,
		 &mmfault->faulted_pbdma);

	g->ops.fb.read_mmu_fault_addr_lo_hi(g, &reg_val, &addr_hi);

	addr_lo = fb_mmu_fault_addr_lo_addr_v(reg_val);
	addr_lo = addr_lo << fb_mmu_fault_addr_lo_addr_b();

	mmfault->fault_addr_aperture =
			 fb_mmu_fault_addr_lo_phys_aperture_v(reg_val);

	addr_hi = fb_mmu_fault_addr_hi_addr_v(addr_hi);
	mmfault->fault_addr = hi32_lo32_to_u64(addr_hi, addr_lo);

	reg_val = g->ops.fb.read_mmu_fault_info(g);
	mmfault->fault_type = fb_mmu_fault_info_fault_type_v(reg_val);
	mmfault->replayable_fault =
			(fb_mmu_fault_info_replayable_fault_v(reg_val) == 1U);
	mmfault->client_id = fb_mmu_fault_info_client_v(reg_val);
	mmfault->access_type = fb_mmu_fault_info_access_type_v(reg_val);
	mmfault->client_type = fb_mmu_fault_info_client_type_v(reg_val);
	mmfault->gpc_id = fb_mmu_fault_info_gpc_id_v(reg_val);
	mmfault->protected_mode =
			 fb_mmu_fault_info_protected_mode_v(reg_val);
	mmfault->replay_fault_en =
			fb_mmu_fault_info_replayable_fault_en_v(reg_val);

	mmfault->valid = (fb_mmu_fault_info_valid_v(reg_val) == 1U);

	fault_status &= ~(fb_mmu_fault_status_valid_m());
	g->ops.fb.write_mmu_fault_status(g, fault_status);

	gv11b_fb_parse_mmfault(mmfault);

}

void gv11b_fb_handle_replay_fault_overflow(struct gk20a *g,
			 u32 fault_status)
{
	u32 reg_val;
	u32 index = NVGPU_FB_MMU_FAULT_REPLAY_REG_INDEX;

	reg_val = g->ops.fb.read_mmu_fault_buffer_get(g, index);

	if ((fault_status &
	     fb_mmu_fault_status_replayable_getptr_corrupted_m()) != 0U) {

		nvgpu_err(g, "replayable getptr corrupted set");

		gv11b_fb_fault_buf_configure_hw(g, index);

		reg_val = set_field(reg_val,
			fb_mmu_fault_buffer_get_getptr_corrupted_m(),
			fb_mmu_fault_buffer_get_getptr_corrupted_clear_f());
	}

	if ((fault_status &
	     fb_mmu_fault_status_replayable_overflow_m()) != 0U) {
		bool buffer_full = gv11b_fb_is_fault_buffer_full(g, index);

		nvgpu_err(g, "replayable overflow: buffer full:%s",
				buffer_full?"true":"false");

		reg_val = set_field(reg_val,
			fb_mmu_fault_buffer_get_overflow_m(),
			fb_mmu_fault_buffer_get_overflow_clear_f());
	}

	g->ops.fb.write_mmu_fault_buffer_get(g, index, reg_val);
}

void gv11b_fb_handle_nonreplay_fault_overflow(struct gk20a *g,
			 u32 fault_status)
{
	u32 reg_val;
	u32 index = NVGPU_FB_MMU_FAULT_NONREPLAY_REG_INDEX;

	reg_val = g->ops.fb.read_mmu_fault_buffer_get(g, index);

	if ((fault_status &
	     fb_mmu_fault_status_non_replayable_getptr_corrupted_m()) != 0U) {

		nvgpu_err(g, "non replayable getptr corrupted set");

		gv11b_fb_fault_buf_configure_hw(g, index);

		reg_val = set_field(reg_val,
			fb_mmu_fault_buffer_get_getptr_corrupted_m(),
			fb_mmu_fault_buffer_get_getptr_corrupted_clear_f());
	}

	if ((fault_status &
	     fb_mmu_fault_status_non_replayable_overflow_m()) != 0U) {

		bool buffer_full = gv11b_fb_is_fault_buffer_full(g, index);

		nvgpu_err(g, "non replayable overflow: buffer full:%s",
				buffer_full?"true":"false");

		reg_val = set_field(reg_val,
			fb_mmu_fault_buffer_get_overflow_m(),
			fb_mmu_fault_buffer_get_overflow_clear_f());
	}

	g->ops.fb.write_mmu_fault_buffer_get(g, index, reg_val);
}

static void gv11b_fb_handle_bar2_fault(struct gk20a *g,
			struct mmu_fault_info *mmfault, u32 fault_status)
{
	int err = 0;

	if ((fault_status &
	     fb_mmu_fault_status_non_replayable_error_m()) != 0U) {
		if (gv11b_fb_is_fault_buf_enabled(g,
				NVGPU_FB_MMU_FAULT_NONREPLAY_REG_INDEX)) {
			gv11b_fb_fault_buf_configure_hw(g, NVGPU_FB_MMU_FAULT_NONREPLAY_REG_INDEX);
		}
	}

	if ((fault_status & fb_mmu_fault_status_replayable_error_m()) != 0U) {
		if (gv11b_fb_is_fault_buf_enabled(g,
				NVGPU_FB_MMU_FAULT_REPLAY_REG_INDEX)) {
			gv11b_fb_fault_buf_configure_hw(g,
				NVGPU_FB_MMU_FAULT_REPLAY_REG_INDEX);
		}
	}
	g->ops.ce.mthd_buffer_fault_in_bar2_fault(g);

	err = g->ops.bus.bar2_bind(g, &g->mm.bar2.inst_block);
	if (err != 0) {
		nvgpu_err(g, "bar2_bind failed err=%d", err);
	}

	if (mmfault->refch != NULL) {
		gk20a_channel_put(mmfault->refch);
		mmfault->refch = NULL;
	}
}

void gv11b_fb_handle_other_fault_notify(struct gk20a *g,
			 u32 fault_status)
{
	struct mmu_fault_info *mmfault;
	u32 invalidate_replay_val = 0;
	int err = 0;

	mmfault = &g->mm.fault_info[NVGPU_MM_MMU_FAULT_TYPE_OTHER_AND_NONREPLAY];

	gv11b_mm_copy_from_fault_snap_reg(g, fault_status, mmfault);

	/* BAR2/Physical faults will not be snapped in hw fault buf */
	if (mmfault->mmu_engine_id == gmmu_fault_mmu_eng_id_bar2_v()) {
		nvgpu_err(g, "BAR2 MMU FAULT");
		gv11b_fb_handle_bar2_fault(g, mmfault, fault_status);

	} else if (mmfault->mmu_engine_id ==
			gmmu_fault_mmu_eng_id_physical_v()) {
		/* usually means VPR or out of bounds physical accesses */
		nvgpu_err(g, "PHYSICAL MMU FAULT");

	} else {
		gv11b_fb_handle_mmu_fault_common(g, mmfault,
				 &invalidate_replay_val);

		if (invalidate_replay_val != 0U) {
			err = gv11b_fb_replay_or_cancel_faults(g,
					invalidate_replay_val);
			if (err != 0) {
				nvgpu_err(g, "replay_or_cancel_faults err=%d",
					err);
			}
		}
	}
}

void gv11b_fb_handle_dropped_mmu_fault(struct gk20a *g, u32 fault_status)
{
	u32 dropped_faults = 0;

	dropped_faults = fb_mmu_fault_status_dropped_bar1_phys_set_f() |
			fb_mmu_fault_status_dropped_bar1_virt_set_f() |
			fb_mmu_fault_status_dropped_bar2_phys_set_f() |
			fb_mmu_fault_status_dropped_bar2_virt_set_f() |
			fb_mmu_fault_status_dropped_ifb_phys_set_f() |
			fb_mmu_fault_status_dropped_ifb_virt_set_f() |
			fb_mmu_fault_status_dropped_other_phys_set_f()|
			fb_mmu_fault_status_dropped_other_virt_set_f();

	if ((fault_status & dropped_faults) != 0U) {
		nvgpu_err(g, "dropped mmu fault (0x%08x)",
				 fault_status & dropped_faults);
		g->ops.fb.write_mmu_fault_status(g, dropped_faults);
	}
}

void gv11b_fb_handle_replayable_mmu_fault(struct gk20a *g)
{
	u32 fault_status = gk20a_readl(g, fb_mmu_fault_status_r());

	if ((fault_status & fb_mmu_fault_status_replayable_m()) == 0U) {
		return;
	}

	if (gv11b_fb_is_fault_buf_enabled(g,
			NVGPU_FB_MMU_FAULT_NONREPLAY_REG_INDEX)) {
		gv11b_fb_handle_mmu_nonreplay_replay_fault(g,
				fault_status,
				NVGPU_FB_MMU_FAULT_REPLAY_REG_INDEX);
	}
}

void gv11b_fb_handle_mmu_fault(struct gk20a *g, u32 niso_intr)
{
	u32 fault_status = g->ops.fb.read_mmu_fault_status(g);

	nvgpu_log(g, gpu_dbg_intr, "mmu_fault_status = 0x%08x", fault_status);

	if ((niso_intr &
	     fb_niso_intr_mmu_other_fault_notify_m()) != 0U) {

		gv11b_fb_handle_dropped_mmu_fault(g, fault_status);

		gv11b_fb_handle_other_fault_notify(g, fault_status);
	}

	if (gv11b_fb_is_fault_buf_enabled(g, NVGPU_FB_MMU_FAULT_NONREPLAY_REG_INDEX)) {

		if ((niso_intr &
		     fb_niso_intr_mmu_nonreplayable_fault_notify_m()) != 0U) {

			gv11b_fb_handle_mmu_nonreplay_replay_fault(g,
					fault_status,
					NVGPU_FB_MMU_FAULT_NONREPLAY_REG_INDEX);

			/*
			 * When all the faults are processed,
			 * GET and PUT will have same value and mmu fault status
			 * bit will be reset by HW
			 */
		}
		if ((niso_intr &
		     fb_niso_intr_mmu_nonreplayable_fault_overflow_m()) != 0U) {

			gv11b_fb_handle_nonreplay_fault_overflow(g,
				 fault_status);
		}

	}

	if (gv11b_fb_is_fault_buf_enabled(g, NVGPU_FB_MMU_FAULT_REPLAY_REG_INDEX)) {

		if ((niso_intr &
		     fb_niso_intr_mmu_replayable_fault_notify_m()) != 0U) {

			gv11b_fb_handle_mmu_nonreplay_replay_fault(g,
					fault_status,
					NVGPU_FB_MMU_FAULT_REPLAY_REG_INDEX);
		}
		if ((niso_intr &
		     fb_niso_intr_mmu_replayable_fault_overflow_m()) != 0U) {

			gv11b_fb_handle_replay_fault_overflow(g,
				 fault_status);
		}

	}

	nvgpu_log(g, gpu_dbg_intr, "clear mmu fault status");
	g->ops.fb.write_mmu_fault_status(g,
			fb_mmu_fault_status_valid_clear_f());
}

int gv11b_fb_mmu_invalidate_replay(struct gk20a *g,
			 u32 invalidate_replay_val)
{
	int err = 0;
	u32 reg_val;
	struct nvgpu_timeout timeout;

	nvgpu_log_fn(g, " ");

	nvgpu_mutex_acquire(&g->mm.tlb_lock);

	reg_val = gk20a_readl(g, fb_mmu_invalidate_r());

	reg_val |= fb_mmu_invalidate_all_va_true_f() |
		fb_mmu_invalidate_all_pdb_true_f() |
		invalidate_replay_val |
		fb_mmu_invalidate_trigger_true_f();

	gk20a_writel(g, fb_mmu_invalidate_r(), reg_val);

	/* retry 200 times */
	err = nvgpu_timeout_init(g, &timeout, 200, NVGPU_TIMER_RETRY_TIMER);
	if (err != 0) {
		nvgpu_err(g, "nvgpu_timeout_init failed err=%d", err);
		goto out;
	}

	err = -ETIMEDOUT;
	do {
		reg_val = gk20a_readl(g, fb_mmu_ctrl_r());
		if (fb_mmu_ctrl_pri_fifo_empty_v(reg_val) !=
			fb_mmu_ctrl_pri_fifo_empty_false_f()) {
			err = 0;
			break;
		}
		nvgpu_udelay(5);
	} while (nvgpu_timeout_expired_msg(&timeout,
			    "invalidate replay failed on 0x%llx") == 0);
	if (err != 0) {
		nvgpu_err(g, "invalidate replay timedout");
	}

out:
	nvgpu_mutex_release(&g->mm.tlb_lock);
	return err;
}

static int gv11b_fb_fix_page_fault(struct gk20a *g,
			 struct mmu_fault_info *mmfault)
{
	int err = 0;
	u32 pte[2];

	if (mmfault->refch == NULL) {
		nvgpu_log(g, gpu_dbg_intr, "refch from mmu_fault_info is NULL");
		return -EINVAL;
	}

	err = __nvgpu_get_pte(g,
			mmfault->refch->vm, mmfault->fault_addr, &pte[0]);
	if (err != 0) {
		nvgpu_log(g, gpu_dbg_intr | gpu_dbg_pte, "pte not found");
		return err;
	}
	nvgpu_log(g, gpu_dbg_intr | gpu_dbg_pte,
			"pte: %#08x %#08x", pte[1], pte[0]);

	if (pte[0] == 0x0U && pte[1] == 0x0U) {
		nvgpu_log(g, gpu_dbg_intr | gpu_dbg_pte,
				"pte all zeros, do not set valid");
		return -1;
	}
	if ((pte[0] & gmmu_new_pte_valid_true_f()) != 0U) {
		nvgpu_log(g, gpu_dbg_intr | gpu_dbg_pte,
				"pte valid already set");
		return -1;
	}

	pte[0] |= gmmu_new_pte_valid_true_f();
	if ((pte[0] & gmmu_new_pte_read_only_true_f()) != 0U) {
		pte[0] &= ~(gmmu_new_pte_read_only_true_f());
	}
	nvgpu_log(g, gpu_dbg_intr | gpu_dbg_pte,
			"new pte: %#08x %#08x", pte[1], pte[0]);

	err = __nvgpu_set_pte(g,
			mmfault->refch->vm, mmfault->fault_addr, &pte[0]);
	if (err != 0) {
		nvgpu_log(g, gpu_dbg_intr | gpu_dbg_pte, "pte not fixed");
		return err;
	}
	/* invalidate tlb so that GMMU does not use old cached translation */
	err = g->ops.fb.tlb_invalidate(g, mmfault->refch->vm->pdb.mem);
	if (err != 0) {
		nvgpu_err(g, "tlb_invalidate failed err=%d", err);
		return err;
	}

	err = __nvgpu_get_pte(g,
			mmfault->refch->vm, mmfault->fault_addr, &pte[0]);
	nvgpu_log(g, gpu_dbg_intr | gpu_dbg_pte,
			"pte after tlb invalidate: %#08x %#08x",
			pte[1], pte[0]);
	return err;
}

void fb_gv11b_write_mmu_fault_buffer_lo_hi(struct gk20a *g, u32 index,
	u32 addr_lo, u32 addr_hi)
{
	nvgpu_writel(g, fb_mmu_fault_buffer_lo_r(index), addr_lo);
	nvgpu_writel(g, fb_mmu_fault_buffer_hi_r(index), addr_hi);
}

u32 fb_gv11b_read_mmu_fault_buffer_get(struct gk20a *g, u32 index)
{
	return nvgpu_readl(g, fb_mmu_fault_buffer_get_r(index));
}

void fb_gv11b_write_mmu_fault_buffer_get(struct gk20a *g, u32 index,
	u32 reg_val)
{
	nvgpu_writel(g, fb_mmu_fault_buffer_get_r(index), reg_val);
}

u32 fb_gv11b_read_mmu_fault_buffer_put(struct gk20a *g, u32 index)
{
	return nvgpu_readl(g, fb_mmu_fault_buffer_put_r(index));
}

u32 fb_gv11b_read_mmu_fault_buffer_size(struct gk20a *g, u32 index)
{
	return nvgpu_readl(g, fb_mmu_fault_buffer_size_r(index));
}

void fb_gv11b_write_mmu_fault_buffer_size(struct gk20a *g, u32 index,
	u32 reg_val)
{
	nvgpu_writel(g, fb_mmu_fault_buffer_size_r(index), reg_val);
}

void fb_gv11b_read_mmu_fault_addr_lo_hi(struct gk20a *g,
	u32 *addr_lo, u32 *addr_hi)
{
	*addr_lo = nvgpu_readl(g, fb_mmu_fault_addr_lo_r());
	*addr_hi = nvgpu_readl(g, fb_mmu_fault_addr_hi_r());
}

void fb_gv11b_read_mmu_fault_inst_lo_hi(struct gk20a *g,
	u32 *inst_lo, u32 *inst_hi)
{
	*inst_lo = nvgpu_readl(g, fb_mmu_fault_inst_lo_r());
	*inst_hi = nvgpu_readl(g, fb_mmu_fault_inst_hi_r());
}

u32 fb_gv11b_read_mmu_fault_info(struct gk20a *g)
{
	return nvgpu_readl(g, fb_mmu_fault_info_r());
}

u32 fb_gv11b_read_mmu_fault_status(struct gk20a *g)
{
	return nvgpu_readl(g, fb_mmu_fault_status_r());
}

void fb_gv11b_write_mmu_fault_status(struct gk20a *g, u32 reg_val)
{
	nvgpu_writel(g, fb_mmu_fault_status_r(), reg_val);
}
