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

#include "gr_falcon_gm20b.h"

#include <nvgpu/hw/gm20b/hw_gr_gm20b.h>

#define FECS_ARB_CMD_TIMEOUT_MAX_US 40U
#define FECS_ARB_CMD_TIMEOUT_DEFAULT_US 2U

void gm20b_gr_falcon_load_gpccs_dmem(struct gk20a *g,
			const u32 *ucode_u32_data, u32 ucode_u32_size)
{
	u32 i, checksum;

	/* enable access for gpccs dmem */
	nvgpu_writel(g, gr_gpccs_dmemc_r(0), (gr_gpccs_dmemc_offs_f(0) |
					gr_gpccs_dmemc_blk_f(0)  |
					gr_gpccs_dmemc_aincw_f(1)));

	for (i = 0, checksum = 0; i < ucode_u32_size; i++) {
		nvgpu_writel(g, gr_gpccs_dmemd_r(0), ucode_u32_data[i]);
		checksum += ucode_u32_data[i];
	}
	nvgpu_log_info(g, "gpccs dmem checksum: 0x%x", checksum);
}

void gm20b_gr_falcon_load_fecs_dmem(struct gk20a *g,
			const u32 *ucode_u32_data, u32 ucode_u32_size)
{
	u32 i, checksum;

	/* set access for fecs dmem */
	nvgpu_writel(g, gr_fecs_dmemc_r(0), (gr_fecs_dmemc_offs_f(0) |
					gr_fecs_dmemc_blk_f(0)  |
					gr_fecs_dmemc_aincw_f(1)));

	for (i = 0, checksum = 0; i < ucode_u32_size; i++) {
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
	u32 checksum;

	/* enable access for gpccs imem */
	nvgpu_writel(g, gr_gpccs_imemc_r(0), (gr_gpccs_imemc_offs_f(0) |
					gr_gpccs_imemc_blk_f(0) |
					gr_gpccs_imemc_aincw_f(1)));

	cfg = nvgpu_readl(g, gr_gpc0_cfg_r());
	gpccs_imem_size = gr_gpc0_cfg_imem_sz_v(cfg);

	/* Setup the tags for the instruction memory. */
	tag = 0;
	nvgpu_writel(g, gr_gpccs_imemt_r(0), gr_gpccs_imemt_tag_f(tag));

	for (i = 0, checksum = 0; i < ucode_u32_size; i++) {
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
	u32 checksum;

	/* set access for fecs imem */
	nvgpu_writel(g, gr_fecs_imemc_r(0), (gr_fecs_imemc_offs_f(0) |
					gr_fecs_imemc_blk_f(0) |
					gr_fecs_imemc_aincw_f(1)));

	cfg = nvgpu_readl(g, gr_fecs_cfg_r());
	fecs_imem_size = gr_fecs_cfg_imem_sz_v(cfg);

	/* Setup the tags for the instruction memory. */
	tag = 0;
	nvgpu_writel(g, gr_fecs_imemt_r(0), gr_fecs_imemt_tag_f(tag));

	for (i = 0, checksum = 0; i < ucode_u32_size; i++) {
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

