/*
 * Copyright (c) 2017-2019, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_LTC_H
#define NVGPU_LTC_H

#include <nvgpu/types.h>
#include <nvgpu/lock.h>

struct gk20a;

struct nvgpu_ltc {
	struct nvgpu_spinlock ltc_enabled_lock;
	u32 max_ltc_count;
	u32 ltc_count;
	u32 slices_per_ltc;
	u32 cacheline_size;
};

void nvgpu_ltc_remove_support(struct gk20a *g);
int nvgpu_init_ltc_support(struct gk20a *g);
void nvgpu_ltc_sync_enabled(struct gk20a *g);
u32 nvgpu_ltc_get_ltc_count(struct gk20a *g);
u32 nvgpu_ltc_get_slices_per_ltc(struct gk20a *g);
u32 nvgpu_ltc_get_cacheline_size(struct gk20a *g);

#endif /* NVGPU_LTC_H */
