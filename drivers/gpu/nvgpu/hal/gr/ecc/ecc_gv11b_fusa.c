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

#include <nvgpu/io.h>
#include <nvgpu/ecc.h>
#include <nvgpu/gk20a.h>

#include <nvgpu/hw/gv11b/hw_gr_gv11b.h>

#include "ecc_gv11b.h"

static struct nvgpu_hw_err_inject_info fecs_ecc_err_desc[] = {
	NVGPU_ECC_ERR("falcon_imem_ecc_corrected",
			gv11b_gr_intr_inject_fecs_ecc_error,
			gr_fecs_falcon_ecc_control_r,
			gr_fecs_falcon_ecc_control_inject_corrected_err_f),
	NVGPU_ECC_ERR("falcon_imem_ecc_uncorrected",
			gv11b_gr_intr_inject_fecs_ecc_error,
			gr_fecs_falcon_ecc_control_r,
			gr_fecs_falcon_ecc_control_inject_uncorrected_err_f),
};

static struct nvgpu_hw_err_inject_info_desc fecs_err_desc;

struct nvgpu_hw_err_inject_info_desc *
gv11b_gr_intr_get_fecs_err_desc(struct gk20a *g)
{
	fecs_err_desc.info_ptr = fecs_ecc_err_desc;
	fecs_err_desc.info_size = nvgpu_safe_cast_u64_to_u32(
			sizeof(fecs_ecc_err_desc) /
			sizeof(struct nvgpu_hw_err_inject_info));

	return &fecs_err_desc;
}

static struct nvgpu_hw_err_inject_info gpccs_ecc_err_desc[] = {
	NVGPU_ECC_ERR("falcon_imem_ecc_corrected",
			gv11b_gr_intr_inject_gpccs_ecc_error,
			gr_gpccs_falcon_ecc_control_r,
			gr_gpccs_falcon_ecc_control_inject_corrected_err_f),
	NVGPU_ECC_ERR("falcon_imem_ecc_uncorrected",
			gv11b_gr_intr_inject_gpccs_ecc_error,
			gr_gpccs_falcon_ecc_control_r,
			gr_gpccs_falcon_ecc_control_inject_uncorrected_err_f),
};

static struct nvgpu_hw_err_inject_info_desc gpccs_err_desc;

struct nvgpu_hw_err_inject_info_desc *
gv11b_gr_intr_get_gpccs_err_desc(struct gk20a *g)
{
	gpccs_err_desc.info_ptr = gpccs_ecc_err_desc;
	gpccs_err_desc.info_size = nvgpu_safe_cast_u64_to_u32(
			sizeof(gpccs_ecc_err_desc) /
			sizeof(struct nvgpu_hw_err_inject_info));

	return &gpccs_err_desc;
}

static struct nvgpu_hw_err_inject_info sm_ecc_err_desc[] = {
	NVGPU_ECC_ERR("l1_tag_ecc_corrected",
			gv11b_gr_intr_inject_sm_ecc_error,
			gr_pri_gpc0_tpc0_sm_l1_tag_ecc_control_r,
			gr_pri_gpc0_tpc0_sm_l1_tag_ecc_control_inject_corrected_err_f),
	NVGPU_ECC_ERR("l1_tag_ecc_uncorrected",
			gv11b_gr_intr_inject_sm_ecc_error,
			gr_pri_gpc0_tpc0_sm_l1_tag_ecc_control_r,
			gr_pri_gpc0_tpc0_sm_l1_tag_ecc_control_inject_uncorrected_err_f),
	NVGPU_ECC_ERR("cbu_ecc_uncorrected",
			gv11b_gr_intr_inject_sm_ecc_error,
			gr_pri_gpc0_tpc0_sm_cbu_ecc_control_r,
			gr_pri_gpc0_tpc0_sm_cbu_ecc_control_inject_uncorrected_err_f),
	NVGPU_ECC_ERR("lrf_ecc_uncorrected",
			gv11b_gr_intr_inject_sm_ecc_error,
			gr_pri_gpc0_tpc0_sm_lrf_ecc_control_r,
			gr_pri_gpc0_tpc0_sm_lrf_ecc_control_inject_uncorrected_err_f),
	NVGPU_ECC_ERR("l1_data_ecc_uncorrected",
			gv11b_gr_intr_inject_sm_ecc_error,
			gr_pri_gpc0_tpc0_sm_l1_data_ecc_control_r,
			gr_pri_gpc0_tpc0_sm_l1_data_ecc_control_inject_uncorrected_err_f),
	NVGPU_ECC_ERR("icache_l0_data_ecc_uncorrected",
			gv11b_gr_intr_inject_sm_ecc_error,
			gr_pri_gpc0_tpc0_sm_icache_ecc_control_r,
			gr_pri_gpc0_tpc0_sm_icache_ecc_control_inject_uncorrected_err_f),
};

static struct nvgpu_hw_err_inject_info_desc sm_err_desc;

struct nvgpu_hw_err_inject_info_desc *
gv11b_gr_intr_get_sm_err_desc(struct gk20a *g)
{
	sm_err_desc.info_ptr = sm_ecc_err_desc;
	sm_err_desc.info_size = nvgpu_safe_cast_u64_to_u32(
			sizeof(sm_ecc_err_desc) /
			sizeof(struct nvgpu_hw_err_inject_info));

	return &sm_err_desc;
}

static struct nvgpu_hw_err_inject_info mmu_ecc_err_desc[] = {
	NVGPU_ECC_ERR("l1tlb_sa_data_ecc_uncorrected",
			gv11b_gr_intr_inject_mmu_ecc_error,
			gr_gpc0_mmu_l1tlb_ecc_control_r,
			gr_gpc0_mmu_l1tlb_ecc_control_inject_uncorrected_err_f),
};

static struct nvgpu_hw_err_inject_info_desc mmu_err_desc;

struct nvgpu_hw_err_inject_info_desc *
gv11b_gr_intr_get_mmu_err_desc(struct gk20a *g)
{
	mmu_err_desc.info_ptr = mmu_ecc_err_desc;
	mmu_err_desc.info_size = nvgpu_safe_cast_u64_to_u32(
			sizeof(mmu_ecc_err_desc) /
			sizeof(struct nvgpu_hw_err_inject_info));

	return &mmu_err_desc;
}

static struct nvgpu_hw_err_inject_info gcc_ecc_err_desc[] = {
	NVGPU_ECC_ERR("l15_ecc_uncorrected",
			gv11b_gr_intr_inject_gcc_ecc_error,
			gr_pri_gpc0_gcc_l15_ecc_control_r,
			gr_pri_gpc0_gcc_l15_ecc_control_inject_uncorrected_err_f),
};

static struct nvgpu_hw_err_inject_info_desc gcc_err_desc;

struct nvgpu_hw_err_inject_info_desc *
gv11b_gr_intr_get_gcc_err_desc(struct gk20a *g)
{
	gcc_err_desc.info_ptr = gcc_ecc_err_desc;
	gcc_err_desc.info_size = nvgpu_safe_cast_u64_to_u32(
			sizeof(gcc_ecc_err_desc) /
			sizeof(struct nvgpu_hw_err_inject_info));

	return &gcc_err_desc;
}

void gv11b_ecc_detect_enabled_units(struct gk20a *g)
{
	bool opt_ecc_en = g->ops.fuse.is_opt_ecc_enable(g);
	bool opt_feature_fuses_override_disable =
		g->ops.fuse.is_opt_feature_override_disable(g);
	u32 fecs_feature_override_ecc =
			nvgpu_readl(g,
				gr_fecs_feature_override_ecc_r());

	if (opt_feature_fuses_override_disable) {
		if (opt_ecc_en) {
			nvgpu_set_enabled(g,
					NVGPU_ECC_ENABLED_SM_LRF, true);
			nvgpu_set_enabled(g,
					NVGPU_ECC_ENABLED_SM_L1_DATA, true);
			nvgpu_set_enabled(g,
					NVGPU_ECC_ENABLED_SM_L1_TAG, true);
			nvgpu_set_enabled(g,
					NVGPU_ECC_ENABLED_SM_ICACHE, true);
			nvgpu_set_enabled(g, NVGPU_ECC_ENABLED_LTC, true);
			nvgpu_set_enabled(g, NVGPU_ECC_ENABLED_SM_CBU, true);
		}
	} else {
		/* SM LRF */
		if (gr_fecs_feature_override_ecc_sm_lrf_override_v(
				fecs_feature_override_ecc) == 1U) {
			if (gr_fecs_feature_override_ecc_sm_lrf_v(
				fecs_feature_override_ecc) == 1U) {
				nvgpu_set_enabled(g,
						NVGPU_ECC_ENABLED_SM_LRF, true);
			}
		} else {
			if (opt_ecc_en) {
				nvgpu_set_enabled(g,
						NVGPU_ECC_ENABLED_SM_LRF, true);
			}
		}
		/* SM L1 DATA*/
		if (gr_fecs_feature_override_ecc_sm_l1_data_override_v(
				fecs_feature_override_ecc) == 1U) {
			if (gr_fecs_feature_override_ecc_sm_l1_data_v(
				fecs_feature_override_ecc) == 1U) {
				nvgpu_set_enabled(g,
					NVGPU_ECC_ENABLED_SM_L1_DATA, true);
			}
		} else {
			if (opt_ecc_en) {
				nvgpu_set_enabled(g,
					NVGPU_ECC_ENABLED_SM_L1_DATA, true);
			}
		}
		/* SM L1 TAG*/
		if (gr_fecs_feature_override_ecc_sm_l1_tag_override_v(
				fecs_feature_override_ecc) == 1U) {
			if (gr_fecs_feature_override_ecc_sm_l1_tag_v(
				fecs_feature_override_ecc) == 1U) {
				nvgpu_set_enabled(g,
					NVGPU_ECC_ENABLED_SM_L1_TAG, true);
			}
		} else {
			if (opt_ecc_en) {
				nvgpu_set_enabled(g,
					NVGPU_ECC_ENABLED_SM_L1_TAG, true);
			}
		}
		/* SM ICACHE*/
		if ((gr_fecs_feature_override_ecc_1_sm_l0_icache_override_v(
				fecs_feature_override_ecc) == 1U) &&
			(gr_fecs_feature_override_ecc_1_sm_l1_icache_override_v(
				fecs_feature_override_ecc) == 1U)) {
			if ((gr_fecs_feature_override_ecc_1_sm_l0_icache_v(
					fecs_feature_override_ecc) == 1U) &&
				(gr_fecs_feature_override_ecc_1_sm_l1_icache_v(
					fecs_feature_override_ecc) == 1U)) {
				nvgpu_set_enabled(g,
					NVGPU_ECC_ENABLED_SM_ICACHE, true);
			}
		} else {
			if (opt_ecc_en) {
				nvgpu_set_enabled(g,
					NVGPU_ECC_ENABLED_SM_ICACHE, true);
			}
		}
		/* LTC */
		if (gr_fecs_feature_override_ecc_ltc_override_v(
				fecs_feature_override_ecc) == 1U) {
			if (gr_fecs_feature_override_ecc_ltc_v(
				fecs_feature_override_ecc) == 1U) {
				nvgpu_set_enabled(g,
						NVGPU_ECC_ENABLED_LTC, true);
			}
		} else {
			if (opt_ecc_en) {
				nvgpu_set_enabled(g,
						NVGPU_ECC_ENABLED_LTC, true);
			}
		}
		/* SM CBU */
		if (gr_fecs_feature_override_ecc_sm_cbu_override_v(
				fecs_feature_override_ecc) == 1U) {
			if (gr_fecs_feature_override_ecc_sm_cbu_v(
				fecs_feature_override_ecc) == 1U) {
				nvgpu_set_enabled(g,
					NVGPU_ECC_ENABLED_SM_CBU, true);
			}
		} else {
			if (opt_ecc_en) {
				nvgpu_set_enabled(g,
					NVGPU_ECC_ENABLED_SM_CBU, true);
			}
		}
	}
}

int gv11b_ecc_init(struct gk20a *g)
{
	int err;

	err = NVGPU_ECC_COUNTER_INIT_PER_TPC(sm_lrf_ecc_single_err_count);
	if (err != 0) {
		goto done;
	}
	err = NVGPU_ECC_COUNTER_INIT_PER_TPC(sm_lrf_ecc_double_err_count);
	if (err != 0) {
		goto done;
	}

	err = NVGPU_ECC_COUNTER_INIT_PER_TPC(
			sm_l1_tag_ecc_corrected_err_count);
	if (err != 0) {
		goto done;
	}
	err = NVGPU_ECC_COUNTER_INIT_PER_TPC(
			sm_l1_tag_ecc_uncorrected_err_count);
	if (err != 0) {
		goto done;
	}

	err = NVGPU_ECC_COUNTER_INIT_PER_TPC(
			sm_cbu_ecc_corrected_err_count);
	if (err != 0) {
		goto done;
	}
	err = NVGPU_ECC_COUNTER_INIT_PER_TPC(
			sm_cbu_ecc_uncorrected_err_count);
	if (err != 0) {
		goto done;
	}

	err = NVGPU_ECC_COUNTER_INIT_PER_TPC(
			sm_l1_data_ecc_corrected_err_count);
	if (err != 0) {
		goto done;
	}
	err = NVGPU_ECC_COUNTER_INIT_PER_TPC(
			sm_l1_data_ecc_uncorrected_err_count);
	if (err != 0) {
		goto done;
	}

	err = NVGPU_ECC_COUNTER_INIT_PER_TPC(
			sm_icache_ecc_corrected_err_count);
	if (err != 0) {
		goto done;
	}
	err = NVGPU_ECC_COUNTER_INIT_PER_TPC(
			sm_icache_ecc_uncorrected_err_count);
	if (err != 0) {
		goto done;
	}

	err = NVGPU_ECC_COUNTER_INIT_PER_GPC(
			gcc_l15_ecc_corrected_err_count);
	if (err != 0) {
		goto done;
	}
	err = NVGPU_ECC_COUNTER_INIT_PER_GPC(
			gcc_l15_ecc_uncorrected_err_count);
	if (err != 0) {
		goto done;
	}

	err = NVGPU_ECC_COUNTER_INIT_PER_LTS(ecc_sec_count);
	if (err != 0) {
		goto done;
	}
	err = NVGPU_ECC_COUNTER_INIT_PER_LTS(ecc_ded_count);
	if (err != 0) {
		goto done;
	}

	err = NVGPU_ECC_COUNTER_INIT_GR(fecs_ecc_uncorrected_err_count);
	if (err != 0) {
		goto done;
	}
	err = NVGPU_ECC_COUNTER_INIT_GR(fecs_ecc_corrected_err_count);
	if (err != 0) {
		goto done;
	}

	err = NVGPU_ECC_COUNTER_INIT_PER_GPC(
			gpccs_ecc_uncorrected_err_count);
	if (err != 0) {
		goto done;
	}
	err = NVGPU_ECC_COUNTER_INIT_PER_GPC(
			gpccs_ecc_corrected_err_count);
	if (err != 0) {
		goto done;
	}

	err = NVGPU_ECC_COUNTER_INIT_PER_GPC(
			mmu_l1tlb_ecc_uncorrected_err_count);
	if (err != 0) {
		goto done;
	}
	err = NVGPU_ECC_COUNTER_INIT_PER_GPC(
			mmu_l1tlb_ecc_corrected_err_count);
	if (err != 0) {
		goto done;
	}

	err = NVGPU_ECC_COUNTER_INIT_FB(mmu_l2tlb_ecc_uncorrected_err_count);
	if (err != 0) {
		goto done;
	}
	err = NVGPU_ECC_COUNTER_INIT_FB(mmu_l2tlb_ecc_corrected_err_count);
	if (err != 0) {
		goto done;
	}

	err = NVGPU_ECC_COUNTER_INIT_FB(mmu_hubtlb_ecc_uncorrected_err_count);
	if (err != 0) {
		goto done;
	}
	err = NVGPU_ECC_COUNTER_INIT_FB(mmu_hubtlb_ecc_corrected_err_count);
	if (err != 0) {
		goto done;
	}

	err = NVGPU_ECC_COUNTER_INIT_FB(
			mmu_fillunit_ecc_uncorrected_err_count);
	if (err != 0) {
		goto done;
	}
	err = NVGPU_ECC_COUNTER_INIT_FB(
			mmu_fillunit_ecc_corrected_err_count);
	if (err != 0) {
		goto done;
	}

	err = NVGPU_ECC_COUNTER_INIT_PMU(pmu_ecc_uncorrected_err_count);
	if (err != 0) {
		goto done;
	}
	err = NVGPU_ECC_COUNTER_INIT_PMU(pmu_ecc_corrected_err_count);
	if (err != 0) {
		goto done;
	}

done:
	if (err != 0) {
		nvgpu_err(g, "ecc counter allocate failed, err=%d", err);
		nvgpu_ecc_free(g);
	}

	return err;
}
