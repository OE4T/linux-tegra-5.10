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

#include <unit/io.h>
#include <unit/unit.h>
#include <unit/core.h>

#include <nvgpu/posix/io.h>
#include "os/posix/os_posix.h"

#include "hal/mm/mm_gp10b.h"
#include "hal/mm/mm_gv11b.h"
#include "hal/mm/cache/flush_gk20a.h"
#include "hal/mm/cache/flush_gv11b.h"
#include "hal/mm/gmmu/gmmu_gp10b.h"
#include "hal/mm/gmmu/gmmu_gv11b.h"
#include "hal/mm/mmu_fault/mmu_fault_gv11b.h"
#include "hal/fb/fb_gm20b.h"
#include "hal/fb/fb_gv11b.h"
#include "hal/fb/fb_mmu_fault_gv11b.h"
#include "hal/fb/intr/fb_intr_gv11b.h"
#include "hal/fifo/ramin_gk20a.h"
#include "hal/fifo/ramin_gp10b.h"
#include <nvgpu/hw/gv11b/hw_fb_gv11b.h>
#include <nvgpu/hw/gv11b/hw_flush_gv11b.h>

#define TEST_ADDRESS 0x10002000

struct unit_module *current_module;
bool test_flag;

/*
 * Write callback (for all nvgpu_writel calls).
 */
static void writel_access_reg_fn(struct gk20a *g,
			     struct nvgpu_reg_access *access)
{
	if (access->addr == flush_fb_flush_r()) {
		if (access->value == flush_fb_flush_pending_busy_v()) {
			unit_info(current_module,
				"writel: setting FB_flush to not pending\n");
			access->value = 0;
		}
	} else if (access->addr == flush_l2_flush_dirty_r()) {
		if (access->value == flush_l2_flush_dirty_pending_busy_v()) {
			unit_info(current_module,
				"writel: setting L2_flush to not pending\n");
			access->value = 0;
		}
	}

	nvgpu_posix_io_writel_reg_space(g, access->addr, access->value);
	nvgpu_posix_io_record_access(g, access);
}

/*
 * Read callback, similar to the write callback above.
 */
static void readl_access_reg_fn(struct gk20a *g,
			    struct nvgpu_reg_access *access)
{
	access->value = nvgpu_posix_io_readl_reg_space(g, access->addr);
}

/*
 * Define all the callbacks to be used during the test. Typically all
 * write operations use the same callback, likewise for all read operations.
 */
static struct nvgpu_posix_io_callbacks mmu_faults_callbacks = {
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

static void init_platform(struct unit_module *m, struct gk20a *g, bool is_iGPU)
{
	if (is_iGPU) {
		nvgpu_set_enabled(g, NVGPU_MM_UNIFIED_MEMORY, true);
	} else {
		nvgpu_set_enabled(g, NVGPU_MM_UNIFIED_MEMORY, false);
	}

	/* Enable extra features to increase line coverage */
	nvgpu_set_enabled(g, NVGPU_SUPPORT_SEC2_VM, true);
	nvgpu_set_enabled(g, NVGPU_SUPPORT_GSP_VM, true);
}

/*
 * Init the minimum set of HALs to use DMA amd GMMU features, then call the
 * init_mm base function.
 */
static int init_mm(struct unit_module *m, struct gk20a *g)
{
	int err;
	struct nvgpu_os_posix *p = nvgpu_os_posix_from_gk20a(g);

	p->mm_is_iommuable = true;

	g->ops.mm.gmmu.get_default_big_page_size =
		gp10b_mm_get_default_big_page_size;
	g->ops.mm.gmmu.get_mmu_levels = gp10b_mm_get_mmu_levels;
	g->ops.mm.init_inst_block = gv11b_mm_init_inst_block;
	g->ops.mm.gmmu.map = nvgpu_gmmu_map_locked;
	g->ops.mm.gmmu.unmap = nvgpu_gmmu_unmap_locked;
	g->ops.mm.gmmu.gpu_phys_addr = gv11b_gpu_phys_addr;
	g->ops.mm.is_bar1_supported = gv11b_mm_is_bar1_supported;
	g->ops.mm.cache.l2_flush = gv11b_mm_l2_flush;
	g->ops.mm.cache.fb_flush = gk20a_mm_fb_flush;
#ifdef CONFIG_NVGPU_DGPU
	g->ops.fb.compression_page_size = gp10b_fb_compression_page_size;
#endif
	g->ops.fb.tlb_invalidate = gm20b_fb_tlb_invalidate;
	g->ops.ramin.init_pdb = gp10b_ramin_init_pdb;
	g->ops.ramin.alloc_size = gk20a_ramin_alloc_size;
	g->ops.fb.is_fault_buf_enabled = gv11b_fb_is_fault_buf_enabled;
	g->ops.fb.read_mmu_fault_buffer_size =
		gv11b_fb_read_mmu_fault_buffer_size;
	g->ops.fb.init_hw = gv11b_fb_init_hw;
	g->ops.fb.intr.enable = gv11b_fb_intr_enable;

	nvgpu_posix_register_io(g, &mmu_faults_callbacks);
	nvgpu_posix_io_init_reg_space(g);

	/* Register space: FB_MMU */
	if (nvgpu_posix_io_add_reg_space(g, fb_niso_intr_r(), 0x800) != 0) {
		unit_return_fail(m, "nvgpu_posix_io_add_reg_space failed\n");
	}

	/* Register space: HW_FLUSH */
	if (nvgpu_posix_io_add_reg_space(g, flush_fb_flush_r(), 0x20) != 0) {
		unit_return_fail(m, "nvgpu_posix_io_add_reg_space failed\n");
	}

	if (g->ops.mm.is_bar1_supported(g)) {
		unit_return_fail(m, "BAR1 is not supported on Volta+\n");
	}

	g->has_cde = true;
	err = nvgpu_init_mm_support(g);
	if (err != 0) {
		unit_return_fail(m, "nvgpu_init_mm_support failed err=%d\n",
				err);
	}

	err = nvgpu_mm_setup_hw(g);
	if (err != 0) {
		unit_return_fail(m, "nvgpu_mm_setup_hw failed err=%d\n",
				err);
	}

	return UNIT_SUCCESS;
}

/*
 * Test: test_mm_mm_init
 * This test must be run once and be the first one as it initializes the MM
 * subsystem.
 */
static int test_mm_init(struct unit_module *m, struct gk20a *g, void *args)
{
	g->log_mask = 0;
	if (verbose_lvl(m) >= 1) {
		g->log_mask = gpu_dbg_map;
	}
	if (verbose_lvl(m) >= 2) {
		g->log_mask |= gpu_dbg_map_v;
	}
	if (verbose_lvl(m) >= 3) {
		g->log_mask |= gpu_dbg_pte;
	}

	current_module = m;

	init_platform(m, g, true);

	if (init_mm(m, g) != 0) {
		unit_return_fail(m, "nvgpu_init_mm_support failed\n");
	}

	return UNIT_SUCCESS;
}

/*
 * Test: test_mm_suspend
 * Test nvgpu_mm_suspend and run through some branches depending on enabled
 * HALs.
 */
static int test_mm_suspend(struct unit_module *m, struct gk20a *g,
				void *args)
{
	int err;

	g->power_on = false;
	err = nvgpu_mm_suspend(g);
	if (err != -ETIMEDOUT) {
		unit_return_fail(m, "suspend did not fail as expected err=%d\n",
				err);
	}

	g->power_on = true;
	err = nvgpu_mm_suspend(g);
	if (err != 0) {
		unit_return_fail(m, "suspend fail err=%d\n", err);
	}

	/*
	 * Some optional HALs are executed if not NULL in nvgpu_mm_suspend.
	 * Calls above went through branches where these HAL pointers were NULL,
	 * now define them and run again for complete coverage.
	 */
	g->ops.fb.intr.disable = gv11b_fb_intr_disable;
	g->ops.mm.mmu_fault.disable_hw = gv11b_mm_mmu_fault_disable_hw;
	g->power_on = true;
	err = nvgpu_mm_suspend(g);
	if (err != 0) {
		unit_return_fail(m, "suspend fail err=%d\n", err);
	}

	return UNIT_SUCCESS;
}

/*
 * Simple helper to toggle a flag when called.
 */
static void helper_deinit_pdb_cache_war(struct gk20a *g)
{
	test_flag = true;
}

/*
 * Test: test_mm_remove_mm_support
 * Test mm.remove_support and run through some branches depending on enabled
 * HALs.
 */
static int test_mm_remove_mm_support(struct unit_module *m, struct gk20a *g,
				void *args)
{
	/* Add BAR2 to have more VMs to free */
	g->ops.mm.init_bar2_vm = gp10b_mm_init_bar2_vm;
	g->ops.mm.init_bar2_vm(g);

	/*
	 * Since the last step of the removal is to call
	 * g->ops.ramin.deinit_pdb_cache_war, it is a good indication that
	 * the removal completed successfully.
	 */
	g->ops.ramin.deinit_pdb_cache_war = helper_deinit_pdb_cache_war;
	test_flag = false;

	g->mm.remove_support(&g->mm);

	g->ops.ramin.deinit_pdb_cache_war = NULL;
	if (!test_flag) {
		unit_return_fail(m, "mm removal did not complete\n");
	}

	/* Add extra HALs to cover some branches */
	g->ops.mm.mmu_fault.info_mem_destroy =
		gv11b_mm_mmu_fault_info_mem_destroy;
	g->ops.mm.remove_bar2_vm = gp10b_mm_remove_bar2_vm;
	g->mm.remove_support(&g->mm);

	return UNIT_SUCCESS;
}

/*
 * Test: test_mm_page_sizes
 * Test a couple of page_size related functions
 */
static int test_mm_page_sizes(struct unit_module *m, struct gk20a *g,
				void *args)
{
	if (nvgpu_mm_get_default_big_page_size(g) != SZ_64K) {
		unit_return_fail(m, "unexpected big page size (1)\n");
	}
	if (nvgpu_mm_get_available_big_page_sizes(g) != SZ_64K) {
		unit_return_fail(m, "unexpected big page size (2)\n");
	}

	/* For branch/line coverage */
	g->mm.disable_bigpage = true;
	if (nvgpu_mm_get_available_big_page_sizes(g) != 0) {
		unit_return_fail(m, "unexpected big page size (3)\n");
	}
	g->mm.disable_bigpage = false;

	return UNIT_SUCCESS;
}

/*
 * Test: test_mm_inst_block
 * Test nvgpu_inst_block_ptr.
 */
static int test_mm_inst_block(struct unit_module *m, struct gk20a *g,
				void *args)
{
	u32 addr;
	struct nvgpu_mem *block = malloc(sizeof(struct nvgpu_mem));

	block->aperture = APERTURE_SYSMEM;
	block->cpu_va = (void *) TEST_ADDRESS;

	g->ops.ramin.base_shift = gk20a_ramin_base_shift;
	addr = nvgpu_inst_block_ptr(g, block);
	free(block);

	if (addr != ((u32) TEST_ADDRESS >> g->ops.ramin.base_shift())) {
		unit_return_fail(m, "invalid inst_block_ptr address\n");
	}

	return UNIT_SUCCESS;
}

struct unit_module_test nvgpu_mm_mm_tests[] = {
	UNIT_TEST(init, test_mm_init, NULL, 0),
	UNIT_TEST(suspend, test_mm_suspend, NULL, 0),
	UNIT_TEST(remove_support, test_mm_remove_mm_support, NULL, 0),
	UNIT_TEST(page_sizes, test_mm_page_sizes, NULL, 0),
	UNIT_TEST(inst_block, test_mm_inst_block, NULL, 0),
};

UNIT_MODULE(mm.mm, nvgpu_mm_mm_tests, UNIT_PRIO_NVGPU_TEST);
