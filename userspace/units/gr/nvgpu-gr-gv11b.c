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

#define NV_PBB_FBHUB_REGSPACE 0x100B00
#define NV_PRI_GPCCS_PPCS_PES_VSC_VPC_REGSPACE 0x41BE04
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

static void gr_io_delete_initialized_reg_space(struct unit_module *m, struct gk20a *g)
{
	u32 i = 0;
	u32 arr_size = gr_array_reg_space(gr_gv11b_initialized_reg_space);

	for (i = 0; i < arr_size; i++) {
		u32 base = gr_gv11b_initialized_reg_space[i].base;

		nvgpu_posix_io_delete_reg_space(g, base);
	}
}

static int gr_io_add_initialized_reg_space(struct unit_module *m, struct gk20a *g)
{
	int ret = UNIT_SUCCESS;
	u32 arr_size = gr_array_reg_space(gr_gv11b_initialized_reg_space);
	u32 i = 0, j = 0;
	u32 base, size;
	struct nvgpu_posix_io_reg_space *gr_io_reg;

	for (i = 0; i < arr_size; i++) {
		base = gr_gv11b_initialized_reg_space[i].base;
		size = gr_gv11b_initialized_reg_space[i].size;

		if (nvgpu_posix_io_add_reg_space(g, base, size) != 0) {
			unit_err(m, "failed to add reg space for %08x\n", base);
			ret = UNIT_FAIL;
			goto clean_init_reg_space;
		}

		gr_io_reg = nvgpu_posix_io_get_reg_space(g, base);
		if (gr_io_reg == NULL) {
			unit_err(m, "failed to get reg space for %08x\n", base);
			ret = UNIT_FAIL;
			goto clean_init_reg_space;
		}

		memcpy(gr_io_reg->data, gr_gv11b_initialized_reg_space[i].data, size);
	}

	return ret;

clean_init_reg_space:
	for (j = 0; j < i; j++) {
		base = gr_gv11b_initialized_reg_space[j].base;
		nvgpu_posix_io_delete_reg_space(g, base);
	}

	return ret;
}

int test_gr_setup_gv11b_reg_space(struct unit_module *m, struct gk20a *g)
{
	/* Create register space */
	nvgpu_posix_io_init_reg_space(g);

	if (gr_io_add_initialized_reg_space(m, g) == UNIT_FAIL) {
		unit_err(m, "failed to get initialized reg space\n");
		return UNIT_FAIL;
	}

	/*
	 * GPCS_SWDX reg space
	 *
	 */
	if (nvgpu_posix_io_add_reg_space(g,
		gr_gpcs_swdx_dss_zbc_color_r_r(0), 0xEFF) != 0) {
		unit_err(m, "Add gpcs swdx reg space failed!\n");
		goto clean_up_reg_space;
	}

	/*
	 * PRI_GPCS_GCC reg space
	 *
	 */
	if (nvgpu_posix_io_add_reg_space(g,
		gr_pri_gpcs_gcc_dbg_r(), 0x60) != 0) {
		unit_err(m, "Add gpcs gcc dbg reg space failed!\n");
		goto clean_up_swdx_space;
	}

	/*
	 * PRI_GPCS_TPCS reg space
	 *
	 */
	if (nvgpu_posix_io_add_reg_space(g,
		gr_gpcs_tpcs_pe_vaf_r(), 0x9FF) != 0) {
		unit_err(m, "Add tpcs pe reg space failed!\n");
		goto clean_up_gcc_space;
	}

	/*
	 * PRI_GPCS_PPCS reg space
	 *
	 */
	if (nvgpu_posix_io_add_reg_space(g,
		NV_PRI_GPCCS_PPCS_PES_VSC_VPC_REGSPACE, 0x1FF) != 0) {
		unit_err(m, "Add gpcs ppcs pes reg space failed!\n");
		goto clean_up_gcc_tpcs_space;
	}

	/*
	 * FB partition reg space
	 *
	 */
	if (nvgpu_posix_io_add_reg_space(g,
		NV_PBB_FBHUB_REGSPACE, 0x1FF) != 0) {
		unit_err(m, "Add fbhub reg space failed!\n");
		goto clean_up_gcc_ppcs_space;
	}

	/*
	 * MC register mc_enable_r() is set during gr_init_prepare_hw hence
	 * add it to reg space
	 */
	if (nvgpu_posix_io_add_reg_space(g,
					 mc_enable_r(), 0x4) != 0) {
		unit_err(m, "Add mc enable reg space failed!\n");
		goto clean_up_fbhub_space;
	}

	(void)nvgpu_posix_register_io(g, &gr_test_reg_callbacks);

	return 0;

clean_up_fbhub_space:
	nvgpu_posix_io_delete_reg_space(g, NV_PBB_FBHUB_REGSPACE);
clean_up_gcc_ppcs_space:
	nvgpu_posix_io_delete_reg_space(g, gr_gpcs_tpcs_pes_vsc_vpc_r());
clean_up_gcc_tpcs_space:
	nvgpu_posix_io_delete_reg_space(g, gr_gpcs_tpcs_pe_vaf_r());
clean_up_gcc_space:
	nvgpu_posix_io_delete_reg_space(g, gr_pri_gpcs_gcc_dbg_r());
clean_up_swdx_space:
	nvgpu_posix_io_delete_reg_space(g, gr_gpcs_swdx_dss_zbc_color_r_r(0));
clean_up_reg_space:
	return -ENOMEM;
}


void test_gr_cleanup_gv11b_reg_space(struct unit_module *m, struct gk20a *g)
{
	nvgpu_posix_io_delete_reg_space(g, NV_PBB_FBHUB_REGSPACE);
	nvgpu_posix_io_delete_reg_space(g, gr_gpcs_tpcs_pes_vsc_vpc_r());
	nvgpu_posix_io_delete_reg_space(g, gr_gpcs_tpcs_pe_vaf_r());
	nvgpu_posix_io_delete_reg_space(g, gr_pri_gpcs_gcc_dbg_r());
	nvgpu_posix_io_delete_reg_space(g, gr_gpcs_swdx_dss_zbc_color_r_r(0));
	gr_io_delete_initialized_reg_space(m, g);
}
