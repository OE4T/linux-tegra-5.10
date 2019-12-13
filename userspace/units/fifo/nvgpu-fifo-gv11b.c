/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/hw/gv11b/hw_usermode_gv11b.h>

#include <gv11b_mock_regs.h>

#include "nvgpu-fifo-gv11b.h"

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

	.tegra_fuse_readl = tegra_fuse_readl_access_reg_fn,
};

struct test_reg_space {
	int idx;
	u32 base;
	u32 size;
	const u32 *data;
	void (*init)(u32 *data, u32 size);
};

static void init_reg_space_usermode(u32 *data, u32 size)
{
	u32 i;

	for (i = 0U; i < size/4U; i++) {
		data[i] = 0xbadf1100;
	}
}

#define NUM_REG_SPACES 11U
struct test_reg_space reg_spaces[NUM_REG_SPACES] = {
	[0] = {	/* FUSE */
		.idx = gv11b_fuse_reg_idx,
		.base = 0x00021000,
		.size = 0,
		.data = NULL,
	},
	[1] = { /* MASTER */
		.idx = gv11b_master_reg_idx,
		.base = 0x00000000,
		.size = 0,
		.data = NULL,
	},
	[2] = { /* TOP */
		.idx = gv11b_top_reg_idx,
		.base = 0x22400,
		.size = 0,
		.data = NULL,
	},
	[3] = { /* PBDMA */
		.idx = gv11b_pbdma_reg_idx,
		.base = 0x00040000,
		.size = 0,
		.data = NULL,
	},
	[4] = { /* CCSR */
		.idx = gv11b_ccsr_reg_idx,
		.base = 0x00800000,
		.size = 0,
		.data = NULL,
	},
	[5] = { /* FIFO */
		.idx = gv11b_fifo_reg_idx,
		.base = 0x2000,
		.size = 0,
		.data = NULL,
	},
	[6] = { /* USERMODE */
		.base = usermode_cfg0_r(),
		.size = 0x10000,
		.data = NULL,
		.init = init_reg_space_usermode,
	},
	[7] = { /* CE */
		.base = 0x104000,
		.size = 0x2000,
		.data = NULL,
	},
	[8] = { /* PBUS */
		.base = 0x1000,
		.size = 0x1000,
		.data = NULL,
	},
	[9] = { /* HSUB_COMMON */
		.base = 0x1fbc00,
		.size = 0x400,
		.data = NULL,
	},
	[10] = { /* PFB */
		.base = 0x100000,
		.size = 0x1000,
		.data = NULL,
	},
};

static void fifo_io_delete_reg_spaces(struct unit_module *m, struct gk20a *g)
{
	u32 i = 0;

	for (i = 0; i < NUM_REG_SPACES; i++) {
		u32 base = reg_spaces[i].base;

		nvgpu_posix_io_delete_reg_space(g, base);
	}
}

static int fifo_io_add_reg_spaces(struct unit_module *m, struct gk20a *g)
{
	int ret = 0;
	u32 i = 0, j = 0;
	u32 base, size;
	struct nvgpu_posix_io_reg_space *reg_space;

	for (i = 0; i < NUM_REG_SPACES; i++) {
		base = reg_spaces[i].base;
		size = reg_spaces[i].size;
		if (size == 0) {
			struct mock_iospace iospace = {0};

			ret = gv11b_get_mock_iospace(reg_spaces[i].idx,
					&iospace);
			if (ret != 0) {
				unit_err(m, "failed to get reg space for %08x\n",
						base);
				goto clean_init_reg_space;
			}
			reg_spaces[i].data = iospace.data;
			reg_spaces[i].size = size = iospace.size;
		}
		if (nvgpu_posix_io_add_reg_space(g, base, size) != 0) {
			unit_err(m, "failed to add reg space for %08x\n", base);
			ret = -ENOMEM;
			goto clean_init_reg_space;
		}

		reg_space = nvgpu_posix_io_get_reg_space(g, base);
		if (reg_space == NULL) {
			unit_err(m, "failed to get reg space for %08x\n", base);
			ret = -EINVAL;
			goto clean_init_reg_space;
		} else {
			unit_info(m, " IO reg space %08x:%08x\n", base + size -1, base);
		}

		if (reg_spaces[i].data != NULL) {
			memcpy(reg_space->data, reg_spaces[i].data, size);
		} else {
			if (reg_spaces[i].init != NULL) {
				reg_spaces[i].init(reg_space->data, size);
			} else {
				memset(reg_space->data, 0, size);
			}
		}
	}

	return ret;

clean_init_reg_space:
	for (j = 0; j < i; j++) {
		base = reg_spaces[j].base;
		nvgpu_posix_io_delete_reg_space(g, base);
	}

	return ret;
}

int test_fifo_setup_gv11b_reg_space(struct unit_module *m, struct gk20a *g)
{
	/* Create register space */
	nvgpu_posix_io_init_reg_space(g);

	if (fifo_io_add_reg_spaces(m, g) != 0) {
		unit_err(m, "failed to get initialized reg space\n");
		return UNIT_FAIL;
	}

	(void)nvgpu_posix_register_io(g, &test_reg_callbacks);

	return 0;
}

void test_fifo_cleanup_gv11b_reg_space(struct unit_module *m, struct gk20a *g)
{
	fifo_io_delete_reg_spaces(m, g);
}
