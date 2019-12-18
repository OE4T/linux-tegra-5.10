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
#include <nvgpu/posix/kmem.h>
#include <nvgpu/posix/posix-fault-injection.h>

#include <nvgpu/gk20a.h>
#include <nvgpu/gr/gr.h>
#include <nvgpu/gr/config.h>

#include <nvgpu/hw/gv11b/hw_fuse_gv11b.h>
#include <nvgpu/hw/gv11b/hw_gr_gv11b.h>

#include "common/gr/gr_priv.h"

#include "../nvgpu-gr.h"
#include "nvgpu-gr-init-hal-gv11b.h"

#define GR_TEST_FUSES_OVERRIDE_DISABLE_TRUE	0x1U
#define GR_TEST_FUSES_OVERRIDE_DISABLE_FALSE	0x0U

#define GR_TEST_FECS_FEATURE_OVERRIDE_ECC		0x00909999
#define GR_TEST_FECS_FEATURE_OVERRIDE_ECC_ONLY		0x00808888
#define GR_TEST_FECS_FEATURE_OVERRIDE_ECC1		0x0000000F
#define GR_TEST_FECS_FEATURE_OVERRIDE_ECC1_ONLY		0x0000000A
#define GR_TEST_FECS_FEATURE_OVERRIDE_ECC1_FAIL1	0x00000002
#define GR_TEST_FECS_FEATURE_OVERRIDE_ECC1_FAIL2	0x0000000B

static int gr_init_ecc_fail_alloc(struct gk20a *g)
{
	int err, i, loop = 26;
	struct nvgpu_posix_fault_inj *kmem_fi =
		nvgpu_kmem_get_fault_injection();
	struct nvgpu_gr_config *save_gr_config = g->gr->config;

	for (i = 0; i < loop; i++) {
		nvgpu_posix_enable_fault_injection(kmem_fi, true, i);
		err = g->ops.gr.ecc.gpc_tpc_ecc_init(g);
		if (err == 0) {
			return UNIT_FAIL;
		}
		nvgpu_posix_enable_fault_injection(kmem_fi, false, 0);
		g->ops.ecc.ecc_init_support(g);
	}

	loop = 2;

	for (i = 0; i < loop; i++) {
		nvgpu_posix_enable_fault_injection(kmem_fi, true, i);
		err = g->ops.gr.ecc.fecs_ecc_init(g);
		if (err == 0) {
			return UNIT_FAIL;
		}
		nvgpu_posix_enable_fault_injection(kmem_fi, false, 0);
		g->ops.ecc.ecc_init_support(g);
	}

	/* Set gr->config to NULL for branch covergae */
	g->gr->config = NULL;
	g->ecc.initialized = true;
	g->ops.ecc.ecc_remove_support(g);
	g->ecc.initialized = false;
	g->gr->config = save_gr_config;

	return UNIT_SUCCESS;
}

struct gr_init_ecc_stats {
	u32 fuse_override;
	u32 opt_enable;
	u32 fecs_override0;
	u32 fecs_override1;
};

int test_gr_init_ecc_features(struct unit_module *m,
		struct gk20a *g, void *args)
{
struct gr_init_ecc_stats ecc_stats[] = {
	[0] = {
		.fuse_override = GR_TEST_FUSES_OVERRIDE_DISABLE_TRUE,
		.opt_enable = 0x1,
		.fecs_override0 = 0x0,
		.fecs_override1 = 0x0,
	      },
	[1] = {
		.fuse_override = GR_TEST_FUSES_OVERRIDE_DISABLE_TRUE,
		.opt_enable = 0x0,
		.fecs_override0 = 0x0,
		.fecs_override1 = 0x0,
	      },
	[2] = {
		.fuse_override = GR_TEST_FUSES_OVERRIDE_DISABLE_FALSE,
		.opt_enable = 0x0,
		.fecs_override0 = 0x0,
		.fecs_override1 = 0x0,
	      },
	[3] = {
		.fuse_override = GR_TEST_FUSES_OVERRIDE_DISABLE_FALSE,
		.opt_enable = 0x1,
		.fecs_override0 = 0,
		.fecs_override1 = GR_TEST_FECS_FEATURE_OVERRIDE_ECC1_FAIL1,
	      },
	[4] = {
		.fuse_override = GR_TEST_FUSES_OVERRIDE_DISABLE_FALSE,
		.opt_enable = 0x1,
		.fecs_override0 = 0,
		.fecs_override1 = GR_TEST_FECS_FEATURE_OVERRIDE_ECC1_FAIL2,
	      },
	[5] = {
		.fuse_override = GR_TEST_FUSES_OVERRIDE_DISABLE_FALSE,
		.opt_enable = 0x1,
		.fecs_override0 = GR_TEST_FECS_FEATURE_OVERRIDE_ECC_ONLY,
		.fecs_override1 = GR_TEST_FECS_FEATURE_OVERRIDE_ECC1_ONLY,
	      },
	[6] = {
		.fuse_override = GR_TEST_FUSES_OVERRIDE_DISABLE_FALSE,
		.opt_enable = 0x1,
		.fecs_override0 = GR_TEST_FECS_FEATURE_OVERRIDE_ECC,
		.fecs_override1 = GR_TEST_FECS_FEATURE_OVERRIDE_ECC1,
	      },
};
	int err, i;
	int arry_cnt = sizeof(ecc_stats)/
			sizeof(struct gr_init_ecc_stats);

	for (i = 0; i < arry_cnt; i++) {
		nvgpu_posix_io_writel_reg_space(g,
			fuse_opt_feature_fuses_override_disable_r(),
			ecc_stats[i].fuse_override);

		nvgpu_posix_io_writel_reg_space(g,
			fuse_opt_ecc_en_r(),
			ecc_stats[i].opt_enable);

		/* set fecs ecc override */
		nvgpu_posix_io_writel_reg_space(g,
			gr_fecs_feature_override_ecc_r(),
			ecc_stats[i].fecs_override0);

		nvgpu_posix_io_writel_reg_space(g,
			gr_fecs_feature_override_ecc_1_r(),
			ecc_stats[i].fecs_override1);

		g->ops.gr.ecc.detect(g);

	}

	err = gr_init_ecc_fail_alloc(g);
	if (err != 0) {
		unit_return_fail(m, "stall isr failed\n");
	}

	return UNIT_SUCCESS;
}

struct unit_module_test nvgpu_gr_init_tests[] = {
	UNIT_TEST(gr_init_setup, test_gr_init_setup, NULL, 0),
	UNIT_TEST(gr_init_prepare, test_gr_init_prepare, NULL, 0),
	UNIT_TEST(gr_init_support, test_gr_init_support, NULL, 0),
	UNIT_TEST(gr_init_hal_error_injection, test_gr_init_hal_error_injection, NULL, 0),
	UNIT_TEST(gr_init_hal_wait_empty, test_gr_init_hal_wait_empty, NULL, 0),
	UNIT_TEST(gr_init_hal_ecc_scrub_reg, test_gr_init_hal_ecc_scrub_reg, NULL, 0),
	UNIT_TEST(gr_init_hal_config_error_injection, test_gr_init_hal_config_error_injection, NULL, 0),
	UNIT_TEST(gr_suspend, test_gr_suspend, NULL, 0),
	UNIT_TEST(gr_ecc_features, test_gr_init_ecc_features, NULL, 0),
	UNIT_TEST(gr_remove_support, test_gr_remove_support, NULL, 0),
	UNIT_TEST(gr_remove_setup, test_gr_remove_setup, NULL, 0),
};

UNIT_MODULE(nvgpu_gr_init, nvgpu_gr_init_tests, UNIT_PRIO_NVGPU_TEST);
