/*
 * Copyright (c) 2020, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_NEXT_PBDMA_H
#define NVGPU_NEXT_PBDMA_H

/**
 * @file
 *
 * Declare pbdma specific struct and defines.
 */
#include <nvgpu/types.h>

#define PBDMA_PER_RUNLIST_SIZE		2U
#define NVGPU_INVALID_PBDMA_PRI_BASE	U32_MAX
#define NVGPU_INVALID_PBDMA_ID		U32_MAX

struct nvgpu_next_pbdma_info {
	/** The pri offset of the i'th PBDMA for runlist_pri_base */
	u32 pbdma_pri_base[PBDMA_PER_RUNLIST_SIZE];
	/** The ID of the i'th PBDMA that runs channels on this runlist */
	u32 pbdma_id[PBDMA_PER_RUNLIST_SIZE];
};

#endif /* NVGPU_NEXT_PBDMA_H */
