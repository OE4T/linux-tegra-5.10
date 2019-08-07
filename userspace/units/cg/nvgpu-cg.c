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

#include <unit/unit.h>
#include <unit/io.h>

#include <nvgpu/io.h>
#include <nvgpu/enabled.h>
#include <nvgpu/power_features/cg.h>
#include <nvgpu/posix/io.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/hw/gp10b/hw_fuse_gp10b.h>

#include "hal/init/hal_gv11b.h"
#include "hal/power_features/cg/gating_reglist.h"
#include "hal/power_features/cg/gv11b_gating_reglist.h"

#include "nvgpu-cg.h"

struct cg_test_data {
	u32 cg_type;
	void (*load_enable)(struct gk20a *g);
	u32 domain_count;
	const struct gating_desc *domain_descs[16];
	u32 domain_desc_sizes[16];
};

static struct cg_test_data blcg_fb_ltc = {
	.cg_type = NVGPU_GPU_CAN_BLCG,
	.load_enable = nvgpu_cg_blcg_fb_ltc_load_enable,
	.domain_count = 2,
};

static struct cg_test_data blcg_fifo = {
	.cg_type = NVGPU_GPU_CAN_BLCG,
	.load_enable = nvgpu_cg_blcg_fifo_load_enable,
	.domain_count = 1,
};

static struct cg_test_data blcg_pmu = {
	.cg_type = NVGPU_GPU_CAN_BLCG,
	.load_enable = nvgpu_cg_blcg_pmu_load_enable,
	.domain_count = 1,
};

static struct cg_test_data blcg_ce = {
	.cg_type = NVGPU_GPU_CAN_BLCG,
	.load_enable = nvgpu_cg_blcg_ce_load_enable,
	.domain_count = 1,
};

static struct cg_test_data blcg_gr = {
	.cg_type = NVGPU_GPU_CAN_BLCG,
	.load_enable = nvgpu_cg_blcg_gr_load_enable,
	.domain_count = 1,
};

static struct cg_test_data slcg_fb_ltc = {
	.cg_type = NVGPU_GPU_CAN_SLCG,
	.load_enable = nvgpu_cg_slcg_fb_ltc_load_enable,
	.domain_count = 2,
};

static struct cg_test_data slcg_priring = {
	.cg_type = NVGPU_GPU_CAN_SLCG,
	.load_enable = nvgpu_cg_slcg_priring_load_enable,
	.domain_count = 1,
};

static struct cg_test_data slcg_fifo = {
	.cg_type = NVGPU_GPU_CAN_SLCG,
	.load_enable = nvgpu_cg_slcg_fifo_load_enable,
	.domain_count = 1,
};

struct cg_test_data slcg_pmu = {
	.cg_type = NVGPU_GPU_CAN_SLCG,
	.load_enable = nvgpu_cg_slcg_pmu_load_enable,
	.domain_count = 1,
};

struct cg_test_data slcg_ce2 = {
	.cg_type = NVGPU_GPU_CAN_SLCG,
	.load_enable = nvgpu_cg_slcg_ce2_load_enable,
	.domain_count = 1,
};

struct cg_test_data slcg_gr_load_gating_prod = {
	.cg_type = NVGPU_GPU_CAN_SLCG,
	.load_enable = nvgpu_cg_init_gr_load_gating_prod,
	.domain_count = 6,
};

struct cg_test_data blcg_gr_load_gating_prod = {
	.cg_type = NVGPU_GPU_CAN_BLCG,
	.load_enable = nvgpu_cg_init_gr_load_gating_prod,
	.domain_count = 4,
};

#define INIT_BLCG_DOMAIN_TEST_DATA(param)	({\
	struct cg_test_data *tmp = &blcg_##param; \
	tmp->domain_descs[0] = gv11b_blcg_##param##_get_gating_prod(); \
	tmp->domain_desc_sizes[0] = gv11b_blcg_##param##_gating_prod_size(); \
	})

static void init_blcg_fb_ltc_data(void)
{
	blcg_fb_ltc.domain_descs[0] = gv11b_blcg_fb_get_gating_prod();
	blcg_fb_ltc.domain_desc_sizes[0] = gv11b_blcg_fb_gating_prod_size();
	blcg_fb_ltc.domain_descs[1] = gv11b_blcg_ltc_get_gating_prod();
	blcg_fb_ltc.domain_desc_sizes[1] = gv11b_blcg_ltc_gating_prod_size();
}

static void init_blcg_fifo_data(void)
{
	INIT_BLCG_DOMAIN_TEST_DATA(fifo);
}

static void init_blcg_pmu_data(void)
{
	INIT_BLCG_DOMAIN_TEST_DATA(pmu);
}

static void init_blcg_ce_data(void)
{
	INIT_BLCG_DOMAIN_TEST_DATA(ce);
}

static void init_blcg_gr_data(void)
{
	INIT_BLCG_DOMAIN_TEST_DATA(gr);
}

static void init_blcg_gr_load_gating_data(void)
{
	blcg_gr_load_gating_prod.domain_descs[0] =
					gv11b_blcg_bus_get_gating_prod();
	blcg_gr_load_gating_prod.domain_desc_sizes[0] =
					gv11b_blcg_bus_gating_prod_size();
	blcg_gr_load_gating_prod.domain_descs[1] =
					gv11b_blcg_gr_get_gating_prod();
	blcg_gr_load_gating_prod.domain_desc_sizes[1] =
					gv11b_blcg_gr_gating_prod_size();
	blcg_gr_load_gating_prod.domain_descs[2] =
					gv11b_blcg_xbar_get_gating_prod();
	blcg_gr_load_gating_prod.domain_desc_sizes[2] =
					gv11b_blcg_xbar_gating_prod_size();
	blcg_gr_load_gating_prod.domain_descs[3] =
					gv11b_blcg_hshub_get_gating_prod();
	blcg_gr_load_gating_prod.domain_desc_sizes[3] =
					gv11b_blcg_hshub_gating_prod_size();
}

#define INIT_SLCG_DOMAIN_TEST_DATA(param)	({\
	struct cg_test_data *tmp = &slcg_##param; \
	tmp->domain_descs[0] = gv11b_slcg_##param##_get_gating_prod(); \
	tmp->domain_desc_sizes[0] = gv11b_slcg_##param##_gating_prod_size(); \
	})

static void init_slcg_fb_ltc_data(void)
{
	slcg_fb_ltc.domain_descs[0] = gv11b_slcg_fb_get_gating_prod();
	slcg_fb_ltc.domain_desc_sizes[0] = gv11b_slcg_fb_gating_prod_size();
	slcg_fb_ltc.domain_descs[1] = gv11b_slcg_ltc_get_gating_prod();
	slcg_fb_ltc.domain_desc_sizes[1] = gv11b_slcg_ltc_gating_prod_size();
}

static void init_slcg_priring_data(void)
{
	INIT_SLCG_DOMAIN_TEST_DATA(priring);
}

static void init_slcg_fifo_data(void)
{
	INIT_SLCG_DOMAIN_TEST_DATA(fifo);
}

static void init_slcg_pmu_data(void)
{
	INIT_SLCG_DOMAIN_TEST_DATA(pmu);
}

static void init_slcg_ce2_data(void)
{
	INIT_SLCG_DOMAIN_TEST_DATA(ce2);
}

static void init_slcg_gr_load_gating_data(void)
{
	slcg_gr_load_gating_prod.domain_descs[0] =
					gv11b_slcg_bus_get_gating_prod();
	slcg_gr_load_gating_prod.domain_desc_sizes[0] =
					gv11b_slcg_bus_gating_prod_size();
	slcg_gr_load_gating_prod.domain_descs[1] =
					gv11b_slcg_chiplet_get_gating_prod();
	slcg_gr_load_gating_prod.domain_desc_sizes[1] =
					gv11b_slcg_chiplet_gating_prod_size();
	slcg_gr_load_gating_prod.domain_descs[2] =
					gv11b_slcg_gr_get_gating_prod();
	slcg_gr_load_gating_prod.domain_desc_sizes[2] =
					gv11b_slcg_gr_gating_prod_size();
	slcg_gr_load_gating_prod.domain_descs[3] =
					gv11b_slcg_perf_get_gating_prod();
	slcg_gr_load_gating_prod.domain_desc_sizes[3] =
					gv11b_slcg_perf_gating_prod_size();
	slcg_gr_load_gating_prod.domain_descs[4] =
					gv11b_slcg_xbar_get_gating_prod();
	slcg_gr_load_gating_prod.domain_desc_sizes[4] =
					gv11b_slcg_xbar_gating_prod_size();
	slcg_gr_load_gating_prod.domain_descs[5] =
					gv11b_slcg_hshub_get_gating_prod();
	slcg_gr_load_gating_prod.domain_desc_sizes[5] =
					gv11b_slcg_hshub_gating_prod_size();
}

static void writel_access_reg_fn(struct gk20a *g,
			     struct nvgpu_reg_access *access)
{
	nvgpu_posix_io_writel_reg_space(g, access->addr, access->value);
	nvgpu_posix_io_record_access(g, access);
}

static void readl_access_reg_fn(struct gk20a *g,
			    struct nvgpu_reg_access *access)
{
	access->value = nvgpu_posix_io_readl_reg_space(g, access->addr);
}


static struct nvgpu_posix_io_callbacks cg_callbacks = {
	/* Write APIs all can use the same accessor. */
	.writel          = writel_access_reg_fn,
	.writel_check    = writel_access_reg_fn,
	.bar1_writel     = writel_access_reg_fn,
	.usermode_writel = writel_access_reg_fn,

	/* Likewise for the read APIs. */
	.__readl         = readl_access_reg_fn,
	.readl           = readl_access_reg_fn,
	.bar1_readl      = readl_access_reg_fn,
};

static int init_test_env(struct unit_module *m, struct gk20a *g, void *args)
{
	nvgpu_posix_register_io(g, &cg_callbacks);
	nvgpu_posix_io_init_reg_space(g);

	/*
	 * Fuse register fuse_opt_priv_sec_en_r() is read during init_hal hence
	 * add it to reg space
	 */
	if (nvgpu_posix_io_add_reg_space(g,
					 fuse_opt_priv_sec_en_r(), 0x4) != 0) {
		unit_err(m, "Add reg space failed!\n");
		return UNIT_FAIL;
	}

	gv11b_init_hal(g);

	init_blcg_fb_ltc_data();
	init_blcg_fifo_data();
	init_blcg_pmu_data();
	init_blcg_ce_data();
	init_blcg_gr_data();
	init_blcg_gr_load_gating_data();

	init_slcg_fb_ltc_data();
	init_slcg_priring_data();
	init_slcg_fifo_data();
	init_slcg_pmu_data();
	init_slcg_ce2_data();
	init_slcg_gr_load_gating_data();

	return UNIT_SUCCESS;
}

static int add_domain_gating_regs(struct gk20a *g,
				  const struct gating_desc *desc, u32 size)
{
	int err = 0;
	u32 i, j;

	for (i = 0; i < size; i++) {
		if (nvgpu_posix_io_add_reg_space(g, desc[i].addr, 0x4) != 0) {
			err = -ENOMEM;
			goto clean_regs;
		}
	}

	return 0;

clean_regs:

	for (j = 0; j < i; j++) {
		nvgpu_posix_io_delete_reg_space(g, desc[j].addr);
	}

	return err;
}

static void delete_domain_gating_regs(struct gk20a *g,
				      const struct gating_desc *desc, u32 size)
{
	u32 i;

	for (i = 0; i < size; i++) {
		nvgpu_posix_io_delete_reg_space(g, desc[i].addr);
	}
}

static void invalid_load_enabled(struct gk20a *g,
				 struct cg_test_data *test_data)
{
	u32 i, j;

	for (i = 0; i < test_data->domain_count; i++) {
		for (j = 0; j < test_data->domain_desc_sizes[i]; j++) {
			nvgpu_writel(g, test_data->domain_descs[i][j].addr,
				     0xdeadbeed);
		}
	}
}

static int verify_load_enabled(struct gk20a *g, struct cg_test_data *test_data)
{
	u32 i, j, value;
	int mismatch = 0;

	for (i = 0; i < test_data->domain_count; i++) {
		for (j = 0; j < test_data->domain_desc_sizes[i]; j++) {
			value =
			    nvgpu_readl(g, test_data->domain_descs[i][j].addr);
			if (value != test_data->domain_descs[i][j].prod) {
				mismatch = 1;
				goto out;
			}
		}
	}

out:
	return mismatch;
}

int test_cg(struct unit_module *m, struct gk20a *g, void *args)
{
	struct cg_test_data *test_data = (struct cg_test_data *) args;
	u32 i;
	int err;

	for (i = 0; i < test_data->domain_count; i++) {
		err = add_domain_gating_regs(g, test_data->domain_descs[i],
					     test_data->domain_desc_sizes[i]);
		if (err != 0) {
			return UNIT_FAIL;
		}
	}

	invalid_load_enabled(g, test_data);

	test_data->load_enable(g);
	err = verify_load_enabled(g, test_data);
	if (err == 0) {
		unit_err(m, "enabled flag not yet set\n");
		return UNIT_FAIL;
	}

	nvgpu_set_enabled(g, test_data->cg_type, true);
	test_data->load_enable(g);
	err = verify_load_enabled(g, test_data);
	if (err == 0) {
		unit_err(m, "platform capability not yet set\n");
		return UNIT_FAIL;
	}

	if (test_data->cg_type == NVGPU_GPU_CAN_BLCG) {
		g->blcg_enabled = true;
	} else if (test_data->cg_type == NVGPU_GPU_CAN_SLCG) {
		g->slcg_enabled = true;
	}
	test_data->load_enable(g);
	err = verify_load_enabled(g, test_data);
	if (err != 0) {
		unit_err(m, "gating registers mismatch\n");
		return UNIT_FAIL;
	}

	for (i = 0; i < test_data->domain_count; i++) {
		delete_domain_gating_regs(g, test_data->domain_descs[i],
					  test_data->domain_desc_sizes[i]);
	}

	nvgpu_set_enabled(g, test_data->cg_type, false);

	g->blcg_enabled = false;
	g->slcg_enabled = false;

	/* Check that no invalid register access occurred */
	if (nvgpu_posix_io_get_error_code(g) != 0) {
		unit_return_fail(m, "Invalid register accessed\n");
	}

	return UNIT_SUCCESS;
}

struct unit_module_test cg_tests[] = {
	UNIT_TEST(init, init_test_env, NULL, 0),

	UNIT_TEST(blcg_fb_ltc, test_cg, &blcg_fb_ltc, 0),
	UNIT_TEST(blcg_fifo,   test_cg, &blcg_fifo, 0),
	UNIT_TEST(blcg_ce,     test_cg, &blcg_ce, 0),
	UNIT_TEST(blcg_pmu,    test_cg, &blcg_pmu, 0),
	UNIT_TEST(blcg_gr,     test_cg, &blcg_gr, 0),

	UNIT_TEST(slcg_fb_ltc,  test_cg, &slcg_fb_ltc, 0),
	UNIT_TEST(slcg_priring, test_cg, &slcg_priring, 0),
	UNIT_TEST(slcg_fifo,    test_cg, &slcg_fifo, 0),
	UNIT_TEST(slcg_pmu,     test_cg, &slcg_pmu, 0),
	UNIT_TEST(slcg_ce2,     test_cg, &slcg_ce2, 0),

	UNIT_TEST(slcg_gr_load_gating_prod, test_cg,
		  &slcg_gr_load_gating_prod, 0),
	UNIT_TEST(blcg_gr_load_gating_prod, test_cg,
		  &blcg_gr_load_gating_prod, 0),
};

UNIT_MODULE(cg, cg_tests, UNIT_PRIO_NVGPU_TEST);
