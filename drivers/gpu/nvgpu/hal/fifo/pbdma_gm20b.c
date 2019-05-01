/*
 * Copyright (c) 2014-2019, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/utils.h>
#include <nvgpu/io.h>
#include <nvgpu/bitops.h>
#include <nvgpu/bug.h>
#include <nvgpu/debug.h>
#include <nvgpu/error_notifier.h>
#include <nvgpu/nvhost.h>
#include <nvgpu/fifo.h>
#include <nvgpu/ptimer.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/pbdma_status.h>

#include <nvgpu/hw/gm20b/hw_pbdma_gm20b.h>

#include "pbdma_gm20b.h"

#define PBDMA_SUBDEVICE_ID  1U

static const char *const pbdma_intr_fault_type_desc[] = {
	"MEMREQ timeout", "MEMACK_TIMEOUT", "MEMACK_EXTRA acks",
	"MEMDAT_TIMEOUT", "MEMDAT_EXTRA acks", "MEMFLUSH noack",
	"MEMOP noack", "LBCONNECT noack", "NONE - was LBREQ",
	"LBACK_TIMEOUT", "LBACK_EXTRA acks", "LBDAT_TIMEOUT",
	"LBDAT_EXTRA acks", "GPFIFO won't fit", "GPPTR invalid",
	"GPENTRY invalid", "GPCRC mismatch", "PBPTR get>put",
	"PBENTRY invld", "PBCRC mismatch", "NONE - was XBARC",
	"METHOD invld", "METHODCRC mismat", "DEVICE sw method",
	"[ENGINE]", "SEMAPHORE invlid", "ACQUIRE timeout",
	"PRI forbidden", "ILLEGAL SYNCPT", "[NO_CTXSW_SEG]",
	"PBSEG badsplit", "SIGNATURE bad"
};

static bool gm20b_pbdma_is_sw_method_subch(struct gk20a *g, u32 pbdma_id,
						u32 pbdma_method_index)
{
	u32 pbdma_method_stride;
	u32 pbdma_method_reg, pbdma_method_subch;

	pbdma_method_stride = pbdma_method1_r(pbdma_id) -
				pbdma_method0_r(pbdma_id);

	pbdma_method_reg = pbdma_method0_r(pbdma_id) +
			(pbdma_method_index * pbdma_method_stride);

	pbdma_method_subch = pbdma_method0_subch_v(
			nvgpu_readl(g, pbdma_method_reg));

	if (pbdma_method_subch == 5U ||
	    pbdma_method_subch == 6U ||
	    pbdma_method_subch == 7U) {
		return true;
	}

	return false;
}

static void gm20b_pbdma_disable_all_intr(struct gk20a *g, u32 pbdma_id)
{
	nvgpu_writel(g, pbdma_intr_en_0_r(pbdma_id), 0U);
	nvgpu_writel(g, pbdma_intr_en_1_r(pbdma_id), 0U);
}

void gm20b_pbdma_clear_all_intr(struct gk20a *g, u32 pbdma_id)
{
	nvgpu_writel(g, pbdma_intr_0_r(pbdma_id), U32_MAX);
	nvgpu_writel(g, pbdma_intr_1_r(pbdma_id), U32_MAX);
}

void gm20b_pbdma_disable_and_clear_all_intr(struct gk20a *g)
{
	u32 pbdma_id = 0;
	u32 num_pbdma = nvgpu_get_litter_value(g, GPU_LIT_HOST_NUM_PBDMA);

	for (pbdma_id = 0; pbdma_id < num_pbdma; pbdma_id++) {
		gm20b_pbdma_disable_all_intr(g, pbdma_id);
		gm20b_pbdma_clear_all_intr(g, pbdma_id);
	}
}

void gm20b_pbdma_intr_enable(struct gk20a *g, bool enable)
{
	u32 pbdma_id = 0;
	u32 intr_stall;
	u32 num_pbdma = nvgpu_get_litter_value(g, GPU_LIT_HOST_NUM_PBDMA);

	if (!enable) {
		gm20b_pbdma_disable_and_clear_all_intr(g);
		return;
	}

	/* clear and enable pbdma interrupts */
	for (pbdma_id = 0; pbdma_id < num_pbdma; pbdma_id++) {

		gm20b_pbdma_clear_all_intr(g, pbdma_id);

		intr_stall = nvgpu_readl(g, pbdma_intr_stall_r(pbdma_id));
		intr_stall &= ~pbdma_intr_stall_lbreq_enabled_f();

		nvgpu_writel(g, pbdma_intr_stall_r(pbdma_id), intr_stall);
		nvgpu_log_info(g, "pbdma id:%u, intr_en_0 0x%08x", pbdma_id,
			intr_stall);
		nvgpu_writel(g, pbdma_intr_en_0_r(pbdma_id), intr_stall);
		intr_stall = nvgpu_readl(g, pbdma_intr_stall_1_r(pbdma_id));
		/*
		 * For bug 2082123
		 * Mask the unused HCE_RE_ILLEGAL_OP bit from the interrupt.
		 */
		intr_stall &= ~pbdma_intr_stall_1_hce_illegal_op_enabled_f();
		nvgpu_log_info(g, "pbdma id:%u, intr_en_1 0x%08x", pbdma_id,
			intr_stall);
		nvgpu_writel(g, pbdma_intr_en_1_r(pbdma_id), intr_stall);
	}
}

bool gm20b_pbdma_handle_intr_0(struct gk20a *g, u32 pbdma_id,
			u32 pbdma_intr_0, u32 *error_notifier)
{
	struct nvgpu_fifo *f = &g->fifo;
	bool recover = false;
	u32 i;
	unsigned long pbdma_intr_err;
	unsigned long bit;

	if (((f->intr.pbdma.device_fatal_0 |
	      f->intr.pbdma.channel_fatal_0 |
	      f->intr.pbdma.restartable_0) & pbdma_intr_0) != 0U) {

		pbdma_intr_err = (unsigned long)pbdma_intr_0;
		for_each_set_bit(bit, &pbdma_intr_err, 32U) {
			nvgpu_err(g, "PBDMA intr %s Error",
				pbdma_intr_fault_type_desc[bit]);
		}

		nvgpu_err(g,
			"pbdma_intr_0(%d):0x%08x PBH: %08x "
			"SHADOW: %08x gp shadow0: %08x gp shadow1: %08x"
			"M0: %08x %08x %08x %08x ",
			pbdma_id, pbdma_intr_0,
			nvgpu_readl(g, pbdma_pb_header_r(pbdma_id)),
			g->ops.pbdma.read_data(g, pbdma_id),
			nvgpu_readl(g, pbdma_gp_shadow_0_r(pbdma_id)),
			nvgpu_readl(g, pbdma_gp_shadow_1_r(pbdma_id)),
			nvgpu_readl(g, pbdma_method0_r(pbdma_id)),
			nvgpu_readl(g, pbdma_method1_r(pbdma_id)),
			nvgpu_readl(g, pbdma_method2_r(pbdma_id)),
			nvgpu_readl(g, pbdma_method3_r(pbdma_id))
			);

		recover = true;
	}

	if ((pbdma_intr_0 & pbdma_intr_0_acquire_pending_f()) != 0U) {
		u32 val = nvgpu_readl(g, pbdma_acquire_r(pbdma_id));

		val &= ~pbdma_acquire_timeout_en_enable_f();
		nvgpu_writel(g, pbdma_acquire_r(pbdma_id), val);
		if (nvgpu_is_timeouts_enabled(g)) {
			recover = true;
			nvgpu_err(g, "semaphore acquire timeout!");

			/*
			 * Note: the error_notifier can be overwritten if
			 * semaphore_timeout is triggered with pbcrc_pending
			 * interrupt below
			 */
			*error_notifier =
				NVGPU_ERR_NOTIFIER_GR_SEMAPHORE_TIMEOUT;
		}
	}

	if ((pbdma_intr_0 & pbdma_intr_0_pbentry_pending_f()) != 0U) {
		g->ops.pbdma.reset_header(g, pbdma_id);
		gm20b_pbdma_reset_method(g, pbdma_id, 0);
		recover = true;
	}

	if ((pbdma_intr_0 & pbdma_intr_0_method_pending_f()) != 0U) {
		gm20b_pbdma_reset_method(g, pbdma_id, 0);
		recover = true;
	}

	if ((pbdma_intr_0 & pbdma_intr_0_pbcrc_pending_f()) != 0U) {
		*error_notifier =
			NVGPU_ERR_NOTIFIER_PBDMA_PUSHBUFFER_CRC_MISMATCH;
		recover = true;
	}

	if ((pbdma_intr_0 & pbdma_intr_0_device_pending_f()) != 0U) {
		g->ops.pbdma.reset_header(g, pbdma_id);

		for (i = 0U; i < 4U; i++) {
			if (gm20b_pbdma_is_sw_method_subch(g,
					pbdma_id, i)) {
				gm20b_pbdma_reset_method(g,
						pbdma_id, i);
			}
		}
		recover = true;
	}

	return recover;
}

bool gm20b_pbdma_handle_intr_1(struct gk20a *g, u32 pbdma_id, u32 pbdma_intr_1,
			u32 *error_notifier)
{
	bool recover = true;
	/*
	 * all of the interrupts in _intr_1 are "host copy engine"
	 * related, which is not supported. For now just make them
	 * channel fatal.
	 */
	nvgpu_err(g, "hce err: pbdma_intr_1(%d):0x%08x",
		pbdma_id, pbdma_intr_1);

	return recover;
}

void gm20b_pbdma_reset_header(struct gk20a *g, u32 pbdma_id)
{
	nvgpu_writel(g, pbdma_pb_header_r(pbdma_id),
			pbdma_pb_header_first_true_f() |
			pbdma_pb_header_type_non_inc_f());
}

void gm20b_pbdma_reset_method(struct gk20a *g, u32 pbdma_id,
			u32 pbdma_method_index)
{
	u32 pbdma_method_stride;
	u32 pbdma_method_reg;

	pbdma_method_stride = pbdma_method1_r(pbdma_id) -
				pbdma_method0_r(pbdma_id);

	pbdma_method_reg = pbdma_method0_r(pbdma_id) +
		(pbdma_method_index * pbdma_method_stride);

	nvgpu_writel(g, pbdma_method_reg,
			pbdma_method0_valid_true_f() |
			pbdma_method0_first_true_f() |
			pbdma_method0_addr_f(
			     pbdma_udma_nop_r() >> 2));
}

u32 gm20b_pbdma_get_signature(struct gk20a *g)
{
	return pbdma_signature_hw_valid_f() | pbdma_signature_sw_zero_f();
}

u32 gm20b_pbdma_acquire_val(u64 timeout)
{
	u32 val, exponent, mantissa;
	unsigned int val_len;
	u64 tmp;

	val = pbdma_acquire_retry_man_2_f() |
		pbdma_acquire_retry_exp_2_f();

	if (timeout == 0ULL) {
		return val;
	}

	timeout *= 80UL;
	do_div(timeout, 100U); /* set acquire timeout to 80% of channel wdt */
	timeout *= 1000000UL; /* ms -> ns */
	do_div(timeout, 1024U); /* in unit of 1024ns */
	tmp = fls(timeout >> 32U);
	BUG_ON(tmp > U64(U32_MAX));
	val_len = (u32)tmp + 32U;
	if (val_len == 32U) {
		val_len = (u32)fls(timeout);
	}
	if (val_len > 16U + pbdma_acquire_timeout_exp_max_v()) { /* man: 16bits */
		exponent = pbdma_acquire_timeout_exp_max_v();
		mantissa = pbdma_acquire_timeout_man_max_v();
	} else if (val_len > 16U) {
		exponent = val_len - 16U;
		BUG_ON((timeout >> exponent) > U64(U32_MAX));
		mantissa = (u32)(timeout >> exponent);
	} else {
		exponent = 0;
		BUG_ON(timeout > U64(U32_MAX));
		mantissa = (u32)timeout;
	}

	val |= pbdma_acquire_timeout_exp_f(exponent) |
		pbdma_acquire_timeout_man_f(mantissa) |
		pbdma_acquire_timeout_en_enable_f();

	return val;
}

void gm20b_pbdma_dump_status(struct gk20a *g, struct gk20a_debug_output *o)
{
	u32 i, host_num_pbdma;
	struct nvgpu_pbdma_status_info pbdma_status;

	host_num_pbdma = nvgpu_get_litter_value(g, GPU_LIT_HOST_NUM_PBDMA);

	gk20a_debug_output(o, "PBDMA Status - chip %-5s", g->name);
	gk20a_debug_output(o, "-------------------------");

	for (i = 0; i < host_num_pbdma; i++) {
		g->ops.pbdma_status.read_pbdma_status_info(g, i,
			&pbdma_status);

		gk20a_debug_output(o, "pbdma %d:", i);
		gk20a_debug_output(o,
			"  id: %d - %-9s next_id: - %d %-9s | status: %s",
			pbdma_status.id,
			nvgpu_pbdma_status_is_id_type_tsg(&pbdma_status) ?
				   "[tsg]" : "[channel]",
			pbdma_status.next_id,
			nvgpu_pbdma_status_is_next_id_type_tsg(
				&pbdma_status) ?
				   "[tsg]" : "[channel]",
			nvgpu_fifo_decode_pbdma_ch_eng_status(
				pbdma_status.pbdma_channel_status));
		gk20a_debug_output(o,
			"  PBDMA_PUT %016llx PBDMA_GET %016llx",
			(u64)nvgpu_readl(g, pbdma_put_r(i)) +
			((u64)nvgpu_readl(g, pbdma_put_hi_r(i)) << 32ULL),
			(u64)nvgpu_readl(g, pbdma_get_r(i)) +
			((u64)nvgpu_readl(g, pbdma_get_hi_r(i)) << 32ULL));
		gk20a_debug_output(o,
			"  GP_PUT    %08x  GP_GET  %08x  "
			"FETCH   %08x HEADER %08x",
			nvgpu_readl(g, pbdma_gp_put_r(i)),
			nvgpu_readl(g, pbdma_gp_get_r(i)),
			nvgpu_readl(g, pbdma_gp_fetch_r(i)),
			nvgpu_readl(g, pbdma_pb_header_r(i)));
		gk20a_debug_output(o,
			"  HDR       %08x  SHADOW0 %08x  SHADOW1 %08x",
			nvgpu_readl(g, pbdma_hdr_shadow_r(i)),
			nvgpu_readl(g, pbdma_gp_shadow_0_r(i)),
			nvgpu_readl(g, pbdma_gp_shadow_1_r(i)));
	}

	gk20a_debug_output(o, " ");
}

u32 gm20b_pbdma_read_data(struct gk20a *g, u32 pbdma_id)
{
	return nvgpu_readl(g, pbdma_hdr_shadow_r(pbdma_id));
}

void gm20b_pbdma_format_gpfifo_entry(struct gk20a *g,
		struct nvgpu_gpfifo_entry *gpfifo_entry,
		u64 pb_gpu_va, u32 method_size)
{
	gpfifo_entry->entry0 = u64_lo32(pb_gpu_va);
	gpfifo_entry->entry1 = u64_hi32(pb_gpu_va) |
					pbdma_gp_entry1_length_f(method_size);
}

u32 gm20b_pbdma_device_fatal_0_intr_descs(void)
{
	/*
	 * These are all errors which indicate something really wrong
	 * going on in the device.
	 */
	u32 fatal_device_0_intr_descs =
		pbdma_intr_0_memreq_pending_f() |
		pbdma_intr_0_memack_timeout_pending_f() |
		pbdma_intr_0_memack_extra_pending_f() |
		pbdma_intr_0_memdat_timeout_pending_f() |
		pbdma_intr_0_memdat_extra_pending_f() |
		pbdma_intr_0_memflush_pending_f() |
		pbdma_intr_0_memop_pending_f() |
		pbdma_intr_0_lbconnect_pending_f() |
		pbdma_intr_0_lback_timeout_pending_f() |
		pbdma_intr_0_lback_extra_pending_f() |
		pbdma_intr_0_lbdat_timeout_pending_f() |
		pbdma_intr_0_lbdat_extra_pending_f() |
		pbdma_intr_0_pri_pending_f();

	return fatal_device_0_intr_descs;
}

u32 gm20b_pbdma_channel_fatal_0_intr_descs(void)
{
	/*
	 * These are data parsing, framing errors or others which can be
	 * recovered from with intervention... or just resetting the
	 * channel
	 */
	u32 channel_fatal_0_intr_descs =
		pbdma_intr_0_gpfifo_pending_f() |
		pbdma_intr_0_gpptr_pending_f() |
		pbdma_intr_0_gpentry_pending_f() |
		pbdma_intr_0_gpcrc_pending_f() |
		pbdma_intr_0_pbptr_pending_f() |
		pbdma_intr_0_pbentry_pending_f() |
		pbdma_intr_0_pbcrc_pending_f() |
		pbdma_intr_0_method_pending_f() |
		pbdma_intr_0_methodcrc_pending_f() |
		pbdma_intr_0_pbseg_pending_f() |
		pbdma_intr_0_signature_pending_f();

	return channel_fatal_0_intr_descs;
}

u32 gm20b_pbdma_restartable_0_intr_descs(void)
{
	/* Can be used for sw-methods, or represents a recoverable timeout. */
	u32 restartable_0_intr_descs =
		pbdma_intr_0_device_pending_f();

	return restartable_0_intr_descs;
}

bool gm20b_pbdma_handle_intr(struct gk20a *g, u32 pbdma_id,
			u32 *error_notifier)
{
	u32 intr_error_notifier = NVGPU_ERR_NOTIFIER_PBDMA_ERROR;

	u32 pbdma_intr_0 = nvgpu_readl(g, pbdma_intr_0_r(pbdma_id));
	u32 pbdma_intr_1 = nvgpu_readl(g, pbdma_intr_1_r(pbdma_id));

	bool recover = false;

	if (pbdma_intr_0 != 0U) {
		nvgpu_log(g, gpu_dbg_info | gpu_dbg_intr,
			"pbdma id %d intr_0 0x%08x pending",
			pbdma_id, pbdma_intr_0);

		if (g->ops.pbdma.handle_intr_0(g, pbdma_id, pbdma_intr_0,
			&intr_error_notifier)) {
			recover = true;
		}
		nvgpu_writel(g, pbdma_intr_0_r(pbdma_id), pbdma_intr_0);
	}

	if (pbdma_intr_1 != 0U) {
		nvgpu_log(g, gpu_dbg_info | gpu_dbg_intr,
			"pbdma id %d intr_1 0x%08x pending",
			pbdma_id, pbdma_intr_1);

		if (g->ops.pbdma.handle_intr_1(g, pbdma_id, pbdma_intr_1,
			&intr_error_notifier)) {
			recover = true;
		}
		nvgpu_writel(g, pbdma_intr_1_r(pbdma_id), pbdma_intr_1);
	}

	if (error_notifier != NULL) {
		*error_notifier = intr_error_notifier;
	}

	return recover;
}

void gm20b_pbdma_syncpoint_debug_dump(struct gk20a *g,
			     struct gk20a_debug_output *o,
			     struct nvgpu_channel_dump_info *info)
{
#ifdef CONFIG_TEGRA_GK20A_NVHOST

	u32 syncpointa, syncpointb;

	syncpointa = info->inst.syncpointa;
	syncpointb = info->inst.syncpointb;


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
}

void gm20b_pbdma_setup_hw(struct gk20a *g)
{
	u32 host_num_pbdma = nvgpu_get_litter_value(g, GPU_LIT_HOST_NUM_PBDMA);
	u32 i, timeout;

	for (i = 0U; i < host_num_pbdma; i++) {
		timeout = nvgpu_readl(g, pbdma_timeout_r(i));
		timeout = set_field(timeout, pbdma_timeout_period_m(),
				    pbdma_timeout_period_max_f());
		nvgpu_log_info(g, "pbdma_timeout reg val = 0x%08x", timeout);
		nvgpu_writel(g, pbdma_timeout_r(i), timeout);
	}
}

u32 gm20b_pbdma_get_gp_base(u64 gpfifo_base)
{
	return pbdma_gp_base_offset_f(
		u64_lo32(gpfifo_base >> pbdma_gp_base_rsvd_s()));
}

u32 gm20b_pbdma_get_gp_base_hi(u64 gpfifo_base, u32 gpfifo_entry)
{
	return 	(pbdma_gp_base_hi_offset_f(u64_hi32(gpfifo_base)) |
		pbdma_gp_base_hi_limit2_f((u32)ilog2(gpfifo_entry)));
}

u32 gm20b_pbdma_get_fc_formats(void)
{
	return	(pbdma_formats_gp_fermi0_f() | pbdma_formats_pb_fermi1_f() |
			pbdma_formats_mp_fermi0_f());
}

u32 gm20b_pbdma_get_fc_pb_header(void)
{
	return (pbdma_pb_header_priv_user_f() |
		pbdma_pb_header_method_zero_f() |
		pbdma_pb_header_subchannel_zero_f() |
		pbdma_pb_header_level_main_f() |
		pbdma_pb_header_first_true_f() |
		pbdma_pb_header_type_inc_f());
}

u32 gm20b_pbdma_get_fc_subdevice(void)
{
	return (pbdma_subdevice_id_f(PBDMA_SUBDEVICE_ID) |
		pbdma_subdevice_status_active_f() |
		pbdma_subdevice_channel_dma_enable_f());
}

u32 gm20b_pbdma_get_fc_target(void)
{
	return pbdma_target_engine_sw_f();
}

u32 gm20b_pbdma_get_ctrl_hce_priv_mode_yes(void)
{
	return pbdma_hce_ctrl_hce_priv_mode_yes_f();
}

u32 gm20b_pbdma_get_userd_aperture_mask(struct gk20a *g,
		struct nvgpu_mem *mem)
{
	return	(nvgpu_aperture_mask(g, mem,
			pbdma_userd_target_sys_mem_ncoh_f(),
			pbdma_userd_target_sys_mem_coh_f(),
			pbdma_userd_target_vid_mem_f()));
}

u32 gm20b_pbdma_get_userd_addr(u32 addr_lo)
{
	return pbdma_userd_addr_f(addr_lo);
}

u32 gm20b_pbdma_get_userd_hi_addr(u32 addr_hi)
{
	return pbdma_userd_hi_addr_f(addr_hi);
}
