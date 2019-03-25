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

#include <nvgpu/gk20a.h>
#include <nvgpu/io.h>
#include <nvgpu/debug.h>

#include "gr_falcon_gm20b.h"

#include <nvgpu/hw/gm20b/hw_gr_gm20b.h>

u32 gm20b_gr_falcon_fecs_base_addr(void)
{
	return gr_fecs_irqsset_r();
}

u32 gm20b_gr_falcon_gpccs_base_addr(void)
{
	return gr_gpcs_gpccs_irqsset_r();
}

void gm20b_gr_falcon_fecs_dump_stats(struct gk20a *g)
{
	unsigned int i;

	nvgpu_falcon_dump_stats(&g->fecs_flcn);

	for (i = 0; i < g->ops.gr.falcon.fecs_ctxsw_mailbox_size(); i++) {
		nvgpu_err(g, "gr_fecs_ctxsw_mailbox_r(%d) : 0x%x",
			i, nvgpu_readl(g, gr_fecs_ctxsw_mailbox_r(i)));
	}
}

u32 gm20b_gr_falcon_get_fecs_ctx_state_store_major_rev_id(struct gk20a *g)
{
	return nvgpu_readl(g, gr_fecs_ctx_state_store_major_rev_id_r());
}

u32 gm20b_gr_falcon_get_fecs_ctxsw_mailbox_size(void)
{
	return gr_fecs_ctxsw_mailbox__size_1_v();
}
