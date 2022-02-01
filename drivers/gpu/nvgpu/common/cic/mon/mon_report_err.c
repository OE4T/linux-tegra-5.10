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

#include <nvgpu/gk20a.h>
#include <nvgpu/nvgpu_init.h>
#include <nvgpu/nvgpu_err.h>
#include <nvgpu/nvgpu_err_info.h>
#include <nvgpu/cic_mon.h>

#include "cic_mon_priv.h"

void nvgpu_report_err_to_sdl(struct gk20a *g, u32 err_id)
{
	if (g->ops.cic_mon.report_err == NULL) {
		return;
	}

	if (g->ops.cic_mon.report_err(g, err_id) != 0) {
		nvgpu_err(g, "Failed to report an error: err_id=%x",
				err_id);
		nvgpu_sw_quiesce(g);
	}
}
