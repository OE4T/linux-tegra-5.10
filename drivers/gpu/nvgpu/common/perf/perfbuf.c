/*
 * Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/mm.h>
#include <nvgpu/sizes.h>
#include <nvgpu/perfbuf.h>
#include <nvgpu/gk20a.h>

int nvgpu_perfbuf_enable_locked(struct gk20a *g, u64 offset, u32 size)
{
	struct mm_gk20a *mm = &g->mm;
	int err;

	err = gk20a_busy(g);
	if (err != 0) {
		nvgpu_err(g, "failed to poweron");
		return err;
	}

	err = nvgpu_alloc_inst_block(g, &mm->perfbuf.inst_block);
	if (err != 0) {
		return err;
	}

	g->ops.mm.init_inst_block(&mm->perfbuf.inst_block, mm->perfbuf.vm, 0);

	g->ops.perf.membuf_reset_streaming(g);
	g->ops.perf.enable_membuf(g, size, offset, &mm->perfbuf.inst_block);

	gk20a_idle(g);

	return 0;
}

int nvgpu_perfbuf_disable_locked(struct gk20a *g)
{
	int err = gk20a_busy(g);
	if (err != 0) {
		nvgpu_err(g, "failed to poweron");
		return err;
	}

	g->ops.perf.membuf_reset_streaming(g);
	g->ops.perf.disable_membuf(g);

	gk20a_idle(g);

	return 0;
}
