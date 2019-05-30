/*
 * Copyright (c) 2016-2019, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef PMU_GV11B_H
#define PMU_GV11B_H

#include <nvgpu/types.h>

struct gk20a;

bool gv11b_pmu_is_debug_mode_en(struct gk20a *g);
void gv11b_pmu_flcn_setup_boot_config(struct gk20a *g);
void gv11b_setup_apertures(struct gk20a *g);
void gv11b_write_dmatrfbase(struct gk20a *g, u32 addr);
u32 gv11b_pmu_falcon_base_addr(void);
void gv11b_secured_pmu_start(struct gk20a *g);
bool gv11b_is_pmu_supported(struct gk20a *g);
int gv11b_pmu_bootstrap(struct gk20a *g, struct nvgpu_pmu *pmu,
	u32 args_offset);
void gv11b_pmu_setup_elpg(struct gk20a *g);
u32 gv11b_pmu_get_irqdest(struct gk20a *g);
void gv11b_pmu_handle_ext_irq(struct gk20a *g, u32 intr0);
void gv11b_clear_pmu_bar0_host_err_status(struct gk20a *g);
int gv11b_pmu_bar0_error_status(struct gk20a *g, u32 *bar0_status,
	u32 *etype);
bool gv11b_pmu_validate_mem_integrity(struct gk20a *g);

#endif /* PMU_GV11B_H */
