/*
 * Copyright (c) 2020, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/netlist.h>
#include <nvgpu/io.h>

#include <nvgpu/gr/nvgpu_next_gr.h>

void nvgpu_next_gr_init_reset_enable_hw_non_ctx_local(struct gk20a *g)
{
	u32 i = 0U;
	struct netlist_av_list *sw_non_ctx_local_compute_load =
		nvgpu_next_netlist_get_sw_non_ctx_local_compute_load_av_list(g);
#ifdef CONFIG_NVGPU_GRAPHICS
	struct netlist_av_list *sw_non_ctx_local_gfx_load =
		nvgpu_next_netlist_get_sw_non_ctx_local_gfx_load_av_list(g);
#endif

	for (i = 0U; i < sw_non_ctx_local_compute_load->count; i++) {
		nvgpu_writel(g, sw_non_ctx_local_compute_load->l[i].addr,
			sw_non_ctx_local_compute_load->l[i].value);
	}

#ifdef CONFIG_NVGPU_GRAPHICS
	if (!nvgpu_is_enabled(g, NVGPU_SUPPORT_MIG)) {
		for (i = 0U; i < sw_non_ctx_local_gfx_load->count; i++) {
			nvgpu_writel(g, sw_non_ctx_local_gfx_load->l[i].addr,
				sw_non_ctx_local_gfx_load->l[i].value);
		}
	}
#endif

	return;
}

void nvgpu_next_gr_init_reset_enable_hw_non_ctx_global(struct gk20a *g)
{
	u32 i = 0U;
	struct netlist_av_list *sw_non_ctx_global_compute_load =
		nvgpu_next_netlist_get_sw_non_ctx_global_compute_load_av_list(g);
#ifdef CONFIG_NVGPU_GRAPHICS
	struct netlist_av_list *sw_non_ctx_global_gfx_load =
		nvgpu_next_netlist_get_sw_non_ctx_global_gfx_load_av_list(g);
#endif

	for (i = 0U; i < sw_non_ctx_global_compute_load->count; i++) {
		nvgpu_writel(g, sw_non_ctx_global_compute_load->l[i].addr,
			sw_non_ctx_global_compute_load->l[i].value);
	}

#ifdef CONFIG_NVGPU_GRAPHICS
	if (!nvgpu_is_enabled(g, NVGPU_SUPPORT_MIG)) {
		for (i = 0U; i < sw_non_ctx_global_gfx_load->count; i++) {
			nvgpu_writel(g, sw_non_ctx_global_gfx_load->l[i].addr,
				sw_non_ctx_global_gfx_load->l[i].value);
		}
	}
#endif

	return;
}
