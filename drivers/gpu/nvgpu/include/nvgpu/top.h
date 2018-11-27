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

#ifndef NVGPU_TOP_H
#define NVGPU_TOP_H

#include <nvgpu/types.h>

struct gk20a;

#define NVGPU_ENGINE_GRAPHICS		0U
#define NVGPU_ENGINE_COPY0		1U
#define NVGPU_ENGINE_COPY1		2U
#define NVGPU_ENGINE_COPY2		3U
#define NVGPU_ENGINE_IOCTRL		18U
#define NVGPU_ENGINE_LCE		19U

struct nvgpu_device_info {
	u32 engine_type;
	u32 inst_id;
	u32 pri_base;
	u32 fault_id;
	u32 engine_id;
	u32 runlist_id;
	u32 intr_id;
	u32 reset_id;
};

#endif /* NVGPU_TOP_H */
