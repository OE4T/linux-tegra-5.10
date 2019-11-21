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
#include <nvgpu/types.h>
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

struct gr_test_reg_info {
	u32 base;
	u32 size;
};

#define gr_reg_arr_size(x, y) sizeof(x)/sizeof(y)

struct gr_test_reg_info gr_reg_info[] = {
	[0] = { /* NV_FBIO_REGSPACE */
		.base = 0x100800,
		.size = 0x7FF,
	      },
	[1] = { /* NV_PLTCG_LTCS_REGSPACE */
		.base = 0x17E200,
		.size = 0x100,
	      },
	[2] = { /* GPCCS_SWDX REGSPACE */
		.base = 0x00418010,
		.size = 0xFFF,
	      },
	[3] = { /* NV_PRI_GPCS_GCC_DBG REGSPACE */
		.base = gr_pri_gpcs_gcc_dbg_r(),
		.size = 0x60,
	      },
	[4] = { /* NV_PRI_GPCS_TPCS REGSPACE */
		.base = gr_gpcs_tpcs_pe_vaf_r(),
		.size = 0x9FF,
	      },
	[5] = { /* NV_PRI_GPCCS_PPCS_PES_VSC_VPC_REGSPACE */
		.base = 0x41BE04,
		.size = 0x1FF,
	      },
	[6] = { /* NV_PRI_GPCCS_GPC_BLOCK_REGS */
		.base = 0x500300,
		.size = 0x7FF,
	      },
	[7] = { /* PRI_GPCS_GPM REGS */
		.base = 0x00500C10,
		.size = 0x10,
	      },
	[8] = { /* NV_PRI_GPC0_GPC_L15_ECC_REGS */
		.base = 0x501048,
		.size = 0x10,
	      },
	[9] = { /* NV_PRI_GPCCS_FALCON_ECC_REGS */
		.base = 0x502678,
		.size = 0x10,
	      },
	[10] = { /* NV_PRI_GPCCS_GPC_EXCEPTION_REGS */
		.base = 0x502c90,
		.size = 0x10,
	      },
	[11] = { /* NV_PRI_GPCCS_PPC0_PES_REGSPACE */
		.base = 0x503010,
		.size = 0x2FFF,
	      },
	[12] = { /* NV_PFB_HSHUB_ACTIVE_LTCS REGSPACE */
		.base = 0x1FBC20,
		.size = 0x4,
	      },
	[13] = { /* NV_PFIFO_INTR_EN REGSPACE */
		.base = 0x2140,
		.size = 0x4,
	      },
	[14] = { /* NV_PFIFO_SCHED REGSPACE */
		.base = 0x2630,
		.size = 0x10,
	      },
	[15] = { /* NV_PFIFO_INTR_CTXSW REGSPACE */
		.base = 0x2a30,
		.size = 0x4,
	      },
	[16] = { /* NV_PRI_GPCS_SWDX_DSS_DEBUG REGSPACE */
		.base = 0x418000,
		.size = 0xc,
	      },
	[17] = { /* NV_PRI_EGPCS_ETPCS_SM_DSM REGSPACE */
		.base = 0x481a00,
		.size = 0x5FF,
	      },
	[18] = { /* NV_PCCSR_CHANNEL REGSPACE */
		.base = 0x800004,
		.size = 0x1F,
	      },
};

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

static void test_gr_clean_gv11b_io_reg_space(struct unit_module *m, struct gk20a *g, u32 arr_index)
{
	u32 j = 0, base;

	for (j = 0; j < arr_index; j++) {
		base = gr_reg_info[j].base;
		nvgpu_posix_io_delete_reg_space(g, base);
	}
}

int test_gr_setup_gv11b_reg_space(struct unit_module *m, struct gk20a *g)
{
	u32 i = 0;
	u32 arr_size = gr_reg_arr_size(gr_reg_info, struct gr_test_reg_info);

	/* Create register space */
	nvgpu_posix_io_init_reg_space(g);

	if (gr_io_add_initialized_reg_space(m, g) == UNIT_FAIL) {
		unit_err(m, "failed to get initialized reg space\n");
		return UNIT_FAIL;
	}

	for (i = 0; i < arr_size; i++) {
		if (nvgpu_posix_io_add_reg_space(g,
			gr_reg_info[i].base, gr_reg_info[i].size) != 0) {
			unit_err(m, "io add reg space failed!\n");
			goto clean_up_io_reg_space;
		}
	}

	(void)nvgpu_posix_register_io(g, &gr_test_reg_callbacks);

	return 0;

clean_up_io_reg_space:
	test_gr_clean_gv11b_io_reg_space(m, g, i);
	return -ENOMEM;
}


void test_gr_cleanup_gv11b_reg_space(struct unit_module *m, struct gk20a *g)
{
	u32 arr_size = gr_reg_arr_size(gr_reg_info, struct gr_test_reg_info);

	gr_io_delete_initialized_reg_space(m, g);
	test_gr_clean_gv11b_io_reg_space(m, g, arr_size);
}
