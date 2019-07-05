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
#include <nvgpu/gk20a.h>
#include <nvgpu/pmu.h>
#include <nvgpu/falcon.h>
#include <nvgpu/posix/io.h>
#include <nvgpu/posix/posix-fault-injection.h>
#include <nvgpu/hal_init.h>
#include <nvgpu/hw/gp10b/hw_fuse_gp10b.h>
#include <nvgpu/hw/gk20a/hw_falcon_gk20a.h>

#include "../falcon/falcon_utf.h"

struct utf_falcon *pmu_flcn;

#define NV_PMC_BOOT_0_ARCHITECTURE_GV110        (0x00000015 << \
						NVGPU_GPU_ARCHITECTURE_SHIFT)
#define NV_PMC_BOOT_0_IMPLEMENTATION_B          0xB

static bool stub_gv11b_is_pmu_supported(struct gk20a *g)
{
	/* set to false to disable LS PMU ucode support */
	return false;
}

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

static int init_pmu_falcon_test_env(struct unit_module *m, struct gk20a *g)
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

	/* HAL init parameters for gv11b */
	g->params.gpu_arch = NV_PMC_BOOT_0_ARCHITECTURE_GV110;
	g->params.gpu_impl = NV_PMC_BOOT_0_IMPLEMENTATION_B;

	/* HAL init required for getting the falcon ops initialized. */
	err = nvgpu_init_hal(g);
	if (err != 0) {
		return -ENODEV;
	}

	/* Initialize utf & nvgpu falcon for test usage */
	pmu_flcn = nvgpu_utf_falcon_init(m, g, FALCON_ID_PMU);
	if (pmu_flcn == NULL) {
		return -ENODEV;
	}

	return 0;
}

static int test_pmu_early_init(struct unit_module *m,
				struct gk20a *g, void *args)
{
	int err;
	struct nvgpu_posix_fault_inj *kmem_fi =
		nvgpu_kmem_get_fault_injection();

	/*
	 * Case 1: nvgpu_pmu_early_init() fails due to memory
	 * allocation failure
	 */
	nvgpu_posix_enable_fault_injection(kmem_fi, true, 0);
	err = nvgpu_pmu_early_init(g, &g->pmu);

	if (err != -ENOMEM) {
		unit_return_fail(m,
			"nvgpu_pmu_early_init init didn't fail as expected\n");
	}
	nvgpu_posix_enable_fault_injection(kmem_fi, false, 0);

	nvgpu_pmu_remove_support(g, g->pmu);

	/* Case 2: nvgpu_pmu_early_init() passes */
	err = nvgpu_pmu_early_init(g, &g->pmu);
	if (err != 0) {
		unit_return_fail(m, "nvgpu_pmu_early_init failed\n");
	}

	nvgpu_pmu_remove_support(g, g->pmu);


	/* case 3: */
	g->support_ls_pmu = false;
	err =  nvgpu_pmu_early_init(g, &g->pmu);
	if (err != 0) {
		unit_return_fail(m, "support_ls_pmu failed\n");
	}

	nvgpu_pmu_remove_support(g, g->pmu);

	/* case 4: */
	g->support_ls_pmu = true;
	g->ops.pmu.is_pmu_supported = stub_gv11b_is_pmu_supported;
	err = nvgpu_pmu_early_init(g, &g->pmu);

	if (g->support_ls_pmu != false || g->can_elpg != false ||
		g->elpg_enabled != false || g->aelpg_enabled != false) {

		unit_return_fail(m, "is_pmu_supported failed\n");
	}

	nvgpu_pmu_remove_support(g, g->pmu);

	return UNIT_SUCCESS;
}

static int test_pmu_remove_support(struct unit_module *m,
				struct gk20a *g, void *args)
{
	int err;

	err =  nvgpu_pmu_early_init(g, &g->pmu);
	if (err != 0) {
		unit_return_fail(m, "support_ls_pmu failed\n");
	}

	/* case 1: nvgpu_pmu_remove_support() passes */
	nvgpu_pmu_remove_support(g, g->pmu);
	if (g->pmu != NULL) {
		unit_return_fail(m, "nvgpu_pmu_remove_support failed\n");
	}

	return UNIT_SUCCESS;
}

static int test_pmu_reset(struct unit_module *m,
				struct gk20a *g, void *args)
{
	int err;

	/* initialize falcon */
	if (init_pmu_falcon_test_env(m, g) != 0) {
		unit_return_fail(m, "Module init failed\n");
	}

	/* initialize PMU */
	err =  nvgpu_pmu_early_init(g, &g->pmu);
	if (err != 0) {
		unit_return_fail(m, "nvgpu_pmu_early_init failed\n");
	}

	/* case 1: reset passes */
	err = nvgpu_falcon_reset(g->pmu->flcn);
	if (err != 0 || (g->ops.pmu.is_engine_in_reset(g) != false)) {
		unit_return_fail(m, "nvgpu_pmu_reset failed\n");
	}

	/* case 2: reset fails */
	nvgpu_utf_falcon_set_dmactl(g, pmu_flcn, 0x2);
	err = nvgpu_falcon_reset(g->pmu->flcn);

	if (err == 0) {
		unit_return_fail(m, "nvgpu_pmu_reset failed\n");
	}

	return UNIT_SUCCESS;
}

static int free_falcon_test_env(struct unit_module *m, struct gk20a *g,
					void *__args)
{
	nvgpu_utf_falcon_free(g, pmu_flcn);
	return UNIT_SUCCESS;
}

struct unit_module_test nvgpu_pmu_tests[] = {
	UNIT_TEST(pmu_early_init, test_pmu_early_init, NULL, 0),
	UNIT_TEST(pmu_remove_support, test_pmu_remove_support, NULL, 0),
	UNIT_TEST(pmu_reset, test_pmu_reset, NULL, 0),

	UNIT_TEST(falcon_free_test_env, free_falcon_test_env, NULL, 0),
};

UNIT_MODULE(nvgpu-pmu, nvgpu_pmu_tests, UNIT_PRIO_NVGPU_TEST);
