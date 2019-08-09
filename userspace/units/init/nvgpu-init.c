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

#include <unit/unit.h>
#include <unit/io.h>
#include <nvgpu/posix/io.h>

#include <nvgpu/gk20a.h>
#include <nvgpu/nvgpu_init.h>
#include <nvgpu/hal_init.h>
#include <nvgpu/enabled.h>
#include <nvgpu/hw/gm20b/hw_mc_gm20b.h>

#include "nvgpu-init.h"

/* value for GV11B */
#define MC_BOOT_0_GV11B ((0x15 << 24) | (0xB << 20))
/* to set the security fuses */
#define GP10B_FUSE_REG_BASE		0x00021000U
#define GP10B_FUSE_OPT_PRIV_SEC_EN	(GP10B_FUSE_REG_BASE+0x434U)

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

static struct nvgpu_posix_io_callbacks test_reg_callbacks = {
	/* Write APIs all can use the same accessor. */
	.writel          = writel_access_reg_fn,
	.writel_check    = writel_access_reg_fn,
	.bar1_writel     = writel_access_reg_fn,
	.usermode_writel = writel_access_reg_fn,

	/* Likewise for the read APIs. */
	.__readl         = readl_access_reg_fn,
	.readl           = readl_access_reg_fn,
	.bar1_readl      = readl_access_reg_fn,
};

/* generic replacement functions that can be assigned to function pointers */
static void no_return(struct gk20a *g)
{
	/* noop */
}

static int test_setup_env(struct unit_module *m,
			  struct gk20a *g, void *args)
{
	/* Create mc register space */
	nvgpu_posix_io_init_reg_space(g);
	if (nvgpu_posix_io_add_reg_space(g, mc_boot_0_r(), 0xfff) != 0) {
		unit_err(m, "%s: failed to create register space\n",
			 __func__);
		return UNIT_FAIL;
	}
	/* Create fuse register space */
	if (nvgpu_posix_io_add_reg_space(g, GP10B_FUSE_REG_BASE, 0xfff) != 0) {
		unit_err(m, "%s: failed to create register space\n",
			 __func__);
		return UNIT_FAIL;
	}
	(void)nvgpu_posix_register_io(g, &test_reg_callbacks);

	return UNIT_SUCCESS;
}

static int test_free_env(struct unit_module *m,
			 struct gk20a *g, void *args)
{
	/* Free mc register space */
	nvgpu_posix_io_delete_reg_space(g, mc_boot_0_r());
	nvgpu_posix_io_delete_reg_space(g, GP10B_FUSE_REG_BASE);

	return UNIT_SUCCESS;
}

static int test_can_busy(struct unit_module *m,
			 struct gk20a *g, void *args)
{
	int ret = UNIT_SUCCESS;

	nvgpu_set_enabled(g, NVGPU_KERNEL_IS_DYING, false);
	nvgpu_set_enabled(g, NVGPU_DRIVER_IS_DYING, false);
	if (nvgpu_can_busy(g) != 1) {
		ret = UNIT_FAIL;
		unit_err(m, "nvgpu_can_busy() returned 0\n");
	}

	nvgpu_set_enabled(g, NVGPU_KERNEL_IS_DYING, true);
	nvgpu_set_enabled(g, NVGPU_DRIVER_IS_DYING, false);
	if (nvgpu_can_busy(g) != 0) {
		ret = UNIT_FAIL;
		unit_err(m, "nvgpu_can_busy() returned 1\n");
	}

	nvgpu_set_enabled(g, NVGPU_KERNEL_IS_DYING, false);
	nvgpu_set_enabled(g, NVGPU_DRIVER_IS_DYING, true);
	if (nvgpu_can_busy(g) != 0) {
		ret = UNIT_FAIL;
		unit_err(m, "nvgpu_can_busy() returned 1\n");
	}

	nvgpu_set_enabled(g, NVGPU_KERNEL_IS_DYING, true);
	nvgpu_set_enabled(g, NVGPU_DRIVER_IS_DYING, true);
	if (nvgpu_can_busy(g) != 0) {
		ret = UNIT_FAIL;
		unit_err(m, "nvgpu_can_busy() returned 1\n");
	}

	return ret;
}

static int test_get_put(struct unit_module *m,
			struct gk20a *g, void *args)
{
	int ret = UNIT_SUCCESS;

	nvgpu_ref_init(&g->refcount);

	if (g != nvgpu_get(g)) {
		ret = UNIT_FAIL;
		unit_err(m, "nvgpu_get() returned NULL\n");
	}
	if (nvgpu_atomic_read(&g->refcount.refcount) != 2) {
		ret = UNIT_FAIL;
		unit_err(m, "nvgpu_get() did not increment refcount\n");
	}

	nvgpu_put(g);
	if (nvgpu_atomic_read(&g->refcount.refcount) != 1) {
		ret = UNIT_FAIL;
		unit_err(m, "nvgpu_put() did not decrement refcount\n");
	}

	/* one more to get to 0 to teardown */
	nvgpu_put(g);
	if (nvgpu_atomic_read(&g->refcount.refcount) != 0) {
		ret = UNIT_FAIL;
		unit_err(m, "nvgpu_put() did not decrement refcount\n");
	}

	/* This is expected to fail */
	if (nvgpu_get(g) != NULL) {
		ret = UNIT_FAIL;
		unit_err(m, "nvgpu_get() did not return NULL\n");
	}
	if (nvgpu_atomic_read(&g->refcount.refcount) != 0) {
		ret = UNIT_FAIL;
		unit_err(m, "nvgpu_get() did not increment refcount\n");
	}

	/* start over */
	nvgpu_ref_init(&g->refcount);

	/* to cover the cases where these are set */
	g->remove_support = no_return;
	g->gfree = no_return;

	if (g != nvgpu_get(g)) {
		ret = UNIT_FAIL;
		unit_err(m, "nvgpu_get() returned NULL\n");
	}
	if (nvgpu_atomic_read(&g->refcount.refcount) != 2) {
		ret = UNIT_FAIL;
		unit_err(m, "nvgpu_get() did not increment refcount\n");
	}

	nvgpu_put(g);
	if (nvgpu_atomic_read(&g->refcount.refcount) != 1) {
		ret = UNIT_FAIL;
		unit_err(m, "nvgpu_put() did not decrement refcount\n");
	}

	/* one more to get to 0 to teardown */
	nvgpu_put(g);
	if (nvgpu_atomic_read(&g->refcount.refcount) != 0) {
		ret = UNIT_FAIL;
		unit_err(m, "nvgpu_put() did not decrement refcount\n");
	}

	return ret;
}

static int test_check_gpu_state(struct unit_module *m,
				struct gk20a *g, void *args)
{
	/* Valid state */
	nvgpu_posix_io_writel_reg_space(g, mc_boot_0_r(), MC_BOOT_0_GV11B);
	nvgpu_check_gpu_state(g);

	/*
	 * Test INVALID state. This should cause a kernel_restart() which
	 * is a BUG() in posix, so verify we hit the BUG().
	 */
	nvgpu_posix_io_writel_reg_space(g, mc_boot_0_r(), U32_MAX);
	if (!EXPECT_BUG(nvgpu_check_gpu_state(g))) {
		unit_err(m, "%s: failed to detect INVALID state\n",
			 __func__);
		return UNIT_FAIL;
	}

	return UNIT_SUCCESS;
}

static int test_hal_init(struct unit_module *m,
			 struct gk20a *g, void *args)
{
	nvgpu_posix_io_writel_reg_space(g, mc_boot_0_r(), MC_BOOT_0_GV11B);
	nvgpu_posix_io_writel_reg_space(g, GP10B_FUSE_OPT_PRIV_SEC_EN, 0x0);
	if (nvgpu_detect_chip(g) != 0) {
		unit_err(m, "%s: failed to init HAL\n", __func__);
		return UNIT_FAIL;
	}

	return UNIT_SUCCESS;
}

struct unit_module_test init_tests[] = {
	UNIT_TEST(init_setup_env,			test_setup_env,		NULL, 0),
	UNIT_TEST(init_can_busy,			test_can_busy,		NULL, 0),
	UNIT_TEST(init_get_put,				test_get_put,		NULL, 0),
	UNIT_TEST(init_check_gpu_state,			test_check_gpu_state,	NULL, 0),
	UNIT_TEST(init_hal_init,			test_hal_init,		NULL, 0),
	UNIT_TEST(init_free_env,			test_free_env,		NULL, 0),
};

UNIT_MODULE(init, init_tests, UNIT_PRIO_NVGPU_TEST);
