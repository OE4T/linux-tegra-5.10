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
