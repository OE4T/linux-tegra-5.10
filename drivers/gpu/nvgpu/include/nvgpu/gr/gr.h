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

#ifndef NVGPU_GR_H
#define NVGPU_GR_H

#include <nvgpu/types.h>

#define NVGPU_GR_IDLE_CHECK_DEFAULT_US		10U
#define NVGPU_GR_IDLE_CHECK_MAX_US		200U

u32 nvgpu_gr_gpc_offset(struct gk20a *g, u32 gpc);
u32 nvgpu_gr_tpc_offset(struct gk20a *g, u32 tpc);
int nvgpu_gr_suspend(struct gk20a *g);
void nvgpu_gr_flush_channel_tlb(struct gk20a *g);
u32 nvgpu_gr_get_idle_timeout(struct gk20a *g);
int nvgpu_gr_init_fs_state(struct gk20a *g);
void nvgpu_gr_wait_initialized(struct gk20a *g);

#endif /* NVGPU_GR_H */
