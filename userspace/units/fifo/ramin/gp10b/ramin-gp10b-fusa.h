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
#ifndef UNIT_FIFO_RAMIN_GP10B_FUSA_H
#define UNIT_FIFO_RAMIN_GP10B_FUSA_H

#include <nvgpu/types.h>

struct unit_module;
struct gk20a;

/** @addtogroup SWUTS-fifo-ramin-gp10b
 *  @{
 *
 * Software Unit Test Specification for fifo/ramin/gp10b
 */

/**
 * Test specification for: test_gp10b_ramin_set_gr_ptr
 *
 * Description: Initialize instance block's PDB
 *
 * Test Type: Feature based
 *
 * Targets: gops_ramin.set_gr_ptr, gp10b_ramin_set_gr_ptr
 *
 * Input: None
 *
 * Steps:
 * - Configure PDB aperture, big page size, pdb address, PT format and default
 *   attribute.
 * - Check page directory base values stored in instance block are correct.
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_gp10b_ramin_init_pdb(struct unit_module *m, struct gk20a *g,
								void *args);

/**
 * @}
 */

#endif /* UNIT_FIFO_RAMIN_GP10B_FUSA_H */
