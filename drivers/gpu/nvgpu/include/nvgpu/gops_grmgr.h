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
#ifndef NVGPU_GOPS_GRMGR_H
#define NVGPU_GOPS_GRMGR_H

#include <nvgpu/types.h>

/**
 * @file
 *
 * GR MANAGER unit HAL interface
 *
 */
struct gk20a;

/**
 * GR MANAGER unit HAL operations
 *
 * @see gpu_ops
 */
struct gops_grmgr {
	/**
	 * @brief Initialize GR Manager unit.
	 *
	 * @param g [in]		Pointer to GPU driver struct.
	 *
	 * @return 0 in case of success, < 0 in case of failure.
	 */
	int (*init_gr_manager)(struct gk20a *g);

	/**
	 * @brief Remove GR Manager unit.
	 *
	 * @param g [in]		Pointer to GPU driver struct.
	 *
	 * @return 0 in case of success, < 0 in case of failure.
	 */
	int (*remove_gr_manager)(struct gk20a *g);

#if defined(CONFIG_NVGPU_NEXT) && defined(CONFIG_NVGPU_MIG)
#include "include/nvgpu/nvgpu_next_gops_grmgr.h"
#endif
};

#endif /* NVGPU_NEXT_GOPS_GRMGR_H */
