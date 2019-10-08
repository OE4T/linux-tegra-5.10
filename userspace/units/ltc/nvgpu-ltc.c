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

#include <nvgpu/gk20a.h>
#include <nvgpu/posix/io.h>
#include <nvgpu/hal_init.h>
#include <nvgpu/enabled.h>
#include <nvgpu/hw/gm20b/hw_mc_gm20b.h>
#include <nvgpu/ltc.h>
#include <nvgpu/nvgpu_mem.h>

#include "nvgpu-ltc.h"

#define NV_PMC_BOOT_0_ARCHITECTURE_GV110	(0x00000015 << \
					NVGPU_GPU_ARCHITECTURE_SHIFT)
#define NV_PMC_BOOT_0_IMPLEMENTATION_B		0xB

/*
 * Mock I/O
 */

/*
 * Write callback. Forward the write access to the mock IO framework.
 */
static void writel_access_reg_fn(struct gk20a *g,
					struct nvgpu_reg_access *access)
{
	nvgpu_posix_io_writel_reg_space(g, access->addr, access->value);
}

/*
 * Read callback. Get the register value from the mock IO framework.
 */
static void readl_access_reg_fn(struct gk20a *g,
					struct nvgpu_reg_access *access)
{
	access->value = nvgpu_posix_io_readl_reg_space(g, access->addr);
}

static struct nvgpu_posix_io_callbacks netlist_test_reg_callbacks = {
	.writel          = writel_access_reg_fn,
	.writel_check    = writel_access_reg_fn,
	.bar1_writel     = writel_access_reg_fn,
	.usermode_writel = writel_access_reg_fn,
	.__readl         = readl_access_reg_fn,
	.readl           = readl_access_reg_fn,
	.bar1_readl      = readl_access_reg_fn,
};


int test_ltc_init_support(struct unit_module *m,
						struct gk20a *g, void *args)
{

	int err = 0;

	nvgpu_posix_io_init_reg_space(g);
	if (nvgpu_posix_io_add_reg_space(g, mc_boot_0_r(), 0xfff) != 0) {
		unit_err(m, "%s: failed to create register space\n", __func__);
		return UNIT_FAIL;
	}

	(void)nvgpu_posix_register_io(g, &netlist_test_reg_callbacks);

	/*
	 * HAL init parameters for gv11b
	*/
	g->params.gpu_arch = NV_PMC_BOOT_0_ARCHITECTURE_GV110;
	g->params.gpu_impl = NV_PMC_BOOT_0_IMPLEMENTATION_B;

	/*
	 * HAL init required for getting
	 * the falcon ops initialized.
	 */
	err = nvgpu_init_hal(g);
	if (err != 0) {
		unit_return_fail(m, "nvgpu_init_hal failed\n");
	}

	err = nvgpu_init_ltc_support(g);
	if (err != 0) {
		unit_return_fail(m, "nvgpu_init_ltc_support failed\n");
	}

	return UNIT_SUCCESS;

}

int test_ltc_functionality_tests(struct unit_module *m,
		struct gk20a *g, void *args)
{
	u32 ltc_count;
	u32 slice_per_ltc;
	u32 cacheline_size;

	g->mm.ltc_enabled_current = false;
	nvgpu_ltc_sync_enabled(g);

	ltc_count = nvgpu_ltc_get_ltc_count(g);
	if (ltc_count != 0) {
		unit_return_fail(m, "nvgpu_ltc_get_ltc_count failed\n");
	}
	slice_per_ltc = nvgpu_ltc_get_slices_per_ltc(g);
	if (slice_per_ltc != 0) {
		unit_return_fail(m, "nvgpu_ltc_get_slices_per_ltc failed\n");
	}
	cacheline_size = nvgpu_ltc_get_cacheline_size(g);
	if (cacheline_size == 0) {
		unit_return_fail(m, "nvgpu_ltc_get_cacheline_size failed\n");
	}

	return UNIT_SUCCESS;
}

int test_ltc_negative_tests(struct unit_module *m,
		struct gk20a *g, void *args)
{
	int err = 0;

	g->ops.ltc.set_enabled = NULL;
	nvgpu_ltc_sync_enabled(g);
	nvgpu_ltc_remove_support(g);
	nvgpu_ltc_remove_support(g);
	err = nvgpu_init_ltc_support(g);
	if (err != 0) {
		unit_return_fail(m, "nvgpu_init_ltc_support failed\n");
	}

	return UNIT_SUCCESS;
}

int test_ltc_remove_support(struct unit_module *m,
		struct gk20a *g, void *args)
{
	nvgpu_ltc_remove_support(g);

	return UNIT_SUCCESS;
}

struct unit_module_test nvgpu_ltc_tests[] = {
	UNIT_TEST(ltc_init_support, test_ltc_init_support, NULL, 0),
	UNIT_TEST(ltc_functionality_tests, test_ltc_functionality_tests,
								NULL, 0),
	UNIT_TEST(ltc_negative_tests, test_ltc_negative_tests, NULL, 0),
	UNIT_TEST(ltc_remove_support, test_ltc_remove_support, NULL, 0),
};

UNIT_MODULE(nvgpu-ltc, nvgpu_ltc_tests, UNIT_PRIO_NVGPU_TEST);


