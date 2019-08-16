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

#include <nvgpu/posix/io.h>
#include <nvgpu/posix/soc_fuse.h>

#include <nvgpu/gk20a.h>

#include "hal/fuse/fuse_gm20b.h"

#include <nvgpu/hw/gv11b/hw_mc_gv11b.h>
#include <nvgpu/hw/gv11b/hw_gr_gv11b.h>

#include "nvgpu-gr-gv11b.h"
#include "nvgpu-gr-gv11b-regs.h"

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

static int tegra_fuse_readl_access_reg_fn(unsigned long offset, u32 *value)
{
	if (offset == FUSE_GCPLEX_CONFIG_FUSE_0) {
		*value = GCPLEX_CONFIG_WPR_ENABLED_MASK;
	}
	return 0;
}

static struct nvgpu_posix_io_callbacks gr_test_reg_callbacks = {
	/* Write APIs all can use the same accessor. */
	.writel          = writel_access_reg_fn,
	.writel_check    = writel_access_reg_fn,
	.bar1_writel     = writel_access_reg_fn,
	.usermode_writel = writel_access_reg_fn,

	/* Likewise for the read APIs. */
	.__readl         = readl_access_reg_fn,
	.readl           = readl_access_reg_fn,
	.bar1_readl      = readl_access_reg_fn,

	.tegra_fuse_readl = tegra_fuse_readl_access_reg_fn,
};

int test_gr_setup_gv11b_reg_space(struct unit_module *m, struct gk20a *g)
{
	/* Create register space */
	nvgpu_posix_io_init_reg_space(g);

	if (nvgpu_posix_io_register_reg_space(g,
			&gv11b_master_reg_space) != 0) {
		unit_err(m, "%s: failed to create master register space\n",
				__func__);
		return UNIT_FAIL;
	}

	if (nvgpu_posix_io_register_reg_space(g, &gv11b_top_reg_space) != 0) {
		unit_err(m, "%s: failed to create top register space\n",
				__func__);
		goto clean_up_master;
	}

	if (nvgpu_posix_io_register_reg_space(g, &gv11b_fuse_reg_space) != 0) {
		unit_err(m, "%s: failed to create fuse register space\n",
				__func__);
		goto clean_up_top;
	}

	if (nvgpu_posix_io_register_reg_space(g, &gv11b_gr_reg_space) != 0) {
		unit_err(m, "%s: failed to create gr register space\n",
				__func__);
		goto clean_up_fuse;
	}

	if (nvgpu_posix_io_register_reg_space(g, &gv11b_priv_ring_reg_space) != 0) {
		unit_err(m, "%s: failed to create priv_ring register space\n",
				__func__);
		goto clean_up_gr;
	}

	if (nvgpu_posix_io_register_reg_space(g, &gv11b_gr_pes_tpc_mask_reg_space) != 0) {
		unit_err(m, "%s: failed to create gr pes_tpc_mask register space\n",
				__func__);
		goto clean_up_priv_ring;
	}

	if (nvgpu_posix_io_register_reg_space(g, &gv11b_gr_fs_reg_space) != 0) {
		unit_err(m, "%s: failed to create gr floorsweep register space\n",
				__func__);
		goto clean_up_pes_tpc_mask;
	}

	/*
	 * MC register mc_enable_r() is set during gr_init_prepare_hw hence
	 * add it to reg space
	 */
	if (nvgpu_posix_io_add_reg_space(g,
					 mc_enable_r(), 0x4) != 0) {
		unit_err(m, "Add mc enable reg space failed!\n");
		goto clean_up_priv_ring;
	}

	(void)nvgpu_posix_register_io(g, &gr_test_reg_callbacks);

	return 0;

clean_up_pes_tpc_mask:
	nvgpu_posix_io_unregister_reg_space(g, &gv11b_gr_pes_tpc_mask_reg_space);
clean_up_priv_ring:
	nvgpu_posix_io_unregister_reg_space(g, &gv11b_priv_ring_reg_space);
clean_up_gr:
	nvgpu_posix_io_unregister_reg_space(g, &gv11b_gr_reg_space);
clean_up_fuse:
	nvgpu_posix_io_unregister_reg_space(g, &gv11b_fuse_reg_space);
clean_up_top:
	nvgpu_posix_io_unregister_reg_space(g, &gv11b_top_reg_space);
clean_up_master:
	nvgpu_posix_io_unregister_reg_space(g, &gv11b_master_reg_space);
	return -ENOMEM;
}


void test_gr_cleanup_gv11b_reg_space(struct unit_module *m, struct gk20a *g)
{
	nvgpu_posix_io_unregister_reg_space(g, &gv11b_top_reg_space);
	nvgpu_posix_io_unregister_reg_space(g, &gv11b_master_reg_space);
	nvgpu_posix_io_unregister_reg_space(g, &gv11b_fuse_reg_space);
	nvgpu_posix_io_unregister_reg_space(g, &gv11b_gr_reg_space);
	nvgpu_posix_io_unregister_reg_space(g, &gv11b_priv_ring_reg_space);
	nvgpu_posix_io_unregister_reg_space(g, &gv11b_gr_pes_tpc_mask_reg_space);
	nvgpu_posix_io_unregister_reg_space(g, &gv11b_gr_fs_reg_space);
}
