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

#include <nvgpu/types.h>
#include <nvgpu/dma.h>
#include <nvgpu/acr/nvgpu_acr.h>
#include <nvgpu/gk20a.h>

#include "acr_gm20b.h"
#include "acr_gp10b.h"
#include "acr_gv11b.h"
#include "acr_gv100.h"
#include "acr_tu104.h"

/* Both size and address of WPR need to be 128K-aligned */
#define DGPU_WPR_SIZE 0x200000U

int nvgpu_acr_alloc_blob_space_sys(struct gk20a *g, size_t size,
	struct nvgpu_mem *mem)
{
	return nvgpu_dma_alloc_flags_sys(g, NVGPU_DMA_PHYSICALLY_ADDRESSED,
		size, mem);
}

int nvgpu_acr_alloc_blob_space_vid(struct gk20a *g, size_t size,
	struct nvgpu_mem *mem)
{
	struct wpr_carveout_info wpr_inf;
	int err;

	if (mem->size != 0ULL) {
		return 0;
	}

	g->acr.get_wpr_info(g, &wpr_inf);

	/*
	 * Even though this mem_desc wouldn't be used, the wpr region needs to
	 * be reserved in the allocator.
	 */
	err = nvgpu_dma_alloc_vid_at(g, wpr_inf.size,
		&g->acr.wpr_dummy, wpr_inf.wpr_base);
	if (err != 0) {
		return err;
	}

	return nvgpu_dma_alloc_vid_at(g, wpr_inf.size, mem,
		wpr_inf.nonwpr_base);
}

void nvgpu_acr_wpr_info_sys(struct gk20a *g, struct wpr_carveout_info *inf)
{
	g->ops.fb.read_wpr_info(g, &inf->wpr_base, &inf->size);
}

void nvgpu_acr_wpr_info_vid(struct gk20a *g, struct wpr_carveout_info *inf)
{
	inf->wpr_base = g->mm.vidmem.bootstrap_base;
	inf->nonwpr_base = inf->wpr_base + DGPU_WPR_SIZE;
	inf->size = DGPU_WPR_SIZE;
}

int nvgpu_acr_construct_execute(struct gk20a *g){
	int err = 0;

	if (nvgpu_is_enabled(g, NVGPU_IS_FMODEL)) {
		goto done;
	}

	err = g->acr.prepare_ucode_blob(g);
	if (err != 0) {
		nvgpu_err(g, "ACR ucode blob prepare failed");
		goto done;
	}

	err = g->acr.bootstrap_hs_acr(g, &g->acr, &g->acr.acr);
	if (err != 0) {
		nvgpu_err(g, "ACR bootstrap failed");
		goto done;
	}

done:
	return err;
}

void nvgpu_acr_init(struct gk20a *g)
{
	u32 ver = g->params.gpu_arch + g->params.gpu_impl;

	if (nvgpu_is_enabled(g, NVGPU_IS_FMODEL)) {
		return;
	}

	switch (ver) {
		case GK20A_GPUID_GM20B:
		case GK20A_GPUID_GM20B_B:
			nvgpu_gm20b_acr_sw_init(g, &g->acr);
			break;
		case NVGPU_GPUID_GP10B:
			nvgpu_gp10b_acr_sw_init(g, &g->acr);
			break;
		case NVGPU_GPUID_GV11B:
			nvgpu_gv11b_acr_sw_init(g, &g->acr);
			break;
		case NVGPU_GPUID_GV100:
			nvgpu_gv100_acr_sw_init(g, &g->acr);
			break;
		case NVGPU_GPUID_TU104:
			nvgpu_tu104_acr_sw_init(g, &g->acr);
			break;
		default:
			nvgpu_err(g, "no support for GPUID %x", ver);
			break;
	}
}


