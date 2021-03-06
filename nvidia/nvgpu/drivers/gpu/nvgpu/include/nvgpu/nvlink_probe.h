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

#ifndef NVGPU_NVLINK_PROBE_H
#define NVGPU_NVLINK_PROBE_H

struct gk20a;

int nvgpu_nvlink_setup_ndev(struct gk20a *g);
int nvgpu_nvlink_init_ops(struct gk20a *g);
int nvgpu_nvlink_read_dt_props(struct gk20a *g);
int nvgpu_nvlink_probe(struct gk20a *g);
int nvgpu_nvlink_register_device(struct gk20a *g);
int nvgpu_nvlink_unregister_device(struct gk20a *g);
int nvgpu_nvlink_register_link(struct gk20a *g);
int nvgpu_nvlink_unregister_link(struct gk20a *g);

#endif /* NVGPU_NVLINK_PROBE_H */
