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

#include <unit/unit.h>
#include <unit/io.h>

#include <nvgpu/posix/io.h>
#include <nvgpu/posix/posix-fault-injection.h>

#include <nvgpu/io.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/gr/gr.h>
#include <nvgpu/gr/ctx.h>
#include "common/gr/gr_priv.h"

#include "../nvgpu-gr.h"
#include "nvgpu-gr-init-hal-gv11b.h"

#include <nvgpu/hw/gv11b/hw_gr_gv11b.h>

#define DUMMY_SIZE	0xF0U

static int dummy_l2_flush(struct gk20a *g, bool invalidate)
{
	return 0;
}

int test_gr_init_hal_wait_empty(struct unit_module *m,
		struct gk20a *g, void *args)
{
	int err;
	int i;
	struct nvgpu_posix_fault_inj *timer_fi =
		nvgpu_timers_get_fault_injection();

	/* Fail timeout initialization */
	nvgpu_posix_enable_fault_injection(timer_fi, true, 0);
	err = g->ops.gr.init.wait_empty(g);
	if (err == 0) {
		return UNIT_FAIL;
	}

	nvgpu_posix_enable_fault_injection(timer_fi, false, 0);

	/* gr_status is non-zero, gr_activity are zero, expect failure */
	nvgpu_writel(g, gr_status_r(), BIT32(7));
	nvgpu_writel(g, gr_activity_0_r(), 0);
	nvgpu_writel(g, gr_activity_1_r(), 0);
	nvgpu_writel(g, gr_activity_2_r(), 0);
	nvgpu_writel(g, gr_activity_4_r(), 0);

	err = g->ops.gr.init.wait_empty(g);
	if (err == 0) {
		return UNIT_FAIL;
	}

	/* gr_status is non-zero, gr_activity are non-zero, expect failure */
	nvgpu_writel(g, gr_status_r(), BIT32(7));
	nvgpu_writel(g, gr_activity_0_r(), 0x4);
	nvgpu_writel(g, gr_activity_1_r(), 0x4);
	nvgpu_writel(g, gr_activity_2_r(), 0x4);
	nvgpu_writel(g, gr_activity_4_r(), 0x4);

	err = g->ops.gr.init.wait_empty(g);
	if (err == 0) {
		return UNIT_FAIL;
	}

	/* gr_status is zero, gr_activity are non-zero, expect failure */
	nvgpu_writel(g, gr_status_r(), 0);
	for (i = 1; i < 16; i++) {
		if (i & 0x1) {
			nvgpu_writel(g, gr_activity_0_r(), 0x2);
		} else {
			nvgpu_writel(g, gr_activity_0_r(), 0x104);
		}
		if (i & 0x2) {
			nvgpu_writel(g, gr_activity_1_r(), 0x2);
		} else {
			nvgpu_writel(g, gr_activity_1_r(), 0x104);
		}
		if (i & 0x4) {
			nvgpu_writel(g, gr_activity_2_r(), 0x2);
		} else {
			nvgpu_writel(g, gr_activity_2_r(), 0x0);
		}
		if (i & 0x8) {
			nvgpu_writel(g, gr_activity_4_r(), 0x2);
		} else {
			nvgpu_writel(g, gr_activity_4_r(), 0x104);
		}

		err = g->ops.gr.init.wait_empty(g);
		if (err == 0) {
			return UNIT_FAIL;
		}
	}

	/* Both gr_status and gr_activity registers are zero, expect success */
	nvgpu_writel(g, gr_status_r(), 0);
	nvgpu_writel(g, gr_activity_0_r(), 0);
	nvgpu_writel(g, gr_activity_1_r(), 0);
	nvgpu_writel(g, gr_activity_2_r(), 0);
	nvgpu_writel(g, gr_activity_4_r(), 0);

	err = g->ops.gr.init.wait_empty(g);
	if (err != 0) {
		return UNIT_FAIL;
	}

	return UNIT_SUCCESS;
}

int test_gr_init_hal_error_injection(struct unit_module *m,
		struct gk20a *g, void *args)
{
	int err;
	struct vm_gk20a *vm;
	struct nvgpu_gr_ctx_desc *desc;
	struct nvgpu_gr_ctx *gr_ctx = NULL;
	u32 size;

	g->ops.mm.cache.l2_flush = dummy_l2_flush;

	vm = nvgpu_vm_init(g, SZ_4K, SZ_4K << 10, (1ULL << 32),
			(1ULL << 32) + (1ULL << 37), false, false, false,
			"dummy");
	if (!vm) {
		unit_return_fail(m, "failed to allocate VM");
	}

	/* Setup gr_ctx and patch_ctx */
	desc = nvgpu_gr_ctx_desc_alloc(g);
	if (!desc) {
		unit_return_fail(m, "failed to allocate memory");
	}

	gr_ctx = nvgpu_alloc_gr_ctx_struct(g);
	if (!gr_ctx) {
		unit_return_fail(m, "failed to allocate memory");
	}

	nvgpu_gr_ctx_set_size(desc, NVGPU_GR_CTX_CTX, DUMMY_SIZE);
	err = nvgpu_gr_ctx_alloc(g, gr_ctx, desc, vm);
	if (err != 0) {
		unit_return_fail(m, "failed to allocate context");
	}

	nvgpu_gr_ctx_set_size(desc, NVGPU_GR_CTX_PATCH_CTX, DUMMY_SIZE);
	err = nvgpu_gr_ctx_alloc_patch_ctx(g, gr_ctx, desc, vm);
	if (err != 0) {
		unit_return_fail(m, "failed to allocate patch context");
	}

	/* global_ctx = false and arbitrary size */
	g->ops.gr.init.commit_global_pagepool(g, gr_ctx, 0x12345678,
		DUMMY_SIZE, false, false);

	/* Verify correct size is set */
	size = nvgpu_readl(g, gr_scc_pagepool_r());
	if ((size & 0x3FF) != DUMMY_SIZE) {
		unit_return_fail(m, "expected size not set");
	}

	/* cleanup */
	nvgpu_gr_ctx_free_patch_ctx(g, vm, gr_ctx);
	nvgpu_free_gr_ctx_struct(g, gr_ctx);
	nvgpu_gr_ctx_desc_free(g, desc);
	nvgpu_vm_put(vm);

	return UNIT_SUCCESS;
}
