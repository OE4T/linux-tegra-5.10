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
#include <nvgpu/posix/posix-fault-injection.h>
#include <nvgpu/dma.h>

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

/*
 * Replacement functions that can be assigned to function pointers
 */
static void no_return(struct gk20a *g)
{
	/* noop */
}

static int return_success(struct gk20a *g)
{
	return 0;
}

static int return_fail(struct gk20a *g)
{
	return -1;
}

/*
 * Falcon is tricky because it is called multiple times with different IDs.
 * So, we use this variable to determine which one will return an error.
 */
static u32 falcon_fail_on_id = U32_MAX;
static int falcon_sw_init(struct gk20a *g, u32 falcon_id)
{
	if (falcon_id == falcon_fail_on_id) {
		return -1;
	}

	return 0;
}

/* pmu_early_init is passed a unique struct */
struct nvgpu_pmu;
static int pmu_early_init_return = 0;
static int pmu_early_init(struct gk20a *g, struct nvgpu_pmu **pmu)
{
	return pmu_early_init_return;
}

/* acr_init is passed a unique struct */
struct nvgpu_acr;
static int acr_init_return = 0;
static int acr_init(struct gk20a *g, struct nvgpu_acr **acr)
{
	return acr_init_return;
}

/* acr_construct_execute is passed a unique struct */
static int acr_construct_execute_return = 0;
static int acr_construct_execute(struct gk20a *g, struct nvgpu_acr *acr)
{
	return acr_construct_execute_return;
}

/* generic for passing in a u32 */
static int return_success_u32_param(struct gk20a *g, u32 dummy)
{
	return 0;
}

/* generic for passing in a u32 and returning int */
static int return_failure_u32_param(struct gk20a *g, u32 dummy)
{
	return -1;
}

/* generic for passing in a u32 and returning u32 */
static u32 return_u32_u32_param(struct gk20a *g, u32 dummy)
{
	return 0;
}

/* generic for passing in a u32 but nothin to return */
static void no_return_u32_param(struct gk20a *g, u32 dummy)
{
	/* no op  */
}

int test_setup_env(struct unit_module *m,
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

int test_free_env(struct unit_module *m,
			 struct gk20a *g, void *args)
{
	/* Free mc register space */
	nvgpu_posix_io_delete_reg_space(g, mc_boot_0_r());
	nvgpu_posix_io_delete_reg_space(g, GP10B_FUSE_REG_BASE);

	return UNIT_SUCCESS;
}

int test_can_busy(struct unit_module *m,
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

int test_get_put(struct unit_module *m,
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
	g->ops.gr.ecc.ecc_remove_support = no_return;
	g->ops.ltc.ltc_remove_support = no_return;

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

int test_check_gpu_state(struct unit_module *m,
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

int test_hal_init(struct unit_module *m,
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

/*
 * For the basic init functions that just take a g pointer, we store them in
 * this array so we can just loop over them later
 */
#define MAX_SIMPLE_INIT_FUNC_PTRS 50
typedef int (*simple_init_func_t)(struct gk20a *g);
static simple_init_func_t *simple_init_func_ptrs[MAX_SIMPLE_INIT_FUNC_PTRS];
static unsigned int simple_init_func_ptrs_count;

/* Store into the simple_init_func_ptrs array and initialize to success */
static void setup_simple_init_func_success(simple_init_func_t *f,
					   unsigned int index)
{
	BUG_ON(index >= MAX_SIMPLE_INIT_FUNC_PTRS);
	simple_init_func_ptrs[index] = f;
	*f = return_success;
}

/*
 * Initialize init poweron function pointers in g to return success, but do
 * nothing else.
 */
static void set_poweron_funcs_success(struct gk20a *g)
{
	unsigned int i = 0;

	/* these are the simple case of just taking a g param */
	setup_simple_init_func_success(&g->ops.mm.pd_cache_init, i++);
	setup_simple_init_func_success(&g->ops.clk.init_clk_support, i++);
	setup_simple_init_func_success(&g->ops.nvlink.init, i++);
	setup_simple_init_func_success(&g->ops.fb.init_fbpa, i++);
	setup_simple_init_func_success(&g->ops.fb.mem_unlock, i++);
	setup_simple_init_func_success(&g->ops.fifo.reset_enable_hw, i++);
	setup_simple_init_func_success(&g->ops.ltc.init_ltc_support, i++);
	setup_simple_init_func_success(&g->ops.mm.init_mm_support, i++);
	setup_simple_init_func_success(&g->ops.fifo.fifo_init_support, i++);
	setup_simple_init_func_success(&g->ops.therm.elcg_init_idle_filters, i++);
	setup_simple_init_func_success(&g->ops.gr.gr_prepare_sw, i++);
	setup_simple_init_func_success(&g->ops.gr.gr_enable_hw, i++);
	setup_simple_init_func_success(&g->ops.fbp.fbp_init_support, i++);
	setup_simple_init_func_success(&g->ops.gr.gr_init_support, i++);
	setup_simple_init_func_success(&g->ops.gr.ecc.ecc_init_support, i++);
	setup_simple_init_func_success(&g->ops.therm.init_therm_support, i++);
	setup_simple_init_func_success(&g->ops.ce.ce_init_support, i++);
	simple_init_func_ptrs_count = i;

	/* these don't even return anything */
	g->ops.bus.init_hw = no_return;
	g->ops.clk.disable_slowboot = no_return;
	g->ops.priv_ring.enable_priv_ring = no_return;
	g->ops.mc.intr_enable = no_return;
	g->ops.channel.resume_all_serviceable_ch = no_return;

	/* these are the exceptions */
	g->ops.falcon.falcon_sw_init = falcon_sw_init;
	falcon_fail_on_id = U32_MAX; /* don't fail */
	g->ops.pmu.pmu_early_init = pmu_early_init;
	pmu_early_init_return = 0;
	g->ops.acr.acr_init = acr_init;
	acr_init_return = 0;
	g->ops.fuse.fuse_status_opt_tpc_gpc = return_u32_u32_param;
	g->ops.tpc.tpc_powergate = return_success_u32_param;
	g->ops.acr.acr_construct_execute = acr_construct_execute;
	acr_construct_execute_return = 0;
	g->ops.falcon.falcon_sw_free = no_return_u32_param;

	/* used in support functions */
	g->ops.gr.init.detect_sm_arch = no_return;
	g->ops.gr.ecc.detect = no_return;
}

int test_poweron(struct unit_module *m, struct gk20a *g, void *args)
{
	int ret = UNIT_SUCCESS;
	int err;
	unsigned int i;

	nvgpu_set_enabled(g, NVGPU_SEC_PRIVSECURITY, true);
	nvgpu_set_enabled(g, NVGPU_SUPPORT_NVLINK, true);

	/* test where everything returns success */
	set_poweron_funcs_success(g);
	err = nvgpu_finalize_poweron(g);
	if (err != 0) {
		unit_return_fail(m,
				 "nvgpu_finalize_poweron returned failure\n");
	}

	/* loop over the simple cases */
	for (i = 0; i < simple_init_func_ptrs_count; i++) {
		*simple_init_func_ptrs[i] = return_fail;
		g->power_on = false;
		err = nvgpu_finalize_poweron(g);
		if (err == 0) {
			unit_return_fail(m,
				"nvgpu_finalize_poweron errantly returned success\n");
		}
		*simple_init_func_ptrs[i] = return_success;
	}

	/* handle the exceptions */

	falcon_fail_on_id = FALCON_ID_PMU;
	g->power_on = false;
	err = nvgpu_finalize_poweron(g);
	if (err == 0) {
		unit_return_fail(m,
			"nvgpu_finalize_poweron errantly returned success\n");
	}

	falcon_fail_on_id = FALCON_ID_FECS;
	g->power_on = false;
	err = nvgpu_finalize_poweron(g);
	if (err == 0) {
		unit_return_fail(m,
			"nvgpu_finalize_poweron errantly returned success\n");
	}
	falcon_fail_on_id = U32_MAX; /* stop failing */

	pmu_early_init_return = -1;
	g->power_on = false;
	err = nvgpu_finalize_poweron(g);
	if (err == 0) {
		unit_return_fail(m,
			"nvgpu_finalize_poweron errantly returned success\n");
	}
	pmu_early_init_return = 0;

	acr_init_return = -1;
	g->power_on = false;
	err = nvgpu_finalize_poweron(g);
	if (err == 0) {
		unit_return_fail(m,
			"nvgpu_finalize_poweron errantly returned success\n");
	}
	acr_init_return = 0;


	g->ops.tpc.tpc_powergate = return_failure_u32_param;
	g->power_on = false;
	err = nvgpu_finalize_poweron(g);
	if (err == 0) {
		unit_return_fail(m,
			"nvgpu_finalize_poweron errantly returned success\n");
	}
	g->ops.tpc.tpc_powergate = return_success_u32_param;

	acr_construct_execute_return = -1;
	g->power_on = false;
	err = nvgpu_finalize_poweron(g);
	if (err == 0) {
		unit_return_fail(m,
			"nvgpu_finalize_poweron errantly returned success\n");
	}
	acr_construct_execute_return = 0;

	/* test the case of already being powered on */
	g->power_on = true;
	err = nvgpu_finalize_poweron(g);
	if (err != 0) {
		unit_return_fail(m,
			"nvgpu_finalize_poweron returned fail\n");
	}

	return ret;
}

int test_poweron_branches(struct unit_module *m, struct gk20a *g, void *args)
{
	int err;
	struct nvgpu_posix_fault_inj *kmem_fi =
			nvgpu_kmem_get_fault_injection();

	nvgpu_set_enabled(g, NVGPU_SEC_PRIVSECURITY, false);
	nvgpu_set_enabled(g, NVGPU_SUPPORT_NVLINK, false);

	set_poweron_funcs_success(g);

	/* hit all the NULL pointer checks */
	g->ops.clk.disable_slowboot = NULL;
	g->ops.clk.init_clk_support = NULL;
	g->ops.fb.init_fbpa = NULL;
	g->ops.fb.mem_unlock = NULL;
	g->ops.tpc.tpc_powergate = NULL;
	g->ops.therm.elcg_init_idle_filters = NULL;
	g->ops.gr.ecc.ecc_init_support = NULL;
	g->ops.channel.resume_all_serviceable_ch = NULL;
	g->power_on = false;
	err = nvgpu_finalize_poweron(g);
	if (err != 0) {
		unit_return_fail(m,
			"nvgpu_finalize_poweron returned fail\n");
	}

	/* test the syncpoint paths here */
	nvgpu_set_enabled(g, NVGPU_HAS_SYNCPOINTS, true);
	g->syncpt_unit_size = 0UL;
	g->power_on = false;
	err = nvgpu_finalize_poweron(g);
	if (err != 0) {
		unit_return_fail(m,
			"nvgpu_finalize_poweron returned fail\n");
	}
	g->syncpt_unit_size = 2UL;
	g->power_on = false;
	err = nvgpu_finalize_poweron(g);
	if (err != 0) {
		unit_return_fail(m,
			"nvgpu_finalize_poweron returned fail\n");
	}
	/*
	 * This redundant call will hit the case where memory is already
	 * valid
	 */
	g->power_on = false;
	err = nvgpu_finalize_poweron(g);
	if (err != 0) {
		unit_return_fail(m,
			"nvgpu_finalize_poweron returned fail\n");
	}
	nvgpu_dma_free(g, &g->syncpt_mem);
	nvgpu_posix_enable_fault_injection(kmem_fi, true, 0);
	g->power_on = false;
	err = nvgpu_finalize_poweron(g);
	if (err == 0) {
		unit_return_fail(m,
			"nvgpu_finalize_poweron errantly returned success\n");
	}
	nvgpu_posix_enable_fault_injection(kmem_fi, false, 0);
	nvgpu_dma_free(g, &g->syncpt_mem);

	return UNIT_SUCCESS;
}

int test_poweroff(struct unit_module *m, struct gk20a *g, void *args)
{
	unsigned int i = 0;
	int err;

	/* setup everything to succeed */
	setup_simple_init_func_success(&g->ops.channel.suspend_all_serviceable_ch, i++);
	setup_simple_init_func_success(&g->ops.gr.gr_suspend, i++);
	setup_simple_init_func_success(&g->ops.mm.mm_suspend, i++);
	setup_simple_init_func_success(&g->ops.fifo.fifo_suspend, i++);
	simple_init_func_ptrs_count = i;

	g->ops.clk.suspend_clk_support = no_return;
	g->ops.mc.log_pending_intrs = no_return;
	g->ops.mc.intr_mask = no_return;
	g->ops.falcon.falcon_sw_free = no_return_u32_param;

	err = nvgpu_prepare_poweroff(g);
	if (err != 0) {
		unit_return_fail(m, "nvgpu_prepare_poweroff returned fail\n");
	}

	/* return fail for each case */
	for (i = 0; i < simple_init_func_ptrs_count; i++) {
		*simple_init_func_ptrs[i] = return_fail;
		err = nvgpu_prepare_poweroff(g);
		if (err == 0) {
			unit_return_fail(m,
			    "nvgpu_prepare_poweroff errantly returned pass\n");
		}
		*simple_init_func_ptrs[i] = return_success;
	}

	/* Cover branches for NULL ptr checks */
	g->ops.mc.intr_mask = NULL;
	g->ops.mc.log_pending_intrs = NULL;
	g->ops.channel.suspend_all_serviceable_ch = NULL;
	g->ops.clk.suspend_clk_support = NULL;
	err = nvgpu_prepare_poweroff(g);
	if (err != 0) {
		unit_return_fail(m, "nvgpu_prepare_poweroff returned fail\n");
	}

	return UNIT_SUCCESS;
}

struct unit_module_test init_tests[] = {
	UNIT_TEST(init_setup_env,			test_setup_env,		NULL, 0),
	UNIT_TEST(init_can_busy,			test_can_busy,		NULL, 0),
	UNIT_TEST(init_get_put,				test_get_put,		NULL, 0),
	UNIT_TEST(init_check_gpu_state,			test_check_gpu_state,	NULL, 0),
	UNIT_TEST(init_hal_init,			test_hal_init,		NULL, 0),
	UNIT_TEST(init_poweron,				test_poweron,		NULL, 0),
	UNIT_TEST(init_poweron_branches,		test_poweron_branches,	NULL, 0),
	UNIT_TEST(init_poweroff,			test_poweroff,		NULL, 0),
	UNIT_TEST(init_free_env,			test_free_env,		NULL, 0),
};

UNIT_MODULE(init, init_tests, UNIT_PRIO_NVGPU_TEST);
