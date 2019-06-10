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

#ifndef NVGPU_INIT_H
#define NVGPU_INIT_H

int gk20a_finalize_poweron(struct gk20a *g);
int gk20a_prepare_poweroff(struct gk20a *g);

int nvgpu_can_busy(struct gk20a *g);
int gk20a_wait_for_idle(struct gk20a *g);

struct gk20a * __must_check gk20a_get(struct gk20a *g);
void gk20a_put(struct gk20a *g);

/* register accessors */
void nvgpu_check_gpu_state(struct gk20a *g);
void gk20a_warn_on_no_regs(void);

void gk20a_init_gpu_characteristics(struct gk20a *g);

#endif /* NVGPU_INIT_H */
