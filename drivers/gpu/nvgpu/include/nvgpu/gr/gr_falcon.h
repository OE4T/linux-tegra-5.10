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

#ifndef NVGPU_GR_FALCON_H
#define NVGPU_GR_FALCON_H

#include <nvgpu/types.h>

struct gk20a;

#define NVGPU_GR_FALCON_METHOD_CTXSW_STOP 0
#define NVGPU_GR_FALCON_METHOD_CTXSW_START 1
#define NVGPU_GR_FALCON_METHOD_HALT_PIPELINE 2

int nvgpu_gr_falcon_init_ctxsw(struct gk20a *g);
int nvgpu_gr_falcon_init_ctxsw_ucode(struct gk20a *g);
int nvgpu_gr_falcon_load_ctxsw_ucode(struct gk20a *g);
int nvgpu_gr_falcon_load_secure_ctxsw_ucode(struct gk20a *g);
int nvgpu_gr_falcon_disable_ctxsw(struct gk20a *g);
int nvgpu_gr_falcon_enable_ctxsw(struct gk20a *g);
int nvgpu_gr_falcon_halt_pipe(struct gk20a *g);

#endif /* NVGPU_GR_FALCON_H */
