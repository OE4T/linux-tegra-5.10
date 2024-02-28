/*
 * GA10B L2 INTR
 *
 * Copyright (c) 2020-2022, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_LTC_INTR_GA10B_H
#define NVGPU_LTC_INTR_GA10B_H

#include <nvgpu/types.h>

struct gk20a;

void ga10b_ltc_intr_configure(struct gk20a *g);
void ga10b_ltc_intr_isr(struct gk20a *g, u32 ltc);
void ga10b_ltc_intr3_configure_extra(struct gk20a *g, u32 *reg);
void ga10b_ltc_intr3_interrupts(struct gk20a *g, u32 ltc, u32 slice,
				u32 ltc_intr3);
void ga10b_ltc_intr_handle_lts_intr(struct gk20a *g, u32 ltc, u32 slice);
void ga10b_ltc_intr_handle_lts_intr2(struct gk20a *g, u32 ltc, u32 slice);
void ga10b_ltc_intr_handle_lts_intr3(struct gk20a *g, u32 ltc, u32 slice);
void ga10b_ltc_intr_handle_lts_intr3_extra(struct gk20a *g, u32 ltc, u32 slice, u32 *reg_value);

#endif /* NVGPU_LTC_INTR_GA10B_H */
