/*
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

#ifndef NVGPU_ACR_H
#define NVGPU_ACR_H

struct gk20a;
struct nvgpu_falcon;
struct nvgpu_firmware;
struct nvgpu_acr;

int nvgpu_acr_init(struct gk20a *g, struct nvgpu_acr **acr);
#ifdef CONFIG_NVGPU_DGPU
int nvgpu_acr_alloc_blob_prerequisite(struct gk20a *g, struct nvgpu_acr *acr,
	size_t size);
#endif
int nvgpu_acr_construct_execute(struct gk20a *g, struct nvgpu_acr *acr);
int nvgpu_acr_bootstrap_hs_acr(struct gk20a *g, struct nvgpu_acr *acr);
bool nvgpu_acr_is_lsf_lazy_bootstrap(struct gk20a *g, struct nvgpu_acr *acr,
	u32 falcon_id);

int nvgpu_acr_self_hs_load_bootstrap(struct gk20a *g, struct nvgpu_falcon *flcn,
	struct nvgpu_firmware *hs_fw, u32 timeout);

#endif /* NVGPU_ACR_H */

