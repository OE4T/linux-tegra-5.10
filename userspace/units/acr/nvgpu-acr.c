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
#include <nvgpu/types.h>
#include <nvgpu/acr.h>
#include <nvgpu/falcon.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/pmu.h>
#include <nvgpu/hal_init.h>
#include <nvgpu/posix/io.h>
#include <nvgpu/posix/posix-fault-injection.h>

#include <nvgpu/hw/gv11b/hw_fuse_gv11b.h>

#include "nvgpu-acr.h"
#include "../falcon/falcon_utf.h"

struct utf_falcon *pmu_flcn;

#define NV_PMC_BOOT_0_ARCHITECTURE_GV110        (0x00000015 << \
						NVGPU_GPU_ARCHITECTURE_SHIFT)
#define NV_PMC_BOOT_0_IMPLEMENTATION_B          0xB

#define NV_PMC_BOOT_0_ARCHITECTURE_INVALID	(0x00000018 << \
						NVGPU_GPU_ARCHITECTURE_SHIFT)
#define NV_PMC_BOOT_0_IMPLEMENTATION_INVALID	0xD

static struct utf_falcon *pmu_flcn_from_addr(struct gk20a *g, u32 addr)
{
	struct utf_falcon *flcn = NULL;
	u32 flcn_base;

	if (pmu_flcn == NULL || pmu_flcn->flcn == NULL) {
		return NULL;
	}

	flcn_base = pmu_flcn->flcn->flcn_base;
	if ((addr >= flcn_base) &&
	    (addr < (flcn_base + UTF_FALCON_MAX_REG_OFFSET))) {
		flcn = pmu_flcn;
	}

	return flcn;
}

static void writel_access_reg_fn(struct gk20a *g,
				 struct nvgpu_reg_access *access)
{
	struct utf_falcon *flcn = NULL;

	flcn = pmu_flcn_from_addr(g, access->addr);
	if (flcn != NULL) {
		nvgpu_utf_falcon_writel_access_reg_fn(g, flcn, access);
	} else {
		nvgpu_posix_io_writel_reg_space(g, access->addr, access->value);
	}
	nvgpu_posix_io_record_access(g, access);
}

static void readl_access_reg_fn(struct gk20a *g,
				struct nvgpu_reg_access *access)
{
	struct utf_falcon *flcn = NULL;

	flcn = pmu_flcn_from_addr(g, access->addr);
	if (flcn != NULL) {
		nvgpu_utf_falcon_readl_access_reg_fn(g, flcn, access);
	} else {
		access->value = nvgpu_posix_io_readl_reg_space(g, access->addr);
	}
}

static struct nvgpu_posix_io_callbacks utf_falcon_reg_callbacks = {
	.writel          = writel_access_reg_fn,
	.writel_check    = writel_access_reg_fn,
	.bar1_writel     = writel_access_reg_fn,
	.usermode_writel = writel_access_reg_fn,

	.__readl         = readl_access_reg_fn,
	.readl           = readl_access_reg_fn,
	.bar1_readl      = readl_access_reg_fn,
};

static void utf_falcon_register_io(struct gk20a *g)
{
	nvgpu_posix_register_io(g, &utf_falcon_reg_callbacks);
}

static int init_acr_falcon_test_env(struct unit_module *m, struct gk20a *g)
{
	int err = 0;

	nvgpu_posix_io_init_reg_space(g);
	utf_falcon_register_io(g);

	/*
	 * Fuse register fuse_opt_priv_sec_en_r() is read during init_hal hence
	 * add it to reg space
	 */
	if (nvgpu_posix_io_add_reg_space(g,
					fuse_opt_priv_sec_en_r(), 0x4) != 0) {
		unit_err(m, "Add reg space failed!\n");
		return -ENOMEM;
	}

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
		return -ENODEV;
	}

	/*
	 * Initialize utf & nvgpu falcon
	 * for test usage
	 */
	pmu_flcn = nvgpu_utf_falcon_init(m, g, FALCON_ID_PMU);
	if (pmu_flcn == NULL) {
		return -ENODEV;
	}

	return 0;
}

int test_acr_init(struct unit_module *m,
				struct gk20a *g, void *args)
{
	int err;
	struct nvgpu_posix_fault_inj *kmem_fi =
		nvgpu_kmem_get_fault_injection();

	/*
	 * initialize falcon
	 */
	if (init_acr_falcon_test_env(m, g) != 0) {
		unit_return_fail(m, "Module init failed\n");
	}

	/*
	 * initialize PMU
	 */
	err =  nvgpu_pmu_early_init(g, &g->pmu);
	if (err != 0) {
		unit_return_fail(m, "nvgpu_pmu_early_init failed\n");
	}

	/*
	 * Case 1: nvgpu_acr_init() fails
	 * due to memory allocation failure
	 */
	nvgpu_posix_enable_fault_injection(kmem_fi, true, 0);
	err = nvgpu_acr_init(g, &g->acr);

	if (err != -ENOMEM) {
		unit_return_fail(m,
			"Memory allocation failure for nvgpu_acr_init() \
			didn't happen as expected\n");
	}
	nvgpu_posix_enable_fault_injection(kmem_fi, false, 0);

	/* Case 2: nvgpu_acr_init() fails due to wrong
	 * version of the chips
	 */

	/*
	 * giving incorrect chip id
	 */
	g->params.gpu_arch = NV_PMC_BOOT_0_ARCHITECTURE_INVALID;
	g->params.gpu_impl = NV_PMC_BOOT_0_IMPLEMENTATION_INVALID;

	err = nvgpu_acr_init(g, &g->acr);
	if (err != -EINVAL) {
		unit_return_fail(m, "Version failure of chip for \
				nvgpu_acr_init() didn't happen as expected\n");
	}

	/*
	 * Case 3: nvgpu_acr_init() passes
	 */

	/*
	 * HAL init parameters for gv11b
	 */
	g->params.gpu_arch = NV_PMC_BOOT_0_ARCHITECTURE_GV110;
	g->params.gpu_impl = NV_PMC_BOOT_0_IMPLEMENTATION_B;

	err = nvgpu_acr_init(g, &g->acr);
	if (err != 0) {
		unit_return_fail(m, "nvgpu_acr_init() failed\n");
	}

	return UNIT_SUCCESS;
}

int free_falcon_test_env(struct unit_module *m, struct gk20a *g,
					void *__args)
{
	if (pmu_flcn == NULL) {
		unit_return_fail(m, "test environment not initialized.");
	}

	/*
	 * Uninitialize the PMU after
	 * the test is done
	 */
	nvgpu_pmu_remove_support(g, g->pmu);
	if (g->pmu != NULL) {
		unit_return_fail(m, "nvgpu_pmu_remove_support failed\n");
	}

	/*
	 * Free the falcon test environment
	 */
	nvgpu_utf_falcon_free(g, pmu_flcn);
	return UNIT_SUCCESS;
}

struct unit_module_test nvgpu_acr_tests[] = {
	UNIT_TEST(acr_init, test_acr_init, NULL, 0),
	UNIT_TEST(acr_free_falcon_test_env, free_falcon_test_env, NULL, 0),

};

UNIT_MODULE(nvgpu-acr, nvgpu_acr_tests, UNIT_PRIO_NVGPU_TEST);
