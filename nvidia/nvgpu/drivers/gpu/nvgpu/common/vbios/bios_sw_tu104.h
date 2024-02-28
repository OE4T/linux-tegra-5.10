/*
 * Copyright (c) 2018-2020, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_BIOS_SW_TU104_H
#define NVGPU_BIOS_SW_TU104_H

#define NVGPU_BIOS_DEVINIT_VERIFY_TIMEOUT_MS		1000U
#define NVGPU_BIOS_DEVINIT_VERIFY_DELAY_US		10U
#define NVGPU_BIOS_DEVINIT_VERIFY_COMPLETION_MS		1U

struct gk20a;

int tu104_bios_verify_devinit(struct gk20a *g);
int tu104_bios_init(struct gk20a *g);
void nvgpu_tu104_bios_sw_init(struct gk20a *g,
		struct nvgpu_bios *bios);

#endif /* NVGPU_BIOS_SW_TU104_H */
