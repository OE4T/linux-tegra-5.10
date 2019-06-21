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
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <unit/io.h>
#include <unit/unit.h>

#include <nvgpu/channel.h>
#include <nvgpu/tsg.h>
#include <nvgpu/mm.h>
#include <nvgpu/fifo/userd.h>
#include <nvgpu/gk20a.h>

#include "hal/init/hal_gv11b.h"

#include "nvgpu-fifo.h"
#include "nvgpu-fifo-gv11b.h"

static struct unit_module *global_m;

bool test_fifo_subtest_pruned(u32 branches, u32 final_branches)
{
	u32 match = branches & final_branches;
	int bit;

	if (match == 0U) {
		return false;
	}
	bit = ffs(match) - 1;

	return (branches > BIT(bit));
}

static int test_fifo_flags_strn(char *dst, size_t size,
		const char *labels[], u32 flags)
{
	int i;
	char *p = dst;
	int len;

	for (i = 0; i < 32; i++) {
		if (flags & BIT(i)) {
			len = snprintf(p, size, "%s ", labels[i]);
			size -= len;
			p += len;
		}
	}

	return (p - dst);
}

/* not MT-safe */
char *test_fifo_flags_str(u32 flags, const char *labels[])
{
	static char buf[256];

	memset(buf, 0, sizeof(buf));
	test_fifo_flags_strn(buf, sizeof(buf), labels, flags);
	return buf;
}

static u32 stub_gv11b_gr_init_get_no_of_sm(struct gk20a *g)
{
	return 8;
}

static int stub_userd_setup_sw(struct gk20a *g)
{
	struct nvgpu_fifo *f = &g->fifo;
	int err;

	f->userd_entry_size = g->ops.userd.entry_size(g);

	err = nvgpu_userd_init_slabs(g);
	if (err != 0) {
		unit_err(global_m, "failed to init userd support");
		return err;
	}

	return 0;
}

int test_fifo_init_support(struct unit_module *m, struct gk20a *g, void *args)
{
	int err;

	err = test_fifo_setup_gv11b_reg_space(m, g);
	if (err != 0) {
		goto fail;
	}

	gv11b_init_hal(g);
	g->ops.fifo.init_fifo_setup_hw = NULL;
	g->ops.gr.init.get_no_of_sm = stub_gv11b_gr_init_get_no_of_sm;
	g->ops.tsg.init_eng_method_buffers = NULL;

	global_m = m;
	/*
	 * Regular USERD init requires bar1.vm to be initialized
	 * Use a stub in unit tests, since it will be disabled in
	 * safety build anyway.
	 */
	g->ops.userd.setup_sw = stub_userd_setup_sw;

	err = nvgpu_fifo_init_support(g);
	if (err != 0) {
		test_fifo_cleanup_gv11b_reg_space(m, g);
		goto fail;
	}

	/* Do not allocate from vidmem */
	nvgpu_set_enabled(g, NVGPU_MM_UNIFIED_MEMORY, true);

	return UNIT_SUCCESS;

fail:
	return UNIT_FAIL;
}

int test_fifo_remove_support(struct unit_module *m,
		struct gk20a *g, void *args)
{
	if (g->fifo.remove_support) {
		g->fifo.remove_support(&g->fifo);
	}

	return UNIT_SUCCESS;
}

