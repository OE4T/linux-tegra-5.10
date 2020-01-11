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
#include <nvgpu/gr/gr.h>
#include <nvgpu/lock.h>
#include <nvgpu/firmware.h>

#include <common/acr/acr_wpr.h>
#include <common/acr/acr_priv.h>

#include <nvgpu/hw/gv11b/hw_fuse_gv11b.h>
#include <nvgpu/hw/gv11b/hw_gr_gv11b.h>
#include <nvgpu/hw/gv11b/hw_mc_gv11b.h>
#include <nvgpu/hw/gv11b/hw_fb_gv11b.h>
#include <nvgpu/hw/gv11b/hw_flush_gv11b.h>
#include <nvgpu/hw/gv11b/hw_falcon_gv11b.h>
#include <nvgpu/hw/gv11b/hw_pwr_gv11b.h>

#include "nvgpu-acr.h"
#include "../falcon/falcon_utf.h"
#include "../gr/nvgpu-gr-gv11b.h"

#define NV_PMC_BOOT_0_ARCHITECTURE_GV110        (0x00000015 << \
						NVGPU_GPU_ARCHITECTURE_SHIFT)
#define NV_PMC_BOOT_0_IMPLEMENTATION_B          0xB

#define NV_PMC_BOOT_0_ARCHITECTURE_INVALID	(0x00000018 << \
						NVGPU_GPU_ARCHITECTURE_SHIFT)
#define NV_PMC_BOOT_0_IMPLEMENTATION_INVALID	0xD

#define NV_PBB_FBHUB_REGSPACE 0x100B00

#define BAR0_ERRORS_NUM 6

struct utf_falcon *pmu_flcn, *gpccs_flcn;

static int stub_gv11b_bar0_error_status(struct gk20a *g, u32 *bar0_status,
	u32 *etype)
{
	/* return error */
	return -EIO;
}

static bool stub_gv11b_validate_mem_integrity(struct gk20a *g)
{
	/* return error */
	return false;
}

static bool stub_gv11b_is_debug_mode_en(struct gk20a *g)
{
	/* DEBUG mode enabbled */
	return true;
}

static struct utf_falcon *get_flcn_from_addr(struct gk20a *g, u32 addr)
{
	struct utf_falcon *flcn = NULL;
	u32 flcn_base;

	if (pmu_flcn == NULL || gpccs_flcn == NULL) {
		return NULL;
	}
	if (pmu_flcn->flcn == NULL || gpccs_flcn->flcn == NULL) {
		return NULL;
	}

	flcn_base = pmu_flcn->flcn->flcn_base;
	if ((addr >= flcn_base) &&
		(addr < (flcn_base + UTF_FALCON_MAX_REG_OFFSET))) {
		flcn = pmu_flcn;
	} else {
		flcn_base = gpccs_flcn->flcn->flcn_base;
		if ((addr >= flcn_base) &&
			(addr < (flcn_base + UTF_FALCON_MAX_REG_OFFSET))) {
			flcn = gpccs_flcn;
		}
	}

	return flcn;
}

static void writel_access_reg_fn(struct gk20a *g,
				 struct nvgpu_reg_access *access)
{
	struct utf_falcon *flcn = NULL;

	flcn = get_flcn_from_addr(g, access->addr);
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

	flcn = get_flcn_from_addr(g, access->addr);
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
	 * Register space: FB_MMU
	 */
	if (nvgpu_posix_io_add_reg_space(g, fb_niso_intr_r(), 0x800) != 0) {
		unit_return_fail(m, "nvgpu_posix_io_add_reg_space failed\n");
	}

	/*
	 * Register space: HW_FLUSH
	 */
	if (nvgpu_posix_io_add_reg_space(g, flush_fb_flush_r(), 0x20) != 0) {
		unit_return_fail(m, "nvgpu_posix_io_add_reg_space failed\n");
	}

	if (g->ops.mm.is_bar1_supported(g)) {
		unit_return_fail(m, "BAR1 is not supported on Volta+\n");
	}

	/*
	 * Initialize utf & nvgpu falcon
	 * for test usage
	 */
	pmu_flcn = nvgpu_utf_falcon_init(m, g, FALCON_ID_PMU);
	if (pmu_flcn == NULL) {
		return -ENODEV;
	}

	gpccs_flcn = nvgpu_utf_falcon_init(m, g, FALCON_ID_GPCCS);
	if (gpccs_flcn == NULL) {
		return -ENODEV;
	}

	return 0;
}

static int init_test_env(struct unit_module *m, struct gk20a *g)
{
	int err;

	err = test_gr_setup_gv11b_reg_space(m, g);
	if (err != 0) {
		goto fail;
	}

	/*
	 * initialize falcon and set the required flags
	 */
	if (init_acr_falcon_test_env(m, g) != 0) {
		unit_return_fail(m, "Module init failed\n");
	}

	nvgpu_set_enabled(g, NVGPU_SEC_SECUREGPCCS, true);

	err = nvgpu_gr_alloc(g);
	if (err != 0) {
		unit_err(m, " Gr allocation failed!\n");
		return -ENOMEM;
	}

	/*
	 * initialize PMU
	 */
	err = g->ops.pmu.pmu_early_init(g);
	if (err != 0) {
		unit_return_fail(m, "nvgpu_pmu_early_init failed\n");
	}

	/*
	 * initialize ACR
	 */
	err = g->ops.acr.acr_init(g);
	if (err != 0) {
		unit_return_fail(m, "nvgpu_acr_init failed\n");
	}

	/*
	 * Initialize the MM unit required in ucode blob
	 * preparation
	 */

	err = g->ops.ecc.ecc_init_support(g);
	if (err != 0) {
		unit_return_fail(m, "ecc init failed\n");
	}

	err = g->ops.mm.init_mm_support(g);
	if (err != 0) {
		unit_return_fail(m, "failed to init gk20a mm");
	}

	return 0;
fail:
	return UNIT_FAIL;

}

static int prepare_gr_hw_sw(struct unit_module *m, struct gk20a *g)
{
	int err;

	/*
	 * prepare portion of sw required
	 * for enable hw
	 */
	err = nvgpu_gr_prepare_sw(g);
	if (err != 0) {
		nvgpu_mutex_release(&g->tpc_pg_lock);
		unit_return_fail(m, "failed to prepare sw");
	}

	err = nvgpu_gr_enable_hw(g);
	if (err != 0) {
		nvgpu_mutex_release(&g->tpc_pg_lock);
		unit_return_fail(m, "failed to enable gr");
	}

	return 0;
}

int test_acr_bootstrap_hs_acr(struct unit_module *m,
				struct gk20a *g, void *args)

{
	int err, i;
	struct nvgpu_reg_access access;
	struct nvgpu_posix_fault_inj *kmem_fi =
		nvgpu_kmem_get_fault_injection();
	u64 pmu_bar0_error[BAR0_ERRORS_NUM] = {
		pwr_pmu_bar0_error_status_timeout_host_m(),
		pwr_pmu_bar0_error_status_timeout_fecs_m(),
		pwr_pmu_bar0_error_status_cmd_hwerr_m(),
		pwr_pmu_bar0_error_status_fecserr_m(),
		pwr_pmu_bar0_error_status_hosterr_m(),
		0xFF
	};

	/*
	 * Initialise the test env
	 * and register space needed for the test
	 */
	if (init_test_env(m, g) != 0) {
		unit_return_fail(m, "Test env init failed\n");
	}

	if (nvgpu_posix_io_add_reg_space(g,
			pwr_pmu_bar0_error_status_r(), 0x4) != 0) {
		unit_err(m, "Add pwr_pmu_bar0_error_status reg space failed!\n");
		return -ENOMEM;
	}

	nvgpu_mutex_acquire(&g->tpc_pg_lock);

	/*
	 * Prepare HW and SW setup needed
	 * for the test
	 */
	if (prepare_gr_hw_sw(m, g) != 0) {
		unit_return_fail(m, "Test env init failed\n");
	}

	/*
	 * Case 1: fail scenario
	 * call prepare_ucode_blob without setting halt bit
	 * so that timeout error occurs in acr bootstrap
	 */
	err = g->acr->prepare_ucode_blob(g);
	if (err != 0) {
		unit_return_fail(m, "test failed\n");
	}

	err = nvgpu_acr_bootstrap_hs_acr(g, g->acr);
	if (err == 0) {
		unit_return_fail(m, "test_acr_bootstrap_hs_acr() did not\
				fail as expected");
	}

	/*
	 * Set the falcon_falcon_cpuctl_halt_intr_m bit
	 * for the register falcon_falcon_cpuctl_r
	 */
	access.addr = pmu_flcn->flcn->flcn_base + falcon_falcon_cpuctl_r();
	access.value = falcon_falcon_cpuctl_halt_intr_m();
	nvgpu_utf_falcon_writel_access_reg_fn(g, pmu_flcn, &access);

	/*
	 * Prepare the ucode blob
	 *
	 */
	err = g->acr->prepare_ucode_blob(g);
	if (err != 0) {
		unit_return_fail(m, "test failed\n");
	}

	/*
	 * Case 2: Fail scenario
	 * Memory allocation failure
	 */
	nvgpu_posix_enable_fault_injection(kmem_fi, true, 1);
	err = nvgpu_acr_bootstrap_hs_acr(g, g->acr);
	nvgpu_posix_enable_fault_injection(kmem_fi, false, 0);
	if (err != -ENOENT) {
		unit_return_fail(m, "test_acr_bootstrap_hs_acr() didn't fail \
					as expected\n");
	}

	/*
	 * Case 3: Calling nvgpu_acr_bootstrap_hs_acr()
	 * twice to cover recovery branch
	 */
	err = nvgpu_acr_bootstrap_hs_acr(g, g->acr);
	err = nvgpu_acr_bootstrap_hs_acr(g, g->acr);
	if (err != 0) {
		unit_return_fail(m, "test_acr_bootstrap_hs_acr() failed");
	}

	/*
	 * Case 4:
	 * Covering branch for fail scenario
	 * when "is_falcon_supported" is set to false
	 */

	pmu_flcn->flcn->is_falcon_supported = false;
	err = nvgpu_acr_bootstrap_hs_acr(g, g->acr);
	if (err != -EINVAL) {
		unit_return_fail(m, "test_acr_bootstrap_hs_acr() failed");
	}

	/*
	 * Case 5: branch coverage
	 */
	pmu_flcn->flcn->is_falcon_supported = true;
	g->acr->acr.acr_engine_bus_err_status = NULL;
	err = nvgpu_acr_bootstrap_hs_acr(g, g->acr);

	/*
	 * Case 6:
	 * Covering branch when "acr_engine_bus_err_status" ops fails
	 */
	pmu_flcn->flcn->is_falcon_supported = true;
	g->acr->acr.acr_engine_bus_err_status = stub_gv11b_bar0_error_status;
	err = nvgpu_acr_bootstrap_hs_acr(g, g->acr);
	if (err != -EIO) {
		unit_return_fail(m, "test_acr_bootstrap_hs_acr() failed");
	}

	/*
	 * Adding test cases to test gv11b_pmu_bar0_error_status()
	 */
	pmu_flcn->flcn->is_falcon_supported = true;
	g->acr->acr.acr_engine_bus_err_status = g->ops.pmu.bar0_error_status;
	for (i = 0; i < BAR0_ERRORS_NUM; i++) {
		/*
		 * Write error values to the
		 * pwr_pmu_bar0_error_status_r() register
		 */
		nvgpu_posix_io_writel_reg_space(g,
			pwr_pmu_bar0_error_status_r(), pmu_bar0_error[i]);
		err = nvgpu_acr_bootstrap_hs_acr(g, g->acr);

		if (err != -EIO) {
			unit_return_fail(m, "bar0_error_status error conditions"
						"failed");
		}
	}

	/*
	 * Case 7: branch coverage
	 */
	nvgpu_posix_io_writel_reg_space(g,
			pwr_pmu_bar0_error_status_r(), 0);

	g->acr->acr.acr_engine_bus_err_status = g->ops.pmu.bar0_error_status;
	g->acr->acr.acr_validate_mem_integrity = NULL;
	err = nvgpu_acr_bootstrap_hs_acr(g, g->acr);

	/*
	 * Case 8:
	 * Covering branch when "acr_validate_mem_integrity" ops fails
	 */
	pmu_flcn->flcn->is_falcon_supported = true;
	g->acr->acr.acr_validate_mem_integrity = stub_gv11b_validate_mem_integrity;
	err = nvgpu_acr_bootstrap_hs_acr(g, g->acr);
	if (err != -EAGAIN) {
		unit_return_fail(m, "test_acr_bootstrap_hs_acr() failed");
	}

	/*
	 * Case 9: branch coverage for debug mode
	 */
	g->acr->acr.acr_validate_mem_integrity = g->ops.pmu.validate_mem_integrity;
	g->ops.pmu.is_debug_mode_enabled = stub_gv11b_is_debug_mode_en;
	err = nvgpu_acr_bootstrap_hs_acr(g, g->acr);

	/*
	 * Case 10: branch coverage
	 */
	g->acr->acr.report_acr_engine_bus_err_status = NULL;
	err = nvgpu_acr_bootstrap_hs_acr(g, g->acr);

	/*
	 * Case 11: Fail scenario of nvgpu_acr_bootstrap_hs_acr()
	 * by passing g->acr = NULL
	 */
	g->acr = NULL;
	err = nvgpu_acr_bootstrap_hs_acr(g, g->acr);

	if (err != -EINVAL) {
		unit_return_fail(m, "test_acr_bootstrap_hs_acr() didn't fail \
					as expected\n");
	}

	nvgpu_mutex_release(&g->tpc_pg_lock);

	return UNIT_SUCCESS;
}

int test_acr_construct_execute(struct unit_module *m,
				struct gk20a *g, void *args)
{
	int err;
	struct nvgpu_reg_access access;
	struct nvgpu_posix_fault_inj *kmem_fi =
		nvgpu_kmem_get_fault_injection();

	/*
	 * Initialise the test env
	 * and register space needed for the test
	 */
	if (init_test_env(m, g) != 0) {
		unit_return_fail(m, "Test env init failed\n");
	}

	nvgpu_mutex_acquire(&g->tpc_pg_lock);

	/*
	 * Prepare HW and SW setup needed for the test
	 */

	if (prepare_gr_hw_sw(m, g) != 0) {
		unit_return_fail(m, "Test env init failed\n");
	}

	/*
	 * Set the falcon_falcon_cpuctl_halt_intr_m bit
	 * for the register falcon_falcon_cpuctl_r
	 */
	access.addr = pmu_flcn->flcn->flcn_base + falcon_falcon_cpuctl_r();
	access.value = falcon_falcon_cpuctl_halt_intr_m();
	nvgpu_utf_falcon_writel_access_reg_fn(g, pmu_flcn, &access);

	/*
	 * Case 1: fail scenario
	 * g->acr->prepare_ucode_blob(g) fails due to memory
	 * allocation failure
	 * Thus, acr_construct_execute() fails
	 *
	 * HAL init parameters for gv11b: Correct chip id
	 */
	g->params.gpu_arch = NV_PMC_BOOT_0_ARCHITECTURE_GV110;
	g->params.gpu_impl = NV_PMC_BOOT_0_IMPLEMENTATION_B;

	nvgpu_posix_enable_fault_injection(kmem_fi, true, 0);

	err = g->ops.acr.acr_construct_execute(g);
	if (err == -ENOENT) {
		unit_info(m, "test failed as expected\n");
	} else {
		unit_return_fail(m, "test did not fail as expected\n");
	}

	nvgpu_posix_enable_fault_injection(kmem_fi, false, 0);

	/*
	 * Case 2: fail scenario
	 * Covering fail scenario when "is_falcon_supported"
	 * is set to false this fails nvgpu_acr_bootstrap_hs_acr()
	 */

	pmu_flcn->flcn->is_falcon_supported = false;
	err = g->ops.acr.acr_construct_execute(g);
	if (err != -EINVAL) {
		unit_return_fail(m, "acr_construct_execute(g) failed");
	}

	/*
	 * case 3: pass scenario
	 */
	pmu_flcn->flcn->is_falcon_supported = true;
	err = g->ops.acr.acr_construct_execute(g);
	if (err != 0) {
		unit_return_fail(m, "Bootstrap HS ACR failed");
	}

	err = g->ops.ecc.ecc_init_support(g);
	if (err != 0) {
		unit_return_fail(m, "ecc init failed\n");
	}
	/*
	 * case 4: pass g->acr as NULL to create fail scenario
	 */
	g->acr = NULL;
	err = g->ops.acr.acr_construct_execute(g);
	if (err != -EINVAL) {
		unit_return_fail(m, "Bootstrap HS ACR didn't failed as \
				expected\n");
	}
	nvgpu_mutex_release(&g->tpc_pg_lock);

	return UNIT_SUCCESS;
}

int test_acr_is_lsf_lazy_bootstrap(struct unit_module *m,
				struct gk20a *g, void *args)
{
	bool ret = false;

	/*
	 * Initialise the test env and register space needed
	 * for the test
	 */
	if (init_test_env(m, g) != 0) {
		unit_return_fail(m, "Test env init failed\n");
	}


	nvgpu_mutex_acquire(&g->tpc_pg_lock);

	/*
	 * Prepare HW and SW setup needed for the test
	 */
	if (prepare_gr_hw_sw(m, g) != 0) {
		unit_return_fail(m, "Test env init failed\n");
	}

	/*
	 * case 1: pass scenario
	 */
	ret = nvgpu_acr_is_lsf_lazy_bootstrap(g, g->acr,
					FALCON_ID_FECS);
	if (ret) {
		unit_return_fail(m, "failed to test lazy bootstrap\n");
	}
	ret = nvgpu_acr_is_lsf_lazy_bootstrap(g, g->acr,
					FALCON_ID_PMU);
	if (ret) {
		unit_return_fail(m, "failed to test lazy bootstrap\n");
	}
	ret = nvgpu_acr_is_lsf_lazy_bootstrap(g, g->acr,
					FALCON_ID_GPCCS);
	if (ret) {
		unit_return_fail(m, "failed to test lazy bootstrap\n");
	}

	/*
	 * case 2: pass invalid falcon id to fail the function
	 */
	ret = nvgpu_acr_is_lsf_lazy_bootstrap(g, g->acr,
						FALCON_ID_INVALID);
	if (ret != false) {
		unit_return_fail(m, "lazy bootstrap failure didn't happen as \
				expected\n");
	}

	/*
	 * case 3: pass acr as NULL to fail
	 * nvgpu_acr_is_lsf_lazy_bootstrap()
	 */
	g->acr = NULL;
	ret = nvgpu_acr_is_lsf_lazy_bootstrap(g, g->acr,
						FALCON_ID_FECS);
	if (ret != false) {
		unit_return_fail(m, "lazy bootstrap failure didn't happen as \
				expected\n");
	}

	nvgpu_mutex_release(&g->tpc_pg_lock);

	return UNIT_SUCCESS;
}

int test_acr_prepare_ucode_blob(struct unit_module *m,
				struct gk20a *g, void *args)
{
	int err;
	struct nvgpu_posix_fault_inj *kmem_fi =
		nvgpu_kmem_get_fault_injection();

	/*
	 * Initialise the test env and register space
	 * needed for the test
	 */
	if (init_test_env(m, g) != 0) {
		unit_return_fail(m, "Test env init failed\n");
	}

	nvgpu_mutex_acquire(&g->tpc_pg_lock);

	/*
	 * Prepare HW and SW setup needed for the test
	 */
	if (prepare_gr_hw_sw(m, g) != 0) {
		unit_return_fail(m, "Test env init failed\n");
	}
	/*
	 * Case 1: fail scenario
	 * g->acr->prepare_ucode_blob(g) fails due to memory
	 * allocation failure
	 *
	 * HAL init parameters for gv11b: Correct chip id
	 */
	g->params.gpu_arch = NV_PMC_BOOT_0_ARCHITECTURE_GV110;
	g->params.gpu_impl = NV_PMC_BOOT_0_IMPLEMENTATION_B;

	nvgpu_posix_enable_fault_injection(kmem_fi, true, 0);

	err = g->acr->prepare_ucode_blob(g);
	if (err == -ENOENT) {
		unit_info(m, "test failed as expected\n");
	} else {
		unit_return_fail(m, "test did not fail as expected\n");
	}

	nvgpu_posix_enable_fault_injection(kmem_fi, false, 0);

	nvgpu_posix_enable_fault_injection(kmem_fi, true, 17);

	err = g->acr->prepare_ucode_blob(g);

	if (err == -ENOENT) {
		unit_info(m, "second mem test failed as expected\n");
	} else {
		unit_return_fail(m, "second mem test did not fail as expected\n");
	}

	nvgpu_posix_enable_fault_injection(kmem_fi, false, 0);

	/*
	 * Case 2: Fail scenario
	 * giving incorrect chip version number
	 */

	/*
	 * giving incorrect chip id
	 */
	g->params.gpu_arch = NV_PMC_BOOT_0_ARCHITECTURE_INVALID;
	g->params.gpu_impl = NV_PMC_BOOT_0_IMPLEMENTATION_INVALID;

	err = g->acr->prepare_ucode_blob(g);
	if (err == -ENOENT) {
		unit_info(m, "test failed as expected\n");
	} else {
		unit_return_fail(m, "test did not fail as expected\n");
	}

	/*
	 * Case 3: pass scenario
	 */
	g->params.gpu_arch = NV_PMC_BOOT_0_ARCHITECTURE_GV110;
	g->params.gpu_impl = NV_PMC_BOOT_0_IMPLEMENTATION_B;

	err = g->acr->prepare_ucode_blob(g);
	if (err != 0) {
		unit_return_fail(m, "prepare_ucode_blob test failed\n");
	}

	nvgpu_mutex_release(&g->tpc_pg_lock);

	return UNIT_SUCCESS;
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

	err = g->ops.ecc.ecc_init_support(g);
	if (err != 0) {
		unit_return_fail(m, "ecc init failed\n");
	}

	/*
	 * initialize PMU
	 */
	err =  nvgpu_pmu_early_init(g);
	if (err != 0) {
		unit_return_fail(m, "nvgpu_pmu_early_init failed\n");
	}

	/*
	 * Case 1: nvgpu_acr_init() fails due to memory allocation failure
	 */
	nvgpu_posix_enable_fault_injection(kmem_fi, true, 0);
	err = nvgpu_acr_init(g);

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

	err = nvgpu_acr_init(g);
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
	g->acr = NULL;
	err = nvgpu_acr_init(g);
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
	 * Uninitialize the PMU after the test is done
	 */
	nvgpu_pmu_remove_support(g, g->pmu);
	if (g->pmu != NULL) {
		unit_return_fail(m, "nvgpu_pmu_remove_support failed\n");
	}

	/*
	 * Free the falcon test environment
	 */
	nvgpu_utf_falcon_free(g, pmu_flcn);
	nvgpu_utf_falcon_free(g, gpccs_flcn);
	return UNIT_SUCCESS;
}

struct unit_module_test nvgpu_acr_tests[] = {
	UNIT_TEST(acr_init, test_acr_init, NULL, 0),
	UNIT_TEST(acr_prepare_ucode_blob, test_acr_prepare_ucode_blob, NULL, 0),
	UNIT_TEST(acr_is_lsf_lazy_bootstrap,
				test_acr_is_lsf_lazy_bootstrap, NULL, 0),
	UNIT_TEST(acr_construct_execute, test_acr_construct_execute,
			NULL, 0),

	UNIT_TEST(acr_bootstrap_hs_acr, test_acr_bootstrap_hs_acr,
			NULL, 0),
	UNIT_TEST(acr_free_falcon_test_env, free_falcon_test_env, NULL, 0),

};

UNIT_MODULE(nvgpu-acr, nvgpu_acr_tests, UNIT_PRIO_NVGPU_TEST);
