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

#ifndef INTR_AND_ERR_HANDLING_GV100_H
#define INTR_AND_ERR_HANDLING_GV100_H

#include <nvgpu/types.h>
struct gk20a;

void gv100_nvlink_minion_clear_interrupts(struct gk20a *g);
void gv100_nvlink_init_minion_intr(struct gk20a *g);
bool gv100_nvlink_minion_falcon_isr(struct gk20a *g);
void gv100_nvlink_common_intr_enable(struct gk20a *g, unsigned long mask);
void gv100_nvlink_init_nvlipt_intr(struct gk20a *g, u32 link_id);
void gv100_nvlink_enable_link_intr(struct gk20a *g, u32 link_id, bool enable);
void gv100_nvlink_init_mif_intr(struct gk20a *g, u32 link_id);
void gv100_nvlink_mif_intr_enable(struct gk20a *g, u32 link_id, bool enable);
void gv100_nvlink_dlpl_intr_enable(struct gk20a *g, u32 link_id, bool enable);
int gv100_nvlink_isr(struct gk20a *g);

#endif /* INTR_AND_ERR_HANDLING_GV100_H */
