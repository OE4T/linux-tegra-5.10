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

#include <nvgpu/hw/gv11b/hw_fuse_gv11b.h>
#include <nvgpu/hw/gv11b/hw_gr_gv11b.h>

#include "common/gr/gr_priv.h"

#include "../nvgpu-gr.h"

#define GR_TEST_FUSES_OVERRIDE_DISABLE_TRUE	0x1U
#define GR_TEST_FUSES_OVERRIDE_DISABLE_FALSE	0x0U

#define GR_TEST_FECS_FEATURE_OVERRIDE_ECC	0x00909999
#define GR_TEST_FECS_FEATURE_OVERRIDE_ECC1	0x0000000F

static int test_gr_init_ecc_features(struct unit_module *m,
		struct gk20a *g, void *args)
{
	nvgpu_posix_io_writel_reg_space(g,
		fuse_opt_feature_fuses_override_disable_r(),
		GR_TEST_FUSES_OVERRIDE_DISABLE_TRUE);

	g->ops.gr.ecc.detect(g);

	nvgpu_posix_io_writel_reg_space(g,
		fuse_opt_feature_fuses_override_disable_r(),
		GR_TEST_FUSES_OVERRIDE_DISABLE_FALSE);

	/* set fecs ecc override */
	nvgpu_posix_io_writel_reg_space(g,
		gr_fecs_feature_override_ecc_r(),
		GR_TEST_FECS_FEATURE_OVERRIDE_ECC);

	nvgpu_posix_io_writel_reg_space(g,
		gr_fecs_feature_override_ecc_1_r(),
		GR_TEST_FECS_FEATURE_OVERRIDE_ECC1);

	g->ops.gr.ecc.detect(g);

	return UNIT_SUCCESS;
}

struct unit_module_test nvgpu_gr_init_tests[] = {
	UNIT_TEST(gr_init_setup, test_gr_init_setup, NULL, 0),
	UNIT_TEST(gr_init_prepare, test_gr_init_prepare, NULL, 0),
	UNIT_TEST(gr_init_support, test_gr_init_support, NULL, 0),
	UNIT_TEST(gr_suspend, test_gr_suspend, NULL, 0),
	UNIT_TEST(gr_ecc_features, test_gr_init_ecc_features, NULL, 0),
	UNIT_TEST(gr_remove_support, test_gr_remove_support, NULL, 0),
	UNIT_TEST(gr_remove_setup, test_gr_remove_setup, NULL, 0),
};

UNIT_MODULE(nvgpu_gr_init, nvgpu_gr_init_tests, UNIT_PRIO_NVGPU_TEST);
