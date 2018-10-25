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

#include <unit/io.h>
#include <unit/unit.h>
#include <nvgpu/posix/io.h>

#include <nvgpu/gk20a.h>
#include <nvgpu/hal_init.h>
#include <nvgpu/enabled.h>

#include "nvgpu-fuse-priv.h"
#include "nvgpu-fuse-gp10b.h"

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

/*
 * Overrides for the fuse functionality
 */

u32 gcplex_config;

/* Return pass and value for reading gcplex */
int read_gcplex_config_fuse_pass(struct gk20a *g, u32 *val)
{
	*val = gcplex_config;
	return 0;
}

/* Return fail for reading gcplex */
int read_gcplex_config_fuse_fail(struct gk20a *g, u32 *val)
{
	return -ENODEV;
}

/*
 * Initialization for this unit test.
 *   Setup g struct
 *   Setup fuse ops
 *   Setup mock I/O
 */
static int test_fuse_device_common_init(struct unit_module *m,
					struct gk20a *g, void *__args)
{
	int ret = UNIT_SUCCESS;
	int result;
	struct fuse_test_args *args = (struct fuse_test_args *)__args;

	/* Create fuse register space */
	nvgpu_posix_io_init_reg_space(g);
	if (nvgpu_posix_io_add_reg_space(g, args->fuse_base_addr, 0xfff) != 0) {
		unit_err(m, "%s: failed to create register space\n",
			 __func__);
		return UNIT_FAIL;
	}

	(void)nvgpu_posix_register_io(g, &test_reg_callbacks);

	g->params.gpu_arch = args->gpu_arch << NVGPU_GPU_ARCHITECTURE_SHIFT;
	g->params.gpu_impl = args->gpu_impl;

	nvgpu_posix_io_writel_reg_space(g, args->sec_fuse_addr, 0x0);

	result = nvgpu_init_hal(g);
	if (result != 0) {
		unit_err(m, "%s: nvgpu_init_hal returned error %d\n",
			__func__, result);
		ret = UNIT_FAIL;
	}

	g->ops.fuse.read_gcplex_config_fuse = read_gcplex_config_fuse_pass;

	return ret;
}

static int test_fuse_device_common_cleanup(struct unit_module *m,
				    struct gk20a *g, void *__args)
{
	struct fuse_test_args *args = (struct fuse_test_args *)__args;

	nvgpu_posix_io_delete_reg_space(g, args->fuse_base_addr);

	return 0;
}

struct unit_module_test fuse_tests[] = {
	UNIT_TEST(fuse_gp10b_init, test_fuse_device_common_init,
		  &gp10b_init_args),
	UNIT_TEST(fuse_gp10b_check_sec, test_fuse_gp10b_check_sec, NULL),
	UNIT_TEST(fuse_gp10b_check_gcplex_fail,
		  test_fuse_gp10b_check_gcplex_fail,
		  NULL),
	UNIT_TEST(fuse_gp10b_check_sec_invalid_gcplex,
		  test_fuse_gp10b_check_sec_invalid_gcplex,
		  NULL),
	UNIT_TEST(fuse_gp10b_check_non_sec,
		  test_fuse_gp10b_check_non_sec,
		  NULL),
	UNIT_TEST(fuse_gp10b_ecc, test_fuse_gp10b_ecc, NULL),
	UNIT_TEST(fuse_gp10b_feature_override_disable,
		  test_fuse_gp10b_feature_override_disable, NULL),
	UNIT_TEST(fuse_gp10b_check_fmodel, test_fuse_gp10b_check_fmodel, NULL),
	UNIT_TEST(fuse_gp10b_cleanup, test_fuse_device_common_cleanup,
		  &gp10b_init_args),
};

UNIT_MODULE(fuse, fuse_tests, UNIT_PRIO_NVGPU_TEST);
