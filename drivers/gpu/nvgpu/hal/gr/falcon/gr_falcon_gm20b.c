/*
 * Copyright (c) 2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/gk20a.h>
#include <nvgpu/gr/gr_falcon.h>
#include <nvgpu/io.h>
#include <nvgpu/debug.h>
#include <nvgpu/power_features/pg.h>
#include <nvgpu/soc.h>

#include "gr_falcon_gm20b.h"
#include "common/gr/gr_falcon_priv.h"
#include "common/gr/gr_priv.h"

#include <nvgpu/hw/gm20b/hw_gr_gm20b.h>

#define GR_FECS_POLL_INTERVAL	5U /* usec */

#define FECS_ARB_CMD_TIMEOUT_MAX_US 40U
#define FECS_ARB_CMD_TIMEOUT_DEFAULT_US 2U
#define CTXSW_MEM_SCRUBBING_TIMEOUT_MAX_US 1000U
#define CTXSW_MEM_SCRUBBING_TIMEOUT_DEFAULT_US 10U

#define CTXSW_WDT_DEFAULT_VALUE 0x7FFFFFFFU
#define CTXSW_INTR0 BIT32(0)
#define CTXSW_INTR1 BIT32(1)

void gm20b_gr_falcon_load_gpccs_dmem(struct gk20a *g,
			const u32 *ucode_u32_data, u32 ucode_u32_size)
{
	u32 i;
	u32 checksum = 0;

	/* enable access for gpccs dmem */
	nvgpu_writel(g, gr_gpccs_dmemc_r(0), (gr_gpccs_dmemc_offs_f(0) |
					gr_gpccs_dmemc_blk_f(0)  |
					gr_gpccs_dmemc_aincw_f(1)));

	for (i = 0; i < ucode_u32_size; i++) {
		nvgpu_writel(g, gr_gpccs_dmemd_r(0), ucode_u32_data[i]);
		checksum += ucode_u32_data[i];
	}
	nvgpu_log_info(g, "gpccs dmem checksum: 0x%x", checksum);
}

void gm20b_gr_falcon_load_fecs_dmem(struct gk20a *g,
			const u32 *ucode_u32_data, u32 ucode_u32_size)
{
	u32 i;
	u32 checksum = 0;

	/* set access for fecs dmem */
	nvgpu_writel(g, gr_fecs_dmemc_r(0), (gr_fecs_dmemc_offs_f(0) |
					gr_fecs_dmemc_blk_f(0)  |
					gr_fecs_dmemc_aincw_f(1)));

	for (i = 0; i < ucode_u32_size; i++) {
		nvgpu_writel(g, gr_fecs_dmemd_r(0), ucode_u32_data[i]);
		checksum += ucode_u32_data[i];
	}
	nvgpu_log_info(g, "fecs dmem checksum: 0x%x", checksum);
}

void gm20b_gr_falcon_load_gpccs_imem(struct gk20a *g,
			const u32 *ucode_u32_data, u32 ucode_u32_size)
{
	u32 cfg, gpccs_imem_size;
	u32 tag, i, pad_start, pad_end;
	u32 checksum = 0;

	/* enable access for gpccs imem */
	nvgpu_writel(g, gr_gpccs_imemc_r(0), (gr_gpccs_imemc_offs_f(0) |
					gr_gpccs_imemc_blk_f(0) |
					gr_gpccs_imemc_aincw_f(1)));

	cfg = nvgpu_readl(g, gr_gpc0_cfg_r());
	gpccs_imem_size = gr_gpc0_cfg_imem_sz_v(cfg);

	/* Setup the tags for the instruction memory. */
	tag = 0;
	nvgpu_writel(g, gr_gpccs_imemt_r(0), gr_gpccs_imemt_tag_f(tag));

	for (i = 0; i < ucode_u32_size; i++) {
		if ((i != 0U) && ((i % (256U/sizeof(u32))) == 0U)) {
			tag++;
			nvgpu_writel(g, gr_gpccs_imemt_r(0),
					gr_gpccs_imemt_tag_f(tag));
		}
		nvgpu_writel(g, gr_gpccs_imemd_r(0), ucode_u32_data[i]);
		checksum += ucode_u32_data[i];
	}

	pad_start = i * 4U;
	pad_end = pad_start + (256U - pad_start % 256U) + 256U;
	for (i = pad_start;
		(i < gpccs_imem_size * 256U) && (i < pad_end); i += 4U) {
		if ((i != 0U) && ((i % 256U) == 0U)) {
			tag++;
			nvgpu_writel(g, gr_gpccs_imemt_r(0),
					gr_gpccs_imemt_tag_f(tag));
		}
		nvgpu_writel(g, gr_gpccs_imemd_r(0), 0);
	}

	nvgpu_log_info(g, "gpccs imem checksum: 0x%x", checksum);
}

void gm20b_gr_falcon_load_fecs_imem(struct gk20a *g,
			const u32 *ucode_u32_data, u32 ucode_u32_size)
{
	u32 cfg, fecs_imem_size;
	u32 tag, i, pad_start, pad_end;
	u32 checksum = 0;

	/* set access for fecs imem */
	nvgpu_writel(g, gr_fecs_imemc_r(0), (gr_fecs_imemc_offs_f(0) |
					gr_fecs_imemc_blk_f(0) |
					gr_fecs_imemc_aincw_f(1)));

	cfg = nvgpu_readl(g, gr_fecs_cfg_r());
	fecs_imem_size = gr_fecs_cfg_imem_sz_v(cfg);

	/* Setup the tags for the instruction memory. */
	tag = 0;
	nvgpu_writel(g, gr_fecs_imemt_r(0), gr_fecs_imemt_tag_f(tag));

	for (i = 0; i < ucode_u32_size; i++) {
		if ((i != 0U) && ((i % (256U/sizeof(u32))) == 0U)) {
			tag++;
			nvgpu_writel(g, gr_fecs_imemt_r(0),
				      gr_fecs_imemt_tag_f(tag));
		}
		nvgpu_writel(g, gr_fecs_imemd_r(0), ucode_u32_data[i]);
		checksum += ucode_u32_data[i];
	}

	pad_start = i * 4U;
	pad_end = pad_start + (256U - pad_start % 256U) + 256U;
	for (i = pad_start;
	     (i < fecs_imem_size * 256U) && i < pad_end;
	     i += 4U) {
		if ((i != 0U) && ((i % 256U) == 0U)) {
			tag++;
			nvgpu_writel(g, gr_fecs_imemt_r(0),
				      gr_fecs_imemt_tag_f(tag));
		}
		nvgpu_writel(g, gr_fecs_imemd_r(0), 0);
	}
	nvgpu_log_info(g, "fecs imem checksum: 0x%x", checksum);
}

u32 gm20b_gr_falcon_get_gpccs_start_reg_offset(void)
{
	return (gr_gpcs_gpccs_falcon_hwcfg_r() - gr_fecs_falcon_hwcfg_r());
}

void gm20b_gr_falcon_configure_fmodel(struct gk20a *g)
{
	nvgpu_log_fn(g, " ");

	nvgpu_writel(g, gr_fecs_ctxsw_mailbox_r(7),
		gr_fecs_ctxsw_mailbox_value_f(0xc0de7777U));
	nvgpu_writel(g, gr_gpccs_ctxsw_mailbox_r(7),
		gr_gpccs_ctxsw_mailbox_value_f(0xc0de7777U));

}

void gm20b_gr_falcon_start_ucode(struct gk20a *g)
{
	nvgpu_log_fn(g, " ");

	nvgpu_writel(g, gr_fecs_ctxsw_mailbox_clear_r(0U),
		gr_fecs_ctxsw_mailbox_clear_value_f(~U32(0U)));

	nvgpu_writel(g, gr_gpccs_dmactl_r(), gr_gpccs_dmactl_require_ctx_f(0U));
	nvgpu_writel(g, gr_fecs_dmactl_r(), gr_fecs_dmactl_require_ctx_f(0U));

	nvgpu_writel(g, gr_gpccs_cpuctl_r(), gr_gpccs_cpuctl_startcpu_f(1U));
	nvgpu_writel(g, gr_fecs_cpuctl_r(), gr_fecs_cpuctl_startcpu_f(1U));

	nvgpu_log_fn(g, "done");
}


void gm20b_gr_falcon_start_gpccs(struct gk20a *g)
{
	u32 reg_offset = gr_gpcs_gpccs_falcon_hwcfg_r() -
					gr_fecs_falcon_hwcfg_r();

	if (nvgpu_is_enabled(g, NVGPU_SEC_SECUREGPCCS)) {
		nvgpu_writel(g, reg_offset +
			gr_fecs_cpuctl_alias_r(),
			gr_gpccs_cpuctl_startcpu_f(1U));
	} else {
		nvgpu_writel(g, gr_gpccs_dmactl_r(),
			gr_gpccs_dmactl_require_ctx_f(0U));
		nvgpu_writel(g, gr_gpccs_cpuctl_r(),
			gr_gpccs_cpuctl_startcpu_f(1U));
	}
}

void gm20b_gr_falcon_start_fecs(struct gk20a *g)
{
	nvgpu_writel(g, gr_fecs_ctxsw_mailbox_clear_r(0U), ~U32(0U));
	nvgpu_writel(g, gr_fecs_ctxsw_mailbox_r(1U), 1U);
	nvgpu_writel(g, gr_fecs_ctxsw_mailbox_clear_r(6U), 0xffffffffU);
	nvgpu_writel(g, gr_fecs_cpuctl_alias_r(),
			gr_fecs_cpuctl_startcpu_f(1U));
}

static void gm20b_gr_falcon_wait_for_fecs_arb_idle(struct gk20a *g)
{
	int retries = FECS_ARB_CMD_TIMEOUT_MAX_US /
			FECS_ARB_CMD_TIMEOUT_DEFAULT_US;
	u32 val;

	val = nvgpu_readl(g, gr_fecs_arb_ctx_cmd_r());
	while ((gr_fecs_arb_ctx_cmd_cmd_v(val) != 0U) && (retries != 0)) {
		nvgpu_udelay(FECS_ARB_CMD_TIMEOUT_DEFAULT_US);
		retries--;
		val = nvgpu_readl(g, gr_fecs_arb_ctx_cmd_r());
	}

	if (retries == 0) {
		nvgpu_err(g, "arbiter cmd timeout, fecs arb ctx cmd: 0x%08x",
				nvgpu_readl(g, gr_fecs_arb_ctx_cmd_r()));
	}

	retries = FECS_ARB_CMD_TIMEOUT_MAX_US /
			FECS_ARB_CMD_TIMEOUT_DEFAULT_US;
	while (((nvgpu_readl(g, gr_fecs_ctxsw_status_1_r()) &
			gr_fecs_ctxsw_status_1_arb_busy_m()) != 0U) &&
	       (retries != 0)) {
		nvgpu_udelay(FECS_ARB_CMD_TIMEOUT_DEFAULT_US);
		retries--;
	}
	if (retries == 0) {
		nvgpu_err(g,
			  "arbiter idle timeout, fecs ctxsw status: 0x%08x",
			  nvgpu_readl(g, gr_fecs_ctxsw_status_1_r()));
	}
}

void gm20b_gr_falcon_bind_instblk(struct gk20a *g,
				struct nvgpu_mem *mem, u64 inst_ptr)
{
	u32 retries = FECS_ARB_CMD_TIMEOUT_MAX_US /
			FECS_ARB_CMD_TIMEOUT_DEFAULT_US;
	u32 inst_ptr_u32;

	nvgpu_writel(g, gr_fecs_ctxsw_mailbox_clear_r(0), 0x0);

	while (((nvgpu_readl(g, gr_fecs_ctxsw_status_1_r()) &
			gr_fecs_ctxsw_status_1_arb_busy_m()) != 0U) &&
	       (retries != 0)) {
		nvgpu_udelay(FECS_ARB_CMD_TIMEOUT_DEFAULT_US);
		retries--;
	}
	if (retries == 0) {
		nvgpu_err(g,
			  "arbiter idle timeout, status: %08x",
			  nvgpu_readl(g, gr_fecs_ctxsw_status_1_r()));
	}

	nvgpu_writel(g, gr_fecs_arb_ctx_adr_r(), 0x0);

	inst_ptr >>= 12;
	BUG_ON(u64_hi32(inst_ptr) != 0U);
	inst_ptr_u32 = (u32)inst_ptr;
	nvgpu_writel(g, gr_fecs_new_ctx_r(),
		     gr_fecs_new_ctx_ptr_f(inst_ptr_u32) |
		     nvgpu_aperture_mask(g, mem,
				gr_fecs_new_ctx_target_sys_mem_ncoh_f(),
				gr_fecs_new_ctx_target_sys_mem_coh_f(),
				gr_fecs_new_ctx_target_vid_mem_f()) |
		     gr_fecs_new_ctx_valid_m());

	nvgpu_writel(g, gr_fecs_arb_ctx_ptr_r(),
		     gr_fecs_arb_ctx_ptr_ptr_f(inst_ptr_u32) |
		     nvgpu_aperture_mask(g, mem,
				gr_fecs_arb_ctx_ptr_target_sys_mem_ncoh_f(),
				gr_fecs_arb_ctx_ptr_target_sys_mem_coh_f(),
				gr_fecs_arb_ctx_ptr_target_vid_mem_f()));

	nvgpu_writel(g, gr_fecs_arb_ctx_cmd_r(), 0x7);

	/* Wait for arbiter command to complete */
	gm20b_gr_falcon_wait_for_fecs_arb_idle(g);

	nvgpu_writel(g, gr_fecs_current_ctx_r(),
			gr_fecs_current_ctx_ptr_f(inst_ptr_u32) |
			gr_fecs_current_ctx_target_m() |
			gr_fecs_current_ctx_valid_m());
	/* Send command to arbiter to flush */
	nvgpu_writel(g, gr_fecs_arb_ctx_cmd_r(), gr_fecs_arb_ctx_cmd_cmd_s());

	gm20b_gr_falcon_wait_for_fecs_arb_idle(g);

}

void gm20b_gr_falcon_load_ctxsw_ucode_header(struct gk20a *g,
	u32 reg_offset, u32 boot_signature, u32 addr_code32,
	u32 addr_data32, u32 code_size, u32 data_size)
{

	nvgpu_writel(g, reg_offset + gr_fecs_dmactl_r(),
			gr_fecs_dmactl_require_ctx_f(0));

	/*
	 * Copy falcon bootloader header into dmem at offset 0.
	 * Configure dmem port 0 for auto-incrementing writes starting at dmem
	 * offset 0.
	 */
	nvgpu_writel(g, reg_offset + gr_fecs_dmemc_r(0),
			gr_fecs_dmemc_offs_f(0) |
			gr_fecs_dmemc_blk_f(0) |
			gr_fecs_dmemc_aincw_f(1));

	/* Write out the actual data */
	switch (boot_signature) {
	case FALCON_UCODE_SIG_T18X_GPCCS_WITH_RESERVED:
	case FALCON_UCODE_SIG_T21X_FECS_WITH_DMEM_SIZE:
	case FALCON_UCODE_SIG_T21X_FECS_WITH_RESERVED:
	case FALCON_UCODE_SIG_T21X_GPCCS_WITH_RESERVED:
	case FALCON_UCODE_SIG_T12X_FECS_WITH_RESERVED:
	case FALCON_UCODE_SIG_T12X_GPCCS_WITH_RESERVED:
		nvgpu_writel(g, reg_offset + gr_fecs_dmemd_r(0), 0);
		nvgpu_writel(g, reg_offset + gr_fecs_dmemd_r(0), 0);
		nvgpu_writel(g, reg_offset + gr_fecs_dmemd_r(0), 0);
		nvgpu_writel(g, reg_offset + gr_fecs_dmemd_r(0), 0);
		/* fallthrough */
	case FALCON_UCODE_SIG_T12X_FECS_WITHOUT_RESERVED:
	case FALCON_UCODE_SIG_T12X_GPCCS_WITHOUT_RESERVED:
	case FALCON_UCODE_SIG_T21X_FECS_WITHOUT_RESERVED:
	case FALCON_UCODE_SIG_T21X_FECS_WITHOUT_RESERVED2:
	case FALCON_UCODE_SIG_T21X_GPCCS_WITHOUT_RESERVED:
		nvgpu_writel(g, reg_offset + gr_fecs_dmemd_r(0), 0);
		nvgpu_writel(g, reg_offset + gr_fecs_dmemd_r(0), 0);
		nvgpu_writel(g, reg_offset + gr_fecs_dmemd_r(0), 0);
		nvgpu_writel(g, reg_offset + gr_fecs_dmemd_r(0), 0);
		nvgpu_writel(g, reg_offset + gr_fecs_dmemd_r(0), 4);
		nvgpu_writel(g, reg_offset + gr_fecs_dmemd_r(0),
				addr_code32);
		nvgpu_writel(g, reg_offset + gr_fecs_dmemd_r(0), 0);
		nvgpu_writel(g, reg_offset + gr_fecs_dmemd_r(0),
				code_size);
		nvgpu_writel(g, reg_offset + gr_fecs_dmemd_r(0), 0);
		nvgpu_writel(g, reg_offset + gr_fecs_dmemd_r(0), 0);
		nvgpu_writel(g, reg_offset + gr_fecs_dmemd_r(0), 0);
		nvgpu_writel(g, reg_offset + gr_fecs_dmemd_r(0),
				addr_data32);
		nvgpu_writel(g, reg_offset + gr_fecs_dmemd_r(0),
				data_size);
		break;
	case FALCON_UCODE_SIG_T12X_FECS_OLDER:
	case FALCON_UCODE_SIG_T12X_GPCCS_OLDER:
		nvgpu_writel(g, reg_offset + gr_fecs_dmemd_r(0), 0);
		nvgpu_writel(g, reg_offset + gr_fecs_dmemd_r(0),
				addr_code32);
		nvgpu_writel(g, reg_offset + gr_fecs_dmemd_r(0), 0);
		nvgpu_writel(g, reg_offset + gr_fecs_dmemd_r(0),
				code_size);
		nvgpu_writel(g, reg_offset + gr_fecs_dmemd_r(0), 0);
		nvgpu_writel(g, reg_offset + gr_fecs_dmemd_r(0),
				addr_data32);
		nvgpu_writel(g, reg_offset + gr_fecs_dmemd_r(0),
				data_size);
		nvgpu_writel(g, reg_offset + gr_fecs_dmemd_r(0),
				addr_code32);
		nvgpu_writel(g, reg_offset + gr_fecs_dmemd_r(0), 0);
		nvgpu_writel(g, reg_offset + gr_fecs_dmemd_r(0), 0);
		break;
	default:
		nvgpu_err(g,
				"unknown falcon ucode boot signature 0x%08x"
				" with reg_offset 0x%08x",
				boot_signature, reg_offset);
		BUG();
	}
}

void gm20b_gr_falcon_load_ctxsw_ucode_boot(struct gk20a *g, u32 reg_offset,
			u32 boot_entry, u32 addr_load32, u32 blocks, u32 dst)
{
	u32 b;

	/*
	 * Set the base FB address for the DMA transfer. Subtract off the 256
	 * byte IMEM block offset such that the relative FB and IMEM offsets
	 * match, allowing the IMEM tags to be properly created.
	 */

	nvgpu_writel(g, reg_offset + gr_fecs_dmatrfbase_r(),
			(addr_load32 - (dst >> 8)));

	for (b = 0; b < blocks; b++) {
		/* Setup destination IMEM offset */
		nvgpu_writel(g, reg_offset + gr_fecs_dmatrfmoffs_r(),
				dst + (b << 8));

		/* Setup source offset (relative to BASE) */
		nvgpu_writel(g, reg_offset + gr_fecs_dmatrffboffs_r(),
				dst + (b << 8));

		nvgpu_writel(g, reg_offset + gr_fecs_dmatrfcmd_r(),
				gr_fecs_dmatrfcmd_imem_f(0x01) |
				gr_fecs_dmatrfcmd_write_f(0x00) |
				gr_fecs_dmatrfcmd_size_f(0x06) |
				gr_fecs_dmatrfcmd_ctxdma_f(0));
	}

	/* Specify the falcon boot vector */
	nvgpu_writel(g, reg_offset + gr_fecs_bootvec_r(),
			gr_fecs_bootvec_vec_f(boot_entry));

	/* start the falcon immediately if PRIV security is disabled*/
	if (!nvgpu_is_enabled(g, NVGPU_SEC_PRIVSECURITY)) {
		nvgpu_writel(g, reg_offset + gr_fecs_cpuctl_r(),
				gr_fecs_cpuctl_startcpu_f(0x01));
	}
}

int gm20b_gr_falcon_wait_mem_scrubbing(struct gk20a *g)
{
	struct nvgpu_timeout timeout;
	int err;
	bool fecs_scrubbing;
	bool gpccs_scrubbing;

	nvgpu_log_fn(g, " ");

	err = nvgpu_timeout_init(g, &timeout,
			   CTXSW_MEM_SCRUBBING_TIMEOUT_MAX_US /
				CTXSW_MEM_SCRUBBING_TIMEOUT_DEFAULT_US,
			   NVGPU_TIMER_RETRY_TIMER);
	if (err != 0) {
		nvgpu_err(g, "ctxsw mem scrub timeout_init failed: %d", err);
		return err;
	}

	do {
		fecs_scrubbing = (nvgpu_readl(g, gr_fecs_dmactl_r()) &
			(gr_fecs_dmactl_imem_scrubbing_m() |
			 gr_fecs_dmactl_dmem_scrubbing_m())) != 0U;

		gpccs_scrubbing = (nvgpu_readl(g, gr_gpccs_dmactl_r()) &
			(gr_gpccs_dmactl_imem_scrubbing_m() |
			 gr_gpccs_dmactl_imem_scrubbing_m())) != 0U;

		if (!fecs_scrubbing && !gpccs_scrubbing) {
			nvgpu_log_fn(g, "done");
			return 0;
		}

		nvgpu_udelay(CTXSW_MEM_SCRUBBING_TIMEOUT_DEFAULT_US);
	} while (nvgpu_timeout_expired(&timeout) == 0);

	nvgpu_err(g, "Falcon mem scrubbing timeout");
	return -ETIMEDOUT;
}

static int gm20b_gr_falcon_ctx_wait_ucode(struct gk20a *g, u32 mailbox_id,
			    u32 *mailbox_ret, u32 opc_success,
			    u32 mailbox_ok, u32 opc_fail,
			    u32 mailbox_fail, bool sleepduringwait)
{
	struct nvgpu_timeout timeout;
	u32 delay = GR_FECS_POLL_INTERVAL;
	enum wait_ucode_status check = WAIT_UCODE_LOOP;
	u32 reg;
	int err;

	nvgpu_log_fn(g, " ");

	if (sleepduringwait) {
		delay = POLL_DELAY_MIN_US;
	}

	err = nvgpu_timeout_init(g, &timeout, nvgpu_get_poll_timeout(g),
			   NVGPU_TIMER_CPU_TIMER);
	if (err != 0) {
		nvgpu_err(g, "ctxsw wait ucode timeout_init failed: %d", err);
		return err;
	}

	while (check == WAIT_UCODE_LOOP) {
		if (nvgpu_timeout_expired(&timeout) != 0) {
			check = WAIT_UCODE_TIMEOUT;
		}

		reg = nvgpu_readl(g, gr_fecs_ctxsw_mailbox_r(mailbox_id));

		if (mailbox_ret != NULL) {
			*mailbox_ret = reg;
		}

		switch (opc_success) {
		case GR_IS_UCODE_OP_EQUAL:
			if (reg == mailbox_ok) {
				check = WAIT_UCODE_OK;
			}
			break;
		case GR_IS_UCODE_OP_NOT_EQUAL:
			if (reg != mailbox_ok) {
				check = WAIT_UCODE_OK;
			}
			break;
		case GR_IS_UCODE_OP_AND:
			if ((reg & mailbox_ok) != 0U) {
				check = WAIT_UCODE_OK;
			}
			break;
		case GR_IS_UCODE_OP_LESSER:
			if (reg < mailbox_ok) {
				check = WAIT_UCODE_OK;
			}
			break;
		case GR_IS_UCODE_OP_LESSER_EQUAL:
			if (reg <= mailbox_ok) {
				check = WAIT_UCODE_OK;
			}
			break;
		case GR_IS_UCODE_OP_SKIP:
			/* do no success check */
			break;
		default:
			nvgpu_err(g,
				   "invalid success opcode 0x%x", opc_success);

			check = WAIT_UCODE_ERROR;
			break;
		}

		switch (opc_fail) {
		case GR_IS_UCODE_OP_EQUAL:
			if (reg == mailbox_fail) {
				check = WAIT_UCODE_ERROR;
			}
			break;
		case GR_IS_UCODE_OP_NOT_EQUAL:
			if (reg != mailbox_fail) {
				check = WAIT_UCODE_ERROR;
			}
			break;
		case GR_IS_UCODE_OP_AND:
			if ((reg & mailbox_fail) != 0U) {
				check = WAIT_UCODE_ERROR;
			}
			break;
		case GR_IS_UCODE_OP_LESSER:
			if (reg < mailbox_fail) {
				check = WAIT_UCODE_ERROR;
			}
			break;
		case GR_IS_UCODE_OP_LESSER_EQUAL:
			if (reg <= mailbox_fail) {
				check = WAIT_UCODE_ERROR;
			}
			break;
		case GR_IS_UCODE_OP_SKIP:
			/* do no check on fail*/
			break;
		default:
			nvgpu_err(g,
				   "invalid fail opcode 0x%x", opc_fail);
			check = WAIT_UCODE_ERROR;
			break;
		}

		if (sleepduringwait) {
			nvgpu_usleep_range(delay, delay * 2U);
			delay = min_t(u32, delay << 1, POLL_DELAY_MAX_US);
		} else {
			nvgpu_udelay(delay);
		}
	}

	if (check == WAIT_UCODE_TIMEOUT) {
		nvgpu_err(g,
			   "timeout waiting on mailbox=%d value=0x%08x",
			   mailbox_id, reg);
		g->ops.gr.falcon.dump_stats(g);
		gk20a_gr_debug_dump(g);
		return -1;
	} else if (check == WAIT_UCODE_ERROR) {
		nvgpu_err(g,
			   "ucode method failed on mailbox=%d value=0x%08x",
			   mailbox_id, reg);
		g->ops.gr.falcon.dump_stats(g);
		return -1;
	}

	nvgpu_log_fn(g, "done");
	return 0;
}

int gm20b_gr_falcon_wait_ctxsw_ready(struct gk20a *g)
{
	int ret;
	uint32_t wdt_val = CTXSW_WDT_DEFAULT_VALUE;
	unsigned long sysclk_freq_mhz = 0UL;

	nvgpu_log_fn(g, " ");

	ret = gm20b_gr_falcon_ctx_wait_ucode(g, 0, NULL,
				      GR_IS_UCODE_OP_EQUAL,
				      eUcodeHandshakeInitComplete,
				      GR_IS_UCODE_OP_SKIP, 0, false);
	if (ret != 0) {
		nvgpu_err(g, "falcon ucode init timeout");
		return ret;
	}

	if (nvgpu_is_enabled(g, NVGPU_GR_USE_DMA_FOR_FW_BOOTSTRAP) ||
		nvgpu_is_enabled(g, NVGPU_SEC_SECUREGPCCS)) {
		nvgpu_writel(g, gr_fecs_current_ctx_r(),
			gr_fecs_current_ctx_valid_false_f());
	}

	if (nvgpu_platform_is_silicon(g)) {
		if (g->ops.clk.measure_freq != NULL) {
			sysclk_freq_mhz = g->ops.clk.measure_freq(g,
					CTRL_CLK_DOMAIN_SYSCLK) / MHZ;
			if (sysclk_freq_mhz == 0UL) {
				nvgpu_err(g, "failed to get SYSCLK freq");

				return -1;
			}
			nvgpu_log_info(g, "SYSCLK = %lu MHz", sysclk_freq_mhz);
			if (g->ctxsw_wdt_period_us != 0U) {
				wdt_val = (unsigned int)(sysclk_freq_mhz *
						g->ctxsw_wdt_period_us);
			}
		}
	}

	nvgpu_log_info(g, "configuring ctxsw_ucode wdt = 0x%x", wdt_val);
	nvgpu_writel(g, gr_fecs_ctxsw_mailbox_clear_r(0), U32_MAX);
	nvgpu_writel(g, gr_fecs_method_data_r(), wdt_val);
	nvgpu_writel(g, gr_fecs_method_push_r(),
		     gr_fecs_method_push_adr_set_watchdog_timeout_f());

	nvgpu_log_fn(g, "done");
	return 0;
}

int gm20b_gr_falcon_init_ctx_state(struct gk20a *g,
		struct nvgpu_gr_falcon_query_sizes *sizes)
{
	int ret;

	nvgpu_log_fn(g, " ");

	ret = gm20b_gr_falcon_ctrl_ctxsw(g,
		NVGPU_GR_FALCON_METHOD_CTXSW_DISCOVER_IMAGE_SIZE,
		0, &sizes->golden_image_size);
	if (ret != 0) {
		nvgpu_err(g,
			   "query golden image size failed");
		return ret;
	}
	ret = gm20b_gr_falcon_ctrl_ctxsw(g,
		NVGPU_GR_FALCON_METHOD_CTXSW_DISCOVER_PM_IMAGE_SIZE,
		0, &sizes->pm_ctxsw_image_size);
	if (ret != 0) {
		nvgpu_err(g,
			   "query pm ctx image size failed");
		return ret;
	}
	ret = gm20b_gr_falcon_ctrl_ctxsw(g,
		NVGPU_GR_FALCON_METHOD_CTXSW_DISCOVER_ZCULL_IMAGE_SIZE,
		0, &sizes->zcull_image_size);
	if (ret != 0) {
		nvgpu_err(g,
			"query zcull ctx image size failed");
		return ret;
	}

	nvgpu_log_fn(g, "done");
	return 0;
}

u32 gm20b_gr_falcon_fecs_base_addr(void)
{
	return gr_fecs_irqsset_r();
}

u32 gm20b_gr_falcon_gpccs_base_addr(void)
{
	return gr_gpcs_gpccs_irqsset_r();
}

void gm20b_gr_falcon_fecs_dump_stats(struct gk20a *g)
{
	unsigned int i;

	nvgpu_falcon_dump_stats(&g->fecs_flcn);

	for (i = 0; i < g->ops.gr.falcon.fecs_ctxsw_mailbox_size(); i++) {
		nvgpu_err(g, "gr_fecs_ctxsw_mailbox_r(%d) : 0x%x",
			i, nvgpu_readl(g, gr_fecs_ctxsw_mailbox_r(i)));
	}
}

u32 gm20b_gr_falcon_get_fecs_ctx_state_store_major_rev_id(struct gk20a *g)
{
	return nvgpu_readl(g, gr_fecs_ctx_state_store_major_rev_id_r());
}

u32 gm20b_gr_falcon_get_fecs_ctxsw_mailbox_size(void)
{
	return gr_fecs_ctxsw_mailbox__size_1_v();
}

void gm20b_gr_falcon_set_current_ctx_invalid(struct gk20a *g)
{
	nvgpu_writel(g, gr_fecs_current_ctx_r(),
		gr_fecs_current_ctx_valid_false_f());
}

/* The following is a less brittle way to call gr_gk20a_submit_fecs_method(...)
 * We should replace most, if not all, fecs method calls to this instead.
 */
int gm20b_gr_falcon_submit_fecs_method_op(struct gk20a *g,
				   struct nvgpu_fecs_method_op op,
				   bool sleepduringwait)
{
	int ret;
	struct nvgpu_gr *gr = g->gr;

	nvgpu_mutex_acquire(&gr->falcon->fecs_mutex);

	if (op.mailbox.id != 0U) {
		nvgpu_writel(g, gr_fecs_ctxsw_mailbox_r(op.mailbox.id),
			     op.mailbox.data);
	}

	nvgpu_writel(g, gr_fecs_ctxsw_mailbox_clear_r(0),
		gr_fecs_ctxsw_mailbox_clear_value_f(op.mailbox.clr));

	nvgpu_writel(g, gr_fecs_method_data_r(), op.method.data);
	nvgpu_writel(g, gr_fecs_method_push_r(),
		gr_fecs_method_push_adr_f(op.method.addr));

	/* op.mailbox.id == 4 cases require waiting for completion on
	 * for op.mailbox.id == 0
	 */
	if (op.mailbox.id == 4U) {
		op.mailbox.id = 0;
	}

	ret = gm20b_gr_falcon_ctx_wait_ucode(g, op.mailbox.id, op.mailbox.ret,
				      op.cond.ok, op.mailbox.ok,
				      op.cond.fail, op.mailbox.fail,
				      sleepduringwait);
	if (ret != 0) {
		nvgpu_err(g, "fecs method: data=0x%08x push adr=0x%08x",
			op.method.data, op.method.addr);
	}

	nvgpu_mutex_release(&gr->falcon->fecs_mutex);

	return ret;
}

/* Sideband mailbox writes are done a bit differently */
int gm20b_gr_falcon_submit_fecs_sideband_method_op(struct gk20a *g,
		struct nvgpu_fecs_method_op op)
{
	int ret;
	struct nvgpu_gr *gr = g->gr;

	nvgpu_mutex_acquire(&gr->falcon->fecs_mutex);

	nvgpu_writel(g, gr_fecs_ctxsw_mailbox_clear_r(op.mailbox.id),
		gr_fecs_ctxsw_mailbox_clear_value_f(op.mailbox.clr));

	nvgpu_writel(g, gr_fecs_method_data_r(), op.method.data);
	nvgpu_writel(g, gr_fecs_method_push_r(),
		gr_fecs_method_push_adr_f(op.method.addr));

	ret = gm20b_gr_falcon_ctx_wait_ucode(g, op.mailbox.id, op.mailbox.ret,
				      op.cond.ok, op.mailbox.ok,
				      op.cond.fail, op.mailbox.fail,
				      false);
	if (ret != 0) {
		nvgpu_err(g, "fecs method: data=0x%08x push adr=0x%08x",
			op.method.data, op.method.addr);
	}

	nvgpu_mutex_release(&gr->falcon->fecs_mutex);

	return ret;
}

int gm20b_gr_falcon_ctrl_ctxsw(struct gk20a *g, u32 fecs_method,
			u32 data, u32 *ret_val)
{
	struct nvgpu_fecs_method_op op = {
		.mailbox = { .id = 0U, .data = 0U, .ret = NULL,
			     .clr = ~U32(0U), .ok = 0U, .fail = 0U},
		.method.data = 0U,
		.cond.ok = GR_IS_UCODE_OP_NOT_EQUAL,
		.cond.fail = GR_IS_UCODE_OP_SKIP,
		};
	bool sleepduringwait = false;

	nvgpu_log_info(g, "fecs method %d data 0x%x ret_value %p",
						fecs_method, data, ret_val);

	switch (fecs_method) {
	case NVGPU_GR_FALCON_METHOD_CTXSW_STOP:
		op.method.addr =
				gr_fecs_method_push_adr_stop_ctxsw_v();
		op.method.data = ~U32(0U);
		op.mailbox.id = 1U, /* sideband mailbox */
		op.mailbox.ok = gr_fecs_ctxsw_mailbox_value_pass_v();
		op.mailbox.fail = gr_fecs_ctxsw_mailbox_value_fail_v();
		op.cond.ok = GR_IS_UCODE_OP_EQUAL;
		op.cond.fail = GR_IS_UCODE_OP_EQUAL;
		sleepduringwait = true;
	break;

	case NVGPU_GR_FALCON_METHOD_CTXSW_START:
		op.method.addr =
				gr_fecs_method_push_adr_start_ctxsw_v();
		op.method.data = ~U32(0U);
		op.mailbox.id = 1U, /* sideband mailbox */
		op.mailbox.ok = gr_fecs_ctxsw_mailbox_value_pass_v();
		op.mailbox.fail = gr_fecs_ctxsw_mailbox_value_fail_v();
		op.cond.ok = GR_IS_UCODE_OP_EQUAL;
		op.cond.fail = GR_IS_UCODE_OP_EQUAL;
		sleepduringwait = true;
	break;

	case NVGPU_GR_FALCON_METHOD_HALT_PIPELINE:
		op.method.addr =
				gr_fecs_method_push_adr_halt_pipeline_v();
		op.method.data = ~U32(0U);
		op.mailbox.id = 1U, /* sideband mailbox */
		op.mailbox.ok = gr_fecs_ctxsw_mailbox_value_pass_v();
		op.mailbox.fail = gr_fecs_ctxsw_mailbox_value_fail_v();
		op.cond.ok = GR_IS_UCODE_OP_EQUAL;
		op.cond.fail = GR_IS_UCODE_OP_EQUAL;
	break;
	case NVGPU_GR_FALCON_METHOD_CTXSW_DISCOVER_IMAGE_SIZE:
		op.method.addr =
			gr_fecs_method_push_adr_discover_image_size_v();
		op.mailbox.ret = ret_val;
		break;
	case NVGPU_GR_FALCON_METHOD_CTXSW_DISCOVER_ZCULL_IMAGE_SIZE:
		op.method.addr =
			gr_fecs_method_push_adr_discover_zcull_image_size_v();
		op.mailbox.ret = ret_val;
		break;
	case NVGPU_GR_FALCON_METHOD_CTXSW_DISCOVER_PM_IMAGE_SIZE:
		op.method.addr =
			gr_fecs_method_push_adr_discover_pm_image_size_v();
		op.mailbox.ret = ret_val;
		sleepduringwait = true;
		break;
	case NVGPU_GR_FALCON_METHOD_REGLIST_DISCOVER_IMAGE_SIZE:
		op.method.addr =
			gr_fecs_method_push_adr_discover_reglist_image_size_v();
		op.method.data = 1U;
		op.mailbox.ret = ret_val;
		break;
	case NVGPU_GR_FALCON_METHOD_REGLIST_BIND_INSTANCE:
		op.method.addr =
			gr_fecs_method_push_adr_set_reglist_bind_instance_v();
		op.method.data = 1U;
		op.mailbox.data = data;
		op.mailbox.id = 4U;
		op.mailbox.ok = 1U;
		op.cond.ok = GR_IS_UCODE_OP_EQUAL;
		break;
	case NVGPU_GR_FALCON_METHOD_REGLIST_SET_VIRTUAL_ADDRESS:
		op.method.addr =
			gr_fecs_method_push_adr_set_reglist_virtual_address_v(),
		op.method.data = 1U;
		op.mailbox.data = data;
		op.mailbox.id = 4U;
		op.mailbox.ok = 1U;
		op.cond.ok = GR_IS_UCODE_OP_EQUAL;
		break;

	case NVGPU_GR_FALCON_METHOD_ADDRESS_BIND_PTR:
		op.method.addr = gr_fecs_method_push_adr_bind_pointer_v();
		op.method.data = data;
		op.mailbox.clr = 0x30U;
		op.mailbox.ok = 0x10U;
		op.mailbox.fail = 0x20U;
		op.cond.ok = GR_IS_UCODE_OP_AND;
		op.cond.fail = GR_IS_UCODE_OP_AND;
		sleepduringwait = true;
		break;

	case NVGPU_GR_FALCON_METHOD_GOLDEN_IMAGE_SAVE:
		op.method.addr = gr_fecs_method_push_adr_wfi_golden_save_v();
		op.method.data = data;
		op.mailbox.clr = 0x3U;
		op.mailbox.ok = 0x1U;
		op.mailbox.fail = 0x2U;
		op.cond.ok = GR_IS_UCODE_OP_AND;
		op.cond.fail = GR_IS_UCODE_OP_AND;
		sleepduringwait = true;
		break;

	case NVGPU_GR_FALCON_METHOD_FECS_TRACE_FLUSH:
		op.method.addr =
			gr_fecs_method_push_adr_write_timestamp_record_v();
		break;

	default:
		nvgpu_err(g, "unsupported fecs mode %d", fecs_method);
		break;
	}
	return gm20b_gr_falcon_submit_fecs_method_op(g, op, sleepduringwait);
}

u32 gm20b_gr_falcon_get_current_ctx(struct gk20a *g)
{
	return nvgpu_readl(g, gr_fecs_current_ctx_r());
}

u32 gm20b_gr_falcon_get_ctx_ptr(u32 ctx)
{
	return gr_fecs_current_ctx_ptr_v(ctx);
}

u32 gm20b_gr_falcon_get_fecs_current_ctx_data(struct gk20a *g,
					struct nvgpu_mem *inst_block)
{
	u64 ptr = nvgpu_inst_block_addr(g, inst_block) >>
			g->ops.ramin.base_shift();

	u32 aperture = nvgpu_aperture_mask(g, inst_block,
				gr_fecs_current_ctx_target_sys_mem_ncoh_f(),
				gr_fecs_current_ctx_target_sys_mem_coh_f(),
				gr_fecs_current_ctx_target_vid_mem_f());

	return gr_fecs_current_ctx_ptr_f(u64_lo32(ptr)) | aperture |
		gr_fecs_current_ctx_valid_f(1);
}

void gm20b_gr_falcon_fecs_host_int_enable(struct gk20a *g)
{
	nvgpu_writel(g, gr_fecs_host_int_enable_r(),
		     gr_fecs_host_int_enable_ctxsw_intr1_enable_f() |
		     gr_fecs_host_int_enable_fault_during_ctxsw_enable_f() |
		     gr_fecs_host_int_enable_umimp_firmware_method_enable_f() |
		     gr_fecs_host_int_enable_umimp_illegal_method_enable_f() |
		     gr_fecs_host_int_enable_watchdog_enable_f());
}

u32 gm20b_gr_falcon_read_fecs_ctxsw_mailbox(struct gk20a *g, u32 reg_index)
{
	return nvgpu_readl(g, gr_fecs_ctxsw_mailbox_r(reg_index));
}

void gm20b_gr_falcon_fecs_host_clear_intr(struct gk20a *g, u32 fecs_intr)
{
	nvgpu_writel(g, gr_fecs_host_int_clear_r(), fecs_intr);
}

u32 gm20b_gr_falcon_fecs_host_intr_status(struct gk20a *g,
			struct nvgpu_fecs_host_intr_status *fecs_host_intr)
{
	u32 gr_fecs_intr = nvgpu_readl(g, gr_fecs_host_int_status_r());

	(void) memset(fecs_host_intr, 0,
				sizeof(struct nvgpu_fecs_host_intr_status));
	if ((gr_fecs_intr &
	     gr_fecs_host_int_status_umimp_firmware_method_f(1)) != 0U) {
		fecs_host_intr->unimp_fw_method_active = true;
	} else if ((gr_fecs_intr &
			gr_fecs_host_int_status_watchdog_active_f()) != 0U) {
		fecs_host_intr->watchdog_active = true;
	} else if ((gr_fecs_intr &
		    gr_fecs_host_int_status_ctxsw_intr_f(CTXSW_INTR0)) != 0U) {
		fecs_host_intr->ctxsw_intr0 =
			gr_fecs_host_int_status_ctxsw_intr_f(CTXSW_INTR0);
	} else if ((gr_fecs_intr &
		    gr_fecs_host_int_status_ctxsw_intr_f(CTXSW_INTR1)) != 0U) {
		fecs_host_intr->ctxsw_intr1 =
			gr_fecs_host_int_clear_ctxsw_intr1_clear_f();
	} else if ((gr_fecs_intr &
		    gr_fecs_host_int_status_fault_during_ctxsw_f(1)) != 0U) {
		fecs_host_intr->fault_during_ctxsw_active = true;
	}

	return gr_fecs_intr;
}

u32 gm20b_gr_falcon_read_fecs_ctxsw_status0(struct gk20a *g)
{
	return nvgpu_readl(g, gr_fecs_ctxsw_status_fe_0_r());
}

u32 gm20b_gr_falcon_read_fecs_ctxsw_status1(struct gk20a *g)
{
	return nvgpu_readl(g, gr_fecs_ctxsw_status_1_r());
}
