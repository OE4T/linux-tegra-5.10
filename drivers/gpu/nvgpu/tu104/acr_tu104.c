/*
 * Copyright (c) 2016-2018, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/acr/nvgpu_acr.h>
#include <nvgpu/enabled.h>
#include <nvgpu/utils.h>
#include <nvgpu/debug.h>
#include <nvgpu/kmem.h>
#include <nvgpu/pmu.h>
#include <nvgpu/dma.h>

#include "tu104/acr_tu104.h"
#include "gp106/acr_gp106.h"
#include "tu104/sec2_tu104.h"

void nvgpu_tu104_acr_sw_init(struct gk20a *g, struct nvgpu_acr *acr)
{
	nvgpu_log_fn(g, " ");

	nvgpu_gp106_acr_sw_init(g, acr);

	acr->bootstrap_owner = LSF_FALCON_ID_SEC2;
	acr->max_supported_lsfm = MAX_SUPPORTED_LSFM;

	acr->acr.acr_flcn_setup_hw_and_bl_bootstrap =
		tu104_sec2_setup_hw_and_bl_bootstrap;
}
