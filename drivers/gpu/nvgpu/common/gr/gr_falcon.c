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
#include <nvgpu/debug.h>
#include <nvgpu/firmware.h>
#include <nvgpu/mm.h>
#include <nvgpu/sec2.h>
#include <nvgpu/acr.h>
#include <nvgpu/power_features/pg.h>

static int nvgpu_gr_falcon_init_ctxsw_ucode_vaspace(struct gk20a *g)
{
	struct mm_gk20a *mm = &g->mm;
	struct vm_gk20a *vm = mm->pmu.vm;
	struct gk20a_ctxsw_ucode_info *ucode_info = &g->ctxsw_ucode_info;
	int err;

	err = g->ops.mm.alloc_inst_block(g, &ucode_info->inst_blk_desc);
	if (err != 0) {
		return err;
	}

	g->ops.mm.init_inst_block(&ucode_info->inst_blk_desc, vm, 0);

	/* Map ucode surface to GMMU */
	ucode_info->surface_desc.gpu_va = nvgpu_gmmu_map(vm,
					&ucode_info->surface_desc,
					ucode_info->surface_desc.size,
					0, /* flags */
					gk20a_mem_flag_read_only,
					false,
					ucode_info->surface_desc.aperture);
	if (ucode_info->surface_desc.gpu_va == 0ULL) {
		nvgpu_err(g, "failed to update gmmu ptes");
		return -ENOMEM;
	}

	return 0;
}

static void nvgpu_gr_falcon_init_ctxsw_ucode_segment(
	struct gk20a_ctxsw_ucode_segment *p_seg, u32 *offset, u32 size)
{
	p_seg->offset = *offset;
	p_seg->size = size;
	*offset = ALIGN(*offset + size, SZ_256);
}

static void nvgpu_gr_falcon_init_ctxsw_ucode_segments(
	struct gk20a_ctxsw_ucode_segments *segments, u32 *offset,
	struct gk20a_ctxsw_bootloader_desc *bootdesc,
	u32 code_size, u32 data_size)
{
	u32 boot_size = ALIGN(bootdesc->size, sizeof(u32));

	segments->boot_entry = bootdesc->entry_point;
	segments->boot_imem_offset = bootdesc->imem_offset;
	nvgpu_gr_falcon_init_ctxsw_ucode_segment(&segments->boot,
							offset, boot_size);
	nvgpu_gr_falcon_init_ctxsw_ucode_segment(&segments->code,
							offset, code_size);
	nvgpu_gr_falcon_init_ctxsw_ucode_segment(&segments->data,
							offset, data_size);
}

static int nvgpu_gr_falcon_copy_ctxsw_ucode_segments(
	struct gk20a *g,
	struct nvgpu_mem *dst,
	struct gk20a_ctxsw_ucode_segments *segments,
	u32 *bootimage,
	u32 *code, u32 *data)
{
	unsigned int i;

	nvgpu_mem_wr_n(g, dst, segments->boot.offset, bootimage,
			segments->boot.size);
	nvgpu_mem_wr_n(g, dst, segments->code.offset, code,
			segments->code.size);
	nvgpu_mem_wr_n(g, dst, segments->data.offset, data,
			segments->data.size);

	/* compute a "checksum" for the boot binary to detect its version */
	segments->boot_signature = 0;
	for (i = 0; i < segments->boot.size / sizeof(u32); i++) {
		segments->boot_signature += bootimage[i];
	}

	return 0;
}

int nvgpu_gr_falcon_init_ctxsw_ucode(struct gk20a *g)
{
	struct mm_gk20a *mm = &g->mm;
	struct vm_gk20a *vm = mm->pmu.vm;
	struct gk20a_ctxsw_bootloader_desc *fecs_boot_desc;
	struct gk20a_ctxsw_bootloader_desc *gpccs_boot_desc;
	struct nvgpu_firmware *fecs_fw;
	struct nvgpu_firmware *gpccs_fw;
	u32 *fecs_boot_image;
	u32 *gpccs_boot_image;
	struct gk20a_ctxsw_ucode_info *ucode_info = &g->ctxsw_ucode_info;
	u32 ucode_size;
	int err = 0;

	fecs_fw = nvgpu_request_firmware(g, GK20A_FECS_UCODE_IMAGE, 0);
	if (fecs_fw == NULL) {
		nvgpu_err(g, "failed to load fecs ucode!!");
		return -ENOENT;
	}

	fecs_boot_desc = (void *)fecs_fw->data;
	fecs_boot_image = (void *)(fecs_fw->data +
				sizeof(struct gk20a_ctxsw_bootloader_desc));

	gpccs_fw = nvgpu_request_firmware(g, GK20A_GPCCS_UCODE_IMAGE, 0);
	if (gpccs_fw == NULL) {
		nvgpu_release_firmware(g, fecs_fw);
		nvgpu_err(g, "failed to load gpccs ucode!!");
		return -ENOENT;
	}

	gpccs_boot_desc = (void *)gpccs_fw->data;
	gpccs_boot_image = (void *)(gpccs_fw->data +
				sizeof(struct gk20a_ctxsw_bootloader_desc));

	ucode_size = 0;
	nvgpu_gr_falcon_init_ctxsw_ucode_segments(&ucode_info->fecs,
		&ucode_size, fecs_boot_desc,
		g->netlist_vars->ucode.fecs.inst.count * (u32)sizeof(u32),
		g->netlist_vars->ucode.fecs.data.count * (u32)sizeof(u32));
	nvgpu_gr_falcon_init_ctxsw_ucode_segments(&ucode_info->gpccs,
		&ucode_size, gpccs_boot_desc,
		g->netlist_vars->ucode.gpccs.inst.count * (u32)sizeof(u32),
		g->netlist_vars->ucode.gpccs.data.count * (u32)sizeof(u32));

	err = nvgpu_dma_alloc_sys(g, ucode_size, &ucode_info->surface_desc);
	if (err != 0) {
		goto clean_up;
	}

	nvgpu_gr_falcon_copy_ctxsw_ucode_segments(g, &ucode_info->surface_desc,
		&ucode_info->fecs,
		fecs_boot_image,
		g->netlist_vars->ucode.fecs.inst.l,
		g->netlist_vars->ucode.fecs.data.l);

	nvgpu_release_firmware(g, fecs_fw);
	fecs_fw = NULL;

	nvgpu_gr_falcon_copy_ctxsw_ucode_segments(g, &ucode_info->surface_desc,
		&ucode_info->gpccs,
		gpccs_boot_image,
		g->netlist_vars->ucode.gpccs.inst.l,
		g->netlist_vars->ucode.gpccs.data.l);

	nvgpu_release_firmware(g, gpccs_fw);
	gpccs_fw = NULL;

	err = nvgpu_gr_falcon_init_ctxsw_ucode_vaspace(g);
	if (err != 0) {
		goto clean_up;
	}

	return 0;

clean_up:
	if (ucode_info->surface_desc.gpu_va != 0ULL) {
		nvgpu_gmmu_unmap(vm, &ucode_info->surface_desc,
				 ucode_info->surface_desc.gpu_va);
	}
	nvgpu_dma_free(g, &ucode_info->surface_desc);

	nvgpu_release_firmware(g, gpccs_fw);
	gpccs_fw = NULL;
	nvgpu_release_firmware(g, fecs_fw);
	fecs_fw = NULL;

	return err;
}

static void nvgpu_gr_falcon_load_dmem(struct gk20a *g)
{
	u32 ucode_u32_size;
	const u32 *ucode_u32_data;

	nvgpu_log_fn(g, " ");

	ucode_u32_size = g->netlist_vars->ucode.gpccs.data.count;
	ucode_u32_data = (const u32 *)g->netlist_vars->ucode.gpccs.data.l;
	g->ops.gr.falcon.load_gpccs_dmem(g, ucode_u32_data, ucode_u32_size);

	ucode_u32_size = g->netlist_vars->ucode.fecs.data.count;
	ucode_u32_data = (const u32 *)g->netlist_vars->ucode.fecs.data.l;
	g->ops.gr.falcon.load_fecs_dmem(g, ucode_u32_data, ucode_u32_size);

	nvgpu_log_fn(g, "done");
}

static void nvgpu_gr_falcon_load_imem(struct gk20a *g)
{
	u32 ucode_u32_size;
	const u32 *ucode_u32_data;

	nvgpu_log_fn(g, " ");

	ucode_u32_size = g->netlist_vars->ucode.gpccs.inst.count;
	ucode_u32_data = (const u32 *)g->netlist_vars->ucode.gpccs.inst.l;
	g->ops.gr.falcon.load_gpccs_imem(g, ucode_u32_data, ucode_u32_size);


	ucode_u32_size = g->netlist_vars->ucode.fecs.inst.count;
	ucode_u32_data = (const u32 *)g->netlist_vars->ucode.fecs.inst.l;
	g->ops.gr.falcon.load_fecs_imem(g, ucode_u32_data, ucode_u32_size);

	nvgpu_log_fn(g, "done");
}

static void nvgpu_gr_falcon_bind_instblk(struct gk20a *g)
{
	struct gk20a_ctxsw_ucode_info *ucode_info = &g->ctxsw_ucode_info;
	u64 inst_ptr;

	inst_ptr = nvgpu_inst_block_addr(g, &ucode_info->inst_blk_desc);

	g->ops.gr.falcon.bind_instblk(g, &ucode_info->inst_blk_desc,
					inst_ptr);

}

static void nvgpu_gr_falcon_load_ctxsw_ucode_header(struct gk20a *g,
	u64 addr_base, struct gk20a_ctxsw_ucode_segments *segments,
	u32 reg_offset)
{
	u32 addr_code32 = u64_lo32((addr_base + segments->code.offset) >> 8);
	u32 addr_data32 = u64_lo32((addr_base + segments->data.offset) >> 8);

	g->ops.gr.falcon.load_ctxsw_ucode_header(g, reg_offset,
		segments->boot_signature, addr_code32, addr_data32,
		segments->code.size, segments->data.size);
}

static void nvgpu_gr_falcon_load_ctxsw_ucode_boot(struct gk20a *g,
	u64 addr_base, struct gk20a_ctxsw_ucode_segments *segments,
	u32 reg_offset)
{
	u32 addr_load32 = u64_lo32((addr_base + segments->boot.offset) >> 8);
	u32 blocks = ((segments->boot.size + 0xFFU) & ~0xFFU) >> 8;
	u32 dst = segments->boot_imem_offset;

	g->ops.gr.falcon.load_ctxsw_ucode_boot(g, reg_offset,
		segments->boot_entry, addr_load32, blocks, dst);

}

static void nvgpu_gr_falcon_load_ctxsw_ucode_segments(
		struct gk20a *g, u64 addr_base,
		struct gk20a_ctxsw_ucode_segments *segments, u32 reg_offset)
{

	/* Copy falcon bootloader into dmem */
	nvgpu_gr_falcon_load_ctxsw_ucode_header(g, addr_base,
						segments, reg_offset);
	nvgpu_gr_falcon_load_ctxsw_ucode_boot(g,
					addr_base, segments, reg_offset);
}


static void nvgpu_gr_falcon_load_with_bootloader(struct gk20a *g)
{
	struct gk20a_ctxsw_ucode_info *ucode_info = &g->ctxsw_ucode_info;
	u64 addr_base = ucode_info->surface_desc.gpu_va;

	nvgpu_gr_falcon_bind_instblk(g);

	nvgpu_gr_falcon_load_ctxsw_ucode_segments(g, addr_base,
		&g->ctxsw_ucode_info.fecs, 0);

	nvgpu_gr_falcon_load_ctxsw_ucode_segments(g, addr_base,
		&g->ctxsw_ucode_info.gpccs,
		g->ops.gr.falcon.get_gpccs_start_reg_offset());
}

int nvgpu_gr_falcon_load_ctxsw_ucode(struct gk20a *g)
{
	int err;

	nvgpu_log_fn(g, " ");

	if (nvgpu_is_enabled(g, NVGPU_IS_FMODEL)) {
		g->ops.gr.falcon.configure_fmodel(g);
	}

	/*
	 * In case bootloader is not supported, revert to the old way of
	 * loading gr ucode, without the faster bootstrap routine.
	 */
	if (!nvgpu_is_enabled(g, NVGPU_GR_USE_DMA_FOR_FW_BOOTSTRAP)) {
		nvgpu_gr_falcon_load_dmem(g);
		nvgpu_gr_falcon_load_imem(g);
		g->ops.gr.falcon.start_ucode(g);
	} else {
		if (!g->gr.skip_ucode_init) {
			err =  nvgpu_gr_falcon_init_ctxsw_ucode(g);
			if (err != 0) {
				return err;
			}
		}
		nvgpu_gr_falcon_load_with_bootloader(g);
		g->gr.skip_ucode_init = true;
	}
	nvgpu_log_fn(g, "done");
	return 0;
}

static void nvgpu_gr_falcon_load_gpccs_with_bootloader(struct gk20a *g)
{
	struct gk20a_ctxsw_ucode_info *ucode_info = &g->ctxsw_ucode_info;
	u64 addr_base = ucode_info->surface_desc.gpu_va;

	nvgpu_gr_falcon_bind_instblk(g);

	nvgpu_gr_falcon_load_ctxsw_ucode_segments(g, addr_base,
		&g->ctxsw_ucode_info.gpccs,
		g->ops.gr.falcon.get_gpccs_start_reg_offset());
}

int nvgpu_gr_falcon_load_secure_ctxsw_ucode(struct gk20a *g)
{
	int err = 0;
	u8 falcon_id_mask = 0;

	nvgpu_log_fn(g, " ");

	if (nvgpu_is_enabled(g, NVGPU_IS_FMODEL)) {
		g->ops.gr.falcon.configure_fmodel(g);
	}

	g->pmu_lsf_loaded_falcon_id = 0;
	if (nvgpu_is_enabled(g, NVGPU_PMU_FECS_BOOTSTRAP_DONE)) {
		/* this must be recovery so bootstrap fecs and gpccs */
		if (!nvgpu_is_enabled(g, NVGPU_SEC_SECUREGPCCS)) {
			nvgpu_gr_falcon_load_gpccs_with_bootloader(g);
			err = g->ops.pmu.load_lsfalcon_ucode(g,
					BIT32(FALCON_ID_FECS));
		} else {
			/* bind WPR VA inst block */
			nvgpu_gr_falcon_bind_instblk(g);
			if (nvgpu_is_enabled(g, NVGPU_SUPPORT_SEC2_RTOS)) {
				err = nvgpu_sec2_bootstrap_ls_falcons(g,
					&g->sec2, FALCON_ID_FECS);
				err = nvgpu_sec2_bootstrap_ls_falcons(g,
					&g->sec2, FALCON_ID_GPCCS);
			} else if (g->support_ls_pmu) {
				err = g->ops.pmu.load_lsfalcon_ucode(g,
					BIT32(FALCON_ID_FECS) |
					BIT32(FALCON_ID_GPCCS));
			} else {
				err = nvgpu_acr_bootstrap_hs_acr(g, g->acr);
				if (err != 0) {
					nvgpu_err(g,
						"ACR GR LSF bootstrap failed");
				}
			}
		}
		if (err != 0) {
			nvgpu_err(g, "Unable to recover GR falcon");
			return err;
		}

	} else {
		/* cold boot or rg exit */
		nvgpu_set_enabled(g, NVGPU_PMU_FECS_BOOTSTRAP_DONE, true);
		if (!nvgpu_is_enabled(g, NVGPU_SEC_SECUREGPCCS)) {
			nvgpu_gr_falcon_load_gpccs_with_bootloader(g);
		} else {
			/* bind WPR VA inst block */
			nvgpu_gr_falcon_bind_instblk(g);
			if (nvgpu_acr_is_lsf_lazy_bootstrap(g, g->acr,
							FALCON_ID_FECS)) {
				falcon_id_mask |= BIT8(FALCON_ID_FECS);
			}
			if (nvgpu_acr_is_lsf_lazy_bootstrap(g, g->acr,
							FALCON_ID_GPCCS)) {
				falcon_id_mask |= BIT8(FALCON_ID_GPCCS);
			}

			if (nvgpu_is_enabled(g, NVGPU_SUPPORT_SEC2_RTOS)) {
				err = nvgpu_sec2_bootstrap_ls_falcons(g,
					&g->sec2, FALCON_ID_FECS);
				err = nvgpu_sec2_bootstrap_ls_falcons(g,
					&g->sec2, FALCON_ID_GPCCS);
			} else if (g->support_ls_pmu) {
				err = g->ops.pmu.load_lsfalcon_ucode(g,
							falcon_id_mask);
			} else {
				/* GR falcons bootstrapped by ACR */
				err = 0;
			}

			if (err != 0) {
				nvgpu_err(g, "Unable to boot GPCCS");
				return err;
			}
		}
	}

	g->ops.gr.falcon.start_gpccs(g);
	g->ops.gr.falcon.start_fecs(g);

	nvgpu_log_fn(g, "done");

	return 0;
}

/**
 * Stop processing (stall) context switches at FECS:-
 * If fecs is sent stop_ctxsw method, elpg entry/exit cannot happen
 * and may timeout. It could manifest as different error signatures
 * depending on when stop_ctxsw fecs method gets sent with respect
 * to pmu elpg sequence. It could come as pmu halt or abort or
 * maybe ext error too.
*/
int nvgpu_gr_falcon_disable_ctxsw(struct gk20a *g)
{
	int err = 0;

	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg, " ");

	nvgpu_mutex_acquire(&g->ctxsw_disable_lock);
	g->ctxsw_disable_count++;
	if (g->ctxsw_disable_count == 1) {
		err = nvgpu_pg_elpg_disable(g);
		if (err != 0) {
			nvgpu_err(g,
				"failed to disable elpg for stop_ctxsw");
			/* stop ctxsw command is not sent */
			g->ctxsw_disable_count--;
		} else {
			err = g->ops.gr.falcon.ctrl_ctxsw(g,
				NVGPU_GR_FALCON_METHOD_CTXSW_STOP, 0U, NULL);
			if (err != 0) {
				nvgpu_err(g, "failed to stop fecs ctxsw");
				/* stop ctxsw failed */
				g->ctxsw_disable_count--;
			}
		}
	} else {
		nvgpu_log_info(g, "ctxsw disabled, ctxsw_disable_count: %d",
			g->ctxsw_disable_count);
	}
	nvgpu_mutex_release(&g->ctxsw_disable_lock);

	return err;
}

/* Start processing (continue) context switches at FECS */
int nvgpu_gr_falcon_enable_ctxsw(struct gk20a *g)
{
	int err = 0;

	nvgpu_log(g, gpu_dbg_fn | gpu_dbg_gpu_dbg, " ");

	nvgpu_mutex_acquire(&g->ctxsw_disable_lock);
	if (g->ctxsw_disable_count == 0) {
		goto ctxsw_already_enabled;
	}
	g->ctxsw_disable_count--;
	WARN_ON(g->ctxsw_disable_count < 0);
	if (g->ctxsw_disable_count == 0) {
		err = g->ops.gr.falcon.ctrl_ctxsw(g,
				NVGPU_GR_FALCON_METHOD_CTXSW_START, 0U, NULL);
		if (err != 0) {
			nvgpu_err(g, "failed to start fecs ctxsw");
		} else {
			if (nvgpu_pg_elpg_enable(g) != 0) {
				nvgpu_err(g,
					"failed to enable elpg for start_ctxsw");
			}
		}
	} else {
		nvgpu_log_info(g, "ctxsw_disable_count: %d is not 0 yet",
			g->ctxsw_disable_count);
	}
ctxsw_already_enabled:
	nvgpu_mutex_release(&g->ctxsw_disable_lock);

	return err;
}

int nvgpu_gr_falcon_halt_pipe(struct gk20a *g)
{
	return g->ops.gr.falcon.ctrl_ctxsw(g,
				NVGPU_GR_FALCON_METHOD_HALT_PIPELINE, 0U, NULL);
}
