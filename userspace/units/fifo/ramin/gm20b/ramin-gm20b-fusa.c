/*
 * Copyright (c) 2019-2020, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/gk20a.h>
#include <nvgpu/dma.h>
#include <nvgpu/nvgpu_mem.h>
#include <nvgpu/hw/gm20b/hw_ram_gm20b.h>

#include "hal/fifo/ramin_gk20a.h"
#include "hal/fifo/ramin_gm20b.h"

#include "../../nvgpu-fifo-common.h"
#include "ramin-gm20b-fusa.h"

#ifdef RAMIN_GM20B_UNIT_DEBUG
#undef unit_verbose
#define unit_verbose	unit_info
#else
#define unit_verbose(unit, msg, ...) \
	do { \
		if (0) { \
			unit_info(unit, msg, ##__VA_ARGS__); \
		} \
	} while (0)
#endif

#define branches_str test_fifo_flags_str
#define pruned test_fifo_subtest_pruned

#define F_SET_BIG_PAGE_SIZE_64K			BIT(0)
#define F_SET_BIG_PAGE_SIZE_LAST		BIT(1)

static const char *f_set_big_page_size[] = {
	"set_big_page_size_64K",
};

int test_gm20b_ramin_set_big_page_size(struct unit_module *m, struct gk20a *g,
								void *args)
{
	struct nvgpu_mem mem;
	int ret = UNIT_FAIL;
	u32 branches = 0U;
	int err;
	u32 size;
	u32 data = 1U;

	g->ops.ramin.alloc_size = gk20a_ramin_alloc_size;

	err = nvgpu_dma_alloc(g, g->ops.ramin.alloc_size(), &mem);
	unit_assert(err == 0, goto done);

	for (branches = 0U; branches < F_SET_BIG_PAGE_SIZE_LAST; branches++) {
		unit_verbose(m, "%s branches=%s\n",
			__func__, branches_str(branches, f_set_big_page_size));

		/* Initialize value of big page size in mem struct */
		nvgpu_mem_wr32(g, &mem, ram_in_big_page_size_w(), data);

		size = branches & F_SET_BIG_PAGE_SIZE_64K ? SZ_64K : SZ_4K;

		gm20b_ramin_set_big_page_size(g, &mem, size);

		if (branches & F_SET_BIG_PAGE_SIZE_64K) {
			unit_assert(nvgpu_mem_rd32(g, &mem,
				ram_in_big_page_size_w()) ==
				(data | ram_in_big_page_size_64kb_f()),
				goto done);
		} else {
			unit_assert(nvgpu_mem_rd32(g, &mem,
				ram_in_big_page_size_w()) ==
				(data | ram_in_big_page_size_128kb_f()),
				goto done);
		}
	}

	ret = UNIT_SUCCESS;
done:
	if (ret != UNIT_SUCCESS) {
		unit_err(m, "%s branches=%s\n", __func__,
				branches_str(branches, f_set_big_page_size));
	}

	nvgpu_dma_free(g, &mem);
	return ret;
}

struct unit_module_test ramin_gm20b_fusa_tests[] = {
	UNIT_TEST(set_big_page_size, test_gm20b_ramin_set_big_page_size, NULL, 0),
};

UNIT_MODULE(ramin_gm20b_fusa, ramin_gm20b_fusa_tests, UNIT_PRIO_NVGPU_TEST);
