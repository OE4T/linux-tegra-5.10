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

#include <gv11b_mock_regs.h>

#include "nvgpu-gr-gv11b.h"

struct gr_test_reg_info {
	int idx;
	u32 base;
	u32 size;
	const u32 *data;
};

#define NUM_REG_SPACES 10U
struct gr_test_reg_info  gr_gv11b_reg_space[NUM_REG_SPACES] = {
	[0] = {
		.idx = gv11b_master_reg_idx,
		.base = 0x00000000,
		.size = 0x0,
		.data = NULL,
	      },
	[1] = {
		.idx = gv11b_pri_reg_idx,
		.base = 0x00120000,
		.size = 0x0,
		.data = NULL,
	      },
	[2] = {
		.idx = gv11b_fuse_reg_idx,
		.base = 0x00021000,
		.size = 0x0,
		.data = NULL,
	      },
	[3] = {
		.idx = gv11b_top_reg_idx,
		.base = 0x00022400,
		.size = 0x0,
		.data = NULL,
	      },
	[4] = {
		.idx = gv11b_gr_reg_idx,
		.base = 0x00400000,
		.size = 0x0,
		.data = NULL,
	      },
	[5] = {
		.idx = gv11b_fifo_reg_idx,
		.base = 0x2000,
		.size = 0x0,
		.data = NULL,
	      },
	[6] = { /* NV_FBIO_REGSPACE */
		.base = 0x100800,
		.size = 0x7FF,
		.data = NULL,
	      },
	[7] = { /* NV_PLTCG_LTCS_REGSPACE */
		.base = 0x17E200,
		.size = 0x100,
		.data = NULL,
	      },
	[8] = { /* NV_PFB_HSHUB_ACTIVE_LTCS REGSPACE */
		.base = 0x1FBC20,
		.size = 0x4,
		.data = NULL,
	      },
	[9] = { /* NV_PCCSR_CHANNEL REGSPACE */
		.base = 0x800004,
		.size = 0x1F,
		.data = NULL,
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

static void gr_io_delete_reg_space(struct unit_module *m, struct gk20a *g)
{
	u32 i = 0;

	for (i = 0; i < NUM_REG_SPACES; i++) {
		u32 base = gr_gv11b_reg_space[i].base;
		nvgpu_posix_io_delete_reg_space(g, base);
	}
}

static int gr_io_add_reg_space(struct unit_module *m, struct gk20a *g)
{
	int ret = UNIT_SUCCESS;
	u32 i = 0, j = 0;
	u32 base, size;
	struct nvgpu_posix_io_reg_space *gr_io_reg;

	for (i = 0; i < NUM_REG_SPACES; i++) {
		base = gr_gv11b_reg_space[i].base;
		size = gr_gv11b_reg_space[i].size;
		if (size == 0) {
			struct mock_iospace iospace = {0};

			ret = gv11b_get_mock_iospace(gr_gv11b_reg_space[i].idx,
					&iospace);
			if (ret != 0) {
				unit_err(m, "failed to get reg space for %08x\n",
						base);
				goto clean_init_reg_space;
			}
			gr_gv11b_reg_space[i].data = iospace.data;
			gr_gv11b_reg_space[i].size = size = iospace.size;
		}
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

		if (gr_gv11b_reg_space[i].data != NULL) {
			memcpy(gr_io_reg->data, gr_gv11b_reg_space[i].data, size);
		} else {
			memset(gr_io_reg->data, 0, size);
		}
	}

	return ret;

clean_init_reg_space:
	for (j = 0; j < i; j++) {
		base = gr_gv11b_reg_space[j].base;
		nvgpu_posix_io_delete_reg_space(g, base);
	}

	return ret;
}

int test_gr_setup_gv11b_reg_space(struct unit_module *m, struct gk20a *g)
{
	/* Create register space */
	nvgpu_posix_io_init_reg_space(g);

	if (gr_io_add_reg_space(m, g) == UNIT_FAIL) {
		unit_err(m, "failed to get initialized reg space\n");
		return UNIT_FAIL;
	}

	(void)nvgpu_posix_register_io(g, &gr_test_reg_callbacks);

	return 0;
}

void test_gr_cleanup_gv11b_reg_space(struct unit_module *m, struct gk20a *g)
{
	gr_io_delete_reg_space(m, g);
}
