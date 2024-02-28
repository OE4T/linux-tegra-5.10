/*
 * Copyright (c) 2022, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_IVM_H
#define NVGPU_IVM_H

#include <nvgpu/types.h>

struct tegra_hv_ivm_cookie;

struct tegra_hv_ivm_cookie *nvgpu_ivm_mempool_reserve(unsigned int id);
int nvgpu_ivm_mempool_unreserve(struct tegra_hv_ivm_cookie *cookie);
u64 nvgpu_ivm_get_ipa(struct tegra_hv_ivm_cookie *cookie);
u64 nvgpu_ivm_get_size(struct tegra_hv_ivm_cookie *cookie);
void *nvgpu_ivm_mempool_map(struct tegra_hv_ivm_cookie *cookie);
void nvgpu_ivm_mempool_unmap(struct tegra_hv_ivm_cookie *cookie,
		void *addr);
#endif /* NVGPU_VGPU_IVM_H */
