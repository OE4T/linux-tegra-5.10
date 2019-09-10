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
#ifndef UNIT_NVGPU_GR_H
#define UNIT_NVGPU_GR_H

#include <nvgpu/types.h>

struct gk20a;
struct unit_module;

/**
 * Allocate and add needed register spaces
 * Initialize gv11b hal
 * Allocate memory for gr unit
 */
int test_gr_init_setup(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Delete the memory for gr unit
 * Delete and remove the register spaces
 */
int test_gr_remove_setup(struct unit_module *m, struct gk20a *g, void *args);

/**
 * call init_prepare functions to GR driver for
 * nvgpu_gr_prepare_sw and nvgpu_gr_enable_hw
 */
int test_gr_init_prepare(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Override falcon.load_ctxsw_ucode hal and call
 * nvgpu_gr_falcon_init_ctxsw_ucode.
 * Initialize ltc and mm units
 * Call nvgpu_gr_init_support driver function
 */
int test_gr_init_support(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Support nvgpu_gr_suspend driver function
 */
int test_gr_suspend(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Support nvgpu_gr_remove_support driver function
 */
int test_gr_remove_support(struct unit_module *m, struct gk20a *g, void *args);
#endif /* UNIT_NVGPU_GR_H */
