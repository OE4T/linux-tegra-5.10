/*
 * Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
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
#ifndef NVGPU_NEXT_GOPS_FB_VAB_H
#define NVGPU_NEXT_GOPS_FB_VAB_H

struct nvgpu_vab_range_checker;

struct gops_fb_vab {
	/**
	 * @brief Initialize VAB
	 *
	 */
	int (*init)(struct gk20a *g);

	/**
	 * @brief Initialize VAB range checkers and enable VAB tracking
	 *
	 */
	int (*reserve)(struct gk20a *g, u32 vab_mode, u32 num_range_checkers,
		struct nvgpu_vab_range_checker *vab_range_checker);

	/**
	 * @brief Trigger VAB dump, copy buffer to user and clear
	 *
	 */
	int (*dump_and_clear)(struct gk20a *g, u64 *user_buf,
		u64 user_buf_size);

	/**
	 * @brief Disable VAB
	 *
	 */
	int (*release)(struct gk20a *g);

	/**
	 * @brief Free VAB resources
	 *
	 */
	int (*teardown)(struct gk20a *g);
};

#endif /* NVGPU_NEXT_GOPS_FB_VAB_H */
