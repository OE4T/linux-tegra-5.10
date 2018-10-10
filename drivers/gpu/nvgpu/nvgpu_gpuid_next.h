/*
 * Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_GPUID_NEXT_H
#define NVGPU_GPUID_NEXT_H

#define NVGPU_GPUID_NEXT	0x00000164

#define NVGPU_GPU_NEXT_FECS_UCODE_SIG "tu104/fecs_sig.bin"
#define NVGPU_GPU_NEXT_GPCCS_UCODE_SIG "tu104/gpccs_sig.bin"

#define NVGPU_NEXT_INIT_HAL	tu104_init_hal
#define NVGPU_NEXT_INIT_OS_OPS	nvgpu_tu104_init_os_ops

struct nvgpu_os_linux;

extern int tu104_init_hal(struct gk20a *g);
extern void nvgpu_tu104_init_os_ops(struct nvgpu_os_linux *l);

#endif /* NVGPU_GPUID_NEXT_H */
