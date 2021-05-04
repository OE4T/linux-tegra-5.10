/*
 * Copyright (c) 2020-2021, NVIDIA CORPORATION.  All rights reserved.
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

#include <common/falcon/falcon_sw_ga10b.h>
#include <common/acr/acr_sw_ga10b.h>
#include <common/acr/acr_sw_ga100.h>

#ifndef NVGPU_NEXT_GPUID_H
#define NVGPU_NEXT_GPUID_H

#define NVGPU_NEXT_GPUID	0x0000017b

#define NVGPU_NEXT_FECS_UCODE_SIG "ga10b/fecs_sig.bin"
#define NVGPU_NEXT_GPCCS_UCODE_SIG "ga10b/gpccs_sig.bin"

#define NVGPU_NEXT_INIT_HAL		ga10b_init_hal
#define NVGPU_NEXT_INIT_OS_OPS		nvgpu_ga10b_init_os_ops
#define NVGPU_NEXT_COMPATIBLE		"nvidia,ga10b"
#define NVGPU_NEXT_PLATFORM		ga10b_tegra_platform

#define NVGPU_NEXT_COMPATIBLE_VGPU "nvidia,ga10b-vgpu"
#define NVGPU_NEXT_VGPU_INIT_HAL vgpu_ga10b_init_hal
#define NVGPU_NEXT_PLATFORM_VGPU ga10b_vgpu_tegra_platform

#define NVGPU_NEXT_PROFILER_QUIESCE	nvgpu_next_profiler_hs_stream_quiesce
#ifdef CONFIG_NVGPU_DGPU
#define NVGPU_NEXT_DGPU_GPUID		0x00000170
#define NVGPU_NEXT_DGPU_INIT_HAL	ga100_init_hal
#define NEXT_DGPU_FECS_UCODE_SIG	"ga100/fecs_sig.bin"
#define NEXT_DGPU_GPCCS_UCODE_SIG	"ga100/gpccs_sig.bin"

extern int ga100_init_hal(struct gk20a *g);
#endif

struct nvgpu_os_linux;

extern int ga10b_init_hal(struct gk20a *g);

extern struct gk20a_platform ga10b_tegra_platform;

extern void nvgpu_next_perfmon_sw_init(struct gk20a *g,
		struct nvgpu_pmu_perfmon *perfmon);

extern void nvgpu_next_pg_sw_init(struct gk20a *g, struct nvgpu_pmu_pg *pg);

int vgpu_ga10b_init_hal(struct gk20a *g);
extern struct gk20a_platform ga10b_vgpu_tegra_platform;

extern void nvgpu_next_profiler_hs_stream_quiesce(struct gk20a *g);

#endif
