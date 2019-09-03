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

#include <unit/unit.h>
#include <unit/io.h>

#include <nvgpu/posix/io.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/gr/gr.h>

#include "hal/init/hal_gv11b.h"

#include "nvgpu-gr.h"
#include "nvgpu-gr-gv11b.h"

int test_gr_init_support(struct unit_module *m, struct gk20a *g, void *args)
{
	int err;

	err = test_gr_setup_gv11b_reg_space(m, g);
	if (err != 0) {
		goto fail;
	}

	gv11b_init_hal(g);

	/*
	 * Allocate gr unit
	 */
	err = nvgpu_gr_alloc(g);
	if (err != 0) {
		unit_err(m, " Gr allocation failed!\n");
		return -ENOMEM;
	}

	return UNIT_SUCCESS;

fail:
	return UNIT_FAIL;
}

int test_gr_remove_support(struct unit_module *m,
		struct gk20a *g, void *args)
{
	nvgpu_gr_free(g);

	return UNIT_SUCCESS;
}
