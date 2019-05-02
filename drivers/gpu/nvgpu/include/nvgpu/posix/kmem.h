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

#ifndef NVGPU_POSIX_KMEM_H
#define NVGPU_POSIX_KMEM_H

#include <nvgpu/types.h>

void *nvgpu_kmalloc_impl(struct gk20a *g, size_t size, void *ip);
void *nvgpu_kzalloc_impl(struct gk20a *g, size_t size, void *ip);
void *nvgpu_kcalloc_impl(struct gk20a *g, size_t n, size_t size, void *ip);
void *nvgpu_vmalloc_impl(struct gk20a *g, unsigned long size, void *ip);
void *nvgpu_vzalloc_impl(struct gk20a *g, unsigned long size, void *ip);
void nvgpu_kfree_impl(struct gk20a *g, void *addr);
void nvgpu_vfree_impl(struct gk20a *g, void *addr);

struct nvgpu_posix_fault_inj *nvgpu_kmem_get_fault_injection(void);

#endif /* NVGPU_POSIX_KMEM_H */
