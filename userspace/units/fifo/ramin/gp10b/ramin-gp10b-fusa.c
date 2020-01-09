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
#include <nvgpu/nvgpu_mem.h>
#include <nvgpu/mm.h>
#include <nvgpu/dma.h>
#include <nvgpu/hw/gv11b/hw_ram_gv11b.h>

#include "hal/fifo/ramin_gk20a.h"
#include "hal/fifo/ramin_gp10b.h"

#include "../../nvgpu-fifo-common.h"
#include "ramin-gp10b-fusa.h"

int test_gp10b_ramin_init_pdb(struct unit_module *m, struct gk20a *g,
								void *args)
{
	struct nvgpu_mem inst_block;
	struct nvgpu_mem pdb_mem;
	int ret = UNIT_FAIL;
	int err;
	u32 data;
	u32 pdb_addr_lo, pdb_addr_hi;
	u64 pdb_addr;
	u32 aperture;

	g->ops.ramin.alloc_size = gk20a_ramin_alloc_size;

	/* Aperture should be fixed = SYSMEM */
	nvgpu_set_enabled(g, NVGPU_MM_HONORS_APERTURE, true);
	err = nvgpu_alloc_inst_block(g, &inst_block);
	unit_assert(err == 0, goto done);

	err = nvgpu_dma_alloc(g, g->ops.ramin.alloc_size(), &pdb_mem);
	unit_assert(err == 0, goto done);

	pdb_addr = nvgpu_mem_get_addr(g, &pdb_mem);
	pdb_addr_lo = u64_lo32(pdb_addr >> ram_in_base_shift_v());
	pdb_addr_hi = u64_hi32(pdb_addr);

	aperture = ram_in_sc_page_dir_base_target_sys_mem_ncoh_v();

	data = aperture | ram_in_page_dir_base_vol_true_f() |
		ram_in_big_page_size_64kb_f() |
		ram_in_page_dir_base_lo_f(pdb_addr_lo) |
		ram_in_use_ver2_pt_format_true_f();

	gp10b_ramin_init_pdb(g, &inst_block, pdb_addr, &pdb_mem);

	unit_assert(nvgpu_mem_rd32(g, &inst_block,
			ram_in_page_dir_base_lo_w()) ==	data, goto done);
	unit_assert(nvgpu_mem_rd32(g, &inst_block,
			ram_in_page_dir_base_hi_w()) ==
			ram_in_page_dir_base_hi_f(pdb_addr_hi), goto done);

	ret = UNIT_SUCCESS;
done:
	if (ret != UNIT_SUCCESS) {
		unit_err(m, "%s failed\n", __func__);
	}

	nvgpu_dma_free(g, &pdb_mem);
	nvgpu_free_inst_block(g, &inst_block);
	nvgpu_set_enabled(g, NVGPU_MM_HONORS_APERTURE, false);
	return ret;
}

struct unit_module_test ramin_gp10b_fusa_tests[] = {
	UNIT_TEST(init_pdb, test_gp10b_ramin_init_pdb, NULL, 0),
};

UNIT_MODULE(ramin_gp10b_fusa, ramin_gp10b_fusa_tests, UNIT_PRIO_NVGPU_TEST);
