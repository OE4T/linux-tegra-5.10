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

#include "common/gr/gr_priv.h"

#include "../nvgpu-gr.h"

struct unit_module_test nvgpu_gr_init_tests[] = {
	UNIT_TEST(gr_init_setup, test_gr_init_setup, NULL, 0),
	UNIT_TEST(gr_init_prepare, test_gr_init_prepare, NULL, 0),
	UNIT_TEST(gr_init_support, test_gr_init_support, NULL, 0),
	UNIT_TEST(gr_suspend, test_gr_suspend, NULL, 0),
	UNIT_TEST(gr_remove_support, test_gr_remove_support, NULL, 0),
	UNIT_TEST(gr_remove_setup, test_gr_remove_setup, NULL, 0),
};

UNIT_MODULE(nvgpu_gr_init, nvgpu_gr_init_tests, UNIT_PRIO_NVGPU_TEST);
